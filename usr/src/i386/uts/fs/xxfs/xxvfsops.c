#ident	"@(#)kern-i386:fs/xxfs/xxvfsops.c	1.3.5.1"

#include <fs/fski.h>

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/xxfs/xxdata.h>
#include <fs/xxfs/xxfilsys.h>
#include <fs/xxfs/xxino.h>
#include <fs/xxfs/xxinode.h>
#include <fs/xxfs/xxmacros.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
#include <fs/specfs/specfs.h>

extern long int xx_ninode;
extern inode_t *xxinode;

extern	int	dnlc_purge_vfsp(vfs_t *, int);
extern	int	xx_flushi(int);
extern	int	xx_iflush(vfs_t *, int);
extern	int	xx_iget(vfs_t *, int, int, inode_t **);
extern	int	xx_igrab(inode_t *);
extern	int	xx_syncip(inode_t *, int);
extern	void	xx_idrop(inode_t *);
extern	void	xx_inull(vfs_t *);
extern	void	xx_iput(inode_t *);
extern	void	xx_iupdat(inode_t *);

/*
 * UNIX file system VFS operations vector.
 */
STATIC int xx_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int xx_unmount(vfs_t *, cred_t *);
STATIC int xx_root(vfs_t *, vnode_t **);
STATIC int xx_statvfs(vfs_t *, statvfs_t *);
STATIC int xx_sync(vfs_t *, int, cred_t *);
STATIC int xx_vget(vfs_t *, vnode_t **vpp, fid_t *);
STATIC int xx_mountroot(vfs_t *, whymountroot_t);

vfsops_t xx_vfsops = {
	xx_mount,
	xx_unmount,
	xx_root,
	xx_statvfs,
	xx_sync,
	xx_vget,
	xx_mountroot,
	(int (*)())fs_nosys	/* setceiling */
};

/*
 * int
 * xx_fs_init(xx_fs_t *xxfsp, int bsize)
 *	Initialize the xxfs structure.
 *
 * Calling/Exit State:
 *	Should only be called when initializing XENIX.
 */
STATIC int
xx_fs_init(xx_fs_t *xxfsp, int bsize)
{
	int	i;

	for (i = bsize, xxfsp->fs_bshift = 0; i > 1; i >>= 1) {
		xxfsp->fs_bshift++;
	}
	xxfsp->fs_nindir = bsize / sizeof(daddr_t);
	xxfsp->fs_inopb = bsize / sizeof(xxdinode_t);
	xxfsp->fs_bsize = bsize;
	xxfsp->fs_bmask = ~(bsize - 1);
	xxfsp->fs_nmask = xxfsp->fs_nindir - 1;
	for (i = bsize/512, xxfsp->fs_ltop = 0; i > 1; i >>= 1) {
		xxfsp->fs_ltop++;
	}
	for (i = xxfsp->fs_nindir, xxfsp->fs_nshift = 0; i > 1; i >>= 1) {
		xxfsp->fs_nshift++;
	}
	for (i = xxfsp->fs_inopb, xxfsp->fs_inoshift = 0; i > 1; i >>= 1) {
		xxfsp->fs_inoshift++;
	}
	return (0);
}

/*
 * int
 * xx_mountfs(vfs_t *vfsp, whymountroot_t why, dev_t dev,
 *	      cred_t *cr, int isroot)
 *	Mount a XENIX filesystem.
 *
 * Calling/Exit State:
 *	May be called from various situations: calling context is
 *	given in <why> and <vfsp->vfs_flag>.
 *
 *	If <why> is ROOT_INIT, then this is a 'normal' mount request,
 *	i.e., not a re-mount operation.
 *
 *	If <vfsp->vfs_flag & VFS_REMOUNT> is set, then we're
 *	going to transition the file system from Read-Only to
 *	Read-Write.
 *
 * Description:
 *	For non-remount mounts we must construct an snode for the
 *	device being mounted and open the device before attempting
 *	the actual mount.
 *
 *	For remounts we must wait for any possible fsck I/O rundown
 *	before we transition to remounting. Note that remounts only
 *	work reliable for mounting the root of the *system* since
 *	after bdwait() we don't do anything to prevent I/O from
 *	occuring. The controlled environment of mounting the system's
 *	root file system prevents I/O from occuring after bdwait()
 *	but for non-root file systems we don't have that luxury.
 */
/* ARGSUSED */
xx_mountfs(vfs_t *vfsp, whymountroot_t why, dev_t dev, cred_t *cr,
	   int isroot)
{
	buf_t	 *bp;
	inode_t	 *rip;
	int	 error;
	int	 rdonly;
	int	 remount;
	xx_fs_t	 *xxfsp;
	vnode_t	 *devvp;
	vnode_t	 *rvp;
	xxfilsys_t *fsp;
	xxfilsys_t *oldfsp;

	devvp = NULLVP;
	oldfsp = (xxfilsys_t *)NULL;
	bp = (buf_t *)NULL;
	xxfsp = (xx_fs_t *)NULL;
	rdonly = (vfsp->vfs_flag & VFS_RDONLY);
	remount = (vfsp->vfs_flag & VFS_REMOUNT);

	if (why == ROOT_INIT) {
		/*
		 * Open the device.
		 */
		devvp = (vnode_t *) makespecvp(dev, VBLK);
		error = VOP_OPEN(&devvp, rdonly ? FREAD : FREAD|FWRITE, cr);
		if (error) {
			VN_RELE(devvp);
			return (error);
		}
		/* 
		 * record the real device for remount cases.
		 */
		dev = devvp->v_rdev;
		/*
		 * Allocate VFS private data.
		 */
		if ((xxfsp = (xx_fs_t *)
		     kmem_alloc(sizeof(xx_fs_t), KM_SLEEP)) == NULL) {
			error = ENOMEM;
			goto closeout;
		}
		vfsp->vfs_bcount = 0;
		vfsp->vfs_data = (caddr_t) xxfsp;
		vfsp->vfs_op_real = &xx_vfsops;
		vfsp->vfs_dev = dev;
		vfsp->vfs_fsid.val[0] = dev;
		vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
		SLEEP_INIT(&xxfsp->fs_sblock,
			(uchar_t) 0, &xx_sblock_lkinfo, KM_SLEEP);
		SLEEP_INIT(&xxfsp->fs_renamelock,
			(uchar_t) 0, &xx_renamelock_lkinfo, KM_SLEEP);
		xxfsp->fs_bufp = geteblk();
		xxfsp->fs_devvp = devvp;
	}

	/*
	 * get dev for already mounted file system since
	 * remount from xx_mountroot is called with rootdev which
	 * sometimes is not the real device.
	 */
	if (remount) {
		xxfsp = (xx_fs_t *)vfsp->vfs_data;
		devvp = xxfsp->fs_devvp;
		dev = devvp->v_rdev;
	}
	ASSERT(devvp != 0);

	if (why == ROOT_REMOUNT) {
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
		error = xx_iflush(vfsp, 1);
		SLEEP_UNLOCK(&vfslist_lock);
		if (error) {
			return (EBUSY);
		}
	}

	/*
	 * Read the superblock.  We do this in the remount case as well
	 * because it might have changed (if fsck was applied, for example).
	 *
	 */
	bp = getblk(dev, XXSUPERB, XXSBSIZE, BG_NOMISS);
	if (bp) {
		bp->b_flags |= B_STALE|B_AGE;
		brelse(bp);
	}
	bp = bread(dev, XXSUPERB, XXSBSIZE);
	if (bp->b_flags & B_ERROR) {
		error = geterror(bp);
		brelse(bp);
		goto out;
	}

	fsp = getfs(vfsp);
	if (why == ROOT_REMOUNT) {
		/*
		 * Save the contents of the in-core superblock so it
		 * can be restored if the mount fails.
		 */
		oldfsp = (xxfilsys_t *)kmem_alloc(XXSBSIZE, KM_SLEEP);
		bcopy((caddr_t)fsp, (caddr_t)oldfsp, XXSBSIZE);
	}
	bcopy((caddr_t)bp->b_un.b_addr, (caddr_t)fsp, XXSBSIZE);

	if (fsp->s_magic != S_S3MAGIC) {
		brelse(bp);
		error = EINVAL;
		goto out;
	}

	fsp->s_ilock = 0;
	fsp->s_flock = 0;
	fsp->s_ninode = 0;
	fsp->s_inode[0] = 0;

	if (rdonly) {
		ASSERT((vfsp->vfs_flag & VFS_REMOUNT) == 0);
		fsp->s_fmod = 0;
		fsp->s_ronly = 1;
		brelse(bp);
	} else {
		if (remount) {
			if (fsp->s_clean != S_CLEAN) {
				error = EINVAL;
				brelse(bp);
				goto out;
			}
		}
		if (fsp->s_clean == S_CLEAN) {
			fsp->s_clean = 0;	/* unclean */
			bcopy((caddr_t)fsp, bp->b_un.b_addr, XXSBSIZE);
			bwrite(bp);
		} else {
			brelse(bp);
			error = ENOSPC;
			goto out;
		}
		fsp->s_fmod = 1;
		fsp->s_ronly = 0;
	}

	/*
	 * XENIX file systems only support 1024 byte blocks.
	 */
	vfsp->vfs_bsize = XXBSIZE;

	xx_fs_init(xxfsp, vfsp->vfs_bsize);
	if (why == ROOT_INIT) {
		error = xx_iget(vfsp, XXROOTINO, 1, &rip);
		if (error) {
			goto out;
		}
		rvp = ITOV(rip);
		rvp->v_flag |= VROOT;
		xxfsp->fs_root = rvp;
		XX_IRWLOCK_UNLOCK(rip);
	}

	return (0);

out:
	/* 
	 * Clean up on error.
	 */
	if (oldfsp) {
		bcopy((caddr_t)oldfsp, (caddr_t)oldfsp, XXSBSIZE);
		kmem_free((caddr_t)oldfsp, XXSBSIZE);
	}
	if (why == ROOT_REMOUNT) {
		return (error);
	}
	brelse(xxfsp->fs_bufp);

	ASSERT(error);
	SLEEP_DEINIT(&xxfsp->fs_sblock);
	SLEEP_DEINIT(&xxfsp->fs_renamelock);
	kmem_free((caddr_t) xxfsp, sizeof(xx_fs_t));
closeout:
	(void) VOP_CLOSE(devvp, rdonly ? FREAD : FREAD|FWRITE, 1, 0, cr);
	VN_RELE(devvp);
	return (error);
}

/*
 * int
 * xx_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
 *	Do fs specific portion of mount.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *	locked at exit. Holding this lock prevents new lookups into the
 *	file system the mount point is in (see the lookup code for more).
 *
 * Description:
 *	We insure the moint point is 'mountable', i.e., is a directory
 *	that is neither currently mounted on or referenced, and that
 *	the file system to mount is OK (block special file).
 */
STATIC int
xx_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	dev_t		dev;
	int		error;
	int		remount;
	xx_fs_t		*xxfsp;
	vfs_t		*dvfsp;
	vnode_t		*bvp;
	whymountroot_t	why;

	remount = (uap->flags & MS_REMOUNT);
	if (pm_denied(cr, P_MOUNT)) {
		return (EPERM);
	}
	if (mvp->v_type != VDIR) {
		return (ENOTDIR);
	}
	if (remount == 0 && (mvp->v_count > 1 || (mvp->v_flag & VROOT))) {
		return (EBUSY);
	}

	/*
	 * Resolve path name of special file being mounted.
	 */
	error = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &bvp);
	if (error) {
		return (error);
	}

	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		return (ENOTBLK);
	}

	/* 
	* Find the real device via open().
	*/
	error = VOP_OPEN(&bvp, FREAD, cr);
	if (error) {
		VN_RELE(bvp);
		return (error);
	}
	dev = bvp->v_rdev;
	(void) VOP_CLOSE(bvp, FREAD, 1, 0, cr);
	VN_RELE(bvp);

	/*
	 * Ensure that this device isn't already mounted, unless this is
	 * a remount request.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	dvfsp = vfs_devsearch(dev);
	if (dvfsp != NULL) {
		if (remount) {
			why = ROOT_REMOUNT;
			vfsp->vfs_flag |= VFS_REMOUNT;
		} else {
			SLEEP_UNLOCK(&vfslist_lock);
			return (EBUSY);
		}
	} else {
		if (remount) {
			SLEEP_UNLOCK(&vfslist_lock);
			return (EINVAL);
		}
		why = ROOT_INIT;
	}
	SLEEP_UNLOCK(&vfslist_lock);
	if (uap->flags & MS_RDONLY) {
		vfsp->vfs_flag |= VFS_RDONLY;
	}

	/*
	 * Mount the filesystem.
	 */
	error = xx_mountfs(vfsp, why, dev, cr, 0);
	if (!error && !remount) {
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
		if (vfs_devsearch(dev) != NULL && why != ROOT_REMOUNT){
			/* if lost the race free up the private data */
			xxfsp = XXFS(vfsp);
			SLEEP_DEINIT(&xxfsp->fs_sblock);
			SLEEP_DEINIT(&xxfsp->fs_renamelock);
			kmem_free((caddr_t)xxfsp, sizeof(xx_fs_t));
			error = EBUSY;
		} else
			vfs_add(mvp, vfsp, uap->flags);
		SLEEP_UNLOCK(&vfslist_lock);
	}
	return (error);
}

/*
 * int
 * xx_unmount(vfs_t *vfsp, cred_t *cr)
 *	Do the fs specific portion of the unmount.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive
 *	on entry and remains locked at exit.
 *
 * Description:
 *	Flushes inodes while holding the vfs list locked. The
 *	root inode and in-core superblock are sync'ed back to disk.
 */
STATIC int
xx_unmount(vfs_t *vfsp, cred_t *cr)
{
	buf_t	 *bp;
	dev_t	 dev;
	inode_t	 *rip;
	int	 error;
	int	 flag;
	pl_t	 s;
	xx_fs_t	 *xxfsp;
	vnode_t	 *bvp;
	vnode_t	 *rvp;
	xxfilsys_t *fp;

	ASSERT(vfsp->vfs_op_real == &xx_vfsops);

	if (pm_denied(cr, P_MOUNT)) {
		return (EPERM);
	}
	xxfsp = XXFS(vfsp);
	dev = vfsp->vfs_dev;

	/*
	 * Grab vfs list lock to prevent NFS from
	 * establishing a new reference to it via fhtovp.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	/*
	 * If NFS wins the race, fail the unmount.
	 */
	s = LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	if (vfsp->vfs_count != 0) {
		UNLOCK(&vfsp->vfs_mutex, s);
		SLEEP_UNLOCK(&vfslist_lock);
		return (EBUSY);
	}
	UNLOCK(&vfsp->vfs_mutex, s);

	/*
	 * dnlc_purge moved here from upper level. It is done after the
	 * vfslist is locked because only then can we be sure that there
	 * will be no more cache entries established via vget by NFS.
	 */
	dnlc_purge_vfsp(vfsp, 0);

	error = xx_iflush(vfsp, 0);
	if (error < 0) {
		SLEEP_UNLOCK(&vfslist_lock);
		return (EBUSY);
	}

	/*
	 * Mark inode as stale.
	 */
	xx_inull(vfsp);

	/*
	 * Flush root inode to disk.
	 */
	rvp = xxfsp->fs_root;
	ASSERT(rvp != NULL);
	rip = VTOI(rvp);
	XX_IRWLOCK_WRLOCK(rip);
	xx_syncip(rip, B_INVAL);
	/*
	 * At this point there should be no active files on the
	 * file system, and the super block should not be locked.
	 * Break the connections.
	 */
	bvp = xxfsp->fs_devvp;
	fp = getfs(vfsp);
	if (!fp->s_ronly) {
		bflush(dev);
		SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
		fp->s_time = hrestime.tv_sec;
		fp->s_clean = S_CLEAN;
		SLEEP_UNLOCK(&xxfsp->fs_sblock);
		bp = getblk(dev, XXSUPERB, XXSBSIZE, BG_NOMISS);
		if (bp) {
			bp->b_flags |= B_STALE|B_AGE;
			brelse(bp);
		}
		bp = bread(dev, XXSUPERB, XXSBSIZE);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			XX_IRWLOCK_UNLOCK(rip);
			SLEEP_UNLOCK(&vfslist_lock);
			return (error);
		}
		bcopy((caddr_t)fp, (caddr_t)bp->b_un.b_addr, XXSBSIZE);
		bwrite(bp);
	}

	flag = !fp->s_ronly;

	/*
	 * Remove vfs from vfs list.
	 */
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);
	error = VOP_CLOSE(bvp, (vfsp->vfs_flag & VFS_RDONLY) ? FREAD :
			  FREAD|FWRITE, 1, (off_t) 0, cr);
	if (error) {
		XX_IRWLOCK_UNLOCK(rip);
		return (error);
	}
	VN_RELE(bvp);
	rvp->v_count = 1;
	xx_iput(rip);
	xx_iunhash(rip);
	bflush(dev);
	binval(dev);
	brelse(xxfsp->fs_bufp);
	rvp->v_vfsp = 0;
	SLEEP_DEINIT(&xxfsp->fs_sblock);
	SLEEP_DEINIT(&xxfsp->fs_renamelock);
	kmem_free((caddr_t)xxfsp, sizeof(xx_fs_t));
	/*
	 * If not mounted read only then call bdwait()
	 * to wait for async i/o to complete.
	 */
	if (flag) {
		bdwait(dev);
	}
	return (0);
}

/*
 * int
 * xx_root(vfs_t *vfsp, vnode_t **vpp)
 *
 * Calling/Exit State:
 *
 */
STATIC int
xx_root(vfs_t *vfsp, vnode_t **vpp)
{
	xx_fs_t	*xxfsp;
	vnode_t	*vp;

	xxfsp = XXFS(vfsp);
	vp = xxfsp->fs_root;
	VN_HOLD(vp);
	*vpp = vp;
	return (0);
}

/*
 * int
 * xx_statvfs(vfs_t *vfsp, statvfs_t *sp)
 *	Return file system specifics for a given XENIX file system.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
STATIC int
xx_statvfs(vfs_t *vfsp, statvfs_t *sp)
{
	char	 *cp;
	int	 i;
	xx_fs_t	 *xxfsp;
	xxfilsys_t *fp;

	xxfsp = XXFS(vfsp);
	fp = getfs(vfsp);
	if (fp->s_magic != S_S3MAGIC) {
		return (EINVAL);
	}

	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize = sp->f_frsize = vfsp->vfs_bsize;
	sp->f_blocks = fp->s_fsize;
	sp->f_files = (fp->s_isize - 2) * xxfsp->fs_inopb;
	SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
	sp->f_bfree = sp->f_bavail = fp->s_tfree;
	sp->f_ffree = sp->f_favail = fp->s_tinode;
	SLEEP_UNLOCK(&xxfsp->fs_sblock);
	sp->f_fsid = vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = XXDIRSIZ;
	cp = &sp->f_fstr[0];
	for (i=0; i < sizeof(fp->s_fname) && fp->s_fname[i] != '\0'; i++,cp++)
		*cp = fp->s_fname[i];
	*cp++ = '\0';
	for (i=0; i < sizeof(fp->s_fpack) && fp->s_fpack[i] != '\0'; i++,cp++)
		*cp = fp->s_fpack[i];
	*cp = '\0';

	return (0);
}

/*
 * void
 * xx_flushb(vfs_t *vfsp)
 *	Update the superblock.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
STATIC void
xx_flushsb(vfs_t *vfsp)
{
	buf_t	 *bp;
	xx_fs_t	 *xxfsp;
	vnode_t	 *vp;
	xxfilsys_t *fp;

	fp = getfs(vfsp);
	xxfsp = XXFS(vfsp);
	SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
	if (fp->s_fmod == 0 || fp->s_ronly != 0) {
		SLEEP_UNLOCK(&xxfsp->fs_sblock);
		return;
	}
	fp->s_fmod = 0;
	fp->s_time = hrestime.tv_sec;
	SLEEP_UNLOCK(&xxfsp->fs_sblock);
	vp = XXFS(vfsp)->fs_devvp;
	VN_HOLD(vp);
	bp = getblk(vfsp->vfs_dev, XXSUPERB, XXSBSIZE, BG_NOMISS);
	if (bp) {
		bp->b_flags |= B_STALE|B_AGE;
		brelse(bp);
	}
	bp = bread(vfsp->vfs_dev, XXSUPERB, XXSBSIZE);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
	} else {
		bcopy((caddr_t)fp, bp->b_un.b_addr, XXSBSIZE);
		bwrite(bp);
	}
	VN_RELE(vp);
}

/*
 * void
 * xx_update()
 *	Performs the XENIX component of 'sync'.
 *
 * Calling/Exit State:
 *	Called from xx_sync(). No locks are held on entry or exit.
 *
 * Description:
 *	We go through the disk queues to initiate sandbagged I/O. It
 *	goes through the list of inodes and writes all modified ones.
 *	Modified superblocks of XENIX file systems are written to disk.
 */
STATIC void
xx_update()
{
	vfs_t	*vfsp;

	/*
	 * Avoid performing a sync if there's one in progress
	 */
	if (SLEEP_TRYLOCK(&xx_updlock) != B_TRUE) {
		return;
	}
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next) {
		if (vfsp->vfs_op_real == &xx_vfsops) {
			xx_flushsb(vfsp);
		}
	}
	SLEEP_UNLOCK(&vfslist_lock);
	SLEEP_UNLOCK(&xx_updlock);

	xx_flushi(0);
	bflush(NODEV);	/* XXX */
}

/*
 * int
 * xx_flushi(int flag)
 *	Each modified inode is written to disk.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	We traverse the inode table looking for idle, modified inodes.
 *	If we find one, we try to grab it. If we do grab it, we flush
 *	the inode to disk (iff flag & SYNC_ATTR).
 */
int
xx_flushi(int flag)
{
	inode_t	*iend;
	inode_t	*ip;
	int	cheap;

	cheap = flag & SYNC_ATTR;
	for (ip = &xxinode[0], iend = &xxinode[xx_ninode]; ip < iend; ip++) {
		if ((ip->i_flag & (IACC|IUPD|ICHG)) == 0 ||	/* not mod'd */
			!XX_IRWLOCK_IDLE(ip)) {		/* or locked */
				continue;			/* ignore */
		}
		if (xx_igrab(ip)) {
			/*
			 * The inode is locked exclusively at this point.
			 * If this is an inode sync for file system
			 * hardening, make sure the inode is up to date.
			 * In other cases, push everything out.
			 */
			if (cheap) {
				xx_iupdat(ip);
			} else {
				(void) xx_syncip(ip, B_ASYNC);
			}
			xx_iput(ip);
		}
	}
	return (0);
}

/* 
 * int
 * xx_sync(vfs_t *vfsp, int flag, cred_t *cr)
 *	Flush the inode using the given flags, then force inode
 *	information to be written back using the given flags.
 *
 * Calling/Exit State:
 *	The inode's rwlock lock must be held on entry and exit.
 * 
 * Remarks:
 *	The xx_update() function only flush XENIX files.
 */
/* ARGSUSED */
STATIC int
xx_sync(vfs_t *vfsp, int flag, cred_t *cr)
{
	if (flag & SYNC_ATTR) {
		xx_flushi(SYNC_ATTR);
	} else {
		xx_update();
	}
	return (0);
}

/*
 * int
 * xx_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
 *	Given a file identifier, return
 *	a vnode for the file if possible.
 *
 * Calling/Exit State:
 *	The file system that we're going to retrieve the inode
 *	from is protected against unmount by getvfs() -- see vfs.c.
 */
STATIC int
xx_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
{
	inode_t	*ip;
	ufid_t	*ufid;

	/* LINTED pointer alignment */
	ufid = (ufid_t *) fidp;
	if (xx_iget(vfsp, ufid->ufid_ino, 0, &ip)) {
		*vpp = NULL;
		return (0);
	}
	if (ip->i_gen != ufid->ufid_gen) {
		xx_idrop(ip);
		*vpp = NULL;
		return (0);
	}
	XX_IRWLOCK_UNLOCK(ip);
	*vpp = ITOV(ip);
	return (0);
}

/*
 * int
 * xx_mountroot(vfs_t *vfsp, whymountroot_t why)
 *	Mount a XENIX file system as root.
 *
 * Calling/Exit State:
 *	The global vfslist_lock is locked on entry and exit.
 *
 * Description:
 *	"why" is ROOT_INIT on initial call, ROOT_REMOUNT if called to
 *	remount the root file system, and ROOT_UNMOUNT if called to
 *	unmount the root (e.g., as part of a system shutdown).
 */
/* ARGSUSED */
STATIC int
xx_mountroot(vfs_t *vfsp, whymountroot_t why)
{
	buf_t	 *bp;
	int	 error;
	vnode_t	 *vp;
	xxfilsys_t *fp;

	switch (why) {
	case ROOT_INIT:
		if (rootdev == (dev_t)NODEV) {
			  return (ENODEV);
		}
		vfsp->vfs_flag |= VFS_RDONLY;
		break;

	case ROOT_REMOUNT:
		fp = getfs(vfsp);
		if (fp->s_clean != S_CLEAN) {
			return (EINVAL);
		}
		vfsp->vfs_flag |= VFS_REMOUNT;
		vfsp->vfs_flag &= ~VFS_RDONLY;
		break;

	case ROOT_UNMOUNT:
		xx_update();
		fp = getfs(vfsp);
		if (fp->s_clean != S_CLEAN) {
			fp->s_time = hrestime.tv_sec;
			fp->s_clean = S_CLEAN;
			vp = XXFS(vfsp)->fs_devvp;
			bp = getblk(vfsp->vfs_dev, XXSUPERB, XXSBSIZE,
				    BG_NOMISS);
			if (bp) {
				bp->b_flags |= B_STALE|B_AGE;
				brelse(bp);
			}
			bp = bread(vfsp->vfs_dev, XXSUPERB, XXSBSIZE);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				error = geterror(bp);
				return (error);
			}
			bcopy((caddr_t)fp, bp->b_un.b_addr, XXSBSIZE);
			bwrite(bp);
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1, (off_t)0, CRED());
			VN_RELE(vp);
		}
		bdwait(NODEV);
		return (0);

	default:
		return (EINVAL);
	}
	
	error = xx_mountfs(vfsp, why, rootdev, CRED(), 1);
	if (error) {
		return (error);
	}
	/*
	 * The routine is called at system boot and at this time
	 * it still in UP state and no one has the access to the fs
	 * therefore no need to lock vfslist when adding the vfs.
	 */
	vfs_add(NULLVP, vfsp, 0);
	fp = getfs(vfsp);
	return (error);
}

/*
 *
 *  Adapted for System V Release 4	(ESIX 4.0.4)	
 *
 *  Gerard van Dorth	(gdorth@nl.oracle.com)
 *  Paul Bauwens	(paul@pphbau.atr.bso.nl)
 *
 *  May 1993
 *
 *  Originally written by Paul Popelka (paulp@uts.amdahl.com)
 *
 *  You can do anything you want with this software,
 *    just don't say you wrote it,
 *    and don't remove this notice.
 *
 *  This software is provided "as is".
 *
 *  The author supplies this software to be publicly
 *  redistributed on the understanding that the author
 *  is not responsible for the correct functioning of
 *  this software in any circumstances and is not liable
 *  for any damages caused by this software.
 *
 *  October 1992
 *
 */

#ident	"@(#)kern-i386:fs/dosfs/dosfs_vfsops.c	1.9.6.1"

#include "dosfs.h"
#include <io/target/sd01/fdisk.h>

STATIC int dosfs_mount(struct vfs *vfsp, struct vnode *mvp, struct mounta *uap, struct cred *cr);
STATIC int dosfs_root(struct vfs *vfsp, struct vnode **vpp);
STATIC int dosfs_statvfs(struct vfs *vfsp, struct statvfs *sp);
STATIC int dosfs_sync (struct vfs *vfsp, int flag, struct cred *cr);
STATIC int dosfs_vget(struct vfs *vfsp, struct vnode **vpp, struct fid *fidp);
STATIC int dosfs_unmount(struct vfs *vfsp, struct cred *cr);
STATIC int dosfs_mountroot(struct vfs *vfsp, enum whymountroot why);

static int mountdosfs(vfs_t *vfsp, dev_t dev, char *path, cred_t *cr);
static void	find_extdosfs(char *, long *);


STATIC int
dosfs_mount (vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	dev_t	  	  dev;
	struct pathname	  dpn;	/* directory point to mount */
	struct vnode	  *bvp;	/* vnode for blk device to mount */
	struct vfs	  *found;
	dosfsmnt_args_t	  args;	/* arguments for this mount call */
	struct dosfs_vfs*  pvp;
	int		  error;

	/*
	 * Trivial checks first:
	 *	- mount only allowed on directories
	 *	- only super user can mount
	 *	- mount on a not already mounted place
	 */

	if (mvp->v_type != VDIR)
	{
		return (ENOTDIR);
	}

	if (pm_denied(cr, P_MOUNT))
	{
		return (EPERM);
	}

	if ((uap->flags & MS_REMOUNT) == 0 &&
	    (mvp->v_count != 1 || (mvp->v_flag & VROOT)))
	{
		return (EBUSY);
	}


	/*
	Get the mount directory; we need it for statvfs
	to display the pathname we're mounted on.
	*/

	if (error = pn_get(uap->dir, UIO_USERSPACE, &dpn))
	{
		return (error);
	}

	/*
	Get the path of the special file being mounted;
	it should be a block device. Pick-up the device
	type.
	*/

	if (error = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &bvp))
	{
		pn_free(&dpn);
		return (error);
	}

	if (bvp->v_type != VBLK)
	{
		VN_RELE(bvp);
		pn_free(&dpn);
		return (ENOTBLK);
	}

	/* 
	* Find the real device via open().
	*/
	error = VOP_OPEN(&bvp, FREAD, cr);
	if (error) {
		VN_RELE(bvp);
		pn_free(&dpn);
		return (error);
	}
	dev = bvp->v_rdev;
	(void) VOP_CLOSE(bvp, FREAD, 1, 0, cr);
	VN_RELE(bvp);

	/*
	Copy the readonly flag.
	*/

	if (uap->flags & MS_RDONLY)
	{
		vfsp->vfs_flag |= VFS_RDONLY;
	}

	/*
	A device can only be mounted once; however one
	is able to remount it - in which case we need
	to find the device on the same mountpoint.
	Note we expect the same filesystem on a remount
	so no actions are performed, just some checks.
	*/
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	found = vfs_devsearch(dev);

	if (uap->flags & MS_REMOUNT)
	{
		pn_free(&dpn);

		if (found != vfsp)
		{
			SLEEP_UNLOCK(&vfslist_lock);
			return EINVAL;
		}
		SLEEP_UNLOCK(&vfslist_lock);
		return 0;
	}
	else if (found != NULL)
	{
		SLEEP_UNLOCK(&vfslist_lock);
		pn_free(&dpn);
		return EBUSY;
	}
	SLEEP_UNLOCK(&vfslist_lock);

	/*
	Well, it's not an update, it's a real mount request.
	Time to get dirty.
	*/

	error = mountdosfs (vfsp, dev, dpn.pn_path, cr);

	pn_free(&dpn);

	if (error)
	{
		return error;
	}

	/*
	Copy in the args for the mount request,
	if there are some. The following actions
	have been defined:
		- set uid
		- set gid
		- set mode mask
	*/

	pvp = DOSFS_VFS (vfsp);

	if (uap->dataptr)
	{
		error = copyin (uap->dataptr, (caddr_t)&args, sizeof args);

		if (error)
		{
			return error;
		}

		if (args.uid >= MAXUID || args.uid < DEFUSR ||
		    args.gid >= MAXUID || args.gid < DEFGRP   )
		{
			return EINVAL;
		}

		pvp->vfs_uid  = args.uid;
		pvp->vfs_gid  = args.gid;
		pvp->vfs_mask = args.mask & 0777;
	}
	else
	{
		pvp->vfs_uid  = DEFUSR;
		pvp->vfs_gid  = DEFGRP;
		pvp->vfs_mask = 0;
	}

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	vfs_add(mvp, vfsp, uap->flags);
	SLEEP_UNLOCK(&vfslist_lock);

	return error;
}


STATIC int
mountdosfs (struct vfs *vfsp, dev_t dev, char *path, struct cred *cr)
{
	int			i;
	int			bpc;
	int			bit;
	int			error;
	int			needclose = 0;
	int			ronly;
	union bootsector	*bsp;
	lbuf_t			*bp0;
	struct byte_bpb33	*b33;
	struct byte_bpb50	*b50;
	struct vnode		*devvp;
	struct dosfs_vfs*	pvp = NULL;	/* private dosfs info */
	struct denode*		ndep;
	struct vnode*		vndep;
	long			dos_offset = 0;
	u_short			jump_inst;

	/*
	Get the device vnode and check whether it's
	not used as swap device.
	*/

	devvp = makespecvp (dev, VBLK);
	ronly = (vfsp->vfs_flag & VFS_RDONLY) != 0;

	if (error = VOP_OPEN(&devvp, ronly ? FREAD : FREAD|FWRITE, cr))
	{
		return (error);
	}
	/*
	 * record the real device for remount cases.
	 */
	dev = devvp->v_rdev;
	needclose = 1;

	vfsp->vfs_bcount      = 0;
	vfsp->vfs_dev         = dev;
	vfsp->vfs_fsid.val[0] = dev;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;

	/*
	Flush back any dirty pages on the block device to
	try and keep the buffer cache in sync with the page
	cache if someone is trying to use block devices when
	they really should be using the raw device.
	*/

	
	(void) VOP_PUTPAGE(common_specvp(devvp), 0, 0, B_INVAL, cr);
	binval(dev);

	/*
	Read the boot sector of the filesystem, and then
	check the boot signature.  If not a dos boot sector
	then error out.  We could also add some checking on
	the bsOemName field.  So far I've seen the following
	values:
		"IBM  3.3"
		"MSDOS3.3"
		"MSDOS5.0"
	*/

	/*
	 * The DOS boot block can be found in the first block of the
	 * boot partition. This is true iff:
	 *	- the DOS FS is in a floppy diskette
	 *		or 
	 *	- the DOS FS is a PRIMARY DOS partition in the hard disk
	 *
	 * If the DOS file system is an EXTENDED DOS partition in the
	 * hardisk, then the first block contains the relevant information
	 * that can be used to locate the actual begining of the DOS
	 * file system.
	 * 
	 */

	bp0 = lbread(dev, dos_offset, 512);

	if (error = lgeterror(bp0))
		goto error_exit;

	bsp = (union bootsector *)bp0->b_un.b_addr;

	if (bsp->bs50.bsBootSectSig != BOOTSIG) {
		error = EINVAL;
		goto error_exit;
	}

	if (*(bsp->bs50.bsJump) == 0) {
		/*
		 * This may be an extended DOS partition. Find out where
		 * DOS file system really starts!
		 */
		find_extdosfs(bp0->b_un.b_addr, &dos_offset);
		if (dos_offset <= 0) {
			error = EINVAL;
			goto error_exit;
		}

		if (bp0)
			lbrelse(bp0);

		bp0 = lbread(dev, dos_offset, 512);

		if (error = lgeterror(bp0))
			goto error_exit;

		bsp = (union bootsector *)bp0->b_un.b_addr;

		if (bsp->bs50.bsBootSectSig != BOOTSIG) {
			error = EINVAL;
			goto error_exit;
		}
	}

	/*
	 * Accepted DOS file system formats must pass the following test:
	 *
	 * - The first byte of the jump instruction must be either
	 *   0xe9 or 0xeb (or 0x69 to support older Floppy disk formats).
	 *
	 */

	jump_inst = getushort(bsp->bs50.bsJump) & 0x00ff;

	if (jump_inst != 0xe9 && jump_inst != 0xeb && jump_inst != 0x69) {
		error = EINVAL;
		goto error_exit;
	}

	b33 = (struct byte_bpb33 *)bsp->bs33.bsBPB;
	b50 = (struct byte_bpb50 *)bsp->bs50.bsBPB;

	pvp = (struct dosfs_vfs *)kmem_zalloc(sizeof *pvp, KM_SLEEP);

	if (pvp == NULL)
	{
		error = ENOMEM;
		goto error_exit;
	}

	vfsp->vfs_data = (caddr_t) pvp;

	if (ronly)
	{
		vfsp->vfs_flag |= VFS_RDONLY;
	}

	pvp->vfs_vfsp     = vfsp;
	pvp->vfs_inusemap = NULL;

	/*
	Compute several useful quantities from the bpb in
	the bootsector.  Copy in the dos 5 variant of the
	bpb then fix up the fields that are different between
	dos 5 and dos 3.3.
	*/

	pvp->vfs_BytesPerSec  = getushort(b50->bpbBytesPerSec);
	pvp->vfs_SectPerClust = b50->bpbSecPerClust;
	pvp->vfs_ResSectors   = getushort(b50->bpbResSectors);
	pvp->vfs_FATs         = b50->bpbFATs;
	pvp->vfs_RootDirEnts  = getushort(b50->bpbRootDirEnts);
	pvp->vfs_Sectors      = getushort(b50->bpbSectors);
	pvp->vfs_Media        = b50->bpbMedia;
	pvp->vfs_FATsecs      = getushort(b50->bpbFATsecs);
	pvp->vfs_SecPerTrack  = getushort(b50->bpbSecPerTrack);
	pvp->vfs_Heads        = getushort(b50->bpbHeads);

	if (pvp->vfs_Sectors <= 0) {
		pvp->vfs_HugeSectors = getulong(b50->bpbHugeSectors);
		pvp->vfs_HiddenSects = getulong(b50->bpbHiddenSecs);
	} else {
		pvp->vfs_HugeSectors = pvp->vfs_Sectors;
		pvp->vfs_HiddenSects = getushort(b33->bpbHiddenSecs);
	}

	if (pvp->vfs_HugeSectors <= 0) {
		error = EINVAL;
		goto error_exit;
	}


	pvp->vfs_fatblk     = pvp->vfs_ResSectors + dos_offset;
	pvp->vfs_rootdirblk = pvp->vfs_fatblk +
	    (pvp->vfs_FATs * pvp->vfs_FATsecs);

	/*
	 * Size in sectors.
	 */

	pvp->vfs_rootdirsize = (pvp->vfs_RootDirEnts * sizeof(struct direntry))
	    / pvp->vfs_BytesPerSec;
	pvp->vfs_firstcluster = pvp->vfs_rootdirblk + pvp->vfs_rootdirsize;
	pvp->vfs_nmbrofclusters = (pvp->vfs_HugeSectors + dos_offset - 
				   pvp->vfs_firstcluster) /
				  pvp->vfs_SectPerClust;
	pvp->vfs_maxcluster = pvp->vfs_nmbrofclusters + 1;

	/*
	Compute mask and shift value for isolating cluster relative
	byte offsets and cluster numbers from a file offset.
	*/

	bpc                = pvp->vfs_SectPerClust * pvp->vfs_BytesPerSec;
	pvp->vfs_bpcluster = bpc;
	pvp->vfs_depclust  = bpc/sizeof(struct direntry);
	pvp->vfs_crbomask  = bpc - 1;

	if (bpc == 0)
	{
		error = EINVAL;
		goto error_exit;
	}

	for (bit = 1, i = 0; i < 32; i++) {
		if (bit & bpc) {
			if (bit ^ bpc) {
				error = EINVAL;
				goto error_exit;
			}
			pvp->vfs_cnshift = i;
			break;
		}
		bit <<= 1;
	}

	vfsp->vfs_bsize = bpc;

	pvp->vfs_brbomask = 0x01ff; /* 512 byte blocks only (so far) */
	pvp->vfs_bnshift  = 9;	    /* shift right 9 bits to get bn  */

	/*
	Release the bootsector buffer.
	*/

	lbrelse(bp0);
	bp0 = NULL;

	/*
	Allocate memory for the bitmap of allocated clusters,
	and then fill it in.
	*/

	pvp->vfs_inusemap = kmem_alloc((pvp->vfs_maxcluster>>3) + 1, KM_SLEEP);

	if (!pvp->vfs_inusemap)
	{
		error = ENOMEM;
		goto error_exit;
	}

	/*
	fillinusemap() needs vfs_dev.
	*/

	pvp->vfs_devvp = devvp;
	pvp->vfs_dev   = dev;

	/*
	Have the inuse map filled in.
	*/

	error = fillinusemap(pvp);

	if (error)
		goto error_exit;

	pvp->vfs_ronly = ronly;

	if (ronly == 0)
		pvp->vfs_fmod = 1;

	FSPIN_INIT(&pvp->vfs_mutex);
	SLEEP_INIT(&pvp->vfs_rename_lock, (uchar_t) 0, &dosfs_rename_lkinfo,
								KM_SLEEP);

	/*
	Finish up.
	*/

	if (error = deget(vfsp, ATTR_DIRECTORY, 0, 0, DOSFSROOT, 0, &ndep)) {
		goto error_exit1;
	}


	vndep = DETOV(ndep);
	vndep->v_flag |= VROOT;

	pvp->vfs_root  = vndep;

	pvp->path[0] = '\0';
	strncat (pvp->path, path, sizeof(pvp->path));

	DE_UNLOCK (ndep);

	return 0;

error_exit1:
	SLEEP_DEINIT(&pvp->vfs_rename_lock);

error_exit:
	;
	if (bp0)
	{
		lbrelse(bp0);
	}

	if (needclose)
	{
		(void) VOP_CLOSE(devvp, ronly ? FREAD : FREAD|FWRITE, 1, 0, cr);
		binval(dev);
	}

	if (pvp)
	{
		if (pvp->vfs_inusemap) {
			kmem_free((caddr_t)pvp->vfs_inusemap,
			    (pvp->vfs_maxcluster >> 3) + 1);
		}

		kmem_free((caddr_t)pvp, sizeof *pvp);
	}

	return error;
}


static void
find_extdosfs(char *bootblock, long *dos_offset)
{
	struct ipart	*ipt;
	int		i;

	for (i = 0; i < FD_NUMPART; i++) {

		ipt = (struct ipart *)((bootblock + BOOTSZ) + 
				       (i * sizeof(struct ipart)));

		if (ipt->numsect > 0) {
			*dos_offset = ipt->relsect;
			break;
		}
	}
}

STATIC int
dosfs_root (struct vfs *vfsp, struct vnode **vpp)
{
	struct denode *ndep;
	int error;

	error = deget(vfsp, ATTR_DIRECTORY, 0, 0, DOSFSROOT, 0, &ndep);

	if (error == 0)
	{
		*vpp = DETOV(ndep);
		DE_UNLOCK (ndep);
	}

	return error;
}


STATIC int
dosfs_statvfs (struct vfs *vfsp, struct statvfs *sp)
{
	struct dosfs_vfs *pvp;

	pvp   = DOSFS_VFS(vfsp);

	sp->f_bsize   = pvp->vfs_bpcluster;
	sp->f_frsize  = pvp->vfs_bpcluster;
	sp->f_blocks  = pvp->vfs_nmbrofclusters;
	sp->f_bfree   = pvp->vfs_freeclustercount;
	sp->f_bavail  = pvp->vfs_freeclustercount;
	sp->f_files   = (fsfilcnt_t)-1;
	sp->f_ffree   = (fsfilcnt_t)-1;		/* what to put here? !0 */
	sp->f_favail  = (fsfilcnt_t)-1;		/* what to put here? !0 */
	sp->f_fsid    = pvp->vfs_dev;
	sp->f_namemax = DOSDIRSIZ;	/* about */

	sp->f_flag    = vfsp->vfs_flag;

	sp->f_fstr[0] = '\0';
	strncat (sp->f_fstr, pvp->path, sizeof(sp->f_fstr));

	strcpy  (sp->f_basetype, "dosfs");

	return 0;
}

/*ARGSUSED*/
STATIC int 
dosfs_sync (struct vfs *vfsp, int flag, struct cred *cr)
{
	static int flushtime = 0;


	if (flag & SYNC_ATTR) {
		if (++flushtime < dosfs_tflush)
			return 0;

		flushtime =0;
	}

	if (!SLEEP_TRYLOCK(&dosfs_updlock))
		return 0;

	dosfs_deflush();

	SLEEP_UNLOCK(&dosfs_updlock);
	return 0;
}


STATIC int
dosfs_vget (struct vfs *vfsp, struct vnode **vpp, struct fid *fidp)
{
	dfid_t*		  dfid;
	denode_t*	  dp;
	int  isadir;

	dfid = (dfid_t *) fidp;

	if (dfid->dfid_diroffset & DOSFS_FID_DIRFLAG)
	{
		isadir	= 1;
	}
	else 
	{
		isadir	= 0;
	}

	if (deget(vfsp, isadir, (u_long)dfid->dfid_dirclust,
	    (u_long)dfid->dfid_diroffset & ~DOSFS_FID_DIRFLAG,
		(u_long)dfid->dfid_startclust, NULL, &dp))
	{
		*vpp = NULL;
		return 0;
	}

	/*
	If the generation number changed
	we know we have an improper file.
	Reject the denode but don't use
	the inactivate path here!
	*/

	if (DE_GEN(dp) != dfid->dfid_gen)
	{
		deput (dp);

		*vpp = NULL;
		return 0;
	}

	DE_UNLOCK(dp);

	*vpp = DETOV (dp);
	return 0;
}



/*
 *  Unmount the filesystem described by mp.
 */
STATIC int
dosfs_unmount (struct vfs *vfsp, struct cred *cr)
{
	int	error;
	dev_t	dev;
	register struct dosfs_vfs *pvp;	/* private dosfs info */
	register struct vnode	 *bvp;	/* blk dev vnode pointer */
	register struct vnode	 *rvp;	/* root vnode pointer */
	struct denode	 *rdp;	/* inode root dir */
	pl_t pl;

	if (pm_denied(cr, P_MOUNT))
	{
		return EPERM;
	}
	/* Grab vfs list lock to prevent NFS establishes
         * a new reference to it via fhtovp.
        */
        SLEEP_LOCK(&vfslist_lock, PRIVFS);
        /* if NFS wins the race fails the unmount */
        pl = LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
        if (vfsp->vfs_count != 0) {
                UNLOCK(&vfsp->vfs_mutex, pl);
                SLEEP_UNLOCK(&vfslist_lock);
                return EBUSY;
        }
        UNLOCK(&vfsp->vfs_mutex, pl);

	dnlc_purge_vfsp(vfsp, 0);

	if (deflush(vfsp,0) < 0 )
	{
		SLEEP_UNLOCK(&vfslist_lock);
		return EBUSY;
	}

	pvp = DOSFS_VFS(vfsp);
	dev = vfsp->vfs_dev;

	/*
	Flush the root inode to disk.
	*/

	rvp = pvp->vfs_root;
	rdp = VTODE(rvp);

	DE_LOCK(rdp);

	/* Remove vfs from vfs list. */
        vfs_remove(vfsp);
        SLEEP_UNLOCK(&vfslist_lock);

	/*
	At this point there should be no active files on the
	file system, and the super block should not be locked.
	Break the connections.
	*/

	if (!(vfsp->vfs_flag & VFS_RDONLY))
	{
		bflush (dev);
		bdwait(dev);
	}

	bvp = pvp->vfs_devvp;

	(void) VOP_PUTPAGE(common_specvp(bvp), 0, 0, B_INVAL, cr);

	if (error = VOP_CLOSE(bvp, (vfsp->vfs_flag & VFS_RDONLY) ?
	    FREAD : FREAD|FWRITE, 1, (off_t) 0, cr))
	{
		DE_UNLOCK(rdp);

		return error;
	}

	VN_LOCK(rvp);
        ASSERT(rvp->v_count == 2);
        rvp->v_count = 1;
	VN_UNLOCK(rvp);

	VN_RELE(bvp);
	binval(dev);

	deput(rdp);
	deunhash(rdp);
	rdp->de_flag = 0;
	rvp->v_vfsp = 0;

	kmem_free((caddr_t)pvp->vfs_inusemap, (pvp->vfs_maxcluster >> 3) + 1);
	kmem_free((caddr_t)pvp, sizeof(struct dosfs_vfs));

	return 0;
}

/*ARGSUSED*/
STATIC int
dosfs_mountroot(struct vfs *vfsp, enum whymountroot why)
{
	/* Fix for DELL kernels that happen to call xxx_mountroot() */
	/* for every filesystem under the sun...					*/ 
	return EINVAL;
}


vfsops_t dosfs_vfsops = {
	        dosfs_mount,
	        dosfs_unmount,
	        dosfs_root,
	        dosfs_statvfs,
	        dosfs_sync,
	        dosfs_vget,
	        dosfs_mountroot,
	        (int (*)())fs_nosys       /* setceiling */
};



#ident	"@(#)kern-i386:fs/xxfs/xxalloc.c	1.2.3.1"

#include <fs/fski.h>

#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/mode.h>
#include <fs/buf.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/xxfs/xxfblk.h>
#include <fs/xxfs/xxfilsys.h>
#include <fs/xxfs/xxino.h>
#include <fs/xxfs/xxinode.h>
#include <fs/xxfs/xxmacros.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

typedef	fblk_t	*FBLKP;

extern	buf_t	*buf_search_hashlist(buf_t *, dev_t, daddr_t);
extern	int	xx_iget(vfs_t *, int, int, inode_t **);
extern	void	xx_iput(inode_t *);
extern	void	xx_iupdat(inode_t *);

/*
 * int
 * xx_badblock(xxfilsys_t *fp, daddr_t bn, dev_t dev)
 *	Check that a specified block number is valid in the
 *	containing file system.
 *
 * Calling/Exit State:
 *	The superblock lock is locked on entry or exit.
 *	If the block number is not valid for the file system,
 *	1 is returned; otherwise 0 is returned.
 *
 * Description:
 *	Check that a block number is in the range between the I list
 *	and the size of the device.  This is used mainly to check that
 *	a garbage file system has not been mounted.
 *
 *	bad block on dev x/y -- not in range
 */
/* ARGSUSED */
int
xx_badblock(xxfilsys_t *fp, daddr_t bn, dev_t dev)
{
	if ((unsigned)bn < (daddr_t)fp->s_isize 
	 || (unsigned)bn >= (daddr_t)fp->s_fsize) {
		/*
		 *+ A out-of-range block number was encountered while the
		 *+ kernel was doing a filesystem block allocation or freeing
		 *+ a filesystem block. This probably indicates a corrupted
		 *+ filesystem. Corrective action: run fsck on the filesystem.
		 */
		cmn_err(CE_WARN, "bad block %d, %s: bad block\n",
			bn, fp->s_fname);
		return (1);
	}
	return (0);
}

/*
 * int
 * xx_blkalloc(vfs_t *vfsp, daddr_t *bnp)
 *	Obtain the next available free disk block from the free list of
 *	the specified device.  The super-block has up to XXNICFREE remembered
 *	free blocks; the last of these is read to obtain XXNICFREE more.
 *
 * Calling/Exit State:
 *	The xx inode rwlock is held on entry and exit.
 *
 *	A return value of 0 indicates success.
 *	Errno returned by this routine:
 *	  ENOSPC	no space left on dev x/y (free list is exhausted).
 *
 * Description:
 *	Superblock sleep lock is held while modifying the superblock info.
 */
int
xx_blkalloc(vfs_t *vfsp, daddr_t *bnp)
{
	dev_t	 dev;
	daddr_t	 bno;
	int	 bsize;
	buf_t	 *bp;
	xx_fs_t	 *xxfsp;
	xxfilsys_t *fp;

	dev = vfsp->vfs_dev;
	bsize = XXBSIZE;
	fp = getfs(vfsp);
	xxfsp = XXFS(vfsp);
	SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
	do {
		if (fp->s_nfree <= 0) {
			goto nospace;
		}
		bno = fp->s_free[--fp->s_nfree];
		if (bno == 0) {
			goto nospace;
		}
	} while (xx_badblock(fp, bno, dev));

	if (fp->s_nfree <= 0) {
		bp = bread(dev, LTOPBLK(bno, bsize), bsize);
		if ((bp->b_flags & B_ERROR) == 0) {
			fp->s_nfree = ((FBLKP)(bp->b_addrp))->df_nfree;
			bcopy((caddr_t)((FBLKP)(bp->b_addrp))->df_free,
			    (caddr_t)fp->s_free, sizeof(fp->s_free));
		}
		bp->b_flags &= ~B_DELWRI;
		bp->b_flags |= (B_STALE|B_AGE);
		brelse(bp);
	}
	if (fp->s_nfree <= 0 || fp->s_nfree > XXNICFREE) {
		goto nospace;
	}
	if (fp->s_tfree) {
		fp->s_tfree--;
	}
	fp->s_fmod = 1;
	SLEEP_UNLOCK(&xxfsp->fs_sblock);
	*bnp = bno;
	return (0);

nospace:
	SLEEP_UNLOCK(&xxfsp->fs_sblock);
	fp->s_nfree = 0;
	fp->s_tfree = 0;
	delay(5*HZ);
	/*
	 *+ Could not allocate a new block on this file system.
	 *+ Corrective action: remove some unused files to free
	 *+ some blocks and try again.
	 */
	cmn_err(CE_NOTE, "File system full, dev = %d\n" , dev);
	return (ENOSPC);
}

/*
 * void
 * xx_blkfree(vfs_t *vfsp, daddr_t bno)
 *	Free a block
 *
 * Calling/Exit State:
 *	The xx inode rwlock is held on entry and exit.
 *
 * Description:
 *	Place the specified disk block back on the free list of the
 *	specified device.
 */
void
xx_blkfree(vfs_t *vfsp, daddr_t bno)
{
	dev_t	 dev;
	int	 bsize;
	buf_t	 *bp;
	buf_t	 *dp;
	daddr_t	 pbno;
	xx_fs_t  *xxfsp;
	xxfilsys_t *fp;

	dev = vfsp->vfs_dev;
	bsize = XXBSIZE;
	fp = getfs(vfsp);
	xxfsp = XXFS(vfsp);
	SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
	fp->s_fmod = 1;
	if (xx_badblock(fp, bno, dev)) {
		SLEEP_UNLOCK(&xxfsp->fs_sblock);
		return;
	}
	if (fp->s_nfree <= 0) {
		fp->s_nfree = 1;
		fp->s_free[0] = 0;
	}
	pbno = LTOPBLK(bno, bsize);
	if (fp->s_nfree >= XXNICFREE) {
		bp = getblk(dev, bno, bsize, 0);
		((FBLKP)(bp->b_addrp))->df_nfree = fp->s_nfree;
		bcopy((caddr_t)fp->s_free,
			(caddr_t)((FBLKP)(bp->b_addrp))->df_free,
			sizeof(fp->s_free));
		fp->s_nfree = 0;
		bdwrite(bp);
	} else {
		/*
		 * There may be a leftover in-core buffer for this block;
		 * if so, make sure it's marked invalid and turn off
		 * B_DELWRI so that it will not subsequently be written
		 * to disk.  Otherwise, if the block is subsequently
		 * allocated as file data, the stale data in the buffer
		 * will be aliasing the data in the cache.
		 */
		if ((bp = getblk(dev, pbno, bsize, BG_NOMISS)) != NULL) { 
			bp->b_flags &= ~B_DELWRI;
			bp->b_flags |= (B_STALE|B_AGE);
			brelse(bp);
		}
	}

	fp->s_free[fp->s_nfree++] = bno;
	fp->s_tfree++;
	fp->s_fmod = 1;
	SLEEP_UNLOCK(&xxfsp->fs_sblock);
}

/*
 * int
 * xx_ialloc(vfs_t *vfsp, ushort_t mode, int nlink, dev_t rdev,
 *	     int uid, int gid, inode_t **ipp)
 *	Allocate an unused inode on the specified device.
 *
 * Calling/Exit State:
 *	On successful returns, <*ipp->i_rwlock> is held exclusive.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *      errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENOSPC	The file system is out of inodes.
 *
 * Description:
 *	Used with file creation.  The algorithm keeps up to XXNICINOD
 *	spare inodes in the super-block.  When this runs out, a linear 
 *	search through the i-list is instituted to pick up XXNICINOD more.
 */
int
xx_ialloc(vfs_t *vfsp, ushort_t mode, int nlink, dev_t rdev,
	  int uid, int gid, inode_t **ipp)
{
	dev_t		dev;
	int		bsize;
	xx_fs_t		*xxfsp;
	xxfilsys_t	*fp;
	vnode_t		*vp;
	inode_t		*ip;
	int		i;
	buf_t		*bp;
	xxdinode_t	*dp;
	ushort_t	ino;
	daddr_t		adr;
	int		error;
	pl_t		s;

	ip = NULL;
	dev = vfsp->vfs_dev;
	bsize = XXBSIZE;
	xxfsp = XXFS(vfsp);
	fp = getfs(vfsp);

	SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
loop:
	if (fp->s_ninode > 0 && (ino = fp->s_inode[--fp->s_ninode])) {
		error = xx_iget(vfsp, ino, 1, ipp);
		if (error) {
			SLEEP_UNLOCK(&xxfsp->fs_sblock);
			return (error);
		}
		/*
		 * (*ipp)->i_rwlock is held *exclusive* here
		 */
		ip = *ipp;
		vp = ITOV(ip);
		if (ip->i_mode == 0) {
			/*
			 * Found inode; update now to avoid races.
			 */
			vtype_t	type;

			vp->v_type = type = IFTOVT((int)mode);
			ip->i_mode = mode;
			ip->i_nlink = (o_nlink_t)nlink;
			ip->i_uid = (o_uid_t)uid;
			ip->i_gid = (o_gid_t)gid;
			ip->i_size = 0;
			s = XX_ILOCK(ip);
			XXIMARK(ip, IACC|IUPD|ICHG|ISYN);
			XX_IUNLOCK(ip, s);
			for (i = 0; i < NADDR; i++) {
				ip->i_addr[i] = 0;
			}
			/*
			 * Must set rdev after address fields are
			 * zeroed because rdev is defined to be the
			 * first address field (xxinode.h).
			 */
			if (type == VCHR || type == VBLK) { 
				ip->i_rdev = rdev;
				/*
				 * update i_addr components
				 */
				ip->i_major = getemajor(rdev);
				ip->i_minor = geteminor(rdev);
				ip->i_bcflag |= NDEVFORMAT;
				/*
				 * To preserve backward compatibility we
				 * store dev in old format if it fits,
				 * otherwise O_NODEV is assigned.
				 */
				if (cmpdev_fits(rdev))
					ip->i_oldrdev = (daddr_t)_cmpdev(rdev);
				else
					ip->i_oldrdev = (daddr_t)O_NODEV;

			} else if (type == VXNAM) {
				/*
				 * XENIX stores semaphore info in rdev.
				 * Need to set rdev to ip->i_oldrdev for
				 * xx_iupdat().
				 */
				ip->i_rdev = rdev;
				ip->i_oldrdev = rdev;
			}

			vp->v_rdev = ip->i_rdev;
			if (fp->s_tinode)
				fp->s_tinode--;
			fp->s_fmod = 1;
			SLEEP_UNLOCK(&xxfsp->fs_sblock);
			xx_iupdat(ip);
			*ipp = ip;
			return (0);
		}
		/*
		 *+ Inode was allocated after all. Search for another one.
		 */
		cmn_err(CE_NOTE, "xx_ialloc: inode was already allocated\n");
		SLEEP_UNLOCK(&xxfsp->fs_sblock);
		xx_iupdat(ip);
		xx_iput(ip);
		SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
		goto loop;
	}
	/*
	 * Only try to rebuild freelist if there are free inodes.
	 */
	if (fp->s_tinode > 0) {
		fp->s_ninode = XXNICINOD;
		ino = FsINOS(xxfsp, fp->s_inode[0]);
		for (adr = FsITOD(xxfsp, ino); adr < (daddr_t)fp->s_isize;
		     adr++) {
			bp = bread(dev, LTOPBLK(adr, bsize), bsize);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				ino += xxfsp->fs_inopb;
				continue;
			}
			dp = (xxdinode_t *)bp->b_addrp;
			for (i = 0; i < xxfsp->fs_inopb; i++, ino++, dp++) {
				if (fp->s_ninode <= 0) {
					break;
				}
				if (dp->di_mode == 0) {
					fp->s_inode[--fp->s_ninode] = ino;
				}
			}
			brelse(bp);
			if (fp->s_ninode <= 0) {
				break;
			}
		}
		if (fp->s_ninode > 0) {
			fp->s_inode[fp->s_ninode-1] = 0;
			fp->s_inode[0] = 0;
		}
		if (fp->s_ninode != XXNICINOD) {
			fp->s_ninode = XXNICINOD;
			goto loop;
		}
	}

	fp->s_ninode = 0;
	fp->s_tinode = 0;
	SLEEP_UNLOCK(&xxfsp->fs_sblock);
	return (ENOSPC);
}

/*
 * void
 * xx_ifree(inode_t *ip)
 *	Free the specified inode on the specified device.
 *
 * Calling/Exit State:
 *	Inode rwlock is locked on entry and on exit.
 *
 * Description:
 *	The algorithm stores up to XXNICINOD inodes in the
 *	super-block and throws away any more.
 */
void
xx_ifree(inode_t *ip)
{
	ushort_t ino;
	vnode_t	 *vp;
	xx_fs_t	 *xxfsp;
	xxfilsys_t *fp;

	/*
	 * Don't put an already free inode on the free list.
	 */
	if (ip->i_mode == 0)
		return;
	vp = ITOV(ip);
	xxfsp = XXFS(vp->v_vfsp);
	ino = ip->i_number;
	fp = getfs(vp->v_vfsp);
	ip->i_mode = 0;		/* zero means inode not allocated */

	SLEEP_LOCK(&xxfsp->fs_sblock, PRIVFS);
	fp->s_tinode++;
	fp->s_fmod = 1;
	if (fp->s_ninode >= XXNICINOD || fp->s_ninode == 0) {
		if (ino < fp->s_inode[0]) {
			fp->s_inode[0] = ino;
		}
	} else {
		fp->s_inode[fp->s_ninode++] = ino;
	}
	SLEEP_UNLOCK(&xxfsp->fs_sblock);
}

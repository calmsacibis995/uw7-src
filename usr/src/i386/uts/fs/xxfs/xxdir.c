#ident	"@(#)kern-i386:fs/xxfs/xxdir.c	1.4.2.1"

/*
 * Directory manipulation routines. From outside this file, only
 * xx_dirlook(), xx_direnter(), and xx_dirremove() should be called.
 */

#include <fs/fski.h>

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/mode.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/xxfs/xxdir.h>
#include <fs/xxfs/xxinode.h>
#include <fs/xxfs/xxmacros.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/metrics.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define	DOT	0x01
#define	DOTDOT	0x02

extern	int	xx_bmap(inode_t *, daddr_t, daddr_t *, daddr_t *, int);
extern	int	xx_iaccess(inode_t *, mode_t, cred_t *);
extern	int	xx_ialloc(vfs_t *, o_mode_t, int, dev_t, int, int, inode_t **);
extern	int	xx_iget(vfs_t *, int, int, inode_t **);
extern	int	xx_itrunc(inode_t *, uint_t);
extern	int	xx_rdwri(uio_rw_t, inode_t *, caddr_t, int, off_t,
			 uio_seg_t, int, int *);
extern	int	xx_searchdir(char *, int, char []);
extern	void	xx_iput(inode_t *);
extern	void	xx_irele(inode_t *);
extern	void	xx_iupdat(inode_t *);
extern short	maxlink;

/*
 * int
 * xx_dirsearch(inode_t *dip, char *comp, inode_t **ipp, off_t *offp, int excl)
 *	Search for an entry.
 *
 *	Arguments passed in:
 *	dip ---> Directory to search.
 *	comp --> Component to search for.
 *	ipp ---> Ptr-to-ptr to result inode, if found.
 *	offp --> Offset of entry or empty entry.
 *	excl --> exclusive or shared mode.
 *
 * Calling/Exit State:
 *	dip is locked on entry and remain locked on exit.
 *	If found, *ipp is locked on exit.
 */
int
xx_dirsearch(inode_t *dip, char *comp, inode_t **ipp, off_t *offp, int excl)
{
	buf_t	   *bp;
	char	   *cp;
	daddr_t	   lbn;
	daddr_t	   bnp;
	int	   bsize;
	int	   count;
	int	   error;
	int	   found;
	int	   n;
	int	   off;
	off_t	   eo;
	off_t	   offset;
	vnode_t	   *dvp;
	xxdirect_t dir;

	dvp = ITOV(dip);
	bsize = VBSIZE(dvp);
	*ipp = NULL;
	dir.d_ino = 0;
	strncpy(dir.d_name, comp, XXDIRSIZ);

	offset = 0;
	eo = -1;
	count = dip->i_size;
	bp = NULL;
	found = 0;

	while (count) {
		/*
		 * Read the next directory block.
		 */
		MET_DIRBLK(MET_OTHER);
		lbn = (offset >> XXFS(dvp->v_vfsp)->fs_bshift);
		error = xx_bmap(dip, lbn, &bnp, NULL, B_READ);
		if (error) {
			goto out;
		} else {
			bp = bread(dip->i_dev, LTOPBLK(bnp, bsize), bsize);
			if (bp->b_flags & B_ERROR) {
				error = geterror(bp);
				brelse(bp);
				goto out;
			}
		}
		/*
		 * Search directory block.  xx_searchdir() returns the offset
		 * of a matching entry, or the offset of an empty entry, or -1.
		 */
		n = MIN(bsize, count);
		cp = bp->b_un.b_addr;
		off = xx_searchdir(cp, n, dir.d_name);
		if (off != -1) {
			cp += off;
			/* LINTED pointer alignment */
			dir.d_ino = ((xxdirect_t *)cp)->d_ino;
			if (dir.d_ino != 0) {
				found++;
				offset += off;
				break;
			}
			/* Keep track of empty slot. */
			if (eo == -1) {
				eo = offset + off;
			}
		}
		offset += n;
		count -= n;
		brelse(bp);
		bp = NULL;
	}

	if (bp) {
		brelse(bp);
	}
	if (found) {
		if (strcmp(dir.d_name, "..") == 0) {
			XX_IRWLOCK_UNLOCK(dip);
			error = xx_iget(dvp->v_vfsp, dir.d_ino, excl, ipp);
			if (excl) {
				XX_IRWLOCK_WRLOCK(dip);
			} else {
				XX_IRWLOCK_RDLOCK(dip);
			}
			if (error) {
				goto out;
			}
		} else if (dir.d_ino == dip->i_number) {
			/* want ourself, i.e., "." */
			VN_HOLD(dvp);
			*ipp = dip;
		} else {
			/* Just get inode. */
			error = xx_iget(dvp->v_vfsp, dir.d_ino, excl, ipp);
			if (error) {
				goto out;
			}
		}
	} else {
		/*
		 * If an empty slot was found, return it.  Otherwise leave
		 * the offset unchanged (pointing at the end of directory).
		 */
		if (eo != -1)
			offset = eo;
		error = ENOENT;
	}

out:
	if (offp)
		*offp = offset;
	return (error);
}

/*
 * int
 * xx_dirlook(inode_t *dp, char *namep, inode_t **ipp, int excl, cred_t *cr)
 *      Look for a given name in a directory.
 *
 * Calling/Exit State:
 *	<dp> is unlocked on entry; remains unlocked on return.
 *
 *	On successful returns, *ipp's rwlock is held *exclusive* if
 *	<excl> is non-zero, otherwise, *shared*.
 *
 *	A return value of 0 indicated success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		ENOTDIR		<dp> is not a directory
 */
int
xx_dirlook(inode_t *dp, char *namep, inode_t **ipp, int excl, cred_t *cr)
{
	int	error;
	int	namelen;
	inode_t	*ip;
	vnode_t	*vp;
	void	*cookie;
	boolean_t softhold;

	*ipp = NULL;
	/*
	 * Check accessibility of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}
	/*
	 * Prevent concurrent directory modifications by
	 * grabbing the directory's rwlock. *Shared*
	 * is sufficient to prevent modifications; the
	 * caller, may, however, specify *exclusive*
	 * via <excl>.
	 */
	namelen = strlen(namep);
	if (excl && (*namep == '\0' || (namelen == 1 && *namep == '.'))) {
		XX_IRWLOCK_WRLOCK(dp);
	} else {
		XX_IRWLOCK_RDLOCK(dp);
	}
	error = xx_iaccess(dp, IEXEC, cr);
	if (error) {
		XX_IRWLOCK_UNLOCK(dp);
		goto out;
	}
	/*
	 * Null component name is synonym for directory being searched.
	 */
	if (*namep == '\0') {
		VN_HOLD(ITOV(dp));
		*ipp = dp;
		goto out;
	}
	/*
	 * Check the directory name lookup cache.
	 */
	vp = dnlc_lookup(ITOV(dp), namep, &cookie, &softhold, NOCRED);
	if (vp) {
		ASSERT(softhold == B_FALSE);
		ip = VTOI(vp);
		/*
		 * It's ok to release the dir lock at this point since
		 * the calling LWP already has a reference to the inode.
		 */
		XX_IRWLOCK_UNLOCK(dp);
		if (excl) {
			XX_IRWLOCK_WRLOCK(ip);
		} else {
			XX_IRWLOCK_RDLOCK(ip);
		}
		*ipp = ip;
		goto out;
	}

	error = xx_dirsearch(dp, namep, ipp, (off_t *) 0, excl);
	if (dp != *ipp || error) {
		XX_IRWLOCK_UNLOCK(dp);
	}
	if (!error) {
		vp = ITOV(*ipp);
		dnlc_enter(ITOV(dp), namep, vp, NULL, NOCRED);
	}
out:
	return (error);
}

/*
 * int
 * xx_dircheckpath(inode_t *source, inode_t *target)
 *	Check if source directory is in the path of the target directory.
 *
 * Calling/Exit State:
 *	The caller holds target's rwlock *exclusive* on entry; the
 *	lock is dropped and re-acquired before returning.
 *
 *	<source> is unlocked on entry; remains unlocked on exit.
 */
int
xx_dircheckpath(inode_t *source, inode_t *target)
{
	inode_t	   *ip;
	int	   error;
	xxdirect_t dir;

	error = 0;
	/*
	 * If two renames of directories were in progress at once, the
	 * partially completed work of one xx_dircheckpath could be
	 * invalidated by the other rename. To avoid this, all directory
	 * renames in the system are serialized.
	 */
	ip = target;
	if (ip->i_number == source->i_number) {
		error = EINVAL;
		return (error);
	}
	if (ip->i_number == XXROOTINO) {
		return (error);
	}
	/*
	 * Search back through the directory tree, using the ".." entries.
	 * Fail any attempt to move a directory into an ancestor directory.
	 */
	for (;;) {
		if (((ip->i_mode & IFMT) != IFDIR) ||
		      ip->i_nlink == 0 || ip->i_size < 2*XXSDSIZ) {
			error = ENOTDIR;
			break;
		}
		error = xx_rdwri(UIO_READ, ip, (caddr_t) &dir, XXSDSIZ,
				(off_t) XXSDSIZ, UIO_SYSSPACE, 0, (int *) 0);
		if (error) {
			break;
		}
		if (strcmp(dir.d_name, "..") != 0) {
			error = ENOTDIR;	/* Sanity check */
			break;
		}
		if (dir.d_ino == source->i_number) {
			error = EINVAL;
			break;
		}
		if (dir.d_ino == XXROOTINO) {
			break;
		}
		if (ip != target) {
			xx_iput(ip);
		} else {
			XX_IRWLOCK_UNLOCK(ip);
		}
		error = xx_iget(ITOV(ip)->v_vfsp, dir.d_ino, 1, &ip);
		if (error) {
			break;
		}
	}
	if (ip) {
		if (ip != target) {
			xx_iput(ip);
			/*
			 * Relock target and make sure it has
			 * not gone away while it was unlocked.
			 */
			XX_IRWLOCK_WRLOCK(target);
			if (!error && target->i_nlink == 0) {
				error = ENOENT;
			}
		}
	}
	return (error);
}

/*
 * int
 * xx_dircheckforname(inode_t *tdp, char *namep, off_t *offp, inode_t **tipp)
 *	Check for the existence of a name in a directory, or else of an empty
 *	slot in which an entry may be made.
 *
 *	Arguments passed in:
 *	tdp -----> inode of directory being checked
 *	namep ---> name we are checking for
 *	offp ----> return offset of old or new entry
 *	tipp ----> return inode if we find one
 *
 * Calling/Exit State:
 *	<tdp> is locked exclusive on entry; remains so locked on exit.
 *
 *	On successful returns, <(*tipp)->i_rwlock> is held exclusive.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. There are no errnos returned directly
 *	by this routine.
 *
 * Description:
 *	If the requested name is found, then on return *tipp points at the
 *	(locked) inode and *offp contains its offset in the directory.  If
 *	the name is not found, then *tipp will be NULL and *offp will contain
 *	the offset of a directory slot in which an entry may be made (either
 *	an empty slot, or the first offset past the end of the directory).
 * 
 *	This may not be used on "." or "..", but aliases of "." are okay.
 */
int
xx_dircheckforname(inode_t *tdp, char *namep, off_t *offp, inode_t **tipp)
{
	int	error;

	/*
	 * Search for entry.  The caller does not require that
	 * it exist, so don't return ENOENT.  The non-existence
	 * of the entry will be indicated by *tipp == NULL.
	 */
	error = xx_dirsearch(tdp, namep, tipp, offp, 1);
	if (error == ENOENT) {
		error = 0;
	}
	return (error);
}

/*
 * int
 * xx_dirempty(inode_t *ip, int *dotflagp)
 *	Check whether a directory is empty (i.e. whether
 *	it contains any entries apart from "." and "..").
 * Calling/Exit State:
 *	Caller holds ip's rwlock exclusive on entry and exit.
 *
 * Description:
 *	The value returned in *dotflagp encodes
 *	whether "." and ".." are actually present.
 */
int
xx_dirempty(inode_t *ip, int *dotflagp)
{
	off_t	   off;
	xxdirect_t dir;
	xxdirect_t *dp = &dir;

	*dotflagp = 0;
	for (off = 0; off < ip->i_size; off += XXSDSIZ) {
		if (xx_rdwri(UIO_READ, ip, (caddr_t) dp, XXSDSIZ, off,
			     UIO_SYSSPACE, 0, (int *) 0)) {
			break;
		}
		if (dp->d_ino != 0) {
			if (strcmp(dp->d_name, ".") == 0) {
				*dotflagp |= DOT;
			} else if (strcmp(dp->d_name, "..") == 0) {
				*dotflagp |= DOTDOT;
			} else {
				return (0);
			}
		}
	}
	return (1);
}


/*
 * int
 * xx_dirfixdotdot(inode_t *dp, inode_t *opdp, inode_t *npdp)
 *	Fix the ".." entry of the child directory so that
 *	it points to the new parent directory instead of the
 *	old one. Routine assumes that dp is a directory and
 *	that all the inodes are on the same file system.
 *
 *	Arguments passed in:
 *	dp ----> child directory
 *	opdp --> old parent directory
 *	npdp --> new parent directory
 *
 * Calling/Exit State:
 *	<dp> is assumed to be a directory and all the inodes are
 *	assumed to be on the same file system.
 *
 *	<dp> and <opdp> are unlocked on entry; remain unlocked on exit.
 *	<npdp> is locked exclusive on entry; remains locked on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		ENOTDIR	<dp> contains a mangled ".." entry
 *		EMLINK	<npdp> already has a link count of maxlink.
 *
 * Description:
 *	Obtain <dp->i_rwlock> in exclusive mode. If <dp> doesn't
 *	exist any more (nlink == 0) or <dp> is smaller than the
 *	smallest allowable directory, than return 0.
 *
 */
int
xx_dirfixdotdot(inode_t *dp, inode_t *opdp, inode_t *npdp)
{
	int	   error;
	pl_t	   s;
	xxdirect_t dir;

	XX_IRWLOCK_WRLOCK(dp);
	/*
	 * Check whether this is an ex-directory.
	 */
	if (dp->i_nlink == 0 || dp->i_size < 2*XXSDSIZ) {
		goto err_rtn;
	}
	error = xx_rdwri(UIO_READ, dp, (caddr_t) &dir, XXSDSIZ, (off_t) XXSDSIZ,
			UIO_SYSSPACE, 0, (int *) 0);
	if (error) {
		goto err_rtn;
	}
	if (dir.d_ino == npdp->i_number) {	/* Just a no-op. */
		goto err_rtn;
	}
	if (strcmp(dir.d_name, "..") != 0) {	/* Sanity check. */
		error = ENOTDIR;
		goto err_rtn;
	}
	/*
	 * Increment the link count in the new parent inode and force it out.
	 */
	if (npdp->i_nlink >= maxlink) {
		error = EMLINK;
		goto err_rtn;
	}

	npdp->i_nlink++;
	s = XX_ILOCK(npdp);
	XXIMARK(npdp, ICHG|ISYN);
	XX_IUNLOCK(npdp, s);
	xx_iupdat(npdp);
	/*
	 * Rewrite the child ".." entry and force it out.
	 */
	dir.d_ino = npdp->i_number;
	error = xx_rdwri(UIO_WRITE, dp, (caddr_t) &dir, XXSDSIZ, (off_t) XXSDSIZ,
			UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (error) {
		goto err_rtn;
	}
	dnlc_remove(ITOV(dp), "..");
	dnlc_enter(ITOV(dp), "..", ITOV(npdp), NULL, NOCRED);
	XX_IRWLOCK_UNLOCK(dp);
	/*
	 * Decrement the link count of the old parent inode and force
	 * it out.  If opdp is NULL, then this is a new directory link;
	 * it has no parent, so we need not do anything.
	 */
	if (opdp != NULL) {
		XX_IRWLOCK_WRLOCK(opdp);
		if (opdp->i_nlink != 0) {
			opdp->i_nlink--;
			s = XX_ILOCK(opdp);
			XXIMARK(opdp, ICHG|ISYN);
			XX_IUNLOCK(opdp, s);
			xx_iupdat(opdp);
		}
		XX_IRWLOCK_UNLOCK(opdp);
	}
	return (0);
err_rtn:
	XX_IRWLOCK_UNLOCK(dp);
	return (error);
}

/*
 * xx_dirrename(inode_t *sdp, inode_t *sip, inode_t *tdp, char *namep,
 *		inode_t *tip, off_t offset, cred_t *cr)
 *	Rename the entry in the directory tdp so
 *	that it points to sip instead of tip.
 *
 *	Arguments passed in:
 *	sdp -------> parent directory of source
 *	sip -------> source inode
 *	tdp -------> parent directory of target
 *	namep -----> entry we are trying to change
 *	tip -------> locked target inode
 *	offset ----> offset of new entry
 *	cr --------> credentials
 *
 * Calling/Exit State:
 *	The calling LWP holds tdp's rwlock *exclusive* on entry
 *	and exit; tip's rwlock *exclusive* on entry and exit.
 *
 *	<sdp> is unlocked on entry; remains unlocked on exit.
 *	<sip> is unlocked on entry; remains unlocked on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine are:
 *		EXDEV	Either <tip> and <tdp> or <tip> and <sip> are
 *			on different file systems
 *		EISDIR	<tip> refers to a directory, but <sip> is not
 *			a directory.
 *		EBUSY	<tip> is a mounted on directory.
 *		EEXIST	<tip> is a non-empty directory.
 *		ENOTDIR	<sip> is a directory but <tip> is not.
 *		EIO	<tip> is a directory that contained more than 2 links.
 *
 * Description:
 *	Insure that the calling LWP has IWRITE access to <tdp> and
 *	that <tip>, <tdp>, and <sip> are all on the same file system.
 *
 *	Only allow renames if the file are compatible; do not allow
 *	renames of mounted file systems or to non-empty directories.
 */
int
xx_dirrename(inode_t *sdp, inode_t *sip, inode_t *tdp, char *namep,
	     inode_t *tip, off_t offset, cred_t *cr)
{
	int	   doingdirectory;
	int	   dotflag;
	int	   error;
	int	   vplocked;
	pl_t	   s;
	vnode_t	   *vp;
	xxdirect_t dir;

	/*
	 * Check that everything is on the same filesystem.
	 */
	if (tip->i_dev != tdp->i_dev || tip->i_dev != sip->i_dev) {
		return (EXDEV);
	}
	/*
	 * Short circuit rename of something to itself.
	 */
	if (sip->i_number == tip->i_number) {
		return (ESAME);		/* special KLUDGE error code */
	}
	/*
	 * Must have write permission to rewrite target entry.
	 */
	error = xx_iaccess(tdp, IWRITE, cr);
	if (error) {
		return (error);
	}
	/*
	 * If the parent directory is "sticky", then
	 * the user must own either the parent directory or the
	 * destination of the rename, or else must have permission
	 * permission to write the destination. Otherwise the 
	 * destination may not be changed (except by a privileged
	 * process).  This implements append-only directories.
	 */
	error = xx_iaccess(tip, IWRITE, cr);
	if (error && (tdp->i_mode & ISVTX) && pm_denied(cr, P_OWNER)
	    && cr->cr_uid != tdp->i_uid && cr->cr_uid != tip->i_uid)
		goto out;
	vplocked = 0;
	/*
	 * Ensure source and target are compatible (both directories
	 * or both not directories).  If target is a directory it must
	 * be empty and have no links to it; in addition it must not
	 * be a mount point.
	 */
	doingdirectory = ((sip->i_mode & IFMT) == IFDIR);
	if ((tip->i_mode & IFMT) == IFDIR) {
		if (!doingdirectory) {
			return (EISDIR);
		}
		/*
		 * Racing with a concurrent mount?
		 * Drop tip's rwlock and get the vnode
		 * rwlock to synchronize with mount.
		 *
		 * It's okay to release the lock here
		 * because we already have our reference
		 * to the inode and we hold the containing
		 * directory locked.
		 */
		XX_IRWLOCK_UNLOCK(tip);

		vp = ITOV(tip);
		RWSLEEP_WRLOCK(&vp->v_lock, PRIVFS);

		vplocked++;
		VN_LOCK(vp);
		if (vp->v_flag & VMOUNTING) {
			VN_UNLOCK(vp);
			/*
			 * concurrent mount...let mount win
			 */
			RWSLEEP_UNLOCK(&vp->v_lock);
			XX_IRWLOCK_WRLOCK(tip);
			return (EBUSY);
		}
		/*
		 * Cannot remove mount points
		 */
		if (vp->v_vfsmountedhere) {
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&vp->v_lock);
			XX_IRWLOCK_WRLOCK(tip);
			return (EBUSY);
		}

		vp->v_flag |= VGONE;
		VN_UNLOCK(vp);
		XX_IRWLOCK_WRLOCK(tip);

		if (!xx_dirempty(tip, &dotflag) || (tip->i_nlink > 2)) {
			error = EEXIST;
			goto out;
		}
	} else if (doingdirectory) {
		return (ENOTDIR);
	}
	/*
	 * Rewrite the inode number for the target name entry from the
	 * target inode (ip) to the source inode (sip). This prevents
	 * the target entry from disappearing during a crash.
	 */
	dir.d_ino = sip->i_number;
	(void) strncpy(dir.d_name, namep, XXDIRSIZ);
	error = xx_rdwri(UIO_WRITE, tdp, (caddr_t) &dir, XXSDSIZ, offset,
			 UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (error) {
		goto out;
	}
	dnlc_remove(ITOV(tdp), namep);
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NULL, NOCRED);

	s = XX_ILOCK(tdp);
	XXIMARK(tdp, ICHG);
	XX_IUNLOCK(tdp, s);
	/*
	 * Decrement the link count of the target inode.
	 * Fix the ".." entry in sip to point to dp.
	 * This is done after the new entry is on the disk.
	 */
	tip->i_nlink--;
	if (doingdirectory) {
		if (dotflag & DOT) {
			tip->i_nlink--;
			dnlc_remove(ITOV(tip), ".");
		}
		if (tip->i_nlink != 0) {
			/*
			 *+ After the kernel ensured that the directory was
			 *+ empty (via xx_dirempty()), and decremented the
			 *+ link count for the entry in the parent dir and
			 *+ the "." entry, it found that the link count was
			 *+ not zero. This indicates a possible filesystem
			 *+ corruption problem. Corrective Action: run
			 *+ fsck(1M) on the filesystem.
			 */
			cmn_err(CE_WARN,
				"xx_dirrename: target directory link count");
			error = EIO;
		}
		error = xx_itrunc(tip, (u_long)0);
		if (error) {
			goto out;
		}

                /*
                 * Renaming a directory with the parent different
                 * requires that ".." be rewritten.  The window is
                 * still there for ".." to be inconsistent, but this
                 * is unavoidable, and a lot shorter than when it was
                 * done in a user process.  We decrement the link
                 * count in the new parent as appropriate to reflect
                 * the just-removed target.  If the parent is the same,
                 * this is appropriate since the original directory
                 * is going away.  If the new parent is different,
                 * xx_dirfixdotdot() will bump the link count back.
                 */
                if (dotflag & DOTDOT) {
                        tdp->i_nlink--;
			dnlc_remove(ITOV(tip), ".");
			s = XX_ILOCK(tdp);
			XXIMARK(tdp, ICHG);
			XX_IUNLOCK(tdp, s);
                }

		if (sdp != tdp) {
			error = xx_dirfixdotdot(sip, sdp, tdp);
			if (error) {
				goto out;
			}
		}
	}
	s = XX_ILOCK(tip);
	XXIMARK(tip, ICHG);
	XX_IUNLOCK(tip, s);
out:
	if (vplocked) {
		if (error) {
			VN_LOCK(vp);
			vp->v_flag &= ~VGONE;
			VN_UNLOCK(vp);
		}
		RWSLEEP_UNLOCK(&vp->v_lock);
	}
	return (error);
}

/*
 * int
 * xx_dirmakedirect(inode_t *ip, inode_t *dp)
 *	Write a prototype directory in the empty inode ip, whose parent is dp.
 *
 *	Arguments passed in:
 *	ip -----> new directory
 *	dp -----> parent directory
 *
 * Calling/Exit State:
 *	The caller holds both ip's and dp's rwlock *exclusive* on
 *	entry and exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EMLINK	<tdp> already has a link count of maxlink.
 */
int
xx_dirmakedirect(inode_t *ip, inode_t *dp)
{
	int	   error;
	pl_t	   s;
	xxdirect_t newdir[2];

	(void) strncpy(newdir[0].d_name, ".", XXDIRSIZ);
	newdir[0].d_ino = ip->i_number;			/* dot */
	(void) strncpy(newdir[1].d_name, "..", XXDIRSIZ);	
	newdir[1].d_ino = dp->i_number;			/* dot-dot */

	error = xx_rdwri(UIO_WRITE, ip, (caddr_t) newdir, 2*XXSDSIZ, (off_t) 0,
			 UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (!error) {
		/*
		 * Synchronously update link count of parent.
		 */
		dp->i_nlink++;
		s = XX_ILOCK(dp);
		XXIMARK(dp, ICHG|ISYN);
		XX_IUNLOCK(dp, s);
		xx_iupdat(dp);
	}
	return (error);
}

/*
 * xx_dirmakeinode(inode_t *tdp, inode_t **ipp, vattr_t *vap,
 *		   enum de_op op, cred_t *cr)
 *	Allocate and initialize a new inode that will go into directory tdp.
 *
 * Calling/Exit State:
 *	The caller holds tdp's rwlock exclusive on entry;
 *	remains locked on exit. On success, <*ipp->i_rwlock>
 *	is returned referenced and unlocked.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned.
 */
int
xx_dirmakeinode(inode_t *tdp, inode_t **ipp, vattr_t *vap,
		enum de_op op, cred_t *cr)
{
	inode_t	*ip;
	int	imode;
	int	nlink;
	int	gid;
	int	error;
	pl_t	s;
	vfs_t	*vfsp;

	ASSERT(vap != NULL);
	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));
	ASSERT(op == DE_CREATE || op == DE_MKDIR);
	/*
	 * Allocate a new inode.
	 */
	imode = MAKEIMODE(vap->va_type, vap->va_mode);
	nlink = (op == DE_MKDIR) ? 2 : 1;
	/*
	 * If ISGID is set on the containing directory, the new
	 * entry inherits the directory's gid; otherwise the gid
	 * is taken from the supplied credentials.
	 */
	if (tdp->i_mode & ISGID) {
		gid = tdp->i_gid;
		if ((imode & IFMT) == IFDIR) {
			imode |= ISGID;
		} else if ((imode & ISGID) && !groupmember(gid, cr)
			 && pm_denied(cr, P_OWNER)) {
			imode &= ~ISGID;
		}
	} else {
		gid = cr->cr_gid;
	}

	vfsp = ITOV(tdp)->v_vfsp;
	error = xx_ialloc(vfsp, imode, nlink, vap->va_rdev,
			  cr->cr_uid, gid, &ip);
	if (error) {
		return error;
	}
	if (op == DE_MKDIR) {
		error = xx_dirmakedirect(ip, tdp);
	}
	if (error) {
		/*
		 * Throw away the inode we just allocated.
		 */
		ip->i_nlink = 0;
		s = XX_ILOCK(ip);
		XXIMARK(ip, ICHG);
		XX_IUNLOCK(ip, s);
		xx_iput(ip);
	} else {
		XX_IRWLOCK_UNLOCK(ip);
		*ipp = ip;
	}
	return (error);
}


/*
 * int
 * xx_diraddentry(inode_t *tdp, char *namep, off_t offset,
 *		  inode_t *sip, inode_t *sdp, enum de_op op)
 *	Enter the file sip in the directory tdp with name namep.
 *
 * Calling/Exit State:
 *	The caller holds tdp's rwlock *exclusive* on entry; locked on exit.
 *	<sip> is unlocked on entry; remains unlocked on exit.
 *	<sdp> is unlocked on entry; remains unlocked on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EXDEV	<tip> and <sip> are on different file systems
 *
 * Description:
 *	Fix the ".." of <sip> and write out the directory entry.
 *
 */
int
xx_diraddentry(inode_t *tdp, char *namep, off_t offset, inode_t *sip,
	       inode_t *sdp, enum de_op op)
{
	int	   error;
	xxdirect_t dir;

	error = xx_dirfixdotdot(sip, sdp, tdp);
	if ((sip->i_mode & IFMT) == IFDIR && op == DE_RENAME && error) {
		return error;
	}
	/*
	 * Fill in entry data.
	 */
	dir.d_ino = sip->i_number;
	(void) strncpy(dir.d_name, namep, XXDIRSIZ);
	/*
	 * Write out the directory entry.
	 */
	error = xx_rdwri(UIO_WRITE, tdp, (caddr_t) &dir, XXSDSIZ, offset,
			 UIO_SYSSPACE, op == DE_MKDIR ? IO_SYNC : 0, (int *) 0);
	if (!error) {
		dnlc_enter(ITOV(tdp), namep, ITOV(sip), NULL, NOCRED);
	}
	return (error);
}

/*
 * int
 * xx_direnter(inode_t *tdp, char *namep, enum de_op op, inode_t *sdp,
 *		inode_t *sip, vattr_t *vap, inode_t **ipp, cred_t *cr)
 *	Write a new directory entry.
 *
 *	Arguments passed in:
 *	tdp -----> target directory to make entry in
 *	namep ---> name of entry
 *	op ------> entry operation
 *	sdp -----> source inode parent if rename
 *	sip -----> source inode if link/rename
 *	vap -----> attributes if new inode needed
 *	ipp -----> return entered inode (locked) here
 *	cr ------> user credentials
 *
 * Calling/Exit State:
 *	<tdp>, <sdp>, and <sip> are unlocked on entry; remains
 *	unlocked on exit. If <ipp> is non-NULL and there's no error,
 *	<*ipp->i_rwlock> is held *exclusive* on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned.
 *
 * Description:
 *	The directory must not have been removed and must be writable.
 *	We distinguish four operations which build a new entry: creating
 *	a file (DE_CREATE), creating a directory (DE_MKDIR), renaming
 *	(DE_RENAME) or linking (DE_LINK).  There are five possible cases 
 *	to consider:
 *
 *	Name
 *	found	op			action
 *	-----	---------------------	--------------------------------------
 *	no	DE_CREATE or DE_MKDIR	create file according to vap and enter
 *	no	DE_LINK or DE_RENAME	enter the file sip
 *	yes	DE_CREATE or DE_MKDIR	error EEXIST *ipp = found file
 *	yes	DE_LINK			error EEXIST
 *	yes	DE_RENAME		remove existing file, enter new file
 *
 *	Note that a directory can be created either by mknod(2) or by
 *	mkdir(2); the operation (DE_CREATE or DE_MKDIR) distinguishes
 *	the two cases, which differ because mkdir(2) creates the
 *	appropriate "." and ".." entries while mknod(2) doesn't.
 */
int
xx_direnter(inode_t *tdp, char *namep, enum de_op op, inode_t *sdp,
	    inode_t *sip, vattr_t *vap, inode_t **ipp, cred_t *cr)
{
	char	*c;
	inode_t	*tip;			/* inode of (existing) target file */
	int	error;			/* error number */
	int	have_write_error;
	int	hold_rename_lock;
	off_t	offset;			/* offset of old or new dir entry */
	pl_t	s;
	xx_fs_t	*xxfsp;
	short	namelen;		/* length of name */
	vfs_t	*vfsp;

	/* don't allow '/' characters in pathname component */
	for (c = namep, namelen = 0; *c; c++, namelen++) {
		if (*c == '/') {
			return (EACCES);
		}
	}
	ASSERT(namelen != 0);
	/*
	 * If name is "." or ".." then if this is a create look it up
	 * and return EEXIST.  Rename or link TO "." or ".." is forbidden.
	 */
	if (namep[0] == '.' &&
	   (namelen == 1 || (namelen == 2 && namep[1] == '.'))) {
		if (op == DE_RENAME) {
			return (EINVAL);
		}
		if (ipp) {
			error = xx_dirlook(tdp, namep, ipp, 1, cr);
			if (error) {
				return (error);
			}
			XX_IRWLOCK_UNLOCK((*ipp));
		}
		return (EEXIST);
	}
	/*
	 * For mkdir, ensure that we won't be exceeding the maximum
	 * link count of the parent directory.
	 */
	if (op == DE_MKDIR && tdp->i_nlink >= maxlink) {
		return (EMLINK);
	}
	/*
	 * For link and rename, ensure that the source has not been
	 * removed while it was unlocked, that the source and target
	 * are on the same file system, and that we won't be exceeding
	 * the maximum link count of the source.  If all is well,
	 * synchronously update the link count.
	 */
	have_write_error = 0;
	if (op == DE_LINK || op == DE_RENAME) {
		XX_IRWLOCK_WRLOCK(sip);
		if (sip->i_nlink == 0) {
			XX_IRWLOCK_UNLOCK(sip);
			return (ENOENT);
		}
		if (sip->i_dev != tdp->i_dev) {
			XX_IRWLOCK_UNLOCK(sip);
			return (EXDEV);
		}
		if (sip->i_nlink >= maxlink) {
			XX_IRWLOCK_UNLOCK(sip);
			return (EMLINK);
		}
		sip->i_nlink++;
		s = XX_ILOCK(sip);
		XXIMARK(sip, ICHG|ISYN);
		XX_IUNLOCK(sip, s);

		xx_iupdat(sip);

		have_write_error = xx_iaccess(sip, IWRITE, cr);
		XX_IRWLOCK_UNLOCK(sip);
	}
	tip = NULL;
	vfsp = ITOV(tdp)->v_vfsp;
	xxfsp = XXFS(vfsp);
	if (op == DE_RENAME && (sip->i_mode & IFMT) == IFDIR && sdp != tdp) {
		SLEEP_LOCK(&xxfsp->fs_renamelock, PRINOD);
		hold_rename_lock = 1;
	} else {
		hold_rename_lock = 0;
	}

	/*
	 * If this is a rename of a directory and the parent is
	 * different (".." must be changed), then the source
	 * directory must not be in the directory hierarchy
	 * above the target, as this would orphan everything
	 * below the source directory.  Also the user must have
	 * write permission in the source so as to be able to
	 * change "..".
	 */
	if (hold_rename_lock) {
		if (have_write_error) {
			error = have_write_error;
			goto out;
		}
		error = xx_dircheckpath(sip, tdp);
		if (error) {
			goto out;
		}
	}

	/*
	 * Lock the directory in which we are trying to make the new entry.
	 */
	XX_IRWLOCK_WRLOCK(tdp);
	/*
	 * If target directory has not been removed, then we can consider
	 * allowing file to be created.
	 */
	if (tdp->i_nlink == 0) {
		error = ENOENT;
		goto out;
	}

	/*
	 * Check accessibility of directory.
	 */
	if ((tdp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}

	/*
	 * Execute access is required to search the directory.
	 */
	error = xx_iaccess(tdp, IEXEC, cr);
	if (error) {
		goto out;
	}

	/*
	 * Search for the entry.
	 */
	error = xx_dircheckforname(tdp, namep, &offset, &tip);
	if (error) {
		goto out;
	}

	if (tip) {
		switch (op) {

		case DE_CREATE:
		case DE_MKDIR:
			if (ipp) {
				*ipp = tip;
				error = EEXIST;
			} else {
				xx_iput(tip);
			}
			break;

		case DE_RENAME:
			error = xx_dirrename(sdp, sip, tdp, namep,
					     tip, offset, cr);
			xx_iput(tip);
			break;

		case DE_LINK:
			/*
			 * Can't link to an existing file.
			 */
			xx_iput(tip);
			error = EEXIST;
			break;
		}
	} else {
		/*
		 * The entry does not exist.  Check write permission in
		 * directory to see if entry can be created.
		 */
		error = xx_iaccess(tdp, IWRITE, cr);
		if (error) {
			goto out;
		}
		if (op == DE_CREATE || op == DE_MKDIR) {
			/*
			 * Make new inode and directory entry as required.
			 */
			error = xx_dirmakeinode(tdp, &sip, vap, op, cr);
			if (error) {
				goto out;
			}
		}
		error = xx_diraddentry(tdp, namep, offset, sip, sdp, op);
		if (error) {
			if (op == DE_CREATE || op == DE_MKDIR) {
				/*
				 * Unmake the inode we just made.
				 */
				if (op == DE_MKDIR) {
					tdp->i_nlink--;
				}
				sip->i_nlink = 0;
				s = XX_ILOCK(sip);
				XXIMARK(sip, ICHG);
				XX_IUNLOCK(sip, s);
				xx_irele(sip);
			}
		} else if (ipp) {
			XX_IRWLOCK_WRLOCK(sip);
			*ipp = sip;
		} else if (op == DE_CREATE || op == DE_MKDIR) {
			xx_irele(sip);
		}
	}

out:
	if (tdp != tip) {
		XX_IRWLOCK_UNLOCK(tdp);
	}

	if (error && (op == DE_LINK || op == DE_RENAME)) {
		/*
		 * Undo bumped link count.
		 */
		XX_IRWLOCK_WRLOCK(sip);
		sip->i_nlink--;
		s = XX_ILOCK(sip);
		XXIMARK(sip, ICHG);
		XX_IUNLOCK(sip, s);
		XX_IRWLOCK_UNLOCK(sip);
	}

	if (hold_rename_lock) {
		SLEEP_UNLOCK(&xxfsp->fs_renamelock);
	}
	return (error);
}

/*
 * int
 * xx_dirremove(inode_t *dp, char *namep, inode_t *oip,
 *		vnode_t *cdir, enum dr_op op, cred_t *cr)
 *	Delete a directory entry.  If oip is nonzero the entry
 *	is checked to make sure it still reflects oip.
 *
 * Calling/Exit State:
 *	<dp> is unlocked on entry; remains unlocked on exit. If
 *	given, <oip> is unlocked on entry; remains unlocked on exit.
 *
 * Description:
 *	If oip is nonzero the entry is checked to make sure it
 *	still reflects oip.
 */
int
xx_dirremove(inode_t *dp, char *namep, inode_t *oip,
	     vnode_t *cdir, enum dr_op op, cred_t *cr)
{
	inode_t	   *ip;
	int	   error;
	int	   dotflag;
	int	   vplocked;
	off_t	   offset;
	pl_t	   s;
	vnode_t	   *vp;
	xxdirect_t dir;
	static	   xxdirect_t emptydirect[] = {
		0, ".",
		0, "..",
	};

	vplocked = 0;
	ip = NULL;
	XX_IRWLOCK_WRLOCK(dp);
	/*
	 * Check accessibility of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}
	error = xx_iaccess(dp, IEXEC|IWRITE, cr);
	if (error) {
		goto out;
	}
	error = xx_dircheckforname(dp, namep, &offset, &ip);
	if (error) {
		goto out;
	}
	if (ip == NULL || (oip && oip != ip)) {
		error = ENOENT;
		goto out;
	}
	/*
	 * Don't remove a mounted-on directory (the possible result
	 * of a race between mount(2) and unlink(2) or rmdir(2)).
	 */
	if ((ip->i_mode & IFMT) == IFDIR) {
		/*
		 * Racing with a concurrent mount?
		 * Drop ip's rwlock and get the vnode
		 * r/w lock to synchronize with mount.
		 * Inode rwlock is dropped to avoid the locking
		 * violation before grabing vnode lock.
		 *
		 * Ok to release lock here because we
		 * already have our reference to the
		 * inode and we hold the containing
		 * directory locked.
		 */
		XX_IRWLOCK_UNLOCK(ip);

		vp = ITOV(ip);
		RWSLEEP_WRLOCK(&vp->v_lock, PRIVFS);

		vplocked++;

		VN_LOCK(vp);
		if (vp->v_flag & VMOUNTING){
			VN_UNLOCK(vp);
			/*
			 * concurrent mount -> let mount win
			 */
			RWSLEEP_UNLOCK(&vp->v_lock);
			XX_IRWLOCK_UNLOCK(dp);
			return (EBUSY);
		}

		/*
		 * Can't remove mount points
		 */
		if (vp->v_vfsmountedhere) {
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&vp->v_lock);
			XX_IRWLOCK_UNLOCK(dp);
			return (EBUSY);
		}

		vp->v_flag |= VGONE;
		VN_UNLOCK(vp);

		XX_IRWLOCK_WRLOCK(ip);
	}

	/*
	 * If the parent directory is "sticky", then the
	 * user must own the parent directory or the file in it, or else
	 * must have permission to write the file.  Otherwise it may not
	 * be deleted. This implements append-only directories.
	 */
	error = xx_iaccess(ip, IWRITE, cr);
	if (error && (dp->i_mode & ISVTX) && pm_denied(cr, P_OWNER)
    	    && cr->cr_uid != dp->i_uid && cr->cr_uid != ip->i_uid) {
		goto out;
	}

	if (op == DR_RMDIR) {
		/*
		 * For rmdir(2), some special checks are required.
		 * (a) Don't remove any alias of the parent (e.g. ".").
		 * (b) Don't remove the current directory.
		 * (c) Make sure the entry is (still) a directory.
		 * (d) Make sure the directory is empty.
		 */
		if (dp == ip || ITOV(ip) == cdir) {
			error = EBUSY;
		} else if ((ip->i_mode & IFMT) != IFDIR) {
			error = ENOTDIR;
		} else if (!xx_dirempty(ip, &dotflag)) {
			error = EEXIST;
		}
		if (error) {
			goto out;
		}
	} else if (op == DR_REMOVE) {
		/*
		 * unlink(2) requires a different check: allow only
		 * the super-user to unlink a directory.
		 */
		if (ITOV(ip)->v_type == VDIR && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}
	}
	/*
	 * Zero the i-number field of the directory entry.  Retain the
	 * file name in the empty slot, as UNIX has always done.
	 */
	dir.d_ino = 0;
	(void) strncpy(dir.d_name, namep, XXDIRSIZ);
	error = xx_rdwri(UIO_WRITE, dp, (caddr_t) &dir, XXSDSIZ, offset,
			 UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (error) {
		goto out;
	}
	dnlc_remove(ITOV(dp), namep);
	/*
	 * Now dispose of the inode.
	 */
	if (op == DR_RMDIR) {
		if (dotflag & DOT) {
			ip->i_nlink -= 2;
			dnlc_remove(ITOV(ip), ".");
		} else {
			ip->i_nlink--;
		}
		if (dotflag & DOTDOT) {
			dp->i_nlink--;
			dnlc_remove(ITOV(ip), "..");
		}
		/*
		 * If other references exist, zero the "." and "..
		 * entries so they're inaccessible (POSIX requirement).
		 * If the directory is going away we can avoid doing
		 * this work.
		 */
		if (ITOV(ip)->v_count > 1 && ip->i_nlink <= 0)
			(void) xx_rdwri(UIO_WRITE, ip, (caddr_t) emptydirect,
					min(sizeof(emptydirect), ip->i_size),
					(off_t) 0, UIO_SYSSPACE, 0, (int *) 0);
	} else {
		ip->i_nlink--;
	}

	if (ip->i_nlink < 0) {	/* Pathological */
		/*
		 *+ Somehow, ip->i_nlink was < 0; this should never
		 *+ occur. Set it to 0.
		 */
		cmn_err(CE_WARN, "xx_dirremove: ino %d, dev %x, nlink %d",
		  ip->i_number, ip->i_dev, ip->i_nlink);
		ip->i_nlink = 0;
	}
	s = XX_ILOCK(dp);
	XXIMARK(dp, ICHG);
	XX_IUNLOCK(dp, s);

	if (ip != dp) {
		s = XX_ILOCK(ip);
		XXIMARK(ip, IUPD|ICHG);
		XX_IUNLOCK(ip, s);
	}
out:
	if (vplocked) {
		if (error) {
			VN_LOCK(vp);
			vp->v_flag &= ~VGONE;
			VN_UNLOCK(vp);
		}
		RWSLEEP_UNLOCK(&vp->v_lock);
	}

	if (ip) {
		xx_iput(ip);
	}
	if (ip != dp) {
		XX_IRWLOCK_UNLOCK(dp);
	}
	return (error);
}


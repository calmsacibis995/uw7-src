#ident	"@(#)kern-i386:fs/xxfs/xxvnops.c	1.5.4.1"

#include <fs/fski.h>

#include <acc/priv/privilege.h>
#include <acc/dac/acl.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/fbuf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/specfs/snode.h>
#include <fs/specfs/specfs.h>
#include <fs/xxfs/xxdir.h>
#include <fs/xxfs/xxfilsys.h>
#include <fs/xxfs/xxinode.h>
#include <fs/xxfs/xxmacros.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/metrics.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

extern	int	xx_bmap(inode_t *, daddr_t, daddr_t *, daddr_t *, int);
extern	int	xx_direnter(inode_t *, char *, enum de_op, inode_t *,
			    inode_t *, vattr_t *, inode_t **, cred_t *);
extern	int	xx_dirlook(inode_t *, char *, inode_t **, int, cred_t *);
extern	int	xx_dirremove(inode_t *, char *, inode_t *, vnode_t *,
			     enum dr_op, cred_t *);
extern	int	xx_iaccess(inode_t *, mode_t, cred_t *);
extern	int	xx_itrunc(inode_t *, uint_t);
extern	int	xx_rdwri(uio_rw_t, inode_t *, caddr_t, int, off_t,
			 uio_seg_t, int, int *);
extern	int	xx_readi(inode_t *, uio_t *, int);
extern	int	xx_sync(vfs_t *, int, cred_t *);
extern	int	xx_syncip(inode_t *, int);
extern	int	xx_writei(inode_t *, uio_t *, int);
extern	void	xx_iinactive(inode_t *, int);
extern	void	xx_iput(inode_t *);
extern	void	xx_irele(inode_t *);
extern	void	xx_iupdat(inode_t *);

/*
 * UNIX file system operations vector.
 */
STATIC int	xx_open(vnode_t **, int, cred_t *);
STATIC int	xx_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC int	xx_read(vnode_t *, uio_t *, int, cred_t *);
STATIC int	xx_write(vnode_t *, uio_t *, int, cred_t *);
STATIC int	xx_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC int	xx_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int	xx_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC int	xx_access(vnode_t *, int, int, cred_t *);
STATIC int	xx_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			  vnode_t *, cred_t *);
STATIC int	xx_create(vnode_t *, char *, vattr_t *, vcexcl_t, int,
			vnode_t **, cred_t *);
STATIC int	xx_remove(vnode_t *, char *, cred_t *);
STATIC int	xx_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC int	xx_rename(vnode_t *, char *, vnode_t *, char *, cred_t *);
STATIC int	xx_mkdir(vnode_t *, char *, vattr_t *, vnode_t **, cred_t *);
STATIC int	xx_rmdir(vnode_t *, char *, vnode_t *, cred_t *);
STATIC int	xx_readdir(vnode_t *, uio_t *, cred_t *, int *);
STATIC int	xx_symlink(vnode_t *, char *, vattr_t *, char *, cred_t *);
STATIC int	xx_readlink(vnode_t *, uio_t *, cred_t *);
STATIC int	xx_fsync(vnode_t *, cred_t *);
STATIC int	xx_fid(vnode_t *, struct fid **);
STATIC int	xx_seek(vnode_t *, off_t, off_t *);
STATIC int	xx_frlock(vnode_t *, int, flock_t *, int, off_t, cred_t *);
STATIC void	xx_inactive(vnode_t *, cred_t *);
STATIC void	xx_rwunlock(vnode_t *, off_t, off_t);
STATIC int	xx_rwlock(vnode_t *, off_t, off_t, int, int);

vnodeops_t xx_vnodeops = {
	xx_open,
	xx_close,
	xx_read,
	xx_write,
	xx_ioctl,
	fs_setfl,
	xx_getattr,
	xx_setattr,
	xx_access,
	xx_lookup,
	xx_create,
	xx_remove,
	xx_link,
	xx_rename,
	xx_mkdir,
	xx_rmdir,
	xx_readdir,
	xx_symlink,
	xx_readlink,
	xx_fsync,
	xx_inactive,
	(void (*)())fs_nosys,	/* release */
	xx_fid,
	xx_rwlock,
	xx_rwunlock,
	xx_seek,
	fs_cmp,
	xx_frlock,
	(int (*)())fs_nosys,	/* realvp */
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* map */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	fs_poll,
	fs_pathconf,
	(int (*)())fs_nosys,	/* getacl */
	(int (*)())fs_nosys,	/* setacl */
	(int (*)())fs_nosys,	/* setlevel */
	(int (*)())fs_nosys,	/* getdvstat */
	(int (*)())fs_nosys,	/* setdvstat */
	(int (*)())fs_nosys,	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	(int (*)())fs_nosys,	/* stablestore */
	(int (*)())fs_nosys,	/* relstore */
	(int (*)())fs_nosys,	/* getpagelist */
	(int (*)())fs_nosys,	/* putpagelist */
	(int (*)())fs_nosys	/* msgio */
};

/*
 * int
 * xx_open(vnode_t **vpp, int flag, cred_t *cr)
 *	Open a file.
 *
 * Calling/Exit State:
 *	No locks are held on entry and on exit.
 *
 * Description:
 *	No special action required for ordinary files.
 *	Devices are handled through the SPECFS.
 */
/* ARGSUSED */
STATIC int
xx_open(vnode_t **vpp, int flag, cred_t *cr)
{
	return (0);
}

/*
 * int 
 * xx_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset,
 *	    cred_t *cr)
 * 	Close a protocol.
 *
 * Calling/Exit State:
 *	No lock is held on entry and exit.
 *
 * Description:
 *	The inode's rwlock is held to prevent another LWP in the same
 *	process from establishing a file/record lock on the file.
 */
/* ARGSUSED */
STATIC int
xx_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	inode_t	*ip;

	ip = VTOI(vp);

	if (vp->v_filocks) {
		XX_IRWLOCK_WRLOCK(ip);
		cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
		XX_IRWLOCK_UNLOCK(ip);
	}

	return (0);
}

/* 
 * int
 * xx_read(vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr)
 *	Transfer data from <vp> to the calling process's address
 *	space.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in at least *shared*
 *	mode. This lock is must be acquired from above the VOP interface
 *	via VOP_RWRDLOCK() (below the VOP interface use xx_rwlock()).
 *	VOP_RWRDLOCK() specifying the same length, offset that's
 *	in <uiop>.
 *
 *	A return value of 0 indicates success; othwerise a valid errno
 *	is returned. 
 *
 */
/* ARGSUSED */
STATIC int
xx_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	ip = VTOI(vp);
	error = xx_readi(ip, uiop, ioflag);
	XXACC_TIMES(ip);
	return (error);
}

/*
 * int
 * xx_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Transfer data from the calling process's address space to <vp>.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock in *exclusive* mode on
 *	entry; it remains held on exit. The rwlock was acquired by calling
 *	VOP_RWWRLOCK specifying the same length, offset pair that's
 *	in <uiop>.
 */
/* ARGSUSED */
STATIC int
xx_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	ASSERT(vp->v_type == VREG);

	ip = VTOI(vp);
	error = fs_vcode(vp, &ip->i_vcode);
	if (vp->v_type == VREG && error) {
		return (error);
	}
	if ((ioflag & IO_APPEND) && vp->v_type == VREG) {
		/*
		 * In append mode start at end of file.
		 * NOTE: lock not required around i_size
		 * sample since r/w lock is held exclusive
		 * by calling process.
		 */
		uiop->uio_offset = ip->i_size;
	}
	error = xx_writei(ip, uiop, ioflag);
	if ((ioflag & IO_SYNC) && vp->v_type == VREG) {
		/* 
		 * If synchronous write, update inode now.
		 */
		xx_iupdat(ip);
	}
	return (error);
}

/*
 *
 * int
 * xx_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
 *	Simply return ENOTTY.
 *
 * Calling/Exit State:
 *	No file/vnode locks held.
 *
 */
/* ARGSUSED */
STATIC int
xx_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
{
	return (ENOTTY);
}

/*
 * int
 * xx_getsp(vnode_t *vp, ulong_t *totp)
 *	Compute the total number of blocks allocated to the file, including
 *	indirect blocks as well as data blocks, on the assumption that the
 *	file contains no holes.  (It's too expensive to account for holes
 *	since that requires a complete scan of all indirect blocks.)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */

STATIC int
xx_getsp(vnode_t *vp, ulong_t *totp)
{
	int	inshift;
	int	indir;
	int	loc;
	xx_fs_t	*xxfsp;
	u_long	blocks;
	u_long	tot;

	xxfsp = XXFS(vp->v_vfsp);
	loc = VTOI(vp)->i_size + VBSIZE(vp) - 1;
	blocks = tot = lblkno(xxfsp, loc);
	if (blocks > NDADDR) {
		inshift = xxfsp->fs_nshift;
		indir = xxfsp->fs_nindir;
		tot += ((blocks - NDADDR - 1) >> inshift) + 1;
		if (blocks > NDADDR + indir) {
			tot += ((blocks - NDADDR - indir - 1) >>
				(inshift * 2)) + 1;
			if (blocks > NDADDR + indir + indir*indir) {
				tot++;
			}
		}
	}
	*totp = tot;
	return (0);
}

/*
 * int
 * xx_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 *	Return attributes for a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	The inode's rwlock is held *shared* while copying the inode's
 *	attributes.
 *
 */
/* ARGSUSED */
STATIC int
xx_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	inode_t	*ip;
	int	error;
	pl_t	s;
	ulong_t	nlblocks;

	error = 0;
	ip = VTOI(vp);
	/*
	 * Return (almost) all the attributes.  This should be refined so
	 * that it only returns what's asked for.
	 */
	XX_IRWLOCK_RDLOCK(ip);
	vap->va_type = vp->v_type;
	vap->va_mode = ip->i_mode & MODEMASK;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_fsid = ip->i_dev;
	vap->va_nodeid = ip->i_number;
	vap->va_nlink = ip->i_nlink;
	vap->va_size = ip->i_size;
	if (vp->v_type == VCHR || vp->v_type == VBLK || vp->v_type == VXNAM) {
		vap->va_rdev = ip->i_rdev;
	} else {
		vap->va_rdev = 0;	/* not a b/c spec. */
	}
	s = XX_ILOCK(ip);
	vap->va_atime.tv_sec = ip->i_atime;
	vap->va_atime.tv_nsec = 0;
	vap->va_mtime.tv_sec = ip->i_mtime;
	vap->va_mtime.tv_nsec = 0;
	vap->va_ctime.tv_sec = ip->i_ctime;
	vap->va_ctime.tv_nsec = 0;
	XX_IUNLOCK(ip, s);
	vap->va_type = vp->v_type;
	if (vp->v_type == VBLK || vp->v_type == VCHR) {
		vap->va_blksize = MAXBSIZE;
	} else {
		vap->va_blksize = VBSIZE(vp);
	}
	vap->va_vcode = ip->i_vcode;
	if (vap->va_mask & AT_NBLOCKS) {
		error = xx_getsp(vp, &nlblocks);
		if (error == 0) {
			vap->va_nblocks = FsLTOP(XXFS(vp->v_vfsp), nlblocks);
		}
	} else {
		vap->va_nblocks = 0;
	}
	if (vap->va_mask & AT_ACLCNT) {
		vap->va_aclcnt = NACLBASE;
	}

	XX_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * int
 * xx_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
 *	Modify/Set a inode's attributes.
 *
 * Calling/Exit State:
 *	The caller doesn't hold any inode locks on entry or exit.
 *
 * Description:
 *	The attributes are set up in the order of AT_SIZE, AT_MODE,
 *	AT_UID|AT_GID, and AT_ATIME|AT_MTIME because if the mode is
 *	set first, then the permissions were changed and truncate
 *	will fail since the mode is not matched with the old one. 
 *
 */
STATIC int
xx_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
{
	inode_t		*ip;
	int		error;
	int		newvcode;
	int		issync;
	long int	mask;
	pl_t		s;
	timestruc_t	timenow;

	error = newvcode = issync = 0;
	mask = vap->va_mask;
	ip = VTOI(vp);

	/*
	 * Cannot set the attributes represented by AT_NOSET.
	 */
	if (mask & AT_NOSET) {
		return (EINVAL);
	}

	XX_IRWLOCK_WRLOCK(ip);

	/*
	 * Truncate file.  Must have write permission
	 * and file must not be a directory.
	 */
	if (mask & AT_SIZE) {
		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}
		if (vap->va_size >
		    u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur) {
			error = EFBIG;
			goto out;
		}
		error = xx_iaccess(ip, IWRITE, cr);
		if (error) {
			goto out;
		}
		
		if (vp->v_type == VREG && !newvcode) {
			error = fs_vcode(vp, &ip->i_vcode);
			if (error) {
				goto out;
			}
		}

		/*
		 * Check if there is any active mandatory lock on the
		 * range that will be truncated/expanded.
		 */

		if (MANDLOCK(vp, ip->i_mode)) {
			off_t	offset;
			int	length;

			if (ip->i_size < vap->va_size) {
				/*
				 * "Truncate up" case: need to make sure
				 * there is no lock beyond current EOF.
				 */
				offset = ip->i_size;
				length = vap->va_size - offset;
			} else {
				offset = vap->va_size;
				length = ip->i_size - offset;
			}
			error = chklock(vp, FWRITE, offset, length, ioflags,
					ip->i_size);
			if (error) {
				goto out;
			}
		}
		error = xx_itrunc(ip, vap->va_size);
		if (error) {
			goto out;
		}
		issync++;
	}
	/*
	 * Change file access modes.  Must be owner or a privileged process.
	 */
	if (mask & AT_MODE) {
		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}
		ip->i_mode &= IFMT;
		ip->i_mode |= vap->va_mode & ~IFMT;
		/*
		 * A non-privileged user can set the sticky bit
		 * on a directory.
		 */
		if (vp->v_type != VDIR) {
			if ((ip->i_mode & ISVTX) && pm_denied(cr, P_OWNER)) {
				ip->i_mode &= ~ISVTX;
			}
		}
		if (!groupmember(ip->i_gid, cr) && pm_denied(cr, P_OWNER)) {
			ip->i_mode &= ~ISGID;
		}
		s = XX_ILOCK(ip);
		XXIMARK(ip, ICHG);
		XX_IUNLOCK(ip, s);
		if (MANDLOCK(vp, vap->va_mode)) {
			error = fs_vcode(vp, &ip->i_vcode);
			if (error) {
				goto out;
			}
			newvcode = 1;
		}
	}
	/*
	 * Change file ownership; must be the owner of the file
	 * or privileged.  If the system was configured with
	 * the "rstchown" option, the owner is not permitted to
	 * give away the file, and can change the group id only
	 * to a group of which he or she is a member.
	 */
	if (mask & (AT_UID|AT_GID)) {
		int	checksu;

		checksu = 0;
		if (rstchown) {
			if (((mask & AT_UID) && vap->va_uid != ip->i_uid)
			 || ((mask & AT_GID) && !groupmember(vap->va_gid, cr)))
				checksu = 1;
		}  else if (cr->cr_uid != ip->i_uid) {
			checksu = 1;
		}

		if (checksu && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}

		if (pm_denied(cr, P_OWNER)) {
			ip->i_mode &= ~(ISUID|ISGID);
		}
		if (mask & AT_UID) {
			ip->i_uid = vap->va_uid;
		}
		if (mask & AT_GID) {
			ip->i_gid = vap->va_gid;
		}
		s = XX_ILOCK(ip);
		XXIMARK(ip, ICHG);
		XX_IUNLOCK(ip, s);
	}
	/*
	 * Change file access or modified times.
	 */
	if (mask & (AT_ATIME|AT_MTIME)) {
		boolean_t	mtime = B_TRUE;
		boolean_t	atime = B_TRUE;

		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			if (flags & ATTR_UTIME) {
				error = EPERM;
			} else {
				error = xx_iaccess(ip, IWRITE, cr);
			}
			if (error) {
				goto out;
			}
		}
		GET_HRESTIME(&timenow);
		s = XX_ILOCK(ip);
                if (mask & AT_MTIME) {
                        if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
                                if ((flags & ATTR_UPDTIME) &&
				    (vap->va_mtime.tv_sec <= ip->i_mtime)) {
						mtime = B_FALSE;
                                } else {
                                        ip->i_mtime = vap->va_mtime.tv_sec;
				}
                        } else
                                ip->i_mtime = timenow.tv_sec;

			if (mtime == B_TRUE) {
                                ip->i_ctime = timenow.tv_sec;
				ip->i_flag &= ~(IUPD | ICHG | IACC);
				ip->i_flag |= IMODTIME;
			}
                }
		if (mask & AT_ATIME) {
                        if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
                                if ((flags & ATTR_UPDTIME) &&
                                    (vap->va_atime.tv_sec <= ip->i_atime)) {
					atime = B_FALSE;
                                } else
                                        ip->i_atime = vap->va_atime.tv_sec;
                        } else
                                ip->i_atime = timenow.tv_sec;
			if (atime == B_TRUE)
				ip->i_flag &= ~IACC;
                }

		if (mtime == B_TRUE || atime == B_TRUE)
			ip->i_flag |= IMOD;
		XX_IUNLOCK(ip, s);
	}
out:
	if ((flags & ATTR_EXEC) == 0) {
		if (issync) {
			ip->i_flag |= ISYN;
		}
		xx_iupdat(ip);		/* XXX: should be async for perf */
	}
	XX_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * int
 * xx_access(vnode_t *vp, int mode, int flags, cred_t *cr)
 *	Determine the accessibility of a file to the calling
 *	process.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *	The inode's rwlock is held *shared* while determining
 *	accessibility of the file to the caller.
 *
 * Description:
 *	If the file system containing <vp> has not been sealed then
 *	obtain the inode shared/exclusive lock in shared mode. Use
 *	xx_iaccess() to determine accessibility. Return what it does
 *	after releasing the shared/exclusive lock.
 */
/* ARGSUSED */
STATIC int
xx_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	ip = VTOI(vp);
	XX_IRWLOCK_RDLOCK(ip);
	error = xx_iaccess(ip, mode, cr);
	XX_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * int
 * xx_lookup(vnode_t *dvp, char *name, vnode_t **vpp, pathname_t *pnp,
 *	     int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *	Check whether a given directory contains a file named <name>.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		ENOSYS  The file found in the directory is a special file
 *			but SPECFS was unable to create a vnode for it.
 *
 * Description:
 *	We treat null components as a synonym for the directory being
 *	searched. In this case, merely increment the directory's reference
 *	count and return. For all other cases, search the directory via
 *	xx_dirlook(). If we find the given file, indirect to SPECFS if
 *	it's a special file.
 */
STATIC int
xx_lookup(vnode_t *dvp, char *name, vnode_t **vpp, pathname_t *pnp,
	  int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	char	nm[XXDIRSIZ+1];
	inode_t	*ip;
	inode_t	*xip;
	int	error;
	vnode_t	*newvp;

	/*
	 * Null component name is synonym for directory being searched.
	 */
	if (*name == '\0') {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	/*
	 * Ensure name is truncated to XXDIRSIZ characters.
	 */
	*nm = '\0';
	(void) strncat(nm, name, XXDIRSIZ);

	ip = VTOI(dvp);
	error = xx_dirlook(ip, nm, &xip, 0, cr);
	/*
	 * If we find the file in the directory, the inode is
	 * returned referenced with the rwlock held *shared*.
	 */
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		XX_IRWLOCK_UNLOCK(ip);
		/*
		 * If vnode is a device return special vnode instead.
		 */
		if (ISVDEV((*vpp)->v_type)) {
			newvp =
			       specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cr);
			VN_RELE(*vpp);
			if (newvp == NULL) {
				error = ENOSYS;
			} else {
				*vpp = newvp;
			}
		}
	}
	return (error);
}

/*
 * int
 * xx_create(vnode_t *dvp, char *name, vattr_t *vap, vcexcl_t excl,
 *	     int mode, vnode_t **vpp, cred_t *cr)
 *	Create a file in a given directory.
 *
 * Calling/Exit State:
 *	The vnode's shared/exclusive sleep lock is held on entry.
 *
 * Description:
 *	If the name of the file to create is null, it is treated as a
 *	synonym for the directory that the file is created in. In this
 *	case, the directory's rwlock is obtained in shared mode to check
 *	the calling LWP's access permission to the directory. The lock is
 *	necessary to prevent the permission vector from being changed.
 *
 *	If name is not null, direnter will create a directory entry
 *	for the new file atomically by holding the directory inode's
 *	rwlock exclusive. The directory is unlocked before returning
 *	to this routine. On success, the new inode is returned with
 *	it's rwlock held exclusive. The lock is held exclusive since
 *	the new entry may require truncation (iff AT_SIZE is specified).
 *
 */
STATIC int
xx_create(vnode_t *dvp, char *name, vattr_t *vap, vcexcl_t excl,
	  int mode, vnode_t **vpp, cred_t *cr)
{
	char	nm[XXDIRSIZ+1];
	inode_t	*dip;
	inode_t	*ip;
	int	error;
	vnode_t	*newvp;

	dip = VTOI(dvp);
	if (*name == '\0') {
		/*
		 * Null component name refers to the directory itself.
		 */
		VN_HOLD(dvp);
		XX_IRWLOCK_RDLOCK(dip);
		error = EEXIST;
		ip = dip;
	} else {
		/*
		 * Ensure name is truncated to XXDIRSIZ characters.
		 */
		*nm = '\0';
		(void) strncat(nm, name, XXDIRSIZ);

		ip = NULL;
		error = xx_direnter(dip, nm, DE_CREATE, (inode_t *) 0,
				   (inode_t *) 0, vap, &ip, cr);
	}

	/*
	 * If the file already exists and this is a non-exclusive create,
	 * check permissions and allow access for non-directories.
	 * Read-only create of an existing directory is also allowed.
	 * We fail an exclusive create of anything which already exists.
	 */
	if (error == EEXIST) {
		if (excl == NONEXCL) {
			if (((ip->i_mode & IFMT) == IFDIR) && (mode & IWRITE)) {
				error = EISDIR;
			} else if (mode) {
				error = xx_iaccess(ip, mode, cr);
			} else {
				error = 0;
			}
		}
		if (error) {
			xx_iput(ip);
			return (error);
		} else if (((ip->i_mode & IFMT) == IFREG)
			 && (vap->va_mask & AT_SIZE) && vap->va_size == 0) {
			/*
			 * Truncate regular files, if requested by caller.
			 */
			error = xx_itrunc(ip, (ulong_t)0);
			if (!error) {
				error = fs_vcode(ITOV(ip), &ip->i_vcode);
			}
			if (error) {
				XX_IRWLOCK_UNLOCK(ip);
				return (error);
			}
		}
	} 
	if (!error) {
		*vpp = ITOV(ip);
		XX_IRWLOCK_UNLOCK(ip);
		/*
	 	* If vnode is a device return special vnode instead.
	 	*/
		if (ISVDEV((*vpp)->v_type)) {
			newvp =
			       specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cr);
			VN_RELE(*vpp);
			if (newvp == NULL) {
				error = ENOSYS;
			} else {
				*vpp = newvp;
			}
		}
	}

	return (error);
}

/*
 * int
 * xx_remove(vnode_t *dvp, char *name, cred_t *cr)
 *	Remove a file from a directory.
 *
 * Calling/Exit State:
 *	The caller holds no inode locks on entry or exit.
 *
 * Description:
 *	xx_remove() uses xx_dirremove() to remove the directory while
 *	holding the containing directory's rwlock in exclusive mode.
 */
STATIC int
xx_remove(vnode_t *dvp, char *name, cred_t *cr)
{
	char	nm[XXDIRSIZ+1];
	inode_t	*ip;
	int	error;

	ip = VTOI(dvp);
	/*
	 * Ensure name is truncated to XXDIRSIZ characters.
	 */
	*nm = '\0';
	(void) strncat(nm, name, XXDIRSIZ);

	error = xx_dirremove(ip, nm, (inode_t *) 0, (vnode_t *) 0,
			     DR_REMOVE, cr);
	return (error);
}

/*
 * int
 * xx_link(vnode_t *tdvp, vnode_t *svp, char *tname, cred_t *cr)
 *	Link a file or a directory. Hard links to directories
 *	are no longer allowed for privileged users.
 *
 * Calling/Exit State:
 *	No vnode/inode locks are held on entry or exit.
 *
 * Description:
 *	direnter adds a link to the source file in the
 *	target directory atomically. If the source file is a directory,
 *	the caller must have privelege to perform the operation (this
 *	is determined by pm_denied). The target directory's rwlock is
 *	held exclusive for the udration of the operation to prevent
 *	other LWP's from accessing the target directory. The
 *	source inode's rwlock is held exclusive while it's link count
 *	is incremented.
 */
STATIC int
xx_link(vnode_t *tdvp, vnode_t *svp, char *tname, cred_t *cr)
{
	char	tnm[XXDIRSIZ+1];
	inode_t	*sip;
	inode_t	*tdp;
	int	error;
	vnode_t	*realvp;

	if (VOP_REALVP(svp, &realvp) == 0) {
		svp = realvp;
	}
	if (svp->v_type == VDIR) {
		return (EPERM);
	}
	sip = VTOI(svp);
	tdp = VTOI(tdvp);

	/*
	 * Ensure name is truncated to XXDIRSIZ characters.
	 */
	*tnm = '\0';
	(void) strncat(tnm, tname, XXDIRSIZ);

	error = xx_direnter(tdp, tnm, DE_LINK, (inode_t *) 0, sip,
			   (vattr_t *) 0, (inode_t **) 0, cr);
	return (error);
}

/*
 * int
 * xx_rename(vnode_t *sdvp, char *sname, vnode_t *tdvp, char *tname, cred_t *cr)
 * 	Rename a file or directory.
 *
 * Calling/Exit State:
 *	 No lock is held on entry or exit.
 *
 * Description:
 * 	We are given the vnode and entry string of the source and the
 * 	vnode and entry string of the place we want to move the source
 * 	to (the target). The essential operation is:
 *		unlink(target);
 *		link(source, target);
 *		unlink(source);
 * 	but "atomically". Can't do full commit without saving state in
 * 	the inode on disk, which isn't feasible at this time. Best we
 *	can do is always guarantee that the TARGET exists.
 *
 *	The rename is performed by a combination of dirlook, direnter
 *	and dirremove. dirlook is used to locate the file being
 *	renamed. If the file exists, it is returned with it's rwlock
 *	held shared; this lock is removed before calling direnter.
 *	direnter holds the target directory's rwlock exclusive and
 *		o removes the entry for the target file (if exists) from
 *		  the target directory
 *		o adds and entry for the source file to the target directory
 *		o if the source file is a directory, the source file's
 *		parent directory's link count is adjusted.
 *	This last step introduces the potential for A->B/B->A deadlock since
 *	both the source file's parent directory and the target directory's
 *	rwlock must be held. Deadlock would result if a rename occurs in the
 *	opposite directory, i.e., with the parent directories reversed. To
 *	prevent deadlock, all rename operations where the source file
 *	is a directory and the parent directory changes must be serialized.
 *	Serialization occurs via a per-file system sleep lock.
 */
STATIC int
xx_rename(vnode_t *sdvp, char *sname, vnode_t *tdvp, char *tname, cred_t *cr)
{
	inode_t	*sip;		/* source inode */
	inode_t	*sdp;		/* old (source) parent inode */
	inode_t	*tdp;		/* new (target) parent inode */
	int	error;
	int	have_write_error;
	char	snm[XXDIRSIZ+1];
	char	tnm[XXDIRSIZ+1];

	/*
	 * Ensure names are truncated to XXDIRSIZ characters.
	 */
	*snm = '\0';
	*tnm = '\0';
	(void) strncat(snm, sname, XXDIRSIZ);
	(void) strncat(tnm, tname, XXDIRSIZ);

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);
	/*
	 * Look up inode of file we're supposed to rename.
	 */
	error = xx_dirlook(sdp, snm, &sip, 0, cr);
	if (error) {
		return error;
	}
	have_write_error = xx_iaccess(sip, IWRITE, cr);
	XX_IRWLOCK_UNLOCK(sip);
	/*
	 * Make sure we can delete the source entry.  This requires
	 * write permission on the containing directory.  If that
	 * directory is "sticky" it further requires that the
	 * user own the directory or the source entry, or else
	 * have permission to write the source entry.
	 */
	XX_IRWLOCK_RDLOCK(sdp);
	error = xx_iaccess(sdp, IWRITE, cr);
	if (!error) {
		if ((sdp->i_mode & ISVTX) && pm_denied(cr, P_OWNER)
	    	  && cr->cr_uid != sdp->i_uid && cr->cr_uid != sip->i_uid
		  && have_write_error) {
			error = have_write_error;
		}
	}
	XX_IRWLOCK_UNLOCK(sdp);
	if (error) {
		xx_irele(sip);
		return (error);
	}

	/*
	 * Check for renaming '.' or '..' or alias of '.'
	 */
	if (strcmp(snm, ".") == 0 || strcmp(snm, "..") == 0 || sdp == sip) {
		error = (EINVAL);
		goto out;
	}
	/*
	 * Link source to the target.
	 */
	error = xx_direnter(tdp, tnm, DE_RENAME, sdp, sip, (vattr_t *) 0,
			   (inode_t **) 0, cr);
	if (error) {
		goto out;
	}
	/*
	 * Remove the source entry.  xx_dirremove() checks that the
	 * entry still reflects sip, and returns an error if it
	 * doesn't. If the entry has changed just forget about it.
	 * Release the source inode.
	 */
	error = xx_dirremove(sdp, snm, sip, NULLVP, DR_RENAME, cr);
	if (error == ENOENT) {
		error = 0;
	}

out:
	/*
	 * Check for special error return which indicates a no-op rename.
	 */
	if (error == ESAME) {
		error = 0;
	}
	xx_irele(sip);
	return (error);
}

/*
 * int
 * xx_mkdir(vnode_t *dvp, char *dirname, vattr_t *vap, vnode_t **vpp,
 *	    cred_t *cr)
 *	Create a directory file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/inode locks on entry or exit.
 *
 * Description:
 *	xx_direnter() creates a directory in <dvp> while holding
 *	the containing directory's rwlock in exclusive mode. On
 *	successful direnter()'s, the newly created directory is
 *	returned with its rwlock held exclusive.
 */
STATIC int
xx_mkdir(vnode_t *dvp, char *dirname, vattr_t *vap, vnode_t **vpp, cred_t *cr)
{
	inode_t	*ip;
	inode_t	*xip;
	int	error;
	char	dirnm[XXDIRSIZ+1];

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	/*
	 * Ensure name is truncated to XXDIRSIZ characters.
	 */
	*dirnm = '\0';
	(void) strncat(dirnm, dirname, XXDIRSIZ);

	ip = VTOI(dvp);
	error = xx_direnter(ip, dirnm, DE_MKDIR, (inode_t *) 0,
			   (inode_t *) 0, vap, &xip, cr);
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		XX_IRWLOCK_UNLOCK(ip);
	} else if (error == EEXIST) {
		xx_iput(xip);
	}
	return (error);
}

/*
 * int
 * xx_rmdir(vnode_t *vp, char *name, vnode_t *cdir, cred_t *cr)
 *	Remove a directory.
 *
 * Calling/Exit State:
 *	The caller holds no vnode/inode locks on entry or exit.
 */
STATIC int
xx_rmdir(vnode_t *vp, char *name, vnode_t *cdir, cred_t *cr)
{
	inode_t	*ip;
	int	error;
	char	nm[XXDIRSIZ+1];

	ip = VTOI(vp);
	/*
	 * Ensure name is truncated to XXDIRSIZ characters.
	 */
	*nm = '\0';
	(void) strncat(nm, name, XXDIRSIZ);

	error = xx_dirremove(ip, nm, (inode_t *) 0, cdir, DR_RMDIR, cr);
	return (error);
}

#define	DIRBUFSIZE	1048
STATIC	void	xx_filldir();

/*
 * int
 * xx_readdir(vnode_t *vp, uio_t *uiop, cred_t *fcr, int *eofp)
 *	Read from a directory.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock in *shared* mode. The
 *	rwlock was obtained by a call to VOP_RWRDLOCK.
 *
 *	A return value of not -1 indicates success;
 *	otherwise, a valid errno is returned.
 *
 *	On success, an <*eofp> value of 1 indicates that end-of-file
 *	has been reached, i.e., there are no more directory entries
 *	that may be read.
 */
/* ARGSUSED */
STATIC int
xx_readdir(vnode_t *vp, uio_t *uiop, cred_t *fcr, int *eofp)
{
	buf_t	*bp;
	caddr_t	direntp;
	daddr_t	bnp;
	daddr_t	lbn;
	inode_t	*ip;
	int	bmask;
	int	bsize;
	int	error;
	int	oresid;
	int	ran_out;
	off_t	diroff;
	off_t	olddiroff;
	pl_t	s;
	xx_fs_t	*xxfsp;

	bp = NULL;
	ip = VTOI(vp);
	error = ran_out = 0;
	oresid = uiop->uio_resid;
	diroff = uiop->uio_offset;
	xxfsp = XXFS(vp->v_vfsp);

	if (vp->v_type != VDIR) {
		return (ENOTDIR);
	}
	/*
	 * Error if not on directory entry boundary.
	 */
	if (diroff % XXSDSIZ != 0) {
		return (ENOENT);
	}
	if (diroff < 0) {
		return (EINVAL);
	}
	bsize = VBSIZE(vp);
	bmask = xxfsp->fs_bmask;
	/*
	 * Allocate space to hold dirent structures.  Use the most common
	 * request size (1048).
	 */
	direntp = (caddr_t) kmem_alloc(DIRBUFSIZE, KM_SLEEP);
	/*
	 * In a loop, read successive blocks of the directory,
	 * converting the entries to fs-independent form and
	 * copying out, until the end of the directory is
	 * reached or the caller's request is satisfied.
	 */
	do {
		int	blkoff;
		int	leftinfile;
		int	leftinblock;
		char	*directp;

		olddiroff = -1;
		/*
		 * If diroff hasn't been changed terminate operation.
		 */
		if (olddiroff == diroff) {
                        /*
                         *+ A directory on a XX file system type
                         *+ contained an invalid directory entry.
                         */
                        cmn_err(CE_WARN, "xx_readdir: bad dir, inumber = %d\n",ip->i_number);
                        error = ENXIO;
                        goto out;
                }
		/*
		 * If at or beyond end of directory, we're done.
		 */
		leftinfile = ip->i_size - diroff;
		if (leftinfile <= 0) {
			break;
		}
		/*
		 * Map in next block of directory entries.
		 */
		lbn = diroff >> xxfsp->fs_bshift;
		error = xx_bmap(ip, lbn, &bnp, NULL, B_READ);
		if (error) {
			goto out;
		} else {
			bp = bread(ip->i_dev, LTOPBLK(bnp, bsize), bsize);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				goto out;
			}
		}
		blkoff = diroff & bmask;	/* offset in block */
		leftinblock = MIN(bsize - blkoff, leftinfile);
		directp = bp->b_un.b_addr + blkoff;
		/*
		 * In a loop, fill the allocated space with fs-independent
		 * directory structures and copy out, until the current
		 * disk block is exhausted or the caller's request has
		 * been satisfied.
		 */
		do {
			int ndirent;	/* nbytes of "dirent" filled */
			int ndirect;	/* nbytes of "direct" consumed */
			int maxndirent;	/* max nbytes of "dirent" to fill */

			maxndirent = MIN(DIRBUFSIZE, uiop->uio_resid);
			xx_filldir(direntp, maxndirent, directp, leftinblock,
				   diroff, &ndirent, &ndirect);
			directp += ndirect;
			leftinblock -= ndirect;
			olddiroff = diroff;	/* save the old diroff */
			diroff += ndirect;
			if (ndirent == -1) {
				ran_out = 1;
				goto out;
			} else if (ndirent == 0)
				break;
			error = uiomove(direntp, ndirent, UIO_READ, uiop);
			if (error) {
				goto out;
			}
		} while (leftinblock > 0 && uiop->uio_resid > 0);
		brelse(bp);
		bp = NULL;
	} while (uiop->uio_resid > 0 && diroff < ip->i_size);
out:
	/*
	 * If we ran out of room but haven't returned any entries, error.
	 */
	if (ran_out && uiop->uio_resid == oresid) {
		error = EINVAL;
	}
	if (bp) {
		brelse(bp);
	}
	kmem_free(direntp, DIRBUFSIZE);
	/*
	 * Offset returned must reflect the position in the directory itself,
	 * independent of how much fs-independent data was returned.
	 */
	if (error == 0) {
		uiop->uio_offset = diroff;
		if (eofp) {
			*eofp = (diroff >= ip->i_size);
		}
	}
	if (((vp)->v_vfsp->vfs_flag & VFS_RDONLY) == 0) {
		s = XX_ILOCK(ip);
		XXIMARK(ip, IACC);
		XX_IUNLOCK(ip, s);
	}
	return (error);
}

/*
 * xx_filldir(char *direntp, int nmax, char *directp, int nleft,
 *	      off_t diroff, int *ndirentp, int *ndirectp)
 *	Convert fs-specific directory entries ("struct direct") to
 *	fs-independent form ("struct dirent") in a supplied buffer.
 *
 *	Arguments passed in:
 *	direntp ----> buffer to be filled
 *	nmax -------> max nbytes to be filled
 *	directp ----> buffer of disk directory entries
 *	nleft ------> nbytes of dir entries left in block
 *	diroff -----> offset in directory
 *	ndirentp ---> nbytes of "struct dirent" filled
 *	ndirectp ---> nbytes of "struct direct" consumed
 *
 * Calling/Exit State:
 *	Inode's rwlock lock is held in shared mode on entry or exit.
 *
 * Description: 
 *	Returns, through reference parameters, the number of bytes of
 *	"struct dirent" with which the buffer was filled and the number of
 *	bytes of "struct direct" which were consumed.  If there was a
 *	directory entry to convert but no room to hold it, the "number
 *	of bytes filled" will be -1.
 */
STATIC void
xx_filldir(char *direntp, int nmax, char *directp, int nleft,
	   off_t diroff, int *ndirentp, int *ndirectp)
{
	dirent_t   *newdirp;
	int	   direntsz;
	int	   namelen;
	int	   ndirect;
	int	   ndirent;
	int	   reclen;
	long int   ino;
	xxdirect_t *olddirp;

	ndirent = ndirect = 0;
	/* LINTED pointer alignment */
	newdirp = (dirent_t *) direntp;
	direntsz = (char *) newdirp->d_name - (char *) newdirp;

	/* LINTED pointer alignment */
	for (olddirp = (xxdirect_t *) directp;
	     /* LINTED pointer alignment */
	     olddirp < (xxdirect_t *) (directp + nleft);
	     olddirp++, ndirect += XXSDSIZ) {
		ino = olddirp->d_ino;
		if (ino == 0) {
			continue;
		}
		namelen = (olddirp->d_name[XXDIRSIZ - 1] == '\0') ?
			   strlen(olddirp->d_name) : XXDIRSIZ;
		reclen = (direntsz + namelen + 1 + (NBPW - 1)) & ~(NBPW - 1);
		if (ndirent + reclen > nmax) {
			if (ndirent == 0) {
				ndirent = -1;
			}
			break;
		}
		ndirent += reclen;
		newdirp->d_reclen = (short)reclen;
		newdirp->d_ino = ino;
		newdirp->d_off = diroff + ndirect + XXSDSIZ;
		bcopy(olddirp->d_name, newdirp->d_name, namelen);
		newdirp->d_name[namelen] = '\0';
		/* LINTED pointer alignment */
		newdirp = (dirent_t *) (((char *) newdirp) + reclen);
	}
	*ndirentp = ndirent;
	*ndirectp = ndirect;
}

/*
 * int
 * xx_symlink(vnode_t *dvp, char *linkname, vattr_t *vap, char *target,
 *	      cred_t *cr)
 *	Create a symbolic link file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/inode locks on entry or exit.
 *
 * Description:
 *	direnter creates a symbolic link in <dvp> while holding
 *	the containing directory's rwlock in exclusive mode.
 *	If the symbolic link is created, it's inode is returned
 *	with the rwlock held exclusive. This lock is held while
 *	the contents for the symbolic link are written.
 */
STATIC int
xx_symlink(vnode_t *dvp, char *linkname, vattr_t *vap, char *target, cred_t *cr)
{
	inode_t	*ip;
	inode_t	*dip;
	int	error;
	char	linknm[XXDIRSIZ+1];

	dip = VTOI(dvp);
	/*
	 * Ensure name is truncated to XXDIRSIZ characters.
	 */
	*linknm = '\0';
	(void) strncat(linknm, linkname, XXDIRSIZ);

	error = xx_direnter(dip, linknm, DE_CREATE, (struct inode *) 0,
			   (inode_t *) 0, vap, &ip, cr);
	if (!error) {
		error = xx_rdwri(UIO_WRITE, ip, target, strlen(target),
				(off_t) 0, UIO_SYSSPACE, IO_SYNC, (int *) 0);
		xx_iput(ip);
	} else if (error == EEXIST) {
		xx_iput(ip);
	}
	return (error);
}

/*
 * int
 * xx_readlink(vnode_t *vp, uio_t *uiop, cred_t *cr)
 *	Read a symbolic link file.
 *
 * Calling/Exit State:
 *	No inode/vnode locks are held on entry or exit.
 *
 *	On success, 0 is returned; otherwise, a valid errno is
 *	returned. Errnos returned directly by this routine are:
 *		EINVAL	The vnode is not a symbolic link file.
 *
 */
/* ARGSUSED */
STATIC int
xx_readlink(vnode_t *vp, uio_t *uiop, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	if (vp->v_type != VLNK) {
		return (EINVAL);
	}
	ip = VTOI(vp);
	XX_IRWLOCK_RDLOCK(ip);
	error = xx_readi(ip, uiop, 0);
	XX_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * xx_fsync(vnode_t *vp, cred_t *cr)
 *	Synchronously flush a file.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 * Description:
 *	If the file system containing <vp> has not been sealed then
 *	obtain the inode sleep lock. Use syncip() to flush the file.
 *	If that completes successfully, then update the inode's
 *	times. Return any errors after releasing the sleep lock.
 */
/* ARGSUSED */
STATIC int
xx_fsync(vnode_t *vp, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	ip = VTOI(vp);
	XX_IRWLOCK_RDLOCK(ip);
	error = xx_syncip(ip, B_WRITE);		/* Do synchronous writes */
	XX_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * void
 * xx_inactive(vnode_t *vp, cred_t *cr)
 *	Perform cleanup on an unreferenced inode.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Simply call xx_iinactive() with the flag indicating
 *	that the inode's rwlock is *not* held for <vp>.
 *
 *	xx_iinactive is called without applying any locks. xx_iinactive
 *	will check is there is another LWP waiting the inode to become
 *	available, i.e., another LWP holds the inode's rwlock and is
 *	spinning in VN_HOLD. In this situation, the inode is not released
 *	and is given to the waiting LWP. Another interaction with xx_iget
 *	is checked for: xx_iget establishes a reference to an inode by
 *	obtaining the inode table lock, searching a hash list for an
 *	inode, and then, atomically enqueuing for the inode's rwlock while
 *	dropping the inode table lock. xx_iinactive will obtain the inode
 *	table lock and check whether any LWPs are queued on the inode's
 *	rwlock. If there are any blocked LWPs, the inode is not inactivated.
 */
/* ARGSUSED */
STATIC void
xx_inactive(vnode_t *vp, cred_t *cr)
{
	if (vp->v_type == VCHR) {
		vp->v_stream = NULL;
	}
	xx_iinactive(VTOI(vp), NODEISUNLOCKED);
}

/*
 * int
 * xx_fid(vnode_t *vp, fid_t **fidpp)
 *	Return a unique identifier for the given file.
 *
 * Calling/Exit State:
 *	No inode/vnode locks are held on entry or exit.
 *
 * Description:
 *	The file identifier for the vnode is generated from
 *	the invariant fields of the inode. Thus, the fields
 *	are simply copied without any locking.
 */
STATIC int
xx_fid(vnode_t *vp, fid_t **fidpp)
{
	ufid_t	*ufid;

	ufid = (ufid_t *) kmem_zalloc(sizeof(ufid_t), KM_SLEEP);
	if (ufid == NULL) {
		return (NULL);
	}
	ufid->ufid_len = sizeof(ufid_t) - sizeof(ushort_t);
	ufid->ufid_ino = VTOI(vp)->i_number;
	ufid->ufid_gen = VTOI(vp)->i_gen;
	*fidpp = (fid_t *) ufid;
	return (0);
}

/*
 * int
 * xx_rwlock(vnode_t *vp, off_t off, off_t len, int fmode, int mode)
 *	Obtain, if possible, the inode's rwlock according to <mode>.
 *
 * Calling/Exit State:
 *	A return value of 0 indicates success.
 *
 *	On success, the rwlock of the inode is held according to
 *	mode. It is also guaranteed that the caller will not block
 *	on I/O operations to the range indicated by <off, len>
 *	while holding the rwlock (i.e., until a subsequent
 *	VOP_RWUNLOCK() is performed).
 *
 *	On failure, the rwlock of the inode is *not* held.
 *
 * Description:
 *	Acquire the inode's rwlock in the requested mode and then
 *	if mandatory locking is enabled for the file, check whether
 *	there are any file/record locks which would cause the LWP
 *	to block on a subsequent I/O operation. If there are, the
 *	caller will block in chklock() if <fmode> indicates it's
 *	OK to do so.
 */
STATIC int
xx_rwlock(vnode_t *vp, off_t off, off_t len, int fmode, int mode)
{
	inode_t *ip;
	int	error;

	ip = VTOI(vp);
	error = 0;

	if (mode == LOCK_EXCL) {
		XX_IRWLOCK_WRLOCK(ip);
	} else if (mode == LOCK_SHARED) {
		XX_IRWLOCK_RDLOCK(ip);
	} else {
		/*
		 *+ An invalid mode was passed as a parameter to this
		 *+ routine. This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC,"xx_rwlock: invalid lock mode requested");
	}

	if (MANDLOCK(vp, ip->i_mode)) {
		if (fmode) {
			error = chklock(vp, (mode == LOCK_SHARED) ? FREAD :
					FWRITE, off, len, fmode, ip->i_size);
		}
	}

	if (error) {
		XX_IRWLOCK_UNLOCK(ip);
	}

	return (error);
}

/*
 * void
 * xx_rwunlock(vnode_t *vp, off_t off, off_t len)
 *	Release the inode's rwlock.
 *
 * Calling/Exit State:
 *	On entry, the calling LWP must hold the inode's rwlock
 *	in either *shared* or *exclusive* mode. On exit, the
 *	caller's hold on the lock is released.
 *
 * Remarks:
 *	Currently, <off> and <len> are ignored. In the future, they
 *	they might be used for a finer grained locking scheme.
 */
/* ARGSUSED */
STATIC void
xx_rwunlock(vnode_t *vp, off_t off, off_t len)
{
	inode_t	*ip;

	ip = VTOI(vp);
	XX_IRWLOCK_UNLOCK(ip);
}

/*
 * int
 * xx_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *	Validate a seek pointer.
 *
 * Calling/Exit State:
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EINVAL	The new seek pointer is negative.
 *
 * Description:
 *	No locking is necessary since the result of this routine
 *	depends entirely on the value of <*noffp>.
 */
/* ARGSUSED */
STATIC int
xx_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	return (*noffp < 0 ? EINVAL : 0);
}

/*
 * int
 * xx_frlock(vnode_t *vp, int cmd, struct flock *bfp, int flag,
 *	     off_t offset, cred_t *cr)
 *	Establish or interrogate the state of an advisory
 *	or mandatory lock on the vnode.
 *
 * Calling/Exit State:
 *	No vnode/inode locks are held on entry or exit.
 *
 * Description:
 *	To set a file/record lock (i.e., cmd is F_SETLK or
 *	F_SETLKW), the inode's rwlock must be held in exclusive
 *	mode before checking whether mandatory locking is enabled
 *	for the vnode to interact correctly with LWPs performing I/O
 *	to or from the file.
 */
/* ARGSUSED */
STATIC int
xx_frlock(vnode_t *vp, int cmd, struct flock *bfp, int flag,
	  off_t offset, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	ip = VTOI(vp);

	if (cmd == F_SETLK || cmd == F_SETLKW) {
		XX_IRWLOCK_WRLOCK(ip);
	} else {
		XX_IRWLOCK_RDLOCK(ip);
	}
	error = fs_frlock(vp, cmd, bfp, flag, offset, cr, ip->i_size);

	XX_IRWLOCK_UNLOCK(ip);
	return (error);
}

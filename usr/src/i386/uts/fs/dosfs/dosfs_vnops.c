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

#ident	"@(#)kern-i386:fs/dosfs/dosfs_vnops.c	1.15.5.1"

#include "dosfs.h"
#include <acc/dac/acl.h>

/*
 *  Some general notes:
 *
 *  In the ufs filesystem the inodes, superblocks, and indirect
 *  blocks are read/written using the vnode for the filesystem.
 *  Blocks that represent the contents of a file are read/written
 *  using the vnode for the file (including directories when
 *  they are read/written as files).
 *  This presents problems for the dos filesystem because data
 *  that should be in an inode (if dos had them) resides in the
 *  directory itself.  Since we must update directory entries
 *  without the benefit of having the vnode for the directory
 *  we must use the vnode for the filesystem.  This means that
 *  when a directory is actually read/written (via read, write,
 *  or readdir, or seek) we must use the vnode for the filesystem
 *  instead of the vnode for the directory as would happen in ufs.
 *  This is to insure we retreive the correct block from the
 *  buffer cache since the hash value is based upon the vnode
 *  address and the desired block number.
 */

/*
 * Prototypes for DOSFS vnode operations
 */
STATIC int dosfs_lookup (vnode_t *dvp, char *name, vnode_t **vpp,
               pathname_t *pnp, int lookup_flags, vnode_t *rootvp, cred_t *cr);
STATIC int dosfs_create (vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl, int mode, vnode_t **vpp, cred_t *cr);
STATIC int dosfs_open (vnode_t **vpp, int flag, cred_t *cr);
STATIC int dosfs_close (vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr);
STATIC int dosfs_daccess (vnode_t *vp, int mode, int flags, cred_t *cr);
STATIC int dosfs_access (vnode_t *vp, int mode, int flags, cred_t *cr);
STATIC int dosfs_getattr (vnode_t *vp, vattr_t *vap, int flags, cred_t *cr);
STATIC int dosfs_setattr (vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr);
STATIC int dosfs_read (vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr);
STATIC int dosfs_write (vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr);
STATIC int dosfs_ioctl (vnode_t *vp, int com, int arg, int fflag, cred_t *cr, int *rvalp);
STATIC int dosfs_fsync (vnode_t *vp, cred_t *cr);
STATIC int dosfs_seek (vnode_t *vp, off_t oldoff, off_t *newoff);
STATIC int dosfs_remove (vnode_t *dvp, char *name, cred_t *cr);
STATIC int dosfs_rename (vnode_t *fdvp, char *fname, vnode_t *tdvp, char *tname, cred_t *cr);
STATIC int dosfs_mkdir (vnode_t *dvp, char *name, vattr_t *vap, vnode_t **vpp, cred_t *cr);
STATIC int dosfs_rmdir (vnode_t *dvp, char *name, vnode_t *cdir, cred_t *cr);
STATIC int dosfs_readdir (vnode_t *vp, uio_t *uio, cred_t *cr, int *eofflagp);
void dosfs_inactive (vnode_t *vp, cred_t *cr);
STATIC int dosfs_lock (vnode_t *vp, off_t, off_t, int, int);
STATIC void dosfs_unlock (vnode_t *vp, off_t, off_t);

extern int dosfs_dirlook(vnode_t *dvp, char *name, vnode_t **vpp, int flags, cred_t *cr);

#define TST_GROUP	3
#define TST_OTHER	6

/*ARGSUSED*/
STATIC int
dosfs_daccess (vnode_t *vp, int mode, int flags, cred_t *cr)
{
	int		dosmode;
	uid_t		uid;
	gid_t		gid;
	struct denode	*dep;
	struct dosfs_vfs *pvp;
	int		denied_mode;
	int		lshift;
	int		i;

	if (mode & VWRITE) {
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return (EROFS);
	}

	dep = VTODE(vp);
	pvp = dep->de_vfs;

	if (pvp->vfs_uid != DEFUSR)	uid = pvp->vfs_uid;
	else	uid = 0;

	if (pvp->vfs_gid != DEFGRP)	gid = pvp->vfs_gid;
	else	gid = 0;

	if (dep->de_Attributes & ATTR_READONLY)	dosmode = 0555;
	else	dosmode = 0777;

	dosmode &= ~pvp->vfs_mask;
	dosmode |= 0111;

	/*
	 * Access check is based on only one of owner, group, public.
	 * If not owner, then check group. If not a member of the
	 * group, then check public access.
	 */

	if (cr->cr_uid == uid)
		lshift = 0;
	else if (groupmember(gid, cr)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	} else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}

	if ((i = (dosmode & mode)) == mode)
		return 0;

	denied_mode = (mode & (~i));
	denied_mode <<= lshift;

	if ((denied_mode & (IREAD | IEXEC)) && pm_denied(cr, P_DACREAD))
		return (EACCES);

	if ((denied_mode & IWRITE) && pm_denied(cr, P_DACWRITE))
		return (EACCES);

	return 0;
}

/*
 *  Create a regular file.
 *  On entry the directory to contain the file being
 *  created is locked.  We must release before we
 *  return.
 *  We must also free the pathname buffer pointed at
 *  by ndp->ni_pnbuf, always on error, or only if the
 *  SAVESTART bit in ni_nameiop is clear on success.
 */


STATIC int
dosfs_create (vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl, int mode, vnode_t **vpp, cred_t *cr)
{
	struct denode	ndirent;
	dosdirent_t 	*ndirp = &ndirent.de_de;
	struct denode	*dep;
	vnode_t	*vp;
	struct denode	*pdep = VTODE(dvp);
	char            nm[DOSDIRSIZ+1];
	int		error = 0;

	/*
	 * Are we allowed to update the directory?
	 */

	if (*name == '\0')
		return EISDIR;
	
	/*
	 * The target directory must still exist in order to create
	 * a file in it.
	 */
	if (pdep->de_refcnt == 0)
		return(ENOENT);
	/*
	 * Check what they want us to do; we are not creating
	 * different things as files and directories.
	 * We return EINVAL on other stuff ... not according
	 * manual.
	 */

	switch (vap->va_type)  {

	case  VREG:	
		break;

	case  VDIR:	
		/*
		 * Note that we call dosfs_mkdir in case of a creating
		 * directory command. This make . and .., which seems
		 * to be NONE standard for mknod ... but better for
		 * DOS.
		 */

		return dosfs_mkdir(dvp, name, vap, vpp, cr);

	default:	
		return EINVAL;

	}

	DE_LOCK(pdep);

	if (error = dosfs_daccess(dvp, VWRITE, 0, cr))
	{
		DE_UNLOCK(pdep);
		return error;
	}

	/*
	 * Check whether the name exists,
	 * that might be the case.
	 */

	if (error = dosfs_dirlook(dvp, name, vpp, GETFREESLOT, NOCRED)) {
		if (error != ENOENT) {
			DE_UNLOCK(pdep);
			return error;
		}
	} else {
		vp = *vpp;
		dep = VTODE (vp);

		error = EEXIST;
	}

	/*
	 * We can accept non exclusive creates of
	 * existing files.
	 */

	if (error == EEXIST) {
		DE_UNLOCK(pdep);
		if (excl == NONEXCL) {
			if (vp->v_type == VDIR && (mode & VWRITE)) {
				DE_UNLOCK (dep);
				VN_RELE(vp);
				return(EISDIR);
			}
			if (mode) {
				error = dosfs_daccess (vp, mode, 0, cr);
			} else {
				error = 0;
			}
		}

		/*
		 * For regular files, we have to truncate the
		 * file to zero bytes, if specified.
		 */

		if (!error && (vap->va_mask & AT_SIZE) && vap->va_size == 0)
			error = detrunc (dep, 0, 0);

		DE_UNLOCK (dep);
		if(error)
			VN_RELE(vp);
		return error;
	}

	/*
	 * Create a directory entry for the file, then call
	 * createde() to have it installed.
	 * NOTE: DOS files are always executable.  We use the
	 * absence of the owner write bit to make the file readonly.
	 */

	bzero((caddr_t)&ndirent, sizeof(ndirent));

	unix2dostime(&hrestime, (union dosdate *)&ndirp->deDate,
	    (union dostime *)&ndirp->deTime);
	if(error = unix2dosfn((unsigned char *)name, ndirp->deName, strlen(name))){
		DE_UNLOCK(pdep);
		return error;
}

	ndirp->deAttributes   = (vap->va_mode & VWRITE) ? 0 : ATTR_READONLY;
	ndirp->deStartCluster = 0;
	ndirp->deFileSize     = 0;

	ndirent.de_vfs   = pdep->de_vfs;
	ndirent.de_dev   = pdep->de_dev;
	ndirent.de_devvp = pdep->de_devvp;

	if ((error = createde(&ndirent, dvp, &dep)) != 0)
		goto out;

        *vpp = DETOV(dep);

        strncpy (nm, (char *)dep->de_Name, DOSDIRSIZ);
        nm[DOSDIRSIZ] = '\0';
        dnlc_enter(dvp, nm, *vpp, NULL, NOCRED);
        pdep->de_flag |= DEUPD;
        DETIMES(pdep, &hrestime);

	DE_UNLOCK (dep);
out:
	DE_UNLOCK(pdep);

	return error;
}

/*
 *  Since DOS directory entries that describe directories
 *  have 0 in the filesize field, we take this opportunity (open)
 *  to find out the length of the directory and plug it
 *  into the denode structure.
 */
/*ARGSUSED*/
STATIC int
dosfs_open (vnode_t **vpp, int flag, cred_t *cr)
{
	int error = 0;
	unsigned long sizeinclusters;
	struct denode *dep = VTODE(*vpp);
	struct dosfs_vfs *pvp = dep->de_vfs;

	if (ISADIR(dep)) {
		DE_LOCK(dep);

		error = pcbmap(dep, 0xffff, 0, &sizeinclusters);

		if (error == E2BIG) {
			if (dep->de_StartCluster == DOSFSROOT &&
			    pvp->vfs_SectPerClust > pvp->vfs_rootdirsize) 
				dep->de_FileSize = pvp->vfs_rootdirsize *
						   pvp->vfs_BytesPerSec;
			else
				dep->de_FileSize = sizeinclusters *
			    			   pvp->vfs_bpcluster;
			error = 0;
		} else {
			cmn_err(CE_WARN, "dosfs_open: pcbmap returned %d\n", error);
		}

		DE_UNLOCK (dep);
	}

	return error;
}

/*ARGSUSED*/
STATIC int
dosfs_close (vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	struct denode *dep = VTODE(vp);

	DE_LOCK(dep);
	DETIMES (dep, &hrestime);
	if (vp->v_filocks)
		cleanlocks (vp, u.u_procp->p_epid, u.u_procp->p_sysid);
	DE_UNLOCK (dep);
	return 0;
}

/*ARGSUSED*/
STATIC int
dosfs_access (vnode_t *vp, int mode, int flags, cred_t *cr)
{
	struct denode	*dep;
	int ret;

	dep = VTODE(vp);

	DE_LOCK(dep);
	ret = dosfs_daccess (vp, mode, flags, cr);
	DE_UNLOCK(dep);

	return ret;
}

/*ARGSUSED*/
STATIC int
dosfs_lookup (vnode_t *dvp, char *name, vnode_t **vpp, pathname_t *pnp,
	      int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	struct denode   *dep;
	int error;

	dep = VTODE(dvp);

	DE_LOCK(dep);
	error = dosfs_dirlook(dvp, name, vpp, 0, cr);
	DE_UNLOCK(dep);

	if (error == 0 && *vpp != dvp)
		DE_UNLOCK(VTODE(*vpp));

	return error;
}

/*ARGSUSED*/
STATIC int
dosfs_getattr (vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	unsigned int	cn;
	struct denode	*dep;
	struct dosfs_vfs *pvp;

	dep = VTODE(vp);
	pvp = dep->de_vfs;

	DE_LOCK(dep);

	DETIMES (dep, &hrestime);

	/*
	 * The following computation of the fileid must be the
	 * same as that used in dosfs_readdir() to compute d_fileno.
	 * If not, pwd doesn't work.
	 */

	if (dep->de_Attributes & ATTR_DIRECTORY)
	{
		if ((cn = dep->de_StartCluster) == DOSFSROOT)
			cn = 1;
	}
	else
	{
		if ((cn = dep->de_dirclust) == DOSFSROOT)
			cn = 1;
		cn = (cn << 16) | (dep->de_diroffset & 0xffff);
	}

	vap->va_nodeid = cn;	/* ino like; identification of this file */
	vap->va_fsid   = dep->de_dev;
	vap->va_mode   = (dep->de_Attributes & ATTR_READONLY) ? 0555 : 0777;

	if (dep->de_Attributes & ATTR_DIRECTORY)
	{
		vap->va_mode |= S_IFDIR;
	}

	if ((flags & ATTR_EXEC) == 0)
		vap->va_mode &= ~pvp->vfs_mask;

	if (pvp->vfs_uid != DEFUSR)	vap->va_uid = pvp->vfs_uid;
	else	vap->va_uid = 0;

	if (pvp->vfs_gid != DEFGRP)	vap->va_gid = pvp->vfs_gid;
	else	vap->va_gid = 0;

	vap->va_nlink = 1;
	vap->va_rdev  = 0;
	vap->va_size  = dep->de_FileSize;

	dos2unixtime((union dosdate *)&dep->de_Date,
	    (union dostime *)&dep->de_Time, &vap->va_atime);

	vap->va_atime.tv_sec  = vap->va_atime.tv_sec;
	vap->va_atime.tv_nsec = 0;
	vap->va_mtime.tv_sec  = vap->va_atime.tv_sec;
	vap->va_mtime.tv_nsec = 0;
	vap->va_ctime.tv_sec  = vap->va_atime.tv_sec;
	vap->va_ctime.tv_nsec = 0;

	vap->va_type    = vp->v_type;
	vap->va_blksize = dep->de_vfs->vfs_bpcluster;
	vap->va_nblocks = (dep->de_FileSize + vap->va_blksize - 1) /
	    vap->va_blksize;
	if (vap->va_mask & AT_ACLCNT)
                vap->va_aclcnt = NACLBASE;

	DE_UNLOCK(dep);
	return 0;
}


/*ARGSUSED*/
STATIC int
dosfs_setattr (vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
{
	long int	mask;
	int		error;
	struct denode	*dep;
	struct dosfs_vfs *pvp;

	mask  = vap->va_mask;
	error = 0;
	dep   = VTODE(vp);
	pvp   = dep->de_vfs;


	if (pvp->vfs_ronly)
		return EROFS;

	/*
	 * Never allow nonsense ...
	 */

	if (mask & AT_NOSET)
	{
		return EINVAL;
	}


	/*
	 * Setting uid/gid (AT_UID/AT_GID) is defined as a no-op for dosfs.
	 * this allows you to use tar/cpio without massive
	 * warnings. 
	 * This is not ideal, but surely more useful.
	 */

	if (!(mask & (AT_SIZE | AT_MODE | AT_ATIME | AT_MTIME)))
	{
		return 0;
	}

	DE_LOCK(dep);

	if (mask & AT_SIZE)
	{
		if (vp->v_type == VDIR)
		{
			error = EISDIR;
			goto out;
		}

		if (error = detrunc(dep, vap->va_size, IO_SYNC))
			goto out;
	}

	if (mask & AT_MODE)
	{
		/* We ignore the read and execute bits */
		if (vap->va_mode & VWRITE)
		{
			dep->de_Attributes &= ~ATTR_READONLY;
		}
		else
		{
			dep->de_Attributes |= ATTR_READONLY;
		}

		dep->de_flag |= DEUPD;
	}

	if (mask & (AT_ATIME | AT_MTIME))
	{
		dep->de_flag |= DEUPD;
		error = deupdat (dep, &vap->va_mtime, 0);
	}
	else
	{
		error = deupdat (dep, &hrestime, 0);
	}

out:
	;
	DE_UNLOCK (dep);

	return error;
}


/*ARGSUSED*/
STATIC int
dosfs_read(vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr)
{
	int error = 0;
	int diff;
	int isadir;
	long n;
	long on;
	long bsize;
	u_long lbn;
	u_long rablock;
	lbuf_t *bp;
	struct denode *dep = VTODE(vp);
	struct dosfs_vfs *pvp = dep->de_vfs;

	ASSERT(SLEEP_LOCKOWNED(&dep->de_lock));

	/*
	 * If they didn't ask for any data, then we
	 * are done.
	 */

	if (uio->uio_resid == 0)
		return 0;
	if (uio->uio_offset < 0)
		return EINVAL;

	isadir = ISADIR(dep);
	if (isadir && dep->de_StartCluster == DOSFSROOT &&
	    pvp->vfs_SectPerClust > pvp->vfs_rootdirsize) 
		bsize = pvp->vfs_rootdirsize * pvp->vfs_BytesPerSec;
	else
		bsize = pvp->vfs_bpcluster;

	do {
		lbn = uio->uio_offset >> pvp->vfs_cnshift;
		on  = uio->uio_offset &  pvp->vfs_crbomask;
		n   = MIN((unsigned)(bsize - on), uio->uio_resid);

		diff = dep->de_FileSize - uio->uio_offset;

		if (diff <= 0)
			return 0;

		/*
		 * Convert cluster number to block number.
		 */
		if (error = pcbmap(dep, lbn, &lbn, 0))
			return error;

		if (diff < n)
			n = diff;

		/*
		 * If we are operating on a directory file then be
		 * sure to do i/o with the vnode for the filesystem
		 * instead of the vnode for the directory.
		 */

		if (isadir)
		{
			bp = lbread(pvp->vfs_dev, lbn, bsize);
			error = lgeterror(bp);
		}
		else
		{
			rablock = lbn + 1;
			if (dep->de_lastr + 1 == lbn  &&
			    rablock * pvp->vfs_bpcluster < dep->de_FileSize)
			{
				if (error = pcbmap(dep, rablock, &rablock, 0))
					return error;

				bp = lbreada(vp->v_rdev, lbn, rablock, bsize);
				error = lgeterror(bp);
			}
			else
			{
				bp = lbread (vp->v_rdev, lbn, bsize);
				error = lgeterror(bp);
			}
			dep->de_lastr = lbn;
		}

		n = MIN(n, bsize);

		if (error)
		{
			lbrelse(bp);
			return error;
		}

		error = uiomove(bp->b_un.b_addr + on, (int)n, UIO_READ, uio);

		lbrelse(bp);

	} while (error == 0  &&  uio->uio_resid > 0  && n != 0);

	DETIMES (dep, &hrestime);

	return error;
}

/*
 *  Write data to a file or directory.
 */

/*ARGSUSED*/
STATIC int
dosfs_write(vnode_t *vp, uio_t *uio, int ioflag, cred_t *cr)
{
	int n;
	int croffset;
	int error;
	u_long bn;
	lbuf_t *bp;
	off_t  foffset;
	rlim_t		limit = uio->uio_limit;
	struct denode	*dep = VTODE(vp);
	struct dosfs_vfs	*pvp = dep->de_vfs;

	ASSERT (vp->v_type == VREG);
	ASSERT(SLEEP_LOCKOWNED(&dep->de_lock));

	if (ioflag & IO_APPEND)
		uio->uio_offset = dep->de_FileSize;

	if (uio->uio_offset < 0)
		return EINVAL;

	if (uio->uio_resid == 0)
		return 0;

	/*
	 * If they've exceeded their filesize limit, tell them about it.
	 */

	if (uio->uio_offset + uio->uio_resid > limit)
		return EFBIG;

	/*
	 * If attempting to write beyond the end of the root
	 * directory we stop that here because the root directory
	 * can not grow.
	 */

	if (ISADIR(dep) && dep->de_StartCluster == DOSFSROOT  &&
	    (uio->uio_offset+uio->uio_resid) > dep->de_FileSize)
		return ENOSPC;

	/*
	 * If the offset we are starting the write at is beyond the
	 * end of the file, then, fill the hole with zero blocks.
	 */

	if (uio->uio_offset > dep->de_FileSize)
	{
		foffset = (dep->de_FileSize + (pvp->vfs_bpcluster -1))
			  & ~pvp->vfs_crbomask;

		error = dosfs_seek_l(vp, foffset, uio->uio_offset);
		if (error)
			return error;
	}


	do {
		bn = uio->uio_offset >> pvp->vfs_cnshift;

		/*
		If we are appending to the file and we are on a
		cluster boundary, then allocate a new cluster
		and chain it onto the file.
		*/

		if (uio->uio_offset == dep->de_FileSize  &&
		    (uio->uio_offset & pvp->vfs_crbomask) == 0)
		{
			if (error = extendfile(dep, &bp, 0))
				break;
		}
		else
		{
			/*
			 * The block we need to write into exists,
			 * so just read it in.
			 */

			if (error = pcbmap(dep, bn, &bn, 0))
				return error;

			bp = lbread (vp->v_rdev, bn, pvp->vfs_bpcluster);

			if (error = lgeterror(bp))
			{
				lbrelse(bp);
				return error;
			}
		}

		croffset = uio->uio_offset & pvp->vfs_crbomask;
		n        = MIN(uio->uio_resid, pvp->vfs_bpcluster-croffset);

		if (uio->uio_offset+n > dep->de_FileSize)
		{
			dep->de_FileSize = uio->uio_offset + n;
		}

		/*
		 * Copy the data from user space into the buf header.
		 */

		error = uiomove(bp->b_un.b_addr+croffset, n, UIO_WRITE, uio);

		/*
		 * If they want this synchronous then write it and wait
		 * for it.  Otherwise, if on a cluster boundary write it
		 * asynchronously so we can move on to the next block
		 * without delay.  Otherwise do a delayed write because
		 * we may want to write somemore into the block later.
		 */

		if ((ioflag & IO_SYNC))
		{
			error = lbwritewait(bp);
		}
		else if (n + croffset == pvp->vfs_bpcluster)
		{
			lbawrite(bp);
		}
		else
		{
			lbdwrite(bp);
		}

		dep->de_flag |= DEUPD;

	} while (error == 0  &&  uio->uio_resid > 0);

	if (!error && (ioflag & IO_SYNC))
	{
		(void) deupdat(dep, &hrestime, 0);

	} 

	return error;
}

/*ARGSUSED*/
STATIC int
dosfs_ioctl (vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
{
	return ENOTTY;
}

/*
 *  Flush the blocks of a file to disk.
 *
 *  This function is worthless for vnodes that represent
 *  directories.
 *  Maybe we could just do a sync if they try an fsync
 *  on a directory file.
 */

/*ARGSUSED*/
STATIC int
dosfs_fsync (vnode_t *vp, cred_t *cr)
{
	struct denode *dep = VTODE(vp);
	int err;

	DE_LOCK(dep);
	dep->de_flag |= DEUPD;

	err =  deupdat (dep, &hrestime, 1);

	DE_UNLOCK(dep);
	return err;
}

/*
 *  Since the dos filesystem does not allow files with
 *  holes in them we must fill the file with zeroed
 *  blocks when a seek past the end of file happens.
 */
/*ARGSUSED*/
int
dosfs_seek_l (vnode_t *vp, off_t foff, off_t noff)
{
	int error = 0;
	lbuf_t *bp;
	struct denode *dep = VTODE(vp);
	struct dosfs_vfs *pvp = dep->de_vfs;


	/*
	Allocate and chain together as many clusters as
	are needed to get to noff.
	*/

	while (foff < noff)
	{
		if (error = extendfile(dep, &bp, 0))
		{
			break;
		}

		dep->de_flag |= DEUPD;
		lbdwrite(bp);
		foff += pvp->vfs_bpcluster;
		dep->de_FileSize += pvp->vfs_bpcluster;
	}

	dep->de_FileSize = noff;
	error = deupdat(dep, &hrestime, 1);

	return error;
}

/*ARGSUSED*/
STATIC int
dosfs_seek (vnode_t *vp, off_t oldoff, off_t *newoff)
{
	int error = 0;
	off_t foff;
	struct denode *dep = VTODE(vp);
	struct dosfs_vfs *pvp = dep->de_vfs;

	if (*newoff < 0) {
		return EINVAL;
	}

	DE_LOCK(dep);
	/*
	 * Compute the offset of the first byte after the
	 * last block in the file.
	 * If seeking beyond the end of file then fill the
	 * file with zeroed blocks up to the seek address.
	 */

	foff = (dep->de_FileSize + (pvp->vfs_bpcluster-1)) & ~pvp->vfs_crbomask;

	if (*newoff > foff) {
		/*
		 * If this is the root directory and we are
		 * attempting to seek beyond the end disallow
		 * it.  DOS filesystem root directories can
		 * not grow.
		 */

		if (vp->v_flag & VROOT)
		{
			error = EINVAL;
			goto out;
		}

		/*
		 * If this is a directory and the caller is not
		 * root, then do not let them seek beyond the end
		 * of file.  If we allowed this then users could
		 * cause directories to grow.  Is this really that
		 * important?
		 */

		if (ISADIR(dep) && pm_denied(u.u_lwpp->l_cred, P_FILESYS))
		{
			error =  EPERM;
			goto out;
		}

		error = dosfs_seek_l (vp, foff, *newoff);

	}
out:

	DE_UNLOCK(dep);
	return error;
}

/*ARGSUSED*/
STATIC int
dosfs_remove(vnode_t *dvp, char *name, cred_t *cr)
{
	int		error;
	vnode_t	*vp;
	denode_t	*pdep;
	char		nm[DOSDIRSIZ+1];

	pdep = VTODE (dvp);

	DE_LOCK(pdep);

	/*
	 * Are we allowed to update the directory?
	 */

	if (error = dosfs_daccess(dvp, VWRITE, 0, cr)) {
		DE_UNLOCK(pdep);
		return error;
	}


	if (error = dosfs_dirlook(dvp, name, &vp, 0, cr)) {
		DE_UNLOCK (pdep);
		return error;
	}

	/*
	 * No mount points can be removed this way.
	 * Don't remove files on used for swapping;
	 * who is goin to use dos-files as swap-area?
	 */

	if (vp->v_vfsmountedhere != NULL) {
		DE_UNLOCK (pdep);
		deput(VTODE(vp));
		return EBUSY;
	}

	/*
	 * Only super users can unlink directories.
	 */

	if (vp->v_type == VDIR && pm_denied(cr, P_FILESYS)) {
		DE_UNLOCK (pdep);
		deput(VTODE(vp));
		return EPERM;
	}

        if (vp->v_type == VDIR) {
		if(error = unix2dosfn((unsigned char *)"..", nm, 2)) {
			DE_UNLOCK (pdep);
			deput(VTODE(vp));
			return (error);
		}

                nm[DOSDIRSIZ] = '\0';
                dnlc_remove(vp, (char *)nm);
        }

	strncpy ((char *)nm, (char *)VTODE(vp)->de_Name, DOSDIRSIZ);
	nm[DOSDIRSIZ] = '\0';
	dnlc_remove(dvp, (char *)nm);

	error = removede (vp);

	if (!error) {
		pdep->de_flag |= DEUPD;
		DETIMES(pdep, &hrestime);
	}

	DE_UNLOCK (pdep);
	return error;
}

/*
 *  Renames on files require moving the denode to
 *  a new hash queue since the denode's location is
 *  used to compute which hash queue to put the file in.
 *  Unless it is a rename in place.  For example "mv a b".
 *
 *  What follows is the basic algorithm:
 */
/*
 *	if (file move) {
 *		if (dest file exists) {
 *			remove dest file
 *		}
 *		if (dest and src in same directory) {
 *			rewrite name in existing directory slot
 *		} else {
 *			write new entry in dest directory
 *			update offset and dirclust in denode
 *			move denode to new hash chain
 *			clear old directory entry
 *		}
 */
/*
 *	} else {  directory move
 *		if (dest directory exists) {
 *			if (dest is not empty) {
 *				return ENOTEMPTY
 *			}
 *			remove dest directory
 *		}
 *		if (dest and src in same directory) {
 *			rewrite name in existing entry
 *		} else {
 *			be sure dest is not a child of src directory
 *			write entry in dest directory
 *			update "." and ".." in moved directory
 *			update offset and dirclust in denode
 *			move denode to new hash chain
 *			clear old directory entry for moved directory
 *		}
 *	}
 *
 */
/*
 *  On entry:
 *    nothing is locked
 *
 *  On exit:
 *    all denodes should be released
 */

STATIC int
dosfs_rename (vnode_t *fdvp, char *fname, vnode_t *tdvp, char *tname, cred_t *cr)

/* old parent vnode (from) */
/* old entry name (from) */
/* new parent vnode (to) */
/* new entry name (to) */

{
	unsigned char	toname[DOSDIRSIZ+1];
	unsigned char	oldname[DOSDIRSIZ+1];
	int		error;
	int		newparent;
	int		srcisdir;
	unsigned long	to_dirclust;
	unsigned long	to_diroffset;
	unsigned long	cn;
	u_long		bn;
	struct denode	*fddep;	/* from file's parent directory	*/
	struct denode	*tddep;	/* to file's parent directory	*/
	struct denode	*fdep   = NULL;	/* from file or directory	*/
	struct denode	*tdep   = NULL;	/* to file or directory		*/
	struct denode	*newdep = NULL; /* the new denode made		*/
	vnode_t	*fvp = NULL;	/* from file vnode		*/
	vnode_t	*tvp = NULL;	/* to file vnode		*/
	struct dosfs_vfs	*pvp;
	dosdirent_t	*dotdotp;
	dosdirent_t	*ep;
	lbuf_t	*bp;
	boolean_t hold_rename_lock = B_FALSE;

	fddep = VTODE(fdvp);
	tddep = VTODE(tdvp);

	DE_LOCK(fddep);

	/*
	 * Ensure there's something like a source with
	 * can be renamed.
	 */
	if (error = dosfs_daccess(fdvp, VWRITE, 0, cr)) {
		DE_UNLOCK(fddep);
		return error;
	}

	if (error = dosfs_dirlook(fdvp, fname, &fvp, 0, cr))
	{
		DE_UNLOCK(fddep);
		return error;
	}

	/*
	 * If renaming a directory to itself, it a no-op. Short cut
         * the rename,
         */
	if (strcmp(fname, ".") == 0  || strcmp(fname, "..") == 0 ||
							fvp == tdvp) {
		DE_UNLOCK(fddep);
		deput(VTODE(fvp));
		return EINVAL;
	}

	if (tdvp != fdvp) {
		DE_LOCK(tddep);
		if (error = dosfs_daccess(tdvp, VWRITE, 0, cr))
		{
			DE_UNLOCK(fddep);
			DE_UNLOCK(tddep);
			deput(VTODE(fvp));
			return error;
		}
	} else if (!strcmp(fname, tname)) {
		DE_UNLOCK(fddep);
		deput(VTODE(fvp));
		return 0;	/* source == destiny; for sure */
	}

	fdep  = VTODE(fvp);
	oldname[0] = '\0';
	strncat ((char *)oldname, (char *)fdep->de_Name, DOSDIRSIZ);

	pvp = fddep->de_vfs;

	/*
	 * Check the destination, it may not exist.
	 */

	if (error = dosfs_dirlook(tdvp, tname, &tvp, GETFREESLOT, cr))
	{
		if (error != ENOENT)
			goto out;
			

		/*
		 * Convert the filename in tdnp into a dos filename.
		 * We copy this into the denode and directory entry
		 * for the destination file/directory.
		 */

		if(error = unix2dosfn((unsigned char *)tname, toname, strlen(tname)))
			goto out;

	}
	else
	{
		/*
		 * Both sides exist, they must be
		 * the same type.
		 */

		tdep = VTODE(tvp);

		if (ISADIR(fdep) && !ISADIR(tdep))
		{
			error = ENOTDIR;
			goto out;
		}

		if (!ISADIR(fdep) && ISADIR(tdep))
		{
			error = EISDIR;
			goto out;
		}

		/*
		 * The destination should neither be
		 * a mount point, nor a swap file.
		 */

		if (tvp->v_vfsmountedhere != NULL)
		{
			error = EBUSY;
			goto out;
		}

		toname[0] = '\0';
		strncat ((char *)toname, (char *)tdep->de_Name, DOSDIRSIZ);
	}

	/*
	 * Be sure we are not renaming ".", "..", or an alias of ".".
	 * This leads to a crippled directory tree.  It's pretty tough
	 * to do a "ls" or "pwd" with the "." directory entry missing,
	 * and "cd .." doesn't work if the ".." entry is missing.
	 */

	if (ISADIR(fdep))
	{
		srcisdir = 1;
	}
	else
	{
		srcisdir = 0;
	}

	/*
	 * If we are renaming a directory, and the directory
	 * is being moved to another directory, then we must
	 * be sure the destination directory is not in the
	 * subtree of the source directory.  This could orphan
	 * everything under the source directory.
	 */

	/*
	 * fddep != tddep will do??? we don't have
	 * links. What happens with "." entries???
	 */

	newparent = fddep->de_StartCluster != tddep->de_StartCluster;

	if (srcisdir && newparent)
	{
		SLEEP_LOCK(&pvp->vfs_rename_lock, PRINOD);
		hold_rename_lock = B_TRUE;
		if (error = doscheckpath (fdep, tddep))
		{
			goto out;
		}
	}

	/*
	 * If the destination exists and if it is a
	 * directory make sure it is empty.
	 */

	if (tdep)
	{
		if (srcisdir && !dosdirempty(tdep))
		{
			error = ENOTEMPTY;
			goto out;
		}

		to_dirclust  = tdep->de_dirclust;
		to_diroffset = tdep->de_diroffset;

		error = removede(tvp);
		/*
		 * removede() unlocks the denode "tdep"
		 * and releases the vnode "tvp"
		 * so, we must be sure it will not be done again!
		 */
		tdep = NULL;
		tvp  = NULL;
		if (error)
			goto out;

		/*
		 * Remember where the slot was for createde().
		 */

		tddep->de_frspace   = 1;
		tddep->de_froffset  = to_diroffset;
		tddep->de_frcluster = to_dirclust;
	}

	/*
	 * If the source and destination are in the same
	 * directory then just read in the directory entry,
	 * change the name in the directory entry and
	 * write it back to disk.
	 */

	dnlc_remove(fdvp, (char *)oldname);
	dnlc_remove(tdvp, (char *)toname);

	if (!newparent)
	{
		if (error = readde(fdep, &bp, &ep))
		{
			/* readde() does lbrelse() on error */
			goto out;
		}

		strncpy ((char *)ep->deName, (char *)toname, DOSDIRSIZ);

		if (error = lbwritewait(bp))
		{
			goto out;
		}

		strncpy((char *)fdep->de_Name, (char *)toname, DOSDIRSIZ);

		newdep = fdep;
	}
	else
	{
		/*
		 * If the source and destination are in different
		 * directories, then mark the entry in the source
		 * directory as deleted and write a new entry in the
		 * destination directory.  Then move the denode to
		 * the correct hash chain for its new location in
		 * the filesystem.  And, if we moved a directory,
		 * then update its .. entry to point to the new
		 * parent directory.
		 */

		strncpy ((char *)fdep->de_Name, (char *)toname, DOSDIRSIZ);

		if (error = createde(fdep, tdvp, NULL))
		{
			/* should put back filename */
			goto out;
		}
		newdep = fdep;

		/*
		 * Read the source directory entry
		 * ep points to a place in the
		 * buffer bp
		 */

		if (error = readde(fdep, &bp, &ep))
		{
			goto out;
		}

		ep->deName[0] = SLOT_DELETED;

		if (error = lbwritewait(bp))
		{
			goto out;
		}

		fdep->de_dirclust  = tddep->de_frcluster;
		fdep->de_diroffset = tddep->de_froffset;

		reinsert(fdep);
	}

	/*
	 * If we moved a directory to a new parent directory,
	 * then we must fixup the ".." entry in the moved
	 * directory.
	 */

	if (srcisdir  &&  newparent)
	{
		cn = fdep->de_StartCluster;

		if (cn == DOSFSROOT)
		{
			/* this should never happen */
			cmn_err (CE_PANIC, "dosfs_rename: updating .. in root directory?\n");
		}
		else
		{
			bn = cntobn(pvp, cn);
		}

		bp = lbread(pvp->vfs_dev, bn, pvp->vfs_bpcluster);

		if (error = lgeterror(bp))
		{
			lbrelse(bp);
			/* should really panic here, fs is corrupt */
			goto out;
		}

		dotdotp = (dosdirent_t *)bp->b_un.b_addr + 1;
		dotdotp->deStartCluster = tddep->de_StartCluster;

		if (error = lbwritewait(bp))
		{
			goto out;
		}
	}

	/*
        Get the directory cache upto date; remove
        old name and insert the now one.
        */
	dnlc_enter(tdvp, (char *)toname, DETOV(newdep), NULL, NOCRED);
out:
	if (fdep)
		DE_UNLOCK (fdep);
	if (tdep)
		DE_UNLOCK (tdep);

	if (newdep && newdep != fdep)
	{
		deput (newdep);
	}
	if (hold_rename_lock)
		SLEEP_UNLOCK(&pvp->vfs_rename_lock);

	if (!error) {
		fddep->de_flag |= DEUPD;
		DETIMES(fddep, &hrestime);
		if (tdvp != fdvp) {
			tddep->de_flag |= DEUPD;
			DETIMES(tddep, &hrestime);
		}
	}

	DE_UNLOCK (fddep);

	if (tdvp != fdvp)
		DE_UNLOCK (tddep);

	if (fvp)
		VN_RELE (fvp);
	if (tvp)
		VN_RELE (tvp);

	return error;
}

struct {
	dosdirent_t dot;
	dosdirent_t dotdot;
} dosdirtemplate = {
		".       ", "   ",		/* the . entry */
		ATTR_DIRECTORY,			/* file attribute */
		0,0,0,0,0,0,0,0,0,0,		/* resevered */
		1234, 1234,			/* time and date */
		0,				/* startcluster */
		0,				/* filesize */
		"..      ", "   ",		/* the .. entry */
		ATTR_DIRECTORY,			/* file attribute */
		0,0,0,0,0,0,0,0,0,0,		/* resevered */
		1234, 1234,			/* time and date */
		0,				/* startcluster */
		0,				/* filesize */
};



/*ARGSUSED*/
STATIC int
dosfs_mkdir(vnode_t *dvp, char *name, vattr_t *vap, vnode_t **vpp, cred_t *cr)
{
	int bn;
	int error;
	unsigned long newcluster;
	struct denode *pdep;
	struct denode *ndep;
	dosdirent_t *denp;
	struct denode ndirent;
	struct dosfs_vfs *pvp;
	lbuf_t *bp;
	char            nm[DOSDIRSIZ+1];

	if (*name == '\0')
		return EISDIR;

	pdep = VTODE(dvp);
	DE_LOCK(pdep);

	/*
	 * Are we allowed to update the directory?
	 */

	if (error = dosfs_daccess(dvp, VWRITE, 0, cr)) {
		DE_UNLOCK (pdep);
		return error;
	}

	if (error = dosfs_dirlook(dvp, name, vpp, GETFREESLOT, NOCRED)) {
		if (error != ENOENT) {
			DE_UNLOCK (pdep);
			return error;
		}
	} else {
		deput(VTODE(*vpp));
		DE_UNLOCK (pdep);
		return EEXIST;
	}

	/*
	 * If this is the root directory and there is no space left
	 * we can't do anything.  This is because the root directory
	 * can not change size.
	 */

	if (pdep->de_StartCluster == DOSFSROOT  &&  !pdep->de_frspace)
	{
		DE_UNLOCK(pdep);
		return ENOSPC;
	}

	pvp = pdep->de_vfs;

	/*
	 * Allocate a cluster to hold the about to be created directory.
	 */

	if (error = clusteralloc(pvp, &newcluster, CLUST_EOFE))
	{
		DE_UNLOCK(pdep);
		return error;
	}

	/*
	 * Now fill the cluster with the "." and ".." entries.
	 * And write the cluster to disk.  This way it is there
	 * for the parent directory to be pointing at if there
	 * were a crash.
	 */

	bn = cntobn(pvp, newcluster);
	bp = lgetblk(pvp->vfs_dev, bn, pvp->vfs_bpcluster);

	bzero(bp->b_un.b_addr, pvp->vfs_bpcluster);
	bcopy((caddr_t)&dosdirtemplate, bp->b_un.b_addr, sizeof dosdirtemplate);
	denp = (dosdirent_t *)bp->b_un.b_addr;
	denp->deStartCluster = newcluster;
	unix2dostime(&hrestime, (union dosdate *)&denp->deDate,
	    (union dostime *)&denp->deTime);
	denp++;
	denp->deStartCluster = pdep->de_StartCluster;
	unix2dostime(&hrestime, (union dosdate *)&denp->deDate,
	    (union dostime *)&denp->deTime);

	if (error = lbwritewait(bp))
	{
		(void)clusterfree(pvp, newcluster);
		DE_UNLOCK(pdep);
		return error;
	}

	/*
	 * Now build up a directory entry pointing to the newly
	 * allocated cluster.  This will be written to an empty
	 * slot in the parent directory.
	 */

	ndep = &ndirent;
	bzero((caddr_t)ndep, sizeof(*ndep));
	if(error = unix2dosfn((unsigned char *)name, ndep->de_Name, strlen(name))) {
		(void)clusterfree(pvp, newcluster);
		DE_UNLOCK(pdep);
		return(error);
	}
	unix2dostime(&hrestime, (union dosdate *)&ndep->de_Date,
	    (union dostime *)&ndep->de_Time);

	ndep->de_StartCluster = newcluster;
	ndep->de_Attributes   = ATTR_DIRECTORY;
	ndep->de_FileSize   = pvp->vfs_bpcluster;
	ndep->de_vfs          = pvp;	/* createde() needs this	*/

	if (error = createde(ndep, dvp, &ndep))
	{
		(void)clusterfree(pvp, newcluster);
	}
	else
	{
		DE_UNLOCK (ndep);
		*vpp = DETOV(ndep);
                strncpy (nm, (char *)ndep->de_Name, DOSDIRSIZ);
                nm[DOSDIRSIZ] = '\0';
                dnlc_enter(dvp, nm, *vpp, NULL, NOCRED);
		pdep->de_flag |= DEUPD;
		DETIMES(pdep, &hrestime);
	}

	DE_UNLOCK (pdep);

	return error;
}

/*ARGSUSED*/
STATIC int
dosfs_rmdir(vnode_t *dvp, char *name, vnode_t *cdir, cred_t *cr)
{
	struct denode	*pdep;
	struct denode	*dep;
	vnode_t	*vp;
	char		nm[DOSDIRSIZ+1];
	int		error;

	/*
	 * Are we allowed to update the directory?
	 */

	pdep = VTODE(dvp);	/* parent dir of dir to delete	*/
	DE_LOCK(pdep);

	if (error = dosfs_daccess(dvp, VWRITE, 0, cr)) {
		DE_UNLOCK(pdep);
		return error;
	}

	if (error = dosfs_dirlook(dvp, name, &vp, 0, cr)) {
		DE_UNLOCK(pdep);
		return error;
	}

	dep  = VTODE(vp);	/* directory to delete	*/

	/*
	 * It should be a directory.
	 */

	if (!ISADIR(dep))
	{
		error = ENOTDIR;
		goto out;
	}

	/*
	 * Don't remove a mount-point.
	 */

	if (vp->v_vfsmountedhere != NULL)
	{
		error = EBUSY;
		goto out;
	}

	/*
	 * Don't let "rmdir ." go thru.
	 */

	if (dvp == vp)
	{
		error = EINVAL;
		goto out;
	}

	/*
	 * Be sure the directory being deleted is empty.
	 */

	if (!dosdirempty(dep))
	{
		error = ENOTEMPTY;
		goto out;
	}

        if ((error = unix2dosfn ((unsigned char *)"..", nm, 2)) != 0)
		goto out;

        nm[DOSDIRSIZ] = '\0';
        dnlc_remove(vp, (char *)nm);

	strncpy ((char *)nm, (char *)dep->de_Name, DOSDIRSIZ);
	nm[DOSDIRSIZ] = '\0';

	dnlc_remove(dvp, (char *)nm);

	/*
	 * Delete the entry from the directory.  For dos filesystems
	 * this gets rid of the directory entry on disk, the in memory
	 * copy still exists but the de_refcnt is <= 0.  This prevents
	 * it from being found by deget().  When the deput() on dep is
	 * done we give up access.
	 */

	error = removede(vp);

	if (!error) {
		pdep->de_flag |= DEUPD;
		DETIMES(pdep, &hrestime);
	}
	DE_UNLOCK(pdep);
	return error;

out:
	DE_UNLOCK(pdep);
	deput(dep);
	return error;
}

/*
 * dosfs_readdir: convert a dos directory to a fs-independent type
 *		 and pass it to the user.
 */

/*ARGSUSED*/
STATIC int
dosfs_readdir(vnode_t *vp, uio_t *uio, cred_t *cr, int *eofflagp)
{
	int		error;
	int		diff;
	long		n;
	long		on;
	long		count;
	int		len;		/* lenght of entry name */
	int 		lastslot;	/* end of directory indication */
	off_t		offset;		/* directory offset in bytes */
	u_long		cn;
	u_long		fileno;		/* kind of ino for dosfs */
	long		bias;		/* for handling "." and ".." */
	u_long		bn;
	u_long		lbn;
	int		fixlen;		/* fixed length of dirent   */
	int		maxlen;		/* maximum length of dirent */
	addr_t		end;
	lbuf_t		*bp;
	struct denode	*dep;
	struct dosfs_vfs	*pvp;
	dosdirent_t	*dentp;
	struct dirent	*crnt;
	unsigned char	dirbuf[512];	/* holds converted dos directories */


	dep = VTODE(vp);
	pvp = dep->de_vfs;

	ASSERT(SLEEP_LOCKOWNED(&dep->de_lock));

	/*
	 * dosfs_readdir() won't operate properly on regular files
	 * since it does i/o only with the the filesystem vnode,
	 * and hence can retrieve the wrong block from the buffer
	 * cache for a plain file.  So, fail attempts to readdir()
	 * on a plain file.
	 */

	if (!ISADIR(dep))
		return ENOTDIR;

	/*
	 * If the user buffer is smaller than the size of one dos
	 * directory entry or the file offset is not a multiple of
	 * the size of a directory entry, then we fail the read.
	 */

	bp     = NULL;
	offset = uio->uio_offset;
	count  = uio->uio_resid;
	fixlen = (int) ((struct dirent *) 0)->d_name;
	maxlen = fixlen + DOSDIRSIZ + 1;

	if (offset & (sizeof(dosdirent_t)-1))
	{
		return ENOENT;
	}

	if (count < maxlen)
	{
		return EINVAL;
	}

	bias 		      = 0;
	lastslot 	      = 0;
	error		      = 0;

	uio->uio_iov->iov_len = count; /* does this assume vector big enough? */

	crnt  		      = (struct dirent *)dirbuf;

	/*
	 * If they are reading from the root directory then,
	 * we simulate the . and .. entries since these don't
	 * exist in the root directory.  We also set the offset
	 * bias to make up for having to simulate these entries.
	 * By this I mean that at file offset 64 we read the first entry
	 * in the root directory that lives on disk.
	 */

	if (dep->de_StartCluster == DOSFSROOT)
	{
		bias = 2 * sizeof(dosdirent_t);

		if (offset < bias)
		{
			/*
			 * Write the "." entry ?
			 */

			if (offset < sizeof(dosdirent_t))
			{
				offset = sizeof(dosdirent_t);
				crnt->d_ino    = 1;
				crnt->d_reclen = fixlen + 2;
				crnt->d_off    = offset;

				strcpy (crnt->d_name, ".");

				count  -= crnt->d_reclen;
				crnt = (dirent_t *)((char*)crnt+crnt->d_reclen);
			}

			if (count >= maxlen)
			{
				/*
				 * Write the ".." entry.
				*/

				offset += sizeof(dosdirent_t);
				crnt->d_ino    = 0;
				crnt->d_reclen = fixlen + 3;
				crnt->d_off    = offset;

				strcpy (crnt->d_name, "..");

				count  -= crnt->d_reclen;
				crnt = (dirent_t *)((char*)crnt+crnt->d_reclen);
			}
		}
	}

	do {
		/*
		 * Stop if we did do everything.
		 */

		if ((diff = dep->de_FileSize - (offset - bias)) <= 0)
		{
			break;
		}

		lbn = (offset - bias) >> pvp->vfs_cnshift;
		on  = (offset - bias) &  pvp->vfs_crbomask;
		n   = (uint)(pvp->vfs_bpcluster - on);

		if (diff < n)
		{
			n = diff;
		}

		if (error = pcbmap(dep, lbn, &bn, &cn))
			break;

		bp = lbread (pvp->vfs_dev, bn, pvp->vfs_bpcluster);

		if (error = lgeterror(bp))
		{
			lbrelse(bp);
			break;
		}

		n = MIN(n, pvp->vfs_bpcluster);

		/*
		 * We assume that an integer number of dosentries
		 * fits in a cluster block.
		 */

		if (n < sizeof(dosdirent_t))
		{
			lbrelse(bp);
			error = EIO;
			break;
		}

		/*
		 * Code to convert from dos directory entries to
		 * fs-independent directory entries.
		 */

		dentp = (dosdirent_t *)(bp->b_un.b_addr + on);
		end   = bp->b_un.b_addr + on + n;

		for (; (addr_t)dentp < end && count >= maxlen; dentp++,
		    offset += sizeof(dosdirent_t))
		{
			/*
			 * If we have a slot from a deleted file, or a volume
			 * label entry just skip it.
			 * If the entry is empty then set a flag saying finish
			 * in the directory.
			 */

			if (dentp->deName[0] == SLOT_EMPTY)
			{
				lastslot = 1;
				break;
			}

			if (dentp->deName[0] == SLOT_DELETED  ||
			    (dentp->deAttributes & ATTR_VOLUME))
			{
				continue;
			}

			/*
			 * This computation of d_fileno must match the
			 * computation of va_fileid in dosfs_getattr
			 */

			if (dentp->deAttributes & ATTR_DIRECTORY)
			{
				/*
				 * If this is the root directory
				 */
				fileno = dentp->deStartCluster;
				if (fileno == DOSFSROOT)
					fileno = 1;
			}
			else
			{
				/*
				 * If the file's dirent lives in root dir
				 */
				if ((fileno = cn) == DOSFSROOT)
					fileno = 1;
				fileno = (fileno << 16) | ((dentp -
				    (dosdirent_t *)bp->b_un.b_addr) & 0xffff);
			}

			if(error = dos2unixfn(dentp->deName, (u_char *)crnt->d_name, &len))
				break;

			crnt->d_ino    = fileno;
			crnt->d_reclen = fixlen + len + 1;
			crnt->d_off    = offset + sizeof(dosdirent_t);

			count -= crnt->d_reclen;

			crnt = (dirent_t *)((char *) crnt + crnt->d_reclen);

			/*
			 * If our intermediate buffer is full then copy
			 * its contents to user space.  I would just
			 * use the buffer the buf header points to but,
			 * I'm afraid that when we lbrelse() it someone else
			 * might find it in the cache and think its contents
			 * are valid.  Maybe there is a way to invalidate
			 * the buffer before lbrelse()'ing it.
			 */

			if (((u_char *)crnt)+maxlen >= &dirbuf[sizeof dirbuf])
			{
				n     = (u_char*) crnt - dirbuf;
				error = uiomove(dirbuf, n, UIO_READ, uio);
				if (error)
					break;
				crnt = (struct dirent *)dirbuf;
			}
		}

		lbrelse(bp);

	} while (!lastslot  &&  error == 0  &&  count >= maxlen);

	/*
	 * Move the rest of the directory
	 * entries to the user.
	 */

	if (!error  &&  (n = (u_char*) crnt - dirbuf))
	{
		error = uiomove(dirbuf, n, UIO_READ, uio);
	}

	uio->uio_offset = offset;

	/*
	 * I don't know why we bother setting this eofflag, getdents()
	 * doesn't bother to look at it when we return.
	 */

	if (lastslot || ((dep->de_FileSize - (uio->uio_offset - bias)) <= 0))
	{
		*eofflagp = 1;
	}
	else
	{
		*eofflagp = 0;
	}

	return error;
}

/*ARGSUSED*/
STATIC int
dosfs_lock(vnode_t *vp, off_t off, off_t len, int fmode, int mode)
{
	struct denode *dep = VTODE(vp);

	DE_LOCK(dep);
	return 0;
}

/*ARGSUSED*/
STATIC void
dosfs_unlock(vnode_t *vp, off_t off, off_t len)
{
	struct denode *dep = VTODE(vp);

	DE_UNLOCK(dep);
}

STATIC int
dosfs_frlock (vnode_t *vp, int cmd, flock_t *flp, int flag, off_t offset, cred_t *cr)
{
	return fs_frlock (vp, cmd, flp, flag, offset, cr,
					VTODE(vp)->de_FileSize);
}

STATIC int
dosfs_fid (vnode_t *vp, struct fid **fidpp)
{
	denode_t* dp;
	dfid_t*	  dfid;

	dp   = VTODE(vp);
	dfid = (dfid_t *) kmem_alloc (sizeof (dfid_t), KM_SLEEP);

	dfid->dfid_len        = sizeof (dfid_t) - sizeof (dfid->dfid_len);
	dfid->dfid_gen        = DE_GEN(dp);
	dfid->dfid_dirclust   = dp->de_dirclust;
	dfid->dfid_diroffset  = dp->de_diroffset;
	dfid->dfid_startclust = dp->de_StartCluster;

	if (ISADIR(dp)) {
		if (dfid->dfid_diroffset > (u_short)(DOSFS_FID_DIRFLAG - 1)) { 
			cmn_err(CE_WARN,
			    "dosfs_fid: subdirectory %s has too many entries",
	    							dp->de_Name);
		}
		dfid->dfid_diroffset  |= DOSFS_FID_DIRFLAG;
	}

	*fidpp = (struct fid *) dfid;
	return 0;
}


vnodeops_t dosfs_vnodeops = {
		dosfs_open,
		dosfs_close,
		dosfs_read,
		dosfs_write,
		dosfs_ioctl,
		fs_setfl,
		dosfs_getattr,
		dosfs_setattr,
		dosfs_access,
		dosfs_lookup,
		dosfs_create,
		dosfs_remove,
		(int (*)())fs_nosys,	/* link */
		dosfs_rename,
		dosfs_mkdir,
		dosfs_rmdir,
		dosfs_readdir,
		(int (*)())fs_nosys,	/* symlink */
		(int (*)())fs_nosys,	/* readlink */
		dosfs_fsync,
		dosfs_inactive,
		(void (*)())fs_nosys,	/* release */
		dosfs_fid,
		dosfs_lock,
		dosfs_unlock,
		dosfs_seek,
		fs_cmp,
		dosfs_frlock,
		(int (*)())fs_nosys, 	/* realvp */
		(int (*)())fs_nosys,	/* getpage, */
		(int (*)())fs_nosys,	/* putpage, */
		(int (*)())fs_nosys,	/* map, */
		(int (*)())fs_nosys,	/* addmap, */
		(int (*)())fs_nosys,	/* delmap, */
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

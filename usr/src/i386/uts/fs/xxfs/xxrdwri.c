#ident	"@(#)kern-i386:fs/xxfs/xxrdwri.c	1.1.2.1"

#include <fs/buf.h>
#include <fs/file.h>
#include <fs/fski.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/xxfs/xxinode.h>
#include <fs/xxfs/xxmacros.h>
#include <fs/xxfs/xxparam.h>
#include <io/conf.h>
#include <io/uio.h>
#include <proc/resource.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern	int	xx_bmap(inode_t *, daddr_t, daddr_t *, daddr_t *, int);

/*
 * int
 * xx_readi(inode_t *ip, uio_t *uiop, int ioflag)
 *	Read the file corresponding to the supplied inode.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in at least *shared*
 *	mode. This lock is must be acquired from above the VOP interface
 *	via VOP_RWRDLOCK() (below the VOP interface use xx_rwlock).
 *
 *	A return value of 0 indicates success; othwerise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	offset in <uio> is negative.
 *
 * Description:
 *	Perform a pre-V4 style read through the buffer cache.
 */
/* ARGSUSED */
int
xx_readi(inode_t *ip, uio_t *uiop, int ioflag)
{
	buf_t	*bp;
	daddr_t	bn;
	daddr_t	lbn;
	dev_t	dev;
	int	bsize;
	int	diff;
	int	error;
	long	oresid;
	off_t	isize;
	off_t	off;
	xx_fs_t	*xxfsp;
	uint_t	n;
	uint_t	on;
	vnode_t	*vp;

	error = 0;
	isize = ip->i_size;
	off = uiop->uio_offset;
	oresid = uiop->uio_resid;
	vp = ITOV(ip);
	xxfsp = XXFS(vp->v_vfsp);
	bsize = VBSIZE(vp);

	if (uiop->uio_resid == 0) {
		return (0);
	}
	if (uiop->uio_offset < 0) {
		return (EINVAL);
	}
	do {
		diff = isize - off;
		/*
		 * Compute n, the number of bytes which
		 * can be read from this mapping.
		 */
		lbn = lblkno(xxfsp, uiop->uio_offset);
		on = blkoff(xxfsp, uiop->uio_offset);

		n = MIN(bsize - on, uiop->uio_resid);
		if (diff <= 0) {
			break;
		}
		if (diff < n) {
			n = diff;
		}

		error = xx_bmap(ip, lbn, &bn, (daddr_t *)NULL, B_READ);
		if (error) {
			return (error);
		}
		bn = LTOPBLK(bn, bsize);
		dev = ip->i_dev;
		if ((long) bn < 0) {
			/* hole in file */
			bp = ngeteblk((long)bsize);
			clrbuf(bp);
		} else {
			bp = bread(dev, bn, bsize);
		}
		ip->i_nextr += bsize;
		n = MIN(n, bsize - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto err;
		}
		error = uiomove(bp->b_un.b_addr + on, n, UIO_READ, uiop);
		if (!error) {
			off = uiop->uio_offset;
			if ((n + on) == bsize || off == isize) {
				bp->b_flags |= B_AGE;
			}
		}
		brelse(bp);
	} while (error == 0 && uiop->uio_resid > 0);

	/* check if it's a partial read, terminate without error */
	if (oresid != uiop->uio_resid) {
		error = 0;
	}

err:
	return (error);
}

/*
 * int
 * xx_writei(inode_t *ip, uio_t *uiop, int ioflag)
 *	Write the file corresponding to the specified inode.
 *
 * Calling/Exit State:
 *	The caller holds the inode's rwlock *exclusive* in
 *	preparation to change the file's contents/allocation
 *	information.
 *
 * Description:
 *	Perform a pre-V4 style write through the buffer cache.
 */
int
xx_writei(inode_t *ip, uio_t *uiop, int ioflag)
{
	buf_t	*bp;
	daddr_t	bn;
	daddr_t	lbn;
	daddr_t	rabn;
	dev_t	dev;
	int	bsize;
	int	error;
	int	mode;
	int	part_write;
	int	type;
	off_t	osize;
	pl_t	s;
	rlim_t	limit;
	xx_fs_t	*xxfsp;
	uint_t	n;
	uint_t	on;
	ulong_t	oresid;
	ulong_t	Usave;
	vnode_t	*vp;

	vp = ITOV(ip);
	xxfsp = XXFS(vp->v_vfsp);
	oresid = uiop->uio_resid;
	limit = uiop->uio_limit;
	mode = ip->i_mode;
	osize = ip->i_size;
	bsize = VBSIZE(vp);
	dev = ip->i_dev;
	error = 0;
	part_write = 0;
	Usave = 0;

	if (uiop->uio_offset < 0) {
		return (EINVAL);
	}

	if (ioflag & IO_SYNC) {
		ip->i_flag |= ISYNC;
	}

	while (error == 0 && uiop->uio_resid > 0) {
		if (part_write) {
			goto err;
		}
		lbn = lblkno(xxfsp, uiop->uio_offset);
		on = blkoff(xxfsp, uiop->uio_offset);

		n = MIN(bsize - on, uiop->uio_resid);
		type = mode & IFMT;
		if (type == IFREG && (uiop->uio_offset + n >= limit)) {
			if (uiop->uio_offset >= limit) {
				error = EFBIG;
				goto err;
			}
			n = limit - uiop->uio_offset;
		}

		osize = ip->i_size;

		error = xx_bmap(ip, lbn, &bn, &rabn, B_WRITE);
		if ((error == ENOSPC) && (n > bsize)) {
			n = bsize;
			error = xx_bmap(ip, lbn, &bn, (daddr_t *)NULL, B_WRITE);
		}
		if (error || (long)bn < 0) {
			if (oresid != uiop->uio_resid) {
				error = 0;
			}
			uiop->uio_resid += Usave;
			return (error);
		}
		if (uiop->uio_offset+n > osize &&
		   (type == IFDIR || type == IFREG || type == IFLNK)) {
			ip->i_size = uiop->uio_offset + n;
		}
		bn = LTOPBLK(bn, bsize);
		if (n == bsize) {
			bp = getblk(dev, bn, bsize, 0);
		} else {
			bp = bread(dev, bn, bsize);
		}
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			uiop->uio_resid += Usave;
			goto err;
		}
		n = MIN(n, bsize - bp->b_resid);
		error = uiomove(bp->b_un.b_addr + on, n, UIO_WRITE, uiop);
		if (ioflag & IO_SYNC || type == IFDIR) {
			bwrite(bp);
		} else if ((n + on) == bsize) {
			bp->b_flags |= B_AGE|B_ASYNC;
			bwrite(bp);
		} else {
			bdwrite(bp);
		}
		s = XX_ILOCK(ip);
		XXIMARK(ip, IUPD|ICHG);
		XX_IUNLOCK(ip, s);
	}

err:
	/*
	 * If we've already done a partial write,
	 * terminate the write but return no error.
	 */
	if (oresid != uiop->uio_resid) {
		error = 0;
	}
	return (error);
}

/*
 * int
 * xx_rdwri(uio_rw_t rw, inode_t *ip, caddr_t base, int len, off_t offset,
 *	    uio_seg_t seg, int ioflag, int *aresid)
 *	Package the arguments into a uio structure and
 *	invoke xx_readi() or xx_writei(), as appropriate.
 *
 * Calling/Exit State:
 *	The caller must hold the inode's rwlock in at
 *	least *shared* mode if doing a read; *exclusive*
 *	mode must be specified when doing a write.
 */
int
xx_rdwri(uio_rw_t rw, inode_t *ip, caddr_t base, int len, off_t offset,
	 uio_seg_t seg, int ioflag, int *aresid)
{
	int	error;
	iovec_t	aiov;
	uio_t	auio;

	aiov.iov_base = base;
	auio.uio_resid = aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = (short)seg;
	auio.uio_limit = offset + NBPSCTR;
	if (rw == UIO_WRITE) {
		auio.uio_fmode = FWRITE;
		error = xx_writei(ip, &auio, ioflag);
	} else {
		auio.uio_fmode = FREAD;
		error = xx_readi(ip, &auio, ioflag);
	}
	if (aresid) {
		*aresid = auio.uio_resid;
	}
	return (error);
}

#ident	"@(#)kern-i386:fs/xxfs/xxbmap.c	1.1.3.1"

#include <fs/fski.h>

#include <fs/buf.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/xxfs/xxinode.h>
#include <fs/xxfs/xxmacros.h>
#include <fs/xxfs/xxparam.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * int
 * xx_bmap(inode_t *ip, daddr_t lbn, daddr_t *bnp, daddr_t *rabnp, int rw)
 *	Defines the structure of file system storage by mapping a
 *	logical block number in a file to a physical block number
 *	on the device.
 *
 *	Arguments passed in:
 *	ip ---------> file to be mapped
 *	lbn --------> logical block number
 *	bnp --------> mapped block number
 *	rabnp ------> read-ahead block
 *	rw ---------> B_READ or B_WRITE
 *	alloc_only -> only allocate disk blocks
 *
 * Calling/Exit State:
 *	If the file size is being changed, the caller holds the
 *	inode's rwlock in *exclusive* mode.
 *
 *	Returns 0 on success, or a non-zero errno on failure.
 *
 * Description:
 *	xx_bmap translates logical block number lbn to a physical block
 *	number and returns it in *bnp, possibly along with a read-ahead
 *	block number in *rabnp.  bnp and rabnp can be NULL if the
 *	information is not required.  rw specifies whether the mapping
 *	is for read or write.
 *
 */
/* ARGSUSED */
int
xx_bmap(inode_t *ip, daddr_t lbn, daddr_t *bnp, daddr_t *rabnp, int rw)
{
	buf_t	*bp;
	buf_t	*nbp;
	daddr_t	*bap;
	daddr_t	bn;
	daddr_t	inb;
	daddr_t	nb;
	dev_t	dev;
	int	bsize;
	int	error;
	int	i;
	int	isdir;
	int	issync;
	int	j;
	int	nshift;
	int	sh;
	int	xx_blkalloc();
	pl_t	s;
	xx_fs_t	*xxfsp;
	vfs_t	*vfsp;
	vnode_t	*vp;

	vp = ITOV(ip);
	bsize = VBSIZE(vp);
	vfsp = vp->v_vfsp;
	xxfsp = XXFS(vfsp);
	dev = ip->i_dev;

	if (bnp) {
		*bnp = XX_HOLE;
	}
	if (rabnp) {
		*rabnp = XX_HOLE;
	}

	isdir = (vp->v_type == VDIR);
	issync = ((ip->i_flag & ISYNC) != 0);

	/*
	 * Blocks 0..NADDR-4 are direct blocks.
	 */
	bn = lbn;
	if (bn < NADDR-3) {
		i = bn;
		nb = ip->i_addr[i];
		if (nb == 0) {
			if (rw != B_WRITE) {
				if (bnp) {
					*bnp = XX_HOLE;
				}
				return (0);
			}
			/*
			 * Obtain the next available free disk block.
			 */
			bp = NULL;
			error = xx_blkalloc(vfsp, &nb);
			if (error) {
				return (error);
			}
			/*
			 * Write directory blocks synchronously so that they
			 * never appear with garbage in them on the disk.
			 */
			if (isdir) {
				bp = getblk(dev, LTOPBLK(nb, bsize), bsize, 0);
				clrbuf(bp);
				bwrite(bp);
			} else if (bp) {
				brelse(bp);
			}
			ip->i_addr[i] = nb;
			s = XX_ILOCK(ip);
			XXIMARK(ip, IUPD|ICHG);
			XX_IUNLOCK(ip, s);
		}
		if (rabnp && i < NADDR-4) {
			*rabnp = (ip->i_addr[i+1] == 0) ?
			  XX_HOLE : ip->i_addr[i+1];
		}
		if (bnp) {
			*bnp = nb;
		}
		return (0);
	}

	/*
	 * Addresses NADDR-3, NADDR-2, and NADDR-1 have single, double,
	 * triple indirect blocks.  The first step is to determine how
	 * many levels of indirection.
	 */
	nshift = xxfsp->fs_nshift;
	sh = 0;
	nb = 1;
	bn -= (NADDR - 3);
	for (j = 3; j > 0; j--) {
		sh += nshift;
		nb <<= nshift;
		if (bn < nb)
			break;
		bn -= nb;
	}
	if (j == 0) {
		return (EFBIG);
	}

	/*
	 * Fetch the address from the inode.
	 */
	inb = ip->i_addr[NADDR-j];
	if (inb == 0) {
		if (rw != B_WRITE) {
			if (bnp) {
				*bnp = XX_HOLE;
			}
			return (0);
		}
		error = xx_blkalloc(vfsp, &inb);
		if (error)
			return (error);
		/*
		 * Zero and synchronously write indirect blocks
		 * so that they never point at garbage.
		 */
		bp = getblk(dev, LTOPBLK(inb, bsize), bsize, 0);
		clrbuf(bp);
		bwrite(bp);
		ip->i_addr[NADDR-j] = inb;
		s = XX_ILOCK(ip);
		XXIMARK(ip, IUPD|ICHG);
		XX_IUNLOCK(ip, s);
	}

	/*
	 * Fetch through the indirect blocks.
	 */
	for (; j <= NIADDR; j++) {
		bp = bread(dev, LTOPBLK(inb, bsize), bsize);
		error = geterror(bp);
		if (error) {
			brelse(bp);
			return (error);
		}
		bap = bp->b_un.b_daddr;
		sh -= nshift;
		i = (bn >> sh) & xxfsp->fs_nmask;
		nb = bap[i];
		if (nb == 0) {
			if (rw != B_WRITE) {
				brelse(bp);
				if (bnp) {
					*bnp = XX_HOLE;
				}
				return (0);
			}
			error = xx_blkalloc(vfsp, &nb);
			if (error) {
				brelse(bp);
				return (error);
			}

			if (j < NIADDR || isdir) {
				/*
				 * Write synchronously so indirect blocks
				 * never point at garbage and blocks in
				 * directories never contain garbage.
				 */
				nbp = getblk(dev, LTOPBLK(nb, bsize), bsize, 0);
				clrbuf(nbp);
				bwrite(nbp);
			}

			bap[i] = nb;
			s = XX_ILOCK(ip);
			XXIMARK(ip, IUPD|ICHG);
			XX_IUNLOCK(ip, s);
			if (issync) {
				bwrite(bp);
			} else {
				bdwrite(bp);
			}
		} else {
			brelse(bp);
		}
		inb = nb;
	}

	if (bnp != NULL) {
		*bnp = nb;
	}
	/*
	 * Calculate read-ahead.
	 */
	if (rabnp != NULL) {
		if (i < NINDIR(xxfsp) - 1) {
			nb = bap[i + 1];
			*rabnp = (nb == 0) ? XX_HOLE : nb;
		} else {
			*rabnp = XX_HOLE;
		}
	}
	return (0);
}

/*
 * int
 * xx_bmapalloc(inode_t *ip, daddr_t first, daddr_t last, int alloc_only,
 *		daddr_t *dblist)
 *	Allocate all the blocks in the range [first, last].
 *
 * Calling/Exit State:
 *	If the file size is being changed, the caller holds the inode's
 *	rwlock in *exclusive* mode.
 *
 *	Returns 0 on success, or a non-zero errno on error.
 */
/* ARGSUSED */
int
xx_bmapalloc(inode_t *ip, daddr_t first, daddr_t last, int alloc_only,
	     daddr_t *dblist)
{
	daddr_t	lbn;
	daddr_t	pbn;
	daddr_t	*dbp;
	int	error;

	error = 0;
	dbp = dblist;
	for (lbn = first; error == 0 && lbn <= last; lbn++) {
		error = xx_bmap(ip, lbn, (daddr_t *)&pbn,
				(daddr_t *)NULL, B_WRITE);
		if (error) {
			break;
		}
		if (dbp != NULL) {
			*dbp++ = pbn;
		}
	}
	return (error);
}

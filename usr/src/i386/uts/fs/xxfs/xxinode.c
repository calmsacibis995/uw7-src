#ident	"@(#)kern-i386:fs/xxfs/xxinode.c	1.2.3.1"
#ident	"$Header$"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/file.h>
#include <fs/fski.h>
#include <fs/fs_subr.h>
#include <fs/mode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/xxfs/xxdata.h>
#include <fs/xxfs/xxdir.h>
#include <fs/xxfs/xxfilsys.h>
#include <fs/xxfs/xxino.h>
#include <fs/xxfs/xxinode.h>
#include <fs/xxfs/xxmacros.h>
#include <io/conf.h>
#include <io/open.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/inline.h>
#include <util/cglocal.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/metrics.h>
#include <util/mod/moddefs.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

extern struct seg *segkmap;
extern long int xx_ninode;

/*
 * inode hashing.
 */

#define xx_ihash(X)	(&xx_hinode[(int) (X) & (NHINO-1)])

struct	hinode	xx_hinode[NHINO];	/* xx hash table */

struct inode *xxinode;
struct ifreelist xx_ifreelist;

extern	int	xx_bmap(inode_t *, daddr_t, daddr_t *, daddr_t *, int);
extern	int	xx_bmapalloc(inode_t *, daddr_t, daddr_t, int, daddr_t *);
extern	void	xx_ifree(inode_t *);
extern	void	xx_blkfree(vfs_t *, daddr_t);

extern vnodeops_t xx_vnodeops;
extern vfsops_t xx_vfsops;

STATIC int xx_load(void);
STATIC int xx_unload(void);

int xx_fsflags = 0;	/* initialize vswp->vsw_flags */
int xx_fstype;		/* index into vfssw[] for this FS */

MOD_FS_WRAPPER(xx, xx_load, xx_unload, "Loadable XENIX FS Type");

/*
 * int
 * xx_doinit(int sleepflag)
 *	Allocate and initialize inodes.
 *
 * Calling/Exit State:
 *	Called at system initialization time so no locks are needed.
 *
 * Description:
 *	Allocate memory for inode table. Initialize
 *	hash lists, free lists and inodes.
 *
 * Remarks:
 *	Called from xxinit() and xx_load().
 */
int
xx_doinit(int sleepflag)
{
	inode_t	*ip;
	int	i;

	xxinode = (inode_t *)kmem_zalloc(xx_ninode * sizeof(inode_t), KM_SLEEP);
	if (xxinode == NULL) {
		/*
		 *+ Kernel memory for the inode table could not be allocated.
		 *+ Reconfigure the system to consume less memory.
		 */
		cmn_err(CE_PANIC, "xx_inoinit: no memory for XENIX inodes");
		return (ENOMEM);
	}
	if (xx_tflush > v.v_autoup) {
		/*
		 *+ Invalid flush time interval.
		 */
		cmn_err(CE_NOTE,
			"XXFLUSH is invalid. It must be less than NAUTOUP.");
		return (EINVAL);
	}
	for (i = 0; i < NHINO; i++) {
		xx_hinode[i].i_forw = (inode_t *) &xx_hinode[i];
		xx_hinode[i].i_back = (inode_t *) &xx_hinode[i];
	}
	xx_ifreeh = xxinode;
	xx_ifreet = &xxinode->av_forw;
	ip = xxinode;
	ip->av_back = &xx_ifreeh;
	XX_INIT_INODE(ip, NULL, 0, 0);
	for (i = xx_ninode; --i > 0;) {
		++ip;
		XX_INIT_INODE(ip, NULL, 0, 0);
		*xx_ifreet = ip;
		ip->av_back = xx_ifreet;
		xx_ifreet =  &ip->av_forw;
	}
	ip->av_forw = NULL;

	/*
	 * Initialize inode table mutex spin lock and update sleep lock.
	 */
	LOCK_INIT(&xx_inode_table_mutex, FS_XXLISTHIER, FS_XXLISTPL,
		  &xx_inode_table_lkinfo, KM_NOSLEEP);
	SLEEP_INIT(&xx_updlock, (uchar_t) 0, &xx_updlock_lkinfo, sleepflag);

	MET_INODE_MAX(MET_OTHER, xx_ninode);
	MET_INODE_CURRENT(MET_OTHER, xx_ninode);

	return (0);
}

/*
 * int
 * xx_load(void)
 *	Load XENIX file system into kernel.
 *
 * Calling/Exit State:
 *	No locks are necessary.
 *
 * Description:
 *	Allocate XENIX inode table, initialize inode table lock,
 *	and initialize update sleep lock.
 */
STATIC int
xx_load(void)
{
	int	error;

	if ((xx_fstype = vfs_attach("XENIX", &xx_vfsops, xx_fsflags)) == -1) {
		/*
		 *+ XENIX file system was not registered
		 *+ before attempting to load it.
		 */
		cmn_err(CE_NOTE, "!MOD: XENIX is not registered.");
		return (EINVAL);
	}
	error = xx_doinit(KM_SLEEP);
	return (error);
}

/*
 * int
 * xx_unload(void)
 *	Unload XENIX file system from kernel.
 *
 * Calling/Exit State:
 *	No locks are necessary.
 *
 * Description:
 *	Deallocate XENIX inode table; de-initialize inode table
 *	lock and update sleep lock.
 */
STATIC int
xx_unload(void)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	vfs_detach("XENIX");
	kmem_free(xxinode, xx_ninode * sizeof(inode_t));
	LOCK_DEINIT(&xx_inode_table_mutex);
	SLEEP_DEINIT(&xx_updlock);
	return (0);
}

/*
 * void
 * xxinit(vfssw_t *vswp)
 *	Initialize XENIX file system data structures.
 *
 * Calling/Exit State:
 *	Called at system initialization time so no locks are necessary.
 *
 * Description:
 *	Initializes the XENIX vfs structure, inode table, free list
 *	and necessary global locks.
 */
void
xxinit(vfssw_t *vswp)
{
	xx_load();
	return;
}

/*
 * inode_t *
 * xx_search_ilist(struct hinode *ih, ino_t ino, vfs_t *vfsp)
 *	Determine whether a specific inode belongs to a given
 *	hash bucket, i.e., has the inode been hashed already?
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode table lock.
 *
 *	On success, a pointer to an *unlocked* inode is returned.
 *	Since the calling LWP has the inode table lock, this is ok.
 *
 *	If NULL is returned, than a matching inode could not be found.
 *
 * Description:
 *	Simply search the hash bucket for a inode matching
 *	<vfsp->vfs_dev, ino>.
 */
inode_t *
xx_search_ilist(struct hinode *ih, ino_t ino, vfs_t *vfsp)
{
	inode_t *ip;

	for (ip = ih->i_forw; ip != (inode_t *)ih; ip = ip->i_forw) {
		if (ino == ip->i_number && vfsp == ITOV(ip)->v_vfsp)
			return (ip);
	}

	/*
	 * Did not find it.
	 */
	return (NULL);
}

/*
 * void
 * xx_idrop(inode_t *ip)
 *	Drop inode without going through the
 *	normal chain of unlocking and releasing.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock exclusive on entry. On
 *	exit, no inode lock is held since the caller may not reference
 *	<ip> any more since the LWP no longer has a reference to the inode.
 */
void
xx_idrop(inode_t *ip)
{
	pl_t	s;
	vnode_t *vp;

	vp = ITOV(ip);

	ASSERT(!XX_IRWLOCK_IDLE(ip));

	VN_LOCK(vp);
	if (vp->v_count == 1) {
		vp->v_count = 0;
		VN_UNLOCK(vp);
		s = LOCK(&xx_inode_table_mutex, FS_XXLISTPL);
		/*
		 * If there's not any LWPs blocked on this inode's rwlock
		 * (i.e, aren't waiting for it to become available) put
		 * the inode back on the free list. If there are blocked
		 * waiters, then let them have the inode...
		 *
		 */
		if (!XX_IRWLOCK_BLKD(ip)) {
			if (xx_ifreeh) {
				*xx_ifreet = ip;
				ip->av_back = xx_ifreet;
			} else {
				xx_ifreeh = ip;
				ip->av_back = &xx_ifreeh;
			}
			ip->av_forw = NULL;
			xx_ifreet = &ip->av_forw;
			ip->i_flag |= IFREE;
			MET_INODE_INUSE(MET_OTHER, -1);
		}

		/*
		 * Release the inode lock before the table lock
		 */
		XX_IRWLOCK_UNLOCK(ip);
		UNLOCK(&xx_inode_table_mutex, s);
	} else {
		vp->v_count--;
		VN_UNLOCK(vp);
		XX_IRWLOCK_UNLOCK(ip);
	}
}

/*
 * void
 * xx_iupdat(inode_t *ip)
 *	Flush inode to disk, updating timestamps if requested.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in *exclusive* mode.
 */
void
xx_iupdat(inode_t *ip)
{
	boolean_t issync;
	buf_t	 *bp;
	char	 *p1;
	char	 *p2;
	xxdinode_t *dp;
	int	 bsize;
	pl_t	 s;
	xx_fs_t	 *xxfsp;
	unsigned i;
	vnode_t  *vp;

	issync = B_FALSE;
	vp = ITOV(ip);
	xxfsp = XXFS(vp->v_vfsp);
	bsize = VBSIZE(vp);
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		return;
	}
	bp = bread(ip->i_dev, LTOPBLK(FsITOD(xxfsp, ip->i_number), bsize),
		   bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return;
	}
	dp = (xxdinode_t *)bp->b_addrp;
	dp += FsITOO(xxfsp, ip->i_number);
	dp->di_mode = ip->i_mode;
	dp->di_nlink = ip->i_nlink;
	dp->di_uid = ip->i_uid;
	dp->di_gid = ip->i_gid;
	dp->di_size = ip->i_size;
	dp->di_gen = ip->i_gen;
	p1 = (char *)dp->di_addr;
	p2 = (char *)ip->i_addr;
	/*
	 * The following copy is machine (byte order) specific.
	 */
	for (i = 0; i < NADDR; i++) {
#ifndef i386
		p2++;
#endif
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
#ifdef i386
		p2++;
#endif
	}
	/*
	 * Update the inode's times if necessary.
	 */
	s = XX_ILOCK(ip);
	XXIMARK(ip, ip->i_flag);
	if (ip->i_flag & ISYN) {
		issync = B_TRUE;
	}
	ip->i_flag &= ~(IACC|IUPD|ICHG|IMOD|ISYN);
	XX_IUNLOCK(ip, s);
	dp->di_atime = ip->i_atime;
	dp->di_mtime = ip->i_mtime;
	dp->di_ctime = ip->i_ctime;
	if (issync == B_TRUE) {
		bwrite(bp);
	} else {
		bdwrite(bp);
	}
}

/*
 * int
 * xx_getfree_inode(inode_t **ipp, struct hinode *ih, ino_t ino,
 *		    vfs_t *vfsp, pl_t s)
 *	Remove an inode from the free list (if there are
 *	any available) for re-use. May cause dnlc purges.
 *
 * Calling/Exit State:
 *	Calling LWP holds the inode table locked. The inode table lock may
 *	be released and reobtained in this routine if the inode free list
 *	is initially empty and the calling LWP tries to free some inodes by
 *	purging the dnlc of some entries.
 *
 *	The inode table lock is held when 0 is returned and <*ipp>
 *	is non-NULL. In all other cases, the inode table lock is
 *	not held on exit.
 *
 *	If 0 is returned, then the inode removed from the free list is placed
 *	in <*ipp>. The rwlock of <*ipp> is held exclusive in this case.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine:
 *		ENFILE  All available XENIX inodes are in use.
 *			<*ipp> is NULL in this case.
 *
 * Description:
 *	Try to take an inode from the free list. If the free list
 *	is empty then purge some dnlc entries from the file system
 *	that the inode will live on. Return ENFILE if there aren't
 *	any inodes available.
 *
 *	If there is an available inode, then remove it from the free
 *	list and insert it into the proper place on the hash list.
 *
 *	Must be careful to re-check the hash list for the requested inode
 *	if we release the lists lock since another LWP could have entered
 *	the inode in the list.
 */
/* ARGSUSED */
int
xx_getfree_inode(inode_t **ipp, struct hinode *ih, ino_t ino,
		 vfs_t *vfsp, pl_t s)
{
	int	error;
	inode_t	*dupip;
	inode_t	*ip;
	inode_t	*iq;
	vnode_t	*vp;

	*ipp = NULL;
	error = 0;

	/*
	 * If inode freelist is empty, release some name cache entries
	 * for this mounted file system in an attempt to reclaim some
	 * inodes.
	 */
again:
	if (xx_ifreeh == NULL) {
		/*
		 * dnlc_purge_vfsp might go to sleep. So we have to check the
		 * hash chain again to handle possible race condition.
		 */
		UNLOCK(&xx_inode_table_mutex, s);
		(void)dnlc_purge_vfsp(vfsp, 50);
		s = LOCK(&xx_inode_table_mutex, FS_XXLISTPL);

		if (xx_ifreeh == NULL) {
			UNLOCK(&xx_inode_table_mutex, s);
			return (ENFILE);
		}
	}

	/*
	 * Remove the inode from the free list.
	 */
	ip = xx_ifreeh;
	if (XX_ITRYWRLOCK(ip) != B_TRUE) {
		/*
		 * Trylock the inode. If trylock fails, someone
		 * raced to re-instate the inode and won. We
		 * need to move it to the end of the freelist
		 * and try to get the next one on the freelist.
		 */
		iq = ip->av_forw;
		if (iq) {
			iq->av_back = &xx_ifreeh;
		}
		xx_ifreeh = iq;

		if (xx_ifreeh) {
			*xx_ifreet = ip;
			ip->av_back = xx_ifreet;
		} else {
			xx_ifreeh = ip;
			ip->av_back = &xx_ifreeh;
		}
		ip->av_forw = NULL;
		xx_ifreet = &ip->av_forw;
		goto again;
	}

	if ((ip->i_flag & IFREE) == 0) {
		/* Some one raced and won. */
		XX_IRWLOCK_UNLOCK(ip);
		iq = ip->av_forw;
		if (iq) {
			iq->av_back = &xx_ifreeh;
		}
		xx_ifreeh = iq;
		goto again;
	}

	iq = ip->av_forw;
	if (iq) {
		iq->av_back = &xx_ifreeh;
	}
	xx_ifreeh = iq;

	ASSERT((ip->i_flag & IFREE) != 0);
	ASSERT(ITOV(ip)->v_count == 0);
	ip->av_forw = NULL;
	ip->av_back = NULL;

	/*
	 * Clear any flags that may have been set, including
	 * IFREE. Also, take the inode out of the hash chain
	 * so no other LWP can find it.
	 */
	ip->i_flag = 0;

	xx_iunhash(ip);

	/*
	 * Make sure that someone else didn't enter the inode into
	 * the list while we were sleeping in xx_syncip(). If they
	 * did, then, must drop our inode (<ip>) and start over.
	 */
	dupip = xx_search_ilist(ih, ino, vfsp);
	if (dupip != NULL) {
		VN_HOLD(ITOV(ip));
		UNLOCK(&xx_inode_table_mutex, s);
		xx_idrop(ip);
		return (0);
	}

	/*
	 * Clear any flags that my have been set, including IFREE.
	 */
	ip->i_flag = 0;

	vp = ITOV(ip);
	ASSERT(vp->v_count == 0);
	ASSERT((ip->i_flag & IFREE) == 0);

	vp->v_flag = 0;
	/*
	 * Put the inode on the chain for its new (ino, dev)
	 * pair and destroy the mapping if any. 
	 */
	ih->i_forw->i_back = ip;
	ip->i_forw = ih->i_forw;
	ih->i_forw = ip;
	ip->i_back = (inode_t *) ih;

	ip->i_dev = vfsp->vfs_dev;
	ip->i_number = (o_ino_t) ino;
	ip->i_nextr = 0;
	ip->i_vcode = 0;
	*ipp = ip;

	return (error);
}

/*
 * void
 * xx_indirtrunc(vfs_t *vfsp, daddr_t bn, daddr_t lastbn, int level)
 *	Release blocks associated with the inode ip and
 *	stored in the indirect block bn.
 *
 * Calling/Exit State:
 *	May be called recursively.
 *
 *	The calling LWP hold's the inode's rwlock in *exclusive* mode
 *	on entry and exit.
 *
 * Description:
 *	Blocks are free'd in LIFO order up to (but not including) <lastbn>.
 *	If level is greater than SINGLE, the block is an indirect
 *	block and recursive calls to xx_indirtrunc must be used to
 *	cleanse other indirect blocks.
 */
void
xx_indirtrunc(vfs_t *vfsp, daddr_t bn, daddr_t lastbn, int level)
{
	int	i;
	buf_t	*bp;
	buf_t	*copy;
	daddr_t	*bap;
	daddr_t	nb;
	daddr_t	last;
	long	bsize;
	long	factor;
	long	nindir;
	xx_fs_t	*xxfsp;

	xxfsp = XXFS(vfsp);
	/*
	 * Calculate index in current block of last block (pointer) to be kept.
	 * A lastbn of -1 indicates that the entire block is going away, so we
	 * need not calculate the index.
	 */

	bsize = XXBSIZE;
	nindir = NINDIR(xxfsp);
	factor = 1;
	for (i = SINGLE; i < level; i++) {
		factor *= nindir;
	}
	last = lastbn;
	if (lastbn > 0) {
		last /= factor;
	}

	/*
	 * Get buffer of block pointers, zero those entries corresponding to
	 * blocks to be freed, and update on-disk copy first.  (If the entire
	 * block is to be discarded, there's no need to zero it out and
	 * rewrite it, since there are no longer any pointers to it, and it
	 * will be freed shortly by the caller anyway.)
	 * Note potential deadlock if we run out of buffers.  One way to
	 * avoid this might be to use statically-allocated memory instead;
	 * you'd have to make sure that only one process at a time got at it.
	 */

	copy = ngeteblk(bsize);
	bp = bread(vfsp->vfs_dev, LTOPBLK(bn, bsize), bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(copy);
		brelse(bp);
		return;
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, copy->b_un.b_addr, bsize);
	bzero((caddr_t)&bap[last+1],
		(int)(nindir - (last+1)) * sizeof(daddr_t));
	bwrite(bp);
	bap = copy->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */

	for (i = nindir-1; i > last; i--) {
		nb = bap[i];
		if (nb != 0) {
			if (level > SINGLE) {
				xx_indirtrunc(vfsp, nb, (daddr_t)-1, level - 1);
			}
			xx_blkfree(vfsp, nb);
		}
	}

	/*
	 * Recursively free last partial block.
	 */

	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0) {
			xx_indirtrunc(vfsp, nb, last, level-1);
		}
	}

	brelse(copy);
}

/*
 * xx_itruncup(inode_t *ip, size_t nsize)
 *	Truncate the inode <ip> up to <nsize> size.
 *
 * Calling/Exit State:
 *	The calling LWP hold the inode's rwlock exclusive on entry and exit.
 */
xx_itruncup(inode_t *ip, size_t nsize)
{
	buf_t	*bp;
	daddr_t	lbn;
	daddr_t	lastblock;
	int	error;
	pl_t	s;
	vfs_t	*vfsp;
	vnode_t *vp;
	xx_fs_t	*xxfsp;

	vp = ITOV(ip);
	vfsp = vp->v_vfsp;
	xxfsp = XXFS(vfsp);

	lastblock = lblkno(xxfsp, (XXBSIZE + nsize - 1)) - 1;
	error = xx_bmapalloc(ip, lastblock, lastblock, 0, 0);
	if (error) {
		return (error);
	}
	/*
	 * Zero and synchronously write till end of block.
	 */
	error = xx_bmap(ip, lastblock, &lbn, (daddr_t *)NULL, B_READ);
	if (error) {
		return (error);
	}
	bp = getblk(vfsp->vfs_dev, LTOPBLK(lbn, XXBSIZE), XXBSIZE, 0);
	clrbuf(bp);
	bwrite(bp);
	/*
	 * If there are no errors or bmap allocated a block
	 * but there was an i/O error, update the inode.
	 */
	if (!error) {
		ip->i_size = nsize;
		s = XX_ILOCK(ip);
		XXIMARK(ip, IUPD|ICHG);
		XX_IUNLOCK(ip, s);
		xx_iupdat(ip);
	}

	return (error);
}

/*
 * int
 * xx_itrunc(inode_t *ip, uint_t length)
 *	Truncate the inode <ip> to at most length size.
 *
 * Calling/Exit State:
 *	The calling LWP hold the inode's rwlock exclusively on entry and exit.
 *
 * Description:
 *	Free all the disk blocks associated with the specified inode
 *	structure.  The blocks of the file are removed in reverse order.
 *	This FILO algorithm will tend to maintain a contiguous free list
 *	much longer than FIFO.
 */
int
xx_itrunc(inode_t *ip, uint_t length)
{
	buf_t	*bp;
	daddr_t	bn;
	daddr_t	lbn;
	daddr_t	curblock;
	daddr_t	lastblock;
	daddr_t	lastiblock[NIADDR];
	daddr_t	save[NADDR];
	int	error;
	int	i;
	int	level;
	int	type;
	pl_t	s;
	size_t	osize;
	xx_fs_t	*xxfsp;
	vfs_t	*vfsp;
	vnode_t	*vp;

	vp = ITOV(ip);
	vfsp = vp->v_vfsp;
	xxfsp = XXFS(vfsp);
	/*
	 * We only allow truncation of regular files and directories
	 * to arbritary lengths here. In addition, we allow symbolic
	 * links to be truncated only to zero length. Other inode
	 * types cannot have their length set here disk blocks are
	 * being dealt with - especially device inodes where
	 * ip->i_rdev is actually being stored in ip->i_db[1]!
	 */
	type = ip->i_mode & IFMT;

	if (type == IFIFO) {
		return (0);
	}

	if (type != IFREG && type != IFDIR &&
	  !(type == IFLNK && length == 0)) {
		return (EINVAL);
	}

	osize = ip->i_size;
	/*
	 * If file size is not changing, mark it as changed for
	 * POSIX. Since there's no space change, no need to write
	 * inode synchronously.
	 */
	if (length == osize) {
		s = XX_ILOCK(ip);
		XXIMARK(ip, ICHG | IUPD);
		XX_IUNLOCK(ip, s);
		xx_iupdat(ip);
		return (0);
	}

	/* Truncate-up case. */
	if (length > osize) {
		return (xx_itruncup(ip, length));
	}

	/* Truncate-down case. */

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep. Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	curblock = lblkno(xxfsp, osize);
	lastblock = lblkno(xxfsp, (length + XXBSIZE - 1)) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(xxfsp);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(xxfsp)
				* NINDIR(xxfsp);

	/*
	 * Update file and block pointers on disk before we start freeing
	 * blocks. If we crash before free'ing blocks below, the blocks
	 * will be returned to the free list. lastiblock values are also
	 * normalized to -1 for calls to xx_indirtrunc() below.
	 */

	for (i = NADDR - 1; i >= 0; i--) {
		save[i] = ip->i_addr[i];
	}

	for (level = TRIPLE; level >= SINGLE; level--) {
		if (lastiblock[level] < 0) {
			ip->i_addr[IB(level)] = 0;
			lastiblock[level] = -1;
		}
	}
	for (i = NDADDR - 1; i > lastblock; i--) {
		ip->i_addr[i] = 0;
	}

	if (curblock < lastblock) {
		error = xx_bmapalloc(ip, lastblock, lastblock, 0, 0);
		if (error) {
			return (error);
		}
		/*
		 * Zero and synchronously write till end of block.
		 */
		error = xx_bmap(ip, lastblock, &lbn, (daddr_t *)NULL, B_READ);
		if (error) {
			return (error);
		}
		bp = getblk(vfsp->vfs_dev, LTOPBLK(lbn, XXBSIZE), XXBSIZE, 0);
		clrbuf(bp);
		bwrite(bp);
	}
	ip->i_size = length;
	s = XX_ILOCK(ip);
	XXIMARK(ip, ICHG|IUPD);
	XX_IUNLOCK(ip, s);

	xx_iupdat(ip);

	/*
	 * Indirect blocks first.
	 */
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = save[IB(level)];
		if (bn != 0) {
			xx_indirtrunc(vfsp, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				xx_blkfree(vfsp, bn);
			}
		}
		if (lastiblock[level] >= 0) {
			goto done;
		}
	}

	/*
	 * All whole direct blocks.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		bn = save[i];
		if (bn != 0) {
			xx_blkfree(vfsp, bn);
		}
	}

done:
	s = XX_ILOCK(ip);
	XXIMARK(ip, ICHG);
	XX_IUNLOCK(ip, s);
	return (0);
}

/*
 * int
 * xx_syncip(inode_t *ip, int flags)
 *	Flush the inode using the given flags, then
 *	force inode information to be written back.
 *
 * Calling/Exit State:
 *	The inode's rwlock is be held on entry and on exit.
 */
int
xx_syncip(inode_t *ip, int flags)
{
	daddr_t	blkno;
	int	bsize;
	int	error;
	long	lastlbn;
	long	lbn;
	vnode_t	*vp;

	error = 0;
	vp = ITOV(ip);
	bsize = VBSIZE(vp);
	if (vp->v_type != VCHR) {
		lastlbn = howmany(ip->i_size, bsize);
		for (lbn = 0 ; lbn < lastlbn; lbn++) {
			error = xx_bmap(ip, lbn, &blkno, NULL, B_READ);
			if (blkno != XX_HOLE) {
				blkflush(ip->i_dev, LTOPBLK(blkno, bsize));
			}
		}
	}
	if (ip->i_flag & (IUPD|IACC|ICHG|IMOD)) {
		if ((flags & B_ASYNC) == 0) {
			ip->i_flag |= ISYN;
		}
		xx_iupdat(ip);
	}
	return (error);
}

/*
 * void
 * xx_iinactive(inode_t *ip, int nodeislocked)
 *	Vnode is no longer referenced, write the inode out
 *	and if necessary, truncate and deallocate the file.
 *
 * Calling/Exit State:
 *	If <flag> is NODEISLOCKED, then the LWP holds the inode's rwlock.
 *	Otherwise, no lock is held on entry.
 *
 *	On exit, the caller may not reference <ip> anymore since
 *	the caller's reference to the inode has been removed.
 *
 * Description:
 *	We have the inode locked if called from xx_iput(). As soon
 *	as we know we have the inode, we unlock the vnode. Holding the
 *	vnode locked until this point ensures that we will notice
 *	if a racing xx_iget() finds this node in the hash list before
 *	we can put it on the free list. This works because holding the
 *	vnode locked causes xx_iget() to spin in VN_HOLD until the
 *	TRY_LOCK below is executed.
 *
 *	We can't keep the vnode locked if we need to sleep in the code
 *	below (e.g., to call xx_itrunc()), so we release it as soon
 *	as possible.
 */
/* ARGSUSED */
void
xx_iinactive(inode_t *ip, int nodeislocked)
{
	vnode_t	*vp;
	pl_t	s;

	ASSERT((ip->i_flag & IFREE) == 0);
	ASSERT((ip->av_forw == 0) && (ip->av_back == 0));

	vp = ITOV(ip);
	if (nodeislocked == NODEISUNLOCKED) {
		XX_IRWLOCK_WRLOCK(ip);
	}

	/*
	 * It is impossible for any new references to be acquired via xx_iget
	 * since any attempt to generate such a reference will block attempting
	 * to acquire the inode lock. Furthermore, if the reference count on
	 * the vnode is exactly 1, we know that we hold the only reference
	 * and that no new ones can be established as long as we hold the
	 * inode locked. We need not hold v_mutex while checking the count.
	 */
	VN_LOCK(vp);
	if (vp->v_count != 1) {
		/*
		 * The new reference is still extant and the vnode
		 * has become someone else's responsibility.  Give
		 * up our reference and return.
		 */
		vp->v_count--;
		VN_UNLOCK(vp);
		XX_IRWLOCK_UNLOCK(ip);
		return;
	}
	/*
	 * The reference count is exactly 1, and
	 * once again we hold the last reference.
	 */
	vp->v_count = 0;
	VN_UNLOCK(vp);
	ASSERT (vp->v_count == 0);
	/*
	 * We hold the only reference and it cannot change while we
	 * hold the inode lock. Clear the reference count; we need
	 * not hold v_mutex while doing this.
	 */
	s = LOCK(&xx_inode_table_mutex, FS_XXLISTPL);
	/*
	 * Someone may previously have blocked in xx_iget() attempting to
	 * acquire the inode lock. If so, they will establish a new
	 * reference to the inode and we can leave them unmolested to
	 * do so. But if not, we return the inode to the freelist and
	 * proceed to the usual cleanup. Note that as long as we hold
	 * hash_list_mutex, no one new can find the inode and block
	 * on the inode lock; note also that as soon as we drop
	 * hash_list_mutex someone in xx_iget() may find the inode,
	 * but will still block until we release the inode lock. The
	 * worst that can happen is that someone may acquire the
	 * inode after we've done the cleanup, but this is benign.
	 */
	if (XX_IRWLOCK_BLKD(ip)) {
		UNLOCK(&xx_inode_table_mutex, s);
		XX_IRWLOCK_UNLOCK(ip);
		return;
	}
	/*
	 * Put the inode on the free list. If the
	 * inode is invalid, put the inode on the
	 * head of the free list.
	 */
	if (ip->i_nlink <= 0 || ip->i_mode == 0) {
		/* Remove from hash. */
		xx_iunhash(ip);
		/* Put the inode on front of freelist */
		if (xx_ifreeh) {
			xx_ifreeh->av_back = &ip->av_forw;
		} else {
			xx_ifreet = &ip->av_forw;
		}
		ip->av_back = &xx_ifreeh;
		ip->av_forw = xx_ifreeh;
		xx_ifreeh = ip;
		ip->i_flag |= IFREE;
	} else if ((ip->i_flag & IFREE) == 0) {
		/*
		 * Otherwise, put the inode back on the end of
		 * the free list.
		 */
		if (xx_ifreeh) {
			*xx_ifreet = ip;
			ip->av_back = xx_ifreet;
		} else {
			xx_ifreeh = ip;
			ip->av_back = &xx_ifreeh;
		}
		ip->av_forw = NULL;
		xx_ifreet = &ip->av_forw;
		ip->i_flag |= IFREE;
	}

	MET_INODE_INUSE(MET_OTHER, -1);
	UNLOCK(&xx_inode_table_mutex, s);

	if (ip->i_nlink <= 0) {
		/* free inode */
		ip->i_gen++;
		xx_itrunc(ip, (u_long)0);
		ip->i_oldrdev = 0;
		ip->i_rdev = 0;
		ip->i_mode = 0;
		ip->i_flag |= IUPD|ICHG;
	} else {
		/*
		 * Do an async write (B_ASYNC) of the vnode.
		 */
		(void) xx_syncip(ip, B_ASYNC);
	}
	xx_iupdat(ip);

	if (ip->i_nlink <= 0) {
		xx_ifree(ip);
	}

	/*
	 * Unlock the inode.
	 */
	XX_IRWLOCK_UNLOCK(ip);
}

/*
 * int
 * xx_init_inode(inode_t *ip, vfs_t *vfsp, ino_t ino)
 *	Initialize an inode from it's on-disk copy to complete
 *	the initialization of an inode.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock *exclusive* on
 *	entry and exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. There are no errnos returned directly
 *	by this routine.
 *
 * Description:
 *	The inode is initialized from the on-disk copy. If there's
 *	an error, the inode number (ip->i_number) is set to 0 so
 *	any LWPs blocked on the inode do not use it (see xx_iget).
 */
int
xx_init_inode(inode_t *ip, vfs_t *vfsp, ino_t ino)
{
	buf_t	 *bp;
	char	 *p1;
	char	 *p2;
	xxdinode_t *dp;
	int	 error;
	int	 i;
	xx_fs_t  *xxfsp;
	vnode_t	 *vp;

	xxfsp = XXFS(vfsp);
	vp = ITOV(ip);
	bp = bread(ip->i_dev, LTOPBLK(FsITOD(xxfsp, ino), XXBSIZE), XXBSIZE);
	error = (bp->b_flags & B_ERROR) ? EIO : 0;
	if (error) {
		goto error_exit;
	}
	dp = (xxdinode_t *)bp->b_addrp;
	dp += FsITOO(xxfsp, ino);
	ip->i_nlink = dp->di_nlink;
	ip->i_uid = dp->di_uid;
	ip->i_gid = dp->di_gid;
	ip->i_size = dp->di_size;
	ip->i_mode = dp->di_mode;
	ip->i_atime = dp->di_atime;
	ip->i_mtime = dp->di_mtime;
	ip->i_ctime = dp->di_ctime;
	ip->i_number = (o_ino_t)ino;
	ip->i_gen = dp->di_gen;
	p1 = (char *) ip->i_addr;
	p2 = (char *) dp->di_addr;

	/*
	 * The following copy is machine (byte order) specific.
	 */
	for (i = 0; i < NADDR; i++) {
#ifndef i386
		*p1++ = 0;
#endif
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
#ifdef i386
		*p1++ = 0;
#endif
	}

	if (ip->i_mode & IFBLK || ip->i_mode == IFCHR) {
		if (ip->i_bcflag & NDEVFORMAT) {
			ip->i_rdev = makedevice(ip->i_major, ip->i_minor);
		} else {
			ip->i_rdev = expdev(ip->i_oldrdev);
		}
	} else if (ip->i_mode & IFNAM) {
		ip->i_rdev = ip->i_oldrdev;
	}

	if (IFTOVT((uint)ip->i_mode) == VREG) {
		error = fs_vcode(vp, &ip->i_vcode);
		if (error) {
			goto error_exit;
		}
	}

	brelse(bp);
	/*
	 * Fill in the rest.
	 */
	vp->v_count = 1;
	vp->v_lid = (lid_t)0;
	vp->v_vfsmountedhere = NULL;
	vop_attach(vp, &xx_vnodeops);
	vp->v_vfsp = vfsp;
	vp->v_stream = NULL;
	vp->v_pages = NULL;
	vp->v_data = (caddr_t)ip;
	vp->v_filocks = NULL;
	vp->v_type = IFTOVT((uint)ip->i_mode);
	vp->v_rdev = ip->i_rdev;
	vp->v_flag = VNOMAP;
	return (0);


error_exit:
	brelse(bp);
	/*
	 * The inode doesn't contain anything useful, so it
	 * would be misleading to leave it on its hash chain.
	 * However, to take it off the hash list causes us
	 * to reget the inode table lock. It doesn't hurt to
	 * leave it on the hash-list since we clear the
	 * i_mode field. Hence, no one will find it (and
	 * iget will drop it). It will eventually be reallocated
	 * and then rehashed. Races on the i_number
	 * are resolved by the loser dropping the inode.
	 */
	ip->i_number = 0;
	ip->i_mode = 0;
	VN_HOLD(ITOV(ip));
	vp->v_vfsp = vfsp;
        vp->v_pages = NULL;
	ip->i_nlink = 1;	/* avoid truncate */
	xx_iinactive(ip, NODEISLOCKED);
	return (error);
}

/*
 * int
 * xx_iget(vfs_t *vfsp, int ino, int excl, inode_t **ipp)
 *	Look up an inode by vfs and i-number.
 *
 * Calling/Exit State:
 *	On success 0 is returned, <*ipp> is returned locked
 *	according to <excl>. If <excl> is non-zero, it's held
 *	exclusive; otherwise, the lock is held shared.
 *
 * Description:
 *	Search for the inode on the hash list. If it's in core, honor
 *	the locking protocol.  If it's not in core, read it in from the
 *	associated device.  In all cases, a pointer to a locked inode
 *	structure is returned.
 */
int
xx_iget(vfs_t *vfsp, int ino, int excl, inode_t **ipp)
{
	inode_t		*ip;
	inode_t		*iq;
	int		error;
	pl_t		s;
	struct hinode	*hip;
	vnode_t		*vp;

	MET_IGET(MET_OTHER);
	*ipp = NULL;
	hip = xx_ihash(ino);
loop:
	s = LOCK(&xx_inode_table_mutex, FS_XXLISTPL);
	ip = xx_search_ilist(hip, ino, vfsp);
	if (ip != NULL) {
		if (excl) {
			XX_IWRLOCK_RELLOCK(ip);
		} else {
			XX_IRDLOCK_RELLOCK(ip);
		}
		/* We assume that an inode on an hash chain does 	*/
		/* have its vnode and other pointers sane!		*/
		vp = ITOV(ip);
		ASSERT(vp == &(ip->i_vnode));

		if (ino != ip->i_number) {
			/*
			 * If someone races with you to get the inode,
			 * release the inode and search again. 
			 */
                        XX_IRWLOCK_UNLOCK(ip);
			goto loop;
		}

		/* found the inode */
		if ((ip->i_flag & IFREE) != 0) {
			s = LOCK(&xx_inode_table_mutex, FS_XXLISTPL);
			/*
			 * Must check whether IFREE is still set while
			 * holding the table lock since we could have
			 * raced with other lwp's executing this same
			 * code segment.
			 */
			if ((ip->i_flag & IFREE) != 0) {
				/*
				 * Remove from freelist.
				 */
				iq = ip->av_forw;
				if (iq) {
					iq->av_back = ip->av_back;
				} else {
					xx_ifreet = ip->av_back;
				}

				*ip->av_back = iq;
				ip->av_back = NULL;
				ip->av_forw = NULL;
				ip->i_flag &= ~IFREE;
				MET_INODE_INUSE(MET_OTHER, 1);
			}
			UNLOCK(&xx_inode_table_mutex, s);
		} 
		VN_HOLD(vp);

		ASSERT((ip->i_flag & IFREE) == 0);

		*ipp = ip;
		return (0);
	}

	/*
	 * We hold the hash lists lock at this point. Take a free
	 * inode and re-assign it to our inode.
	 */
	error = xx_getfree_inode(&ip, hip, ino, vfsp, s);
	if (error != 0) {
		if (error == ENFILE) {
			MET_INODE_FAIL(MET_OTHER);
			/*
			 *+ The inode table has filled up. Corrective
			 *+ action: reconfigure the kernel to
			 *+ increase the inode table size.
			 */
			cmn_err(CE_WARN,
				"xx_iget: inode table overflow");
		}
		return (error);
	}

	if (ip == NULL) {
		goto loop;
	}

	MET_INODE_INUSE(MET_OTHER, 1);
	/*
	 * Can safely release the inode table lock at this point
	 * since the executing LWP has already established its
	 * reference to the inode and the inode is in the hash
	 * chain. Regardless of how the caller asked for the
	 * lock to be obtained, it is always obtained in *exclusive*
	 * mode in xx_getfree_inode to prevent another LWP from using
	 * the inode before it's fully initialized.
	*/
	UNLOCK(&xx_inode_table_mutex, s);

	error = xx_init_inode(ip, vfsp, ino);
	/*
	 *
	 * When initialization is complete, then downgrade lock
	 * if necessary. The downgrade is accomplished by
	 * releasing the lock and reacquiring it in shared mode.
	 * Note that we already hold a reference to the inode
	 * so it's OK to do this. This may be a potential
	 * performance problem if there's already an LWP
	 * blocked on the inode in iget up above. This isn't
	 * likely to be the case, however.
	 */
	if ((error == 0) && !excl) {
		XX_IRWLOCK_UNLOCK(ip);
		XX_IRWLOCK_RDLOCK(ip);
	}

	if (error == 0) {
		ASSERT(ip->i_number == ino);
		*ipp = ip;
	}
	return (error);
}

/*
 * void
 * xx_iput(inode_t *ip)
 *	Unlock inode and release associated vnode.
 *
 * Calling/Exit State:
 *	The caller must hold the inode's rwlock in *exclusive* mode
 *	on entry. On exit, the calling LWP's reference to the inode
 *	is removed and the caller may not reference the inode anymore.
 */
void
xx_iput(inode_t *ip)
{
	vnode_t	*vp;

	vp = ITOV(ip);
	ASSERT(!XX_IRWLOCK_IDLE(ip));

	VN_LOCK(vp);
	if (vp->v_count == 1) {
		VN_UNLOCK(vp);
		xx_iinactive(ip, NODEISLOCKED);
	} else {
		vp->v_count--;
		VN_UNLOCK(vp);
		XX_IRWLOCK_UNLOCK(ip);
	}
}

/*
 * void
 * xx_irele(inode_t *ip)
 *	Release a reference to an unlocked inode.
 *
 * Calling/Exit State:
 *	The inode is unlocked on entry. On exit, the caller may
 *	not reference <ip> any more since the LWP no longer has
 *	a reference to the inode.
 */
void
xx_irele(inode_t *ip)
{
	vnode_t	*vp;

	vp = ITOV(ip);
	VN_LOCK(vp);
	if (vp->v_count == 1) {
		VN_UNLOCK(vp);
		xx_iinactive(ip, NODEISUNLOCKED);
	} else {
		vp->v_count--;
		VN_UNLOCK(vp);
	}
}

/*
 * int
 * xx_iflush(vfs_t *vfsp, int force)
 *	Purge any cached inodes on the given VFS.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode hash list locked.
 *
 * Description:
 *	If "force" is 0, -1 is returned if an active inode (other than the
 *	filesystem root) is found, otherwise 0.  If "force" is non-zero,
 *	the search doesn't stop if an active inode is encountered.
 */
int
xx_iflush(vfs_t *vfsp, int force)
{
	dev_t	dev;
	inode_t	*ip;
	inode_t	*ipx;
	int	i;
	pl_t	s;
	struct	hinode	*ih;
	vnode_t	*vp;
	vnode_t	*rvp;

	ip = NULL;
	rvp = XXFS(vfsp)->fs_root;
	dev = vfsp->vfs_dev;
	ASSERT(rvp != NULL);

	/*
	 * This search should run through the hash chains (rather
	 * than the entire inode table) so that we only examine
	 * inodes that we know are currently valid.
	 */
	s = LOCK(&xx_inode_table_mutex, FS_XXLISTPL);
	for (i = 0; i < NHINO; i++) {
		ih = &xx_hinode[i];
loop:
		for (ip = ih->i_forw; ip != (inode_t *)ih; ip = ipx) {
			ipx = ip->i_forw;
			if (ip->i_dev == dev) {
				vp = ITOV(ip);
				if (vp == rvp) {
					if (vp->v_count > 2 && force == 0) {
						UNLOCK(&xx_inode_table_mutex,s);
						return (-1);
					}
				continue;
				}
				if (vp->v_count == 0) {
					if (vp->v_vfsp == 0) {
						continue;
					}
					if (XX_ITRYWRLOCK(ip) ==
					    B_FALSE) {
						if (force) {
							continue;
						}
						UNLOCK(&xx_inode_table_mutex,s);
						return (-1);
					}
					/*
					 * Dispose this inode; remove
					 * it from its hash chain.
					 */
					xx_iunhash(ip);
					UNLOCK(&xx_inode_table_mutex, s);
					xx_syncip(ip, B_INVAL);
					XX_IRWLOCK_UNLOCK(ip);
					s = LOCK(&xx_inode_table_mutex,
						 FS_XXLISTPL);
					if (ip->i_forw != ipx) {
						goto loop;
					}
					
				} else if (force == 0) {
					UNLOCK(&xx_inode_table_mutex, s);
					return (-1);
				}
			}
		}
	}
	UNLOCK(&xx_inode_table_mutex, s);
	return (0);
}

#define TST_GROUP	3
#define TST_OTHER	6

/*
 * int
 * xx_iaccess(inode_t *ip, mode_t mode, cred_t *cr)
 *	Check mode permission on inode.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock
 *	minimally in shared mode.
 *
 * Description:
 *	Mode is READ, WRITE or EXEC. In the case of WRITE, the
 *	read-only status of the file system is checked. Also in
 *	WRITE, prototype text segments cannot be written. The
 *	mode is shifted to select the owner/group/other fields.
 */
int
xx_iaccess(inode_t *ip, int mode, cred_t *cr)
{
	int	denied_mode;
	int	lshift;
	int	i;
	vnode_t	*vp;

	vp = ITOV(ip);
	if ((mode & IWRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		return (EROFS);
	}

	if (cr->cr_uid == ip->i_uid) {
		lshift = 0;			/* TST OWNER */
	} else if (groupmember(ip->i_gid, cr)) {
		mode >>= TST_GROUP;
		lshift = TST_GROUP;
	} else {
		mode >>= TST_OTHER;
		lshift = TST_OTHER;
	}
	i = (ip->i_mode & mode);
	if (i == mode) {
		return (0);
	}
	if ((ip->i_mode & IEXEC == IEXEC) && is286EMUL) {
		return (0);
	}
	denied_mode = (mode & (~i));
	denied_mode <<= lshift;
	if ((denied_mode & (IREAD | IEXEC)) && pm_denied(cr, P_DACREAD)) {
		return (EACCES);
	}
	if ((denied_mode & IWRITE) && pm_denied(cr, P_DACWRITE)) {
		return (EACCES);
	}

	return (0);
}

/*
 * xx_inum(vfs_t *vfsp)
 *	Mark the inode stale.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 */
void
xx_inull(vfs_t *vfsp)
{
	dev_t	dev;
	inode_t	*ip;
	int	i;
	vnode_t	*vp;
	vnode_t	*rvp;

	ip = NULL;
	dev = vfsp->vfs_dev;
	rvp = XXFS(vfsp)->fs_root;
	ASSERT(rvp != NULL);

	for (i = 0, ip = xxinode; i < xx_ninode; i++, ip++) {
		if (ip->i_dev == dev) {
			vp = ITOV(ip);
			if (vp == rvp) {
				continue;
			}
			vp->v_vfsp = 0;
		}
	}
}

/*
 * int
 * xx_igrab(inode_t *ip)
 *	Lock and hold an inode that caller thinks is currently
 *	referenced but unlocked.
 *
 * Calling/Exit State:
 *	If these conditions no longer hold, then return 0;
 *	otherwise, return locked inode with an added reference.
 */
int
xx_igrab(inode_t *ip)
{
	vnode_t *vp;
	
	/*
	 * Race analysis:
	 *
	 * We know the inode is NOT on the free list because we check its
	 * count while holding the vnode lock.
	 *
	 * If reference count is 0 we are no longer interested, as the inode
	 * is (most likely) heading to the freelist, before which it will be
	 * sxxiupdat()ed, else (less likely) it got picked up by an xx_iget().
	 *
	 * As in xx_iinactive(), holding the vnode lock until we see if we
	 * can lock the inode (TRY_LOCK()) ensures that we don't race
	 * with xx_iget. If an xx_iget() was in the process of getting this
	 * inode, then *it* will have the inode locked at the point it trys
	 * to get the vnode lock in VN_HOLD. In this case TRY_LOCK() will fail,
	 * and releasing the vnode lock gives the inode to xx_iget().
	 *
	 * Code in xx_iget() is careful when pulling inode off free list to
	 * "lock" the inode and initialize v_count in that order, to avoid
	 * race with the test below. Also, VN_INIT() doesn't re-init
	 * locks to avoid races.
	 *
	 * If we increment reference count, then no one releasing the inode
	 * will try to put it on the free list. If someone does release
	 * the inode, then we MAY be the one who calls xx_iinactive() to
	 * free the inode.
	 */

	vp = ITOV(ip);

	if (vp->v_count != 0) {
		if (B_TRUE == XX_ITRYWRLOCK(ip)) {
			VN_LOCK(vp);
			if (vp->v_count != 0) {
				++vp->v_count;
				VN_UNLOCK(vp);
				return (1);
			}
			VN_UNLOCK(vp);
			XX_IRWLOCK_UNLOCK(ip);
		}
	}
	return (0);
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_xenix_inode(const inode_t *ip)
 *	Print a XENIX inode.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_xenix_inode(const inode_t *ip)
{
	debug_printf("XENIX inode = 0x%x\tvnode = 0x%x\n", ip, ITOV(ip));
	debug_printf("\tiflag = %8x  number = %d  dev = %d,%d  rdev = %d,%d\n",
			ip->i_flag, ip->i_number,
			getemajor(ip->i_dev), geteminor(ip->i_dev),
			getemajor(ip->i_rdev), geteminor(ip->i_rdev));
	debug_printf("\tlinks = %d  uid = %d  gid = %d  isize = %d\n",
			ip->i_nlink, ip->i_uid, ip->i_gid, ip->i_size);
	debug_printf("\timode = %06o  igen = %d  iaddr = %8x  nextr = %8x\n",
			ip->i_mode, ip->i_gen, ip->i_addr[0], ip->i_nextr);
}

#endif	/* defined(DEBUG) || defined(DEBUG_TOOLS) */

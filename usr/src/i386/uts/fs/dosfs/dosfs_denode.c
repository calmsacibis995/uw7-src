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

#ident	"@(#)kern-i386:fs/dosfs/dosfs_denode.c	1.7.1.1"

#include <util/mod/moddefs.h>
#include "dosfs.h"

STATIC int dosfs_load(void);
STATIC int dosfs_unload(void);

int dosfs_fsflags = 0;     /* to initialize vswp->vsw_flags */
int dosfs_fstype;	/* index into vfssw[] for this FS */

MOD_FS_WRAPPER(dosfs, dosfs_load, dosfs_unload, "Loadable DOS FS Type");

#define	DEHSZ	512
#if ((DEHSZ & (DEHSZ-1)) == 0)
#define	DEHASH(dev, deno)	(((dev)+(deno)+((deno)>>16))&(DEHSZ-1))
#else
#define	DEHASH(dev, deno)	(((dev)+(deno)+((deno)>>16))%DEHSZ)
#endif /* ((DEHSZ & (DEHSZ-1)) == 0) */

union dehead {
	union dehead *deh_head[2];
	struct denode *deh_chain[2];
} dehead[DEHSZ];




extern vfsops_t dosfs_vfsops;
extern long int		ndenode;


#ifndef NO_GENOFF
int					dosfs_shared; /* tells whether shared in kernel life */
#endif
struct denode*		denode;
struct denode*		dfreeh;		/* denode free head */
struct denode*		dfreet;		/* denode free tail */

static void
dosfs_doinit(void)
{
	struct denode*	dp;
	union dehead*	deh;
	int				i;

	denode = (struct denode *) kmem_zalloc(ndenode*sizeof(*dp), KM_SLEEP);

	if (denode == NULL)
	{
		cmn_err(CE_PANIC, "denodeinit: no memory for dosfs inodes");
	}

	dp = denode;

	for (i = DEHSZ, deh = dehead; --i >= 0; deh++)
	{
		deh->deh_head[0] = deh;
		deh->deh_head[1] = deh;
	}

	/*
	 * Setup free list of denodes, link all
	 * configured denodes and intialize them.
	 */

	dfreeh = dp;

	dp->de_freeb = NULL;

	for (i = ndenode;  ;)
	{
		dp->de_forw = dp;
		dp->de_back = dp;
		dp->de_vnode.v_data = (caddr_t) dp;
		vop_attach(&(dp->de_vnode), &dosfs_vnodeops);
		VN_INIT(&dp->de_vnode, NULL, 0, 0, 0, KM_SLEEP);
		SLEEP_INIT(&dp->de_lock, FS_DOSFSINOHIER, &dosfs_deno_lock_lkinfo,
					KM_SLEEP);

		if (--i == 0)
			break;

		dp->de_freef = dp+1;
		dp++;
		dp->de_freeb = dp-1;
	}

	dp->de_freef = NULL;
	dfreet = dp;

	LOCK_INIT(&dosfs_denode_table_mutex, FS_DOSFSLISTHIER, FS_DOSFSLISTPL,
                  &dosfs_denode_table_lkinfo, KM_SLEEP);

        SLEEP_INIT(&dosfs_updlock, (uchar_t) 0, &dosfs_updlock_lkinfo, KM_SLEEP);


}

dosfsinit(struct vfssw *vswp)
{
	dosfs_load();
	return;
}


/*
 * STATIC int 
 * dosfs_load(void)
 *	Initialize the denode table, denode table lock,
 *	and global dosfs synchronization objects.
 * 
 * Calling/Exit State:
 *	No locks held on entry and exit.
 *
 * Description:
 *	Called when loading a dosfs module. 
 *
 */
STATIC int
dosfs_load(void)
{
	if ((dosfs_fstype = vfs_attach("dosfs", &dosfs_vfsops, dosfs_fsflags))
	 == -1) {
		/*
                 *+ dosfs file system is not registered before
                 *+ attempting to load it.
                 */
                cmn_err(CE_NOTE, "!MOD: S5 is not registered.");
                return (EINVAL);
	}
	/* check tunable flushtime parameter */
	if (dosfs_tflush > v.v_autoup) {
		/*
		 *+ Invalid flush time parameter
		 */
		cmn_err(CE_NOTE, "S5FLUSH is invalid. It should be less than NAUTOUP");
                return (EINVAL);
	}
		
	dosfs_doinit();

#ifndef NO_GENOFF
	dosfs_shared		 = 0;
#endif
	
	return(0);
}

/*
 * STATIC int 
 * dosfs_unload(void)
 *	Deallocate the inode table, inode table lock,
 *	and global dosfs synchronization objects.
 * 
 * Calling/Exit State:
 *	No locks held on entry and exit.
 *
 * Description:
 *	Should be called when unloading a dosfs module. 
 *
 */
STATIC int
dosfs_unload()
{
	vfs_detach("dosfs");
	/*
	 * Free all inode storage.
	 */
	kmem_free(denode, ndenode * sizeof(struct denode));
	LOCK_DEINIT(&dosfs_denode_table_mutex);
        SLEEP_DEINIT(&dosfs_updlock);
	return(0);
}

static void
dehash(struct denode *dp, union dehead *deh)
{
	deh->deh_chain[0]->de_back = dp;
	dp->de_forw = deh->deh_chain[0];
	deh->deh_chain[0] = dp;
	dp->de_back = (struct denode *) deh;
}


void
deunhash(struct denode *dp)
{
	dp->de_back->de_forw = dp->de_forw;
	dp->de_forw->de_back = dp->de_back;
	dp->de_forw = dp->de_back = dp;
}


/*
 *  dosfs_deflush:	called by dosfs_sync, till now write-out
 *			all updated denodes and sync the buffers.
 */

void
dosfs_deflush(void)
{
	register struct denode	*dp;
	register struct vnode	*vp;
	register int			i;

	/*
	 * Update active, nolocked, changed denodes,
	 * which reside on read/write filesystems
	 * (deupdat does a lot of the checking).
	 */

	(void) LOCK(&dosfs_denode_table_mutex, FS_DOSFSLISTPL);
	for (i = 0, dp = denode; i < ndenode; i++, dp++)
	{
		if ((dp->de_flag & DEREF))
		{
			vp = DETOV(dp);

			ASSERT (vp->v_count != 0  &&  vp->v_vfsp != NULL);

			VN_HOLD (vp);
			UNLOCK(&dosfs_denode_table_mutex, PLBASE);
			DE_LOCK  (dp);

			(void)deupdat(dp, &hrestime, 0);

			deput (dp);
			(void) LOCK(&dosfs_denode_table_mutex, FS_DOSFSLISTPL);
		}
	}
	UNLOCK(&dosfs_denode_table_mutex, PLBASE);

	/*
	Now flush all blocks with a delayed write
	pending, all should be written now.
	*/

	bflush (NODEV);
}


/*
 *  deflush:	called by dosfs_unmount, write-out
 *		all updated denodes, sync the buffers
 *		and mark the denodes as stale.
 */

int
deflush (struct vfs *vfsp, int force)
{
	register struct denode	*dp;
	register struct vnode	*vp;
	register struct vnode	*rvp;
	register int i;
	dev_t dev;

	dev = vfsp->vfs_dev;
	rvp = ((struct dosfs_vfs *)(vfsp->vfs_data))->vfs_root;

	/*
	This search should run through the hash chains (rather
	than the entire inode table) so that we only examine
	inodes that we know are currently valid.
	*/

	(void) LOCK(&dosfs_denode_table_mutex, FS_DOSFSLISTPL);
	for (i = 0, dp = denode; i < ndenode; i++, dp++)
	{
		if (dp->de_dev != dev)
			continue;

		vp = DETOV(dp);
		if (vp->v_vfsp == 0)
			continue;

		if (vp == rvp) {
			if (vp->v_count > 2 && force == 0) {
				UNLOCK(&dosfs_denode_table_mutex, PLBASE);
				return -1;
			}
			continue;
		}

		VN_LOCK(vp);
		if (vp->v_count == 0) {
			VN_UNLOCK(vp);
			if (!DE_TRYLOCK(dp)) {
				if (force)
					continue;
				UNLOCK(&dosfs_denode_table_mutex, PLBASE);
				return -1;
			}

			UNLOCK(&dosfs_denode_table_mutex, PLBASE);
			(void)deupdat(dp, &hrestime, 0);
			DE_UNLOCK(dp);
			deunhash(dp);
			vp->v_vfsp = 0;
			(void) LOCK(&dosfs_denode_table_mutex, FS_DOSFSLISTPL);
		} else if (force == 0) {
			UNLOCK(&dosfs_denode_table_mutex, PLBASE);
			VN_UNLOCK(vp);
			return -1;
		}
	}

	UNLOCK(&dosfs_denode_table_mutex, PLBASE);
	return 0;
}

/*
 *  If deget() succeeds it returns with the gotten denode
 *  locked().
 *  pvp - address of dosfs_vfs structure of the filesystem
 *    containing the denode of interest.  The vfs_dev field
 *    and the address of the dosfs_vfs structure are used. 
 *  isadir - a flag used to indicate whether the denode of
 *    interest represents a file or a directory.
 *  dirclust - which cluster bp contains, if dirclust is 0
 *    (root directory) diroffset is relative to the beginning
 *    of the root directory, otherwise it is cluster relative.
 *  diroffset - offset past begin of cluster of denode we
 *    want
 *  startclust - number of 1st cluster in the file the
 *    denode represents.  Similar to an inode number.
 *  bp - address of the buf header for the buffer containing
 *    the direntry structure of interest.
 *  depp - returns the address of the gotten denode.
 */

int
deget(vfs_t *vfsp, int isadir, u_long dirclust, u_long diroffset, u_long startclust, dosdirent_t *direntp, struct denode **depp)

/* vfsp - so we know the maj/min number */
/* isadir -  ~0 means the denode is a directory */
/* dirclust - cluster this dir entry came from	*/
/* diroffset - index of entry within the cluster */
/* startclust - # of the 1st cluster in file this 'de' points to */
/* direntp - pointer to the dos dir entry	*/
/* depp - returns the addr of the gotten denode */
{
	dosfs_vfs_t *pvp = vfsp->vfs_data;
	dev_t dev = pvp->vfs_dev;
	union dehead *deh;
	struct denode *ldep;
	struct vnode *nvp;
	int count = 0;

	/*
	 * See if the denode is in the denode cache.
	 * If the denode is for a directory then use the
	 * startcluster in computing the hash value.  If
	 * a regular file then use the location of the directory
	 * entry to compute the hash value.  We use startcluster
	 * for directories because several directory entries
	 * may point to the same directory.  For files
	 * we use the directory entry location because
	 * empty files have a startcluster of 0, which
	 * is non-unique and because it matches the root
	 * directory too.  I don't think the dos filesystem
	 * was designed.
	 * 
	 * NOTE: The check for de_refcnt > 0 below insures the denode
	 * being examined does not represent an unlinked but
	 * still open file.  These files are not to be accessible
	 * even when the directory entry that represented the
	 * file happens to be reused while the deleted file is still
	 * open.
	*/

	if (isadir) {
		deh = &dehead[DEHASH(dev, startclust)];
	} else {
		deh = &dehead[DEHASH(dev, dirclust+diroffset)];
	}

loop:
	(void) LOCK(&dosfs_denode_table_mutex, FS_DOSFSINOPL);
	for (ldep = deh->deh_chain[0]; ldep != (struct denode *)deh;
	    ldep = ldep->de_forw) {
		if (dev == ldep->de_dev  &&  ldep->de_refcnt > 0 &&
		    vfsp == DETOV(ldep)->v_vfsp) {
			if (ISADIR(ldep)) {
				if (ldep->de_StartCluster != startclust  || !isadir)
					continue;
			} else {
				if (isadir  || dirclust  != ldep->de_dirclust ||
				    diroffset != ldep->de_diroffset)
					continue;
			}

			/*
			 * Remove from freelist if it's on.
			 */

			if ((ldep->de_flag & DEREF) == 0) {
				if (ldep->de_freeb)
					ldep->de_freeb->de_freef =
								ldep->de_freef;
				else
					dfreeh = ldep->de_freef;

				if (ldep->de_freef)
					ldep->de_freef->de_freeb =
								ldep->de_freeb;
				else
					dfreet = ldep->de_freeb;

				ldep->de_freef = NULL;
				ldep->de_freeb = NULL;
			}

			ldep->de_flag |= DEREF;
			SLEEP_LOCK_RELLOCK(&ldep->de_lock, PRINOD,
						&dosfs_denode_table_mutex);
			VN_HOLD (DETOV(ldep));

			*depp = ldep;
			return 0;
		}
	}


	/*
	 * We can enter with direntp == NULL in case of a vget;
	 * read in the dirent, cleanup and reloop as we might
	 * fall asleep along this way.
	 */

	if (direntp == NULL  &&  (startclust != DOSFSROOT  ||  !isadir)) {
		dosdirent_t		dirent;

		if (get_direntp (pvp, dirclust, diroffset, &dirent) ||
					  dirent.deName[0] == SLOT_DELETED ||
					      dirent.deName[0] == SLOT_EMPTY) {
			UNLOCK(&dosfs_denode_table_mutex, PLBASE);
			return ENOENT;
		}

		direntp = &dirent;

		goto loop;
	}

	/*
	 * Directory entry was not in cache, have to get
	 * one from the free list. If the list is empty
	 * try to purge the cache.
	 * Note that dnlc_purge_vfsp can (indirectly) sleep
	 * so we have to check again and see what happened
	 * meanwhile (someone else could have "degotten"
	 * the same file).
	 */

	count++;
	if (dfreeh == NULL || !SLEEP_LOCKAVAIL(&dfreeh->de_lock)) {

		UNLOCK(&dosfs_denode_table_mutex, PLBASE);
		(void) dnlc_purge_vfsp(vfsp, 5);

		if (dfreeh == NULL || count > 5) {
			cmn_err (CE_WARN, "deget: out of dosfs inodes");
			return  ENFILE;
		} else {
			goto loop;
		}
	}

	ldep = dfreeh;

	/*
	 * Got one, remove it from the freelist,
	 * the hash list and reinitialize it.
	 */

	dfreeh = ldep->de_freef;

	if (dfreeh) {
		dfreeh->de_freeb = NULL;
	} else {
		dfreet = NULL;
	}

	ldep->de_freef = NULL;
	ldep->de_freeb = NULL;

	deunhash (ldep);

	ldep->de_flag  = DEREF;
	ldep->de_devvp = 0;
	ldep->de_dev   = dev;

	fc_purge(ldep, 0);	/* init the fat cache for this denode */

	/*
	 * Insert the denode into the hash queue and lock the
	 * denode so it can't be accessed until we've read it
	 * in and have done what we need to it.
	 */

	ASSERT(SLEEP_LOCKAVAIL(&ldep->de_lock));
	DE_TRYLOCK(ldep);

	dehash(ldep, deh);
	UNLOCK(&dosfs_denode_table_mutex, PLBASE);

	/*
	 * Copy the directory entry into the denode area of the
	 * vnode.  If they are going after the directory entry
	 * for the root directory, there isn't one so we manufacture
	 * one.
	 * We should probably rummage through the root directory and
	 * find a label entry (if it exists), and then use the time
	 * and date from that entry as the time and date for the
	 * root denode.
	 */

	if (startclust == DOSFSROOT  &&  isadir) {
		ldep->de_Attributes = ATTR_DIRECTORY;
		ldep->de_StartCluster = DOSFSROOT;
		ldep->de_FileSize = 0;

		/*
		 * Fill in time and date so that dos2unixtime() doesn't split
		 * up when called from dosfs_getattr() with root denode
		 */

		ldep->de_Time = 0x0000;		/* 00:00:00	*/
		ldep->de_Date = (0 << 9) | (1 << 5) | (1 << 0);
		/* Jan 1, 1980	*/
		ldep->de_vfs  = pvp;
		/* leave the other fields as garbage */
	} else {
		ldep->de_de = *direntp;
	}

	/*
	 * Fill in a few fields of the vnode and finish filling
	 * in the denode.  Then return the address of the found
	 * denode.
	 */

	nvp = DETOV(ldep);

	if (ldep->de_Attributes & ATTR_DIRECTORY)
		nvp->v_type = VDIR;
	else
		nvp->v_type = VREG;

	nvp->v_flag = VNOMAP;

	nvp->v_count   = 1;
	nvp->v_softcnt   = 0;
	nvp->v_vfsp    = pvp->vfs_vfsp;
	nvp->v_stream  = NULL;
	nvp->v_pages   = NULL;
	nvp->v_filocks = NULL;
	nvp->v_rdev    = pvp->vfs_dev;

	ldep->de_vfs       = pvp;
	ldep->de_devvp     = pvp->vfs_devvp;
	ldep->de_refcnt    = 1;
	ldep->de_dirclust  = dirclust;
	ldep->de_diroffset = diroffset;

	*depp = ldep;

	return 0;
}

void
deput(struct denode *dep)
{

	DE_UNLOCK(dep);

	VN_RELE(DETOV(dep));
}

int
deupdat (struct denode *dep, timestruc_t *tp, int waitfor)
{
	int error;
	lbuf_t *bp;
	struct direntry *dirp;
	struct vnode *vp = DETOV(dep);

	ASSERT(SLEEP_LOCKOWNED(&dep->de_lock));

	/*
	 * If the update bit is off, or this denode is from
	 * a readonly filesystem, or this denode is for a root
	 * directory, or the denode represents an open but
	 * unlinked file then don't do anything.
	 */

	if ((dep->de_flag & (DEUPD|DEMOD)) == 0  ||
	    (vp->v_vfsp->vfs_flag & VFS_RDONLY) ||
	    vp->v_flag & VROOT)
	{
		return 0;
	}

	/*
	 * Read in the cluster containing the directory entry
	 * we want to update.
	 */

	if (error = readde(dep, &bp, &dirp))
	{
		return error;
	}

	/*
	 * Put the passed in time into the directory entry.
	 */

	unix2dostime(tp, (dosdate_t *)&dep->de_Date, (dostime_t *)&dep->de_Time);

	dep->de_flag &= ~(DEUPD|DEMOD);

	/*
	Copy the directory entry out of the denode into
	the cluster it came from.
	*/

	*dirp = dep->de_de;	/* structure copy */

	/*
	Write the cluster back to disk.  If they asked
	for us to wait for the write to complete, then
	use lbwrite() otherwise use lbdwrite().
	*/

	if (waitfor)
	{
		error = lbwritewait(bp);
	}
	else
	{
		lbdwrite(bp);
	}

	return error;
}

/*
 *  Truncate the file described by dep to the length
 *  specified by length.
 */
int
detrunc(struct denode *dep, u_long length, int flags)
{
	int error;
	int allerror;
	unsigned long eofentry;
	unsigned long chaintofree;
	u_long bn;
	int boff;
	int isadir = dep->de_Attributes & ATTR_DIRECTORY;
	lbuf_t *bp;
	struct dosfs_vfs *pvp = dep->de_vfs;

	ASSERT(SLEEP_LOCKOWNED(&dep->de_lock));

	/*
	 * Disallow attempts to truncate the root directory
	 * since it is of fixed size.  That's just the way
	 * dos filesystems are.  We use the VROOT bit in the
	 * vnode because checking for the directory bit and
	 * a startcluster of 0 in the denode is not adequate
	 * to recognize the root directory at this point in
	 * a file or directory's life.
	 */

	if (DETOV(dep)->v_flag & VROOT)
	{
		cmn_err (CE_WARN, "detrunc: can't truncate root directory, clust %ld, offset %ld\n",
		    dep->de_dirclust, dep->de_diroffset);
		return EINVAL;
	}

	/*
	 * If we are going to truncate a directory then we better
	 * find out how long it is.  DOS doesn't keep the length of
	 * a directory file in its directory entry.
	 */

	if (isadir)
	{
		/* pcbmap() returns the # of clusters in the file */
		error = pcbmap(dep, 0xffff, 0, &eofentry);
		if (error != 0  &&  error != E2BIG)
			return error;
		dep->de_FileSize = eofentry << pvp->vfs_cnshift;
	}

	if (dep->de_FileSize <= length)
	{
		dep->de_flag |= DEUPD;
		error =  deupdat(dep, &hrestime, 1);
		return error;
	}

	/*
	 * If the desired length is 0 then remember the starting
	 * cluster of the file and set the StartCluster field in
	 * the directory entry to 0.  If the desired length is
	 * not zero, then get the number of the last cluster in
	 * the shortened file.  Then get the number of the first
	 * cluster in the part of the file that is to be freed.
	 * Then set the next cluster pointer in the last cluster
	 * of the file to CLUST_EOFE.
	 */

	if (length == 0)
	{
		chaintofree = dep->de_StartCluster;
		dep->de_StartCluster = 0;
		eofentry = (u_long) ~0;
	}
	else
	{
		error = pcbmap(dep, (u_long)((length-1) >> pvp->vfs_cnshift),
								0, &eofentry);
		if (error)
		{
			return error;
		}
	}

	fc_purge(dep, (length + pvp->vfs_crbomask) >> pvp->vfs_cnshift);

	/*
	 * If the new length is not a multiple of the cluster size
	 * then we must zero the tail end of the new last cluster in case
	 * it becomes part of the file again because of a seek.
	 */

	if ((boff = length & pvp->vfs_crbomask) != 0)
	{
		bn = cntobn(pvp, eofentry);
		bp = lbread(pvp->vfs_dev, bn, pvp->vfs_bpcluster);

		if (error = lgeterror(bp))
		{
			lbrelse(bp);
			return error;
		}

		bzero(bp->b_un.b_addr + boff, pvp->vfs_bpcluster - boff);

		if (flags & IO_SYNC)
		{
			lbwrite(bp);
		}
		else
		{
			lbdwrite(bp);
		}
	}

	/*
	 * Write out the updated directory entry.  Even
	 * if the update fails we free the trailing clusters.
	 */

	dep->de_FileSize = length;
	dep->de_flag |= DEUPD;

	allerror = deupdat(dep, &hrestime, 1);

	/*
	 * If we need to break the cluster chain for the file
	 * then do it now.
	 */

	if (eofentry != (u_long) ~0)
	{
		error = fatentry(FAT_GET_AND_SET, pvp, eofentry,
		    &chaintofree, CLUST_EOFE);
		if (error)
		{
			return error;
		}

		fc_setcache(dep, FC_LASTFC, (length - 1) >> pvp->vfs_cnshift,
								eofentry);
	}

	/*
	 * Now free the clusters removed from the file because
	 * of the truncation.
	 */

	if (chaintofree != 0  &&  !DOSFSEOF(chaintofree))
		(void)freeclusterchain(pvp, chaintofree);

	return allerror;
}

/*
 *  Move a denode to its correct hash queue after
 *  the file it represents has been moved to a new
 *  directory.
 */
void
reinsert(struct denode *dep)
{
	struct dosfs_vfs *pvp = dep->de_vfs;
	union dehead *deh;

	/*
	 * Fix up the denode cache.  If the denode is
	 * for a directory, there is nothing to do since the
	 * hash is based on the starting cluster of the directory
	 * file and that hasn't changed.  If for a file the hash
	 * is based on the location
	 * of the directory entry, so we must remove it from the
	 * cache and re-enter it with the hash based on the new
	 * location of the directory entry.
	 */

	if ((dep->de_Attributes & ATTR_DIRECTORY) == 0)
	{
		(void) LOCK(&dosfs_denode_table_mutex, FS_DOSFSINOPL);
		deunhash(dep);
		deh = &dehead[DEHASH(pvp->vfs_dev,
			dep->de_dirclust + dep->de_diroffset)];
		dehash(dep, deh);
		UNLOCK(&dosfs_denode_table_mutex, PLBASE);
	}
}


/*
 * dosfs_inactive:
 * Called when the last reference of the file
 * is released. If the file has been removed
 * meanwhile this removal gets active now.
 */

/*ARGSUSED*/
void
dosfs_inactive(struct vnode *vp, struct cred *cr)
{
	struct denode *dep = VTODE(vp);

	ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);

	DE_LOCK(dep);

	VN_LOCK(vp);
	ASSERT(vp->v_count != 0);
	if (vp->v_count != 1) {
		/*
                 * Someone generated a new reference before we acquired
                 * both locks.
                 *
                 * The new reference is still extant, so that inactivating
                 * the vnode will become someone else's responsibility. Give
                 * up our reference and return.
                 */

		vp->v_count--;
		VN_UNLOCK(vp);
		DE_UNLOCK(dep);
		return;
	} 

	/*
	 * The reference count is exactly 1, so that now we can be sure
	 * that we really hold the last hard reference.
	 *
	 */
	vp->v_count = 0;
	VN_UNLOCK(vp);

	/*
	 * If someone else is blocked in deget() trying to acquire
	 * this denode, we short-circuit the inactive process and
	 * let the other one get a handle of this denode.
	 */

	(void) LOCK(&dosfs_denode_table_mutex, FS_DOSFSLISTPL);
	if (SLEEP_LOCKBLKD(&dep->de_lock)) {
		UNLOCK(&dosfs_denode_table_mutex, PLBASE);
		DE_UNLOCK(dep);
		return;
	}

	/*
	 * If we are done with the denode, then insert
	 * it so that it can be reused now, else put on
	 * the end of the queue.
	 */

	if (dep->de_refcnt <= 0 || dfreeh == NULL)
	{
		deunhash(dep);

		dep->de_freef = dfreeh;
		dep->de_freeb = NULL;

		if (dfreeh == NULL)
		{
			dfreeh = dep;
			dfreet = dep;
		}
		else
		{
			dfreeh->de_freeb = dep;
			dfreeh           = dep;
		}
	}
	else
	{
		dep->de_freef    = NULL;
		dep->de_freeb    = dfreet;
		dfreet->de_freef = dep;
		dfreet           = dep;
	}
	UNLOCK(&dosfs_denode_table_mutex, PLBASE);


	/*
	 * If the file has been deleted then truncate the file.
	 * (This may not be necessary for the dos filesystem.)
	*/

	if (dep->de_refcnt <= 0)
	{
#ifndef NO_GENOFF
		/*
		Fill in a new generation number for the
		next occupying this denode.
		*/

		if (dosfs_shared)
		{
			DE_GEN(dep)++;
		}
#endif
		dep->de_Name[0] = SLOT_DELETED;
		(void) detrunc(dep, (u_long)0, 0);
	} else {
		DEUPDAT(dep, &hrestime, 0);
	}

	dep->de_flag = 0;

	DE_UNLOCK(dep);

}

#ifdef DEBUG

void
print_dosnode(const struct denode *dep)
{
	struct vnode *vp = DETOV(dep);

	debug_printf("\tforw = 0x%x, back = 0x%x, freef = 0x%x, freeb = 0x%x\n",
		dep->de_chain[0], dep->de_chain[1], dep->de_freef,
							dep->de_freeb);
	debug_printf("\tdeflag = 0x%x, dev = 0x%x, dirclust = %d, diroffset = %d\n",
		dep->de_flag, dep->de_dev, dep->de_dirclust, dep->de_diroffset);
	debug_printf("\tfrspace = %d, froffset = %d, frcluster = %d\n\n",
		dep->de_frspace, dep->de_froffset, dep->de_frcluster);

	debug_printf("direntry:\n");
	debug_printf("\tName = %-8s, Extension = %-3s, Attr = %c\n",
		dep->de_Name, dep->de_Extension, dep->de_Attributes);
	debug_printf("\tTime = %d, Date = %d, StartClust = %d\n",
		dep->de_Time, dep->de_Date, dep->de_StartCluster);

	debug_printf("\tde_fc[0].fc_frcn = %d, fc_fsrcn = %d\n",
		dep->de_fc[0].fc_frcn, dep->de_fc[0].fc_fsrcn);
	debug_printf("\tde_fc[1].fc_frcn = %d, fc_fsrcn = %d\n",
		dep->de_fc[1].fc_frcn, dep->de_fc[1].fc_fsrcn);

	debug_printf("\tde_FileSize = %d, dosfs_vfs = 0x%x\n",
					dep->de_FileSize, dep->de_vfs);

	debug_printf("\tvcount = %d, vtype = %d, vfsp = %x\n",
		vp->v_count, vp->v_type, vp->v_vfsp);
	
}

void
print_dosvfs(const struct dosfs_vfs *pvp)
{
	debug_printf("\tvfsp = 0x%x, devvp= 0x%x, rootvp = 0x%x\n",
		pvp->vfs_vfsp, pvp->vfs_devvp, pvp->vfs_root);
	debug_printf("\tfatblk = %d, rootdirblk= %d, rootdirsize = %d\n",
		pvp->vfs_fatblk, pvp->vfs_rootdirblk, pvp->vfs_rootdirsize);
	debug_printf("\tfirstclust = %d, nclust= %d, maxclust = %d\n",
		pvp->vfs_firstcluster, pvp->vfs_nmbrofclusters,
		pvp->vfs_maxcluster);
	debug_printf("\tfreeclust = %d, lookhere= %d, cnshift = %d\n",
		pvp->vfs_freeclustercount, pvp->vfs_lookhere,
		pvp->vfs_cnshift);
	debug_printf("\tbpcluster = %d, depclust= %d\n",
		pvp->vfs_bpcluster, pvp->vfs_depclust);
	debug_printf("\tfmod = %d, ronly= %c, waitonfat = %c, inusemap = 0x%x\n",
		pvp->vfs_fmod, pvp->vfs_ronly, pvp->vfs_waitonfat,
		pvp->vfs_inusemap);
	debug_printf("bpb50:\n");
	debug_printf("\tBytesPerSec = %d, SecPerClust = %c, ResSectors = %d\n",
					pvp->vfs_bpb.bpbBytesPerSec,
					pvp->vfs_bpb.bpbSecPerClust,
					pvp->vfs_bpb.bpbResSectors);
	debug_printf("\tFATs = %c, RootDirEnts = %d, Sectors = %d\n",
					pvp->vfs_bpb.bpbFATs,
					pvp->vfs_bpb.bpbRootDirEnts,
					pvp->vfs_bpb.bpbSectors);
	debug_printf("\tMedia = %c, FATsecs = %d, SecPerTrack = %d\n",
					pvp->vfs_bpb.bpbMedia,
					pvp->vfs_bpb.bpbFATsecs,
					pvp->vfs_bpb.bpbSecPerTrack);
	debug_printf("\tHeads = %d, HiddenSecs = %d, HugeSectors = %d\n",
					pvp->vfs_bpb.bpbHeads,
					pvp->vfs_bpb.bpbHiddenSecs,
					pvp->vfs_bpb.bpbHugeSectors);


}

#endif

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

#ident	"@(#)kern-i386:fs/dosfs/dosfs_lookup.c	1.5.1.2"

#include "dosfs.h"

/*
 *  When we search a directory the blocks containing directory
 *  entries are read and examined.  The directory entries
 *  contain information that would normally be in the inode
 *  of a unix filesystem.  This means that some of a directory's
 *  contents may also be in memory resident denodes (sort of
 *  an inode).  This can cause problems if we are searching
 *  while some other process is modifying a directory.  To
 *  prevent one process from accessing incompletely modified
 *  directory information we depend upon being the soul owner
 *  of a directory block.  lbread/lbrelse provide this service.
 *  This being the case, when a process modifies a directory
 *  it must first acquire the disk block that contains the
 *  directory entry to be modified.  Then update the disk
 *  block and the denode, and then write the disk block out
 *  to disk.  This way disk blocks containing directory
 *  entries and in memory denode's will be in synch.
 *
 * Locking: dvp locked on entry. *vpp locked on exit if !error.
 *
 */


/*ARGSUSED*/
int
dosfs_dirlook(vnode_t *dvp, char *name, vnode_t **vpp, int flags, cred_t *cr)
{
	u_long		bn;
	int		error;
#define	NONE	0
#define	FOUND	1
	int		slotstatus;
	int		slotoffset;
	int		slotcluster;
	u_long		frcn;
	vnode_t		*vp;
	ulong_t		cluster;
	int		rootreloff;
	ulong_t		diroff;
	int		isadir;	/* ~0 if found direntry is a directory	*/
	ulong_t		scn;	/* starting cluster number		*/
	denode_t	*dp;
	denode_t	*tdp;
	dosfs_vfs_t	*pvp;
	lbuf_t		*bp;
	dosdirent_t	*dep;
	dosdirent_t	direntry;
	int		nlen;
	unsigned char	nm[DOSDIRSIZ+1];
	void		*cookie;
	boolean_t	softhold;
	long		entry;

	/*
	Be sure dp is a directory.  Since dos filesystems
	don't have the concept of execute permission anybody
	can search a directory.
	*/

	dp  = VTODE(dvp);
	pvp = dp->de_vfs;


	if (!ISADIR(dp))
	{
		return ENOTDIR;
	}


	nlen = strlen(name);

	if ( !nlen  ||  nlen == 1 && name[0] == '.' )
	{
		/*
		No name or '.' equals this directory.
		*/

		VN_HOLD(dvp)
		    *vpp = dvp;

		return 0;
	}

	/*
	We don't expect to be here for ".." in the
	root directory.
	*/

	ASSERT (!(dvp->v_flag & VROOT) || nlen != 2 ||
	    name[0] != '.'  || name[1] != '.');

	if (flags & GETFREESLOT)
	{
		slotstatus = NONE;
		slotoffset = -1;
	}
	else
	{
		slotstatus = FOUND;
	}

	if(error = unix2dosfn ((unsigned char *)name, nm, nlen))
		return error;
	nm[DOSDIRSIZ] = '\0';

	/*
	See if the component of the pathname we are looking for
	is in the directory cache.  If so then do a few things
	and return.
	*/

	vp = dnlc_lookup(dvp, (char*)nm, &cookie, &softhold, NOCRED);
	if (vp != NULL) {
		ASSERT(softhold == B_FALSE);
		DE_LOCK(VTODE(vp));
		*vpp = vp;

		return 0;
	}

	/*
	Search the directory pointed at by dvp for the
	name pointed at by nm.
	*/

	tdp = NULL;

	/*
	The outer loop ranges over the clusters that make
	up the directory.  Note that the root directory is
	different from all other directories.  It has a
	fixed number of blocks that are not part of the
	pool of allocatable clusters.  So, we treat it a
	little differently.
	The root directory starts at "cluster" 0.
	*/

	rootreloff = 0;
	bp         = 0;

	for (frcn = 0; ; frcn++)
	{
		if (error = pcbmap(dp, frcn, &bn, &cluster))
		{
			if (error == E2BIG)
				break;

			return error;
		}

		bp = lbread (pvp->vfs_dev, bn, pvp->vfs_bpcluster);

		if (error = lgeterror(bp))
		{
			lbrelse   (bp);
			return error;
		}

		dep = (struct direntry *)bp->b_un.b_addr;

		if (dp->de_StartCluster == DOSFSROOT &&
		    pvp->vfs_SectPerClust > pvp->vfs_rootdirsize)
			entry = (pvp->vfs_rootdirsize * pvp->vfs_BytesPerSec) /
				sizeof(struct direntry);
		else
			entry = pvp->vfs_depclust;

		for (diroff = 0;  diroff < entry;  diroff++, dep++)
		{

			/*
			If the slot is empty and we are still looking for
			an empty then remember this one.  If the slot is
			not empty then check to see if it matches what we
			are looking for.  If the slot has never been filled
			with anything, then the remainder of the directory
			has never been used, so there is no point in searching
			it.
			*/

			if (dep->deName[0] == SLOT_EMPTY || dep->deName[0] == SLOT_DELETED)
			{
				if (slotstatus != FOUND)
				{
					slotstatus = FOUND;

					if (cluster == DOSFSROOT)
					{
						slotoffset = rootreloff;
					}
					else
					{
						slotoffset = diroff;
					}

					slotcluster = cluster;
				}

				if (dep->deName[0] == SLOT_EMPTY)
				{
					lbrelse(bp);
					goto notfound;
				}
			}
			else
			{
				/*
				Ignore volume labels (anywhere, not
				just the root directory).
				*/

				if ((dep->deAttributes & ATTR_VOLUME) == 0  &&
				    bcmp((char *)nm, (char *)dep->deName, 11) == 0)
				{
					/*
					Remember where this directory entry came from
					for whoever did this lookup.
					If this is the root directory we are interested
					in the offset relative to the beginning of the
					directory (not the beginning of the cluster).
					*/

					if (cluster == DOSFSROOT)
						diroff = rootreloff;

					goto found;
				}
			}
			rootreloff++;
		}

		/*
		Release the buffer holding the directory cluster
		just searched.
		*/

		lbrelse(bp);
	}


notfound:
	;

	/*
	If we get here we didn't find the entry we were looking
	for.  But that's ok if we are creating or renaming and
	are at the end of the pathname and the directory hasn't
	been removed.
	Note that we hold no disk buffers at this point.
	*/

	if (flags & GETFREESLOT)
	{
		if (slotstatus == NONE)
		{
			dp->de_froffset  = 0;
			dp->de_frcluster = 0;
			dp->de_frspace   = 0;
		}
		else
		{
			dp->de_froffset  = slotoffset;
			dp->de_frcluster = slotcluster;
			dp->de_frspace   = 1;
		}
	}

	return ENOENT;


found:
	;

	/*
	NOTE:  We still have the buffer with matched
	directory entry at this point. As deget may
	sleep we copy the entry and release the buf.
	*/

	direntry = *dep;
	isadir   = dep->deAttributes & ATTR_DIRECTORY;
	scn      = dep->deStartCluster;

	lbrelse(bp);

	error = deget(dvp->v_vfsp, isadir, cluster, diroff, scn, &direntry, &tdp);

	if (!error)
	{
		vp = DETOV(tdp);
		dnlc_enter (dvp, (char*) nm, vp, NULL, NOCRED);
		*vpp = vp;
	}

	return error;
}

/*
 *  dep - directory to copy into the directory
 *  ndp - nameidata structure containing info on
 *    where to put the directory entry in the directory.
 *  depp - return the address of the denode for the
 *    created directory entry if depp != 0
 *
 * Locking : dep and dvp are locked on entry, remain locked at exit.
 *		*depp locked at exit.
 */
int
createde(struct denode *dep, struct vnode *dvp, struct denode **depp)
{
	int				bn;
	int				error;
	int				theoff;
	int				isadir = ISADIR(dep);
	unsigned long	newcluster;
	dosdirent_t		*ndep;
	struct denode	*ddep = VTODE(dvp);	/* directory to add to */
	struct dosfs_vfs	*pvp = dep->de_vfs;
	lbuf_t		*bp;

	/*
	If no space left in the directory then allocate
	another cluster and chain it onto the end of the
	file.  There is one exception to this.  That is,
	if the root directory has no more space it can NOT
	be expanded.  extendfile() checks for and fails attempts to
	extend the root directory.  We just return an error
	in that case.
	*/

	if (!ddep->de_frspace)
	{
		if (error = extendfile(ddep, 0, &newcluster))
		{
			return error;
		}

		/*
		If they want us to return with the denode gotten.
		*/

		if (depp)
		{
			error = deget(dvp->v_vfsp, isadir, newcluster, 0, (u_long)dep->de_StartCluster,
			    &dep->de_de, depp);
			if (error)
			{
				return error;
			}
		}

		bp = lgetblk(pvp->vfs_dev, cntobn(pvp,newcluster), pvp->vfs_bpcluster);
		lclrbuf(bp);

		ndep  = (dosdirent_t *) paddr(bp);
#ifndef NO_GENOFF
		if (depp) {
			DE_GEN(*depp) = DIR_GEN(ndep);
		}
#endif
		*ndep = dep->de_de;

		if (error = lbwritewait(bp))
		{
			if (depp)
			{
				deput (*depp);
			}

			return error;
		}

		/*
		Let caller know where we put the directory entry. who ???
		*/

		ddep->de_frcluster = newcluster;
		ddep->de_froffset  = 0;
		return 0;
	}

	/*
	There is space in the existing directory.  So,
	we just read in the cluster with space.  Copy
	the new directory entry in.  Then write it to
	disk.
	NOTE:  DOS directories do not get smaller as
	clusters are emptied.
	*/

	if (ddep->de_frcluster == DOSFSROOT)
	{
		bn = pvp->vfs_rootdirblk + (ddep->de_froffset /
			(pvp->vfs_BytesPerSec/sizeof (struct direntry)));
		theoff = ddep->de_froffset %
			(pvp->vfs_BytesPerSec/sizeof (struct direntry));
	}
	else
	{
		bn     = cntobn(pvp, ddep->de_frcluster);
		theoff = ddep->de_froffset;
	}

	/*
	If they want us to return with the denode gotten.
	*/

	if (depp)
	{
		error = deget(dvp->v_vfsp, isadir, ddep->de_frcluster, ddep->de_froffset,
		    (u_long)dep->de_StartCluster, &dep->de_de, depp);
		if (error)
		{
			return error;
		}
	}

	bp = lbread (pvp->vfs_dev, bn, pvp->vfs_bpcluster);

	if (error = lgeterror(bp))
	{
		if (depp)
		{
			deput (*depp);
		}

		lbrelse(bp);
		return error;
	}

	ndep  = (struct direntry *)(bp->b_un.b_addr) + theoff;
	*ndep = dep->de_de;

	if (error = lbwritewait(bp))
	{
		if (depp)
		{
			deput (*depp);
		}
	}

	return error;
}

/*
 *  Read in a directory entry and mark it as being deleted.
 */
int
markdeleted(struct dosfs_vfs *pvp, u_long dirclust, u_long diroffset)
{
	int offset;
	int error;
	u_long bn;
	struct direntry *ep;
	lbuf_t *bp;

	if (dirclust == DOSFSROOT)
	{
		bn = pvp->vfs_rootdirblk + (diroffset /
			(pvp->vfs_BytesPerSec/sizeof (struct direntry)));
		offset = diroffset %
			(pvp->vfs_BytesPerSec/sizeof (struct direntry));
	}
	else
	{
		bn = cntobn(pvp, dirclust);
		offset = diroffset;
	}

	bp = lbread(pvp->vfs_dev, bn, pvp->vfs_bpcluster);

	if (error = lgeterror(bp))
	{
		lbrelse(bp);
		return error;
	}

	ep = (struct direntry *)bp->b_un.b_addr + offset;
	ep->deName[0] = SLOT_DELETED;

	/*
	Might do lbdwrite here; ref to zero
	entry out of dnlc, block update
	follows via dosfs_inactive???
	*/

	lbdwrite (bp);

	return 0;
}

/*
 *  Remove a directory entry.
 *  At this point the file represented by the directory
 *  entry to be removed is still full length until no
 *  one has it open.  When the file no longer being
 *  used dosfs_inactive() is called and will truncate
 *  the file to 0 length.
 *
 * Locking: vp held and locked on entry, released and unlocked at exit.
 *
 */
int
removede(struct vnode *vp)
{
	struct denode	*dp;
	struct dosfs_vfs	*pvp;
	int				error;

	dp  = VTODE(vp);		/* the file being removed */
	pvp = dp->de_vfs;

	ASSERT(SLEEP_LOCKOWNED(&dp->de_lock));

	/*
	Read the directory block containing the directory
	entry we are to make free.
	*/

	error = markdeleted (pvp, dp->de_dirclust, dp->de_diroffset);

	dp->de_refcnt--;

	deput (VTODE(vp));

	return error;
}

/*
 *  Be sure a directory is empty except for "." and "..".
 *  Return 1 if empty, return 0 if not empty or error.
 */
int
dosdirempty(struct denode *dep)
{
	int dei;
	int error;
	u_long cn;
	u_long bn;
	lbuf_t *bp;
	struct dosfs_vfs *pvp = dep->de_vfs;
	struct direntry *dentp;

#ifdef DOSFS_DEBUG
	printf ("In dosdirempty\n");
#endif
	/*
	 *  Since the filesize field in directory entries for a directory
	 *  is zero, we just have to feel our way through the directory
	 *  until we hit end of file.
	 */
	for (cn = 0;; cn++) {
		error = pcbmap(dep, cn, &bn, 0);
		if (error == E2BIG)
			return 1;	/* it's empty */
		bp = lbread(pvp->vfs_dev, bn, pvp->vfs_bpcluster);

		if (error = lgeterror(bp))
		{
			lbrelse(bp);
			return error;
		}
		dentp = (struct direntry *)bp->b_un.b_addr;
		for (dei = 0; dei < pvp->vfs_depclust; dei++) {
			if (dentp->deName[0] != SLOT_DELETED) {
				/*
				 *  In dos directories an entry whose name starts with SLOT_EMPTY (0)
				 *  starts the beginning of the unused part of the directory, so we
				 *  can just return that it is empty.
				 */
				if (dentp->deName[0] == SLOT_EMPTY) {
					lbrelse(bp);
					return 1;
				}
				/*
				 *  Any names other than "." and ".." in a directory mean
				 *  it is not empty.
				 */
				if (bcmp((char *)dentp->deName, ".          ", 11)  &&
				    bcmp((char *)dentp->deName, "..         ", 11)) {
					lbrelse(bp);
#ifdef DOSFS_DEBUG
					printf("dosdirempty(): entry %d found %02x, %02x\n", dei, dentp->deName[0],
					    dentp->deName[1]);
#endif
					return 0;	/* not empty */
				}
			}
			dentp++;
		}
		lbrelse(bp);
	}
	/*NOTREACHED*/
}

/*
 *  Check to see if the directory described by target is
 *  in some subdirectory of source.  This prevents something
 *  like the following from succeeding and leaving a bunch
 *  or files and directories orphaned.
 *	mv /a/b/c /a/b/c/d/e/f
 *  Where c and f are directories.
 *  source - the inode for /a/b/c
 *  target - the inode for /a/b/c/d/e/f
 *  Returns 0 if target is NOT a subdirectory of source.
 *  Otherwise returns a non-zero error number.
 *  The target inode is expected to be locked on entrance;
 *  it is (re-)locked at return.
 *  This routine never crosses devices (mountpoints).
 */
int
doscheckpath (struct denode *source, struct denode *target)
{
	unsigned long	scn;
	struct dosfs_vfs	*pvp;
	dosdirent_t		direntry;
	struct denode	*newdep;
	struct denode	*dep;
	lbuf_t		*bp;
	int				error;

	ASSERT (SLEEP_LOCKOWNED(&target->de_lock));

	if (!ISADIR(target) || !ISADIR(source))
	{
		return ENOTDIR;
	}

	if (target->de_StartCluster == source->de_StartCluster)
	{
		return EEXIST;
	}

	if (target->de_StartCluster == DOSFSROOT)
	{
		return 0;
	}

	dep = target;
	pvp = dep->de_vfs;

	for (;;)
	{
		scn = dep->de_StartCluster;
		bp  = lbread(pvp->vfs_dev, cntobn(pvp, scn), pvp->vfs_bpcluster);

		if (error = lgeterror(bp))
		{
			lbrelse(bp);
			break;
		}

		/*
		Go up by using the .. entry. Copy that
		entry and release the buffer. We keep
		the directory locked till we don't need
		the buffer info anymore.
		*/

		direntry = *((dosdirent_t *) paddr(bp) + 1);
		lbrelse(bp);

		if ((direntry.deAttributes & ATTR_DIRECTORY) == 0  ||
		    bcmp((char *)direntry.deName, "..         ", 11) != 0)
		{
			error = ENOTDIR;
			break;
		}

		if (direntry.deStartCluster == source->de_StartCluster)
		{
			error = EINVAL;
			break;
		}

		if (direntry.deStartCluster == DOSFSROOT)
		{
			break;
		}

		/*
				First deget the .. entry then deput the current
				one to keep locked.
				NOTE: deget() clears dep on error.
				*/

		error = deget((DETOV(target))->v_vfsp, ATTR_DIRECTORY, scn, 1,
			   (u_long)direntry.deStartCluster, &direntry, &newdep);
		/*
		Never deput the target, others could
		reuse the denode. We just unlock it
		and relock it at the end.
		*/

		if (dep == target)
		{
			DE_UNLOCK (dep);
		}
		else
		{
			deput (dep);
		}

		if (error)
		{
			break;
		}

		dep = newdep;

		if (!ISADIR(dep))
		{
			error = ENOTDIR;
			break;
		}

	}

	/*
	Now we most check whether we need to
	relock the target - the target should
	be locked at the end. If the current
	dep is the target, we are done else
	we need to relock it and check whether
	it still exist.
	*/

	if (dep != target)
	{
		if (dep != NULL)
		{
			deput(dep);
		}

		DE_LOCK (target);

		if (error == 0 && target->de_Name[0] == SLOT_DELETED)
			error = ENOENT;
	}

	return error;
}

/*
 *  Read in the disk block containing the directory entry
 *  dep came from and return the address of the buf header,
 *  and the address of the directory entry within the block.
 *
 * Locking : dep locked on entry, remains locked at exit.
 */
int
readde(struct denode *dep, lbuf_t **bpp, struct direntry **epp)
{
	int error;
	u_long bn;
	unsigned long theoff;
	struct dosfs_vfs *pvp = dep->de_vfs;

	if (dep->de_dirclust == DOSFSROOT) {
		bn = pvp->vfs_rootdirblk + (dep->de_diroffset /
			(pvp->vfs_BytesPerSec/sizeof (struct direntry)));
		theoff = dep->de_diroffset %
			(pvp->vfs_BytesPerSec/sizeof (struct direntry));
	} else {
		bn = cntobn(pvp, dep->de_dirclust);
		theoff = dep->de_diroffset;
	}

	*bpp = lbread(pvp->vfs_dev, bn, pvp->vfs_bpcluster);

	if (error = lgeterror(*bpp))
	{
		lbrelse(*bpp);
		*bpp = NULL;
		return error;
	}
	if (epp)
		*epp = (struct direntry *)((*bpp)->b_un.b_addr) + theoff;
	return 0;
}


/*
 * get_direntp: Like readde, other arguments, slightly different
 *				results.
 */

int
get_direntp (struct dosfs_vfs *pvp, u_long dirclust, u_long diroffset, dosdirent_t *dep)
{
	int		error;
	u_long	bn;
	lbuf_t*	bp;
	u_long	theoff;

	if (dirclust == DOSFSROOT)
	{
		bn = pvp->vfs_rootdirblk + (diroffset /
			(pvp->vfs_BytesPerSec/sizeof (struct direntry)));
		theoff = diroffset %
			(pvp->vfs_BytesPerSec/sizeof (struct direntry));
	}
	else
	{
		bn     = cntobn(pvp, dirclust);
		theoff = diroffset;
	}

	bp = lbread (pvp->vfs_dev, bn, pvp->vfs_bpcluster);

	error = lgeterror(bp);

	if (!error)
	{
		*dep = *((dosdirent_t*) bp->b_un.b_addr + theoff);
	}

	lbrelse(bp);
	return error;
}

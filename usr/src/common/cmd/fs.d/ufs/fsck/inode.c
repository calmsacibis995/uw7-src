/*	copyright	"%c%"	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */
#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/inode.c	1.3.9.13"

/*  "errexit()" and "pfatal()" have been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "pwarn()", "dofix()" and "clri()" have been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
 */

#include <stdio.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/mntent.h>
#include <sys/fs/sfs_fs.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#define _KERNEL
#include <sys/fs/sfs_fsdir.h>
#undef _KERNEL
#include <sys/mnttab.h>
#include "fsck.h"
#include <pfmt.h>

extern 	int pass1check();
extern 	int pass1bcheck();
void	setstate();
char	get_state();

/*
 * Check and record all direct and indirect data blocks of an inode
 */
ckinode(dp, idescp, addrcheck, allocino, discino, entrychk)
	DINODE *dp;
	register struct inodesc *idescp;
	int	addrcheck, allocino, discino, entrychk;
{
	register daddr_t *ap;
	int ret = 0, n, ndb, offset, j=0;
	DINODE dino;
	struct	dirmap *dirp;

	idescp->id_fix = DONTKNOW;
	idescp->id_filesize = dp->di_size;
	idescp->id_loc = 0;
	idescp->id_entryno = 0;
	if (SPECIAL(dp))
		return (KEEPON);
	if (MEM && DIRCT(dp) && (idescp->id_func != pass1bcheck)) {
		dirp = nstatemap(idescp->id_number).dir_p;
		dirp->filesize = dp->di_size;
	}
	/* 
	 * if an inode had type DATA and don't want to check address
         * ignore address check.
         */
	if (MEM && (idescp->id_type == DATA) && (addrcheck == 0)) {
		return(inmem_readdir(dirp, 0, idescp, discino, 0, 0, 0));
	}

	dino = *dp;
	ndb = howmany(dino.di_size, sblock.fs_bsize);
	for (ap = &dino.di_db[0]; ap < &dino.di_db[NDADDR]; ap++, j++) {
		if (--ndb == 0 && (offset = blkoff(&sblock, dino.di_size)) != 0)
			idescp->id_numfrags =
				numfrags(&sblock, fragroundup(&sblock, offset));
		else
			idescp->id_numfrags = sblock.fs_frag;
		if (*ap <= 0) {
			if ((ap == &dino.di_db[0]) && DIRCT(dp) &&
				idescp->id_filesize != 0) {
				setstate(idescp->id_number,DCLEAR);
			}
			continue;
		}
		/* if an inode is just allocated, recorded it's data block
		 * number in dblist.
		 */
		idescp->id_blkno = *ap;
		if (MEM && DIRCT(dp) && idescp->id_func != pass1bcheck){
			*(dirp->dblist + j) = *ap;
		}
		if (idescp->id_type == ADDR) {
			ret = (*idescp->id_func)(idescp);
		} else {
			if (!MEM)
				ret = dirscan(idescp);
		}
		if (ret & STOP)
			return (ret);
	}
	idescp->id_numfrags = sblock.fs_frag;
	for (ap = &dino.di_ib[0], n = 1; n <= NIADDR; ap++, n++, j++) {
		/* No backing storage for indirect blocks. remove it */
		if (*ap > 0) { 
			unsigned long blks = NDADDR;	/* direct blocks */
			if (n > 1) {		/* + indirect blocks */
			    blks += NINDIR(&sblock);
			}
			if (n > 2) {		/* + double indirect blocks */
			    blks += NINDIR(&sblock) * NINDIR(&sblock);
			}

			idescp->id_blkno = *ap;
			ret = iblock(idescp, n,
				dino.di_size - sblock.fs_bsize * blks, dirp, j);
			if (ret & STOP)
				return (ret);
			if ((ret & ALTERED) && DIRCT(dp)) {
				setstate(idescp->id_number, DCLEAR);
				break;
			}
		}
	}
	if (!MEM || !entrychk)
		return(KEEPON);
	if (DIRCT(dp) && (idescp->id_type == DATA)) {
		ret = inmem_readdir(dirp, 0, idescp, discino, 0 ,0, 0);
		if (ret & STOP)
			return (ret);
	}
	return (KEEPON);
}

/*
 * Check an indirect level data block 
 */ 
iblock(idesc, ilevel, isize, dirp, dbcnt)
	struct inodesc *idesc;
	register ilevel;
	long isize;
	struct	dirmap	*dirp;
	int dbcnt;
{
	register daddr_t *ap;
	register daddr_t *aplim;
	int i, n, (*func)(), nif, sizepb;
	BUFAREA ib;
	extern int pass1check();

	func = idesc->id_func;
	if (idesc->id_type == ADDR) {
		if (((n = (*func)(idesc)) & KEEPON) == 0)
			return (n);
	}
	if (outrange(idesc->id_blkno, idesc->id_numfrags)) /* protect thyself */
		return (SKIP);
	initbarea(&ib);
	getblk(&ib, idesc->id_blkno, sblock.fs_bsize);

	if (ib.b_errs != NULL)
		return (SKIP);
	ilevel--;
	for (sizepb = sblock.fs_bsize, i = 0; i < ilevel; i++)
		sizepb *= NINDIR(&sblock);

	/* 
	 * isize < 0 (PARTIALLY TRUNCATED INODE): there are no valid blocks.
	 * else don't add 1 if isize is a multiple of sizepb.
	 */
	if (isize < 0) 
		nif = 0;
	else {
		if (sizepb == 0 && ilevel == 2)
			return(ALTERED);
		nif = isize / sizepb;
		if (isize % sizepb) nif++;
	}

	if (nif > NINDIR(&sblock))
		nif = NINDIR(&sblock);
	if (idesc->id_func == pass1check && nif < NINDIR(&sblock)) {
		aplim = &ib.b_un.b_indir[NINDIR(&sblock)];
		for (ap = &ib.b_un.b_indir[nif]; ap < aplim; ap++) {
			if (*ap == 0)
				continue;
			if (dofix(idesc, ":200:PARTIALLY TRUNCATED INODE I=%d",
				idesc->id_number)) {
				*ap = 0;
				dirty(&ib);
			}
		}
		/* flush out indirect block that are dirty */
		flush(&dfile, &ib);
	}
	aplim = &ib.b_un.b_indir[nif];
	/*
	 * i has to start from 0 since the correct value for isize is passed
	 * to iblock();
	 */
	for (ap = ib.b_un.b_indir, i = 0; ap < aplim; ap++, i++)
		if (*ap > 0) {
			idesc->id_blkno = *ap;
			if (MEM && (idesc->id_func != pass1bcheck) &&
				 nstatemap(idesc->id_number).flag == DSTATE) {
				*(dirp->dblist + dbcnt) = *ap;
				++dbcnt;
			}
			if (ilevel > 0) {
				n = iblock(idesc, ilevel, isize - i * sizepb, dirp, dbcnt);
			} else
				n = (*func)(idesc);
			if (n & STOP)
				return (n);
		}
	return (KEEPON);
}

/*
 * Check if a block or a fragment is in the valid range.
 */
outrange(blk, cnt)
	daddr_t blk;
	int cnt;
{
	register int c;

	if ((unsigned)(blk+cnt) > fmax)
		return (1);
	c = dtog(&sblock, blk);
	if (blk < cgdmin(&sblock, c)) {
		if ((blk+cnt) > cgsblock(&sblock, c)) {
			if (debug) {
				Pprintf(":341:blk %d < cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				Pprintf(":342: blk+cnt %d > cgsbase %d\n",
				    blk+cnt, cgsblock(&sblock, c));
			}
			return (1);
		}
	} else {
		if ((blk+cnt) > cgbase(&sblock, c+1)) {
			if (debug)  {
				Pprintf(":343:blk %d >= cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				Pprintf(":344: blk+cnt %d > sblock.fs_fpg %d\n",
				    blk+cnt, sblock.fs_fpg);
			}
			return (1);
		}
	}
	return (0);
}

/*
 * Read 64k inodes from disk.
 */
DINODE *
ginode(inumber)
	ino_t inumber;
{
	daddr_t iblk;
	register DINODE *dp;
	int iperb = inobuf.bb_iperb;
	static	ino_t startino = 0;

	if (inumber > imax || inumber < SFSROOTINO)
		errexit(":201:Bad inode number %d for ginode\n", inumber);
	if (MEM) {		/* get 64k inode block */
		if (inobuf.bb_strino == -1) {
			inobuf.bb_strino = 0;
			iblk = itod(&sblock, (int)inumber);
			getbigblk(iblk);
			if (inumber == SFSROOTINO) 
				startino = inobuf.bb_strino = 0;
			else
				startino = inobuf.bb_strino = inumber;
		} else if (inumber < startino || 
			(inumber >= startino + (ino_t)iperb)) {
			startino = inobuf.bb_strino = inumber;
			iblk = itod(&sblock, (int)inumber);
			getbigblk(iblk);
	    	}
		dp = (DINODE *)(inobuf.bb_buf + (int)(inumber - startino));

	} else 		/* get file system block of inode block */
		return(sginode(inumber));

	if (dp ->di_eftflag != EFT_MAGIC) {
		dp->di_mode = dp->di_smode;
		dp->di_uid = dp->di_suid;
		dp->di_gid = dp->di_sgid;
	}
	return(dp);
}

/*
 * Read a block size of inodes from disk.
 */
DINODE *
sginode(inumber)
	ino_t inumber;
{
	daddr_t iblk;
	register DINODE *dp;
	static	ino_t startinum = 0;
	
	if (inumber > imax || inumber < SFSROOTINO)
		errexit(":201:Bad inode number %d for ginode\n", inumber);

	if (startinum == 0 || inumber < startinum ||
		 inumber >= (ino_t)(startinum + (ino_t)INOPB(&sblock))) {
		iblk = itod(&sblock, (int)inumber);
		getblk(&inoblk, iblk, sblock.fs_bsize);
		startinum = (ino_t)(((int)inumber / INOPB(&sblock)) * INOPB(&sblock));
	}
        dp =&inoblk.b_un.b_dinode[(int)inumber % INOPB(&sblock)];

	if (dp ->di_eftflag != EFT_MAGIC) {
		dp->di_mode = dp->di_smode;
		dp->di_uid = dp->di_suid;
		dp->di_gid = dp->di_sgid;
	}

	return(dp);
}

/* 
 * Clear inode
 */
clri(idesc, s, flg)
	register struct inodesc *idesc;
	char *s;
	int flg;
{
	register DINODE *dp;
	int nosem=0;
	
	WYPFLG(wflag, yflag, preen);
	dp = sginode(idesc->id_number);
	getsem();
	if ((flg == 1) && !bflg) {
		pwarn(s);
		myfprintf(stderr, MM_NOSTD|MM_NOGET, " %s", DIRCT(dp) ?
			gettxt(":88","DIR") :
			gettxt(":89","FILE"));
		pinode(dp,idesc->id_number);
	}
	if (preen || reply(1, gettxt(":32","CLEAR")) == 1) {
		if (preen && !bflg)
			Pprintf(":202: (CLEARED)\n");
		n_files--;
		relsem();
		nosem++;
		(void)ckinode(dp, idesc, 1, 0, 0, 0);
		zapino(dp);
		setstate(idesc->id_number, USTATE);
		inodirty();
	}
	if (Pflag && !nosem)
		relsem();
}

findname(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (dirp->d_ino != idesc->id_parent)
		return (KEEPON);
	memcpy(idesc->id_name, dirp->d_name, dirp->d_namlen + 1);
	return (STOP);
}

findino(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (dirp->d_ino == 0)
		return (KEEPON);
	if (strcmp(dirp->d_name, idesc->id_name) == 0 &&
	    dirp->d_ino >= SFSROOTINO && dirp->d_ino <= imax) {
		idesc->id_parent = dirp->d_ino;
		return (STOP);
	}
	return (KEEPON);
}

pinode(dp, ino)
	DINODE *dp;
	ino_t	ino;
{
	char p1[128], p2[128];
	struct passwd *pw;

	cftime(p1, (char *)gettxt(":204","%b %d %H:%M"), &dp->di_mtime);
	cftime(p2, (char *)gettxt(":205","%Y"), &dp->di_mtime);
	if ((pw = getpwuid((int)dp->di_uid)) != 0) {
		Sprintf(":345: I=%u OWNER= %s MODE=%o\n", ino, pw->pw_name,dp->di_mode);
		if (preen)
			Sprintf(":346:%s: ", devname);
		Pprintf(":347:SIZE=%ld MTIME=%s %s\n", dp->di_size, p1, p2);
	} else {
		Sprintf(":348: I=%u OWNER= %d MODE=%o\n", ino, dp->di_uid,dp->di_mode);
		if (preen)
			Sprintf(":346:%s: ", devname);
		Pprintf(":347:SIZE=%ld MTIME=%s %s\n", dp->di_size, p1, p2);
	}
}

blkerr(ino, s, blk)
	ino_t ino;
	char *s;
	daddr_t blk;
{
	char state;

	state = get_state(ino);
	mypfatal(":349:%ld %s I=%u\n", blk, s, ino);
	switch (state) {

	case FSTATE:
		setstate(ino, FCLEAR);
		return;

	case DSTATE:
		setstate(ino, DCLEAR);
		return;

	case FCLEAR:
	case DCLEAR:
	case DCORRPT:
		return;

	default:
		errexit(":207:BAD STATE %d TO BLKERR\n", state);
		/* NOTREACHED */
	}
}

/*
 * allocate an unused inode
 */
ino_t
allocino(request, type)
	ino_t request;
	int type;
{
	register ino_t ino;
	DINODE	*dp;
	char state;

	state = get_state(request);
	if (request == 0)
		request = SFSROOTINO;
	else if (state != USTATE)
		return (0);
	for (ino = request; ino < imax; ino++) {
		state = get_state(ino);
		if (state == USTATE)
			break;
	}
	if (ino == imax)
		return (0);
	switch (type & IFMT) {
	case IFDIR:
		setstate(ino, DSTATE);
		break;
	case IFREG:
	case IFLNK:
		setstate(ino, FSTATE);
		break;
	default:
		return (0);
	}
	dp = sginode(ino);
	dp->di_db[0] = allocblk(1);
	if (dp->di_db[0] == 0) {
		setstate(ino, USTATE);
		return (0);
	}
	/* create dirmap for a new directory and record a new block */
	dp->di_size = sblock.fs_fsize;
	if (MEM && (type & IFMT) == IFDIR) {
        	creatdirmap(ino, dp->di_size, 1);
		*(nstatemap(ino).dir_p->dblist) = dp->di_db[0];
	}
	dp->di_smode = dp->di_mode = type;
	dp->di_eftflag = EFT_MAGIC;                                 
	time(&dp->di_atime);
	dp->di_mtime = dp->di_ctime = dp->di_atime;
	dp->di_blocks = btodb(sblock.fs_fsize);
	n_files++;
	inodirty();
	return (ino);
}

/*
 * deallocate an inode
 */
freeino(ino)
	ino_t ino;
{
	struct inodesc idesc;
	extern int pass4check();
	DINODE *dp;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	idesc.id_number = ino;
	dp = sginode(ino);
	(void)ckinode(dp, &idesc, 1, 0, 0, 0);
	zapino(dp);
	inodirty();
	setstate(ino, USTATE);
	n_files--;
}
/*
 * set state of an inode to the appropriate state.
 */
void
setstate(ino, state)
	ino_t	ino;
	char state;
{
	if (MEM)
		nstatemap(ino).flag = state;
	else
		ostatemap(ino) = state;
}

/*
 * Get a state of an inode from the statemap table.
 */
char
get_state(ino)
	ino_t	ino;
{
	char state;

	if (MEM)
		return(nstatemap(ino).flag);
	else
		return(ostatemap(ino));
}

/*	copyright	"%c%"	*/

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
#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/dir.c	1.3.6.18"

/*  "errexit()", "pfatal()", "pwarn()", "dofix()", "direrr()", "clri()",
 *  "Pprintf()", "Sprintf()", "mypfatal()" and "myprintf()" have been
 *  internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *
 *  "errexit()" and "pfatal()" output using <MM_ERROR>.
 *  "pwarn()", "dofix()", "direrr()" and "clri()" output using <MM_WARNING>.
 */

#include <stdio.h>
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

#define MINDIRSIZE	(sizeof (struct dirtemplate))

char	*endpathname = &pathname[PATHBUFSIZ - 2];
char	*lfname = "lost+found";
struct	dirtemplate emptydir = { 0, DIRBLKSIZ};
struct	dirtemplate dirhead = { 0, 12, 1, ".", 0, DIRBLKSIZ - 12, 2, ".." };
extern int	pass2sechk();
int	fixentry();
DIRECT *fsck_readdir();

/*
 * Procedure:     chkdirsiz
 *
 * Restrictions:
 *                printf: none
*/
chkdirsiz_descend(parentino, inumber)
	struct inodesc *parentino;
	ino_t inumber;
{
	register DINODE *dp;
	ulong	size;
	char	state;

	state = get_state(inumber);
	if (state == DCORRPT)
		return;
	if (state != DSTATE && state != DFOUND)
		if (MEM)
			errexit(":333:BAD STATE %d FOR INODE DIRECTORY %d\n", state,inumber);
		else
			errexit(":373:BAD STATE %d TO DESCEND %d\n", state,inumber);
	if (MEM)
		size = nstatemap(inumber).dir_p->filesize;
	else {
		setstate(inumber, DFOUND);
		dp = sginode(inumber);
		size = dp->di_size;
	}
	if (size == 0) {
		WYPFLG(wflg, yflag, preen);
		getsem();
		direrr(inumber, ":175:ZERO LENGTH DIRECTORY");
		if (reply(1, gettxt(":86","REMOVE")) == 1)
			setstate(inumber, DCLEAR);
		relsem();
		return (STOP);
	}
	if (size < MINDIRSIZE) {
		WYPFLG(wflg, yflag, preen);
		dp = sginode(inumber);
		dp->di_size = MINDIRSIZE;
		getsem();
		direrr(inumber, ":176:DIRECTORY TOO SHORT");
		if (reply(1, gettxt(":42","FIX")) == 1)
			inodirty();
		relsem();
	}
	if ((size & (DIRBLKSIZ - 1)) != 0) {
		WYPFLG(wflg, yflag, preen);
		dp = sginode(inumber);
		dp->di_size = roundup(dp->di_size, DIRBLKSIZ);
		getsem();
		pwarn(":177:DIRECTORY %d: LENGTH %d NOT MULTIPLE OF %d\n",
			inumber, size, DIRBLKSIZ);
		if (preen)
			Pprintf(":178: (ADJUSTED)\n");
		if (preen || reply(1, gettxt(":96","ADJUST")) == 1)
			inodirty();
		relsem();
	}

	if (!MEM) {
		struct inodesc curino;

		memset((char *)&curino, 0, sizeof(struct inodesc));
		curino.id_type = DATA;
		curino.id_func = parentino->id_func;
		curino.id_parent = parentino->id_number;
		curino.id_number = inumber;
		(void)ckinode(dp, &curino, 0, 0, 0, 0);
	}
	return (0);
}

/*
 * Procedure:     dirscan
 *
 * Restrictions:  none
*/

dirscan(idesc)
	register struct inodesc *idesc;
{
	register DIRECT *dp;
	int dsize, n;
	long blksiz;
	char dbuf[DIRBLKSIZ];

	if (idesc->id_type != DATA)
		errexit(":375:wrong type to dirscan %d\n", idesc->id_type);
	if (idesc->id_entryno == 0 &&
	    (idesc->id_filesize & (DIRBLKSIZ - 1)) != 0)
		idesc->id_filesize = roundup(idesc->id_filesize, DIRBLKSIZ);
	blksiz = idesc->id_numfrags * sblock.fs_fsize;
	if (outrange(idesc->id_blkno, idesc->id_numfrags)) {
		idesc->id_filesize -= blksiz;
		return (SKIP);
	}
	idesc->id_loc = 0;
	for (dp = fsck_readdir(idesc); dp != NULL; dp = fsck_readdir(idesc)) {
		dsize = dp->d_reclen;
		memcpy(dbuf, (char *)dp, dsize);
		idesc->id_dirp = (DIRECT *)dbuf;
		if ((n = (*idesc->id_func)(idesc)) & ALTERED) {
			getblk(&fileblk, idesc->id_blkno, blksiz);
			if (fileblk.b_errs != NULL) {
				n &= ~ALTERED;
			} else {
				memcpy((char *)dp, dbuf, dsize);
				dirty(&fileblk);
				sbdirty();
			}
		}
		if (n & STOP) 
			return (n);
	}
	return (idesc->id_filesize > 0 ? KEEPON : STOP);
}


/*
 * Procedure:     fsck_readdir
 *
 * Restrictions:  none
 *
 * Notes:         get next entry in a directory.
 */
DIRECT *
fsck_readdir(idesc)
	register struct inodesc *idesc;
{
	register DIRECT *dp, *ndp;
	long size, blksiz;

	blksiz = idesc->id_numfrags * sblock.fs_fsize;
	getblk(&fileblk, idesc->id_blkno, blksiz);
	if (fileblk.b_errs != NULL) {
		idesc->id_filesize -= blksiz - idesc->id_loc;
		return NULL;
	}
	if (idesc->id_loc % DIRBLKSIZ == 0 && idesc->id_filesize > 0 &&
	    idesc->id_loc < blksiz) {
		dp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
		if (dircheck(idesc, dp))
			goto dpok;
		idesc->id_loc += DIRBLKSIZ;
		idesc->id_filesize -= DIRBLKSIZ;
		dp->d_reclen = DIRBLKSIZ;
		dp->d_ino = 0;
		dp->d_namlen = 0;
		dp->d_name[0] = '\0';
		if (dofix(idesc, ":180:DIRECTORY CORRUPTED"))
			dirty(&fileblk);
		return (dp);
	}
dpok:
	if (idesc->id_filesize <= 0 || idesc->id_loc >= blksiz)
		return NULL;
	dp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
	idesc->id_loc += dp->d_reclen;
	idesc->id_filesize -= dp->d_reclen;
	if ((idesc->id_loc % DIRBLKSIZ) == 0)
		return (dp);
	ndp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
	if (idesc->id_loc < blksiz && idesc->id_filesize > 0 &&
	    dircheck(idesc, ndp) == 0) {
		size = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
		dp->d_reclen += size;
		idesc->id_loc += size;
		idesc->id_filesize -= size;
		if (dofix(idesc, ":180:DIRECTORY CORRUPTED"))
			dirty(&fileblk);
	}
	return (dp);
}
/*
 * Procedure:     dircheck
 *
 * Restrictions:  none
 *
 * Notes:         Verify that a directory entry is valid.
 *                This is a superset of the checks made in the kernel.
 */
dircheck(idesc, dp)
	struct inodesc *idesc;
	register DIRECT *dp;
{
	register int size;
	register char *cp;
	int spaceleft;

	size = DIRSIZ(dp);
	spaceleft = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
	if (dp->d_ino < imax &&
	    dp->d_reclen != 0 &&
	    (int)dp->d_reclen <= spaceleft &&
	    (dp->d_reclen & 0x3) == 0 &&
	    (int)dp->d_reclen >= size &&
	    idesc->id_filesize >= size &&
	    dp->d_namlen <= SFS_MAXNAMLEN) {
		if (dp->d_ino < 2)
			return (1);
		for (cp = dp->d_name, size = 0; size < (int)dp->d_namlen; size++)
			if (*cp++ == 0)
				return (0);
		if (*cp == 0)
			return (1);
	}
	return (0);
}


/*
 * Procedure:     direrr
 *
 * Restrictions:
 *                printf: none
*/

direrr(ino, s, a1)
	ino_t ino;
	char *s;
{
	register DINODE *dp;

	pwarn(s, a1);
	dp = sginode(ino);
	pinode(dp, ino);
}


/*
 * Procedure:     adjust
 *
 * Restrictions:
 *               printf: none
*/

adjust(idesc, lcnt)
	register struct inodesc *idesc;
	short lcnt;
{
	register DINODE *dp;

	dp = sginode(idesc->id_number);
	if (dp->di_nlink == lcnt) {
		if (linkup(idesc->id_number, (ino_t)0) == 0)
			clri(idesc, ":46:UNREF", 0);
	} else {
		getsem();
		pwarn(":182:LINK COUNT %s",
		    (lfdir == idesc->id_number) ? lfname : (DIRCT(dp) ?
		    (char *)gettxt(":88","DIR") : (char *)gettxt(":89","FILE")));
		pinode(dp, idesc->id_number);
		Pprintf(":183:COUNT %d SHOULD BE %d\n",
			dp->di_nlink, dp->di_nlink-lcnt);
		if (preen) {
			if (lcnt < 0) {
				Pprintf("\n");
				Pprintf(":184:LINK COUNT INCREASING\n");
				Pprintf(":334:UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY. \n");
				relsem();
				exit(36);
			} else
				Pprintf(":178: (ADJUSTED)\n");
		}
		if (preen || reply(1, gettxt(":96","ADJUST")) == 1) {
			if (dp->di_nlink - lcnt < 0) {
				relsem();
				return;
			}
			dp->di_nlink -= lcnt;
			inodirty();
		}
		relsem();
	}
}


/*
 * Procedure:     mkentry
 *
 * Restrictions:  none
*/

mkentry(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;
	DIRECT newent;
	int newlen, oldlen;

	newent.d_namlen = 11;
	newlen = DIRSIZ(&newent);
	if (dirp->d_ino != 0)
		oldlen = DIRSIZ(dirp);
	else
		oldlen = 0;
	if ((int)dirp->d_reclen - oldlen < newlen)
		return (KEEPON);
	newent.d_reclen = dirp->d_reclen - oldlen;
	dirp->d_reclen = oldlen;
	dirp = (struct direct *)(((char *)dirp) + oldlen);
	dirp->d_ino = idesc->id_parent;	/* ino to be entered is in id_parent */
	dirp->d_reclen = newent.d_reclen;
	dirp->d_namlen = strlen(idesc->id_name);
	memcpy(dirp->d_name, idesc->id_name, dirp->d_namlen + 1);
	return (ALTERED|STOP);
}


/*
 * Procedure:     chgino
 *
 * Restrictions:  none
*/

chgino(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (memcmp(dirp->d_name, idesc->id_name, dirp->d_namlen + 1))
		return (KEEPON);
	dirp->d_ino = idesc->id_parent;
	if (MEM)
		nstatemap(idesc->id_number).dir_p->dotdot = idesc->id_parent;
	return (ALTERED|STOP);
}


/*
 * Procedure:     linkup
 *
 * Restrictions:
 *                printf: none
*/

linkup(orphan, pdir)
	ino_t orphan;
	ino_t pdir;
{
	register DINODE *dp;
	int lostdir, len;
	ino_t oldlfdir;
	struct inodesc idesc;
	char tempname[BUFSIZ];
	extern int pass4check();
	extern int pass4sechk();
	char state;

	WYPFLG(wflg, yflag, preen);
	memset((char *)&idesc, 0, sizeof(struct inodesc));
	dp = sginode(orphan);
	lostdir = DIRCT(dp);
	getsem();
	pwarn(":185:UNREF %s ", 
		lostdir ? gettxt(":88","DIR") : gettxt(":89","FILE"));
	pinode(dp, orphan);
	if (preen && dp->di_size == 0) {
		Sprintf ("\n");
		relsem();
		return (0);
	}
	if (preen)
		Pprintf(":186: (RECONNECTED)\n");
	else if (reply(1, gettxt(":135","RECONNECT")) == 0) {
			relsem();
			return (0);
	}
	pathp = pathname;
	*pathp++ = '/';
	*pathp = '\0';

	/*  No lost+found. Create one */
	if (!MEM) {
 		dp = sginode(SFSROOTINO);
 		idesc.id_name = lfname;
 		idesc.id_type = DATA;
 		idesc.id_func = findino;
 		idesc.id_number = SFSROOTINO;
 		(void)ckinode(dp, &idesc, 0, 0, 0, 0);
 		if (idesc.id_parent >= SFSROOTINO && idesc.id_parent < imax)
 			lfdir = idesc.id_parent;
	}
	if (lfdir == 0) {
		pwarn(":187:NO lost+found DIRECTORY\n");
		if (preen || reply(1, gettxt(":188","CREATE"))) {
			relsem();
			lfdir = allocdir(SFSROOTINO, 0);
			if (lfdir != 0) {
				if (makeentry(SFSROOTINO, lfdir, lfname) != 0) {
					if (preen && !bflg)
						myprintf(":189: (CREATED)\n");
					if (MEM)
						nstatemap(lfdir).dir_p->dotdot = SFSROOTINO;
				} else {
					freedir(lfdir, SFSROOTINO);
					lfdir = 0;
					if (preen)
						Sprintf("\n");
				}
			}
		}
		getsem();
	}
	relsem();
	if (lfdir == 0) {
		mypfatal(":190:SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
		return (0);
	} else {	/* lost+found already created */
		idesc.id_name = lfname;
	}
	dp = sginode(lfdir);
	if (!DIRCT(dp)) {
		getsem();
		pfatal(":191:lost+found IS NOT A DIRECTORY\n");
		if (reply(1, gettxt(":192","REALLOCATE")) == 0) {
			relsem();
			return (0);
		}
		relsem();
		oldlfdir = lfdir;
		if ((lfdir = allocdir(SFSROOTINO, 0)) == 0) {
			mypfatal(":190:SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			return (0);
		}
		idesc.id_type = DATA;
		idesc.id_func = chgino;
		idesc.id_secfunc = 0;   /* no security function */
		idesc.id_number = SFSROOTINO;
		idesc.id_parent = lfdir;	/* new inumber for lost+found */
		idesc.id_name = lfname;
		if ((ckinode(sginode(SFSROOTINO), &idesc, 0, 0, 0, 1) & ALTERED) == 0) {
			mypfatal(":190:SORRY. CANNOT CREATE lost+found DIRECTORY\n\n");
			return (0);
		}
		inodirty();
		idesc.id_type = ADDR;
		idesc.id_func = pass4check;
		idesc.id_secfunc = pass4sechk;
		idesc.id_number = oldlfdir;
		adjust(&idesc, 1);
		lncntp[oldlfdir] = 0;
		dp = sginode(lfdir);
		if (MEM) {
			nstatemap(lfdir).dir_p->dotdot = SFSROOTINO;
			nstatemap(lfdir).dir_p->dot = lfdir;
		}
	}

	state = get_state(lfdir);
	if (state != DFOUND) {
		mypfatal(":193:SORRY. NO lost+found DIRECTORY\n\n");
		return (0);
	}
	len = strlen(lfname);
	memcpy(pathp, lfname, len + 1);
	pathp += len;
	len = lftempname(tempname, orphan);
	if (makeentry(lfdir, orphan, tempname) == 0) {
		mypfatal(":194:SORRY. NO SPACE IN lost+found DIRECTORY\n\n");
		return (0);
	}
	if (!MEM)
		lncntp[orphan]--;
	*pathp++ = '/';
	memcpy(pathp, idesc.id_name, len + 1);
	pathp += len;
	if (lostdir) {
		dp = sginode(orphan);
		idesc.id_type = DATA;
		idesc.id_func = chgino;
		idesc.id_secfunc = 0;   /* no security function */
		idesc.id_number = orphan;
		idesc.id_fix = DONTKNOW;
		idesc.id_name = "..";
		idesc.id_parent = lfdir;	/* new value for ".." */
		(void)ckinode(dp, &idesc, 1, 0, 0, 1);
		dp = sginode(lfdir);
		dp->di_nlink++;
		inodirty();
		if (!MEM)
			lncntp[lfdir]++;
		getsem();
		pwarn(":195:DIR I=%u CONNECTED. ", orphan);
		Pprintf(":196:PARENT WAS I=%u\n", pdir);
		if (preen == 0)
			Sprintf("\n");
		relsem();
	} else {
		dp = sginode(orphan);
		dp->di_nlink = 1;
		inodirty();
		lncntp[orphan] = 0;
	}	
	
	return (1);
}


/*
 * Procedure:     makeentry
 *
 * Restrictions:  none
 *
 * Notes:	make an entry in a directory
 */

makeentry(parent, ino, name)
	ino_t parent, ino;
	char *name;
{
	DINODE *dp;
	struct inodesc idesc;
	
	if (parent < SFSROOTINO || parent >= imax || ino < SFSROOTINO || ino >= imax)
		return (0);
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	idesc.id_func = mkentry;
	idesc.id_secfunc = 0;   /* no security function */
	idesc.id_number = parent;
	idesc.id_parent = ino;	/* this is the inode to enter */
	idesc.id_fix = DONTKNOW;
	idesc.id_name = name;
	dp = sginode(parent);
	if (dp->di_size % DIRBLKSIZ) {
		dp->di_size = roundup(dp->di_size, DIRBLKSIZ);
		inodirty();
	}
	if ((ckinode(dp, &idesc, 0, 0, 0, 1) & ALTERED) != 0)
		return (1);
	if (expanddir(dp, &idesc) == 0)
		return (0);
	return (ckinode(dp, &idesc, 0, 0, 0, 1) & ALTERED);
}


/*
 * Procedure:     expanddir
 *
 * Restrictions:
 *                printf: none
 *
 * Notes:         Attempt to expand the size of a directory
 */
expanddir(dp, idescp)
	register DINODE *dp;
	struct inodesc	*idescp;
{
	daddr_t lastbn, newblk;
	char *cp, firstblk[DIRBLKSIZ];

	/* lblkno(sblock, size) returns size / sblock->fs_bsize */
	lastbn = lblkno(&sblock, dp->di_size > 0 ? dp->di_size - 1 : 0);
	if (lastbn >= NDADDR - 1)
		return (0);
	if ((newblk = allocblk(sblock.fs_frag)) == 0)
		return (0);
	dp->di_db[lastbn + 1] = dp->di_db[lastbn];
	dp->di_db[lastbn] = newblk;
	dp->di_size += sblock.fs_bsize;
	dp->di_blocks += btodb(sblock.fs_bsize);
	getblk(&fileblk, dp->di_db[lastbn + 1],
	    dblksize(&sblock, dp, lastbn + 1));
	if (fileblk.b_errs != NULL)
		goto bad;
	memcpy(firstblk, dirblk.b_buf, DIRBLKSIZ);
	getblk(&fileblk, newblk, sblock.fs_bsize);
	if (fileblk.b_errs != NULL)
		goto bad;
	memcpy(dirblk.b_buf, firstblk, DIRBLKSIZ);
	for (cp = &dirblk.b_buf[DIRBLKSIZ];
	     cp < &dirblk.b_buf[sblock.fs_bsize];
	     cp += DIRBLKSIZ)
		memcpy(cp, (char *)&emptydir, sizeof emptydir);
	dirty(&fileblk);
	getblk(&fileblk, dp->di_db[lastbn + 1],
	    dblksize(&sblock, dp, lastbn + 1));
	if (fileblk.b_errs != NULL)
		goto bad;
	memcpy(dirblk.b_buf, (char *)&emptydir, sizeof emptydir);

	/* realloc dblist when expand a directory */
	if (MEM) {
		struct  dirmap  *dirp;

		dirp = nstatemap(idescp->id_number).dir_p;
		/* realloc (lastbn +2) since lastbn is indexing to db[] */
		dirp->dblist = (daddr_t *)realloc(dirp->dblist, sizeof (daddr_t ) * (lastbn + 2));
		if (dirp->dblist == NULL) {
			myprintf(stdout, MM_ERROR, ":297:Cannot alloc %d bytes for dblist\n",(lastbn +2));
			goto bad;
		}
		*(dirp->dblist +lastbn +1) = *(dirp->dblist + lastbn); 
		*(dirp->dblist +lastbn) = newblk;    /* record the new data block */
	}
	getsem();
	pwarn(":335:NO SPACE LEFT IN lost+found DIRECTORY\n");
	if (preen)
		Pprintf(":198: (EXPANDED)\n");
	else if (reply(1, gettxt(":199","EXPAND")) == 0)
		goto bad;
	relsem();
	dirty(&fileblk);
	inodirty();
	return (1);
bad:
	relsem();
	dp->di_db[lastbn] = dp->di_db[lastbn + 1];
	dp->di_db[lastbn + 1] = 0;
	dp->di_size -= sblock.fs_bsize;
	dp->di_blocks -= btodb(sblock.fs_bsize);
	freeblk(newblk, sblock.fs_frag);
	return (0);
}


/*
 * Procedure:     allocdir
 *
 * Restrictions:  none
 * Notes:         allocate a new directory
 *	- dot,
 *	- dotdot,
 *	- zero out (fs size - 512)  of the new directory
 *      - update the new directory state in statemap,
 *      - update link count of the new directory and parent.
 */

allocdir(parent, request)
	ino_t parent, request;
{
	ino_t ino;
	char *cp;
	DINODE *dp;
	char state;

	ino = allocino(request, IFDIR|0755);
	dirhead.dot_ino = ino;
	dirhead.dotdot_ino = parent;
	dp = sginode(ino);
	getblk(&fileblk, dp->di_db[0], sblock.fs_fsize);
	if (fileblk.b_errs != NULL) {
		freeino(ino);
		return (0);
	}
	memcpy(dirblk.b_buf, (char *)&dirhead, sizeof dirhead);
	for (cp = &dirblk.b_buf[DIRBLKSIZ];
	     cp < &dirblk.b_buf[sblock.fs_fsize];
	     cp += DIRBLKSIZ)
		memcpy(cp, (char *)&emptydir, sizeof emptydir);
	dirty(&fileblk);
	dp->di_nlink = 2;
	inodirty();
	if (ino == SFSROOTINO) {
		lncntp[ino] = 2;
		return(ino);
	}
	state = get_state(parent);
	if (state != DSTATE && state != DFOUND) {
		freeino(ino);
		return (0);
	}
	setstate(ino, DFOUND);
	dp = sginode(parent);
	dp->di_nlink++;
	inodirty();
	return (ino);
}


/*
 * Procedure:     freedir
 *
 * Restrictions:  none
 *
 * Notes:				  free a directory inode
 */
freedir(ino, parent)
	ino_t ino, parent;
{
	DINODE *dp;

	if (ino != parent) {
		dp = sginode(parent);
		dp->di_nlink--;
		inodirty();
	}
	freeino(ino);
}


/*
 * Procedure:     lftempname
 *
 * Restrictions:  none
 *
 * Notes:         generate a temporary name for the lost+found directory.
 */

lftempname(bufp, ino)
	char *bufp;
	ino_t ino;
{
	register ino_t in;
	register char *cp;
	int namlen;

	cp = bufp + 2;
	for (in = imax; in > 0; in /= 10)
		cp++;
	*--cp = 0;
	namlen = cp - bufp;
	in = ino;
	while (cp > bufp) {
		*--cp = (in % 10) + '0';
		in /= 10;
	}
	*cp = '#';
	return (namlen);
}

/*
 * Procedure:     traverse
 *
 * Restrictions:  none
 *
 * Note:	Traverse the memory directory to check for '.' and '..' inodes.
 * 		'.' and '..' inodes will be updated if bad.
 * 		Update link count of each directory as traverse the tree.
 */
traverse(ino, parent, saveparent)
ino_t	ino, parent, saveparent;
{
	struct dirmap   *dir_p;
        struct inolist  *inop;
	struct  inodesc idesc;
	int i=0;

	if (ino == 0 || parent == 0 || ino > imax || parent > imax)
		return;
	if (nstatemap(ino).flag == DCLEAR || nstatemap(ino).flag == USTATE ||
		nstatemap(ino).flag == DCORRPT)
		return;
	memset((char *)&idesc, 0, sizeof(struct inodesc));
        dir_p = nstatemap(ino).dir_p;
	idesc.id_filesize = dir_p->filesize;
	idesc.id_number = ino;
	idesc.id_loc = 0;
	idesc.id_func = fixentry;
	idesc.id_type = DATA;
	/* 
	 * if inode flag is DFOUND, it's hard link to directory
         * call inmem_readdir to zero out the inode for this entry
         */
	if (nstatemap(ino).flag == DFOUND) {
		WYPFLG(wflg, yflag, preen);
		getsem();
		pwarn(":336:DIR INODE %d HAS AN EXTRANEOUS HARD LINK OF DIR INODE %d\n", parent, ino);
		if (preen)
			Pprintf(":274: (IGNORED)\n");
		else if (reply(1, gettxt(":86","REMOVE")) == 1) {
			relsem();
			if (ino == parent)
				inmem_readdir(dir_p, 0, &idesc, 0, ino, 1, 0);
			else {
        			dir_p = nstatemap(parent).dir_p;
				idesc.id_filesize = dir_p->filesize;
				idesc.id_number = parent;
				inmem_readdir(dir_p, 0, &idesc, 0, ino, 1, 0);
			}
			getsem();
		}
		relsem();
		lncntp[parent]++;
		return;
	} 
	/* bad inode number for ".", called inmem_readdir to fix it */
        if (dir_p->dot != ino) {
		WYPFLG(wflg, yflag, preen);
		getsem();
               	direrr(dir_p->dot, ":262:BAD INODE NUMBER FOR '.'");
		if (reply(1, gettxt(":337","FIX '.'"))== 1){
			relsem();
			inmem_readdir(dir_p, 0, &idesc, 0, ino, 0, 0);
			dir_p->dot = ino;
			getsem();
		}
		relsem();
		lncntp[ino]--;
	} else
		lncntp[ino]--;

	/* bad inode number for "..", called inmem_readdir to fix it */
        if (dir_p->dotdot != parent) {
		if (dir_p->dotdot) {
			WYPFLG(wflg, yflag, preen);
			getsem();
                	direrr(dir_p->dotdot, ":266:BAD INODE NUMBER FOR '..'");
                	/* fix dotdot entry */
			if (reply(1, gettxt(":338","FIX '..'")) == 1){
				relsem();
				inmem_readdir(dir_p, 0, &idesc, 0, parent, 0, 0);
				getsem();
			}
			relsem();
		} else {
                	/* fix dotdot entry */
			inmem_readdir(dir_p, 0, &idesc, 0, parent, 0, 0);
		}
		dir_p->dotdot = parent;		/* fix dirmap dotdot */
		lncntp[ino]--;
        } else
		lncntp[ino]--;

	/* if an inode flag is DSTATE, traverse it tree to count link count */
	if (nstatemap(ino).flag != DFOUND) {
		nstatemap(ino).flag = DFOUND;
		inop = dir_p->inolist;
		while (inop != NULL) {
			lncntp[ino]--;
			saveparent = parent;
               		traverse(inop->inumber, ino, saveparent);
			inop = inop->next;
		}
	}
}

/*
 * Procedure:     inmem_readdir
 *
 * Restrictions:  none
 *
 * Note:	Read data blocks of each directory.
 *		Check for missing '.' , '..', * extra '.', and '..'.
 *		Check corrupted directories  and record inodes of
 * 		subdirectories in the inolist of the memory tree.
 */
inmem_readdir(dmap_p, nolist, idescp, discino, ino, hardlink, nolcnt)
struct dirmap	*dmap_p;
int	nolist;
struct inodesc	*idescp;
int	discino;
ino_t	ino;
int hardlink, nolcnt;
{
	register DIRECT *dp, *tdp;
	DIRECT	proto, *ndp, *dirp;
	long size, blksiz;
	int	ret= 0, entrysize, n, first_block =1;
	int	offset, ndb, cnt, dbcnt, j = 0, nosem=0;
	daddr_t	*dbp;
	ulong	filesize = idescp->id_filesize, readsize;
	char	dbuf[DIRBLKSIZ];
	int	missdotdot = 0;
	struct inolist	*tinolp;
	DINODE *tp;

	blksiz = sblock.fs_bsize;
	ndb = dbcnt = howmany(idescp->id_filesize, sblock.fs_bsize);
	if (filesize > blksiz)
		readsize = blksiz;
	else
		readsize = filesize;
	size = idescp->id_filesize;
	if ((idescp->id_filesize & (DIRBLKSIZ - 1)) != 0) {
		idescp->id_filesize = roundup(idescp->id_filesize, DIRBLKSIZ);
	}
	for (cnt = 0, dbp = dmap_p->dblist; cnt < dbcnt; cnt++, dbp++) {

		/*
 		* Calculate number of fragments
 		*/
		if (--ndb == 0 && (offset = blkoff(&sblock, size)) != 0)
			idescp->id_numfrags =
				numfrags(&sblock, fragroundup(&sblock, offset));
		else
			idescp->id_numfrags = sblock.fs_frag;
		if (outrange(*dbp, idescp->id_numfrags)) {
			idescp->id_filesize -= (idescp->id_numfrags * sblock.fs_fsize);
			continue;
		}
		if (idescp->id_filesize < readsize)
                       readsize = idescp->id_filesize;
		getblk(&fileblk, *dbp, readsize);
		if (fileblk.b_errs != NULL) {
			idescp->id_filesize -= readsize;
			idescp->id_loc += readsize;
			continue;
		}
		dp = (DIRECT *)dirblk.b_buf;
		if (first_block && (hardlink == 0)) {
			first_block = 0;
			if (dirchk(idescp, dp) == NULL)
				return(0);
			memcpy(dbuf, (char *)dp, dp->d_reclen);
			idescp->id_dirp = (DIRECT *)dbuf;

			if (nstatemap(dp->d_ino).flag == DCLEAR){
				direrr(dp->d_ino, ":82:DUP/BAD");
				if (reply(1, gettxt(":86","REMOVE")) == 1){
					getsem();
					idescp->id_dirp->d_ino = 0;
					fixentry(dp, *dbp, readsize, dbuf);
					relsem();
					return(0);
				} else
					setstate(dp->d_ino, DSTATE);
			}
			if (idescp->id_func == mkentry ||
				idescp->id_func == chgino ||
				idescp->id_func == findino)
				goto chk1;

			/* first datablock  - check/record "." and ".." */
			if (ino == idescp->id_number && dmap_p->dot != ino &&  
				idescp->id_func == fixentry) {
				idescp->id_dirp->d_ino = ino;
				fixentry(dp, *dbp, readsize, dbuf);
				return(0);
			}
			dmap_p->dot = dp->d_ino;
			/* fix bad inode number for dot */
			if (strcmp(dp->d_name, ".") != 0) {
				WYPFLG(wflg, yflag, preen);
	                        getsem();
	        		direrr(idescp->id_number, ":263:MISSING '.'");
				proto.d_ino = dmap_p->dot = idescp->id_number;
				proto.d_namlen = 1;
				(void)strcpy(proto.d_name, ".");
				entrysize = DIRSIZ(&proto);
				if (preen)
	                        	relsem();
				if ((int)dp->d_reclen < entrysize)
					mypfatal(":265:CANNOT FIX, INSUFFICIENT SPACE TO ADD '.'\n");
				else if ((int)dp->d_reclen < 2 * entrysize) {
					proto.d_reclen = dp->d_reclen;
					memcpy(dbuf, (char *)&proto, entrysize);
					if (reply(1, gettxt(":42","FIX")) == 1){
						relsem();
						fixentry(dp, *dbp, readsize, dbuf);
						nosem++;
					}
				} else {
					/* fix both "." and ".." entries 
					* when a director is corrupted
					*/
					n = dp->d_reclen - entrysize;
					proto.d_reclen = entrysize;
					dp->d_reclen = entrysize;
					if (reply(1, gettxt(":42","FIX")) == 1) {
						relsem();
						memcpy(dbuf, (char *)&proto, entrysize);
						fixentry(dp, *dbp, readsize, dbuf);
						proto.d_reclen = n;
						proto.d_ino = 0;
						proto.d_namlen = 2;
						(void)strcpy(proto.d_name, "..");
						tdp = (DIRECT *)((char *)dp + entrysize);
						memset((char *)tdp, 0, n);
						tdp->d_reclen = n;
						memcpy(dbuf, (char *)&proto, n);
						fixentry(tdp, *dbp, readsize, dbuf);
						nosem++;
					}
				}
				if (!nosem) {
					relsem();
					nosem = 0;
				}
	                }
			tdp = dp;
chk1:
			dp = (DIRECT *)((int)dp + dp->d_reclen);
			if (dirchk(idescp, dp) == NULL)
				return(0);
			memcpy(dbuf, (char *)dp, dp->d_reclen);
			idescp->id_dirp = (DIRECT *)dbuf;

			/* fix bad inode number for dotdot */
			if (ino != dmap_p->dotdot && 
				idescp->id_func == fixentry) {
				idescp->id_dirp->d_ino = ino;
				fixentry(dp, *dbp, readsize, dbuf);
				return(0);
			}
			/* func is either chgino, mkentry, or findino
                         * when inmem_readdir is called from ckinode.
                         */
			if (idescp->id_func != NULL && idescp->id_func != fixentry) {
				if ((ret = (*idescp->id_func)(idescp)) & ALTERED)
					fixentry(dp, *dbp, readsize, dbuf);
				if (ret & STOP)
					return(ret);
				goto next;
			}
			proto.d_namlen = 2;
			(void)strcpy(proto.d_name, "..");
			entrysize = DIRSIZ(&proto);
			if (strcmp(dp->d_name, "..") != 0) {
				/* dotdot is missing but cannot fix its number
				 * here since we don't know the parent.
                                 * It will be fix when the tree is traversed
                                 * for now 0 is used except for root.
                                 */
				WYPFLG(wflg, yflag, preen);
				missdotdot++;
				if (idescp->id_number == SFSROOTINO)
					proto.d_ino = SFSROOTINO;
				else 
					proto.d_ino = 0;
	                        getsem();
				direrr(idescp->id_number,":267:MISSING '..'");
				if (preen)
	                        	relsem();
				if (tdp->d_ino != 0 && strcmp(tdp->d_name, ".") != 0)
					mypfatal(":268:CANNOT FIX, SECOND ENTRY IN DIRECTORY CONTAINS %s\n", tdp->d_name);
				else if ((int)tdp->d_reclen < entrysize)
					mypfatal(":269:CANNOT FIX, INSUFFICIENT SPACE TO ADD '..'\n");
				else {
					n = DIRSIZ(tdp);
					if ((int)tdp->d_reclen < n + entrysize){
						if (tdp->d_reclen >= 12) {
						    if (reply(1, gettxt(":42","FIX")) == 1){
							relsem();
							proto.d_reclen = dp->d_reclen;
							proto.d_ino = dp->d_ino;
							memcpy(dbuf, (char *)&proto, entrysize);
							fixentry(dp, *dbp, readsize,dbuf);
							nosem++;
						    }
						}
						goto next; 
					}
					proto.d_reclen = tdp->d_reclen - n;
					tdp->d_reclen = n;
					if (reply(1, gettxt(":42","FIX")) == 1){
						relsem();
						memcpy(dbuf, (char *)tdp, n);
						fixentry(tdp, *dbp, readsize,dbuf);
						dp = (DIRECT *)((int)tdp + n);
						memcpy(dbuf, (char *)dp, dp->d_reclen);
						idescp->id_dirp = (DIRECT *)dbuf;
						memset((char *)dp , 0, n);
						dp->d_reclen = proto.d_reclen;
						memcpy(dbuf, (char *)&proto, dp->d_reclen);
						fixentry(dp, *dbp, readsize,dbuf);
						nosem++;
					}
				}
				if (!nosem)
					relsem();
			}
next:
			if (discino)
				dmap_p->dotdot = idescp->id_parent;
			else if (missdotdot)
				dmap_p->dotdot = proto.d_ino;
			else if (idescp->id_func == NULL)
				dmap_p->dotdot = dp->d_ino;
			dp = (DIRECT *)((int)dp + dp->d_reclen);
		} else if (first_block && hardlink) {
			first_block = 0;
       			idescp->id_filesize -= dp->d_reclen;
       			idescp->id_loc += dp->d_reclen;
			dp = (DIRECT *)((int)dp + dp->d_reclen);
       			idescp->id_filesize -= dp->d_reclen;
       			idescp->id_loc += dp->d_reclen;
			dp = (DIRECT *)((int)dp + dp->d_reclen);
		}
chk2:		while (idescp->id_filesize > 0 &&
			 (int)((char *)dp - (char *)&dirblk) < readsize) {
			if (*dbp == 0){
				/* no backing storage for this dir, skips it */
				idescp->id_filesize -= readsize;
				tp = sginode(idescp->id_number);
				tp->di_size -= readsize;
				inodirty();
				break;
			}
			
			/* fix the extra hard link to directory */ 
			if (dp->d_ino == ino && idescp->id_func == fixentry && hardlink) {
				dp->d_ino = 0;
				dirty(&fileblk);
				sbdirty();
				return(0);
			}
			if (hardlink) {
       				idescp->id_filesize -= dp->d_reclen;
       				idescp->id_loc += dp->d_reclen;
				goto skip;
			}

			/* check the entry */
			if (dirchk(idescp, dp) == NULL)
				return(0);

			*idescp->id_dirp = *dp;
			memcpy(pathname, dp->d_name, dp->d_namlen + 1);
			memcpy(dbuf, (char *)dp, dp->d_reclen);
			idescp->id_dirp = (DIRECT *)dbuf;

			if (idescp->id_func != NULL) {
				if ((ret = (*idescp->id_func)(idescp)) & ALTERED)
					fixentry(dp, *dbp, readsize, dbuf);
				if (ret & STOP)
					return(ret);
				goto skip;
			}
			if (dp->d_ino == 0) {
				if (dp->d_reclen == 0) {
					dp->d_reclen = 512;
					idescp->id_dirp->d_reclen = 512;
					fixentry(dp, *dbp, readsize, dbuf);
					idescp->id_filesize -= dp->d_reclen;
                                        idescp->id_loc += dp->d_reclen;
				}
				goto skip;
			}
			if (strcmp(dp->d_name,".") == 0) {
				if (dp->d_ino == 0)
					goto skip;
				WYPFLG(wflg, yflag, preen);
				getsem();
				direrr(idescp->id_number, ":270:EXTRA '.' ENTRY");
				if (reply(1, gettxt(":42","FIX")) == 1){
					relsem();
					idescp->id_dirp->d_ino = 0;
					fixentry(dp, *dbp, readsize, dbuf);
					getsem();
				}
				relsem();
				goto skip;
			} else if (strcmp(dp->d_name, "..") == 0) {
				if (dp->d_ino == 0)
					goto skip;
				WYPFLG(wflg, yflag, preen);
				getsem();
				direrr(idescp->id_number, ":271:EXTRA '..' ENTRY");
				if (reply(1, gettxt(":42","FIX")) == 1){
					relsem();
					idescp->id_dirp->d_ino = 0;
					fixentry(dp, *dbp, readsize, dbuf);
					getsem();
				}
				relsem();
				goto skip;
			} 

			/* for each entry of a directory check it status
                         * fix if it's unallocate, unreference, update
                         * link count map and if it's a directory create
                         * it's inolist from dirmap
                         */
again :
			switch (nstatemap(dp->d_ino).flag) {
			case USTATE: 
				WYPFLG(wflg, yflag, preen);
				getsem();
				direrr(dp->d_ino, ":81:UNALLOCATED");
				if (reply(1, gettxt(":86","REMOVE"))) {
					relsem();
					idescp->id_dirp->d_ino = 0;
					fixentry(dp, *dbp, readsize, dbuf);
					break;
				}
				relsem();
				break;

			case DCLEAR:
			case FCLEAR:
				WYPFLG(wflg, yflag, preen);
				getsem();
				direrr(dp->d_ino, ":82:DUP/BAD");
				if (reply(1, gettxt(":86","REMOVE")) == 1) {
					relsem();
					idescp->id_dirp->d_ino = 0;
					fixentry(dp, *dbp, readsize, dbuf);
					break;
				}
				relsem();
				tp = sginode(dp->d_ino);
				nstatemap(dp->d_ino).flag = DIRCT(tp) ? DSTATE : FSTATE;
				goto again;

			case FSTATE:
				if (pass2sechk(dp->d_ino)) {
					idescp->id_dirp->d_ino = 0;
					fixentry(dp, *dbp, readsize, dbuf);
				}
				if (!nolcnt)
					lncntp[dp->d_ino]--;
				if (idescp->id_number == SFSROOTINO)
					if (strcmp(dp->d_name,"lost+found") == 0)
						lfdir = (lfdir == 0)? dp->d_ino: lfdir;
				break;

			case DFOUND:
			case DSTATE:
				if ((chkdirsiz_descend(NULL,dp->d_ino) & STOP) != 0) {
                                        idescp->id_dirp->d_ino = 0;
                                        fixentry(dp, *dbp, readsize, dbuf);
                                        break;
                                }
				if (pass2sechk(dp->d_ino)) {
					idescp->id_dirp->d_ino = 0;
					fixentry(dp, *dbp, readsize, dbuf);
					break;
				}
				if (nolist)
					break;
				tinolp = (struct inolist *)malloc(sizeof (struct inolist));
				if (tinolp == NULL)
					errexit(":340:No space\n");
				tinolp->inumber = dp->d_ino;
				tinolp->next = NULL;
				if (dmap_p->inolist != NULL)
					tinolp->next = dmap_p->inolist;
				dmap_p->inolist = tinolp;
				if (idescp->id_number == SFSROOTINO)
					if (strcmp(dp->d_name,"lost+found") == 0)
						lfdir = (lfdir == 0)? dp->d_ino: lfdir;
				break;

			default:
				/* this is invalid inode, remove the entry */
				getsem();
				idescp->id_dirp->d_ino = 0;
				fixentry(dp, *dbp, readsize, dbuf);
				relsem();
				break;
			}
skip:
	        	dp = (DIRECT *)((int)dp + dp->d_reclen);

		}
	}
	return(ret);
}

/*
 * Procedure:     fixentry
 *
 * Restrictions:  none
 *
 * Note:	
 * Copy the fixed directory entry to the buffer and mark the buffer dirty. 
 */
fixentry(dp, db, size, dbuf)
DIRECT	*dp;
daddr_t	db;
ulong	size;
char	dbuf[DIRBLKSIZ];
{
	getblk(&fileblk, db, size);
	if (fileblk.b_errs != NULL) {
		errexit(":339:Cannot read data block\n");
	}
	memcpy((char *)dp, dbuf, dp->d_reclen);	
	dirty(&fileblk);
	sbdirty();
}

/*
 * Procedure:     dirchk
 *
 * Restrictions:  none
 *
 * Note:	
 * Check directory entry
 */
dirchk(idescp, dp)
struct	inodesc	*idescp;
DIRECT	*dp;
{
	ulong	size, left;
	DIRECT	*tdp;

	if (idescp->id_loc % DIRBLKSIZ == 0 && idescp->id_filesize > 0 ) {
		if (dircheck(idescp, dp) == 0) {
			/*
                        * if reclen is legitimate then it is used to skip
                        * and fix the bad file. Otherwize DIRBLKSIZ is used.
                        */
			if ((dp->d_reclen != 0) &&
			    ((dp->d_reclen & 0x3) == 0) &&
   			    ((int)dp->d_reclen <= DIRBLKSIZ) &&
			    (int)dp->d_reclen >= DIRSIZ(dp)) {
				size = dp->d_reclen;
			} else
				size = DIRBLKSIZ;

			/* for "." don't remove it */
			if (idescp->id_loc == 0) {
               			dp->d_namlen = 1;
			} else {
               			dp->d_ino = 0;
               			dp->d_namlen = 0;
               			dp->d_name[0] = '\0';
			}
               		dp->d_reclen = size;
       			idescp->id_filesize -= size;
       			idescp->id_loc += size;
               		if (dofix(idescp, ":180:DIRECTORY CORRUPTED"))
                	       	dirty(&fileblk);
			return 1;
		}
	}
	if (idescp->id_filesize <= 0)
       	        return NULL;
       	idescp->id_filesize -= dp->d_reclen;
       	idescp->id_loc += dp->d_reclen;
       	if ((idescp->id_loc % DIRBLKSIZ) == 0)
      		return 1;
       	tdp = (DIRECT *)((int)dp + dp->d_reclen);

       	if (idescp->id_filesize > 0 && dircheck(idescp, tdp) == 0) {
		left = DIRBLKSIZ - (idescp->id_loc % DIRBLKSIZ);
		/*
                 * if reclen is legitimate then it is used to skip
                 * and fix the bad file.
                 */
		if (tdp->d_ino != 0 &&
		    tdp->d_reclen <= left &&
		    (dp->d_reclen & 0x3) == 0 &&
		    tdp->d_reclen >= DIRSIZ(tdp)) {
			size = tdp->d_reclen;
		} else {
			size = left;
		}
		/* for ".." don't clump it with "." */
		if (idescp->id_loc == 12) {
       			tdp->d_namlen = 2;
	       		tdp->d_reclen = size;
		} else { 
	       		dp->d_reclen += size;
       			idescp->id_loc += size;
       			idescp->id_filesize -= size;
		}
       		if (dofix(idescp, ":180:DIRECTORY CORRUPTED"))
               		dirty(&fileblk);
	}
	return 1;
}

/*	copyright	"%c%"	*/

/*
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

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/utilities.c	1.4.6.1"

/*  "errexit()", "pfatal()", "pwarn()", "dofix()", "dirent()", "Pprintf()",
 *  "Sprintf()", "mypfatal()", "myprintf()" and "myfprintf()" have
 *  been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *
 *  "errexit()" and "pfatal()" output using <MM_ERROR>.
 *  "pwarn()", "dofix()" and "direrr()" output using <MM_WARNING>.
 *  "myfprint()" requires the output severity as an argument.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mntent.h>
#include <sys/fs/sfs_fs.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#define _KERNEL
#include <sys/fs/sfs_fsdir.h>
#undef _KERNEL
#include <sys/mnttab.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <string.h>
#include "fsck.h"
#include <sys/vfstab.h>
#include <errno.h>
#include <pfmt.h>
#include <regex.h>

long	lseek();
extern int macinit;
extern regex_t yesre, nore;

/*
 * Procedure:     reply
 *
 * Restrictions:
 *                printf: none
 */

reply(lock, s)
	int lock;
	char *s;
{
	char line[80];
	int ret, len;

	if ((!lock && Pflag) && (!Lflag || preen || yflag))
		getsem();
	if (preen)
		pfatal(":301:INTERNAL ERROR: GOT TO reply()");
	Pprintf(":302:%s? ", s);
	if (nflag || dfile.wfdes < 0) {
		Sprintf(":114: no\n\n");
		exitstat = 36;		/* remember there's still an error */
		ret = 0;
		goto exit;
	}
	if (yflag) {
		Sprintf(":115: yes\n\n");
		ret = 1;
		goto exit;
	}
	if (Lflag && (Parent_notified == B_FALSE) && !preen && !yflag) {
                kill(getppid(), SIGUSR1);
                Parent_notified = B_TRUE;
        }
	line[0] = '\0';
        while (line[0] == '\0') {
                if (getline(stdin, line, sizeof(line)) == EOF) {
                        if (!lock)
                                relsem();
                        errexit("\n");
                }
                Sprintf("\n");

                len = strlen(line);
                if (len && regexec(&yesre, line, (size_t)0, 
		    (regmatch_t *)0, 0) == 0)
                        ret = 1;
                else  if (len && regexec(&nore, line, (size_t)0, 
		    (regmatch_t *)0, 0) == 0) {
                        exitstat = 36;  /* remember there's still an error */
                        ret = 0;
                } else {
			pfmt(stdout,MM_NOSTD,
                        ":410:Answer 'y' or 'n' (yes or no)\n");
                        line[0] = '\0';
		}
	}
exit:	if (Pflag && !lock)
		relsem();
	return(ret);
}


/*
 * Procedure:     getline
 *
 * Restrictions:  none
*/

getline(fp, loc, maxlen)
	FILE *fp;
	char *loc;
{
	register n;
	register char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF)
			return (EOF);
		if (!isspace(n) && p < lastloc)
			*p++ = n;
	}
	*p = 0;
	return (p - loc);
}


/*
 * Procedure:     getblk
 *
 * Restrictions:  none
*/

BUFAREA *
getblk(bp, blk, size)
	register BUFAREA *bp;
	daddr_t blk;
	long size;
{
	register struct filecntl *fcp;
	daddr_t dblk;

	fcp = &dfile;
	dblk = fsbtodb(&sblock, blk);
	if (bp->b_bno == dblk)
		return (bp);
	flush(fcp, bp);
	bp->b_errs = bread(fcp, bp->b_un.b_buf, dblk, size);
	bp->b_bno = dblk;
	bp->b_size = size;
	return (bp);
}


/*
 * Procedure:     flush
 *
 * Restrictions:  none
*/

flush(fcp, bp)
	struct filecntl *fcp;
	register BUFAREA *bp;
{
	register int i, j;

	if (!bp->b_dirty)
		return;
	if (bp->b_errs != 0)
		mypfatal(":303:WRITING ZERO'ED BLOCK %d TO DISK\n", bp->b_bno);
	bp->b_dirty = 0;
	bp->b_errs = 0;
	bwrite(fcp, bp->b_un.b_buf, bp->b_bno, (long)bp->b_size);
	if (bp != &sblk)
		return;
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		bwrite(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize);
	}
}


/*
 * Procedure:     rwerr
 *
 * Restrictions:
 *                printf: none
 */

rwerr(s, blk)
	char *s;
	daddr_t blk;
{

	WYPFLG(wflg, yflag, preen);
	if (preen == 0)
		myfprintf(stdout, MM_NOGET|MM_NOSTD, "\n");
	mypfatal(":304:CANNOT %s: BLK %ld", s, blk);
	if (reply(0, gettxt(":35","CONTINUE")) == 0) {
		errexit(":305:Program terminated\n");
	}
}


/*
 * Procedure:     ckfini
 *
 * Restrictions:  none
*/

ckfini()
{

	flush(&dfile, &fileblk);
	flush(&dfile, &sblk);
	if (sblk.b_bno != SBLOCK) {
		sblk.b_bno = SBLOCK;
		sbdirty();
		flush(&dfile, &sblk);
	}
	if (MEM)
		bigflush();
	flush(&dfile, &inoblk);
	flush(&dfile, &cgblk);
	(void)close(dfile.rfdes);
	(void)close(dfile.wfdes);
}


/*
 * Procedure:     bread
 *
 * Restrictions:
 *                read(2): none
 *                printf: none
*/

bread(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	long size;
{
	char *cp;
	int i, errs;

	if (lseek(fcp->rfdes, (long)dbtob(blk), 0) < 0)
		rwerr(gettxt(":119","SEEK"), blk);
	else {
		if (read(fcp->rfdes, buf, (int)size) == size) {
			return (0);
		}
	}
	rwerr(gettxt(":120","READ"), blk);
	if (lseek(fcp->rfdes, (long)dbtob(blk), 0) < 0)
		rwerr(gettxt(":119","SEEK"), blk);
	errs = 0;
	getsem();
	pfatal(":306:THE FOLLOWING SECTORS COULD NOT BE READ:");
	for (cp = buf, i = 0; i < size; i += DEV_BSIZE, cp += DEV_BSIZE) {
		if (read(fcp->rfdes, cp, DEV_BSIZE) < 0) {
			Pprintf(":350: %d,", blk + i / DEV_BSIZE);
			memset(cp, 0, DEV_BSIZE);
			errs++;
		}
	}
	Sprintf("\n");
	relsem();
	return (errs);
}


/*
 * Procedure:     bwrite
 *
 * Restrictions:
 *                write(2): none
 *                printf: none
 */

bwrite(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	long size;
{
	int i;
	char *cp;

	if (fcp->wfdes < 0)
		return;
	if (lseek(fcp->wfdes, (long)dbtob(blk), 0) < 0)
		rwerr(gettxt(":119","SEEK"), blk);
	else {
		if (write(fcp->wfdes, buf, (int)size) == size) {
			fcp->mod = 1;
			return;
		}
	}
	rwerr(gettxt(":121","WRITE"), blk);
	if (lseek(fcp->wfdes, (long)dbtob(blk), 0) < 0)
		rwerr(gettxt(":119","SEEK"), blk);
	getsem();
	pfatal(":307:THE FOLLOWING SECTORS COULD NOT BE WRITTEN:");
	for (cp = buf, i = 0; i < size; i += DEV_BSIZE, cp += DEV_BSIZE)
		/*
		 * The following write(2) needs P_DEV. It should be in the
		 * working set since it was put there at the beginning of
		 *  main().
		 */
		if (write(fcp->wfdes, cp, DEV_BSIZE) < 0)
			Pprintf(":350: %d,", blk + i / DEV_BSIZE);
	Sprintf("\n");
	relsem();
	return;
}


/*
 * Procedure:     allocblk
 *
 * Restrictions:	none
 *
 * Notes:	allocate a data block with the specified number of fragments
 */

allocblk(frags)
	int frags;
{
	register int i, j, k;

	if (frags <= 0 || frags > sblock.fs_frag)
		return (0);
	for (i = 0; i < fmax - sblock.fs_frag; i += sblock.fs_frag) {
		for (j = 0; j <= sblock.fs_frag - frags; j++) {
			if (getbmap(i + j))
				continue;
			for (k = 1; k < frags; k++)
				if (getbmap(i + j + k))
					break;
			if (k < frags) {
				j += k;
				continue;
			}
			for (k = 0; k < frags; k++)
				setbmap(i + j + k);
			n_blks += frags;
			return (i + j);
		}
	}
	return (0);
}


/*
 * Procedure:     freeblk
 *
 * Restrictions:  none
 * Notes:         Free a previously allocated block
 */

freeblk(blkno, frags)
	daddr_t blkno;
	int frags;
{
	struct inodesc idesc;

	idesc.id_blkno = blkno;
	idesc.id_numfrags = frags;
	pass4check(&idesc);
}


/*
 * Procedure:     getpathname
 *
 * Restrictions:	none
 * Notes:         Find a pathname
 */
getpathname(namebuf, curdir, ino)
	char *namebuf;
	ino_t curdir, ino;
{
	int len;
	register char *cp;
	struct inodesc idesc;
	extern int findname();
	char state;

	state = get_state(ino);
	if (state != DSTATE && state != DFOUND) {
		strcpy(namebuf, "?");
		return;
	}
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	cp = &namebuf[BUFSIZ - 1];
	*cp-- = '\0';
	if (curdir != ino) {
		idesc.id_parent = curdir;
		goto namelookup;
	}
	while (ino != SFSROOTINO) {
		idesc.id_number = ino;
		idesc.id_func = findino;
		idesc.id_secfunc = 0;
		idesc.id_name = "..";
		if ((ckinode(ginode(ino), &idesc, 0, 0, 0, 1) & STOP) == 0)
			break;
	namelookup:
		idesc.id_number = idesc.id_parent;
		idesc.id_parent = ino;
		idesc.id_func = findname;
		idesc.id_secfunc = 0;
		idesc.id_name = namebuf;
		if ((ckinode(ginode(idesc.id_number), &idesc, 0, 0, 0, 1) & STOP) == 0)
			break;
		len = strlen(namebuf);
		cp -= len;
		if (cp < &namebuf[SFS_MAXNAMLEN])
			break;
		memcpy(cp, namebuf, len);
		*--cp = '/';
		ino = idesc.id_number;
	}
	if (ino != SFSROOTINO) {
		strcpy(namebuf, "?");
		return;
	}
	memcpy(namebuf, cp, &namebuf[BUFSIZ] - cp);
}


/*
 * Procedure:     catch
 *
 * Restrictions:
*/

catch()
{

	ckfini();
	exit(37);
}


/*
 * Procedure:     catchquit
 *
 * Restrictions:
 *                printf: none
 * Notes:         When preening, allow a single quit to signal
 *                a special exit after filesystem checks complete
 *                so that reboot sequence may be interrupted.
 */

catchquit()
{
	extern returntosingle;

	myprintf(":308:returning to single-user after filesystem check\n");
	returntosingle = 1;
	(void)signal(SIGQUIT, SIG_DFL);
}


/*
 * Procedure:     voidquit
 *
 * Restrictions:  none
 *
 * Notes:
 *
 * Ignore a single quit signal; wait and flush just in case.
 * Used by child processes in preen.
 */
voidquit()
{

	sleep(1);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_DFL);
}


/*
 * Procedure:     dofix
 *
 * Restrictions:
 *                printf: none
 *
 * Notes:         determine whether an inode should be fixed.
 */

dofix(idesc, msg, a1)
	register struct inodesc *idesc;
	char *msg;
{
	char state;
        ino_t   inumber;

	inumber = idesc->id_number;
        state =get_state(inumber);
	WYPFLG(wflg, yflag, preen);
	switch (idesc->id_fix) {

	case DONTKNOW:
		getsem();
		if (idesc->id_type == DATA)
			direrr(inumber, msg, a1);
		else
			pwarn(msg,a1);
		if (preen) {
			Pprintf(":309: (SALVAGED)\n");
			idesc->id_fix = FIX;
			if (state == DSTATE)
                                setstate(inumber, DCORRPT);
			relsem();
			return (ALTERED);
		}
		if (reply(1, gettxt(":59","SALVAGE")) == 0) {
			idesc->id_fix = NOFIX;
			if (state == DSTATE)
                                setstate(inumber, DCORRPT);
			relsem();
			return (0);
		}
		relsem();
		if (state == DSTATE)
			setstate(inumber, DCORRPT);
		idesc->id_fix = FIX;
		return (ALTERED);

	case FIX:
		if (state == DSTATE)
			setstate(inumber, DCORRPT);
		return (ALTERED);

	case NOFIX:
		return (0);

	default:
		errexit(":310:UNKNOWN INODESC FIX MODE %d\n", idesc->id_fix);
	}
	/* NOTREACHED */
}


/*
 * Procedure:     errexit
 *
 * Restrictions:
 *                printf: none
*/

/* VARARGS1 */
errexit(s1, s2, s3, s4)
char *s1;
{
	if(*s1 == '\n')
		fprintf(stderr, s1);
	else if(*s1 != NULL)
		myfprintf(stderr, MM_ERROR, s1, s2, s3, s4);
	exit(39);
}


/*
 * Procedure:     pfatal
 *
 * Restrictions:
 *                printf: none
 * Notes:
 *
 * An inconsistency occured which shouldn't during normal operations.
 * Die if preening, otherwise just printf.
 */

/* VARARGS1 */
pfatal(s, a1, a2, a3)
char *s;
{
	if (preen) {
		Pprintf(":346:%s: ", devname);
		Pprintf(s, a1, a2, a3);
		Pprintf("\n");
		Pprintf(":311:%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n",
			devname);
		relsem();
		exit(36);
	}
	if (Pflag && !bflg)	{
		Sprintf(":346:%s: ",devname);
		Sprintf(s, a1, a2, a3);
	} else
		pfmt(stdout, MM_NOSTD, s, a1, a2, a3);
}


/*
 * Procedure:     mypfatal
 *
 * Restrictions:
 *                printf: none
 * Notes:
 * 	derived from pfatal(). 
 */
/* VARARGS1 */
mypfatal(s, a1, a2, a3)
	char *s;
{
	getsem();
	if (preen) {
		Pprintf(":346:%s: ", devname);
		Pprintf(s, a1, a2, a3);
		Pprintf("\n");
		Pprintf(":311:%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n", devname);
		relsem();
		exit(36);
	}
	if (Pflag && !bflg) {
		Pprintf(":346:%s: ", devname);
		Pprintf(s, a1, a2, a3);
	} else
		pfmt(stdout, MM_NOSTD, s, a1, a2, a3);
	relsem();
}

/*
 * Procedure:     pwarn
 *
 * Restrictions:
 *                printf: none
 * Notes:
 *
 * Pwarn is like printf when not preening,
 * or a warning (preceded by filename) when preening.
 */

/* VARARGS1 */
pwarn(s, a1, a2, a3, a4, a5, a6, a7, a8)
char *s;
{
	if (Pflag && Parent_notified == B_FALSE) {
		pfmt(print_fp, MM_WARNING|MM_NOGET, "%s: ", devname);
		pfmt(print_fp, MM_NOSTD, s, a1, a2, a3, a4, a5, a6, a7, a8);
		fflush(print_fp);
	} else if ((Pflag || preen) && !bflag) {
		pfmt(stdout, MM_WARNING|MM_NOGET, "%s: ", devname);
		pfmt(stdout, MM_NOSTD, s,a1, a2, a3, a4, a5, a6, a7, a8);
	} else
		pfmt(stdout, MM_WARNING, s, a1, a2, a3, a4, a5, a6, a7, a8);
}

#ifndef lint


/*
 * Procedure:     panic
 *
 * Restrictions:  none
 *
 * Notes:         Stub for routines from kernel.
 */

panic(s)
	char *s;
{

	mypfatal(":312:INTERNAL INCONSISTENCY:");
	errexit(s);
}

#define CE_PANIC 3

/*
 * Procedure:     cmn_err
 *
 * Restrictions:
 *                printf: none
*/

void
cmn_err(level, s)
	register int level;
	char *s;
{

	if (level == CE_PANIC) {
		mypfatal(":312:INTERNAL INCONSISTENCY:");
		errexit(s);
	}
	else
		myfprintf(stderr, MM_ERROR, s);
}
#endif


/*
 * Procedure:     mounted
 *
 * Restrictions:
 *                fopen: P_MACREAD, only if the MAC level of the process
 *			 is initialized.
 *                printf: none
 *                getmntent: none
 *                stat(2): none
 *                fclose: none
 * Notes:
 *
 * Check to see if unraw version of name is already mounted.
 * Since we do not believe /etc/mnttab, we stat the mount point
 * to see if it is really looks mounted.
 */

mounted(name)
	char *name;
{
	int found = 0;
	struct mnttab mnt;
	FILE *mnttab;
	struct stat device_stat, mount_stat;
	int status;
	char *blkname, *unrawname();
	static char buf[MAXPATHLEN];

		

	if ((mnttab = fopen(MNTTAB, "r")) == NULL) {
		myfprintf(stderr, MM_ERROR, ":313:cannot open %s\n", MNTTAB);
		return (0);
	}

	(void) strcpy(buf, name);
	blkname = unrawname(buf);
	while ((getmntent(mnttab, &mnt)) == NULL) {
		if (strcmp(mnt.mnt_fstype, MNTTYPE_SFS) != 0) {
			continue;
		}
		if (strcmp(blkname, mnt.mnt_special) == 0) {
			stat(mnt.mnt_mountp, &mount_stat);
			stat(mnt.mnt_special, &device_stat);
			if (device_stat.st_rdev == mount_stat.st_dev) {
				if (hasmntopt (&mnt, MNTOPT_RO) != 0)
					found = 2;	/* mounted as RO */
				else	
					found = 1; 	/* mounted as R/W */
			}
			break;
		}
	}
	fclose(mnttab);
	return (found);
}


/*
 * Procedure:     writable
 *
 * Restrictions:
 *                fopen: P_MACREAD, only if the MAC level of the process
 *			 is installed.
 *                printf: none
 *                getvfsent: none
 *                fclose: none
 * Notes:
 *
 * Check to see if name corresponds to an entry in vfstab, and that the entry
 * does not have option ro.
 */

writable(name)
	char *name;
{
	int rw = 1;
	struct vfstab vfsbuf;
	FILE *vfstab;

	if ((vfstab = fopen(VFSTAB, "r")) == NULL) {
		myfprintf(stderr, MM_ERROR, ":313:cannot open %s\n", VFSTAB);
		return (1);
	}

	while ((getvfsent(vfstab, &vfsbuf)) == NULL) {
		if (vfsbuf.vfs_fstype &&
			strcmp(vfsbuf.vfs_fstype, MNTTYPE_SFS) != 0) {
			if ((vfsbuf.vfs_special &&
				strcmp(name, vfsbuf.vfs_special) == 0) ||
			    (vfsbuf.vfs_mountp &&
				strcmp(name, vfsbuf.vfs_mountp) == 0)) {
				myfprintf(stderr, MM_ERROR, ":314:%s is nfs mounted - ignored\n", name);
				rw = 0;
			}
			continue;
		}
		if ((strcmp(name, vfsbuf.vfs_special) == 0) ||
		    (strcmp(name, vfsbuf.vfs_mountp) == 0)) {
			if (hasvfsopt(&vfsbuf, MNTOPT_RO)) {
				rw = 0;
			}
			break;
		}
	}
	fclose(vfstab);
	return (rw);
}


/*
 * Procedure:     xmalloc
 *
 * Restrictions:  none
 */

char *
xmalloc(size)
	int size;
{
	char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		errexit(":315:ran out of memory!\n");
	}
	return (ret);
}


/*
 * Procedure:     mntdup
 *
 * Restrictions:	none
*/

struct mnttab *
mntdup(mnt)
	struct mnttab *mnt;
{
	struct mnttab *new;

	new = (struct mnttab *)xmalloc(sizeof(*new));

	new->mnt_special = (char *)xmalloc(strlen(mnt->mnt_special) + 1);
	strcpy(new->mnt_special, mnt->mnt_special);

	new->mnt_mountp = (char *)xmalloc(strlen(mnt->mnt_mountp) + 1);
	strcpy(new->mnt_mountp, mnt->mnt_mountp);

	new->mnt_fstype = (char *)xmalloc(strlen(mnt->mnt_fstype) + 1);
	strcpy(new->mnt_fstype, mnt->mnt_fstype);

	new->mnt_mntopts = (char *)xmalloc(strlen(mnt->mnt_mntopts) + 1);
	strcpy(new->mnt_mntopts, mnt->mnt_mntopts);

	return (new);
}


/*
 * Procedure:     numbers
 *
 * Restrictions:	none
 *
 * Notes:         see if all numbers 
 */

numbers(yp)
	char	*yp;
{
	if (yp == NULL)
		return	0;
	while ('0' <= *yp && *yp <= '9')
		yp++;
	if (*yp)
		return	0;
	return	1;
}

/*
 * Procedure:     Pprintf
 *
 * Restrictions:	none
 * Pprintf is like printf when not preening,
 * or a warning (preceded by filename) when preening.
 */
/* VARARGS1 */
Pprintf(s, a1, a2, a3, a4, a5, a6, a7, a8)
char *s;
{
	long flags = 0;

	if(*s == '\n')
		flags = MM_NOGET;
	if (Pflag && Parent_notified == B_FALSE) {
		pfmt(print_fp, MM_WARNING|MM_NOGET, "%s: ", devname);
		pfmt(print_fp, MM_NOSTD|flags, s, a1, a2, a3, a4, a5, a6, a7, a8);
		fflush(print_fp);
	} else if ((Pflag ||preen) && !bflag) {
		pfmt(stdout, MM_WARNING|MM_NOGET, "%s: ", devname);
		pfmt(stdout, MM_NOSTD|flags, s,a1, a2, a3, a4, a5, a6, a7, a8);
	} else
		pfmt(stdout, MM_NOSTD|flags, s, a1, a2, a3, a4, a5, a6, a7, a8);
}

/*
 * Procedure:     Sprintf
 *
 * Restrictions:	none
 * Sprintf is like printf when not preening,
 * or a warning (preceded by filename) when preening.
 */
/* VARARGS1 */
Sprintf(s, a1, a2, a3, a4, a5, a6, a7, a8)
char *s;
{
	long flags = 0;

	if(*s == '\n')
		flags = MM_NOGET;
	if (Pflag && Parent_notified == B_FALSE) {
		pfmt(print_fp, MM_WARNING|flags, s, a1, a2, a3, a4, a5, a6, a7, a8);
		fflush(print_fp);
	} else if ((Pflag||preen)  && !bflag) {
		pfmt(stdout, MM_WARNING|flags, s, a1, a2, a3, a4, a5, a6, a7, a8);
	} else
		pfmt(stdout, MM_NOSTD|flags, s, a1, a2, a3, a4, a5, a6, a7, a8);
}

/*
 * Procedure:     myprintf
 *
 * Restrictions:	none
 *
 * Notes:         derivered for printf 
 */
myprintf(s, a1, a2, a3, a4, a5, a6, a7, a8)
char	*s;
{
	char	*tmpbuf;

	getsem();
	Pprintf(s, a1, a2, a3, a4, a5, a6, a7, a8);
	relsem();
}

/*
 * Procedure:     numbers
 *
 * Restrictions:	none
 *
 * Notes:         get semaphore id. 
 */
int getsemid()
{
	key_t	fsck_key;
	char	fsck_file[80];
	int	semid;
	
	sprintf(fsck_file,"%s.%d", FSCK_FILE, getppid());
	if ((fsck_key = ftok(fsck_file, FSCK_ID)) < 0) {
		pfmt(stderr, MM_ERROR, ":354:Cannot get the semaphore key =%d\n",errno);
		exit(1);
	}
	if ((semid = semget(fsck_key, 1, 0)) < 0) {
		pfmt(stderr, MM_ERROR, ":355:Cannot get the semaphore id \n");
		exit(1);
	}
	sembuf[0].sem_num =0;	
	sembuf[0].sem_flg =0;	
	return(semid);
}

/*
 * Procedure:     numbers
 *
 * Restrictions:	none
 *
 * Notes:         get/release semaphore. 
 */
getrelsem(getsem)
int getsem;
{
	struct sembuf	*sembufp=&sembuf[0];

	if (!Pflag || Lflag)
		return;
	if (getsem == 1) {
		sembuf[0].sem_op = -1; 
		if (semop(semid, sembufp, 1) < 0) {
			pfmt(stderr, MM_ERROR, ":356:Cannot get the semaphore\n");
			exit(1);
		}
	 } else {
		sembuf[0].sem_op = 1; 
		if (semop(semid, sembufp, 1) < 0) {
			pfmt(stderr, MM_ERROR, ":357:Cannot release the semaphore");
			exit(1);
		}
	} 
}

/*
 * Procedure:     numbers
 *
 * Restrictions:	none
 *
 * Notes:         
 */
myfprintf(file, flags, s, a1, a2, a3, a4, a5, a6, a7)
FILE	*file;
long	flags;
char	*s;
{
	getsem();
	if (Pflag && Parent_notified == B_FALSE) {
		pfmt(print_fp, flags|MM_NOGET, "%s:", devname);
		pfmt(print_fp, MM_NOSTD, s, a1, a2, a3, a4, a5, a6, a7);
		fflush(print_fp);
	} else if (Pflag && !bflg) {
		pfmt(file, flags|MM_NOGET, "%s:", devname);
		pfmt(file, MM_NOSTD, s, a1, a2, a3, a4, a5, a6, a7);
	} else
		pfmt(file, flags, s, a1, a2, a3, a4, a5, a6, a7);
	relsem();
}

/*
 * Procedure:     numbers
 *
 * Restrictions:	none
 *
 * Notes: 	get inode block of 64k (512 inodes);        
 */
int
getbigblk(blk)
	daddr_t	blk;
{
	register struct filecntl *fcp;
	daddr_t dblk;
	int cnt;

	fcp = &dfile;
	dblk = fsbtodb(&sblock, blk);

	if (inobuf.bb_blk != -1 &&  inobuf.bb_blk <= dblk
	&& dblk < (inobuf.bb_blk + inobuf.bb_dbpbuf)) 
		return 0;
	if (inobuf.bb_blk != -1 )
		bigflush();

	inobuf.bb_blk = dblk;

	return(bread(fcp, inobuf.bb_buf, dblk, inobuf.bb_size));
}

/*
 * Procedure:     numbers
 *
 * Restrictions:	none
 *
 * Notes:  	flush a block of 8k/4k of 64k inode which is dirty.	
 */
bigflush()
{
	struct filecntl *fcp;
	register int cnt;
	daddr_t	blkno;
	char	*tbufp;
	char	*basep = (char *)inobuf.bb_buf;

	fcp = &dfile;
	for (cnt = 0; cnt <  dirtycnt; cnt++) {
		if (*(inobuf.bb_dirty + cnt) == 1) {	/* DIRTY */
			blkno = inobuf.bb_blk + (cnt * blkcnt);
			tbufp = basep + (cnt * sblock.fs_bsize);
			bwrite(fcp, tbufp, blkno, sblock.fs_bsize);
			*(inobuf.bb_dirty + cnt) = 0;
		}
	}
}

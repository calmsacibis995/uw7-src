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
#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/pass1.c	1.4.8.11"

/*  "errexit()" and "pfatal()" have been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "pwarn()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
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

static daddr_t badblk;
static daddr_t dupblk;
int pass1check();

pass1()
{
	register int c, i, j;
	register DINODE *dp;
	struct zlncnt *zlnp;
	int ndb, ondb, partial, cgd;
	struct inodesc idesc;
	ino_t inumber;
	int error = 0;

	/*
	 * Set file system reserved blocks in used block map.
	 */
	for (c = 0; c < sblock.fs_ncg; c++) {
		cgd = cgdmin(&sblock, c);
		if (c == 0) {
			i = cgbase(&sblock, c);
			cgd += howmany(sblock.fs_cssize, sblock.fs_fsize);
		} else
			i = cgsblock(&sblock, c);
		for (; i < cgd; i++) {
			setbmap(i);
		}
	}
	/*
	 * Find all allocated blocks.
	 */
	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1check;
	inumber = 0;
	n_files = n_blks = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {

		/* initialize to the beginning of a cylinder group */
		inobuf.bb_strino = -1;
		for (i = 0; i < sblock.fs_ipg; i++, inumber++) {
			if (inumber < SFSROOTINO)
				continue;
			dp = ginode(inumber);
			if (!ALLOC(dp)) {
				if (memcmp((char *)dp->di_db, (char *)zino.di_db,
					NDADDR * sizeof(daddr_t)) ||
				    memcmp((char *)dp->di_ib, (char *)zino.di_ib,
					NIADDR * sizeof(daddr_t)) ||
				    dp->di_mode || dp->di_size) {
					WYPFLG(wflg, yflag, preen);
					getsem(); 
					pfatal(":238:PARTIALLY ALLOCATED INODE I=%u\n", inumber);
					if (reply(1, gettxt(":32","CLEAR")) == 1) {
						zapino(dp);
						INODIRTY(inumber);
						error++;
					}
					relsem();
				} else { 
					if (inumber == SFSROOTINO) {
						dp->di_mode = IFDIR;
						INODIRTY(SFSROOTINO);
						error++;
						setstate(SFSROOTINO, DSTATE);
						goto skip;
					}
				}
				setstate(inumber, USTATE);
				continue;
			}
skip:
			lastino = inumber;
			if (dp->di_size < 0 ||
			    dp->di_size + sblock.fs_bsize - 1 < 0) {
				if (debug)
					myfprintf(stdout, MM_ERROR, ":359:Bad file size %d:", dp->di_size);
				goto unknown;
			}
			if (!preen && (dp->di_mode & IFMT) == IFMT) {
				WYPFLG(wflg, yflag, preen);
				getsem();
				if (reply(1, gettxt(":240","BAD MODE: MAKE IT A FILE")) == 1) {
					dp->di_size = sblock.fs_fsize;
					dp->di_mode = IFREG|0600;
					dp->di_smode = dp->di_mode;
					INODIRTY(inumber);
					error++;
				}
				relsem();
			}
			ondb = ndb = howmany(dp->di_size, sblock.fs_bsize);
			if (SPECIAL(dp)) {
				ndb++;
				if (dp->di_eftflag == EFT_MAGIC)
					ndb++;
			}					
			/* check direct blocks */
			for (j = ndb; j < NDADDR; j++) {
				if (dp->di_db[j] != 0) {
					if (debug)
						myfprintf(stdout, MM_ERROR, ":241:bad direct addr: %d\n",
							dp->di_db[j]);
					goto unknown;
				}
			}
			if (ndb > NDADDR) {
				for (j = 0, ndb -= NDADDR; ndb > 0; j++)
					ndb /= NINDIR(&sblock);
				for (; j < NIADDR; j++) {
					if (dp->di_ib[j] != 0) {
						if (debug)
							myfprintf(stdout, MM_ERROR, ":242:bad indirect addr: %d\n",
								dp->di_ib[j]);
						goto unknown;
					}
				}
			}
			if (ftypeok(dp) == 0)
				goto unknown;
			n_files++;
			lncntp[inumber] = dp->di_nlink;
			if (dp->di_nlink <= 0) {
				zlnp = (struct zlncnt *)malloc(sizeof *zlnp);
				if (zlnp == NULL) {
					WYPFLG(wflg, yflag, preen);
					getsem();
					pfatal(":243:LINK COUNT TABLE OVERFLOW\n");
					if (reply(1, gettxt(":35","CONTINUE")) == 0) {
						relsem();
						errexit("");
					}
					relsem();
				} else {
					zlnp->zlncnt = inumber;
					zlnp->next = zlnhead;
					zlnhead = zlnp;
				}
			}
			setstate(inumber, DIRCT(dp) ? DSTATE : FSTATE);
			badblk = dupblk = 0; maxblk = 0;
			idesc.id_number = inumber;

			/* construct dirmap */
			if (MEM && DIRCT(dp))
				creatdirmap(inumber, dp->di_size, ondb);
			(void)ckinode(dp, &idesc, 1, 1, 0, 0);
			idesc.id_entryno *= btodb(sblock.fs_fsize);
			if (dp->di_blocks != idesc.id_entryno) {
				WYPFLG(wflg, yflag, preen);
				if (preen) {
					myprintf(":360:%s: INCORRECT BLOCK COUNT I=%u (%ld should be %ld) (CORRECTED)\n", devname, inumber, dp->di_blocks, idesc.id_entryno);
				} else {
					myprintf(":244:INCORRECT BLOCK COUNT I=%u (%ld should be %ld)\n", inumber, dp->di_blocks, idesc.id_entryno);
					if (reply(1, gettxt(":246","CORRECT")) == 0)
						continue;
				}
				dp->di_blocks = idesc.id_entryno;
				INODIRTY(inumber);
				error++;
				
			}
			continue;
	unknown:
			WYPFLG(wflg, yflag, preen);
			setstate(inumber, FCLEAR);
			getsem(); 
			pfatal(":361:UNKNOWN FILE TYPE OR BAD SIZE I=%u\n", inumber);
			if (reply(1, gettxt(":32","CLEAR")) == 1) {
				setstate(inumber, USTATE);
				dp->di_mode = 0;
				zapino(dp);
				INODIRTY(inumber);
				error++;
			}
			relsem();
		}
	}
	if (error)
		bigflush();
}

/*
 * Create dirmap for each directory entry
 * ckinode() and iblock() will record data block numbers for each dirmap.
 */
void
creatdirmap(inumber, size, ndb)
ino_t	inumber;
long	size;
int	ndb;
{
	struct dirmap *tdirmapp;

	/* construct dirmap for a directory inode */
	tdirmapp = (struct dirmap *)malloc(sizeof (struct dirmap));
	if (tdirmapp == NULL)
		errexit(":351:cannot alloc dirmap\n");
	nstatemap(inumber).dir_p = tdirmapp;
	tdirmapp->inolist = NULL;
	tdirmapp->dot = tdirmapp->dotdot = 0; 
	tdirmapp->filesize = size;
	tdirmapp->dblist = (daddr_t *)calloc(ndb, sizeof(daddr_t));
	if (tdirmapp->dblist == NULL)
		errexit(":352:cannot alloc dblist\n");
}

pass1check(idesc)
	register struct inodesc *idesc;
{
	int res = KEEPON;
	int anyout, nfrags;
	daddr_t blkno = idesc->id_blkno;
	register struct dups *dlp;
	struct dups *new;

	if ((anyout = outrange(blkno, idesc->id_numfrags)) != 0) {
		blkerr(idesc->id_number, gettxt(":75","BAD"), blkno);
		if (++badblk >= MAXBAD) {
			WYPFLG(wflg, yflag, preen);
			getsem();
			pwarn(":248:EXCESSIVE BAD BLKS I=%u\n",idesc->id_number);
			if (preen)
				Pprintf(":249: (SKIPPING)\n");
			else if (reply(1, gettxt(":35","CONTINUE")) == 0) {
				Pprintf("");
				relsem();
				exit(39);
			}
			relsem();
			return (STOP);
		}
	}
	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (anyout && outrange(blkno, 1)) {
			res = SKIP;
		} else if (!getbmap(blkno)) {
			n_blks++;
			setbmap(blkno);
		} else {
			blkerr(idesc->id_number, gettxt(":77","DUP"), blkno);
			if (++dupblk >= MAXDUP) {
				WYPFLG(wflg, yflag, preen);
				getsem();
				pwarn(":250:EXCESSIVE DUP BLKS I=%u\n", idesc->id_number);
				if (preen)
					Pprintf(":249: (SKIPPING)\n");
				else if (reply(1, gettxt(":35","CONTINUE")) == 0) {
					relsem();
					errexit("");
				}
				relsem();
				return (STOP);
			}
			new = (struct dups *)malloc(sizeof(struct dups));
			if (new == NULL) {
				WYPFLG(wflg, yflag, preen);
				getsem();
				pfatal(":251:DUP TABLE OVERFLOW.\n");
				if (reply(1, gettxt(":35","CONTINUE")) == 0) {
					relsem();
					errexit("");
				}
				relsem();
				return (STOP);
			}
			new->dup = blkno;
			if (muldup == 0) {
				duplist = muldup = new;
				new->next = 0;
			} else {
				new->next = muldup->next;
				muldup->next = new;
			}
			for (dlp = duplist; dlp != muldup; dlp = dlp->next)
				if (dlp->dup == blkno)
					break;
			if (dlp == muldup && dlp->dup != blkno)
				muldup = new;
		}
		/*
		 * count the number of blocks found in id_entryno
		 */
		idesc->id_entryno++;
	}
	return (res);
}

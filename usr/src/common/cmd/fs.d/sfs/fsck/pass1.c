/*	copyright	"%c%	*/

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

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/pass1.c	1.3.6.9"

/*  "errexit()", "pfatal()", "pwarn()", "Pprintf()", "mypfatal()"
 *  and "myfprintf()" have been internationalized.
 *  The string to be output must at least include the message number
 *  and optionally a catalog name.
 *
 *  "errexit()" and "pfatal()" output using <MM_ERROR>.
 *  "pwarn()" outputs using <MM_WARNING>.
 *  "myfprintf()" requires the output severity as an argument.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <acl.h>
#include <sys/sysmacros.h>
#include <sys/mntent.h>
#include <sys/fs/sfs_fs.h>
#include <sys/vnode.h>
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
int pass1sechk();

/*
 * Procedure:     pass1
 *
 * Restrictions:
 *                printf: none
*/

pass1()
{
	register int c, i, j;
	register DINODE *dp;
	struct zlncnt *zlnp;
	int ndb, ondb, partial, cgd;
	struct inodesc idesc;
	ino_t inumber;
	struct dirmap	*tdirmapp;
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
		for (; i < cgd; i++)
			setbmap(i);
	}
	/*
	 * Find all allocated blocks.
	 */
	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1check;
	idesc.id_secfunc = pass1sechk;
	inumber = 0;
	n_files = n_blks = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		inobuf.bb_strino = -1; 
		/*
                 * Make sure to skip alternate inodes
                 */
                for (i = 0; i < sblock.fs_ipg;
                                i += NIPFILE, inumber += NIPFILE) {
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
					pfatal(":238:PARTIALLY ALLOCATED INODE I=%u\n",
						inumber);
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
					myfprintf(stdout, MM_ERROR, ":239:bad size %d:", dp->di_size);
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
			for (j = ndb; j < NDADDR; j++)
				if (dp->di_db[j] != 0) {
					if (debug)
						myfprintf(stdout, MM_ERROR, ":241:bad direct addr: %d\n",
							dp->di_db[j]);
					goto unknown;
				}
			for (j = 0, ndb -= NDADDR; ndb > 0; j++)
				ndb /= NINDIR(&sblock);
			for (; j < NIADDR; j++)
				if (dp->di_ib[j] != 0) {
					if (debug)
						myfprintf(stdout, MM_ERROR, ":242:bad indirect addr: %d\n",
							dp->di_ib[j]);
					goto unknown;
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
					if (reply(1, gettxt(":35","CONTINUE")) == 0){
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

			/* construct dirmap and recorded data block numbers */
			if (MEM && DIRCT(dp))
				creatdirmap(inumber, dp->di_size, ondb);
			(void)ckinode(dp, &idesc, 1, 1, 0, 0);
			idesc.id_entryno *= btodb(sblock.fs_fsize);
			if (dp->di_blocks != idesc.id_entryno) {
				WYPFLG(wflg, yflag, preen);
				getsem();
				pwarn(":244:INCORRECT BLOCK COUNT I=%u (%ld should be %ld)\n",
				    inumber, dp->di_blocks, idesc.id_entryno);
				if (preen)
					Pprintf(":245: (CORRECTED)\n");
				else if (reply(1, gettxt(":246","CORRECT")) == 0) {
					relsem();
					continue;
				}
				relsem();
				dp->di_blocks = idesc.id_entryno;
				INODIRTY(inumber);
				error++;
			}
			continue;
	unknown:
			WYPFLG(wflg, yflag, preen);
			getsem();
			pfatal(":247:UNKNOWN FILE TYPE I=%u\n", inumber);
			setstate(inumber, FCLEAR);
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
 * Procedure:     createdirmap
 *
 * Restrictions:
 * Note:  Create dirmap for each directory entry
 * 	ckinode() and iblock() will record data block numbers for each dirmap.
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


/*
 * Procedure:     pass1check
 *
 * Restrictions:
 *               printf: none
*/

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
			pwarn(":248:EXCESSIVE BAD BLKS I=%u\n",
				idesc->id_number);
			if (preen)
				Pprintf(":249: (SKIPPING)\n");
			else if (reply(1, gettxt(":35","CONTINUE")) == 0) {
				relsem();
				errexit("");
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
			WYPFLG(wflg, yflag, preen);
			blkerr(idesc->id_number, gettxt(":77","DUP"), blkno);
			if (++dupblk >= MAXDUP) {
				getsem();
				pwarn(":250:EXCESSIVE DUP BLKS I=%u\n",
					idesc->id_number);
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
				if (reply(1, gettxt(":35","CONTINUE")) == 0){
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


/*
 * Procedure:     pass1sechk-perform security consistency check on secure inode 
 *
 * Restrictions:  none
 *
 * Notes:
 *      1. Only ACL consistency check need to be performed.
 *      2. A security inode state map is used to record all inodes
 *         which have invalid/bad ACLs or duplicate ACL blocks.
 *      3. Whenever a bad ACL is encountered, code resumes at the
 *         recover goto tag where the secure inode state map is updated.
 *      4. The ACL blocks are first stored in an ACL block array, and
 *         only after it has been determined that the ACL is valid,
 *         is the regular inode block map updated.
 *      5. Duplicate blocks (ACL and file pointing to same data block)
 *         are also first stored in a duplicate ACL block array.
 *         After the ACL has been determined to be valid, the
 *         recorded duplicates are then added to the regular
 *         duplicate list.  Recording duplicates only after the ACL
 *         has been determined to be valid, guarantees that there can be
 *         no ACL blocks referencing the same data block.
 *         Note that the secure inode state map is also updated on
 *         duplicate blocks.
 *      6. The working inode ACL block arrays start off with NACLBLKS
 *         size.  If there are more ACL blocks for an inode, these
 *         arrays are expanded when necessary.
 *      7. Return is KEEPON on success, SKIP on failure; this routine
 *         is called from ckinode().
 */

#define NACLBLKS        10      /* initial guess of number of ACL blocks */

pass1sechk(dp, ino)
        register DINODE *dp;
        ino_t ino;
{
        register struct acl *aclp;      /* current ACL entry ptr */
        register int acleft;            /* number of ACL entries left */
        register int i;                 /* loop counter */
        struct aclhdr *ahdrp;           /* ACL block header ptr */
        daddr_t blk;                    /* next ACL block */
        int bsize;                      /* size of ACL block to read */
        int size;                       /* size of ACL block */
        int frags;                      /* number of frags in ACL block */
        int cnt;                        /* number of entries to check at a time */
        int dftcnt;                     /* number of defalut entries */
        int group_obj = 0;
        int defaultusers = 0;
        int defaultgroups = 0;
        int defaultclass = 0;
        int defaultother = 0;
        int nusedaclblks = 0;           /* number of ACL blocks used so far */
        int nuseddupblks = 0;           /* number of duplicate ACL blocks so far */
        struct dups *dlp;               /* handle duplicate blocks */
        struct dups *new;
        daddr_t *tmpp;                  /* temp ptr when expanding arrays */        /* parameters to hold ACL block information for the working inode */
        static daddr_t allocaclblk[NACLBLKS];
        static daddr_t *aclblkp = &allocaclblk[0];
        static int naclblks = NACLBLKS;
        /* parameters to hold dup ACL block information for the working inode */
        static daddr_t allocdupblk[NACLBLKS];
        static daddr_t *dupblkp = &allocdupblk[0];
        static int ndupblks = NACLBLKS;


        acleft = dp->di_aclcnt;
        cnt = (acleft > NACLI) ? NACLI : dp->di_aclcnt;
        blk = dp->di_aclblk;
        aclp = &dp->di_acl[0];
        dftcnt = dp->di_daclcnt;

        /*
         * In this implementation, the object owner and object other are
         * represented by the uid and permission mode bits.
         * The object owning group is represented  by the gid and permission
         * mode bits if the acl count (excluding defaults) is zero.
         */
        if (dftcnt > acleft)
                goto recover;
        if (dftcnt == acleft)
                group_obj++;
        if (acleft == 0)
                return(KEEPON);

        while (cnt) {
                for (i = cnt; i > 0; i--, aclp++, acleft--) {
                        switch (aclp->a_type) {
                        case USER:
                                if (group_obj)
                                        goto recover;
                                break;

                        case GROUP_OBJ:
                                if (group_obj)
                                        goto recover;
                                group_obj++;
                                break;

                        case GROUP:
                                if (!group_obj ||  defaultusers || defaultgroups 
					|| defaultclass || defaultother)
                                        goto recover;
                                break;

                        case DEF_USER_OBJ:
                                if (!group_obj ||  defaultusers || defaultgroups 
					|| defaultclass || defaultother)
                                        goto recover;
                                defaultusers++;
				break;

                        case DEF_USER:
                                if (!group_obj || defaultgroups || defaultclass
					|| defaultother)
                                        goto recover;
                                defaultusers++;
                                break;

                        case DEF_GROUP_OBJ:
                                if (!group_obj || defaultgroups || defaultclass
					|| defaultother)
                                        goto recover;
                                defaultgroups++;
                                break;

                        case DEF_GROUP:
                                if (!group_obj || defaultclass || defaultother)
                                        goto recover;
                                defaultgroups++;
                                break;

                        case DEF_CLASS_OBJ:
                                if (!group_obj || defaultclass || defaultother)
                                        goto recover;
                                defaultclass++;
                                break;

                        case DEF_OTHER_OBJ:
                                if (!group_obj || defaultother)
                                        goto recover;
                                defaultother++;
                                break;

                        default:
                                goto recover;
                        } /* end switch */
                } /* end for */

                /* fetch the next ACL block */
                if (blk) {
                        bsize = ACLBLKSIZ(acleft) < sblock.fs_bsize ?
                                ACLBLKSIZ(acleft) : sblock.fs_bsize;
                        getblk(&aclblk, blk, bsize);
                        ahdrp = (struct aclhdr *)&aclblk.b_un.b_buf[0];
                        cnt = ahdrp->a_size;
                        size = ACLBLKSIZ(cnt);
                        if ((ahdrp->a_ino != ino) || (cnt < 0)
                        ||  (acleft < cnt) || (size > bsize))
                                goto recover;
                        frags = numfrags(&sblock, size);
                        if (outrange(blk, frags) != 0) {
                                mypfatal(":252:%ld ACL BAD I=%u\n", blk, ino);
                                goto recover;
                        }
                        /* may need to enlarge ACL block array */
                        if ((nusedaclblks + frags) >= naclblks) {
                                naclblks *= 2;
                                tmpp = (daddr_t *)
                                        xmalloc(naclblks * sizeof(daddr_t));
                                memcpy((char *)tmpp, (char *)aclblkp,
                                        nusedaclblks * sizeof(daddr_t));                                free(aclblkp);
                                aclblkp = tmpp;
                        }
                        for (i = frags-1; i >= 0; i--) {
                                aclblkp[nusedaclblks++] = blk+i;
                                if (getbmap(blk+i) != 0) {
                                        /*
                                         * may need to enlarge duplicate ACL block
                                         * array.
                                         */
                                        if (nuseddupblks >= ndupblks) {
                                                ndupblks *= 2;
                                                tmpp = (daddr_t *)
                                                xmalloc(ndupblks * sizeof(daddr_t));
                                                memcpy((char *)tmpp,
                                                       (char *)dupblkp,
                                                      nuseddupblks*sizeof(daddr_t));
                                                free(dupblkp);
                                                dupblkp = tmpp;
                                        }
                                        dupblkp[nuseddupblks++] = blk+i;
                                }
                        }
                        blk = ahdrp->a_nxtblk;
                        aclp = (struct acl *)((int)ahdrp + sizeof(struct aclhdr));
                } else
                        cnt = 0;
        } /* end while */

        if (acleft == 0) {
                if ((dftcnt == (defaultusers + defaultgroups + defaultclass + 
			defaultother)) && group_obj) {
                        for (i = nusedaclblks - 1; i >= 0; i--)
                                setbmap(aclblkp[i]);
                        n_blks += nusedaclblks;
                        for (i = 0; i < nuseddupblks; i++) {
                                mypfatal(":253:%ld ACL DUP I=%u\n", dupblkp[i], ino);
                                new = (struct dups *)xmalloc(sizeof(struct dups));
                                new->dup = dupblkp[i];
                                if (muldup == 0) {
                                        duplist = muldup = new;
                                        new->next = 0;
                                } else {
                                        new->next = muldup->next;
                                        muldup->next = new;
                                }
                                for (dlp = duplist; dlp != muldup; dlp = dlp->next)
                                        if (dlp->dup == dupblkp[i])
                                                break;
                                if (dlp == muldup && dlp->dup != dupblkp[i])
                                        muldup = new;
                        }
                        /* update secure inode state map on dup blocks */
                        if (nuseddupblks != 0)
                                secstatemap[ino] |= SEC_DUPACL;                        	return(KEEPON);
                }
        }

recover:
        secstatemap[ino] |= SEC_BADACL;
        return(SKIP);
}

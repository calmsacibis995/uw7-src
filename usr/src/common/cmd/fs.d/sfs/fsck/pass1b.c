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
#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/pass1b.c	1.3.5.6"

/*  "pfatal()" and "mypfatal()" have been internationalized.
 *  The string to be output *  must at least include the message number
 *  and optionally a catalog name.
 *
 *  "pfatal()" outputs using <MM_ERROR>.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <acl.h>
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

int	pass1bcheck();
int	pass1bsechk();
static  struct dups *duphead;


/*
 * Procedure:     pass1b
 *
 * Restrictions:  none
*/

pass1b()
{
	register int c, i;
	register DINODE *dp;
	struct inodesc idesc;
	ino_t inumber;
	char state;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1bcheck;
	idesc.id_secfunc = pass1bsechk;
	duphead = duplist;
	inumber = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		inobuf.bb_strino = -1;
		for (i = 0; i < sblock.fs_ipg; 
			i += NIPFILE, inumber += NIPFILE) {
			if (inumber < SFSROOTINO)
				continue;
			dp = sginode(inumber);
			if (dp == NULL)
				continue;
			idesc.id_number = inumber;
			state = get_state(inumber);
			if (state != USTATE &&
			    (ckinode(dp, &idesc, 1, 0, 0, 0) & STOP))
				goto out1b;
		}
	}
out1b:
	if (MEM)
		bigflush();
	else
		flush(&dfile, &inoblk);
}

/* inline code for pass1b to scan duplicate list for a matching blkno */
#define SCANDUP1B(blkno, ino, sec) \
{ \
        register struct dups *dlp; \
        for (dlp = duphead; dlp; dlp = dlp->next) { \
                if (dlp->dup == blkno) { \
                        if (sec) { \
                                secstatemap[ino] |= SEC_DUPACL; \
                                mypfatal(":253:%ld ACL DUP I=%u\n", blkno, ino); \
                        } else \
                                blkerr(ino, gettxt(":77","DUP"), blkno); \
                        dlp->dup = duphead->dup; \
                        duphead->dup = blkno; \
                        duphead = duphead->next; \
                } \
                if (dlp == muldup) \
                        break; \
        } \
        if (muldup == 0 || duphead == muldup->next) \
                return (STOP); \
}

/* inline code for pass1b to scan duplicate list for a matching blkno */
#define SCANDUP1B(blkno, ino, sec) \
{ \
        register struct dups *dlp; \
        for (dlp = duphead; dlp; dlp = dlp->next) { \
                if (dlp->dup == blkno) { \
                        if (sec) { \
                                secstatemap[ino] |= SEC_DUPACL; \
                                mypfatal(":253:%ld ACL DUP I=%u\n",blkno, ino); \
                        } else \
                                blkerr(ino, gettxt(":77","DUP"), blkno); \
                        dlp->dup = duphead->dup; \
                        duphead->dup = blkno; \
                        duphead = duphead->next; \
                } \
                if (dlp == muldup) \
                        break; \
        } \
        if (muldup == 0 || duphead == muldup->next) \
                return (STOP); \
}


/*
 * Procedure:     pass1bcheck
 *
 * Restrictions:  none
*/

pass1bcheck(idesc)
	register struct inodesc *idesc;
{
	register struct dups *dlp;
	int nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;

	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (outrange(blkno, 1))
			res = SKIP;
		SCANDUP1B(blkno, idesc->id_number, 0);
	}
	return (res);
}


/*
 * Procedure:     pass1bsechk
 *
 * Restrictions:  none
 *
 * Notes:         pass1b security check
 *
 * 								Search for duplicate blocks by traversing the 
 *                ACL blocks for an inode.
 */

pass1bsechk(dp, ino)
        DINODE *dp;
        ino_t   ino;
{
        int res = KEEPON;
        daddr_t blkno;
        struct aclhdr *ahdrp;
        int cnt;
        int size;
        int i;

        /* don't bother checking if ACL for inode is bad */
        if ((secstatemap[ino] & (SEC_BADACL|SEC_DUPACL)) == 0) {
                for (blkno = dp->di_aclblk, cnt = dp->di_aclcnt - NACLI;
                     cnt > 0; blkno = ahdrp->a_nxtblk, cnt -= ahdrp->a_size) {
                        getblk(&aclblk, blkno, sblock.fs_fsize);
                        ahdrp = (struct aclhdr *)&aclblk.b_un.b_buf[0];
                        size = ACLBLKSIZ(ahdrp->a_size);
                        /*
                         * Since pass1 has validated the ACL, this
                         * check is not necessary.
                         */
                        if ((ahdrp->a_ino != ino) || (ahdrp->a_size <= 0)
                        ||  (cnt < ahdrp->a_size) || (size > sblock.fs_bsize)) {
                                res = SKIP;
                                break;
                        }
                        for (i = numfrags(&sblock, size) - 1; i >= 0; i--) {
                                SCANDUP1B(blkno+i, ino, 1);
                        }
                }
        }

        return(res);
}

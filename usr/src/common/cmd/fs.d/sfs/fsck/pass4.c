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

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/pass4.c	1.3.3.7"
/*  "errexit()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_ERROR>.
 *
 *  "clri()" has been internationalized. The string to be output
 *  must at least include the message number and optionally a catalog name.
 *  The string is output using <MM_WARNING>.
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

int	pass4check();
int	pass4sechk();


/*
 * Procedure:     pass4
 *
 * Restrictions:  none
*/

pass4()
{
	register ino_t inumber;
	register struct zlncnt *zlnp;
	struct inodesc idesc;
	int n;
	char state;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	/*
         * skip alternate inodes
         */
        for (inumber = SFSROOTINO; inumber <= lastino; inumber += NIPFILE) {
		idesc.id_number = inumber;
		state = get_state(inumber);
		switch (state) {

		case FSTATE:
		case DFOUND:
			n = lncntp[inumber];
			if (n) {
				WYPFLG(wflg, yflag, preen);
				adjust(&idesc, (short)n);
			} else {
				for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
					if (zlnp->zlncnt == inumber) {
						WYPFLG(wflag, yflag, preen);
						zlnp->zlncnt = zlnhead->zlncnt;
						zlnp = zlnhead;
						zlnhead = zlnhead->next;
						free(zlnp);
						clri(&idesc, ":46:UNREF", 1);

						break;
					}
			}
			break;
		
		case DCORRPT:
                        clri(&idesc, ":180:DIRECTORY CORRUPTED", 1);
                        break;

		case DSTATE:
			clri(&idesc, ":46:UNREF", 1);
			break;

		case DCLEAR:
		case FCLEAR:
			clri(&idesc, ":47:BAD/DUP", 1);
			break;

		case USTATE:
			break;

		default:
			errexit(":276:BAD STATE %d FOR INODE I=%d\n",
			    state, inumber);
		}
	}
}

/* inline code to remove blkno from duplicate list */
#define SCANDUP4(blkno) \
{ \
        register struct dups *dlp; \
        for (dlp = duplist; dlp; dlp = dlp->next) { \
                if (dlp->dup != blkno) \
                        continue; \
                dlp->dup = duplist->dup; \
                dlp = duplist; \
                duplist = duplist->next; \
                free(dlp); \
                break; \
        } \
        if (dlp == 0) { \
                clrbmap(blkno); \
                n_blks--; \
        } \
}


/*
 * Procedure:     pass4check
 *
 * Restrictions:
*/

pass4check(idesc)
	register struct inodesc *idesc;
{
	int nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;

        /* do SCANDUP4 for each block in file */
	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (outrange(blkno, 1)) {
			res = SKIP;
		} else if (getbmap(blkno)) {
                        SCANDUP4(blkno);
		}
	}
	return (res);
}


/*
 * Procedure:     pass4sechk
 *
 * Restrictions:  none
 * 
 *
 * Notes:         pass4 security check 
 *
 *      1. if a file's ACL is invalid, neither the inode block map
 *         nor the duplicate block list have been updated; no
 *         action is necessary.
 */

pass4sechk(dp, ino)
        DINODE *dp;
        ino_t ino;
{
        register daddr_t blkno;
        register struct aclhdr *ahdrp;
        int res = KEEPON;
        int cnt;
        int size;
        int i;

        if ((secstatemap[ino] & SEC_BADACL) == 0) {
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
                                if (secstatemap[ino] & SEC_DUPACL) {
                                        SCANDUP4(blkno+i);
                                } else {
                                        clrbmap(blkno+i);
                                        n_blks--;
                                }
                        } /* end frags for loop */
                } /* end traversing ACL blocks for loop */
        }

        return (res);
}

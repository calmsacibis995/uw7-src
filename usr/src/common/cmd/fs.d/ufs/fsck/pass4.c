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
#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/fsck/pass4.c	1.2.8.6"

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

int	pass4check();

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
	for (inumber = SFSROOTINO; inumber <= lastino; inumber++) {
		idesc.id_number = inumber;
		state = get_state(inumber);

		switch (state) {

		case FSTATE:
		case DFOUND:
			n = lncntp[inumber];
			if (n) {
				WYPFLG(wflag, yflag, preen);
				adjust(&idesc, (short)n);
			} else {  /* if inode is recorded in znlhead -> clear */
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

pass4check(idesc)
	register struct inodesc *idesc;
{
	register struct dups *dlp;
	int nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;

	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (outrange(blkno, 1)) {
			res = SKIP;
		} else if (getbmap(blkno)) {
			for (dlp = duplist; dlp; dlp = dlp->next) {
				if (dlp->dup != blkno)
					continue;
				dlp->dup = duplist->dup;
				dlp = duplist;
				duplist = duplist->next;
				free(dlp);
				break;
			}
			if (dlp == 0) {
				clrbmap(blkno);
				n_blks--;
			}
		}
	}
	return (res);
}

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

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/pass3.c	1.3.3.9"
#ident "$Header$"

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

int	pass2check();

/*
 * Procedure:     pass3
 *
 * Restrictions:  none
*/

pass3()
{
	register DINODE *dp;
	struct inodesc idesc;
	ino_t inumber, orphan;
	struct	dirmap	*dirp;
	char state;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	/*
         *      skip alternate inodes
         */
        for (inumber = SFSROOTINO; inumber <= lastino; inumber += NIPFILE) {
		state = get_state(inumber);
		if (state == DSTATE) {
		    if (MEM) {
			WYPFLG(wflg, yflag, preen);
			orphan = inumber;
			dirp = nstatemap(inumber).dir_p;
			idesc.id_parent = dirp->dotdot;	
			if (lncntp[inumber] <= 0)
				continue;
			if (dirp->filesize == 0) {
				continue;
			}
			do {
				if (nstatemap(dirp->dotdot).flag == DFOUND) 
					break;
				if (nstatemap(dirp->dotdot).flag == DCLEAR) 
					break;
				if (nstatemap(dirp->dotdot).flag == DCORRPT) 
					break;
				inumber = dirp->dotdot;
				dirp = nstatemap(inumber).dir_p;
				if (dirp == NULL)
					break;
			} while (nstatemap(dirp->dotdot).flag != DFOUND);
			if (linkup(orphan, dirp->dotdot) == 1) {
				chkdirsiz_descend(NULL, orphan);
				if (nstatemap(orphan).flag == DSTATE) {
					idesc.id_number = orphan;
					idesc.id_fix = DONTKNOW;
					idesc.id_func = 0;
					idesc.id_filesize = dirp->filesize;
					idesc.id_loc = idesc.id_entryno = 0;
					inmem_readdir(dirp, 1, &idesc, 0, 0, 0, 1);
					traverse(orphan, lfdir, lfdir);
				}

			}
			inumber = orphan;
		    } else {
			int loopcnt;

			pathp = pathname;
			*pathp++ = '?';
			*pathp = '\0';
			idesc.id_func = findino;
			idesc.id_secfunc = 0;
			idesc.id_name = "..";
			idesc.id_parent = inumber;
			loopcnt = 0;
			do {
				orphan = idesc.id_parent;
				if (orphan < SFSROOTINO || orphan > imax)
					break;
				dp = sginode(orphan);
				idesc.id_parent = 0;
				idesc.id_number = orphan;
				(void)ckinode(dp, &idesc, 0 ,0,0,0);
				if (idesc.id_parent == 0)
					break;
				if (loopcnt >= sblock.fs_cstotal.cs_ndir)
					break;
				loopcnt++;
			} while (ostatemap(idesc.id_parent) == DSTATE);
			if (linkup(orphan, idesc.id_parent) == 1) {
				idesc.id_func = pass2check;
				idesc.id_secfunc = 0;
				idesc.id_number = lfdir;
				chkdirsiz_descend(&idesc, orphan);
			}
		    }
	    	}
		
	}
}

 /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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
#ident  "@(#)sfs.cmds:common/cmd/fs.d/sfs/fsck/sfs_subr.c	1.1.3.4"
#ident "$Header$"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/fs/sfs_fs.h>
#include <sys/fs/sfs_tables.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>

/*
 * Procedure:     sfs_fragacct
 *
 * Restrictions:	none
 *
 * Notes:
 *
 * Update the frsum fields to reflect addition or deletion
 * of some frags.
 */

void
sfs_fragacct(fs, fragmap, fraglist, cnt)
	struct fs *fs;
	int fragmap;
	long fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(sfs_fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field = sfs_around[siz];
		subfield = sfs_inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
	return;
}


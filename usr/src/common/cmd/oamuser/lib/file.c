#ident	"@(#)file.c	1.2"
#ident  "$Header$"

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/mac.h>
#include	<sys/errno.h>

int
check_perm( statbuf, uid, gid, perm )
	struct stat statbuf;
	uid_t uid;
	gid_t gid;
	mode_t perm;
{
	int fail = -1;	/* assume no permission at onset */

	/* Make sure we're dealing with a directory */
	if( S_ISDIR( statbuf.st_mode )) {
		/*
		 * Have a directory, so make sure user has permission
		 * by the various possible methods to this directory.
		 */
		if( (statbuf.st_uid == uid) &&
		    (statbuf.st_mode & (perm << 6)) == (perm << 6) )
			fail = 0;
		else
		if( (statbuf.st_gid == gid) &&
		    (statbuf.st_mode & (perm << 3)) == (perm << 3) )
			fail = 0;
		else
		if( (statbuf.st_mode & perm) == perm )
			fail = 0;
	}

	return fail;
}



/*
 * Procedure:	ck_and_set_flevel
 *
 * Notes:	determines if a level should be assigned to a file,
 *		and if so, what LID it should be.
*/
int
ck_and_set_flevel(stat_lid, fname)
	level_t	stat_lid;
	char	*fname;
{
	extern	int	errno;

	if (stat_lid) {
		if (lvlfile(fname, MAC_SET, &stat_lid) < 0)
			return errno;
	}
	return 0;
}

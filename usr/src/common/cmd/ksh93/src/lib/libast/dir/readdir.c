#ident	"@(#)ksh93:src/lib/libast/dir/readdir.c	1.1"
#pragma prototyped
/*
 * readdir
 *
 * read from directory stream
 *
 * NOTE: directory entries must fit within DIRBLKSIZ boundaries
 */

#include "dirlib.h"

#if _dir_ok

NoN(readdir)

#else

struct dirent*
readdir(register DIR* dirp)
{
	register struct dirent*	dp;

	for (;;)
	{
		if (dirp->dd_loc >= dirp->dd_size)
		{
			if (dirp->dd_size < 0) return(0);
			dirp->dd_loc = 0;
			if ((dirp->dd_size = getdents(dirp->dd_fd, dirp->dd_buf, DIRBLKSIZ)) <= 0)
				return(0);
		}
		dp = (struct dirent*)&dirp->dd_buf[dirp->dd_loc];
		if (dp->d_reclen <= 0) return(0);
		dirp->dd_loc += dp->d_reclen;
		if (dp->d_fileno) return(dp);
	}
	/*NOTREACHED*/
}

#endif

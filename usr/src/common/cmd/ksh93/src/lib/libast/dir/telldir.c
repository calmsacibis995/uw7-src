#ident	"@(#)ksh93:src/lib/libast/dir/telldir.c	1.1"
#pragma prototyped
/*
 * telldir
 *
 * get directory stream pointer offset for seekdir()
 */

#include "dirlib.h"

#if _dir_ok

NoN(telldir)

#else

long
telldir(DIR* dirp)
{
	return(lseek(dirp->dd_fd, 0L, SEEK_CUR) + (long)dirp->dd_loc);
}

#endif

#ident	"@(#)ksh93:src/lib/libast/dir/seekdir.c	1.1"
#pragma prototyped

/*
 * seekdir
 *
 * seek on directory stream
 * this is not optimal because there aren't portable
 * semantics for directory seeks
 */

#include "dirlib.h"

#if _dir_ok

NoN(seekdir)

#else

void
seekdir(register DIR* dirp, long loc)
{
	off_t	base;		/* file location of block */
	off_t	offset; 	/* offset within block */

	if (telldir(dirp) != loc)
	{
		lseek(dirp->dd_fd, 0L, SEEK_SET);
		dirp->dd_loc = dirp->dd_size = 0;
		while (telldir(dirp) != loc)
			if (!readdir(dirp))
				break; 	/* "can't happen" */
	}
}

#endif

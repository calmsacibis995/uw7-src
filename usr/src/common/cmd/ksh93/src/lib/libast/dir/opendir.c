#ident	"@(#)ksh93:src/lib/libast/dir/opendir.c	1.1"
#pragma prototyped
/*
 * opendir, closedir
 *
 * open|close directory stream
 *
 * POSIX compatible directory stream access routines:
 *
 *	#include <sys/types.h>
 *	#include <dirent.h>
 *
 * NOTE: readdir() returns a pointer to struct dirent
 */

#include "dirlib.h"

#if _dir_ok

NoN(opendir)

#else

static const char id_dir[] = "\n@(#)directory (AT&T Bell Laboratories) 04/01/93\0\n";

static DIR*	freedirp;		/* always keep one dirp */

DIR*
opendir(register const char* path)
{
	register DIR*	dirp = 0;
	register int	fd;
	struct stat	st;

	if ((fd = open(path, 0)) < 0) return(0);
	if (fstat(fd, &st) < 0 ||
	   !S_ISDIR(st.st_mode) && (errno = ENOTDIR) ||
	   fcntl(fd, F_SETFD, FD_CLOEXEC) ||
	   !(dirp = freedirp ? freedirp :
#if defined(_DIR_PRIVATE) || _ptr_dd_buf
	   newof(0, DIR, 1, DIRBLKSIZ)
#else
	   newof(0, DIR, 1, 0)
#endif
		))
	{
		close(fd);
		if (dirp)
		{
			if (!freedirp) freedirp = dirp;
			else free(dirp);
		}
		return(0);
	}
	freedirp = 0;
	dirp->dd_fd = fd;
	dirp->dd_loc = dirp->dd_size = 0;	/* refill needed */
#if defined(_DIR_PRIVATE) || _ptr_dd_buf
	dirp->dd_buf = (void*)((char*)dirp + sizeof(DIR));
#endif
	return(dirp);
}

void
closedir(register DIR* dirp)
{
	if (dirp)
	{
		close(dirp->dd_fd);
		if (!freedirp) freedirp = dirp;
		else free(dirp);
	}
}

#endif

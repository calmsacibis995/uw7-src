#ident	"@(#)ksh93:src/lib/libast/path/pathstat.c	1.1"
#pragma prototyped

#include <ast.h>
#include <ls.h>
#include <error.h>

/*
 * physical stat if logical fails
 */

int
pathstat(const char* path, struct stat* st)
{
#if _lib_lstat
	int	oerrno;

	oerrno = errno;
	if (!stat(path, st)) return(0);
	errno = oerrno;
	return(lstat(path, st));
#else
	return(stat(path, st));
#endif
}

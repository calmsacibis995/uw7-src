#ident	"@(#)ksh93:src/lib/libast/comp/lstat.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_lstat

NoN(lstat)

#else

#include <ls.h>

int
lstat(const char* path, struct stat* st)
{
	return(stat(path, st));
}

#endif

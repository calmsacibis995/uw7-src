#ident	"@(#)ksh93:src/lib/libast/comp/mkfifo.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_mkfifo

NoN(mkfifo)

#else

#include <ls.h>
#include <error.h>

int
mkfifo(const char* path, mode_t mode)
{
#ifdef S_IFIFO
	return(mknod(path, S_IFIFO|(mode & ~S_IFMT), 0));
#else
	errno = EINVAL;
	return(-1);
#endif
}

#endif

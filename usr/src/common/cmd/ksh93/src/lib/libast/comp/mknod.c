#ident	"@(#)ksh93:src/lib/libast/comp/mknod.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_mknod

NoN(mknod)

#else

#include <ls.h>
#include <error.h>

int
mknod(const char* path, mode_t mode, dev_t dev)
{
	if (S_ISFIFO(mode)) return(mkfifo(path, mode));
	if (S_ISDIR(mode)) return(mkdir(path, mode));
	errno = EINVAL;
	return(-1);
}

#endif

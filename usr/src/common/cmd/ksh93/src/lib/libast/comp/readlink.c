#ident	"@(#)ksh93:src/lib/libast/comp/readlink.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_readlink

NoN(readlink)

#else

#include <error.h>

int
readlink(const char* path, char* buf, int siz)
{
	NoP(path);
	NoP(buf);
	NoP(siz);
	errno = EINVAL;
	return(-1);
}

#endif

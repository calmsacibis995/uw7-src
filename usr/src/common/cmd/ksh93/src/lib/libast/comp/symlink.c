#ident	"@(#)ksh93:src/lib/libast/comp/symlink.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_symlink

NoN(symlink)

#else

#include <error.h>

int
symlink(const char* a, char* b)
{
	NoP(a);
	NoP(b);
	errno = EINVAL;
	return(-1);
}

#endif

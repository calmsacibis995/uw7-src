#ident	"@(#)ksh93:src/lib/libast/comp/mount.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_mount

NoN(mount)

#else

#include <error.h>

int
mount(const char* a, char* b, int c, void* d)
{
	NoP(a);
	NoP(b);
	NoP(c);
	NoP(d);
	errno = EINVAL;
	return(-1);
}

#endif

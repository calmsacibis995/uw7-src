#ident	"@(#)ksh93:src/lib/libast/comp/unlink.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_unlink

NoN(unlink)

#else

int
unlink(const char* path)
{
	return(remove(path));
}

#endif

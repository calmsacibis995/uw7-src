#ident	"@(#)ksh93:src/lib/libast/comp/remove.c	1.1"
#pragma prototyped

#include <ast.h>

#ifndef _lib_unlink

NoN(remove)

#else

#undef	remove

int
remove(const char* path)
{
	return(unlink(path));
}

#endif

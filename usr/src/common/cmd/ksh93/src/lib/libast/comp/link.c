#ident	"@(#)ksh93:src/lib/libast/comp/link.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_link

NoN(link)

#else

#include <error.h>

int
link(const char* from, const char* to)
{
	NoP(from);
	NoP(to);
	errno = EINVAL;
	return(-1);
}

#endif

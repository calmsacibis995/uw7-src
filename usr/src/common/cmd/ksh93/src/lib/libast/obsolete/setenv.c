#ident	"@(#)ksh93:src/lib/libast/obsolete/setenv.c	1.1"
#pragma prototyped

/* OBSOLETE 940214 -- use setenviron */

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide setenv
#else
#define setenv		______setenv
#endif

#include <ast.h>

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide setenv
#else
#undef	setenv
#endif

char*
setenv(const char* p)
{
	return(setenviron(p));
}

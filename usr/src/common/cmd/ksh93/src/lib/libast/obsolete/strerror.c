#ident	"@(#)ksh93:src/lib/libast/obsolete/strerror.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use fmterror */

#include <ast.h>

#ifdef _lib_strerror

NoN(strerror)

#else

char*
strerror(int err)
{
	return(fmterror(err));
}

#endif

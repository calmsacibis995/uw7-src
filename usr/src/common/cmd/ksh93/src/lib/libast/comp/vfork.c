#ident	"@(#)ksh93:src/lib/libast/comp/vfork.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_vfork

NoN(vfork)

#else

#include <error.h>

#ifndef ENOSYS
#define ENOSYS		EINVAL
#endif

pid_t
vfork(void)
{
#if _lib_fork
	return(fork());
#else
	errno = ENOSYS;
	return(-1);
#endif
}

#endif

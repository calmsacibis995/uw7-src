#ident	"@(#)ksh93:src/lib/libast/comp/killpg.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_killpg

NoN(killpg)

#else

#include <sig.h>

int
killpg(pid_t g, int s)
{
	return(kill(-g, s));
}

#endif

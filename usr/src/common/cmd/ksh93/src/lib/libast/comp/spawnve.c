#ident	"@(#)ksh93:src/lib/libast/comp/spawnve.c	1.1"
#pragma prototyped

#include <ast.h>

#if _lib_spawnve

NoN(spawnve)

#else

pid_t
spawnve(const char* cmd, char* const argv[], char* const envv[])
{
	return(spawnveg(cmd, argv, envv, 0));
}

#endif

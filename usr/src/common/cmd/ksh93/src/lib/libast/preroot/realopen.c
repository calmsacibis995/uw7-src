#ident	"@(#)ksh93:src/lib/libast/preroot/realopen.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 * disable preroot and open path relative to the real root
 */

#include <ast.h>
#include <preroot.h>

#if FS_PREROOT

int
realopen(const char* path, int mode, int perm)
{
	char		buf[PATH_MAX + 8];

	if (*path != '/' || !ispreroot(NiL)) return(-1);
	strcopy(strcopy(buf, PR_REAL), path);
	return(open(buf, mode, perm));
}

#else

NoN(realopen)

#endif

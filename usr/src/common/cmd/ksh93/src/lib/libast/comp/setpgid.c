#ident	"@(#)ksh93:src/lib/libast/comp/setpgid.c	1.1"
#pragma prototyped

#include <ast.h>

#if _lib_setpgid

NoN(setpgid)

#else

#include <error.h>

#ifndef ENOSYS
#define ENOSYS		EINVAL
#endif

#if _lib_setpgrp2
#define setpgrp		setpgrp2
#else
#if _lib_BSDsetpgrp
#define _lib_setpgrp2	1
#define setpgrp		BSDsetpgrp
#else
#if _lib_wait3
#define	_lib_setpgrp2	1
#endif
#endif
#endif

#if _lib_setpgrp2
extern int		setpgrp(int, int);
#else
extern int		setpgrp(void);
#endif

/*
 * set process group id
 */

int
setpgid(pid_t pid, pid_t pgid)
{
#if _lib_setpgrp2
	return(setpgrp(pid, pgid));
#else
#if _lib_setpgrp
	int	caller = getpid();

	if ((pid == 0 || pid == caller) && (pgid == 0 || pgid == caller))
		return(setpgrp());
	errno = EINVAL;
#else
	errno = ENOSYS;
#endif
	return(-1);
#endif
}

#endif

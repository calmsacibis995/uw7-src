#ident	"@(#)ksh93:src/lib/libast/comp/dup2.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_dup2

NoN(dup2)

#else

#include <error.h>

int
dup2(int d1, int d2)
{
	int	save_errno;

	if (d1 == d2) return(d1);
	save_errno = errno;
	close(d2);
	errno = save_errno;
	return(fcntl(d1, F_DUPFD, d2));
}

#endif

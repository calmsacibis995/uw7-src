#ident	"@(#)ksh93:src/lib/libast/comp/rename.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_rename

NoN(rename)

#else

#include <error.h>

int
rename(const char* from, const char* to)
{
	int	oerrno;
	int	ooerrno;

	ooerrno = errno;
	while (link(from, to))
	{
		oerrno = errno;
		if (unlink(to))
		{
			errno = oerrno;
			return(-1);
		}
	}
	errno = ooerrno;
	unlink(from);
	return(0);
}

#endif

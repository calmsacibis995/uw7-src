#ident	"@(#)ksh93:src/lib/libast/comp/mktemp.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_mktemp

NoN(mktemp)

#else

char*
mktemp(char* buf)
{
	char*	s;
	char*	d;
	int	n;

	if (s = strrchr(buf, '/'))
	{
		*s++ = 0;
		d = buf;
	}
	else
	{
		s = buf;
		d = "";
	}
	if ((n = strlen(s)) < 6 || strcmp(s + n - 6, "XXXXXX"))
	{
		*buf = 0;
		return(buf);
	}
	*(s + n - 6) = 0;
	if (!pathtemp(buf, d, s)) *buf = 0;
	return(buf);
}

#endif

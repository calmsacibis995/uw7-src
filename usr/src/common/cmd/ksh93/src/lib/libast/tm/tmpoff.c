#ident	"@(#)ksh93:src/lib/libast/tm/tmpoff.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>

/*
 * append p and hh:mm part of n to s
 *
 * n ignored if n==d
 * end of s is returned
 */

char*
tmpoff(register char* s, register const char* p, register int n, int d)
{
	while (*s = *p++) s++;
	if (n != d)
	{
		if (n < 0)
		{
			n = -n;
			*s++ = '-';
		}
		else *s++ = '+';
		s += sfsprintf(s, 16, "%d", n / 60);
		if (n %= 60)
			s += sfsprintf(s, 16, ":%02d", n);
	}
	return(s);
}

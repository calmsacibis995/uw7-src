#ident	"@(#)ksh93:src/lib/libast/tm/tmgoff.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>
#include <ctype.h>

/*
 * return minutes offset from [[-+]hh[:mm[:ss]]] expression
 *
 * if e is non-null then it points to the first unrecognized char in s
 * d returned if no offset in s
 */

int
tmgoff(register const char* s, char** e, int d)
{
	register int	n;
	char*		t;

	if (isdigit(n = *s) || n == '-' || n == '+')
	{
		n = strtol(s, &t, 10);
		if (t > s)
		{
			n *= 60;
			if (*t == ':') n += (n < 0 ? -1 : 1) * strtol(t + 1, &t, 10);
			if (*t == ':') strtol(t + 1, &t, 10);
		}
		else n = d;
	}
	else
	{
		n = d;
		t = (char*)s;
	}
	if (e) *e = t;
	return(n);
}

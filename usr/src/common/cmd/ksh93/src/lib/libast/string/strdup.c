#ident	"@(#)ksh93:src/lib/libast/string/strdup.c	1.1"
#pragma prototyped

#include <ast.h>

/*
 * return a copy of s using malloc
 */

char*
strdup(register const char* s)
{
	register char*	t;
	register int	n;

	return((t = newof(0, char, n = strlen(s) + 1, 0)) ? (char*)memcpy(t, s, n) : 0);
}

#ident	"@(#)ksh93:src/lib/libast/string/strcopy.c	1.1"
#pragma prototyped

#include <ast.h>

/*
 * copy t into s, return a pointer to the end of s ('\0')
 */

char*
strcopy(register char* s, register const char* t)
{
	if (!t) return(s);
	while (*s++ = *t++);
	return(--s);
}

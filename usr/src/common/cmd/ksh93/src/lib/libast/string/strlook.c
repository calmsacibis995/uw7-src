#ident	"@(#)ksh93:src/lib/libast/string/strlook.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 */

#include <ast.h>

/*
 * return pointer to name in tab with element size siz
 *
 * the last name in tab must be 0
 * 0 returned if name not found
 */

void*
strlook(const void* tab, int siz, register const char* name)
{
	register char*	t = (char*)tab;
	register char*	s;

	for (; s = *((char**)t); t += siz)
		if (streq(s, name))
			return((void*)t);
	return(0);
}

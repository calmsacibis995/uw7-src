#ident	"@(#)ksh93:src/lib/libast/vec/vecstring.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * string vector load support
 */

#include <ast.h>
#include <vecargs.h>

/*
 * load a string vector from lines in str
 */

char**
vecstring(const char* str)
{
	register char*	buf;
	register char**	vec;

	if (!str || !*str || !(buf = strdup(str))) vec = 0;
	else if (!(vec = vecload(buf))) free(buf);
	return(vec);
}

#ident	"@(#)ksh93:src/lib/libast/string/fmtbase.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return base b representation for n
 * if n==0 or b==0 then output is signed base 10
 * if p!=0 then base prefix is included
 */

#include <ast.h>

char*
fmtbase(register long n, register int b, int p)
{
	static char	buf[36];

	if (n == 0 || b == 0) sfsprintf(buf, sizeof(buf), "%ld", n);
	else sfsprintf(buf, sizeof(buf), p ? "%#..*u" : "%..*u", b, n);
	return(buf);
}

#ident	"@(#)sccs:lib/mpwlib/cat.c	6.3"
#include	<varargs.h>

/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen,0);
*/

/*VARARGS*/
char *
cat(va_alist)
va_dcl
{
	register char *d, *s, *dest;
	va_list ap;

	va_start(ap);
	dest = va_arg(ap, char *);
	d = dest;

	while (s = va_arg(ap, char *)) {
		while (*d++ = *s++) ;
		d--;
	}
	return (dest);
}

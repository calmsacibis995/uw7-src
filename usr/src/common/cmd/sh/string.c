/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/string.c	1.8.7.2"
#ident "$Header$"
/*
 * UNIX shell
 */

#include	"defs.h"


/* ========	general purpose string handling ======== */


unsigned char *
movstr(a, b)
register unsigned char	*a, *b;
{
	while (*b++ = *a++);
	return(--b);
}

int
any(c, s)
register unsigned char	c;
unsigned char	*s;
{
	register unsigned char d;

	while (d = *s++)
	{
		if (d == c)
			return(TRUE);
	}
	return(FALSE);
}

int
anys(c, s)
unsigned char *c, *s;
{
	wchar_t f, e;
	register wchar_t d;
	register int n;
	if((n = mbtowc(&f, (char *)c, MULTI_BYTE_MAX)) <= 0)
		return(FALSE);
	d = f;
	for(;;) {
		if((n = mbtowc(&e, (char *)s, MULTI_BYTE_MAX)) <= 0)
			return(FALSE);
		if(d == e)
			return(TRUE);
		s += n;
	}
/*NOTREACHED*/
}

int
cf(s1, s2)
register unsigned char *s1, *s2;
{
	while (*s1++ == *s2)
		if (*s2++ == 0)
			return(0);
	return(*--s1 - *s2);
}

int
length(as)
unsigned char	*as;
{
	register unsigned char	*s;

	if ((s = as) != (unsigned char *)0)
		while (*s++);
	return(s - as);
}

unsigned char *
movstrn(a, b, n)
	register unsigned char *a, *b;
	register int n;
{
	while ((n-- > 0) && *a)
		*b++ = *a++;

	return(b);
}

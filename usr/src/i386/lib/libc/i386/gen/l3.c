#ident	"@(#)libc-i386:gen/l3.c	1.15"
/*LINTLIBRARY*/
/*
 * Convert longs to and from 3-byte disk addresses
 */
#ifdef __STDC__
	#pragma weak l3tol = _l3tol
	#pragma weak ltol3 = _ltol3
#endif
#include "synonyms.h"

#if M32
#define BIGENDIAN
#endif

void
ltol3(cp, lp, n)
char	*cp;
long	*lp;
int	n;
{
	register i;
	register char *a, *b;

	a = cp;
	b = (char *)lp;
	for(i=0; i < n; ++i) {
#if BIGENDIAN
		b++;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
#else
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
		b++;
#endif
	}
}

void
l3tol(lp, cp, n)
long	*lp;
char	*cp;
int	n;
{
	register i;
	register char *a, *b;

	a = (char *)lp;
	b = cp;
	for(i=0; i < n; ++i) {
#if BIGENDIAN
		*a++ = 0;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
#else
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = 0;
#endif
	}
}

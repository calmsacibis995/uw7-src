/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/memshift.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"

char *
memshift(dst, src, len)
char	*dst;
char	*src;
int	len;
{
	register char	*d;
	register char	*s;
	register int	n;
	extern char	*memcpy();

	if (dst < src)
		if (dst + len <= src)
			memcpy(dst, src, len);
		else
			for (s = src, d = dst, n = len; n > 0; n--)
				*d++ = *s++;
	else
		if (src + len <= dst)
			return memcpy(dst, src, len);
		else
			for (n = len, s = src + n, d = dst + n; n > 0; n--)
				*--d = *--s;
	return dst;
}

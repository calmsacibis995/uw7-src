/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/strsave.c	1.4.3.3"

#include	<stdio.h>
#include	<string.h>
#include	"terror.h"
#include	"wish.h"

/* les: changing to MACRO
char *
strsave(s)
char	s[];
{
	char	*strnsave();

	return s ? strnsave(s, strlen(s)) : NULL;
}
*/

char	*
strnsave(s, len)
char	s[];
unsigned int	len;
{
	register char	*p;

	if ((p = malloc(len + 1)) == NULL)
		fatal(NOMEM,nil);		/* dmd s15 */
	strncpy(p, s, len);
	p[len] = '\0';
	return p;
}

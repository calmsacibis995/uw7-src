/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/stream.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"token.h"

token
stream(t, s)
register token t;
register token (*s[])();
{
	register int	i;

	for (i = 0; s[i]; i++)
		if ((t = (*(s[i]))(t)) == TOK_NOP)
			break;
	return t;
}

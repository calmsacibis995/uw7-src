#ident "@(#)error.c	3.1"
#ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include "include.h"

static char errorbuf[256];

Error(msg_id, fmt, a,b,c,d,e,f,g,h)
int	msg_id;
char	*fmt;
{

	static char Ebuf[1024];

	catfprintf(stderr, msg_id, fmt, a, b, c, d, e, f, g, h);
	sprintf(Ebuf, fmt,a,b,c,d,e,f,g,h);
	sprintf(errorbuf, "%s\n", Ebuf);
	Log(errorbuf);
}

SystemError(msg_id, fmt, a,b,c,d,e,f,g,h)
int	msg_id;
char	*fmt;
{
	extern int errno;

	Error(msg_id,fmt,a,b,c,d,e,f,g,h);
	perror("dlpid");
}

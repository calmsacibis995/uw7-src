/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*		copyright	"%c%" 	*/

#ident	"@(#)log.c	1.3"
#ident	"$Header$"

/*******************************************************************************
 *
 * FILENAME:    log.c
 *
 * DESCRIPTION: Log message for the lpNet
 *
 * SCCS:	log.c 1.3  10/30/97 at 11:23:24
 *
 * CHANGE HISTORY:
 *
 * 30-10-97  Paul Cunningham        no MR
 *           Change function log() so that it checks environment varaiable
 *           LPNET_DEBUG before not logging a message. With LPNET_DEBUG=debug
 *           all messages will be logged.
 *
 *******************************************************************************
 */

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "logMgmt.h"
#include "lpd.h"

/*
 * Format and log message
 */
/*VARARGS2*/
void
#if defined (__STDC__)
logit(int type, char *msg, ...)
#else
logit(type, msg, va_alist)
int	 type;
char	*msg;
va_dcl
#endif
{
	va_list		 argp;
	static char	*buf;

	if ( !(type & LOG_MASK))
	{
	  /* check the environment variable LPNET_DEBUG isn't set to "debug"
	   */
	  if ( strcmp( getenv( "LPNET_DEBUG"), "debug") != 0)
	  {
		/* don't log this message
		 */
		return;
	  }
	}
	/*
	 * Use special buffer for log activity to avoid overwriting
	 * general buffer, Buf[], which may otherwise, be in use.
	 */
	if (!buf && !(buf = (char *)malloc(LOGBUFSZ)))
		return;			/* We're in trouble */
#if defined (__STDC__)
	va_start(argp, msg);
#else
	va_start(argp);
#endif
	(void)vsprintf(buf, msg, argp);
	va_end(argp);
	WriteLogMsg(buf);
}

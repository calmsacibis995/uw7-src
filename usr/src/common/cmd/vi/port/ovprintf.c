/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Copyright (c) 1979 Regents of the University of California */
#ident	"@(#)vi:port/ovprintf.c	1.5.1.4"
#ident  "$Header$"

/*
 * This version of printf calls doprnt, and as such is not portable,
 * since doprnt is written in pdp-11 assembly language.  (There is a
 * vax doprnt which has the first 2 arguments reversed.  We don't use it.)
 * This version is used because it is about 900 bytes smaller than the
 * portable version, which is also included in case it is needed.
 */
#ifdef TRACE
#include	<stdio.h>
#undef putchar
#endif

printf(fmt, args)
char *fmt;
{
	_doprnt(fmt, &args, 0);
}

_strout(string, count, adjust, file, fillch)
register char *string;
register count;
int adjust;
register struct _iobuf *file;
{
	while (adjust < 0) {
		if (*string=='-' && fillch=='0') {
			putchar(*string++);
			count--;
		}
		putchar(fillch);
		adjust++;
	}
	while (--count>=0)
		putchar(*string++);
	while (adjust) {
		putchar(fillch);
		adjust--;
	}
}

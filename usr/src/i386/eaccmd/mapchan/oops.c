/*		copyright	"%c%" 	*/

#ident	"@(#)oops.c	1.2"
#ident  "$Header$"
/*
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987, 1988.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987, 1988.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
#include <stdio.h>

/*
 * print an error message on stderr.
 * this is only for syntactic brevity.
 */
error(fmt, args)
char *fmt;
int args;
{
	vfprintf(stderr, fmt, &args);
}

/*
 * print an error message and quit. 
 * again, only for syntactic brevity.
 */
oops(fmt, args)
char *fmt;
int args;
{
	vfprintf(stderr, fmt, &args);
	exit(1);
}

#ident	"@(#)OSRcmds:lib/libgen/errafter.c	1.1"
/*
 *	@(#) errafter.c 20.1 94/12/04 
 *
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1995 The Santa Cruz Operation, Inc.
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libgen:errafter.c	1.3" */
#pragma comment(exestr, "@(#) errafter.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*
	Customized routine called after error message has been printed.
	Command and library version.
	Return a value to indicate action.
*/

#include	"../../include/errmsg.h"
#include	<stdio.h>
#include	<varargs.h>

int
errafter( severity, format, print_args )
int	severity;
char	*format;
va_list print_args;
{
	switch( severity ) {
	case EHALT:
		return EABORT;
	case EERROR:
		return EEXIT;
	case EWARN:
		return ERETURN;
	case EINFO:
		return ERETURN;
	}
	return ERETURN;
}

#ident	"@(#)OSRcmds:lib/libgen/errbefore.c	1.1"
/*
 *	@(#) errbefore.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:errbefore.c	1.3" */
#pragma comment(exestr, "@(#) errbefore.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*
	Routine called before error message has been printed.
	Command and library version.
*/

#include	<varargs.h>
#include	"../../include/errmsg.h"


void
errbefore(severity, format, print_args)
int      severity;
char     *format;
va_list  print_args;
{
	switch( severity ) {
	case EHALT:
	case EERROR:
	case EWARN:
	case EINFO:
		break;
	}
	return;
}

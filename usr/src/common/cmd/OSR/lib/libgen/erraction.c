#ident	"@(#)OSRcmds:lib/libgen/erraction.c	1.1"
/*
 *	@(#) erraction.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:erraction.c	1.3" */
#pragma comment(exestr, "@(#) erraction.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*
	Routine called after error message has been printed.
	Dependent upon the return code of errafter.
	Command and library version.
*/

#include	"../../include/errmsg.h"
#include	<stdio.h>

void
erraction( action )
int      action;
{


	switch( action ){
	case EABORT:
	     abort();
	     break;
	case EEXIT:
	     exit( Err.exit );
	     break;
	case ERETURN:
	     break;
	}
	return;
}

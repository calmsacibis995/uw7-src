#ident	"@(#)OSRcmds:sh/setbrk.c	1.1"
/*
 *	@(#) setbrk.c 1.4 88/11/11 
 *
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1989 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:setbrk.c	1.4" */
#pragma comment(exestr, "@(#) setbrk.c 1.4 88/11/11 ")
/*
 *	UNIX shell
 */

#include	"defs.h"

char 	*sbrk();

unsigned char*
setbrk(incr)
{

	register unsigned char *a = (unsigned char *)sbrk(incr);

	brkend = a + incr;
	return(a);
}

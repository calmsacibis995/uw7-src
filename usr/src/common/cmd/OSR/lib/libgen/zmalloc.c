#ident	"@(#)OSRcmds:lib/libgen/zmalloc.c	1.1"
/*
 *	@(#) zmalloc.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:zmalloc.c	1.3" */
#pragma comment(exestr, "@(#) zmalloc.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	malloc(3C) with error checking
*/

#include <stdio.h>
#include "../../include/errmsg.h"


char *
zmalloc( severity, n)
int		severity;
unsigned	n;
{
	char	*p;

	if( (p = (char *)malloc(n)) == NULL )
		_errmsg("UXzmalloc1", severity,
			"Cannot allocate a block of %d bytes.",
			n);

	return  p;
}

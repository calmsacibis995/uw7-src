#ident	"@(#)OSRcmds:lib/libgen/zrealloc.c	1.1"
/*
 *	@(#) zrealloc.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:zrealloc.c	1.3" */
#pragma comment(exestr, "@(#) zrealloc.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	realloc(3C) with error checking
*/

#include	"../../include/errmsg.h"
#include	<stdio.h>

char *
zrealloc( severity, ptr, size)
int	  severity;
char	  *ptr;
unsigned  size;
{
	 char *p;

	if( ( p = (char *)realloc(ptr, size) ) == NULL )
		_errmsg("UXzrealloc1", severity,
		       "Cannot reallocate %d bytes.",
			size);
	return p;
}

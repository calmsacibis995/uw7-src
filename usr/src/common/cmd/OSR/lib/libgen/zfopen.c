#ident	"@(#)OSRcmds:lib/libgen/zfopen.c	1.1"
/*
 *	@(#) zfopen.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:zfopen.c	1.3" */
#pragma comment(exestr, "@(#) zfopen.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	fopen(3S) with error checking
*/

#include	"../../include/errmsg.h"
#include	<stdio.h>


FILE *
zfopen( severity, path, type )
int	severity;
char	*path;
char	*type;
{
	register FILE	*fp;	/* file pointer */

	if( (fp = fopen( path, type )) == NULL ) {
		char	*mode;

		if( type[1] == '+' )
			mode = "updating";
		else
			switch( type[0] ) {
			case 'r':
				mode = "reading";
				break;
			case 'w':
				mode = "writing";
				break;
			case 'a':
				mode = "appending";
				break;
			default:
				mode = type;
			}
		_errmsg( "UXzfopen1", severity,
			"Cannot open file \"%s\" for %s.",
			 path, mode );
	}
	return  fp;
}

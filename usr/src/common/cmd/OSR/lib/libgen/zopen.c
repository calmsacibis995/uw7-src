#ident	"@(#)OSRcmds:lib/libgen/zopen.c	1.1"
/*
 *	@(#) zopen.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:zopen.c	1.3" */
#pragma comment(exestr, "@(#) zopen.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	open(2) with error checking
*/

#include	"../../include/errmsg.h"
#include	<fcntl.h>
#include	<stdio.h>


zopen(  severity, path, oflag, mode )
int	severity;
char	*path;
int	oflag;
int	mode;
{
	register int	fd;	/* file descriptor */

	if( (fd = open( path, oflag, mode )) == -1 ) {
		_errmsg( "UXzopen1", severity,
			"Cannot open file \"%s\".",
			 path );
	}
	return  fd;
}

#ident	"@(#)OSRcmds:lib/libgen/zclose.c	1.1"
/*
 *	@(#) zclose.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:zclose.c	1.3" */
#pragma comment(exestr, "@(#) zclose.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	close(2) with error checking
*/

#include	<stdio.h>
#include	"../../include/errmsg.h"

int
zclose( severity, fildes )
int	 severity;
int	 fildes;
{

	int	err_ind;

	if( (err_ind = close( fildes )) == -1 )
	    _errmsg("UXzclose1", severity,
		  "Cannot close file descriptor %d.",
		   fildes);

	return err_ind;
}

#ident	"@(#)OSRcmds:lib/libgen/zchmod.c	1.1"
/*
 *	@(#) zchmod.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:zchmod.c	1.3" */
#pragma comment(exestr, "@(#) zchmod.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	chmod(2) with error checking
*/

#include	"../../include/errmsg.h"

int
zchmod( severity, path, mode )
int	 severity;
char	*path;
int	 mode;
{

	int	err_ind;

	if( (err_ind = chmod(path, mode )) == -1 )
	    _errmsg ( "UXzchmod1", severity,
		  "Cannot change the mode of file \"%s\" to %d.",
		   path, mode);

	return err_ind;
}

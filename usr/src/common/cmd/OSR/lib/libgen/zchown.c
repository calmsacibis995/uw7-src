#ident	"@(#)OSRcmds:lib/libgen/zchown.c	1.1"
/*
 *	@(#) zchown.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:zchown.c	1.3" */
#pragma comment(exestr, "@(#) zchown.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	chown(2) with error checking
*/

#include	<stdio.h>
#include	"../../include/errmsg.h"

int
zchown( severity, path, owner, group )
int	severity;
char	*path;
int	 owner;
int	 group;
{

	int	err_ind;

	if( (err_ind = chown( path, owner, group )) == -1 )
	    _errmsg("UXzchown1", severity,
		  "Cannot change owner to %d and group to %d for file \"%s\".",
		   owner,group,path);

	return err_ind;
}

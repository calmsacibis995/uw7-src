#ident	"@(#)OSRcmds:lib/libgen/zstat.c	1.1"

#ifdef __STDC__
#pragma comment(exestr, "@(#) zstat32.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif
#else
#ident "@(#) zstat32.c 20.1 94/12/04 "
#endif /* __STDC__ */
/*
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

/*	stat(2) with error checking
*/

#include "../../include/errmsg.h"
#include <sys/stat.h>

int
zstat( severity, path, buf)
int	severity;
char	*path;
struct	stat *buf;
{
	int	rc;

	if((rc = stat(path, buf)) == -1 )
		_errmsg("UXzstat1", severity,
		      "Cannot obtain information about file:  \"%s\".",
		       path);


	return  rc;
}

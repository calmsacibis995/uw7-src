#ident	"@(#)OSRcmds:lib/libgen/Err.c	1.1"
/*
 *	@(#) Err.c 20.1 94/12/04 
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

/* #ident	"@(#)libgen:Err.c	1.3" */
#pragma comment(exestr, "@(#) Err.c 20.1 94/12/04 ")

#ifdef __FROM_USL
#include <synonyms.h>
#endif

/*	Set default values in Err global structure.
*/

#include	"../../include/errmsg.h"

static	char	deftofix[] = "Refer to help error database or manual.";
static	char	*defsevmsg[] = {	/* default severity messages */
		"INFORM: ",
		"WARNING: ",
		"ERROR: ",
		"HALT: ",
		0
	};

struct Err	Err = {
					/* verbosity flags */
		/* vbell */	ENO,
		/* vprefix */	EYES,
		/* vsource */	EYES,
		/* vsevmsg */	EYES,
		/* vsyserr */	EDEF,
		/* vfix */	EYES,
		/* vtag */	EYES,
		/* vtext */	EYES,
					/* message content */
		/* prefix */	0,
		/* envsource */	0,
		/* source */	0,
		/* severity */	0,
		/* sevmsg */	defsevmsg,
		/* tofix */	deftofix,
		/* tagnum */	0,
		/* tagstr */	0,
		/* exit */	1,
};


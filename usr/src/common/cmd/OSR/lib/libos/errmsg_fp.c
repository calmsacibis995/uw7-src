#ident	"@(#)OSRcmds:lib/libos/errmsg_fp.c	1.2"
#pragma comment(exestr, "@(#) errmsg_fp.c 25.1 92/07/06 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * errmsg_fp	-- Where to send error messages (usually stderr)
 *
 *  MODIFICATION HISTORY
 *	12 May 1992	scol!blf	creation
 *		- Slightly reworked generalization of routines used for
 *		  some time in varying forms -- to encourge the radical
 *		  concept of standardisation and modularity.
 */

#include <unistd.h>
/*#include "errormsg.h"	/* use local master not unknown installed version */
#include <stdio.h>

FILE	*errmsg_fp	= stderr;

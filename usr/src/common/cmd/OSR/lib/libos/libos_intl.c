#ident	"@(#)OSRcmds:lib/libos/libos_intl.c	1.1"
#pragma comment(exestr, "@(#) libos_intl.c 26.2 95/07/26 ")
/*
 *	Copyright (C) 1995 The Santa Cruz Operation, Inc.
 *		All rights reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */

/* MODIFICATION HISTORY
 *
 *	26 May 1995	scol!ashleyb
 *	- Created
 *	24 Jul 1995	scol!ianw	[not marked]
 *	- Added the message catalog name parameter to allow use by other
 *	  libraries and changed the name to be more generic.
 */

#ifdef INTL

/* #include <locale.h> */
#include "libos_msg.h"

#include <stdio.h>

char *
__lib_msg(const char *catname, int message_id, char *default_string)
{
	char *msg;
	nl_catd catd;

	catd = catopen(catname, MC_FLAGS);
	msg = MSGSTR(message_id, default_string);
	catclose(catd);
	return(msg);
}

#endif /* INTL */

#ident	"@(#)OSRcmds:ksh/shlib/chkid.c	1.1"
#pragma comment(exestr, "@(#) chkid.c 25.2 92/12/11 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1990-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*									*/
/*		   Copyright (C) AT&T, 1984-1992			*/
/*			All Rights Reserved				*/
/*									*/
/*	  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.		*/
/*	    The copyright notice above does not evidence any		*/
/*	   actual or intended publication of such source code.		*/
/*									*/


/*
 *   NAM_HASH (NAME)
 *
 *        char *NAME;
 *
 *	Return a hash value of the string given by name.
 *      Trial and error has shown this hash function to perform well
 *
 */

#include	"sh_config.h"

int nam_hash(name)
register const char *name;
{
	register int h = *name;
	register int c;
	while(c= *++name)
	{
		if((h = (h>>2) ^ (h<<3) ^ c) < 0)
			h = ~h;
	}
	return (h);
}


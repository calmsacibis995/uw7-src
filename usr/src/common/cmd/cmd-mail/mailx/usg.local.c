#ident	"@(#)usg.local.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)usg.local.c	1.10 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Local routines that are installation dependent.
 */

#include "rcv.h"

/*
 * Locate the user's mailbox file (ie, the place where new, unread
 * mail is queued).  In SVr4 UNIX, it is in /var/mail/name.
 * In preSVr4 UNIX, it is in either /usr/mail/name or /usr/spool/mail/name.
 */
void
findmail()
{
	/* let c-client figure out where the user's default mailbox is */
	strcpy(mailname, "INBOX");
}

/*
 * Return the value of $PAGER
 */
const char *pager()
{
	char *pg = value("PAGER");
	if (!pg)		/* default to pg -e if not set */
		pg = "pg -e";
	else if (pg && !*pg)	/* default to cat if no value */
		pg = "cat";
	return pg;
}

/*
 * Return the value of $LISTER
 */
const char *lister()
{
	char *ls = value("LISTER");
	if (!ls || !*ls)	/* default to ls if not set or no value */
		ls = "ls";
	return ls;
}



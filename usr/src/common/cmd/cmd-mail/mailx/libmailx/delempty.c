#ident	"@(#)delempty.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)delempty.c	1.7 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	delempty - delete an empty mail box

    SYNOPSIS
	int delempty(mode_t mode, const char *mailname)

    DESCRIPTION
	Delete an empty mail box if it's allowed. Check
	the value of mgetenv("DEL_EMPTY_MFILE") for
	"yes" (always), "no" (never) or the default (based
	on the mode).	
*/

int delempty(mode, mailname)
mode_t mode;
MAILSTREAM *mailname;
{
    char *del_empty = mgetenv("DEL_EMPTY_MFILE");
    int do_del = 0;

    if (del_empty)
	{
	int del_len = strlen(del_empty);

	/* an empty string means to remove the mailfile */
	if (del_len == 0)
	    do_del = 1;

	/* "yes" means always remove the mailfile */
	else if (casncmp(del_empty, "yes", del_len) == 0)
	    do_del = 1;

	/* "no" means never remove the mailfile */
	else if (casncmp(del_empty, "no", del_len) == 0)
	    /* EMPTY */;

	/* all other values say to check for mode 0660 */
	else if ((mode & 07777) == MFMODE)
	    do_del = 1;
	}

    /* missing value says to check for mode 0660 */
    else if ((mode & 07777) == MFMODE)
	do_del = 1;

    if (do_del)
	    (void) mail_delete(mailname, mailname->mailbox);
    return do_del;
}

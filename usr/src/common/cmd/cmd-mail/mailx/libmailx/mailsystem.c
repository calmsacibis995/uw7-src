#ident	"@(#)mailsystem.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mailsystem.c	1.2 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	mailsystem() - retrieve the system name

    SYNOPSIS
	char *mailsystem(int which)

    DESCRIPTION
	Return the system name. If which == 0, the
	system name is that of mgetenv("CLUSTER").
	If that is not set, or if which == 1, use uname(2).
*/

static struct utsname mailuname;	/* name of the system */

char *mailsystem(which)
int which;
{
    static char *p;

    if (!p)
	{
	if (uname(&mailuname) == -1)
	    (void) strcpy(mailuname.nodename, "UNKNOWN");
	/* Are we who we appear to be? */
	if((p = mgetenv("FROM_SYSTEM")) != (char *)NULL)
	    {
	    }
	else if ((p = mgetenv("CLUSTER")) == (char *)NULL)
	    p = mailuname.nodename;
	}

    return which ? mailuname.nodename : p;
}

#ident	"@(#)mgetcharset.c	11.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mgetcharset.c	1.1 'attmail mail(1) command'"
#include <langinfo.h>
#include "libmail.h"
/*
    NAME
	mail_get_charset - get the character set associated with the current locale

    SYNOPSIS
	const char *mail_get_charset()

    DESCRIPTION
	Return the character set associated with the current locale.
*/

const char *mail_get_charset()
{
	char *p;
	char *cp;
	static char buf[MAXPATHLEN];

	p = getenv("MM_CHARSET");
	if (p && *p)
		return p;

	p = nl_langinfo(CODESET);
	if (p == 0 || *p == 0)
		p = "us-ascii";
	if ((strlen(p) > 3) && (casncmp(p, "ISO8859-", 8) == 0)) {
		strcpy(buf, "ISO-8859-");
		strcat(buf, p + 8);
		p = buf;
	}
	else {
		cp = strchr(p, '.');
		if (cp) {
			strcpy(buf, cp+1);
			p = buf;
		}
	}
	return(p);
}

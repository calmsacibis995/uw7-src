/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)trimnl.c	11.1"
#ident	"@(#)trimnl.c	11.1"
#include "libmail.h"
/*
    NAME
	trimnl - trim trailing newlines from string
	trimcrlf - trim trailing newlines and linefeeds from string

    SYNOPSIS
	void trimnl(char *s)
	void trimcrlf(char *s)

    DESCRIPTION
	trimnl() goes to the end of the string and
	removes an trailing newlines.
*/

void trimnl(s)
register char 	*s;
{
    register char	*p;

    p = s + strlen(s) - 1;
    while ((*p == '\n') && (p >= s))
	*p-- = '\0';
}

void
trimcrlf(s)
register char 	*s;
{
    register char	*p;

    p = s + strlen(s) - 1;
    while (((*p == '\n') || (*p == '\r')) && (p >= s))
	*p-- = '\0';
}

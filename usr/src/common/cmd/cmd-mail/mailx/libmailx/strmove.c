/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)strmove.c	11.1"
#ident	"@(#)strmove.c	11.1"
#include "libmail.h"
/*
    NAME
	strmove - copy a string, permitting overlaps

    SYNOPSIS
	void strmove(char *to, const char *from)

    DESCRIPTION
	strmove() acts exactly like strcpy() with the additional
	guarantee that it will work with overlapping strings.
	It does it left-to-right, a byte at a time.
*/

void strmove(to, from)
register char *to;
register const char *from;
{
    while ((*to++ = *from++) != 0)
	;
}

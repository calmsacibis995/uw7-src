/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cascmp.c	11.1"
#ident	"@(#)cascmp.c	11.1"
#include "libmail.h"
#include <ctype.h>
/*
    NAME
	cascmp - compare strings ignoring case

    SYNOPSIS
	int cascmp(const char *s1, const char *s2)

    DESCRIPTION
	Compare two strings ignoring case differences.
	Stop at the trailing NUL.
*/

#define lower(c)        ( Isupper(c) ? _tolower(c) : (c) )

int cascmp(s1, s2)
register const char *s1, *s2;
{
	if (s1 == s2)
		return(0);
	while (*s1 && (lower(*s1) == lower(*s2)))
		s1++, s2++;
	return (lower(*s1) - lower(*s2));
}

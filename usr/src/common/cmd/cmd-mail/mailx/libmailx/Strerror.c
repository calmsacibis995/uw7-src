/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)Strerror.c	11.1"
#ident	"@(#)Strerror.c	11.1"
#include "libmail.h"
/*
    NAME
	Strerror - version of strerror() that doesn't return NULL

    SYNOPSIS
	const char *Strerror(int errno)

    DESCRIPTION
	The strerror() function returns NULL if it is given a bad
	error number. This function calls strerror(). If NULL is
	returned, the message "Unknown error #%d" is returned instead.
*/

const char *Strerror(errnum)
int errnum;
{
    const char *ret = strerror(errnum);
    char buf[40];
    if (ret)
	return ret;
    (void) sprintf(buf, "Unknown error #%d", errnum);
    return buf;
}

#ident	"@(#)lock.c	11.1"
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

#ident "@(#)lock.c	1.4 'attmail mail(1) command'"
/*
 * lock a file pointer
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

lock(fp, mode, blk)
FILE	*fp;
char	*mode;
int	blk;
{
	struct flock	l;

	l.l_type = !strcmp(mode, "r") ? F_RDLCK : F_WRLCK;
	l.l_whence = 0;
	l.l_start = l.l_len = 0L;
	return fcntl(fileno(fp), blk ? F_SETLKW : F_SETLK, &l);
}

/*
 * Copyright 1990, 1991, 1992 by the Massachusetts Institute of Technology and
 * UniSoft Group Limited.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the names of MIT and UniSoft not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  MIT and UniSoft
 * make no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * $XConsortium: getcwd.c,v 1.2 92/07/01 11:59:01 rws Exp $
 */

#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>

/*
 * A simple emulation of getcwd() to allow the tcc to run on BSD systems
 * that don't provide this.
 * getcwd() differs from the BSD getwd() by specifing a size.
 * 
 * Do not use if the system provides getcwd()
 * 
 */
char *
getcwd(buf, size)
char	*buf;
int 	size;
{
char	*bp;
char	*getwd();

	if (size <= 0) {
		errno = EINVAL;
		return((char*) 0);
	}

	/*
	 * get a buffer that hold the max possible BSD pathname.
	 */
	bp = (char *)malloc((size_t)(MAXPATHLEN+1));
	if (bp == (char *)0) {
		errno = ENOMEM;
		return((char *) 0);
	}

	if (getwd(bp) == (char *)0) {
		/* errno should have been set */
		return((char *) 0);
	}

	if (strlen(bp) >= size) {
		errno = ERANGE;
		return((char *) 0);
	}

	strcpy(buf, bp);

	free(bp);

	return(buf);
	
}

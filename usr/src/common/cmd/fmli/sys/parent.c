/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/parent.c	1.2.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"sizes.h"

char *
parent(path)
register char *path;
{
	register char	*dst;
	register char	*place;
	register bool	slash;
	static char	dirname[PATHSIZ];

	/* first, put a "well-behaved" path into dirname */
	place = NULL;
	slash = FALSE;
	for (dst = dirname; *path; path++)
		if (*path == '/')
			slash = TRUE;
		else {
			if (slash) {
				place = dst;
				*dst++ = '/';
				slash = FALSE;
			}
			*dst++ = *path;
		}
	if (dst == dirname) {
		place = dst;
		*dst++ = '/';
	}
	if (place == NULL) {
		dirname[0] = '.';
		dirname[1] = '\0';
	}
	else if (place == dirname)
		dirname[1] = '\0';
	else
		*place = '\0';
	return dirname;
}

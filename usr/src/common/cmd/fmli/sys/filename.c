/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/filename.c	1.1.4.3"

#include <stdio.h>

char *
filename(pt)
register char *pt;
{
	register char *name;
	char *strrchr();

	if (pt == NULL)
		return "(null)";
	if ((name = strrchr(pt, '/')) == NULL)
		return pt;

	return name + 1;
}

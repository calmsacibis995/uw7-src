/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/child.c	1.1.1.2"
#ident  "$Header$"
/*	@(#) child.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

/*	child(path,separator)  --  returns a pointer to the last component
 *			of the path, given the component separator.
 */

#include	<stdio.h>
#include	"dosutil.h"


char *child(path,separator)
char *path, separator;
{
	char *c;

	if ((c = strrchr(path,separator)) == NULL)
		return(path);
	return(++c);
}

/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/chgepenv.c	1.2.4.4"

#include	<stdio.h>
#include	"sizes.h"


char *
chgepenv(name, value)
char	*name, *value;
{
	char dirpath[PATHSIZ];
	extern char	*Home;
	char	*strcat();
	char	*strcpy();
	char	*chgenv();

	return chgenv(strcat(strcpy(dirpath, Home ? Home : ""), "/pref/.environ"), name, value);
}

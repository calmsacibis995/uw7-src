/*		copyright	"%c%" 	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/externoot.c	1.2.4.4"

#include "sizes.h"

char *
externoot(obj)
char *obj;
{
	char *extdir = "/info/OH/externals/";
	extern char *Oasys;
	static char fname[PATHSIZ];

	strcpy(fname, Oasys ? Oasys : "");
	strcat(fname, extdir);
	strcat(fname, obj);
	return(fname);
}


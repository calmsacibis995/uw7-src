/*		copyright	"%c%" 	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/if_exec.c	1.1.4.3"

#include <stdio.h>
#include <varargs.h>
#include "wish.h"
#include "terror.h"


int
IF_exec_open(argv)
char *argv[];
{
	(void) proc_openv(0, NULL, NULL, argv);
}

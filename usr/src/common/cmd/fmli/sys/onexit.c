/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/onexit.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"var_arrays.h"

extern int	(**Onexit)();

void
onexit(func)
int	(*func)();
{
	Onexit = (int (**)()) array_check_append(sizeof(int (*)()), (struct v_array *) Onexit, &func);
}

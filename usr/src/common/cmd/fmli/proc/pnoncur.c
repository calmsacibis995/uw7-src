/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:proc/pnoncur.c	1.5.3.3"

#include <stdio.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "terror.h"
#include "proc.h"
#include "procdefs.h"

extern struct proc_ref PR_all;

int
proc_noncurrent(p, all)
proc_id p;
bool all;
{
	/* suspend process */
#ifdef _DEBUG
	_debug(stderr, "proc_noncurrent not yet implemented\n");
#endif
}

#ident	"@(#)ksh93:src/lib/libast/misc/procfree.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * free a proc opened by procopen()
 * skipping wait() and close()
 */

#include "proclib.h"

int
procfree(register Proc_t* p)
{
	if (!p) return(-1);
	if (p == &proc_default) p->pid = -1;
	else free(p);
	return(0);
}

#ident	"@(#)ksh93:src/cmd/ksh93/sh/waitevent.c	1.2"
#pragma prototyped

#include	"defs.h"
/*
 *  This installs a hook to allow the processing of events when
 *  the shell is waiting for input and when the shell is
 *  waiting for job completion.
 *  The previous waitevent hook function is returned
 */

void  *sh_waitnotify(int(*newevent)(int,long))
{
	int (*old)(int,long);
	old = sh.waitevent;
	sh.waitevent = newevent;
	return((void*)old);
}

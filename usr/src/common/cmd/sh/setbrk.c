/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/setbrk.c	1.8.6.2"
#ident "$Header$"
/*
 *	UNIX shell
 */

#include	"defs.h"

#ifdef __STDC__
extern void *sbrk();
#else
extern char 	*sbrk();
#endif

unsigned char*
setbrk(incr)
{

	register unsigned char *a = (unsigned char *)sbrk(incr);

	brkend = a + incr;
	return(a);
}

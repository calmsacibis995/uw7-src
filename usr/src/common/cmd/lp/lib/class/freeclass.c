/*		copyright	"%c%" 	*/

#ident	"@(#)freeclass.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#include "lp.h"
#include "class.h"

/**
 ** freeclass() - FREE SPACE USED BY CLASS STRUCTURE
 **/

void
#if	defined(__STDC__)
freeclass (
	CLASS *			clsbufp
)
#else
freeclass (clsbufp)
	CLASS			*clsbufp;
#endif
{
	if (!clsbufp)
		return;
	if (clsbufp->name)
		Free (clsbufp->name);
	freelist (clsbufp->members);
	return;
}

/*		copyright	"%c%" 	*/

#ident	"@(#)freelist.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "sys/types.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** freelist() - FREE ALL SPACE USED BY LIST
 **/

void
#if	defined(__STDC__)
freelist (
	char **			list
)
#else
freelist (list)
	char			**list;
#endif
{
	register char		**pp;

	if (list) {
		for (pp = list; *pp; pp++)
			Free (*pp);
		Free ((char *)list);
	}
	return;
}

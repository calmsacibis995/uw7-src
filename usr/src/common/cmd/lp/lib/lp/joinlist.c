/*		copyright	"%c%" 	*/

#ident	"@(#)joinlist.c	1.2"
#ident	"$Header$"
/* LINTLIBRARY */

#include "lp.h"

/**
 ** mergelist() - ADD CONTENT OF ONE LIST TO ANOTHER
 **/

int
#if	defined(__STDC__)
joinlist (
	char ***		dstlist,
	char **			srclist
)
#else
joinlist (dstlist, srclist)
	char ***		dstlist;
	char **			srclist;
#endif
{
	if (!srclist || !*srclist)
		return (0);

	while (*srclist)
		if (appendlist(dstlist, *srclist++) == -1)
			return (-1);

	return (0);
}

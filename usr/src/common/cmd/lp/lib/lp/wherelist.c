/*		copyright	"%c%" 	*/

#ident	"@(#)wherelist.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "lp.h"

/**
 ** wherelist() - RETURN POINTER TO ITEM IN LIST
 **/

char **
#if	defined(__STDC__)
wherelist (
	char *			item,
	char **			list
)
#else
wherelist (item, list)
	register char		*item;
	register char		**list;
#endif
{
	if (!list || !*list)
		return (0);

	while (*list) {
		if (STREQU(*list, item))
			return (list);
		list++;
	}
	return (0);
}

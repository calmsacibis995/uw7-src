/*		copyright	"%c%" 	*/

#ident	"@(#)charset.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "lp.h"

/**
 ** search_cslist() - SEARCH CHARACTER SET ALIASES FOR CHARACTER SET
 **/

char *
#if	defined(__STDC__)
search_cslist (
	char *			item,
	char **			list
)
#else
search_cslist (item, list)
	register char		*item;
	register char		**list;
#endif
{
	register char		*alias;


	if (!list || !*list)
		return (0);

	else if (STREQU(item, NAME_ANY))
		return (item);

	/*
	 * This is a linear search--we believe that the lists
	 * will be short.
	 */
	while (*list) {
		alias = strchr(*list, '=');
		if (alias && STREQU(alias+1, item))
			return (*list);
		list++;
	}
	return (0);
}

/*		copyright	"%c%" 	*/

#ident	"@(#)search.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "lp.h"
#include "filters.h"

/**
 ** search_filter() - SEARCH INTERNAL FILTER TABLE FOR FILTER BY NAME
 **/

_FILTER *
#if	defined(__STDC__)
search_filter (
	char *			name
)
#else
search_filter (name)
	register char		*name;
#endif
{
	register _FILTER	*pf;

	for (pf = filters; pf->name; pf++)
		if (STREQU(pf->name, name))
			break;
	return (pf->name? pf : 0);
}

/*		copyright	"%c%" 	*/

#ident	"@(#)trash.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdlib.h"

#include "lp.h"
#include "filters.h"

/**
 ** trash_filters() - FREE ALL SPACE ALLOCATED FOR FILTER TABLE
 **/

void			trash_filters ()
{
	register _FILTER	*pf;

	if (filters) {
		for (pf = filters; pf->name; pf++)
			free_filter (pf);
		Free ((char *)filters);
		nfilters = 0;
		filters = 0;
	}
	return;
}

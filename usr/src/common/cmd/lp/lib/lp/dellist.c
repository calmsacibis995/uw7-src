/*		copyright	"%c%" 	*/

#ident	"@(#)dellist.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** dellist() - REMOVE ITEM FROM (char **) LIST
 **/

int
#if	defined(__STDC__)
dellist (
	char ***		plist,
	char *			item
)
#else
dellist (plist, item)
	register char		***plist,
				*item;
#endif
{
	register char		**pl;

	register int		n;

	if (*plist) {

		n = lenlist(*plist);

		for (pl = *plist; *pl; pl++)
			if (STREQU(*pl, item))
				break;

		if (*pl) {
			Free (*pl);
			for (; *pl; pl++)
				*pl = *(pl+1);
			if (--n == 0) {
				Free ((char *)*plist);
				*plist = 0;
			} else {
				*plist = (char **)Realloc(
					(char *)*plist,
					(n + 1) * sizeof(char *)
				);
				if (!*plist) {
					errno = ENOMEM;
					return (-1);
				}
/*				(*plist)[n] = 0; /* done in "for" loop */
			}
		}

	}

	return (0);
}

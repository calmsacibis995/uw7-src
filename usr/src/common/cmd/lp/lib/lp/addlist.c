/*		copyright	"%c%" 	*/


#ident	"@(#)addlist.c	1.2"
#ident  "$Header$"

#include "string.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"

/*
**  addlist() - ADD ITEM TO (char **) LIST
**
**  Do not allow duplicate items.
*/

int
#if	defined(__STDC__)
addlist (
	char ***		plist,
	char *			item
)
#else
addlist (plist, item)
	register char		***plist;
	char			*item;
#endif
{
	register char		**pl;

	register int		n;

	if (*plist) {

		n = lenlist(*plist);

		for (pl = *plist; *pl; pl++)
			if (STREQU(*pl, item))
				break;

		if (!*pl) {

			n++;
			*plist = (char **)Realloc(
				(char *)*plist,
				(n + 1) * sizeof(char *)
			);
			if (!*plist) {
				errno = ENOMEM;
				return (-1);
			}
			(*plist)[n - 1] = Strdup(item);
			(*plist)[n] = 0;

		}

	} else {

		*plist = (char **)Malloc(2 * sizeof(char *));
		if (!*plist) {
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[0] = Strdup(item);
		(*plist)[1] = 0;

	}

	return (0);
}

/*
**  addlist2() - ADD ITEM TO (char **) LIST
**
**  Do not check for duplicate items.
*/

int
#if	defined(__STDC__)
addlist2 (char ***plist, char *item)
#else
addlist2 (plist, item)

register
char ***	plist;
char *		item;
#endif
{
	register int		n;

	if (*plist)
	{
		n = lenlist (*plist);

		n++;
		*plist = (char **)Realloc((char *)*plist,
			(n + 1) * sizeof(char *));
		if (!*plist)
		{
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[n - 1] = Strdup(item);
		(*plist)[n] = 0;
	}
	else
	{
		*plist = (char **)Malloc(2 * sizeof(char *));
		if (!*plist)
		{
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[0] = Strdup (item);
		(*plist)[1] = 0;

	}
	return	0;
}

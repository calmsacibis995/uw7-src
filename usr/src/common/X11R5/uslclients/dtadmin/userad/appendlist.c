/*		copyright	"%c%" 	*/

#ident	"@(#)dtadmin:userad/appendlist.c	1.1"

#include "string.h"
#include "errno.h"
#include "sys/types.h"
#include "stdlib.h"

#include "findlocales.h"


/**
 ** lenlist() - COMPUTE LENGTH OF LIST
 **/

int
#if	defined(__STDC__)
lenlist (
	char **			list
)
#else
lenlist (list)
	char			**list;
#endif
{
	register char **	pl;

	if (!list)
		return (0);
	for (pl = list; *pl; pl++)
		;
	return (pl - list);
}


/**
 ** appendlist() - ADD ITEM TO (char **) LIST
 **/

int
#if	defined(__STDC__)
appendlist (
	char ***		plist,
	char *			item
)
#else
appendlist (plist, item)
	register char		***plist,
				*item;
#endif
{
	register int		n;

	if (*plist) {

		n = lenlist(*plist);

		n++;
		*plist = (char **)realloc(
			(char *)*plist,
			(n + 1) * sizeof(char *)
		);
		if (!*plist) {
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[n - 1] = strdup(item);
		(*plist)[n] = 0;

	} else {

		*plist = (char **)malloc(2 * sizeof(char *));
		if (!*plist) {
			errno = ENOMEM;
			return (-1);
		}
		(*plist)[0] = strdup(item);
		(*plist)[1] = 0;

	}

	return (0);
}

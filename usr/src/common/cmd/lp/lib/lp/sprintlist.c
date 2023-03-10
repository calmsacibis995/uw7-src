/*		copyright	"%c%" 	*/

#ident	"@(#)sprintlist.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** sprintlist() - FLATTEN (char **) LIST INTO (char *) LIST
 **/

char *
#if	defined(__STDC__)
sprintlist (
	char **			list
)
#else
sprintlist (list)
	char			**list;
#endif
{
	register char		**plist,
				*p,
				*q;

	char			*ret;

	int			len	= 0;


	if (!list || !*list)
		return (0);

	for (plist = list; *plist; plist++)
		len += strlen(*plist) + 1;

	if (!(ret = Malloc(len))) {
		errno = ENOMEM;
		return (0);
	}

	q = ret;
	for (plist = list; *plist; plist++) {
		p = *plist;
		while (*q++ = *p++)
			;
		q[-1] = ' ';
	}
	q[-1] = 0;

	return (ret);
}

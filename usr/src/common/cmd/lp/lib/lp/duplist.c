/*		copyright	"%c%" 	*/

#ident	"@(#)duplist.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "errno.h"
#include "string.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** duplist() - DUPLICATE A LIST OF STRINGS
 **/

char **
#if	defined(__STDC__)
duplist (
	char **			src
)
#else
duplist (src)
	register char		**src;
#endif
{
	register char		**dst;

	register int		nitems,
				n;

	if (!src || !*src)
		return (0);

	nitems = lenlist(src);
	if (!(dst = (char **)Malloc((nitems + 1) * sizeof(char *)))) {
		errno = ENOMEM;
		return (0);
	}

	for (n = 0; n < nitems; n++)
		if (!(dst[n] = Strdup(src[n]))) {
			while (n--)
				Free (dst[n]);
			Free ((char *)dst);
			errno = ENOMEM;
			return (0);
		} 
	dst[nitems] = 0;

	return (dst);
}


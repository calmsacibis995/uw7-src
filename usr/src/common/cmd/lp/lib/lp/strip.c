/*		copyright	"%c%" 	*/

#ident	"@(#)strip.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

/**
 ** strip() - STRIP LEADING AND TRAILING BLANKS
 **/

char *
#if	defined(__STDC__)
strip (
	char *			str
)
#else
strip (str)
	register char		*str;
#endif
{
	register char		*p;

	if (!str || !*str)
		return (0);

	str += strspn(str, " ");
	for (p = str; *p; p++)
		;
	p--;
	for (; p >= str && *p == ' '; p--)
		;
	*++p = 0;
	return (str);
}

/*		copyright	"%c%" 	*/

#ident	"@(#)cs_strcmp.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "ctype.h"

/*
 * Compare strings ignoring case:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

int
#if	defined(__STDC__)
cs_strcmp(
	char *			s1,
	char *			s2
)
#else
cs_strcmp(s1, s2)
register char *s1, *s2;
#endif
{

	if(s1 == s2)
		return(0);
	while(toupper(*s1) == toupper(*s2++))
		if(*s1++ == '\0')
			return(0);
	return(toupper(*s1) - toupper(*--s2));
}

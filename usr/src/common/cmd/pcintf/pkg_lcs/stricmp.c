#ident	"@(#)pcintf:pkg_lcs/stricmp.c	1.2"
/* SCCSID(@(#)stricmp.c	7.1	LCC)	* Modified: 15:35:21 10/15/90 */
/*
 *  stricmp function
 */

#include <ctype.h>


stricmp(s1, s2)
register char *s1, *s2;
{
	if (s1 == s2)
		return 0;
	while (tolower(*s1) == tolower(*s2)) {
		if (*s1++ == '\0')
			return 0;
		s2++;
	}
	return *s1 - *s2;
}

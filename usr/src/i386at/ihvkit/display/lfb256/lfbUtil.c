#ident	"@(#)ihvkit:display/lfb256/lfbUtil.c	1.1"

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#include <ctype.h>

int lfbstrcasecmp(s1, s2)
register char *s1, *s2;

{
    while (tolower(*s1) == tolower(*s2)) {
	if (*s1 == '\0')
	    return(0);

	s1++;
	s2++;
    }

    return(*s1 - *s2);
}

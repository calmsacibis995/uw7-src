/*
 * Copyright 1990, 1991, 1992 by the Massachusetts Institute of Technology and
 * UniSoft Group Limited.
 * 
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the names of MIT and UniSoft not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  MIT and UniSoft
 * make no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * $XConsortium: strtok.c,v 1.2 92/07/01 11:59:26 rws Exp $
 */


/*
 * A version of strtok for systems that do not have it
 */
char *
strtok(s1, sep)
char	*s1;
char	*sep;
{
static	char	*savpos;
int 	n;

	if (s1)
		savpos = s1;
	else
		s1 = savpos;

	/* Skip over leading separators */
	n = strspn(s1, sep);
	s1 += n;

	n = strcspn(s1, sep);
	if (n == 0)
		return((char *) 0);

	savpos = s1 + n;
	if (*savpos != '\0')
		*savpos++ = '\0';

	return(s1);
}

#ifdef test
#include <stdio.h>
main()
{
char *s;
char	*strtok();

	s = strtok("this is   :, . string", " :,");
	do {
		printf("%s\n", s);
		fflush(stdout);
	} while ((s = strtok((char*)0, " :,")) != 0);

}
#endif

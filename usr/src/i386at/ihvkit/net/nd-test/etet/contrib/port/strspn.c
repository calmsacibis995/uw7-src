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
 * $XConsortium: strspn.c,v 1.2 92/07/01 11:59:23 rws Exp $
 */
/*
 * returns the number of contiguous characters from the beginning
 * of str that are in set.
 * 
 * A simple emulation provided for (eg BSD) systems that do not provide
 * this routine.
 */
int
strspn(str, set)
char	*str;
char	*set;
{
char	*cp, *sp;

	for (cp = str; *cp; cp++) {
		for (sp = set; *sp && *cp != *sp; sp++)
			;

		if (*sp == '\0')
			break;

	}
	return(cp - str);
}

/*
 * returns the number of contiguous characters from the beginning
 * of str that are not in set.
 * 
 * A simple emulation provided for (eg BSD) systems that do not provide
 * this routine.
 */
int
strcspn(str, set)
char	*str;
char	*set;
{
char	*cp, *sp;

	for (cp = str; *cp; cp++) {
		for (sp = set; *sp && *cp != *sp; sp++)
			;

		if (*sp != '\0')
			break;

	}
	return(cp - str);
}

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
 * $XConsortium: toupper.c,v 1.2 92/07/01 11:59:28 rws Exp $
 */


/*
 * A function version of toupper().
 * The BSD4.2 version of toupper in ctype.h does
 * not leave non-lowercase letters unchanged.
 */
toupper(c)
int 	c;
{
	return(upcase(c));
}

tolower(c)
int 	c;
{
	return(locase(c));
}


#include <ctype.h>

upcase(c)
int 	c;
{
	if (islower(c))
		return(toupper(c));
	else
		return(c);
}

locase(c)
int 	c;
{
	if (isupper(c))
		return(tolower(c));
	else
		return(c);
}

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
 * $XConsortium: strpbrk.c,v 1.2 92/07/01 11:59:20 rws Exp $
 */
/* A version of strpbrk for systems without it. */

/* Return pointer to first occurence of any character from `bset' */
/* in `string'. NULL is returned if none exists. */

#ifndef	NULL
#define	NULL	(char *) 0
#endif

char *
strpbrk(str, bset)
char *str;
char *bset;
{
	char *ptr;

	do {
		for(ptr=bset; *ptr != '\0' && *ptr != *str; ++ptr)
			;
		if(*ptr != '\0')
			return(str);
	}
	while(*str++);
	return(NULL);
}

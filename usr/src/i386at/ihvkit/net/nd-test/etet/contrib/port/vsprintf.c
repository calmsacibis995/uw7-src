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
 * $XConsortium: vsprintf.c,v 1.2 92/07/01 11:59:31 rws Exp $
 */

/*
 * This is a very simple emulation for BSD with the standard compiler
 * It is limited to 6 args.
 * Do NOT use if the system posesses vsprintf()
 * NB. This is not even necessarily portable even to all BSD systems.
 * This is only required temporarily until the tet_infoline() issue
 * is resolved.
 */

#include <varargs.h>

vsprintf(buf, fmt, args)
char	*buf;
char	*fmt;
va_list	args;
{
int 	a1, a2, a3, a4, a5, a6;

	a1  = va_arg(args, int);
	a2  = va_arg(args, int);
	a3  = va_arg(args, int);
	a4  = va_arg(args, int);
	a5  = va_arg(args, int);
	a6  = va_arg(args, int);

	sprintf(buf, fmt, a1, a2, a3, a4, a5, a6);

}


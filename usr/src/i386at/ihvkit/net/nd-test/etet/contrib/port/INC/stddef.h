/*
 * Copyright 1990, 1991 by the Massachusetts Institute of Technology and
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
 * $XConsortium: stddef.h,v 1.2 92/06/11 15:30:18 rws Exp $
 */

#ifndef	_STDDEF_H
#define	_STDDEF_H

typedef	int		ptrdiff_t;	/* difference of two pointers */

#ifndef _WCHAR_T
#define _WCHAR_T
typedef	unsigned char	wchar_t;	/* size of largest character */
#endif

#ifndef	_SIZE_T
#define	_SIZE_T
typedef	unsigned int	size_t;		/* type of sizeof */
#endif

/*
	Null pointer.
 */
#ifndef NULL
#define	NULL 0
#endif

/*
	Offset in bytes of a member of the specified type.
 */
#define	offsetof(strct_t, member) (size_t)&(((strct_t *)0)->member)

#endif

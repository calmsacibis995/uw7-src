/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/wgetchar.c	1.1.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vtdefs.h"
#include	"vt.h"
#include	"token.h"

token
wgetchar()
{
	register struct vt	*v;

	v = &VT_array[VT_curid];
	return (token) wgetch(v->win);
}

/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/wdelchar.c	1.1.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vtdefs.h"
#include	"vt.h"

void
wdelchar()
{
	register struct	vt	*v;

	v = &VT_array[VT_curid];
	v->flags |= VT_DIRTY;
	if (!(v->flags & VT_NOBORDER)) {
		/*
		 * insert character before border
		 * (not necessary yet, handled in fields)
		 */
		 ;
	}
	wdelch(v->win);
}

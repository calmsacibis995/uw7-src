/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/wgo.c	1.1.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vtdefs.h"
#include	"vt.h"

void
wgo(r, c)
unsigned	r;
unsigned	c;
{
	register struct vt	*v;
	int	mr, mc;

	v = &VT_array[VT_curid];
	getmaxyx(v->win, mr, mc);
	if (!(v->flags & VT_NOBORDER)) {
		r++;
		c++;
		mr--;
		mc--;
	}
	if (r > mr || c > mc)
		return;
	wmove(v->win, r, c);
	v->flags |= VT_DIRTY;
}

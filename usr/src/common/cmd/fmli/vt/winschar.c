/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/winschar.c	1.3.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vtdefs.h"
#include	"vt.h"

void
winschar(ch, attr)
char ch;
unsigned attr;
{
	register struct	vt	*v;

	v = &VT_array[VT_curid];
	v->flags |= VT_DIRTY;
	if (!(v->flags & VT_NOBORDER)) {
		/*
		 * delete character before border
		 * (not necessary yet, handled in fields)
		 */
		 ;
	}
	winsch(v->win, ch | attr);
}

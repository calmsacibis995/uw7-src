/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/move.c	1.1.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vt.h"
#include	"vtdefs.h"

/*
 * moves a vt to row, col
 */
vt_move(newrow, newcol)
unsigned	newrow;
unsigned	newcol;
{
	register struct vt	*v;
	int	n;
	int	row;
	int	col;
	extern unsigned	VT_firstline;

	n = VT_curid;
	v = &VT_array[n];
	getmaxyx(v->win, row, col);
	if (off_screen(newrow, newcol, row, col))
		return FAIL;
	_vt_hide(n, FALSE);
	mvwin(v->win, newrow + VT_firstline, newcol);
	vt_current(n);
	return TRUE;
}

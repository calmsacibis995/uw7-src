/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/redraw.c	1.1.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vt.h"
#include	"vtdefs.h"

vt_redraw()
{
	wrefresh(curscr);
}

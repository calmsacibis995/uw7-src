/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/indicator.c	1.2.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vt.h"
#include	"vtdefs.h"

void
indicator(message, col)
char *message;
int col;
{
	WINDOW		*win;

	win = VT_array[ STATUS_WIN ].win;
	/* error check */
/* abs: change output routine to one that handles escape sequences
	mvwaddstr(win, 0, col, message);
*/
	wmove(win, 0, col);
	wputs(message, win);
/*****/
	wnoutrefresh( win );
	doupdate();
}

/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/working.c	1.4.4.3"

#include	<curses.h>
#include	"wish.h"
#include	"vt.h"
#include	"vtdefs.h"

/* set in if_init.c */
extern int	Work_col;
extern char	*Work_msg;

/*
 * puts up or removes "Working" message on status line at "Work_col"
 */

void
working(flag)
bool	flag;
{
	register vt_id	oldvid;
/* new */
	WINDOW		*win;

	win = VT_array[ STATUS_WIN ].win;
	if (flag)
		mvwaddstr(win, 0, Work_col, Work_msg);
	else {
		wmove(win, 0, Work_col);
		wclrtoeol(win);		/* assumes right-most! */
	}
	wnoutrefresh( win );
	if ( flag )
		doupdate();
/***/
/*
	oldvid = vt_current(STATUS_WIN);
	wgo(0, Work_col);
	wputs(flag ? Work_msg: blanks, NULL);
	vt_current(oldvid);
	if (flag)
		flush_output();
*/
}

#ident	"@(#)display.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)display.c	5.3 (Berkeley)	6/29/88
 */

/*	Convergent Technologies - System V - Aug 1987	*/
#ident	"@(#)display.c	1.1 :/source/net/cmd/talk/s.display.c 8/22/87 22:39:14"


/*
 * The window 'manager', initializes curses and handles the actual
 * displaying of text
 */
#include <sys/types.h>
#include "talk.h"

xwin_t	my_win;
xwin_t	his_win;
WINDOW	*line_win;

int	curses_initialized = 0;

/*
 * max HAS to be a function, it is called with
 * a argument of the form --foo at least once.
 */
max(a,b)
	int a, b;
{

	return (a > b ? a : b);
}

/*
 * Display some text on somebody's window, processing some control
 * characters while we are at it.
 */
display(win, text, size)
	register xwin_t *win;
	register char *text;
	int size;
{
	register int i;
	char cch;

	for (i = 0; i < size; i++) {
		if (*text == '\n') {
			xscroll(win, 0);
			text++;
			continue;
		}
		/* erase character */
		if (*text == win->cerase) {
			wmove(win->x_win, win->x_line, max(--win->x_col, 0));
			getyx(win->x_win, win->x_line, win->x_col);
			waddch(win->x_win, ' ');
			wmove(win->x_win, win->x_line, win->x_col);
			getyx(win->x_win, win->x_line, win->x_col);
			text++;
			continue;
		}
		/*
		 * On word erase search backwards until we find
		 * the beginning of a word or the beginning of
		 * the line.
		 */
		if (*text == win->werase) {
			int endcol, xcol, i, c;

			endcol = win->x_col;
			xcol = endcol - 1;
			while (xcol >= 0) {
				c = readwin(win->x_win, win->x_line, xcol);
				if (c != ' ')
					break;
				xcol--;
			}
			while (xcol >= 0) {
				c = readwin(win->x_win, win->x_line, xcol);
				if (c == ' ')
					break;
				xcol--;
			}
			wmove(win->x_win, win->x_line, xcol + 1);
			for (i = xcol + 1; i < endcol; i++)
				waddch(win->x_win, ' ');
			wmove(win->x_win, win->x_line, xcol + 1);
			getyx(win->x_win, win->x_line, win->x_col);
			continue;
		}
		/* line kill */
		if (*text == win->kill) {
			wmove(win->x_win, win->x_line, 0);
			wclrtoeol(win->x_win);
			getyx(win->x_win, win->x_line, win->x_col);
			text++;
			continue;
		}
		if (*text == '\f') {
			if (win == &my_win)
				wrefresh(curscr);
			text++;
			continue;
		}
		if (win->x_col == COLS-1) {
			/* check for wraparound */
			xscroll(win, 0);
		}
		if (*text < ' ' && *text != '\t') {
			waddch(win->x_win, '^');
			getyx(win->x_win, win->x_line, win->x_col);
			if (win->x_col == COLS-1) /* check for wraparound */
				xscroll(win, 0);
			cch = (*text & 63) + 64;
			waddch(win->x_win, cch);
		} else
			waddch(win->x_win, *text);
		getyx(win->x_win, win->x_line, win->x_col);
		text++;
	}
	wrefresh(win->x_win);
}

/*
 * Read the character at the indicated position in win
 */
readwin(win, line, col)
	WINDOW *win;
{
	int oldline, oldcol;
	register int c;

	getyx(win, oldline, oldcol);
	wmove(win, line, col);
	c = winch(win);
	wmove(win, oldline, oldcol);
	return (c);
}

/*
 * Scroll a window, blanking out the line following the current line
 * so that the current position is obvious
 */
xscroll(win, flag)
	register xwin_t *win;
	int flag;
{

	if (flag == -1) {
		wmove(win->x_win, 0, 0);
		win->x_line = 0;
		win->x_col = 0;
		return;
	}
	win->x_line = (win->x_line + 1) % win->x_nlines;
	win->x_col = 0;
	wmove(win->x_win, win->x_line, win->x_col);
	wclrtoeol(win->x_win);
	wmove(win->x_win, (win->x_line + 1) % win->x_nlines, win->x_col);
	wclrtoeol(win->x_win);
	wmove(win->x_win, win->x_line, win->x_col);
}

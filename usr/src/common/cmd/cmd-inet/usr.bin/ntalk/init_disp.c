#ident	"@(#)init_disp.c	1.2"
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
 *	@(#)init_disp.c	5.3 (Berkeley)	6/29/88
 */

/*	Convergent Technologies - System V - Aug 1987	*/
#ident	"@(#)init_disp.c	1.2 :/source/net/cmd/talk/s.init_disp.c 6/29/88 13:53:20"


/*
 * Initialization code for the display package,
 * as well as the signal handling routines.
 */

#include <sys/types.h>
#include "talk.h"
#include <signal.h>

/* 
 * Set up curses, catch the appropriate signals,
 * and build the various windows.
 */
init_display()
{
	void sig_sent();
	struct termio tv;
#ifdef BSD
	struct sigvec sigv;
#endif

	initscr();
#ifdef BSD
	(void) sigvec(SIGTSTP, (struct sigvec *)0, &sigv);
	sigv.sv_mask |= sigmask(SIGALRM);
	(void) sigvec(SIGTSTP, &sigv, (struct sigvec *)0);
#endif
	curses_initialized = 1;
	clear();
	refresh();
	noecho();
	crmode();
	ioctl(0, TCGETA, &tv);
	if ( ! (tv.c_iflag & ICRNL)) {
		tv.c_iflag |= ICRNL;		/* should crmode do this? */
		ioctl(0, TCSETA, &tv);
	}
	signal(SIGINT, sig_sent);
	signal(SIGPIPE, sig_sent);
	/* curses takes care of ^Z */
	my_win.x_nlines = LINES / 2;
	my_win.x_ncols = COLS;
	my_win.x_win = newwin(my_win.x_nlines, my_win.x_ncols, 0, 0);
	scrollok(my_win.x_win, FALSE);
	wclear(my_win.x_win);

	his_win.x_nlines = LINES / 2 - 1;
	his_win.x_ncols = COLS;
	his_win.x_win = newwin(his_win.x_nlines, his_win.x_ncols,
	    my_win.x_nlines+1, 0);
	scrollok(his_win.x_win, FALSE);
	wclear(his_win.x_win);

	line_win = newwin(1, COLS, my_win.x_nlines, 0);
	{
		register int i;
		for (i = 0; i < COLS; i++)
			wprintw(line_win,"-");
	}
	wrefresh(line_win);
	/* let them know we are working on it */
	current_state = "No connection yet";
}

/*
 * Trade edit characters with the other talk. By agreement
 * the first three characters each talk transmits after
 * connection are the three edit characters.
 */
set_edit_chars()
{
	char buf[3];
	int cc;
#ifdef BSD
	struct sgttyb tty;
	struct ltchars ltc;
	
	ioctl(0, TIOCGETP, &tty);
	ioctl(0, TIOCGLTC, (struct sgttyb *)&ltc);
	my_win.cerase = tty.sg_erase;
	my_win.kill = tty.sg_kill;
	if (ltc.t_werasc == (char) -1)
		my_win.werase = '\027';	 /* control W */
	else
		my_win.werase = ltc.t_werasc;
#else
	struct	termio	tv;

	ioctl(0, TCGETA, &tv);
	my_win.cerase = tv.c_cc[VERASE];
	my_win.kill = tv.c_cc[VKILL];
	my_win.werase = '\027';		/* control-W, fixed */
#endif

	buf[0] = my_win.cerase;
	buf[1] = my_win.kill;
	buf[2] = my_win.werase;
	cc = write(sockt, buf, sizeof(buf));
	if (cc != sizeof(buf) )
		p_error("Lost the connection");
	cc = read(sockt, buf, sizeof(buf));
	if (cc != sizeof(buf) )
		p_error("Lost the connection");
	his_win.cerase = buf[0];
	his_win.kill = buf[1];
	his_win.werase = buf[2];
}

void
sig_sent()
{

	message("Connection closing. Exiting");
	quit();
}

/*
 * All done talking...hang up the phone and reset terminal thingy's
 */
quit()
{

	if (curses_initialized) {
		wmove(his_win.x_win, his_win.x_nlines-1, 0);
		wclrtoeol(his_win.x_win);
		wrefresh(his_win.x_win);
		endwin();
	}
	if (invitation_waiting)
		send_delete();
	exit(0);
}


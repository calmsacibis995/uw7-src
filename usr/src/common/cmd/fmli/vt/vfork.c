/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/vfork.c	1.5.4.4"

#include	<curses.h>
#include	<term.h>
#include	"wish.h"

/* Functions for use before and after forking processes */
extern int ShellInterrupt;

void
vt_before_fork()
{
	ShellInterrupt = TRUE;
	endwin();
}

void
vt_after_fork()
{
	/*
	 * Reset color pairs upon return from UNIX ....
	 * If this isn't a color terminal then set_def_colors()
	 * returns without doing anything
	 *
	 * Also re-set mouse information (vinit.c)
	 */
        /*
         * Reset PFK for terminals like DMD and 5620
         */
        init_sfk(FALSE);
	set_def_colors();
	set_mouse_info();
}

void
fork_clrscr()
{
	putp(tparm(clear_screen));
	fflush(stdout);
}

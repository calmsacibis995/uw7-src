/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)talk.h	1.2"
#ident  "$Header$"

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


#include <curses.h>
#include <termios.h>
#include <utmp.h>

#define	forever		for(;;)

#define	BUF_SIZE	512

FILE *popen();
int quit(), sleeper();

extern int sockt;
extern int curses_initialized;
extern int invitation_waiting;

extern char *current_state;
extern int current_line;

typedef struct xwin {
	WINDOW *x_win;
	int x_nlines;
	int x_ncols;
	int x_line;
	int x_col;
	char kill;
	char cerase;
	char werase;
} xwin_t;

extern xwin_t my_win;
extern xwin_t rem_win;
extern WINDOW *line_win;

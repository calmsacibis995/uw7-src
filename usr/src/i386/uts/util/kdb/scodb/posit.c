#ident	"@(#)kern-i386:util/kdb/scodb/posit.c	1.1.2.1"
#ident  "$Header$"
/*
 *	Copyright (C) The Santa Cruz Operation, 1989-1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 * Modification History
 *
 *	L000	nadeem@sco.com	13dec94
 *	- modifications for user level scodb:
 *	- user level versions of p_row() and p_col() return the row/column
 *	  from internal variables,
 *	- wrote new routine get_curpos() to return the cursor position.  This
 *	  works on either the console, or on an xterm/scoterm session.
 */


#ifdef USER_LEVEL						/* L000v */

#include "sys/termios.h"
extern int errno, cur_row, cur_col;
int use_ioctl = 1;

NOTSTATIC
p_row()
{
	return(cur_row-1);
}

NOTSTATIC
p_col()
{
	return(cur_col-1);
}

/*
 * Return the current cursor position.  Firstly, the CONS_GETINFO ioctl
 * is attempted on standard output.  If this succeeds, then we are
 * running on the console.  Part of the information returned is the
 * current cursor position.  If this ioctl fails, however, then we
 * assume that we are running on a scoterm/xterm session.  In this case,
 * the cursor position is obtained by sending out a special escape sequence
 * to the terminal emulator.  The terminal emulator responds by sending
 * back an escape with the current cursor position.
 *
 * This routine is called once when scodb starts up to obtain the physical
 * cursor position.  Thereafter, scodb keeps track of the cursor position
 * itself according to the characters and escape sequences that it outputs
 * to the screen.
 */

get_curpos(int *row, int *col)
{
	{
		/*
		 * Assume that we are on a scoterm/xterm session.  The
		 * escape sequence "<ESC>[6n" will request the current cursor
		 * position from the emulator.  The return string is of
		 * the form "<ESC>[<row>;<col>R".
		 */

		char line[80];
		char *s;
		int done = 0, retries = 10;

		while (!done && --retries > 0) {
			s = line;

			printf("\033[6n");

			while ((*s = getchar()) != 'R')
				s++;
	
			*s = '\0';
			sscanf(line+2, "%d;%d", row, col);

			/*
			 * The correct return string was not received - flush
			 * the input queue and try again.
			 */

			if (line[0] != '\033' || line[1] != '[') {
				ioctl(1, TCFLSH, 0);
			} else
				done = 1;
		}

		if (!done) {
			printf("Could not obtain cursor position\n");
			*row = 1;
			*col = 1;
		}
	}
}

#else								/* L000^ */

#include <io/ws/ws.h>

extern ws_channel_t Kd0chan;

NOTSTATIC
p_row() {
	return (Kd0chan.ch_tstate.t_row);
}

NOTSTATIC
p_col() {
	return (Kd0chan.ch_tstate.t_col);
}

#endif								/* L000 */


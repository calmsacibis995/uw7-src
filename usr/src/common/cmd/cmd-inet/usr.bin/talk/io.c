/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)io.c	1.3"
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


/* this file contains the I/O handling and the exchange of 
   edit characters. This connection itself is established in
   ctl.c
 */

#include "talk.h"
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/filio.h>

#define A_LONG_TIME 10000000
extern int errno;

/*
 * The routine to do the actual talking
 */

talk()
{
    int nfds, nb;
    fd_set read_set, read_template;
    char buf[BUFSIZ];
    struct timeval wait;

    beep();
    message("Connection established");
    current_line = 0;

    /* Wait on both the other process (sockt) and standard input */
    FD_ZERO(&read_template);
    FD_SET(sockt, &read_template);
    FD_SET(fileno(stdin), &read_template);
    nfds = (sockt > fileno(stdin)) ? sockt : fileno(stdin);
    nfds++;

    forever {

	read_set = read_template;

	wait.tv_sec = A_LONG_TIME;
	wait.tv_usec = 0;

	nb = select(nfds, &read_set, 0, 0, &wait);

	if (nb <= 0) {

		/* We may be returning from an interupt handler */

	    if (errno == EINTR) {
		read_set = read_template;
		continue;
	    } else {
		    /* panic, we don't know what happened */
		p_error("Unexpected error from select");
		quit();
	    }
	}

	if (FD_ISSET(sockt, &read_set)) { 

		/* There is data on sockt */
	    nb = read(sockt, buf, sizeof buf);

	    if (nb <= 0) {
		message("Connection closed. Exiting");
		quit();
	    } else {
		display(&rem_win, buf, nb);
	    }
	}
	
	if (FD_ISSET(fileno(stdin), &read_set)) {

		/* we can't make the tty non_blocking, because
		   curses's output routines would screw up */

	    ioctl(0, FIONREAD, (struct sgttyb *) &nb);
	    nb = read(0, buf, nb);

	    display(&my_win, buf, nb);
	    write(sockt, buf, nb);	/* We might lose data here
					   because sockt is non-blocking
					 */

	}
    }
}

extern int	errno;
extern int	sys_nerr;
extern char	*sys_errlist[];

    /* p_error prints the system error message on the standard location
       on the screen and then exits. (i.e. a curses version of perror)
     */

p_error(string) 
char *string;
{
    char *sys;

    sys = "Unknown error";
    if(errno < sys_nerr) {
	sys = sys_errlist[errno];
    }


    wmove(my_win.x_win, current_line%my_win.x_nlines, 0);
    wprintw(my_win.x_win, "[%s : %s (%d)]\n", string, sys, errno);
    wrefresh(my_win.x_win);
    move(LINES-1, 0);
    refresh();
    quit();
}

    /* display string in the standard location */

message(string)
char *string;
{
    wmove(my_win.x_win, current_line%my_win.x_nlines, 0);
    wprintw(my_win.x_win, "[%s]\n", string);
    wrefresh(my_win.x_win);
}

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)get_names.c	1.3"
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


#include "talk.h"
#include "ctl.h"

#ifdef SYSV
#define	rindex	strrchr
#endif /* SYSV */

char *getlogin(), *ttyname(), *rindex();

extern CTL_MSG msg;

/*
 * Determine the local and remote user, tty, and machines
 */

struct hostent *gethostbyname();

get_names(argc, argv)
int argc;
char *argv[];
{
    char hostname[MAXHOSTNAMELEN + 1];
    char *rem_name;
    char *my_name;
    char *my_machine_name;
    char *rem_machine_name;
    char *my_tty;
    char *rem_tty;
    char *ptr;

    if (argc < 2 ) {
	printf("Usage:  talk user [ttyname]\n");
	exit(-1);
    }
    if ( !isatty(0) ) {
	printf("Standard input must be a tty, not a pipe or a file\n");
	exit(-1);
    }

    my_name = getlogin();
    if (my_name == NULL) {
	printf("Who are you?  You have no entry in /etc/utmp!  Aborting...\n");
	exit(-1);
    }

    gethostname(hostname, sizeof (hostname) - 1);
    hostname[sizeof (hostname) - 1] = '\0';
    my_machine_name = hostname;

    my_tty = rindex(ttyname(0), '/') + 1;

	/* check for, and strip out, the machine name 
	    of the target */

    for (ptr = argv[1]; *ptr != '\0' &&
			 *ptr != '@' &&
			 *ptr != ':' &&
			 *ptr != '!' &&
			 *ptr != '.'     ; ptr++) {
    }

    if (*ptr == '\0') {

	    /* this is a local to local talk */

	rem_name = argv[1];
	rem_machine_name = my_machine_name;

    } else {

	if (*ptr == '@') {
		/* user@host */
	    rem_name = argv[1];
	    rem_machine_name = ptr + 1;
	} else {
		/* host.user or host!user or host:user */
	    rem_name = ptr + 1;
	    rem_machine_name = argv[1];
	}
	*ptr = '\0';
    }


    if (argc > 2) {
	rem_tty = argv[2];	/* tty name is arg 2 */
    } else {
	rem_tty = "";
    }

    get_addrs(my_machine_name, rem_machine_name);

	/* Load these useful values into the standard message header */

    msg.id_num = 0;

    strncpy(msg.l_name, my_name, NAME_SIZE);
    msg.l_name[NAME_SIZE - 1] = '\0';

    strncpy(msg.r_name, rem_name, NAME_SIZE);
    msg.r_name[NAME_SIZE - 1] = '\0';

    strncpy(msg.r_tty, rem_tty, TTY_SIZE);
    msg.r_tty[TTY_SIZE - 1] = '\0';
}

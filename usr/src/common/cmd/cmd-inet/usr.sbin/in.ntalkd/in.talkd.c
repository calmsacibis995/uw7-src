#ident	"@(#)in.talkd.c	1.4"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
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
 *	@(#)talkd.c	5.4 (Berkeley)	6/18/88
 */

/*
 * The top level of the daemon, the format is heavily borrowed
 * from rwhod.c. Basically: find out who and where you are; 
 * disconnect all descriptors and ttys, and then endless
 * loop on waiting for and processing requests
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <netdb.h>

#include <protocols/talkd.h>

CTL_MSG		request;
CTL_RESPONSE	response;

int	sockt = 0;
int	debug = 0;
int	timeout();
long	lastmsgtime;

char	hostname[MAXHOSTNAMELEN + 1];

#define TIMEOUT 30
#define MAXIDLE 120

main(argc, argv)
	int argc;
	char *argv[];
{
	register CTL_MSG *mp = &request;
	int cc;

	if (getuid()) {
		fprintf(stderr, "%s: getuid: not super-user\n", argv[0]);
		exit(1);
	}
	openlog("talkd", LOG_PID, LOG_DAEMON);
	if (gethostname(hostname, sizeof (hostname) - 1) < 0) {
		syslog(LOG_ERR, "gethostname: %m");
		_exit(1);
	}
	if (chdir("/dev") < 0) {
		syslog(LOG_ERR, "chdir: /dev: %m");
		_exit(1);
	}
	if (argc > 1 && strcmp(argv[1], "-d") == 0)
		debug = 1;
	signal(SIGALRM, (void (*)())timeout);
	alarm(TIMEOUT);
	for (;;) {
		extern int errno;

		cc = recv(0, (char *)mp, sizeof (*mp), 0);
		if (cc != sizeof (*mp)) {
			if (cc < 0 && errno != EINTR)
				syslog(LOG_WARNING, "recv: %m");
			continue;
		}
		lastmsgtime = time((time_t *)0);
		process_request(mp, &response);
		/* can block here, is this what I want? */
		cc = sendto(sockt, (char *)&response,
		    sizeof (response), 0, (struct sockaddr *)&mp->ctl_addr,
		    sizeof (mp->ctl_addr));
		if (cc != sizeof (response))
			syslog(LOG_WARNING, "sendto: %m");
	}
}

timeout()
{

	if (time((time_t *)0) - lastmsgtime >= MAXIDLE)
		_exit(0);
	signal(SIGALRM, (void (*)())timeout);
	alarm(TIMEOUT);
}

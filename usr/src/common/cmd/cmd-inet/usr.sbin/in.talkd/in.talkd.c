/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)in.talkd.c	1.4"
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


/*
 * Invoked by the Internet daemon to handle talk requests
 * Processes talk requests until MAX_LIFE seconds go by with 
 * no action, then dies.
 */

#include "ctl.h"

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

extern	errno;

CTL_MSG request;
CTL_RESPONSE response;

char hostname[MAXHOSTNAMELEN + 1];

int debug = 0;

CTL_MSG swapmsg();

main()
{
    struct sockaddr_in from;
    size_t from_size;
    int cc;
    fd_set rfds, rmask;
    struct timeval tv;

    gethostname(hostname, sizeof (hostname) - 1);
    FD_ZERO(&rmask);
    FD_SET(0, &rmask);

    for (;;) {
	rfds = rmask;
	tv.tv_sec = MAX_LIFE;
	tv.tv_usec = 0;
	if (select(1, &rfds, 0, 0, &tv) <= 0)
		exit(0); 
	from_size = sizeof(from);
	cc = recvfrom(0, (char *)&request, sizeof (request), 0, 
		      (struct sockaddr *)&from, &from_size);

	if (cc != sizeof(request)) {
	    if (cc < 0 && errno != EINTR) {
		print_error("receive");
	    }
	} else {

	    if (debug) printf("Request received : \n");
	    if (debug) print_request(&request);

	    request = swapmsg(request);
	    process_request(&request, &response);

	    if (debug) printf("Response sent : \n");
	    if (debug) print_response(&response);

		/* can block here, is this what I want? */

	    cc = sendto(0, (char *) &response, sizeof(response), 0,
			(struct sockaddr *)&request.ctl_addr,
			sizeof(request.ctl_addr));

	    if (cc != sizeof(response)) {
		print_error("Send");
	    }
	}
    }
}

extern int sys_nerr;
extern char *sys_errlist[];

print_error(string)
char *string;
{
    FILE *cons;
    char *err_dev = "/dev/console";
    char *sys;
    pid_t val, pid;

    if (debug) err_dev = "/dev/tty";

    sys = "Unknown error";

    if(errno < sys_nerr) {
	sys = sys_errlist[errno];
    }

	/* don't ever open tty's directly, let a child do it */
    if ((pid = fork()) == 0) {
	cons = fopen(err_dev, "a");
	if (cons != NULL) {
	    fprintf(cons, "Talkd : %s : %s(%d)\n\r", string, sys,
		    errno);
	    fclose(cons);
	}
	exit(0);
    } else {
	    /* wait for the child process to return */
	do {
	    val = wait(0);
	    if (val == (pid_t)-1) {
		if (errno == EINTR) {
		    continue;
		} else if (errno == ECHILD) {
		    break;
		}
	    }
	} while (val != pid);
    }
}

#define swapshort(a) (((a << 8) | ((unsigned short) a >> 8)) & 0xffff)
#define swaplong(a) ((swapshort(a) << 16) | (swapshort(((unsigned)a >> 16))))

/*  
 * heuristic to detect if need to swap bytes
 */

CTL_MSG
swapmsg(req)
	CTL_MSG req;
{
	CTL_MSG swapreq;
	
	if (req.ctl_addr.sin_family == swapshort(AF_INET)) {
		swapreq = req;
		swapreq.id_num = swaplong(req.id_num);
		swapreq.pid = swaplong(req.pid);
		swapreq.addr.sin_family = swapshort(req.addr.sin_family);
		swapreq.ctl_addr.sin_family =
			swapshort(req.ctl_addr.sin_family);
		return swapreq;
	}
	else
		return req;
}

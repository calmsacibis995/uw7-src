#ident	"@(#)ctl_trans.c	1.3"
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
 * 
 *	@(#)ctl_transact.c	5.4 (Berkeley)	6/29/88
 */

/*	Convergent Technologies - System V - Aug 1987	*/
#ident	"@(#)ctl_trans.c	1.1 :/source/net/cmd/talk/s.ctl_trans.c 8/26/87 19:33:12"


#include "talk_ctl.h"
#include <sys/time.h>

#define CTL_WAIT 2	/* time to wait for a response, in seconds */

/*
 * SOCKDGRAM is unreliable, so we must repeat messages if we have
 * not recieved an acknowledgement within a reasonable amount
 * of time
 */
ctl_transact(target, msg, type, rp)
	struct in_addr target;
	CTL_MSG msg;
	int type;
	CTL_RESPONSE *rp;
{
	fd_set read_mask, ctl_mask;
	int nready, cc;
	struct timeval wait;

	msg.type = type;
	daemon_addr.sin_addr = target;
	daemon_addr.sin_port = daemon_port;
	FD_ZERO(&ctl_mask);
	FD_SET(ctl_sockt, &ctl_mask);

	/*
	 * Keep sending the message until a response of
	 * the proper type is obtained.
	 */
	do {
		wait.tv_sec = CTL_WAIT;
		wait.tv_usec = 0;
		/* resend message until a response is obtained */
		do {
			cc = sendto(ctl_sockt, (char *)&msg, sizeof (msg), 0,
				(struct sockaddr *)&daemon_addr,
				sizeof (daemon_addr));
			if (cc != sizeof (msg)) {
				if (errno == EINTR)
					continue;
				p_error("Error on write to talk daemon");
			}
			read_mask = ctl_mask;
			nready = select(ctl_sockt+1, &read_mask, 0, 0, &wait);
			if (nready < 0) {
				if (errno == EINTR)
					continue;
				p_error("Error waiting for daemon response");
			}
		} while (nready == 0);
		/*
		 * Keep reading while there are queued messages 
		 * (this is not necessary, it just saves extra
		 * request/acknowledgements being sent)
		 */
		do {
			cc = recv(ctl_sockt, (char *)rp, sizeof (*rp), 0);
			if (cc < 0) {
				if (errno == EINTR)
					continue;
				p_error("Error on read from talk daemon");
			}
			read_mask = ctl_mask;
			/* an immediate poll */
			timerclear(&wait);
			nready = select(ctl_sockt+1, &read_mask, 0, 0, &wait);
		} while (nready > 0 && (rp->vers != TALK_VERSION ||
		    rp->type != type));
	} while (rp->vers != TALK_VERSION || rp->type != type);
	rp->id_num = ntohl(rp->id_num);
	rp->addr.sa_family = ntohs(rp->addr.sa_family);
}

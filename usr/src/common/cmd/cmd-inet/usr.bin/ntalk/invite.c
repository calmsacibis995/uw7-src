#ident	"@(#)invite.c	1.3"
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
 *	@(#)invite.c	5.5 (Berkeley)	6/29/88
 */

/*	Convergent Technologies - System V - Aug 1987	*/
#ident	"@(#)invite.c	1.1 :/source/net/cmd/talk/s.invite.c 8/22/87 22:40:08"


#include "talk_ctl.h"
#include <sys/time.h>
#include <signal.h>
#include <setjmp.h>

/*
 * There wasn't an invitation waiting, so send a request containing
 * our sockt address to the remote talk daemon so it can invite
 * him 
 */

/*
 * The msg.id's for the invitations
 * on the local and remote machines.
 * These are used to delete the 
 * invitations.
 */
int	local_id, remote_id;
void	re_invite();
jmp_buf invitebuf;

invite_remote()
{
	int nfd, read_mask, template, new_sockt;
#ifdef BSD
	struct itimerval itimer;
#endif
	CTL_RESPONSE response;

#ifdef BSD
	itimer.it_value.tv_sec = RING_WAIT;
	itimer.it_value.tv_usec = 0;
	itimer.it_interval = itimer.it_value;
#endif
	if (listen(sockt, 5) != 0)
		p_error("Error on attempt to listen for caller");
	msg.addr = *(struct osockaddr *)&my_addr;
	msg.addr.sa_family = htons(my_addr.sin_family);
	msg.id_num = htonl(-1);		/* an impossible id_num */
	invitation_waiting = 1;
	announce_invite();
	/*
	 * Shut off the automatic messages for a while,
	 * so we can use the interupt timer to resend the invitation
	 */
	end_msgs();
#ifdef BSD
	setitimer(ITIMER_REAL, &itimer, (struct itimerval *)0);
#else
	(void) alarm(RING_WAIT);
#endif
	message("Waiting for your party to respond");
	signal(SIGALRM, re_invite);
	(void) setjmp(invitebuf);
	while ((new_sockt = accept(sockt, 0, 0)) < 0) {
		if (errno == EINTR)
			continue;
		p_error("Unable to connect with your party");
	}
	close(sockt);
	sockt = new_sockt;

	/*
	 * Have the daemons delete the invitations now that we
	 * have connected.
	 */
	current_state = "Waiting for your party to respond";
	start_msgs();

	msg.id_num = htonl(local_id);
	ctl_transact(my_machine_addr, msg, DELETE, &response);
	msg.id_num = htonl(remote_id);
	ctl_transact(his_machine_addr, msg, DELETE, &response);
	invitation_waiting = 0;
}

/*
 * Routine called on interupt to re-invite the callee
 */
void
re_invite()
{

	message("Ringing your party again");
	current_line++;
	/* force a re-announce */
	msg.id_num = htonl(remote_id + 1);
	announce_invite();
	longjmp(invitebuf, 1);
}

static	char *answers[] = {
	"answer #0",					/* SUCCESS */
	"Your party is not logged on",			/* NOT_HERE */
	"Target machine is too confused to talk to us",	/* FAILED */
	"Target machine does not recognize us",		/* MACHINE_UNKNOWN */
	"Your party is refusing messages",		/* PERMISSION_REFUSED */
	"Target machine can not handle remote talk",	/* UNKNOWN_REQUEST */
	"Target machine indicates protocol mismatch",	/* BADVERSION */
	"Target machine indicates protocol botch (addr)",/* BADADDR */
	"Target machine indicates protocol botch (ctl_addr)",/* BADCTLADDR */
};
#define	NANSWERS	(sizeof (answers) / sizeof (answers[0]))

/*
 * Transmit the invitation and process the response
 */
announce_invite()
{
	CTL_RESPONSE response;

	current_state = "Trying to connect to your party's talk daemon";
	ctl_transact(his_machine_addr, msg, ANNOUNCE, &response);
	remote_id = response.id_num;
	if (response.answer != SUCCESS) {
		if (response.answer < NANSWERS)
			message(answers[response.answer]);
		quit();
	}
	/* leave the actual invitation on my talk daemon */
	ctl_transact(my_machine_addr, msg, LEAVE_INVITE, &response);
	local_id = response.id_num;
}

/*
 * Tell the daemon to remove your invitation
 */
send_delete()
{

	msg.type = DELETE;
	/*
	 * This is just a extra clean up, so just send it
	 * and don't wait for an answer
	 */
	msg.id_num = htonl(remote_id);
	daemon_addr.sin_addr = his_machine_addr;
	if (sendto(ctl_sockt, &msg, sizeof (msg), 0,
	    (struct sockaddr *)&daemon_addr,
	    sizeof (daemon_addr)) != sizeof(msg))
		perror("send_delete (remote)");
	msg.id_num = htonl(local_id);
	daemon_addr.sin_addr = my_machine_addr;
	if (sendto(ctl_sockt, &msg, sizeof (msg), 0,
	    (struct sockaddr *)&daemon_addr,
	    sizeof (daemon_addr)) != sizeof (msg))
		perror("send_delete (local)");
}

#ident	"@(#)ppp.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: ppp.c,v 1.3 1994/12/12 18:40:50 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <sys/socket.h>

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#ifndef _KERNEL
#define _KERNEL
#include <sys/ksynch.h>
#undef _KERNEL
#else
#include <sys/ksynch.h>
#endif

#include <netinet/ppp_lock.h>
#include <netinet/ppp.h>
#include "pppd.h"
#include "pppu_proto.h"


char *program_name;

int qflag = 0;  /* Do not print banner message at startup */
#define PPP_BANNER "STREAMware PPP 5.1"
#define USAGE() fprintf(stderr, "usage: %s [-q]\n" , program_name)

/* SIGNAL Processing
 * SIGUSR1 -- sent by pppd when it gets a close from kernel or a second
 *           SEND_FD from this process
 * SIGHUP -- sent by tty driver when modem's data carrier detect signal is off
 * SIGQUIT -- ignored
 * SIGTERM -- notify daemon that its being killed so that daemon
 *            can close the connection
 */

int	pid;

/*
 * notify_daemon -
 *  SIGTERM, SIGHUP signal handler
 */
notify_daemon(sig)
	int	sig;
{
	msg_t	msg;
	int	s, rval;

	s = ppp_sockinit();
	if (s < 0) {
		syslog(LOG_INFO, gettxt(":82", "can't connect to pppd"));
		exit(2);
	}

	memset((char *)&msg, 0, sizeof(msg));
	msg.m_type = MPID;
	msg.m_pid = pid;
	rval = write(s, (char *)&msg, sizeof(msg));
	
	if (rval < 0) {
		syslog(LOG_INFO, gettxt(":83", "write to socket failed: %m"));
		exit(2);
	}
	close(s); 

	syslog(LOG_INFO, gettxt(":84", "notify_daemon sig=%d pid=%d"), sig, pid);
	exit(0);
}

/*
 * sig_usr - SIGUSR1 signal handler
 *		pppd sends a SIGUSR1 when it receives a PPCID_CLOSE.
 */
sig_usr(sig)
	int	sig;
{
	syslog(LOG_INFO, gettxt(":85", "sig_usr sig=%d"), sig);
	exit(0);
}

main (argc,argv)
	int	argc;
	char	*argv[];
{
	char *p, *ttyname(), *loginname;
	msg_t msg;
	int	s, rval, userid;
	struct 	passwd *pw;
	char c;

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxppp");
        (void)setlabel("UX:ppp");
	
	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];

        while ((c = getopt(argc, argv, "q")) != EOF) {
                switch (c) {
                case 'q':
			qflag++;
                        break;
                default:
                        USAGE();
                        exit(1);
                }
        }


	sigignore(SIGQUIT);
	sigignore(SIGINT);
	sigset(SIGUSR1, sig_usr);
	sigset(SIGTERM, notify_daemon);
	sigset(SIGHUP, notify_daemon);
	pid = getpid();

#if defined(LOG_DAEMON)
	openlog(program_name, LOG_PID|LOG_CONS|LOG_NDELAY, LOG_DAEMON);
#else
	openlog(program_name, LOG_PID);
#endif

	if (!(p = ttyname(0))) {
		syslog(LOG_INFO, gettxt(":86", "not a tty!?"));
		exit(1);
	}

	syslog(LOG_INFO, gettxt(":87", "login on '%s'"), p);

	/* get the login name, which will be used to get
	 * configurable parameters */
	/*
	 * This will allow login names longer than 8 character
	 * to be used.  
	 *
	 * NOTE: getlogin() truncates login name to 8 characters
	 *       on NCR's SVR4.
	 */

	{
		struct passwd *p_ent;
		
		p_ent = getpwuid(getuid());
		loginname = p_ent->pw_name;
        }

	if (loginname == NULL) {
		syslog(LOG_INFO, gettxt(":88", "can't get login name %m"));
		exit(1);
	}
	
	memset((char *)&msg, 0, sizeof(msg));
	msg.m_type = MTTY;
	msg.m_pid = pid;
	strncpy(msg.m_tty, p, TTY_SIZE);
	strncpy(msg.m_name, loginname, NAME_SIZE);

	s = ppp_sockinit();
	if (s < 0) {
		syslog(LOG_INFO, gettxt(":82", "can't connect to pppd"));
		exit(1);
	}
	rval = write(s, (char *)&msg, sizeof(msg));
	if (rval < 0) {
		syslog(LOG_INFO, gettxt(":83", "write to socket failed: %m"));
		exit(1);
	}
	close(s); 
	syslog(LOG_INFO, gettxt(":89", "sent pppd m_type=%d, m_pid=%d, m_tty='%s'"),
			msg.m_type, msg.m_pid, msg.m_tty);

	/*
	 * Print banner message to let other side know that the
	 * PPP shell is running.
	 */
	if (! qflag)
		printf("%s\n", PPP_BANNER);

	/*
	 * Wait for daemon to notify (signal)
	 * us that we can go away.
	 */
	while(pause() == -1);

	exit(0);
}

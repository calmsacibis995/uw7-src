#ident	"@(#)cmd4.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)cmd4.c	1.10 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * More commands..
 */


static dopipe1 ARGS((char str[], int doign));

/*
 * pipe messages to cmd.
 */

dopipe(str)
	char str[];
{
	return dopipe1(str, 0);
}

doPipe(str)
	char str[];
{
	return dopipe1(str, 1);
}

static dopipe1(str, doign)
	char str[];
{
	register int *ip, mesg;
	register struct message *mp;
	char *cp;
	int f, nowait=0;
	void (*sigint)(), (*sigpipe)();
	long lc, cc, t;
	register pid_t pid;
	int page, s, pivec[2];
	char *Shell;
	FILE *pio;
	int *msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	char *cmd = snarf2(str, &f);

	if (cmd == NOSTR) {
		if (f == -1) {
			pfmt(stdout, MM_ERROR, ":207:pipe command error\n");
			return(1);
			}
		if ( (cmd = value("cmd")) == NOSTR) {
			pfmt(stdout, MM_ERROR, 
				":208:\"cmd\" not set, ignored.\n");
			return(1);
			}
		}
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			pfmt(stdout, MM_ERROR, ":209:No messages to pipe.\n");
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	if (*(cp=cmd+strlen(cmd)-1)=='&'){
		*cp=0;
		nowait++;
		}
	pfmt(stdout, MM_NOSTD, ":210:Pipe to: \"%s\"\n", cmd);
	flush();

					/*  setup pipe */
	if (pipe(pivec) < 0) {
		pfmt(stderr, MM_ERROR, failed, "pipe", Strerror(errno));
		return(0);
	}

	if ((pid = vfork()) == 0) {
		close(pivec[1]);	/* child */
		fclose(stdin);
		dup(pivec[0]);
		close(pivec[0]);
		if ((Shell = value("SHELL")) == NOSTR || *Shell=='\0')
			Shell = SHELL;
		execlp(Shell, Shell, "-c", cmd, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, Shell, Strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	if (pid == (pid_t)-1) {		/* error */
		pfmt(stderr, MM_ERROR, failed, "fork", Strerror(errno));
		close(pivec[0]);
		close(pivec[1]);
		return(0);
	}

	close(pivec[0]);		/* parent */
	pio=fdopen(pivec[1],"w");
	sigint = sigset(SIGINT, SIG_IGN);
	sigpipe = sigset(SIGPIPE, SIG_IGN);

					/* send all messages to cmd */
	page = (value("page")!=NOSTR);
	lc = cc = 0;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		dot = mp = &message[mesg-1];
		if ((t = sendmsg(mp, pio, doign, 0, 1, 0)) < 0) {
			pfmt(stderr, MM_ERROR, errmsg, cmd, Strerror(errno));
			sigset(SIGPIPE, sigpipe);
			sigset(SIGINT, sigint);
			return(1);
		}
		lc += t;
		cc += mp->m_size;
		if (page) putc('\f', pio);
	}

	fflush(pio);
	if (ferror(pio))
	      pfmt(stderr, MM_ERROR, errmsg, cmd, Strerror(errno));
	fclose(pio);

					/* wait */
	if (!nowait) {
		while (wait(&s) != pid);
		if (s != 0)
			pfmt(stdout, MM_ERROR, ":211:Pipe to \"%s\" failed\n", 
				cmd);
	}
	if (nowait || s == 0)
		printf("\"%s\" %ld/%ld\n", cmd, lc, cc);
	sigset(SIGPIPE, sigpipe);
	sigset(SIGINT, sigint);
	return(0);
}

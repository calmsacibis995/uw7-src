#ident	"@(#)in.rexecd.c	1.4"
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


#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/filio.h>
#ifdef AUDIT
#include <sys/label.h>
#include <sys/audit.h>
#endif AUDIT

#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <ia.h>
#include <lastlog.h>
#ifdef AUDIT
#include <pwdadj.h>
#endif AUDIT
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

#include "./security.h"

/*
 * NCARGS has become enormous, making *	it impractical to allocate arrays
 * with NCARGS / 6 entries on the stack.  We simply fall back on using
 * the old value of NCARGS.
 */
#define	AVSIZ	(10240 / 6)

extern	errno;

#ifdef SYSV
#define rindex strrchr
#define killpg(a,b)	kill(-(a),(b))
#endif SYSV

char	*bigcrypt(), *rindex(), *strncat();

#ifdef AUDIT
struct  passwd_adjunct *getpwanam();
char	*audit_argv[] = { "in.rexecd", 0, 0, 0, 0 };
#endif AUDIT
/*VARARGS1*/
int	error();

/*
 * This stuff is used for checking login expiration and inactivity
 */

#define LASTLOG		"/var/adm/lastlog"
#define DAY_SECS	(24L * 60 * 60)

/*
 * remote execute server:
 *	username\0
 *	password\0
 *	command\0
 *	data
 */
/*ARGSUSED*/
main(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_in from;
	int fromlen;

	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		exit(1);
	}
	doit(0, &from);
	/* NOTREACHED */
}

char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
#ifdef SYSV
char	username[23] = "LOGNAME=";
char	*envinit[] =
	    {homedir, shell, "PATH=:/bin:/usr/bin", username, 0};
#else
char	username[20] = "USER=";
char	*envinit[] =
	    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", username, 0};
#endif /* SYSV */
extern	char	**environ;

#ifdef __NEW_SOCKADDR__
struct	sockaddr_in asin = { sizeof(struct sockaddr_in), AF_INET };
#else
struct	sockaddr_in asin = { AF_INET };
#endif

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	char cmdbuf[AVSIZ+1], *cp, *namep;
	char user[16], pass[PASS_MAX];
	struct	hostent *chostp;
	uinfo_t uinfo;
	uid_t uid;
	gid_t gid, *grouplist;
	long ngroups, expire, inact, today;
	char *password, *dir, *ushell, *tmpstr;
	struct lastlog ll;
	int llfd;
#ifdef AUDIT
	struct  passwd_adjunct *apw;
	audit_state_t audit_state;
#endif AUDIT
	int s;
	u_short port;
	pid_t pid;
	int pv[2], ready, readfrom, cc;
	char buf[BUFSIZ], sig;
	int one = 1;

#ifdef AUDIT
	/*
	 * Set up the audit user information so that auditing failures
	 * can occur.
	 */
	if (issecure()) {
		CLR_WORKPRIVS_NON_ADMIN(setauid(0));
		audit_state.as_success = AU_LOGIN;
		audit_state.as_failure = AU_LOGIN;
        	setaudit(&audit_state);
	}
#endif AUDIT

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef DEBUG
	{ int t = open("/dev/tty", 2);
	  if (t >= 0) {
#ifdef SYSV
		setsid();
#else
		ioctl(t, TIOCNOTTY, (char *)0);
#endif SYSV
		(void) close(t);
	  }
	}
#endif
        /*
	 * store common info. for audit record 
         */
	chostp = gethostbyaddr(&fromp->sin_addr, sizeof(struct in_addr),
				fromp->sin_family);
#ifdef AUDIT
	audit_argv[2] = chostp->h_name; 
#endif AUDIT
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if (read(f, &c, 1) != 1)
			exit(1);
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0)
			exit(1);
		if (bind(s, &asin, sizeof (asin)) < 0)
			exit(1);
		(void) alarm(60);
		fromp->sin_port = htons((u_short)port);
		if (connect(s, fromp, sizeof (*fromp)) < 0)
			exit(1);
		(void) alarm(0);
	}
	getstr(user, sizeof(user), "username");
	getstr(pass, sizeof(pass), "password");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
#ifdef AUDIT
	audit_argv[1] = user; 
	audit_argv[3] = cmdbuf; 
#endif AUDIT

	if (ia_openinfo(user, &uinfo) == -1) {
#ifdef AUDIT
		audit_note(1, 1, "Login incorrect");
#endif AUDIT
		error("Login incorrect.\n");
		exit(1);
	}

	ia_get_uid(uinfo, &uid);
	ia_get_gid(uinfo, &gid);
	ia_get_sgid(uinfo, &grouplist, &ngroups);
	ia_get_logpwd(uinfo, &password);
	ia_get_dir(uinfo, &dir);
	ia_get_sh(uinfo, &tmpstr);
	/* make a copy of shell so we can use it after ia_closeinfo */
	ushell = strdup(tmpstr);
	ia_get_logexpire(uinfo, &expire);
	ia_get_loginact(uinfo, &inact);

#ifdef AUDIT
	/* 
	 * set audit uid 
	 */
	CLR_WORKPRIVS_NON_ADMIN(setauid(uid));

	/*
	 * get audit flags for user and set for this process
	 */
	if ((apw = getpwanam(user)) != NULL) {
		audit_state.as_success = 0;
		audit_state.as_failure = 0;

        	if ((getfauditflags(&apw->pwa_au_always,
			&apw->pwa_au_never, &audit_state))!=0) {
              		/*
               	 	 * if we can't tell how to audit from the 
			 * flags, audit everything that's not never 
			 * for this user.
               	 	 */
              		audit_state.as_success =
			    apw->pwa_au_never.as_success ^ (-1);
              		audit_state.as_failure =
			    apw->pwa_au_never.as_success ^ (-1);
		}

	} else {
		/* user entry in the passwd file but not the 
		 * passwd.adjunct file.  Audit everything.  
		 * If this isn't a secure system nothing will 
		 * be audited
		 */
                audit_state.as_success = -1; 
                audit_state.as_failure = -1; 
	}
	if (issecure())
        	setaudit(&audit_state);
#endif AUDIT

	if (*password != '\0') {
		namep = bigcrypt(pass, password);
		if (strcmp(namep, password)) {
#ifdef AUDIT
			audit_note(1, 1, "Password incorrect");
#endif AUDIT
			error("Password incorrect.\n");
			exit(1);
		}
	}

	/*
	 * Check for expired login or inactive login
	 */

	today = time((long *) 0) / DAY_SECS;
	if (expire > 0 && expire < today) {
#ifdef AUDIT
		audit_note(1, 1, "Login expired"); 
#endif
		error("Login expired.\n");
		exit(1);
	}
	if (inact > 0) {
		if ((llfd = open(LASTLOG, O_RDONLY)) != -1) {
			(void) lseek(llfd, uid*sizeof(struct lastlog),
			  SEEK_SET);
			if (read(llfd, &ll, sizeof(struct lastlog))
			    == sizeof(struct lastlog)
			  && ll.ll_time > 0
			  && ((ll.ll_time / DAY_SECS) + inact) < today) {
#ifdef AUDIT
				audit_note(1, 1, "Login inactivity limit exceeded"); 
#endif
				error("Login inactivity limit exceeded.\n");
				exit(1);
			}
			close(llfd);
		}
	}

	if (chdir(dir) < 0) {
#ifdef AUDIT
		audit_note(1, 1, "No remote directory");
#endif AUDIT
		error("No remote directory.\n");
		exit(1);
	}
	(void) write(2, "\0", 1);
	if (port) {
		(void) pipe(pv);
		pid = fork();
		if (pid == (pid_t)-1)  {
			error("Try again.\n");
			exit(1);
		}
		if (pid) {
			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			readfrom = (1<<s) | (1<<pv[0]);
			ioctl(pv[1], FIONBIO, (char *)&one);
			/* should set s nbio! */
			do {
				ready = readfrom;
				(void) select(16, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0);
				if (ready & (1<<s)) {
					if (read(s, &sig, 1) <= 0)
						readfrom &= ~(1<<s);
					else
						killpg(pid, sig);
				}
				if (ready & (1<<pv[0])) {
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						shutdown(s, 1+1);
						readfrom &= ~(1<<pv[0]);
					} else
						(void) write(s, buf, cc);
				}
			} while (readfrom);
			exit(0);
		}
		setpgrp(0, getpid());
		(void) close(s); (void)close(pv[0]);
		dup2(pv[1], 2);
	}
#ifdef AUDIT
	/* 
	 * output audit record for successful operation 
	 */
	audit_note(0, 0, "user authenticated");
#endif AUDIT

	if (*ushell == '\0')
		ushell = "/bin/sh";
	if (f > 2)
		(void) close(f);
	if ( setgid(gid) < 0 ) {
#ifdef AUDIT
		audit_note(1, 1, "can't setgid");
#endif AUDIT
		error("setgid");
		exit(1);
	}
	(void) setgroups(ngroups, grouplist);
	if (  CLR_WORKPRIVS_NON_ADMIN(setuid(uid)) < 0 ) {
#ifdef AUDIT
		audit_note(1, 1, "can't setuid");
#endif AUDIT
		error("setuid");
		exit(1);
	}
	environ = envinit;
	strncat(homedir, dir, sizeof(homedir)-6);
	strncat(shell, ushell, sizeof(shell)-7);
	strncat(username, uinfo->ia_name, sizeof(username)-6);
	cp = rindex(ushell, '/');
	if (cp)
		cp++;
	else
		cp = ushell;
	ia_closeinfo(uinfo);
	CLR_MAXPRIVS_FOR_EXEC execl(ushell, cp, "-c", cmdbuf, (char *)0);
	perror(ushell);
#ifdef AUDIT
	audit_note(1, 1, "can't exec");
#endif AUDIT
	exit(1);
}

/*VARARGS1*/
error(fmt, a1, a2, a3)
	char *fmt;
	int a1, a2, a3;
{
	char buf[BUFSIZ];

	buf[0] = 1;
	(void) sprintf(buf+1, fmt, a1, a2, a3);
	(void) write(2, buf, strlen(buf));
}

getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error("%s too long\n", err);
			exit(1);
		}
	} while (c != 0);
}

#ifdef AUDIT
audit_note(err, retcode, s)
	int err;
	int retcode;
	char *s;
{
	audit_argv[4] = s;
	audit_text(AU_LOGIN, err, retcode, 5, audit_argv);
}
#endif AUDIT

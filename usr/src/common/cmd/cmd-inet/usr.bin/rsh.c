/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
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

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)rsh.c	1.3"
#ident	"$Header$"

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
#include <locale.h>
#include <pfmt.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
/* just for FIONBIO ... */
#include <sys/filio.h>

#include <netinet/in.h>

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <netdb.h>
#include "../usr.sbin/security.h"

#ifdef SYSV
#define	rindex		strrchr
#define	index		strchr
#define	bcopy(a,b,c)	memcpy((b),(a),(c))
#define	bzero(s,n)	memset((s), 0, (n))
#endif /* SYSV */

int	error();
char	*index(), *rindex(), *malloc(), *getpass(), *strcpy();
struct passwd	*getpwuid();

extern	int	errno;
int	options;
int	rfd2;
int	sendsig();

#ifdef SYSV
#ifndef sigmask
#define sigmask(m)      (1 << ((m)-1))
#endif

#define set2mask(setp) ((setp)->sa_sigbits[0])
#define mask2set(mask, setp) \
	((mask) == -1 ? sigfillset(setp) : (((setp)->sa_sigbits[0]) = (mask)))
	

static sigsetmask(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_SETMASK, &nset, &oset);
	return set2mask(&oset);
}

static sigblock(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_BLOCK, &nset, &oset);
	return set2mask(&oset);
}

#endif

#define	mask(s)	(1 << ((s) - 1))

/*
 * rsh - remote shell
 */
/* VARARGS */
main(argc, argv)
	int argc;
	char *argv[];
{
	pid_t pid;
	int rem;
	char *host, *cp, **ap, buf[BUFSIZ], *args, *user = 0;
	register int cc;
	int asrsh = 0;
	struct passwd *pwd;
	int readfrom, ready;
	int one = 1;
	struct servent *sp;
	int omask;
	int c;
	int ac;
	char **av;
	extern int optind;
	extern char *optarg;
	struct linger linger;

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxrsh");
        (void)setlabel("UX:rsh");
 
	DISABLE_WORK_PRIVS;
	linger.l_onoff = 1;
	linger.l_linger = 60;

	/*
	 * must be compatible with old-style command line parsing,
	 * which allowed hostname to come before arguments, so
	 *	rsh host -l user cmd
	 * is permissible, even tho' it doesn't fit the command
	 * syntax standard.
	 */
	host = rindex(argv[0], '/');
	if (host)
		host++;
	else
		host = argv[0];
	if ( strcmp(host, "rsh") == 0 && argc > 1 && argv[1][0] != '-' ) {
		ac = argc -1; av = &argv[1];
	} else {
		ac = argc; av = argv;
	}
	while ( (c = getopt(ac, av, "l:ndLwe:8")) != -1 ) {
		switch (c) {
		case 'l':
			user = optarg;
			break;
		case 'n':
			(void) close(0);
			(void) open("/dev/null", 0);
			break;
		case 'd':
			options |= SO_DEBUG;
			break;
		/*
		 * Ignore the -L, -w, -e and -8 flags to allow aliases
		 * with rlogin to work
		 */
		case 'L': case 'w': case 'e': case '8':
			break;
		default:
			goto usage;
		}
	}

	if ( strcmp(host, "rsh") == 0 ) {
		if ( ac == argc ) {
			if ( optind >= argc )
				goto usage;
			host = argv[optind];
		} else {
			host = argv[1];
		}
		++optind;
		asrsh = 1;
	}
	
	if ( optind >= argc ) {
		if (asrsh)
			argv[0] = "rlogin";
		execv("/usr/bin/rlogin", argv);
		execv("/usr/ucb/rlogin", argv);
		pfmt(stderr, MM_ERROR, ":1:No local rlogin program found\n");
		exit(1);
	}
	pwd = getpwuid(getuid());
	if (pwd == 0) {
		pfmt(stderr, MM_ERROR, ":2:who are you?\n");
		exit(1);
	}
	cc = 0;
	for (ap = &argv[optind]; *ap; ap++)
		cc += strlen(*ap) + 1;
	if ((cp = args = malloc(cc)) == (char *)NULL) {
		pfmt(stderr, MM_ERROR, ":3:malloc:%s\n", strerror(errno));
		exit(1);
	}
	memset(args, 0,  cc);
	for (ap = &argv[optind]; *ap; ap++) {
		(void) strcpy(cp, *ap);
		while (*cp)
			cp++;
		if (ap[1])
			*cp++ = ' ';
	}
	sp = getservbyname("shell", "tcp");
	if (sp == 0) {
		pfmt(stderr, MM_ERROR, ":4:shell/tcp: unknown service\n");
		exit(1);
	}
	ENABLE_WORK_PRIVS;
        rem = rcmd(&host, sp->s_port, pwd->pw_name,
	    user ? user : pwd->pw_name, args, &rfd2);
        if (rem < 0)
                exit(1);
	if (rfd2 < 0) {
		pfmt(stderr, MM_ERROR, ":5:can't establish stderr\n");
		exit(2);
	}
#ifdef SYSV
	if (is_enhancedsecurity()) {
		fd_set_to_my_level(rem);
		fd_set_to_my_level(rfd2);
	}
#endif SYSV
	if (options & SO_DEBUG) {
		if (setsockopt(rem, SOL_SOCKET, SO_DEBUG, (char *) &one, sizeof (one)) < 0)
			pfmt(stderr, MM_ERROR, ":6:setsockopt (stdin) (ignored): %s\n",
				strerror(errno));
		if (setsockopt(rfd2, SOL_SOCKET, SO_DEBUG, (char *) &one, sizeof (one)) < 0)
			pfmt(stderr, MM_ERROR, ":7:setsockopt (stderr) (ignored): %s\n",
				strerror(errno));
	}
#ifndef SYSV
	(void) setuid(getuid());
#endif !SYSV
	omask = sigblock(mask(SIGINT)|mask(SIGQUIT)|mask(SIGTERM));
        pid = fork();
	DISABLE_WORK_PRIVS;
        if (pid < 0) {
		pfmt(stderr, MM_ERROR, ":8:fork: %s\n", strerror(errno));
                exit(1);
        }
	ioctl(rfd2, FIONBIO, &one);
	ioctl(rem, FIONBIO, &one);
        if (pid == 0) {
		char *bp; int rembits, wc;
		(void) close(rfd2);
	reread:
		errno = 0;
		cc = read(0, buf, sizeof buf);
		if (cc <= 0)
			goto done;
		bp = buf;
	rewrite:
		rembits = 1<<rem;
		if (select(sizeof(int)*8, (fd_set *) 0, (fd_set *) &rembits,
					0, 0) < 0) {
			if (errno != EINTR) {
				pfmt(stderr, MM_ERROR, ":9:select: %s\n",
					strerror(errno));
				exit(1);
			}
			goto rewrite;
		}
		if ((rembits & (1<<rem)) == 0)
			goto rewrite;
		wc = write(rem, bp, cc);
		if (wc < 0) {
			if (errno == EWOULDBLOCK)
				goto rewrite;
			goto done;
		}
		cc -= wc; bp += wc;
		if (cc == 0)
			goto reread;
		goto rewrite;
	done:
		ENABLE_WORK_PRIVS;
		(void) shutdown(rem, 1);
		DISABLE_WORK_PRIVS;
		exit(0);
	}
	readfrom = (1<<rfd2) | (1<<rem);
	/*
	 * The following logic is necessary to test whether the signal
	 * was ignored without actually setting it to be ignored; this would
	 * cause any pending signals to be lost.
	 * sigset() is used so that the SIG_DFL disposition is not restored
	 * after the handler has been executed once
	 */
	if (signal(SIGINT, (void (*)())sendsig) != SIG_IGN)
		sigset(SIGINT, (void (*)())sendsig);
	else
		signal(SIGINT, SIG_IGN);
	if (signal(SIGQUIT, (void (*)())sendsig) != SIG_IGN)
		sigset(SIGQUIT, (void (*)())sendsig);
	else
		signal(SIGQUIT, SIG_IGN);
	if (signal(SIGTERM, (void (*)())sendsig) != SIG_IGN)
		sigset(SIGTERM, (void (*)())sendsig);
	else
		signal(SIGTERM, SIG_IGN);
	sigsetmask(omask);

	do {
		ready = readfrom;
		if (select(sizeof(int)*8, (fd_set *) &ready, 0, 0, 0) < 0) {
			if (errno != EINTR) {
				pfmt(stderr, MM_ERROR, ":9:select: %s\n",
					strerror(errno));
				exit(1);
			}
			continue;
		}
		if (ready & (1<<rfd2)) {
			errno = 0;
			cc = read(rfd2, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK)
					readfrom &= ~(1<<rfd2);
			} else
				(void) write(2, buf, cc);
		}
		if (ready & (1<<rem)) {
			errno = 0;
			cc = read(rem, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK)
					readfrom &= ~(1<<rem);
			} else
				(void) write(1, buf, cc);
		}
        } while (readfrom);
        (void) kill(pid, SIGKILL);
	exit(0);
usage:
	pfmt(stderr,
	    MM_ERROR, ":10:usage: rsh [ -l login ] [ -n ] host command\n");
	exit(1);
	/* NOTREACHED */
}

sendsig(signo)
	char signo;
{
	(void) write(rfd2, &signo, 1);
}

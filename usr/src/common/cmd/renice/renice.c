/*	copyright "%c%" */

#ident	"@(#)renice:renice.c	1.1.1.4"
#ident	"@(#)ucb:common/ucbcmd/renice/renice.c	1.1"
#ident	"$Header$"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#ifndef lint
static	char *sccsid = "@(#)renice.c 1.7 88/08/04 SMI"; /* from UCB 4.6 83/07/24 */
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <locale.h>
#include <pfmt.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

static void	usage();
static int	valid_args(int, char **);
static int	getint(const char *);
static int	donice(int, int, int, int);
static int	nflag = 0;
static int	posix=0;
static const char posix_var[]="POSIX2";

#define PRIO_MAX	20
#define PRIO_MIN	-20

/*
 * Change the priority (nice) of processes
 * or groups of processes which are already
 * running.
 */
main(argc, argv)
	char **argv;
{
	int which = PRIO_PROCESS;
	int who = 0, prio, errs = 0;
	int increment = 0;
	struct passwd *pwd;
	int old = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:renice");

	if (getenv(posix_var))
		posix=1;

	if (!valid_args(argc, argv))
		usage();

	argc--, argv++;
	if (argc < 1) {
		usage();
		/* NOTREACHED */
	}
	prio = getint(*argv);
	if (errno == 0) {
		/* obsolescent version */
		old=1;
		argc--, argv++;
		if (prio > PRIO_MAX)
			prio = PRIO_MAX;
		if (prio < PRIO_MIN)
			prio = PRIO_MIN;
	}
	for (; argc > 0; argc--, argv++) {
		if (strcmp(*argv, "-n") == 0) {
			if (old || argc < 3 )
				usage();
			argc--, argv++;
			nflag = 1;
			increment = getint(*argv);
			if (errno != 0)
				usage();
			continue;
		}
		if (strcmp(*argv, "-g") == 0) {
			which = PRIO_PGRP;
			continue;
		}
		if (strcmp(*argv, "-u") == 0) {
			which = PRIO_USER;
			continue;
		}
		if (strcmp(*argv, "-p") == 0) {
			which = PRIO_PROCESS;
			continue;
		}
	        who = getint (*argv);
		if (which == PRIO_USER) {
                      if (errno != 0 || who < 0) {
				pwd = getpwnam(*argv);
				if (pwd == NULL) {
					(void) pfmt(stderr, MM_ERROR,
						 ":1085:%s: unknown user\n",
					*argv);
					continue;
				} else 
					who = pwd->pw_uid;
			}
		} else {
                      if (errno != 0 || who < 0) {
				(void) pfmt(stderr, MM_ERROR, 
					":1086:%s: bad value\n", *argv);
				continue;
			}
		}
		errs += donice(which, who, prio, increment);
	}
	exit(errs != 0);
	/* NOTREACHED */
}

static int
donice(which, who, prio, increment)
	int which, who, prio, increment;
{
	int oldprio;
	extern int errno;

	errno = 0, oldprio = getpriority(which, who);
	if (oldprio == -1 && errno) {
		(void) pfmt(stderr, MM_ERROR, ":192:%d:", who);
		perror(gettxt(":1087", " getpriority"));
		return (1);
	}
	if (nflag) {
		if (setpriority(which, who, oldprio + increment) < 0) {
			(void) pfmt(stderr, MM_ERROR, ":192:%d:", who);
			perror(gettxt(":1088", " setpriority"));
			return (1);
		}
		if (!posix) 
			(void) pfmt(stdout, MM_NOSTD,
				":1089:%d: old priority %d, new priority %d\n",
				who, oldprio, oldprio + increment);
	} else {
		if (setpriority(which, who, prio) < 0) {
			(void) pfmt(stderr, MM_ERROR, ":192:%d:", who);
			perror(gettxt(":1088", " setpriority"));
			return (1);
		}
		if (!posix) 
			(void) pfmt(stdout, MM_NOSTD,
				":1089:%d: old priority %d, new priority %d\n",
				who, oldprio, prio);
	}
	return (0);
}

static void
usage()
{
	(void) pfmt(stderr, MM_ACTION, 
	   ":1090:Usage: renice [ -n increment ] [ -g | -p | -u ] ID\n");
	(void) pfmt(stderr, MM_NOSTD, 
		":1091:                          renice priority [ [ -p ] pids ]");
	(void) pfmt(stderr, MM_NOSTD,
		":1092: [ [ -g ] pgrps ] [ [ -u ] users ]\n");
	exit(1);
}

/*
 * If argument is a number, with no other characters, set errno to 0 and
 * return the value of the number.  Else set errno to EINVAL
 */
static int
getint(const char *arg)
{
	const char *c;
	int neg = 0;

	errno=0;

        c = arg;
	if (*c == '-') {
		neg = 1;
		c++;
	}
        while (*c != '\0') {
		if (!isdigit(*c)) 
        		break;
        	else c++;
        }
        if (*c == '\0')
        	return(atoi(arg));
	else  {
		errno=EINVAL;
		return(-1);
	}
}

static int
valid_args(int argcnt, char **args)
{
	int    c;
	char **argptr = args;

	while (--argcnt > 0 && (*++argptr)[0] == '-')
	{
		if ((c = (*argptr)[1]) != '\0' && (!isdigit(c)))
		{
			switch (c)
			{
			  case 'n':
			  case 'g':
			  case 'p':
			  case 'u': break;
			  default : return (0);   /* FAIL */
			}
		}
	}
	return (1);   /* SUCCESS */
}

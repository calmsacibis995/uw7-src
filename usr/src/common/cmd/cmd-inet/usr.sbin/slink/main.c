#ident "@(#)main.c	1.4"
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
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <signal.h>
#include <stropts.h>
#include "defs.h"
#include "proto.h" 
#include "pathnames.h" 


char           *cmdname;
int             verbose = 0;
int		dounlink = 0;
int		Gflag = 0;
FILE	       *devfile = NULL;
int		nopen = 0;

int ksl_fd = 0;

/*
 * error - print error message and possibly exit.
 * error(flags,fmt,arg1,arg2,...) prints error message on stderr of the form:
 * command-name: message[: system error message] If flags & E_SYS, system
 * error message is included. If flags & E_FATAL, program exits (with code
 * 1).
 */
#ifdef __STDC__
void
error(int flags, ...)
#else
void
error(va_alist)
va_dcl
#endif
{
	va_list         args;
#ifndef	__STDC__
	int             flags;
#endif
	char           *fmt;
	extern int      sys_nerr, errno;
	extern char    *sys_errlist[];

#ifdef __STDC__
	va_start(args,flags);
#else
	va_start(args);
	flags = va_arg(args, int);
#endif
	fmt = va_arg(args, char *);
	fprintf(stderr, "%s: ", cmdname);
	vfprintf(stderr, fmt, args);
	va_end(args);
	if (flags & E_SYS) {
		if (errno > sys_nerr)
			fprintf(stderr, ": Error %d\n", errno);
		else
			fprintf(stderr, ": %s\n", sys_errlist[errno]);
	} else
		putc('\n', stderr);
	if (flags & E_FATAL)
		exit(1);
}

void
catch(signo)
int signo;
{
}

main(argc, argv)
	int             argc;
	char           *argv[];
{
	int             c;
	extern char    *optarg;
	extern int      optind, opterr;
	char           *ifile = _PATH_STRCF;
	char           *func;
	struct val     *arglist = NULL;
	int             i;
	struct fntab   *f;
	struct fntab   *findfunc();
	FILE           *cons;
	int             pid;
	int             dofork = 1;

	cmdname = argv[0];
	opterr = 0;
	while ((c = getopt(argc, argv, "vc:fuGn:")) != -1) {
		switch (c) {
		case 'G':
			Gflag++;
			break;
		case 'u':
			dounlink = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'c':
			ifile = optarg;
			break;
		case 'f':
			/* assumption is forking means no PLINKs */
			dofork = 0;
			break;
		case 'n':
			if (!(devfile = fopen(optarg, "w")))
			  error(E_FATAL, "cannot open \"%s\"", optarg);
			break;
		case '?':
			error(E_FATAL, "usage: slink [-f] [-u] [-v] [-G] [-c file] [func arg...]");
			/* NOTREACHED */
		}
	}
	fclose(stdin);
	if (!fopen(ifile, "r"))
		error(E_FATAL, "can't open \"%s\"", ifile);
	argv += optind;
	argc -= optind;
	if (argc) {
		if (verbose)
			fprintf(stderr, "%s", *argv);
		func = *argv++;
		if (--argc) {
			arglist = (struct val *) xmalloc(argc * sizeof(struct val));
			for (i = 0; i < argc; i++) {
				if (verbose)
					fprintf(stderr, " %s", *argv);
				arglist[i].vtype = V_STR;
				arglist[i].u.sval = *argv++;
			}
			if (verbose)
				putc('\n', stderr);
		}
	} else {
		if (verbose)
			fprintf(stderr, "boot\n");
		func = "boot";
	}

	binit();
	parse();
	if (!(f = findfunc(func)) || f->type != F_USER)
		error(E_FATAL, "Function \"%s\" not defined", func);
	push_fn(func);
	userfunc(f->u.ufunc, argc, arglist);
	pop_fn(func);
	if (arglist)
		free(arglist);
	
	/* if generating c-code, or no file descriptors
	 * still open, then exit (number of file descriptions open
	 * can be negative due to link/close pairs on the same
	 * descriptor).
	 */
	if (dounlink || Gflag || (nopen <= 0))
		exit(0);

	if (dofork) {
		if ((pid = fork()) < 0)
			error(E_FSYS, "can't fork");
		else if (pid)
			exit(0);
		close(0);
		close(1);
		setpgrp();
		sigset(SIGHUP, SIG_IGN);
		sigset(SIGINT, SIG_IGN);
		sigset(SIGQUIT, SIG_IGN);
		sigset(SIGTERM, catch);
		/* hang around */
		close(2);
		pause();
		if (cons = fopen(_PATH_CONSOLE, "w"))
			fprintf(cons, "%s exiting: SIGTERM\n", cmdname);
	} else {
		close(2);
		pause();
	}
	exit(0);
}

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

#ident	"@(#)main.c	1.4"
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


/*
 * FTP User Program -- Command Interface.
 */

#include <sys/types.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/socket.h>

#include <arpa/ftp.h>

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <pwd.h>
#include <varargs.h>

#include "ftp_var.h"

extern void (*Signal())();



uid_t	getuid();
void	intr();
void	usage();
void	lostpeer();
extern	char *home;
char	*getlogin();
u_short ftp_port;

main(argc, argv)
	char *argv[];
{
	register char *cp;
	int top;
	struct passwd *pw = NULL;
	char homedir[MAXPATHLEN];
	int c;
	extern int optind;
	int cflag;

	(void)setlocale(LC_ALL,"");
        (void)setcat("uxftp");
        (void)setlabel("UX:ftp");
	(void) init_cmdtab();
	sp = getservbyname("ftp", "tcp");
	if (sp == 0) {
		pfmt(stderr,
			MM_ERROR, ":1:ftp/tcp: unknown service\n");
		exit(1);
	}
	ftp_port = sp->s_port;
	doglob = 1;
	interactive = 1;
	autologin = 1;
	sendport = -1;	/* tri-state variable. start out in "automatic" mode. */
	cflag = 0;
#ifndef NO_PASSIVE_MODE
	passivemode = 0; /* passive mode not active by default */
#endif
	compat = 0; /* use SYST by default */
	while ( (c = getopt(argc, argv, "Ccdvtingp")) != -1 ) {

		switch (c) {

		case 'd':
			options |= SO_DEBUG;
			debug++;
			break;
		
		case 'v':
			verbose++;
			break;

		case 't':
			trace++;
			break;

		case 'i':
			interactive = 0;
			break;

		case 'n':
			autologin = 0;
			break;

		case 'g':
			doglob = 0;
			break;

		case 'c':
			if (cflag) usage(argv[0]);
			compat = 1;
			cflag = 1;
			break;

		case 'C':
			if (cflag) usage(argv[0]);
			compat = 0;
			cflag = 1;
			break;

#ifndef NO_PASSIVE_MODE
		case 'p':
			passivemode = 1; /* passive mode */
			break;
#endif

		default:
			usage(argv[0]);
		}
	}

	fromatty = isatty(fileno(stdin));
	/*
	 * Set up defaults for FTP.
	 */
	(void) strcpy(typename, "ascii"), type = TYPE_A;
	(void) strcpy(formname, "non-print"), form = FORM_N;
	(void) strcpy(modename, "stream"), mode = MODE_S;
	(void) strcpy(structname, "file"), stru = STRU_F;
	(void) strcpy(bytename, "8"), bytesize = 8;
	if (fromatty)
		verbose++;
	cpend = 0;           /* no pending replies */
	proxy = 0;	/* proxy not active */
	crflag = 1;    /* strip c.r. on ascii gets */
	/*
	 * Set up the home directory in case we're globbing.
	 */
	cp = getlogin();
	if (cp != NULL) {
		pw = getpwnam(cp);
	}
	if (pw == NULL)
		pw = getpwuid(getuid());
	if (pw != NULL) {
		home = homedir;
		(void) strcpy(home, pw->pw_dir);
	}
	if (optind < argc) {
		if (setjmp(toplevel))
			exit(0);
		(void) Signal(SIGINT, intr);
		(void) Signal(SIGPIPE, lostpeer);
		--optind;
		setpeer((argc-optind), &(argv[optind]));
	}
	top = setjmp(toplevel) == 0;
	if (top) {
		(void) Signal(SIGINT, intr);
		(void) Signal(SIGPIPE, lostpeer);
	}

	for (;;) {
		cmdscanner(top);
		top = 1;
	}
}

void
usage(cmd)
char *cmd;
{
	pfmt(stderr,
	   MM_ERROR, ":2:USAGE: %s [-c | -C] [-d] [-v] [-t] [-i] [-n] [-g] [-p] [hostname]\n",
		cmd);
	exit(1);
}

void
intr()
{

	sigrelse(SIGINT);
	longjmp(toplevel, 1);
}

void
lostpeer()
{
	extern FILE *cout;
	extern int data;

	if (connected) {
		if (cout != NULL) {
			(void) shutdown(fileno(cout), 1+1);
			(void) fclose(cout);
			cout = NULL;
		}
		if (data >= 0) {
			(void) shutdown(data, 1+1);
			(void) close(data);
			data = -1;
		}
		connected = 0;
	}
	pswitch(1);
	if (connected) {
		if (cout != NULL) {
			(void) shutdown(fileno(cout), 1+1);
			(void) fclose(cout);
			cout = NULL;
		}
		connected = 0;
	}
	proxflag = 0;
	pswitch(0);
}

/*char *
tail(filename)
	char *filename;
{
	register char *s;
	
	while (*filename) {
		s = rindex(filename, '/');
		if (s == NULL)
			break;
		if (s[1])
			return (s + 1);
		*s = '\0';
	}
	return (filename);
}
*/
/*
 * Command parser.
 */
cmdscanner(top)
	int top;
{
	register struct cmd *c;
	struct cmd *getcmd();
	extern struct cmd cmdtab[];
	extern int help();

	if (!top)
		(void) putchar('\n');
	for (;;) {
		if (fromatty) {
			pfmt(stdout,
				MM_NOSTD, ":3:ftp> ");
			(void) fflush(stdout);
		}
		if (fgets(line,LINSIZ,stdin) == 0) {
			if (feof(stdin) || ferror(stdin))
				quit();
			break;
		}
		line[strlen(line)-1]='\0';
		if (line[0] == 0)
			break;
		makeargv();
		if (margc == 0) {
			continue;
		}
		c = getcmd(margv[0]);
		if (c == (struct cmd *)-1) {
			pfmt(stdout,
				MM_NOSTD, ":4:?Ambiguous command\n");
			continue;
		}
		if (c == 0) {
			pfmt(stdout,
				MM_NOSTD, ":5:?Invalid command\n");
			continue;
		}
		if (c->c_conn && !connected) {
			pfmt(stdout,
				MM_NOSTD, ":6:Not connected.\n");
			continue;
		}
		(*c->c_handler)(margc, margv);
#ifndef CTRL
#define CTRL(c) ((c)&037)
#endif
		if (bell && c->c_bell)
			(void) putchar(CTRL('g'));
		if (c->c_handler != help)
			break;
	}
	(void) Signal(SIGINT, (void (*)())intr);
	(void) Signal(SIGPIPE, (void (*)())lostpeer);
}

struct cmd *
getcmd(name)
	register char *name;
{
	register char *p, *q;
	register struct cmd *c, *found;
	register int nmatches, longest;
	extern struct cmd cmdtab[];

	longest = 0;
	nmatches = 0;
	found = 0;
	for (c = cmdtab; p = c->c_name; c++) {
		for (q = name; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return (c);
		if (!*q) {			/* the name was a prefix */
			if (q - name > longest) {
				longest = q - name;
				nmatches = 1;
				found = c;
			} else if (q - name == longest)
				nmatches++;
		}
	}
	if (nmatches > 1)
		return ((struct cmd *)-1);
	return (found);
}

/*
 * Slice a string up into argc/argv.
 */

int slrflag;

makeargv()
{
	char **argp;
	char *slurpstring();

	margc = 0;
	argp = margv;
	stringbase = line;		/* scan from first of buffer */
	argbase = argbuf;		/* store from first of buffer */
	slrflag = 0;

	while (*argp++ = slurpstring())
		margc++;
}

/*
 * Parse string into argbuf;
 * implemented with FSM to
 * handle quoting and strings
 */
char *
slurpstring()
{
	int got_one = 0;
	register char *sb = stringbase;
	register char *ap = argbase;
	char *tmp = argbase;		/* will return this if token found */

	if (*sb == '!' || *sb == '$') {	/* recognize ! as a token for shell */
		switch (slrflag) {	/* and $ as token for macro invoke */
			case 0:
				slrflag++;
				stringbase++;
				return ((*sb == '!') ? "!" : "$");
				break;
			case 1:
				slrflag++;
				altarg = stringbase;
				break;
			default:
				break;
		}
	}

S0:
	switch (*sb) {

	case '\0':
		goto OUT;

	case ' ':
	case '\t':
		sb++; goto S0;

	default:
		switch (slrflag) {
			case 0:
				slrflag++;
				break;
			case 1:
				slrflag++;
				altarg = sb;
				break;
			default:
				break;
		}
		goto S1;
	}

S1:
	switch (*sb) {

	case ' ':
	case '\t':
	case '\0':
		goto OUT;	/* end of token */

	case '\\':
		sb++; goto S2;	/* slurp next character */

	case '"':
		sb++; goto S3;	/* slurp quoted string */

	default:
		*ap++ = *sb++;	/* add character to token */
		got_one = 1;
		goto S1;
	}

S2:
	switch (*sb) {

	case '\0':
		goto OUT;

	default:
		*ap++ = *sb++;
		got_one = 1;
		goto S1;
	}

S3:
	switch (*sb) {

	case '\0':
		goto OUT;

	case '"':
		sb++; goto S1;

	default:
		*ap++ = *sb++;
		got_one = 1;
		goto S3;
	}

OUT:
	if (got_one)
		*ap++ = '\0';
	argbase = ap;			/* update storage pointer */
	stringbase = sb;		/* update scan pointer */
	if (got_one) {
		return(tmp);
	}
	switch (slrflag) {
		case 0:
			slrflag++;
			break;
		case 1:
			slrflag++;
			altarg = (char *) 0;
			break;
		default:
			break;
	}
	return((char *)0);
}

#define HELPINDENT (sizeof ("directory"))

/*
 * Help command.
 * Call each command handler with argc == 0 and argv[0] == name.
 */
help(argc, argv)
	int argc;
	char *argv[];
{
	register struct cmd *c;
	extern struct cmd cmdtab[];

	if (argc == 1) {
		register int i, j, w, k;
		int columns, width = 0, lines;
		extern int NCMDS;

		pfmt(stdout,
			MM_NOSTD, ":7:Commands may be abbreviated.  Commands are:\n\n");
		for (c = cmdtab; c < &cmdtab[NCMDS]; c++) {
			int len = strlen(c->c_name);

			if (len > width)
				width = len;
		}
		width = (width + 8) &~ 7;
		columns = 80 / width;
		if (columns == 0)
			columns = 1;
		lines = (NCMDS + columns - 1) / columns;
		for (i = 0; i < lines; i++) {
			for (j = 0; j < columns; j++) {
				c = cmdtab + j * lines + i;
				if (c->c_name && (!proxy || c->c_proxy)) {
					pfmt(stdout,
						MM_NOSTD, ":8:%s", c->c_name);
				}
				else if (c->c_name) {
					for (k=0; k < strlen(c->c_name); k++) {
						(void) putchar(' ');
					}
				}
				if (c + lines >= &cmdtab[NCMDS]) {
					pfmt(stdout,
						MM_NOSTD, ":34:\n");
					break;
				}
				w = strlen(c->c_name);
				while (w < width) {
					w = (w + 8) &~ 7;
					(void) putchar('\t');
				}
			}
		}
		return;
	}
	while (--argc > 0) {
		register char *arg;
		arg = *++argv;
		c = getcmd(arg);
		if (c == (struct cmd *)-1)
			pfmt(stdout,
				MM_NOSTD, ":09:?Ambiguous help command %s\n", arg);
		else if (c == (struct cmd *)0)
			pfmt(stdout,
				MM_NOSTD, ":10:?Invalid help command %s\n", arg);
		else
			pfmt(stdout,
				MM_NOSTD, ":11:%-*s\t%s\n", HELPINDENT,
					c->c_name, c->c_help);
	}
}

/*
 * Call routine with argc, argv set from args (terminated by 0).
 */
call(routine, va_alist)
	int (*routine)();
	va_dcl
{
	va_list ap;
	char *argv[10];
	register int argc = 0;

	va_start(ap);
	while ((argv[argc] = va_arg(ap, char *)) != (char *)0)
		argc++;
	va_end(ap);
	return (*routine)(argc, argv);
}

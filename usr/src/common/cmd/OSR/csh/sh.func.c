#ident	"@(#)OSRcmds:csh/sh.func.c	1.1"
#pragma comment(exestr, "@(#) sh.func.c 25.7 95/03/09 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1995 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/* #ident	"@(#)csh:sh.func.c	1.2" */

/*
 *	@(#) sh.func.c 1.1 88/03/29 csh:sh.func.c
 */
/*	@(#)sh.func.c	2.1	SCCS id keyword	*/
/* Copyright (c) 1980 Regents of the University of California */

/* Modification History
 * S000		4 Apr 1989	sco!hoeshuen
 * - modified to use the same spell-checker that sh(C) uses for "cd"
 *
 * L001		16 Mar 1992	scol!markhe
 * - added support for symbolic modes to umask.  Needed for POSIX.2 and XPG4.
 *
 * L002		1 July 1993	scol!gregw
 * - added variable name length check for loop variable.
 *   From greim@sbsvax.UUCP (Michael Greim).
 * L003		17 Feb 1994	scol!gregw
 * - renamed calloc and free to Calloc and Free to avoid clash with libc.
 * L004		1 August 1994	scol!gregw
 * - added new functions for unsetenv. Code from BSD-4.4.
 * L005		23aug94		scol!hughd
 * - stop "Invalid reference" notice from setenv(), and allow setenv()
 *   to work in an ELF+DLLs csh: this wasn't obvious, but the fix seems
 *   to be that char **environ must be declared as extern: csh modifies
 *   environ in libc, which is then picked up when it later uses execv()
 * - note that this implies commons cannot be resolved across DLLs
 * S006		15 Feb 1995	sco!vinces
 * - added support to cd and pwd for logical path processing of symbolic
 *   links. Brought pwd into the list of built in functions for this shell.
 * - changed a few variable declarations from signed to unsigned in order to
 *   avoid numerous compile time warnings.
 */

#include "sh.h"
#include "../include/sym_mode.h"				/* L001 */
#include <sys/param.h>						/* S006 */
#include <limits.h>						/* L001 */
#include "../include/osr.h"

/*
 * C shell
 */
static int strperm(char *, mode_t, mode_t *);			/* L001 */
static char *permtostr(register mode_t);			/* L001 */
static void Unsetenv(char *);					/* L004 */

static char permstr[18];	/* space to hold symbolic mode  L001 */

struct biltins *
isbfunc(cp)
	register char *cp;
{
	register char *dp;
	register struct biltins *bp;

	if (lastchr(cp) == ':')
		return ((struct biltins *) 1);
	for (bp = bfunc; dp = bp->bname; bp++) {
		if (dp[0] == cp[0] && eq(dp, cp))
			return (bp);
		if (dp[0] > cp[0])
			break;
	}
	return (0);
}

func(t)
	register struct command *t;
{
	register struct biltins *bp;
	int i;

	bp = bfunc;
	if (lastchr(t->t_dcom[0]) == ':') {
		xechoit(t->t_dcom);
		if (!eq(t->t_dcom[0], ":") && t->t_dcom[1])
			error("No args on labels");
		return (1);
	}
	bp = isbfunc(t->t_dcom[0]);
	if (bp == 0)
		return (0);
	/* timed builtins must go in background if output is pipe, or &'ed */
	if (eq(bp->bname, "time"))
		if ((t->t_dflg & FAND) || (t->t_dflg & FPOU))
			return (0);
	if (eq(bp->bname, "nohup") && t->t_dcom[1])
		return (0);
	xechoit(t->t_dcom);
	setname(bp->bname);
	i = blklen(t->t_dcom) - 1;
	if (i < bp->minargs)
		bferr("Too few arguments");
	if (i > bp->maxargs)
		bferr("Too many arguments");
	i = (*bp->bfunct)(t->t_dcom, t);
	/* time and nice may not do their deeds, all others guarantee too */
	return (eq(bp->bname, "time") || eq(bp->bname, "nice") ? i : 1);
}

doonintr(v)
	char **v;
{
	register char *cp;
	register char *vv = v[1];

	if (parintr == SIG_IGN)
		return;
	if (setintr && intty)
		bferr("Can't from terminal");
	cp = gointr, gointr = 0, xfree(cp);
	if (vv == 0) {
		signal(SIGINT, setintr ? SIG_IGN : SIG_DFL);
		gointr = 0;
	} else if (eq((vv = strip(vv)), "-")) {
		signal(SIGINT, SIG_IGN);
		gointr = "-";
	} else {
		gointr = savestr(vv);
		signal(SIGINT, pintr);
	}
}

donohup()
{

	if (intty)
		bferr("Can't from terminal");
	if (setintr == 0) {
		signal(SIGHUP, SIG_IGN);
#ifdef CC
		submit(getpid());
#endif
	}
}

dozip()
{

	;
}

#ifndef TRUE							/* begin S006 */
#  define TRUE 1
#  define FALSE 0
#endif
#define MAXPWD (PATHSIZE+(PATHSIZE/2))
unsigned char cwdname[MAXPWD] = {0};
int didpwd = FALSE;						/* end S006 */

chngd(vp)
	register char **vp;
{
	register int i;
	register char *dp;
	int flag_error = FALSE;					/* S006 */
	int physical = (adrof("cdlogical"))?FALSE:TRUE;		/* S006 */
	register char **cdp;

	/* skip past the command name argument */
	vp++;

	/* check to see if the next argument is a flag */	/* begin S006 */
	if (*vp && **vp == '-' ) {
		if (strcmp(*vp, "-L") == 0)
			physical = FALSE;
		else if (strcmp(*vp, "-P") == 0)
			physical = TRUE;
		else 
			flag_error = TRUE;
		/* no longer need this argument */
		xfree(*vp);
		vp++;
	}							/* end S006 */

	/* get new directory name */
	dp = *vp;

	if (dp)
		dp = globone(dp);
	else {
		dp = value("home");
		if (*dp == 0)
			bferr("No home");
	}

	if (flag_error) {					/* begin S006 */
		/* free the dir argument */
		if (*vp)
			xfree(dp);
		bferr("bad option(s)");
	}							/* end S006 */

	/* try to change directory to new path */
	i = cd(dp, physical);				/* S000 */ /* S006 */

	/* if cd failed and not an absolute path, check other possible paths */
	if (i < 0 && dp[0] != '/') {
		struct varent *c = adrof("cdpath");

		if (c == 0)
			goto simple;
		for (cdp = c->vec; *cdp; cdp++) {
			char buf[BUFSIZ];

			strcpy(buf, *cdp);
			strcat(buf, "/");
			strcat(buf, dp);
			i = cd(buf, physical);		/* S000 */ /* S006 */
			if (i >= 0)
				goto simple;
		}
	}
simple:

	/* if we still fail and the dir argument is an env variable, try it */
	if (i < 0 && adrof(dp)) {
		char *cp = value(dp);

		if (cp[0] == '/')
			i = cd(cp, physical);		/* S000 */ /* S006 */
	}

	/* free the dir argument */
	if (*vp)
		xfree(dp);

	/* if we still failed, report error */
	if (i < 0)
		Perror(dp);
}

prvars()
{

	plist(&shvhed);
}

doalias(v)
	register char **v;
{
	register struct varent *vp;
	register char *p;

	v++;
	p = *v++;
	if (p == 0)
		plist(&aliases);
	else if (*v == 0) {
		vp = adrof1(strip(p), &aliases);
		if (vp)
			blkpr(vp->vec), printf("\n");
	} else {
		if (eq(p, "alias") || eq(p, "unalias")) {
			setname(p);
			bferr("Too dangerous to alias that");
		}
		set1(strip(p), saveblk(v), &aliases);
	}
}

unalias(v)
	char **v;
{

	unset1(v, &aliases);
}

dologout()
{

	islogin();
	goodbye();
}

#ifdef	LOGIN
dologin(v)
	char **v;
{

	islogin();
	execl("/bin/login", "login", v[1], 0);
	exit(1);
}
#endif

donewgrp(v)
	char **v;
{

	signal(SIGINT, SIG_DFL);			/* M002 */
	signal(SIGQUIT, SIG_DFL);			/* M002 */
	signal(SIGTERM, SIG_DFL);			/* M002 */
#ifndef V6
	execlp("newgrp", "newgrp", v[1], 0);
#endif
	execl("/bin/newgrp", "newgrp", v[1], 0);	/* just in case */
	execl("/usr/bin/newgrp", "newgrp", v[1], 0);
	signal(SIGINT, SIG_IGN);			/* M002 */
	signal(SIGQUIT, SIG_IGN);			/* M002 */
	signal(SIGTERM, SIG_IGN);			/* M002 */
}

islogin()
{

	if (loginsh)
		return;
	error("Not login shell");
}

doif(v, kp)
	char **v;
	struct command *kp;
{
	register int i;
	register char **vv;

	v++;
	i = expa(&v);
	vv = v;
	if (*vv && eq(*vv, "then")) {
		vv++;
		if (*vv)
			bferr("Improper then");
		setname("then");
		/*
		 * If expression was zero, then scan to else,
		 * otherwise just fall into following code.
		 */
		if (!i)
			search(ZIF, 0);
		return;
	}
	/*
	 * Simple command attached to this if.
	 * Left shift the node in this tree, munging it
	 * so we can reexecute it.
	 */
	if (i) {
		lshift(kp->t_dcom, vv - kp->t_dcom);
		reexecute(kp);
		donefds();
	}
}

/*
 * Reexecute a command, being careful not
 * to redo i/o redirection, which is already set up.
 */
reexecute(kp)
	register struct command *kp;
{

	kp->t_dflg = FREDO;
	execute(kp);
}

doelse()
{

	search(ZELSE, 0);
}

dogoto(v)
	char **v;
{
	register struct whyle *wp;
	char *lp;

	/*
	 * While we still can, locate any unknown ends of existing loops.
	 * This obscure code is the WORST result of the fact that we
	 * don't really parse.
	 */
	for (wp = whyles; wp; wp = wp->w_next)
		if (wp->w_end == 0)
			wp->w_end = search(ZBREAK, 0);
		else
			bseek(wp->w_end);
	search(ZGOTO, 0, lp = globone(v[1]));
	xfree(lp);
	/*
	 * Eliminate loops which were exited.
	 */
	wfree();
}

doswitch(v)
	register char **v;
{
	register char *cp, *lp;

	v++;
	if (!*v || *(*v++) != '(')
		goto Lsyntax;
	cp = **v == ')' ? "" : *v++;
	if (*(*v++) != ')')
		v--;
	if (*v)
Lsyntax:
		error("Syntax error");
	search(ZSWITCH, 0, lp = globone(cp));
	xfree(lp);
}

dobreak()
{

	if (whyles)
		toend();
	else
		bferr("Not in while/foreach");
}

doexit(v)
	char **v;
{

	/*
	 * Don't DEMAND parentheses here either.
	 */
	v++;
	if (*v) {
		set("status", putn(expa(&v)));
		if (*v)
			bferr("Expression syntax");
	}
	btoeof();
	if (intty)
		close(SHIN);
}

doforeach(v)
	register char **v;
{
	register char *cp;
	register struct whyle *nwp;

	v++;
	cp = strip(*v);
	if (!letter(*cp))					/* L002 { */
		bferr("Variable syntax");
	if (strlen(*v) > MAX_NAME_LEN)
		bferr("Loop variable name too long");		/* L002 } */
	while (*cp && letter(*cp))
		cp++;
	if (*cp)						/* L002 */
		bferr("Invalid variable");
	cp = *v++;
	if (v[0][0] != '(' || v[blklen(v) - 1][0] != ')')
		bferr("Words not ()'ed");
	v++;
	gflag = 0, rscan(v, tglob);
	v = glob(v);
	if (v == 0)
		bferr("No match");
	nwp = (struct whyle *) Calloc(1, sizeof *nwp);		/* L003 */
	nwp->w_fe = nwp->w_fe0 = v; gargv = 0;
	nwp->w_start = btell();
	nwp->w_fename = savestr(cp);
	nwp->w_next = whyles;
	whyles = nwp;
	/*
	 * Pre-read the loop so as to be more
	 * comprehensible to a terminal user.
	 */
	if (intty)
		preread();
	doagain();
}

dowhile(v)
	char **v;
{
	register int status;
	register bool again = whyles && whyles->w_start == lineloc && whyles->w_fename == 0;

	v++;
	/*
	 * Implement prereading here also, taking care not to
	 * evaluate the expression before the loop has been read up
	 * from a terminal.
	 */
	if (intty && !again)
		status = !exp0(&v, 1);
	else
		status = !expa(&v);
	if (*v)
		bferr("Expression syntax");
	if (!again) {
								/* L003 */
		register struct whyle *nwp = (struct whyle *) Calloc(1, sizeof (*nwp));

		nwp->w_start = lineloc;
		nwp->w_end = 0;
		nwp->w_next = whyles;
		whyles = nwp;
		if (intty) {
			/*
			 * The tty preread
			 */
			preread();
			doagain();
			return;
		}
	}
	if (status)
		/* We ain't gonna loop no more, no more! */
		toend();
}

preread()
{
	register void (*oldint)();

	whyles->w_end = -1;
	if (setintr)
		oldint = signal(SIGINT, pintr);
	search(ZBREAK, 0);
	if (setintr)
		signal(SIGINT, oldint);
	whyles->w_end = btell();
}

doend()
{

	if (!whyles)
		bferr("Not in while/foreach");
	whyles->w_end = btell();
	doagain();
}

docontin()
{

	if (!whyles)
		bferr("Not in while/foreach");
	doagain();
}

doagain()
{

	/* Repeating a while is simple */
	if (whyles->w_fename == 0) {
		bseek(whyles->w_start);
		return;
	}
	/*
	 * The foreach variable list actually has a spurious word
	 * ")" at the end of the w_fe list.  Thus we are at the
	 * of the list if one word beyond this is 0.
	 */
	if (!whyles->w_fe[1]) {
		dobreak();
		return;
	}
	set(whyles->w_fename, savestr(*whyles->w_fe++));
	bseek(whyles->w_start);
}

dorepeat(v, kp)
	char **v;
	struct command *kp;
{
	register int i;
	register void (*saveintr)();

	i = getn(v[1]);
	if (setintr)
		saveintr = signal(SIGINT, SIG_IGN);
	lshift(v, 2);
	while (i > 0) {
		if (setintr)
			signal(SIGINT, pintr);
		reexecute(kp);
		--i;
	}
	donefds();
	if (setintr)
		signal(SIGINT, saveintr);
}

doswbrk()
{

	search(ZBRKSW, 0);
}

srchx(cp)
	register char *cp;
{
	register struct srch *sp;

	for (sp = srchn; sp->s_name; sp++)
		if (eq(cp, sp->s_name))
			return (sp->s_value);
	return (-1);
}

char	Stype;
char	*Sgoal;

/*VARARGS2*/
search(type, level, goal)
	int type;
	register int level;
	char *goal;
{
	char wordbuf[BUFSIZ];
	register char *aword = wordbuf;
	register char *cp;

	Stype = type; Sgoal = goal;
	if (type == ZGOTO)
		bseek(0l);
	do {
		if (intty && fseekp == feobp)
			printf("? "), flush();
		aword[0] = 0, getword(aword);
		switch (srchx(aword)) {

		case ZELSE:
			if (level == 0 && type == ZIF)
				return;
			break;

		case ZIF:
			while (getword(aword))
				continue;
			if ((type == ZIF || type == ZELSE) && eq(aword, "then"))
				level++;
			break;

		case ZENDIF:
			if (type == ZIF || type == ZELSE)
				level--;
			break;

		case ZFOREACH:
		case ZWHILE:
			if (type == ZBREAK)
				level++;
			break;

		case ZEND:
			if (type == ZBREAK)
				level--;
			break;

		case ZSWITCH:
			if (type == ZSWITCH || type == ZBRKSW)
				level++;
			break;

		case ZENDSW:
			if (type == ZSWITCH || type == ZBRKSW)
				level--;
			break;

		case ZLABEL:
			if (type == ZGOTO && getword(aword) && eq(aword, goal))
				level = -1;
			break;

		default:
			if (type != ZGOTO && (type != ZSWITCH || level != 0))
				break;
			if (lastchr(aword) != ':')
				break;
			aword[strlen(aword) - 1] = 0;
			if (type == ZGOTO && eq(aword, goal) || type == ZSWITCH && eq(aword, "default"))
				level = -1;
			break;

		case ZCASE:
			if (type != ZSWITCH || level != 0)
				break;
			getword(aword);
			if (lastchr(aword) == ':')
				aword[strlen(aword) - 1] = 0;
			cp = strip(Dfix1(aword));
			if (Gmatch(goal, cp))
				level = -1;
			xfree(cp);
			break;

		case ZDEFAULT:
			if (type == ZSWITCH && level == 0)
				level = -1;
			break;
		}
		getword(0);
	} while (level >= 0);
}

getword(wp)
	register char *wp;
{
	register int found = 0;
	register int c, d;

	c = readc(1);
	d = 0;
	do {
		while (c == ' ' || c == '\t')
			c = readc(1);
		if (c < 0)
			goto past;
		if (c == '\n') {
			if (wp)
				break;
			return (0);
		}
		unreadc(c);
		found = 1;
		do {
			c = readc(1);
			if (c == '\\' && (c = readc(1)) == '\n')
				c = ' ';
			if (any(c, "'\""))
				if (d == 0)
					d = c;
				else if (d == c)
					d = 0;
			if (c < 0)
				goto past;
			if (wp)
				*wp++ = c;
		} while ((d || c != ' ' && c != '\t') && c != '\n');
	} while (wp == 0);
	unreadc(c);
	if (found)
		*--wp = 0;
	return (found);

past:
	switch (Stype) {

	case ZIF:
		bferr("then/endif not found");

	case ZELSE:
		bferr("endif not found");

	case ZBRKSW:
	case ZSWITCH:
		bferr("endsw not found");

	case ZBREAK:
		bferr("end not found");

	case ZGOTO:
		setname(Sgoal);
		bferr("label not found");
	}
	/*NOTREACHED*/
}

toend()
{

	if (whyles->w_end == 0) {
		search(ZBREAK, 0);
		whyles->w_end = btell() - 1;
	} else
		bseek(whyles->w_end);
	wfree();
}

wfree()
{
	long o = btell();

	while (whyles) {
		register struct whyle *wp = whyles;
		register struct whyle *nwp = wp->w_next;

		if (o >= wp->w_start && (wp->w_end == 0 || o < wp->w_end))
			break;
		if (wp->w_fe0)
			blkfree(wp->w_fe0);
		if (wp->w_fename)
			xfree(wp->w_fename);
		xfree(wp);
		whyles = nwp;
	}
}

doecho(v)
	char **v;
{

	echo(' ', v);
}

doglob(v)
	char **v;
{

	echo(0, v);
	flush();
}

echo(sep, v)
	char sep;
	register char **v;
{
	register char *cp;
	void (*saveintr)();
	if (setintr)
		saveintr = signal(SIGINT, pintr);

	v++;
	if (*v == 0)
		return;
	gflag = 0; rscan(v, tglob);
	if (gflag) {
		v = glob(v);
		if (v == 0)
			bferr("No match");
	} else
		scan(v, trim);
	while (cp = *v++) {
		register int c;

		while (c = *cp++) {
			if (sep == ' ' && *cp && c == '\\') {
				c = *cp++;
				if (c == 'c') {
					flush();
					return;
				} else if (c == 'n')
					c = '\n';
				else
					putchar('\\');
			}
			putchar(c | QUOTE);
		}
		if (*v)
			putchar(sep | QUOTE);
	}
	if (sep)
		putchar('\n');
	if (setintr)
		signal(SIGINT, saveintr);
	if (gargv)
		blkfree(gargv), gargv = 0;
}

#ifndef	V6
extern char	**environ;					/* L005 */

dosetenv(v)
	register char **v;
{
	char *lp = globone(v[2]);

	setenv(v[1], lp);
	if (eq(v[1], "PATH"))
		importpath(lp);
	xfree(lp);
}

setenv(name, value)
	char *name, *value;
{
	register char **ep = environ;
	register char *cp, *dp;
	char *blk[2], **oep = ep;

	for (; *ep; ep++) {
		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;
		cp = strspl("=", value);
		xfree(*ep);
		*ep = strspl(name, cp);
		xfree(cp);
		scan(ep, trim);
		return;
	}
	blk[0] = strspl(name, "="); blk[1] = 0;
	environ = blkspl(environ, blk);
	xfree(oep);
	setenv(name, value);
}

int
dounsetenv(v, c)
	register char **v;
	struct command *c;
{
	char **ep, *p, *n;
	int i, maxi;
	static char *name = (char *) 0;

	if (name)
		xfree(name);

	/*
	 * Find the longest environment variable.
	 */
	for (maxi = 0, ep = environ; *ep; ep++) {
		for (i = 0, p = *ep; *p && *p != '='; p++, i++)
			continue;
		if (i> maxi)
			maxi = i;
	}

	/*
	 * Malloc enough space for the longest variable plus a bit for the
	 * NULL byte at the end.
	 */
	name = (char *) malloc((size_t) ((maxi + 1) * sizeof(char)));

	/*
	 * while we have names to unset.
	 */
	while (++v && *v)
		/*
		 * This loop terminates when the inner loop runs out of
		 * environment variables to process.
		 */
		for (maxi = 1; maxi ;)
			for (maxi = 0, ep = environ; *ep; ep++) {
				/*
				 * copy the name of this environment variable
				 * into name.
				 */
				for (n = name, p = *ep; *p && *p != '='; *n++ = *p++)
					continue;
				*n = '\0';

				/*
				 * If it doesn't match the one we want to unset
				 * continue.
				 */
				if (!Gmatch(name, *v))
					continue;
				maxi = 1;

				/*
				 * Unset the name.
				 */
				Unsetenv(name);

				/*
				 * start again cause the environment changes
				 */
				break;
			}
	xfree(name);
	name = (char *) 0;
	return (0);
}

static void
Unsetenv(name)
	char *name;
{
	register char **ep = environ;
	register char *cp, *dp;
	char **oep = ep;

	for (; *ep; ep++) {
		for (cp = name, dp = *ep; *cp && *cp == *dp; cp++, dp++)
			continue;
		if (*cp != 0 || *dp != '=')
			continue;
		cp = *ep;
		*ep = 0;
		environ = blkspl(environ, ep + 1);
		*ep = cp;
		xfree(cp);
		xfree(oep);
		return;
	}
}
/*								L004 } */

doumask(v)
	register char **v;
{
	register char *cp;					/* L001 begin */
	char *endptr;
	mode_t mode;
	int symbolic_mode = 0;

	cp = *++v;
	while (cp && *cp == '-') {
		/* -- marks end of options */
		if (*(cp+1) == '-' && *(cp+2) == '\0') {
			cp = *++v;
			break;
		}
		if (*(cp+1) == 'S' && *(cp+2) == '\0')
			symbolic_mode++;
		else
			bferr("Bad option");
		cp = *++v;
	}

	if (cp == 0) {
		umask(mode = umask(mode));
		if (symbolic_mode) {
			cp = permtostr(mode);
			printf("%s\n", cp);
		} else
			printf("%o\n", mode);
		return;
	}
	if (digit(*cp)) {
		mode = 0;
		while (digit(*cp) && *cp != '8' && *cp != '9')
			mode = mode * 8 + *cp++ - '0';
		if (*cp || mode < 0 || mode > 0777)
			bferr("Improper mask");
	} else {
		mode = umask(0);
		if (strperm(cp, (mode_t) ~mode, &mode) == -1) {
			umask(mode);
			bferr("Improper mask");
		}
		mode = ~mode;
	}
	umask(mode);						/* L001 end */
}
#endif

								/* begin S006 */
cd(dir, physical)
	register char *dir;
	int physical;
{
	register rval = -1;
	unsigned char tmp_cwdname[MAXPWD];

	if (!physical) {
		memset(tmp_cwdname, 0, MAXPWD);

		if (*dir == '/' )
			strcpy((char *)tmp_cwdname, dir);
		else {
			if (!strlen((char *)cwdname))
				getcwd(cwdname, MAXPWD-1);
			sprintf(tmp_cwdname, "%s/%s", cwdname, dir);
		}

		pwdalmost((char *)tmp_cwdname, strlen((char *)cwdname), physical);

	}
	else
		strcpy((char *)tmp_cwdname, dir);

	if ((rval = physical_cd(tmp_cwdname)) >= 0)
		if (physical)
			getcwd(cwdname, MAXPWD-1);
		else
			strcpy((char *)cwdname, (char *)tmp_cwdname);

	return rval;
}								/* end S006 */

/* Begin SCO_BASE S000
 *
 * From cithep!norman Wed Apr 20 05:37:56 1983
 * Routine added by Rob Pike Mar/80 to call spname if bad dir,
 * modified to work with C-shell not Bourne shell.
 */

#define	EXECUTE	01		/* is it searchable */

/*
 * fancy ifdef'ing renames the following function.
 * Be sure to take care if modifying code here.
 */
physical_cd(dir)
	register char *dir;
{
	extern char *spname(), *strrchr();
	register rval;
	register c;
	register char *end;
	struct stat sb;
	int saverrno;
	char *tmpdir;

	if((rval=chdir(dir)) < 0 && adrof("cdspell") && intty) {
		saverrno = errno;
		if((tmpdir=spname(dir)) && (stat(tmpdir,&sb)>=0)) {
			if (!isdir(sb)) {
				end = strrchr(tmpdir, '/');
				if (end) {
					while (end != tmpdir && *end == '/')
						--end;
					*(end+1) = '\0';
				}
				else {
					errno = saverrno;
					return (rval);
				}
			}
			else if (access(tmpdir, EXECUTE) < 0) {
				errno = saverrno;
				return (rval);
			}
			printf("cd %s? ", tmpdir);
			flush();
			while ((c = readc(0)) == ' ' || c == '\t')
				;
			if(c!='n' && c!='N'){
				printf("ok\n");
				rval = chdir(tmpdir);
				saverrno = errno;
				strcpy(dir, tmpdir);		/* S006 */
			}
			while (c != '\n')
				c = readc(0);
		}
		errno = saverrno;
	}
	return(rval);
}

/* End SCO_BASE S000 */


static int							/* L001 begin */
strperm(char *expr, mode_t old_umask, mode_t *new_umask)
{
	action_atom *action;

	if ((action = comp_mode(expr, 0)) == (action_atom *) 0)
		return(-1);

	*new_umask = exec_mode(old_umask, action);

	Free((char *) action);					/* L003 */

	return(1);
}

/*
 * produce a symbolic string based upon the file mode creation mask
 */
static char *
permtostr(register mode_t flag)
{
	register char *str = permstr;
	int i;

	flag = ~flag;

	for (i=0; i < 3; flag = (flag << 3) & 0777, i++) {
		switch(i) {
			case 0 :
				*str++ = 'u';
				break;
			case 1 :
				*str++ = 'g';
				break;
			default:
				*str++ = 'o';
				/* FALLTHROUGH */
		}
		*str++ = '=';

		if (flag & S_IRUSR)
			*str++ = 'r';
		if (flag & S_IWUSR)
			*str++ = 'w';
		if (flag & S_IXUSR)
			*str++ = 'x';
		*str++ = ',';
	}
	*--str = '\0';

	return(permstr);
}								/* L001 end */

								/* begin S006 */
/*
 * We have an `almost' valid path in cwdname.
 * Components in the path could be symbolic links that need expanding
 * This module was derived from path_physical() in the korn shell
 */
static
pwdalmost(unsigned char *cwd, unsigned int index, unsigned int physical)
{
	register unsigned char *cp = &cwd[index];
	register unsigned char *savecp;
	struct stat buf;
	char buffer[PATHSIZE];
	int c, n;

	memset(buffer,0,sizeof buffer);			
	while (*cp) {
		/* skip over '/' */
		savecp = cp+1;
		while (*cp == '/')
			cp++;
		/* eliminate multiple slashes */
		if (cp > savecp) {
			strcpy((char *)savecp, (char *)cp);
			cp = savecp;
		}
		/* check for .. */
		if (*cp == '.') {
			switch(cp[1]) {
				case '\0':
				case '/':
					/* eliminate /. */
					cp--;
					strcpy((char *)cp, (char *)cp+2);
					continue;
				case '.':
					if (cp[2] == '/' || cp[2] == '\0') {
						/* backup, but not past root */
						savecp = cp+2;
						cp--;
						while (cp > cwd && *--cp != '/')
							;
						strcpy((char *)cp, (char *)savecp);
						continue;
					}
					break;
			}
		}
		savecp = cp;
		/* goto end of component */
		while (*cp && *cp != '/')
			cp++;
		c = *cp;
		*cp = '\0';
		if (lstat((char *)cwd, &buf) == -1 || (S_ISLNK(buf.st_mode) && (n = readlink(cwd, buffer, PATHSIZE)) == -1)) {
			didpwd = FALSE;
			return;
		}

		*cp = c;
		if (S_ISLNK(buf.st_mode) && n > 0 && physical) {
			/* process a symbolic link */
	/* The automounter, for a direct map entry, creates a symlink 
	 * 1K (aka PATHSIZE) in size.  Don't just blindly write into
	 * buffer without checking the size.  If overflow, try to recover
	 * by setting n to the length of the pathname returned by readlink().
	 * Now let's just hope that the data in the block holding the
	 * automounter's symlink is zero'ed out so that strlen() returns
	 * a reasonable value.
	 */
			if (n+strlen((char *)cp) >= PATHSIZE) {	
				n=strlen(buffer);
				if (n+strlen((char *)cp) >= PATHSIZE) {
					didpwd = FALSE;
					return;
				}
			}
			strcpy(buffer+n, (char *)cp);
			if (*buffer == '/') {
				strcpy((char *)cwd, buffer);
				cp = cwd;
			} else {
				/* check for path overflow */
				cp = savecp;
				if ((strlen(buffer)+(cp-cwd)-1) >= MAXPWD) {
					didpwd = FALSE;
					return;
				}
				strcpy((char *)cp, buffer);
			}
		}
	}
	if (cp==cwd) {
		*cp = '/';
		*++cp = '\0';
	} else
		while (--cp > cwd && *cp == '/')
			/* eliminate trailing slashes */
			*cp = '\0';

	/* all of cwd is now valid */
	/* validto = cp; */
	didpwd = TRUE;

	return;
}

dopwd(vp)
	register char **vp;
{
	register int i;
	register char *fp = 0;
	int physical = (adrof("cdlogical"))?FALSE:TRUE;
	unsigned char tmp_cwdname[MAXPWD+1];

	memset(tmp_cwdname, 0, MAXPWD+1);

	vp++;
	if (*vp && **vp == '-' ) {
		fp = *vp;
		if (strcmp(fp, "-L") == 0)
			physical = FALSE;
		else if (strcmp(fp, "-P") == 0)
			physical = TRUE;
		else
			bferr("Bad pwd flag"); /* this needs to be postponed */
	}

	if (*vp)
		xfree(fp);

	if (physical) {
		if (getcwd(tmp_cwdname, MAXPWD) < 0) {
			Perror("pwd");
		}
	}
	else {
		if (!strlen((char *)cwdname)) {
			if (getcwd(cwdname, MAXPWD-1) < 0) {
				Perror("pwd");
			}
		}
		
		strcpy((char *)tmp_cwdname, (char *)cwdname);
	}
	printf("%s\n", tmp_cwdname);
}								/* end S006 */

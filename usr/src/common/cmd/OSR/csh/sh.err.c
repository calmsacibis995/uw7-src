#ident	"@(#)OSRcmds:csh/sh.err.c	1.1"
/*
 *	@(#) sh.err.c 26.1 95/12/14 
 *
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

/* #ident	"@(#)csh:sh.err.c	1.2" */
#pragma comment(exestr, "@(#) sh.err.c 26.1 95/12/14 ")

/*
 * Modification History
 *
 *	L000	08 Dec 1995	scol!ianw
 *	- Moved the call to btoeof() to after the conditional call to exit().
 *	  As the parent and child shells share the same input file pointer,
 *	  calling btoeof (which seeks to the end of the input file) in the
 *	  child shell when the input is being read from a file (as opposed
 *	  to a terminal) causes subsequent reads by the parent shell to return
 *	  with no characters.
 */

/*
 *	@(#) sh.err.c 1.1 88/03/29 csh:sh.err.c
 */
/*	@(#)sh.err.c	2.1	SCCS id keyword	*/
/* Copyright (c) 1980 Regents of the University of California */
#include "sh.h"

/*
 * C Shell
 */

bool	errspl;			/* Argument to error was spliced by seterr2 */
char	one[2] = { '1', 0 };
char	*onev[2] = { one, NOSTR };
/*
 * Print error string s with optional argument arg.
 * This routine always resets or exits.  The flag haderr
 * is set so the routine who catches the unwind can propogate
 * it if they want.
 *
 * Note that any open files at the point of error will eventually
 * be closed in the routine process in sh.c which is the only
 * place error unwinds are ever caught.
 */
error(s, arg)
	char *s;
{
	register char **v;
	register char *ep;

	/*
	 * Must flush before we print as we wish output before the error
	 * to go on (some form of) standard output, while output after
	 * goes on (some form of) diagnostic output.
	 * If didfds then output will go to 1/2 else to FSHOUT/FSHDIAG.
	 * See flush in sh.print.c.
	 */
	flush();
	haderr = 1;		/* Now to diagnostic output */
	timflg = 0;		/* This isn't otherwise reset */
	if (v = pargv)
		pargv = 0, blkfree(v);
	if (v = gargv)
		gargv = 0, blkfree(v);

	/*
	 * A zero arguments causes no printing, else print
	 * an error diagnostic here.
	 */
	if (s)
		printf(s, arg), printf(".\n");

	didfds = 0;		/* Forget about 0,1,2 */
	if ((ep = err) && errspl) {
		errspl = 0;
		xfree(ep);
	}
	errspl = 0;

	/*
	 * Go away if -e or we are a child shell
	 */
	if (exiterr || child)
		exit(1);

	/*
	 * Reset the state of the input.
	 * This buffered seek to end of file will also
	 * clear the while/foreach stack.
	 */
	btoeof();						/* L000 */

	setq("status", onev, &shvhed);
	reset();		/* Unwind */
}

/*
 * Perror is the shells version of perror which should otherwise
 * never be called.
 */
Perror(s)
	char *s;
{

	/*
	 * Perror uses unit 2, thus if we didn't set up the fd's
	 * we must set up unit 2 now else the diagnostic will disappear
	 */
	if (!didfds) {
		register int oerrno = errno;

		dcopy(SHDIAG, 2);
		errno = oerrno;
	}
	perror(s);
	error(0);		/* To exit or unwind */
}

/*
 * For builtin functions, the routine bferr may be called
 * to print a diagnostic of the form:
 *	name: Diagnostic.
 * where name has been setup by setname.
 * (Made into a macro to save space)
 *
char	*bname;

setname(cp)
	char *cp;
{

	bname = cp;
}
 */

bferr(cp)
	char *cp;
{

	flush();
	haderr = 1;
	printf("%s: ", bname);
	error(cp);
}

/*
 * The parser and scanner set up errors for later by calling seterr,
 * which sets the variable err as a side effect; later to be tested,
 * e.g. in process.
 */
seterr(s)
	char *s;
{

	if (err == 0)
		err = s, errspl = 0;
}

/* Set err to a splice of cp and dp, to be freed later in error() */
seterr2(cp, dp)
	char *cp, *dp;
{

	if (err)
		return;
	err = strspl(cp, dp);
	errspl++;
}

/* Set err to a splice of cp with a string form of character d */
seterrc(cp, d)
	char *cp, d;
{
	char chbuf[2];

	chbuf[0] = d;
	chbuf[1] = 0;
	seterr2(cp, chbuf);
}

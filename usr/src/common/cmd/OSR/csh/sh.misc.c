#ident	"@(#)OSRcmds:csh/sh.misc.c	1.1"
#pragma comment(exestr, "@(#) sh.misc.c 25.3 94/02/18 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1994 The Santa Cruz Operation, Inc
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

/* #ident	"@(#)csh:sh.misc.c	1.2" */

/*
 *	L000	scol!hughd	19nov90
 *	- NOFILE is now configurable, so get it from sysconf()
 *	L001	scol!hughd	24apr92
 *	- corrected fallback if sysconf(_SC_OPEN_MAX) fails(!)
 *	  from 20 to 60; confirmed that when this module is
 *	  built in the standard way, the #ifdefs work out that
 *	  sysconf(_SC_OPEN_MAX) is indeed used for NOFILE
 *	L002	scol!sohrab	4aug92
 *	- Using the sysconf(_SC_OPEN_MAX) returns the NOFILE value 
 *	  configured into the system which may be up to 11000.
 *	  To avoid calling close() upto the system limit, 
 *	  fcntl( , F_GETHFDO, ) is called first to get the 
 *	  highest file descriptor opened by the process
 *	  and we call close() for the returned values. 
 *	L003	scol!gregw	17feb94
 *	- renamed malloc, calloc, free, and cfree to Malloc, Calloc, Free,
 *	  and Cfree to avoid clash with libc.
 *	   
 */

/*
 *	@(#) sh.misc.c 1.1 88/03/29 csh:sh.misc.c
 */
/*	@(#)sh.misc.c	2.1	SCCS id keyword	*/
/* Copyright (c) 1980 Regents of the University of California */
#include "sh.h"
#include <sys/unistd.h>						/* L000 begin */
#include <sys/fcntl.h>						/* L002 */
#include "../include/osr.h"

#ifndef NOFILE
#define NOFILE 	nofile()
static int
nofile()
{
	extern long sysconf(int);
	extern int fcntl( int, int, int);			/* L002 */
	int nofiles;						/* L002 */

	nofiles=f_gethfdo();

	return((int)nofiles);
}
#endif /* NOFILE */						/* L000 end */

/*
 * C Shell
 */

letter(c)
	register char c;
{

	return (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_');
}

digit(c)
	register char c;
{

	return (c >= '0' && c <= '9');
}

alnum(c)
	register char c;
{
	return (letter(c) || digit(c));
}

any(c, s)
	register int c;
	register char *s;
{

	while (*s)
		if (*s++ == c)
			return(1);
	return(0);
}

char *
Calloc(i, j)							/* L003 */
	register int i;
	int j;
{
	register char *cp, *dp;
#ifdef debug
	static char *av[2] = {0, 0};
#endif

	i *= j;
	cp = (char *) Malloc(i);				/* L003 */
	if (cp == 0) {
		child++;
#ifndef debug
		error("Out of memory");
#else
		showall(av);
		printf("i=%d, j=%d: ", i/j, j);
		printf("Out of memory\n");
		chdir("/usr/bill/cshcore");
		abort();
#endif
	}
	dp = cp;
	if (i > 0)
		do
			*dp++ = 0;
		while (--i);
	return (cp);
}

Cfree(p)							/* L003 */
	char *p;
{

	Free(p);						/* L003 */
}

char **
blkend(up)
	register char **up;
{

	while (*up)
		up++;
	return (up);
}
 
blkpr(av)
	register int *av;
{

	for (; *av; av++) {
		printf("%s", *av);
		if (av[1])
			printf(" ");
	}
}

blklen(av)
	register char **av;
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

char **
blkcpy(oav, bv)
	char **oav;
	register char **bv;
{
	register char **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

char **
blkcat(up, vp)
	char **up, **vp;
{

	blkcpy(blkend(up), vp);
	return (up);
}

blkfree(av0)
	char **av0;
{
	register char **av = av0;

	while (*av)
		xfree(*av++);
	xfree(av0);
}

char **
saveblk(v)
	register char **v;
{
	register int len = blklen(v) + 1;
								/* L003 */
	register char **newv = (char **) Calloc(len, sizeof (char **));
	char **onewv = newv;

	while (*v)
		*newv++ = savestr(*v++);
	return (onewv);
}

char *
strspl(cp, dp)
	register char *cp, *dp;
{
								/* L003 */
	register char *ep = Calloc(1, strlen(cp) + strlen(dp) + 1);

	strcpy(ep, cp);
	return (strcat(ep, dp));
}

char **
blkspl(up, vp)
	register char **up, **vp;
{
								/* L003 */
	register char **wp = (char **) Calloc(blklen(up) + blklen(vp) + 1, sizeof (char **));

	blkcpy(wp, up);
	return (blkcat(wp, vp));
}

lastchr(cp)
	register char *cp;
{

	if (!*cp)
		return (0);
	while (cp[1])
		cp++;
	return (*cp);
}

/*
 * This routine is called after an error to close up
 * any units which may have been left open accidentally.
 */
closem()
{
	register int f;

	for (f = 0; f < NOFILE; f++)
		if (f != SHIN && f != SHOUT && f != SHDIAG && f != OLDSTD)
			close(f);
}

/*
 * Close files before executing a file.
 * We could be MUCH more intelligent, since (on a version 7 system)
 * we need only close files here during a source, the other
 * shell fd's being in units 16-19 which are closed automatically!
 */
closech()
{
	register int f;

	if (didcch)
		return;
	didcch = 1;
	SHIN = 0; SHOUT = 1; SHDIAG = 2; OLDSTD = 0;
	for (f = 3; f < NOFILE; f++)
		close(f);
}

donefds()
{

	close(0), close(1), close(2);
	didfds = 0;
}

/*
 * Move descriptor i to j.
 * If j is -1 then we just want to get i to a safe place,
 * i.e. to a unit > 2.  This also happens in dcopy.
 */
dmove(i, j)
	register int i, j;
{

	if (i == j || i < 0)
		return (i);
#ifdef V7
	if (j >= 0) {
		dup2(i, j);
		return (j);
	} else
#endif
		j = dcopy(i, j);
	if (j != i)
		close(i);
	return (j);
}

dcopy(i, j)
	register int i, j;
{

	if (i == j || i < 0 || j < 0 && i > 2)
		return (i);
#ifdef V7
	if (j >= 0) {
		dup2(i, j);
		return (j);
	}
#endif
	close(j);
	return (renum(i, j));
}

renum(i, j)
	register int i, j;
{
	register int k = dup(i);

	if (k < 0)
		return (-1);
	if (j == -1 && k > 2)
		return (k);
	if (k != j) {
		j = renum(k, j);
		close(k);
		return (j);
	}
	return (k);
}

copy(to, from, size)
	register char *to, *from;
	register int size;
{

	if (size)
		do
			*to++ = *from++;
		while (--size != 0);
}

/*
 * Left shift a command argument list, discarding
 * the first c arguments.  Used in "shift" commands
 * as well as by commands like "repeat".
 */
lshift(v, c)
	register char **v;
	register int c;
{
	register char **u = v;

	while (*u && --c >= 0)
		xfree(*u++);
	blkcpy(v, u);
}

number(cp)
	char *cp;
{

	if (*cp == '-') {
		cp++;
		if (!digit(*cp++))
			return (0);
	}
	while (*cp && digit(*cp))
		cp++;
	return (*cp == 0);
}

char **
copyblk(v)
	register char **v;
{
								/* L003 */
	register char **nv = (char **) Calloc(blklen(v) + 1, sizeof (char **));

	return (blkcpy(nv, v));
}

char *
strend(cp)
	register char *cp;
{

	while (*cp)
		cp++;
	return (cp);
}

char *
strip(cp)
	char *cp;
{
	register char *dp = cp;

	while (*dp++ &= TRIM)
		continue;
	return (cp);
}

udvar(name)
	char *name;
{

	setname(name);
	bferr("Undefined variable");
}

#ident	"@(#)OSRcmds:csh/sh.set.c	1.1"
#pragma comment(exestr, "@(#) sh.set.c 25.3 94/02/18 ")
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

/* #ident	"@(#)csh:sh.set.c	1.2" */

/*
 *	@(#) sh.set.c 1.1 88/03/29 csh:sh.set.c
 */
/*	@(#)sh.set.c	2.1	SCCS id keyword	*/
/* Copyright (c) 1980 Regents of the University of California */

/*
 * Modifications:
 *
 * S001 waynef@sco 08/20/90
 *	- fixed exportpath() to check for overflow of it's local buffer
 *	- doubled size of that buffer
 * L002 scol!dipakg 31 Oct 90
 *	- allow for long filenames
 * L003	scol!hughd	16jul92
 *	- stop compiler warning with 3.2.5 DS (overflow in constant
 *	  arithmetic): define MINLONG as 0x80000000 instead of expression
 * L004 scol!gregw	01jul93
 *	- added checks for long variable names. From greim@sbsvax.UUCP
 *	  (Michael Greim).
 * L005	scol!gregw	17feb94
 *	- Renamed calloc to Calloc and cfree to Cfree to avoid possible
 *	  conflict with libc.
 *
 */

#include "sh.h"
#include <sys/param.h>			/* L002 */
#include "../include/osr.h"

/*
 * C Shell
 */

doset(v)
	register char **v;
{
	register char *p;
	char *vp, op;
	char **vecp;
	bool hadsub;
	int subscr;
	int i;							/* L004 { */
	char name[MAX_NAME_LEN + 1];				/* L004 } */

	v++;
	p = *v++;
	if (p == 0) {
		prvars();
		return;
	}
	do {
		hadsub = 0;
		if (!letter(*p))				/* L004 { */
			goto setsyn;
		for (vp = p, i = 0; alnum(*p); p++)
			if (i <= MAX_NAME_LEN)
				name[i++] = *p;
		name[i] = '\0';
		vp = name;
		if (i > MAX_NAME_LEN) {
			bferr("Variable name too long");
			return;
		}						/* L004 } */
		if (*p == '[') {
			hadsub++;
			p = getinx(p, &subscr);
		}
		if (op = *p) {
			*p++ = 0;
			if (*p == 0 && *v && **v == '(')
				p = *v++;
		} else if (*v && eq(*v, "=")) {
			op = '=', v++;
			if (*v)
				p = *v++;
		}
		if (op && op != '=')
setsyn:
			bferr("Syntax error");
		if (eq(p, "(")) {
			register char **e = v;

			if (hadsub)
				goto setsyn;
			for (;;) {
				if (!*e)
					bferr("Missing )");
				if (**e == ')')
					break;
				e++;
			}
			p = *e;
			*e = 0;
			vecp = saveblk(v);
			set1(vp, vecp, &shvhed);
#ifndef V6
			if (eq(vp, "path"))
				exportpath(vecp);
#endif
			*e = p;
			v = e + 1;
		} else if (hadsub)
			asx(vp, subscr, savestr(p));
		else
			set(vp, savestr(p));
		if (eq(vp, "path"))
			dohash();
		else if (eq(vp, "histchars")) {
			register char *p = value("histchars");

			HIST = *p++;
			HISTSUB = *p;
		}
	} while (p = *v++);
}

char *
getinx(cp, ip)
	register char *cp;
	register int *ip;
{

	*ip = 0;
	*cp++ = 0;
	while (*cp && digit(*cp))
		*ip = *ip * 10 + *cp++ - '0';
	if (*cp++ != ']')
		bferr("Subscript error");
	return (cp);
}

asx(vp, subscr, p)
	char *vp;
	int subscr;
	char *p;
{
	register struct varent *v = getvx(vp, subscr);

	xfree(v->vec[subscr - 1]);
	v->vec[subscr - 1] = globone(p);
}

struct varent *
getvx(vp, subscr)
{
	register struct varent *v = adrof(vp);

	if (v == 0)
		udvar(vp);
	if (subscr < 1 || subscr > blklen(v->vec))
		bferr("Subscript out of range");
	return (v);
}

char	plusplus[2] = { '1', 0 };


dolet(v)
	char **v;
{
	register char *p;
	char *vp, c, op;
	bool hadsub;
	int subscr;

	v++;
	p = *v++;
	if (p == 0) {
		prvars();
		return;
	}
	do {
		hadsub = 0;
		for (vp = p; letter(*p); p++)
			continue;
		if (vp == p)
			goto letsyn;
		if (*p == '[') {
			hadsub++;
			p = getinx(p, &subscr);
		}
		if (*p == 0 && *v)
			p = *v++;
		if (op = *p)
			*p++ = 0;
		else
			goto letsyn;
		vp = savestr(vp);
		if (op == '=') {
			c = '=';
			p = xset(p, &v);
		} else {
			c = *p++;
			if (any(c, "+-")) {
				if (c != op || *p)
					goto letsyn;
				p = plusplus;
			} else {
				if (any(op, "<>")) {
					if (c != op)
						goto letsyn;
					c = *p++;
letsyn:
					bferr("Syntax error");
				}
				if (c != '=')
					goto letsyn;
				p = xset(p, &v);
			}
		}
		if (op == '=')
			if (hadsub)
				asx(vp, subscr, p);
			else
				set(vp, p);
		else
			if (hadsub)
#ifndef V6
				/* avoid bug in vax CC */
				{
					struct varent *gv = getvx(vp, subscr);

					asx(vp, subscr, operate(op, gv->vec[subscr - 1], p));
				}
#else
				asx(vp, subscr, operate(op, getvx(vp, subscr)->vec[subscr - 1], p));
#endif
			else
				set(vp, operate(op, value(vp), p));
		if (strcmp(vp, "path") == 0)
			dohash();
		xfree(vp);
		if (c != '=')
			xfree(p);
	} while (p = *v++);
}

char *
xset(cp, vp)
	char *cp, ***vp;
{
	register char *dp;

	if (*cp) {
		dp = savestr(cp);
		--(*vp);
		xfree(**vp);
		**vp = dp;
	}
	return (putn(expa(vp)));
}

char *
operate(op, vp, p)
	char op, *vp, *p;
{
	char opr[2];
	char *vec[5];
	register char **v = vec;
	char **vecp = v;
	register int i;

	if (op != '=') {
		if (*vp)
			*v++ = vp;
		opr[0] = op;
		opr[1] = 0;
		*v++ = opr;
		if (op == '<' || op == '>')
			*v++ = opr;
	}
	*v++ = p;
	*v++ = 0;
	i = expa(&vecp);
	if (*vecp)
		bferr("Expression syntax");
	return (putn(i));
}

onlyread(cp)
	char *cp;
{
	extern char *End;	/* M000 was char end[] */

	return (cp < End);	/* M000 */
}

xfree(cp)
	char *cp;
{
	extern char *sbrk();
	extern char *End;			/* M000 */

	if (cp == (char *)0)			/* M000 */
		return;
	if (cp >= End && cp < sbrk(0))		/* M000 */
		Cfree(cp);			/* L005 */
#ifdef	NEVER
	if (cp >= end && cp < (char *) &cp)
		Cfree(cp);			/* L005 */
#endif
}

char *
savestr(s)
	register char *s;
{

	if (s == 0)
		s = "";
	return (strcpy(Calloc(1, strlen(s) + 1), s));		/* L005 */
}

static	char *putp;
 
char *
putn(n)
	register int n;
{
	static char number[15];

	putp = number;
	if (n < 0) {
		n = -n;
		*putp++ = '-';
	}
#define	MINSHORT	0x8000		/* ((-32767) - 1)	L003 */
#define	MINLONG		0x80000000	/* ((-2147483647L) - 1)	L003 */
	if (sizeof (int) == 2 && n == MINSHORT) {
		*putp++ = '3';
		n = 2768;
	} else if (sizeof (int) == 4 && n == MINLONG) {
		*putp++ = '2';
		n = 147483648;
	}
	putn1(n);
	*putp = 0;
	return (savestr(number));
}

putn1(n)
	register int n;
{
	if (n > 9)
		putn1(n / 10);
	*putp++ = n % 10 + '0';
}

getn(cp)
	register char *cp;
{
	register int n;
	int sign;

	sign = 0;
	if (cp[0] == '+' && cp[1])
		cp++;
	if (*cp == '-') {
		sign++;
		cp++;
		if (!digit(*cp))
			goto badnum;
	}
	n = 0;
	while (digit(*cp))
		n = n * 10 + *cp++ - '0';
	if (*cp)
		goto badnum;
	return (sign ? -n : n);
badnum:
	bferr("Badly formed number");
	return (0);
}

char *
value(var)
	char *var;
{

	return (value1(var, &shvhed));
}

char *
value1(var, head)
	char *var;
	struct varent *head;
{
	register struct varent *vp;

	vp = adrof1(var, head);
	return (vp == 0 || vp->vec[0] == 0 ? "" : vp->vec[0]);
}

static	struct varent *shprev;

struct varent *
adrof(var)
	char *var;
{

	return (adrof1(var, &shvhed));
}

struct varent *
madrof(pat, head)
	char *pat;
	struct varent *head;
{
	register struct varent *vp;

	shprev = head;
	for (vp = shprev->link; vp != 0; vp = vp->link) {
		if (Gmatch(vp->name, pat))
			return (vp);
		shprev = vp;
	}
	return (0);
}

struct varent *
adrof1(var, head)
	char *var;
	struct varent *head;
{
	register struct varent *vp;
	int cmp;

	shprev = head;
	for (vp = shprev->link; vp != 0; vp = vp->link) {
		cmp = strcmp(vp->name, var);
		if (cmp == 0)
			return (vp);
		else if (cmp > 0)
			return (0);
		shprev = vp;
	}
	return (0);
}

/*
 * The caller is responsible for putting value in a safe place
 */
set(var, value)
	char *var, *value;
{
								/* L005 */
	register char **vec = (char **) Calloc(2, sizeof (char **));

	vec[0] = onlyread(value) ? savestr(value) : value;
	set1(var, vec, &shvhed);
}

set1(var, vec, head)
	char *var, **vec;
	struct varent *head;
{

	register char **oldv = vec;

	gflag = 0; rscan(oldv, tglob);
	if (gflag) {
		vec = glob(oldv);
		if (vec == 0) {
			bferr("No match");
			blkfree(oldv);
			return;
		}
		blkfree(oldv);
		gargv = 0;
	}
	setq(var, vec, head);
}

setq(var, vec, head)
	char *var, **vec;
	struct varent *head;
{
	register struct varent *vp;

	vp = adrof1(var, head);
	if (vp == 0) {
		vp = (struct varent *) Calloc(1, sizeof *vp);	/* L005 */
		vp->name = savestr(var);
		vp->link = shprev->link;
		shprev->link = vp;
	}
	if (vp->vec)
		blkfree(vp->vec);
	scan(vec, trim);
	vp->vec = vec;
}

unset(v)
	register char *v[];
{

	unset1(v, &shvhed);
	if (adrof("histchars") == 0) {
		HIST = '!';
		HISTSUB = '^';
	}
}

unset1(v, head)
	register char *v[];
	struct varent *head;
{
	register char *var;
	register struct varent *vp;
	register int cnt;

	v++;
	while (var = *v++) {
		cnt = 0;
		while (vp = madrof(var, head))
			unsetv1(vp->name, head), cnt++;
/*
		if (cnt == 0)
			setname(var), bferr("No match");
*/
	}
}

unsetv(var)
	char *var;
{

	unsetv1(var, &shvhed);
}

unsetv1(var, head)
	char *var;
	struct varent *head;
{
	register struct varent *vp;

	vp = adrof1(var, head);
	if (vp == 0)
		udvar(var);
	vp = shprev->link;
	shprev->link = vp->link;
	blkfree(vp->vec);
	xfree(vp->name);
	xfree(vp);
}

setNS(cp)
	char *cp;
{

	set(cp, "");
}

shift(v)
	register char **v;
{
	register struct varent *argv;
	register char *name;

	v++;
	name = *v;
	if (name == 0)
		name = "argv";
	else
		strip(name);
	argv = adrof(name);
	if (argv == 0)
		udvar(name);
	if (argv->vec[0] == 0)
		bferr("No more words");
	lshift(argv->vec, 1);
}

deletev(cp)
	register char *cp;
{

	if (adrof(cp))
		unsetv(cp);
}

#ifndef V6
exportpath(val)
char **val;
{
	char exppath[PATHSIZE]; /* L002 */ /* S001 doubled the size */
	register char *p, *dir;
	int len = 1;			/* S001 just the terminator */

	exppath[0] = 0;
	for(;;) {
		dir = *val;
		if (!eq(dir, ".")) {
			if((len += strlen(dir)) > sizeof(exppath)) { /* S001 */
			  bferr("path too long; unable to export");  /* S001 */
			  break;				     /* S001 */
			}					     /* S001 */
			strcat(exppath, dir);
		}
		if ((dir = *++val) && !eq(dir, ")")) {
			if((len += 1) > sizeof(exppath)) {	     /* S001 */
			  bferr("path too long; unable to export");  /* S001 */
			  break;				     /* S001 */
			}					     /* S001 */
			strcat(exppath, ":");
		} else
			break;
	}
	setenv("PATH", exppath);
}
#endif

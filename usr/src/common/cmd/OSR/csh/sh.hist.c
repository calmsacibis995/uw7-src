#ident	"@(#)OSRcmds:csh/sh.hist.c	1.1"
/*
 *	@(#) sh.hist.c 25.1 94/02/18 
 *
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

/* #ident	"@(#)csh:sh.hist.c	1.2" */
#pragma comment(exestr, "@(#) sh.hist.c 25.1 94/02/18 ")
/*
 * Modification History
 *
 *	L000	17 February 1994	scol!gregw
 *	- Renamed calloc to Calloc to avoid possible conflict with libc.
 */

/*
 *	@(#) sh.hist.c 1.1 88/03/29 csh:sh.hist.c
 */
/*	@(#)sh.hist.c	2.1	SCCS id keyword	*/
/* Copyright (c) 1980 Regents of the University of California */
#include "sh.h"

/*
 * C shell
 */

savehist(sp)
	struct wordent *sp;
{
	register struct Hist *hp, *np;
	int histlen;
	register char *cp;

	cp = value("history");
	if (*cp == 0)
		histlen = 0;
	else {
		while (*cp && digit(*cp))
			cp++;
		/* avoid a looping snafu */
		if (*cp)
			set("history", "10");
		histlen = getn(value("history"));
	}
	/* throw away null lines */
	if (sp->next->word[0] == '\n')
		return;
	for (hp = &Histlist; np = hp->Hnext;)
		if (eventno - np->Href >= histlen || histlen == 0)
			hp->Hnext = np->Hnext, hfree(np);
		else
			hp = np;
	enthist(++eventno, sp, 1);
}

struct Hist *
enthist(event, lp, docopy)
	int event;
	register struct wordent *lp;
	bool docopy;
{
	register struct Hist *np;

	np = (struct Hist *) Calloc(1, sizeof *np);		/* L000 */
	np->Hnum = np->Href = event;
	if (docopy)
		copylex(&np->Hlex, lp);
	else {
		np->Hlex.next = lp->next;
		lp->next->prev = &np->Hlex;
		np->Hlex.prev = lp->prev;
		lp->prev->next = &np->Hlex;
	}
	np->Hnext = Histlist.Hnext;
	Histlist.Hnext = np;
	return (np);
}

hfree(hp)
	register struct Hist *hp;
{

	freelex(&hp->Hlex);
	xfree(hp);
}

dohist()
{

	if (getn(value("history")) == 0)
		return;
	dohist1(Histlist.Hnext);
}

dohist1(hp)
	register struct Hist *hp;
{

	if (hp == 0)
		return;
	hp->Href++;
	dohist1(hp->Hnext);
	phist(hp);
}

phist(hp)
	register struct Hist *hp;
{

	printf("%6d\t", hp->Hnum);
	prlex(&hp->Hlex);
}

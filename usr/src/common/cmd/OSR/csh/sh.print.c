#ident	"@(#)OSRcmds:csh/sh.print.c	1.1"
#pragma comment(exestr, "@(#) sh.print.c 23.1 91/05/02 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1991 The Santa Cruz Operation, Inc
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

/* #ident	"@(#)csh:sh.print.c	1.2" */

/*
 *	@(#) sh.print.c 1.1 88/03/29 csh:sh.print.c
 */

/* Copyright (c) 1980 Regents of the University of California */
/*	MODIFICATION HISTORY
 *
 *	L000   scol!panosd 18 Mar 1991
 *	- make it not complain if HZ env variable is not set.
 *	  Neither sh nor ksh do.
 *	- hughd: and cut out the else, proceed to the printf,
 *	  that was the whole point of taking HZ from param.h
 */
#include "sh.h"
#include <sys/param.h>

/*
 * C Shell
 */

p60ths(l)
	long l;
{
	l += 3;
	if (Hz <= 0 && (Hz = gethz()) <= 0)
		Hz = HZ;				/* L000 */
	printf("%d.%d", (int) (l / Hz), (int) ((l % Hz) / (Hz/10)));
}

psecs(l)
	long l;
{
	register int i;

	i = l / 3600;
	if (i) {
		printf("%d:", i);
		i = l % 3600;
		p2dig(i / 60);
		goto minsec;
	}
	i = l;
	printf("%d", i / 60);
minsec:
	i %= 60;
	printf(":");
	p2dig(i);
}

p2dig(i)
	register int i;
{

	printf("%d%d", i / 10, i % 10);
}

char	linbuf[64];
char	*linp = linbuf;

putchar(c)
	register int c;
{

	if ((c & QUOTE) == 0 && (c == 0177 || c < ' ' && c != '\t' && c != '\n')) {
		putchar('^');
		if (c == 0177)
			c = '?';
		else
			c |= 'A' - 1;
	}
	c &= TRIM;
	*linp++ = c;
	if (c == '\n' || linp >= &linbuf[sizeof linbuf - 2])
		flush();
}

draino()
{

	linp = linbuf;
}

flush()
{
	register int unit;

	if (haderr)
		unit = didfds ? 2 : SHDIAG;
	else
		unit = didfds ? 1 : SHOUT;
	if (linp != linbuf) {
		write(unit, linbuf, linp - linbuf);
		linp = linbuf;
	}
}

plist(vp)
	register struct varent *vp;
{
	register void (*wasintr)();

	if (setintr)
		wasintr = signal(SIGINT, pintr);
	for (vp = vp->link; vp != 0; vp = vp->link) {
		int len = blklen(vp->vec);

		printf(vp->name);
		printf("\t");
		if (len != 1)
			putchar('(');
		blkpr(vp->vec);
		if (len != 1)
			putchar(')');
		printf("\n");
	}
	if (setintr)
		signal(SIGINT, wasintr);
}

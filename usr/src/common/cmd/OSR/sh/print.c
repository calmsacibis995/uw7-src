#ident	"@(#)OSRcmds:sh/print.c	1.1"
#pragma comment(exestr, "@(#) print.c 25.1 92/09/16 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1992 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)sh:print.c	1.5" */
/*
 * UNIX shell
 *
 */
/*
 *	Modification History
 *
 *	S000	scol!wooch	Jun 19 89
 *	Internationalization Phase I - use toint()
 *
 *	18 Jan 1990	sco!rr S001
 *	- Get HZ out of the environment (via gethz()) rather than from param.h.
 *		If, for some reason this fails, use the value from param.h
 *
 *	L002	scol!markhe	16th March, 1992
 *	- removed definition of toint() and todigit() from "mac.h",
 *	  so now need to include <ctype.h>
 */

#include	"defs.h"
#include	<ctype.h>					/* L002 */
#include	<sys/param.h>
#include	"../include/osr.h"

#define		BUFLEN		256

static unsigned char	buffer[BUFLEN];
static int	index = 0;
unsigned char		numbuf[12];

extern void	prc_buff();
extern void	prs_buff();
extern void	prn_buff();
extern void	prs_cntl();
extern void	prn_buff();

/*
 * printing and io conversion
 */
prp()
{
	if ((flags & prompt) == 0 && cmdadr)
	{
		prs_cntl(cmdadr);
		prs(colon);
	}
}

prs(as)
unsigned char	*as;
{
	register unsigned char	*s;

	if (s = as)
		write(output, s, length(s) - 1);
}

prc(c)
unsigned char	c;
{
	if (c)
		write(output, &c, 1);
}

prt(t)
long	t;
{
	register int hr, min, sec;
	int Hz = 0;		/* S001 */

	if (Hz == 0 && (Hz = gethz()) <= 0)		/* S001 */
		Hz = HZ;
	t += Hz / 2;
	t /= Hz;
	sec = t % 60;
	t /= 60;
	min = t % 60;

	if (hr = t / 60)
	{
		prn_buff(hr);
		prc_buff('h');
	}

	prn_buff(min);
	prc_buff('m');
	prn_buff(sec);
	prc_buff('s');
}

prn(n)
	int	n;
{
	itos(n);

	prs(numbuf);
}

itos(n)
{
	register unsigned char *abuf;
	register unsigned a, i;
	int pr, d;

	abuf = numbuf;

	pr = FALSE;
	a = n;
	for (i = 10000; i != 1; i /= 10)
	{
		if ((pr |= (d = a / i)))
			*abuf++ = todigit(d);
		a %= i;
	}
	*abuf++ = todigit(a);
	*abuf++ = 0;
}

stoi(icp)
unsigned char	*icp;
{
	register unsigned char	*cp = icp;
	register int	r = 0;
	register unsigned char	c;

	while ((c = *cp, digit(c)) && c && r >= 0)
	{
#ifdef	INTL
		r = r * 10 + toint(c);
#else
		r = r * 10 + c - '0';
#endif
		cp++;
	}
	if (r < 0 || cp == icp)
		failed(icp, badnum);
	else
		return(r);
}

prl(n)
long n;
{
	int i;

	i = 11;
	while (n > 0 && --i >= 0)
	{
		numbuf[i] = todigit(n % 10);
		n /= 10;
	}
	numbuf[11] = '\0';
	prs_buff(&numbuf[i]);
}

void
flushb()
{
	if (index)
	{
		buffer[index] = '\0';
		write(1, buffer, length(buffer) - 1);
		index = 0;
	}
}

void
prc_buff(c)
	unsigned char c;
{
	if (c)
	{
		if (index + 1 >= BUFLEN)
			flushb();

		buffer[index++] = c;
	}
	else
	{
		flushb();
		write(1, &c, 1);
	}
}

void
prs_buff(s)
	unsigned char *s;
{
	register int len = length(s) - 1;

	if (index + len >= BUFLEN)
		flushb();

	if (len >= BUFLEN)
		write(1, s, len);
	else
	{
		movstr(s, &buffer[index]);
		index += len;
	}
}


clear_buff()
{
	index = 0;
}


void
prs_cntl(s)
	unsigned char *s;
{
	register unsigned char *ptr = buffer;
	register unsigned char c;

	while (*s != '\0') 
	{
		c = *s;
		
		/* translate a control character into a printable sequence */

		if (c < '\040') 
		{	/* assumes ASCII char */
			*ptr++ = '^';
			*ptr++ = (c + 0100);	/* assumes ASCII char */
		}
		else if (c == 0177) 
		{	/* '\0177' does not work */
			*ptr++ = '^';
			*ptr++ = '?';
		}
		else 
		{	/* printable character */
			*ptr++ = c;
		}

		++s;
		if ( ptr - buffer >= BUFLEN - 2 ) {
			*ptr = '\0';
			prs(buffer);
			ptr = buffer;
		}
	}

	*ptr = '\0';
	prs(buffer);
}


void
prn_buff(n)
	int	n;
{
	itos(n);

	prs_buff(numbuf);
}

/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)csh:common/cmd/csh/sh.print.c	1.3.5.4"
#ident  "$Header$"

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

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "sh.h"
/*
 * C Shell
 */

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

char linbuf[128];
char *linp = linbuf;

#ifdef MBCHAR
/* putbyte() send a byte to SHOUT.  No interpretation is done
 * except an un-QUOTE'd control character, which is displayed 
 * as ^x.
 */
putbyte(c)
	register int c;
{

	if ((c & QUOTE) == 0 && (c == 0177 || c < ' ' && c != '\t' && c != '\n')) {
		putbyte('^');
		if (c == 0177)
			c = '?';
		else
			c |= 'A' - 1;
	}
	c &= TRIM;
	*linp++ = c;

	if (c == '\n' || linp >= &linbuf[sizeof linbuf - 1 - MB_CUR_MAX])
		flush();/* 'cause the next putchar() call may
			  * overflow the buffer.
			  */
}

/* putchar(tc) does what putbyte(c) do for a byte c.
 * Note that putbyte(c) just send the byte c (provided c is not
 * a control character) as it is, while putchar(tc) may expand the
 * character tc to some byte sequnce that represents the character
 * in EUC form.
 */
putchar(tc)
     register tchar tc;
{
	int	n;

	if( isascii(tc&TRIM) ){
		putbyte((int)tc);
		return;
	}
	tc &= TRIM;
	n=wctomb(linp, tc);
	if(n==-1) return;
	linp+=n;
	if ( linp >= &linbuf[sizeof linbuf - 1 - MB_CUR_MAX])
		flush(); 
}
#else/*!MBCHAR*/
/* putbyte() send a byte to SHOUT.  No interpretation is done
 * except an un-QUOTE'd control character, which is displayed 
 * as ^x.
 */
putbyte(c)
	register int c;
{

	if ((c & QUOTE) == 0 && (c == 0177 || c < ' ' && c != '\t' && c != '\n')) {
		putbyte('^');
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

/* putchar(tc) does what putbyte(c) do for a byte c.
 * For single-byte character only environment, there is no
 * difference between putchar() and putbyte() though.
 */
putchar(tc)
     register tchar tc;
{
	putbyte((int)tc);
}
#endif/*!MBCHAR*/
draino()
{

	linp = linbuf;
}

flush()
{
	register int unit;
	int lmode;

	if (linp == linbuf)
		return;
	if (haderr)
		unit = didfds ? 2 : SHDIAG;
	else
		unit = didfds ? 1 : SHOUT;
#ifdef TIOCLGET
	if (didfds == 0 && ioctl(unit, TIOCLGET,  (char *)&lmode) == 0 &&
	    lmode&LFLUSHO) {
		lmode = LFLUSHO;
		(void) ioctl(unit, TIOCLBIC,  (char *)&lmode);
		(void) erwrite(unit, "\n", 1);
	}
#endif
	(void) erwrite(unit, linbuf, linp - linbuf);
	linp = linbuf;
}

/*
 * Should not be needed.
 */
write_string(s)
	char *s;
{
	register int unit;
	/*
	 * First let's make it sure to flush out things.
	 */
	 flush();

	if (haderr)
		unit = didfds ? 2 : SHDIAG;
	else
		unit = didfds ? 1 : SHOUT;

	(void) erwrite(unit, s, strlen(s));
}


#ident	"@(#)str.c	1.4"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */


#define	INCLUDE_CTYPE
#define	INCLUDE_TIME

#include "include.h"
#ifdef	FLOATING_POINT
#include "math.h"
#endif	/* FLOATING_POINT */
#ifdef	PROTO_INET
#include "inet.h"
#endif	/* PROTO_INET */


#ifdef	vax11c
extern char *gd_error();

#define	strerror(str)	gd_error(str)
#endif

/*
 *	Make a copy of a string and lowercase it
 */
char *
gd_uplow __PF2(str, const char *,
	       up, int)
{
    register const char *src = str;
    register char *dst;
    size_t len = strlen(str) + 1;
    static char *buf;
    static size_t buflen;

    /* Make sure the buffer is big enough */
    if (len > buflen) {
	if (buf) {
	    task_mem_free((task *) 0, buf);
	}
	buflen = len;
	buf = (char *) task_mem_calloc((task *) 0, 1, buflen);
    }

    /* Can convert to ... */
    if (up) {
	/* upper case */
	
	for (dst = buf; *src; src++) {
	    *dst++ = islower(*src) ? toupper(*src) : *src;
	}
    } else {
	/* lower case */
	
	for (dst = buf; *src; src++) {
	    *dst++ = isupper(*src) ? tolower(*src) : *src;
	}
    }
    *dst = (char) 0;

    return buf;
}


/*
 *	Gated version of fprintf
 */
#ifdef	STDARG
/*VARARGS2*/
int
fprintf(FILE * stream, const char *format,...)
#else	/* STDARG */
/*ARGSUSED*/
/*VARARGS0*/
int
fprintf(va_alist)
va_dcl

#endif	/* STDARG */
{
    int rc;
    va_list ap;
    char buffer[BUFSIZ];

#ifdef	STDARG
    va_start(ap, format);
#else	/* STDARG */
    const char *format;
    FILE *stream;

    va_start(ap);

    stream = va_arg(ap, FILE *);
    format = va_arg(ap, const char *);
#endif	/* STDARG */
    rc = vsprintf(buffer, format, ap);
    (void) fwrite((char *) buffer, sizeof (char), (u_int) rc, stream);

    va_end(ap);
    return rc;
}


/*
 *	Gated version of sprintf
 */
#ifdef	STDARG
/*VARARGS2*/
int
sprintf(char *s, const char *format,...)
#else	/* STDARG */
/*ARGSUSED*/
/*VARARGS0*/
int
sprintf(va_alist)
va_dcl

#endif	/* STDARG */
{
    int rc;
    va_list ap;

#ifdef	STDARG

    va_start(ap, format);
#else	/* STDARG */
    const char *format;
    char *s;

    va_start(ap);

    s = va_arg(ap, char *);
    format = va_arg(ap, const char *);
#endif	/* STDARG */
    rc = vsprintf(s, format, ap);

    va_end(ap);
    return rc;
}


#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)doprnt.c	5.35 (Berkeley) 6/27/88";

#endif				/* LIBC_SCCS and not lint */

/* 11-bit exponent (VAX G floating point) is 308 decimal digits */
#define	MAXEXP		308
/* 128 bit fraction takes up 39 decimal digits; max reasonable precision */
#define	MAXFRACT	39

#define	DEFPREC		6

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */

#define	PUTC(ch)	*dp++ = ch; cnt++;

#define	ARG() \
	_ulong = flags&LONGINT ? va_arg(ap, long) : \
	    flags&SHORTINT ? va_arg(ap, short) : va_arg(ap, int);

#define	todigit(c)	((c) - '0')
#define	tochar(n)	((n) + '0')

/* have to deal with the negative buffer count kludge */
#define	NEGATIVE_COUNT_KLUDGE

#define	LONGINT		0x01		/* long integer */
#define	LONGDBL		0x02		/* long double; unimplemented */
#define	SHORTINT	0x04		/* short integer */
#define	ALT		0x08		/* alternate form */
#define	LADJUST		0x10		/* left adjustment */
#define	ZEROPAD		0x20		/* zero (as opposed to blank) pad */
#define	HEXPREFIX	0x40		/* add 0x or 0X prefix */


#ifdef	FLOATING_POINT
static char *
round __PF6(fract, double,
	    expon, int *,
	    start, register char *,
	    end, register char *,
	    ch, char,
	    signp, char *)
{
    double tmp;

    if (fract)
	(void) modf(fract * 10, &tmp);
    else
	tmp = (double) todigit(ch);
    if (tmp > 4)
	for (;; --end) {
	    if (*end == '.')
		--end;
	    if (++*end <= '9')
		break;
	    *end = '0';
	    if (end == start) {
		if (expon) {		/* e/E; increment exponent */
		    *end = '1';
		    ++*expon;
		} else {		/* f; add extra digit */
		    *--end = '1';
		    --start;
		}
		break;
	    }
	}
    /* ``"%.3f", (double)-0.0004'' gives you a negative 0. */
    else if (*signp == '-')
	for (;; --end) {
	    if (*end == '.')
		--end;
	    if (*end != '0')
		break;
	    if (end == start)
		*signp = 0;
	}
    return start;
}


static char *
exponent __PF3(p, register char *,
	       expon, register int,
	       fmtch, char)
{
    register char *t;
    char expbuf[MAXEXP];

    *p++ = fmtch;
    if (expon < 0) {
	expon = -expon;
	*p++ = '-';
    } else
	*p++ = '+';
    t = expbuf + MAXEXP;
    if (expon > 9) {
	do {
	    *--t = tochar(expon % 10);
	} while ((expon /= 10) > 9);
	*--t = tochar(expon);
	for (; t < expbuf + MAXEXP; *p++ = *t++) ;
    } else {
	*p++ = '0';
	*p++ = tochar(expon);
    }
    return p;
}


static int
cvt __PF7(number, double,
	  prec, register int,
	  flags, int,
	  signp, char *,
	  fmtch, char,
	  startp, char *,
	  endp, char *)
{
    register char *p, *t;
    register double fract;
    int dotrim, expcnt, gformat;
    double integer, tmp;

    dotrim = expcnt = gformat = 0;
    fract = modf(number, &integer);

    /* get an extra slot for rounding. */
    t = ++startp;

    /*
     * get integer portion of number; put into the end of the buffer; the
     * .01 is added for modf(356.0 / 10, &integer) returning .59999999...
     */
    for (p = endp - 1; integer; ++expcnt) {
	tmp = modf(integer / 10, &integer);
	*p-- = tochar((int) ((tmp + .01) * 10));
    }
    switch (fmtch) {
	case 'f':
	    /* reverse integer into beginning of buffer */
	    if (expcnt)
		for (; ++p < endp; *t++ = *p) ;
	    else
		*t++ = '0';
	    /*
	     * if precision required or alternate flag set, add in a
	     * decimal point.
	     */
	    if (prec || BIT_TEST(flags, ALT))
		*t++ = '.';
	    /* if requires more precision and some fraction left */
	    if (fract) {
		if (prec)
		    do {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    } while (--prec && fract);
		if (fract)
		    startp = round(fract, (int *) NULL, startp,
				   t - 1, (char) 0, signp);
	    }
	    for (; prec--; *t++ = '0') ;
	    break;
	case 'e':
	case 'E':
	  eformat:if (expcnt) {
		*t++ = *++p;
		if (prec || BIT_TEST(flags, ALT))
		    *t++ = '.';
		/* if requires more precision and some integer left */
		for (; prec && ++p < endp; --prec)
		    *t++ = *p;
		/*
		 * if done precision and more of the integer component,
		 * round using it; adjust fract so we don't re-round
		 * later.
		 */
		if (!prec && ++p < endp) {
		    fract = 0.0;
		    startp = round((double) 0,
				   &expcnt,
				   startp,
				   t - 1,
				   *p,
				   signp);
		}
		/* adjust expcnt for digit in front of decimal */
		--expcnt;
	    }
	    /* until first fractional digit, decrement exponent */
	    else if (fract) {
		/* adjust expcnt for digit in front of decimal */
		for (expcnt = -1;; --expcnt) {
		    fract = modf(fract * 10, &tmp);
		    if (tmp)
			break;
		}
		*t++ = tochar((int) tmp);
		if (prec || BIT_TEST(flags, ALT))
		    *t++ = '.';
	    } else {
		*t++ = '0';
		if (prec || BIT_TEST(flags, ALT))
		    *t++ = '.';
	    }
	    /* if requires more precision and some fraction left */
	    if (fract) {
		if (prec)
		    do {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    } while (--prec && fract);
		if (fract)
		    startp = round(fract, &expcnt, startp,
				   t - 1, (char) 0, signp);
	    }
	    /* if requires more precision */
	    for (; prec--; *t++ = '0') ;

	    /* unless alternate flag, trim any g/G format trailing 0's */
	    if (gformat && !BIT_TEST(flags, ALT)) {
		while (t > startp && *--t == '0') ;
		if (*t == '.')
		    --t;
		++t;
	    }
	    t = exponent(t, expcnt, fmtch);
	    break;
	case 'g':
	case 'G':
	    /* a precision of 0 is treated as a precision of 1. */
	    if (!prec)
		++prec;
	    /*
	     * ``The style used depends on the value converted; style e
	     * will be used only if the exponent resulting from the
	     * conversion is less than -4 or greater than the precision.''
	     *	-- ANSI X3J11
	     */
	    if (expcnt > prec || (!expcnt && fract && fract < .0001)) {
		/*
		 * g/G format counts "significant digits, not digits of
		 * precision; for the e/E format, this just causes an
		 * off-by-one problem, i.e. g/G considers the digit
		 * before the decimal point significant and e/E doesn't
		 * count it as precision."
		 */
		--prec;
		fmtch -= 2;		/* G->E, g->e */
		gformat = 1;
		goto eformat;
	    }
	    /*
	     * reverse integer into beginning of buffer,
	     * note, decrement precision
	     */
	    if (expcnt)
		for (; ++p < endp; *t++ = *p, --prec) ;
	    else
		*t++ = '0';
	    /*
	     * if precision required or alternate flag set, add in a
	     * decimal point.  If no digits yet, add in leading 0.
	     */
	    if (prec || BIT_TEST(flags, ALT)) {
		dotrim = 1;
		*t++ = '.';
	    } else
		dotrim = 0;
	    /* if requires more precision and some fraction left */
	    if (fract) {
		if (prec) {
		    do {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    } while (!tmp);
		    while (--prec && fract) {
			fract = modf(fract * 10, &tmp);
			*t++ = tochar((int) tmp);
		    }
		}
		if (fract)
		    startp = round(fract, (int *) NULL, startp,
				   t - 1, (char) 0, signp);
	    }
	    /* alternate format, adds 0's for precision, else trim 0's */
	    if (BIT_TEST(flags, ALT))
		for (; prec--; *t++ = '0') ;
	    else if (dotrim) {
		while (t > startp && *--t == '0') ;
		if (*t != '.')
		    ++t;
	    }
    }
    return t - startp;
}
#endif	/* FLOATING_POINT */


int
vsprintf(dest, fmt0, ap)
char *dest;
const char *fmt0;
va_list ap;
{
    register const char *fmt;		/* format string */
    register char *dp;			/* Destination pointer */
    register int ch;			/* character from fmt */
    register int cnt;			/* return value accumulator */
    register int n;			/* random handy integer */
    register char *t;			/* buffer pointer */
#ifdef	FLOATING_POINT
    double _double;			/* double precision arguments %[eEfgG] */
    char softsign;			/* temporary negative sign for floats */
#endif	/* FLOATING_POINT */
    u_long _ulong;			/* integer arguments %[diouxX] */
    int base = 10;			/* base for [diouxX] conversion */
    int dprec;				/* decimal precision in [diouxX] */
    int fieldsz;			/* field size expanded by sign, etc */
    int flags;				/* flags as above */
    int fpprec;				/* `extra' floating precision in [eEfgG] */
    int prec;				/* precision from format (%.3d), or -1 */
    int realsz;				/* field size expanded by decimal precision */
    int size;				/* size of converted field or string */
    int width;				/* width from format (%8d), or 0 */
    char sign;				/* sign prefix (' ', '+', '-', or \0) */
    const char *digs;			/* digits for [diouxX] conversion */
    utime_t ut;				/* used for time computations */
    char buf[BUF];			/* space for %c, %[diouxX], %[eEfgG] */
    int error_number = errno;

    dp = dest;
    fmt = fmt0;
    digs = "0123456789abcdef";
    for (cnt = 0;; ++fmt) {
	for (; (ch = *fmt) && ch != '%'; ++fmt) {
	    PUTC(ch);
	}
	if (!ch) {
	    PUTC(ch);
	    return --cnt;
	}
	flags = 0;
	dprec = 0;
	fpprec = 0;
	width = 0;
	prec = -1;
	sign = '\0';

      rflag:
	switch (*++fmt) {
	case ' ':
	    /*
	     * ``If the space and + flags both appear, the space
	     * flag will be ignored.''
	     *	-- ANSI X3J11
	     */
	    if (!sign)
		sign = ' ';
	    goto rflag;

	case '#':
	    BIT_SET(flags, ALT);
	    goto rflag;

	case '*':
	    /*
	     * ``A negative field width argument is taken as a
	     * - flag followed by a  positive field width.''
	     *	-- ANSI X3J11
	     * They don't exclude field widths read from args.
	     */
	    if ((width = va_arg(ap, int)) >= 0)
		goto rflag;
	    width = -width;
	    /* FALLTHROUGH */

	case '-':
	    BIT_SET(flags, LADJUST);
	    goto rflag;

	case '+':
	    sign = '+';
	    goto rflag;

	case '.':
	    if (*++fmt == '*')
		n = va_arg(ap, int);
	    else {
		n = 0;
		while (isascii(*fmt) && isdigit(*fmt))
		    n = 10 * n + todigit(*fmt++);
		--fmt;
	    }
	    prec = n < 0 ? -1 : n;
	    goto rflag;

	case '0':
	    /*
	     * ``Note that 0 is taken as a flag, not as the
	     * beginning of a field width.''
	     *	-- ANSI X3J11
	     */
	    BIT_SET(flags, ZEROPAD);
	    goto rflag;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	    n = 0;
	    do {
		n = 10 * n + todigit(*fmt);
	    } while (isascii(*++fmt) && isdigit(*fmt));
	    width = n;
	    --fmt;
	    goto rflag;

	case 'L':
	    BIT_SET(flags, LONGDBL);
	    goto rflag;

	case 'h':
	    BIT_SET(flags, SHORTINT);
	    goto rflag;

	case 'l':
	    BIT_SET(flags, LONGINT);
	    goto rflag;

	case 'c':
	    *(t = buf) = va_arg(ap, int);
	    size = 1;
	    sign = '\0';
	    goto pforw;

	case 'D':
	    BIT_SET(flags, LONGINT);
	    /*FALLTHROUGH*/

	case 'd':
	case 'i':
	    ARG();
	    if ((long) _ulong < 0) {
		_ulong = -_ulong;
		sign = '-';
	    }
	    base = 10;
	    goto number;
	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
#ifdef	FLOATING_POINT
	    _double = va_arg(ap, double);
	    /*
	     * don't do unrealistic precision; just pad it with
	     * zeroes later, so buffer size stays rational.
	     */
	    if (prec > MAXFRACT) {
		if ((*fmt != 'g' && *fmt != 'G') || BIT_TEST(flags, ALT))
		    fpprec = prec - MAXFRACT;
		prec = MAXFRACT;
	    } else if (prec == -1)
		prec = DEFPREC;
	    /*
	     * softsign avoids negative 0 if _double is < 0 and
	     * no significant digits will be shown
	     */
	    if (_double < 0) {
		softsign = '-';
		_double = -_double;
	    } else
		softsign = 0;
	    /*
	     * cvt may have to round up past the "start" of the
	     * buffer, i.e. ``intf("%.2f", (double)9.999);'';
	     * if the first char isn't NULL, it did.
	     */
	    *buf = (char) 0;
	    size = cvt(_double,
		       prec,
		       flags,
		       &softsign,
		       *fmt,
		       buf,
		       buf + sizeof(buf));
	    if (softsign)
		sign = '-';
	    t = *buf ? buf : buf + 1;
	    goto pforw;
#else	/* FLOATING_POINT */
	    assert(FALSE);
	    break;
#endif	/* FLOATING_POINT */

	case 'n':
	    if (BIT_TEST(flags, LONGINT))
		*va_arg(ap, long *) = cnt;
	    else if (BIT_TEST(flags, SHORTINT))
		*va_arg(ap, short *) = cnt;
	    else
		*va_arg(ap, int *) = cnt;
	    break;

	case 'O':
	    BIT_SET(flags, LONGINT);
	    /*FALLTHROUGH*/

	case 'o':
	    ARG();
	    base = 8;
	    goto nosign;

	case 'p':
	    /*
	     * ``The argument shall be a pointer to void.  The
	     * value of the pointer is converted to a sequence
	     * of printable characters, in an implementation-
	     * defined manner.''
	     *	-- ANSI X3J11
	     */
	    /* NOSTRICT */
	    _ulong = (u_long) va_arg(ap, void *);
	    base = 16;
	    goto nosign;

	case 'T':
	    /* Time as a time_t */
	    ut.ut_sec = va_arg(ap, time_t);
	    ut.ut_usec = 0;
	    n = 0;
	    /* FALLSTHROUGH */

	case 't':
	    /* Set up buffer */
	    t = buf + BUF;
	    *--t = (char) 0;

	    /* Time as a utime */
	    if (*fmt == 't') {
		utime_t *utp = va_arg(ap, utime_t *);
		if (utp == NULL) {
		    *--t = '>';
		    *--t = 'e';
		    *--t = 'n';
		    *--t = 'o';
		    *--t = 'n';
		    *--t = '<';
		    goto string;
		}
		ut = *utp;
	    }

	    /* Normalize */
	    if (ut.ut_usec >= 1000000) {
		ut.ut_sec += ut.ut_usec / 1000000;
		ut.ut_usec %= 1000000;
	    }

	    /* If this is the normal form, compute time of day */
	    if (!BIT_TEST(flags, ALT)) {
		struct tm *tm;

		ut.ut_sec += utime_boot.ut_sec;
		ut.ut_usec += utime_boot.ut_usec;
		if (ut.ut_usec >= 1000000) {
		    ut.ut_usec -= 1000000;
		    ut.ut_sec += 1;
		}
		tm = localtime(&ut.ut_sec);
#define	MULBY60(X)	(((X) << 6) - ((X) << 2))
		ut.ut_sec = (time_t)(MULBY60(tm->tm_hour) + tm->tm_min);
		ut.ut_sec = MULBY60(ut.ut_sec) + (time_t) tm->tm_sec;
#undef	MULBY60
		BIT_SET(flags, ZEROPAD);
	    } else if (prec < 0) {
		prec = 6;
	    }

	    /* If usec part is non-zero, print it with trailing zeroes supressed */
	    if (ut.ut_usec > 0 && prec != 0 && (BIT_TEST(flags, ALT) || prec > 0)) {
		int didprint = 0;
		register u_long tmp = ut.ut_usec;

		for (n = 6; n > 0; n--) {
		    _ulong = tmp;
		    if (_ulong) {
			tmp /= 10;
			_ulong -= ((tmp << 3) + (tmp << 1));
		    }
		    if ((prec < 0 && (didprint || _ulong)) || (prec >= n)) {
			didprint = 1;
			*--t = digs[_ulong];
		    }
		}
		if (didprint) {
		    *--t = '.';
		}
	    }

	    /* Now work on seconds, in XX:XX:XX format.  If alt form, neglect leading */
	    _ulong = ut.ut_sec;
	    for (n = 2; ; n--) {
		register u_long tmp;

		tmp = _ulong;
		_ulong /= 10;
		*--t = digs[tmp - ((_ulong << 3) + (_ulong << 1))];
		if (n == 0  || (_ulong == 0 && !BIT_TEST(flags, ZEROPAD))) {
		    break;
		}
		tmp = _ulong;
		_ulong /= 6;
		*--t = digs[tmp - ((_ulong << 2) + (_ulong << 1))];
		if (_ulong == 0 && !BIT_TEST(flags, ZEROPAD)) {
		    break;
		}
		*--t = ':';
	    }

	    if (_ulong != 0 || BIT_TEST(flags, ZEROPAD)) {
		do {
		    register u_long tmp = _ulong;
		    _ulong /= 10;
		    *--t = digs[tmp - ((_ulong << 3) + (_ulong << 1))];
		} while (_ulong);
	    }

	    size = strlen(t);
	    sign = '\0';
	    goto pforw;

	case 'A':
	    /* socket address */
	    {
		/* XXX - How about being able to specify a different delimiter */
		char delim = '.';
		int len;
		int trailing = FALSE;
		register byte *cp, *cp1;
		register sockaddr_un *addr;

		cp = cp1 = (byte *) 0;
		addr = va_arg(ap, sockaddr_un *);

		if (!addr) {
		    (void) strcpy(buf, "(null)");
		    t = buf;
		    goto string;
		}
		/* Get the size */
		len = socksize(addr);
		assert(len);
		t = buf + BUF;
		*--t = (char) 0;
		*buf = (char) 0;

		switch (socktype(addr)) {
		case AF_UNSPEC:
		    (void) strcpy(buf, "*Unspecified");
		    if (socksize(addr)) {
			(void) strcat(buf, ":");
			*--t = '*';

			base = 10;
			cp1 = (byte *) addr;
			cp = cp1 + len ;
		    } else {
			(void) strcat(buf, "*");
			t = (char *) 0;
		    }
		    break;

#ifdef	PROTO_UNIX
		case AF_UNIX:
		    len -= (sizeof(addr->un) - 1);
		    strncpy(buf, addr->un.gun_path, len);
		    buf[len] = (char) 0;
		    t = buf;
		    goto string;
#endif	/* PROTO_UNIX */

#ifdef	PROTO_INET
		case AF_INET:
		    base = 10;

		    if (BIT_TEST(flags, ALT)) {
			if ((_ulong = ntohs(sock2port(addr)))) {
			    do {
				*--t = digs[_ulong % base];
			    } while (_ulong /= base);
			    *--t = '+';
			}
		    }
		    cp1 = (byte *) &sock2ip(addr);

		    /* Reset length in case this address has been trimmed */
		    cp = cp1 + (len = sizeof(sock2ip(addr)));

		    if (sock2ip(addr)) {
			trailing = TRUE;	/* Skip trailing zeros */
		    }
		    break;
#endif	/* PROTO_INET */

		case AF_LL:
		    base = 16;

		    
		    strcpy(buf, trace_state(ll_type_bits, addr->ll.gll_type));
		    strcat(buf, " ");

		    cp1 = (byte *) addr->ll.gll_addr;
		    cp = cp1 + addr->ll.gll_len - ((caddr_t) cp1 - (caddr_t) addr);
		    switch (addr->ll.gll_type) {
		    case LL_SYSTEMID:
			/* Must be even length */
			assert(!((cp - cp1) & 0x01));

			/* Print two bytes at a time */
			while (cp > cp1 + 1) {
			    _ulong = *--cp;
			    *--t = digs[_ulong % base];
			    *--t = digs[_ulong / base];

			    _ulong = *--cp;
			    *--t = digs[_ulong % base];
			    *--t = digs[_ulong / base];

			    *--t = delim;
			}
			/* Skip over first delim */
			t++;
			break;

		    default:
			/* Use default printing method */
			delim = ':';
			break;
		    }
		    break;
		    
#ifdef	PROTO_ISO
		case AF_ISO:
		    base = 16;
		    trailing = FALSE;

		    /* Point at ISO address */
		    cp1 = addr->iso.giso_addr;
		    cp = (byte *) addr + len;

		    len = cp - cp1;

		    if (len > 3) {
			if (~len & 0x01) {
			    /* Length (excluding first byte) is odd, print the odd byte */

			    _ulong = *--cp;
			    *--t = digs[_ulong % base];
			    *--t = digs[_ulong / base];

			    *--t = delim;
			}
		    } else if (len < 3) {
			/* Print at least three digits so the address is recognizable */
			/* as an ISO address */

			cp = cp1 + (len = 3);
		    }

		    /* Print two bytes at a time */
		    while (cp > cp1 + 1) {
			_ulong = *--cp;
			*--t = digs[_ulong % base];
			*--t = digs[_ulong / base];

			_ulong = *--cp;
			*--t = digs[_ulong % base];
			*--t = digs[_ulong / base];

			*--t = delim;
		    }

		    /* Finally do the first byte */
		    _ulong = *--cp;
		    *--t = digs[_ulong % base];
		    *--t = digs[_ulong / base];
		    break;
#endif	/* PROTO_ISO */

#ifdef	SOCKADDR_DL
		case AF_LINK:
		    /* Format link level address as: '#<index> <name> <link_address>" */
		    /* Alternate : 'index <index> name <name> addr <link_address>" */

		    /* Assume index is always present */
		    (void) strcpy(buf, BIT_TEST(flags, ALT) ? "index " : "#");
		    cp = (byte *) t;
		    base = 10;
		    _ulong = addr->dl.gdl_index;
		    do {
			*--t = digs[_ulong % base];
		    } while (_ulong /= base);
		    (void) strcat(buf, t);
		    t = (char *) cp;

		    /* Add name if present */
		    if (addr->dl.gdl_nlen) {
			(void) strcat(buf, BIT_TEST(flags, ALT) ? " name " : " ");
			(void) strncat(buf, addr->dl.gdl_data, addr->dl.gdl_nlen);
		    }
		    /* Set up to display the link level address in HEX if present */
		    if (addr->dl.gdl_alen + addr->dl.gdl_slen) {
			(void) strcat(buf, BIT_TEST(flags, ALT) ? " addr " : " ");
			base = 16;
			cp1 = (byte *) addr->dl.gdl_data + addr->dl.gdl_nlen;
			cp = cp1 + addr->dl.gdl_alen + addr->dl.gdl_slen;
		    } else {
			cp = cp1 = (byte *) 0;
		    }
		    break;
#endif	/* SOCKADDR_DL */

#ifdef	KRT_RT_SOCK
		case AF_ROUTE:
		    (void) strcpy(buf, "*RoutingSocket*");
		    t = (char *) 0;
		    break;
			    
#endif	/* KRT_RT_SOCK */
#ifdef	notdef
		case AF_CCITT:
		    /* XXX - x25_net (DNIC, 4 nibbles) */
		    /* XXX - x25_addr (address, null terminated) */
		    break;
#endif	/* notdef */

		case AF_STRING:
		    /* Copy string into buffer */
		    strncpy(buf, addr->s.gs_string, addr->s.gs_len);
		    t = buf;
		    goto string;
		    
		default:
		    base = 16;

		    cp1 = (byte *) addr;
		    cp = cp1 + len;
		}

		if (t) {
		    if (cp > cp1) {
			if (cp > (cp1 + len)) {
			    cp = cp1 + len;
			}
			do {

			    _ulong = *--cp;
			    /* Skip trailing zeros if desired */
			    if (trailing) {
				if (_ulong) {
				    trailing = FALSE;
				} else {
				    continue;
				}
			    }
			    do {
				*--t = digs[_ulong % base];
			    } while (_ulong /= base);

			    *--t = delim;
			} while (cp > cp1);
			if (trailing) {
			    trailing = FALSE;
			    *--t = '0';
			} else {
			    t++;
			}
		    }
		    if (*buf) {
			t -= strlen(buf);
			(void) strncpy(t, buf, strlen(buf));
		    }
		} else {
		    t = buf;
		}
	    }
	    goto string;

	case 'm':
	    (void) strcpy(t = buf, strerror(error_number));
	    goto string;

	case 's':
	    t = va_arg(ap, char *);
	    if (!t) {
		(void) strcpy(buf, "(null)");
		t = buf;
	    }

	string:
	    if (prec >= 0) {
		for (size = 0; (size < prec) && t[size]; size++) ;
	    } else
		size = strlen(t);
	    sign = '\0';
	    goto pforw;

	case 'U':
	    BIT_SET(flags, LONGINT);
	    /*FALLTHROUGH*/

	case 'u':
	    ARG();
	    base = 10;
	    goto nosign;

	case 'B':
	    digs = "_|";
	    /* FALLTHROUGH */

	case 'b':
	    ARG();
	    base = 2;
	    goto binhex;

	case 'X':
	    digs = "0123456789ABCDEF";
	    /* FALLTHROUGH */

	case 'x':
	    ARG();
	    base = 16;

	binhex:
	    /* leading 0x/X only if non-zero */
	    if (BIT_TEST(flags, ALT) && _ulong != 0)
		BIT_SET(flags, HEXPREFIX);

	    /* unsigned conversions */
	nosign:
	    sign = '\0';
	    /*
	     * ``... diouXx conversions ... if a precision is
	     * specified, the 0 flag will be ignored.''
	     *	-- ANSI X3J11
	     */
	number:
	    if ((dprec = prec) >= 0)
		BIT_RESET(flags, ZEROPAD);

	    /*
	     * ``The result of converting a zero value with an
	     * explicit precision of zero is no characters.''
	     *	-- ANSI X3J11
	     */
	    t = buf + BUF;
	    if (_ulong != 0 || prec != 0) {
		do {
		    *--t = digs[_ulong % base];
		    _ulong /= base;
		} while (_ulong);
		if (BIT_TEST(flags, ALT) && base == 8 && *t != *digs)
		    *--t = *digs;	/* octal leading 0 */
	    }	
	    size = buf + BUF - t;

	pforw:
	    /*
	     * All reasonable formats wind up here.  At this point,
	     * `t' points to a string which (if not flags&LADJUST)
	     * should be padded out to `width' places.  If
	     * flags&ZEROPAD, it should first be prefixed by any
	     * sign or other prefix; otherwise, it should be blank
	     * padded before the prefix is emitted.  After any
	     * left-hand padding and prefixing, emit zeroes
	     * required by a decimal [diouxX] precision, then print
	     * the string proper, then emit zeroes required by any
	     * leftover floating precision; finally, if LADJUST,
	     * pad with blanks.
	     */

	    /*
	     * compute actual size, so we know how much to pad
	     * fieldsz excludes decimal prec; realsz includes it
	     */
	    fieldsz = size + fpprec;
	    if (sign)
		fieldsz++;
	    if (BIT_TEST(flags, HEXPREFIX))
		fieldsz += 2;
	    realsz = dprec > fieldsz ? dprec : fieldsz;

	    /* right-adjusting blank padding */
	    if ((flags & (LADJUST | ZEROPAD)) == 0 && width)
		for (n = realsz; n < width; n++) {
		    PUTC(' ');
		}
	    /* prefix */
	    if (sign) {
		PUTC(sign);
	    }
	    if (BIT_TEST(flags, HEXPREFIX)) {
		PUTC(*digs);
		PUTC((char) *fmt);
	    }
	    /* right-adjusting zero padding */
	    if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD)
		for (n = realsz; n < width; n++) {
		    PUTC(*digs);
		}
	    /* leading zeroes from decimal precision */
	    for (n = fieldsz; n < dprec; n++) {
		PUTC(*digs);
	    }

	    /* the string or number proper */
	    bcopy(t, (char *) dp, (size_t) size);
	    dp += size;
	    cnt += size;
	    /* trailing f.p. zeroes */
	    while (--fpprec >= 0) {
		PUTC(*digs);
	    }
	    /* left-adjusting padding (always blank) */
	    if (BIT_TEST(flags, LADJUST))
		for (n = realsz; n < width; n++) {
		    PUTC(' ');
		}
	    digs = "0123456789abcdef";
	    break;

	case '\0':			/* "%?" prints ?, unless ? is NULL */
	    PUTC((char) *fmt);
	    return --cnt;

	default:
	    PUTC((char) *fmt);
	}
    }
    /* NOTREACHED */
}

/*		copyright	"%c%" 	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* The pwb version this is based on */
static char *printf_id = "@(#) printf.c:2.2 6/5/79";
/* The local sccs version within ex */
#ident	"@(#)vi:port/printf.c	1.6.1.8"
#ident "$Header$"
#include <varargs.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
extern short putoctal;
/*
 * This version of printf is compatible with the Version 7 C
 * printf. The differences are only minor except that this
 * printf assumes it is to print through putchar. Version 7
 * printf is more general (and is much larger) and includes
 * provisions for floating point.
 */
 

#define MAXOCT	11	/* Maximum octal digits in a long */
#define MAXINT	32767	/* largest normal length positive integer */
#define BIG	1000000000  /* largest power of 10 less than an unsigned long */
#define MAXDIGS	10	/* number of digits in BIG */

static int width, sign, fill;

unsigned char *_p_dconv();

mprintf(id, fmt, va_alist)
char	*id, *fmt;
va_dcl
{
	va_list args;
	va_start(args);
	_printf(mesg(gettxt(id, fmt)), args);
	va_end(args);
}

gprintf(id, fmt, va_alist)
char	*id, *fmt;
va_dcl
{
	va_list args;
	va_start(args);
	_printf(gettxt(id, fmt), args);
	va_end(args);
}

printf(fmt, va_alist)
char *fmt;
va_dcl
{
	va_list ap;
	va_start(ap);
	_printf(fmt, ap);
	va_end(ap);
}

_vprintf(fmt, ap)
char *fmt;
va_list ap;
{
	_printf(fmt, ap);
}

_printf(fmt, ap)
char *fmt;
va_list ap;
{
	static char *allobuf;
	static size_t allolen;
	char buf[1024];
	char *ptr;
	size_t len;
	wchar_t wc;
	int n;

	if ((ptr = allobuf) != 0)
		len = allolen;
	else {
		ptr = buf;
		len = sizeof(buf);
	}
	/* loop until have room for the entire result string */
	while (vsnprintf(ptr, len, fmt, ap) < 0) {
		len <<= 1;
		if ((ptr = realloc(allobuf, len)) == 0) {
			if ((ptr = allobuf) == 0)
				ptr = buf;
			break; /* use the truncated result */
		}
		allobuf = ptr;
		allolen = len;
	}
	while (*ptr != '\0') {
		if ((n = mbtowc(&wc, ptr, MB_LEN_MAX)) <= 0) {
			putoctal = 1;
			putchar((unsigned char)*ptr++);
			putoctal = 0;
			continue;
		}
		ptr += n;
		putchar(wc);
	}
}

#ident "@(#)tvtoa.c	1.3"

/*
 * tvtoa - return an asciized representation of a struct timeval
 */
#include <stdio.h>
#ifndef SYS_WINNT
#include <sys/time.h>
#endif /* SYS_WINNT */

#include "lib_strbuf.h"
#if defined(VMS)
#include "ntp_fp.h"
#endif /* VMS */
#include "ntp_stdlib.h"

char *
tvtoa(tv)
	struct timeval *tv;
{
	register char *buf;
	register u_long sec;
	register u_long usec;
	register int isneg;

	if (tv->tv_sec < 0 || tv->tv_usec < 0) {
		sec = -tv->tv_sec;
		usec = -tv->tv_usec;
		isneg = 1;
	} else {
		sec = tv->tv_sec;
		usec = tv->tv_usec;
		isneg = 0;
	}

	LIB_GETBUF(buf);

	(void) sprintf(buf, "%s%lu.%06lu", (isneg?"-":""), sec, usec);
	return buf;
}

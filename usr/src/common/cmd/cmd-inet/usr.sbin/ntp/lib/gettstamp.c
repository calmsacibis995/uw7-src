#ident "@(#)gettstamp.c	1.3"

/*
 * gettstamp - return the system time in timestamp format
 */
#include <stdio.h>
#ifndef SYS_WINNT
#include <sys/time.h>
#endif /* SYS_WINNT */

#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "ntp_stdlib.h"

void
gettstamp(ts)
	l_fp *ts;
{
	struct timeval tv;

	/*
	 * Quickly get the time of day and convert it
	 */
	(void) GETTIMEOFDAY(&tv, (struct timezone *)NULL);

	if (tv.tv_usec >= 1000000) {	/* bum solaris */
		tv.tv_usec -= 1000000;
		tv.tv_sec++;
	}
	TVTOTS(&tv, ts);
	ts->l_uf += TS_ROUNDBIT;	/* guaranteed not to overflow */
	ts->l_ui += JAN_1970;
	ts->l_uf &= TS_MASK;
}

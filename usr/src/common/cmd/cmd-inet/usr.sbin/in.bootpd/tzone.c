#ident	"@(#)tzone.c	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * tzone.c - get the timezone
 *
 * This is shared by bootpd and bootpef
 */

#ifdef	SVR4
/* XXX - Is this really SunOS specific? -gwr */
/* This is in <time.h> but only visible if (__STDC__ == 1). */
extern long timezone;
#else	/* SVR4 */
/* BSD or SunOS */
# include <sys/time.h>
# include <syslog.h>
#endif	/* SVR4 */

#include "report.h"
#include "tzone.h"

/* This is what other modules use. */
long secondswest;

/*
 * Get our timezone offset so we can give it to clients if the
 * configuration file doesn't specify one.
 */
void
tzone_init()
{
#ifdef	SVR4
	/* XXX - Is this really SunOS specific? -gwr */
	secondswest = timezone;
#else	/* SVR4 */
	struct timezone tzp;	/* Time zone offset for clients */
	struct timeval tp;		/* Time (extra baggage) */
	if (gettimeofday(&tp, &tzp) < 0) {
		secondswest = 0L;	/* Assume GMT for lack of anything better */
		report(LOG_ERR, "gettimeofday: %s", get_errmsg());
	} else {
		secondswest = 60L * tzp.tz_minuteswest; /* Convert to seconds */
	}
#endif	/* SVR4 */
}

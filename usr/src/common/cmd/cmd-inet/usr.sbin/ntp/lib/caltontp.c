#ident "@(#)caltontp.c	1.3"

/*
 * caltontp - convert a julian date to an NTP time
 */
#include <sys/types.h>

#include "ntp_types.h"
#include "ntp_calendar.h"
#include "ntp_stdlib.h"

/*
 * calmonthtab - month start offsets from the beginning of a cycle.
 */
static u_short calmonthtab[12] = {
	0,						/* March */
	MAR,						/* April */
	(MAR+APR),					/* May */
	(MAR+APR+MAY),					/* June */
	(MAR+APR+MAY+JUN),				/* July */
	(MAR+APR+MAY+JUN+JUL),				/* August */
	(MAR+APR+MAY+JUN+JUL+AUG),			/* September */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP),			/* October */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT),		/* November */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV),		/* December */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC),	/* January */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC+JAN),	/* February */
};

u_long
caltontp(jt)
	register const struct calendar *jt;
{
	register int cyear;
	register int resyear;
	register u_long nt;
	register int yearday;

	/*
	 * Find the start of the cycle this is in.
	 */
	cyear = (int)(jt->year - 1900) >> 2;
	resyear = (jt->year - 1900) - (cyear << 2);
	yearday = 0;
	if (resyear == 0) {
		if (jt->yearday == 0) {
			if (jt->month == 1 || jt->month == 2) {
				cyear--;
				resyear = 3;
			}
		} else {
			if (jt->yearday <= (u_short)(JAN+FEBLEAP)) {
				cyear--;
				resyear = 3;
				yearday = calmonthtab[10] + jt->yearday;
			} else {
				yearday = jt->yearday - (JAN+FEBLEAP);
			}
		}
	} else {
		if (jt->yearday == 0) {
			if (jt->month == 1 || jt->month == 2)
				resyear--;
		} else {
			if (jt->yearday <= (u_short)(JAN+FEB)) {
				resyear--;
				yearday = calmonthtab[10] + jt->yearday;
			} else {
				yearday = jt->yearday - (JAN+FEB);
			}
		}
	}

	if (yearday == 0) {
		if (jt->month >= 3) {
			yearday = calmonthtab[jt->month - 3] + jt->monthday;
		} else {
			yearday = calmonthtab[jt->month + 9] + jt->monthday;
		}
	}

	nt = TIMESDPERC((u_long)cyear);
	while (resyear-- > 0)
		nt += DAYSPERYEAR;
	nt += (u_long) (yearday - 1);

	nt = TIMES24(nt) + (u_long)jt->hour;
	nt = TIMES60(nt) + (u_long)jt->minute;
	nt = TIMES60(nt) + (u_long)jt->second;

	return nt + MAR1900;
}

/*
 *
 *  Adapted for System V Release 4	(ESIX 4.0.4)	
 *
 *  Gerard van Dorth	(gdorth@nl.oracle.com)
 *  Paul Bauwens	(paul@pphbau.atr.bso.nl)
 *
 *  May 1993
 *
 *  Originally written by Paul Popelka (paulp@uts.amdahl.com)
 *
 *  You can do anything you want with this software,
 *    just don't say you wrote it,
 *    and don't remove this notice.
 *
 *  This software is provided "as is".
 *
 *  The author supplies this software to be publicly
 *  redistributed on the understanding that the author
 *  is not responsible for the correct functioning of
 *  this software in any circumstances and is not liable
 *  for any damages caused by this software.
 *
 *  October 1992
 *
 */

#ident	"@(#)kern-i386:fs/dosfs/dosfs_conv.c	1.2"

#include "dosfs.h"


static struct timezone tz;	/* nop for now ??? */

/*
 *  Days in each month in a regular year.
 */

unsigned short regyear[] = {
		31,	28,	31,	30,	31,	30,
		31,	31,	30,	31,	30,	31
};

/*
 *  Days in each month in a leap year.
 */
unsigned short leapyear[] = {
		31,	29,	31,	30,	31,	30,
		31,	31,	30,	31,	30,	31
};

/*
 *  Variables used to remember parts of the last time
 *  conversion.  Maybe we can avoid a full conversion.
 */
unsigned long lasttime = 0;
unsigned long lastday;
union dosdate lastddate;
union dostime lastdtime;

/*
 *  Convert the unix version of time to dos's idea of time
 *  to be used in file timestamps.
 *  The passed in unix time is assumed to be in GMT.
 */
void
unix2dostime(timestruc_t *tvp, union dosdate *ddp, union dostime *dtp)
{
	unsigned long days;
	unsigned long inc;
	unsigned long year;
	unsigned long month;
	unsigned short *months;
	/*
	 *  If the time from the last conversion is the same
	 *  as now, then skip the computations and use the
	 *  saved result.
	 */
	if (lasttime != tvp->tv_sec) {
		lasttime = tvp->tv_sec - (tz.tz_minuteswest * 60)
		    /* +- daylight savings time correction */;
		lastdtime.dts.dt_2seconds = ((lasttime % 60) >> 1);
		lastdtime.dts.dt_minutes  = ((lasttime / 60) % 60);
		lastdtime.dts.dt_hours = ((lasttime / (60 * 60)) % 24);

		/*
		 *  If the number of days since 1970 is the same as the
		 *  last time we did the computation then skip all this
		 *  leap year and month stuff.
		 */
		days = lasttime / (24 * 60 * 60);
		if (days != lastday) {
			lastday = days;
			for (year = 1970; ; year++) {
				inc = year & 0x03 ? 365 : 366;
				if (days < inc) break;
				days -= inc;
			}
			months = year & 0x03 ? regyear : leapyear;
			for (month = 0; month < 12; month++) {
				if (days < months[month]) break;
				days -= months[month];
			}
			lastddate.dds.dd_day = days + 1;
			lastddate.dds.dd_month = month+1;
			/*
			 *  Remember dos's idea of time is relative to 1980.
			 *  unix's is relative to 1970.  If somehow we get a
			 *  time before 1980 then don't give totally crazy
			 *  results.
			 */
			lastddate.dds.dd_year = (year < 1980 ? 0 : year - 1980);
		}
	}
	dtp->dti = lastdtime.dti;
	ddp->ddi = lastddate.ddi;
}

/*
 *  The number of seconds between Jan 1, 1970 and
 *  Jan 1, 1980.
 *  In that interval there were 8 regular years and
 *  2 leap years.
 */
#define	SECONDSTO1980	(((8 * 365) + (2 * 366)) * (24 * 60 * 60))

union dosdate lastdosdate;
unsigned long lastseconds;

/*
 *  Convert from dos' idea of time to unix'.
 *  This will probably only be called from the
 *  stat(), and fstat() system calls
 *  and so probably need not be too efficient.
 */
void
dos2unixtime(union dosdate *ddp, union dostime *dtp, timestruc_t *tvp)
{
	unsigned long seconds;
	unsigned long month;
	unsigned long yr;
	unsigned long days;
	unsigned short *months;

	seconds = (dtp->dts.dt_2seconds << 1) +
	    (dtp->dts.dt_minutes * 60) +
	    (dtp->dts.dt_hours * 60 * 60);
	/*
	 *  If the year, month, and day from the last conversion
	 *  are the same then use the saved value.
	 */
	if (lastdosdate.ddi != ddp->ddi) {
		lastdosdate.ddi = ddp->ddi;
		days = 0;
		for (yr = 0; yr < ddp->dds.dd_year; yr++) {
			days += yr & 0x03 ? 365 : 366;
		}
		months = yr & 0x03 ? regyear : leapyear;
		/*
		 *  Prevent going from 0 to 0xffffffff in the following
		 *  loop.
		 */
		if (ddp->dds.dd_month == 0)
		{
			cmn_err (CE_WARN, "dos2unixtime: month value out of range (%d)\n",
			    ddp->dds.dd_month);
			ddp->dds.dd_month = 1;
		}
		for (month = 0; month < ddp->dds.dd_month-1; month++)
		{
			days += months[month];
		}
		days += ddp->dds.dd_day - 1;
		lastseconds = (days * 24 * 60 * 60) + SECONDSTO1980;
	}
	tvp->tv_sec = seconds + lastseconds + (tz.tz_minuteswest * 60)
	    /* -+ daylight savings time correction */;
	tvp->tv_nsec = 0;
}

#ident	"@(#)kern-i386at:svc/clock_p.c	1.19.2.2"
#ident	"$Header$"

/*
 * Machine-dependent clock routines.  This file defines routines that
 * are called from machine-independent files (usually) and call routines
 * whose names may not be mentioned in machine-independent files.
 */

#include <acc/priv/privilege.h>
#include <io/rtc/rtc.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/timem.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/types.h>
#include <svc/psm.h>

/*
 * This file contains interface routines to access the hardware time
 * of day clock:  rtodc to read it, wtodc to write it.  The actual
 * routines for talking to the rtc chip are in the rtc driver.  rtodc
 * merely allows access to the driver by code such as the sysi86 RTODC
 * command.  wtodc takes the internal time (a form of GMT), converts it
 * to local time (see the uadmin A_CLOCK for how kernel gets the needed
 * correction) in the rtc's format and writes it.  xtodc reads the
 * hardware clock (stored in local time), and converts it to GMT (using
 * the correction passed through boot or uadmin A_CLOCK).
 */

time_t c_correct;	/* Stores timezone correction. */

#define	HEXIZEIT(i)	((((i) / 10) << 4) | ((i) % 10))
#define	UNHEXIZE(A)	(((((A) >> 4) & 0xF) * 10) + ((A) & 0xF))

static int dmsize[MON_YEAR] = {
	DAY_JAN,	DAY_MINFEB,	DAY_MAR,
	DAY_APR,	DAY_MAY,	DAY_JUN,
	DAY_JUL,	DAY_AUG,	DAY_SEP,
	DAY_OCT,	DAY_NOV,	DAY_DEC,
};

/*
 * int
 * rtodc(struct rtc_t *clkp)
 *
 *	Read the hardware time-of-day clock.
 *
 * Calling/Exit State:
 *
 *	None.
 */
int
rtodc(struct rtc_t *clkp)
{
	return rtcget(clkp);
}

/*
 * int xtodc(timestruc_t *hrt)
 *	Read the hardware time-of-day clock, and convert to timestruct_t
 *	format.
 *
 * Calling/Exit State:
 *	None.
 */
int
xtodc(timestruc_t *hrt)
{
	struct rtc_t clk;
	time_t clock_val;
	struct tm tmp;

	if (rtcget(&clk)) {
		return 1;
	}
	tmp.tm_sec = UNHEXIZE(clk.rtc_sec);
	tmp.tm_min = UNHEXIZE(clk.rtc_min);
	tmp.tm_hour = UNHEXIZE(clk.rtc_hr);
	tmp.tm_mday = UNHEXIZE(clk.rtc_dom);
	tmp.tm_mon = UNHEXIZE(clk.rtc_mon) - 1;	/* 0 based */
	tmp.tm_year = UNHEXIZE(clk.rtc_yr);
	if (tmp.tm_year < (EPOCH_YEAR - BASE_YEAR)) {
		tmp.tm_year += 100;
	}
	if ((clock_val = (time_t)norm_tm(&tmp)) == LONG_MIN) {
		return 1;
	}
	hrt->tv_sec = clock_val + c_correct;
	hrt->tv_nsec = 0;
	return 0;
}

/*
 * void wtodc(timestruc_t *hrt)
 *	Set the hardware time-of-day clock, if any, to the given time.
 *
 *	This code has been modelled on offtime() routine of libc.
 *	offtime() does most of the computation for localtime() and
 *	gmtime().
 *
 * Calling/Exit State:
 *	hrt is the time to set the clock to.
 */
void
wtodc(timestruc_t *hrt)
{
        struct rtc_t clkx;
	long rem;
        long days;
        long tim;
        int i, yleap;
        int yr, ydc, mon;

	/*
	 * Read the hardware clock to initialize fields that are not set
	 * in this routine.
	 */
        if (rtcget(&clkx) == -1) {
		return;
	}

        /*
	 * The format of an rtc byte is BCD: if the decimal value for the
	 * field is TU, the hexadecimal value for the rtc byte is 0xTU.
         * Of course, I have no documentation on the conventions to use
	 * for dow (day of week) values.  My machine has a value of 4 for
	 * Sunday???
         */
        tim = hrt->tv_sec - c_correct;
	days = tim / SEC_DAY;
	rem = tim % SEC_DAY;
	while (rem < 0) {
		rem += SEC_DAY;
		--days;
	}
	while (rem >= SEC_DAY) {
		rem -= SEC_DAY;
		++days;
	}
        clkx.rtc_hr = HEXIZEIT(rem / SEC_HOUR);
	rem %= SEC_HOUR;
        clkx.rtc_min = HEXIZEIT(rem / SEC_MIN);
        clkx.rtc_sec = HEXIZEIT(rem % SEC_MIN);
        i = (int)((EPOCH_WDAY + days) % DAY_WEEK);
	if (i < 0) {
		i += DAY_WEEK;
	}
        clkx.rtc_dow = (char)i;
	yr = EPOCH_YEAR;
	if (days >= 0) {
		for (;;) {
			ydc = (yleap = ISLEAPYEAR(yr)) ? DAY_MAXYEAR : DAY_MINYEAR;
			if (days < (long)ydc) {
				break;
			}
			yr++;
			days -= ydc;
		}
	} else {
		do {
			--yr;
			ydc = (yleap = ISLEAPYEAR(yr)) ? DAY_MAXYEAR : DAY_MINYEAR;
			days += ydc;
		} while(days < 0);
	}
	if ((yr -= BASE_YEAR) >= 100) {
		yr -= 100;
	}
        clkx.rtc_yr = HEXIZEIT(yr);
	if (yleap) {
		/*
		 * Leap year so increment February.
		 */
                dmsize[1]++;
	}
        for (mon = 0; days >= dmsize[mon]; mon++) {
                days -= dmsize[mon];
	}
	/*
	 * If February was incremented because it's leap year, let's reset
	 * it now.
	 */
	if (yleap) {
		dmsize[1]--;
	}
        mon++;
        clkx.rtc_mon = HEXIZEIT(mon);
        days++;
        clkx.rtc_dom = HEXIZEIT(days);

        rtcput(&clkx);
	return;
}

/*
 * this routine is defined here to make the TIME_INIT_ASM and TIME_UPDATE_ASM
 *
 * ZZZ PSM V2 no longer supports a coherent microsecond clock so we are 
 * going to use ulbolt. This gives a coherent microsecond clock but it it is
 * updated only once per tick (ie., HZ time/sec, 1000000/HZ id added to ulbolt)
 *
 * psm_usec_time is used only for lockstats & only when _LOCKTEST is defined.
 * For now, we are not going to do anything more. If you average lockstats
 * over a long enough period of time, the accuracy should be good enough.
 * Later, we should revisit this & see if something more is needed.
 *
 * ZZZ - should use ms_time_get/cvt ??? Probably right but more overhead.
 *	 wait for resolution on clock discussions.
 */
ulong_t
psm_usec_time(void)
{
	extern ulong_t ulbolt;
        return(ulbolt);
}

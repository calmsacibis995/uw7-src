#ident	"@(#)dstime:dstime/norm_tm.c	1.2"

#include <time.h>
#include <limits.h>
#include "timem.h"

/*
 * Not leap year.
 */
STATIC const char tm_day_mon[MON_YEAR] = {
	DAY_JAN,	DAY_MINFEB,	DAY_MAR,	DAY_APR,
	DAY_MAY,	DAY_JUN,	DAY_JUL,	DAY_AUG,
	DAY_SEP,	DAY_OCT,	DAY_NOV,	DAY_DEC
};

/*
 * Not leap year.
 */
STATIC const short tm_cum_day_mon[MON_YEAR] = {
	0,
	DAY_JAN,
	DAY_JAN + DAY_MINFEB,
	DAY_JAN + DAY_MINFEB + DAY_MAR,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG + DAY_SEP,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG + DAY_SEP + DAY_OCT,
	DAY_JAN + DAY_MINFEB + DAY_MAR + DAY_APR + DAY_MAY
		+ DAY_JUN + DAY_JUL + DAY_AUG + DAY_SEP + DAY_OCT + DAY_NOV
};

/*
 * *sum += value * scale; scale > 0
 */
STATIC int
muladd(long *sum, long value, long scale)
{
	if (value < -1) {
		if (LONG_MIN / value < scale)
			return -1;
	} else if (value > 1) {
		if (LONG_MAX / value < scale)
			return -1;
	}
	value *= scale;
	if (*sum > 0) {
		if (LONG_MAX - *sum < value)
			return -1;
	} else {
		if (LONG_MIN - *sum > value)
			return -1;
	}
	*sum += value;
	return 0;
}

/*
 * idivrem(*num, den, carry):
 *	*num += carry; carry = *num / den; *num %= den; return carry;
 * where:
 *	no overflow occurs,
 *	den must be 2 or more, and
 *	*num is left in the range [0,den-1].
 */
STATIC int
idivrem(int *num, int den, int carry)
{
	long tot = carry;

	tot += *num;
	carry = 0;
	if (tot >= den) {
		carry = tot / den;
		tot -= carry * (long)den;
	} else if (tot < 0) {
		carry = tot / -den;
		if ((tot += carry * den) != 0) {
			carry++;
			tot += den;
		}
		carry = -carry;
	}
	*num = tot;
	return carry;
}

/*
 * Number of leap days relative to Epoch to beginning of year.
 */
STATIC long
nlday(long yr)
{
	long n = -(EPOCH_YEAR / 4 - EPOCH_YEAR / 100 + EPOCH_YEAR / 400);

	yr--;
	/*
	* This pretends as if there were leap years from year 1 A.D.,
	* the first thus being year 4.  In actuality, there were no
	* leap years until 1752, which was "fixed" in September.
	*/
	if ((yr >>= 2) != 0) {
		n += yr;
		if ((yr /= 25) != 0) {
			n -= yr;
			if ((yr >>= 2) != 0)
				n += yr;
		}
	}
	return n;
}

/*
 * correct tm_*'s, return seconds relative to Epoch
 */
long
norm_tm(struct tm *ptm)
{
	int i, nld, epyr, flag;
	long val;

	flag = 0;
	/*
	* First, normalize from the finest granularity up the line,
	* each step spilling into (or borrowing from) the next bucket.
	* Temporarily adjust tm_mday's and tm_year's origins to 0.
	* In this pass, pretend as if all months have maximum length.
	*/
	i = idivrem(&ptm->tm_sec, SEC_MIN, 0);
	i = idivrem(&ptm->tm_min, MIN_HOUR, i);
	i = idivrem(&ptm->tm_hour, HOUR_DAY, i);
	i = idivrem(&ptm->tm_mday, DAY_MAXMON, i - 1);
	i = idivrem(&ptm->tm_mon, MON_YEAR, i);
	i += BASE_YEAR;
	if (ptm->tm_year > 0) {
		if (INT_MAX - ptm->tm_year < i) {
			flag = -1;
			ptm->tm_year = INT_MAX;
			epyr = INT_MAX - EPOCH_YEAR;
			goto skip;
		}
	} else {
		if (INT_MIN + EPOCH_YEAR - ptm->tm_year > i) {
			flag = -1;
			ptm->tm_year = INT_MIN + EPOCH_YEAR;
			epyr = INT_MIN;
			goto skip;
		}
	}
	if ((ptm->tm_year += i) <= 0)
		flag = -1;	/* cannot handle B.C. */
	epyr = ptm->tm_year - EPOCH_YEAR;
skip:;
	/*
	 * Note in flag whether it's a leap year.
	 */
	if (flag == 0 && ISLEAPYEAR(ptm->tm_year))
		flag = 1;
	/*
	 * Go back and fix tm_mday.  The only remaining special case is
	 * February 29 which is not a spill into March for leap years.
	 */
	if ((i = tm_day_mon[ptm->tm_mon]) <= ptm->tm_mday) {
		if (flag <= 0 || i != DAY_MINFEB
			|| ptm->tm_mday != DAY_MINFEB) {
			ptm->tm_mday -= i;
			if (++ptm->tm_mon == MON_YEAR) {
				ptm->tm_mon = 0;
				if (ptm->tm_year == INT_MAX)
					flag = -1;
				else
					ptm->tm_year++;
				epyr++;
			}
		}
	}
	/*
	 * Determine tm_yday, and nld.
	 */
	if ((ptm->tm_yday = tm_cum_day_mon[ptm->tm_mon] + ptm->tm_mday)
		>= DAY_JAN + DAY_MINFEB) {
		if (ptm->tm_mon > 1 && flag > 0)
			ptm->tm_yday++;
	}
	if (ptm->tm_year <= 0)
		nld = 0;
	else
		nld = nlday(ptm->tm_year);
	if (flag >= 0) {
		/* still viable */
		/*
		 * Try to compute the seconds since the Epoch.
		 * It can only overflow now because of a large tm_year.
		 */
		val = ptm->tm_sec + SEC_MIN * ptm->tm_min
			+ SEC_HOUR * ptm->tm_hour + SEC_DAY * ptm->tm_yday;
		if (muladd(&val, (long)epyr, SEC_MINYEAR) != 0
			|| muladd(&val, (long)nld, SEC_DAY) != 0) {
			flag = -1;
		}
	}
	/*
	 * Convert tm_mday and tm_year back to their nonzero origins.
	 */
	ptm->tm_mday++;
	ptm->tm_year -= BASE_YEAR;
	if (flag < 0)
		return LONG_MIN;
	return val;
}

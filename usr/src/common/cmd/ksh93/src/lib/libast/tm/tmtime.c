#ident	"@(#)ksh93:src/lib/libast/tm/tmtime.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>

/*
 * convert Tm_t to time_t
 *
 * if west==TM_LOCALZONE then the local timezone is used
 * otherwise west is the number of minutes west
 * of GMT with DST taken into account
 */

time_t
tmtime(register Tm_t* tp, int west)
{
	register time_t		clock;
	register Tm_leap_t*	lp;
	int			n;
	int			sec;
	time_t			now;

	tmset(tm_info.zone);
	clock = (tp->tm_year * (4 * 365 + 1) - 69) / 4 - 70 * 365;
	if ((n = tp->tm_mon) > 11) n = 11;
	if (n > 1 && !(tp->tm_year % 4)) clock++;
	clock += tm_data.sum[n] + tp->tm_mday - 1;
	clock *= 24;
	clock += tp->tm_hour;
	clock *= 60;
	clock += tp->tm_min;
	clock *= 60;
	clock += sec = tp->tm_sec;
	if (!(tm_info.flags & TM_UTC))
	{
		/*
		 * time zone adjustments
		 */

		if (west == TM_LOCALZONE)
		{
			clock += tm_info.zone->west * 60;
			now = clock;
			if (tmmake(&now)->tm_isdst) clock += tm_info.zone->dst * 60;
		}
		else clock += west * 60;
	}
	if (tm_info.flags & TM_LEAP)
	{
		/*
		 * leap second adjustments
		 */

		if (clock > 0)
		{
			for (lp = &tm_data.leap[0]; clock < lp->time - (lp+1)->total; lp++);
			clock += lp->total;
			n = lp->total - (lp+1)->total;
			if (clock <= (lp->time + n) && (n > 0 && sec > 59 || n < 0 && sec > (59 + n) && sec <= 59)) clock -= n;
		}
	}
	return(clock);
}

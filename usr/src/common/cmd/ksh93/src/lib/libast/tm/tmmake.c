#ident	"@(#)ksh93:src/lib/libast/tm/tmmake.c	1.1"
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
 * return Tm_t for clock
 * time zone and leap seconds accounted for in return value
 */

Tm_t*
tmmake(time_t* clock)
{
	register Tm_t*		tp;
	register Tm_leap_t*	lp;
	int			leapsec;
	time_t			now;

	tmset(tm_info.zone);
	if (clock) now = *clock;
	else time(&now);
	leapsec = 0;
	if ((tm_info.flags & (TM_ADJUST|TM_LEAP)) == (TM_ADJUST|TM_LEAP) && now > 0)
	{
		for (lp = &tm_data.leap[0]; now < lp->time; lp++);
		if (lp->total)
		{
			if (now == lp->time && (leapsec = (lp->total - (lp+1)->total)) < 0) leapsec = 0;
			now -= lp->total;
		}
	}
	if (tm_info.flags & TM_UTC) tp = (Tm_t*)gmtime(&now);
	else
	{
		now += 60 * ((tm_info.local->west + tm_info.local->dst) - (tm_info.zone->west + tm_info.zone->dst));
		tp = (Tm_t*)localtime(&now);
	}
	tp->tm_sec += leapsec;
	return(tp);
}

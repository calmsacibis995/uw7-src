#ident	"@(#)ksh93:src/lib/libast/tm/tmleap.c	1.1"
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
 * return clock with leap seconds adjusted
 * for direct localtime() access
 */

time_t
tmleap(register time_t* clock)
{
	register Tm_leap_t*	lp;
	time_t			now;

	tmset(tm_info.zone);
	if (clock) now = *clock;
	else time(&now);
	if (tm_info.flags & TM_ADJUST)
	{
		for (lp = &tm_data.leap[0]; now < (lp->time - lp->total); lp++);
		now += lp->total;
	}
	return(now);
}

#ident	"@(#)ksh93:src/lib/libast/tm/tmzone.c	1.1"
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
 * return minutes west of GMT for timezone name and type
 *
 * if type==0 then all time zone types match
 * otherwise type must be one of tm_info.zone[].type
 *
 * if end is non-null then it will point to the next
 * unmatched char in name
 *
 * if dst!=0 then it will point to 0 for standard zones
 * and the offset for daylight zones
 *
 * 0 returned for no match
 */

Tm_zone_t*
tmzone(register const char* name, char** end, const char* type, int* dst)
{
	register Tm_zone_t*	zp;
	register char*		prev;

	tmset(tm_info.zone);
	zp = tm_info.local;
	prev = 0;
	do
	{
		if (zp->type) prev = zp->type;
		if (!type || type == prev || !prev)
		{
			if (tmword(name, end, zp->standard, NiL, 0))
			{
				if (dst) *dst = 0;
				return(zp);
			}
			if (zp->dst && zp->daylight && tmword(name, end, zp->daylight, NiL, 0))
			{
				if (dst) *dst = zp->dst;
				return(zp);
			}
		}
		if (zp == tm_info.local) zp = tm_data.zone;
		else zp++;
	} while (zp->standard);
	return(0);
}

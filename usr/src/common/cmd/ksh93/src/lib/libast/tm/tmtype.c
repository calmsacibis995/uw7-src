#ident	"@(#)ksh93:src/lib/libast/tm/tmtype.c	1.1"
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
 * return the tm_data.zone[] time zone entry for type s
 *
 * if e is non-null then it will point to the first
 * unmatched char in s
 *
 * 0 returned for no match
 */

Tm_zone_t*
tmtype(register const char* s, char** e)
{
	register Tm_zone_t*	zp;
	register char*		t;

	tmset(tm_info.zone);
	zp = tm_info.local;
	do
	{
		if ((t = zp->type) && tmword(s, e, t, NiL, 0)) return(zp);
		if (zp == tm_info.local) zp = tm_data.zone;
		else zp++;
	} while (zp->standard);
	return(0);
}

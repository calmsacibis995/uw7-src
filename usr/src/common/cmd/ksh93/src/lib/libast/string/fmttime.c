#ident	"@(#)ksh93:src/lib/libast/string/fmttime.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return tmform() formatted time in static buffer
 */

#include "tm.h"

char*
fmttime(const char* format, time_t tm)
{
	static char	buf[64];

	tmfmt(buf, sizeof(buf), format, &tm);
	return(buf);
}

#ident	"@(#)ksh93:src/lib/libast/tm/tmform.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * OBSOLETE: use tmfmt() instead
 *
 * time conversion support
 */

#include <ast.h>
#include <tm.h>

/*
 * format date given clock
 * end of buf is returned
 */

char*
tmform(char* buf, const char* format, time_t* clock)
{
	return(tmfmt(buf, 256, format, clock));
}

#ident	"@(#)ksh93:src/lib/libast/re/reerror.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 *
 * regular expression error routine
 */

#include "relib.h"
#include <error.h>

void
reerror(const char* s)
{
	liberror("re", 3, gettxt(":261","regular expression: %s"), s);
}

#ident	"@(#)ksh93:src/lib/libast/misc/liberror.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * library error handler
 */

#include <error.h>

void
liberror(const char* lib, int level, ...)
{
	va_list	ap;

	va_start(ap, level);
	errorv(lib, level, ap);
	va_end(ap);
}

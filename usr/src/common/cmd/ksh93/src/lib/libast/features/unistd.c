#ident	"@(#)ksh93:src/lib/libast/features/unistd.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * generate unistd.h definitions for posix conf function
 */

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:hide printf
#else
#define printf		______printf
#endif

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE	1
#endif

#include <sys/types.h>

#include "FEATURE/lib"
#include "FEATURE/limits"
#include "FEATURE/unistd.lcl"
#include "FEATURE/types"

#if defined(__STDPP__directive) && defined(__STDPP__hide)
__STDPP__directive pragma pp:nohide printf
#endif

#if defined(__STDPP__hide) || defined(printf)
#undef	printf
extern int		printf(const char*, ...);
#endif

#include "conflib.h"

main()
{
#include "confuni.h"
	return(0);
}

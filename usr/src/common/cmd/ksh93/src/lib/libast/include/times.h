#ident	"@(#)ksh93:src/lib/libast/include/times.h	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * -last clock_t,time_t interface definitions plus
 *
 *	<ast.h>
 *	<time.h>
 *	<sys/time.h>
 *	<sys/times.h>
 */

#ifndef _TIMES_H
#define _TIMES_H

#if defined(__STDPP__directive) && defined(__STDPP__note)
#if !noticed(clock_t)
__STDPP__directive pragma pp:note clock_t
#endif
#if !noticed(time_t)
__STDPP__directive pragma pp:note time_t
#endif
#endif

#include <ast.h>

#undef	_TIMES_H
#include <ast_time.h>
#ifndef _TIMES_H
#define _TIMES_H
#endif

#if defined(__STDPP__directive) && defined(__STDPP__note)
#if !noticed(clock_t)
typedef unsigned long clock_t;
#endif
#if !noticed(time_t)
typedef unsigned long time_t;
#endif
#endif

extern int		touch(const char*, time_t, time_t, int);

#endif

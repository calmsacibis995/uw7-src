#ident	"@(#)ksh93:include/ast/ast_time.h	1.1"
/***************************************************************
*                                                              *
*                      AT&T - PROPRIETARY                      *
*                                                              *
*         THIS IS PROPRIETARY SOURCE CODE LICENSED BY          *
*                          AT&T CORP.                          *
*                                                              *
*                Copyright (c) 1995 AT&T Corp.                 *
*                     All Rights Reserved                      *
*                                                              *
*           This software is licensed by AT&T Corp.            *
*       under the terms and conditions of the license in       *
*       http://www.research.att.com/orgs/ssr/book/reuse        *
*                                                              *
*               This software was created by the               *
*           Software Engineering Research Department           *
*                    AT&T Bell Laboratories                    *
*                                                              *
*               For further information contact                *
*                     gsf@research.att.com                     *
*                                                              *
***************************************************************/

/* : : generated by proto : : */
/* : : generated from features/time by iffe version 05/09/95 : : */
                  
#ifndef _def_time_ast
#if !defined(__PROTO__)
#if defined(__STDC__) || defined(__cplusplus) || defined(_proto) || defined(c_plusplus)
#if defined(__cplusplus)
#define __MANGLE__	"C"
#else
#define __MANGLE__
#endif
#define __STDARG__
#define __PROTO__(x)	x
#define __OTORP__(x)
#define __PARAM__(n,o)	n
#if !defined(__STDC__) && !defined(__cplusplus)
#if !defined(c_plusplus)
#define const
#endif
#define signed
#define void		int
#define volatile
#define __V_		char
#else
#define __V_		void
#endif
#else
#define __PROTO__(x)	()
#define __OTORP__(x)	x
#define __PARAM__(n,o)	o
#define __MANGLE__
#define __V_		char
#define const
#define signed
#define void		int
#define volatile
#endif
#if defined(__cplusplus) || defined(c_plusplus)
#define __VARARG__	...
#else
#define __VARARG__
#endif
#if defined(__STDARG__)
#define __VA_START__(p,a)	va_start(p,a)
#else
#define __VA_START__(p,a)	va_start(p)
#endif
#endif

#define _def_time_ast	1
#define _hdr_time	1	/* #include <time.h> ok */
#define _sys_time	1	/* #include <sys/time.h> ok */
#define _sys_times	1	/* #include <sys/times.h> ok */
#define _sys_time	1	/* #include <sys/time.h> ok */
#define _mem_tm_sec_tm	1	/* tm_sec is member of struct tm */
#define _mem_tv_sec_timeval	1	/* tv_sec is member of struct timeval */
#define _hdr_time	1	/* #include <time.h> ok */
#define _sys_times	1	/* #include <sys/times.h> ok */
#define _hdr_stddef	1	/* #include <stddef.h> ok */
#define _hdr_stdlib	1	/* #include <stdlib.h> ok */
#define _typ_clock_t	1	/* clock_t is a type */
#define _typ_time_t	1	/* time_t is a type */

#if !_typ_clock_t
#define _typ_clock_t	1
typedef unsigned long clock_t;
#endif
#if !_typ_time_t
#define _typ_time_t	1
typedef unsigned long time_t;
#endif
#if _sys_time
#include <sys/time.h>
#endif
#if _hdr_time && !_mem_tm_sec_tm
#include <time.h>
#endif
#if _sys_times
#include <sys/times.h>
#else
struct tms
{
clock_t	tms_utime;
clock_t	tms_stime;
clock_t	tms_cutime;
clock_t	tms_cstime;
};
extern __MANGLE__ clock_t		times __PROTO__((struct tms*));
#endif
#if !_mem_tv_sec_timeval
struct timeval
{
time_t	tv_sec;
time_t	tv_usec;
};
#endif
#endif
#ifndef _SVC_TIMEB_H	/* wrapper symbol for kernel use */
#define _SVC_TIMEB_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/timeb.h	1.4.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 *	timeb.h 1.2 88/05/04 head.sys:timeb.h
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * THIS FILE CONTAINS CODE WHICH IS DESIGNED TO BE
 * PORTABLE BETWEEN DIFFERENT MACHINE ARCHITECTURES
 * AND CONFIGURATIONS. IT SHOULD NOT REQUIRE ANY
 * MODIFICATIONS WHEN ADAPTING XENIX TO NEW HARDWARE.
 */

#ifndef _TIME_T
#define _TIME_T
typedef	long	time_t;		/* time of day in seconds */
#endif

#if defined(__STDC__)
	#pragma	pack(2)
#endif

/*
 * Structure returned by ftime system call
 */
struct timeb {
	time_t	time;		/* time, seconds since the epoch */
	unsigned short	millitm;/* 1000 msec of additional accuracy */
	short	timezone;	/* timezone, minutes west of GMT */
	short	dstflag;	/* daylight savings when appropriate? */
};

#if defined(__STDC__)
	#pragma	pack()
#endif

#if defined(__STDC__) && !defined(_KERNEL)
int ftime(struct timeb *);
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_TIMEB_H */

#ifndef _IO_METDISK_P_H	/* wrapper symbol for kernel use */
#define _IO_METDISK_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/metdisk_p.h	1.7.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/param_p.h>	/* PORTABILITY */
#include <svc/clock.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * GET_MET_TIME() is used by met_ds_queued() and met_ds_iodone() to
 * get the current time in the best way for the machine.
 * If a usec time is easy to access, that's the best choice.
 * Anything that fits in a ulong is fine though, it doesn't have
 * to be usecs.  It is DIFF_TIME()'s job to take a difference of
 * two of these times and return usecs.
 */
#define GET_MET_TIME(time)	((time) = TICKS())

/*
 * DIFF_MET_TIME() is used by met_ds_queued() and met_ds_iodone() to
 * take the difference between time and lasttime and to return the
 * usec equivalent in since_lasttime.
 */
#if ( HZ == 60 )

#define DIFF_MET_TIME(time, lasttime, since_lasttime)	\
	((since_lasttime) = ((time) - (lasttime)) * 16667)

#elif ( HZ == 100 )

#define DIFF_MET_TIME(time, lasttime, since_lasttime)	\
	((since_lasttime) = ((time) - (lasttime)) * 10000)

#else

#define DIFF_MET_TIME(time, lasttime, since_lasttime)	\
	((since_lasttime) = ((time) - (lasttime)) * (1000000 / HZ))

#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_METDISK_P_H */

#ifndef _UTIL_PARAM_P_H	/* wrapper symbol for kernel use */
#define _UTIL_PARAM_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/param_p.h	1.11.4.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#define CANBSIZ			256	/* max size of typewriter line	*/
#define HZ			100	/* 100 ticks/second of the clock */
#define NANOSEC_PER_TICK   10000000	/* nanoseconds per tick */

/*
 * Maximum supported number of processors;
 * most of the kernel is independent of this.
 */
#ifndef UNIPROC
#define MAXNUMCPU		32	/* max supported # processors */
#else
#define MAXNUMCPU		1	/* max supported # processors */
#endif /* UNIPROC */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_PARAM_P_H */

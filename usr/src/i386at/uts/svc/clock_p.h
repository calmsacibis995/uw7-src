#ifndef	_SVC_CLOCK_P_H	/* wrapper symbol for kernel use */
#define	_SVC_CLOCK_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/clock_p.h	1.16.5.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/seg.h>	/* PORTABILITY */
#include <svc/reg.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * We allow a toid_t to be a 64-bit integer on certain platforms.
 * Such platforms will define toid_incr as an asm or function.
 * The value zero must be skipped, since it is used to indicate failure.
 * toid_incr() returns the new value of (id).
 *
 * For NUMA, the CG on which the callout is placed will be stored in
 * the upper 4 bits of the id field, so the ceiling of the actual
 * id number returned by toid_incr is 0xfffffff. new macros added to
 * pack and unpack the CG number and actual ID number.  Therefore, 
 * platforms that allows toid_t to be 64-bit integer need to perform
 * the same.
 */
#define CGBITS	4
#define IDBITS	28
#ifdef CCNUMA
#if MAXNUMCG <= (1 << CGBITS)
#define toid_incr(id)   (++(id) == 0x10000000 ? ((id) = 1) : (id))
#define toid_packid(idval, cgno) (((idval) & 0xfffffff)|(((cgno) & 0xf) << IDBITS))
#define toid_unpackcg(idval)  (((idval) >> IDBITS) & 0xf)
#define toid_unpackid(idval)  ((idval) & 0xfffffff)
#else
#	error "CGs are greater than callout id can handle"
#endif /* MAXNUMCG <= (1 << CGBITS) */
#else /* CCNUMA */
#define toid_incr(id)   (++(id) == 0 ? ++id : (id))
#define toid_packid(idval, cgno)	(idval)
#define toid_unpackcg(idval)		0
#define toid_unpackid(idval)		(idval)
#endif /* CCNUMA */

/*
 * Number of buckets in the callout hash list.  This is essentially
 * a platform specific tuneable, as each platform is designed to
 * handle a certain workload.  Each platform should tune this
 * bucket size according to their expected workload.
 */
#define	NCALLOUT_HASH	1024	/* some power of 2 */

/*
 * Platform specific macros which specify what the arguments to the
 * clock handler are for this particular architecture.
 *
 * Arguments for the global clock handler.
 */
#define	TODCLOCK_ARGS

/*
 * Determine if the interrupted priority was at base level.
 */
#define	TOD_WAS_BASE_PRIORITY	(((r0ptr[T_EDX - 1]) & 0xFF) == PL0)

/*
 * Per-processor clock handler arguments.
 */
#define	LCLCLOCK_ARGS		boolean_t user_mode

/*
 * Determine if the interrupted priority was base level.
 */
#define	LCL_WAS_BASE_PRIORITY	TOD_WAS_BASE_PRIORITY

/*
 * Determine if the interrupted mode was user-mode.
 */
#define	LCL_WAS_USER_MODE	(user_mode)

#ifdef CCNUMA
extern void cgclock(void);
#endif
extern void todclock(TODCLOCK_ARGS);
extern void lclclock(LCLCLOCK_ARGS);

/*
 * Take a profiling sample from lclclock.  A no-op on this platform,
 *	since profiling is done directly from the interrupt handler.
 */
#define	PRFINTR(pc, usermode)	/* no-op */

/*
 * Per-processor program counter to be passed to kernel profiler
 * Irrelevant on this platform since profiling is done directly from
 * the interrupt handler.
 */
#define	LCLCLOCK_PC		(r0ptr[T_EIP])

/*
 * Largest, signed integer.
 */
#define	CALLOUT_MAXVAL	0x7fffffff

/*
 * Flag for ticks argument of itimeout, et al.
 */
#define TO_PERIODIC	0x80000000	/* periodic repeating timer */

/*
 * Flag for processorid argument of dtimeout.
 * This is not an advertised featuire of the interface.
 */
#define TO_IMMEDIATE	0x80000000	/* when I say now, I mean now */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_CLOCK_P_H */

#ifndef _UTIL_KSYNCH_P_H	/* wrapper symbol for kernel use */
#define _UTIL_KSYNCH_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/ksynch_p.h	1.9"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * kernel synchronization primitives
 */
#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/dl.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>
#include <sys/dl.h>

#endif /* _KERNEL_HEADERS */


#ifdef _KERNEL

#ifdef __STDC__
extern ulong_t psm_usec_time(void);
#else
extern ulong_t psm_usec_time();
#endif

/*
 * MACRO
 * TIME_INIT(dl_t *dlp)
 *	put the current time in a dl_t.
 *
 * Calling/Exit State:
 *	The dlp is a pointer to a 64-bit wide memory location.
 *
 * Remarks:
 *	Platform dependent. See remarks in __TIME_INIT_ASM macro 
 *	in util/ksynch_p.m4
 */
#define TIME_INIT(dlp) \
	((*(dlp)).dl_lop = psm_usec_time(), (*(dlp)).dl_hop = 0)


/*
 * MACRO
 * TIME_UPDATE(dl_t *dlp, dl_t stime)  
 *	Update the dlp with the difference between the stime (start time)
 *	and the current time.
 *
 * Calling/Exit State:
 *	The dlp is a pointer to a double long 64-bit wide memory
 *	location. It holds the cumalitive lock hold time.
 *	The stime is the time when lock acquistion started.
 *
 * Remarks:
 *	Platform dependant. See remarks in __TIME_INIT_ASM macro 
 *	in util/ksynch_p.m4
 */
#define TIME_UPDATE(dlp, stime) { \
	dl_t dltmp;                                     \
	dltmp.dl_lop = psm_usec_time();			\
	dltmp.dl_hop = 0;				\
	*(dlp) = ladd(*(dlp), lsub(dltmp, (stime)));	\
}


/*
 * MACRO
 * GET_TIME(ulong_t *timep)
 *	Initialize the parameter with the current time.
 *
 * Calling/Exit State:
 *	The timep is a pointer to an unsigned long.
 *	
 * Remarks:
 *	Platform dependent. See remarks in __TIME_INIT_ASM macro 
 *	in util/ksynch_p.m4
 */
#define GET_TIME(timep) \
	((*(timep)) = psm_usec_time())

/*
 * maximum lock hold time for an uncontested lock. Such locks
 * are required for correctness and have no performance impact.
 * For example, the hold time for the cmn_err_lock is not 
 * critical, but the lock is necessary for correctness since
 * we only want one agent to write to console at any time.
 */
#define	LK_THRESHOLD	500000    /* 500ms */

/*
 * MACRO
 * GET_DELTA(ulong_t *deltap, ulong_t *stime) 
 *	Initializes the first parameter with the difference between
 *	the current time and the second parameter. Also, if the delta
 *	is greater than a threshold, it will set delta to 0.
 *
 * Calling/Exit State:
 *	The first arguement is a pointer to an unsigned long and
 *	the second argument is an unsigned long.
 *	
 * Remarks:
 *	Platform dependent. See remarks in __TIME_INIT_ASM macro 
 *	in util/ksynch_p.m4
 */
#define GET_DELTA(deltap, stime) \
{ \
	ulong_t deltmp; \
	deltmp = psm_usec_time() - (stime); \
	if (deltmp > LK_THRESHOLD) \
		deltmp = 0; \
	(*(deltap)) = deltmp; \
}


/*
 * MACRO
 * CONVERT_IPL(int var, int ipl) 
 *	Stores the ipl passed in the second argument in the
 *	first argument. 
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	If the platform uses negative logic to manipulate ipls, 
 *	the ones complement of the passed in ipl is stored in 
 *	the first argument. This is not such a platform.
 */
#define CONVERT_IPL(var, ipl)		((var) = (((ipl)) & (0xff)))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KSYNCH_P_H */

#ifndef _UTIL_VAR_H	/* wrapper symbol for kernel use */
#define _UTIL_VAR_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/var.h	1.14.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * System Configuration Information
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

struct var {
	int	v_buf;		/* Nbr of I/O buffers.			*/
	int	v_call;		/* Nbr of callout (timeout) entries.	*/
	int	v_proc;		/* Max nbr of processes system wide	*/
	int	v_maxsyspri;	/* Max global pri used by sys class.	*/
	int	v_maxup;	/* Max number of processes per user.	*/
	int	v_hbuf;		/* Nbr of hash buffers to allocate.	*/
	int	v_hmask;	/* Hash mask for buffers.		*/
	int	v_pbuf;		/* Nbr of physical I/O buffers.		*/
	int	v_autoup;	/* The age a delayed-write buffer must	*/
				/* be in seconds before bdflush will	*/
				/* write it out.			*/
	int	v_bufhwm;	/* high-water-mark of buffer cache	*/
				/* memory usage, in units of K Bytes	*/
	/* XENIX Support */
	int 	v_scrn;		/* number of multi-screens. (XENIX) 	*/
	int 	v_emap;		/* number of i/o mappings (XENIX) 	*/
	int 	v_sxt;		/* number sxt's for shell layers (XENIX)*/
	int	v_xsdsegs;	/* Number of XENIX shared data segs     */
	int	v_xsdslots;	/* Number of slots in xsdtab[] per seg  */
	/* End XENIX Support */
	uint_t	v_maxulwp;	/* per-uid # of lwps limit		*/
	uint_t	v_nonexclusive;	/* minimum number of non-exclusively bound engines */

	/*
	 * Maximum number of id's operated upon by exclusive bind system call.
 	 * This should be small enough so as not affect kernel preemption
	 * latency, but large enough for interested applications.
	 */
	uint_t	v_max_proc_exbind;/* maximum number of id's operated upon by
				   exclusive bind system call */
	uint_t	v_static_sq;	/* number of sync-queues */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL
extern struct var v;
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_VAR_H */

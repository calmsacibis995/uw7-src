#ifndef _UTIL_FAULT_H	/* wrapper symbol for kernel use */
#define _UTIL_FAULT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/fault.h	1.6"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Fault numbers, analagous to signals.  These correspond to
 * hardware faults.  Setting the appropriate flags in a process's
 * set of traced faults via /proc causes the process to stop each
 * time one of the designated faults occurs so that a debugger can
 * take action.  See proc(4) for details.
 */

	/* fault enumeration must begin with 1 */
#define	FLTILL		1	/* Illegal instruction */
#define	FLTPRIV		2	/* Privileged instruction */
#define	FLTBPT		3	/* Breakpoint instruction */
#define	FLTTRACE	4	/* Trace trap (single-step) */
#define	FLTACCESS	5	/* Memory access (e.g., alignment) */
#define	FLTBOUNDS	6	/* Memory bounds (invalid address) */
#define	FLTIOVF		7	/* Integer overflow */
#define	FLTIZDIV	8	/* Integer zero divide */
#define	FLTFPE		9	/* Floating-point exception */
#define	FLTSTACK	10	/* Unrecoverable stack fault */
#define	FLTPAGE		11	/* Recoverable page fault (no associated sig) */

typedef struct {		/* fault set type */
	unsigned int word[4];
} fltset_t;

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef unsigned int k_fltset_t;/* kernel internal version of fltset_t
				 * to save space, since fltset_t is
				 * not fully used yet. */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
        }
#endif

#endif /* _UTIL_FAULT_H */

#ifndef _UTIL_LOCKTEST_H	/* wrapper symbol for kernel use */
#define _UTIL_LOCKTEST_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/locktest.h	1.7"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* required */
#include <util/ksynch.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* required */
#include <sys/ksynch.h>

#endif /* _KERNEL_HEADERS */

typedef struct {
	ushort_t hss_value;		/* combined minipl+hier */
	void *hss_lockp;		/* pointer to lock */
	lkinfo_t *hss_infop;		/* pointer to lock info */
} hier_stackslot_t;

#define STACKMAX	100		/* no one would lock more than this
					 * many locks at once
					 */
typedef struct {
	int hs_top;			/* the next free slot */
	hier_stackslot_t hs_stack[STACKMAX];
} hier_stack_t;

#define	RWS_LOCKTYPE	0
#define	SP_LOCKTYPE	1

#if defined _KERNEL && defined __STDC__
extern void hier_push(void *, int, lkinfo_t *);
extern void hier_push_same(void *, int, lkinfo_t *);
extern void hier_push_nchk(void *, int, lkinfo_t *);
extern void hier_remove(void *, lkinfo_t *, pl_t);
extern boolean_t hier_findlock(void *lockp);
extern boolean_t hier_lockcount(int);
extern void locktest(void *, pl_t, pl_t , int, lkinfo_t *);
#endif /* _KERNEL && __STDC__ */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_LOCKTEST_H */

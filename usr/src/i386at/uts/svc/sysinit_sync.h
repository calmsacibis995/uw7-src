#ident	"@(#)kern-i386at:svc/sysinit_sync.h	1.1.2.1"
#ident	"$Header$ *"

#ifndef _SYSINIT_SYNC_H
#define _SYSINIT_SYNC_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/cguser.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/cguser.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef CCNUMA

typedef enum {
	SYNC_PARALLEL,
	SYNC_SEQUENTIAL,
} sync_type_t;

/*
 * Sync events (in order of execution).
 */

typedef enum {
	SYNC_EVENT_MAP_BOOTCG_CALLOC		= 1,
	SYNC_EVENT_MAP_PHYSTOKV			= 2,
	SYNC_EVENT_SERIALIZE1_CALLOC		= 3,
	SYNC_EVENT_SERIALIZE2_CALLOC		= 4,
	SYNC_EVENT_PAGE_HASH			= 5,
	SYNC_EVENT_KMA_CALLOC			= 6,
	SYNC_EVENT_SERIALIZE3_CALLOC		= 7,
	SYNC_EVENT_HATKSEG			= 8,
	SYNC_EVENT_CGVAR			= 9,
	SYNC_EVENT_PAGEPOOL			= 10,
	SYNC_EVENT_KMA				= 11,
	SYNC_EVENT_RESERVE			= 12,
	SYNC_EVENT_GLOBALS			= 13,
	SYNC_EVENT_SEGMENTS			= 14,
	SYNC_EVENT_PSM				= 15,
	SYNC_EVENT_CG_MAIN			= 16,
	SYNC_EVENT_POSTROOT			= 17
} sync_event_t;

/*
 * Barriers
 *
 *	Each sync point brings all CG leaders to three barriers.
 */
typedef enum {
	SYNC_PRE_CALLOC		= 0,	/* before calloc()ing */
	SYNC_DONE_CALLOC	= 1,	/* before cg_fixup() */
	SYNC_DONE_PT		= 2,	/* page table clone complete */
	SYNC_DONE_FIXUP		= 3,	/* kl2ptes complete */
} sync_barrier_t;

#define SYNC_MAKE_STAGE(event, barrier)		\
				(((event) << 8) + (barrier))

/*
 * sysinit_sync control information
 */
typedef struct {
	cgnum_t			ss_cg;			/* holding ss_spin */
	volatile int		ss_stage[MAXNUMCG];	/* current stage */
	vaddr_t			ss_begin_addr[MAXNUMCG];/* for cg_fixup() */
	vaddr_t			ss_end_addr[MAXNUMCG];	/* for cg_fixup() */
} sync_control_t;

extern void sysinit_sync(sync_event_t, sync_type_t);
extern void sysinit_sync_range(sync_event_t, vaddr_t, vaddr_t);
void sysinit_sync_init(void);

#else /* !CCNUMA */

#define sysinit_sync(event, type)	((void)0)
#define sysinit_sync_init()		((void)0)
#define sysinit_sync_range(event, s, e)	((void)0)

#endif /* CCNUMA */

#if defined(__cplusplus)
	}
#endif

#endif /* _SYSINIT_SYNC_H */

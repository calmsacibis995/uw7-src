#ifndef _UTIL_KSYNCH_H	/* wrapper symbol for kernel use */
#define _UTIL_KSYNCH_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/ksynch.h	1.47.6.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * kernel synchronization primitives
 */
#ifdef _KERNEL_HEADERS

#include <util/dl.h>		/* REQUIRED */
#include <util/ipl.h>		/* FOR DDI DRIVERS */
#include <util/ksynch_p.h>	/* PORTABILITY */
#include <util/list.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/dl.h>		/* REQUIRED */
#include <sys/ipl.h>		/* FOR DDI DRIVERS */
#include <sys/ksynch_p.h>	/* PORTABILITY */
#include <sys/list.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL) || defined(_KMEMUSER)


/*
 * atomic_int_t 
 *	This is a synthetic data type on which a few operations (increments,
 *	decrements, addition, subtraction) would be provided as
 *	atomic without need for external locking. On the x86 and other
 *	processor families that support locked instructions, the data
 *	type is just an int. On other processor families, the data type
 *	consists of an embedded locking structure (effectively an fspin_t) 
 *	protecting the int.
 */
typedef struct {
	volatile int ai_int;
} atomic_int_t;

/* 
 * lkinfo_t -- the owner of a lock allocates one of these when the lock
 *	is allocated and initializes it to the name of the lock.  The INIT
 *	function places a pointer to it from the lkstat_t, and the
 *	statistics-gathering tools follow the pointer to identify the
 *	lock with its statistics.
 *
 *	lkinfo_t's may be shared by more than one lock.  However, all the
 *	locks that share a lkinfo_t must be of the same type, sleep locks
 *	or basic locks.
 */
typedef struct {
	char *lk_name;		/* name of the lock, null terminated */
	int lk_flags;		/* flags, see values below */
	long lk_pad[2];		/* for future expansion, should be zeroed */
} lkinfo_t;

/*
 * values for lk_flags; BASIC and SLEEP are exclusive, NOSTATS only
 * affects sleep locks.
 */
#define LK_BASIC	0x1	/* for a basic lock, set by XX_INIT() */
#define LK_SLEEP	0x2	/* for a blocking lock, set by XX_INIT() */
#define LK_NOSTATS	0x4	/* don't collect stats; set by user */

#ifdef _KERNEL

/*
 * macro to declare a lkinfo_t.  This will make it easier to find all the
 * locks in the kernel, for purposes of assigning hierarchy values, etc.
 * NOTE: This macro is used by DDI drivers.
 */
#define LKINFO_DECL(var, name, flags)	\
	lkinfo_t var = { name, flags, {0,0} }

/*
 * macro to initialize a lkinfo_t.
 */
#define LKINFO_INIT(var, name, flags)		\
	((void)(				\
		(var).lk_name = (name),		\
		(var).lk_flags = (flags),	\
		(var).lk_pad[0] = 0,		\
		(var).lk_pad[1] = 0		\
	       )				\
	)

#endif /* _KERNEL */

#ifndef _DDI

/*
 * statistics buffer.  One allocated for each interesting lock.
 */
typedef struct _lkstat {
	lkinfo_t *lks_infop;	/* pointer to info structure */
	ulong_t lks_wrcnt;	/* num times writelocked (exclusive) */
	ulong_t lks_rdcnt;	/* num times readlocked (shared-RW only) */
	ulong_t lks_solordcnt;	/* num times rdlcked w/out other readers */
	ulong_t lks_fail;	/* number of failed acquisitions */
	/*
	 * this union will be a problem if sizeof(lkstat_t *) >
	 * 	sizeof(dl_t)
	 */
	union {
		dl_t lksu_stime;	/* start time when acquired */
		struct _lkstat *lksu_next;	/* for free list */
	} lksu;
#define lks_stime lksu.lksu_stime
#define lks_next lksu.lksu_next
	dl_t lks_wtime;		/* cumulative wait time */
	dl_t lks_htime;		/* cumulative hold time */
} lkstat_t;

#define LSB_NLKDS	92	/* sizeof(lksblk_t) should be <= k*NBPG */

typedef struct _lksblk {
	struct _lksblk *lsb_prev, *lsb_next;	/* doubly-linked list */
	int lsb_nfree;			/* num free lkstat_t's in block */
	lkstat_t *lsb_free;		/* head of free list */
	lkstat_t lsb_bufs[LSB_NLKDS];	/* block of bufs */
} lksblk_t;		/* size should be close to pagesize */

#endif /* !_DDI */

#ifdef _KERNEL

/*
 * Macros for manipulating the preemption state of the engine.
 */
extern pl_t getpl(void);

#ifndef _DDI

extern volatile int prmpt_state;
extern short prmpt_count;
extern uint_t engine_evtflags;

#define EVT_RUNRUN	0x01	/* user-level preemption pending */
#define EVT_KPRUNRUN	0x02	/* kernel preemption pending */

#define DISABLE_PRMPT() (++prmpt_state) 
#define ENABLE_PRMPT()  \
		(--prmpt_state == 0 ? check_preemption() : (void)0)
extern void check_preemption(void);

/*
 * prototypes for functions in lkstat.c; there's no corresponding .h
 * file for that .c file, so these go here.
 */
extern void lkstat_init(void);
extern lkstat_t *lkstat_alloc(lkinfo_t *, int);
extern void lkstat_free(lkstat_t *, boolean_t);
extern void blk_statzero_all(void);

extern char *lk_panicstr[];	/* used by assembly functions */

#if (defined _LOCKTEST && !defined UNIPROC)
extern boolean_t hier_lockcount(int);
#define KS_HOLD1LOCK()	(hier_lockcount(1))
#define KS_HOLD0LOCKS()	(hier_lockcount(0))
#define KS_HOLDLOCKS()	(!KS_HOLD0LOCKS())
#else
#define KS_HOLD1LOCK()	(B_TRUE)
#define KS_HOLD0LOCKS()	(B_TRUE)
#define KS_HOLDLOCKS()	(B_TRUE)
#endif /* _LOCKTEST && !UNIPROC */

#endif /* !_DDI */

#endif /* _KERNEL */

typedef unsigned char fspin_t;		/* fast spin lock */

typedef struct {
	uchar_t sp_lock;		/* lock field (see below) */
#if (defined DEBUG || defined SPINDEBUG) 
	uchar_t sp_flags;		/* flags (see below) */
	union {
		struct {
			uchar_t spus_hier;   /* hierarchy value */
			uchar_t spus_minipl; /* min ipl to acquire lock */
		} spus;
		ushort_t spu_value;	/* combined */
	} spu;
#define sp_minipl spu.spus.spus_minipl
#define sp_hier   spu.spus.spus_hier
#define sp_value  spu.spu_value
	struct _lkstat *sp_lkstatp;           /* pointer to statistics buf */
	lkinfo_t *sp_lkinfop;		/* pointer to the info struct. */
#endif /* (DEBUG || SPINDEBUG) */
} lock_t;

#ifndef _DDI

/*
 * values for lock_t.sp_lock
 */
#define SP_UNLOCKED	0
#define SP_LOCKED	1

#endif /* !_DDI */

/*
 * values for lock_t.sp_flags
 */
#define KS_LOCKTEST	0x01
#define KS_MPSTATS	0x02
#define KS_DEINITED	0x04
#define KS_NVLTTRACE	0x08	/* trace this lock with NVLT */

/*
 * the reader-writer spin lock.
 */
typedef struct {
	fspin_t rws_fspin;		/* fspin protecting rws_rdcount field */
	volatile uchar_t rws_lock;	/* state interlock */
	volatile uchar_t rws_rdcount;	/* number of readers minus 1 */
	volatile uchar_t rws_state; 	/* state (see below) */
#if (defined DEBUG || defined SPINDEBUG)
	uchar_t rws_flags;		/* flags, same as lock_t */
	uchar_t rws_pad;		/* pad to word-boundry */
	union {
		struct {
			uchar_t rwsu_hier;	/* hierarchy value */
			uchar_t rwsu_minipl;	/* min ipl to acquire lock */
		} rwsus;
		ushort_t rwsu_value;		/* hierarchy+ipl value */
	} rwsu;
#define rws_hier	rwsu.rwsus.rwsu_hier
#define rws_minipl	rwsu.rwsus.rwsu_minipl
#define rws_value	rwsu.rwsu_value
	struct _lkstat *rws_lkstatp;		/* pointer to stat buf */
	lkinfo_t *rws_lkinfop;		/* pointer to info struct */
#endif /* DEBUG || SPINDEBUG */
} rwlock_t;

#ifndef _DDI

/*
 * states for rws_state
 */
#define RWS_READ	('r')
#define RWS_WRITE	('w')
#define RWS_UNLOCKED	('u')
#define	RWS_BUSY	('b')

#endif /* _DDI */

#ifdef _KERNEL

/*
 * externs for priorities that can be passed to blocking synch primitives by
 * DDI drivers.
 */

extern int pridisk, prinet, pritty, pritape, prihi, primed, prilo;

#endif /* _KERNEL */

/*
 * the sleep lock
 *
 * N.B. For performance reasons, align the sl_avail field on
 *	a four-byte boundary; placing the field after sl_list
 *	insures this.  (Note that alignment is not required
 *	for correctness, only for performance.)
 */
typedef struct {
	list_t sl_list;			/* private sleep queues */
#define	sl_head	sl_list.flink
#define	sl_tail	sl_list.rlink
	volatile uchar_t sl_avail;	/* 1 if free, 0 if held */
	fspin_t sl_fspin;		/* state interlock */
	uchar_t sl_flags;		/* same as for lock_t.sp_flags */
	uchar_t sl_hierarchy;		/* to enforce lock ordering protocol */
	volatile short sl_pend;		/* non-zero if waiters not on queue */
	volatile k_lwpid_t sl_lwpid;	/* id of the owner lwp */
	volatile uchar_t sl_fixpend;	/* correction to pending count */
	volatile pid_t sl_pid;		/* id of the containing process */
	struct _lkstat *sl_lkstatp;		/* statistics buffer */
} sleep_t;

/*
 * Special pid value written into sl_pid when lock is becoming free, but not
 *	quite free.  Used to prevent races in sleep_unlock.
 */
#define	SL_BUSY	((pid_t)(NOPID - 1))

#ifdef _KERNEL

#if !defined(DEBUG) && !defined(_DDI)
/*
 * macros to peek at the state of the sleep lock.  Not for DDI
 * drivers, so should be undef'ed in <ddi.h>
 */
#define SLEEP_LOCKAVAIL(lkp)	(((lkp)->sl_avail != 0) && \
					((lkp)->sl_pid != SL_BUSY))
#define SLEEP_LOCKBLKD(lkp)	(FSPIN_IS_LOCKED(&(lkp)->sl_fspin) \
					|| !EMPTYQUE(&(lkp)->sl_list) \
					|| ((lkp)->sl_pend > 0))
#define SLEEP_LOCKOWNED(lkp)	\
	((lkp)->sl_pid == u.u_lwpp->l_procp->p_pidp->pid_id \
		&& (lkp)->sl_lwpid == u.u_lwpp->l_lwpid)
#else

extern boolean_t SLEEP_LOCKAVAIL(sleep_t *);
extern boolean_t SLEEP_LOCKOWNED(sleep_t *);
#if !defined(_DDI)
extern boolean_t SLEEP_LOCKBLKD(sleep_t *);
#endif /* _DDI */

#endif /* DEBUG */

#if !defined(_DDI)
/*
 * Macro to disown a sleep lock. The calling context must own the lock.
 * After disowning the lock, the lock is still held but all the ownership
 * info is lost. 
 */
#define SLEEP_DISOWN(lkp)	\
{ \
	ASSERT(SLEEP_LOCKOWNED((lkp)) || ((lkp)->sl_pid == NOPID)); \
	(lkp)->sl_pid = NOPID; \
}	
#endif /* !_DDI */

#endif /* _KERNEL */

/*
 * the synchronization variable
 */
typedef struct {
	list_t sv_list;         /* private sleep queues */
#define sv_head sv_list.flink
#define sv_tail sv_list.rlink
	fspin_t sv_lock;        /* interlock protecting the queues */
} sv_t;

#if defined(_KERNEL) && !defined(_DDI)

/*
 * macros to peek at the state of an sv.  Should not be used by DDI
 * drivers, so will be undef'ed in <ddi.h>
 */
#define SV_BLKD(svp)	(!EMPTYQUE(&(svp)->sv_list))

#endif /* _KERNEL && !_DDI */

/*
 * flags to indicate the type of wakeup desired.  No bits set
 * indicates the default, preemptive wakeup.  Used in an |int|.
 */
#define KS_NOPRMPT	0x00000001

/*
 * the reader-writer sleep lock.
 *
 * N.B. For performance reasons, align the rw_avail field on
 *	a four-byte boundary; placing the field after rw_list
 *	insures this.  (Note that alignment is not required
 *	for correctness, only for performance.)
 *
 *	The rw_pend field may be modified by an LWP only if it
 *	holds the lock in write mode, or if the LWP holds the
 *	lock in read mode and holds the fspin lock (rw_fspin).
 */
typedef struct {
	list_t rw_list;			/* private sleep queues */
#define rw_head rw_list.flink
#define rw_tail rw_list.rlink
	volatile uchar_t rw_avail;	/* 1 if free, 0 if held */
        fspin_t rw_fspin;		/* fspin protecting rw_read & rw_pend */
        uchar_t rw_flags;		/* same as for lock_t.sp_flags */
	uchar_t rw_hierarchy;		/* to enforce lock ordering protocol */
	volatile uchar_t rw_mode;	/* RWS_READ, RWS_WRITE, RWS_UNLOCKED */
	volatile short rw_read; 	/* number of readers minus 1 */
	volatile short rw_pend;		/* non-zero if waiters not on queue */
        struct _lkstat *rw_lkstatp;		/* pointer to stat structure */
} rwsleep_t;

#if defined(_KERNEL) && !defined(_DDI)

/*
 * macros to peek at the state of a rwsleep_t.  Not to be used by DDI
 * drivers, so these should are undef'ed in <ddi.h>
 */

#define RWSLEEP_IDLE(lkp)	(((lkp)->rw_avail) && \
					((lkp)->rw_mode != RWS_BUSY))

#define RWSLEEP_LOCKBLKD(lkp)	(FSPIN_IS_LOCKED(&(lkp)->rw_fspin) \
					|| !EMPTYQUE(&(lkp)->rw_list) \
					|| ((lkp)->rw_pend > 0))

#endif /* _KERNEL && !_DDI */

typedef struct {
	list_t ev_list;		/* private sleep queues */
#define ev_head ev_list.flink
#define ev_tail ev_list.rlink
	fspin_t ev_lock;	/* Interlock protecting the queues */
	volatile short ev_state;/* state info:
				 * 0: initial state
				 * 1: event ocurred
			   	 * if ev_state < 0, then |ev_state|
			         * is the number of blocked lwps.
			         */
} event_t;

#if defined(_KERNEL) && !defined(_DDI)

/*
 * macros to peek at the state of the event_t.  Should not be called
 * from DDI drivers, so these are undef'ed in <ddi.h>
 */
#define EVENT_BLKD(evp)		((evp)->ev_state < 0)

#endif /* _KERNEL && !_DDI */

#ifdef _KERNEL

/*
 * figure out what KSFLAGS should be; the combination of _LOCKTEST,
 * _MPSTATS, et al, present at compile time, passed to XX_ALLOC and XX_INIT
 * functions of basic and blocking locks under DEBUG.
 */
#if defined _LOCKTEST
#define __KS_LOCKTEST	KS_LOCKTEST
#else
#define __KS_LOCKTEST	0
#endif
#if defined _MPSTATS
#define __KS_MPSTATS	KS_MPSTATS
#else
#define __KS_MPSTATS	0
#endif
#if defined DEBUG_TRACE_LOCKS
#define __KS_NVLTTRACE	KS_NVLTTRACE
#else
#define __KS_NVLTTRACE	0
#endif
#define KSFLAGS (__KS_LOCKTEST|__KS_MPSTATS|__KS_NVLTTRACE)

/*
 * Figure out the argument to be passed into sv primitives.
 */
#define KSVUNIPROC	1
#define KSVMPDEBUG	2
#define KSVMPNODEBUG	3

#if defined UNIPROC
#define KSVFLAG	KSVUNIPROC	
#endif /* UNIPROC */

#if (defined DEBUG || defined SPINDEBUG)  && !defined UNIPROC
#define KSVFLAG	KSVMPDEBUG	
#endif /* (DEBUG || SPINDEBUG) && !UNIPROC */

#if (!defined DEBUG && !defined SPINDEBUG) && !defined UNIPROC
#define KSVFLAG	KSVMPNODEBUG 
#endif /* (!DEBUG && !SPINDEBUG) && !UNIPROC */


#if !defined(_DDI)

/*
 * Include the inline file for synch primitives. This include is done 
 * here so that all the synch primitive types will have already been 
 * defined.
 */

#ifdef _KERNEL_HEADERS
#include <util/ksinline.h>
#else
#include <sys/ksinline.h>
#endif

#define _ATOMIC_INT_INIT(atomic_intp, ival)	((atomic_intp)->ai_int = ival)
#define _ATOMIC_INT_READ(atomic_intp)		((atomic_intp)->ai_int)

/*
 * The ATOMIC_INT_WRITE() construct below does not guarantee that the
 * value being written is flushed out to a coherent location; just that
 * the write would be atomic with respect to a competing write of the
 * variable. This is trivially guaranteed for 8, 16, or 32 byte
 * writes for the x86 family.
 * If the atomic_write_and_flush behavior is desired, it should be
 * achieved either by explicit lock based serialization or through
 * the use of additional WRITE_SYNC() operations following the
 * ATOMIC_INT_WRITE().
 */
#define _ATOMIC_INT_WRITE(atomic_intp, ival)	((atomic_intp)->ai_int = ival)

#define _ATOMIC_INT_INCR(atomic_intp)			\
	atomic_int_incr(&((atomic_intp)->ai_int))
#define _ATOMIC_INT_DECR(atomic_intp)			\
	atomic_int_decr(&((atomic_intp)->ai_int))
#define _ATOMIC_INT_ADD(atomic_intp, ival)		\
	atomic_int_add(&((atomic_intp)->ai_int), ival)
#define _ATOMIC_INT_SUB(atomic_intp, ival)		\
	atomic_int_sub(&((atomic_intp)->ai_int), ival)

#define ATOMIC_INT_INIT(atomic_intp, ival)  _ATOMIC_INT_INIT(atomic_intp, ival)
#define ATOMIC_INT_READ(atomic_intp) 	    _ATOMIC_INT_READ(atomic_intp)
#define ATOMIC_INT_WRITE(atomic_intp, ival) _ATOMIC_INT_WRITE(atomic_intp, ival)
#define ATOMIC_INT_INCR(atomic_intp) 	    _ATOMIC_INT_INCR(atomic_intp)
#define ATOMIC_INT_DECR(atomic_intp) 	    _ATOMIC_INT_DECR(atomic_intp)
#define ATOMIC_INT_ADD(atomic_intp, ival)   _ATOMIC_INT_ADD(atomic_intp, ival)
#define ATOMIC_INT_SUB(atomic_intp, ival)   _ATOMIC_INT_SUB(atomic_intp, ival)

#endif /* _DDI */

extern atomic_int_t *ATOMIC_INT_ALLOC(int);
extern void ATOMIC_INT_DEALLOC(atomic_int_t *);

#if defined(UNIPROC) && !defined(_DDI)

/*
 * In a uniprocessor kernel, the basic XX_LOCK functions are changed
 * to spl(ipl), XX_UNLOCKs are changed to splx(ipl), and everything
 * else goes away.  Note that XX_ALLOC functions are changed to "1"
 * instead of NULL because these functions return NULL to indicate
 * out-of-memory.
 */
#define FSPIN_INIT(lp)          /* undef */
#define FSPIN_LOCK(l)		DISABLE()

/*
 * FSPIN_TRYLOCK() returns a boolean_t.         
 */
#define FSPIN_TRYLOCK(l)	(DISABLE(), B_TRUE)
#define FSPIN_UNLOCK(l)		ENABLE()

/*
 * For a uniprocessor kernel, the READ_SYNC and WRITE_SYNC operations
 * are null.
 */
#define READ_SYNC()		/* undef */
#define WRITE_SYNC()		/* undef */

#define LOCK_INIT(lp, h, m, p, s)	/* undef */
#define LOCK_DEINIT(lp)		/* undef */
#define LOCK_ALLOC(h,i,p,s)     ((lock_t *)1)
#define LOCK_DEALLOC(lp)	/* undef */
#define LOCK_OWNED(lp)		(B_TRUE)

#define LOCK(lockp, ipl) ( \
		DISABLE_PRMPT(), \
		spl(ipl) \
	)	

#define LOCK_SH(lockp, ipl) 	LOCK(lockp, ipl) 
#define TRYLOCK(lockp, ipl)	LOCK(lockp, ipl) 

/*
 * XXX: We should use the ENABLE_PRMPT() macro in the UNLOCK() macro.
 * For now, we ignore dispatch latency issues and simply enable 
 * preemption instead.
 */
#define UNLOCK(lockp, ipl) ( \
	        splx(ipl), \
		(void)(--prmpt_state) \
	)
#define RW_INIT(lp, h, m, p, s)	/* undef */
#define RW_DEINIT(lp)		/* undef */
#define RW_DEALLOC(lp)		/* undef */
#define RW_ALLOC(h,i,p,s)       ((rwlock_t *)1)
#define RW_OWNED(lp)		(B_TRUE)
#define RW_RDLOCK(lockp, ipl)	LOCK(lockp, ipl) 
#define RW_RDLOCK_SH(lockp, ipl) LOCK(lockp, ipl)
#define RW_WRLOCK(lockp, ipl)   LOCK(lockp, ipl) 
#define RW_WRLOCK_SH(lockp, ipl) LOCK(lockp, ipl)
#define RW_TRYRDLOCK(lp, ipl)   LOCK(lockp, ipl) 
#define RW_TRYWRLOCK(lp, ipl)   LOCK(lockp, ipl) 
#define RW_UNLOCK(lockp, ipl)   UNLOCK(lockp, ipl) 

#define IS_LOCKED(lockp)	B_FALSE

/*
 * The following PLMIN lock macros are processsor dependent.
 * On architectures that can support the state of disabling interrupts
 * and set the executing pl value to be PLBASE at system initialization
 * time (only an issue at initialization time) then these macros just act
 * as aliases to the corresponding lock macros. On architectures that
 * cannot support this state, there needs to be an indication from sysinit code
 * that initialization is taking place. This is because acquiring this lock
 * at MINPL may result in lowering of ipl.  On such architectures,
 * the sysinit code needs to set a variable state indicating that the system
 * is initializing and on this notification these macros will be no-ops.
 *
 * These macros do nothing in the UNIPROC case except for enabling and
 * disabling interrupts.
 */

#define LOCK_PLMIN(lockp)	\
	(ASSERT(PLMIN == PLBASE), DISABLE_PRMPT(), PLBASE)

#define UNLOCK_PLMIN(lockp, ipl)	ENABLE_PRMPT()
#define TRYLOCK_PLMIN(lockp)		LOCK_PLMIN(lockp)
#define LOCK_SH_PLMIN(lockp)		LOCK_PLMIN(lockp)
#define RW_RDLOCK_PLMIN(lockp)		LOCK_PLMIN(lockp)
#define RW_WRLOCK_PLMIN(lockp)		LOCK_PLMIN(lockp)
#define RW_UNLOCK_PLMIN(lockp, ipl)	UNLOCK_PLMIN(lockp, ipl)


#else /* !UNIPROC || _DDI */

#if !defined(_DDI)

/*
 * READ_SYNC is a noop for the i386 family.
 * WRITE_SYNC needs to flush the processor write buffer.
 */
#define READ_SYNC()	/* undef */
#define WRITE_SYNC()	__write_sync()

#endif /* !_DDI */

/*
 * entry point aliases:  every kernel contains an entry point for
 * each routine with and without debugging.  This file is included
 * in driver source, and it arranges for "LOCK" to be changed to
 * "lock" or "lock_dbg", depending on whether DEBUG is defined.
 * The kernel itself uses inlined primitives, and the lock_dbg
 * entry points when DEBUG is defined.  This trickery is performed
 * here, as well.  SPINDEBUG has the same effect on this code as
 * DEBUG.
 */
#if defined DEBUG || defined SPINDEBUG

#define LOCK(lockp, pl)		lock_dbg(lockp, pl, B_FALSE)
#define TRYLOCK			trylock_dbg
#define UNLOCK			unlock_dbg
#define LOCK_DEALLOC		lock_dealloc_dbg
#define LOCK_OWNED		lock_owned_dbg

#define RW_RDLOCK(lockp, pl)	rw_rdlock_dbg(lockp, pl, B_FALSE)
#define RW_WRLOCK(lockp, pl)	rw_wrlock_dbg(lockp, pl, B_FALSE)
#define RW_TRYRDLOCK		rw_tryrdlock_dbg
#define RW_TRYWRLOCK		rw_trywrlock_dbg
#define RW_UNLOCK		rw_unlock_dbg
#define RW_DEALLOC		rw_dealloc_dbg
#define RW_OWNED		rw_owned_dbg

#define LOCK_ALLOC(h, i, p, s)	lock_alloc_dbg(h, i, p, s, KSFLAGS)
#define RW_ALLOC(h, i, p, s)	rw_alloc_dbg(h, i, p, s, KSFLAGS)

#if !defined(_DDI)

#define LOCK_INIT(l, h, i, p, s) lock_init_dbg(l, h, i, p, s, KSFLAGS)
#define RW_INIT(l, h, i, p, s)	rw_init_dbg(l, h, i, p, s, KSFLAGS)
#define LOCK_DEINIT		lock_deinit_dbg
#define RW_DEINIT		rw_deinit_dbg

#define LOCK_SH(lockp, pl)	lock_dbg(lockp, pl, B_TRUE)
#define RW_RDLOCK_SH(lockp, pl)	rw_rdlock_dbg(lockp, pl, B_TRUE)
#define RW_WRLOCK_SH(lockp, pl)	rw_wrlock_dbg(lockp, pl, B_TRUE)

#define IS_LOCKED(lockp)	((lockp)->sp_lock)

#endif /* !_DDI */

#else /* DEBUG || SPINDEBUG */

#define LOCK		lock_nodbg
#define TRYLOCK		trylock_nodbg
#define UNLOCK		unlock_nodbg
#define LOCK_ALLOC	lock_alloc
#define LOCK_DEALLOC	lock_dealloc
#define LOCK_OWNED	lock_owned

#define RW_RDLOCK	rw_rdlock
#define RW_WRLOCK	rw_wrlock
#define RW_TRYRDLOCK	rw_tryrdlock
#define RW_TRYWRLOCK	rw_trywrlock
#define RW_UNLOCK	rw_unlock
#define RW_ALLOC	rw_alloc
#define RW_DEALLOC	rw_dealloc
#define RW_OWNED	rw_owned

#if !defined(_DDI)

#define LOCK_INIT	lock_init
#define LOCK_DEINIT(l)	/* un-defined */
#define RW_INIT		rw_init
#define RW_DEINIT	/* un-defined */

#define LOCK_SH		lock_nodbg		/* same as lock */
#define RW_RDLOCK_SH	rw_rdlock	/* same as rw_rdlock */
#define RW_WRLOCK_SH	rw_wrlock	/* same as rw_wrlock */

#endif /* !_DDI */

#endif /* DEBUG || SPINDEBUG */

#if !defined(_DDI)

/*
 * See comments above in the UNIPROC case for the PLMIN lock macro
 * comments.
 */
#define LOCK_PLMIN(lockp)		LOCK((lockp), PLMIN)
#define UNLOCK_PLMIN(lockp, ipl)	UNLOCK((lockp), (ipl))
#define TRYLOCK_PLMIN(lockp)		TRYLOCK((lockp), PLMIN)
#define LOCK_SH_PLMIN(lockp)		LOCK_SH((lockp), PLMIN)
#define RW_RDLOCK_PLMIN(lockp)		RW_RDLOCK((lockp), PLMIN)
#define RW_WRLOCK_PLMIN(lockp)		RW_WRLOCK((lockp), PLMIN)
#define RW_UNLOCK_PLMIN(lockp, ipl)	RW_UNLOCK((lockp), (ipl))

#endif /* !_DDI */

#endif /* !UNIPROC */

#if defined(UNIPROC) && !defined(DEBUG) && !defined(SPINDEBUG) && !defined(_DDI)

/*
 * Under these conditions we want to use the inline variants.
 */
#define spl		__spl
#define splx		__splx

#endif /* UNIPROC && !DEBUG && !SPINDEBUG && !_DDI */

#define SLEEP_ALLOC(h, p, s)	sleep_alloc(h, p, s, KSFLAGS)
#define SLEEP_DEALLOC		sleep_dealloc
#define SLEEP_LOCK		sleep_lock
#define SLEEP_LOCK_SIG		sleep_lock_sig
#define SLEEP_UNLOCK		sleep_unlock
#define SLEEP_TRYLOCK		sleep_trylock

#if !defined(_DDI)

#define SLEEP_INIT(l, h, p, s)	sleep_init(l, h, p, s, KSFLAGS)
#define SLEEP_DEINIT		sleep_deinit
#define SLEEP_LOCK_PRIVATE	sleep_lock_private
#define SLEEP_LOCK_RELLOCK	sleep_lock_rellock
#define SLEEP_UNSLEEP		sleep_unsleep

#define RWSLEEP_ALLOC(h, p, s)	rwsleep_alloc(h, p, s, KSFLAGS)
#define RWSLEEP_INIT(l, h, p, s)	rwsleep_init(l, h, p, s, KSFLAGS)
#define RWSLEEP_DEINIT		rwsleep_deinit
#define RWSLEEP_DEALLOC		rwsleep_dealloc
#define RWSLEEP_RDLOCK		rwsleep_rdlock
#define RWSLEEP_RDLOCK_RELLOCK	rwsleep_rdlock_rellock
#define RWSLEEP_WRLOCK		rwsleep_wrlock
#define RWSLEEP_WRLOCK_RELLOCK	rwsleep_wrlock_rellock
#define RWSLEEP_TRYRDLOCK	rwsleep_tryrdlock
#define RWSLEEP_TRYWRLOCK	rwsleep_trywrlock
#define RWSLEEP_UNLOCK		rwsleep_unlock
#define RWSLEEP_RDAVAIL		rwsleep_rdavail

#endif /* !_DDI */

#define SV_ALLOC		sv_alloc

#ifdef DEBUG
#define SV_DEALLOC		sv_dealloc_dbg
#else
#define SV_DEALLOC              sv_dealloc
#endif /* DEBUG */

#define SV_WAIT(sp, pri,lkp)	sv_wait(sp, pri, lkp, KSVFLAG)
#define SV_WAIT_SIG(sp, pri, lkp)	sv_wait_sig(sp, pri, lkp, KSVFLAG)
#define SV_SIGNAL		sv_signal
#define SV_BROADCAST		sv_broadcast

#if !defined(_DDI)

#define SV_INIT			sv_init
#define SV_UNSLEEP		sv_unsleep

struct lwp;		/* for slpdeque and XX_UNSLEEP */

boolean_t slpdeque(list_t *listp, struct lwp *lwpp);

#ifdef	UNIPROC

#define	FSPIN_IS_LOCKED(fsp)	B_FALSE

#else

#define	FSPIN_IS_LOCKED(fsp)	(*(fsp))

#endif

#ifdef UNIPROC
#define	SPIN_IS_LOCKED(lockp)	B_FALSE
#else
#define	SPIN_IS_LOCKED(lockp)	(*((char *)lockp) != 0)
#endif

#ifndef UNIPROC
extern void FSPIN_LOCK(fspin_t *);
extern boolean_t FSPIN_TRYLOCK(fspin_t *);
extern void FSPIN_UNLOCK(fspin_t *);
#endif /* UNIPROC */

#endif /* !_DDI */

#if defined DEBUG || defined SPINDEBUG
#if (defined _LOCKTEST && !defined UNIPROC)
extern boolean_t FSPIN_OWNED(fspin_t *);
#else
#define FSPIN_OWNED(l)	(B_TRUE)
#endif /* _LOCKTEST && !UNIPROC */
extern void lock_init_dbg(lock_t *, uchar_t, pl_t, lkinfo_t *, int, int);
extern void lock_deinit_dbg(lock_t *);
extern lock_t *lock_alloc_dbg(uchar_t, pl_t, lkinfo_t *, int, int);
extern pl_t lock_dbg(lock_t *, pl_t, boolean_t);
extern void rw_init_dbg(rwlock_t *, uchar_t, pl_t, lkinfo_t *, int, int);
extern rwlock_t *rw_alloc_dbg(uchar_t, pl_t, lkinfo_t *, int, int);
extern void rw_deinit_dbg(rwlock_t *);
extern pl_t rw_wrlock_dbg(rwlock_t *, pl_t, boolean_t);
extern pl_t rw_rdlock_dbg(rwlock_t *, pl_t, boolean_t);
extern void unlock_dbg(lock_t *, pl_t);
extern pl_t trylock_dbg(lock_t *, pl_t);
#else	/* DEBUG || SPINDEBUG */
extern pl_t lock_nodbg(lock_t *, pl_t);
extern void unlock_nodbg(lock_t *, pl_t);
extern pl_t trylock_nodbg(lock_t *, pl_t);
extern lock_t *lock_alloc(uchar_t h, pl_t min, lkinfo_t *lk, int flags);
extern void lock_init(lock_t *l, uchar_t h, pl_t min, lkinfo_t *lk, int flags);
extern void rw_init(rwlock_t *l, uchar_t h, pl_t min, lkinfo_t *lk, int flags);
extern rwlock_t *rw_alloc(uchar_t h, pl_t min, lkinfo_t *lk, int flags);
extern pl_t rw_wrlock(rwlock_t *, pl_t);
extern pl_t rw_rdlock(rwlock_t *, pl_t);
#endif /* DEBUG || SPINDEBUG */

extern void sleep_init(sleep_t *, uchar_t, lkinfo_t *, int, int);
extern sleep_t *sleep_alloc(uchar_t, lkinfo_t *, int, int);
extern void rwsleep_init(rwsleep_t *, uchar_t, lkinfo_t *, int, int);
extern rwsleep_t *rwsleep_alloc(uchar_t, lkinfo_t *, int, int);
extern void sv_wait(sv_t *, int, lock_t *, int);
extern boolean_t sv_wait_sig(sv_t *, int, lock_t *, int);

#ifndef UNIPROC
extern boolean_t LOCK_OWNED(lock_t *);
extern void LOCK_DEALLOC(lock_t *);

extern boolean_t RW_OWNED(rwlock_t *);
extern pl_t RW_TRYRDLOCK(rwlock_t *, pl_t);
extern pl_t RW_TRYWRLOCK(rwlock_t *, pl_t);
extern void RW_UNLOCK(rwlock_t *, pl_t);
extern void RW_DEALLOC(rwlock_t *);

#endif /* !UNIPROC */

extern void DISABLE(void);
extern void ENABLE(void);

extern void SLEEP_DEALLOC(sleep_t *);
extern void SLEEP_LOCK(sleep_t *, int);
extern boolean_t SLEEP_LOCK_SIG(sleep_t *, int);
extern boolean_t SLEEP_TRYLOCK(sleep_t *);
extern void SLEEP_UNLOCK(sleep_t *);

extern sv_t *SV_ALLOC(int);
extern void SV_DEALLOC(sv_t *);
extern void SV_SIGNAL(sv_t *, int);
extern void SV_BROADCAST(sv_t *, int);

#ifndef _DDI

#ifndef UNIPROC
extern void FSPIN_INIT(fspin_t *);
#endif /* !UNIPROC */

extern void SLEEP_DEINIT(sleep_t *);
extern void SLEEP_LOCK_RELLOCK(sleep_t *, int, lock_t *);
extern void SLEEP_LOCK_PRIVATE(sleep_t *);
extern boolean_t SLEEP_UNSLEEP(sleep_t *, struct lwp *);

extern void SV_INIT(sv_t *);
extern boolean_t SV_UNSLEEP(sv_t *, struct lwp *);

extern void RWSLEEP_DEINIT(rwsleep_t *);
extern void RWSLEEP_DEALLOC(rwsleep_t *);
extern void RWSLEEP_RDLOCK(rwsleep_t *, int);
extern void RWSLEEP_RDLOCK_RELLOCK(rwsleep_t *, int, lock_t *);
extern void RWSLEEP_WRLOCK(rwsleep_t *, int);
extern void RWSLEEP_WRLOCK_RELLOCK(rwsleep_t *, int, lock_t *);
extern boolean_t RWSLEEP_TRYRDLOCK(rwsleep_t *);
extern boolean_t RWSLEEP_TRYWRLOCK(rwsleep_t *);
extern void RWSLEEP_UNLOCK(rwsleep_t *);
extern boolean_t RWSLEEP_RDAVAIL(rwsleep_t *);

extern void EVENT_INIT(event_t *);
extern void EVENT_CLEAR(event_t *);
extern event_t *EVENT_ALLOC(int);
extern void EVENT_DEALLOC(event_t *);
extern void EVENT_WAIT(event_t *, int);
extern boolean_t EVENT_WAIT_SIG(event_t *, int);
extern void EVENT_SIGNAL(event_t *, int);
extern boolean_t EVENT_UNSLEEP(event_t *, struct lwp *);
extern void EVENT_BROADCAST(event_t *, int);

#endif /* !_DDI */

#endif /* _KERNEL */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KSYNCH_H */

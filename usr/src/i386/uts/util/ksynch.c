#ident	"@(#)kern-i386:util/ksynch.c	1.14"
#ident	"$Header$"

/*
 * Initialization and allocation of basic locks:  these C routines
 * go with the inline asm routines in inline.h and the asm functions
 * in asmlocks.s.  This file will be compiled once with DEBUG defined,
 * once without.
 */

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/list.h>
#include <util/plocal.h>
#include <mem/kmem.h>
#include <proc/signal.h>
#include <proc/lwp.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#if (defined DEBUG || defined SPINDEBUG)
char *lk_panicstr[] = {
	/* 0, 0x00 */ "locking at below minipl",
	/* 1, 0x04 */ "locking with lowered ipl",
	/* 2, 0x08 */ "attempt to lock held lock",
	/* 3, 0x0C */ "unlocking unlocked lock",
	/* 4, 0x10 */ "bad rwlock state value",
	/* 5, 0x14 */ "nested fspin locks",
	/* 6, 0x18 */ "nested DISABLE",
	/* 7, 0x1C */ "ENABLE without DISABLE"
};
#endif /* DEBUG */

/*
 * void
 * LOCK_INIT(lock_t *, uchar_t hier, pl_t min, lkinfo_t *, int sleep)
 *	Initializes the given spin lock.  The other parameters
 *	are used for debugging and statistics gathering.
 *
 * Calling/Exit State:
 *	hier is the position of this lock in the basic-lock hierarchy:
 *	if lock B is to be acquired unconditionally while holding lock A,
 *	then the hierarchy value associated with lock B should be higher
 *	than the hierarchy value associated with lock A.  All the locks
 *	held at the same time must form a strict ordering.  Driver locks
 *	should be numbered 1-32, kernel locks 33-255 (assuming 1-byte
 *	chars).
 *
 *	min_ipl is the minimum ipl value at which this lock may be
 *	held.
 *
 *	sleep is either KM_SLEEP or KM_NOSLEEP, depending on whether
 *	the caller is willing to block while acquiring memory.
 *
 *	lkinfop is a pointer to a lkinfo_t allocated and initialized
 *	by the caller.
 */
/*ARGSUSED*/
#ifdef DEBUG
void
lock_init_dbg(lock_t *lockp,		/* the lock to init */
	uchar_t hierarchy,		/* to enforce lock ordering */
	pl_t min_ipl,			/* to enforce ipl protocol */
	lkinfo_t *lkinfop,		/* associated with stat buf */
	int sleep,			/* allowed to sleep or not */
	int ksflags)			/* combination of MPSTATS, LOCKTEST */
#else
void
lock_init(lock_t *lockp,
	uchar_t hierarchy,		/* to enforce lock ordering */
	pl_t min_ipl,			/* to enforce ipl protocol */
	lkinfo_t *lkinfop,		/* associated with stat buf */
	int sleep)			/* allowed to sleep or not */
#endif /* DEBUG */
{
	ASSERT((sleep == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

	lockp->sp_lock = SP_UNLOCKED;

#ifdef DEBUG
	lockp->sp_hier = hierarchy;
	lockp->sp_lkinfop = lkinfop;
	/*
	 * Store the minipl. Since some platforms may use negative logic to
	 * represent ipls we use the CONVERT_IPL() macro to save the minipl
	 * with the appropriate platform specific processing. We would like 
	 * higher logical ipls to have higher values. This simplifies the 
	 * hierarchy checking.
	 */
	CONVERT_IPL(lockp->sp_minipl, min_ipl);
	lkinfop->lk_flags |= LK_BASIC;

	if ((ksflags & ~(KS_LOCKTEST|KS_MPSTATS|KS_NVLTTRACE)) != 0) {
		/* 
		 *+ The LOCK_INIT function was called with an invalid
		 *+ argument.  This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "invalid argument to LOCK_INIT");
		/* NOTREACHED */
	}
	lockp->sp_flags = ksflags;

	lockp->sp_lkstatp = NULL;
	if (ksflags & KS_MPSTATS)
		lockp->sp_lkstatp = lkstat_alloc(lkinfop, sleep);
#endif /* DEBUG */
}

#ifdef	DEBUG
/*
 * void
 * LOCK_DEINIT(lock_t *)
 *	De-initialize the given lock.
 *
 * Calling/Exit State:
 *	If MPSTATS is defined, frees the associated statistics buffer.
 *	The pointer from the statistics buffer to the lkinfo_t associated
 *	with the lock is retained.
 *
 * Remarks:
 *	If DEBUG is not defined, calls to this function are
 *	removed via #define in "util/ksynch.h"
 */
void
lock_deinit_dbg(lock_t *lockp)
{
	if (lockp->sp_flags & KS_LOCKTEST) {
		ASSERT(lockp->sp_lock != SP_LOCKED);
		/* "de-initing locked lock" */
	}
	/*
	 * Assert that the lock has not already been de-inited before.
	 */
	ASSERT((lockp->sp_flags & KS_DEINITED) == 0);
	if (lockp->sp_lkstatp != NULL) {
		lkstat_free((void *)lockp->sp_lkstatp, B_TRUE);
	}
	lockp->sp_flags |= KS_DEINITED;
}
#endif


/*
 * lock_t *
 * LOCK_ALLOC(uchar_t hier, pl_t, lkinfo_t *, int sleep)
 *	Allocate and initialize a lock. 
 *
 * Calling/Exit State:
 *	The 'sleep' parameter should be either KM_SLEEP or KM_NOSLEEP,
 *	depending on whether the caller is willing to sleep while memory
 *	is allocated or not.  Returns NULL if no memory can be allocated.
 */
#ifdef DEBUG
lock_t *
lock_alloc_dbg(uchar_t hierarchy,	/* to enforce lock ordering */
    pl_t min_ipl,			/* to enforce ipl protocol */
    lkinfo_t *lkinfop,			/* to associate with debug buf */
    int sleep,				/* to sleep or not */
    int ksflags)			/* combination of MPSTATS, LOCKTEST */
#else
lock_t *
lock_alloc(uchar_t hierarchy,		/* to enforce lock ordering */
    pl_t min_ipl,			/* to enforce ipl protocol */
    lkinfo_t *lkinfop,			/* to associate with debug buf */
    int sleep)				/* to sleep or not */
#endif /* DEBUG */
{
	lock_t *lockp;

	ASSERT((sleep == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

	lockp = (lock_t *)kmem_alloc(sizeof(*lockp), sleep);
	if (lockp == NULL)
		return (NULL);

#ifdef DEBUG
	lock_init_dbg(lockp, hierarchy, min_ipl, lkinfop, sleep, ksflags);
#else
	lock_init(lockp, hierarchy, min_ipl, lkinfop, sleep);
#endif /* DEBUG */
	return (lockp);
}

/*
 * void
 * LOCK_DEALLOC(lock_t *)
 *	Frees the given spin lock.
 *
 * Calling/Exit State:
 *	If MPSTATS is defined, the statistics buffer associated with
 *	the lock is also freed.  The pointer from the statistics buffer
 *	to the lkinfo_t associated with the lock is retained.
 *
 * Remarks:
 *	Two versions of this routine are compiled, one with DEBUG defined
 *	called "lock_dealloc_dbg", one without called "lock_dealloc".  The
 *	name is changed in "util/ksynch.h"
 */
void
#ifdef DEBUG
lock_dealloc_dbg(lock_t *lockp)
#else
lock_dealloc(lock_t *lockp)
#endif /* DEBUG */
{
#ifdef DEBUG
	lock_deinit_dbg(lockp);
#endif /* DEBUG */
	kmem_free((void *)lockp, sizeof(*lockp));
}

#if (defined DEBUG && _LOCKTEST && ! defined UNIPROC)
/*
 * boolean_t
 * FSPIN_OWNED(fspin_t *lockp)
 *	See whether this processor holds a specific fast spin lock.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock in question.  Returns:
 *		B_TRUE if this processor owns the lock.
 *		B_FALSE otherwise.
 *
 * Description:
 *	Uses the l.fspin member of the struct plocal.  This pointer
 *	is updated by the FSPIN_LOCK and UNLOCK code.  This works
 *	because a processor can only hold one fast spin lock at a time.
 *
 * Remarks:
 *	This routine should only be called when both DEBUG and _LOCKTEST
 *	are defined.
 */
boolean_t
FSPIN_OWNED(fspin_t *lockp)
{
	return (lockp == l.fspin);
}
#endif /* DEBUG && _LOCKTEST && !UNIPROC  */

#if (defined DEBUG && ! defined UNIPROC)
/*
 * boolean_t
 * LOCK_OWNED(lock_t *lockp)
 *	See whether this processor holds a specific lock. Only to be called
 * 	under DEBUG.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock in question.  Returns:
 *		B_TRUE if this processor owns the lock.
 *		B_FALSE otherwise.
 *
 * Remarks:
 *	We have to make sure that _LOCKTEST was defined when
 *	the routine that allocated the lock was compiled, otherwise
 *	the lock would not have been pushed on the hierarchy stack.
 *	In cases in which _LOCKTEST was not defined, we return TRUE.
 */
boolean_t
LOCK_OWNED(lock_t *lockp)
{

	if (! (lockp->sp_flags & KS_LOCKTEST))
		return (B_TRUE);
	return (hier_findlock(lockp));
}
#endif /* DEBUG && !UNIPROC */


/*
 * void
 * RW_INIT(rwlock_t *, uchar_t hierarchy, pl_t min_ipl, lkinfo_t
 *	*lkinfop, int sleep)
 *	Initializes a reader-writer spin lock.
 *
 * Calling/Exit State:
 *	hierarchy is the hierarchy position for this lock.  min_ipl is
 *	the minimum ipl required to hold this lock.  lkinfop is a pointer
 *	to the info to be associated with this lock.  sleep should be
 *	either KM_SLEEP or KM_NOSLEEP, depending on whether the caller
 *	is willing to sleep while memory is allocated or not.
 *
 * Remarks:
 *	Note that during initialization, the state interlock is not held;
 *	we assume that only one thread will try to initialize each lock.
 *
 */
/* ARGSUSED */
#ifdef DEBUG
void
rw_init_dbg(rwlock_t *lockp,
	uchar_t hierarchy,		/* for lock hierarchy */
	pl_t min_ipl,			/* to enforce ipl protocol */
	lkinfo_t *lkinfop,		/* info to associate with lock */
	int sleep,			/* to sleep or not? */
    	int ksflags)			/* combination of MPSTATS, LOCKTEST */
#else
void
rw_init(rwlock_t *lockp,
	uchar_t hierarchy,		/* for lock hierarchy */
	pl_t min_ipl,			/* to enforce ipl protocol */
	lkinfo_t *lkinfop,		/* info to associate with lock */
	int sleep)			/* to sleep or not? */
#endif /* DEBUG */
{
	ASSERT((sleep == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

	FSPIN_INIT(& lockp->rws_fspin);
	lockp->rws_lock = SP_UNLOCKED;
	lockp->rws_rdcount = 0;
	lockp->rws_state = RWS_UNLOCKED;
#ifdef DEBUG
	/*
	 * Store the minipl. Since some platforms may use negative logic to
	 * represent ipls we use the CONVERT_IPL() macro to save the minipl
	 * with the appropriate platform specific processing. We would like
	 * higher logical ipls to have higher values. This simplifies the
	 * hierarchy checking.
	 */

	CONVERT_IPL(lockp->rws_minipl, min_ipl);
	lockp->rws_hier = hierarchy;
	lockp->rws_lkinfop = lkinfop;
	lkinfop->lk_flags |= LK_BASIC;

	if ((ksflags & ~(KS_LOCKTEST|KS_MPSTATS|KS_NVLTTRACE)) != 0) {
		/* 
		 *+ The RW_INIT function was called with an invalid
		 *+ argument.  This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "invalid argument to RW_INIT");
		/* NOTREACHED */
	}
	lockp->rws_flags = ksflags;

	lockp->rws_lkstatp = NULL;
	if (ksflags & KS_MPSTATS)
		lockp->rws_lkstatp = lkstat_alloc(lkinfop, sleep);
#endif /* DEBUG */
}

/*
 * rwlock_t *
 * RW_ALLOC(uchar_t hierarchy, pl_t min_ipl, lkinfo_t *lkinfop, int sleep)
 *	Allocates and initializes a rwspin lock.
 *
 * Calling/Exit State:
 *	hierarchy is the value of this lock in the lock hierarchy.  min_ipl
 *	is the minimum ipl required to hold this lock.  lkinfop is a pointer
 *	to the info to be associated with this lock.  sleep should be either
 *	KM_SLEEP or KM_NOSLEEP, depending on whether the caller is willing
 *	to sleep while memory is allocated or not.
 *
 *	Returns the new rwspin lock, or NULL if memory for the lock cannot
 *	be allocated (when KM_NOSLEEP was given).
 *
 * Remarks:
 *	If KM_NOSLEEP is specified and memory for the lock can be allocated
 *	but memory for the statistics buffer cannot, the pointer to the lock
 *	is returned anyway.
 */
#ifdef DEBUG
rwlock_t *
rw_alloc_dbg(uchar_t hierarchy,		/* to enforce lock order protocol */
	pl_t min_ipl,			/* to enforce ipl protocol */
	lkinfo_t *lkinfop,		/* info to associate with lock */
	int sleep,			/* to sleep or not */
	int ksflags)			/* combination of MPSTATS, LOCKTEST */
#else
rwlock_t *
rw_alloc(uchar_t hierarchy,		/* to enforce lock order protocol */
	pl_t min_ipl,			/* to enforce ipl protocol */
	lkinfo_t *lkinfop,		/* info to associate with lock */
	int sleep)			/* to sleep or not */
#endif /* DEBUG */
{
	rwlock_t *lockp;

	ASSERT((sleep == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

	lockp = (rwlock_t *)kmem_alloc(sizeof(*lockp), sleep);
	if (lockp == NULL)
		return (NULL);

#ifdef DEBUG
	rw_init_dbg(lockp, hierarchy, min_ipl, lkinfop, sleep, ksflags);
#else
	rw_init(lockp, hierarchy, min_ipl, lkinfop, sleep);
#endif /* DEBUG */
	return (lockp);
}

#ifdef DEBUG 
/*
 * void
 * RW_DEINIT(rwlock_t *)
 *	De-initializes the given lock.
 *
 * Calling/Exit State:
 *	free the statistics buffer associated with
 *	the given rwspin lock.  Calls to this are #define'd
 *	away in <ksynch.h>, unless DEBUG is defined.
 */
void
rw_deinit_dbg(rwlock_t *lockp)
{
	if (lockp->rws_flags & KS_LOCKTEST) {
		ASSERT(lockp->rws_state == RWS_UNLOCKED);
		/* "deinitting in-use lock" */

	}
	/*
	 * Assert that the lock has not already been de-inited before.
	 */
	ASSERT((lockp->rws_flags & KS_DEINITED) == 0);
	if (lockp->rws_lkstatp != NULL) {
		lkstat_free(lockp->rws_lkstatp, B_TRUE);
	}
	lockp->rws_flags |= KS_DEINITED;
}
#endif 

/*
 * void
 * RW_DEALLOC(rwlock_t *lockp)
 *	Free an rwspin lock.  
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be freed.  Returns: none.
 *
 * Remarks:
 * 	Two versions of this routine are compiled, one with DEBUG defined
 *	called "rw_dealloc_dbg", and one without called "rw_dealloc".  The
 *	name is changed in <ksynch.h>
 */
void
#ifdef DEBUG
rw_dealloc_dbg(rwlock_t *lockp)
#else
rw_dealloc(rwlock_t *lockp)
#endif /* DEBUG */
{
#ifdef DEBUG
	rw_deinit_dbg(lockp);
#endif
	kmem_free(lockp, sizeof (rwlock_t));
}

#if (defined DEBUG && ! defined UNIPROC)
/*
 * boolean_t
 * RW_OWNED(rwlock_t *lockp)
 *	See whether this processor holds a specific lock.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock in question.  Returns:
 *		B_TRUE if this processor owns the lock.
 *		B_FALSE otherwise.
 *
 * Remarks:
 *	Only to be called when DEBUG is defined.  If LOCKTEST is
 *	not defined for this lock, we return B_TRUE with the
 *	assumption that callers are interested in asserting that
 *	they *do* hold a lock, never that they *don't*.
 */
boolean_t
RW_OWNED(rwlock_t *lockp)
{
	if (! (lockp->rws_flags & KS_LOCKTEST)) {
		return B_TRUE;
	}
	return (hier_findlock(lockp));
}
#endif /* DEBUG && !UNIPROC */

#ifndef DEBUG

/*
 * atomic_int_t *
 * ATOMIC_INT_ALLOC(int sleep)
 *	Allocates and initializes an atomic_int_t.
 *
 * Calling/Exit State:
 *	sleep should be either KM_SLEEP or KM_NOSLEEP, depending on whether
 *	the caller is willing to sleep while memory is allocated or not.
 *
 *	Returns the new atomic int structure, or NULL if memory for the
 *	atomic int cannot be allocated (when KM_NOSLEEP was given.
 */
atomic_int_t *
ATOMIC_INT_ALLOC(int sleep)
{
	return kmem_alloc(sizeof(atomic_int_t), sleep);
}

/*
 * void
 * ATOMIC_INT_DEALLOC(atomic_int_t *atomic_intp)
 *	Deallocates an atomic_int_t allocated by ATOMIC_INT_ALLOC.
 *
 * Calling/Exit State:
 *	atomic_intp is a pointer to the atomic_int_t to be freed.
 */
void
ATOMIC_INT_DEALLOC(atomic_int_t *atomic_intp)
{
	kmem_free(atomic_intp, sizeof(atomic_int_t));
}

/*
 * boolean_t
 * slpdeque(list_t *listp, lwp_t * lwpp)
 *	Remove an lwp from a NULL-terminated list.
 *
 * Calling/Exit State:
 *	listp is the sleep queue, lwpp is the lwp to remove.  Returns:
 *		B_TRUE if the lwp was dequeued.
 *		B_FALSE if the lwp was not on the queue.
 *
 *	Expects both the l_mutex and the synchronization object state
 *	lock to be held.
 *
 * Remarks:
 *	We only need one copy of this routine, since it has nothing to
 *	do with the size of the synch object.  So we only compile when
 *	DEBUG is not defined.
 */
boolean_t
slpdeque(list_t *listp, struct lwp *lwpp)
{
	list_t *lp;

	lp = listp->flink;
	while (lp != (list_t *)lwpp && lp != listp)
		lp = lp->flink;
	if (lp == listp) {
		/* the lwp was not on the list */
		return (B_FALSE);
	}
	remque(lp);
	return (B_TRUE);
}

#endif /* !DEBUG */

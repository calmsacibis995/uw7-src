#ident	"@(#)kern-i386:util/locks_dbg.c	1.3.1.1"
#ident	"$Header$"

#ifndef	UNIPROC

#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/locktest.h>
#include <util/param.h>
#include <util/types.h>
#include <util/nuc_tools/trace/nwctrace.h>

#define NVLT_TRACE_LOCK(lockp, type, retpc)				\
		if ((lockp)->sp_flags & KS_NVLTTRACE) {			\
			NVLT_TRACE((type),				\
				(uint_t)(lockp),			\
				(uint_t)(retpc),			\
				(uint_t)(lockp)->sp_lkinfop->lk_name,	\
				(uint_t)(lockp)->sp_value);		\
		}

#ifdef _MPSTATS
int spinlk_count;
#endif

extern pl_t trylock_nodbg(lock_t *, pl_t);
extern void unlock_nodbg(lock_t *, pl_t);
extern void xcall_intr(void);

/* 
 * pl_t
 * lock_dbgC(lock_t *lockp, pl_t newpl, boolean_t same_hier, void *retpc)
 * 	Acquire a lock in debug mode.
 *
 * Calling/Exit State:
 *	lockp is the lock to be acquired.  newpl is the desired ipl, which
 *	must be greater than or equal to the current ipl.  Upon return,
 *	the lock is held at the given ipl.
 *	Returns:  the previous ipl.
 */
/*ARGSUSED3*/
pl_t
lock_dbgC(lock_t *lockp, pl_t newpl, boolean_t same_hier, void *retpc)
{
	boolean_t block = B_FALSE;
	dl_t wtime;
	pl_t oldpl;

	if (lockp->sp_flags & KS_LOCKTEST)
		locktest(lockp, lockp->sp_minipl, newpl, SP_LOCKTYPE,
			lockp->sp_lkinfop);
	while ((oldpl = trylock_nodbg(lockp, newpl)) == INVPL) {
		if (lockp->sp_lkstatp != NULL) {
			++lockp->sp_lkstatp->lks_fail;
			if (!block) {
				TIME_INIT(&wtime);
				block = B_TRUE;
			}
		}
		NVLT_TRACE_LOCK(lockp, NVLTT_spinLockWait, retpc);
	}
	if (lockp->sp_flags & KS_LOCKTEST)
		(same_hier ? hier_push_same : hier_push)(lockp,
			lockp->sp_value, lockp->sp_lkinfop);
	if (lockp->sp_lkstatp != NULL) {
		if (block)
			TIME_UPDATE(&lockp->sp_lkstatp->lks_wtime, wtime);
		TIME_INIT(&lockp->sp_lkstatp->lks_stime);
	}
#ifdef	_MPSTATS
	++spinlk_count;
	begin_lkprocess(lockp, retpc);
#endif
	NVLT_TRACE_LOCK(lockp, NVLTT_spinLockGet, retpc);
	return oldpl;
}

/* 
 * pl_t
 * trylock_dbgC(lock_t *lockp, pl_t newpl, void *retpc)
 *	Attempts to lock the given lock at the given ipl in debug mode.
 *	If at first it does not succeed, gives up.
 *
 * Calling/Exit State:
 *	lockp is the lock to attempt to lock, newpl is the interrupt level
 *	at which the acquisition should be attempted.  Returns the old
 *	ipl if the lock is acquired, INVPL otherwise.
 */
/*ARGSUSED2*/
pl_t
trylock_dbgC(lock_t *lockp, pl_t newpl, void *retpc)
{
	pl_t oldpl;

	if (lockp->sp_flags & KS_LOCKTEST)
		locktest(lockp, lockp->sp_minipl, newpl, SP_LOCKTYPE,
			lockp->sp_lkinfop);
	oldpl = trylock_nodbg(lockp, newpl);
	if (oldpl != INVPL) {
#ifdef	_MPSTATS
		++spinlk_count;
		begin_lkprocess(lockp, retpc);
#endif
		NVLT_TRACE_LOCK(lockp, NVLTT_spinTrylockGet, retpc);
		if (lockp->sp_lkstatp != NULL) {
			++lockp->sp_lkstatp->lks_wrcnt;
			TIME_INIT(&lockp->sp_lkstatp->lks_stime);
		}
		if (lockp->sp_flags & KS_LOCKTEST) {
			hier_push_nchk(lockp, lockp->sp_value,
				lockp->sp_lkinfop);
		}
	} else {
		NVLT_TRACE_LOCK(lockp, NVLTT_spinTrylockFail, retpc);
	}
	return oldpl;
}

/* 
 * void
 * unlock_dbgC(lock_t *lockp, pl_t newpl, void *retpc)
 * 	Release a lock in debug mode.
 *
 * Calling/Exit State:
 *	lockp is the lock to unlock, newpl is the ipl level to return at.
 *	Returns:  None.
 */	
/*ARGSUSED2*/
void
unlock_dbgC(lock_t *lockp, pl_t newpl, void *retpc)
{

	if (lockp->sp_lkstatp != NULL)
		TIME_UPDATE(&lockp->sp_lkstatp->lks_htime,
			lockp->sp_lkstatp->lks_stime)
	if (lockp->sp_flags & KS_LOCKTEST) {
		if (lockp->sp_lock == SP_UNLOCKED)
			/*
		 	*+A processor attempted to unlock a lock
		 	*+which was already unlocked.  This indicates
		 	*+a kernel software problem.
		 	*/
			cmn_err(CE_PANIC, lk_panicstr[3]);
		hier_remove(lockp, lockp->sp_lkinfop, newpl);
	}
#ifdef	_MPSTATS
	end_lkprocess(lockp, retpc);
#endif
	NVLT_TRACE_LOCK(lockp, NVLTT_spinLockFree, retpc);
	unlock_nodbg(lockp, newpl);
}

#endif

#ident	"@(#)kern-i386:util/rwlocks_dbg.c	1.1.1.1"
#ident	"$Header$"

#ifndef	UNIPROC

#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/locktest.h>
#include <util/param.h>
#ifdef DEBUG_TRACE
#include <util/nuc_tools/trace/nwctrace.h>
#endif /* DEBUG_TRACE */
#include <util/types.h>

#ifdef	_MPSTATS
int rwspinlk_count;
#endif


extern pl_t rw_rdlock(rwlock_t *, pl_t);
extern pl_t rw_wrlock(rwlock_t *, pl_t);
extern pl_t rw_tryrdlock(rwlock_t *, pl_t);
extern pl_t rw_trywrlock(rwlock_t *, pl_t);
extern void rw_unlock(rwlock_t *, pl_t);

/* 
 * pl_t
 * rw_rdlock_dbgC(rwlock_t *lockp, pl_t newpl, boolean_t same_hier, void *retpc)
 * 	Acquire a read-write spin lock in read mode in debug mode.
 *
 * Calling/Exit State:
 *	lockp is the read-write spin lock to be acquired.  newpl is the
 *	desired ipl, which must be greater than or equal to the current
 *	ipl.  Upon return, the lock is held at the given ipl.
 *
 *	Returns:  the previous ipl.
 */
/*ARGSUSED3*/
pl_t
rw_rdlock_dbgC(rwlock_t *lockp, pl_t newpl, boolean_t same_hier, void *retpc)
{
	dl_t wtime;
	pl_t oldpl;
#ifdef	DEBUG
	int state;
#endif

	if (lockp->rws_flags & KS_LOCKTEST)
		locktest(lockp, lockp->rws_minipl, newpl, RWS_LOCKTYPE,
			lockp->rws_lkinfop);
	if ((oldpl = rw_tryrdlock(lockp, newpl)) == INVPL) {
		if (lockp->rws_lkstatp != NULL) {
			++lockp->rws_lkstatp->lks_fail;
			TIME_INIT(&wtime);
		}
#ifdef	DEBUG_TRACE
		if (lockp->rws_flags & KV_NVLTTRACE)
			NVLTtrace(NVLTT_rwLockReadWait, lockp, retpc,
				lockp->rws_infop->lk_name, lockp->rws_hier);
#endif
		oldpl = rw_rdlock(lockp, newpl);
		ASSERT(lockp->rws_lock == SP_LOCKED);
		ASSERT(((state = lockp->rws_state) == RWS_READ) ||
			(state == RWS_UNLOCKED));
		if (lockp->rws_lkstatp != NULL)
			TIME_UPDATE(&lockp->rws_lkstatp->lks_wtime, wtime);
	}
	if (lockp->rws_lkstatp != NULL) {
		++lockp->rws_lkstatp->lks_rdcnt;
		if (lockp->rws_rdcount == 0) {
			++lockp->rws_lkstatp->lks_solordcnt;
			TIME_INIT(&lockp->rws_lkstatp->lks_stime);
		}
	}
	if (lockp->rws_flags & KS_LOCKTEST)
		(same_hier ? hier_push_same : hier_push)(lockp,
			lockp->rws_value, lockp->rws_lkinfop);
#ifdef	_MPSTATS
	++rwspinlk_count;
	begin_lkprocess(lockp, retpc);
#endif
	ASSERT(lockp->rws_lock == SP_LOCKED);
	ASSERT(((state = lockp->rws_state) == RWS_READ) ||
			(state == RWS_UNLOCKED));
#ifdef	DEBUG_TRACE
	if (lockp->rws_flags & KS_NVLTTRACE)
		NVLTtrace(NVLTT_rwLockReadGet, lockp, retpc,
			lockp->rws_infop->lk_name, lockp->rws_hier);
#endif
	return oldpl;
}


/* 
 * pl_t
 * rw_wrlock_dbgC(rwlock_t *lockp, pl_t newpl, boolean_t same_hier, void *retpc)
 * 	Acquire a read-write spin lock in write mode in debug mode.
 *
 * Calling/Exit State:
 *	lockp is the read-write spin lock to be acquired.  newpl is the
 *	desired ipl, which must be greater than or equal to the current
 *	ipl.  Upon return, the lock is held at the given ipl.
 *
 *	Returns:  the previous ipl.
 */
/*ARGSUSED3*/
pl_t
rw_wrlock_dbgC(rwlock_t *lockp, pl_t newpl, boolean_t same_hier, void *retpc)
{
	dl_t wtime;
	pl_t oldpl;

	if (lockp->rws_flags & KS_LOCKTEST)
		locktest(lockp, lockp->rws_minipl, newpl, RWS_LOCKTYPE,
			lockp->rws_lkinfop);
	if ((oldpl = rw_trywrlock(lockp, newpl)) == INVPL) {
		if (lockp->rws_lkstatp != NULL) {
			++lockp->rws_lkstatp->lks_fail;
			TIME_INIT(&wtime);
		}
#ifdef	DEBUG_TRACE
		if (lockp->rws_flags & KV_NVLTTRACE)
			NVLTtrace(NVLTT_rwLockWriteWait, lockp, retpc,
				lockp->rws_infop->lk_name, lockp->rws_hier);
#endif
		oldpl = rw_wrlock(lockp, newpl);
		ASSERT((lockp->rws_lock == SP_LOCKED) &&
			(lockp->rws_state == RWS_WRITE));
		if (lockp->rws_lkstatp != NULL)
			TIME_UPDATE(&lockp->rws_lkstatp->lks_wtime, wtime);
	}
	if (lockp->rws_lkstatp != NULL) {
		++lockp->rws_lkstatp->lks_wrcnt;
		TIME_INIT(&lockp->rws_lkstatp->lks_stime);
	}
	if (lockp->rws_flags & KS_LOCKTEST)
		(same_hier ? hier_push_same : hier_push)(lockp,
			lockp->rws_value, lockp->rws_lkinfop);
#ifdef	_MPSTATS
	++rwspinlk_count;
	begin_lkprocess(lockp, retpc);
#endif
	ASSERT((lockp->rws_lock == SP_LOCKED) &&
		(lockp->rws_state == RWS_WRITE));
#ifdef	DEBUG_TRACE
	if (lockp->rws_flags & KS_NVLTTRACE)
		NVLTtrace(NVLTT_rwLockWriteGet, lockp, retpc,
			lockp->rws_infop->lk_name, lockp->rws_hier);
#endif
	return oldpl;
}

/* 
 * pl_t
 * rw_tryrdlock_dbgC(rwlock_t *lockp, pl_t newpl, void *retpc)
 *	Attempts to acquire the given read-write spin lock in read mode
 *	at the given ipl.  If at first we do not succeed, give up.
 *
 * Calling/Exit State:
 *	lockp is the read-write spin lock to attempt to lock, newpl
 *	is the interrupt level at which the acquisition should be
 *	attempted.  Returns the old ipl if the lock is acquired, INVPL
 *	otherwise.
 */
/*ARGSUSED2*/
pl_t
rw_tryrdlock_dbgC(rwlock_t *lockp, pl_t newpl, void *retpc)
{
	pl_t oldpl;
#ifdef	DEBUG
	int state;
#endif

	if (lockp->rws_flags & KS_LOCKTEST)
		locktest(lockp, lockp->rws_minipl, newpl, RWS_LOCKTYPE,
			lockp->rws_lkinfop);
	oldpl = rw_tryrdlock(lockp, newpl);
	if (oldpl != INVPL) {
		if (lockp->rws_lkstatp != NULL) {
			++lockp->rws_lkstatp->lks_rdcnt;
			if (lockp->rws_rdcount == 0) {
				++lockp->rws_lkstatp->lks_solordcnt;
				TIME_INIT(&lockp->rws_lkstatp->lks_stime);
			}
		}
		if (lockp->rws_flags & KS_LOCKTEST)
			hier_push_nchk(lockp, lockp->rws_value,
				lockp->rws_lkinfop);
#ifdef	_MPSTATS
		++rwspinlk_count;
		begin_lkprocess(lockp, retpc);
#endif
	}
#ifdef	DEBUG_TRACE
	if (lockp->rws_flags & KS_NVLTTRACE)
		NVLTtrace((oldpl == INVPL) ? NVLTT_rwTryReadFail :
			NVLTT_rwTryReadGet, lockp, retpc,
			lockp->rws_infop->lk_name, lockp->rws_hier);
#endif
	ASSERT((oldpl == INVPL) || ((lockp->rws_lock == SP_LOCKED) &&
		(((state = lockp->rws_state) == RWS_READ) ||
		(state == RWS_UNLOCKED))));
	return oldpl;
}

/* 
 * pl_t
 * rw_trywrlock_dbgC(rwlock_t *lockp, pl_t newpl, void *retpc)
 *	Attempts to acquire the given read-write spin lock in write mode
 *	at the given ipl.  If at first we do not succeed, give up.
 *
 * Calling/Exit State:
 *	lockp is the read-write spin lock to attempt to lock, newpl
 *	is the interrupt level at which the acquisition should be
 *	attempted.  Returns the old ipl if the lock is acquired, INVPL
 *	otherwise.
 */
/*ARGSUSED2*/
pl_t
rw_trywrlock_dbgC(rwlock_t *lockp, pl_t newpl, void *retpc)
{
	pl_t oldpl;

	if (lockp->rws_flags & KS_LOCKTEST)
		locktest(lockp, lockp->rws_minipl, newpl, RWS_LOCKTYPE,
			lockp->rws_lkinfop);
	oldpl = rw_trywrlock(lockp, newpl);
	if (oldpl != INVPL) {
		if (lockp->rws_lkstatp != NULL) {
			++lockp->rws_lkstatp->lks_wrcnt;
			TIME_INIT(&lockp->rws_lkstatp->lks_stime);
		}
		if (lockp->rws_flags & KS_LOCKTEST)
			hier_push_nchk(lockp, lockp->rws_value,
				lockp->rws_lkinfop);
#ifdef	_MPSTATS
		++rwspinlk_count;
		begin_lkprocess(lockp, retpc);
#endif
	}
	ASSERT((oldpl == INVPL) || ((lockp->rws_lock == SP_LOCKED) &&
		(lockp->rws_state == RWS_WRITE)));
#ifdef	DEBUG_TRACE
	if (lockp->rws_flags & KS_NVLTTRACE)
		NVLTtrace((oldpl == INVPL) ? NVLTT_rwTryWriteFail :
			NVLTT_rwTryWriteGet, lockp, retpc,
			lockp->rws_infop->lk_name, lockp->rws_hier);
#endif
	return oldpl;
}


/* 
 * void
 * rw_unlock_dbgC(rwlock_t *lockp, pl_t newpl, void *retpc)
 * 	Release a read_write spin lock in debug mode.
 *
 * Calling/Exit State:
 *	lockp is the read-write spin lock to unlock, newpl is the ipl
 *	level to return at.
 *	Returns:  None.
 */	
/*ARGSUSED2*/
void
rw_unlock_dbgC(rwlock_t *lockp, pl_t newpl, void *retpc)
{

	if (lockp->rws_lkstatp != NULL) {
		if (lockp->rws_rdcount == 0)
			TIME_UPDATE(&lockp->rws_lkstatp->lks_htime,
				lockp->rws_lkstatp->lks_stime)
	}
	if (lockp->rws_flags & KS_LOCKTEST) {
		if (lockp->rws_lock == SP_UNLOCKED)
			cmn_err(CE_PANIC, lk_panicstr[3]);
		hier_remove(lockp, lockp->rws_lkinfop, newpl);
	}
#ifdef	DEBUG_TRACE
	if (lockp->rws_flags & KS_NVLTTRACE)
		NVLTtrace(NVLTT_rwLockFree, lockp, retpc,
			lockp->rws_infop->lk_name, lockp->rws_hier);
#endif
#ifdef	_MPSTATS
	end_lkprocess(lockp, retpc);
#endif
	rw_unlock(lockp, newpl);
}

#endif	/* UNIPROC */

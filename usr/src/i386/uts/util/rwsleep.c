#ident	"@(#)kern-i386:util/rwsleep.c	1.7.2.1"
#ident	"$Header$"

/*
 * Reader/writer sleep locks and everything to do with them. 
 */

#include <svc/cpu.h>
#include <util/assym_c.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/list.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/class.h>
#include <proc/user.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <util/plocal.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <mem/kmem.h>

#include <util/inline.h>

#ifdef	UNIPROC

asm boolean_t
rwsleep_take(rwsleep_t *lockp)
{
%reg lockp;
	btrl	$0, _A_RW_AVAIL(lockp)
	setc	%al
	andl	$1, %eax	
%mem lockp;
	movl	lockp, %ecx
	btrl	$0, _A_RW_AVAIL(%ecx)
	setc	%al
	andl	$1, %eax	
}

#else

asm boolean_t
rwsleep_take(rwsleep_t *lockp)
{
%ureg lockp;
	xorl	%eax, %eax
	xchgb	%al, _A_RW_AVAIL(lockp)
%mem lockp;
	movl	lockp, %ecx
	xorl	%eax, %eax
	xchgb	%al, _A_RW_AVAIL(%ecx)
}

#endif

#pragma asm partial_optimization rwsleep_take

/*
 * If _MPSTATS is defined, we keep a running total of the
 * number of times rw sleep locks in the system are acquired. We are
 * not interested in exact numbers here and so, we bump this count
 * without holding any locks. Further, we do not distinguish between 
 * readers and writers.
 */

ulong_t	rwsleeplk_count;

#define	LWP_WAKEUP(lwpp) { \
	(lwpp)->l_stat = SRUN; \
	(lwpp)->l_syncp = NULL; \
	(lwpp)->l_flag &= ~L_SIGWOKE; \
	(lwpp)->l_stype = ST_NONE; \
	CL_WAKEUP((lwpp), (lwpp)->l_cllwpp, 0); \
}

#ifdef	DEBUG
void print_rwlock(rwsleep_t *lockp);
STATIC boolean_t rw_is_sane(rwsleep_t *lockp);
#endif

/* 
 * void
 * rwsleep_init(rwsleep_t *lockp, uchar_t  hierarchy, lkinfo_t *lkinfop,
 *	int km_flags, int ksflags)
 *	Initializes an instance of a reader/writer sleep lock. 
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to init.  Hierarchy is the position
 *	of this lock in the sleep lock hierarchy, but may safely be 0, since
 *	the sleep lock hierarchy isn't checked.  lkinfop is a pointer to
 *	a lkinfo_t which the caller has allocated and initialized.  km_flags
 *	is KM_SLEEP or KM_NOSLEEP, depending on whether the caller is willing
 *	to block or not. Code defines the compilation options that were in 
 *	effect in the caller.
 *
 *	Returns:  none.
 */
/*ARGSUSED*/
void 
rwsleep_init
	(rwsleep_t *lockp,		/* the lock to init */
	uchar_t hierarchy,		/* to enforce lock ordering */
	lkinfo_t *lkinfop,		/* info about the lock */
	int km_flags,			/* indicates if the caller can sleep */
	int ksflags)			/* encodes compile-time options */
{
	ASSERT((km_flags == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

        /*
	 * the sleep queue is a doubly-linked list.  Set the forward pointer
         * of the last element and the backward pointer of the first element
         * to the head pointer of the synch object.
         */
	INITQUE(&lockp->rw_list);
	lockp->rw_avail = 1;
	lockp->rw_mode = RWS_UNLOCKED;
	FSPIN_INIT(&lockp->rw_fspin);
	lockp->rw_hierarchy = hierarchy;
	lockp->rw_pend = 0;	/* no pending LWPs */
	lockp->rw_read = 0;	/* no readers */
	lkinfop->lk_flags |= LK_SLEEP;

#ifdef DEBUG
	if ((ksflags & ~(KS_LOCKTEST|KS_MPSTATS|KS_NVLTTRACE)) != 0) {
		/* 
		 *+ The RWSLEEP_INIT function was called with an invalid
		 *+ argument.  This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "invalid argument to RWSLEEP_INIT");
		/* NOTREACHED */
	}
#endif
	lockp->rw_flags = ksflags;

	lockp->rw_lkstatp = NULL;
        if ((ksflags & KS_MPSTATS) && !(lkinfop->lk_flags & LK_NOSTATS)) {
		/*
		 * Allocate a stats structure since _MPSTATS is on and the
		 * lkinfo structure does not have LK_NOSTATS set.
		 * lkstat_alloc() allocates and initializes a
		 * statistics structure.
		 */
		lockp->rw_lkstatp = lkstat_alloc(lkinfop, km_flags);
        }
}

/*
 * rwsleep_t *
 * rwsleep_alloc(uchar_t hierarchy, lkinfo_t *lkinfop, int km_flags,
		int ksflags)
 *	Allocates and initializes an instance of a reade/writer
 * 	sleep lock. 
 *
 * Calling/Exit State:
 *	hierarchy is the position of this lock in the sleep lock hierarchy.
 *	This may be 0, since the sleep lock hierarchy is currently
 *	unimplemented.  lkinfop is a pointer to a lkinfo_t which has
 *	been allocated an initialized by the caller.  km_flags is either
 *	KM_SLEEP or KM_NOSLEEP, depending on whether the caller is willing
 *	to block or not. Code defines the compilation options that were in 
 *	effect in the caller.
 */
rwsleep_t *
rwsleep_alloc
	(uchar_t hierarchy,		/* to enforce lock ordering */
	lkinfo_t *lkinfop,		/* info about the lock */
	int km_flags,			/* indicates if the caller can sleep */
	int ksflags)			/* encodes compile-time options */
{
	rwsleep_t *lockp;

	ASSERT((km_flags == KM_NOSLEEP) || (KS_HOLD0LOCKS()));
        /* allocate the lock */
        lockp = (rwsleep_t *)kmem_alloc(sizeof(*lockp), km_flags);
	if (lockp == NULL)
		return  (NULL);

        /* initialize the lock */
        rwsleep_init(lockp, hierarchy, lkinfop, km_flags, ksflags);
        return (lockp);
}

/*
 * void
 * rwsleep_deinit(rwsleep_t *lockp)
 *	De-initialize a reader/writer sleep lock.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to deinit.  Returns:  none.
 *
 */
void 
rwsleep_deinit(rwsleep_t *lockp)
{
	ASSERT(rw_is_sane(lockp));
	if ((lockp->rw_flags & KS_LOCKTEST) && 
			(FSPIN_IS_LOCKED(&lockp->rw_fspin)
			|| !EMPTYQUE(&lockp->rw_list)
			|| (lockp->rw_pend > 0) || !lockp->rw_avail
			|| (lockp->rw_mode == RWS_BUSY))) {
		/*
		 *+ A reader/writer sleep lock was de-initialized
		 *+ while in use.  This indicates a kernel 
		 *+ software problem.
		 */
		cmn_err(CE_PANIC, "rwsleep lock deinited: not idle");
		/*NOTREACHED*/
	}

	ASSERT(lockp->rw_read == 0);
	if (lockp->rw_lkstatp != NULL)
		lkstat_free(lockp->rw_lkstatp, B_TRUE);
}

/*
 * void
 * rwsleep_dealloc(rwsleep_t *lockp)
 *	Deallocate an instance of the read/write sleep lock.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to deallocate.  Returns:  none.
 */
void 
rwsleep_dealloc(rwsleep_t *lockp)
{
	/* first de-initialize. */
	RWSLEEP_DEINIT(lockp);

	/* free the object itself */
	kmem_free(lockp, sizeof(*lockp));
}

/*
 * void
 * rwsleep_rdlock(rwsleep_t *lockp, int priority)
 *	Acquire a lock in read mode.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.  priority is a hint 
 *	to the class-specific code concerning our sleep/dispatch priority.  
 *	Returns:  None.
 *
 *	Returns at PL0.  
 *
 * Remarks:
 *	This primitive is not signalable.
 */
void 
rwsleep_rdlock(rwsleep_t *lockp, int priority)
{
	int nsleeps = 0;
	lwp_t *lwpp = u.u_lwpp;
	dl_t wstart, wtime;		/* start and total of wait time */
#ifdef	DEBUG
	int mode;
#endif

#ifdef _MPSTATS 
	rwsleeplk_count++;
#endif	/* _MPSTATS */

	ASSERT(KS_HOLD0LOCKS());

	ASSERT(rw_is_sane(lockp));

	if ((lockp->rw_lkstatp == NULL) && lockp->rw_avail &&
			rwsleep_take(lockp)) {
		while (lockp->rw_mode == RWS_BUSY)
			;
		lockp->rw_mode = RWS_READ;
		ASSERT(!lockp->rw_avail);
		ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
			(mode == RWS_UNLOCKED));
		return;
	}
	while (!lockp->rw_avail || !rwsleep_take(lockp)) {

		/*
		 * If the lock is already in read mode, increment the
		 * read count under cover of the FSPIN lock.  There
		 * is some interaction between this code and the order
		 * in which rwsleep_unlock does things; see comments in
		 * rwsleep_unlock below.
		 */
		if (lockp->rw_mode == RWS_READ) {
			FSPIN_LOCK(&lockp->rw_fspin);
			++lockp->rw_read;
			ASSERT(lockp->rw_read > 0);
			/*
			 * Make sure it's still held in read mode
			 */
			if (!lockp->rw_avail && (lockp->rw_mode == RWS_READ)) {
				lockp->rw_pend -= nsleeps;
				if (lockp->rw_lkstatp != NULL) {
					++lockp->rw_lkstatp->lks_rdcnt;
					if (nsleeps > 0) {
						lockp->rw_lkstatp->lks_fail +=
							nsleeps;
						/* LINTED used before set */
						lockp->rw_lkstatp->lks_wtime = 
							ladd(lockp->rw_lkstatp->lks_wtime,
								wtime);
					}
				}
				FSPIN_UNLOCK(&lockp->rw_fspin);
				ASSERT(!lockp->rw_avail);
				ASSERT(((mode = lockp->rw_mode) == RWS_READ)
					|| (mode == RWS_UNLOCKED));
				return;
			}
			--lockp->rw_read;
			ASSERT(lockp->rw_read >= 0);
			FSPIN_UNLOCK(&lockp->rw_fspin);
		}

		/*
		 * Barring races, we will put the caller to sleep.
		 */
		(void)LOCK(&lwpp->l_mutex, PLHI);
		FSPIN_LOCK(&lockp->rw_fspin);

		/*
		 * Check to see if the LWP raced with an unlock before
		 * actually putting it to sleep
		 */
		if (lockp->rw_avail && rwsleep_take(lockp)) {
			while (lockp->rw_mode == RWS_BUSY)
				;
			lockp->rw_mode = RWS_READ;
			lockp->rw_pend -= nsleeps;
			if (lockp->rw_lkstatp != NULL) {
				if (nsleeps > 0) {
					lockp->rw_lkstatp->lks_fail +=
						nsleeps;
					lockp->rw_lkstatp->lks_wtime = 
					ladd(lockp->rw_lkstatp->lks_wtime,
							wtime);
				}
				++lockp->rw_lkstatp->lks_rdcnt;
				++lockp->rw_lkstatp->lks_solordcnt;
				TIME_INIT(&lockp->rw_lkstatp->lks_stime);
			}
			FSPIN_UNLOCK(&lockp->rw_fspin);
			UNLOCK(&lwpp->l_mutex, PLBASE);
			ASSERT(!lockp->rw_avail);
			ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
				(mode == RWS_UNLOCKED));
			return;
		}

		/*
		 * If the lock is already in read mode, increment the
		 * read count under cover of the FSPIN lock (which
		 * is already held here), release the locks and return.
		 */
		if (lockp->rw_mode == RWS_READ) {
			++lockp->rw_read;
			ASSERT(lockp->rw_read > 0);
			/*
			 * Make sure it's still held in read mode
			 */
			if (!lockp->rw_avail && (lockp->rw_mode == RWS_READ)) {
				lockp->rw_pend -= nsleeps;
				if (lockp->rw_lkstatp != NULL) {
					++lockp->rw_lkstatp->lks_rdcnt;
					if (nsleeps > 0) {
						lockp->rw_lkstatp->lks_fail +=
							nsleeps;
						lockp->rw_lkstatp->lks_wtime = 
							ladd(lockp->rw_lkstatp->lks_wtime,
								wtime);
					}
				}
				FSPIN_UNLOCK(&lockp->rw_fspin);
				UNLOCK(&lwpp->l_mutex, PLBASE);
				ASSERT(!lockp->rw_avail);
				ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
					(mode == RWS_UNLOCKED));
				return;
			}
			/*
			 * Another race: lock was dropped, so decrement read
			 * count
			 */
			--lockp->rw_read;
			ASSERT(lockp->rw_read >= 0);
		}

		/* Lock is not available, put the caller to sleep. */

		if (lockp->rw_lkstatp != NULL) {
			/*
			 * initialize wtime if first sleep, and record
			 *	start of wait time
			 */
			if (nsleeps == 0)
				wtime.dl_hop = wtime.dl_lop = 0;
			TIME_INIT(&wstart);
		}
		if (nsleeps == 0) {
			/*
			 * Give control to the class specific code
			 *	to enqueue the lwp
			 */
			CL_INSQUE(lwpp, &lockp->rw_list, priority);
		} else {
			insque(lwpp, lockp->rw_head);
		}
		FSPIN_UNLOCK(&lockp->rw_fspin);

		/* Update the the lwp state */
		lwpp->l_stype = ST_RDLOCK;	/* blocked on a r/w
							 * lock for read */
		lwpp->l_flag |= L_NWAKE;/* don't wake us */
		lwpp->l_slptime = 0;	/* init start time for sleep */
		lwpp->l_stat = SSLEEP;	/* sleeping... */
		lwpp->l_syncp =  lockp;	/* waiting for this lock */

		/*
		 * Give up processor. The swtch() code will release
		 * the lwp state lock at PL0 when it is safe
		 */
		++nsleeps;
		swtch(lwpp);
		if (lockp->rw_lkstatp != NULL) {
			/*
			 * Update the cumulative wait time from the
			 * LWP wakeup time and the begin time. 
			 */
			wtime = ladd(wtime, lsub(lwpp->l_waket, wstart));
		}
	}
	while (lockp->rw_mode == RWS_BUSY)
		;
	lockp->rw_mode = RWS_READ;
stats:
	if ((nsleeps > 0) || (lockp->rw_lkstatp != NULL)) {
		FSPIN_LOCK(&lockp->rw_fspin);
		lockp->rw_pend -= nsleeps;
		if (lockp->rw_lkstatp != NULL) {
			if (nsleeps > 0) {
				lockp->rw_lkstatp->lks_fail += nsleeps;
				lockp->rw_lkstatp->lks_wtime = 
					ladd(lockp->rw_lkstatp->lks_wtime,
						wtime);
			}
			++lockp->rw_lkstatp->lks_rdcnt;
			++lockp->rw_lkstatp->lks_solordcnt;
			TIME_INIT(&lockp->rw_lkstatp->lks_stime);
		}
		FSPIN_UNLOCK(&lockp->rw_fspin);
	}
	ASSERT(!lockp->rw_avail);
	ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
			(mode == RWS_UNLOCKED));
}


/*
 * void
 * rwsleep_rdlock_rellock(rwsleep_t *lockp, int priority, lock_t *lkp)
 *	Acquire a lock in read mode. The entry spin lock is atomically
 *	dropped.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.  priority is
 *	a hint to the class-specific code concerning our sleep/dispatch
 *	priority.  lkp is a pointer to a spin lock which is atomically
 *	dropped at the right time.  Returns:  None.
 *
 *	Returns at PL0.  
 *
 * Remarks:
 *	WARNING!
 *	This primitive is not signalable. If this primitive is included in the
 *	DDI/DKI, we need to pass in an extra argument that encodes the 
 *	compilation options that were in effect in the caller. Refer to 
 *	util/sv.c for details.
 */
void 
rwsleep_rdlock_rellock(rwsleep_t *lockp, int priority, lock_t *lkp)
{
	int nsleeps = 0;
	lwp_t *lwpp = u.u_lwpp;
	dl_t wstart, wtime;		/* start and total of wait time */
#ifdef	DEBUG
	int mode;
#endif

#ifdef _MPSTATS 
	rwsleeplk_count++;
#endif	/* _MPSTATS */

	ASSERT(LOCK_OWNED(lkp));
	ASSERT(KS_HOLD1LOCK());

	ASSERT(rw_is_sane(lockp));

	if ((lockp->rw_lkstatp == NULL) && lockp->rw_avail &&
			rwsleep_take(lockp)) {
		while (lockp->rw_mode == RWS_BUSY)
			;
		lockp->rw_mode = RWS_READ;
		UNLOCK(lkp, PLBASE);
		ASSERT(!lockp->rw_avail);
		ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
			(mode == RWS_UNLOCKED));
		return;
	}

	while (!lockp->rw_avail || !rwsleep_take(lockp)) {

		/*
		 * If the lock is already in read mode, increment the
		 * read count under cover of the FSPIN lock.  There
		 * is some interaction between this code and the order
		 * in which rwsleep_unlock does things; see comments in
		 * rwsleep_unlock below.
		 */
		if (lockp->rw_mode == RWS_READ) {
			FSPIN_LOCK(&lockp->rw_fspin);
			++lockp->rw_read;
			ASSERT(lockp->rw_read > 0);
			/*
			 * Make sure it's still held in read mode
			 */
			if (!lockp->rw_avail && (lockp->rw_mode == RWS_READ)) {
				lockp->rw_pend -= nsleeps;
				if (lockp->rw_lkstatp != NULL) {
					++lockp->rw_lkstatp->lks_rdcnt;
					if (nsleeps > 0) {
						lockp->rw_lkstatp->lks_fail +=
							nsleeps;
						/* LINTED used before set */
						lockp->rw_lkstatp->lks_wtime = 
							ladd(lockp->rw_lkstatp->lks_wtime,
								wtime);
					}
				}
				FSPIN_UNLOCK(&lockp->rw_fspin);
				UNLOCK(lkp, PLBASE);
				ASSERT(!lockp->rw_avail);
				ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
					(mode == RWS_UNLOCKED));
				return;
			}
			--lockp->rw_read;
			ASSERT(lockp->rw_read >= 0);
			FSPIN_UNLOCK(&lockp->rw_fspin);
		}

		/*
		 * Barring races, we will put the caller to sleep.
		 */
		(void)LOCK(&lwpp->l_mutex, PLHI);
		FSPIN_LOCK(&lockp->rw_fspin);

		/*
		 * Check to see if the LWP raced with an unlock before
		 * actually putting it to sleep
		 */
		if (lockp->rw_avail && rwsleep_take(lockp)) {
			while (lockp->rw_mode == RWS_BUSY)
				;
			lockp->rw_mode = RWS_READ;
			lockp->rw_pend -= nsleeps;
			if (lockp->rw_lkstatp != NULL) {
				if (nsleeps > 0) {
					lockp->rw_lkstatp->lks_fail +=
						nsleeps;
					lockp->rw_lkstatp->lks_wtime = 
					ladd(lockp->rw_lkstatp->lks_wtime,
							wtime);
				}
				++lockp->rw_lkstatp->lks_rdcnt;
				++lockp->rw_lkstatp->lks_solordcnt;
				TIME_INIT(&lockp->rw_lkstatp->lks_stime);
			}
			FSPIN_UNLOCK(&lockp->rw_fspin);
			UNLOCK(&lwpp->l_mutex, PLHI);
			UNLOCK(lkp, PLBASE);
			ASSERT(!lockp->rw_avail);
			ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
				(mode == RWS_UNLOCKED));
			return;
		}

		/*
		 * If the lock is already in read mode, increment the
		 * read count under cover of the FSPIN lock (which
		 * is already held here), release the locks and return.
		 */
		if (lockp->rw_mode == RWS_READ) {
			++lockp->rw_read;
			ASSERT(lockp->rw_read > 0);
			/*
			 * Make sure it's still held in read mode
			 */
			if (!lockp->rw_avail && (lockp->rw_mode == RWS_READ)) {
				lockp->rw_pend -= nsleeps;
				if (lockp->rw_lkstatp != NULL) {
					++lockp->rw_lkstatp->lks_rdcnt;
					if (nsleeps > 0) {
						lockp->rw_lkstatp->lks_fail +=
							nsleeps;
						lockp->rw_lkstatp->lks_wtime = 
							ladd(lockp->rw_lkstatp->lks_wtime,
								wtime);
					}
				}
				FSPIN_UNLOCK(&lockp->rw_fspin);
				UNLOCK(&lwpp->l_mutex, PLHI);
				UNLOCK(lkp, PLBASE);
				ASSERT(!lockp->rw_avail);
				ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
					(mode == RWS_UNLOCKED));
				return;
			}
			/*
			 * Another race: lock was dropped, so decrement read
			 * count
			 */
			--lockp->rw_read;
			ASSERT(lockp->rw_read >= 0);
		}

		/* Lock is not available, put the caller to sleep. */

		if (lockp->rw_lkstatp != NULL) {
			/*
			 * initialize wtime if first sleep, and record
			 *	start of wait time
			 */
			if (nsleeps == 0)
				wtime.dl_hop = wtime.dl_lop = 0;
			TIME_INIT(&wstart);
		}
		if (nsleeps == 0) {
			/*
			 * Give control to the class specific code
			 *	to enqueue the lwp
			 */
			CL_INSQUE(lwpp, &lockp->rw_list, priority);
		} else {
			insque(lwpp, lockp->rw_head);
		}
		FSPIN_UNLOCK(&lockp->rw_fspin);

		/* Update the the lwp state */
		lwpp->l_stype = ST_RDLOCK;	/* blocked on a r/w
							 * lock for read */
		lwpp->l_flag |= L_NWAKE;/* don't wake us */
		lwpp->l_slptime = 0;	/* init start time for sleep */
		lwpp->l_stat = SSLEEP;	/* sleeping... */
		lwpp->l_syncp =  lockp;	/* waiting for this lock */

		/*
		 * Give up processor. The swtch() code will release
		 * the lwp state lock at PL0 when it is safe
		 */
		UNLOCK(lkp, PLHI);
		++nsleeps;
		swtch(lwpp);
		if (lockp->rw_lkstatp != NULL) {
			/*
			 * Update the cumulative wait time from the
			 * LWP wakeup time and the begin time. 
			 */
			wtime = ladd(wtime, lsub(lwpp->l_waket, wstart));
		}
		LOCK(lkp, PLHI);
	}
	while (lockp->rw_mode == RWS_BUSY)
		;
	lockp->rw_mode = RWS_READ;
	if ((nsleeps > 0) || (lockp->rw_lkstatp != NULL)) {
		FSPIN_LOCK(&lockp->rw_fspin);
		lockp->rw_pend -= nsleeps;
		if (lockp->rw_lkstatp != NULL) {
			if (nsleeps > 0) {
				lockp->rw_lkstatp->lks_fail += nsleeps;
				lockp->rw_lkstatp->lks_wtime = 
					ladd(lockp->rw_lkstatp->lks_wtime,
						wtime);
			}
			++lockp->rw_lkstatp->lks_rdcnt;
			++lockp->rw_lkstatp->lks_solordcnt;
			TIME_INIT(&lockp->rw_lkstatp->lks_stime);
		}
		FSPIN_UNLOCK(&lockp->rw_fspin);
	}
	UNLOCK(lkp, PLBASE);
	ASSERT(!lockp->rw_avail);
	ASSERT(((mode = lockp->rw_mode) == RWS_READ) || (mode == RWS_UNLOCKED));
}

/*
 * void
 * rwsleep_wrlock(rwsleep_t *lockp, int priority)
 *	Acquires a reader-writer sleep lock in write mode.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.  priority is a hint
 *	to the class specific code as to what our sleep/dispatch priority
 *	should be.  Returns: none.
 *
 *	Returns at PL0.
 *
 * Remarks:
 *	This primitive is not signallable.
 */
void 
rwsleep_wrlock(rwsleep_t *lockp, int priority)
{
	int nsleeps = 0;
	lwp_t *lwpp = u.u_lwpp;
	dl_t wstart, wtime;		/* start and total of wait time */

#ifdef _MPSTATS 
	rwsleeplk_count++;
#endif	/* _MPSTATS */

	ASSERT(KS_HOLD0LOCKS());

	ASSERT(rw_is_sane(lockp));

	if ((lockp->rw_lkstatp == NULL) && lockp->rw_avail &&
			rwsleep_take(lockp)) {
		while (lockp->rw_mode == RWS_BUSY)
			;
		lockp->rw_mode = RWS_WRITE;
		ASSERT(!lockp->rw_avail && (lockp->rw_mode == RWS_WRITE));
		return;
	}
	while (!lockp->rw_avail || !rwsleep_take(lockp)) {
		ASSERT(lockp->rw_pend >= nsleeps);
		/*
		 * Barring races, we will put the caller to sleep.
		 */
		(void)LOCK(&lwpp->l_mutex, PLHI);
		FSPIN_LOCK(&lockp->rw_fspin);

		/*
		 * Check to see if the LWP raced with an unlock before
		 * actually putting it to sleep.
		 */
		if (lockp->rw_avail && rwsleep_take(lockp)) {
			FSPIN_UNLOCK(&lockp->rw_fspin);
			UNLOCK(&lwpp->l_mutex, PLBASE);
			break;
		}

		/*
		 * Lock isn't available, put the caller to sleep
		 */
		if (lockp->rw_lkstatp != NULL) {
			/*
			 * initialize wtime if first sleep, and record
			 *	start of wait time
			 */
			if (nsleeps == 0)
				wtime.dl_hop = wtime.dl_lop = 0;
			TIME_INIT(&wstart);
		}

		if (nsleeps == 0) {
			/*
			 * Give control to the class specific code
			 *	to enqueue the lwp
			 */
			CL_INSQUE(lwpp, &lockp->rw_list, priority);
		} else {
			insque(lwpp, lockp->rw_head);
		}
		FSPIN_UNLOCK(&lockp->rw_fspin);

		/* Update the the lwp state */
		lwpp->l_stype = ST_WRLOCK;	/* blocked on a r/w
							 * lock for read */
		lwpp->l_flag |= L_NWAKE;/* don't wake us */
		lwpp->l_slptime = 0;	/* init start time for sleep */
		lwpp->l_stat = SSLEEP;	/* sleeping... */
		lwpp->l_syncp =  lockp;	/* waiting for this lock */

		/*
		 * Give up processor. The swtch() code will release
		 * the lwp state lock at PL0 when it is safe
		 */
		++nsleeps;
		swtch(lwpp);
		if (lockp->rw_lkstatp != NULL) {
			/*
			 * Update the cumulative wait time from the
			 * LWP wakeup time and the begin time. 
			 */
			wtime = ladd(wtime, lsub(lwpp->l_waket, wstart));
		}
	}
	while (lockp->rw_mode == RWS_BUSY)
		;
	lockp->rw_mode = RWS_WRITE;
	lockp->rw_pend -= nsleeps;
	if (lockp->rw_lkstatp != NULL) {
		/*
		 * if slept, update failure count
		 */
		if (lockp->rw_lkstatp != NULL) {
			if (nsleeps > 0) {
				lockp->rw_lkstatp->lks_fail += nsleeps;
				lockp->rw_lkstatp->lks_wtime = 
					ladd(lockp->rw_lkstatp->lks_wtime,
						wtime);
			}
			++lockp->rw_lkstatp->lks_wrcnt;
			TIME_INIT(&lockp->rw_lkstatp->lks_stime);
		}
	}
	ASSERT(!lockp->rw_avail && (lockp->rw_mode == RWS_WRITE));
}

/*
 * void
 * rwsleep_wrlock_rellock(rwsleep_t *lockp, int priority, lock_t *lkp)
 *	Acquires a reader-writer sleep lock in write mode.  The entry spin
 *	lock is atomically dropped.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.  priority is a hint
 *	to the class specific code as to what our sleep/dispatch priority
 *	should be.  lkp is a pointer to the entry spin lock.  Returns: none.
 *
 *	Returns at PL0.
 *
 * Remarks:
 *	Refer to remarks under rwsleep_rdlock_rellock().
 */
void 
rwsleep_wrlock_rellock(rwsleep_t *lockp, int priority, lock_t *lkp)
{
	int nsleeps = 0;
	lwp_t *lwpp = u.u_lwpp;
	dl_t wstart, wtime;		/* start and total of wait time */

#ifdef _MPSTATS 
	rwsleeplk_count++;
#endif	/* _MPSTATS */

	ASSERT(LOCK_OWNED(lkp));
	ASSERT(KS_HOLD1LOCK());

	ASSERT(rw_is_sane(lockp));

	if ((lockp->rw_lkstatp == NULL) && lockp->rw_avail &&
			rwsleep_take(lockp)) {
		while (lockp->rw_mode == RWS_BUSY)
			;
		lockp->rw_mode = RWS_WRITE;
		UNLOCK(lkp, PLBASE);
		ASSERT(!lockp->rw_avail && (lockp->rw_mode == RWS_WRITE));
		return;
	}
	while (!lockp->rw_avail || !rwsleep_take(lockp)) {
		ASSERT(lockp->rw_pend >= nsleeps);
		/*
		 * Barring races, we will put the caller to sleep.
		 */
		(void)LOCK(&lwpp->l_mutex, PLHI);
		FSPIN_LOCK(&lockp->rw_fspin);

		/*
		 * Check to see if the LWP raced with an unlock before
		 * actually putting it to sleep.
		 */
		if (lockp->rw_avail && rwsleep_take(lockp)) {
			FSPIN_UNLOCK(&lockp->rw_fspin);
			UNLOCK(&lwpp->l_mutex, PLBASE);
			break;
		}

		/*
		 * Lock isn't available, put the caller to sleep
		 */
		if (lockp->rw_lkstatp != NULL) {
			/*
			 * initialize wtime if first sleep, and record
			 *	start of wait time
			 */
			if (nsleeps == 0)
				wtime.dl_hop = wtime.dl_lop = 0;
			TIME_INIT(&wstart);
		}
		if (nsleeps == 0) {
			/*
			 * Give control to the class specific code
			 *	to enqueue the lwp
			 */
			CL_INSQUE(lwpp, &lockp->rw_list, priority);
		} else {
			insque(lwpp, lockp->rw_head);
		}
		FSPIN_UNLOCK(&lockp->rw_fspin);

		/* Update the the lwp state */
		lwpp->l_stype = ST_WRLOCK;	/* blocked on a r/w
							 * lock for read */
		lwpp->l_flag |= L_NWAKE;/* don't wake us */
		lwpp->l_slptime = 0;	/* init start time for sleep */
		lwpp->l_stat = SSLEEP;	/* sleeping... */
		lwpp->l_syncp =  lockp;	/* waiting for this lock */

		UNLOCK(lkp, PLHI);

		/*
		 * Give up processor. The swtch() code will release
		 * the lwp state lock at PL0 when it is safe
		 */
		++nsleeps;
		swtch(lwpp);
		if (lockp->rw_lkstatp != NULL) {
			/*
			 * Update the cumulative wait time from the
			 * LWP wakeup time and the begin time. 
			 */
			wtime = ladd(wtime, lsub(lwpp->l_waket, wstart));
		}
		LOCK(lkp, PLHI);
	}
	while (lockp->rw_mode == RWS_BUSY)
		;
	lockp->rw_mode = RWS_WRITE;
	lockp->rw_pend -= nsleeps;
	if (lockp->rw_lkstatp != NULL) {
		/*
		 * if slept, update failure count
		 */
		if (lockp->rw_lkstatp != NULL) {
			if (nsleeps > 0) {
				lockp->rw_lkstatp->lks_fail += nsleeps;
				lockp->rw_lkstatp->lks_wtime = 
					ladd(lockp->rw_lkstatp->lks_wtime,
						wtime);
			}
			++lockp->rw_lkstatp->lks_wrcnt;
			TIME_INIT(&lockp->rw_lkstatp->lks_stime);
		}
	}
	UNLOCK(lkp, PLBASE);
	ASSERT(!lockp->rw_avail && (lockp->rw_mode == RWS_WRITE));
}

/*
 * boolean_t
 * rwsleep_tryrdlock(rwsleep_t *lockp);
 *	Attempt to acquire the lock in read mode without blocking.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.  Returns:  B_TRUE
 *	if the lock was acquired, false otherwise.
 * 
 *	The ipl is the same upon return as it was at the time of the
 *	call.
 *
 * Remarks: The lock may be denied in read mode to newcomers if there is a 
 *	blocked LWP waiting to acquire it in write mode. 
 */
boolean_t 
rwsleep_tryrdlock(rwsleep_t *lockp)
{
#ifdef	DEBUG
	int mode;
#endif

#ifdef _MPSTATS 
	rwsleeplk_count++;
#endif	/* _MPSTATS */

	ASSERT(rw_is_sane(lockp));

	/*
	 * If the lock is available, take it and put it in
	 * read mode
	 */
	if (lockp->rw_avail && rwsleep_take(lockp)) {
		while (lockp->rw_mode == RWS_BUSY)
			;
		lockp->rw_mode = RWS_READ;
		if (lockp->rw_lkstatp == NULL)
			return B_TRUE;
		FSPIN_LOCK(&lockp->rw_fspin);
		++lockp->rw_lkstatp->lks_rdcnt;
		++lockp->rw_lkstatp->lks_solordcnt;
		TIME_INIT(&lockp->rw_lkstatp->lks_stime);
		FSPIN_UNLOCK(&lockp->rw_fspin);
		return B_TRUE;
	}
	if (lockp->rw_mode == RWS_READ) {
		/*
		 * If the lock is already in read mode, increment the
		 * read count under cover of the FSPIN lock.  There
		 * is some interaction between this code and the order
		 * in which rwsleep_unlock does things; see comments in
		 * rwsleep_unlock below.
		 */
		FSPIN_LOCK(&lockp->rw_fspin);
		++lockp->rw_read;
		ASSERT(lockp->rw_read > 0);
		/*
		 * Make sure it's still held in read mode
		 */
		if (!lockp->rw_avail && (lockp->rw_mode == RWS_READ)) {
			if (lockp->rw_lkstatp != NULL)
				++lockp->rw_lkstatp->lks_rdcnt;
			FSPIN_UNLOCK(&lockp->rw_fspin);
			ASSERT(!lockp->rw_avail);
			ASSERT(((mode = lockp->rw_mode) == RWS_READ) ||
				(mode == RWS_UNLOCKED));
			return B_TRUE;
		}
		--lockp->rw_read;
		ASSERT(lockp->rw_read >= 0);
		FSPIN_UNLOCK(&lockp->rw_fspin);
	}

	/*
	 * didn't get lock
	 */
	return B_FALSE;
}

/*
 * boolean_t
 * rwsleep_trywrlock(rwsleep_t *lockp)
 *	Attempts to acquire a lock in write mode without blocking.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.  Returns:  B_TRUE if
 *	the lock was acquired, B_FALSE otherwise.
 *
 *	The ipl is the same upon return as it was at the time of the call.
 */
boolean_t 
rwsleep_trywrlock(rwsleep_t *lockp)
{
#ifdef _MPSTATS 
	rwsleeplk_count++;
#endif	/* _MPSTATS */

	ASSERT(rw_is_sane(lockp));

	if (lockp->rw_avail && rwsleep_take(lockp)) {
		while (lockp->rw_mode == RWS_BUSY)
			;
		lockp->rw_mode = RWS_WRITE;
		ASSERT(!lockp->rw_avail && (lockp->rw_mode == RWS_WRITE));
		if (lockp->rw_lkstatp == NULL)
			return B_TRUE;
		FSPIN_LOCK(&lockp->rw_fspin);
		TIME_INIT(&lockp->rw_lkstatp->lks_stime);
		++lockp->rw_lkstatp->lks_wrcnt;
		FSPIN_UNLOCK(&lockp->rw_fspin);
		return B_TRUE;
	}
	return B_FALSE;
}

STATIC lwp_t * rwsleep_dequeue(rwsleep_t *);

/*
 * void
 * rwsleep_unlock(rwsleep_t *lockp)
 *	Releases a reader-writer sleep lock, waking up 
 *	one or more lwp's, if any are blocked.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be released.  Returns:  None.
 *
 * Description:
 *	Wake up lwp's from the head of the queue and update the lock state:
 *	if the first lwp is a reader, awaken lwps in the list 'til the
 *	first writer is encountered.
 *	Continue to awaken more blocked LWPs if all of the LWPs already
 *	woken up are swapped out.
 */
void 
rwsleep_unlock(rwsleep_t *lockp)
{
	pl_t s;
	lwp_t *lwpp, *wakeup_chain = NULL;
	dl_t ctime;
#ifdef	DEBUG
	int mode;
#endif

	ASSERT(rw_is_sane(lockp));

	ASSERT(!lockp->rw_avail);
	if (lockp->rw_mode != RWS_WRITE) {
		/*
		 * If the fspin lock is not held and there are no other
		 * readers, then this is the last releasing reader, so
		 * set the state to UNLOCKED.  Note that we must check
		 * the fspin lock before checking the read count in order
		 * to prevent races with any simultaneous rwsleep_rdlocks,
		 * since acquiring readers will increment the read count
		 * and then release the fspin lock.
		 */
		if (!FSPIN_IS_LOCKED(&lockp->rw_fspin) &&
				(lockp->rw_read == 0)) {
			lockp->rw_mode = RWS_UNLOCKED;
			WRITE_SYNC();
			/*
			 * Now check for races: make sure the fspin
			 * is still unlocked, there are still no other
			 * readers, and the state is still unlocked.
			 * If these are all true, then go to L1, where
			 * the lock is released; if any of the tests
			 * fail, then handle as a shared unlock.
			 *
			 * Note that we must check the fspin lock before
			 * checking the read count in order to prevent races
			 * with any simultaneous rwsleep_rdlocks, since
			 * acquiring readers will increment the read count
			 * and then release the fspin lock.
			 */
			if (!FSPIN_IS_LOCKED(&lockp->rw_fspin) &&
					(lockp->rw_read == 0) &&
					(lockp->rw_mode == RWS_UNLOCKED)) {
				goto L1;
			}
		}
		FSPIN_LOCK(&lockp->rw_fspin);
		if (lockp->rw_read > 0) {
			lockp->rw_mode = RWS_READ;
			--lockp->rw_read;
			ASSERT(lockp->rw_read >= 0);
			FSPIN_UNLOCK(&lockp->rw_fspin);
			return;
		}
		lockp->rw_mode = RWS_UNLOCKED;
		FSPIN_UNLOCK(&lockp->rw_fspin);
	}
L1:
	/*
	 * update the cumulative hold time
	 */
	if (lockp->rw_lkstatp != NULL) {
		TIME_INIT(&ctime);
		lockp->rw_lkstatp->lks_htime =
			ladd(lockp->rw_lkstatp->lks_htime,
				lsub(ctime, lockp->rw_lkstatp->lks_stime));
	}

	ASSERT((lockp->rw_mode == RWS_UNLOCKED) ||
			(lockp->rw_mode == RWS_WRITE));
	if (!FSPIN_IS_LOCKED(&lockp->rw_fspin) && EMPTYQUE(&lockp->rw_list)) {
		lockp->rw_mode = RWS_BUSY;
		DISABLE();					
		lockp->rw_avail = 1;
		WRITE_SYNC();
		if (!FSPIN_IS_LOCKED(&lockp->rw_fspin) &&
				EMPTYQUE(&lockp->rw_list)) {
			lockp->rw_mode = RWS_UNLOCKED;
			ENABLE();				
			return;
		}
		if (!lockp->rw_avail || !rwsleep_take(lockp)) {
			lockp->rw_mode = RWS_UNLOCKED;
			ENABLE();				
			return;
		}
		ENABLE();					
	}
	ASSERT(!lockp->rw_avail);

	wakeup_chain = rwsleep_dequeue(lockp);

	while (wakeup_chain != NULL) {
		lwpp = wakeup_chain;
		s = LOCK(&lwpp->l_mutex, PLHI);
		wakeup_chain = (lwp_t *)(lwpp->l_flink);
		LWP_WAKEUP(lwpp);
		/*
		 * Just copy ctime into wake; it's cheaper to do the
		 * copy than to do the test also.  The value will be
		 * garbage, and also ignored, if this lock doesn't
		 * have a lkstat structure.
		 */
		lwpp->l_waket = ctime;
		UNLOCK(&(lwpp->l_mutex), s);
	}
}

STATIC lwp_t *
rwsleep_dequeue(rwsleep_t *lockp)
{
	lwp_t *lwpp, *wakeup_chain = NULL;
	boolean_t swappedin = B_FALSE;

	FSPIN_LOCK(&lockp->rw_fspin);
	/*
	 * We wakeup lwps at the head of the queue until:
	 *	(1) We reach the end of a list of readers having found
	 *		at least one swapped in reader
	 *	(2) We find a single swapped in writer after having found
	 *		no swapped in readers
	 *	(3) We get to the end of the queue.
	 */
	while (!EMPTYQUE(&lockp->rw_list)) {
		lwpp = (lwp_t *)lockp->rw_head;
		ASSERT(lwpp != (lwp_t *)lockp);
		ASSERT(lockp->rw_tail != (list_t *)lockp);
		if ((lwpp->l_stype == ST_WRLOCK) && (wakeup_chain != NULL)
				&& swappedin)
			/*
			 * We've just reached the end of a list of readers
			 * having found at least one swapped in reader.
			 * Exit from the loop and wakeup the waiters
			 * we've found.
			 */
			break;
		++lockp->rw_pend;
		remque((list_t *)lwpp);
		lwpp->l_flink = (list_t *)wakeup_chain;
		wakeup_chain = lwpp;	
		if (LWP_LOADED(lwpp)) {
			if (lwpp->l_stype == ST_WRLOCK) {
				/*
				 * We've just found a swapped in writer,
				 * and we've found no swapped in waiters
				 * ahead of it.  Exit from the loop and
				 * wakeup the waiters we've found.
				 */
				ASSERT(!swappedin);
				break;
			}
			swappedin = B_TRUE;
		}
	}
	lockp->rw_mode = RWS_UNLOCKED;				
	lockp->rw_avail = 1;
	FSPIN_UNLOCK(&lockp->rw_fspin);
	return wakeup_chain;
}

/*
 * boolean_t
 * rwsleep_rdavail(rwsleep_t *lockp)
 *	See if a reader-writer sleep lock is available for read.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to examine.  Returns:
 *		B_TRUE if the lock could be acquired in the read mode,
 *		B_FALSE otherwise.
 *
 * Remarks:
 *	Returns stale data, unless the caller has a means to ensure
 *	freshness.
 */
boolean_t 
rwsleep_rdavail(rwsleep_t *lockp)
{

	return (lockp->rw_avail || (lockp->rw_mode == RWS_READ));
}

#undef RWSLEEP_IDLE
/*
 * boolean_t
 * RWSLEEP_IDLE(rwsleep_t *lockp)
 *	See if a reader-writer sleep lock is idle or not.
 *
 * Calling/Exit State:
 *	lockp is the lock to examine.  Returns:
 *		B_TRUE if the lock is idle, B_FALSE if not.
 *
 * Remarks:
 *	The return value is stale data, unless the caller has arranged
 *	to preserve its freshness.
 */
boolean_t
RWSLEEP_IDLE(rwsleep_t *lockp)
{

	return ((lockp->rw_avail) && (lockp->rw_mode != RWS_BUSY));
}

#undef RWSLEEP_LOCKBLKD
/*
 * boolean_t
 * RWSLEEP_LOCKBLKD(rwsleep_t *lockp)
 *	See if there are lwp's blocked/pending on a reader-writer sleep lock.
 *
 * Calling/Exit State:
 *	lockp is the lock to examine.  Returns: B_TRUE if there are lwp's
 *	blocked, B_FALSE otherwise.  The return value reflects stale state
 *	data, unless the caller has taken steps to preserve freshness.
 */
boolean_t
RWSLEEP_LOCKBLKD(rwsleep_t *lockp)
{

	return (FSPIN_IS_LOCKED(&lockp->rw_fspin) || !EMPTYQUE(&lockp->rw_list)
			|| (lockp->rw_pend > 0));
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)
/*
 * void
 * print_rwlock(rwsleep_t *lockp)
 *	Debugging print of rwsleep lock. Can be invoked from the
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_rwlock(rwsleep_t *lockp)
{

	debug_printf("avail = %d mode = %c readers = %d\n", lockp->rw_avail,
		lockp->rw_mode, lockp->rw_read);
	debug_printf(
		"pend = %d queue ptr = 0x%x statbuf ptr = 0x%x\n", 
		lockp->rw_pend, &lockp->rw_list, lockp->rw_lkstatp);

}
#endif

#ifdef	DEBUG
boolean_t
rw_is_sane(rwsleep_t *lockp) 
{
	uchar_t mode;

	if ((lockp->rw_read < 0) || (lockp->rw_pend < 0))
		return B_FALSE;
	mode = lockp->rw_mode;
	if ((mode != RWS_UNLOCKED) && (mode != RWS_READ)
			&& (mode != RWS_WRITE) && (mode != RWS_BUSY))
		return B_FALSE;
	return B_TRUE;
}
#endif

#ident	"@(#)kern-i386:util/sleep.c	1.6.2.1"
#ident	"$Header$"

/*
 * kernel synch primitives:  sleep locks.  
 */

#include <svc/cpu.h>
#include <util/assym_c.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/list.h>
#include <proc/signal.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <proc/class.h>
#include <proc/disp.h>
#include <mem/kmem.h>
#include <util/plocal.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>

#ifdef	UNIPROC

asm boolean_t
sleep_take(sleep_t *lockp)
{
%reg lockp;
	btrl	$0, _A_SL_AVAIL(lockp)
	setc	%al
	andl	$1, %eax	
%mem lockp;
	movl	lockp, %ecx
	btrl	$0, _A_SL_AVAIL(%ecx)
	setc	%al
	andl	$1, %eax	
}

#else	/* !UNIPROC */

asm boolean_t
sleep_take(sleep_t *lockp)
{
%ureg lockp;
	xorl	%eax, %eax
	xchgb	%al, _A_SL_AVAIL(lockp)
%mem lockp;
	movl	lockp, %ecx
	xorl	%eax, %eax
	xchgb	%al, _A_SL_AVAIL(%ecx)
}

#endif

#pragma asm partial_optimization sleep_take

/*
 * If _MPSTATS is defined, we keep a running total of the 
 * number of times sleep locks in the system are acquired. We are
 * not interested in exact numbers here and so, we bump this count
 * without holding any locks. 
 */

ulong_t		sleeplk_count;

/*
 * utility macro to perform wakeup actions when a lock is released. it is
 * assumed that the macro will be invoked under the cover of lwp mutex.
 */
#define	LWP_WAKEUP(lwpp) { \
	(lwpp)->l_stat = SRUN; \
	(lwpp)->l_syncp = NULL; \
	(lwpp)->l_flag &= ~L_SIGWOKE; \
	(lwpp)->l_stype = ST_NONE; \
	CL_WAKEUP((lwpp), (lwpp)->l_cllwpp, 0); \
}

#define	NOLWPID	((k_lwpid_t)USHRT_MAX)

/*
 * void
 * sleep_init(sleep_t *lockp, uchar_t hierarchy, lkinfo_t *lkinfop, 
 *	      int km_flags, int ksflags)
 * 	Initializes an instance of a sleep lock. 
 *
 * Calling/Exit State:
 *	hier is the position of this lock in the sleep lock hierarchy.
 *	Hierarchy checking is currently unimplemented for sleep locks, so
 *	zero may be substituted.  lkinfop is a pointer to a lkstat_t that
 *	the caller has allocated, and km_flags is either KM_SLEEP or
 *	KM_NOSLEEP, depending on whether the caller is willing to block
 *	while allocating memory. Code defines the compilation options
 *	that were in effect in the caller. 
 *
 *
 */
/*ARGSUSED*/
void 
sleep_init
	(sleep_t *lockp,	/* pointer to the lock */
	uchar_t hierarchy,	/* to enforce lock ordering */
	lkinfo_t *lkinfop,	/* info about the lock */
	int km_flags,		/* whether caller can sleep */
	int ksflags)		/* compilation options */
{
	ASSERT((km_flags == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

        FSPIN_INIT(&lockp->sl_fspin);

        /*
	 * the sleep queue is a doubly linked list with the forward pointer
         * of the last element and the backward pointer of the first element
	 * set to point to the head pointer of the synch object.
         */
	INITQUE(&lockp->sl_list);
        lockp->sl_pid = NOPID;
	lockp->sl_lwpid = NOLWPID;
	lockp->sl_hierarchy = hierarchy;
	lockp->sl_avail = 1;	/* lock is free */
        lockp->sl_pend = 0;
        lockp->sl_fixpend = 0;
	lkinfop->lk_flags |= LK_SLEEP;		/* this is a blocking lock */

	ASSERT(lockp->sl_head == lockp->sl_tail);
	ASSERT(lockp->sl_head == (list_t *)lockp);

#ifdef DEBUG
	if ((ksflags & ~(KS_LOCKTEST|KS_MPSTATS|KS_NVLTTRACE)) != 0) {
		/* 
		 *+ The SLEEP_INIT function was called with an invalid
		 *+ argument.  This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "invalid argument to SLEEP_INIT");
		/* NOTREACHED */
	}
#endif
	lockp->sl_flags = ksflags;

	lockp->sl_lkstatp = NULL;
        if ((ksflags & KS_MPSTATS) && !(lkinfop->lk_flags & LK_NOSTATS)) {
		/*
		 * Allocate a stats structure since _MPSTATS is on and the
		 * lkinfo structure does not have LK_NOSTATS set.
		 * lkstat_alloc() allocates and initializes a
		 * statistics structure.
		 */
		lockp->sl_lkstatp = lkstat_alloc(lkinfop, km_flags);
        }
}

/*
 * sleep_t *
 * sleep_alloc(uchar_t hierarchy, lkinfo_t *lkinfop, int km_flags, int ksflags)
 * 	Allocates and initializes a sleep lock. 
 *
 * Calling/Exit State:
 *	hierarchy is the position of this lock in the sleep lock hierarchy.
 *	Since hierarchy checking for sleep locks is currently not implemented,
 *	0 is accepted.  lkinfop is a pointer to a lkinfo_t that the caller
 *	has allocated and inititialized.  km_flags tells whether the caller
 *	is willing to sleep or not (should be KM_SLEEP or KM_NOSLEEP). Code
 *	defines the compilation options that were in effect in the caller.
 *	
 *	Returns:  a pointer to the lock.  If no lock can be allocated,
 *		NULL is returned.  If the lock can be allocated but the
 *		statistics buffer cannot, the pointer to the lock is returned
 *		anyway.
 */
sleep_t *
sleep_alloc
	(uchar_t hierarchy,		/* to enforce lock ordering */
	lkinfo_t *lkinfop,		/* info about the lock */
	int km_flags,			/* KM_SLEEP or KM_NOSLEEP */
	int ksflags)			/* hidden compile-time options */
{
	sleep_t *lockp;

	ASSERT((km_flags == KM_NOSLEEP) || (KS_HOLD0LOCKS()));

	/* allocate the lock */
	lockp = (sleep_t *)kmem_alloc(sizeof(*lockp), km_flags);
	if (lockp == NULL)
		return (NULL);

	/* initialize the lock */
	sleep_init(lockp, hierarchy, lkinfop, km_flags, ksflags);

	ASSERT(lockp->sl_head == lockp->sl_tail);
	ASSERT(lockp->sl_head == (list_t *)lockp);

	return (lockp);
}

/*
 * void
 * sleep_deinit(sleep_t *lockp)
 *	Frees up the stats structure if one was allocated. 
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to deinit.
 *
 * 	Returns:  None.
 */
void 
sleep_deinit(sleep_t *lockp)
{
	if ((lockp->sl_flags & KS_LOCKTEST) && 
		(FSPIN_IS_LOCKED(&lockp->sl_fspin) || !EMPTYQUE(&lockp->sl_list)
			|| (lockp->sl_pend > 0) || !lockp->sl_avail)
			|| (lockp->sl_pid == SL_BUSY)) {
		/*
		 *+ a sleep lock was de-initialized while in use.  This
		 *+ indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "deinitting active sleep lock");
		/*NOTREACHED*/
	}
        if (lockp->sl_lkstatp != NULL) {
                lkstat_free(lockp->sl_lkstatp, B_TRUE);
	}
}

/*
 * void
 * sleep_dealloc(sleep_t *lockp)
 *	Deallocates a sleep lock.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be de-allocated.
 */
void 
sleep_dealloc(sleep_t *lockp)
{
	/* get rid of the stats structure if one was allocated */
	SLEEP_DEINIT(lockp);

	/* free up the lock structure */
        kmem_free(lockp, sizeof(*lockp));
}

/*
 * void
 * sleep_lock(sleep_t *lockp, int priority)
 *	Acquires a sleep lock.  Not signalable.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.
 *	
 *	priority is a hint to the scheduling class as to what our priority
 *	should be when we are awarded the resource.  Higher priorities
 *	indicate a more more highly-contested resource.
 *
 *	No basic locks should be held while SLEEP_LOCK is called, since
 *	it may cause a context switch.
 *
 * 	Returns: none.
 *
 * Remarks:
 *	The base kernel can use the following symbols for specifying the
 *	priority: 
 *
 *		PRIMEM		(PMEM, PSWP)
 *		PRINOD		(PINOD, PSNOD)
 *		PRIBIO		(PRIBIO)
 *		PRIMED		(PZERO)
 *		PRIPIPE		(PPIPE)
 *		PRIVFS		(PVFS)
 *		PRIWAIT		(PWAIT)
 *		PRIREMOTE	(PREMOTE)
 *		PRISLEP		(PSLEP)
 *
 *	The list is ordered, with PRIMEM highest.  Additionally, positive
 *	and negative offsets from these values are accepted.  The maximum
 *	allowable offset it 3.
 *
 *	Device drivers may specify pridisk, prinet, pritty, and pritape,
 *	or prihi, primed, and prilo, and offsets up to 3 from prihi, primed,
 *	and prilo.
 */
void 
sleep_lock(sleep_t *lockp, int priority)
{
	pl_t s;
	lwp_t *lwpp = u.u_lwpp;
	dl_t wstart, wtime;		/* start and total of wait time */
	int nsleeps = 0;		/* number of times slept for lock */

	ASSERT(KS_HOLD0LOCKS());	

/*
 * Handle fast path case first
 *	- LOCKTEST is off
 *	- no adjustment of pending based on signalling needed
 *	- no lockstats
 *	- the lock is immediately available
 * If any of these fails, handle things with the slow path
 */
	if (!(lockp->sl_flags & KS_LOCKTEST)) {
		if ((lockp->sl_fixpend == 0) && (lockp->sl_lkstatp == NULL) &&
				lockp->sl_avail && sleep_take(lockp)) {
			/* record the new owner */
			while (lockp->sl_pid == SL_BUSY)
				;
			lockp->sl_pid = lwpp->l_procp->p_pidp->pid_id;	
			lockp->sl_lwpid = lwpp->l_lwpid;
			ASSERT(KS_HOLD0LOCKS());
			return;
		}
	} else {
		/* are we acquiring a lock we already hold? */
		if (lockp->sl_pid == lwpp->l_procp->p_pidp->pid_id &&
		    lockp->sl_lwpid == lwpp->l_lwpid) {
			/*
			 *+ An lwp attempted to acquire a sleep lock that
			 *+ it already held.  This indicates a kernel software
			 *+ problem.
			 */
			cmn_err(CE_PANIC, 
				"recursive acquisition of sleep lock");
			/*NOTREACHED*/
		}
	}

#ifdef _MPSTATS
        sleeplk_count++;
#endif  /* _MPSTATS */

	while (!lockp->sl_avail || !sleep_take(lockp)) {
		ASSERT(lockp->sl_pend >= nsleeps);


		/*
		 * We may have to put the caller to sleep.  Need to
		 * acquire l_mutex and sl_fspin. At this point, no fast
		 * spin locks are held and the waiter count on the lock
		 * has been incremented on behalf of the current context.
		 */
		s = LOCK(&lwpp->l_mutex, PLHI);
		FSPIN_LOCK(&lockp->sl_fspin);

		/*
		 * Check to see whether we raced with an unlock, before we do
		 * go to sleep.
		 */
		if (lockp->sl_avail && sleep_take(lockp)) {
        		/* We raced with an unlock. */
			ASSERT(lockp->sl_pend >= nsleeps);
        		FSPIN_UNLOCK(&lockp->sl_fspin);
			UNLOCK(&lwpp->l_mutex, s);
			break;
		}
		/* Lock is not available, put the caller to sleep. */

		if (lockp->sl_lkstatp != NULL) {
			/*
			 * Initialize wtime if first sleep, and record
			 *	start of wait time
			 */
			if (nsleeps == 0)
				wtime.dl_hop = wtime.dl_lop = 0;
			TIME_INIT(&wstart);
		}

		if (nsleeps == 0) {
			/* Give control to the class specific code to
			 * enqueue the lwp */
			CL_INSQUE(lwpp, &lockp->sl_list, priority);
		} else {
			/* 
			 * the current context was blocked earlier, and needs
			 * to block again, because the freed lock was obtained
			 * by someone else. if so, insert it at the head of
			 * the queue of waiters this time around.
			 */
			insque(lwpp, lockp->sl_head);
		}

		FSPIN_UNLOCK(&lockp->sl_fspin);

		/* Update the the lwp state */
		lwpp->l_stype = ST_SLPLOCK;	/* blocked on a sleep lock */
		lwpp->l_flag |= L_NWAKE;	/* don't wake for signals */
		lwpp->l_slptime = 0;		/* init start time for sleep */
		lwpp->l_stat = SSLEEP;		/* going to sleep */
		lwpp->l_syncp = lockp;		/* waiting on this lock */

	       /*
		* Give up the processor.  The swtch() code will release the
		* lwp state lock at PLBASE when it is safe
		*/
		++nsleeps;
		swtch(lwpp);
		if (lockp->sl_lkstatp != NULL) {
			/*
			 * Update the cumulative wait time from the
			 * LWP wakeup time and the begin time. 
			 */
			wtime = ladd(wtime, lsub(lwpp->l_waket, wstart));
		}
	} /* end while */

	lockp->sl_pend -= nsleeps;
	if (lockp->sl_fixpend > 0) {
		FSPIN_LOCK(&lockp->sl_fspin);
		lockp->sl_pend -= lockp->sl_fixpend;
		lockp->sl_fixpend = 0;
		FSPIN_UNLOCK(&lockp->sl_fspin);
	}
	/* record the new owner */
	while (lockp->sl_pid == SL_BUSY)
		;
	ASSERT(lockp->sl_pend >= 0);
	ASSERT(lockp->sl_pid == NOPID);
	ASSERT(lockp->sl_lwpid == NOLWPID);
	lockp->sl_pid = lwpp->l_procp->p_pidp->pid_id;	
	lockp->sl_lwpid = lwpp->l_lwpid;
	if (lockp->sl_lkstatp != NULL) {
		/*
		 * if slept, update failure count and lock wait times
		 */
		if (nsleeps > 0) {
			lockp->sl_lkstatp->lks_fail += nsleeps;
			lockp->sl_lkstatp->lks_wtime = 
				ladd(lockp->sl_lkstatp->lks_wtime,
					wtime);
		}
		/*
		 * update start time and counter
		 */
		TIME_INIT(&lockp->sl_lkstatp->lks_stime);
		++lockp->sl_lkstatp->lks_wrcnt;
	}
	ASSERT(KS_HOLD0LOCKS());
} 

/*
 * boolean_t
 * sleep_lock_sig(sleep_t *lockp, int priority)
 * 	Acquires a lock.  Returns when signaled.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to be acquired.  Priority is a
 *	hint to the scheduling class as to our sleep/dispatch priority.
 *	Returns at PLBASE.
 *	
 *	Returns:  B_TRUE if the lock is acquired, B_FALSE if the sleep was
 *	interrupted by a signal.
 *
 *	No basic locks should be held at the time of the call to this
 *	function, since it may cause a context switch.
 *
 * Description:
 *	In the event of a job control stop/continue, the primitive
 *	transparently tries to reacquire the lock.
 */
boolean_t 
sleep_lock_sig(sleep_t *lockp, int priority)
{
	pl_t	s;
	lwp_t 	*lwpp = u.u_lwpp;
	proc_t	*p = u.u_procp;
	int nsleeps = 0;		/* number of times slept for lock */
	dl_t wstart, wtime;		/* start and total of wait time */

	ASSERT(KS_HOLD0LOCKS());

        if (lockp->sl_flags & KS_LOCKTEST) {
                if (lockp->sl_pid == lwpp->l_procp->p_pidp->pid_id &&
		    lockp->sl_lwpid == lwpp->l_lwpid) {
			/*
			 *+ An lwp attempted to acquire a lock it already
			 *+ held.  This indicates a kernel software problem.
			 */
			cmn_err(CE_PANIC, "recursive acquisition of sleep lock");
			/*NOTREACHED*/
                }
        }

again:

	ASSERT(KS_HOLD0LOCKS());

	/* are there pending signals? */
	if (!QUEUEDSIG(lwpp)) {
		s = LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_flag &= ~L_SIGWOKE;
		if (!QUEUEDSIG(lwpp)) {
			/* for sure no pending signals; acquire the lock */
			goto acquire;
		}
		/* must call issig() */
		UNLOCK(&lwpp->l_mutex, s);
	} else
		s = getpl();

	/*
	 * issig() is called to determine if there are any pending signals.
	 * Note that issig also processes other events that may be posted.
	 */
	switch (issig((lock_t *)NULL)) {
	case ISSIG_NONE:
		/*
		 * No signals or other update events posted.
		 *
		 * A job control stop signal was cancelled by a defaulted
		 * or ignored continue signal, or a caught SIGCONT signal
		 * was cancelled by an ignored or held job control stop
		 * signal.  In any case, l_mutex is still held.
		 *
		 * We go for the lock again.
		 */
		break;
	case ISSIG_SIGNALLED:
		/*
		 * The LWP has a signal to process, one or more update 
		 * event flags are set, or the current system call 
		 * has been aborted. Also, the lwp may have been stopped.
		 */
		ASSERT(KS_HOLD0LOCKS());
		if (nsleeps > 1) {
			FSPIN_LOCK(&lockp->sl_fspin);
			lockp->sl_fixpend += nsleeps - 1;
			FSPIN_UNLOCK(&lockp->sl_fspin);
		}
		return (B_FALSE);		/* a signal is pending */
	case ISSIG_STOPPED:
		/*
		 * transparently go for the lock.  all locks are dropped in
		 * this case. And so, we need to test if there are any
		 * signals pending as we opened up a window.
		 *
		 * The LWP was stopped by job control, a ptrace(2) stop, a
		 * /proc requested stop, or a fork(2) rendezvous stop.
		 * However, no signals or update events exist for the LWP.
		 */
		ASSERT(KS_HOLD0LOCKS());
		goto again;
	default:
		/* this should never happen. */
		/*
		 *+ issig returned an unexpected value.  This indicates
		 *+ a kernel software problem.
		 */
		cmn_err(CE_PANIC, "unexpected return from issig");	
		/*NOTREACHED*/
	}

acquire:
	ASSERT(LOCK_OWNED(&lwpp->l_mutex));
	ASSERT(KS_HOLD1LOCK());
	/* 
	 * Go for the lock, if it's available. However, we may block if the 
	 * lock is not available. Check to see if we are the last context to
	 * block interruptibly. 
	 * 
	 */

	FSPIN_LOCK(&p->p_niblk_mutex);
	++p->p_niblked;
	if ((p->p_niblked >= p->p_nlwp) && p->p_sigwait &&
	    !sigismember(&p->p_sigignore, SIGWAITING) &&
	    !sigismember(&p->p_sigs, SIGWAITING)) {
		/*
		 * There is a possibility that we may be the last context in
		 * the process to block interruptibly.  Send the SIGWAITING
		 * signal. It is possible that we may not block or a different
		 * context in the process could wakeup even as we are preparing
		 * to to send the SIGWAITING signal.  This is a harmless race
		 * and cannot be closed; the worst that can happen is that a
		 * signal will be sent when none should have been sent.
		 *
		 * Note that the SIGWAITING signal is ignored by default.
		 * Hence to avoid an infinite loop, we send the signal only if
		 * it is caught and not already pending to the calling process.
		 */
		--p->p_niblked;
		FSPIN_UNLOCK(&p->p_niblk_mutex);
		UNLOCK(&lwpp->l_mutex, PLBASE);
		sigtoproc(p, SIGWAITING, (sigqueue_t *)NULL);
		goto again;
	}
	FSPIN_UNLOCK(&p->p_niblk_mutex);
	if (lockp->sl_avail && sleep_take(lockp)) {
		/* got the lock */
		lockp->sl_pend -= nsleeps;
		ASSERT(lockp->sl_pend >= 0);
                UNLOCK(&lwpp->l_mutex, s);
		if (lockp->sl_fixpend > 0) {
			FSPIN_LOCK(&lockp->sl_fspin);
			lockp->sl_pend -= lockp->sl_fixpend;
			lockp->sl_fixpend = 0;
			FSPIN_UNLOCK(&lockp->sl_fspin);
		}

		/* set lock owner */
		while (lockp->sl_pid == SL_BUSY)
			;

		ASSERT(lockp->sl_pid == NOPID);
		ASSERT(lockp->sl_lwpid == NOLWPID);

		lockp->sl_pid = lwpp->l_procp->p_pidp->pid_id;
		lockp->sl_lwpid = lwpp->l_lwpid;
		if (lockp->sl_lkstatp != NULL) {
			/*
			 * if slept, update failure count and lock wait times
			 */
			if (nsleeps > 0) {
				lockp->sl_lkstatp->lks_fail += nsleeps;
				/* LINTED used before set */
				lockp->sl_lkstatp->lks_wtime = 
					ladd(lockp->sl_lkstatp->lks_wtime,
						wtime);
			}
			/*
			 * update start time and counter
			 */
			TIME_INIT(&lockp->sl_lkstatp->lks_stime);
               		++lockp->sl_lkstatp->lks_wrcnt;
		}

		/*
		 * Did not block; decrement the p_niblked count.
		 */
		FSPIN_LOCK(&p->p_niblk_mutex);
		--p->p_niblked;
		FSPIN_UNLOCK(&p->p_niblk_mutex);
#ifdef _MPSTATS
		sleeplk_count++;
#endif /* _MPSTATS*/
		ASSERT(KS_HOLD0LOCKS());
		return (B_TRUE);
	}

	ASSERT(lockp->sl_pend >= nsleeps);


	/*
	 * We may have to put the caller to sleep.  l_mutex is
	 * already held, but need to acquire sl_fspin.
	 */
	ASSERT(LOCK_OWNED(&lwpp->l_mutex));
	ASSERT(KS_HOLD1LOCK());
	FSPIN_LOCK(&lockp->sl_fspin);

	/*
	 * Check to see whether we raced with an unlock, before we do
	 * go to sleep.
	 */
        if (lockp->sl_avail) { /* We raced with an unlock. */
		ASSERT(lockp->sl_pend >= nsleeps);
        	FSPIN_UNLOCK(&lockp->sl_fspin);
		UNLOCK(&lwpp->l_mutex, s);
		goto acquire;
	}
	if (lockp->sl_lkstatp != NULL) {
		/*
		 * initialize wtime if first sleep, and record
		 *	start of wait time
		 */
		if (nsleeps == 0)
			wtime.dl_hop = wtime.dl_lop = 0;
		TIME_INIT(&wstart);
	}

	if (nsleeps > 0) {
		ASSERT(lockp->sl_pend >= nsleeps);
		/*
		 * the caller had blocked at least once before; and has failed 
		 * to get the lock on a retry after being woken up. This time,
		 * it goes to the head of the list of waiters. Also, since
		 * this is a retry, the count of waiters does not go up.
		 */
		insque(lwpp, lockp->sl_head);
	} else {
		/*
		 * the caller is being blocked, and it is the first time.
		 * so give control to the class specific code to enqueue 
		 * the lwp. also, increase the count of waiters.
		 */
	        CL_INSQUE(lwpp, &lockp->sl_list, priority);
	}	

        FSPIN_UNLOCK(&lockp->sl_fspin);

        /* Update the the lwp state */
        lwpp->l_stype = ST_SLPLOCK;	/* blocked on a sleep lock*/
        lwpp->l_flag &= ~L_NWAKE;	/* turn NWAKE off; we want to wake. */
        lwpp->l_slptime = 0;            /* init the start time for sleep */
        lwpp->l_stat = SSLEEP;		/* blocked on sleep lock */
        lwpp->l_syncp = lockp;		/* waiting on this lock */

	/*
	 * Give up the processor.  The swtch() code will release the
	 * lwp state lock at PLBASE when it is safe to do so.
	 */
	++nsleeps;
        swtch(lwpp);
	if (lockp->sl_lkstatp != NULL) {
		/*
		 * Update the cumulative wait time from the
		 * LWP wakeup time and the begin time. 
		 */
		wtime = ladd(wtime, lsub(lwpp->l_waket, wstart));
	}
	FSPIN_LOCK(&p->p_niblk_mutex);
	--p->p_niblked;
	FSPIN_UNLOCK(&p->p_niblk_mutex);
	
	if (lwpp->l_flag & L_SIGWOKE) {
		/* 
		 * awakened because of a signal or other update event. 
		 */
		switch (issig((lock_t *)NULL)) {
		case ISSIG_NONE:
                        /*
			 * The only way we could have gotten here is if a
                         * debugger had cleared the signal or if a job control
                         * stop signal was posted that was discarded by a
                         * subsequent SIGCONT. Go for the lock. Note that
                         * the l_mutex lock is held.
			 */
			lwpp->l_flag &= ~L_SIGWOKE;		
			goto acquire;
		case ISSIG_SIGNALLED:
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_flag &= ~L_SIGWOKE;
			UNLOCK(&lwpp->l_mutex, PLBASE);
			ASSERT(KS_HOLD0LOCKS());
			if (nsleeps > 1) {
				FSPIN_LOCK(&lockp->sl_fspin);
				lockp->sl_fixpend += nsleeps - 1;
				FSPIN_UNLOCK(&lockp->sl_fspin);
			}
			return (B_FALSE);
		case ISSIG_STOPPED:
			/*
			 * Go for the lock transparently. Since issig returns
			 * with l_mutex dropped, need to start all over. Note
			 * that we could directly go for the lock if issig
			 * had not opened the window.
			 */
			ASSERT(KS_HOLD0LOCKS());
			goto again;
		default:
			/* this should never happen */
			/*
			 *+ issig returned an unexpected value.  This indicates
			 *+ a kernel software problem.
			 */
			cmn_err(CE_PANIC, "unexpected return from issig");
			/*NOTREACHED*/
			break;
		}
	}

	ASSERT(KS_HOLD0LOCKS());
	
#ifdef _MPSTATS 
	sleeplk_count++;
#endif	/* _MPSTATS */

	(void)LOCK(&lwpp->l_mutex, PLHI);	
	goto acquire;
}

/*
 * void
 * sleep_lock_rellock(sleep_t *lockp, int priority, lock_t *lkp)
 *	Acquire the lock, atomically releasing the basic lock before
 *	returning or context switching.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the sleep lock to be acquired, priority is a 
 *	hint to the scheduling class for our sleep/dispatch priority.
 *	lkp is a pointer to the basic lock to be released.  
 *
 *	Returns at PLBASE with lkp dropped.
 *
 *	No basic locks other than lkp should be held at the time of the call
 *	to this function, since it may context switch.
 *
 * Remarks:
 *	WARNING!
 *	If this entry point is to be included in the DDI/DKI, we need to 
 *	pass an extra argument that encodes the compilation options 
 *	that were in effect for the caller. Refer to the file util/sv.c
 *	for details.
 *
 */
void 
sleep_lock_rellock(sleep_t *lockp,	/* the lock to acquire */
	int priority,		/* dispatch priority */
	lock_t *lkp)		/* pointer to a spin lock held on entry */
{
	pl_t s;
	lwp_t *lwpp = u.u_lwpp;
	dl_t wstart, wtime;		/* start and total of wait time */
	int nsleeps = 0;		/* number of times slept for lock */

	ASSERT(LOCK_OWNED(lkp));
	ASSERT(KS_HOLD1LOCK());

/*
 * Handle fast path case first
 *	- LOCKTEST is off
 *	- no adjustment of pending based on signalling needed
 *	- no lockstats
 *	- the lock is immediately available
 * If any of these fails, handle things with the slow path
 */
	if (!(lockp->sl_flags & KS_LOCKTEST)) {
		if ((lockp->sl_fixpend == 0) && (lockp->sl_lkstatp == NULL) &&
				lockp->sl_avail && sleep_take(lockp)) {
			/*
			 * release the spin lock at PLBASE and
			 * record the new owner of the sleep lock
			 */
			UNLOCK(lkp, PLBASE);
			while (lockp->sl_pid == SL_BUSY)
				;
			lockp->sl_pid = lwpp->l_procp->p_pidp->pid_id;	
			lockp->sl_lwpid = lwpp->l_lwpid;
			ASSERT(KS_HOLD0LOCKS());
			return;
		}
	} else {
		/* are we acquiring a lock we already hold? */
		if (lockp->sl_pid == lwpp->l_procp->p_pidp->pid_id &&
		    lockp->sl_lwpid == lwpp->l_lwpid) {
			/*
			 *+ An lwp attempted to acquire a sleep lock that
			 *+ it already held.  This indicates a kernel software
			 *+ problem.
			 */
			cmn_err(CE_PANIC, 
				"recursive acquisition of sleep lock");
			/*NOTREACHED*/
		}
	}

#ifdef _MPSTATS 
	sleeplk_count++;
#endif	/* _MPSTATS */

	while (!lockp->sl_avail || !sleep_take(lockp)) {
		ASSERT(lockp->sl_pend >= nsleeps);
		/*
		 * We may have to put the caller to sleep.  Need to
		 * acquire l_mutex and sl_fspin.
		 */
		s = LOCK(&lwpp->l_mutex, PLHI);
		FSPIN_LOCK(&lockp->sl_fspin);

		/*
		 * Check to see whether we raced with an unlock, before we do
		 * go to sleep.
		 */
		if (lockp->sl_avail && sleep_take(lockp)) {
        		/* We raced with an unlock. */
			ASSERT(lockp->sl_pend >= nsleeps);
        		FSPIN_UNLOCK(&lockp->sl_fspin);
			UNLOCK(&lwpp->l_mutex, s);
			break;
		}

		/* Lock is not available, put the caller to sleep. */
		if (lockp->sl_lkstatp != NULL) {
			/*
			 * initialize wtime if first sleep, and record
			 *	start of wait time
			 */
			if (nsleeps == 0)
				wtime.dl_hop = wtime.dl_lop = 0;
			TIME_INIT(&wstart);
		}

		if (nsleeps == 0) {
			/* Give control to the class specific code to
			 * enqueue the lwp */
			CL_INSQUE(lwpp, &lockp->sl_list, priority);
		} else {
			/* 
			 * the current context was blocked earlier, and needs
			 * to block again, because the freed lock was obtained
			 * by someone else. if so, insert it at the head of
			 * the queue of waiters this time around.
			 */
			insque(lwpp, lockp->sl_head);
		}

		FSPIN_UNLOCK(&lockp->sl_fspin);

		/* Update the the lwp state */
		lwpp->l_stype = ST_SLPLOCK;	/* blocked on a sleep lock */
		lwpp->l_flag |= L_NWAKE;	/* don't wake for signals */
		lwpp->l_slptime = 0;		/* init start time for sleep */
		lwpp->l_stat = SSLEEP;		/* going to sleep */
		lwpp->l_syncp = lockp;		/* waiting on this lock */

		/*
		* we want to be at plhi, because that's where the l_mutex needs
		 * to be held.
		 */
		UNLOCK(lkp, PLHI);
		++nsleeps;

	       /*
		* Give up the processor.  The swtch() code will release the
		* lwp state lock at PLBASE when it is safe
		*/
		swtch(lwpp);

		if (lockp->sl_lkstatp != NULL) {
			/*
			 * Update the cumulative wait time from the
			 * LWP wakeup time and the begin time. 
			 */
			wtime = ladd(wtime, lsub(lwpp->l_waket, wstart));
		}

		(void)LOCK(lkp, PLHI);
	}

	lockp->sl_pend -= nsleeps;
	if (lockp->sl_fixpend > 0) {
		FSPIN_LOCK(&lockp->sl_fspin);
		lockp->sl_pend -= lockp->sl_fixpend;
		lockp->sl_fixpend = 0;
		FSPIN_UNLOCK(&lockp->sl_fspin);
	}
	UNLOCK(lkp, PLBASE);	/* drop the entry lock */
	/* record the new owner */
	while (lockp->sl_pid == SL_BUSY)
		;
	ASSERT(lockp->sl_pend >= 0);
	ASSERT(lockp->sl_pid == NOPID);
	ASSERT(lockp->sl_lwpid == NOLWPID);
	lockp->sl_pid = lwpp->l_procp->p_pidp->pid_id;	
	lockp->sl_lwpid = lwpp->l_lwpid;
	if (lockp->sl_lkstatp != NULL) {
		/*
		 * if slept, update failure count and lock wait times
		 */
		if (nsleeps > 0) {
			lockp->sl_lkstatp->lks_fail += nsleeps;
			lockp->sl_lkstatp->lks_wtime = 
				ladd(lockp->sl_lkstatp->lks_wtime,
					wtime);
		}
		/*
		 * update start time and counter
		 */
		TIME_INIT(&lockp->sl_lkstatp->lks_stime);
               	++lockp->sl_lkstatp->lks_wrcnt;
	}
	ASSERT(KS_HOLD0LOCKS());
}

/*
 * boolean_t
 * sleep_trylock(sleep_t *lockp)
 *	Attempts to acquire the lock without sleeping.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to attempt to acquire.
 *
 *	Returns:  B_TRUE if the lock is acquired, B_FALSE otherwise.
 */ 
boolean_t  
sleep_trylock(sleep_t *lockp)
{
	lwp_t	*lwpp = u.u_lwpp;

        if (lockp->sl_flags & KS_LOCKTEST) {
                if (lockp->sl_pid == lwpp->l_procp->p_pidp->pid_id &&
		    lockp->sl_lwpid == lwpp->l_lwpid) {
			/*
			 *+ An lwp attempted to acquire a sleep lock
			 *+ that it already held.
			 */
			cmn_err(CE_PANIC, "recursive acquisition of sleep lock");
			/*NOTREACHED*/
                }
        }

	/* go for the lock */

	if (lockp->sl_avail && sleep_take(lockp)) {	/* we claim this lock */
		/* record the owner */
		while (lockp->sl_pid == SL_BUSY)
			;
		ASSERT(lockp->sl_pid == NOPID);
		ASSERT(lockp->sl_lwpid == NOLWPID);
		lockp->sl_pid = lwpp->l_procp->p_pidp->pid_id;
		lockp->sl_lwpid = lwpp->l_lwpid;

                if (lockp->sl_lkstatp != NULL) {
			/*
			 * update start time and counter
			 */
                        TIME_INIT(&lockp->sl_lkstatp->lks_stime);
                        ++lockp->sl_lkstatp->lks_wrcnt;
                }
#ifdef _MPSTATS 
		sleeplk_count++;
#endif	/* _MPSTATS */
		return (B_TRUE);
	}
	return (B_FALSE);
}

/*
 * void
 * sleep_lock_private(sleep_t *lockp)
 *	Acquires the lock without checking if it's already held..
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to acquire.
 *	Caller guarantees it holds the lock privately and it is currently
 *	unlocked.  The lock will be locked, in a "disowned" state, on
 *	return.  This function can be used even if there is no user context
 *	(i.e. u.u_lwpp == NULL).
 */ 
void
sleep_lock_private(sleep_t *lockp)
{
	ASSERT(SLEEP_LOCKAVAIL(lockp) && !SLEEP_LOCKBLKD(lockp));

	/* get the lock */

	ASSERT(lockp->sl_avail);
	(void)sleep_take(lockp);	/* we claim this lock */

	/* record no owner ("disowned") */
	lockp->sl_pid = NOPID;

	if (lockp->sl_lkstatp != NULL) {
		/*
		 * update start time and counter
		 */
		TIME_INIT(&lockp->sl_lkstatp->lks_stime);
		++lockp->sl_lkstatp->lks_wrcnt;
	}

#ifdef _MPSTATS 
	sleeplk_count++;
#endif	/* _MPSTATS */
}

STATIC lwp_t * sleep_dequeue(sleep_t *);

/*
 * void
 * sleep_unlock(sleep_t *lockp)
 *	Releases the sleep lock. If there are waiting lwps,
 *	the lwp at the head of the list is dequeued and woken up.
 *	In addition, if the head lwp is not in-core, then the next
 *	waiting lwp (if any) is also dequeued and woken up --this
 *	is repeated until either the queue is empty or an lwp
 *	is woken up that is in-core at the time of wakeup.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to unlock.  Returns:  none.
 */ 
void 
sleep_unlock(sleep_t *lockp)
{
	pl_t s;
	lwp_t *lwpp, *wakeup_chain = NULL;
	dl_t ctime;

	/*
	 * this would be a good place to assert that the lock is being
	 * unlocked by the same context that unlocked it.  However, some
	 * code (having to do with the bufcache) violates that constraint,
	 * so this assertion has been removed.
	 */

	/* update the cumulative hold time */
	if (lockp->sl_lkstatp != NULL) {
		TIME_INIT(&ctime);
		lockp->sl_lkstatp->lks_htime =
			ladd(lockp->sl_lkstatp->lks_htime,
				lsub(ctime, lockp->sl_lkstatp->lks_stime));
	}
	ASSERT(!lockp->sl_avail); 

	if (!FSPIN_IS_LOCKED(&lockp->sl_fspin) && EMPTYQUE(&lockp->sl_list) &&
			(lockp->sl_fixpend == 0)) {
		lockp->sl_pid = SL_BUSY;
		lockp->sl_lwpid = NOLWPID;
		DISABLE();					
		lockp->sl_avail = 1;
		WRITE_SYNC();
		if (!FSPIN_IS_LOCKED(&lockp->sl_fspin) &&
				EMPTYQUE(&lockp->sl_list)) {
			lockp->sl_pid = NOPID;
			ENABLE();				
			return;
		}
		if (!lockp->sl_avail || !sleep_take(lockp)) {
			lockp->sl_pid = NOPID;
			ENABLE();				
			return;
		}
		ENABLE();					
	}
	wakeup_chain = sleep_dequeue(lockp);
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

/*
 * must own lock on entry
 */
STATIC lwp_t *
sleep_dequeue(sleep_t *lockp)
{
	lwp_t *lwpp, *wakeup_chain = NULL;

	FSPIN_LOCK(&lockp->sl_fspin);
	/*
	 * wake up the first waiter. if it is not in-core, we could
	 * choose to wake it up anyway, and then wakeup the next
	 * waiter as well (and if it happens to be swapped out as
	 * well, then the one after it ...) -- or wakeup the first in-core 
	 * LWP from the queue of waiters. we choose to keep waking up
	 * successive waiters until we have either exhausted the queue,
	 * or woken up an LWP that is not swapped out.
	 */
	while (!EMPTYQUE(&lockp->sl_list)) {
		lwpp = (lwp_t *)lockp->sl_head;
		ASSERT(lwpp != (lwp_t *)lockp);
		ASSERT(lockp->sl_tail != (list_t *)lockp);
		remque((list_t *)lwpp);
		lwpp->l_flink = (list_t *)wakeup_chain;
		wakeup_chain = lwpp;	
		++lockp->sl_pend;
		if (LWP_LOADED(lwpp)) {
			break;
		}
	}
	if (lockp->sl_fixpend != 0) {
		lockp->sl_pend -= lockp->sl_fixpend;
		lockp->sl_fixpend = 0;
	}
	lockp->sl_lwpid = NOLWPID;
	lockp->sl_pid = NOPID;					
	lockp->sl_avail = 1;
	FSPIN_UNLOCK(&lockp->sl_fspin);
	return wakeup_chain;
}

/*
 * boolean_t
 * sleep_unsleep(sleep_t *lockp, struct lwp *lwpp)
 *	Remove a specified lwp from the sleep queue.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to remove the lwp from its queue.
 *	lwpp is a pointer to the lwp to remove.
 *
 *	Returns:  B_TRUE if the specified lwp was dequeued from the specified
 *	queue, B_FALSE if not.
 *
 *	This function should be called with the lwp state lock held, and
 *	it returns with the state lock still held.
 */
boolean_t 
sleep_unsleep(sleep_t *lockp, struct lwp *lwpp)
{
	boolean_t dequeued;
	ASSERT(LOCK_OWNED(&lwpp->l_mutex));

	/* if _LOCKTEST is set perform the necessary sanity checks */
	if (lockp->sl_flags & KS_LOCKTEST) {
		/*EMPTY*/
		ASSERT(hier_findlock(&lwpp->l_mutex));
		/* "lock not held on entry to SLEEP_UNSLEEP()" */
		/*NOTREACHED*/
	}

	/* Remove the lwp from the sleep queue */

	FSPIN_LOCK(&lockp->sl_fspin);

        /*
	 * slpdeque() returns B_TRUE if it can successfully
         * dequeue, B_FALSE otherwise (when setrun() has lost the race with
	 * a normal wakeup). 
         */
	dequeued = slpdeque(&lockp->sl_list, lwpp);

	FSPIN_UNLOCK(&lockp->sl_fspin);
	return (dequeued);
}

#undef SLEEP_LOCKAVAIL
/*
 * boolean_t
 * SLEEP_LOCKAVAIL(sleep_t *lockp)
 *	See if a lock is available.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock to examine.
 *
 *	Returns:  B_TRUE if the lock is available, B_FALSE otherwise.
 *
 * Remarks:
 *	Returns stale data, unless the caller has arranged to keep
 *	the data fresh by some means.
 */
boolean_t 
SLEEP_LOCKAVAIL(sleep_t *lockp)
{
	return ((lockp->sl_avail) && (lockp->sl_pid != SL_BUSY));
}

#undef SLEEP_LOCKBLKD
/* 
 * boolean_t
 * SLEEP_LOCKBLKD(sleep_t *lockp)
 *	See if there are any lwps blocked on a sleep lock.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the sleep lock to be examined.  Returns: 
 *	B_TRUE if there are any lwps blocked, B_FALSE otherwise.
 *
 * Remarks:
 *	Returns stale data, unless the caller has arranged to keep the
 *	data fresh by some means.
 */
boolean_t 
SLEEP_LOCKBLKD(sleep_t *lockp)
{
	return (FSPIN_IS_LOCKED(&lockp->sl_fspin) || !EMPTYQUE(&lockp->sl_list)
			|| (lockp->sl_pend > 0));
}

#undef SLEEP_LOCKOWNED
/*
 * boolean_t
 * SLEEP_LOCKOWNED(sleep_t *lockp)
 *	See if a lock is owned by the current context.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the sleep lock to be examined.  Returns:
 *	B_TRUE if the lock is owned by the calling context, B_FALSE otherwise.
 */
boolean_t 
SLEEP_LOCKOWNED(sleep_t *lockp)
{
        if (lockp->sl_pid == u.u_lwpp->l_procp->p_pidp->pid_id &&
	    lockp->sl_lwpid == u.u_lwpp->l_lwpid) {
			return (B_TRUE);
	}
	return (B_FALSE);
}

#ifdef	DEBUG
/*
 * void
 * print_sleep_lock(sleep_t *lockp)
 *      Print the fields of a given sleep lock for aid in debugging.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_sleep_lock(sleep_t *lockp)
{
	list_t *listp;

	debug_printf("sleep lock 0x%x\n", lockp);

	listp = &lockp->sl_list;
	debug_printf("list:   forward       back\n");
	debug_printf("        0x%x     0x%x\n", listp->flink, listp->rlink);
	while (listp->flink != (&lockp->sl_list)) {
		listp = (listp->flink);
		debug_printf("        0x%x     0x%x\n", 
			listp->flink, listp->rlink);
	}
	debug_printf("\n");
	
	debug_printf("\tsl_pend %d\tsl_hier %d\tsl_fspn 0x%x\tsl_flags 0x%x\n",
		lockp->sl_pend,
		lockp->sl_hierarchy,
		lockp->sl_fspin,
		lockp->sl_flags);

	debug_printf("\tsl_lwpid %d\tsl_avail %d\tsl_pid %d\tsl_lkstat 0x%x\n",
		lockp->sl_lwpid,
		lockp->sl_avail,
		lockp->sl_pid,
		lockp->sl_lkstatp);

	debug_printf("\tsl_fixpend %d\n",
		lockp->sl_fixpend);
}

#endif

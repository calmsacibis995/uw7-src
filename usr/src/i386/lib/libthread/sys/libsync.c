#ident	"@(#)libthread:i386/lib/libthread/sys/libsync.c	1.1.5.9"

#include <memory.h>
#include <libthread.h>

/* 
 * This file implements the interface between libc and libthread for
 * making the libc reentrant. The purposes of this interface is to: 
 *   1) initialize three pointers which exist in libc.
 *   2) Provide block and unblock functions for synchronizing some
 *      functions in libc.
 */

/* sleepq structure used by libc_mutex */
typedef struct libc_sleepq libc_sleepq_t;

struct libc_sleepq {
	lwp_mutex_t libc_sync_lock;	/* synchronization lock for sleepq */
	thrq_elt_t  libc_sleepq;	/* sleep queue */
	libc_sleepq_t *next;		/* pointer to the next */
	int	    filler[2];		/* not currently used */
};


/* free sleepq */
libc_sleepq_t *_thr_libc_free_mutex_sleepq = (libc_sleepq_t *)0;

lwp_mutex_t _thr_libc_mutex_sleepq_list_lock; /* lock to protect sleepq */

/* 
 * libc_mutex_t is the type used internally by the thread's library. The
 * size of this structure is 8 bytes and this cannot be changed unless the
 * the interface with libc changes.
 * The field lock of type _simplelock_t is a char for now. If this changes
 * to a short in the future, the next field 'unused' must be removed.
 * This structure is to be changed if the interface with libc changes.
 */

typedef volatile struct {
	_simplelock_t lock;	/* lock used by _lock_try() */
	char unused;		/* unused */
	char wanted_bound;	/* = 1 when bound threads are waiting */
	char wanted_mux;	/* = 1 when multiplexed threads are waiting */
	libc_sleepq_t *libc_sqp;/* pointer to the sleepq */
} libc_mutex_t;

/* macro to try to get the libc_lock */
#define LIBC_TRYLOCK(libc_lp) (!_libc_lock_try(&(libc_lp)->lock))

/* locks the sync lock that protects the sleepq */
#define LOCK_LIBC_SYNC_LOCK(lp) \
		 _lwp_mutex_lock(&(lp)->libc_sqp->libc_sync_lock);

/* unlocks the sync lock that protects the sleepq */
#define UNLOCK_LIBC_SYNC_LOCK(lp) \
		_lwp_mutex_unlock(&(lp)->libc_sqp->libc_sync_lock);

/* to see if the internal sleepq is allocated for the specified libc_mutex */
#define INTERNAL_LOCK_ALLOCATED(lock) (lock[1] != 0)

#define PRINTF3(f,t1,t2,t3)
#define PRINTF4(f,t1,t2,t3,t4)

void _thr_libc_alloc_sleepq(volatile int *, thread_desc_t *);

void _thr_libc_free_sleepq(libc_sleepq_t *);

int _libc_mutex_lock(libc_mutex_t *);

int _thr_remove_from_libc_queue(thread_desc_t *);

void _thr_dump_libc_lock(libc_mutex_t *, char *);


/*
 * _libc_mutex_lock(libc_mutex_t *libc_lp)
 *      obtains a libc_mutex; blocks if necessary
 *
 * Parameter/Calling State:
 *      argument is a pointer to a libc_mutex
 *
 * Return Values/Exit State:
 *      returns zero on success; otherwise, an appropriate errno value
 */

int
_libc_mutex_lock(libc_mutex_t *libc_lp)
{
	int rval;

	ASSERT(libc_lp != (libc_mutex_t *)NULL);
loop:
	/*
	 * Attempt to acquire the mutex.
	 */

	if (LIBC_TRYLOCK(libc_lp) == 0) {
		/*
		 * We got the mutex.
		 */
		PRINTF3("pid=%d, lwpid=%d: %s: 0",
			pid, lwpid, "_libc_mutex_lock");
		return (0);
	}

	/*
	 * We didn't get the mutex, call prepblock to place the calling LWP
	 * on the sync-queue associated with lmp.
	 * Prepblock will set lmp->wanted.
	 */
	while ((rval = prepblock((vaddr_t)libc_lp, 
			(char *)&libc_lp->wanted_bound,
				 PREPBLOCK_WRITER)) == EINVAL) {
		/*
		 * The calling LWP is placed on another sync-queue.
		 * Remove it from the sync-queue and prepblock again.
		 */
		PRINTF4("pid=%d, lwpid=%d: %s: prepblock=%d",
			pid, lwpid, "_libc_mutex_lock", rval);
		cancelblock();
		continue;
	}
	PRINTF4("pid=%d, lwpid=%d: %s: prepblock=%d",
		pid, lwpid, "_libc_mutex_lock", rval);
	if (rval != 0) {
		return (rval); /* EFAULT */
	}

	/*
	 * Attempt to acquire the mutex which may be released in the meantime.
	 * This attempt closes the window between _lock_try and prepblock.
	 */
	if (LIBC_TRYLOCK(libc_lp) == 0) {
		/*
		 * We got the mutex.
		 * Remove the calling LWP from the sync-queue.
		 */
		cancelblock();
		PRINTF3("pid=%d, lwpid=%d: %s: 0",
			pid, lwpid, "_libc_mutex_lock");
		return (0);
	}

	/*
	 * Give up a processor and wait for the mutex to be released.
	 */
	while ((rval = block((const timestruc_t *)0)) != 0) {
		PRINTF4("pid=%d, lwpid=%d: %s: block=%d",
			pid, lwpid, "_libc_mutex_lock", rval);

		switch (rval) {
		case EINTR:
		case ERESTART:
			/*
			 * Block was interrupted by a signal or a forkall.
			 * The calling LWP may be still placed on the
			 * sync-queue.  Call block again.
			 */
			continue;
		case ENOENT:
			/*
			 * Prepblock was canceled by a signal handler or a
			 * forkall, so that the calling LWP was removed from
			 * the sync-queue.  Try again.
			 */
			goto loop;
		default:
			PRINTF3("pid=%d, lwpid=%d: %s: block invalid value",
				pid, lwpid, "_libc_mutex_lock");
			break;
		}
		return (rval);
	}
	PRINTF4("pid=%d, lwpid=%d: %s: block=%d",
		pid, lwpid, "_libc_mutex_lock", rval);

	/*
	 * Try to acquire the mutex again.
	 */
	goto loop;

}

/*
 * void _thr_libcsync_block()
 *      obtains a libc_mutex; if the libc_mutex is held by another thread
 *      the function blocks until the libc_mutex is available. When the
 *      mutex is blocked, the appropriate wanted field (bound or multiplexed)
 *      is set, so that another thread that unlocks the libc_mutex, wakes up
 *      the calling thread.
 *
 * Parameter/Calling State:
 *      argument is a pointer to an array of two integers which can be
 *      cast to the type libc_mutex.
 *
 * Return Values/Exit State:
 *      returns none.
 */

void
_thr_libcsync_block(volatile int *lock)
{
	libc_mutex_t *libc_lock = (libc_mutex_t *)lock;
	thread_desc_t *t = curthread;

	ASSERT(lock != (int *)NULL);

#ifdef THR_DEBUG
	_thr_dump_libc_lock(libc_lock,"_thr_libcsync_block");
#endif

	if (LIBC_TRYLOCK(libc_lock) == 0)
		return;

	if (ISBOUND(t)) {
		_libc_mutex_lock(libc_lock);
		return;
	}

	if (!INTERNAL_LOCK_ALLOCATED(lock)) {
		_thr_libc_alloc_sleepq(lock, t);
	}

	/* multiplexed thread */
	while (LIBC_TRYLOCK(libc_lock) != 0) {
		_thr_sigoff(t);
		LOCK_THREAD(t);
		LOCK_LIBC_SYNC_LOCK(libc_lock);
		t->t_sync_addr = (void *)libc_lock;
		t->t_sync_type = TL_LIBC;
		_thrq_prio_ins(&libc_lock->libc_sqp->libc_sleepq, t);
		libc_lock->wanted_mux = 1;
		if (LIBC_TRYLOCK(libc_lock) == 0) {
			_thrq_rem_from_q(t);
			t->t_sync_addr = NULL;
			t->t_sync_type = TL_NONE;
			if ((THRQ_ISEMPTY((
					&libc_lock->libc_sqp->libc_sleepq))))
				libc_lock->wanted_mux = 0;
			UNLOCK_LIBC_SYNC_LOCK(libc_lock);
			UNLOCK_THREAD(t);
			_thr_sigon(t);
			break;
		}
		UNLOCK_LIBC_SYNC_LOCK(libc_lock);
		t->t_state = TS_SLEEP;
		_thr_swtch(1, t);
		ASSERT(t->t_state == TS_ONPROC);
		_thr_sigon(t);
	}
}

/*
 * void _thr_libcsync_unblock()
 *      unlocks a libc_mutex. If any threads are blocked waiting for the 
 *      libc_mutex, it awakens one waiting thread. Note that bound threads
 *      are given preference over multiplexed threads.
 *
 * Parameter/Calling State:
 *      argument is a pointer to an array of two integers which can be
 *      cast to the type libc_mutex.
 *
 * Return Values/Exit State:
 *      returns none.
 */

void
_thr_libcsync_unblock(volatile int *lock)
{
	libc_mutex_t *libc_lock = (libc_mutex_t *)lock;
	thread_desc_t *t = curthread;
	/* LINTED */
	int temp, needlwp;

	ASSERT(lock != (int *)NULL);	

#ifdef THR_DEBUG
	_thr_dump_libc_lock(libc_lock,"_thr_libcsync_unblock");
#endif

	if ((temp = libc_lock->wanted_bound) != 0) {
		/* bound thread is waiting */
		int rval;

        	rval = unblock((vaddr_t)libc_lock, 
		(char *)&(libc_lock->wanted_bound), UNBLOCK_ANY);
		if (rval != EINVAL) {
			return;
#ifdef THR_DEBUG
		} else {
			PRINTF2(
			  "unblock returned EINVAL...temp=%d,wanted_bound=%d\n",
				temp, libc_lock->wanted_bound);
#endif
		}
	}

	if (libc_lock->wanted_mux) { /* multiplexed thread is waiting */
		thread_desc_t *temp;

		_thr_sigoff(t);
		LOCK_LIBC_SYNC_LOCK(libc_lock);
		temp = _thrq_rem_first(&libc_lock->libc_sqp->libc_sleepq);
		if (temp != NULL) {
			if ((THRQ_ISEMPTY((
					&libc_lock->libc_sqp->libc_sleepq))))
				libc_lock->wanted_mux = 0;
		}
		UNLOCK_LIBC_SYNC_LOCK(libc_lock);
		if (temp != NULL) { /* found a thread */
			int prio;

			LOCK_THREAD(temp);
			if (THRQ_ISEMPTY(&temp->t_thrq_elt)) {
				temp->t_sync_addr = NULL;
				temp->t_sync_type = TL_NONE;
				needlwp = _thr_setrq(temp, 0);
			} else {
				needlwp = INVALID_PRIO;
			}
			UNLOCK_THREAD(temp);
			if (needlwp != INVALID_PRIO) {
				_thr_activate_lwp(needlwp);
			}
		}
		_thr_sigon(t);
	}
}

/*
 * void _thr_libcsync_init()
 *      Initializes the three pointers defined in the libc.
 *
 * Parameter/Calling State:
 *      No arguments are passed. It is called during the initialization of
 *      libc.
 *
 * Return Values/Exit State:
 *      returns none.
 */


void
_thr_libcsync_init(void)
{
	extern void (*_libc_block)(volatile int *);
	extern void (*_libc_unblock)(volatile int *);
	extern id_t (*_libc_self)(void);

	_libc_block = _thr_libcsync_block;
	_libc_unblock = _thr_libcsync_unblock;
	_libc_self = (id_t (*)(void))thr_self;
}


/*
 * void _thr_libc_alloc_sleepq()
 *	Allocate a libc_sleepq.
 *
 * Calling/Exit state:
 *	Called without holding any locks. During processing, the
 *      _thr_libc_mutex_sleepq_list_lock lock is obtained and released.
 *
 * Return Values/Exit State:
 *      returns none.
 */
void
_thr_libc_alloc_sleepq(volatile int *lock, thread_desc_t *t)
{
	libc_sleepq_t *lp;
	int chunksize;
	int i;
	
	ASSERT(lock != (int *)NULL);
	_thr_sigoff(t);
	_lwp_mutex_lock(&_thr_libc_mutex_sleepq_list_lock);
	if (INTERNAL_LOCK_ALLOCATED(lock)) {
		_lwp_mutex_unlock(&_thr_libc_mutex_sleepq_list_lock);
		_thr_sigon(t);
		return;
	}
	if ((lp = _thr_libc_free_mutex_sleepq) == (libc_sleepq_t *)0) {
		chunksize = (sizeof (libc_sleepq_t) + PAGESIZE - 1)
			    / PAGESIZE * PAGESIZE;
		if (_thr_alloc_chunk(0, chunksize, (caddr_t *)&lp) == 0) {
			_lwp_mutex_unlock(&_thr_libc_mutex_sleepq_list_lock);
			_thr_sigon(t);
			_thr_panic("_thr_libc_alloc_lock: _thr_alloc_chunk");
		}
		_thr_libc_free_mutex_sleepq = lp;
		for (i = 0; i < chunksize - (sizeof (libc_sleepq_t) * 2);
		     i += sizeof (libc_sleepq_t)) {
			lp->next = lp + 1;
			lp++;
		}
		ASSERT(lp->next == (libc_sleepq_t *)0);
		lp = _thr_libc_free_mutex_sleepq;
	}
	_thr_libc_free_mutex_sleepq = lp->next;
	lock[1] = (int)lp;
	_lwp_mutex_unlock(&_thr_libc_mutex_sleepq_list_lock);
	_thr_sigon(t);
}

/*
 * void _thr_libc_free_sleepq()
 *	Free a libc_sleepq.
 *
 * Calling/Exit state:
 *	Called without holding any locks. During processing, the
 *      _thr_libc_mutex_sleepq_list_lock lock is obtained and released.
 *
 * Return Values/Exit State:
 *      returns none.
 */

void
_thr_libc_free_sleepq(libc_sleepq_t *ldp)
{
	libc_sleepq_t *lp = (libc_sleepq_t *)ldp;
	thread_desc_t *t = curthread;

	ASSERT(ldp != (libc_sleepq_t *)NULL);
	_thr_sigoff(t);
	_lwp_mutex_lock(&_thr_libc_mutex_sleepq_list_lock);
	lp->next = _thr_libc_free_mutex_sleepq;
	_thr_libc_free_mutex_sleepq = lp;
	_lwp_mutex_unlock(&_thr_libc_mutex_sleepq_list_lock);
	_thr_sigon(t);
}

/*
 * int _thr_remove_from_libc_queue()
 *	Removes the specified thread from the libc_sleepq it is on.
 *
 * Calling/Exit state:
 *	Pointer to the thread is passed as an argument. The thread must be
 *	when this function is called. During processing, the sleepq lock is 
 *	obtained and released.
 *
 * Return Values/Exit State:
 *      returns 1, if the thread is removed from the queue, otherwise 0.
 */

int
_thr_remove_from_libc_queue(thread_desc_t *tp)
{
	int rval = 0;
	libc_mutex_t *libc_lp;
	
	ASSERT(tp != (thread_desc_t *)NULL);
	ASSERT(THR_ISSIGOFF(curthread));

	libc_lp = (libc_mutex_t *)tp->t_sync_addr;
	ASSERT(libc_lp != (libc_mutex_t *)NULL);

	LOCK_LIBC_SYNC_LOCK(libc_lp);
	if (!THRQ_ISEMPTY(&(tp->t_thrq_elt))) {
		_thrq_rem_from_q(tp);
		tp->t_sync_addr = NULL;
		tp->t_sync_type = TL_NONE;
		rval = 1;
	}
	UNLOCK_LIBC_SYNC_LOCK(libc_lp);
	return(rval);
}

/*
 * void thr_requeue_libc(thread_desc_t *tp, int prio);
 *	Repositions the specified thread on the libc_sleepq according
 *	to its new priority.
 *
 * Calling/Exit state:
 *      The thread lock of the calling thread is held.
 *	First argument is a pointer to the thread descriptor.
 *	Second argument is the new priority value.
 *	During processing, the sleepq lock is obtained and released.
 *
 * Return Values/Exit State:
 *	On exit the thread lock remains held.
 */

void
_thr_requeue_libc(thread_desc_t *tp, int prio)
{
	libc_mutex_t *libc_lp;
	
	ASSERT(tp != (thread_desc_t *)NULL);

	libc_lp = (libc_mutex_t *)tp->t_sync_addr;
	ASSERT(libc_lp != (libc_mutex_t *)NULL);

	LOCK_LIBC_SYNC_LOCK(libc_lp);
	tp->t_pri = prio;
	if (!THRQ_ISEMPTY(&(tp->t_thrq_elt))) {
		_thrq_rem_from_q(tp);
		_thrq_prio_ins(&libc_lp->libc_sqp->libc_sleepq, tp);
	}
	UNLOCK_LIBC_SYNC_LOCK(libc_lp);
	return;
}

void
_thr_dump_libc_lock(libc_mutex_t *lock, char *name)
{
	printf("%s:lock->lock=%d\n", name, lock->lock);
	printf("lock->wanted_bound=%d, lock->wanted_mux=%d\n", 
				lock->wanted_bound, lock->wanted_mux);
	printf("lock->libc_sqp=0x%x\n", lock->libc_sqp);

}

#ident	"@(#)libc-i386:sys/lwpmutex.c	1.2"
/*LINTLIBRARY*/
/*
 * LWP mutex lock
 */
#include <liblwpsynch.h>
#if defined(LWPSYNC_DEBUG)
#include <sys/types.h>
#endif

/*
 * int _lwp_mutex_trylock(lwp_mutex_t *lmp)
 *	Try to acquire an LWP mutex pointed to by lmp.
 *
 * Return Values/Exit State:
 *	0:	the LWP mutex was acquired.
 *		The LWP mutex pointed to by lmp is in locked state.
 *	EBUSY:	the LWP mutex was not available.
 *	SIGSEGV:lmp points outside the process's allocated address space.
 */
int
_lwp_mutex_trylock(lwp_mutex_t *lmp)
{
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	PRINTF4("pid=%d, lwpid=%d: %s: mutex=0x%x",
		pid, lwpid, "_lwp_mutex_trylock", lmp);
	if (_lock_try(&lmp->lock)) {
		/*
		 * We got the mutex.
		 */
		PRINTF3("pid=%d, lwpid=%d: %s: 0",
			pid, lwpid, "_lwp_mutex_trylock");
		return (0);
	}
	PRINTF3("pid=%d, lwpid=%d: %s: EBUSY",
		pid, lwpid, "_lwp_mutex_trylock");
	return (EBUSY);
}

/*
 * int _lwp_mutex_lock(lwp_mutex_t *lmp)
 *	Acquire an LWP mutex pointed to by lmp.
 *
 * Return Values/Exit State:
 *	0:	the LWP mutex was acquired.
 *		The LWP mutex pointed to by lmp is in locked state.
 *	EFAULT or SIGSEGV:
 *		lmp points outside the process's allocated address space.
 */
int
_lwp_mutex_lock(lwp_mutex_t *lmp)
{
	int rval;
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	PRINTF4("pid=%d, lwpid=%d: %s: mutex=0x%x",
		pid, lwpid, "_lwp_mutex_lock", lmp);
loop:
	/*
	 * Attempt to acquire the mutex.
	 */
	if (_lock_try(&lmp->lock)) {
		/*
		 * We got the mutex.
		 */
		PRINTF3("pid=%d, lwpid=%d: %s: 0",
			pid, lwpid, "_lwp_mutex_lock");
		return (0);
	}

	/*
	 * We didn't get the mutex, call prepblock to place the calling LWP
	 * on the sync-queue associated with lmp.
	 * Prepblock will set lmp->wanted.
	 */
	while ((rval = prepblock((vaddr_t)lmp, (char *)&lmp->wanted,
				 PREPBLOCK_WRITER)) == EINVAL) {
		/*
		 * The calling LWP is placed on another sync-queue.
		 * Remove it from the sync-queue and prepblock again.
		 */
		PRINTF4("pid=%d, lwpid=%d: %s: prepblock=%d",
			pid, lwpid, "_lwp_mutex_lock", rval);
		cancelblock();
		continue;
	}
	PRINTF4("pid=%d, lwpid=%d: %s: prepblock=%d",
		pid, lwpid, "_lwp_mutex_lock", rval);
	if (rval != 0) {
		return (rval); /* EFAULT */
	}

	/*
	 * Attempt to acquire the mutex which may be released in the meantime.
	 * This attempt closes the window between _lock_try and prepblock.
	 */
	if (_lock_try(&lmp->lock)) {
		/*
		 * We got the mutex.
		 * Remove the calling LWP from the sync-queue.
		 */
		cancelblock();
		PRINTF3("pid=%d, lwpid=%d: %s: 0",
			pid, lwpid, "_lwp_mutex_lock");
		return (0);
	}

	/*
	 * Give up a processor and wait for the mutex to be released.
	 */
	while ((rval = block((const timestruc_t *)0)) != 0) {
		PRINTF4("pid=%d, lwpid=%d: %s: block=%d",
			pid, lwpid, "_lwp_mutex_lock", rval);

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
				pid, lwpid, "_lwp_mutex_lock");
			break;
		}
		return (rval);
	}
	PRINTF4("pid=%d, lwpid=%d: %s: block=%d",
		pid, lwpid, "_lwp_mutex_lock", rval);

	/*
	 * Try to acquire the mutex again.
	 */
	goto loop;
}

/*
 * int _lwp_mutex_unlock(lwp_mutex_t *lmp)
 *	Release an LWP mutex pointed to by lmp.
 *
 * Return Values/Exit State:
 *	0:	the LWP mutex was released.
 *		The LWP mutex pointed to by lmp is in unlocked state.
 *	EFAULT or SIGSEGV:
 *		lmp points outside the process's allocated address space.
 */
int
_lwp_mutex_unlock(lwp_mutex_t *lmp)
{
	int rval;
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	PRINTF4("pid=%d, lwpid=%d: %s: mutex=0x%x",
		pid, lwpid, "_lwp_mutex_unlock", lmp);
	/*
	 * Release the mutex.
	 */
	rval = _lock_and_flag_clear(lmp);
	/*
	 * _lock_and_flag_clear() clears both the lock and the wanted flag
	 * and returns the previous value of the wanted flag.
	 * If the wanted flag was set, we call unblock().  By specifying
	 * the UNBLOCK_RESET flag to unblock(), we assure that the wanted
	 * flag will be properly reset.  If the wanted flag wasn't set, we
	 * don't have to do anything and can return immediately.
	 */
	if (rval == 0) {
		/*
		 * No LWPs are waiting.
		 */
		PRINTF3("pid=%d, lwpid=%d: %s: 0 (no waiters)",
			pid, lwpid, "_lwp_mutex_unlock");
		return (0);
	}

	/*
	 * Wake up one of the waiting LWPs.
	 * Unblock clears lmp->wanted if the sync-queue is emptied.
	 */
	rval = unblock((vaddr_t)lmp, (char *)&lmp->wanted, 
	   (UNBLOCK_ANY | UNBLOCK_RESET));
	PRINTF4("pid=%d, lwpid=%d: %s: unblock=%d",
		pid, lwpid, "_lwp_mutex_unlock", rval);
	if (rval == EINVAL) {
		/*
		 * The sync-queue associated with lmp does not exist.
		 * Ignore the error.
		 */
		rval = 0;
	} /* Otherwise, rval must be 0 or EFAULT */
	PRINTF4("pid=%d, lwpid=%d: %s: %d",
		pid, lwpid, "_lwp_mutex_unlock", rval);
	return (rval);
}

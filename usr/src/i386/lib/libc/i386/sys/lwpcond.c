#ident	"@(#)libc-i386:sys/lwpcond.c	1.1"
/*LINTLIBRARY*/
/*
 * LWP condition variable
 */
#include <liblwpsynch.h>
#if defined(LWPSYNC_DEBUG)
#include <sys/types.h>
#endif

/*
 * int _lwp_cond_signal(lwp_cond_t *lcp)
 *	Wakes up a single LWP waiting on an LWP condition variable pointed to
 *	by lcp if any.
 *
 * Return Values/Exit State:
 *	0:	successful completion
 *	EFAULT or SIGSEGV:
 *		lcp points outside the process's allocated address space.
 *
 *	Unblock system call sets lcp->wanted to zero when unblock emptied a
 *	sync-queue.
 */
int
_lwp_cond_signal(lwp_cond_t *lcp)
{
	int rval;
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	PRINTF4("pid=%d, lwpid=%d: %s: cond=0x%x",
		pid, lwpid, "_lwp_cond_signal", lcp);

	if (lcp->wanted == 0) {
		/*
		 * No LWPs are waiting.
		 */
		PRINTF3("pid=%d, lwpid=%d: %s: 0 (no waiters)",
			pid, lwpid, "_lwp_cond_signal");
		return (0);
	}

	/*
	 * Wake up one of the waiting LWPs.
	 * Unblock clears lcp->wanted if the sync-queue is emptied.
	 */
	rval = unblock((vaddr_t)lcp, (char *)&lcp->wanted, UNBLOCK_ANY);
	PRINTF4("pid=%d, lwpid=%d: %s: unblock=%d",
		pid, lwpid, "_lwp_cond_signal", rval);
	if (rval == EINVAL) {
		/*
		 * The sync-queue associated with lmp does not exist.
		 * Ignore the error.
		 */
		rval = 0;
	} /* Otherwise, rval must be 0 or EFAULT */
	PRINTF4("pid=%d, lwpid=%d: %s: %d",
		pid, lwpid, "_lwp_cond_signal", rval);
	return (rval);
}

/*
 * int _lwp_cond_broadcast(lwp_cond_t *lcp)
 *	Wakes up all the LWPs waiting on an LWP condition variable pointed to
 *	by lcp if any.
 *
 * Return Values/Exit State:
 *	0:	successful completion
 *	EFAULT or SIGSEGV:
 *		lcp points outside the process's allocated address space.
 *
 *	Unblock system call sets lcp->wanted to zero.
 */
int
_lwp_cond_broadcast(lwp_cond_t *lcp)
{
	int rval;
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	PRINTF4("pid=%d, lwpid=%d: %s: cond=0x%x",
		pid, lwpid, "_lwp_cond_broadcast", lcp);

	if (lcp->wanted == 0) {
		/*
		 * No LWPs are waiting.
		 */
		PRINTF3("pid=%d, lwpid=%d: %s: 0 (no waiters)",
			pid, lwpid, "_lwp_cond_broadcast");
		return (0);
	}

	/*
	 * Wake up all of the waiting LWPs.
	 * Unblock clears lcp->wanted.
	 */
	rval = unblock((vaddr_t)lcp, (char *)&lcp->wanted, UNBLOCK_ALL);
	PRINTF4("pid=%d, lwpid=%d: %s: unblock=%d",
		pid, lwpid, "_lwp_cond_broadcast", rval);
	if (rval == EINVAL) {
		/*
		 * The sync-queue associated with lmp does not exist.
		 * Ignore the error.
		 */
		rval = 0;
	} /* Otherwise, rval must be 0 or EFAULT */
	PRINTF4("pid=%d, lwpid=%d: %s: %d",
		pid, lwpid, "_lwp_cond_broadcast", rval);
	return (rval);
}

/*
 * int __lwp_cond_wait(lwp_cond_t *lcp, lwp_mutex_t *lmp,
 *		       const timestruc_t *abstime)
 *	Wait for the occurrence of a condition on an LWP condition variable
 *	pointed to by lcp.
 *
 *	If the absolute time specified by abstime has passed and the indicated
 *	condition is not signaled, __lwp_cond_wait returns ETIME.  If abstime
 *	is equal to zero, time out request is not set.
 *
 *	__lwp_cond_wait releases the LWP mutex pointed to by lmp before it
 *	calls block, and re-acquires the LWP mutex before it returns.
 *
 * Calling State:
 *	The LWP mutex pointed to by lmp must be in locked state.
 *
 * Return Values/Exit State:
 *	0:	Either _lwp_cond_signal or _lwp_cond_broadcast awakened up the
 *		LWP.
 *	ETIME:	Time specified by abstime has passed.
 *	EINVAL:	Time specified by abstime is invalid.
 *	EINTR:	A signal or a forkall system call interrupted the calling LWP.
 *	EFAULT or SIGSEGV:
 *		Either lcp, lmp or abstime point outside the process's
 *		allocated address space.
 *
 *	Prepblock system call sets lcp->wanted to non-zero value when prepblock
 *	places the first LWP on a sync-queue.
 *	Cancelblock system call sets lcp->wanted to zero when cancelblock
 *	emptied a sync-queue.
 *	The LWP mutex pointed to by lmp is in locked state.
 */
STATIC int
__lwp_cond_wait(lwp_cond_t *lcp, lwp_mutex_t *lmp, const timestruc_t *abstime)
{
	int rval;
	int rval2;
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	/*
	 * Call prepblock to place the calling LWP on the sync-queue
	 * associated with lcp.  Prepblock may set lcp->wanted.
	 */
	while ((rval = prepblock((vaddr_t)lcp, (char *)&lcp->wanted,
				 PREPBLOCK_WRITER)) == EINVAL) {
		/*
		 * The calling LWP is placed on another sync-queue.
		 * Remove it from the sync-queue and prepblock again.
		 */
		PRINTF4("pid=%d, lwpid=%d: %s: prepblock=%d",
			pid, lwpid, "__lwp_cond_wait", rval);
		cancelblock();
		continue;
	}
	PRINTF4("pid=%d, lwpid=%d: %s: prepblock=%d",
		pid, lwpid, "__lwp_cond_wait", rval);
	if (rval != 0) {
		return (rval); /* EFAULT */
	}

	/*
	 * Release the mutex.
	 */
	rval = _lwp_mutex_unlock(lmp);
	PRINTF4("pid=%d, lwpid=%d: %s: _lwp_mutex_unlock=%d",
		pid, lwpid, "__lwp_cond_wait", rval);

	if (rval != 0) {
		/*
		 * _lwp_mutex_unlock failed (EFAULT).
		 * Remove the calling LWP from the sync-queue, and return.
		 */
		cancelblock();
		return (rval);
	}

	/*
	 * Give up a processor and wait for being awakened.
	 */
	if ((rval = block(abstime)) != 0) {
		PRINTF4("pid=%d, lwpid=%d: %s: block=%d",
			pid, lwpid, "__lwp_cond_wait", rval);

		switch (rval) {
		case ERESTART:
			/*
			 * Block was interrupted by a signal or a forkall.
			 */
			rval = EINTR;
			/*FALLTHROUGH*/
		case EINTR:
			/* Block was interrupted by a signal. */
		case EFAULT:	/* abstime is invalid. */
		case EINVAL:	/* *abstime is invalid. */
			/*
			 * The calling LWP may be still placed on the
			 * sync-queue.  Remove it from the sync-queue.
			 */
			cancelblock();
			break;

		case ETIME:	/* *abstime has passed. */
			/*
			 * The calling LWP is not on the sync-queue.
			 */
			break;

		case ENOENT:
			/*
			 * Prepblock was canceled by a signal handler, or the
			 * calling LWP was in a child process created by a
			 * forkall, so that the calling LWP was removed from
			 * the sync-queue.
			 */
			rval = EINTR;
			break;

		default:
			PRINTF3("pid=%d, lwpid=%d: %s: block invalid value",
				pid, lwpid, "__lwp_cond_wait");
			break;
		}
	}

	/*
	 * Re-acquire the mutex.
	 */
	rval2 = _lwp_mutex_lock(lmp);
	PRINTF4("pid=%d, lwpid=%d: %s: _lwp_mutex_lock=%d",
		pid, lwpid, "__lwp_cond_wait", rval);

	if (rval2 != 0) {
		/*
		 * _lwp_mutex_unlock failed (EFAULT).
		 * Replace a return value with rval2 only if block returned 0
		 * or ETIME.
		 */
		if (rval == 0 || rval == ETIME) {
			rval = rval2;
		}
	}
	PRINTF4("pid=%d, lwpid=%d: %s: %d",
		pid, lwpid, "__lwp_cond_wait", rval);
	return (rval);
}

/*
 * int _lwp_cond_wait(lwp_cond_t *lcp, lwp_mutex_t *lmp)
 *	Wait for the occurrence of a condition on an LWP condition variable
 *	pointed to by lcp.
 *
 *	_lwp_cond_wait calls __lwp_cond_wait which would release an LWP
 *	mutex pointed to by lmp before calling block, and re-acquire an LWP
 *	mutex before returning.
 *
 * Calling State:
 *	The LWP mutex pointed to by lmp must be in locked state.
 *
 * Return Values/Exit State:
 *	0:	Either _lwp_cond_signal or _lwp_cond_broadcast awakened up the
 *		LWP.
 *	EINTR:	A signal or a forkall system call interrupted the calling LWP.
 *	EFAULT or SIGSEGV:
 *		Either lcp or lmp point outside the process's allocated address
 *		space.
 *
 *	The LWP condition variable pointed to by lcp may be modified.
 *	The LWP mutex pointed to by lmp is in locked state.
 */
int
_lwp_cond_wait(lwp_cond_t *lcp, lwp_mutex_t *lmp)
{
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	PRINTF5("pid=%d, lwpid=%d: %s: cond=0x%x, mutex=0x%x",
		pid, lwpid, "_lwp_cond_wait", lcp, lmp);

	return (__lwp_cond_wait(lcp, lmp, (const timestruc_t *)0));
}

/*
 * int _lwp_cond_timedwait(lwp_cond_t *lcp, lwp_mutex_t *lmp,
 *			   const timestruc_t *abstime)
 *	Wait for the occurrence of a condition on an LWP condition variable
 *	pointed to by lcp.
 *
 *	If the absolute time specified by abstime has passed and the indicated
 *	condition is not signaled, _lwp_cond_timedwait returns ETIME.
 *
 *	_lwp_cond_timedwait calls __lwp_cond_wait which would release an LWP
 *	mutex pointed to by lmp before calling block, and re-acquire an LWP
 *	mutex before returning.
 *
 * Calling State:
 *	The LWP mutex pointed to by lmp must be in locked state.
 *
 * Return Values/Exit State:
 *	0:	Either _lwp_cond_signal or _lwp_cond_broadcast awakened up the
 *		LWP.
 *	ETIME:	Time specified by abstime has passed.
 *	EINVAL:	Time specified by abstime is invalid.
 *	EINTR:	A signal or a forkall system call interrupted the calling LWP.
 *	EFAULT or SIGSEGV:
 *		Either lcp, lmp or abstime point outside the process's
 *		allocated address space.
 *
 *	The LWP condition variable pointed to by lcp may be modified.
 *	The LWP mutex pointed to by lmp is in locked state.
 */
int
_lwp_cond_timedwait(lwp_cond_t *lcp, lwp_mutex_t *lmp,
		    const timestruc_t *abstime)
{
	timestruc_t *timep;
	timestruc_t time;
#if defined(LWPSYNC_DEBUG)
	pid_t	pid = getpid();
	lwpid_t	lwpid = __lwp_self();
#endif

	PRINTF6("pid=%d, lwpid=%d: %s: cond=0x%x, mutex=0x%x, abstime=0x%x",
		pid, lwpid, "_lwp_cond_timedwait", lcp, lmp, abstime);

	if ((timep = (timestruc_t *)abstime) == (timestruc_t *)0) {
		/*
		 * We replace 0 with &time because __lwp_cond_wait ignores
		 * abstime which is equal to 0.
		 * Address 0 may be accessible, or it causes a SIGSEGV signal.
		 */
		PRINTF3("pid=%d, lwpid=%d: %s: abstime = 0",
			pid, lwpid, "_lwp_cond_timedwait");
		time = *abstime;
		timep = &time;
	}
	PRINTF5("pid=%d, lwpid=%d: %s: sec=%d, nsec=%d", pid, lwpid,
		"_lwp_cond_timedwait", timep->tv_sec, timep->tv_nsec);
	return (__lwp_cond_wait(lcp, lmp, (const timestruc_t *)timep));
}

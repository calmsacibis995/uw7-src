#ident	"@(#)libthread:i386/lib/libthread/sys/fork.c	1.1"

#include <trace.h>
#include <libthread.h>

#if !defined(ABI) && defined(__STDC__)
#pragma weak fork = _fork
#pragma weak forkall = _forkall
#endif /*  !defined(ABI) && defined(__STDC__) */

/*
 * pid_t
 * _fork(void)
 *	This is a wrapper of the _fork(2) system call.  It allows threads
 *	in the process to detect if _fork() is occurring or has occurred
 *	by manipulating global variables _thr_trace_forkcount and
 *	_thr_trace_pid.
 *
 * Parameter/Calling State:
 *      On entry, no locks are held; this is a user interface.
 *      During processing, if TRACE is defined, signal handlers are 
 *	disabled and _thr_trace_lock is acquired and released.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held and the return value is the same as
 *	for the _fork() system call.
 *
 */

pid_t
_fork(void)
{
	pid_t pid;

#ifdef TRACE
	thread_desc_t *ctp = curthread;
	_thr_sigoff(ctp);
	LOCK_TRACE;
	/*
	 * indicate fork is in progress;  if other threads detect this
	 * while trying to write a trace record, they will block on
	 * _thr_trace_cond to allow the fork to complete.
	 */
	_thr_trace_forkcount++;		
	UNLOCK_TRACE;
	/* 
	 * flush all file buffers; this prevents the child process from
	 * inheriting trace records in its trace buffer that were written 
	 * for the parent.
	 */
	fflush(0);
#endif
	pid = (*_sys_fork)();

#ifdef TRACE
	LOCK_TRACE;

	/*
	 * if this is the child process, we must update _thr_trace_pid,
	 * which is a global variable identifying the process ID.  This
	 * lets threads in the child process recognize that fork has
	 * occurred.
	 */
	if (pid == 0) {			
		/* Child process */
		_thr_trace_pid = getpid();
		PRINTF1("_fork: child process pid =%d\n", getpid());
	} else {
		/* Parent process (whether fork succeeded or not) */
		PRINTF1("_fork: parent process pid =%d\n", getpid());
	}
	/*
	 * indicate fork is complete and wake up waiting threads
	 */
	_thr_trace_forkcount--;			
	_lwp_cond_broadcast(&_thr_trace_cond);	
	UNLOCK_TRACE;
	_thr_sigon(ctp);
#endif

	return(pid);
}


/*
 * pid_t
 * _forkall(void)
 *	This is a wrapper of the _forkall(2) system call.  It allows threads
 *	in the process to detect if _forkall() is occurring or has occurred
 *	by manipulating global variables _thr_trace_forkcount and
 *	_thr_trace_pid.
 *
 * Parameter/Calling State:
 *      On entry, no locks are held; this is a user interface.
 *      During processing, if TRACE is defined, signal handlers are 
 *	disabled and _thr_trace_lock is acquired and released.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held and the return value is the same as
 *	for the _forkall() system call.
 *
 */

pid_t
_forkall(void)
{
	pid_t pid;

#ifdef TRACE
	thread_desc_t *ctp = curthread;
	_thr_sigoff(ctp);
	LOCK_TRACE;
	/*
	 * indicate fork is in progress;  if other threads detect this
	 * while trying to write a trace record, they will block on
	 * _thr_trace_cond to allow the fork to complete.
	 */
	_thr_trace_forkcount++;	
	UNLOCK_TRACE;

	/* 
	 * flush all file buffers; this prevents the child process from
	 * inheriting trace records in its trace buffer that were written 
	 * for the parent.
	 */
	fflush(0);
#endif
	pid = (*_sys_forkall)();

#ifdef TRACE
	LOCK_TRACE;

	/*
	 * if this is the child process, we must update _thr_trace_pid,
	 * which is a global variable identifying the process ID.  This
	 * lets threads in the child process recognize that fork has
	 * occurred.
	 */
	if (pid == 0) {
		/* Child process */
		_thr_trace_pid = getpid();
		PRINTF1("_forkall: child process pid =%d\n", getpid());
	} else {
		/* Parent process (whether fork succeeded or not) */
		PRINTF1("_forkall: parent process pid =%d\n", getpid());
	}

	/*
	 * indicate fork is complete and wake up waiting threads
	 */
	_thr_trace_forkcount--;
	_lwp_cond_broadcast(&_thr_trace_cond);
	UNLOCK_TRACE;
	_thr_sigon(ctp);
#endif

	return(pid);
}

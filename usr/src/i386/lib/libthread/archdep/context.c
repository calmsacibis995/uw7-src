#ident	"@(#)libthread:i386/lib/libthread/archdep/context.c	1.1.9.5"

#include <libthread.h>

/*
 * void
 * _thr_start(void *(*func)(void *arg), void *arg)
 *
 * This is thread's start up function.
 * It turns on the signal handlers and executes
 * the user-defined function passing to it an argument arg.
 * Upon return from user-defined function _thr_start() calls
 * thr_exit().
 * 
 * Parameter/Calling State:
 *      Signal handlers must be disabled upon entry.
 *
 * Return Values/Exit State:
 *	No locks are held, and signal handlers are enabled.
 *
 */


void
_thr_start(void *(*func)(void *arg), void *arg)
{
	void *rval;
	thread_desc_t *tp = curthread;
	int hashprio;

	/*
	 * before starting the thread, make sure that if it is a
	 * multiplexed thread, there is not a higher priority thread
	 * on the runnable queue.  A race condition exists in the
	 * preemption code which could strand a higher priority thread
	 * on the runnable queue while a lower priority thread is
	 * being placed on an LWP (since a search of the threads on
	 * LWPs may indicate that a higher priority thread was on this
	 * LWP just prior to switch).  This check helps prevent that race.
	 * This check is only for higher priority threads that are
	 * already on the runnable queue when this thread starts;
	 * threads placed on the runnable queue after this thread
	 * starts are not susceptible to this race condition.
	 *
	 * Note that at this point, signal handlers are disabled
	 * so _thr_sigoff() doesn't need to be called.
	 */
	/* if(!ISBOUND(tp) && (_thr_preempt_off != B_TRUE)) { */
	if(!ISBOUND(tp) && (_thr_preempt_ok == B_TRUE)) {
		HASH_PRIORITY(tp->t_pri, hashprio);
		if (hashprio < _thr_maxpriq) {
			LOCK_THREAD(tp);
			HASH_PRIORITY(tp->t_pri, hashprio);
			if (hashprio < _thr_maxpriq) {
				PREEMPT_SELF(tp);
			} else {
				UNLOCK_THREAD(tp);
			}
		}
	}

	_thr_sigon(tp);
        /* the startup flag is maintained for debuggers, so if
         * they grab a process they can tell whether the
         * existing threads have reached their startup routines
         * yet
         */
        tp->t_dbg_startup = 0;
	rval = func(arg);
	thr_exit(rval);
	/*NOTREACHED*/
}


/*
 * void
 * _thr_makecontext(thread_desc_t *tp, void *(*func)(void *arg), void *arg)
 *
 *	This function is called by thr_create() to initialize an uninitialized
 *	thread context structure based on the parameters passed in.
 *	The caller of this function must have created the stack prior to calling
 *	this function.
 *
 * Calling/Exit State:
 *	Signal handlers must be disabled upon entry.
 *
 * Return Values/Exit State:
 *	Thread context structure is fully initialized,
 *	and signal handlers are still disabled.
 *
 */


void
_thr_makecontext(thread_desc_t *tp, void *(*func)(void *arg), void *arg)
{
	int *sp;

	ASSERT(THR_ISSIGOFF(curthread));	
	ASSERT(tp != NULL);
	ASSERT(func != NULL);
	ASSERT(tp->t_sp != NULL);

	tp->t_pc = (greg_t)_thr_start;  /*pc*/
	tp->t_ucontext.uc_mcontext.gregs[R_EBP] = (greg_t)0;  /*frame_pointer*/

	sp = (int *)(tp->t_sp);
	*--sp = (int)arg;	/* the second argument to _thr_start() */
	*--sp = (int)func;	/* the first argument to _thr_start() */
	*--sp = (int)thr_exit;	/* return address */
	tp->t_sp = (greg_t)sp;  /*stack pointer*/

}


/*
 * void
 * _thr_make_idle_context(thread_desc_t *tp)
 *
 *      This function is called by _thr_idle_thread_create() to initialize 
 *	an uninitialized thread context structure.
 *
 * Calling/Exit State:
 *      Signal handlers must be disabled upon entry.
 *
 * Return Values/Exit State:
 *      Thread context structure is fully initialized,
 *      and signal handlers are still disabled.
 *
 */

void
_thr_make_idle_context(thread_desc_t *tp)
{
	int *sp;
	
	ASSERT(THR_ISSIGOFF(curthread));	
	ASSERT(tp != NULL);	
	ASSERT(tp->t_sp != NULL);
	
	tp->t_pc = (greg_t)_thr_age;  /*pc*/
	tp->t_ucontext.uc_mcontext.gregs[R_EBP] = (greg_t)0;  /*frame_pointer*/

	sp = (int *)(tp->t_sp);
	*--sp = (int)thr_exit;	/* return address */
	tp->t_sp = (greg_t)sp;  /*stack pointer*/

}

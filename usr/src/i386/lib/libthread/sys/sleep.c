#ident	"@(#)libthread:i386/lib/libthread/sys/sleep.c	1.1.3.1"

/*LINTLIBRARY*/

#if !defined(ABI) && defined(__STDC__)
	#pragma weak sleep = _sleep
#endif
#include "synonyms.h"
#include <synch.h>
#include <errno.h>
#include <libthread.h>

#define HALF_SEC	500000000

/*
 * unsigned
 * _sleep(unsigned seconds)
 *
 *      This function is a wrapper of the sleep() function call to
 *      suspend execution of a thread for number of seconds.
 *      If the calling thread is a bound thread, the function blocks on
 *     	an lwp condition variable. 
 *      If the calling thread is a multiplexed thread, the function
 *	blocks on a thread library condition variable.
 *
 * Parameter/Calling State:
 *      On entry, no locks are held.
 *      During processing, signal handlers may be disabled and
 *      the callout queue lock may be acquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held.
 */

unsigned
_sleep(unsigned seconds)
{

	struct timeval	     clocktime;
	timestruc_t	     abs_time;
	int 	             rval;
	thread_desc_t *ctp = curthread;
	lwp_mutex_t  sleep_mutex_bound = {0};
	lwp_cond_t   sleep_cond_bound = {0};
#ifdef NOTYET
	mutex_t      sleep_mutex_mux = {0};
	cond_t       sleep_cond_mux = {0};
#endif

	gettimeofday(&clocktime, (struct timezone *) NULL);
	abs_time.tv_sec = clocktime.tv_sec + seconds;
	abs_time.tv_nsec = clocktime.tv_usec * (NANOSEC/MICROSEC);
	if (abs_time.tv_sec < clocktime.tv_sec) { /* check overflow */
		return (seconds);
	}
#ifdef NOTYET
	if (ISBOUND(ctp)) {
		/*
		 * For a bound thread the t_flags of the thread descriptor are
		 * used to determine if sleep was interrupted by SIGWAITING or
		 * SIGLWP signal.  Flag T_SIGWAITUP is cleared before calling
		 * _lwp_cond_timedwait().  If one of these signals is generated
		 * during the wait, the flag T_SIGWAITUP will be set and
		 * _lwp_cond_timedwait()  will be called again.  If the return
		 * value for _lwp_cond_timedwait() is EINTR and the flag
		 * T_SIGWAITUP is cleared, then it must be an interrupt from
		 * the user and we get out of this loop.
		 */
#endif
		_lwp_mutex_lock(&sleep_mutex_bound);
		do {
			_thr_sigoff(ctp);
			LOCK_THREAD(ctp);
			ctp->t_flags &= ~T_SIGWAITUP;
			ctp->t_flags |= T_PREPBLOCKED;
			UNLOCK_THREAD(ctp);
			_thr_sigon(ctp);
			rval = _lwp_cond_timedwait(&sleep_cond_bound,
						   &sleep_mutex_bound,
						   &abs_time);
			_thr_sigoff(ctp);
			LOCK_THREAD(ctp);
			ctp->t_flags &= ~T_PREPBLOCKED;
			UNLOCK_THREAD(ctp);
			_thr_sigon(ctp);
		} while (rval == EINTR && (ctp->t_flags & T_SIGWAITUP));
		_lwp_mutex_unlock(&sleep_mutex_bound);

#ifdef NOTYET
	} else {
		/*
		 * In the case of a multiplexed thread block thread in the 
		 * library on cond_t.
		 */

		_THR_MUTEX_LOCK(&sleep_mutex_mux);
		do {
			ctp->t_flags &= ~T_SIGWAITUP;
			rval = _thr_cond_wait(&sleep_cond_mux, &sleep_mutex_mux,
					      (lwp_mutex_t *) NULL, &abs_time,
					      B_TRUE, B_TRUE);
		} while (rval == EINTR && (ctp->t_flags & T_SIGWAITUP));

		_THR_MUTEX_UNLOCK(&sleep_mutex_mux);
	}
#endif
	if (rval == ETIME) {
		return (0);
	}
	PRINTF("sleep: returns prematurely\n");	
	/*
	 * return unslept amount
	 */
	gettimeofday(&clocktime, (struct timezone *) NULL);
	if (abs_time.tv_nsec - (clocktime.tv_usec * (NANOSEC/MICROSEC))
	    >= HALF_SEC) {
		/*
		 * round up to 1 sec.
		 */
		abs_time.tv_sec += 1;
	}
	if (abs_time.tv_sec <= clocktime.tv_sec) {
		return (0);
	}
	return ((unsigned)(abs_time.tv_sec - clocktime.tv_sec));
}

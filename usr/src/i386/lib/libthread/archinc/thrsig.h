#ident	"@(#)libthread:i386/lib/libthread/archinc/thrsig.h	1.1.1.4"

#define PREEMPTED(tp)   (tp->t_flags & T_PREEMPT)

/*
 * macro definitions for signal operations. must include limits.h.
 * (would functions be more efficient?)
 */
#if MAXSIG <= WORD_BIT	/* for sa_sigbits[1] in a sigset_t */
#define _thr_sigorset(set, hold) \
                (set)->sa_sigbits[0] |= (hold)->sa_sigbits[0]; 

#define _thr_sigdiffset(set, hold) \
                (set)->sa_sigbits[0] &= ~ (hold)->sa_sigbits[0];

#define _thr_sigandset(set, hold) \
                (set)->sa_sigbits[0] &=  (hold)->sa_sigbits[0];

#define _thr_sigisempty(set) \
                (! (set)->sa_sigbits[0])

#define _thr_sigcmpset(x, y) ((x)->sa_sigbits[0] != (y)->sa_sigbits[0])

#define _thr_sigisset(pending, hold) \
                ((pending)->sa_sigbits[0] & ~(hold)->sa_sigbits[0])

#elif MAXSIG <= 2 * WORD_BIT	/* for sa_sigbits[2] in a sigset_t */
#define _thr_sigorset(set, hold) \
                (set)->sa_sigbits[0] |= (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] |= (hold)->sa_sigbits[1];

#define _thr_sigdiffset(set, hold) \
                (set)->sa_sigbits[0] &= ~ (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] &= ~ (hold)->sa_sigbits[1];

#define _thr_sigandset(set, hold) \
                (set)->sa_sigbits[0] &=  (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] &=  (hold)->sa_sigbits[1];

#define _thr_sigisempty(set) \
                (! ((set)->sa_sigbits[0] |  (set)->sa_sigbits[1]))

#define _thr_sigcmpset(x, y) (((x)->sa_sigbits[0] != (y)->sa_sigbits[0]) || \
                         ((x)->sa_sigbits[1] != (y)->sa_sigbits[1]))

#define _thr_sigisset(pending, hold) \
                (((pending)->sa_sigbits[0] & ~(hold)->sa_sigbits[0]) || \
                ((pending)->sa_sigbits[1] & ~(hold)->sa_sigbits[1]))

#elif MAXSIG <= 3 * WORD_BIT	/* for sa_sigbits[3] in a sigset_t */
#define _thr_sigorset(set, hold) \
                (set)->sa_sigbits[0] |= (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] |= (hold)->sa_sigbits[1]; \
                (set)->sa_sigbits[2] |= (hold)->sa_sigbits[2];

#define _thr_sigdiffset(set, hold) \
                (set)->sa_sigbits[0] &= ~ (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] &= ~ (hold)->sa_sigbits[1]; \
                (set)->sa_sigbits[2] &= ~ (hold)->sa_sigbits[2];

#define _thr_sigandset(set, hold) \
                (set)->sa_sigbits[0] &=  (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] &=  (hold)->sa_sigbits[1]; \
                (set)->sa_sigbits[2] &=  (hold)->sa_sigbits[2];

#define _thr_sigisempty(set) \
                (! ((set)->sa_sigbits[0] | (set)->sa_sigbits[1] | (set)->sa_sigbits[2]))

#define _thr_sigcmpset(x, y) (((x)->sa_sigbits[0] != (y)->sa_sigbits[0]) || \
                         ((x)->sa_sigbits[1] != (y)->sa_sigbits[1]) || \
                         ((x)->sa_sigbits[2] != (y)->sa_sigbits[2]))

#define _thr_sigisset(pending, hold) \
                (((pending)->sa_sigbits[0] & ~(hold)->sa_sigbits[0]) || \
                ((pending)->sa_sigbits[1] & ~(hold)->sa_sigbits[1]) || \
                ((pending)->sa_sigbits[2] & ~(hold)->sa_sigbits[2]))

#elif MAXSIG <= 4 * WORD_BIT	/* for sa_sigbits[4] in a sigset_t */
#define _thr_sigorset(set, hold) \
                (set)->sa_sigbits[0] |= (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] |= (hold)->sa_sigbits[1]; \
                (set)->sa_sigbits[2] |= (hold)->sa_sigbits[2]; \
                (set)->sa_sigbits[3] |= (hold)->sa_sigbits[3];

#define _thr_sigdiffset(set, hold) \
                (set)->sa_sigbits[0] &= ~ (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] &= ~ (hold)->sa_sigbits[1]; \
                (set)->sa_sigbits[2] &= ~ (hold)->sa_sigbits[2]; \
                (set)->sa_sigbits[3] &= ~ (hold)->sa_sigbits[3];

#define _thr_sigandset(set, hold) \
                (set)->sa_sigbits[0] &=  (hold)->sa_sigbits[0]; \
                (set)->sa_sigbits[1] &=  (hold)->sa_sigbits[1]; \
                (set)->sa_sigbits[2] &=  (hold)->sa_sigbits[2]; \
                (set)->sa_sigbits[3] &=  (hold)->sa_sigbits[3];

#define _thr_sigisempty(set) \
                (! ((set)->sa_sigbits[0] | (set)->sa_sigbits[1] | (set)->sa_sigbits[2] | (set)->sa_sigbits[3]))

#define _thr_sigcmpset(x, y) (((x)->sa_sigbits[0] != (y)->sa_sigbits[0]) || \
                         ((x)->sa_sigbits[1] != (y)->sa_sigbits[1]) || \
                         ((x)->sa_sigbits[2] != (y)->sa_sigbits[2]) || \
                         ((x)->sa_sigbits[3] != (y)->sa_sigbits[3]))

#define _thr_sigisset(pending, hold) \
                (((pending)->sa_sigbits[0] & ~(hold)->sa_sigbits[0]) || \
                ((pending)->sa_sigbits[1] & ~(hold)->sa_sigbits[1]) || \
                ((pending)->sa_sigbits[2] & ~(hold)->sa_sigbits[2]) || \
                ((pending)->sa_sigbits[3] & ~(hold)->sa_sigbits[3]))
#endif

#define THR_SIGMASK_OFF(tp) \
	{\
		boolean_t repeat;\
		do { \
			repeat = B_FALSE;\
			_thr_sigoff(tp);\
		 	LOCK_THREAD(tp);\
			(*_sys_sigprocmask)(SIG_SETMASK, &_thr_sig_allmask, NULL);\
			if (tp->t_sig != 0) {\
				UNLOCK_THREAD(tp);\
				_thr_sigon(tp);\
				repeat = B_TRUE;\
			}\
		} while (repeat == B_TRUE);\
	}

/*
 * void
 * _thr_sigoff(tp)
 *
 * 	_thr_sigoff() is a macro to disable signal handlers.
 *	The macro is called by a Threads Library function before it
 *	enters a critical section.
 *	Signal handlers are enabled when the thread exits the critical
 *	section by calling _thr_sigon().
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, no locks are held.
 * Return Values/Exit State:
 *	On exit, no locks are held, signal handlers are disabled.
 */

#define _thr_sigoff(tp)\
	ASSERT((tp)->t_nosig >= 0);\
	PRINTF5("_thr_sigoff: thread: %d, lwp = %d, nosig: %d, file: %s, line:%d\n", (tp)->t_tid, LWPID(tp), (tp)->t_nosig, __FILE__, __LINE__);\
	(tp)->t_nosig++

/*
 * void
 * _thr_sigon(tp)
 *
 *	_thr_sigon() is a macro called on exit from a critical section.
 *	The macro enables signal handlers and if a signal was 
 *	delivered at a critical section it calls a low level function 
 *	to dispatch the signal.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and no locks are held.
 *	During processing, signal handlers are disabled and the calling
 *	thread lock and _thr_siguinfolock locks are acquired.
 *
 * Return Values/Exit State:
 *	On exit, if it is the last exit from a critical section
 *	(t_nosig == 0) signal handlers are enabled. No locks are held.
 */

#define _thr_sigon(tp) \
	ASSERT((tp)->t_nosig > 0);\
	PRINTF6("_thr_sigon: thread: %d, lwp = %d, nosig: %d, t_sig: %d, file: %s, line:%d\n", (tp)->t_tid, LWPID(tp), (tp)->t_nosig, (tp)->t_sig, __FILE__, __LINE__);\
	if (--(tp)->t_nosig == 0 && (tp)->t_sig) {\
 		_thr_sigx((tp), B_TRUE);\
	}

#define _thr_tsigon(tp) \
	ASSERT((tp)->t_nosig > 0);\
	PRINTF6("_thr_tsigon: thread: %d, lwp = %d, nosig: %d, t_sig: %d, file: %s, line:%d\n", (tp)->t_tid, LWPID(tp), (tp)->t_nosig, (tp)->t_sig, __FILE__, __LINE__);\
	if (--(tp)->t_nosig == 0 && (tp)->t_sig) {\
 		_thr_sigx((tp), B_FALSE);\
	}

#define THR_ISSIGOFF(tp) ((tp)->t_nosig > 0)
#define THR_ISSIGON(tp) ((tp)->t_nosig == 0)

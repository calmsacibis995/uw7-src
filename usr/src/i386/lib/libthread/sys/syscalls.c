#ident	"@(#)libthread:i386/lib/libthread/sys/syscalls.c	1.6.3.1"

#include <sys/reg.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <ucontext.h>
#include <stdlib.h>
#include <libthread.h>


/*
 * highbit_table used by _thr_hibit to determine the highest bit set.
 */

static const unsigned char    highbit_table[256] = {
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/* 
 * calls _thr_resetsig to redispatch set of thread's pending signals to 
 * the currently running LWP.
 */
void _thr_xresetsig(thread_desc_t *, boolean_t); 

/*
 * int
 * _thr_hibit(int i)
 *
 *      This function returns an integer between 1 and 32
 *      corresponding to the highest set bit in i.
 *      This function is used to determine which subqueue
 *      of the runnable queue has the highest priority thread
 *      on it.
 *
 *      This is done by determining the highest 8 bits, i.e.,
 *      whether the first 8 bits, the second 8 bits, etc.,
 *      of i which are non-zero.  It then determines which
 *      is the highest order bit set among those 8 bits using
 *      the highbit_table shown above which translates the
 *      value of the 8 bits to show the highest set bit.  It
 *      then adds on the number of bits in the 8 bit quarters
 *      of lower magnitude.
 *
 * Parameter/Calling State:
 *      No locks need to be held on entry; however, this is
 *      called while holding _thr_counterlock and with signal
 *      handlers disabled.
 *
 *      No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *      This function returns an integer between 1 and 32
 *      corresponding to the highest set bit in i.
 *      The returned value is normally decremented by 1 in
 *      order to match the appropriate array index.
 */
int
_thr_hibit(register unsigned long  i)
{
	register unsigned long j;
	register unsigned long k;


	/* check if no bits are set */
	if (i == 0) {
		return (0);
	}
	/*
	 * Check if any of the 16 high bits are set, else if any of the 3rd 8
	 * high bits are set.
	 */
	if ((j = (i >> 16)) != 0) { 
		/*
		 * Check if any of the 8 high bits are set, or if some of the
		 * 2nd 8 high bits are set.
		 */
		if ((k = (j >> 8)) != 0) {
			return (highbit_table[k] + 24);
		}		
		return (highbit_table[j] + 16);

	}
	if ((k = (i >> 8)) != 0) {
		return (highbit_table[k] + 8);
	}
	/* Some of the 8 low bits are set. */
	return (highbit_table[i]); 
}

/*
 * void
 * _init_sys_calls()
 *
 *	_init_sys_calls() retrieves the addresses of the 
 *	functions in libc.so.1, that will be called by
 *	the wrapper functions and the Threads Library in general. For example,
 *	sighold() wrapper has to call _sigprocmask(), _sigprocmask() is called
 *	by calling (*_sys_sigprocmask)().
 *	This has to be done during initialization.
 */
int (*_sys_sigprocmask) (int, const sigset_t *, sigset_t *);
int (*_sys_sigaction) (int, const struct sigaction *, struct sigaction *);
int (*_sys__sigaction) (int, const struct sigaction *, struct sigaction *, void (*)(int, siginfo_t *, ucontext_t *, void (*)()));
int (*_sys_sigsuspend) (const sigset_t *);
int (*_sys_sigpause) (int);
int (*_sys_sigpending) (sigset_t *);
int (*_sys_sigwait) (sigset_t *);
int (*_sys_setcontext) (ucontext_t *);
unsigned (*_sys_alarm) (unsigned);
int (*_sys_setitimer) (int, struct itimerval *, struct itimerval *);
int (*_sys_getitimer) (int, struct itimerval *);
pid_t (*_sys_forkall) (void);
pid_t (*_sys_fork) (void);
int (*_sys_chroot) (const char *);
int (*_sys_close) (int);

void
_init_sys_calls()
{
	void *hlibc;	/* handle for access to libc.so.1 */

	
	hlibc = (void *)_dlopen("/usr/lib/libc.so.1", RTLD_LAZY);
	if (hlibc == NULL) {
		printf("dlopen failed: %s\n", _dlerror());
		exit(1);
	}
	_sys_sigprocmask = (int (*) (int, const sigset_t *, sigset_t *)) 
		_dlsym(hlibc, "_sigprocmask"); 
	_sys_sigaction = (int (*) (int, const struct sigaction *, 
		struct sigaction *)) _dlsym(hlibc, "_sigaction");
	_sys__sigaction = (int (*) (int, const struct sigaction *, 
		struct sigaction *, void (*)(int, siginfo_t *, ucontext_t *, 
		void (*)()))) _dlsym(hlibc, "__sigaction");
	_sys_setcontext = (int (*) (ucontext_t *)) _dlsym(hlibc, "_setcontext");
	_sys_sigsuspend = (int (*) (const sigset_t *)) _dlsym(hlibc, "_sigsuspend");
	_sys_sigpause = (int (*) (int)) _dlsym(hlibc, "_sigpause");
	_sys_sigpending = (int (*) (sigset_t *)) _dlsym(hlibc, "_sigpending");
	_sys_sigwait = (int (*) (sigset_t *)) _dlsym(hlibc, "_sigwait");

	_sys_alarm = (unsigned (*) (unsigned)) _dlsym(hlibc,"_alarm");
	_sys_setitimer = (int (*) (int, struct itimerval *, struct itimerval *)) _dlsym(hlibc, "_setitimer");
	_sys_getitimer = (int (*) (int, struct itimerval *)) _dlsym(hlibc, "_getitimer");
	_sys_fork = (pid_t (*) (void)) _dlsym(hlibc, "_fork");
	_sys_forkall = (pid_t (*) (void)) _dlsym(hlibc, "_forkall");
	_sys_chroot = (int (*) (const char *)) _dlsym(hlibc, "_chroot");
	_sys_close = (int (*) (int)) _dlsym(hlibc, "_close");

#ifdef THR_DEBUG
	PRINTF1(" *** _init_sys_calls(): func _sys_sigprocmask addr 0x%x\n", _sys_sigprocmask);
	PRINTF1(" *** _init_sys_calls(): func _sys_sigaction addr 0x%x\n", _sys_sigaction);
	PRINTF1(" *** _init_sys_calls(): func _sys__sigaction addr 0x%x\n", _sys__sigaction);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_setcontext addr 0x%x\n", _sys_setcontext);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_sigsuspend addr 0x%x\n", _sys_sigsuspend);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_sigpause addr 0x%x\n", _sys_sigpause);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_sigpending addr 0x%x\n", _sys_sigpending);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_sigwait addr 0x%x\n", _sys_sigwait);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_fork addr 0x%x\n", _sys_fork);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_forkall addr 0x%x\n", _sys_forkall);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_chroot addr 0x%x\n", _sys_chroot);
	PRINTF1(" *** _init_sig_sys_calls(): func _sys_close addr 0x%x\n", _sys_close);
#endif /* THR_DEBUG */
}

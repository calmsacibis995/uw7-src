#ident	"@(#)libthread:i386/lib/libthread/archdep/machdep.c	1.4.12.10"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#define NOTHREAD

#ifdef TRACE_INTERNAL
#define NOVPID
#endif

#include <sys/reg.h>
#include <memory.h>
#include <libthread.h>

STATIC struct tls _thr_firsttls;	/* tls for premordial thread t0  */


/* Should be in header files. */
extern void __thr_multithread(__lwp_desc_t *);
extern void _init_sys_calls(void);

/*
 * int 
 * _thr_alloc_tls(caddr_t stk, int stksize, thread_desc_t **t)
 *	This function allocates a stack if the application hasn't provided
 *	one, creates the TLS within the stack, and initializes the TLS fields.
 *
 * Parameter/Calling State:
 *	Caller's signal handlers must be disabled upon entry.
 *	
 * Calling/Exit state:
 *	Caller's signal handlers are still disabled on exit.
 *	The TLS and stack fields in the thread descriptor are initialized.
 *	
 */
int
_thr_alloc_tls(caddr_t stk, size_t stksize, thread_desc_t **t)
{
	caddr_t tls;
	caddr_t stktop;

	ASSERT(THR_ISSIGOFF(curthread));

	PRINTF4("_thr_alloc_tls: stk=0x%x, stksize=0x%x, t=0x%x, *t=0x%x\n",
	      stk, stksize, t, *t);

	if ((stktop = stk) == NULL) {
		if (stksize == 0)
			stksize = DEFAULTSTACK;
		if (!_thr_alloc_stack(stksize, &stktop))
			return (0);
	}
	tls = stktop + stksize - SA((int)_thr_tls_size());
	*t =  &((struct tls *)tls)->tls_thread;
	memset(*t, 0, sizeof (thread_desc_t));
	if (stk == NULL) {
		(*t)->t_flags |= T_ALLOCSTK;
	}
	(*t)->t_tls = tls;
	(*t)->t_sp = (long)(tls - SA(MINFRAME));
	(*t)->t_stk = stktop;
	(*t)->t_stksize = stksize;
	((struct tls *)tls)->tls_thread_desc.thr_errp =
		&((struct tls *)tls)->tls_thread_desc.thr_errno;

#if defined(TRACE_INTERNAL)
	*( (int *)(tls + (int)&_thr_vpid - (int)_thr_tls())) = -1;
	*( (int *)(tls + (int)&_thr_vlwpid - (int)_thr_tls())) = -1;

	_THR_MUTEX_LOCK(&_thr_tid_lock);
	*((int *)(tls + (int)&_thr_tid - (int)_thr_tls()))= ++_thr_last_tid;
	_THR_MUTEX_UNLOCK(&_thr_tid_lock);
       /* will be set later when this thread calls trace() for the first time */
#endif
	return (1);
}

#if defined(sparc)
#define FUNC_TO_PC(func) ((uint)(func) - 8)
#else
#define FUNC_TO_PC(func) ((uint)(func))
#endif

thread_desc_t *_thr_t0;		/* primordial thread */
thread_desc_t *_thr_i0;		/* primordial lwp's idle thread */
char _thr_i0stk[DEFAULTSTACK];	/* i0's stack,later allocate dynamically */
lwppriv_block_t _thr_t0_lwpp;   /* initial LWP's private area */


/*
 * We do this to avoid confusion with the multithreaded definition of errno.
 * Here we want the int itself, to initialize the primordial thead.
 */

#undef errno
extern int errno;

/*
 * void _thr_t0init(void)
 *	This function is called by _thr_init() to initialize 
 *	the primordial thread and turn it into a real thread. 
 *	This function is called only once at start up time.
 *
 * Parameter/Calling State:
 *	No locks are held on entry.
 *
 * Return Values/Exit State:
 *	The primordial thread and its LWP are initialized.
 */
void
_thr_t0init(void)
{
	struct tls *tls = &_thr_firsttls;
	caddr_t stk;
	caddr_t i0tls;
	lwpid_t lwpid;

	/* initialize an idle thread */
	PRINTF2("_thr_t0init:_lwp_getprivate=0x%x,curthread=0x%x\n",
			_lwp_getprivate(), curthread);

	lwpid = _lwp_self();

	/*
	 * The initial thread's errno is the old, static one.  We delay calling
	 * _thr_multithread until after this initialization to make sure we get
	 * the address of the static.
	 */

	tls->tls_thread_desc.thr_errp = &errno;
	__thr_multithread(&_thr_t0_lwpp.l_lwpdesc);
	_thr_t0_lwpp.l_lwpdesc.lwp_id = lwpid;
	_thr_t0_lwpp.l_lwpdesc.lwp_thread = &tls->tls_thread_desc;

	stk = (caddr_t)(((int)(_thr_i0stk + DEFAULTSTACK)) & ~(STACK_ALIGN-1));
	i0tls = (caddr_t)(stk - SA((int)_thr_tls_size()));
	_thr_i0 = &((struct tls *)i0tls)->tls_thread;

	PRINTF1("_thr_t0init: stk = 0x%x\n", stk);
	PRINTF1("_thr_t0init: i0tls = 0x%x\n", i0tls);
	PRINTF1("_thr_t0init: &_thr_thread = 0x%x\n", &_thr_thread);
	PRINTF1("_thr_t0init: _thr_tls = 0x%x\n", _thr_tls());
	PRINTF1("_thr_t0init: _thr_i0 = 0x%x\n", _thr_i0);

	memset((caddr_t)_thr_i0, '\0', sizeof (thread_desc_t));
	_thr_i0->t_tls = i0tls;
	((struct tls *)i0tls)->tls_thread_desc.thr_errp =
		&((struct tls *)i0tls)->tls_thread_desc.thr_errno;
	_thr_i0->t_stk = (void *)_thr_i0stk;
	_thr_i0->t_stksize = DEFAULTSTACK;
	_thr_i0->t_sp = (long)(i0tls - SA(MINFRAME));
	_thr_i0->t_pc = (greg_t)_thr_age;
	_thr_i0->t_lwpp = &_thr_t0_lwpp;
	_thr_i0->t_state = TS_RUNNABLE;
	_thr_i0->t_pri = -1;
	_thr_i0->t_tid = -1;
	_thr_i0->t_usingfpu = B_FALSE;
	_thr_i0->t_idle =  _thr_i0;
	_thr_i0->t_nosig = 1;
	_thr_idlethreads = _thr_i0;
	_thr_i0->t_prev = _thr_i0->t_next = _thr_i0;
	sigfillset(&_thr_i0->t_hold);
        _thr_sigdiffset(&_thr_i0->t_hold, &_thr_sig_programerror);

	/* initialize the primordial thread */
	_thr_t0 = &((struct tls *)tls)->tls_thread;
	_thr_tidvec[0].thr_ptr = (thread_desc_t *)NULL;
	_thr_tidvec[0].id_gen = 0;
	_thr_tidvec[1].thr_ptr = (thread_desc_t *) _thr_t0;
	_thr_tidvec[1].id_gen = 0;
	_thr_total_tids = 1;
	
	memset((caddr_t)_thr_t0, '\0', sizeof (thread_desc_t));
	_thr_t0->t_tls = (char *)tls;
	_thr_t0->t_sp = (long)(tls - SA(MINFRAME));

	_thr_t0->t_state = TS_ONPROC;
	_thr_t0->t_usingfpu = B_FALSE;
	_thr_t0->t_pri = DEFAULTMUXPRI;
	_thr_t0->t_tid = 1;

	_thr_t0->t_lwpp = &_thr_t0_lwpp;
	_thr_t0->t_nosig = 1;
	_thr_t0->t_idle = _thr_i0;

	_thr_t0->t_next = _thr_t0->t_prev = _thr_t0;
	_thr_allthreads = _thr_t0;
        _THR_SEMA_INIT(&_thr_house_sema, 0, USYNC_THREAD, NULL);

	/*
	 * _init_sys_calls() call caches system calls addresses
	 * so that system calls can be trapped by the thread library.
	 */

	_init_sys_calls();

	_thr_sigt0init(_thr_t0);
	_thr_sigon(_thr_t0);
}

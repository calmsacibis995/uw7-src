#ident	"@(#)libthread:i386/lib/libthread/sys/lwpmakectxt.c	1.2.6.4"

/* @(#)libc-esmp:sys/lwpmakectxt.c	1.2 */

#include <sys/types.h>
#include <sys/user.h>
#include <sys/ucontext.h>
#include <sys/signal.h>
#include <stdio.h>
#include <libthread.h>

extern int _getcontext(ucontext_t *);

/*
 * void
 * _lwp_makecontext(ucontext_t *ucp, void (*func)(void *arg), void *arg,
 *		    void *private, caddr_t stackbase, size_t stacksize)
 *
 * 	Initializes an uninitialilzed context structure based on the parameters
 * 	passed in. 
 *
 * Calling/Exit state:
 *
 *	The context structure pointed to by ucp is fully initialized on exit.
 *
 * Remarks:
 *	_lwp_makecontext() is expected to be used to create a context structure
 *	that can be passed to the _lwp_create() system call. Other fields 
 *	of the context structure not specified in the _lwp_makecontext()
 *	interface will be initialized from the current context.
 */
void
_lwp_make_context(ucontext_t *ucp, void (*func)(void *arg), void *arg, 
		 void *private, thread_desc_t *tp)
{
	int *sp;
	
        ASSERT(tp->t_stk != NULL);
        ASSERT(tp->t_stksize >= THR_MIN_STACK);
        ASSERT(private != NULL);

	/*
	 * Initialize the passed-in context with the current context
	 * first.
	 */
	(void) _getcontext(ucp);

	/*
	 * Now fix up the passed in context.
	 */
	ucp->uc_flags = UC_SIGMASK|UC_STACK|UC_CPU;
	ucp->uc_stack.ss_sp = (char *)tp->t_stk;
	ucp->uc_stack.ss_size = (int)tp->t_stksize;
	ucp->uc_stack.ss_flags &= ~SS_ONSTACK;

	ucp->uc_mcontext.gregs[ EIP ] = (int)_thr_start;
	ucp->uc_privatedatap = private;

	sp = (int *)tp->t_sp;
	/*
	 * Save the context that we should return to on the stack.
	 */
	*--sp = (int)(arg);
	*--sp = (int)func;
	*--sp = (int)thr_exit;		/* return address */
	tp->t_sp = (greg_t)sp;
	ucp->uc_mcontext.gregs[ UESP ] = (int)sp;
}

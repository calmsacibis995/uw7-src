#ident	"@(#)libc-i386:sys/lwpmakectxt.c	1.2"

#ifdef __STDC__
	#pragma weak _lwp_makecontext = __lwp_makecontext
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/user.h>
#include <sys/ucontext.h>
#include <sys/signal.h>

#define NULL 0

/*
 * void
 * _lwp_makecontext(ucontext_t *ucp, void (*func)(void *arg), void *arg,
 *		    void *private, caddr_t stackbase, size_t stacksize)
 *
 * 	Initializes an uninitialilzed context structure based on the parameters
 * 	passed in. 
 *
 * Calling/Exit State:
 *	The context structure pointed to by ucp is fully initialized.
 *
 * Remarks:
 *	_lwp_makecontext() is expected to be used to create a context structure
 *	that can be passed to the _lwp_create() system call. Other fields 
 *	of the context structure not specified in the _lwp_makecontext()
 *	interface will be initialized from the current context.
 */

void
#ifdef	__STDC__

_lwp_makecontext(ucontext_t *ucp, void (*func)(void *arg), void *arg, 
		 void *private, caddr_t stackbase, size_t stacksize)

#else

_lwp_makecontext(ucp, func, arg, private, stackbase, stacksize)
	ucontext_t *ucp; 
	void (*func)(); 
	void *arg; 
	void *private; 
	caddr_t stackbase;
	size_t stacksize;
	int	getcontext();

#endif
{
	int *sp;
	int *argp;
	static void set_old_context();

	/*
	 * Initialize the passed-in context with the current context
	 * first.
	 */
	(void) getcontext(ucp);

	/*
	 * Now fix up the passed in context.
	 */
	ucp->uc_flags = UC_ALL;
	ucp->uc_stack.ss_sp = (char *)stackbase;
	ucp->uc_stack.ss_size = (int)stacksize;
	ucp->uc_stack.ss_flags &= ~SS_ONSTACK;

	ucp->uc_mcontext.gregs[ EIP ] = (ulong)func;
	ucp->uc_privatedatap = private;

	sp = (int *)(stackbase + stacksize);
	/*
	 * Save the context that we should return to on the stack.
	 */
	*--sp = (int)(ucp->uc_link);
	*--sp = (int)arg;
	*--sp = (int)set_old_context;		/* return address */
	ucp->uc_mcontext.gregs[ UESP ] = (ulong)sp;
}

/*
 * void
 * set_old_context()
 *	This functions resumes execution of the "correct" context.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	This function is executed when the context created by the 
 *	_lwp_makecontext() function returns. This function transfers control 
 *	to the context pointed to by the uc_link field of the context that is
 *	returning. 
 */

static void
set_old_context()
{
	int	retval;
	ucontext_t uc;
	int *sp;
	int __getcontext();
	int  setcontext();

	/*
	 * We had stored the return context on the stack that we made in
	 * _lwp_makecontext() function. Get that stack; the assumption here is
	 * that user has not changed this!
	 */
	(void) __getcontext(&uc);
	sp = ((int *)((char *)uc.uc_stack.ss_sp + uc.uc_stack.ss_size)) - 1;
	if ((*(ucontext_t **)sp) == NULL) {
		/*
		 * We have no context to return to; exit.
		 */
		_lwp_exit();

		/* NOTREACHED */
	} 
	if ( (retval = setcontext(*(ucontext_t **)sp)) == -1) {
		/*
		 * We have no context to return to; exit.
		 */
		_lwp_exit();

		/* NOTREACHED */
	}

	/* NOTREACHED */
}

#ident	"@(#)libthread:i386/lib/libthread/sys/lwp_create.c	1.5.7.3"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#include <memory.h>
#include <libthread.h>

/*
 * int _thr_lwpcreate(thread_desc_t *t, long retpc, caddr_t sp,
 *		      void (*func)(void *), void *arg, int flags) 
 *
 *	Create an LWP.
 *
 * Calling/Exit state:
 *
 *	Called without holding any locks.
 */
/*ARGSUSED*/
int
_thr_lwpcreate(thread_desc_t *t, long retpc, caddr_t sp,
	   void *(*func)(void *), void *arg, int flags)
{
	ucontext_t uc;
	lwppriv_block_t *lwpp;
	lwpid_t newlwp;
	int rval;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(t != NULL);
	ASSERT(retpc != NULL);
	ASSERT(sp != NULL);

	lwpp = (lwppriv_block_t *)_thr_lwp_allocprivate();

	ASSERT(lwpp != NULL);

	memset((caddr_t)&uc, 0, sizeof (ucontext_t));
	t->t_lwpp = lwpp;
	lwpp->l_lwpdesc.lwp_thread = (__thread_desc_t *)t->t_tls;
	_lwp_make_context(&uc, (void (*)(void *))func, arg, lwpp, t);
        uc.uc_sigmask = t->t_hold;
	if ((rval = _lwp_create(&uc, flags, &newlwp)) != 0) {
		_thr_lwp_freeprivate((__lwp_desc_t *)lwpp);
		PRINTF1("_thr_lwpcreate: _lwp_create error %d\n", rval);
		return (rval);
	}
	lwpp->l_lwpdesc.lwp_id = newlwp;
	return (rval);
}

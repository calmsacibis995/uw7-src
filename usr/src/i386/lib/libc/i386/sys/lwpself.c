#ident	"@(#)libc-i386:sys/lwpself.c	1.2"
#include <thread.h>

lwpid_t
_lwp_self(void)
{
#ifdef _REENTRANT
	extern __lwp_desc_t **__lwp_priv_datap;

	if (__lwp_priv_datap == 0)
		__thr_init();

	if ((*__lwp_priv_datap)->lwp_id == 0) {
		/* not yet initialized */
		(*__lwp_priv_datap)->lwp_id = __lwp_self();
	}
	return ((*__lwp_priv_datap)->lwp_id);
#else
	return __lwp_self();
#endif
}

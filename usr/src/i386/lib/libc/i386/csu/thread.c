#ident	"@(#)libc-i386:csu/thread.c	1.10"
/*
 * Thread initialization and errno retrieval. Here we define an lwp
 * descriptor, used to implement lwp_self in single-threaded programs.
 * In multithreaded programs, libthread substitutes another lwp
 * descriptor and provides a thread descriptor.
 */
#pragma weak __errno = __thr_errno
#pragma weak ___errno = __thr_errno /* "they" changed their minds */

#include <thread.h>

int __multithreaded;	/* set below by libthread startup */

#undef errno		/* just in case */
extern int errno;

#ifdef _REENTRANT

extern void *__lwp_private(void *);

__lwp_desc_t **__lwp_priv_datap;	/* lwp-private data area */
__lwp_desc_t __lwp_first;		/* descriptor for first lwp */

/*
 * __thr_init()
 *	Initialize the lwp-private data area pointer and such.
 *
 */
void
__thr_init(void)
{
	__lwp_priv_datap = (__lwp_desc_t **)__lwp_private((void *)&__lwp_first);
}

/*
 * __thr_multithreaded()
 *	Entry point for libthread startup to tell libc that we are
 *	multithreaded, and to slide new lwp descriptor under us.
 *	Caller must initialize lwpp.
 */
void
__thr_multithread(__lwp_desc_t *lwpp)
{
	__lwp_priv_datap = (__lwp_desc_t **)__lwp_private((void *)lwpp);
	__multithreaded = 1;
}

/*
 * __thr_errno()
 *	Return a pointer to this thread's private errno.
 */
int *
__thr_errno(void)
{
	if (!__multithreaded)
		return &errno;
	return (*__lwp_priv_datap)->lwp_thread->thr_errp;
}

#else /*!_REENTRANT*/

int *
__thr_errno(void)
{
	return &errno;
}

#endif /*_REENTRANT*/

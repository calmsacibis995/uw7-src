#ident	"@(#)libc-i386:sys/lwpprivate.c	1.2"

/*
 * _lwp_setprivate and _lwp_getprivate.
 */

#ifndef _REENTRANT
static void *saved_lwp_ptr;
#endif

/*
 * void _lwp_setprivate(void *p)
 *	Set the private data pointer of the calling lwp.
 */
void
_lwp_setprivate(void *p)
{
#ifdef _REENTRANT
	extern  void **__lwp_priv_datap;

	if (__lwp_priv_datap == 0)
		__thr_init();
#else
	saved_lwp_ptr = p; /* save it ourselves for later "get" */
#endif
	(void)__lwp_private(p);
}

/*
 * void *_lwp_getprivate(void)
 *	Return the private data pointer of the calling LWP.
 */
void *
_lwp_getprivate(void)
{
#ifdef _REENTRANT
	extern  void **__lwp_priv_datap;

	if (__lwp_priv_datap == 0)
		__thr_init();

	return (*__lwp_priv_datap);
#else
	return saved_lwp_ptr; /* might be a null pointer */
#endif
}

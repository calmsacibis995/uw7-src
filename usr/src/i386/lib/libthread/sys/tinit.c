#ident	"@(#)libthread:i386/lib/libthread/sys/tinit.c	1.5.3.1"

#include <libthread.h>

/*
 * void _init(void)
 *
 *	Initialize the thread library.
 *
 * Calling/Exit state:
 *
 *	Called without holding any locks.
 *
 * Remarks:
 *	_init() is the very first function in libthread that is called
 *	by libc (e.g., from callInit()). Before reaching main() in the
 *	application code, many other libthread functions are called such
 *	as _thr_init, __thr_init, _lwp_getprivate, _thr_t0init, and
 *	_thr_sigt0init. (The function, __thr_init, is actually defined
 *	in libc.)
 */

/* weak alias used by the dynamic linker to get the order
 * of inits right for certain system libraries
 */
#pragma weak __libthread_init = _init

void
_init(void)
{
	_thr_init();
}


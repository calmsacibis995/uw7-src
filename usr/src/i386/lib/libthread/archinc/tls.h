#if !defined(_TLS_H)
#define _TLS_H
#ident	"@(#)libthread:i386/lib/libthread/archinc/tls.h	1.2.3.3"

#include <thread.h>

/*
 * Thread's local storage structure
 */
struct tls {
	__thread_desc_t tls_thread_desc; /* thread's private area; see  __thread_desc_t */
	thread_desc_t tls_thread;	/* thread descriptor	*/
	int	tls_vlwpid;		/* virtual lwp id	*/
	int	tls_vpid;		/* virtual process id	*/
	int	tls_tid;		/* thread id		*/
};

#if defined(THR_DEBUG)
#define STATIC
#else
#define STATIC static
#endif


/*
 * curtls() is a macro to obtain pointer to caller's tls.
 */

#define curtls() ((struct tls *)(((__lwp_desc_t *)_lwp_getprivate())->lwp_thread))

/*
 * Definitions of variables in a thread local storage
 * for the system whose cc does not support #pragma unshared.
 */

#if !defined(TLS)
#define _thr_thread	(*(thread_desc_t *)(_thr_tls_thread()))
#define _thr_vpid	(*(int *)(_thr_tls_vpid()))
#define _thr_vlwpid	(*(int *)(_thr_tls_vlwpid()))
#define _thr_tid	(*(int *)(_thr_tls_tid()))

#define _thr_tls() 		((void *)curtls())
#define _thr_tls_thread() 	((void *)(&curtls()->tls_thread))
#define _thr_tls_vpid() 	((void *)(&curtls()->tls_vpid))
#define _thr_tls_vlwpid() 	((void *)(&curtls()->tls_vlwpid))
#define _thr_tls_tid()		((void *)(&curtls()->tls_tid))
#define _thr_tls_size() 	(sizeof (struct tls))

#endif /* !defined(TLS) */
#endif /* !defined(_TLS_H) */

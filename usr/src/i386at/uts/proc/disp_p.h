#ifndef _PROC_DISP_P_H	/* wrapper symbol for kernel use */
#define _PROC_DISP_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:proc/disp_p.h	1.12.3.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h> /* PORTABILITY */
#include <svc/trap.h> /* PORTABILITY */
#include <svc/intr.h> /* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ipl.h> /* PORTABILITY */
#include <sys/trap.h> /* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

/*
 * Values for l.eventflags. The values for posting scheduling requests
 * are defined in ksynch.h to avoid header nesting.
 * 
 */

#define EVT_STRSCHED	0x04	/* streams scheduler needs to be run */
#define EVT_GLOBCALLOUT	0x08	/* global callout processing pending */
#define EVT_LCLCALLOUT	0x10	/* local callout processing pending */

#define EVT_SOFTINTMASK	(EVT_STRSCHED|EVT_GLOBCALLOUT|EVT_LCLCALLOUT)

/*
 * "User" preemption point.
 */
#define	UPREEMPT()	\
	if (l.eventflags & (EVT_RUNRUN|EVT_SOFTINTMASK)) { \
		if (l.eventflags & EVT_SOFTINTMASK) { \
			ASSERT(getpl() == PLBASE); \
			(void)spltimeout(); \
			softint_hdlr(); \
			spl0(); \
		} \
		if (l.eventflags & EVT_RUNRUN) \
			CL_PREEMPT(u.u_lwpp, u.u_lwpp->l_cllwpp); \
	}

#define EVT_UPREEMPT	(EVT_RUNRUN|EVT_SOFTINTMASK)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_DISP_P_H */

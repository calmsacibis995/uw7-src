#ifndef	_UTIL_KCONTEXT_H	/* wrapper symbol for kernel use */
#define	_UTIL_KCONTEXT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kcontext.h	1.10"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <proc/regset.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <sys/regset.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined _KERNEL || defined _KMEMUSER

/*
 * LWP saved register context.  Given the context switch routines may
 * be called by C routines which don't allocate a frame, it's no longer
 * OK for "save" to leave the SP where the caller doesn't expect it.
 */
typedef struct kcontext {
	uint_t		kctx_esp;	/* stack pointer */
	uint_t		kctx_ebx;
	uint_t		kctx_ebp;	/* frame pointer */
	uint_t		kctx_esi;
	uint_t		kctx_edi;
	uint_t		kctx_eip;	/* pc */
	uint_t		kctx_eax;	/* need this to make save work correctly */
	uint_t		kctx_ecx;	/* only used by panic */
	uint_t		kctx_edx;	/* only used by panic */
	uint_t		kctx_fs;
	uint_t		kctx_gs;
	fpregset_t	kctx_fpregs;	/* FPU and FPA state */
	char		kctx_fpvalid;	/* set if LWP has used FPU */
	char		kctx_weitek;	/* set if LWP has used weitek FPA */
	char		kctx_debugon;	/* flag set if LWP using debug regs */
	dbregset_t	kctx_dbregs;	/* H/W debug registers */
} kcontext_t;

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KCONTEXT_H */

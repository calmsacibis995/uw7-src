#ifndef _PROC_UCONTEXT_H	/* wrapper symbol for kernel use */
#define _PROC_UCONTEXT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/ucontext.h	1.1.2.4"
#ident	"$Header$"


#if defined(__cplusplus)
extern "C" {

#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/regset.h>	/* REQUIRED */
#include <proc/signal.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/regset.h>	/* REQUIRED */
#include <sys/signal.h>	/* REQUIRED */

#else

#include <sys/types.h> /* SVR4.0COMPAT */
#include <sys/regset.h> /* SVR4.0COMPAT */

#if !(defined( _XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)

#include <sys/signal.h> /* SVR4.0COMPAT */

#endif /* !(defined( _XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/

/* XPG4 V2 definitions */

#ifndef _SIGSET_T
#define _SIGSET_T
typedef struct {		/* signal set type */
	unsigned int	sa_sigbits[4];
} sigset_t;
#endif

#ifndef _STACK_T
#define _STACK_T
typedef struct
#if !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
		sigaltstack
#endif
{
	void	*ss_sp;
	size_t	ss_size;
	int	ss_flags;
} stack_t;
#endif

#endif /* _KERNEL_HEADERS */

typedef struct {
#ifdef gregs
	gregset_t	__gregs;	/* general register set */
#else
	gregset_t	gregs;	/* general register set */
#endif
#ifdef fpregs
	fpregset_t 	__fpregs;	/* floating point register set */
#else
	fpregset_t 	fpregs;	/* floating point register set */
#endif
} mcontext_t;

#if !(defined( _XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
typedef struct ucontext
#else
typedef struct __ucontext
#endif
{
#ifdef uc_flags
	ulong_t		__uc_flags;
#else
	ulong_t		uc_flags;
#endif
#ifdef uc_link
	struct ucontext_t	*__uc_link;
#else
#if !(defined( _XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
	struct ucontext	*uc_link;
#else
	struct __ucontext	*uc_link;
#endif
#endif
#ifdef uc_sigmask
	sigset_t   	__uc_sigmask;
#else
	sigset_t   	uc_sigmask;
#endif
#ifdef uc_stack
	stack_t 	__uc_stack;
#else
	stack_t 	uc_stack;
#endif
#ifdef uc_mcontext
	mcontext_t 	__uc_mcontext;
#else
	mcontext_t 	uc_mcontext;
#endif
#ifdef uc_privatedatap
	void		*__uc_privatedatap;
#else
	void		*uc_privatedatap;
#endif
#ifdef uc_filler
	long		__uc_filler[4];		/* pad the structureto 512 bytes */
#else
	long		uc_filler[4];		/* pad the structureto 512 bytes */
#endif
} ucontext_t;


#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)

#define GETCONTEXT	0
#define SETCONTEXT	1

/* 
 * values for uc_flags
 * these are implementation dependent flags, that should be hidden
 * from the user interface, defining which elements of ucontext
 * are valid, and should be restored on call to setcontext
 */

#define	UC_SIGMASK	001
#define	UC_STACK	002
#define	UC_CPU		004
#define	UC_FP		010


#define UC_MCONTEXT (UC_CPU|UC_FP)

/* 
 * UC_ALL specifies the default context
 */

#define UC_ALL		(UC_SIGMASK|UC_STACK|UC_MCONTEXT)

#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/

#ifdef _KERNEL

void savecontext(ucontext_t *, k_sigset_t);
void restorecontext(ucontext_t *);

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif
#endif /* _PROC_UCONTEXT_H */

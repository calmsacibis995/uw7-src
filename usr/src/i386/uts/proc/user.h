#ifndef _PROC_USER_H	/* wrapper symbol for kernel use */
#define _PROC_USER_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/user.h	1.51.6.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <mem/faultcatch.h>	/* REQUIRED */
#include <mem/seg_map_u.h>	/* REQUIRED */
#include <proc/seg.h>		/* REQUIRED */
#include <proc/siginfo.h>	/* REQUIRED */
#include <proc/signal.h>	/* REQUIRED */
#include <svc/fp.h>		/* REQUIRED */
#include <svc/reg.h>		/* REQUIRED */
#include <util/kcontext.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/faultcatch.h>	/* REQUIRED */
#include <vm/seg_map_u.h>	/* REQUIRED */
#include <sys/seg.h>		/* REQUIRED */
#include <sys/siginfo.h>	/* REQUIRED */
#include <sys/signal.h>		/* REQUIRED */
#include <sys/fp.h>		/* REQUIRED */
#include <sys/reg.h>		/* REQUIRED */
#include <sys/kcontext.h>	/* REQUIRED */
#include <sys/param.h>		/* SVR4.0COMPAT */
#include <sys/types.h>		/* REQUIRED */
#ifdef CC_PARTIAL
#include <sys/covert.h>		/* REQUIRED */
#endif

#endif /* _KERNEL_HEADERS */

/*
 * The user block is (USIZE * PAGESIZE) bytes long.
 * Inside of the user block is a kernel stack and the user-structure (u.).
 * On the i386, the kernel stack comes first, followed by the user-structure
 * at the very end of the user block.
 */


#define MAXSYSARGS	8	/* Maximum # of arguments passed to a syscall */ 


/* flags for u_sigflag field */
#define SOMASK 		0x001	/* use u_sigoldmask on return from signal */

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct user {
#ifdef MERGE386
	void *u_vm86p;		/* MERGE386 data pointer */
#endif

	kcontext_t u_kcontext;	/* context, or info to find context of lwp */

	struct fpemul_restart u_fpe_restart;	/* FP emulator restart state */

	char	u_acflag;	/* LWP accounting flags */

	char	u_sigfault;	/* set when general protection violations due */
				/* to pgm corruption of the context save area */
				/* during signal handling are to be caught */
	char	u_sigfailed;	/* set when general protection violation was */
				/* due to pgm corruption of the context save */
				/* area during signal handling */

	char	u_flags;	/* special flags for the context*/

	/*
	 * Info on how to handle failed memory faults encountered by the kernel.
	 * This replaces u_caddrflt.  See <mem/faultcatch.h>.
	 */
	fault_catch_t   u_fault_catch;

	int	u_traptype;		/* trap: remembered trap type */
	int	u_syscall;		/* syscall: system call number */
	int	*u_ar0;			/* syscall: address of user's save R0 */
	int	u_arg[MAXSYSARGS];	/* syscall: current system call args */

	label_t	u_qsav;			/* syscall: longjmp label for signals */
					/*  (used by dkibind for old drivers) */

	k_siginfo_t u_siginfo;		/* /proc: stop on fault */

	struct ucontext *u_oldcontext;	/* previous user context */

	struct itimerval *u_italarm[2];	/* timers: ptrs to the alarms for */
					/*         the clocks measuring user */
					/*	   LWP virtual time */
					/*	   (SIGVTALRM) and */
					/*	   user+system (SIGPROF) */

	int	u_sigflag;		/* signals: per-LWP signal flags */
	stack_t u_sigaltstack;		/* signals: sp & on-stack state */
	k_sigset_t u_sigoldmask;	/* signals: old sigheld for sigsuspend*/

        vaddr_t	u_stkbase;              /* LWP's notion of signal stack base */
        u_int	u_stksize;              /* LWP's notion of signal stack size */


	struct rlimits *u_rlimits;	/* LWP view of proc resource limits */

	dl_t	u_ior;			/* #of block reads */
	dl_t	u_iow;			/* #of block writes */
	dl_t	u_ioch;			/* #of bytes read/written */

	struct proc *u_procp;		/* pointer to proc structure */
	struct lwp *u_lwpp;		/* pointer to LWP structure */

	void	*u_privatedatap;	/* LWP private data */

	void	*u_kse_ptep;		/* pointer to stack extension PTE */

	/* information about private GDT and LDT. */
	struct desctab u_gdt_desc;
	struct segment_desc u_ldt_desc;
	struct desctab_info *u_dt_infop[NDESCTAB];

	void	*u_cfgp;			/* driver config entry */
	char	u_pad1[4];			/* padding */
	struct gate_desc reserved;	/* Was FP intr gate descriptor image */
					/* Now available		     */

	boolean_t u_fp_used;		/* FP used since last setcontext;
					   made visible to user in
					   uvwin.uv_fp_used */

	void (*(*u_physio_start)())();	/* if non-NULL, special processing
					   for physio transfers */

	ulong_t *u_kpgflt_stack;	/* esp at kernel pageflt */

	/*
	 * Per-context storage for segmap; a context may hold at most
	 * one segmap chunk active at a time.
	 */
	struct segmap_u_data u_segmap_data;

	/*
	 * Driver compatibility: support for seterror().
	 */
	int u_compat_errno;

#ifdef CC_PARTIAL
	covert_t u_covert;		/* covert channel event info */
#endif /* CC_PARTIAL */

#ifdef DEBUG
	uint_t		u_debugflags;	/* various flags to support testing */
#define FAIL_KMEM_ALLOC	0x0001		/* fail kmem_alloc(..., KM_NOSLEEP) */
#define FAIL_KPG_VM_ALLOC 0x0002	/* fail kpg_vm_alloc(..., NOSLEEP) */
#define NO_PAGEIO_DONE	0x0004		/* pageio_done() becomes a no-op */
#endif	/* DEBUG */

	void	*u_load_handle;		/* handle of loadable kernel module */

	int	u_rm_mode;		/* resmgr transaction owned by proc */
} user_t;

#define u_ap	u_arg

/* flag values for NFA */
#define NFA_SERVER	0x4000	/* the NFA network server */
#define NFA_CASELESS	0x8000	/* caseless support for DOS */

/* u_flags values */
#define U_CRITICAL_MEM	(1 << 0)	/* critical process */

/*
 * Reserved space at base of kernel stack, in bytes.
 */
#ifdef MERGE386
/*
 * MERGE386 needs 16 extra bytes to turn a normal protected-mode kernel entry
 * into a V86 mode return frame, to start a process running in V86 mode.
 * Note that this is only needed on stacks associated with user LWPs.
 */
#define KSTACK_RESERVE	16

/*
 * Merge function offset into the u_vm86p table.
 */
#define	MRG_SWTCHAWAY_OFF	0
#define MRG_SWTCHTO_OFF		1
#define MRG_CHKINTS_OFF		2
#define MRG_LWPEXIT_OFF		3

#else /* !MERGE386 */

#define KSTACK_RESERVE	0

#endif /* MERGE386 */

/*
 * Platform dependent macro to set the value of u_ar0.
 * This macro has knowledge of the trap frame format.
 */
#define SET_U_AR0(up)	\
	((up)->u_ar0 = (int *)((char *)(up) - KSTACK_RESERVE) - U_EAX)

/*
 * Macros to convert between ublocks and uareas.  A uarea is the struct user
 * for an LWP.  A ublock is the uarea plus the LWP's kernel stack.
 */
#define UAREA_OFFSET	(USIZE * PAGESIZE - sizeof(struct user))
#define UBLOCK_TO_UAREA(x) ((struct user *)((vaddr_t)(x) + UAREA_OFFSET))
#define UAREA_TO_UBLOCK(up) ((vaddr_t)(up) - UAREA_OFFSET)

/*
 * Safety margin for preemption disable when executing close to the
 * stack extension page.
 */
#define UAREA_TRUE_MARGIN	-0x80			/* true margin */
#define UAREA_NO_MARGIN		(2 * PAGESIZE)		/* always preempt */
extern int ovstack_preempt;	/* preemption enabled on overflow stack? */
extern int ovstack_margin;	/* preemption enabled on overflow stack? */

/*
 * Set safety margin when preemption on the extension page is disabled.
 * Set ``totally unsafe'' margin when we wish to enable preemption on the
 * extension page.
 */
#define ublock_init_f() {						 \
	ovstack_margin =						 \
		(ovstack_preempt) ? UAREA_NO_MARGIN : UAREA_TRUE_MARGIN; \
}

/*
 * Compatibility hook for users of (obsolete) seterror().
 * Error set by seterror() (this time around) overrides error return value.
 */
#define CHECK_COMPAT_ERRNO(errno) \
	if (u.u_compat_errno) { \
		(errno) = u.u_compat_errno; \
		u.u_compat_errno = 0; \
	}

/*
 * Structure of user-visible window.  This structure is in per-engine space
 * and is also mapped readable by user mode.
 */
struct uvwindow {
	void		*uv_privatedatap;   /* LWP private data pointer */
	boolean_t	uv_fp_used;	    /* FP used since last setcontext */
	uint_t		uv_fp_hw;	    /* FP type; same as SI86FPHW */
};

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern user_t	*upointer;		/* current user structure */
#define u	(*upointer)

/*
 * NOTE: The upointer symbol is exported to the MERGE386 binary module,
 * and must always be present.  Any such module, however, must not depend
 * on any specific offsets in the user structure.  Offsets for exported
 * fields must be obtained through a function like mrg_getparm().
 */

extern user_t	ueng;			/* per-engine "idle" user structure */

extern volatile struct uvwindow uvwin;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_USER_H */

#ident	"@(#)kern-i386:proc/sigmdep.c	1.32.4.2"
#ident	"$Header$"

#include <mem/as.h>
#include <mem/vmparam.h>
#include <proc/class.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/regset.h>
#include <proc/resource.h>
#include <proc/seg.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/ucontext.h>
#include <proc/user.h>
#include <svc/reg.h>
#include <svc/sco.h>
#include <svc/systm.h>
#ifdef MERGE386
#include <svc/mki.h>
#endif
#include <util/debug.h>
#include <util/types.h>

extern int core(pid_t, struct cred *, rlim_t, int, boolean_t);

/* These structures define what is pushed on the stack */

struct argpframe {
	void		(*retadr)();
	uint		signo;
	siginfo_t	*sip;
	ucontext_t	*ucp;
	void		(*handler_addr)();	/* SST_NSIGACT only */
};

struct compat_frame {
	void		(*retadr)();
	uint		signo;
	union {
		gregset_t	gregs;
		struct {
			siginfo_t	*info;
			ucontext_t	*uc;
			int		dummy[(sizeof(gregset_t)/sizeof(int)) - 2];
		} handler_args;
	} gen;
	void		*fpsp;
	void		*wsp;
	sco_sigset_t	mask;
	unsigned int	frametype;
};

/*
 *
 * boolean_t sendsig(int sig, sigqueue_t *sqp)
 * 	Dispatch signal handler.
 *
 * Calling/Exit State:
 *	No locks must be held by the caller and none held on exit.  Return
 *	B_FALSE if stack can not be built.  Otherwise, B_TRUE is returned.
 *
 */

boolean_t
sendsig(int sig, sigqueue_t *sqp)
{
	ucontext_t uc;
	boolean_t altstack;	/* if true, switching to alternate stack */
	int minstacksz;		/* size of stack required to catch signal */
	vaddr_t esp, eip;
	proc_t *p;
	lwp_t *lwpp;
	sigstate_t *ssp;
	struct argpframe argpframe;

	p = u.u_procp;
	lwpp = u.u_lwpp;

	/*
	 * l_cursigst field cannot be modified by any kernel execution
	 * entity other than the LWP itself, unless the LWP is in the
	 * stopped state. Therefore, it is safe to access the field
	 * here without holding any lock.
	 */

	ssp = &lwpp->l_cursigst;

#ifdef MERGE386	
	if (lwpp->l_special & SPECF_VM86)
		mhi_signal((struct regs *)&u.u_ar0[T_EAX], sig);
#endif
	/* Compute absolute 32-bit flat address of stack pointer in esp. */
	{
		uint_t ssdt;		/* descriptor type (LDT/GDT) */
		struct segment_desc *ssdp; /* pointer to descriptor */

		esp = u.u_ar0[T_UESP];
		ssdt = (u.u_ar0[T_SS] & SEL_LDT) ? DT_LDT : DT_GDT;
		if (u.u_dt_infop[ssdt] != &myglobal_dt_info[ssdt]) {
			/*
			 * The process has a private DT, so SS might refer to
			 * a custom descriptor.  Check the descriptor and
			 * convert %ss:%esp to a linear address.
			 */
			ssdp = &u.u_dt_infop[ssdt]->
				di_table[seltoi(u.u_ar0[T_SS])];
			if (!(SD_GET_ACC2(ssdp) & BIGSTACK))
				esp &= 0xFFFF;
			esp += SD_GET_BASE(ssdp);
		}
	}

	/*
	 * Check if the signal is to be handled on an alternate signal stack.
	 * Don't use an alternate stack if a non-standard segment is in use.
	 */
	altstack = ((ssp->sst_cflags & SA_ONSTACK) &&
		    !(u.u_sigaltstack.ss_flags & (SS_ONSTACK|SS_DISABLE)));

	/*
	 * Check if signal was established using pre SVR4
	 * signal system calls -- signal or sigset.
	 */

	if (ssp->sst_rflags & SST_OLDSIG) {
		minstacksz = sizeof (struct compat_frame) + sizeof (ucontext_t);

		/* Check the need for additional signal information */
		if ((ssp->sst_cflags & SA_SIGINFO) || altstack) {
			minstacksz += sizeof (siginfo_t);
		}
	}
	else {
		minstacksz = sizeof (ucontext_t) + sizeof (struct argpframe);

		/* Check the need for additional signal information */
		if (sqp != NULL)
			minstacksz += sizeof (siginfo_t);
	}

	if (altstack) {
		if (minstacksz >= u.u_sigaltstack.ss_size)
			return B_FALSE;		/* Not enough space */
		esp = (vaddr_t)u.u_sigaltstack.ss_sp + u.u_sigaltstack.ss_size;
	}

	if (sqp != NULL) {
		esp -= sizeof(siginfo_t);

		/* Enhanced Application Compatibility Support */
		/*
		 * Note: we're not doing any translation on the siginfo
		 * structure for SCO apps. At first glance there are 2
		 * incompats, there is a pid_t and uid_t (shorts on OSR5,
		 * longs on UW), however since the pid_t is after an int and
		 * before a union and the first memeber of the union is uid_t
		 * (and the union has an int in it also) the structures line
		 * up. siginfo is pretty stable now so this isn't likely
		 * to change. The second incompat is the time stuff isn't
		 * in _cld. comments in 'winfo()' (in sig.c) indicate that for
		 * ES/MP this was a known binary incompat, the values are zero
		 * and we'll live with it.
		 */
		/* End Enhanced Application Compatibility Support */
		if (ucopyout(&sqp->sq_info, (void *)esp,
			     sizeof (k_siginfo_t), 0) != 0)
			return B_FALSE;
		argpframe.sip = (siginfo_t *)esp;
	} else
		argpframe.sip = NULL;

	savecontext(&uc, u.u_sigoldmask);

	/*
	 * iBCS puts trap type and err code in saved context even though
	 * this information is of dubious value and non-portable.
	 */
	uc.uc_mcontext.gregs[12] = u.u_traptype & ~USERFLT;
	uc.uc_mcontext.gregs[13] = 0;

	/*
	 * We need to provide a clean FPU to the signal handler.
	 * savecontext() has saved the FPU state (if FPU was in use)
	 * and has disabled the FPU (as part of save_fpu()).
	 * We clear the kctx_fpvalid flag; this will force the kernel to
	 * re-initialize the FPU if the signal handler uses the FPU.
	 */

	u.u_kcontext.kctx_fpvalid = 0;

	esp -= sizeof uc;
	if (ucopyout(&uc, (void *)esp, sizeof uc, 0) != 0)
		return B_FALSE;
	argpframe.ucp = (ucontext_t *)esp;

	if (ssp->sst_rflags & SST_OLDSIG) {
		struct compat_frame cframe;
		int user_signo = sig;	/* Signal number to send to the user */

/* Enhanced Application Compatibility Support */
		/* 
		 * Convert the SVR4 Signal number to the signal number
		 * the process may be expecting.  This may different
		 * because of the ISC signal emulation, which uses old-style
		 * signal handling.
		 */
#ifdef ISC_USES_POSIX
	        if (ISC_USES_POSIX) {
	                switch (sig) {
	                case SIGCONT:
	                        user_signo = ISC_SIGCONT;
	                        break;
	                case SIGSTOP:
	                        user_signo = ISC_SIGSTOP;
	                        break;
	                case SIGTSTP:
	                        user_signo = ISC_SIGTSTP;
	                        break;
	                }
		}
#endif /* ISC_USES_POSIX */
/* End Enhanced Application Compatibility Support */

		cframe.retadr = p->p_sigreturn;
		eip = (vaddr_t)ssp->sst_handler;
		cframe.signo = user_signo;
		if((ssp->sst_cflags & SA_SIGINFO) || altstack) {
			cframe.gen.handler_args.info = argpframe.sip;
			cframe.gen.handler_args.uc = argpframe.ucp;
			bzero((caddr_t)&cframe.gen.handler_args.dummy[0],
				sizeof(cframe.gen.handler_args.dummy));
			cframe.fpsp = 0;
			cframe.wsp = 0;
			cframe.mask = 0;
			cframe.frametype = 1;
		}
		else {
			bcopy(uc.uc_mcontext.gregs, cframe.gen.gregs, sizeof cframe.gen.gregs);
			cframe.fpsp = &argpframe.ucp->uc_mcontext.fpregs.fp_reg_set;
			cframe.wsp = &argpframe.ucp->uc_mcontext.fpregs.f_wregs[0];
			cframe.mask = uc.uc_sigmask.sa_sigbits[0];
			cframe.frametype = 0;
		}


		esp -= sizeof cframe;
		if (ucopyout(&cframe, (void *)esp, sizeof cframe, 0) != 0)
			return B_FALSE;
	} else {
		/* should not return via this; if they do, fault. */
		argpframe.retadr = (void (*)())0xFFFFFFFF;
		/* Pass user defined handler to a common handler */
		argpframe.handler_addr = ssp->sst_handler;
		if (ssp->sst_rflags & SST_NSIGACT)
			eip = (vaddr_t)p->p_sigactret; /* ESMP sigaction */
		else
			eip = (vaddr_t)ssp->sst_handler; /* pre-ESMP sigaction */
		argpframe.signo = sig;

		esp -= sizeof argpframe;
		if (ucopyout(&argpframe, (void *)esp, sizeof argpframe, 0) != 0)
			return B_FALSE;
	}

	/*
	 * Set segment registers to proper selectors for signal delivery.
	 * Someday these should come from user-settable per-process variables,
	 * but for now they are constants.
	 */
	u.u_ar0[T_DS] = u.u_ar0[T_ES] = u.u_ar0[T_SS] = USER_DS;
	u.u_ar0[T_CS] = USER_CS;

	/* Check validity of IP and SP against segment bounds. */
	if ((lwpp->l_special & SPECF_NONSTDLDT) ||
	    u.u_ar0[T_SS] != USER_DS ||
	    u.u_ar0[T_CS] != USER_CS) {
		/*
		 * The user has changed some of the standard segments or has
		 * requested use of non-standard selectors for signal
		 * delivery.  This means we need to check IP and SP for
		 * validity against the base and limit of their respective
		 * segments, and check the type of CS and IP.
		 */

		uint_t sdt;		/* segment descriptor type */
		struct segment_desc *sdp; /* segment descriptor pointer */
		unsigned long base, limit;

		/* Check SS/SP.  esp is flat address at this point. */
		sdt = (u.u_ar0[T_SS] & SEL_LDT) ? DT_LDT : DT_GDT;
		sdp = &u.u_dt_infop[sdt]->di_table[seltoi(u.u_ar0[T_SS])];
		if ((SD_GET_ACC1(sdp) & SEG_CODE) ||
		    !(SD_GET_ACC1(sdp) & SEG_WRITEABLE))
			return B_FALSE;
		base = SD_GET_BASE(sdp);
		limit = SD_GET_LIMIT(sdp);
		if (SD_GET_ACC2(sdp) & GRANBIT)
			limit = limit << 12 | 0xFFF;
		if (!(SD_GET_ACC2(sdp) & BIGSTACK))
			limit &= 0xFFFF;
		if (esp < base || esp - base > limit)
			return B_FALSE;
		esp -= base;		/* esp is now offset to SS. */

		/* Check CS/IP. */
		sdt = (u.u_ar0[T_CS] & SEL_LDT) ? DT_LDT : DT_GDT;
		sdp = &u.u_dt_infop[sdt]->di_table[seltoi(u.u_ar0[T_CS])];
		if (!(SD_GET_ACC1(sdp) & SEG_CODE))
			return B_FALSE;
		limit = SD_GET_LIMIT(sdp);
		if (SD_GET_ACC2(sdp) & GRANBIT)
			limit = limit << 12 | 0xFFF;
		if (eip > limit)
			return B_FALSE;
	} else {
		/*
		 * Standard segments, so we only need the quick
		 * VALID_USR_RANGE check.
		 */
		if (!VALID_USR_RANGE(esp, 1) ||
		    !VALID_USR_RANGE(eip, 1))
			return B_FALSE;
	}

	/*
	 * Now that we can no longer fault, update the u-block,
	 * and push context.
	 */

	u.u_oldcontext = argpframe.ucp;
	u.u_ar0[T_EIP] = eip;
	u.u_ar0[T_UESP] = esp;
	((flags_t *)&u.u_ar0[T_EFL])->fl_tf = 0;  /* disable single step */

	if (altstack) {
		u.u_sigaltstack.ss_flags |= SS_ONSTACK;
		u.u_stkbase = (vaddr_t)u.u_sigaltstack.ss_sp +
					u.u_sigaltstack.ss_size;
		u.u_stksize = u.u_sigaltstack.ss_size;
	}

	return B_TRUE;
}



/*
 * void
 * sigclean(.....)
 *	Restore user's context after execution of user signal handler
 * 	This code restores all registers to what they were at the time
 * 	signal occured. So any changes made to things like flags will
 * 	disappear.
 *
 * Calling/Exit State:
 * 	The saved context is assumed to be at esp+xxx address on the user's
 * 	stack. It is assumed that the stack on which the handler was
 *	dispatched is sane. On entry into this function, the user stack
 *	looks like this:
 *
 *	------------------------ <------ user esp - 2
 * 	|			|	
 *	| Compatibility Frame	|
 *	|			|
 *	|-----------------------|
 *	|			|
 *	| U Context structure 	|
 *	| that we should return	|
 *	| to.			|
 *	|-----------------------|
 *	|			|
 *
 * On entry, assume all registers are pushed; that is the standard trap
 * entry frame. This function returns like other system calls.
 *
 * Remarks:
 *	On return from the user signal handler, the user stack pointer will
 *	be pointing at the second word in the compatibility frame (the
 *	first word is the return address pointing to the function invoking
 *	sigclean()). The library code advances the stack pointer by a word
 *	to get rid of the argument to the handler. Hence we need to decrement
 *	two words from the user stack pointer to get at the base of the
 *	compatibility frame.
 */


/* ARGSUSED */
void
sigclean(volatile uint_t edi,		/* user register */
	 volatile uint_t esi,		/* user register */
	 volatile uint_t ebp,		/* user frame pointer register */
	 volatile uint_t unused,	/* temp from "pushal" instruction */
	 volatile uint_t ebx,		/* user register */
	 volatile uint_t edx,		/* user register */
	 volatile uint_t ecx,		/* user register */
	 volatile uint_t eax,		/* user register */
	 volatile uint_t es,		/* user "extra" segment register */
	 volatile uint_t ds,		/* user data segment register */
	 volatile uint_t eip,		/* user instruction pointer register */
	 volatile uint_t cs,		/* user code segment register */
	 volatile uint_t flags,		/* user flags register */
	 volatile uint_t esp,		/* user stack pointer register */
	 volatile uint_t ss)		/* user stack segment register */
{
	struct compat_frame *cframe;
	ucontext_t	uc, *ucp;
	proc_t		*p = u.u_procp;
	lwp_t		*lwpp = u.u_lwpp;
	int		frametype;

	u.u_ar0 = (int *)&eax;

	/*
	 * The user's stack pointer currently points into compat_frame
	 * on the user stack.  Adjust it to the base of compat_frame.
	 */

	cframe = (struct compat_frame *)(esp - 2 * sizeof(uint_t));
	ucp = (ucontext_t *)(cframe + 1);

	/*
	 * Copy in the saved ucontext structure.
	 * Since the old-style stack frame has gregs in a different place,
	 * we have to copy it in to the ucontext structure instead of using
	 * the original gregs saved in the ucontext.
	 * 
	 * We now only do the extra gregs copyin if we hadn't had a 
	 * SA_SIGINFO sent in the signal, in that case the signfo stuff
	 * is a union with gregs and gregs isn't valid
	 */
	
	frametype = fuword ((const int *) &cframe->frametype);
	if(frametype) { /* SA_SIGINFO */
		if (ucopyin((caddr_t)ucp, &uc, sizeof uc, 0) != 0) {
			/* ucopyin() failed; dump core */
			exit((core(p->p_pidp->pid_id, lwpp->l_cred,
			   	u.u_rlimits->rl_limits[RLIMIT_CORE].rlim_cur,
			   	SIGSEGV, B_FALSE) ? CLD_KILLED : CLD_DUMPED),
		     	SIGSEGV);
		}
	}
	else {
		if (ucopyin((caddr_t)ucp, &uc, sizeof uc, 0) != 0 ||
	    	ucopyin((caddr_t)&cframe->gen.gregs,
		    	&uc.uc_mcontext.gregs,
		    	sizeof uc.uc_mcontext.gregs, 0) != 0) {
			/* ucopyin() failed; dump core */
			exit((core(p->p_pidp->pid_id, lwpp->l_cred,
			   	u.u_rlimits->rl_limits[RLIMIT_CORE].rlim_cur,
			   	SIGSEGV, B_FALSE) ? CLD_KILLED : CLD_DUMPED),
		     	SIGSEGV);
		}
	}

	restorecontext(&uc);
	/*
	 * We don't need to call CL_TRAPRET() since we didn't do anything
	 * which would block and change to system priority.
	 */
}

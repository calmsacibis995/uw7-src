#ident	"@(#)kern-i386:proc/procmdep.c	1.58.5.2"
#ident	"$Header$"

#include <fs/procfs/prdata.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prsystm.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <proc/iobitmap.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/signal.h>
#include <proc/siginfo.h>
#include <proc/tss.h>
#include <proc/ucontext.h>
#include <proc/user.h>
#include <proc/cg.h>
#include <svc/errno.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern void fpu_disable(void);
extern void fixuserefl(int *);
void *lwp_setprivate(void *);

STATIC void dt_reset(lwp_t *);
STATIC void dt_reset_gdt(void);
STATIC void dt_reset_ldt(void);
STATIC void dt_new_table(uint_t dt, struct segment_desc *ndtp, size_t size);
STATIC void dt_grow_table(uint_t dt, size_t size);

/*
 *+ Sleeplock to serialize access to process and LWP TSS's.
 */
STATIC LKINFO_DECL(tsslkinfo, "PP::p_tsslock", 0);


/*
 * void restorecontext(ucontext_t *ucp)
 *	Restores the context of the calling context to what is specified in the 
 *	ucontext structure specified by ucp.
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on exit.
 */	
void
restorecontext(ucontext_t *ucp)
{
	u.u_oldcontext = ucp->uc_link;

	if (ucp->uc_flags & UC_CPU) {
		if (prsetregs(&u, ucp->uc_mcontext.gregs) != 0) {
			/* Send SIGSEGV:SEGV_MAPERR. */
			incur_fault(FLTBOUNDS, SIGSEGV, SEGV_MAPERR,
				    (caddr_t)u.u_ar0[T_EIP]);
			return;
		}
	}

	if (ucp->uc_flags & UC_STACK) {

		/* 
		 * Initialize the lwp's notion of stack size and stack base
		 * with the values from the context structure. Note that these
		 * will be the values returned on a getcontext(2) call.
		 */

		u.u_stkbase = (vaddr_t)ucp->uc_stack.ss_sp +
					ucp->uc_stack.ss_size;
		u.u_stksize = ucp->uc_stack.ss_size;

		if (ucp->uc_stack.ss_flags & SS_ONSTACK) {
			bcopy(&ucp->uc_stack,
			      &u.u_sigaltstack,
			      sizeof(struct sigaltstack));
		} else
			u.u_sigaltstack.ss_flags &= ~SS_ONSTACK;
	}

	DISABLE_PRMPT();
	if (using_fpu) {
		/*
		 * The previous context was using the FPU.  Disable it.
		 */
		fpu_disable();
		uvwin.uv_fp_used = u.u_fp_used = B_FALSE;
	}
	ENABLE_PRMPT();
	if (ucp->uc_flags & UC_FP) {
		/*
		 * The context contains a valid FPU state.
		 */
		u.u_kcontext.kctx_fpregs = ucp->uc_mcontext.fpregs;
		u.u_kcontext.kctx_fpvalid = 1;
	} else {
		/*
		 * The user did not specify a FPU state.
		 */
		u.u_kcontext.kctx_fpvalid = 0;
	}

	if (ucp->uc_flags & UC_SIGMASK) {
		k_sigset_t kset;
		sigutok(&ucp->uc_sigmask, &kset);
		sigdiffset(&kset, &sig_cantmask);
		lwpsigmask(kset);
	}

}


/*
 * void savecontext(ucontext_t *ucp, k_sigset_t mask)
 *	Copy the context of the calling context to the ucontext structure
 *	pointed to by the ucp parameter. Note that the register state that
 *	is copied is the register state on entry into the kernel.
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on exit.
 */
void
savecontext(ucontext_t *ucp, k_sigset_t mask)
{
	proc_t *pp = u.u_procp;
	int stksize;	

	ucp->uc_flags = UC_ALL;
	ucp->uc_link = u.u_oldcontext;

	/*
	 * Save current stack state. If we appear to be running on the
	 * auto-grow stack, then size must be reflected from the proc
	 * structure, maintained by grow().
	 */

	if (u.u_stkbase == (vaddr_t)u.u_sigaltstack.ss_sp + 
					u.u_sigaltstack.ss_size) {
                ucp->uc_stack.ss_flags = SS_ONSTACK;
	} else {
		ucp->uc_stack.ss_flags = 0;
	}
	stksize = u.u_stksize;
	ucp->uc_stack.ss_sp = (char *)(u.u_stkbase - stksize);
	ucp->uc_stack.ss_size = stksize;

	/* 
	 * Save FPU machine context 
	 */
	if (u.u_kcontext.kctx_fpvalid) {
		/*
		 * The current LWP is using the FPU. However, we need to save 
		 * the FPU state only if the state has not already been 
		 * saved during a context switch. That is, do an explicit
		 * state save only if the current LWP is presently using 
		 * the FPU. Note that save_fpu() also disables the FPU
		 * and hence the next time the LWP uses the FPU, we will
		 * take the fault and restore the FPU state or initialize
		 * the FPU state as appropriate. 
		 */
		DISABLE_PRMPT();
		if (using_fpu)
			save_fpu();
		ucp->uc_mcontext.fpregs = u.u_kcontext.kctx_fpregs;
		ucp->uc_flags |= UC_FP;
		ENABLE_PRMPT();
	} else {
		ucp->uc_flags &= ~UC_FP;
		ASSERT(!using_fpu);
	}
	
	/*
	 * Save general registers.
	 */

	prgetregs(&u, ucp->uc_mcontext.gregs);

	/* save signal mask */

	sigktou(&mask,&ucp->uc_sigmask);
 	((flags_t *)&u.u_ar0[T_EFL])->fl_tf = 0;    /* disable single step */
}

struct setcontexta {
	int flag;
	caddr_t *ucp;
};

/*
 * int setcontext(struct setcontexta *uap, rval_t *rvp)
 *	This is the common entry point for the setcontext(2) and getcontext(2)
 *	system calls.
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on exit.
 */ 
int
setcontext(struct setcontexta *uap, rval_t *rvp)
{
	ucontext_t uc;

	switch (uap->flag) {

	default:
		return EINVAL;

	case GETCONTEXT:
		savecontext(&uc, u.u_lwpp->l_sigheld);
		if (copyout(&uc, uap->ucp, sizeof(ucontext_t)))
			return EFAULT;
		return 0;

	case SETCONTEXT:
		/*
		 * SVR4.0 makecontext() library function expects that 
		 * a setcontext() performed with a NULL context pointer 
		 * will be equivalent to a call to exit(). Since makecontext()
		 * was in the archive, we are stuck with this compatibility.
		 * Make the old code happy....
		 */

		if (uap->ucp == NULL)
			exit(CLD_EXITED, 0);
		if (copyin(uap->ucp, &uc, sizeof(ucontext_t)))
			return EFAULT;

		restorecontext(&uc);

		/* 
		 * On return from system calls, eax and edx are overwritten with 
		 * r_val1 and r_val2 respectively, so set r_val1 and r_val2 to 
		 * eax and edx here.
		 */
                rvp->r_val1 = u.u_ar0[T_EAX];
		rvp->r_val2 = u.u_ar0[T_EDX];

		return 0;
	}
}

/*
 * void set_sigreturn(void)
 *	Sets the p_sigreturn field to the old-style signal trampoline function.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	The address of the trampoline function is passed via register EDX.
 */
void
set_sigreturn(void)
{
	u.u_procp->p_sigreturn = (void (*)())u.u_ar0[T_EDX];
}

/*
 * void copy_ublock(caddr_t curu, caddr_t newup)
 *	Copy the u area and the trap frame from the creating context to the 
 *	created context. All of the u area and the standard trap frame
 *	(excluding the trap number) will be copied. Note that we assume that 
 *	the kernel stack is allocated contiguously with the u area.
 *
 * Calling/Exit State:
 *	None.
 */
void
copy_ublock(caddr_t curu, caddr_t newup)
{
	size_t offset = KSTACK_RESERVE +
			 (U_EDI * sizeof(int));  /* size of the trap frame */

	bcopy(curu - offset, newup - offset, offset + sizeof(struct user));
}


/*
 * void
 * complete_fork_f(void)
 *	Family-specific fork completion code.
 *
 * Calling/Exit State:
 *	Called in context of the new LWP before it starts running.
 */
void
complete_fork_f(void)
{
	/*
	 * Indicate floating-point not yet used in child of invoking context.
	 */
	if (u.u_lwpp->l_flag & L_INITLWP) {
		ASSERT(u.u_fpe_restart.fr_esp == 0);
		u.u_kcontext.kctx_fpvalid = u.u_fp_used = B_FALSE;
	}
	complete_fork();
}


/*
 * void setup_newcontext(dupflags_t dupflags, boolean_t is_sysproc, struct user
 *			 *userp, void (*funcp)(void *), void *argp)
 * 	Initialize the state for the newly created context.
 *
 * Calling/Exit State:
 *	None.
 */
void
setup_newcontext(dupflag_t dupflags, boolean_t is_sysproc, struct user
		 *userp, void (*funcp)(void *), void *argp)
{
	struct desctab_info *dp;
	uint_t *sp;

	/*
	 * Setup for the kernel stack extension page:
	 * compute address of PTE below normal swappable stack
	 * and cache it in userp->u_kse_ptep.
	 */
	if (PAE_ENABLED())
		userp->u_kse_ptep = kvtol2ptep64(UAREA_TO_UBLOCK(userp) -
					mmu_ptob(KSE_PAGES));
	else
		userp->u_kse_ptep = kvtol2ptep(UAREA_TO_UBLOCK(userp) -
					mmu_ptob(KSE_PAGES));

	/*
	 * Check if we are inheriting a private GDT or LDT.
	 * If so, we need to clone it.
	 */
	if ((dp = userp->u_dt_infop[DT_GDT]) != &myglobal_dt_info[DT_GDT]) {
		struct desctab_info *ndp = kmem_alloc(sizeof *dp, KM_SLEEP);
		void *ndtp = kmem_alloc(dp->di_size, KM_SLEEP);
		ASSERT(dp->di_table != l.global_gdt);
		bcopy(dp->di_table, ndtp, dp->di_size);
		ndp->di_size = dp->di_size;
		ndp->di_installed = 0;
		ndp->di_link = &myglobal_dt_info[DT_GDT];
		ndp->di_table = ndtp;
		BUILD_TABLE_DESC(&userp->u_gdt_desc,
				 ndp->di_table,
				 ndp->di_size / sizeof(struct segment_desc));
		userp->u_dt_infop[DT_GDT] = ndp;
		userp->u_lwpp->l_special |= SPECF_PRIVGDT;
	}
	if ((dp = userp->u_dt_infop[DT_LDT]) != &myglobal_dt_info[DT_LDT]) {
		struct desctab_info *ndp = kmem_alloc(sizeof *dp, KM_SLEEP);
		void *ndtp = kmem_alloc(dp->di_size, KM_SLEEP);
		ASSERT(dp->di_table != myglobal_ldt);
		bcopy(dp->di_table, ndtp, dp->di_size);
		ndp->di_size = dp->di_size;
		ndp->di_installed = 0;
		ndp->di_link = &myglobal_dt_info[DT_LDT];
		ndp->di_table = ndtp;
		BUILD_SYS_DESC(&userp->u_ldt_desc,
			       ndp->di_table,
			       ndp->di_size, LDT_KACC1, LDT_ACC2);
		userp->u_dt_infop[DT_LDT] = ndp;
		userp->u_lwpp->l_special |= SPECF_PRIVLDT;
	}

	/*
	 * See if we are inheriting use of the debug registers.
	 */
	if (userp->u_kcontext.kctx_debugon)
		prdebugon(userp->u_lwpp);

	/*
	 * Find the kernel stack from the uarea pointer.
	 * We assume that the kernel stack is contiguous with the uarea.
	 */
	if (is_sysproc)
		sp = (uint_t *)userp;
	else {
		sp = (uint_t *)((uint_t)userp -
				 (KSTACK_RESERVE + (U_EDI * sizeof(int))));
	}
	if (funcp != NULL) {
		userp->u_kcontext.kctx_eip = (uint_t)funcp;
		*--sp = (uint_t)argp;
		if (is_sysproc) {
			/*
			 * This is a system process and should not
			 * return from the specified function.
			 */
			*--sp = (uint_t)syscontext_returned;
		} else {
			/*
			 * We are creating the init process - initialize
			 * the stack to return to user mode.
			 */
			SET_U_AR0(userp);
			/*
			 * Fake the initial trap frame for the init
			 * context. exec() of init will fill in the 
			 * right values.
			 */
			userp->u_ar0[T_SS] = USER_DS;
			userp->u_ar0[T_CS] = USER_CS;
			userp->u_ar0[T_DS] = USER_DS;
			userp->u_ar0[T_ES] = USER_DS;
			*--sp = (uint_t)initproc_return;
		} 
		/*
		 * Initialize the saved esp in the kcontext structure.
		 */
		userp->u_kcontext.kctx_esp = (uint_t)sp;
		return;
	}
	if (dupflags == DUP_LWPCR) {
		userp->u_kcontext.kctx_eip = (uint_t)complete_lwpcreate;
	} else {
		userp->u_kcontext.kctx_eip = (uint_t)complete_fork_f;
	}
	*--sp = (uint_t)(process_trapret);

	/*
	 * Initialize the saved esp in the kcontext structure.
	 */
	userp->u_kcontext.kctx_esp = (uint_t)sp;

	/*
	 * Initialize u_ar0.
	 */
	SET_U_AR0(userp);
}


/*
 * __lwp_private(2) system call.
 */

struct lwp_priva {
        void    *privatedatap;
};


/*
 * int __lwp_private(struct lwp_priva *uap, rval_t *rvp)
 *      This is the system call to register the private data pointer of the
 *      calling context. The function returns the virtual address at which
 *      user level code can access the private data pointer of the executing
 *      context.
 *
 * Calling/Exit State:
 *      None.
 *
 * Remarks: This system call has been defined to efficiently support the 
 * _lwp_getprivate() functionality. Note that for register-rich architectures,
 * this system call is not needed.
 */
int
__lwp_private(struct lwp_priva *uap, rval_t *rvp)
{
        rvp->r_val1 = (int)lwp_setprivate(uap->privatedatap);
	u.u_privatedatap = uap->privatedatap;
        return (0);
}

/*
 * void *lwp_setprivate(void *privatedatap)
 *      This function registers the private data pointer in the per-CPU page
 *      that is readable from user level. The address at which the private data
 *      pointer can be read from the user level is returned by this function.
 *
 * Calling/Exit State:
 *      None.
 */
/* ARGSUSED */
void *
lwp_setprivate(void *privatedatap)
{
	uvwin.uv_privatedatap = privatedatap;
	return &((struct uvwindow *)UVUVWIN)->uv_privatedatap;
}

/*
 * void fix_retval(rval_t *rvp)
 *	Sets the sys call return values from the trap framei so that 
 *	eax and edx will have the correct value. This is done to provide 
 *	the user the context that was specified.
 * 
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 * 	This is provided as a function so that lwpscalls.c file can be 
 *	machine independent.
 */ 
void
fix_retval(rval_t *rvp)
{
	rvp->r_val1 = u.u_ar0[T_EAX];
	rvp->r_val2 = u.u_ar0[T_EDX];
}

/*
 * void prgetregs(user_t const *up, gregset_t rp)
 * 	Return general registers.
 *
 * Calling/Exit State:
 *	The caller must guarantee that the u-area is locked in core.
 */
void
prgetregs(user_t const *up, gregset_t rp)
{
	greg_t *uregs;

	if (up->u_ar0 == NULL) {  /* system process - no user regs */
		bzero(rp, sizeof *rp);
		return;
	}

	uregs = (greg_t *)up->u_ar0;

	/*
	 * Initialize the specified gregset_t from the trap frame
	 * that has the user registers on entry into the system.
	 */

	rp[R_SS] = uregs[T_SS];
	rp[R_ESP] = uregs[T_UESP];
	rp[R_EFL] = uregs[T_EFL];
	rp[R_CS] = uregs[T_CS];
	rp[R_EIP] = uregs[T_EIP];
	rp[R_DS] = uregs[T_DS];
	rp[R_ES] = uregs[T_ES];
	rp[R_EAX] = uregs[T_EAX];
	rp[R_ECX] = uregs[T_ECX];
	rp[R_EDX] = uregs[T_EDX];
	rp[R_EBX] = uregs[T_EBX];
	rp[R_EBP] = uregs[T_EBP];
	rp[R_ESI] = uregs[T_ESI];
	rp[R_EDI] = uregs[T_EDI];

	/*
	 * FS and GS are not saved in the trap frame.
	 */
	if (up->u_lwpp == u.u_lwpp) {
		rp[R_FS] = _fs();
		rp[R_GS] = _gs();
	} else {
		rp[R_FS] = up->u_kcontext.kctx_fs;
		rp[R_GS] = up->u_kcontext.kctx_gs;
	}
}


/*
 * STATIC boolean_t invalidseg(user_t *up, ushort_t sel, int reg)
 *	Detect an invalid segment selector.
 *
 * Calling/Exit State:
 *	sel is a segment selector which the user is attempting to load
 *	into segment register reg for the process described by up.
 *	The return value is B_TRUE iff sel is invalid for reg (if
 *	loading it would cause a fault).
 *
 *	No locking requirements.
 */
STATIC boolean_t
invalidseg(user_t *up, ushort_t sel, int reg)
{
	uint_t dt;
	struct desctab_info *dp;
	uint_t idx, limit;
	struct segment_desc *descp;

	/* Null selectors are always OK. */
	if ((sel & ~RPL_MASK) == 0)
		return B_FALSE;

	/* If not user mode, reject. */
	if (!USERMODE(sel, 0))
		return B_TRUE;

	dt = ((sel & SEL_LDT) ? DT_LDT : DT_GDT);
	dp = up->u_dt_infop[dt];

	/* If beyond end of table, reject. */
	limit = dp->di_size / sizeof (struct segment_desc);
	if ((idx = seltoi(sel)) >= limit)
		return B_TRUE;

	descp = &dp->di_table[idx];

	/*
	 * Only code and data segments (not call gates, etc.) can be
	 * loaded into a register.  If anything else, reject.
	 */
	switch (SD_GET_ACC1(descp) & 0x1C) {
	case 0x10:	/* expand-up data segment */
	case 0x18:	/* non-conforming code segment */
	case 0x14:	/* expand-down data segment */
		break;			/* OK */
	default:
		return B_TRUE;		/* BAD */
	}

	switch (reg) {
	case R_CS:
		/* If CS is not a code segment, reject. */
		if (!(SD_GET_ACC1(descp) & SEG_CODE))
			return B_TRUE;
		break;
	case R_SS:
		/* If stack segment is unwritable, reject. */
		if ((SD_GET_ACC1(descp) & SEG_CODE) ||
		    !(SD_GET_ACC1(descp) & SEG_WRITEABLE))
			return B_TRUE;
		break;
	default:
		/*
		 * If other than CS or SS is an unreadable code segment,
		 * reject.
		 */
		if ((SD_GET_ACC1(descp) & SEG_CODE) &&
		    !(SD_GET_ACC1(descp) & SEG_READABLE))
			return B_TRUE;
		break;
	}

	/* Check privilege levels:  if RPL != DPL, reject. */
	if ((sel & RPL_MASK) != (SD_GET_ACC1(descp) & SEG_DPL) >> DPL_SHIFT)
		return B_TRUE;

	/* Everything looks OK. */
	return B_FALSE;
}

/*
 * int prsetregs(user_t *up, gregset_t rp)
 * 	Set general registers.
 *	Returns 0 for success or
 *	EINVAL if rp contains invalid register values.
 *
 * Calling/Exit State:
 *	The caller must guarantee that the u-area is locked in core.
 */
int
prsetregs(user_t *up, gregset_t rp)
{
	greg_t *uregs;
	ulong_t limit;
	uint_t dt;
	struct desctab_info *dp;
	struct segment_desc *descp;

	ASSERT(up->u_ar0 != NULL);		/* proc must have user regs */

	uregs = (greg_t *)up->u_ar0;

	/* Reject bad segment register values that would cause a fault. */

	if ((rp[R_CS] & RPL_MASK) != (rp[R_SS] & RPL_MASK) ||
	    uregs[T_CS] != rp[R_CS] && invalidseg(up, rp[R_CS], R_CS) ||
	    uregs[T_SS] != rp[R_SS] && invalidseg(up, rp[R_SS], R_SS) ||
	    uregs[T_DS] != rp[R_DS] && invalidseg(up, rp[R_DS], R_DS) ||
	    uregs[T_ES] != rp[R_ES] && invalidseg(up, rp[R_ES], R_ES))
		return EINVAL;

	/* EIP must be within the range of CS's descriptor. */
	dt = ((rp[R_CS] & SEL_LDT) ? DT_LDT : DT_GDT);
	dp = up->u_dt_infop[dt];
	descp = &dp->di_table[seltoi(rp[R_CS])];
	limit = SD_GET_LIMIT(descp);
	if (SD_GET_ACC2(descp) & GRANBIT)
		limit = limit << 12 | 0xFFF;
	if (rp[R_EIP] > limit)
		return EINVAL;

	/* FS and GS are not saved in the trap frame. */
	if (up->u_lwpp == u.u_lwpp) {
		if (_fs() != rp[R_FS] && invalidseg(up, rp[R_FS], R_FS) ||
		    _gs() != rp[R_GS] && invalidseg(up, rp[R_GS], R_GS))
			return EINVAL;
		_wfs(rp[R_FS]);
		_wgs(rp[R_GS]);
	} else {
		if (up->u_kcontext.kctx_fs != rp[R_FS] &&
		    invalidseg(up, rp[R_FS], R_FS) ||
		    up->u_kcontext.kctx_gs != rp[R_GS] &&
		    invalidseg(up, rp[R_GS], R_GS))
			return EINVAL;
		up->u_kcontext.kctx_fs = rp[R_FS];
		up->u_kcontext.kctx_gs = rp[R_GS];
	}

	/*
	 * Set the user registers stored in the trap frame to the 
	 * contents in the specified gregset_t object.
	 */

	uregs[T_SS] = rp[R_SS]; 
	uregs[T_UESP] = rp[R_ESP];
	uregs[T_CS] = rp[R_CS]; 
	uregs[T_EIP] = rp[R_EIP];
	uregs[T_DS] = rp[R_DS]; 
	uregs[T_ES] = rp[R_ES]; 
	uregs[T_EAX] = rp[R_EAX];
	uregs[T_ECX] = rp[R_ECX];
	uregs[T_EDX] = rp[R_EDX];
	uregs[T_EBX] = rp[R_EBX];
	uregs[T_EBP] = rp[R_EBP];
	uregs[T_ESI] = rp[R_ESI];
	uregs[T_EDI] = rp[R_EDI];

	uregs[T_EFL] = (uregs[T_EFL] & ~PS_USERMASK) |
			(rp[R_EFL] & PS_USERMASK);

	return 0;
}


/*
 * void prgetfpregs(user_t *up, fpregset_t fp)
 *	Return floating point registers.
 *
 * Calling/Exit State:
 *	The caller must guarantee that the u-area is locked in core.
 */
void
prgetfpregs(user_t const *up, fpregset_t *fp)
{
	bcopy(&up->u_kcontext.kctx_fpregs.fp_reg_set, &fp->fp_reg_set,
	      sizeof fp->fp_reg_set);
}

/*
 * int prsetfpregs(user_t *up, register fpregset_t *fp)
 *	Set floating-point registers.
 *	Returns 0 for success or
 *	EINVAL if *fp contains invalid register values.
 *
 * Calling/Exit State:
 *	The caller must guarantee that the u-area is locked in core.
 */
int
prsetfpregs(user_t *up, fpregset_t const *fp)
{
	bcopy(&fp->fp_reg_set, &up->u_kcontext.kctx_fpregs.fp_reg_set,
	      sizeof fp->fp_reg_set);
	return 0;
}


/*
 * int
 * proc_setup_f(proc_t *prp, int cond)
 *	Family-specific process setup.
 *
 * Calling/Exit State:
 *	Called with exclusive access to the process, prp, so no locks
 *	are needed.  Cond has the newproc() flags.
 */
/* ARGSUSED */
int
proc_setup_f(proc_t *prp, int cond)
{
	SLEEP_INIT(&prp->p_tsslock, 0, &tsslkinfo, KM_SLEEP);
	prp->p_tssp = NULL;
	return 0;
}


/*
 * void
 * proc_cleanup_f(proc_t *prp)
 *	Family-specific process cleanup.
 *
 * Calling/Exit State:
 *	Called with exclusive access to the process, prp, so no locks
 *	are needed.  Called for both cleanup of failed proc_setup() and
 *	before final destruction of an exitted process.
 */
void
proc_cleanup_f(proc_t *prp)
{
	ASSERT(prp->p_tssp == NULL);
	SLEEP_DEINIT(&prp->p_tsslock);
}


/*
 * void
 * lwp_setup_f(lwp_t *lwpp)
 *	Family-specific LWP setup.
 *
 * Calling/Exit State:
 *	Called with exclusive access to the LWP, lwpp, so no locks
 *	are needed.
 */
void
lwp_setup_f(lwp_t *lwpp)
{
	lwpp->l_tssp = NULL;
	lwpp->l_special &= ~SPECF_PRIVTSS;
}


/*
 * void
 * lwp_cleanup_f(lwp_t *lwpp)
 *	Family-specific LWP cleanup.
 *
 * Calling/Exit State:
 *	Called with exclusive access to the LWP, lwpp, so no locks
 *	are needed.  Called for both cleanup of failed lwp_setup() and
 *	before final destruction of an exitted LWP.  In the failed
 *	lwp_setup case, lwpp->l_up will be NULL, indicating that the LWP
 *	never really existed.
 */
void
lwp_cleanup_f(lwp_t *lwpp)
{
	struct stss *tssp;

	if (lwpp->l_up) {
		dt_reset(lwpp);
		if (lwpp->l_up->u_kcontext.kctx_debugon)
			prdebugoff(lwpp);
	}
	if ((tssp = lwpp->l_tssp) != NULL) {
		ASSERT(tssp->st_refcnt >= 1);
		if (--tssp->st_refcnt == 0) {
			ASSERT(lwpp->l_procp->p_tssp != tssp);
			kmem_free(tssp, tssp->st_allocsz);
		}
	}
}


/*
 * void
 * destroy_proc_f(void)
 *	Family-specific process destruction processing.
 *
 * Description:
 *	Called during exit and exec to tear down any special process
 *	state which should not survive an exit or exec.
 *
 * Calling/Exit State:
 *	Caller guarantees there are no other LWPs in the process.
 */
void
destroy_proc_f(void)
{
	/* Reset GDT and LDT to defaults for exec. */
	dt_reset(u.u_lwpp);

	iobitmap_reset();
}


/*
 * void
 * qswtch_f(lwp_t *lwp)
 *	Family-specific involuntary preemption processing.
 *
 * Calling/Exit State:
 *	[See qswtch().]
 */
/* ARGSUSED */
void
qswtch_f(lwp_t *lwp)
{
}


/*
 * int
 * set_dt_entry(ushort_t sel, const struct segment_desc *sdp)
 *	Change an LDT or GDT entry for the current process.
 *
 * Calling/Exit State:
 *	sel is a segment selector; this indicates
 *	which LDT or GDT entry to change.
 *
 *	sdp points to the descriptor to set the entry to.
 *
 *	No locks are held on entry or on exit.
 *	This routine may block.
 *
 *	Returns 0 for success or errno for error.
 */
int
set_dt_entry(ushort_t sel, const struct segment_desc *sdp)
{
	struct segment_desc *odtp;
	size_t size;
	uint_t idx, dt;
	struct desctab_info *dp;

	ASSERT(KS_HOLD0LOCKS());

	dt = ((sel & SEL_LDT) ? DT_LDT : DT_GDT);
	dp = u.u_dt_infop[dt];

	size = ((idx = seltoi(sel)) + 1) * sizeof(struct segment_desc);

	if (dp == &myglobal_dt_info[dt]) {
		/*
		 * First time allocation.
		 */
		dt_new_table(dt, NULL, size);
		dp = u.u_dt_infop[dt];
	} else if (size > dp->di_size) {
		/*
		 * Need to grow.
		 */
		if (dp->di_installed) {
			/* Can't grow installed table. */
			return EINVAL;
		}
		dt_grow_table(dt, size);
		dp = u.u_dt_infop[dt];
	}

	dp->di_table[idx] = *sdp;
	if (dt == DT_LDT && idx < STDLDTSZ)
		u.u_lwpp->l_special |= SPECF_NONSTDLDT;

	if (idx == seltoi(_fs()))
		_wfs(_fs());
	if (idx == seltoi(_gs()))
		_wgs(_gs());

	return 0;
}


/*
 * int
 * set_private_dt(uint_t dt, struct segment_desc *table, size_t size)
 *	Install/de-install private fixed-size descriptor table.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 *	If table is non-NULL, it will be installed as the indicated
 *	descriptor table (GDT if dt == DT_GDT, LDT if dt == DT_LDT),
 *	and will remain fixed at the indicated size (in bytes).
 *	The memory for table must be locked-down (e.g. by acquiring
 *	it using kmem_alloc()).
 *
 *	If table is NULL, a previously installed descriptor table
 *	will be un-installed.  The previous state of the descriptor
 *	table will be restored.  It is the caller's responsibility
 *	to free the memory subsequently.
 */
int
set_private_dt(uint_t dt, struct segment_desc *table, size_t size)
{
	struct desctab_info *dp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(dt == DT_GDT || dt == DT_LDT);

	dp = u.u_dt_infop[dt];

	if (table == NULL) {
		/* De-install private table */

		if (!dp->di_installed)
			return EINVAL;
		u.u_dt_infop[dt] = dp->di_link;
		if (dt == DT_GDT) {
			if (u.u_dt_infop[DT_GDT] == &myglobal_dt_info[DT_GDT])
				dt_reset_gdt();
			else {
				ASSERT(l.special_lwp & SPECF_PRIVGDT);
				DISABLE_PRMPT();
				BUILD_TABLE_DESC(&u.u_gdt_desc,
					 u.u_dt_infop[DT_GDT]->di_table,
					 u.u_dt_infop[DT_GDT]->di_size /
						sizeof(struct segment_desc));
				ENABLE_PRMPT();
				loadgdt(&u.u_gdt_desc);
			}
		} else {
			if (u.u_dt_infop[DT_LDT] == &myglobal_dt_info[DT_LDT])
				dt_reset_ldt();
			else {
				ASSERT(l.special_lwp & SPECF_PRIVLDT);
				DISABLE_PRMPT();
				BUILD_SYS_DESC(&u.u_ldt_desc,
					       u.u_dt_infop[DT_LDT]->di_table,
					       u.u_dt_infop[DT_LDT]->di_size,
					       LDT_KACC1, LDT_ACC2);
				u.u_dt_infop[DT_GDT]->
					di_table[seltoi(KPRIVLDTSEL)] =
								u.u_ldt_desc;
				ENABLE_PRMPT();
				loadldt(KPRIVLDTSEL);
			}
		}
		kmem_free(dp, sizeof *dp);
		return 0;
	}

	ASSERT(size >= LDTSZ * sizeof(struct segment_desc));
	ASSERT(size % sizeof(struct segment_desc) == 0);

	if (dp->di_installed)
		return EBUSY;

	dt_new_table(dt, table, size);

	return 0;
}


/*
 * STATIC void
 * dt_new_table(uint_t dt, struct segment_desc *ndtp, size_t size)
 *	Allocate a new descriptor table.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
dt_new_table(uint_t dt, struct segment_desc *ndtp, size_t size)
{
	struct desctab_info *ndp;
	size_t osize;

	ndp = kmem_alloc(sizeof *ndp, KM_SLEEP);
	ndp->di_link = u.u_dt_infop[dt];
	ndp->di_installed = (ndtp != NULL);

	osize = u.u_dt_infop[dt]->di_size;
	if (size < osize) {
		ASSERT(ndtp == NULL);
		size = osize;
	}
	if (ndtp == NULL)
		ndtp = kmem_alloc(size, KM_SLEEP);
	bcopy(u.u_dt_infop[dt]->di_table, ndtp, osize);
	if (size != osize)
		bzero((char *)ndtp + osize, size - osize);

	ndp->di_table = ndtp;
	ndp->di_size = size;

	u.u_dt_infop[dt] = ndp;

	if (dt == DT_GDT) {
		DISABLE_PRMPT();
		BUILD_TABLE_DESC(&u.u_gdt_desc, ndtp,
				 size / sizeof (struct segment_desc));
		ENABLE_PRMPT();
		l.special_lwp = (u.u_lwpp->l_special |= SPECF_PRIVGDT);
		loadgdt(&u.u_gdt_desc);
	} else {
		DISABLE_PRMPT();
		BUILD_SYS_DESC(&u.u_ldt_desc, ndtp, size, LDT_KACC1, LDT_ACC2);
		ENABLE_PRMPT();
		l.special_lwp = (u.u_lwpp->l_special |= SPECF_PRIVLDT);
		u.u_dt_infop[DT_GDT]->di_table[seltoi(KPRIVLDTSEL)] =
								u.u_ldt_desc;
		loadldt(KPRIVLDTSEL);
	}
}

/*
 * STATIC void
 * dt_grow_table(uint_t dt, size_t size)
 *	Expand an existing private descriptor table.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
dt_grow_table(uint_t dt, size_t size)
{
	struct desctab_info *dp = u.u_dt_infop[dt];
	struct segment_desc *ndtp;
	struct segment_desc *odtp = dp->di_table;
	size_t osize = dp->di_size;

	ASSERT(dp != &myglobal_dt_info[dt]);
	ASSERT(dp->di_table != myglobal_dt_info[dt].di_table);
	ASSERT(!dp->di_installed);
	ASSERT(size > osize);

	ndtp = kmem_alloc(size, KM_SLEEP);
	bcopy(odtp, ndtp, osize);
	if (size != osize)
		bzero((char *)ndtp + osize, size - osize);

	dp->di_table = ndtp;
	dp->di_size = size;

	if (dt == DT_GDT) {
		DISABLE_PRMPT();
		BUILD_TABLE_DESC(&u.u_gdt_desc, ndtp,
			size / sizeof(struct segment_desc));
		ENABLE_PRMPT();
		l.special_lwp = (u.u_lwpp->l_special |= SPECF_PRIVGDT);
		loadgdt(&u.u_gdt_desc);
	} else {
		DISABLE_PRMPT();
		BUILD_SYS_DESC(&u.u_ldt_desc, ndtp, size, LDT_KACC1, LDT_ACC2);
		u.u_dt_infop[DT_GDT]->di_table[seltoi(KPRIVLDTSEL)] =
								u.u_ldt_desc;
		ENABLE_PRMPT();
		l.special_lwp = (u.u_lwpp->l_special |= SPECF_PRIVLDT);
		loadldt(KPRIVLDTSEL);
	}

	kmem_free(odtp, osize);
}


/*
 * STATIC void
 * dt_reset(lwp_t *lwpp)
 *	Reset the specified LWP to use the standard GDT and LDT.  This is
 *	called in two situations: to reset the descriptors of an LWP
 *	performing an exec(), and when an LWP is being destroyed.  In
 *	the first case, the LWP is on CPU of course, and the l. fields
 *	must be updated and the tables must be loaded.  In the second
 *	case, the target LWP is essentially already gone, and this
 *	function's main job is just to free the memory that held the
 *	tables.
 *
 * Calling/Exit State:
 *	No locks required on entry.
 */
STATIC void
dt_reset(lwp_t *lwpp)
{
	struct desctab_info *dp;
	user_t *up = lwpp->l_up;
	uint_t dt;

	if (up == &u) {
		/*
		 * Only need to reset the actual descriptors if running
		 * in context.  If not in context, the LWP is being destroyed
		 * and will never run again.
		 */
		if (up->u_dt_infop[DT_GDT] != &myglobal_dt_info[DT_GDT])
			dt_reset_gdt();
		if (up->u_dt_infop[DT_LDT] != &myglobal_dt_info[DT_LDT])
			dt_reset_ldt();
	}

	for (dt = 0; dt < NDESCTAB; ++dt) {
		while ((dp = up->u_dt_infop[dt]) != &myglobal_dt_info[dt]) {
			if (!dp->di_installed) {
				/*
				 * Free the table space, but only if it was not
				 * installed by an external client.  It is the
				 * client's responsibility to free the space.
				 * (Preferably, though, it would have already
				 * de-installed its table by this point.)
				 */
				kmem_free(dp->di_table, dp->di_size);
			}
			up->u_dt_infop[dt] = dp->di_link;
			kmem_free(dp, sizeof *dp);
		}
	}
}

/*
 * STATIC void
 * dt_reset_gdt(void)
 *	Reset the global descriptor table for the current LWP.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
dt_reset_gdt(void)
{
	DISABLE_PRMPT();
	BUILD_TABLE_DESC(&u.u_gdt_desc, l.global_gdt, GDTSZ);
	l.special_lwp = (u.u_lwpp->l_special &= ~(SPECF_PRIVGDT));
	loadgdt(&u.u_gdt_desc);
	ENABLE_PRMPT();
}

/*
 * STATIC void
 * dt_reset_ldt(void)
 *	Reset the local descriptor table for the current LWP.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
dt_reset_ldt(void)
{
	DISABLE_PRMPT();
	loadldt(KLDTSEL);
	l.special_lwp = (u.u_lwpp->l_special &= ~(SPECF_PRIVLDT));
	ENABLE_PRMPT();
}


/*
 * void
 * set_private_idt(struct gate_desc *idtp)
 *	Install private IDT.
 *
 * Calling/Exit State:
 *	Must be called with preemption disabled (usually arranged
 *	implicitly by being called from context-switch).
 *
 *	idtp points to the new IDT, which must be 2048 bytes long,
 *	aligned on an 8-byte boundary, and non-pageable.
 */
void
set_private_idt(struct gate_desc *idtp)
{
	struct desctab desctab;

	ASSERT(prmpt_count > 0);

	cur_idtp = idtp;
	BUILD_TABLE_DESC(&desctab, idtp, IDTSZ);
	loadidt(&desctab);
}

/*
 * void
 * restore_std_idt(void)
 *	Restore standard IDT.
 *
 * Calling/Exit State:
 *	Must be called with preemption disabled (usually arranged
 *	implicitly by being called from context-switch).
 */
void
restore_std_idt(void)
{
	ASSERT(prmpt_count > 0);

	cur_idtp = l.std_idt;
	/*
	 * Intel Workaround for the "Invalid Operand
	 * with Locked Compare Exchange 8Byte (CMPXCHG8B) Instruction"
	 */
	if (l.cpu_id == CPU_P5) {
		extern struct desctab p5_std_idt_desc;

		loadidt(&p5_std_idt_desc);
	}
	else
		loadidt(&mystd_idt_desc);
}

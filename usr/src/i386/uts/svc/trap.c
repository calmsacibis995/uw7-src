#ident	"@(#)kern-i386:svc/trap.c	1.69.6.3"
#ident	"$Header$"

/*
 * trap() exception handler.
 */

#include <acc/audit/audit.h>
#include <fs/procfs/procfs.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/iobitmap.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/ucontext.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/debugreg.h>
#include <svc/errno.h>
#include <svc/fault.h>
#include <svc/fp.h>
#include <svc/msr.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <svc/trap.h>
#include <svc/v86bios.h>
#ifdef MERGE386
#include <svc/mki.h>
#endif
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#endif

extern boolean_t upageflt(sigqueue_t **, vaddr_t *);
extern boolean_t kpageflt(uint_t);
extern void addupc(void(*)(), int);
extern void fpu_init(void), fpu_init_old(void);
extern void fpu_restore(void);
extern void fpu_disable(void);
extern void psig(void);
extern int check_preemption_f(void);
extern int v86bios_error;

int stop_on_fault(uint_t, sigqueue_t *);
STATIC boolean_t kern_gpfault(int *r0ptr);
STATIC void fpu_clex(void);

#ifndef NODEBUGGER
void (* volatile cdebugger)() = nullsys;
int demon_call_type = -1;	/* Debugger type code for inline INT 3 traps */
uint_t user_debug_count;	/* # LWPs with u.u_debugon */
fspin_t debug_count_mutex;	/* Lock for user_debug_count */
#endif

#ifdef	DEBUG
int	syscalltrace = 0;
int	traptrace = 0;
int	abtdebug = 0;
int	emuladebug = 0;
#endif	/* DEBUG  */

asm void setfpcw(newcwaddr)
{
%	mem	newcwaddr;
	movl	newcwaddr, %eax
	fldcw	(%eax)
}
#pragma asm partial_optimization setfpcw

asm void getfpenv(envaddr)
{
%	mem	envaddr;
	movl	envaddr, %eax
	fnstenv	(%eax)
}
#pragma asm partial_optimization getfpenv

static int fperrtocode(unsigned int, unsigned int);

static char *trap_type[] = {
	"Integer Zero-Divide",
	"Single Step",
	"Non-Maskable Interrupt",
	"Breakpoint",
	"Interrupt On Overflow",
	"Array Bounds Check",
	"Undefined/Illegal Op-Code",
	"Math Coprocessor Not Present",
	"Double Fault",
	"Math Coprocessor Overrun",
	"Invalid Task-State Segment",
	"Segment Not Present",
	"Stack Fault",
	"General Protection Fault",
	"Page Fault",
	"Reserved Exception 15",
	"Math Coprocessor Error",
	"Alignment Fault",
	"Machine Check"
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

/*
 * void
 * trap(int, uint, uint, uint, uint, uint, uint, uint, uint, uint,
 *		uint, uint, uint, uint)
 *	Deal with a processor trap.
 *
 * Calling/Exit State:
 *
 *	type   : type of trap
 *	edi    : register variable
 *	esi    : register variable
 *	ebp    : of interrupted frame
 *	unused : temp SP (from push-all instruction)
 *	ebx    : register variable
 *	edx    : register variable
 *	ecx    : register variable
 *	eax    : scratch register
 *	eip    : return context
 *	cs     : sense user-mode entry from RPL field
 *	flags  : extended flags
 *	esp    : only if inter-segment (user->kernel)
 *	ss     : only if inter-segment (user->kernel)
 *
 * 	Returns: None.
 *
 *	Although it is not the normal case, under some conditions
 *	this routine may be entered while holding locks or at elevated
 *	ipl levels.  In general, since trap() is called as a result
 *	of hardware conditions, it is best not to make assumptions.
 *	However, if trap() was entered as a result of a user-mode exception,
 *	it will always be true that no locks are held and the ipl is PLBASE.
 *
 * Description:
 *	The major component of this function is switch statement
 *	based on the trap type.
 */

/* ARGSUSED */
void
trap(volatile int type, volatile uint edi, volatile uint esi,
	volatile uint ebp, volatile uint unused, volatile uint ebx,
	volatile uint edx, volatile uint ecx, volatile uint eax,
	volatile uint es, volatile uint ds,
	volatile uint eip, volatile uint cs, volatile uint flags,
	volatile uint esp, volatile uint ss)
{
	uint_t fcflags, fault = 0;
	sigqueue_t *sig = NULL;
	lwp_t *lwpp;
	vaddr_t	faultaddr;
	boolean_t was_usingfpu;
	pl_t pl;
	ulong_t         *save_kpgflt_stack;

	unsigned int fpstate[7];
	unsigned int _fpsw_;
	unsigned int _fpcw_;
	caddr_t _fpaddr_;

	/*
	 * Save current fault-catch flags;
	 * set to zero for duration of trap processing,
	 * so we don't incorrectly "catch" bad pointer dereferences.
	 */

	fcflags = u.u_fault_catch.fc_flags;
	u.u_fault_catch.fc_flags = 0;

	/*
	 * Trap handling for V86BIOS
	 */

	if (V86BIOS(flags)) {
		if (!v86bios_chkopcode(eip, type)) {
			/*
			 * => Restricted I/O port access:
			 *	Ignore the instruction.
			 * => int N:
			 *	Redirect the int call.
			 */
			goto v86bios_exit;	
		} else {
			/*
			 * Unrecoverable faults
			 */
			v86bios_error = 1;
			v86bios_leave();
			/* NOTREACHED */
		}
	}

	if (USERMODE(cs, flags)) {
		type |= USERFLT;
		ASSERT(!servicing_interrupt());
		ASSERT(KS_HOLD0LOCKS());
		ASSERT(getpl() == PLBASE);
		lwpp = u.u_lwpp;
		ASSERT(lwpp != NULL);
		u.u_ar0 = (int *)&eax;
		lwpp->l_start = lwpp->l_stime;
#ifdef PERFMTR_TRAP_CALLBACKS
		_trapentry_hook(lwpp, type);
#endif /* PERFMTR_TRAP_CALLBACKS */
	}

	switch (type) {

	default:
	{
		char *modename = ((type & USERFLT) ? "user" : "kernel");

		/*
		 * For restricted I/O port access.
		 * Some BIOS calls access critical ports, such as i8259
		 * command ports.
		 */ 
		if ((type &= ~USERFLT) >= TRAP_TYPES) {
			/*
			 *+ A hardware exception of an unrecognized type
			 *+ occurred.
			 */
			cmn_err(CE_PANIC, "Unknown trap type 0x%x in %s mode.",
					  type, modename);
		} else {
			/*
			 *+ An unexpected hardware exception occurred.
			 *+ This indicates either a serious hardware problem
			 *+ or kernel or driver software error.
			 */
			cmn_err(CE_PANIC, "%s trap in %s mode.",
					  trap_type[type], modename);
		}
		/* NOTREACHED */
	}

	case BPTFLT:		/* breakpoint */
#ifndef NODEBUGGER
		if (cdebugger != nullsys) {
			int	reason = DR_BPT3;

			if (demon_call_type != -1) {
				/*
				 * Inline debugger request.
				 * Convert to specified type.
				 */
				reason = demon_call_type;
				demon_call_type = -1;
			}
			(*cdebugger)(reason, &eax);
			break;
		}
#endif
		/*
		 *+ A kernel-mode debugger breakpoint occurred,
		 *+ even though there's no kernel debugger.
		 */
		cmn_err(CE_PANIC, "Unexpected INT 3 breakpoint in kernel mode");
		/* NOTREACHED */

	case BPTFLT+USERFLT:
		/*
		 * User breakpoint; send SIGTRAP:TRAP_BRKPT.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGTRAP;
		sig->sq_info.si_code = TRAP_BRKPT;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTBPT;
		break;

	case SGLSTP:
	case SGLSTP+USERFLT: {	/* Debugger traps and faults */
		uint_t debugstatus, dr6, dr7, code;

		debugstatus = dr6 = _dr6();
		dr7 = _dr7();

		_wdr6(0);

		/* Mask out disabled breakpoints */
		if (!(dr7 & DR_ENABLE0))
			debugstatus &= ~DR_TRAP0;
		if (!(dr7 & DR_ENABLE1))
			debugstatus &= ~DR_TRAP1;
		if (!(dr7 & DR_ENABLE2))
			debugstatus &= ~DR_TRAP2;
		if (!(dr7 & DR_ENABLE3))
			debugstatus &= ~DR_TRAP3;

		/* Now evaluate how we got here */
		if (debugstatus & DR_SINGLESTEP) {
			/* turn off trace */
			((flags_t *)&flags)->fl_tf = 0;

			if (!(type & USERFLT)) {
				/*
				 * The i386 even single-steps through lcalls
				 * which change the privilege level, in which
				 * case we take the trap in kernel mode rather
				 * than user mode.  We detect this case by
				 * checking if %eip is at any of the kernel
				 * call gate starting addresses.
				 *
				 * If this is the case, we want to deliver the
				 * signal at the end of the system call, not
				 * now, so we don't interrupt interruptable
				 * system calls.  So we set a flag to indicate
				 * that upon completion of the system call,
				 * a SIGTRAP:TRAP_TRACE signal should be
				 * delivered.
				 */
				if (eip == (uint_t)sys_call ||
				    eip == (uint_t)sig_clean) {
					ASSERT(getpl() == PLBASE);
					(void) LOCK(&u.u_lwpp->l_mutex, PLHI);
					u.u_lwpp->l_trapevf |= EVF_L_SIGTRAP;
					UNLOCK(&u.u_lwpp->l_mutex, PLBASE);
					break;
				}
#ifndef NODEBUGGER
				if (cdebugger != nullsys) {
					_wdr6(debugstatus);
					(*cdebugger)(DR_STEP, &eax);
					_wdr6(0);
					break;
				}
#endif
				/*
				 *+ A single-step trap occurred in kernel mode,
				 *+ even though there's no kernel debugger.
				 *+ It is ignored; no action necessary.
				 */
				cmn_err(CE_WARN,
					"Unexpected kernel-mode single-step");
				break;
			}
			code = TRAP_TRACE;
			fault = FLTTRACE;
		}
		else if (debugstatus & DR_TRAPS) {
			((flags_t *)&flags)->fl_rf = 1;
			if (user_debug_count == 0) {
				/* Must be a kernel debugger breakpoint */
#ifndef NODEBUGGER
				if (cdebugger != nullsys) {
					_wdr6(debugstatus);
					(*cdebugger)(DR_BPT1, &eax);
					_wdr6(0);
					break;
				}
#endif
				/*
				 *+ An INT 1 breakpoint fault occurred,
				 *+ even though there's no kernel debugger
				 *+ and no user debugger is using the debug
				 *+ registers.
				 *+ It is ignored; no action necessary.
				 */
				cmn_err(CE_WARN,
					"Unexpected breakpoint fault (INT 1)");
				break;
			}
			code = TRAP_BRKPT;
			fault = FLTBPT;
		}
		else if (type & USERFLT) {
			/*
			 * Breakpoint occured for no evident reason;
			 * just go ahead and send a signal to the LWP.
			 * Note that reserved opcode 0xF1 has been known
			 * to produce this behavior if executed by the
			 * user program.
			 */
			code = TRAP_BRKPT;
			fault = FLTBPT;
		}
		else {
			/*
			 *+ An INT 1 breakpoint trap occurred, but no
			 *+ breakpoints or single-stepping were enabled.
			 *+ It is ignored; no action necessary.
			 */
			cmn_err(CE_WARN,
				"Unexpected INT 1 trap; dr6 = 0x%x, dr7 = 0x%x",
				dr6, dr7);
			break;
		}

		u.u_kcontext.kctx_dbregs.debugreg[6] = dr6;

		ASSERT(KS_HOLD0LOCKS());
		ASSERT(getpl() == PLBASE);

		/* Deliver SIGTRAP */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGTRAP;
		sig->sq_info.si_code = code;
		if (type & USERFLT)
			sig->sq_info.si_addr = (caddr_t)eip;
		else {
			sig->sq_info.si_addr = (caddr_t)u.u_ar0[T_EIP];
			lwpp = u.u_lwpp;
			ASSERT(lwpp != NULL);
		}
		break;
	}

	case INVOPFLT+USERFLT:
p5_workaround:			/* type has been changed in p5_bug() */
		/*
		 * Invalid opcode; send SIGILL:ILL_ILLOPC.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGILL;
		sig->sq_info.si_code = ILL_ILLOPC;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTILL;
		break;

	case DIVERR+USERFLT:
		/*
		 * Divide by 0 error; send SIGFPE:FPE_INTDIV.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGFPE;
		sig->sq_info.si_code = FPE_INTDIV;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTIZDIV;
		break;

	case INTOFLT+USERFLT:
		/*
		 * Integer overflow; send SIGSEGV:SEGV_MAPERR.
		 * This should be SIGFPE:FPE_INTOVF but i386 ABI
		 * requires SIGSEGV:SEGV_MAPERR.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGSEGV;
		sig->sq_info.si_code = SEGV_MAPERR;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTIOVF;
		break;

	case BOUNDFLT+USERFLT:
		/*
		 * Subscript out of range; send SIGSEGV:SEGV_MAPERR.
		 * This should be SIGFPE:FPE_FLTSUB but i386 ABI
		 * requires SIGSEGV:SEGV_MAPERR.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGSEGV;
		sig->sq_info.si_code = SEGV_MAPERR;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTBOUNDS;
		break;

	case GPFLT+USERFLT:
		/*
		 * General protection violation.
		 */
		if (u.u_lwpp) {
			/*
			 * Fault may have been caused by an out-of-date
			 * I/O bitmap.  Sync it up and try again.
			 */
			if (iobitmap_sync())
				break;
		}
		/*
		 * Send SIGSEGV:SEGV_MAPERR.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGSEGV;
		sig->sq_info.si_code = SEGV_MAPERR;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTBOUNDS;
		break;

	case ALIGNFLT+USERFLT:
		/*
		 * Alignment fault; send SIGBUS:BUS_ADRALN
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGBUS;
		sig->sq_info.si_code = BUS_ADRALN;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTACCESS;
		break;

	case STKFLT+USERFLT:
		/*
		 * Stack fault; send SIGILL:ILL_BADSTK.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGILL;
		sig->sq_info.si_code = ILL_BADSTK;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTSTACK;
		break;

	case INVTSSFLT+USERFLT:
		/*
		 * Invalid TSS fault (iret w/NT); send SIGILL:ILL_ILLOPN.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGILL;
		sig->sq_info.si_code = ILL_ILLOPN;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTILL;
		break;

	case NOEXTFLT+USERFLT:
		if (fp_kind == FP_NO) {
			/*
			 * Send SIGFPE:FPE_FLTINV.
			 */
			sig = siginfo_get(KM_SLEEP, 0);
			sig->sq_info.si_signo = SIGFPE;
			sig->sq_info.si_code = FPE_FLTINV;
			sig->sq_info.si_addr = (caddr_t)eip;
			fault = FLTFPE;
			break;
		}
		/*
		 * LWP quits using FPU at each context switch.
		 * This speeds up context switch for FPU users when there
		 * is no use of FPU between switches, at the expense of up
		 * to an additional trap per dispatch of an FPU-using process.
		 */
		if (!u.u_kcontext.kctx_fpvalid) {
			if (fp_kind == FP_SW) {
				extern void user_fpu_init(void);
				extern size_t user_fpu_size;
				extern void user_fpu_init_old(void);
				extern size_t user_fpu_old_size;
				void (*func)(void);
				size_t size;

				/*
				 * Arrange for the user to take care of its
				 * own initialization, since we can't use the
				 * emulator from the kernel.
				 */
				if (isSCO) {
					func = user_fpu_init_old;
					size = user_fpu_old_size;
				} else {
					func = user_fpu_init;
					size = user_fpu_size;
				}
				if (copyout((void *)func,
					    (void *)(esp -= size),
					    size) == -1 ||
				    suword((int *)(esp - 8), eip) == -1) {
					esp += size;
					/*
					 * Send SIGSEGV:SEGV_MAPERR.
					 */
					sig = siginfo_get(KM_SLEEP, 0);
					sig->sq_info.si_signo = SIGSEGV;
					sig->sq_info.si_code = SEGV_MAPERR;
					sig->sq_info.si_addr = (caddr_t)eip;
					fault = FLTBOUNDS;
					break;
				}
				eip = esp;	/* "jump" to user_fpu_init */
				esp -= 8;
				DISABLE_PRMPT();
				u.u_kcontext.kctx_fpvalid = using_fpu = B_TRUE;
				uvwin.uv_fp_used = u.u_fp_used = B_TRUE;
				cur_idtp[NOEXTFLT] = fpuon_noextflt;
				ENABLE_PRMPT();
			} else {
				DISABLE_PRMPT();
				u.u_kcontext.kctx_fpvalid = using_fpu = B_TRUE;
				uvwin.uv_fp_used = u.u_fp_used = B_TRUE;
				if (isSCO)
					fpu_init_old();
				else
					fpu_init();
				ENABLE_PRMPT();
			}
			u.u_kcontext.kctx_fpregs.fp_reg_set.fpchip_state.status
									= 0;
		} else {
			DISABLE_PRMPT();
			fpu_restore();
			uvwin.uv_fp_used = u.u_fp_used = B_TRUE;
			ENABLE_PRMPT();
		}
		break;

	case NOEXTFLT:
		/*
		 *+ The kernel attempted to use the floating point unit;
		 *+ the kernel should not use the floating point.  Corrective
		 *+ action:  examine third-party, drivers or other kernel
		 *+ configured software for usage of the floating point unit.
		 */
		cmn_err(CE_PANIC, "Kernel Usage of Floating Point");
		/* NOTREACHED */

	case EXTERRFLT+USERFLT:
		/*
		 * User incurred an FPU error trap.  We will save the
		 * FPU state and send the SIGFPE signal to the LWP
		 * incurring the trap.  The key thing to note is that we
		 * want to provide a clean FPU for the signal handler to
		 * use.  We save the FPU state in savecontext() and
		 * sendsig() will clear kctx_fpvalid flag for the current
		 * LWP.  If the signal handler uses the FPU, we take the
		 * EXTERRFLT (save_fpu() also disables the FPU) and we will
		 * initialize the FPU since kctx_fpvalid flag is not set.
		 *
		 * Note that sendsig() will ensure that each signal
		 * handler will get a clean FPU. This works even in the
		 * case where we process one or more other signals before
		 * getting around to handling the SIGFPE signal we will
		 * shortly be sending!
		 */

		if (!using_fpu) {
			/*
			 *+ User-mode encountered a FPU (i387) exception
			 *+ when kernel-mode state indicated that the user
			 *+ was not currently using the FPU. User cannot
			 *+ take any corrective action.
			 */
			cmn_err(CE_WARN, "FPU fault and using_fpu not set");
			break;
		}

		if (fp_kind == FP_SW) {
		_fpcw_ = *(ushort_t *)&l.fpe_kstate.fpe_state.fp_emul[0];
		_fpsw_ = *(ushort_t *)&l.fpe_kstate.fpe_state.fp_emul[4];
		_fpaddr_ = *(caddr_t *)&l.fpe_kstate.fpe_state.fp_emul[12];
		} else {
			getfpenv(&fpstate[0]);		/* Get FP Environment */
			_fpcw_ = fpstate[0] & 0x0ffff;	/* Get Control Word */
			_fpsw_ = fpstate[1] & 0x0ffff;	/* Get Status Word */
			_fpaddr_ = (caddr_t)fpstate[3];	/* Get FP Fault Addr */
		}

		/*
		 * Before delivering the signal we have to clear the
		 * exception mask, so the kernel doesn't get stuck in the
		 * fnsave instruction.
		 */
		fpu_clex();

			/* Set CW back to what user wanted */
		if (fp_kind & FP_HW)
			setfpcw(&fpstate[0]);

		/*
		 * Send SIGFPE:FPE_FLTINV.
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGFPE;
		sig->sq_info.si_code = fperrtocode(_fpcw_, _fpsw_);
		sig->sq_info.si_addr = _fpaddr_;
		fault = FLTFPE;
		break;

	case EXTERRFLT:
		/*
		 * FPU error in kernel mode.  This must be as a result of
		 * the fnsave/fwait in context-switch or save_fpu.
		 *
		 * This error is on behalf of the current user context,
		 * as a result of the last FP instruction it executed.
		 * We cannot, however, deliver a signal at this time,
		 * since we may be holding l_mutex.  Instead, we defer
		 * the signal delivery by posting a trap event.
		 *
		 * First, clear the pending exception (after storing it
		 * in the kcontext structure).
		 */
		ASSERT(fp_kind & FP_HW);
		fpu_clex();

		/*
		 * Determine whether or not the fault occurred in a
		 * section of code which is already holding the l_mutex
		 * lock; if not, we need to acquire it here in order to
		 * post the event.
		 */
		{	extern void cswtch_fwait(), save_fpu_fwait(),
				    save_fpu_l_fwait(), saveregs_fwait();

			if (eip != (uint_t)cswtch_fwait &&
			    eip != (uint_t)save_fpu_l_fwait) {
				pl = LOCK(&u.u_lwpp->l_mutex, PLHI);
				u.u_lwpp->l_trapevf |= EVF_L_SIGFPE;
				UNLOCK(&u.u_lwpp->l_mutex, pl);
			} else {
				ASSERT(eip == (uint_t)save_fpu_fwait ||
				       eip == (uint_t)saveregs_fwait);
				u.u_lwpp->l_trapevf |= EVF_L_SIGFPE;
			}
		}
		break;

	case EXTOVRFLT+USERFLT:
		/*
		 * User incurred an FPU segment overrun.  We must reset
		 * the FPU and deliver a signal to the user.  This means
		 * we lose the current FPU state, but we have no choice,
		 * since the hardware's in a non-restartable state.
		 * Note that to close preemption windows we come here with
		 * preemption disabled.
		 */

		was_usingfpu = using_fpu;

		if (fp_kind & FP_HW) {
			fpu_init();
			fpu_disable();
			uvwin.uv_fp_used = u.u_fp_used = B_FALSE;
		}

		/*
		 * Now that we have disabled the FPU, enable preemption.
		 */
		ENABLE_PRMPT();

		if (!was_usingfpu) {
			/*
			 *+ User-mode encountered a FPU (i387) exception
			 *+ when kernel-mode state indicated that the user
			 *+ was not currently using the FPU. User cannot
			 *+ take any corrective action.
			 */
			cmn_err(CE_WARN, "FPU fault and using_fpu not set");
			break;
		}

		/*
		 * Send SIGSEGV:SEGV_MAPERR;
		 */
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_signo = SIGSEGV;
		sig->sq_info.si_code = SEGV_MAPERR;
		sig->sq_info.si_addr = (caddr_t)eip;
		fault = FLTBOUNDS;
		break;

	case MCEFLT:		/* Machine Check Exception */
	case MCEFLT+USERFLT:
		ASSERT(l.cpu_features[0] & CPUFEAT_MCE);
		ASSERT(!ignore_machine_check);

		if ((l.cpu_features[0] & CPUFEAT_MCA)) {

			/*
			 *  _mca_hook panics if the CPU is not restartable,
			 *  otherwise it returns.
 			 */

			_mca_hook((int *)&eax);

		} else {	/* Pentium has MCE but not MCA */
			ulong_t mceflt_addr[2], mcetype[2];

			_rdmsr(P5_MC_ADDR, mceflt_addr);
			_rdmsr(P5_MC_TYPE, mcetype);

			cmn_err(CE_PANIC,
				"Machine Check Exception in %s mode\n"
				"\tfault address: 0x%x  type: 0x%x\n",
				((type & USERFLT) ? "user" : "kernel"),
				mceflt_addr[0], mcetype[0]);
		}
		break;

	case DBLFLT+USERFLT:
	case DBLFLT: {		/* Double-Fault Exception Task */
		struct tss386 *o_tssp = (struct tss386 *)
				SD_GET_BASE(&u.u_dt_infop[DT_GDT]->
					di_table[seltoi(l.dftss.t_link)]);
		user_t *o_upointer = upointer;

		upointer = &ueng;

		/*
		 * Save register values into the offending context
		 * (from the TSS values saved by the task switch) as if
		 * saveregs were called.
		 */

		o_upointer->u_kcontext.kctx_ecx = o_tssp->t_ecx;
		o_upointer->u_kcontext.kctx_edx = o_tssp->t_edx;
		o_upointer->u_kcontext.kctx_eax = o_tssp->t_eax;
		o_upointer->u_kcontext.kctx_ebx = o_tssp->t_ebx;
		o_upointer->u_kcontext.kctx_ebp = o_tssp->t_ebp;
		o_upointer->u_kcontext.kctx_edi = o_tssp->t_edi;
		o_upointer->u_kcontext.kctx_esi = o_tssp->t_esi;
		o_upointer->u_kcontext.kctx_eip = o_tssp->t_eip;
		o_upointer->u_kcontext.kctx_esp = o_tssp->t_esp;
		o_upointer->u_kcontext.kctx_fs = o_tssp->t_fs;
		o_upointer->u_kcontext.kctx_gs = o_tssp->t_gs;

		/*
		 *+ A Double-Fault exception occurred.  This indicates
		 *+ a serious problem in the state of the system.
		 *+ Most often this is caused by kernel stack overflow.
		 */
		cmn_err(CE_PANIC,
			"DBLFLT exception; registers saved in TSS @ 0x%lx;\n"
			"\toriginal upointer = 0x%lx",
			(ulong_t)o_tssp, (ulong_t)o_upointer);
		/* NOTREACHED */
	}

	case PGFLT+USERFLT:
		/*
		 * Call user-mode page-fault routine.  Record the
		 * fault type in cases faults are being traced.  We
		 * should probably try to distinguish more types of
		 * faults than just these.
		 */
		if (upageflt(&sig, &faultaddr))
			fault = FLTPAGE;
		else if (l.cpu_id == CPU_P5) {
			if (p5_bug(&type)) {
				siginfo_free(sig); /* allocated by upageflt */
				goto p5_workaround;
			}
			else
				fault = FLTPAGE;
		}
		else if (sig->sq_info.si_code == SEGV_MAPERR)
			fault = FLTBOUNDS;
		else
			fault = FLTACCESS;
		break;

	case PGFLT:
		/*
		 * Call kernel-mode pageflt routine.
		 */
		save_kpgflt_stack = u.u_kpgflt_stack;
		if (l.panic_level == 0)
			u.u_kpgflt_stack = (ulong_t *) &eax;
		if (!kpageflt(fcflags)) {

			/*
			 * Fatal faults are 'caught' and panic the
			 * machine in kpageflt. Here we are are going to
			 * restore the fault catch flags and return
			 * indicating that we should call the fault
			 * handler (eip).
			 */

			ASSERT(u.u_fault_catch.fc_errno);

			u.u_fault_catch.fc_flags = fcflags;
                        eip = (uint_t)u.u_fault_catch.fc_func;
			u.u_kpgflt_stack = save_kpgflt_stack;
                        return;
		}
		u.u_kpgflt_stack = save_kpgflt_stack;
		break;

	case TRP_PREEMPT+USERFLT:
		if (lwpp->l_trapevf & UPDATE_FLAGS) {
			/* Handle process attribute updates */
			lwp_attrupdate();
		}

		/*
		 * See if we need to give up the cpu.
		 */
		/* Fallthrough */

	case TRP_PREEMPT:
		ASSERT(KS_HOLD0LOCKS());
		ASSERT(getpl() == PLBASE);
		if (!check_preemption_f())
			UPREEMPT();
		break;

	} /* end of switch */

	/* Restore fault-catch flags */
	u.u_fault_catch.fc_flags = fcflags;

	/*
	 * Check for signals to be delivered to the user LWP.
	 * Also, if a debugger has directed the process to stop upon
	 * incurring this fault, do so.  The pending signal may
	 * be cancelled during the stop.
	 */
	ASSERT(fault || sig == NULL);
	if (fault) {
		ASSERT(fault == FLTPAGE || sig != NULL);
		if (prismember(&u.u_procp->p_fltmask, fault)) {
			if (fault == FLTPAGE) {
				/*
				 * No sigqueue_structure was allocated, but
				 * we need one for the debugger.
				 */
				ASSERT(sig == NULL);
				sig = kmem_zalloc(sizeof *sig, KM_SLEEP);
				sig->sq_info.si_addr = (caddr_t)faultaddr;
			}
			if (sig->sq_info.si_signo != SIGKILL) {
				/*
				 * Before allowing the LWP to be visible to
				 * a debugger, make sure it's not in the
				 * middle of the floating-point emulator.
				 */
				if ((ushort_t)u.u_ar0[T_CS] == FPESEL)
					fpeclean();
				if (stop_on_fault(fault, sig) == 0 ||
				    fault == FLTPAGE) {
					kmem_free(sig, sizeof *sig);
					sig = NULL;
				}
			}
		}

		/*
		 * Check if the fault requires a signal to be sent to the user.
		 * If so, deliver it to the current LWP.
		 */
		if (sig) {
			/*
			 * iBCS puts trap type in signal context even
			 * though this information is of dubious value
			 * and non-portable.  Save the trap type in
			 * u_traptype for later use in sendsig().
			 */
			u.u_traptype = type;
			sigsynchronous(lwpp, sig->sq_info.si_signo, sig);
		}
	}

v86bios_exit:
	if (type & USERFLT) {
#ifdef CC_PARTIAL
		/* Treat covert channels if necessary */
		if (u.u_covert.c_bitmap)
			cc_limit_all(&u.u_covert, CRED());
#endif /* CC_PARTIAL */
		CL_TRAPRET(lwpp, lwpp->l_cllwpp);
	}

}


/*
 * void
 * cl_trapret(void)
 *	Invoke scheduling class-specific trap return handling.
 *
 * Calling/Exit State:
 *	Called on return to user mode on return from a trap when
 *	the trap handling code might have blocked.
 */
void
cl_trapret(void)
{
	ASSERT(u.u_lwpp != NULL);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	CL_TRAPRET(u.u_lwpp, u.u_lwpp->l_cllwpp);
}


/*
 * void
 * fpu_error(void)
 *	Handle a floating-point error indication.
 *
 * Calling/Exit State:
 *	Caller must not hold l_mutex.
 *
 * Description:
 *	For h/w which handles notification of FPU errors through other
 *	than the standard EXTERRFLT trap.  (E.g. PC-compatible delivery
 *	through the IRQ13 interrupt.)
 */
void
fpu_error(void)
{
	pl_t pl;

	ASSERT(fp_kind & FP_HW);

	fpu_clex();

	/*
	 * We can't deliver the signal at this time, since we may be
	 * at interrupt level.  Defer it until we return to user mode.
	 */
	pl = LOCK(&u.u_lwpp->l_mutex, PLHI);
	u.u_lwpp->l_trapevf |= EVF_L_SIGFPE;
	UNLOCK(&u.u_lwpp->l_mutex, pl);
}


/*
 * STATIC void
 * fpu_clex(void)
 *	Clear FPU exceptions.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Clears any pending exceptions in the Floating-Point Unit,
 *	first saving the current status word in u.u_kcontext.
 */
STATIC void
fpu_clex(void)
{
	extern ushort_t fpu_stsw_clex(void);

	ASSERT(fp_kind != FP_NO);

	if (fp_kind & FP_HW) {
		/* Get current status and clear exceptions */
		u.u_kcontext.kctx_fpregs.fp_reg_set.fpchip_state.status =
				fpu_stsw_clex();
	} else {
		/*
		 * We can't execute the emulator (and thus FP instructions)
		 * from the kernel.  Also, we need to get control back
		 * after these operations, so we can't use the user_fpu_init
		 * trick to execute the FP instructions from user mode.
		 * Instead, use knowledge of the emulator's use of memory
		 * to access its sr_errors status word.
		 */
#define sr_errors 4
		u.u_kcontext.kctx_fpregs.fp_reg_set.fpchip_state.status =
			*(ushort_t *)&l.fpe_kstate.fpe_state.fp_emul[sr_errors];
		l.fpe_kstate.fpe_state.fp_emul[sr_errors] = 0;
	}
}



/*
 * void
 * evt_process(...)
 *	This function is called prior to returning to user level.
 *	This function performs the necessary trap event processing.
 *
 * Calling/Exit State:
 *	No locks are to be held on entry and none will be held on return.
 *	To fast-path the common interrupt return code, this function
 *	should only be called if there is work to be done!
 *
 * Remarks:
 *	If profiling is turned on, the user level PC is charged
 *	appropriately.  Further, if the LWP was hit by a clock interrupt
 *	(local clock) an extra tick is charged to the user PC if the
 *	previous state was user mode. This state is recorded in the l_flag
 *	field.  If the flag L_CLOCK is set, an extra tick will be charged.
 *
 *	Note that it is assumed that this function will be called with
 *	the standard trap stack frame.
 *
 */

/* ARGSUSED */
void
evt_process(volatile uint_t edi,/* user register */
	volatile uint_t esi,	/* user register */
	volatile uint_t ebp,	/* user frame pointer register */
	volatile uint_t unused,	/* temp from "pushal" instruction */
	volatile uint_t ebx,	/* user register */
	volatile uint_t edx,	/* user register */
	volatile uint_t ecx,	/* user register */
	volatile uint_t eax,	/* user register */
	volatile uint_t es,	/* user "extra" segment register */
	volatile uint_t ds,	/* user data segment register */
	volatile uint_t eip,	/* user instruction pointer register */
	volatile uint_t cs,	/* user code segment register */
	volatile uint_t flags,	/* user flags register */
	volatile uint_t esp,	/* user stack pointer register */
	volatile uint_t ss)	/* user stack segment register */
{
	lwp_t	*lwpp = u.u_lwpp;
	clock_t	start = lwpp->l_stime;
	int	delta;
	sigqueue_t *sig;
	uint_t	fault;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (lwpp->l_trapevf & (EVF_L_SIGTRAP|EVF_L_SIGFPE)) {
		sig = siginfo_get(KM_SLEEP, 0);
		sig->sq_info.si_addr = (caddr_t)eip;
		if (lwpp->l_trapevf & EVF_L_SIGTRAP) {
			/*
			 * User single-step which was fielded in kernel
			 * mode at a call gate, and deferred.
			 * Send SIGTRAP:TRAP_TRACE.
			 */
			sig->sq_info.si_signo = SIGTRAP;
			sig->sq_info.si_code = TRAP_TRACE;
			fault = FLTTRACE;

			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf &= ~EVF_L_SIGTRAP;
			UNLOCK(&lwpp->l_mutex, PLBASE);
		} else {
			/*
			 * A floating-point error was detected during context
			 * switch, and deferred.
			 * Send SIGFPE:FPE_FLTINV.
			 */
			sig->sq_info.si_signo = SIGFPE;
			sig->sq_info.si_code = FPE_FLTINV;
			fault = FLTFPE;

			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_trapevf &= ~EVF_L_SIGFPE;
			UNLOCK(&lwpp->l_mutex, PLBASE);
		}
		/*
		 * If a debugger has directed the process to stop upon
		 * incurring this fault, do so.  The pending signal
		 * may be cancelled during the stop.
		 */
		if (prismember(&u.u_procp->p_fltmask, fault)) {
			ASSERT((ushort_t)u.u_ar0[T_CS] != FPESEL);
			if (stop_on_fault(fault, sig) == 0) {
				kmem_free(sig, sizeof *sig);
				sig = NULL;
			}
		}
		if (sig && sigtolwp(lwpp, sig->sq_info.si_signo, sig) == 0)
			kmem_free(sig, sizeof *sig);
	}

	if (lwpp->l_trapevf & (EVF_L_ASAGE | EVF_PL_SWAPWAIT)) {
		if (lwpp->l_trapevf & EVF_PL_SWAPWAIT)
			lwp_swapout_wait();
		else
			as_age();
	}

	if (lwpp->l_trapevf & UPDATE_FLAGS) {
		lwp_attrupdate();
	}

	if (lwpp->l_trapevf & ADTEXIT_FLAGS) {
		adt_attrupdate();
	}

	if (QUEUEDSIG(lwpp)) {
		/*
		 * Before allowing the LWP to be visible to a debugger or
		 * a signal handler, make sure it's not in the middle of
		 * the floating-point emulator.
		 */
		if ((ushort_t)u.u_ar0[T_CS] == FPESEL)
			fpeclean();
		if (lwpp->l_cursig != 0) {
			psig();
		} else {
			switch (issig((lock_t *)NULL)) {
			case ISSIG_NONE:
				UNLOCK(&lwpp->l_mutex, PLBASE);
				break;
			case ISSIG_SIGNALLED:
				psig();
				break;
			default:                /* ISSIG_STOPPED */
				break;
			}
		}
	}

	/*
	 * If profiling has been enabled charge the user PC with the
	 * time it took to process the events. Also if the current context was
	 * interrupted by the local clock interrupt (while executing
	 * in user mode), charge an extra tick.
	 */

	if (lwpp->l_trapevf & EVF_PL_PROF_EVT) {
		proc_t	*procp = lwpp->l_procp;
		/*
		 * If we had processed a trap or a system call, the 
		 * starting time for the operation would be 
		 * recorded in the LWP structure.
		 */
		if (lwpp->l_start) {
			/*
			 * Trap or syscall processing.
			 */
			delta = (int)(lwpp->l_stime - lwpp->l_start);
			lwpp->l_start = 0;
		} else {
			/*
			 * Interrupt processing or a newly created context.
			 */
			delta = (int)(lwpp->l_stime - start);
		}
		(void)LOCK(&procp->p_mutex, PLHI);
		(void)LOCK(&lwpp->l_mutex, PLHI);
		if (lwpp->l_flag & L_CLOCK) {
			lwpp->l_flag &= ~L_CLOCK;
			delta++;
		}
		lwpp->l_trapevf &= ~EVF_PL_PROF_EVT;
		UNLOCK(&lwpp->l_mutex, PLHI);
		UNLOCK(&procp->p_mutex, PLBASE);
		if (delta > 0)
			addupc((void(*)())eip, delta);
	}

	/*
	 * Single step.
	 */
	if (lwpp->l_trapevf & EVF_PL_STEP) {
		((flags_t *)&flags)->fl_tf = 1;
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_STEP;
		UNLOCK(&lwpp->l_mutex, PLBASE);
	}

#ifdef MERGE386
	/*
	 * Check for posted MERGE event and call the registered routine.
	 */
	if (lwpp->l_trapevf & EVF_L_MERGE) {
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_L_MERGE;
		UNLOCK(&lwpp->l_mutex, PLBASE);
		mhi_ret_user((struct regs *) &eax);
		
	}
#endif /* MERGE386 */
#ifdef PERFMTR_TRAP_CALLBACKS
	_trapexit_hook(lwpp);
#endif /* PERFMTR_TRAP_CALLBACKS */

	CL_TRAPRET(lwpp, lwpp->l_cllwpp);
}


/* 
 * int stop_on_fault(uint_t fault, sigqueue_t *sqp)
 *	Handle stop-on-fault processing for the debugger.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the calling process is acquired and released
 *	by this function.  The l_mutex lock of the calling LWP is also
 *	acquired and released (via the call to stop()).
 *
 * Returns:
 *	Returns 0 if the fault is cleared during the stop, nonzero if it 
 *	isn't.
 */
int
stop_on_fault(uint_t fault, sigqueue_t *sqp)
{
	proc_t *p = u.u_procp;
	lwp_t *lwp = u.u_lwpp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(prismember(&p->p_fltmask, fault));
	ASSERT(sqp);

	(void) LOCK(&p->p_mutex, PLHI);

	/*
	 * Record current fault and siginfo structure
	 * so debugger can find it.
	 */
	lwp->l_curflt = (uchar_t)fault;
	u.u_siginfo = sqp->sq_info;

	if (stop(PR_FAULTED, fault) == STOP_SUCCESS)
		swtch(lwp);

	(void) LOCK(&p->p_mutex, PLHI);
	fault = lwp->l_curflt;
	lwp->l_curflt = 0;
	UNLOCK(&p->p_mutex, PLBASE);

	return fault;
}


/* 
 * void incur_fault(uint_t fault, int signo, int code, caddr_t addr)
 *	Cause the calling context to incur the specified fault.  The
 *	signo, code, and addr are available to a debugger.  If a
 *	debugger is tracing the fault, the process stops and the
 *	debugger may modify the state of the process, possibly clearing
 *	the fault.  Otherwise, the signal signo will be delivered.
 *
 * Calling/Exit State:
 *	The p_mutex and l_mutex of the calling context are acquired and
 *	released by this function via the call to stop_on_fault.
 */
void
incur_fault(uint_t fault, int signo, int code, caddr_t addr)
{
	sigqueue_t *sig;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	sig = siginfo_get(KM_SLEEP, 0);
	sig->sq_info.si_signo = signo;
	sig->sq_info.si_code = code;
	sig->sq_info.si_addr = addr;

	if (prismember(&u.u_procp->p_fltmask, fault))
		if (stop_on_fault(fault, sig) == 0) {
			kmem_free(sig, sizeof *sig);
			sig = NULL;
		}

	if (sig && sigtolwp(u.u_lwpp, signo, sig) == 0)
		kmem_free(sig, sizeof *sig);
}


static int
fperrtocode(cw, sw)
unsigned int cw, sw;
{
	int i;

	sw &= 0x3f;		/* mask off all but exception bits */
	cw &= 0x3f;		/* mask off all but exception bits */

	cw = ~cw;		/* flip bits in cw: if bit is set, FPU */

	i = sw & cw;

	/*
	**	Convert FPU exception into signal FPU exception
	**	to send to user.
	*/
	switch(i) {
	case 0:
		i = 0; break;
	case 1:
		i = FPE_FLTINV; break;
	case 2:
		i = FPE_FLTSUB; break;
	case 4:
		i = FPE_FLTDIV; break;
	case 8:
		i = FPE_FLTOVF; break;
	case 16:
		i = FPE_FLTUND; break;
	case 32:
		i = FPE_FLTRES; break;
	default:
		i = FPE_FLTINV; break;
	}
	return (i);
}

/*
 * Intel Workaround for the "Invalid Operand
 * with Locked Compare Exchange 8Byte (CMPXCHG8B) Instruction"
 * Erratum.
 */
asm 	void store_idt(struct desctab *addr)
{
%reg	addr
	sidt	(addr)
%mem	addr
	movl	addr,%eax
	sidt	(%eax)
}
#pragma asm partial_optimization store_idt

STATIC int
p5_bug(volatile int *type)
{
	struct desctab idt_desc;
	struct gate_desc *idtp, *p5_std_INVOPFLT;
	
	vaddr_t faultaddr = (vaddr_t)_cr2();

	/*
	 * Get the base of the current IDT
	 */
	store_idt(&idt_desc);
	idtp = (struct gate_desc *) DT_GET_BASE(&idt_desc);
	p5_std_INVOPFLT = idtp + INVOPFLT;
	
	/*
	 * p5_std_INVOPFLT should be on 4-byte alignment.
	 */
	if (faultaddr == (vaddr_t) p5_std_INVOPFLT) {
		/*
		 * l.trap_err_code is already set.
		 */
		if ((l.trap_err_code & (PF_ERR_PROT|PF_ERR_WRITE|PF_ERR_USER))
		    == (PF_ERR_PROT|PF_ERR_WRITE)) {
			*type = INVOPFLT+USERFLT;
			/*
			 * As l.trap_err_code is not defined for INVOPFLT,
			 * it's not changed.
			 */
			return 1;
		}
	}

	return 0;
}
	

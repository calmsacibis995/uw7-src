#ident	"@(#)kern-i386:svc/systrap.c	1.41.3.1"
#ident	"$Header$"

#include <acc/audit/audit.h>
#include <fs/procfs/procfs.h>
#include <mem/faultcatch.h>
#include <mem/uas.h>
#include <mem/vmparam.h>
#include <proc/class.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/fault.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/metrics.h>
#include <util/types.h>

extern int stop_on_fault(uint_t, sigqueue_t *);

#ifdef DEBUG
/*
 * For now we turn this on whenever DEBUG is defined.
 * Take this out later.
 */
#define _SYSTRAPTRACE
#endif

#ifdef _SYSTRAPTRACE
#include <util/cmn_err.h>
#include <svc/sysentnm.c>		/* maps scall number to name (text) */
int	systraptrace = 0;
#endif /*_SYSTRAPTRACE*/

void systrap_cleanup(rval_t *, unsigned int, int);

/*
 * SYSTRAP_CLEANUP(rval_t *rvp, unsigned int scall, int error, lwp_t *lwpp);
 *
 *	Clean up before returning from a user-mode system call.
 *
 * Calling/Exit State:
 *
 *
 * Description:
 *
 *	Called from systrap() immediately after the sysent[] function
 *	returns, or from the systrap_cleanup function.
 */

#define SYSTRAP_CLEANUP(rvp, scall, error, lwpp) \
	((((error) == 0) && ((scall) != SYS_vfork) && \
		!((lwpp)->l_trapevf & (EVF_PL_SYSEXIT | EVF_PL_AUDIT))) ? \
	(u.u_ar0[T_EAX] = (rvp)->r_val1, \
		u.u_ar0[T_EDX] = (rvp)->r_val2, \
		CL_TRAPRET((lwpp), (lwpp)->l_cllwpp)) : \
	systrap_cleanup(rvp, scall, error))

/*
 * void systrap(...);
 *
 *	Execute a user-mode system call.
 *
 * Calling/Exit State:
 *
 *	Called directly from the locore.s system-call entry point (syscall).
 *	The previous mode must be user-mode.
 *
 * Description:
 *
 *	We're called as a result of a system call lcall.  Our arguments are
 *	the saved, user-mode registers.  Unlike other C routines, these
 *	registers are "call by reference" in that if we (or any of the routines
 *	we call) modify these registers, the modified values will be restored
 *	back to user-mode.
 *
 *	The system call we're interested in is in "eax", arguments to the
 *	system call are stored at "sp + 4".  We validate the system call
 *	number stored in eax and retrieve the associated sysent entry.  This
 *	sysent table entry contains the routine to be called and the number
 *	of arguments to the system call.  We copy the arguments from the
 *	user-mode stack to the u.u_arg area and pass a pointer to the beginning
 *	of this area to the system call along with a pointer to the rval_t
 *	structure to receive the return value of the system-call.
 *
 *	If a system-call returns non-zero, it indicates the system call has
 *	failed, the error code returned is copied to the saved eax register
 *	and the carry-bit in the flags word is set to indicate a failure.
 *	This allows the user-mode program to quickly detect a failed system
 *	call by using the appropriate "jump if carry set" instruction
 *	immediately after the system call "lcall".
 *
 *	If a system-call returns zero, it indicates the system call was
 *	successful and return values are copied to the return registers.
 *	Two values may be returned in the %eax and %edx registers.
 *
 *	Additional processing is performed to check for other special events
 *	such as user-mode preemption, profiling changes, /proc operations, etc.
 *
 *	N.B.:  this is the BCS compatible system entry point.  We should have
 *	       an optimized "int" based entry point which speeds copy-in, etc.
 */
/* ARGSUSED */
void
systrap(volatile uint_t edi,	/* user register */
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
	volatile uint_t sp,	/* user stack pointer register */
	volatile uint_t ss)	/* user stack segment register */
{
	struct sysent *callp;		/* pointed to the system call entry */
	lwp_t *lwpp;			/* current lwp */
	uint_t	scall;			/* system call number */
	int	error;			/* error return */
	rval_t	rval;			/* return values */

	ASSERT(USERMODE(cs, flags));

	MET_SYSCALL();
	lwpp = u.u_lwpp;


	lwpp->l_start = lwpp->l_stime; /* start time for profiling */
	scall = eax & 0xff;
	u.u_syscall = eax;
	u.u_ar0 = (int *)&eax;

	/*
	 * Clear the carry flag to indicate a successful system call.
	 */
	flags &= ~PS_C;

	if (scall >= sysentsize) {
		/*
		 * Set to illegal value if off end of table.
		 */
		scall = 0;
		u.u_syscall = 0;
	}

	callp = &sysent[scall];

	if (lwpp->l_trapevf & TRAPENTRY_FLAGS) {
		if (lwpp->l_trapevf & EVF_PL_SYSENTRY) {
			(void)LOCK(&u.u_procp->p_mutex, PLHI);
			if ((lwpp->l_trapevf & EVF_PL_SYSENTRY) &&
			    prismember(u.u_procp->p_entrymask, scall)) {
				if (stop(PR_SYSENTRY, scall) == STOP_SUCCESS) {
					ASSERT(LOCK_OWNED(&lwpp->l_mutex));
					swtch(lwpp);
				}
			} else {
				UNLOCK(&u.u_procp->p_mutex, PLBASE);
			}
		}
		if (lwpp->l_trapevf & UPDATE_FLAGS) {
			lwp_attrupdate();   /* Handle process attr updates */
		}
		if (lwpp->l_trapevf & ADTENTRY_FLAGS)
			adt_attrupdate();   /* Handle audit attr updates */
	}

	/*
	 * Copy arguments into the u-block.
	 * Sp points to the return addr on the user's stack, thus, the
	 * arguments to the system call are at sp + 1.
	 */

	if (callp->sy_narg) {
		uint_t	*argp = (uint_t *)u.u_arg;
		uint_t	*usp = (uint_t *)sp + 1;
		size_t	arglen;

		arglen = callp->sy_narg * sizeof(*usp);

		/* inline expansion of copyin to speed up system calls */
		if (!VALID_USR_RANGE(usp, arglen))
			error = EFAULT;
		else {
			CATCH_FAULTS(CATCH_UFAULT) {
				switch (callp->sy_narg) {
					/* 
					 * OK to use direct access instead
					 * of uas_xxx(), since this code
					 * is i386-specific.
					 */
					case 7: argp[6] = usp[6]; /* FALLTHRU */
					case 6: argp[5] = usp[5]; /* FALLTHRU */
					case 5: argp[4] = usp[4]; /* FALLTHRU */
					case 4: argp[3] = usp[3]; /* FALLTHRU */
					case 3: argp[2] = usp[2]; /* FALLTHRU */
					case 2: argp[1] = usp[1]; /* FALLTHRU */
					case 1:	argp[0] = *usp;
						break;
					default:
						bcopy(usp, argp, arglen);
				}
			}
			error = END_CATCH();
		}
		if (error) {
#ifdef _SYSTRAPTRACE
			if (systraptrace) {
				/*
				 *+ Print copyin failure message:
				 *+  systrap(pid,lwpid) arg copyin failure
				 */
				cmn_err(CE_CONT,
					"systrap:(%d,%d) arg copyin failure\n",
					u.u_procp->p_pidp->pid_id,
					lwpp->l_lwpid);
			}
#endif /*_SYSTRAPTRACE*/
			goto skip;
		}
	}

	if (lwpp->l_trapevf & (EVF_PL_AUDIT|EVF_SYSCALL_CALLBACK)) {
		/* hardware metering hook */
		if (lwpp->l_trapevf & EVF_SYSCALL_CALLBACK)
			_sysentry_hook(lwpp, scall);  

		/* auditability check */
		if (lwpp->l_trapevf & EVF_PL_AUDIT)
			adt_auditchk(scall, u.u_arg);  
	}

	rval.r_val1 = 0;			/* default return values */
	rval.r_val2 = edx;

	if (lwpp->l_trapevf & EVF_PL_SYSABORT) {
		/*
		 * System call has been aborted while we were
		 * stopped by the debugger.
		 */
		error = EINTR;
		goto skip;
	}

#ifdef _SYSTRAPTRACE
	if (systraptrace) {
		int	i;
		char	*cp;

		/*
		 *+ Print system call parameters:
		 *+  systrap(pid,lwpid) [scall #n] sysentname(arg1, arg2, ...)
		 */
		cmn_err(CE_CONT, "systrap:(%d,%d) [scall #%d] %s",
		    u.u_procp->p_pidp->pid_id, lwpp->l_lwpid,
		    scall, sysentnames[scall]);

		cp = "(";
		for (i = 0; i < callp->sy_narg; i++) {
			/*
			 *+ Print each system call argument.
			 */
			cmn_err(CE_CONT, "%s0x%x", cp, u.u_ap[i]);
			cp = ", ";
		}

		cp = (i != 0) ? ")\n" : "\n";
		/*
		 *+ Print closing parentheses for arguments if needed.
		 */
		cmn_err(CE_CONT, "%s", cp);
	}
#endif /*_SYSTRAPTRACE*/

	error = (*callp->sy_call)(u.u_arg, &rval);
skip:
#ifdef CC_PARTIAL
	/* Treat covert channels if necessary */
	if (u.u_covert.c_bitmap)
		cc_limit_all(&u.u_covert, CRED());
#endif /* CC_PARTIAL */
	SYSTRAP_CLEANUP(&rval, scall, error, lwpp);
}


/*
 * void systrap_cleanup(rval_t *rvp, unsigned int scall, int error);
 *
 *	Clean up before returning from a user-mode system call.
 *
 * Calling/Exit State:
 *
 *
 * Description:
 *
 *	Called from systrap() immediately after the sysent[] function
 *	returns, or from a newly created context which is ready to
 *	"return" to user mode.
 */
void
systrap_cleanup(rval_t *rvp, unsigned int scall, int error)
{
	lwp_t *lwpp = u.u_lwpp;

	if (error) {
#ifdef _SYSTRAPTRACE
		if (systraptrace) {
			/*
			 *+ Print system call error code:
			 *+  -->systrap(pid,lwpid)sysentname: ERROR=n
			 */
			cmn_err(CE_CONT,"-->systrap(%d,%d)%s: ERROR=%d\n",
				u.u_procp->p_pidp->pid_id, lwpp->l_lwpid,
				sysentnames[scall], error);
		}
#endif /*_SYSTRAPTRACE*/

		/* XXX Does SIGXFSZ need to send siginfo? */
		if (error == EFBIG)
			sigtolwp(lwpp, SIGXFSZ, (sigqueue_t *)0);

		if (error == EINTR) {
			int cursig = lwpp->l_cursig;

			/*
			 * If the system call was aborted by /proc, leave
			 * errno alone (but clear the flag!); otherwise,
			 * if there is no cursig (i.e. it was a forkall()
			 * that interrupted us), or cursig is marked as
			 * restartable, change errno to ERESTART so the
			 * library stub can retry the syscall.
			 */
			if (lwpp->l_trapevf & EVF_PL_SYSABORT) {
				pl_t pl = LOCK(&lwpp->l_mutex, PLHI);
				lwpp->l_trapevf &= ~EVF_PL_SYSABORT;
				UNLOCK(&lwpp->l_mutex, pl);
			} else if (cursig == 0 ||
				  u.u_procp->p_sigstate[cursig - 1].sst_cflags &
				    SA_RESTART) {
				error = ERESTART;
			}
		}

		/* Enhanced Application Compatibility */

		if (isSCO)
			error = coff_errno(error);

		/* End Enhanced Application Compatibility */

		u.u_ar0[T_EAX] = error;		/* set return value */
		u.u_ar0[T_EFL] |= PS_C;		/* carry flag set indicates
						 * an error */
	} else {
#ifdef _SYSTRAPTRACE
		if (systraptrace) {
			/*
			 *+ Print successful system call return value:
			 *+  -->systrap(pid,lwpid)sysentname: OK rval=n
			 */
			cmn_err(CE_CONT,"-->systrap(%d,%d)%s: OK rval=%d\n",
				u.u_procp->p_pidp->pid_id, lwpp->l_lwpid,
				sysentnames[scall], rvp->r_val1);
		}
#endif /*_SYSTRAPTRACE*/
		u.u_ar0[T_EAX] = rvp->r_val1;
		u.u_ar0[T_EDX] = rvp->r_val2;
	}

	ASSERT((lwpp->l_trapevf & EVF_PL_SYSABORT) == 0);

	/*
	 * Stop-on-syscall-exit test.
	 */
	if (lwpp->l_trapevf & EVF_PL_SYSEXIT) {
		(void)LOCK(&u.u_procp->p_mutex, PLHI);
		if ((lwpp->l_trapevf & EVF_PL_SYSEXIT) &&
		    prismember(u.u_procp->p_exitmask, scall)) {
			if (stop(PR_SYSEXIT, scall) == STOP_SUCCESS) {
				ASSERT(LOCK_OWNED(&lwpp->l_mutex));
				swtch(lwpp);
			}
		} else {
			UNLOCK(&u.u_procp->p_mutex, PLBASE);
		}
	}

	/*
	 * If we are the parent returning from a successful
	 * vfork, then wait for the newly created child to
	 * relinquish our address space (via relvm() called
	 * from exec or exit).
	 */
	if (scall == SYS_vfork && rvp->r_val2 == 0 && error == 0)
		vfwait((pid_t)rvp->r_val1);

	if (lwpp->l_trapevf & (EVF_PL_AUDIT|EVF_SYSCALL_CALLBACK)) {
		/*
		 * If auditing is enabled for this lwp
		 * and the AUDITME flag is set,
		 * then call the appropriate recording function.
		 */
		if ((lwpp->l_trapevf & EVF_PL_AUDIT) && (lwpp->l_auditp))
			adt_record(scall, error, u.u_arg, rvp);

		/* hardware metering hook */
		if (lwpp->l_trapevf & EVF_SYSCALL_CALLBACK)
			_sysexit_hook(lwpp, scall);  
	}

	CL_TRAPRET(lwpp, lwpp->l_cllwpp);
}

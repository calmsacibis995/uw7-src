#ident	"@(#)kern-i386:proc/execmdep.c	1.37.4.2"
#ident	"$Header$"

#include <fs/procfs/prdata.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/seg.h>
#include <mem/seg_dz.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/tss.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

STATIC uint_t exec_initialstk = ptob(SSIZE);
extern void parmsp1init(void);

/*
 * void execstk_size(struct uarg *args, vaddr_t stkbase)
 *	Compute the argument/stack image size for the new image.
 *
 * Calling/Exit State:
 *	args->estksize is computed based on values of other fields in
 *	the uarg structure.  This is the number of bytes, rounded to
 *	the nearest PAGESIZE multiple, needed for the new stack image.
 *
 *	This function does not block.
 *	No locks are required on entry and none are held on exit.
 *
 * Remarks:
 *	The estksize, and possibly other computed values, will be used
 *	subsequently by extractarg() to actually set up the image;
 *	but the caller needs it first, so the rlimits can be checked
 *	before committing to the exec(). The stack segment size for the
 *	new address space is also computed here for the same reason.
 */
void
execstk_size(struct uarg *args, vaddr_t stkbase)
{
	size_t stgsize;	/* # bytes for strings */
	uint_t xargc;	/* # extra arguments */
	size_t rlim;
	vaddr_t lowlim;

	stgsize = (uint_t)args->argsize + (uint_t)args->envsize +
			(uint_t)args->prefixsize + (uint_t)args->auxsize;

	if ((xargc = args->prefixc) != 0) {
		if (args->intpvp == NULL) {
			ASSERT(args->fname != NULL);
			stgsize += strlen(args->fname) + 1;
		} else {
			/*
			 * Due to the security implementation for setid intp
			 * scripts, args->fname is going to be replaced with
			 * "/dev/fd/n".  However, this has to be done after
			 * we're single-threaded, so we don't know yet how
			 * many characters there are in "n".  We just pick
			 * something which will be big enough; at worst we
			 * end up allocating a few extra bytes in the stack.
			 */
			stgsize += DEVFD_SIZE;
		}
	} else if (args->flags & EMULA) {
		ASSERT(args->fname != NULL);
		stgsize += strlen(args->fname) + 1;
		xargc = 1;
	}

	args->stringsize = stgsize;

	/*
	 * Magic 3 below is broken down as follows:
	 *	1 word for argc storage +
	 *	1 word for argv list NULL terminator +
	 *	1 word for env  list NULL terminator.
	 */
	args->vectorsize = (3 + args->argc + args->envc + xargc) * NBPW;

	args->estksize = ptob(btopr(args->vectorsize + stgsize +
				     exec_initialstk));
	/*
	 * Compute the new stack segment size and base for the new AS
	 * here.
	 */
	lowlim = ptob(1);	/* don't allow stack to cause null pointers */
	/*
	 * If the selected stack address is UVSTACK, then this means
	 * that the process has mapped its text or libraries below
	 * the default stack address and thus the system has picked
	 * UVSTACK. In this case, we restrict the maximum stack usage
	 * to be STACK_SIZE.
	 */
	rlim = u.u_rlimits->rl_limits[RLIMIT_STACK].rlim_cur;
	if (stkbase != UVSTACK) {
		if (lowlim + rlim >= stkbase)
			args->stkseglow = lowlim;
		else
			args->stkseglow = stkbase - rlim;
	} else
		args->stkseglow = UVSTACK - min(UVMAX_STKSIZE, rlim);

	args->stksegsize = stkbase - args->stkseglow;
}


/*
 * int extractarg(struct uarg *args)
 *	Gather user arguments from an exec system call to a new
 *	(interim) spot in the callers address space.  The arguments
 *	are put into a format suitable for a newly exec'd process image.
 *
 * Calling/Exit State:
 *	The process must be single threaded on entry to this function.
 *	No spinlocks can be held on entry, none are held on return.
 *
 * Remarks:
 *	u.u_stkbase must be set before calling this routine.
 *	Many out arguments are set by this function in the passed
 *	in uarg structure.
 */
int
extractarg(struct uarg *args)
{
	/*
	 * Machine dependent: Depends on the arrangement of the user stack.
	 *			On 80x86 the userstack grows downwards toward
	 *			low addresses; the arrangement is:
	 *
	 * (low address).
	 * argc
	 * prefix ptrs (no NULL ptr)
	 * argv ptrs (with NULL ptr)
	 *	(old argv0 points to fname copy if prefix exits)
	 *	(last argv ptr points to fname copy if EMULATOR present)
	 * env ptrs (with NULL ptr)
	 * postfix values (put here later if they exist (auxaddr))
	 * prefix strings
	 * (fname if a prefix exists)
	 * argv strings
	 * (fname if EMULATOR exists)
	 * env strings
	 * (high address)
	 *
	 */

	int		error;
	uint_t		fnsize;
	vaddr_t		nsp;
	uint_t		xargc;
	uint_t		actualstksz;
	vaddr_t		ptrstart;
	vaddr_t		cp;
	uint_t		ptrdelta;	/* for argument pointer relocation */
	proc_t		*p = u.u_procp;
	size_t		i;
	caddr_t		sp;
	struct segdz_crargs a;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(SINGLE_THREADED());

	as_wrlock(p->p_as);

	/*
	 * Find a suitable hole in our address space for the
	 * new stack image.
	 */
	if ((nsp = execstk_addr(args->estksize, &args->estkhflag)) == NULL) {
#ifdef DEBUG
		cmn_err(CE_NOTE, "extractarg: execstk_addr failure\n");
#endif
		as_unlock(p->p_as);
		return ENOMEM;
	}
	args->estkstart = nsp;	/* interim addr for new stack image */

	a.flags = SEGDZ_NOCONCAT;
	a.prot = PROT_ALL;

	error = as_map(p->p_as, nsp, args->estksize, segdz_create, &a);
	if (error != 0) {
#ifdef DEBUG
		cmn_err(CE_NOTE, "extractarg: as_map failure; error = %d\n",
			error);
#endif
		as_unlock(p->p_as);
		return error;
	}
	as_unlock(p->p_as);

	if (args->intpvp) {
		/*
		 * Adjust stringsize to reflect the actual "/dev/fd/n"
		 * name length, now that we know it.
		 */
		ASSERT(args->fname != NULL);
		args->stringsize += strlen(args->fname) + 1 - DEVFD_SIZE;
	}

	args->stacklow = u.u_stkbase;
	actualstksz = args->vectorsize + args->stringsize +
		       ((NBPW*2) - 1) & ~((NBPW*2) - 1); /* 8 byte alignment */

	nsp += args->estksize;
	ptrdelta = args->stacklow - nsp;
	nsp -= actualstksz;

	ptrstart = nsp;
	cp = nsp + args->vectorsize;
	args->stackend = args->stacklow - actualstksz;

	if ((xargc = args->prefixc) != 0) {
		fnsize = strlen(args->fname) + 1;
	} else if (args->flags & EMULA) {
		fnsize = strlen(args->fname) + 1;
		xargc = 1;
	}

	/* Store argc. */
	if (suword((int *)ptrstart, args->argc + xargc))
		goto err;

	/*
	 * Decrement the effect of the above suword(): See ul96-13502
	 */
	if (p->p_flag & P_VFORK) {
		p->p_as->a_isize -= PAGESIZE;
	}

	ptrstart += sizeof (int);		/* account for argc */

	/* Set auxaddr for use by elfexec (execpoststack). */
	args->auxaddr = cp + ptrdelta;
	cp += args->auxsize;

	/*
	 * For an interpreter (intp), the pathname specified in the
	 * interpreter file is passed as arg0 to the interpreter.
	 * If the optional argument was specified in the interpreter
	 * file, it is passed as arg1 to the interpreter.  The remaining
	 * arguments to the interpreter are the arg0 through argN of the
	 * originally exec'd file.
	 */
	if (args->prefixc) {
		if (copyarglist(args->prefixc, args->prefixp, ptrdelta,
				ptrstart, cp, B_TRUE) != args->prefixsize) {
			goto err;
		}
		ptrstart += xargc * sizeof (int);
		cp += args->prefixsize;

		/* Copyout filename of intp script. */
		if (copyout(args->fname, (void *)cp, fnsize) != 0)
			goto err;
		cp += fnsize;
	}

	/* Copy arguments to new location. */
	if (copyarglist(args->argc, args->argp, ptrdelta, ptrstart, cp,
			B_FALSE) != args->argsize) {
		goto err;
	}

	/*
	 * For intp, modify argv0 of originally exec'd file (which is
	 * now argv1 or argv2 to the interpreter) to be the filename
	 * of the script.
	 */
	if (args->prefixc) {
		if (suword((int *)ptrstart, (int)(cp + ptrdelta - fnsize)))
			goto err;
	}

	/* Copy arguments for /proc psinfo to execinfo structure. */
	sp = args->execinfop->ei_psargs;
	i = min(args->argsize, PSARGSZ - 1);
	if (copyin((void *)cp, sp, i) != 0)
		goto err;
	/*
	 * The psinfo structure needs the arguments to be separated by
	 * blanks instead of null bytes.  Perhaps it would be better to
	 * modify the proc(5) spec so that this would not be necessary.
	 */
	while (i--) {
		if (*sp == '\0')
			*sp = ' ';
		sp++;
	}
	*sp = '\0';

	ptrstart += (args->argc + 1) * sizeof (int);
	cp += args->argsize;

	/* Filename for emulator follows argv strings. */
	if (args->flags & EMULA) {
		if (copyarglist(1, (vaddr_t)&args->fname, ptrdelta,
				ptrstart - sizeof (int), cp,
				B_TRUE) != fnsize) {
			goto err;
		}
		ptrstart += sizeof (char *);
		cp += fnsize;
	}

	/* Copy environment variables (if any) to new location. */
	if ((args->envc != 0) &&
	    copyarglist(args->envc, args->envp, ptrdelta, ptrstart,
			cp, B_FALSE) != args->envsize) {
		goto err;
	}

	/*
	 * Verify that ptrstart has been updated appropriately.
	 * (Check that pointers have been put in the appropriate place.)
	 */
	ASSERT(ptrstart + (args->envc + 1) * sizeof (char *) ==
	       args->auxaddr - ptrdelta);

	/*
	 * Verify that cp has been updated appropriately.
	 * (Check that strings have been put in the appropriate place.)
	 */
	ASSERT(((cp + args->envsize + (NBPW*2 - 1)) &
		~(NBPW*2 - 1)) == args->estkstart + args->estksize);

	return 0;

err:
#ifdef DEBUG
	cmn_err(CE_NOTE, "extractarg: arguments\n");
#endif
	as_wrlock(u.u_procp->p_as);
	as_unmap(u.u_procp->p_as, args->estkstart, args->estksize);
	as_unlock(u.u_procp->p_as);
	return EFAULT;
}


/*
 * int setregs(struct uarg *args)
 *	Machine-dependent final setup code for exec.
 *
 * Calling/Exit State:
 *	The process reader/writer lock (p_rdwrlock) is held on entry.
 *	No spinlocks should be held on entry, none are held on return.
 */
int
setregs(struct uarg *args)
{
	execinfo_t *eip;
	extern void fpu_disable(void);
	extern void *lwp_setprivate(void *);

	ASSERT(KS_HOLD0LOCKS());

	eip = args->execinfop;

	/* Set stack pointer and program counter (EIP) */
	u.u_ar0[T_UESP] = args->stackend;
	u.u_ar0[T_EIP] = eip->ei_exdata.ex_entloc;

	/* Initialize all segment registers. */
	u.u_ar0[T_CS] = USER_CS;
	u.u_ar0[T_DS] = u.u_ar0[T_ES] = u.u_ar0[T_SS] = USER_DS;
	_wfs(0);
	_wgs(0);

	/*
	 * Initialize EFL: clear direction flag, trace flag, and all
	 * system flags except interrupt enable, which is set; other
	 * flags stay the same.
	 */
	u.u_ar0[T_EFL] =
		((u.u_ar0[T_EFL] & PS_USERMASK) & ~(PS_D|PS_T)) | PS_IE;

	/*
	 * If user was using H/W debug registers, relinquish them now.
	 */
	if (u.u_kcontext.kctx_debugon) {
		prdebugoff(u.u_lwpp);
		bzero(&u.u_kcontext.kctx_dbregs,
		      sizeof u.u_kcontext.kctx_dbregs);
	}

	/*
	 * Set the LWPs private data pointer to NULL to indicate that the
	 * per-lwp and per-thread descriptors should be re-initialized.
	 */
	u.u_privatedatap = NULL;
	(void)lwp_setprivate((void *)NULL);

	/*
	 * Arrange for the FPU to be initialized correctly.
	 */
	DISABLE_PRMPT();
	u.u_kcontext.kctx_fpvalid = 0;
	uvwin.uv_fp_used = u.u_fp_used = B_FALSE;
	fpu_disable();
	ENABLE_PRMPT();

	return 0;
}


/*
 * int transfer_stack(struct as *as, struct as *nas, struct uarg *args)
 *	This function is used by the remove_proc() function in
 * 	proc/exec.c to hide the direction of stack growth. It maps in the
 *	autogrow stack segment.
 *
 * Calling/Exit State:
 *	At this point, the context is single threaded.
 *	The nas address space is held privately, so no locking is needed.
 */
int
transfer_stack(struct as *as, struct as *nas, struct uarg *args)
{ 
	struct segdz_crargs a;
	int err = 0;

	err = as_exec(as, args->estkstart, args->estksize, nas,
		      args->stacklow - args->estksize, args->estkhflag);
	if (err != 0)
		return err;

	a.flags = SEGDZ_CONCAT;	/* concatenate to current stack */
	a.prot = PROT_ALL;

	if (args->estksize < args->stksegsize)
		err = as_map(nas, args->stkseglow, 
			     args->stksegsize - args->estksize, 
			     segdz_create, &a);
	if (!err)
		args->estksize = args->stksegsize;

	return err;
}



STATIC char *init_name = "/sbin/init";		/* should be configurable */

char *initstate;	/* Override starting state for init */

#define SOPT	"-s"	/* Option for passing initstate */

/* Format of args to exec system call. */
struct execa {
	vaddr_t	fname;		/* filename */
	vaddr_t	argp;		/* argument pointers */
	vaddr_t	envp;		/* environment pointers */
};


/*
 * void exec_initproc(void *init_argp)
 *	Exec the system init process (process id 1).
 *
 * Calling/Exit State:
 *	No locks are held on entry, none are held on return.
 *	u.u_ar0 must be appropriately initialized prior to calling
 *	this function.
 *
 * Remarks:
 *	Does an exec("/sbin/init", "/sbin/init", (char *)NULL) by copying
 *	out the arguments to the exec system call to a temporary user
 *	stack, and then directly calling exece().
 *	This function knows about the direction of user stack growth,
 *	and about the format of arguments to an exec system call.
 *	On succesful completion of this function, the process address
 *	space is set up for the init process and u.u_ar0 is set up
 *	for return to user mode via the normal user mode trap return
 *	sequence.
 */
/* ARGSUSED */
void
exec_initproc(void *init_argp)
{
	proc_t	*p = u.u_procp;
	int	error, arg0_len, sopt_len;
	vaddr_t	ucp;
	vaddr_t	uargv;
	vaddr_t	arg0, sopt, sopt_arg;
	rval_t	rval;
	struct	execa *ap;
	list_t *anchorp;
	struct as *as;
	extern int exece(struct execa *, rval_t *);
	extern size_t maxrss_tune;
	extern clock_t et_age_interval_tune;
	extern clock_t init_agequantum_tune;
	extern clock_t min_agequantum_tune;
	extern clock_t max_agequantum_tune;

	ASSERT(p->p_pidp->pid_id == 1);
	/*
	 * u.u_ar0 must be set, exec (via setregs()) will set
	 * appropriate values for the return to user mode.
	 */
	ASSERT(u.u_ar0 != NULL);
	proc_init = p;
	/* Allocate user address space. */
	p->p_as = as = as_alloc();
	anchorp = kmem_alloc(sizeof(list_t), KM_NOSLEEP);
	if (p->p_as == NULL || anchorp == NULL) {
		/*
		 *+ The kernel was unable to allocate an address
		 *+ space structure for the first user-mode process.
		 *+ This is happening at a time where there should
		 *+ be a great deal of free memory on the system.
		 *+ Corrective action:  Check the kernel configuration
		 *+ for excessive static data space allocation or
		 *+ increase the amount of memory on the system.
		 */
		cmn_err(CE_PANIC, "exec_initproc: as_alloc returned NULL");
	}

	anchorp->flink = anchorp->rlink = &as->a_forklist;
	as->a_forklist.flink = as->a_forklist.rlink = as->a_anchor = anchorp;
	HOP_ASLOAD(as);

	/*
	 * Move init to the appropriate scheduling class.
	 */
	parmsp1init();

	/* Allocate a one page user stack (temporary). */
	(void)as_map(p->p_as, UVSTACK - ptob(1), ptob(1),
		     segvn_create, zfod_argsp);

	/* Copyout arguments to the user stack. */
	ucp = UVSTACK;

	ucp -= (arg0_len = strlen(init_name) + 1);
	(void)copyout(init_name, (void *)ucp, arg0_len);
	arg0 = ucp;					/* arg0 and filename */

	if (initstate) {
		ucp -= sizeof SOPT;
		(void)copyout(SOPT, (void *)ucp, sizeof SOPT);
		sopt = ucp;
		ucp -= (sopt_len = strlen(initstate) + 1);
		(void)copyout(initstate, (void *)ucp, sopt_len);
		sopt_arg = ucp;
	}

	/* Copyout argv pointers to the user stack. */
	uargv = ucp & ~(NBPW - 1);			/* word align */
	uargv -= sizeof (char *);
	(void)suword((int *)uargv, NULL);		/* argv terminator */
	if (initstate) {
		uargv -= sizeof (char *);
		(void)suword((int *)uargv, (int)sopt_arg);	/* argv[2] */
		uargv -= sizeof (char *);
		(void)suword((int *)uargv, (int)sopt);		/* argv[1] */
	}
	uargv -= sizeof (char *);
	(void)suword((int *)uargv, (int)arg0);		/* argv[0] */

	/* Set up arguments to exece(). */
	ap = (struct execa *)u.u_arg;
	ap->fname = arg0;				/* user stack addr */
	ap->argp = uargv;				/* user stack addr */
	ap->envp = NULL;				/* no environment */

	/* set the aging parameters for the first process */
	p->p_as->a_maxrss = maxrss_tune;
	p->p_as->a_et_age_interval = et_age_interval_tune;
	p->p_as->a_init_agequantum = init_agequantum_tune;
	p->p_as->a_min_agequantum = min_agequantum_tune;
	p->p_as->a_max_agequantum = max_agequantum_tune;
	p->p_as->a_agequantum = init_agequantum_tune;

	/* Let exec do the hard work. */
	if ((error = exece(ap, &rval)) != 0) {
		/*
		 *+ The exec of the first user mode process failed.
		 *+ This can be due to a kernel bug in the exec logic,
		 *+ or more likely, due to a bad file for the pathname
		 *+ given by 'init_name'.  Check the access permissions,
		 *+ and the object type (e.g. elf, coff) for the file
		 *+ given by 'init_name'.
		 */
		cmn_err(CE_PANIC, "exec_initproc: Can't exec %s, error %d\n",
			init_name, error);
	}
}

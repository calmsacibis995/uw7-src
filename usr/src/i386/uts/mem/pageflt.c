#ident	"@(#)kern-i386:mem/pageflt.c	1.43.4.1"

#include <fs/memfs/memfs.h>
#include <mem/anon.h>
#include <mem/as.h>
#include <mem/faultcatch.h>
#include <mem/faultcode.h>
#include <mem/hat.h> 
#include <mem/immu.h>
#include <mem/mem_hier.h> 
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/exec.h>
#include <proc/proc.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/trap.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h> 
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

extern struct seg *segkmap;  
extern struct seg *segkvn;

#ifndef NO_ADDR0

/*
 * "ADDR0" null-pointer dereference workaround.  If this is enabled
 * (for a particular user), memory reads anywhere in the first page
 * (addresses starting at 0) will cause a read-only page of zeros to
 * be mapped into the process's address space (if nothing is mapped
 * there already).  Otherwise, such references would cause SIGSEGVs.
 */

extern boolean_t addr0_enabled(void);
extern boolean_t nullptr_log;

STATIC struct vnode *addr0_mvp;	/* Used for shared null-pointer page */

/*
 * STATIC void
 * addr0_mvp_alloc(void)
 *	Allocate an anonmap for the shared null-pointer zero page
 *
 * Calling/Exit State:
 *	This routine may block, so the caller must not hold any spinlocks.
 *	Serialization of multiple allocation requests is handled here;
 *	only one anonmap will be created.
 *
 * Remarks:
 *	Rather than create a new lock just to mutex this allocation,
 *	we use the vm_pgfreelk.  It's only held here for a very short time,
 *	and only at most once during the life of the system.
 */
STATIC void
addr0_mvp_alloc(void)
{
	struct vnode *mvp;

	mvp = memfs_create_unnamed(PAGESIZE, MEMFS_FIXEDSIZE);
	if (mvp == NULL) {
		/*
		 * Couldn't reserve swap space.
		 */
		return;
	}
	VM_PAGEFREELOCK();
	if (addr0_mvp != NULL) {
		/*
		 * We lost the race, and someone else allocated it
		 * first.
		 */
		VM_PAGEFREEUNLOCK();
		VN_RELE(mvp);
	} else {
		addr0_mvp = mvp;
		VM_PAGEFREEUNLOCK();
	}
}

#endif	/* !NO_ADDR0 */

/*
 * STATIC sigqueue_t *
 * fault_info(faultcode_t flt, vaddr_t faultaddr)
 *	Allocate a sigqueue_t structure, and fill it in based on the
 *	fault code, flt.
 *
 * Calling/Exit State:
 *	All data is local to the calling context and requires no
 *	special locking. 
 */
STATIC sigqueue_t *
fault_info(faultcode_t flt, vaddr_t faultaddr)
{
	sigqueue_t *sqp = siginfo_get(KM_SLEEP, 0);

	switch (FC_CODE(flt)) {
	case FC_NOMAP:
		sqp->sq_info.si_signo = SIGSEGV;
		sqp->sq_info.si_code = SEGV_MAPERR;
		break;

	case FC_PROT:
		sqp->sq_info.si_signo = SIGSEGV;
		sqp->sq_info.si_code = SEGV_ACCERR;
		break;

	case FC_OBJERR:
		sqp->sq_info.si_signo = SIGBUS;
		sqp->sq_info.si_code = BUS_OBJERR;
		sqp->sq_info.si_errno = FC_ERRNO(flt);
		break;

	default:
		sqp->sq_info.si_signo = SIGKILL;
		break;
	}

	sqp->sq_info.si_addr = (caddr_t)faultaddr;

	return sqp;
}

/*
 * boolean_t
 * upageflt_cmn(vaddr_t faultaddr, int ecode, sigqueue_t **sqpp) 
 *	i386 family-specific pagefault handling routine to service page
 *	faults on user virtual addresses whether generated from user or
 *	kernel mode.
 *
 * Calling/Exit State: 
 *	The i386-family version of upageflt_cmn is called with three 
 *	arguments, the faulting address (not aligned to a page boundary), 
 *	the error code for the fault, and a sigqueue_t pointer pointer used
 *	as an outarg. All other fault specifics can be determined from machine 
 *	registers and global data.	
 *
 *
 *	The user address space structure is unlocked when we are called.
 *	This routine can acquire the AS lock in reader mode and upgrade it to
 *	writer mode as necessary, dropping it before returning. An
 *	optimization is implemented, which skips taking the lock for
 *	SINGLE_THREADED processes.
 *
 *	A boolean is returned to the caller to indicate the success (B_TRUE)
 *	or failure (B_FALSE) of the operation. If B_FALSE is returned, a
 *	sigqueue_t is allocated, assigned to (*sqpp), and its sq_info is
 *	filled in.
 *
 * Remarks:
 *	This is only global so that it can be accessed from the
 *	i386-specific begin_user_write code.
 */
boolean_t
upageflt_cmn(vaddr_t faultaddr, int ecode, sigqueue_t **sqpp) 
{
	struct as *as;
	enum aslock_stat { NOT_AS_LOCKED, AS_READ_LOCKED, AS_WRITE_LOCKED };
	enum aslock_stat aslock_stat = NOT_AS_LOCKED;
	enum fault_type type;
	enum seg_rw rw;
	faultcode_t flt;
	int errcode = ecode;

	ASSERT(u.u_procp != NULL);
	as = u.u_procp->p_as;
	ASSERT(as != NULL);

	errcode = ecode & (PF_ERR_WRITE|PF_ERR_USER|PF_ERR_PROT);
	rw = (errcode & PF_ERR_WRITE) ? S_WRITE : S_READ_ACCESS;

	/*
	 * Get a read lock on the segment. Do this to insure that
	 * no other unmaps or creates are in progress.
	 */

	if (!SINGLE_THREADED()) {
		as_rdlock(as);
		aslock_stat = AS_READ_LOCKED;
	}

	/*
	 * Now case out the various scenarios 
	 */

	type = ((errcode & PF_ERR_MASK) ? F_PROT : F_INVAL);

	switch (errcode & ~(PF_ERR_WRITE|PF_ERR_USER)) {

        case PF_ERR_PAGE:
	case PF_ERR_PROT:

		/*
	 	 * Determine if this fault is contained within a known segment.
		 * If not, attempt to dynamically map in an appropriate
		 * new segment. 
		 *
		 * If we need to map a new segment (case of either
	 	 * the 0th page or call to grow()) we will need to be holding
		 * the AS lock in write mode. Sooo... if we currently hold it in
		 * read mode then we need to drop the AS lock, reacquire it in
		 * write mode and restart (from the as_segat). This is needed to
		 * catch the case where someone else did our work (i.e. got
		 * the lock in write mode first). However, we can only upgrade
		 * the AS lock to write mode if the caller had not acquired the
		 * lock on our behalf; otherwise (i.e., the caller is doing a
		 * userwrite and has already acquired the AS lock), we have to
		 * return failure and let the caller retry after upgrading 
		 * the AS lock to write mode.
	 	 */

		ASSERT(SINGLE_THREADED() || aslock_stat != NOT_AS_LOCKED);
#ifndef NO_ADDR0
               	if (btop(faultaddr) == 0) {
retest:
			if (as_segat(as, faultaddr) == NULL) {

                	/*
                 	 * For backward compatibility, we open a door here to
                 	 * allow virtual page 0 to be accessible (read-only) 
			 * even with the new 4.0 virtual map.  This is for 
			 * those applications that depend on NULL pointer 
			 * dereferences working.
                 	 *
			 * We establish a read-only SHARED mapping to a
			 * globally shared one page long memfs file.
                 	 */
				struct segvn_crargs a;

				flt = FC_NOMAP;
				if (!addr0_enabled())
					break;

				/*
				 * WARNING: the unlocked test of
				 * addr0_mvp assumes STORE-ORDERED
				 * memory hardware architecture.
				 */
				if (addr0_mvp == NULL) {
					/*
					 * This is the first time; create an
					 * anonmap to hold the shared page.
					 */
					addr0_mvp_alloc();
					if (addr0_mvp == NULL) {
						/* Couldn't reserve swap */
						break;
					}
				}

				if (aslock_stat != AS_WRITE_LOCKED) {
					if (aslock_stat == AS_READ_LOCKED)
						as_unlock(as);
					as_wrlock(as);
					aslock_stat = AS_WRITE_LOCKED;
					goto retest;
				}

				if (nullptr_log) {
					/*
					 *+ notice if nullptr dreferencing.
					 */
					cmn_err(CE_NOTE,
					 "Null pointer reference in process %d"
					 " at 0x%x", u.u_procp->p_pidp->pid_id,
					 u.u_ar0[T_EIP]);
					cmn_err(CE_CONT,
					 "\tCommand is: %s (%s)\n",
					 u.u_procp->p_execinfo->ei_psargs,
					 u.u_procp->p_execinfo->ei_comm);
				}

				a = *(struct segvn_crargs *)zfod_argsp;
				a.vp = addr0_mvp;
				a.type = MAP_SHARED;
				a.prot = a.maxprot = PROT_READ | PROT_USER;

				if (as_map(as, (vaddr_t)0, PAGESIZE,
					   segvn_create, &a) != 0)
					break;
			}
                }
#endif	/* !NO_ADDR0 */

		flt = as_fault(as, faultaddr, 1, type, rw);

                break;

        default:
		/*
		 *+ The family-specific page fault handing routine 
		 *+ was called and l.trap_err_code was set indicating an 
		 *+ unrecognized fault type. This is not a recoverable
		 *+ condition.
		 *+
		 *+ On Pentium and Pentium Pro, PF_ERR_RESV 8 would be
		 *+ set if a reserved bit is set ina pte e.g. if PG_PS,
		 *+ This would imply page table corruption, so panic.
		 */
                cmn_err(CE_PANIC, "upageflt_cmn: impossible faultcode") ;
        }

	/*
	 * Done with this fault and the address space, unlock and 
	 * outta here.
	 */

	if (aslock_stat != NOT_AS_LOCKED)
		/* orignally unlocked, so return it that way */
		as_unlock(as);

	if (!flt) 
		return B_TRUE;
	if (sqpp)
		*sqpp = fault_info(flt, faultaddr);
	return B_FALSE;                             
}

#ifdef MERGE386

/*
 * boolean_t
 * mki_uaddr_mapped(vaddr_t addr)
 *	Function to check if the addr is mapped in the current address
 *	space.
 *
 * Calling/Exit State:
 *	Should be called in process context.
 *	Returns B_TRUE if addr is part of the AS layout.
 *		B_FALSE otherwise.
 *	The return value is stale.
 */
boolean_t
mki_uaddr_mapped(vaddr_t addr)
{
	struct as *as = u.u_procp->p_as;
	struct seg *seg;

	ASSERT(VALID_USR_RANGE(addr, 1));

	as_rdlock(as);
	seg = as_segat(as, addr);
	as_unlock(as);
	if (seg != (struct seg *)NULL)
		return B_TRUE;
	return B_FALSE;
}

/*
 * boolean_t
 * mki_upageflt(vaddr_t faultaddr, int errcode)
 *	This function is a wrapper around upageflt_cmn for merge.
 *
 * Calling/Exit State:
 *	Must be called to either resolve a user mode fault or a 
 * 	kernel mode fault for a user address with CATCH_UFAULT
 *	flag set.
 */
boolean_t
mki_upageflt(vaddr_t faultaddr, int errcode)
{
	ASSERT(VALID_USR_RANGE(faultaddr, 1));
	return upageflt_cmn(faultaddr, errcode, NULL);
}

#endif

/*
 * boolean_t 
 * upageflt(sigqueue_t **sqpp, vaddr_t *faultaddrp)
 *	i386 family-specific pagefault handling routine to service user-mode
 *	originating faults
 *
 * Calling/Exit State: 
 *	The i386-family version of upageflt is called with a sigqueue_t
 *	pointer outarg.  This must be passed in since it allows all error
 *	processing (signal delivery etc) to be common to our caller (trap).
 *	All other specifics can be determined from machine registers and
 *	global data.  It is also passed a vaddr_t pointer outarg in which
 *	the fault address can be returned; this is needed by stop-on-fault.
 *
 *	A boolean is returned to the caller to indicate the success (B_TRUE)
 *	or failure (B_FALSE) of the operation.  If B_FALSE is returned, a
 *	sigqueue_t is allocated, assigned to (*sqpp), and its sq_info is
 *	filled in.
 *
 *	Since we have not yet picked up the faulting address (and the error
 *	code), we cannot be preempted.  This function is entered with
 *	preemption disabled.  We enable preemption after the necessary
 *	fault state has been saved.
 */
boolean_t
upageflt(sigqueue_t **sqpp, vaddr_t *faultaddrp)
{
	vaddr_t faultaddr = (vaddr_t)_cr2();
	int errcode = l.trap_err_code;

	ASSERT(!servicing_interrupt());

	*faultaddrp = faultaddr;

	/*
	 * Enable preemption now that we've stored faultaddr and errcode
	 * in local variables.
	 */
	ENABLE_PRMPT();

	return upageflt_cmn(faultaddr, errcode, sqpp);
}

/*
 * boolean_t 
 * kpageflt(uint_t fcflags)
 *	i386 family-specific pagefault handling routine to service kernel-mode
 *	originating faults
 *
 * Calling/Exit State: 
 *	The i386-family version of kpageflt is called with a single argument,
 *	the fault-catch flags from the u area. These must be passed since the
 *	flags must be zeroed by our caller (trap) to protect against the
 *	erroneous processing of subsequent wild pointer derefs (in other words,
 * 	we don't trust ourselves). All other specifics can be determined from 
 *	machine registers and global data.	
 *
 *	A boolean is returned to the caller to indicate the success (B_TRUE)
 *	or failure (B_FALSE) of the operation. A failure indicates that the
 *	fault could not be satisfied but that the generating code had installed
 *	a fault handler to attempt recovery. u.u_fault_catch.fc_errno will be
 *	set to indicate the exact error.
 *
 *	Since we have not yet picked up the faulting address (and the error
 *	code), we cannot be preempted. This function is entered with
 *	preemption disabled. We enable preemption after we have saved the 
 *	necessary state.
 *
 */
boolean_t
kpageflt(uint_t fcflags)
{
	vaddr_t faultaddr = (vaddr_t)_cr2();
	int errcode = l.trap_err_code;
	enum seg_rw rw;
	faultcode_t flt;

	/*
	 * Enable preemption now that we've stored faultaddr and errcode
	 * in local variables.
	 */
	ENABLE_PRMPT();

	/*
	 * If this is a fault on a user virtual address generated from within
	 * kernel mode attempt to handle it (call upageflt) if the following 
	 * is true:
	 *
	 * 	- Context has indicated it is manipulating user data and has
	 *	  marked itself as catching faults in anticipation of such.
	 *
 	 * AND:
 	 *	
	 *	- We are NOT in the middle of servicing an interrupt. Since
	 *	  I/O can be generated by one context and serviced in another
	 *	  it is a mistake to manipulate user addresses at this time.
	 *
	 * If these conditions are not met, we panic.
	 */

	if (VALID_USR_RANGE(faultaddr, 1)) {
		proc_t *pp;
		struct as *asp;

		if ((pp = u.u_procp) != NULL && (asp = pp->p_as) != NULL &&
		    !servicing_interrupt()) {
			if (fcflags & (CATCH_UFAULT|CATCH_ALL_FAULTS)) {
				if (upageflt_cmn(faultaddr, errcode,
					     (sigqueue_t **)NULL) == B_FALSE) {
					u.u_fault_catch.fc_errno = EFAULT;
					return B_FALSE;
				}
				return B_TRUE;
			}
#ifndef UNIPROC
			/*
			 * We have to handle an obscure situation here.
			 * If the page is softlocked or memory locked,
			 * drivers can access this page without 
			 * calling CATCH_FAULTS(). But this could still
			 * fault because of the shootdown optimization
			 * of unloading the L1 entries (only in MP kernels).
			 */
			else if (HOP_FASTFAULT(asp, faultaddr, S_READ))
					return B_TRUE;
#endif
		}	/* asp != NULL &&  !servicing_interrupt() */
		/*
		 *+ A reference to an unloaded and/or unmapped user virtual
		 *+ address was generated from within the kernel. The reference
		 *+ was either unanticipated by the generating code (e.g. we
		 *+ may be dealing with a wild pointer) and we did not indicate
		 *+ interest in handling user virtual address page faults or
		 *+ the reference was generated in an interrupt context where
		 *+ user virtual space cannot be reliably examined.
		 *+ The situation requires post-mortem analysis to determine 
		 *+ the exact cause and response.
		 */
		cmn_err(CE_PANIC, 
			"kernel-mode address fault on user address 0x%x\n",
			faultaddr);
	}

	if (servicing_interrupt()) {
		/*
		 *+ A reference to an unloaded and/or unmapped user virtual
		 *+ address was generated from within the kernel while
		 *+ while servicing an interrupt.  This may have been an
		 *+ invalid address, or it may have been a pageable address,
		 *+ which is illegal to access from interrupt level.
		 *+ The situation requires post-mortem analysis to determine 
		 *+ the exact cause and response.
		 */
		cmn_err(CE_PANIC,
			"address fault from interrupt routine; "
			"kernel address 0x%x\n", faultaddr);
	}

	rw = (errcode & PF_ERR_WRITE) ? S_WRITE : S_READ_ACCESS;

	switch (errcode & PF_ERR_MASK) {

	case PF_ERR_PAGE:
		flt = as_fault(&kas, faultaddr, 1, F_INVAL, rw);
		break;

	case PF_ERR_PROT:
		flt = as_fault(&kas, faultaddr, 1, F_PROT, rw);
		break;

	default:
		flt = FC_HWERR;
		break;
	}

	/*
	 * 'normalize' the code returned from as_fault into an errno
	 * as appropriate
	 */

	if (flt == 0)
		errcode = 0;
	else if (FC_CODE(flt) == FC_OBJERR)
                errcode = FC_ERRNO(flt);
        else
		errcode = EFAULT;

	/*
	 * If we were unable to satisfy this fault then look to see if we
	 * were catching faults on all or some kernel segments and continue
	 * on as appropriate.
	 */

	if (errcode != 0) {
		struct seg *seg;

		if (!servicing_interrupt() && ((fcflags & CATCH_ALL_FAULTS) ||
                    (((seg = as_segat(&kas, faultaddr)) == segkmap &&
                    (fcflags & CATCH_SEGMAP_FAULT)) ||
                    (seg == segkvn && (fcflags & CATCH_SEGKVN_FAULT))))) {
			u.u_fault_catch.fc_errno = errcode;
			return B_FALSE;
		}
		
		/*
		 *+ Reference to an unmapped kernel virtual address was made
		 *+ and the code which generated the reference did not install
		 *+ a fault catching routine to recover. The situation requires
		 *+ post-mortem analysis to determine the cause and response.
		 */
		cmn_err(CE_PANIC, 
			"kernel-mode address fault on kernel address 0x%x\n",
			faultaddr);
	}

	return B_TRUE;
}


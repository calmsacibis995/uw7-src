#ident	"@(#)kern-i386:util/locktest.c	1.19.2.1"
#ident	"$Header$"

/*
 * code for the _LOCKTEST option, including hierarchy stack manipulations
 * and other sanity checks. Also, code to examine locking related
 * performance problems.
 */

/*
 * functions in this file should only be called when DEBUG is defined,
 * so we only want to know about DEBUG versions of structs.  We define
 * DEBUG here, and this file gets compiled just once.  Even non-DEBUG
 * kernels may have these routines, for drivers that do have DEBUG.
 * This file is only compiled once, and DEBUG is always on.
 */
#ifndef DEBUG
#define DEBUG
#endif /* DEBUG */

#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/locktest.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#define PLBASE_ON_INT(ipl)	( ((ipl) == plbase) && servicing_interrupt() )

/*
 * We keep track of the  number of spin lock round-trips. Since we are 
 * not interested in exact counts, these are bumped up without the 
 * protection of any locks.
 */

ulong_t	spinlk_count, fspinlk_count, rwspinlk_count;

/*
 * void
 * xintr_restore(ulong_t)
 *	Restore interrupts.
 *
 * Calling/Exit State:
 *	Takes the new flags word (previously returned from xintr_disable)
 *	and loads it into the eflags.  Returns: none.
 */
asm void
xintr_restore(ulong_t w)
{
%mem w;
	movl	w, %eax
	pushl	%eax
	popfl
}

/*
 * ulong_t
 * xintr_disable(void)
 *	Disable interrupts.
 *
 * Calling/Exit State:
 *	Returns the previous flags word.
 */
asm ulong_t
xintr_disable(void)
{
	pushfl
	cli
	popl	%eax
}

/*
 * void
 * hier_push(void *lockp, int hierarchy, lkinfo_t *lockinfop)
 *	Pushes a lock on the hierarchy stack, and checks that the
 *	hierarchy is preserved.
 *
 * Description:
 *	Pushes the pointer to the lock, the
 *	hierarchy value of the lock and info on the lock.  
 *	This way, if the locks aren't released
 * 	in the opposite order of acquisition, we're still capable
 *	of finding the right hierarchy value to remove.  Also, it
 *	lets functions assert, for instance, that l_mutex is held
 *	by this processor.
 *
 * Calling/Exit State:
 *	Returns:  None.
 */
void
hier_push(void *lockp, int hierarchy, lkinfo_t *lockinfop)
{
	register int oldhier;		/* previous hierarchy value */
	register int top;		/* temp for l.hier_stack.hs_top */
	ulong_t w;
	char	*oldnamep;

	if (l.disable_locktest)
		return;

	if (l.panic_level) {
		hier_push_nchk(lockp, hierarchy, lockinfop);
		return;
	}
	/*
	 * we need to disable interrupts here because we may take
	 * an interrupt, and the interrupt context may acquire a
	 * lock and call this code, causing the data structure to
	 * change under our feet.
	 */
	w = xintr_disable();
	top = l.hier_stack.hs_top;
	ASSERT(top < STACKMAX);

	/*
	 * whenever we push, we make sure that the hierarchy
	 * is preserved; that is, that the top element of the
	 * stack has a lower hierarchy value than the one we're
	 * about to push.  That is, if you want to lock A, then
	 * B, B's hierarchy value must be greater than
	 * A's hierarchy value.
	 */
	l.hier_stack.hs_stack[top].hss_lockp = lockp;
	l.hier_stack.hs_stack[top].hss_value = (ushort_t) hierarchy;
	l.hier_stack.hs_stack[top].hss_infop = lockinfop;
	if (top > 0) {
		/* this is not the first lock */
		oldhier = l.hier_stack.hs_stack[top - 1].hss_value;
		oldnamep = l.hier_stack.hs_stack[top - 1].hss_infop->lk_name;
		if (oldhier >= hierarchy) {
			/*
			 * hierarchy violation.  We don't change the 'top'
			 * pointer, but the kernel programmer can find the
			 * address of the lock that's causing the problem.
			 */
			xintr_restore(w);
			/*
			 *+ A processor attempted to acquire a lock out of
			 *+ order in the lock hierarchy.  This indicates a
			 *+ kernel software problem.
			 */
			cmn_err(CE_PANIC,
				"lock hierarchy violation\n"
				 "\told lock = 0x%x, old hier = 0x%x,"
				  " name = %s\n"
                                 "\tnew lock = 0x%x, new hier = 0x%x,"
				  " name = %s",
                                l.hier_stack.hs_stack[top-1].hss_lockp,
                                oldhier, oldnamep, lockp, 
				hierarchy, lockinfop->lk_name);
			/* NOTREACHED */
		}
	}
	++l.hier_stack.hs_top;
	xintr_restore(w);
}

/*
 * void
 * hier_push_same(void *lockp, int hierarchy, lkinfo_t *lockinfop)
 *	Pushes a lock of the same hierarchy on the stack.
 *
 * Description:
 *	Pushes both the pointer to the lock  the
 *	hierarchy value and info pointer on the stack.  
 *	This way, if the locks aren't released
 * 	in the opposite order of acquisition, we're still capable
 *	of finding the right hierarchy value to remove.  Also, it
 *	lets functions assert, for instance, that l_mutex is held
 *	by this processor.  This routine is called only by the
 *	LOCK_SH() and RW_XXLOCK_SH() interfaces.
 *
 * Calling/Exit State:
 *	Returns:  None.
 */
void
hier_push_same(void *lockp, int hierarchy, lkinfo_t *lockinfop)
{
	register int oldhier;		/* previous hierarchy value */
	register int top;		/* temp for l.hier_stack.hs_top */
	ulong_t w;
	char *oldnamep;

	if (l.disable_locktest)
		return;

	if (l.panic_level) {
		hier_push_nchk(lockp, hierarchy, lockinfop);
		return;
	}
	/*
	 * we need to disable interrupts here because we may take
	 * an interrupt, and the interrupt context may acquire a
	 * lock and call this code, causing the data structure to
	 * change under our feet.
	 */
	w = xintr_disable();
	top = l.hier_stack.hs_top;
	ASSERT(top < STACKMAX);

	/*
	 * whenever we push, we make sure that the hierarchy
	 * is preserved; that is, that the top element of the
	 * stack has a lower hierarchy value than the one we're
	 * about to push.  That is, if you want to lock A, then
	 * B, B's hierarchy value must be greater than
	 * A's hierarchy value.
	 */
	if (top > 0) {
		/* this is not the first lock */
		oldhier = l.hier_stack.hs_stack[top - 1].hss_value;
		oldnamep = l.hier_stack.hs_stack[top - 1].hss_infop->lk_name;
		if (oldhier != hierarchy) {
			xintr_restore(w);
			/*
			 *+ LOCK_SH was used to acquire a lock with a
			 *+ hierarchy value different than the hierarchy
			 *+ value of the top lock on the stack.  This indicates
			 *+ a kernel software problem.
			 */
			cmn_err(CE_PANIC,
				"LOCK_SH hierarchy violation\n"
				 "\told lock = 0x%x, old hier = 0x%x,"
				  " name = %s\n"
				 "\tnew lock = 0x%x, new hier = 0x%x,"
				  " name = %s",
				l.hier_stack.hs_stack[top-1].hss_lockp,
				oldhier, oldnamep, lockp,
				hierarchy, lockinfop->lk_name);
				/* NOTREACHED */
		}
		l.hier_stack.hs_stack[top].hss_lockp = lockp;
		l.hier_stack.hs_stack[top].hss_value = (ushort_t) hierarchy;
		l.hier_stack.hs_stack[top].hss_infop = lockinfop;
		++l.hier_stack.hs_top;
		xintr_restore(w);
		return;
	}
	xintr_restore(w);
	/*
	 *+ A processor called LOCK_SH when no other spin locks were
	 *+ held.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "LOCK_SH called with no other locks held\n"
			   "\tnew lock = 0x%x, new hier = 0x%x, name = %s",
			  lockp, hierarchy, lockinfop->lk_name);
	/* NOTREACHED */
}

/*
 * void
 * hier_push_nchk(void *lockp, int hierarchy, lkinfo_t *lockinfop)
 *	The same as hier_push, but no checking of the hierarchy is
 *	done. 
 *
 * Calling/Exit State:
 * 	lockp is the lock being acquired.  hierarchy is it's hierarchy
 *	value.
 *
 * Description:
 *	This is for, say, XX_TRYLOCK, which is a way of avoiding a strict
 *	lock ordering.  This isn't really a push, because we insert the
 *	given lock in *	the hierarchy where it belongs, not necessarily
 *	on the top.  So you can't do a LOCK w/ hier 5, TRYLOCK w/ hier
 *	3, and then a LOCK w/ hier 4.  But the first two would be okay.
 */
void
hier_push_nchk(void *lockp, int hierarchy, lkinfo_t *lockinfop)
{
	register int top, i;
	int spareh;				/* temp holds h value */
	void *sparelockp;				/* temp holds lockp */
	lkinfo_t *spareinfop;
	ulong_t w;

	if (l.disable_locktest)
		return;

	/*
	 * we disable interrupts here because we may take an interrupt,
	 * and the interrupt context may acquire a lock and call this
	 * code, causing the data structure to change under our feet.
	 */
	w = xintr_disable();
	top = l.hier_stack.hs_top;
	ASSERT(top < STACKMAX);

	/*
	 * find the place on the stack where we want to insert the
	 * hierarchy value.
	 */
	for (i = top; i > 0; --i) {
		if (l.hier_stack.hs_stack[i-1].hss_value <= (ushort_t)hierarchy)
			break;
	}
	/*
	 * insert the new value in the stack, using 'spare' as a temp
	 * while we bubble up the ones above.  If we happen to find an
	 * unused slot, we fill it and quit.
	 */
	while (i < top) {
		spareh = l.hier_stack.hs_stack[i].hss_value;
		sparelockp = l.hier_stack.hs_stack[i].hss_lockp;
		spareinfop = l.hier_stack.hs_stack[i].hss_infop;
		l.hier_stack.hs_stack[i].hss_value = (ushort_t) hierarchy;
		l.hier_stack.hs_stack[i].hss_lockp = lockp;
		l.hier_stack.hs_stack[i].hss_infop = lockinfop;
		hierarchy = spareh;
		lockp = sparelockp;
		lockinfop = spareinfop;
		++i;
	}
	l.hier_stack.hs_stack[top].hss_value = (ushort_t) hierarchy;
	l.hier_stack.hs_stack[top].hss_lockp = lockp;
	l.hier_stack.hs_stack[top].hss_infop = lockinfop;
	++l.hier_stack.hs_top;
	xintr_restore(w);
}

/*
 * void
 * hier_remove(void *lockp, lkinfo_t *lockinfop, pl_t ipl)
 *	Remove a lock from the hierarchy stack.
 *
 * Calling/Exit State:
 *	lockp is a pointer to the lock being released.
 *
 * Description:
 *	This is not a pop, becuase we look down the stack and remove the
 *	entry with the right lock pointer.  We then copy the rest of the
 *	stack down over the removed entry.
 */
void
hier_remove(void *lockp, lkinfo_t *lockinfop, pl_t ipl)
{
	int i;
	int top;
	int hier;
	int max_minipl;
	ulong_t w;

	if (l.disable_locktest)
		return;

	/* 
	 * we disable interrupts here because we may take an interrupt,
	 * and the interrupt context may acquire a lock and call this
	 * code, causing the data structure to change under our feet.
	 */
	w = xintr_disable();
	ASSERT(!PLBASE_ON_INT(ipl));
	ASSERT(l.hier_stack.hs_top > 0);
	--l.hier_stack.hs_top;
	for (i = l.hier_stack.hs_top; i >= 0; --i) {
		if (l.hier_stack.hs_stack[i].hss_lockp == lockp) {
			/*
			 * Copy the rest of the stack down.
			 */
			while (i <= l.hier_stack.hs_top) {
				l.hier_stack.hs_stack[i] =
					l.hier_stack.hs_stack[i+1];
				++i;
			}
			/*
			 * Assert that the ipl at which the lock is
			 * being unlocked is sufficiently high to protect
			 * the other locks presently held
			 */
			if (l.hier_stack.hs_top > 0) {
				top = l.hier_stack.hs_top;
				hier = l.hier_stack.hs_stack[top -1].hss_value;
				max_minipl = (hier >> NBBY);
				CONVERT_IPL(ipl, ipl);
				ASSERT(ipl >= (pl_t)max_minipl);
			}
			xintr_restore(w);
			return;
		}
	}
	xintr_restore(w);
	/*
	 *+ An attempt was made to remove the hierarchy stack entry for
	 *+ a lock that was not in the hierarchy statck.  This indicates
	 *+ a kernel software problem.
	 */
	cmn_err(CE_PANIC, "can't find lock in hierarchy stack\n"
			   "\tlock = 0x%x, ipl = 0x%x, name = %s",
			  lockp, ipl, lockinfop->lk_name);
	/* NOTREACHED */
}

/*
 * boolean_t
 * hier_findlock(void *lockp)
 *	See if we hold the given lock.
 *
 * Calling/Exit State:
 *	returns B_TRUE if we hold lockp, B_FALSE otherwise.
 */
boolean_t
hier_findlock(void *lockp)
{
	int i;
	ulong_t w;

	if (l.disable_locktest)
		return (B_TRUE);
	/*
	 * we must disable interrupts to prevent interrupt contexts from
	 * manipulating this data structure.
	 */
	w = xintr_disable();
	for (i = l.hier_stack.hs_top - 1; i >= 0; --i) {
		if (lockp == l.hier_stack.hs_stack[i].hss_lockp) {
			xintr_restore(w);
			return (B_TRUE);
		}
	}
	xintr_restore(w);
	return (B_FALSE);
}

/*
 * boolean_t
 * hier_lockcount(int count) 
 *	count the total number of locks held by this processor.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if the number of held locks equals count.
 */
boolean_t
hier_lockcount(int count) 
{
	int	lkcount = l.hier_stack.hs_top;

	if (l.disable_locktest)
		return (B_TRUE);
	/*
	 * If the calling context holds any fast spin lock include that 
	 * in the count. Note that a context can at most hold one fast 
	 * spin lock at a time.
	 */
	if (l.holdfastlock)
		++lkcount;

	return (lkcount == count);
}

#undef KS_HOLD0LOCKS

/*
 * boolean_t
 * KS_HOLD0LOCKS(void)
 *	Check if no locks are held by current context.
 *
 * Calling/Exit State:
 *	Only locks acquired under _LOCKTEST will be counted.
 */
boolean_t
KS_HOLD0LOCKS(void)
{
	return hier_lockcount(0);
}

/*
 * void
 * locktest(void *lockp, pl_t minipl, pl_t newipl, int locktype,
 *		lkinfo_t *lockinfop) 
 *	Performs sanity checks for lock being acquired. locktype specifies
 *	whether the lock is an exclusive spin lock or a read/write spin lock.
 *
 * Calling/Exit State:
 *	None.  If a problem is found, the system is panic'ed.
 *
 * Remarks:
 *	We want only one version of this routine, and it should be created
 *	when DEBUG is defined (so the ASSERTS compile into real code).
 *
 *	Called only when SP_LOCKTEST is defined.
 *
 */
void
locktest(void *lockp,			/* the lock being locked */
	pl_t minipl,			/* lockp->minipl */
	pl_t newipl,			/* ipl at which lock is being locked */
	int locktype,			/* type of lock: excl. or r/w */
	lkinfo_t *lockinfop)		/* infop */
{
	pl_t	curipl, ipl_base, max_minipl;
	lkstat_t *lksp;
	int top, hier;

	if (l.disable_locktest)
		return;

	if (locktype == SP_LOCKTYPE) {
		/*
		 * Exclusive spin lock
		 */
	   	if (((lock_t *)lockp)->sp_flags & KS_DEINITED) {
			/*
			 *+ Someone has attempted to use a spin lock that was 
			 *+ already de-initialized. This is a bug in system
			 *+ software, and needs to be detected and corrected.
			 */
	 	   	cmn_err(CE_PANIC, 
				"lock 0x%x lkinfo 0x%x already deinited",
				lockp, lockinfop);
			/* NOTREACHED */
	   	}
		lksp = ((lock_t *)lockp)->sp_lkstatp;
		if ((lksp != NULL) && (lksp->lks_infop != lockinfop)) {
			/*
			 *+ There is a name mismatch between the lock and its
			 *+ stat buffer. This may indicate that some data
			 *+ structure corruption has occurred, which needs to
			 *+ be investigated and corrected.
			 */
			cmn_err(CE_PANIC, "Mismatched LKINFOP: sp lock 0x%x\n",
				lockp);
			/* NOTREACHED */
		}

	   } else {
		/*
		 * It must be a R/W spin lock.
		 */
	   	ASSERT(locktype == RWS_LOCKTYPE); 
	   	if (((rwlock_t *)lockp)->rws_flags & KS_DEINITED) {
			/*
			 *+ Someone has attempted to use a r/w spin lock that 
			 *+ was already de-initialized. This is a bug in system
			 *+ software, and needs to be detected and corrected.
			 */
			cmn_err(CE_PANIC, 
				"rwlock 0x%x lkinfo 0x%x already deinited\n",
				lockp, lockinfop);
			/* NOTREACHED */
	   	}
		
		lksp = ((rwlock_t *)lockp)->rws_lkstatp;
		if ((lksp != NULL) && (lksp->lks_infop != lockinfop)) {
			/*
			 *+ There is a name mismatch between the lock and its
			 *+ stat buffer. This may indicate that some data
			 *+ structure corruption has occurred, which needs to
			 *+ be investigated and corrected.
			 */
			cmn_err(CE_PANIC,
				"Mismatched LKINFOP: rws lock 0x%x\n",
				lockp);
			/* NOTREACHED */
		}
	}

	/*
	 * The new ipl must be as high as the min ipl.
	 */
	CONVERT_IPL(newipl, newipl);
	if (newipl < minipl) {
		/*
		 *+ A spin lock acquired at an ipl lower than its minipl.
		 */
		cmn_err(CE_PANIC,
			"Lock acquired below its minipl\n"
			 "\tlock = 0x%x, minipl = 0x%x, name = %s\n"
			 "\tacquisition ipl = 0x%x",
			lockp, minipl, lockinfop->lk_name, newipl);
		/* NOTREACHED */
	}

	/*
	 * It's an error to lower the ipl while locking unless no other
	 * locks are held.
	 */
	if (!hier_lockcount(0)) {
		curipl = getpl();
		CONVERT_IPL(curipl, curipl);
		if (newipl < curipl) {
			/*
			 * We need to allow for compatibility workarounds
			 * here. Under DDI specifications, timeout() 
			 * will dispatch the callouts at plhi and it raises
			 * pl value to be PLHI through splhi(). To allow for
			 * case, we should not panic the system if this
			 * situation was caused by timeout(). We detect if
			 * this is the case through the following mechanism:
			 *        timeouts can occur only when 
			 *		1) no locks are held or
			 *		2) only if PLBASE locks are held.
			 *		i.e. the system needs to be at 
			 * 		base level.
			 * We wouldn't be in this code path if condition (1)
			 * were true. So we check for condition (2) here.
			 */
			top = l.hier_stack.hs_top;
			hier = l.hier_stack.hs_stack[top -1].hss_value;
			max_minipl = (hier >> NBBY);
			CONVERT_IPL(ipl_base, PLBASE);
			if (max_minipl != ipl_base) {
				/*
				 *+ Illegal ipl lowering.
				 */
				cmn_err(CE_PANIC,
					"Illegal ipl lowering\n"
					"\tlock = 0x%x, curipl = 0x%x, \n name = %s\n" 
					"\tacquisition ipl = 0x%x",
					lockp, curipl, lockinfop->lk_name,
					newipl);
				/* NOTREACHED */
			}
		}
	}

	/*
	 * it's an error to lock a lock we already hold.
	 */
	if (hier_findlock(lockp)) {
		/*
		 *+ A processor attempted to acquire a lock already held
		 *+ on that processor.  This indicates a kernel software
		 *+ problem.
		 */
		cmn_err(CE_PANIC,
			"recursive acquisition of lock\n"
			 "\tlock = 0x%x",
			lockp);
		/* NOTREACHED */
	}
}



#ifdef _LOCKTEST

/*
 * void
 * print_locks(int cpu)
 *	Produce a formatted pretty-printed version of the lock stack
 *
 * Calling/Exit State:
 *	Generally called from kdb by explict request (command line)
 *	after a panic due to a lock violation. A nagative CPU ID will 
 *	dump the lock stacks of all the CPUs while a valid CPU ID will 
 *	dump the lock stack of the specific CPU.
 */ 
void
print_locks(int cpu)
{
	int i, j;
	struct plocal *plp;
	hier_stackslot_t *hsp;

	if (cpu >= Nengine) {
		debug_printf("Invalid Engine #\n");
		return;
	}
	for (j = Nengine; j-- != 0;) {
		if (cpu >= 0 && cpu != j)
			continue;
		plp = ENGINE_PLOCAL_PTR(j);
		debug_printf("\n ENGINE #: %d\n", j);
		debug_printf("\thier\tlockp\t\tname\n");
		hsp = plp->hier_stack.hs_stack;
		for (i = 0; i < plp->hier_stack.hs_top; i++, hsp++) {
			debug_printf("  [%2d]\t0x%x\t0x%x\t%s\n",
				     i, hsp->hss_value, hsp->hss_lockp,
				     hsp->hss_infop->lk_name);
			debug_printf("\n");
			if (debug_output_aborted())
				return;
		}
	}
} 

#endif /* _LOCKTEST */

#ifdef _MPSTATS

/*
 * void print_lkstat(const lkstat_t *statp)
 * 	This function can be used to dump statistics of a given lock. The 
 * 	input parameter to this function is a pointer to a statistics
 *	structure.
 *
 * Calling/Exit State: 
 *	The function assumes the argument is valid.
 */

void
print_lkstat(const lkstat_t *statp)
{
	debug_printf("Name of the lock: %s\n", statp->lks_infop->lk_name);
	debug_printf(
		"Number of times the lock was acquired in the write mode: %d\n",
		statp->lks_wrcnt);
	debug_printf(
		"Number of times the lock was acquired in the read mode: %d\n",
		statp->lks_rdcnt);
	debug_printf(
	  "Number of times the lock was first acquired in the read mode: %d\n",
		statp->lks_solordcnt);
	debug_printf("Number of times the lock acquisition failed: %d\n",
		statp->lks_fail);

	debug_printf("Cumulative wait time low order: 0x%x\n", 
		statp->lks_wtime.dl_lop);
	debug_printf("Cumulative wait time high order: 0x%x\n", 
	        statp->lks_wtime.dl_hop);
	debug_printf("Cumulative hold time low order: 0x%x\n", 
		statp->lks_htime.dl_lop);
	debug_printf("Cumulative hold time high order: 0x%x\n", 
	        statp->lks_htime.dl_hop);
}

#endif /* _MPSTATS */

/*
 * void begin_lkprocess(void *lkp, void *retpc)
 *	This function will be called after a spin lock has been acquired
 *	to collect and maintain several important system characteristics
 *	with respect to spin locks (fast, regular and r/w kind). Some of the 
 *	metrics that will be collected will be:
 *		a) Maximum nesting depth of spin locks.
 *		b) Maximum duration for which a given CPU is holding
 *		   one or more spin locks.
 *
 * Calling/Exit State:
 *	None. All the statistics will be maintained on a per-engine basis.
 */
void
begin_lkprocess(void *lkp, void *retpc)
{
	ulong_t		psw;

	if (l.disable_locktest)
		return;

	/*
	 * Block interrupts to atomically get all the info.
	 */

	psw = xintr_disable();
	if (++l.lk_depth == 1) {
		/*
		 * This is the first lock aquisition. Get all the info.
		 */
		l.lk_lkpf = lkp;
		l.lk_retpcf = retpc;
		GET_TIME(&l.lk_stime);
	}
	if (l.lk_depth > l.lk_mxdepth)
		l.lk_mxdepth = l.lk_depth;
	xintr_restore(psw);
}

/*
 * void end_lkprocess(void *lkp, void *retpc)
 *	This function is called after the lock is released to record
 * 	locking related metrics.
 *
 * Calling/Exit State:
 *	None.
 */

void
end_lkprocess(void *lkp, void *retpc)
{
	ulong_t		psw;
	ulong_t		delta;

	if (l.disable_locktest)
		return;

	/*
	 * Block interrupts to get a stable view.
	 */
	psw = xintr_disable();
	if (--l.lk_depth == 0) {
		/*
		 * We now no longer hold any locks - get all the stats.
		 */
		GET_DELTA(&delta, l.lk_stime);
		if (delta > l.lk_mxtime) {
			/*
			 * Record the max duration for which we held
			 * one or more spin locks.
			 */
			l.lk_mxtime = delta;
			/*
			 * Record the lock pointer and the return pc (that
			 * is the pc past the call to unlock) that terminated
			 * this long sequence.
			 */
			l.lk_lkpl = lkp;
			l.lk_retpcl = retpc;
			/*
			 * Record the lock pointer and the initial 
			 * return pc (that is the address past the first
			 * call to acquire the lock) that started this 
			 * long sequence.
			 */
			l.lk_mxlkpf = l.lk_lkpf;
			l.lk_mxretpcf = l.lk_retpcf;
		}
	}
	xintr_restore(psw);
}

/*
 * void print_lock_stats(void)
 *	This function prints several locking related metrics for  
 *	the system.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	It is expected that this function will be called from kdb.
 */

void
print_lock_stats(void)
{
	int	i;
	struct plocal	*plp;
	extern ulong_t sleeplk_count, rwsleeplk_count;

	debug_printf("Total number of spin lock round trips: %d\n",
		     spinlk_count);
	debug_printf("Total number of fast spin lock round trips: %d\n",
		     fspinlk_count);
	debug_printf("Total number of r/w spin lock round trips: %d\n",
		     rwspinlk_count);
	debug_printf("Total number of sleep lock round trips: %d\n",
		     sleeplk_count);
	debug_printf("Total number of r/w sleep lock round trips: %d\n",
		     rwsleeplk_count);

	for (i = Nengine; i-- != 0;) {
		debug_printf("\n Engine #: %d\n", i);
		plp = ENGINE_PLOCAL_PTR(i);
		debug_printf("\tMaximum lock nesting depth: %d\n", 
			     plp->lk_mxdepth);
		debug_printf("\tMax duration for which locks were held"
			      " (micro seconds): %d\n",
			     plp->lk_mxtime);
		debug_printf("\tInitial return PC: 0x%x\n", plp->lk_mxretpcf);
		debug_printf("\tInitial lock pointer: 0x%x\n",
			     plp->lk_mxlkpf);
		debug_printf("\tFinal return PC: 0x%x\n", plp->lk_retpcl);
		debug_printf("\tFinal lock pointer: 0x%x\n",
			     plp->lk_lkpl);
		if (debug_output_aborted())
			return;
	}
}


/*
 * void begin_intprocess(void *handler, void *retpc, int bin)
 *	Instrument interrupt processing.
 * Calling/Exit State: 
 *	It is assumed that when this code is called, all interrupts that
 *	come in at the ipl at which this interrupt is being serviced will
 *	be blocked. If this assumption is violated, then the statistics
 *	gathered will not be accurate.
 *
 * Remarks:
 *	presently, we schedule timeouts directly from the clock interrupt
 *	handler at pltimeout (pl1) if the previous ipl was PLBASE. So, 
 *	it is possible that we  may take interrupts at the same level as
 *	those that may have been preempted by higher level interrupts.
 *	Infact, we could take another clock interrupt, prior to returning
 *	from the first clock handler. Under these cicumstances the data
 *	gathered will not be accurate. A possible solution will be to 
 *	maintain all statistics on a stack structure. For now, this should 
 *	do! 
 *	
 */
void
begin_intprocess(void *handler, void *retpc, int bin)
{
	int	top = l.intr_stack.intr_top++;

	if ((bin >= MAX_INTR_LEVELS) || (top >= MAX_INTR_NESTING)) {
		/*
	 	 *+ The bin/ipl passed in exceeds the present limit. Increase
	 	 *+ MAX_INTR_LEVELS and/or MAX_INTR_NESTING to avoid 
		 *+ this problem.
	 	 */
		cmn_err(CE_PANIC, 
			"Interrupt statistics data structures too small\n");
		/* NOTREACHED */
	}

	l.intr_stat[bin].intr_count++;
	GET_TIME(&l.intr_stat[bin].intr_start);
	l.intr_stat[bin].intr_handler = handler;
	l.intr_stat[bin].intr_retpc = retpc;
	/*
	 * Now save the bin number on the stack.
	 */
	l.intr_stack.intr_stack[top] = bin;
}

/*
 * void end_intprocess(void)
 *	Do all the terminal processing.
 *
 * Calling/Exit State: 
 *	None.
 */
void
end_intprocess(void)
{
	ulong_t		delta;
	int		bin;
	int		top;

	/*
	 * get our bin number from the stack.
	 */
	ASSERT(l.intr_stack.intr_top > 0);
	top = --l.intr_stack.intr_top;
	bin = l.intr_stack.intr_stack[top];

	GET_DELTA(&delta, l.intr_stat[bin].intr_start);
	if (delta > l.intr_stat[bin].intr_mx) {
		l.intr_stat[bin].intr_mx = delta;
		l.intr_stat[bin].intr_mxhandler = l.intr_stat[bin].intr_handler;
		l.intr_stat[bin].intr_mxretpc = l.intr_stat[bin].intr_retpc;
	}
}

/*
 * void print_intr_stats(int cpu)
 *	Prints all the interrupt metrics. If a valid processor Id is 
 *	specified, the metrics for the specified processor are printed. If a 
 *	negative processor ID is specified, then the metrics for all the 
 *	processors are printed.
 *
 * Calling/Exit State:
 *	None.
 */

void
print_intr_stats(int cpu)
{
	int	i, j;
	struct plocal	*plp;
	
	if (cpu < 0) {
		for (i = Nengine; i-- != 0;) {
			debug_printf("\n Engine #: %d\n", i);
			plp = ENGINE_PLOCAL_PTR(i);
			for (j=0; j < MAX_INTR_LEVELS; j++) {
				debug_printf("\n Bin/ipl #: %d\n", j);
				debug_printf("\tTotal # of interrupts: %d\n",  
					     plp->intr_stat[j].intr_count);
				debug_printf(
					"\tMax service time (micro secs): %d\n",
					     plp->intr_stat[j].intr_mx);
				debug_printf(
					"\tMax service handler pointer: 0x%x\n",
					     plp->intr_stat[j].intr_mxhandler); 
				debug_printf(
					"\tMax service return pc: 0x%x\n",
					     plp->intr_stat[j].intr_retpc);
				if (debug_output_aborted())
					return;
			 }
		}
		return;
	}
	if (cpu >= Nengine) {
		debug_printf("Invalid processor ID\n");
		return;
	}
	plp = ENGINE_PLOCAL_PTR(cpu);
	for (j = 0; j < MAX_INTR_LEVELS; j++) {
		debug_printf("\n Bin #: %d\n", j);
		debug_printf("Total # of interrupts %d\n",
			     plp->intr_stat[j].intr_count);
		debug_printf("Max service time (micro secs): %d\n",
			     plp->intr_stat[j].intr_mx);
		debug_printf("Max service handler pointer 0x%x\n",
			     plp->intr_stat[j].intr_mxhandler);
		debug_printf("Max service return pc: 0x%x\n",
			     plp->intr_stat[j].intr_retpc);
		if (debug_output_aborted())
			return;
	}
}

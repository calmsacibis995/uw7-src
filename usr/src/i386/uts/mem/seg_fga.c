/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)kern-i386:mem/seg_fga.c	1.1.2.2"
#ident	"$Header$"

/*
 * segfga is a user level segment driver for mapping a special brand
 * of shared memory (one which is always locked into memory and which
 * supports page-granular affinity within the segment).
 *
 * segfga uses identityless memory (memory which has no associated page_t
 * structures, and thus no vnode,offset identity) and shared page tables.
 * The use of shared page tables imposes several externally visible
 * restrictions, such as strict alignment restrictions, the inability to
 * change protections on the pages (because they would be seen by all users
 * of the page), etc.
 */
#include <io/conf.h>
#include <mem/as.h>
#include <mem/fgashm.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/rzbm.h>
#include <mem/seg.h>
#include <mem/seg_fga.h>
#include <mem/vmparam.h>
#include <proc/cg.h>
#include <proc/cguser.h>
#include <proc/ipc/shm.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/list.h>
#include <util/map.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef PAE_MODE

/*
 * Stubs for non-PAE kernels. Typically only used for
 * kstuff'ed kernels.
 */

int segfga_create(struct seg *seg, void *argsp) { return ENOSYS; }
int shm_makefga(struct kshmid_ds *kshmp, size_t nbytes,
		int flag) { return ENOSYS; }
void shm_rm_fga(struct kshmid_ds *kshmp) { return; }
int fgashm_reaffine(struct kshmid_ds *kshmp,
		    struct shmid_ds *shmp){ return ENOSYS; }
void fgashm_lockmem(struct kshmid_ds *kshmp) { return; }

struct seg_ops segfga_ops;

#else /* PAE_MODE *?


/*
 * Private seg op routines.
 */
STATIC int segfga_dup(struct seg *, struct seg *);
STATIC int segfga_unmap(struct seg *, vaddr_t, uint_t);
STATIC void segfga_free(struct seg *);
STATIC faultcode_t segfga_fault(struct seg *, vaddr_t, uint_t, enum fault_type,
				enum seg_rw);
STATIC int segfga_setprot(struct seg *, vaddr_t, uint_t, uint_t);
STATIC int segfga_checkprot(struct seg *, vaddr_t, uint_t);
STATIC int segfga_incore(struct seg *, vaddr_t, uint_t, char *);
STATIC int segfga_getprot(struct seg *, vaddr_t, uint_t *);
STATIC off64_t segfga_getoffset(struct seg *, vaddr_t);
STATIC int segfga_gettype(struct seg *, vaddr_t);
STATIC int segfga_getvp(struct seg *, vaddr_t, vnode_t **);
STATIC void segfga_badop(void);
STATIC int segfga_nop(void);
STATIC void segfga_age(struct seg *, u_int);
STATIC int segfga_memory(struct seg *, vaddr_t *basep, u_int *lenp);
STATIC int segfga_lockop(struct seg *seg, vaddr_t addr, u_int len,
			 int attr, int op);


/*
 * NOT static because needed by as_aio_prep(), as_aio_needs_prep(), and
 * as_aio_unprep().
 */
struct seg_ops segfga_ops = {
	segfga_unmap,
	segfga_free,
	segfga_fault,
	segfga_setprot,
	segfga_checkprot,
	(int (*)())segfga_badop,	/* kluster */
	(int (*)())segfga_nop,		/* sync */
	segfga_incore,
	segfga_lockop,
	segfga_dup,
	(void (*)()) segfga_nop,	/* childload */
	segfga_getprot,
	segfga_getoffset,
	segfga_gettype,
	segfga_getvp,
	segfga_age,			/* age */
	(boolean_t (*)())segfga_nop,	/* lazy_shootdown */
	segfga_memory,
	(boolean_t(*)())segfga_nop,	/* xlat_op */
	(int(*)())segfga_nop,		/* memloc */
};

STATIC LKINFO_DECL(segfga_lkinfo, "MS:FGA:segfga mutex", 0);

/*
 * Shared page table interfaces (move to vm_hatshm.h???)
 */
extern hatshpt_t * shpt_hat_init(uint_t sz);
extern void shpt_hat_load(struct as *as, hatshpt_t *hshptp, vaddr_t baseaddr,
      		          uint_t off, ppid_t pf, uint_t prot);
extern void shpt_hat_unload(struct as *as, hatshpt_t *hshptp, uint_t off);
extern void shpt_hat_attach(struct as *as, vaddr_t addr, size_t sz,
               		    hatshpt_t *hshptp, cgnum_t cgnum);
extern void shpt_hat_detach(struct as *as, vaddr_t addr, size_t sz,
				hatshpt_t *hshptp);
extern void shpt_hat_deinit(hatshpt_t *hshptp);
extern uint_t shpt_hat_xlat_info(struct as *as, hatshpt_t *hshptp,
				 vaddr_t addr);



/*
 * int
 * segfga_create(struct seg *seg, void *argsp)
 *	Create a user segment for mapping shared memory which supports
 *	fine grained affinity.
 *
 * Calling/Exit State:
 *	Called with the AS write locked.
 *	Returns with the AS write locked.
 *
 * Description:
 *	Create a segfga type segment.
 */
int
segfga_create(struct seg *seg, void *argsp)
{
	struct segfga_data *sfdp;
	struct segfga_crargs *a = argsp;
	struct fgashm_t *fgap = a->fgap;

	ASSERT(seg->s_as != &kas);
	ASSERT((seg->s_base & (FGASHM_ALIGN - 1)) == 0);
	ASSERT((seg->s_size & (FGASHM_SIZE_QUANTA - 1)) == 0);

	/*
	 * segfga only supports RW attaches (due to the use of shared
	 * page tables amongst all users of the segfga segment).
	 */
	if (((a->prot & SEGFGA_PROT_MASK) != SEGFGA_PROT_MASK) ||
	    ((a->maxprot & SEGFGA_PROT_MASK) != SEGFGA_PROT_MASK)) {
		return EINVAL;
	}

	sfdp = kmem_alloc(sizeof(struct segfga_data), KM_SLEEP);
	sfdp->sfd_vpgpp = fgap->fshm_vpgpp;
	sfdp->sfd_vpglckp = &fgap->fshm_vpglck;
	sfdp->sfd_vpgsvp = &fgap->fshm_vpgsv;
	sfdp->sfd_hatshptp = fgap->fshm_hatshptp;
	sfdp->sfd_affp = &fgap->fshm_aff_head;

	/*
	 * Attach to the shared page tables.
	 */
	
	shpt_hat_attach(seg->s_as, seg->s_base, seg->s_size,
	                fgap->fshm_hatshptp, mycg);

	seg->s_ops = &segfga_ops;
	seg->s_data = sfdp;
	seg->s_as->a_isize += seg->s_size;

	return 0;
}

/*
 * STATIC void
 * segfga_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */
STATIC void
segfga_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ segfga segment driver.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segfga_badop");
	/*NOTREACHED*/
}

/*
 * STATIC int
 * segfga_nop(void)
 *	Do-nothing operation.
 *
 * Calling/Exit State:
 *	Always returns success w/o doing anything.
 */
STATIC int
segfga_nop(void)
{
	return 0;
}

/*
 * STATIC int
 * segfga_dup(struct seg *pseg, struct seg *cseg)
 *	Called from as_dup to replicate segment specific data structures.
 *
 * Calling/Exit State:
 *	The parent's address space is read locked on entry to the call and
 *	remains so on return.
 *
 *	The child's address space is not locked on entry to the call since
 *	there can be no active LWPs in it at this point in time.
 *
 *	On success, 0 is returned to the caller and s_data in the child
 *	generic segment stucture points to the newly created segfga_data.
 *	On failure, non-zero is returned and indicates the appropriate
 *	errno.
 */
STATIC int
segfga_dup(struct seg *pseg, struct seg *cseg)
{
	struct segfga_data *psfdp = pseg->s_data;
	struct segfga_data *csfdp;

	csfdp = kmem_alloc(sizeof(struct segfga_data), KM_SLEEP);
	*csfdp = *psfdp;

	shpt_hat_attach(cseg->s_as, cseg->s_base, cseg->s_size,
			  csfdp->sfd_hatshptp, mycg);

	cseg->s_ops = &segfga_ops;
	cseg->s_data = csfdp;
	cseg->s_as->a_isize += pseg->s_size;

	return 0;
}

/*
 * STATIC int
 * segfga_unmap(struct seg *seg, vaddr_t addr, size_t len)
 *	Unmap a portion (possibly all) of the specified segment.
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusively locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 *
 *	On success, 0 is returned and the request chunk of the address
 *	space has been deleted. On failure, non-zero is returned and
 *	indicates the appropriate errno.
 *
 * Remarks:
 *
 *	segfga currently does not support a partial unmap, and any such
 *	attempt will return an error.
 */
STATIC int
segfga_unmap(struct seg *seg, vaddr_t addr, size_t len)
{
	struct segfga_data *sfdp = seg->s_data;

	/*
	 * Check for bad sizes.
	 * In other segment drivers this is a true if statement - does
	 * anyone know why!?
	 */
	ASSERT( !((addr < seg->s_base) ||
		 ((addr + len) > (seg->s_base + seg->s_size)) ||
		 (len & PAGEOFFSET) || (addr & PAGEOFFSET)) ) ;

	/*
	 * If not entire segment, then fail.
	 */
	if ((addr != seg->s_base) || (len != seg->s_size)) {
		return EINVAL;
	}

	/*
	 * Detach the shared page tables and free the segment.
	 */
	shpt_hat_detach(seg->s_as, seg->s_base, seg->s_size, sfdp->sfd_hatshptp);

	seg->s_as->a_isize -= seg->s_size;
	seg_free(seg);

	return 0;
}

/*
 * STATIC void
 * segfga_free(struct seg *seg)
 *	Free a segment.
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusivley locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 */
STATIC void
segfga_free(struct seg *seg)
{
	struct segfga_data *sfdp = seg->s_data;

	kmem_free(sfdp, sizeof(*sfdp));
}

/*
 * STATIC faultcode_t
 * segfga_fault(struct seg *seg, vaddr_t addr, size_t len,
 *		enum fault_type type, enum seg_rw rw)
 *	Fault handler; called for both hardware faults and softlock requests.
 *
 * Calling/Exit State:
 *	Called with the AS lock held (in read mode) and returns the same
 *	(except when called from aio_done(), in which case the as lock is
 *	not held and the only valid type is F_SOFTUNLOCK).
 *
 *	Addr and len arguments have been properly aligned and rounded
 *	with respect to page boundaries by the caller (this is true of
 *	all SOP interfaces).
 *
 *	On success, 0 is returned and the requested fault processing has
 *	taken place. On error, non-zero is returned in the form of a
 *	fault error code.
 *
 * Remarks:
 *	Because segfga makes use of shared page tables, all pages are always
 *	mapped RW, thus, protections never need to be checked, and a F_PROT
 *	fault should never occur.
 */
/*ARGSUSED*/
STATIC faultcode_t
segfga_fault(struct seg *seg, vaddr_t addr, size_t len, enum fault_type type,
	     enum seg_rw rw)
{
	struct segfga_data *	sfdp = seg->s_data;
	fgavpg_t *		vpgp;
	uint_t			seg_pg_offset;
	uint_t			num_pages;
	int			is_softlock;

	ASSERT((addr >= seg->s_base) && (addr < (seg->s_base + seg->s_size)));
	ASSERT((addr + len) <= (seg->s_base + seg->s_size));

	num_pages = btop(len);

	ASSERT(num_pages != 0);
	ASSERT((getpl() == PLBASE) || (type == F_SOFTUNLOCK));

	seg_pg_offset = btop(addr - seg->s_base);

	if (type == F_SOFTUNLOCK) {
	    for ( ; num_pages != 0; num_pages--, seg_pg_offset++) {

			vpgp = &sfdp->sfd_vpgpp[fshm_idx1(seg_pg_offset)]
						    [fshm_idx2(seg_pg_offset)];

			/*
			 * Note that a F_SOFTUNLOCK will decrement the softlock
			 * count of a fga shm page without the VPG lock.  This
			 * is safe because the existance of the SOFTLOCK
			 * guarantees the existance of the page (and the
			 * segment can't be remapped while a softlock is
			 * outstanding).  The only operations which are valid
			 * on the page while a softlock is outstanding is
			 * another SOFTLOCK or SOFTUNLOCK request (which is
			 * safe, since both requests will use atomic operations
			 * when manipulating the softlock count) and setting
			 * the FVP_DEL_MIG_FLAG (i.e.  via a reaffine) which
			 * won't conflict with the F_SOFTUNLOCK, since the
			 * latter won't hold the VPG lock and thus won't
			 * manipulate the page flags.
			 */
			ASSERT(vpgp->fvp_softlock_cnt > 0);
			atomic_int_decr((volatile int *) &vpgp->fvp_softlock_cnt);
		}

		return(0);
	}

	is_softlock = ((type == F_SOFTLOCK) || (type == F_MAXPROT_SOFTLOCK));
	for ( ; num_pages != 0; num_pages--, seg_pg_offset++) {

		vpgp = &sfdp->sfd_vpgpp[fshm_idx1(seg_pg_offset)]
						[fshm_idx2(seg_pg_offset)];

		FGASHM_LOCK_VPG(sfdp->sfd_vpglckp, sfdp->sfd_vpgsvp, vpgp);

		ASSERT ((type == F_SOFTLOCK) || (type == F_MAXPROT_SOFTLOCK) ||
			(type == F_INVAL));

		/*
		 * If the softlock count is 0, we may have to provoke a
		 * migration (if a migrate request came in while the
		 * page was locked into a specific frame - due to I/O
		 * or something).  Invalidate the translation and flush
		 * ALL engine's TLBs (would be sufficient to flush TLBs
		 * only on engines currently running a process attached
		 * to this same set of shared page tables, but no
		 * interface exists to do that).
		 */
		if ((vpgp->fvp_softlock_cnt == 0) &&
				(vpgp->fvp_flag & FVP_DEL_MIGRATE_FLAG)) {
			vpgp->fvp_flag &= ~FVP_DEL_MIGRATE_FLAG;
			vpgp->fvp_flag |= FVP_MIGRATE_FLAG;
			shpt_hat_unload(NULL,
					sfdp->sfd_hatshptp,
					ptob(seg_pg_offset));

			hat_shootdown((TLBScookie_t) 0, HAT_NOCOOKIE);
		}

		fgashm_fault(vpgp, ptob(seg_pg_offset), sfdp);

		ASSERT(vpgp->fvp_ppid != VPG_NONEXISTENT);
		ASSERT(shpt_hat_xlat_info(seg->s_as,
			sfdp->sfd_hatshptp,
			seg->s_base + ptob(seg_pg_offset)) &
					    HAT_XLAT_EXISTS);

		/*
		 * Handle a softlock request.  It is possible, though unlikely,
		 * that the softlock count can overflow.  Handle it as
		 * gracefully as possible.
		 */
		if (is_softlock) {
			if (vpgp->fvp_softlock_cnt == VPG_MAX_SOFTLOCK_COUNT) {
				if (seg_pg_offset != btop(addr - seg->s_base)) {
					segfga_fault(seg, addr,
						     ptob(seg_pg_offset) - addr,
						     F_SOFTUNLOCK, S_OTHER);
				}
				FGASHM_UNLOCK_VPG(sfdp->sfd_vpglckp,
						  sfdp->sfd_vpgsvp, vpgp);
				return(FC_MAKE_ERR(EAGAIN));
			}

			atomic_int_incr((volatile int *) &vpgp->fvp_softlock_cnt);
		}

		FGASHM_UNLOCK_VPG(sfdp->sfd_vpglckp, sfdp->sfd_vpgsvp, vpgp);
	} /* for */

	ASSERT((getpl() == PLBASE) || (type == F_SOFTUNLOCK));
	return(0);
}

/*
 * STATIC int
 * segfga_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
 *	Change the protections on a range of pages in the segment.
 *
 * Calling/Exit State:
 *	Called and exits with the address space exclusively locked.
 *
 *	Returns zero on success, returns a non-zero errno on failure.
 *
 * Remarks:
 *
 *	Segfga segments are always mapped RW and cannot be changed.
 */
STATIC int
segfga_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
{

	if (prot == (PROT_USER | PROT_READ | PROT_WRITE)) {
	    return(0);
	} else {
	    return(EACCES);	/* ?????? */
	}

}

/*
 * STATIC int
 * segfga_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
 *	Determine that the vpage protection for addr  
 *	is at least equal to prot.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	On success, 0 is returned, indicating that the addr
 *	allow accesses indicated by the specified protection.
 *	Actual protection may be greater.
 *	On failure, EACCES is returned, to indicate that 
 *	the page does not allow the desired access.
 *
 * Remarks:
 *
 *	Segfga segments are always mapped RW and cannot be changed.
 */
STATIC int
segfga_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
{

	if ((prot & (PROT_USER | PROT_READ | PROT_WRITE)) == prot) {
	    return(0);
	} else {
	    return(EACCES);
	}

}

/*
 * STATIC int
 * segfga_lockop(struct seg *seg, vaddr_t addr, uint_t len, int attr, int op)
 *	Lock the pages from addr through addr + len into memory.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 */
STATIC
int segfga_lockop(struct seg *seg, vaddr_t addr, uint_t len, int attr, int op)
{
	if (attr != 0) {
		if (!(attr & SHARED)) {
			return(0);
		}

		if ((attr & (PROT_READ | PROT_WRITE)) !=
						(PROT_READ | PROT_WRITE)) {
			return(0);
		}
	}

	/*
	 * To lock the pages into memory, we first SOFTLOCK the whole range
	 * to make sure that they have all been faulted in.  We then
	 * SOFTUNLOCK them - since fga shm is never paged, the pages will stay
	 * memory resident (unless a migrate request via shmctl() is made).
	 *
 	 * Unlocking fga shm is easy - since it is never paged to begin
	 * with, this is a nop.
	 */
	switch (op) {
		case MC_LOCK:
		case MC_LOCKAS:
			segfga_fault(seg, addr, len, F_SOFTLOCK, S_OTHER);
			segfga_fault(seg, addr, len, F_SOFTUNLOCK, S_OTHER);
			break;

		case MC_UNLOCK:
		case MC_UNLOCKAS:
			break;

		default:
			ASSERT(0);
			break;
	} /* switch */

return(0);
}


/*
 * STATIC int
 * segfga_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
 *	Return the protections on the page at addr.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	This function, which cannot fail, returns the permissions of the
 *	indicated pages in the protv array.
 */
STATIC int
segfga_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
{
/*
 * Q: Should there be a PROT_USER here also???
 */
	*protv = (PROT_READ | PROT_WRITE);
	return (0);
}


/*
 * STATIC off64_t
 * segfga_getoffset(struct seg *seg, vaddr_t addr)
 *	Return the fga shm offset mapped at the given address within
 *	the segment.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
STATIC off64_t
segfga_getoffset(struct seg *seg, vaddr_t addr)
{
	return (addr - seg->s_base);
}

/*
 * STATIC int
 * segfga_gettype(struct seg *seg, vaddr_t addr)
 *	Return the segment type (MAP_SHARED||MAP_PRIVATE) to the caller.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC int
segfga_gettype(struct seg *seg, vaddr_t addr)
{
	return(MAP_SHARED);
}

/*
 * STATIC int
 * segfga_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
 *	Return the vnode associated with the segment.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC int
segfga_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
{
	*vpp = NULL;
	return(-1);
}

/*
 * STATIC int
 * segfga_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
 *	Return an indication, in the array, vec, of whether each page
 *	in the given range is "in core".
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *
 *	Set the bit if the page has been instantiated in core, leave
 *	it clear if the page has never been resident.  If that page has
 *	been instantiated but the translation is currently invalid due
 *	to an outstanding migrate request, report the page as incore.
 */
/*ARGSUSED*/
STATIC int
segfga_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
{
	size_t 			offset;
	size_t			seg_pg_offset;
	fgavpg_t *	vpgp;
	struct segfga_data *	sfdp = seg->s_data;

	seg_pg_offset = btop(addr - seg->s_base);
	
	for (offset = 0; offset < len; offset += PAGESIZE) {

		vpgp = &sfdp->sfd_vpgpp[fshm_idx1(seg_pg_offset)]
						[fshm_idx2(seg_pg_offset)];

		/*
		 * We are examining the fvp_ppid field without the VPG lock,
		 * but it is benign in this case.  The information returned
		 * by this function is stale in any event.
		 */
		if (vpgp->fvp_ppid != VPG_NONEXISTENT) {
			*vec = 1;
		} else {
			*vec = 0;
		}

		seg_pg_offset++;
		vec++;
	}
	return ((int) len);
}


/*
 * STATIC void
 * segfga_age(struct seg *, u_int type)
 *	Age the translations for a segfga segment.
 *
 * Calling/Exit State:
 *	The process owning the AS which owns the argument segment has
 *	been seized.
 *
 *	This function does not block.
 *
 * Remarks:
 *	This is a no-op, but must be present in order to prevent
 *	hat_agerange from aging this segment.
 */
/*ARGSUSED*/
STATIC void
segfga_age(struct seg * seg, u_int type)
{

}

/*
 * STATIC int
 * segfga_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
 *	This is a no-op for segfga.
 *
 * Calling/Exit State:
 *	returns ENOMEM.
 */
/*ARGSUSED*/
STATIC int
segfga_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
{
	return(ENOMEM);
}
#endif /* PAE_MODE */

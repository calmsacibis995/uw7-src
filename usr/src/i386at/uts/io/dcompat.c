#ident	"@(#)kern-i386at:io/dcompat.c	1.21.7.1"
#ident	"$Header$"

/*
 * Non-DDI driver and older DDI driver compatibility.
 *
 * These routines provide support for binary drivers from previous releases
 * which were almost, but not quite, DDI conforming.  Such drivers were
 * marked as DDI conforming (or, rather, as not D_OLD, which isn't exactly
 * the same thing), but used some interfaces which were not actually part
 * of the DDI spec.
 *
 * We do not attempt to provide support for arbitrary non-conforming drivers,
 * as it would be impossible to determine which interfaces were necessary.
 * In particular, we do not provide support for access to kernel global
 * variables, such as "u".
 *
 * To prevent these old interfaces from accidentally being used in new code,
 * we prefix their names with "_Compat_".  At kernel build time, the kernel
 * configuration tool will detect old drivers using these interfaces and map
 * the original names to the "_Compat_" names.  For example, references to
 * "sptalloc" will be changed to references to "_Compat_sptalloc".
 *
 * Old interfaces which are just aliases for current routines are directly
 * mapped to those routines, rather than creating "_Compat_" routines here.
 */

#include <fs/buf.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg_kmem.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <net/inet/if.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/cg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

/*
 * ifstats -- list of NIC statistics structures
 * Ensure that drivers can call ifstats_attach()/ifstats_detach()
 * with locks held and not encounter hierarchy violations.
 */
#define IFSTATS_RWLCK_HIER	KERNEL_HIER_BASE
STATIC LKINFO_DECL(ifstats_lkinfo, "ifstats_rwlck", 0);
rwlock_t *ifstats_rwlck;
STATIC rwlock_t	ifstats_rwlck_shadow;

ifstats_t *ifstats;

/*
 * void
 * dcompat_init(void)
 *	Initialize compatibility support.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
dcompat_init(void)
{
	RW_INIT(&ifstats_rwlck_shadow, IFSTATS_RWLCK_HIER, PLSTR,
		&ifstats_lkinfo, KM_NOSLEEP);
	ifstats_rwlck = &ifstats_rwlck_shadow;
}

/*
 * int
 * _Compat_spl1(void)
 *	Set interrupt priority level to PL1 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl1(void)
{
	pl_t pl = getpl();
	splx(PL1);
	return pl;
}

/*
 * int
 * _Compat_spl4(void)
 *	Set interrupt priority level to PL4 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl4(void)
{
	pl_t pl = getpl();
	splx(PL4);
	return pl;
}

/*
 * int
 * _Compat_spl5(void)
 *	Set interrupt priority level to PL5 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl5(void)
{
	pl_t pl = getpl();
	splx(PL5);
	return pl;
}

/*
 * int
 * _Compat_spl6(void)
 *	Set interrupt priority level to PL6 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl6(void)
{
	pl_t pl = getpl();
	splx(PL6);
	return pl;
}

/*
 * paddr_t
 * _Compat_kvtophys(vaddr_t vaddr)
 *
 * Calling/Exit State:
 *	Return the physical address equivalent of the virtual address
 *	argument.
 *
 * Remarks:
 *	Argument was of type caddr_t, but is now vaddr_t for consistency
 *	with the macro form used by the kernel; it doesn't matter for drivers
 *	since only old binaries should be using this function.
 */
paddr_t
_Compat_kvtophys(vaddr_t vaddr)
{
#ifdef PAE_MODE
	if (PAE_ENABLED())
		return _KVTOPHYS64(vaddr);
	else
#endif /* PAE_MODE */
		return _KVTOPHYS(vaddr);
}

/*
 * uint_t
 * _Compat_hat_getkpfnum(caddr_t vaddr)
 *
 * Calling/Exit State:
 *	Return the physical page ID equivalent to the kernel virtual
 *	address argument.
 */
uint_t
_Compat_hat_getkpfnum(caddr_t vaddr)
{
	return (uint_t)kvtoppid(vaddr);
}

/*
 * uint_t
 * _Compat_hat_getppfnum(caddr_t paddr, u_int pspace)
 *
 * Calling/Exit State:
 *	Return the physical page ID equivalent to the physical address
 *	argument.
 */
uint_t
_Compat_hat_getppfnum(caddr_t paddr, u_int pspace)
{
	if (pspace != PSPACE_MAINSTORE)
		return NOPAGE;

	return (uint_t)phystoppid((paddr_t)paddr);
}

/*
 * queue_t *
 * _Compat_backq(q)
 *	Return the queue upstream from this one.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */
queue_t *
_Compat_backq(queue_t *q)
{
	pl_t pl;
	queue_t *retq;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	retq = backq_l(q);
	UNLOCK(q->q_str->sd_mutex, pl);
	return retq;
}

/*
 * int
 * _Compat_useracc(caddr_t addr, uint_t count, int access)
 *	Determine if a user address range is accessible
 *
 * Calling/Exit State:
 *	Return 1 if the specified range is accessible.
 *	Otherwise return 0.
 *
 * Remarks:
 *	Note that in a multi-LWP process, the answer we give is
 *	potentially stale.
 */
int
_Compat_useracc(caddr_t addr, uint_t count, int access)
{
	uint_t prot;
	struct as *as = u.u_procp->p_as;
	int err;

	ASSERT(as != NULL);

	prot = PROT_USER | ((access == B_READ) ? PROT_READ : PROT_WRITE);
	as_rdlock(as);
	err = as_checkprot(as, (vaddr_t)addr, count, prot);
	as_unlock(as);
	return (err == 0);
}

/*
 * void
 * _Compat_dma_pageio(void (*strat)(), buf_t *bp)
 *	Break up a B_PHYS request at page boundaries (for DMA).
 *
 * Calling/Exit State:
 *	See buf_breakup().
 */
void
_Compat_dma_pageio(void (*strat)(), buf_t *bp)
{
	static physreq_t dma_preq = {
		/* phys_align */	1,
		/* phys_boundary */	PAGESIZE,
		/* phys_dmasize */	0,
		/* phys_max_scgth */	0
	};
	static const bcb_t dma_bcb = {
		/* bcb_addrtypes */	BA_KVIRT|BA_UVIRT|BA_PAGELIST,
		/* bcb_flags */		BCB_PHYSCONTIG,
		/* bcb_max_xfer */	PAGESIZE,
		/* bcb_granularity */	NBPSCTR,
		/* bcb_physreqp */	&dma_preq
	};
	(void)physreq_prep(&dma_preq, KM_SLEEP);
	buf_breakup(strat, bp, &dma_bcb);
}

/*
 * void
 * _Compat_rdma_filter(void (*strat)(), buf_t *bp)
 *	Make sure a buffer is DMAable.
 *
 * Calling/Exit State:
 *	See buf_breakup().
 */
void
_Compat_rdma_filter(void (*strat)(), buf_t *bp)
{
#ifndef NO_RDMA
	if (rdma_mode != RDMA_DISABLED)
		buf_breakup(strat, bp, &rdma_dflt_bcb);
	else
#endif
		(*strat)(bp);
}

#if defined (_KMEM_STATS) || defined(_KMEM_HIST) || defined(MINI_KERNEL)
/*
 * Kernels built with _KMEM_STATS and/or _KMEM_HIST turn
 * kmem_alloc, kmem_zalloc, kmem_alloc_physcontig, and
 * kmem_alloc_physcreq into macros.  Such macros are unwelcome here.
 */
#undef kmem_alloc
#undef kmem_zalloc
#undef kmem_alloc_physcontig
#undef kmem_alloc_physreq
extern void *kmem_alloc(size_t, int);
extern void *kmem_zalloc(size_t, int);
extern void *kmem_alloc_physcontig(size_t, const physreq_t *, int);
extern void *kmem_alloc_physreq(size_t, const physreq_t *, int);

/*
 * void *
 * kmem_i_alloc_physcont(size_t size, physreq_t *physreq, int flags,
 *			 int line, char *file)
 *	Allocate physically-contiguous memory.
 *
 * Calling/Exit State:
 *	Instrumented older version of kmem_alloc_physreq() which always
 *	returns physically contiguous memory. This is now a ``level 2''
 *	interface. New code should be using kmem_alloc_physreq() in
 *	preference to this interface.
 *
 *	The only valid flags are KM_SLEEP and KM_NOSLEEP.
 */
void *
kmem_i_alloc_physcont(size_t size, const physreq_t *physreq, int flags,
		      int line, char *file)
{
	ASSERT(!(flags & ~KM_NOSLEEP));

	return kmem_i_alloc_physreq(size, physreq, flags | KM_PHYSCONTIG,
				    line, file);
}

/*
 * void *
 * _Compat_kmem_instr_alloc(size_t size, int flags, int line, char *file)
 *	Allocate virtual and physical memory of arbitrary size, plus
 *	gather statistics and/or history.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_instr_alloc(size_t size, int flags, int line, char *file)
{
	return kmem_instr_alloc(size, flags | KM_REQ_DMA, line, file);
}

/*
 * void *
 * _Compat_kmem_instr_zalloc(size_t size, int flags, int line, char *file)
 *	Allocate virtual and physical memory of arbitrary size, and
 *	return it zeroed-out. Also gather statistics and/or history.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_instr_zalloc(size_t size, int flags, int line, char *file)
{
	return kmem_instr_zalloc(size, flags | KM_REQ_DMA, line, file);
}

/*
 * void *
 * _Compat_kmem_i_alloc_physcont(size_t size, physreq_t *physreq,
 *				       int flags, int line, char *file))
 *	Allocate physically-contiguous memory. Also gather statistics and/or
 *	history.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_i_alloc_physcont(size_t size, physreq_t *physreq, int flags,
				    int line, char *file)
{
	return kmem_i_alloc_physreq(size, physreq,
				    flags | KM_REQ_DMA | KM_PHYSCONTIG,
				    line, file);
}

#endif /* _KMEM_STATS || _KMEM_HIST || MINI_KERNEL */

/*
 * void *
 * _Compat_kmem_alloc5(size_t size, int flags)
 *	Allocate virtual and physical memory of arbitrary size.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_alloc5(size_t size, int flags)
{
	return kmem_alloc(size, flags | KM_REQ_DMA);
}

/*
 * void *
 * _Compat_kmem_zalloc5(size_t size, int flags)
 *	Allocate virtual and physical memory of arbitrary size, and
 *	return it zeroed-out.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_zalloc5(size_t size, int flags)
{
	return kmem_zalloc(size, flags | KM_REQ_DMA);
}

/*
 * void *
 * _Compat_kmem_alloc_physcontig(size_t size, const physreq_t *physreq,
 *				 int flags)
 *	Allocate physically-contiguous memory.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_alloc_physcontig(size_t size, const physreq_t *physreq, int flags)
{
	return kmem_alloc_physreq(size, physreq,
				  flags | KM_REQ_DMA | KM_PHYSCONTIG);
}

/*
 * mblk_t *
 * _Compat_allocb(int size, uint_t pri)
 *	Allocate a new message block.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
mblk_t *
_Compat_allocb(int size, uint_t pri)
{
	return allocb_physreq(size, pri, strphysreq);
}

/*
 * mblk_t *
 * _Compat_copyb(mblk_t *bp)
 *	Copy data from message block to newly allocated message block and
 *	data block.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	Returns new message block pointer, or NULL if error.  No locking
 *	assumptions.
 */
mblk_t *
_Compat_copyb(mblk_t *bp)
{
	return copyb_physreq(bp, strphysreq);
}

/*
 * mblk_t *
 * _Compat_copymsg(mblk_t *bp)
 *	Copy data from message to newly allocated message using new
 *	data blocks.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	Returns a pointer to the new message, or NULL if error.  No locking
 *	assumptions.
 */
mblk_t *
_Compat_copymsg(mblk_t *bp)
{
	return copymsg_physreq(bp, strphysreq);
}

/*
 * mblk_t *
 * _Compat_msgpullup(mblk_t *bp, int len)
 *      Concatenate and align first len bytes of common message type.
 *      len == -1, means concat everything.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
mblk_t *
_Compat_msgpullup(mblk_t *bp, int len)
{
	return msgpullup_physreq(bp, len, strphysreq);
}

/*
 * void
 * _Compat_ifstats_attach(struct ifstats *ifsp)
 *	Add a module/driver's interface statistics structure (struct ifstats)
 *	to the global list of such structures.  Drivers/modules will access
 *	their entry in the list directly, without needing to acquire the lock
 *	first.  The lock is only necessary in order to add/remove elements
 *	from the list and to traverse the list (which IP does to collect the
 *	statistics for the user).
 *
 * Calling/Exit State:
 *	None.
 */
void
_Compat_ifstats_attach(struct ifstats *ifsp)
{
	pl_t	pl;

	pl = RW_WRLOCK(ifstats_rwlck, plstr);
	ifsp->ifs_next = ifstats;
	ifstats = ifsp;
	RW_UNLOCK(ifstats_rwlck, pl);
}

/*
 * struct ifstats *
 * _Compat_ifstats_detach(struct ifstats *ifsp)
 *	Remove a module/driver's interface statistics structure
 *	(struct ifstats) from the global list of such structures.
 *	Returns the address of the structure (if found) or NULL (if not)
 *	to allow the caller to take action (if appropriate) if its
 *	statistics structure wasn't on the list.
 *
 * Calling/Exit State:
 *	None.
 */
struct ifstats *
_Compat_ifstats_detach(struct ifstats *ifsp)
{
	struct ifstats	*tifsp;
	struct ifstats	**lastifspp;
	pl_t	pl;

	pl = RW_WRLOCK(ifstats_rwlck, plstr);
	lastifspp = &ifstats;
	for (tifsp = ifstats; tifsp != NULL; tifsp = tifsp->ifs_next) {
		if (tifsp == ifsp) {
			*lastifspp = tifsp->ifs_next;
			break;
		}
		lastifspp = &tifsp->ifs_next;
	}
	RW_UNLOCK(ifstats_rwlck, pl);
	return tifsp;
}

/* TEMPORARY! (Until i8237A.c stops calling it.) */
int
drv_gethardware(ulong_t parm, void *valuep)
{
	return _Compat_drv_gethardware(parm, valuep);
}

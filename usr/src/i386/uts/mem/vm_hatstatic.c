#ident	"@(#)kern-i386:mem/vm_hatstatic.c	1.34.8.4"
#ident	"$Header$"

#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/seg_kmem.h>
#include <mem/vm_mdep.h>
#include <mem/page.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/cg.h>
#include <svc/cpu.h>
#include <svc/creg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>

/*
 * This file contains support for all static kernel virtual memory
 * allocation needed during system initialization (boot).
 *
 * Most of these functions operate only during the window of time
 * after virtual paging is enabled but before the general page pool
 * is initialized.  A few continue to operate beyond this time
 * to allow callers to load and unload ptes from static page tables
 * previously allocated.
 */
boolean_t hat_static_callocup;	/* B_TRUE => calloc() enabled */
STATIC boolean_t hat_static_pallocup;	/* B_TRUE => palloc() enabled */

STATIC uint_t  curphystype;
STATIC vaddr_t nextvaddr;

vaddr_t calloc_end;       /* For ASSERT_PP_IN_RANGE */

STATIC void   *calloc_typed(ulong_t, uint_t);
STATIC void    loadpte(vaddr_t, paddr_t, uint_t);
STATIC void    pae_alloc_l2pt(vaddr_t);
STATIC paddr64_t palloc(uint_t, uint_t, uint_t *);
extern paddr64_t getnextpaddr(uint_t, uint_t);
extern void    pae_hat_statpt_memload(vaddr_t, ulong_t, struct page *, uint_t);
extern void    pae_hat_statpt_devload(vaddr_t, ulong_t, ppid_t, uint_t);
extern void    pae_hat_statpt_unload(vaddr_t, ulong_t);
extern void    idf_palloc_init(void);

void alloc_l2pt(vaddr_t);

#ifdef CCNUMA
/*
 * void
 * hat_static_startcgpalloc(void)
 *
 * Enable local physical allocation before variables controlling
 * hat_static_init() are visible in a CG's virtual map.
 */
void
hat_static_startcgpalloc(void)
{
	cg.hat_static_cg_pallocup = B_TRUE;
}

/*
 * void
 * hat_static_stopcgpalloc(void)
 *
 * Disable explicit local physical allocation; called right after varibales 
 * controlling hat_static_init() are visible in a CG's virtual map.
 */
void
hat_static_stopcgpalloc(void)
{
	cg.hat_static_cg_pallocup = B_FALSE;
}
#endif /* CCNUMA */

/*
 * void
 * hat_static_init(vaddr_t vaddr)
 *
 *	Enable the system initialization (boot) time static memory allocator.
 *
 * Calling/Exit State:
 *
 *	vaddr is the next available kernel virtual address to allocate.
 *
 *	Ownership of all remaining kernel virtual space and physical
 *	memory is passed to this allocator until hat_static_stopcalloc()
 *	and hat_static_stoppalloc() are called, respectively.
 */
void
hat_static_init(vaddr_t vaddr)
{
	/*
	 * Rounding up to a page boundary eliminates the need
	 * to initialize curphystype.
	 */
	nextvaddr = roundup(vaddr, MMU_PAGESIZE);

	hat_static_callocup = B_TRUE;	/* calloc() enabled */
	hat_static_pallocup = B_TRUE;	/* palloc() enabled */

	if (mycg == BOOTCG) {
		hat_vprotinit();		/* init. the vprot array */
	}
}


/*
 * void
 * hat_static_stopcalloc(void)
 *
 *	Disable the calloc() function.
 *
 * Calling/Exit State:
 *
 *	Ownership of all remaining kernel virtual space is
 *	relinquished to the caller.  The caller should call
 *	hat_static_nextvaddr() to determine the remainder
 *	immediately BEFORE calling this routine.
 *
 * Remarks:
 *
 *	For NUMA simulation, we take a snapshot of the boot pm's
 *	translation map; this is made available to other pm's as they
 *	come up. At the time of this snapshot, all page tables needed to
 *	map statically allocated kernel data structues have been allocated.
 *	No page tables have been allocated yet for dynamic kernel segments. 
 *	Second level page tables to map all second level page tables on 
 *	all CG's have been allocated; as we keep allocating level 2 page
 *	tables later for dynamic segments, the page table mapping page table 
 *	for this CG will be updated correspondingly.
 *
 */
void
hat_static_stopcalloc(void)
{
	ASSERT(hat_static_callocup);
	calloc_end = nextvaddr;
	hat_static_callocup = B_FALSE;		/* calloc() disabled */
}

/*
 * void
 * hat_static_stoppalloc(void)
 *
 *	Disable the palloc() function.
 *
 * Calling/Exit State:
 *
 *	Ownership of all remaining kernel physical memory is
 *	relinquished to the caller.
 *
 * Note:
 *	Do not set the global bit for per-engine pages because the TLB
 *	could have stale entries if a process has a private L1.
 */
void
hat_static_stoppalloc(void)
{
	if (hat_static_callocup) {
		/*
		 *+ An attempt was made to disable the kernel static
		 *+ physical memory allocator before the kernel
		 *+ static virtual memory allocator was disabled.
		 *+ This indicates a kernel software problem.
		 *+ Corrective action:  None.
		 */
		cmn_err(CE_PANIC, "hat_static_stoppalloc: calloc enabled");
	}
	ASSERT(hat_static_pallocup);
	hat_static_pallocup = B_FALSE;	/* palloc() disabled */

	/*
	 * Now that we've established all of the permanent mappings and haven't
	 * yet made any transient mappings, it's time to turn on page-global
	 * bits, if we can.  This will keep global mappings in the TLB during
	 * flushes.
	 */
	if (PGE_ENABLED()) {
#ifdef PAE_MODE
		if (PAE_ENABLED()) {
			pte64_t *kl1pt;
			pte64_t *ptep, *kl1ptep;
			vaddr_t addr;
			uint_t n;

			kl1pt = engine[myengnum].e_local_pae->pp_kl1pt[3];
			kl1ptep = &kl1pt[pae_ptnum(addr = kvbase)];
			do {
				if (KADDR_PER_ENG(addr))
					continue;
				if (!PG_ISVALID(kl1ptep))
					continue;
				if (PG_ISPSE(kl1ptep)) {
					kl1ptep->pgm.pg_g = 1;
					continue;
				}
				ptep = kvtol2ptep64(addr);
				for (n = PAE_NPGPT; n-- != 0; ++ptep) {
					if (PG_ISVALID(ptep))
						ptep->pgm.pg_g = 1;
				}
			} while ((addr += PAE_VPTSIZE), (++kl1ptep != &kl1pt[PAE_NPGPT]));
		} else
#endif /* PAE_MODE */
		{
			pte_t *kl1pt;
			pte_t *ptep, *kl1ptep;
			vaddr_t addr;
			uint_t n;

			kl1pt = engine[myengnum].e_local->pp_kl1pt[0];
			kl1ptep = &kl1pt[ptnum(addr = kvbase)];
			do {
				if (KADDR_PER_ENG(addr))
					continue;
				if (!PG_ISVALID(kl1ptep))
					continue;
				if (PG_ISPSE(kl1ptep)) {
					kl1ptep->pgm.pg_g = 1;
					continue;
				}
				ptep = kvtol2ptep(addr);
				for (n = NPGPT; n-- != 0; ++ptep) {
					if (PG_ISVALID(ptep))
						ptep->pgm.pg_g = 1;
				}
			} while ((addr += VPTSIZE), (++kl1ptep != &kl1pt[NPGPT]));
		}
	}
}

/*
 * vaddr_t
 * hat_static_nextvaddr(void)
 *
 * Calling/Exit State:
 *
 *	Retrieve the next available kernel virtual address from the 
 *	static memory allocator.
 */
vaddr_t
hat_static_nextvaddr(void)
{
	/*
	 * The following test is invalid in the CCNUMA kernel,
	 * since sysinit_sync() continues to call us even after
	 * calloc is disabled.
	 */
	if (!hat_static_callocup) {
		/*
		 *+ An attempt was made to interrogate the kernel
		 *+ static memory allocator before it was initialized
		 *+ or after it was disabled.  This indicates
		 *+ a kernel software problem.
		 *+ Corrective action:  None.
		 */
		cmn_err(CE_PANIC, "hat_static_nextvaddr: called while disabled");
	}
	return(nextvaddr);
}

/*
 * STATIC paddr64_t
 * palloc(uint_t size, uint_t flag, uint_t *phystypep);
 *
 *	Return the physical address of newly allocated, page aligned,
 *	physical memory.
 *
 *	Note:  The memory is *not* zeroed; it contains garbage.
 *
 * Calling/Exit State:
 *
 *	size specifies the number of bytes to allocate.
 *
 *	flag may be either:
 *		PMEM_PHYSIO
 *		      Caller requires physical pages meeting all I/O
 *		      constraints such as DMA-ability for all devices.
 *		PMEM_ANY
 *		      Caller accepts any physical pages (but implies
 *		      a preference for physical pages that fail the
 *		      conditions required by PMEM_PHYSIO).
 *
 *	Return the physical address of the newly allocated memory.
 *	Note:  The memory is *not* zeroed; it contains garbage
 *
 *	On return, *phystypep contains the "flag" value (PMEM_PHYSIO or
 *	PMEM_ANY) appropriate to the returned physical page.  This will
 *	always be PMEM_PHYSIO if the caller specified flag = PMEM_PHYSIO,
 *	but may be either PMEM_PHYSIO or PMEM_ANY if the caller specified
 *	PMEM_ANY.  This is useful if the caller needs only a portion of the
 *	page now, but may want to use the rest later for a different purpose.
 *	(If PMEM_ANY is returned, it indicates that the page is not suitable
 *	for PMEM_PHYSIO.)
 *
 * Description:
 *
 *	Called only during system initialization when only one processor
 *	is active, so no mutexing required.
 */
STATIC paddr64_t
palloc(uint_t size, uint_t flag, uint_t *phystypep)
{
	if (!hat_static_pallocup && !cg.hat_static_cg_pallocup) {
		/* All our callers should already check this, but... */

		/*
		 *+ An attempt was made to allocate kernel memory
		 *+ via a memory allocator which is only functioning
		 *+ during kernel initialization (boot) and the allocation
		 *+ request occurred either too early, or too late,
		 *+ in the kernel initialization sequence.  This indicates
		 *+ a kernel software problem.
		 *+ Corrective action:  None.
		 */
		cmn_err(CE_PANIC, "palloc: called while disabled");
	}

	/*
	 * Since getnextpaddr must succeed, we can set the allocated type now.
	 */
	*phystypep = flag;

	return getnextpaddr(size, flag);
}


/*
 * void *
 * calloc(ulong_t size)
 *
 *	Allocate zeroed memory at boot time.
 *
 * Calling/Exit State:
 *
 *	Returns the virtual address of a zeroed chunk of memory
 *	at least "size" bytes long.  The returned address is 
 *	aligned to at least a sizeof(long) boundary.  Also rounds
 *	up "size" to a sizeof(long) multiple.
 *
 *	Use callocrnd() to cause next calloc() allocation to occur
 *	on a given boundary.
 *
 *	Use calloc_physio() to get physical memory suitable for
 *	all forms of physical I/O.
 */
void *
calloc(ulong_t size)
{
	return(calloc_typed(size, PMEM_ANY));
}


/*
 * void *
 * calloc_physio(ulong_t size)
 *
 *	Allocate zeroed memory at boot time using physical memory
 *	which is suitable for all forms of physical I/O.
 *
 * Calling/Exit State:
 *
 *	Just like calloc() except that the caller is guaranteed
 *	the physical memory used is suitable for all forms of
 *	physical I/O.  (E.g., some DMA controllers cannot address
 *	all of physical memory.)
 *
 *	Callers should use calloc() if possible.
 */
void *
calloc_physio(ulong_t size)
{
	return(calloc_typed(size, PMEM_PHYSIO));
}


/*
 * void
 * callocrnd(ulong_t bound)
 *
 * Calling/Exit State:
 *
 *	Cause the next calloc()/calloc_physio() to return an address
 *	aligned on a "bound" boundary.
 */
void
callocrnd(ulong_t bound)
{
	ASSERT(bound != 0);
	ASSERT((bound & (bound-1)) == 0);	/* it is power of 2 */
	nextvaddr = roundup(nextvaddr, bound);
}

/*
 * STATIC void *
 * calloc_typed(ulong_t size, uint_t flag)
 *
 *	Allocate zeroed memory at boot time; optionally, with
 *	physical memory suitable for all forms of physical I/O.
 *
 * Calling/Exit State:
 *
 *	Returns the virtual address of a zeroed chunk of memory
 *	at least "size" bytes long.  The returned address is 
 *	aligned to at least a sizeof(long) boundary.  Also rounds
 *	up "size" to a sizeof(long) multiple.
 *
 *	Use callocrnd() to cause next allocation to occur
 *	on a given boundary.
 *
 *	flag may be either:
 *		PMEM_PHYSIO
 *		      Caller requires physical pages meeting all I/O
 *		      constraints such as DMA-ability for all devices.
 *		      Also requires the memory to be physically contiguous.
 *		PMEM_ANY
 *		      Caller accepts any physical pages (but implies
 *		      a preference for physical pages that fail the
 *		      conditions required by PMEM_PHYSIO).
 */
STATIC void *
calloc_typed(ulong_t size, uint_t flag)
{
	vaddr_t nextvpage;
	uint_t bytesleft;
	vaddr_t raddr;
	paddr_t newphysp;
	ulong_t unused_bytes, psize, npages;

	if (!hat_static_callocup) {
		/*
		 *+ An attempt was made to allocate kernel memory
		 *+ via a memory allocator which is only functioning
		 *+ during kernel initialization (boot) and the allocation
		 *+ request occurred either too early, or too late,
		 *+ in the kernel initialization sequence.  This indicates
		 *+ a kernel software problem.
		 *+ Corrective action:  None.
		 */
		cmn_err(CE_PANIC, "calloc_typed: called while disabled");
	}

	size = roundup(size, sizeof(long));
	nextvpage = roundup(nextvaddr, MMU_PAGESIZE);
	bytesleft = nextvpage - nextvaddr;

	if (flag == PMEM_PHYSIO &&
	    (curphystype != PMEM_PHYSIO || bytesleft < size)) {
		/* we could queue fragments, but not now */
		raddr = nextvpage;
		goto moremem;
	}

	if (bytesleft >= size) {
		/*
		 * easiest case: it fits in the current page.
		 */
		raddr = nextvaddr;
		nextvaddr += size;
		return((void *)raddr);
	}

	/*
	 * Use the remaining bytes in the current page;
	 * adjust the size to indicating the additionally
	 * needed bytes.
	 */

	raddr = nextvaddr;
	size -= bytesleft;
moremem:
	nextvaddr = nextvpage;
	npages = mmu_btopr(size);
	psize = mmu_ptob(npages);
	unused_bytes = psize - size;
	ASSERT(npages != 0);
	do {
		/*
		 * No need to obtain physically contiguous memory
		 * except in the case of a PMEM_PHYSIO request.
		 */
		if (flag != PMEM_PHYSIO)
			psize = MMU_PAGESIZE;
		newphysp = palloc(psize, flag, &curphystype);
		do {
			loadpte(nextvaddr, newphysp, PG_RW | PG_V);
			bzero((void *)nextvaddr, MMU_PAGESIZE);

			nextvaddr += MMU_PAGESIZE;
			newphysp += MMU_PAGESIZE;
			psize -= MMU_PAGESIZE;
			--npages;
		} while (psize != 0);
	} while (npages != 0);

	nextvaddr -= unused_bytes;

	return((void *)raddr);
}

/*
 * STATIC void
 * loadpte(vaddr_t addr, paddr_t paddr, uint_t pte_flags)
 *
 *	Load a static kernel level 2 pte, allocating the level 2 page table,
 *	if necessary.
 *
 * Calling/Exit State:
 *
 *	The loaded pte is the one which maps the kernel virtual address,
 *	"addr".  "paddr" is the physical address of the page on CG cgid which 
 * 	is mapped by this pte.
 *
 */
STATIC void
loadpte(vaddr_t addr, paddr_t paddr, uint_t pte_flags)
{
	/* Allocate a level 2 page table, if needed  */

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		pae_alloc_l2pt(addr);

		/* Fill in the pte in the level 2 page table */

		kvtol2ptep64(addr)->pg_pte = pae_mkpte(pte_flags, pae_pfnum(paddr));
	} else
#endif /* PAE_MODE */
	{
		alloc_l2pt(addr);

		/* Fill in the pte in the level 2 page table */

		kvtol2ptep(addr)->pg_pte = mkpte(pte_flags, pfnum(paddr));
	}
}


/*
 * void
 * alloc_l2pt(vaddr_t vaddr)
 *
 * Calling/Exit State:
 *
 *	Allocate and zero a level 2 page table, if needed,
 *	corresponding to the specified kernel virtual address.
 */
void
alloc_l2pt(vaddr_t vaddr)
{
	pte_t *l1ptep;
	pte_t *l2ptep;
	paddr_t newpt;
	uint_t dummy;

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		pae_alloc_l2pt(vaddr);
		return;
	}
#endif /* PAE_MODE */

	l1ptep = kvtol1ptep(vaddr);
	if (l1ptep->pg_pte == 0) {
		/*
		 * We shouldn't be allocating the per-engine 
		 * level 2 page table.  It should already
		 * be allocated during system initialization.
		 */
		ASSERT(!KADDR_PER_ENG(vaddr));

		/*
		 * Allocate a new level 2 page table.
		 *
		 * We allow the level 1 pte to have user mode access,
		 * in case a user-accessible translation is loaded.
		 * This doesn't give automatic user access; it just allows
		 * whatever the level 2 pte permissions to control access.
		 */

		newpt = palloc(MMU_PAGESIZE, PMEM_ANY, &dummy);
		l1ptep->pg_pte = mkpte(PG_RW | PG_V, pfnum(newpt));

		/* Zero the new level 2 page table */

		l2ptep = kvtol2ptep(vaddr);
		l2ptep = (pte_t *)((vaddr_t)l2ptep & MMU_PAGEMASK);
		bzero((void *)l2ptep, MMU_PAGESIZE);
	}
}

#ifdef PAE_MODE
/*
 * STATIC void
 * pae_alloc_l2pt(vaddr_t vaddr)
 *
 * Calling/Exit State:
 *
 *	Allocate and zero a level 2 page table, if needed,
 *	corresponding to the specified kernel virtual address.
 */
STATIC void
pae_alloc_l2pt(vaddr_t vaddr)
{
	pte64_t *l1ptep;
	pte64_t *l2ptep;
	paddr_t newpt;
	uint_t dummy;
#ifdef CCNUMA
	pte64_t *pteforl2v;
#endif /* CCNUMA */

	l1ptep = kvtol1ptep64(vaddr);
	if (l1ptep->pg_pte == 0) {
		/*
		 * We shouldn't be allocating the per-engine 
		 * level 2 page table.  It should already
		 * be allocated during system initialization.
		 */
		ASSERT(!KADDR_PER_ENG(vaddr));

		/*
		 * Allocate a new level 2 page table.
		 *
		 * We allow the level 1 pte to have user mode access,
		 * in case a user-accessible translation is loaded.
		 * This doesn't give automatic user access; it just allows
		 * whatever the level 2 pte permissions to control access.
		 */

		newpt = palloc(MMU_PAGESIZE, PMEM_ANY, &dummy);
		l1ptep->pg_pte = pae_mkpte(PG_RW | PG_V, pae_pfnum(newpt));

#ifdef CCNUMA
		pteforl2v = kvtol2pteptep64(vaddr);
		pteforl2v->pg_pte = pae_mkpte(PG_RW | PG_V, pae_pfnum(newpt));
#endif /* CCNUMA */

		/* Zero the new level 2 page table */

		l2ptep = kvtol2ptep64(vaddr);
		l2ptep = (pte64_t *)((vaddr_t)l2ptep & MMU_PAGEMASK);
		bzero((void *)l2ptep, MMU_PAGESIZE);
	}
}
#endif /* PAE_MODE */

/*
 * vaddr_t
 * physmap2(paddr_t paddr, vaddr_t vaddr)
 *
 *	Use a temporary mapping with per-engine kernel virtual.
 *
 * Calling/Exit State:
 *	Must be called at early sysinit time, before kvm_init().
 */
vaddr_t
physmap2(paddr_t paddr, vaddr_t vaddr)
{
	ASSERT(KADDR_PER_ENG(vaddr));

	loadpte(vaddr, paddr, PG_RW | PG_CD | PG_V);
	TLBSflush1(vaddr);

	return vaddr;
}

/*
 * vaddr_t
 * physmap0(paddr_t paddr, size_t nbytes)
 * 	Allocate a permanent virtual address mapping for a given range
 *	of physical addresses.
 *
 * Calling/Exit State:
 *	Must be called at early sysinit time, before kvm_init().
 */
vaddr_t
physmap0(paddr_t paddr, size_t nbytes)
{
	ulong_t npages;
	vaddr_t raddr;
	int offset;

	if (!hat_static_callocup) {
		/*
		 * An attempt was made to allocate kernel memory
		 * via a memory allocator which is only functioning
		 * during kernel initialization (boot) and the allocation
		 * request occurred either too early, or too late,
		 * in the kernel initialization sequence.
		 */
		return (NULL);
	}

	nextvaddr = roundup(nextvaddr, MMU_PAGESIZE);
	offset = (int)(paddr & MMU_PAGEOFFSET);
	raddr = nextvaddr + offset;
	paddr &= MMU_PAGEMASK;

	for (npages = mmu_btopr(nbytes + offset); npages-- != 0;) {
		loadpte(nextvaddr, paddr, PG_RW | PG_CD | PG_V);
		nextvaddr += MMU_PAGESIZE;
		paddr += MMU_PAGESIZE;
	}

	return raddr;
}

/*
 * boolean_t
 * physmap0_free(vaddr_t vaddr, size_t nbytes)
 *	Check if a virtual address mapping is permanent (calloc space).
 *
 * Calling/Exit State:
 *	Return B_TRUE if cannot free the virtual address, otherwise
 *	return B_FALSE.
 */
/* ARGSUSED */
boolean_t
physmap0_free(vaddr_t vaddr, size_t nbytes)
{
	ASSERT(!hat_static_callocup || vaddr < nextvaddr);

	if (hat_static_callocup || vaddr < nextvaddr)
		/* cannot free permanent virtual address */
		return B_TRUE;

	return B_FALSE;
}

#ifdef PAE_MODE
/*
 * vaddr_t
 * pae_physmap1(paddr_t paddr, paddr_t *opaddr)
 *	Return a virtual mapping for the MMU_PAGESIZE page containing
 *	physical address "paddr", unless paddr is -1.
 *
 * Calling/Exit State:
 *	Returns the old physical address mapping in "opaddr",
 *	if opaddr is not NULL.
 *
 *	Preemption must be disabled on entry, either explicitly or
 *	by being called from interrupt level.
 */
vaddr_t
pae_physmap1(paddr_t paddr, paddr_t *opaddr)
{
	pte64_t *ptep;

	ptep = kvtol2ptep64(KVPHYSMAP1);
	ASSERT(prmpt_state != 0 || servicing_interrupt());

	/*
	 * Determine the old address from the PTE.
	 */
	if (opaddr != NULL) {
		*opaddr = (paddr_t)-1;
		if (ptep->pg_pte & PG_V)
			*opaddr = pfntophys(ptep->pgm.pg_pfn);
	}

	/*
	 * Re-map the per-engine physmap1-private PTE to the new address.
	 */
	ptep->pg_pte = 0;
	if (paddr != (paddr_t)-1)
		ptep->pg_pte = pae_mkpte(PG_RW | PG_CD | PG_V, phystopfn(paddr));
	TLBSflush1(KVPHYSMAP1);

	return KVPHYSMAP1 + (paddr & MMU_PAGEOFFSET);
}
#endif /* PAE_MODE */

/*
 * vaddr_t
 * physmap1(paddr_t paddr, paddr_t *opaddr)
 *	Return a virtual mapping for the MMU_PAGESIZE page containing
 *	physical address "paddr", unless paddr is -1.
 *
 * Calling/Exit State:
 *	Returns the old physical address mapping in "opaddr",
 *	if opaddr is not NULL.
 *
 *	Preemption must be disabled on entry, either explicitly or
 *	by being called from interrupt level.
 */
vaddr_t
physmap1(paddr_t paddr, paddr_t *opaddr)
{
	pte_t *ptep;

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		return pae_physmap1(paddr, opaddr);
	}
#endif /* PAE_MODE */

	ptep = kvtol2ptep(KVPHYSMAP1);
	ASSERT(prmpt_state != 0 || servicing_interrupt());

	/*
	 * Determine the old address from the PTE.
	 */
	if (opaddr != NULL) {
		*opaddr = (paddr_t)-1;
		if (ptep->pg_pte & PG_V)
			*opaddr = pfntophys(ptep->pgm.pg_pfn);
	}

	/*
	 * Re-map the per-engine physmap1-private PTE to the new address.
	 */
	ptep->pg_pte = 0;
	if (paddr != (paddr_t)-1)
		ptep->pg_pte = mkpte(PG_RW | PG_CD | PG_V,
			phystopfn(paddr));
	TLBSflush1(KVPHYSMAP1);

	return KVPHYSMAP1 + (paddr & MMU_PAGEOFFSET);
}

/*
 * caddr_t
 * physmap64(paddr64_t, ulong_t, uint_t)
 *	A 64-bit version of non-DDI physmap interface.
 *
 *      Allocate a virtual address mapping for a given range of
 *      physical addresses.  Generally used from a driver's init or
 *      start routine to get a pointer to device memory (a.k.a.
 *      memory-mapped I/O).  The flags argument may be KM_SLEEP or
 *      KM_NOSLEEP.  Returns a virtual address, or NULL if the
 *      mapping cannot be allocated.
 *
 * Calling/Exit State:
 *      None.
 */
caddr_t
physmap64(paddr64_t physaddr, ulong_t nbytes, uint_t flags)
{
	paddr64_t base;
	ulong_t npages;
	caddr_t addr;

	if (nbytes == 0)
		return (caddr_t)NULL;

	base = physaddr & PAGEMASK;     /* round-down physical address */
	npages = btopr64(physaddr - base + nbytes);      /* round-up pages */

	addr = (caddr_t) kpg_ppid_mapin(npages, mmu_btop64(base),
				PROT_ALL & ~PROT_USER, flags);

	if (addr == (caddr_t)NULL)
		return (caddr_t)NULL;
	else
		return addr + PAGOFF(physaddr);
}

/*
 * void
 * hat_statpt_alloc(vaddr_t addr, ulong_t size)
 *
 *	Allocate static page tables for the specified kernel
 *	virtual address range.
 *
 * Calling/Exit State:
 *
 *	addr is the starting kernel virtual address.
 *	size is the number of bytes of kernel virtual space.
 *
 * Description:
 *
 *	Must be called once at system initialization time for each
 *	kernel virtual address range which will be managed by hat
 *	static page tables.
 *
 *	After calling this function, specific page translations
 *	can be loaded/unloaded via hat_statpt_memload(),
 *	hat_statpt_devload(), and hat_statpt_unload().
 *
 *	Called only during system initialization when only one processor
 *	is active, so no mutexing required.
 */
void
hat_statpt_alloc(vaddr_t addr, ulong_t size)
{
	vaddr_t eaddr = addr + size;

	ASSERT(addr < eaddr);

	if (!hat_static_pallocup) {
		/*
		 *+ An attempt was made to allocate kernel memory
		 *+ via a memory allocator which is only functioning
		 *+ during kernel initialization (boot) and the allocation
		 *+ request occurred either too early, or too late,
		 *+ in the kernel initialization sequence.  This indicates
		 *+ a kernel software problem.
		 *+ Corrective action:  None.
		 */
		cmn_err(CE_PANIC, "hat_statpt_alloc: called while disabled");
	}

	while (addr < eaddr) {

#ifdef PAE_MODE
		if (PAE_ENABLED()) {
			/* allocate level 2 page table, if needed */
			pae_alloc_l2pt(addr);
			addr = ((addr + PAE_VPTSIZE) & PAE_VPTMASK);
		} else
#endif /* PAE_MODE */
		{
			/* allocate level 2 page table, if needed */
			alloc_l2pt(addr);
			addr = ((addr + VPTSIZE) & VPTMASK);
		}
	}
}

#ifdef PAE_MODE
/*
 * void
 * pae_hat_statpt_memload(vaddr_t addr, ulong_t npages, page_t *pp_list,
 *		      uint_t prot)
 *
 *	Load static page table page translations for physical pages.
 *
 * Calling/Exit State:
 *
 *	addr is the starting kernel virtual address.
 *	npages is the number of PAGESIZE pages to translate.
 *	pp_list point to an ordered list of `npages' pages.
 *		The first page in pp_list corresponds to `addr'.
 *	prot is the protection to establish for the page translations
 *		specified in the same format as supplied to hat_vtop_prot().
 *
 *	Caller ensures the specified address range is managed by hat
 *	static page tables previously allocated via hat_statpt_alloc()
 *	and that the range is not currently translated.
 */
void
pae_hat_statpt_memload(vaddr_t addr, ulong_t npages, page_t *pp_list, uint_t prot)
{
	page_t *pp;
#ifndef CCNUMA
	pte64_t *ptep;
#endif /* CCNUMA */
	uint_t pprot;

#if (PAGESIZE != MMU_PAGESIZE)
#error pae_hat_statpt_memload depends on PAGESIZE == MMU_PAGESIZE
#endif

	ASSERT(KADDR(addr));

	pp = pp_list;
	pprot = hat_vtop_prot(prot);
#ifndef CCNUMA
	ptep = kvtol2ptep64(addr);
	do {
		ASSERT(ptep->pg_pte == 0);
		ASSERT(pp != (page_t *)NULL);
		ptep->pg_pte = pae_mkpte(pprot, page_pptonum(pp));
		ptep++;
		pp = pp->p_next;
		npages--;
		ASSERT(npages == 0 || pp != pp_list);
	} while (npages);
#else /* CCNUMA */
	do {
		pte64_t lpte;

		ASSERT(pp != (page_t *)NULL);
		lpte.pg_pte = pae_mkpte(pprot, page_pptonum(pp));

		FOR_EACH_CG_PTE(addr)
			cg_ptep->pg_pte = lpte.pg_pte;
#ifdef DEBUG_EARLY
			cmn_err(CE_NOTE, "In memload: cg_ptep = 0x%x "
				"pp = 0x%x page_pptonum(pp) = 0x%x "
				"addr = 0x%x\n",
				(int)cg_ptep, (int)pp, (int)page_pptonum(pp),
				(int)addr);
#endif
		END_FOR_EACH_CG_PTE
		pp = pp->p_next;
		npages--;
		addr += PAGESIZE;
		ASSERT(npages == 0 || pp != pp_list);
	} while (npages);
#endif /* CCNUMA */
}
#endif /* PAE_MODE */

/*
 * void
 * hat_statpt_memload(vaddr_t addr, ulong_t npages, page_t *pp_list,
 *		      uint_t prot)
 *
 *	Load static page table page translations for physical pages.
 *
 * Calling/Exit State:
 *
 *	addr is the starting kernel virtual address.
 *	npages is the number of PAGESIZE pages to translate.
 *	pp_list point to an ordered list of `npages' pages.
 *		The first page in pp_list corresponds to `addr'.
 *	prot is the protection to establish for the page translations
 *		specified in the same format as supplied to hat_vtop_prot().
 *
 *	Caller ensures the specified address range is managed by hat
 *	static page tables previously allocated via hat_statpt_alloc()
 *	and that the range is not currently translated.
 */
void
hat_statpt_memload(vaddr_t addr, ulong_t npages, page_t *pp_list, uint_t prot)
{
	page_t *pp;
	pte_t *ptep;
	uint_t pprot;

#if (PAGESIZE != MMU_PAGESIZE)
#error hat_statpt_memload depends on PAGESIZE == MMU_PAGESIZE
#endif

	ASSERT(KADDR(addr));

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		pae_hat_statpt_memload(addr, npages, pp_list, prot);
		return;
	}
#endif /* PAE_MODE */
	pp = pp_list;
	pprot = hat_vtop_prot(prot);
	ptep = kvtol2ptep(addr);
	do {
		ASSERT(ptep->pg_pte == 0);
		ASSERT(pp != (page_t *)NULL);
		ptep->pg_pte = (uint_t)mkpte(pprot, page_pptonum(pp));
		ptep++;
		pp = pp->p_next;
		npages--;
		ASSERT(npages == 0 || pp != pp_list);
	} while (npages);
}

#ifdef PAE_MODE
/*
 * void
 * pae_hat_statpt_devload(vaddr_t addr, ulong_t npages, ppid_t phys, uint_t prot)
 *
 *	Load static page table page translations for physical I/O pages.
 *
 * Calling/Exit State:
 *
 *	addr is the starting kernel virtual address.
 *	npages is the number of PAGESIZE pages to translate.
 *	phys is the starting physical page ID to map.
 *		`phys' is mapped at `addr', `phys'+1 is mapped at `addr'+1, etc.
 *	prot is the protection to establish for the page translations
 *		specified in the same format as supplied to hat_vtop_prot().
 *
 *	Caller ensures the specified address range is managed by hat
 *	static page tables previously allocated via hat_statpt_alloc()
 *	and that the range is not currently translated.
 *
 *	This functions assumes that the memory range does not consist of
 *	mixed mainstore pages and device memory pages. The entire range
 *	is of one storage class.
 */
void
pae_hat_statpt_devload(vaddr_t addr, ulong_t npages, ppid_t phys, uint_t prot)
{
#ifndef CCNUMA
	pte64_t *ptep;
#endif /* CCNUMA */
	uint_t pprot;

#if (PAGESIZE != MMU_PAGESIZE)
#error pae_hat_statpt_devload depends on PAGESIZE == MMU_PAGESIZE
#endif

	ASSERT(KADDR(addr));

	pprot = hat_vtop_prot(prot);

	if (!mainstore_memory(mmu_ptob(phys)))
		pprot |= PG_CD;

#ifndef CCNUMA
	ptep = kvtol2ptep64(addr);
	do {
		ASSERT(ptep->pg_pte == 0);
		ptep->pg_pte = pae_mkpte(pprot, phys);
		ptep++;
		phys++;
		npages--;
	} while (npages);
#else /* CCNUMA */
 	do {
		pte64_t lpte;

		lpte.pg_pte = pae_mkpte(pprot, phys);
		FOR_EACH_CG_PTE(addr)
			ASSERT(cg_ptep->pg_pte == 0);
			cg_ptep->pg_pte = lpte.pg_pte;
#ifdef DEBUG_EARLY
			if (npages <= 5)
				cmn_err(CE_NOTE, "In devload: ptep = 0x%x "
					"phys = 0x%x addr = 0x%x\n",
					(int)cg_ptep, (int)phys, (int)addr);
#endif
		END_FOR_EACH_CG_PTE
		addr += MMU_PAGESIZE;
		phys++;
		npages--;
	} while (npages);
#endif /* CCNUMA */
}
#endif /* PAE_MODE */

/*
 * void
 * hat_statpt_devload(vaddr_t addr, ulong_t npages, ppid_t phys, uint_t prot)
 *
 *	Load static page table page translations for physical I/O pages.
 *
 * Calling/Exit State:
 *
 *	addr is the starting kernel virtual address.
 *	npages is the number of PAGESIZE pages to translate.
 *	phys is the starting physical page ID to map.
 *		`phys' is mapped at `addr', `phys'+1 is mapped at `addr'+1, etc.
 *	prot is the protection to establish for the page translations
 *		specified in the same format as supplied to hat_vtop_prot().
 *
 *	Caller ensures the specified address range is managed by hat
 *	static page tables previously allocated via hat_statpt_alloc()
 *	and that the range is not currently translated.
 *
 *	This functions assumes that the memory range does not consist of
 *	mixed mainstore pages and device memory pages. The entire range
 *	is of one storage class.
 */
void
hat_statpt_devload(vaddr_t addr, ulong_t npages, ppid_t phys, uint_t prot)
{
	pte_t *ptep;
	uint_t pprot;

#if (PAGESIZE != MMU_PAGESIZE)
#error hat_statpt_devload depends on PAGESIZE == MMU_PAGESIZE
#endif

	ASSERT(KADDR(addr));

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		pae_hat_statpt_devload(addr, npages, phys, prot);
		return;
	}
#endif /* PAE_MODE */

	pprot = hat_vtop_prot(prot);

	if (!mainstore_memory(mmu_ptob(phys)))
		pprot |= PG_CD;

	ptep = kvtol2ptep(addr);
	do {
		ASSERT(ptep->pg_pte == 0);
		ptep->pg_pte = (uint_t)mkpte(pprot, phys);
		ptep++;
		phys++;
		npages--;
	} while (npages);
}

#ifdef PAE_MODE
/*
 * void
 * pae_hat_statpt_unload(vaddr_t addr, ulong_t npages)
 *
 *	Unload static page table page translations.
 *
 * Calling/Exit State:
 *
 *	addr is the starting kernel virtual address.
 *	npages is the number of PAGESIZE pages to unload.
 *
 *	Caller is responsible for doing TLB flush/shootdown!
 *	Caller ensures the specified address range is managed by hat
 *	static page tables previously allocated via hat_statpt_alloc().
 */
void
pae_hat_statpt_unload(vaddr_t addr, ulong_t npages)
{
	pte64_t *ptep;

#if (PAGESIZE != MMU_PAGESIZE)
#error pae_hat_statpt_unload depends on PAGESIZE == MMU_PAGESIZE
#endif

	ASSERT(KADDR(addr));

#ifndef CCNUMA
	ptep = kvtol2ptep64(addr);
	do {
		ptep->pg_pte = 0;
		ptep++;
		npages--;
	} while (npages);
#else /* CCNUMA */
 	do {
		FOR_EACH_CG_PTE(addr)
			cg_ptep->pg_pte = 0;
		END_FOR_EACH_CG_PTE
		addr += MMU_PAGESIZE;
		npages--;
	} while (npages);
#endif /* CCNUMA */
}
#endif /* PAE_MODE */

/*
 * void
 * hat_statpt_unload(vaddr_t addr, ulong_t npages)
 *
 *	Unload static page table page translations.
 *
 * Calling/Exit State:
 *
 *	addr is the starting kernel virtual address.
 *	npages is the number of PAGESIZE pages to unload.
 *
 *	Caller is responsible for doing TLB flush/shootdown!
 *	Caller ensures the specified address range is managed by hat
 *	static page tables previously allocated via hat_statpt_alloc().
 */
void
hat_statpt_unload(vaddr_t addr, ulong_t npages)
{
	pte_t *ptep;

#if (PAGESIZE != MMU_PAGESIZE)
#error hat_statpt_unload depends on PAGESIZE == MMU_PAGESIZE
#endif

	ASSERT(KADDR(addr));

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		pae_hat_statpt_unload(addr, npages);
		return;
	}
#endif /* PAE_MODE */
	ptep = kvtol2ptep(addr);
	do {
		ptep->pg_pte = 0;
		ptep++;
		npages--;
	} while (npages);
}

/*
 * void *
 * calloc_virtual(ulong_t size)
 *	Allocate "calloc" virtual.
 *
 * Calling/Exit State:
 *	Returns the virtual address of a chunk of virtual memory of
 *	size, size.
 *
 *	Must be called at "calloc" time.
 */
void *
calloc_virtual(ulong_t size)
{
	void *va;

	va = (void *)nextvaddr;
	nextvaddr += size;
	return va;
}

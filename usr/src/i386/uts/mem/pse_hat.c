/*	Copyright (c) 1993 UNIX System Laboratories, Inc.	*/
/*	(a wholly-owned subsidiary of Novell, Inc.).     	*/
/*	All Rights Reserved.                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF UNIX SYSTEM     	*/
/*	LABORATORIES, INC. (A WHOLLY-OWNED SUBSIDIARY OF NOVELL, INC.).	*/
/*	The copyright notice above does not evidence any actual or     	*/
/*	intended publication of such source code.                      	*/

#ident	"@(#)kern-i386:mem/pse_hat.c	1.3.8.1"
#ident	"$Header$"

#include <acc/mac/mac.h>
#include <util/types.h>
#include <util/param.h>
#include <util/emask.h>
#include <mem/immu.h>
#include <mem/immu64.h>
#include <fs/vnode.h>
#include <proc/mman.h>
#include <mem/tuneable.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <util/inline.h>
#include <svc/errno.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <mem/kmem.h>
#include <mem/pse.h>
#include <mem/vmparam.h>
#include <mem/vm_hat.h>
#include <mem/hat.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/anon.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <mem/mem_hier.h>
#include <proc/cred.h>
#include <svc/creg.h>
#include <svc/cpu.h>
#include <util/plocal.h>
#include <svc/clock.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <proc/disp.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

/*
 * Hat layer support for PSE.  Supports the following entries:
 *	pse_hat_chgprot
 *	pse_hat_devload
 *	pse_hat_unload
 *	pse_pae_hat_chgprot
 *	pse_pae_hat_devload
 *	pse_pae_hat_unload
 *
 * The PSE hat layer support is used by the seg_pse and seg_kpse
 * drivers.  It's written with the following assumptions:
 *	- PSE pages are locked in memory.
 *	- PSE pages have no backing store; their contents aren't
 *		saved anywhere.  Therefore, the reference and
 *		modify bits of PSE page directory entries
 *		are not tracked.
 *	- User-level PSE translations are created when a seg_pse
 *		segment is created, and unloaded only when the
 *		segment is unmapped.
 *	- In PAE mode, the logical size of PSE page is 4MB, instead
 *	  of 2MB provided by the Pentium Pro. IOW, two consecutive
 *	  page directory entries are programmed.
 */

extern void hat_kas_l1pteload(pte_t *, uint_t);
extern void hat_kas_zerol1ptes(pte_t *, uint_t);
extern int hat_uas_shootdown_l(hat_t *);
extern boolean_t hat_prep(struct as *, vaddr_t *, vaddr_t, uint_t,
	pte_t **, pte_t **, pte_t **, page_t **, vaddr_t *);
extern void hat_unprep(struct as *, page_t **, pte_t *, vaddr_t);
extern void hat_switch_l1(struct as *);


extern void pae_hat_kas_l1pteload(vaddr_t, pte64_t *);
extern void pae_hat_kas_zerol1ptes(vaddr_t, pte64_t *);
extern boolean_t pae_hat_prep(struct as *, vaddr_t *, vaddr_t, uint_t,
	pte64_t **, pte64_t **, pte64_t **, page_t **, vaddr_t *, vaddr_t *);
extern void pae_hat_unprep(struct as *, vaddr_t, page_t **, pte64_t *, vaddr_t);
extern void pae_hat_switch_l1(struct as *);

extern hat_t *kas_hatp;


/*
 * boolean_t
 * pse_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
 *		bolean_t doshoot)
 *	Change the protections for the virtual address range
 *	[addr,addr+len] to the protection prot.
 *
 * Calling/Exit State:
 *	The return value is useful only when argument doshoot is B_FALSE.
 *	It return B_TRUE, if the caller needs to perform a shootdown.
 *	It returns B_FALSE, if no shootdown is neccessary. 
 *	No hat or page locks are held by the caller.
 *	No hat or page locks are held on return.
 *	This is never used on the kas.
 *	This changes only active PTEs, no mappings are added,
 *	and none are removed. So only the hat resource lock is needed.
 *	TLB Shootdown is performed based on the doshoot flag. If the flag
 *	is B_FALSE, then the caller gurantees that there are no accesses
 *	to this range and the caller is reponsible to perfrom the shootdown.
 *	Since vtop operations must search the hatpt list anyway, the trick of 
 *	unloading level 1 entries is used to minimize the TLBS
 *	synchronization time (the time between TLBSsetup and TLBSteardown).
 *
 * Remarks:
 *	We don't sweat the loss of modify and reference information from
 *	the pte, since the contents of PSE pages aren't saved anywhere.
 */
/*ARGSUSED4*/
boolean_t
pse_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
		boolean_t doshoot)
{
	struct hat *hatp;
	pl_t opl;
	page_t *ptpp;
	pte_t *pdtep, *epdtep, *ptep;
	uint_t pprot, pmask;
	vaddr_t kvaddr = KVTMPPT1;
	struct as *as;
	vaddr_t endaddr;
	int ptndx;

	as = seg->s_as;
	hatp = &as->a_hat;

	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((len & PSE_PAGEOFFSET) == 0);
	ASSERT(PSE_SUPPORTED());
	ASSERT(hatp->hat_privl1_list);

	if (prot == (unsigned)~PROT_WRITE) {
		pmask = (unsigned)~PG_RW;
		pprot = 0;
	} else {
		pmask = (unsigned)~(PTE_PROTMASK|PG_V);
		pprot = hat_vtop_prot(prot);
	}

	endaddr = addr + len;
	ASSERT(endaddr >= addr);

	opl = HATRL_LOCK(hatp);

	do {
		if (hat_prep(as, &addr, endaddr, 0, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr) == B_FALSE) {
			HATRL_UNLOCK(hatp, opl);
			return B_FALSE;
		}
		ASSERT(ptep->pg_pte & PG_PS);
		atomic_and(&ptep->pg_pte, pmask);
		atomic_or(&ptep->pg_pte, pprot); 
		ptndx = ptep - (pte_t *)pagerounddown((vaddr_t)ptep);
		hat_l1pteload(hatp, ptndx, ptep);
		if (hat_uas_shootdown_l(hatp))
			TLBSflushtlb();
		hat_unprep(as, &ptpp, ptep, kvaddr);
		addr = (addr + VPTSIZE) & VPTMASK;
	} while (addr < endaddr);

	HATRL_UNLOCK(hatp, opl);
	return B_FALSE;
}

#ifdef PAE_MODE
/*
 * boolean
 * pse_pae_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
 *		bolean_t doshoot)
 *	Change the protections for the virtual address range
 *	[addr,addr+len] to the protection prot.
 *
 * Calling/Exit State:
 *	See comments above.
 */
/*ARGSUSED4*/
boolean_t
pse_pae_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
		boolean_t doshoot)
{
	struct hat *hatp;
	pl_t opl;
	page_t *ptpp;
	pte64_t *pdtep, *epdtep, *ptep;
	uint_t pprot, curprot;
	vaddr_t kvaddr = KVTMPPT1;
	struct as *as;
	vaddr_t endaddr, savaddr;

	as = seg->s_as;
	hatp = &as->a_hat;

	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((len & PSE_PAGEOFFSET) == 0);
	ASSERT(PSE_SUPPORTED());
	ASSERT(hatp->hat_privl164_list);

	if (prot == (unsigned)~PROT_WRITE)
		pprot = PG_US|PG_V;
	else
		pprot = hat_vtop_prot(prot);
	
	endaddr = addr + len;
	ASSERT(endaddr >= addr);

	opl = HATRL_LOCK(hatp);

	do {
		if (pae_hat_prep(as, &addr, endaddr, 0, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
			HATRL_UNLOCK(hatp, opl);
			return B_FALSE;
		}
		ASSERT(ptep->pg_pte & PG_PS);
		curprot = (ptep->pte32.pg_low & (PG_US|PG_RW|PG_V));
		/* skip pages that already have the correct protections */
		if ( ((prot == ~PROT_WRITE) && (curprot & PG_RW)) ||
		     ((prot != ~PROT_WRITE) && (curprot != pprot))) {
			ptep->pte32.pg_low = pprot | (ptep->pte32.pg_low & ~(PG_US|PG_RW|PG_V));
			pae_hat_l1pteload(hatp, addr, ptep);
			if (hat_uas_shootdown_l(hatp))
				TLBSflushtlb();
		}
		pae_hat_unprep(as, savaddr, &ptpp, ptep, kvaddr);
		addr = (addr + PAE_VPTSIZE) & PAE_VPTMASK;
	} while (addr < endaddr);

	HATRL_UNLOCK(hatp, opl);
	return B_FALSE;
}
#endif /* PAE_MODE */

/*
 * void
 * pse_hat_devload(struct seg *seg, vaddr_t addr, ppid_t ppid, uint_t prot)
 *
 * Calling/Exit State:
 *	The containing as is write-locked on entry and remains
 *	write-locked on exit.
 * 
 * Remarks:
 *	Don't bother trimming after loading the pte, since we don't
 *	count this against a_rss.
 */
void
pse_hat_devload(struct seg *seg, vaddr_t addr, ppid_t ppid, uint_t prot)
{
	hat_t *hatp;
	uint_t mode;
	struct as *as;
	vaddr_t endaddr;
	pte_t *ptep, *pdtep, *epdtep;
	page_t *ptpp;
	vaddr_t kvaddr = KVTMPPT1;
	int ptndx;

	as = seg->s_as;
	hatp = &as->a_hat;

	ASSERT(PSE_SUPPORTED());
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((ppid & PSE_NPGOFFSET) == 0);
	ASSERT(as != &kas);

	if (!hatp->hat_privl1_list)
		hat_switch_l1(as);
		
	ASSERT(hatp->hat_privl1_list);
	endaddr = addr + PSE_PAGESIZE - 1;
	/*
	 * Disable caching for anything outside of the mainstore memory.
	 */
	mode = hat_vtop_prot(prot);
	if (!mainstore_memory(mmu_ptob(ppid)))
		mode |= PG_CD;

	HATRL_LOCK_SVPL(hatp);
	if (hat_prep(as, &addr, endaddr, HAT_ALLOCPSE, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr) == B_FALSE) {
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}
	ptep->pg_pte = pse_mkpte(mode, ppid);
	ptndx = ptep - (pte_t *)pagerounddown((vaddr_t)ptep);
	hat_l1pteload(hatp, ptndx, ptep);
	hat_unprep(as, &ptpp, ptep, kvaddr);
	HATRL_UNLOCK_SVDPL(hatp);
}

#ifdef PAE_MODE
/*
 * void
 * pse_pae_hat_devload(struct seg *seg, vaddr_t addr, ppid_t ppid, uint_t prot)
 *
 * Calling/Exit State:
 *	See comments above.
 */
void
pse_pae_hat_devload(struct seg *seg, vaddr_t addr, ppid_t ppid, uint_t prot)
{
	hat_t *hatp;
	uint_t mode;
	struct as *as;
	vaddr_t endaddr, savaddr;
	pte64_t *ptep, *pdtep, *epdtep;
	page_t *ptpp;
	vaddr_t kvaddr = KVTMPPT1;

	as = seg->s_as;
	hatp = &as->a_hat;

	ASSERT(PSE_SUPPORTED());
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((ppid & PSE_NPGOFFSET) == 0);
	ASSERT(as != &kas);

	if (!hatp->hat_privl164_list)
		pae_hat_switch_l1(as);
		
	ASSERT(hatp->hat_privl164_list);
	endaddr = addr + PSE_PAGESIZE - 1;
	/*
	 * Disable caching for anything outside of the mainstore memory.
	 */
	mode = hat_vtop_prot(prot);
	if (!mainstore_memory(mmu_ptob(ppid)))
		mode |= PG_CD;

	HATRL_LOCK_SVPL(hatp);
	do {
		if (pae_hat_prep(as, &addr, endaddr, HAT_ALLOCPSE, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
			HATRL_UNLOCK_SVDPL(hatp);
			return;
		}
		ptep->pg_pte = pse_pae_mkpte(mode, ppid);
		pae_hat_l1pteload(hatp, addr, ptep);
		pae_hat_unprep(as, savaddr, &ptpp, ptep, kvaddr);
		addr = (addr + PAE_VPTSIZE) & PAE_VPTMASK;
		ppid = ppid + (PAE_VPTSIZE >> PAGESHIFT);
	} while (addr < endaddr);

	HATRL_UNLOCK_SVDPL(hatp);
}
#endif /* PAE_MODE */

/*
 * void
 * pse_hat_unload(struct seg *seg, vaddr_t addr, ulong_t len)
 *	Unload PSE mappings in the range [addr, addr + len).
 *
 * Calling/Exit State:
 *	HATRL_LOCK is available on entry and exit.
 *
 *	Takes care of TLB synchronization.
 */
/*ARGSUSED3*/
void
pse_hat_unload(struct seg *seg, vaddr_t addr, ulong_t len)
{
	struct hat *hatp;
	pl_t opl;
	struct as *as;
	vaddr_t endaddr;
	pte_t *ptep, *pdtep, *epdtep;
	page_t *ptpp;
	vaddr_t kvaddr = KVTMPPT1;

	as = seg->s_as;
	hatp = &as->a_hat;

	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((len & PSE_PAGEOFFSET) == 0);
	ASSERT(PSE_SUPPORTED());
	ASSERT(hatp->hat_privl1_list);

	opl = HATRL_LOCK(hatp);

	endaddr = addr + len - 1;
	do {
		if (hat_prep(as, &addr, endaddr, 0, &pdtep, &epdtep,
					&ptep, &ptpp, &kvaddr) == B_FALSE) {
			HATRL_UNLOCK(hatp, opl);
			return;
		}
		ASSERT(ptep->pg_pte & PG_PS);
		hat_zerol1pte(hatp, ptep);
		if (hat_uas_shootdown_l(hatp))
			TLBSflushtlb();
		hat_unprep(as, &ptpp, ptep, kvaddr);
		addr = (addr + VPTSIZE) & VPTMASK;
	} while (addr < endaddr);
	HATRL_UNLOCK(hatp, opl);
}

#ifdef PAE_MODE
/*
 * void
 * pse_pae_hat_unload(struct seg *seg, vaddr_t addr, ulong_t len)
 *	Unload PSE mappings in the range [addr, addr + len).
 *
 * Calling/Exit State:
 *	See comments above.
 */
void
pse_pae_hat_unload(struct seg *seg, vaddr_t addr, ulong_t len)
{
	struct hat *hatp;
	pl_t opl;
	struct as *as;
	vaddr_t endaddr, savaddr;
	pte64_t *ptep, *pdtep, *epdtep;
	page_t *ptpp;
	vaddr_t kvaddr = KVTMPPT1;

	as = seg->s_as;
	hatp = &as->a_hat;

	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((len & PSE_PAGEOFFSET) == 0);
	ASSERT(PSE_SUPPORTED());
	ASSERT(hatp->hat_privl164_list);

	opl = HATRL_LOCK(hatp);

	endaddr = addr + len - 1;
	do {
		if (pae_hat_prep(as, &addr, endaddr, 0, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
			HATRL_UNLOCK(hatp, opl);
			return;
		}
		ASSERT(ptep->pg_pte & PG_PS);
		pae_hat_zerol1pte(hatp, savaddr, ptep);
		if (hat_uas_shootdown_l(hatp))
			TLBSflushtlb();
		pae_hat_unprep(as, savaddr, &ptpp, ptep, kvaddr);
		addr = (addr + PAE_VPTSIZE) & PAE_VPTMASK;
	} while (addr < endaddr);
	HATRL_UNLOCK(hatp, opl);
}
#endif /* PAE_MODE */

/*
 * void
 * pse_hat_statpt_devload(vaddr_t addr, ulong_t npse, ppid_t phys,
 *		uint_t prot)
 *	Load static PSE page translations.
 *
 * Calling/Exit State:
 *	addr is the starting kernel virtual address.
 *	npse is the number of PSE_PAGESIZE pages to translate.
 *	phys is the starting physical page ID to map.
 *	prot is the protection to establish for the page translations
 *		specified in the same format as supplied to hat_vtop_prot().
 *
 *	Caller ensures the specified address range is not currently translated.
 *
 *	Both addr and phys are on a PSE_PAGESIZE-aligned boundary (virtual
 *	for addr, physical for phys).
 */
void
pse_hat_statpt_devload(vaddr_t addr, ulong_t npse, ppid_t phys, uint_t prot)
{
	pte_t *ptep, *baseptep;
	uint_t pprot, i;

	ASSERT(KADDR(addr));
	ASSERT(npse != 0);
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((phys & PSE_NPGOFFSET) == 0);

	baseptep = ptep = kvtol1ptep(addr);
	pprot = hat_vtop_prot(prot);
	for (i = 0 ; i < npse ; ++i) {
		ASSERT(ptep->pg_pte == 0);
		ptep->pg_pte = pse_mkpte(pprot, phys);
		ptep++;
		phys += PSE_NPAGES;
	}
	hat_kas_l1pteload(baseptep, npse);
}

#ifdef PAE_MODE
/*
 * void
 * pse_pae_hat_statpt_devload(vaddr_t addr, ulong_t npse, ppid_t phys,
 *		uint_t prot)
 *	Load static PSE page translations.
 *
 * Calling/Exit State:
 *	See comments above.
 */
void
pse_pae_hat_statpt_devload(vaddr_t addr, ulong_t npse, ppid_t phys, uint_t prot)
{
	pte64_t *ptep;
	uint_t pprot, i;

	ASSERT(KADDR(addr));
	ASSERT(npse != 0);
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((phys & PSE_NPGOFFSET) == 0);

	/*
	 * Note: npse should be multiplied by 2 
	 */
	npse = npse << 1;

	pprot = hat_vtop_prot(prot);
	for (i = 0 ; i < npse ; ++i) {
		ptep = kvtol1ptep64(addr);
		ASSERT(ptep->pg_pte == 0);
		ptep->pg_pte = pse_pae_mkpte(pprot, phys);
		phys += (PSE_NPAGES >> 1);
		pae_hat_kas_l1pteload(addr, ptep);
		addr = (addr + PAE_VPTSIZE) & PAE_VPTMASK;
	}
}
#endif /* PAE_MODE */

/*
 * void
 * pse_hat_statpt_unload(vaddr_t addr, ulong_t npse)
 *	Unload static PSE page translations.
 *
 * Calling/Exit State:
 *	addr is the starting kernel virtual address.
 *	npse is the number of 4MB pages to unload.
 *
 *	Caller is responsible for doing TLB flush/shootdown!
 */
void
pse_hat_statpt_unload(vaddr_t addr, ulong_t npse)
{
	ASSERT(KADDR(addr));
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT(npse != 0);

	hat_kas_zerol1ptes(kvtol1ptep(addr), npse);
}

#ifdef PAE_MODE
/*
 * void
 * pse_pae_hat_statpt_unload(vaddr_t addr, ulong_t npse)
 *	Unload static PSE page translations.
 *
 * Calling/Exit State:
 *	See comments above.
 */
void
pse_pae_hat_statpt_unload(vaddr_t addr, ulong_t npse)
{
	pte64_t *ptep;
	int i;

	ASSERT(KADDR(addr));
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT(npse != 0);

	/*
	 * Note: npse should be multiplied by 2 
	 */
	npse = npse << 1;

	for (i = 0 ; i < npse ; ++i) {
		ptep = kvtol1ptep64(addr);
		ASSERT(ptep->pg_pte != 0);
		ptep->pg_pte = 0;
		pae_hat_kas_zerol1ptes(addr, ptep);
		addr = (addr + PAE_VPTSIZE) & PAE_VPTMASK;
	}
}
#endif /* PAE_MODE */

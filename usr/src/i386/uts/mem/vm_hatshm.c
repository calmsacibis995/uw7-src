#ident	"@(#)kern-i386:mem/vm_hatshm.c	1.3.4.3"
#ident	"$Header$"

/*
 * Shared Page Table (SHPT) interfaces for Dynamically Mapped Shared 
 * Memory (DSHM) and Fine Grained Shared Memory segments.
 */

#include <acc/mac/mac.h>
#include <util/types.h>
#include <util/param.h>
#include <util/emask.h>
#include <mem/immu.h>
#include <mem/immu64.h>
#include <proc/cg.h>
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
#include <mem/rzbm.h>
#include <mem/vmparam.h>
#include <mem/hat.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <mem/mem_hier.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <proc/disp.h>

#ifdef PAE_MODE

/* Temporary, for debugging only */
#undef STATIC
#define STATIC

#define	PFN64(ptep)	((ptep)->pgm.pg_pfn)
#define	HAT_PRIVL164_LIST(hatp)	((hatp)->hat_privl164_list)

#define HATSHPT_LOCK(hatp)        \
	LOCK(&hatp->hat_shptlock, VM_HAT_RESOURCE_IPL)

#define HATSHPT_UNLOCK(hatp, pl)  \
	UNLOCK(&hatp->hat_shptlock, pl)

/*
 * This lkinfo structure is used for all hat_shptlock instances.
 */
STATIC LKINFO_DECL(hatshpt_lkinfo, "hat shared page table lock", 0);

extern uint_t hat_privl1_size;

extern struct page *allocpg(void);
extern void freepg(struct page *);

/*
 * shpt_hat_t *
 * shpt_hat_init(uint_t sz)
 *	Allocate all the L2 pages necessary to map <sz> bytes of
 *	virtual memory.
 *	
 * Calling/Exit State:
 *	Returns NULL if there is not enough memory available for
 *	page tables.
 */
hatshpt_t *
shpt_hat_init(uint_t sz)
{
	hatshpt_t *hshptp;
	cgnum_t cgnum;
	int i, npts;
	ppid_t ppid;
	page_t *ptpp;
	
	if(!PAE_ENABLED())
		return NULL;

	npts = sz >> PAE_PTNUMSHFT; 
#ifdef NOTYET
	if (idf_resv(npts * Ncg, SLEEP) == 0)
		return (NULL);
#endif /* NOTYET */

	hshptp = kmem_zalloc(sizeof(hatshpt_t), KM_SLEEP);
	LOCK_INIT(&hshptp->hat_shptlock, VM_HAT_RESOURCE_HIER,
                        VM_HAT_RESOURCE_IPL, &hatshpt_lkinfo, KM_SLEEP);

	for (cgnum = 0; cgnum < Ncg; cgnum++) {
		for (i = 0; i < npts; i++) {
#ifdef NOTYET
			ppid = idf_page_get(cgnum, PAGESIZE);
			hshptp->hat_rpt[cgnum][i] = ppid;
			pzero(ppid);
#else
			ptpp = allocpg();
			hshptp->hat_rpt[cgnum][i] = ptpp->p_pfn;
			/* set aec to non-zero to avoid releasing the page */
			HATPT_AEC(ptpp) = 1;
			pagezero(ptpp, 0, PAGESIZE);
#endif /* NOTYET */
		}
	}
	return (hshptp);
}

/*
 * void
 * shpt_hat_load(struct as *as, hatshpt_t *hatp, vaddr_t baseaddr, uint_t off,
 *		ppid_t pf, uint_t prot)
 *	Find an L2 based on <off> in the shared memory segment and load the 
 *	pte corressponding to the page frame number <pf>.
 *
 * Calling/Exit State:
 *	The L2 is already attached to the process L1.
 */
void
shpt_hat_load(struct as *as, hatshpt_t *hshptp, vaddr_t baseaddr,
		uint_t off, ppid_t pf, uint_t prot)
{
	pte64_t tpte, spte, *ptep;
	hatpgt64_t *pt;
	uint_t ptndx, pndx, ptpfn;
	uint_t mode;
	cgnum_t cgnum;
	pl_t opl;
	vaddr_t addr;

	mode = hat_vtop_prot(prot);
	ptndx = off >> PAE_PTNUMSHFT;
	pndx =  pae_pnum(off);
	opl = HATSHPT_LOCK(hshptp);
	for (cgnum = 0; cgnum < Ncg; cgnum++) {
		ptpfn = hshptp->hat_rpt[cgnum][ptndx];
		SHPT_MAPPT64((vaddr_t)KVTMPPT1, ptpfn, &spte);
		pt = (hatpgt64_t *)KVTMPPT1;
		ptep = pt->hat_pte + pndx;
		tpte.pg_pte = pae_mkpte(mode, pf);
		pae_setpte(&tpte, ptep);
		SHPT_UNMAPPT64((vaddr_t)KVTMPPT1, &spte);
	}
	TLBSflush1(baseaddr + off);
	HATSHPT_UNLOCK(hshptp, opl);
}

/*
 * void
 * shpt_hat_unload(struct seg *seg, hatshpt_t *hshptp, uint_t off)
 *	Find an L2 based on <off> in the shared memory segment and 
 *	unload the translation.
 *
 * Calling/Exit State:
 *	None.
 */
void
shpt_hat_unload(struct as *as, hatshpt_t *hshptp, uint_t off)
{
	pte64_t tpte, *ptep;
	pte64_t *privl1virt;
	hatpgt64_t *pt;
	uint_t ptndx, pndx, ptpfn;
	uint_t mode;
	cgnum_t cgnum;
	pl_t opl;

	ptndx = off >> PAE_PTNUMSHFT;
	pndx = pae_pnum(off);
	opl = HATSHPT_LOCK(hshptp);
	for (cgnum = 0; cgnum < Ncg; cgnum++) {
		ptpfn = hshptp->hat_rpt[cgnum][ptndx];
		SHPT_MAPPT64((vaddr_t)KVTMPPT1, ptpfn, &tpte);
		pt = (hatpgt64_t *)KVTMPPT1;
		ptep = pt->hat_pte + pndx;
		pae_clrpte(ptep);
		SHPT_UNMAPPT64((vaddr_t)KVTMPPT1, &tpte);
	}
	HATSHPT_UNLOCK(hshptp, opl);
}

/*
 * void
 * shpt_hat_attach(struct as *as, vaddr_t addr, size_t sz, hatshpt_t *hshptp)
 *	Attach all the shared L2 page tables to the process private L1
 *	for the shared memory segment.
 *
 * Calling/Exit State:
 *	Must be called with process context.
 */
void
shpt_hat_attach(struct as *as, vaddr_t addr, size_t sz, 
		hatshpt_t *hshptp, cgnum_t cgnum)
{
	vaddr_t saddr, eaddr;
	pte64_t *ptep, tpte;
	pte64_t *privl1virt;
	int i, npts, pdptndx;
	ppid_t pfn;
	hat_t *hatp = &as->a_hat;
	pl_t opl;

	if (HAT_PRIVL164_LIST(hatp) == NULL) {
		ASSERT(hat_privl1_size != 0);
		pae_hat_switch_l1(as);
	}

	saddr = addr;
	eaddr = saddr + sz;
	npts = sz >> PAE_PTNUMSHFT;
	opl = HATRL_LOCK(hatp);
	for (i = 0; i < npts; i++, saddr += PAE_VPTSIZE) {
		ASSERT(saddr < eaddr);
		pfn = hshptp->hat_rpt[cgnum][i];
		pdptndx = PDPTNDX64(saddr);
		privl1virt = HAT_PRIVL164_LIST(hatp)->hp_privl1virt[pdptndx];
		ptep = privl1virt + pae_ptnum(saddr);
		tpte.pg_pte = pae_mkpte(PTE_RW|PG_V, pfn);
		pae_setpte(&tpte, ptep);
	}
	HATRL_UNLOCK(hatp, opl);
}

/*
 * void
 * shpt_hat_detach(struct as *as, vaddr_t addr, size_t sz, hatshpt_t *hshptp)
 *	Detach all the shared L2 page tables from the process private L1
 *	for the shared memory segment.
 *
 * Calling/Exit State:
 *	None.
 */
void
shpt_hat_detach(struct as *as, vaddr_t addr, size_t sz, hatshpt_t *hshptp)
{
	vaddr_t saddr, eaddr;
	pte64_t *ptep;
	pte64_t *privl1virt;
	int i, npts, pdptndx;
	hat_t *hatp = &as->a_hat;
	pl_t opl;

	ASSERT(hat_privl1_size != 0);
	ASSERT(HAT_PRIVL164_LIST(hatp) != NULL);

	saddr = addr;
	eaddr = saddr + sz;
	npts = sz >> PAE_PTNUMSHFT;
	ASSERT(npts < PDPTSZ * PAE_HAT_EPPT);
	opl = HATRL_LOCK(hatp);
	for (i = 0; i < npts; i++, saddr += PAE_VPTSIZE) {
		ASSERT(saddr < eaddr);
		pdptndx = PDPTNDX64(saddr);
		privl1virt = HAT_PRIVL164_LIST(hatp)->hp_privl1virt[pdptndx];
		ptep = privl1virt + pae_ptnum(saddr);
		ASSERT(ptep->pg_pte);
		pae_clrpte(ptep);
	}
	HATRL_UNLOCK(hatp, opl);
}

/*
 * void
 * shpt_hat_deinit(hatshpt_t *hshptp)
 *	Free all the shared L2 page table.
 *
 * Calling/Exit State:
 *	None.
 */
void
shpt_hat_deinit(hatshpt_t *hshptp)
{
	cgnum_t cgnum;
	int i;
	uint_t pfn;
	page_t *ptpp;

	for (cgnum = 0; cgnum < Ncg; cgnum++) {
		for (i = 0; i < PDPTSZ * PAE_HAT_EPPT; i++) {
			if ((pfn = hshptp->hat_rpt[cgnum][i]) == 0)
				continue;
#ifdef NOTYET
			idf_page_free(pfn, PAGESIZE);
			idf_unresv(1);
#else
			ptpp = page_numtopp(pfn);
			HATPT_AEC(ptpp) = 0;
			freepg(ptpp);
#endif /* NOTYET */
			hshptp->hat_rpt[cgnum][i] = 0;
		}
	}
	kmem_free(hshptp, sizeof(hatshpt_t));
	return;
}

/*
 * uint_t
 * shpt_hat_xlat_info(struct as *as, hatshpt_t *hshptp, vaddr_t addr)
 *	Returns whether or not a translation is loaded at the given address,
 *	which must be a user address in the user address space, as.
 *
 * Calling/Exit State:
 *	The return value is stale unless the caller stabilizes the
 *	access to this address.
 */
uint_t
shpt_hat_xlat_info(struct as *as, hatshpt_t *hshptp, vaddr_t addr)
{
	hat_t *hatp = &as->a_hat;
	uint_t flags = 0;
	pte64_t tpte, *ptep;
	pte64_t *privl1virt;
	hatpgt64_t *pt;
	uint_t pndx, pdptndx;
	pl_t opl;
	/* xlat_flags indexed by PTE bits PG_US|PG_RW|PG_V */
	static int xlat_flags[] = {
		/* !U !W !V */	HAT_XLAT_EXISTS,
		/* !U !W  V */	HAT_XLAT_EXISTS,
		/* !U  W !V */	HAT_XLAT_EXISTS,
		/* !U  W  V */	HAT_XLAT_EXISTS,
		/*  U !W !V */	HAT_XLAT_EXISTS,
		/*  U !W  V */	HAT_XLAT_EXISTS | HAT_XLAT_READ,
		/*  U  W !V */	HAT_XLAT_EXISTS,
		/*  U  W  V */	HAT_XLAT_EXISTS | HAT_XLAT_READ | HAT_XLAT_WRITE
	};

	ASSERT(!KADDR(addr));
	ASSERT(as != (struct as *)NULL);

	pdptndx = PDPTNDX64(addr);
	opl = HATSHPT_LOCK(hshptp);
	privl1virt = HAT_PRIVL164_LIST(hatp)->hp_privl1virt[pdptndx];
	ptep = privl1virt + pae_ptnum(addr);
	SHPT_MAPPT64((vaddr_t)KVTMPPT1, PFN64(ptep), &tpte);
	pt = (hatpgt64_t *)KVTMPPT1;
	ptep = pt->hat_pte + pae_pnum(addr);
	if (ptep->pg_pte != 0)
		flags = xlat_flags[ptep->pg_pte & (PG_US|PG_RW|PG_V)];
	SHPT_UNMAPPT64((vaddr_t)KVTMPPT1, &tpte);
	HATSHPT_UNLOCK(hshptp, opl);

	return flags;
}
#endif /* PAE_MODE */

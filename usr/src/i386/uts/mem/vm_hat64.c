#ident	"@(#)kern-i386:mem/vm_hat64.c	1.3.10.2"
#ident	"$Header$"

/*
 * The hat layer manages the address translation hardware as a cache
 * driven by calls from the higher levels in the VM system.  Nearly
 * all the details of how the hardware is managed should not be visible
 * above this layer except for miscellaneous machine specific functions
 * (e.g. mapin/mapout) that work in conjunction with this code.	 Other
 * than a small number of machine specific places, the hat data
 * structures seen by the higher levels in the VM system are opaque
 * and are only operated on by the hat routines.  In SVR4 each address space
 * contains a struct hat and each page structure contains an opaque pointer
 * which is used by the hat code to hold a list of active translations to
 * that page. However, now the linkages to the translations are removed
 * from the HAT layer and are replaced by the segment chaining. See
 * "VM Design for Large Memory and ccNUMA" document for details.
 *
 * It is assumed by this code:
 *
 *	- no load/unload requests for a range of addresses will
 *		span kernel/user boundaries.
 *	- all addresses to be mapped are on page boundaries.
 *	- all segment sizes, s_size, are multiples of the page
 *		size.
 *
 * NOMINAL SPIN LOCK ORDER:
 *	vm_hashlock
 *	p_vpglock in a page_t
 *	hat_resourcelock in a hat_t
 *	vm_pagefreelck
 *	hat_mcpoollock	hat_ptpoollock (same hierarchy)
 *
 *	The only routine that will be called from interrupt level is vtop.
 */

#include <acc/mac/mac.h>
#include <util/types.h>
#include <util/param.h>
#include <util/emask.h>
#include <mem/immu.h>
#include <mem/immu64.h>
#include <fs/fs_hier.h>
#include <fs/vnode.h>
#include <proc/mman.h>
#include <mem/tuneable.h>
#include <util/cglocal.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <util/inline.h>
#include <svc/errno.h>
#include <proc/cg.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <mem/kmem.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <mem/hat.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/anon.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <mem/mem_hier.h>
#include <proc/cred.h>
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

#ifdef PAE_MODE

/* Temporary, for debugging only */
#undef STATIC
#define STATIC

#define	KPDTE(addr)	(l.kpd64[PDPTNDX64((addr))] + pae_ptnum((addr)))
#define	PFN64(ptep)	((ptep)->pgm.pg_pfn)
#define	HAT_PRIVL164_LIST(hatp)	((hatp)->hat_privl164_list)
#undef hatpt_pdtep
#define hatpt_pdtep	hatpt_pdtep64
#undef hatpt_pde
#define	hatpt_pde	hatpt_pde64
#undef stats_pdtep
#define	stats_pdtep	stats_pdtep64

void pae_hat_kas_unload(vaddr_t addr, ulong_t nbytes, uint_t flags);
void pae_hat_kas_memload(vaddr_t addr, page_t *pp, uint_t prot);
void pae_hat_kas_valid(vaddr_t addr, ulong_t nbytes, boolean_t set);
boolean_t pae_hat_kas_agerange(vaddr_t addr, vaddr_t endaddr);

void pae_hat_pageunload(page_t *pp, struct as *as, vaddr_t addr);
void pae_hat_pagesyncmod(page_t *pp, struct as *as, vaddr_t addr);
void pae_hat_pagegetmod(page_t *pp, struct as *as, vaddr_t addr);

int pae_hat_fastfault(struct as *as, vaddr_t addr, enum seg_rw rw);
boolean_t pae_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len,
			uint_t prot, boolean_t doshoot);
uint_t pae_hat_map(struct seg *seg);
uint_t pae_hat_preload(struct seg *seg, vnode_t *vp, off64_t vp_base, 
			uint_t prot, uint_t flags);
void pae_hat_unlock(struct seg *seg, vaddr_t addr);
int pae_hat_exec(struct as *oas, vaddr_t ostka, ulong_t stksz, struct as *nas,
                vaddr_t nstka, uint_t hatflag);
void pae_hat_asload(struct as *as);
void pae_hat_asunload(struct as *as, boolean_t doflush);
int pae_hat_load(struct seg *seg, vaddr_t addr, page_t *pp, uint_t pf,
                size_t npages, uint_t prot, uint_t flag);
vaddr_t pae_hat_unload(struct as *as, vaddr_t addr_arg, ulong_t len,
             enum age_type howhard, uint_t flags);
vaddr_t pae_hat_dup(struct seg *pseg, struct seg *cseg, uint_t flags);

ppid_t pae_hat_vtoppid(struct as *as, vaddr_t vaddr);
page_t *pae_hat_vtopp(struct as *as, vaddr_t vaddr, enum seg_rw rw);
page_t *pae_hat_vtopp_l(struct as *as, vaddr_t vaddr);
uint_t pae_hat_xlat_info(struct as *as, vaddr_t addr);
#ifdef DEBUG
STATIC void pae_hat_checkptinit(void *pdtep, page_t *ptpp);
#endif /* DEBUG */

extern boolean_t pse_pae_hat_chgprot(struct seg *, vaddr_t, ulong_t, 
			uint_t, boolean_t);
extern void pse_pae_hat_devload(struct seg *, vaddr_t, ppid_t, uint_t);
extern void pse_pae_hat_unload(struct seg *, vaddr_t, ulong_t);
extern void pse_pae_hat_statpt_devload(vaddr_t, ulong_t, ppid_t, uint_t);
extern void pse_pae_hat_statpt_unload(vaddr_t, ulong_t);
extern void pae_hat_statpt_memload(vaddr_t, ulong_t, struct page *, uint_t);
extern void pae_hat_statpt_devload(vaddr_t, ulong_t, ppid_t, uint_t);
extern void pae_hat_statpt_unload(vaddr_t, ulong_t);

hatops_t pae_hatops = {
	pae_hat_load,
	pae_hat_unload,
	pae_hat_kas_unload,
	pae_hat_kas_memload,
	pae_hat_pageunload,
	pae_hat_pagesyncmod,
	pae_hat_pagegetmod,
#ifndef UNIPROC
	pae_hat_fastfault,
#endif /* !UNIPROC */
	pae_hat_chgprot,
	pae_hat_map,
	pae_hat_preload,
	pae_hat_unlock,
	pae_hat_exec,
	pae_hat_asload,
	pae_hat_asunload,
	pae_hat_kas_agerange,
	pae_hat_dup,
	pae_hat_vtopp,
	pae_hat_vtoppid,
	pae_hat_xlat_info,
#ifdef DEBUG
	pae_hat_checkptinit,
#endif /* DEBUG */
	pse_pae_hat_chgprot,
	pse_pae_hat_devload,
	pse_pae_hat_unload,
	pse_pae_hat_statpt_devload,
	pse_pae_hat_statpt_unload,
	pae_hat_kas_valid,
	pae_hat_vtopp_l,
};

#if (VM_HAT_RESOURCE_HIER > VM_HAT_PAGE_HIER_MAX)
#error hat resource lock hierarchy exceeding max. level
#endif

#if (VM_HAT_MCPOOL_HIER > VM_HAT_LOCAL_HIER_MAX)
#error mcpool lock hierarchy exceeding max. level
#endif

#if (VM_HAT_PTPOOL_HIER > VM_HAT_LOCAL_HIER_MAX)
#error ptpool lock hierarchy exceeding max. level
#endif

#if (VM_HAT_TLBS_HIER > VM_HAT_LOCAL_HIER_MAX)
#error tlb lock hierarchy exceeding max. level
#endif

#ifdef DEBUG
/*
 * Some debugging is always available if DEBUG is defined.
 * But some debugging is too expensive and needs further control.
 * Special debugging variables are provided to control the more
 * costly or intrusive debugging audits.
 */
int pae_hatdb_ptfreeck = 1;	/*
			 * If this is nonzero, any page table that is
			 * being freed is audited for all zeros.
			 * The mapping chunk pointers are also audited
			 * for all zeros.
			 * The audit is performed with the hat_resourcelock
			 * held for the as that was using the page table.
			 */
#endif

/*
 * MACRO
 * HAT_SET_MODSTATS(hatp, pdtep, pndx)
 *
 * Calling/Exit State:
 *	None.
 */
#define HAT_SET_MODSTATS(hatp, pdtep, pndx) { \
	hat_stats_t *modstatsp; \
				\
	if ((hatp)->hat_modstatsp != NULL) { \
		modstatsp = pae_hat_findstats((hatp), (pdtep)); \
		if (modstatsp != NULL && modstatsp->stats_pdtep == (pdtep)) { \
			BITMASKN_SET1(modstatsp->stats_modinfo, (pndx)); \
		} \
	} \
}

extern boolean_t need_clean_pages;	/* select clean pages to free */
extern uint_t hat_privl1_size;

extern hat_t *kas_hatp;
/* vprot array used in the hat_vtop_prot macro */
uint_t vprot_array[PROT_ALL+1];

extern page_t *hat_pgalloc(uint_t flag);
extern hatpt_t *hat_ptalloc(uint_t flag);
extern boolean_t intrpend(pl_t);
extern int hat_uas_shootdown_l(hat_t *);
extern void hat_free_modstats(struct as *as);

/* function prototypes */
STATIC hatpt_t *pae_hat_findfirstpt(vaddr_t *, hat_t *);
STATIC enum hat_cont_type pae_hat_dup_range(struct seg *cseg, pte64_t *pptep, 
	  vaddr_t *addrp, vaddr_t endaddr, int pndx,
	  int *num_vpglocks, vnode_t **vpglocks, uint_t flags);
STATIC hatpt_t *pae_hat_findpt(hat_t *, pte64_t *);
hat_stats_t *pae_hat_findstats(hat_t *, pte64_t *);
void pae_hat_vtopte_l(struct as *, vaddr_t, pte64_t *, pte64_t *);
STATIC void pae_hat_switch_l1(struct as *as);

#define	pae_hat_kvtol1ptep64(addr, ptndx, engno) \
	(&(ENGINE_PLOCAL_PAE_PTR(engno)->kl1ptp64[PDPTNDX64((addr))][ptndx]))

#ifndef UNIPROC
/*
 * Functions created because the P6 EFLAGS erratum makes it dangerous to
 * remove write or access permission or validity from a pte while another
 * engine might be performing an ADC, SBB, RCR or RCL to the page (if its
 * address falls out of the TLB, fault EFLAGS are not restartable).  An
 * xcall() interrupts the other engines into the kernel (never midway through
 * an instruction), when it is then safe to remove the valid bit before
 * flushing the TLB.  For a single page, the L2 entry is invalidated; for
 * multiple pages, the L1 entries.
 */

/*
 * void pae_hat_uas_xshield_l1(pae_xshield_t *xshieldp)
 *	Invalidate L1 pte(s)
 *
 * Calling/Exit State:
 *	Called via xcall() from pae_hat_uas_shield() below.
 *	The hat resource lock is held by another engine
 *	[ the caller of	pae_hat_uas_shield()]
 * 	only those engines with this HAT loaded will be
 *	executing this routine.
 */
STATIC void
pae_hat_uas_xshield_l1(xshield_t *xshieldp)
{
	vaddr_t	vaddr = xshieldp->xs_vaddr;
	vaddr_t eaddr = xshieldp->xs_eaddr;
	pte64_t *ptep;
	
	/*
	 * We may need to wait here in the private L1 case
	 * [private L1 shares the 4 pde tables]
	 * because (a) we must ensure PG_V is clear and TLB flushed
	 * before returning to user space; but (b) the entry may be
	 * shared with another cpu which has not yet been signalled,
	 * we must not interfere with its entry until it is.  The
	 * sending cpu sets NO_WAIT_ENG once it knows all have received;
	 * but last cpu to be signalled set NO_WAIT_ENG too, to let us
	 * proceed if the sending cpu got held up.  If some other cpu
	 * is slow to respond, then it must already be in the kernel,
	 * or in SMM: not doing an ADC/SBB on user space.  We could
	 * do this sooner if we knew once xcall() had sent to all,
	 * before collecting its responses.
	 *
	 */
	if (ATOMIC_INT_READ(&xshieldp->xs_wait_eng) != NO_WAIT_ENG) {
		if (ATOMIC_INT_READ(&xshieldp->xs_wait_eng) == l.eng_num)
			ATOMIC_INT_WRITE(&xshieldp->xs_wait_eng, NO_WAIT_ENG);
		else while (ATOMIC_INT_READ(&xshieldp->xs_wait_eng) != 
								NO_WAIT_ENG)
			;/* spin */
	}

	/*
	 * Note: other cpus may be doing exactly the same as
	 * we are, easier to duplicate effort than wait again
	 */

	vaddr &= PAE_VPTMASK;
	do {
		ptep = KPDTE(vaddr);
		ptep->pte32.pg_low &= ~PG_V;
		vaddr += PAE_VPTSIZE;
	} while (vaddr < eaddr); /* uvirt therefore won't wrap */

	/*
	 * Let the sender proceed now: flushing our TLB is a local
	 * matter.
	 */
	ATOMIC_INT_DECR(&xshieldp->xs_wait_cnt);

	TLBSflushtlb();
}
/*
 * void pae_hat_uas_xshield_l2(pae_xshield_t *xshieldp)
 *	Invalidate L2 pte
 *
 * Calling/Exit State:
 *	Called via xcall() from pae_hat_uas_shield() below.
 *	The hat resource lock is held by another engine
 *	[ the caller of	pae_hat_uas_shield()]
 * 	only those engines with this HAT loaded will be
 *	executing this routine.
 */
STATIC void
pae_hat_uas_xshield_l2(xshield_t *xshieldp)
{
	vaddr_t	vaddr = xshieldp->xs_vaddr;
	pte64_t *ptep;
	/*
	 * We may need to wait here.
	 * because (a) we must ensure PG_V is clear and TLB flushed
	 * before returning to user space; but (b) the entry may be
	 * shared with another cpu which has not yet been signalled,
	 * we must not interfere with its entry until it is.  The
	 * sending cpu sets NO_WAIT_ENG once it knows all have received;
	 * but last cpu to be signalled set NO_WAIT_ENG too, to let us
	 * proceed if the sending cpu got held up.  If some other cpu
	 * is slow to respond, then it must already be in the kernel,
	 * or in SMM: not doing an ADC/SBB on user space.  We could
	 * do this sooner if we knew once xcall() had sent to all,
	 * before collecting its responses.
	 *
	 */
	if (ATOMIC_INT_READ(&xshieldp->xs_wait_eng) != NO_WAIT_ENG) {
		if (ATOMIC_INT_READ(&xshieldp->xs_wait_eng) == l.eng_num)
			ATOMIC_INT_WRITE(&xshieldp->xs_wait_eng, NO_WAIT_ENG);
		else while (ATOMIC_INT_READ(&xshieldp->xs_wait_eng) != 
								NO_WAIT_ENG)
			;/* spin */
	}

	/*
	 * Note: other cpus may be doing exactly the same as
	 * we are, easier to duplicate effort than wait again
	 */

	ptep = vtol2ptep64(vaddr);
	ptep->pte32.pg_low &= ~PG_V;

	/*
	 * Let the sender proceed now: flushing our TLB is a local
	 * matter, and we have copied vaddr off the sender's stack
	 */
	ATOMIC_INT_DECR(&xshieldp->xs_wait_cnt);

	flushtlb1(vaddr);
}
#endif /* ndef UNIPROC */

/*
 * boolean_t pae_hat_uas_shield(hat_t *hatp,
 *		vaddr_t uvaddr, vaddr_t evaddr, pte64_t *ptep)
 *	If this uas is active on another engine, shield the given range
 *	by removing the valid bit from the L2 pte for a single page,
 *	or from the L1 pdes for multiple pages, immediately flushing
 *	the addresses out of the TLBs of all those engines affected.
 *
 * Calling/Exit State:
 *	The HAT lock is held on entry and remains held on exit.
 *	Returns to the caller an indication of whether the local
 *	TLB will need to be flushed (like hat_uas_shootdown_l).
 *
 * Remarks:
 *	The caller must allow that it may or may not remove PG_V
 *	from the L1 pdes, and may or may not remove PG_V from the
 *	L2 pte: it does as little as is necessary to shield the
 *	the range.  If necessary and possible, it uses flushtlb1()
 *	for a single page; if necessary but impossible to do that,
 *	will alert the caller to use TLBSflushtlb().
 *
 *	The single page L1 case is mainly for hat_unload(), which
 *	is frequently called on a single page, when breaking COW;
 *	also useful for the expensive but uncommon hat_pageunload().
 */
STATIC boolean_t
pae_hat_uas_shield(hat_t *hatp, vaddr_t uvaddr, vaddr_t evaddr, pte64_t *ptep)
{
#ifndef UNIPROC
	emask_t actives;
#endif
	int lactive, xactive;
	ulong_t doL1;
 
	ASSERT(HATRL_OWNED(hatp));
	ASSERT(hatp != kas_hatp);
	ASSERT(uvaddr < evaddr);
	ASSERT(evaddr <= uvend);
	ASSERT((vaddr_t)ptep >= uvend);
	ASSERT(ptep->pg_pte & PG_V);

	if ((xactive = hatp->hat_activecpucnt) == 0)
		return B_FALSE;

	doL1 = (uvaddr ^ (evaddr-1)) & PG_ADDR;	/* more than one page */

#ifdef UNIPROC
	lactive = 1;
#else
	lactive = (EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask) != 0);
	if ((xactive -= lactive)) {
		xshield[l.eng_num].xs_vaddr = uvaddr;
		xshield[l.eng_num].xs_eaddr = evaddr;

		actives = hatp->hat_activecpubits;
		if (lactive)
			EMASK_CLRS(&actives, &l.eng_mask);
		ATOMIC_INT_INIT(&xshield[l.eng_num].xs_wait_eng,
				((doL1 && !HAT_PRIVL164_LIST(hatp))
				 || xactive == 1) ?
				NO_WAIT_ENG: EMASK_FLS(&actives));
		ATOMIC_INT_INIT(&xshield[l.eng_num].xs_wait_cnt, xactive);

		(void)TLBS_LOCK();
		if(doL1) {
			TLBS_CPU_SIGNAL(&actives, pae_hat_uas_xshield_l1,
					&xshield[l.eng_num]);
		} else {
			TLBS_CPU_SIGNAL(&actives, pae_hat_uas_xshield_l2,
					&xshield[l.eng_num]);
		}			
		/*
		 * All cpus signalled have now responded to the xcall,
		 * so they are in kernel mode, but might not yet have
		 * have called their hat_uas_xshield() function.
		 */
		ATOMIC_INT_WRITE(&xshield[l.eng_num].xs_wait_eng,
				 NO_WAIT_ENG);
		TLBS_UNLOCK(VM_HAT_RESOURCE_IPL);

		/*
		 * We must wait here because other cpus may have been
		 * held up (by SMM?), and still be updating page table
		 * entries to which we shall want to assume sole access,
		 * and still referring to the structure on our stack.
		 * But perhaps this wait could somehow be avoided?
		 */
		while (ATOMIC_INT_READ(&xshield[l.eng_num].xs_wait_cnt) != 0)
			;/* spin */
	}
#endif /* ndef UNIPROC */

	if (lactive && !doL1) {
		ptep->pte32.pg_low &= ~PG_V;
		flushtlb1(uvaddr);	/* this is the single L2 pte case */
		return B_FALSE;		/* TLBSflushtlb() not required */
	}

	return (boolean_t)lactive;	/* TLBSflushtlb() may be required */
}

/*
 * void pae_hat_uas_unshield(hat_t *hatp, vaddr_t uvaddr, vaddr_t evaddr)
 *	Restore the valid bit to user address space L1 page directory
 *	entries, if hat_uas_shield() removed them, and they have not
 *	since been wiped by hat_zerol1ptes() or FREE_PT().
 *
 * Calling/Exit State:
 *	The HAT lock is held on entry and remains held on exit.
 *
 * Remarks:
 *	Does not restore the valid bit to any L2 ptes affected by
 *	hat_uas_shield(): the caller should do those if required.
 */
STATIC void
pae_hat_uas_unshield(hat_t *hatp, vaddr_t uvaddr, vaddr_t evaddr)
{
#ifndef UNIPROC
	pte64_t *ptep;
	emask_t actives;
	int eng_num;
 
	ASSERT(HATRL_OWNED(hatp));
	ASSERT(hatp != kas_hatp);
	ASSERT(uvaddr < evaddr);
	ASSERT(evaddr <= uvend);

	/*
	 * If inactive, we didn't invalidate L1 pdes.
	 */
	if (hatp->hat_activecpucnt == 0)
		return;

	/*
	 * If active only on our engine, we didn't invalidate L1 pdes
	 */
	if (hatp->hat_activecpucnt == 1
	&&  EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask) != 0)
		return;

	/*
	 * But don't check for single page range: we might well want
	 * to change the callers, to shield a large range in one go
	 * (as now), but unshield it page table by page table: then
	 * the first or last call might appear to be for a single
	 * page, but would actually need L1 pde restored.
	 */

	actives = hatp->hat_activecpubits;

	/*
	 * No need for synchronization with other engines
	 */
	while ((eng_num = EMASK_FFSCLR(&actives)) != -1) {
		vaddr_t addr;
		if (eng_num == l.eng_num)
			continue;
		addr = uvaddr & PAE_VPTMASK;
		while(addr < evaddr) {
			ptep = pae_hat_kvtol1ptep64(addr, pae_ptnum(addr),
						    eng_num);
			if (ptep->pg_pte != 0)
				ptep->pte32.pg_low |= PG_V;
			addr += PAE_VPTSIZE;
		}
	}
#endif /* ndef UNIPROC */
}

#ifdef DEBUG

extern void hat_log(struct as *, struct page *, vaddr_t, 
			vaddr_t, uint_t, char *, ulong_t, int);

#define	HAT_LOG(as, ptpp, addr, endaddr, flags, func, lckcnt) \
		hat_log((as), (ptpp), (addr), (endaddr), (flags), (func), (lckcnt), __LINE__)


/*
 * void
 * pae_hat_checkptinit(void *pdtep, page_t *ptpp)
 *	Debugging subroutine for checking page table initialization.
 *
 * Calling/Exit State:
 *	Page table body and the mapping chunk pointer array should be zero.
 *
 * Remarks:
 *	Should not be called with PSE type ptap.
 */
/* ARGSUSED */
void
pae_hat_checkptinit(void *pdtep, page_t *ptpp)
{
	pte64_t *ptep, tpte;
	int i;
	hatpgt64_t *pt;

	ASSERT(ptpp);

	if (pae_hatdb_ptfreeck == 0)
		return;
	DISABLE();
	/*
	 * Create temporary mapping to the page and then check for
	 * the validity of ptes. Note, it is invalid to check if
	 * we are in context and then do the checks, because the
	 * the virtual address may already be reallocated within
	 * the same process.
	 */
	tpte.pg_pte = 0;
	MAPPT64(KVTMPPT2, ptpp, &tpte);
	pt = (hatpgt64_t *)KVTMPPT2;
	ptep = pt->hat_pte;
	for (i = 0; i < PAE_HAT_EPPT; ptep++, i++) {
		ASSERT(ptep->pg_pte == 0);
	}
	ASSERT(HATPT_AEC(ptpp) == 0);
	ASSERT(HATPT_LOCKS(ptpp) == 0);
	UNMAPPT64((vaddr_t)KVTMPPT2, &tpte);
	ENABLE();
}

/*
 * void
 * pae_hat_checkptcnt(void *pdtep, page_t *ptpp)
 *	Debugging subroutine for checking page table active entry count.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
pae_hat_checkptcnt(page_t *ptpp)
{
	pte64_t *ptep, tpte;
	int i;
	hatpgt64_t *pt;
	uint_t aec = 0;
	uint_t lckcnt = 0;

	ASSERT(ptpp);

	DISABLE();
	tpte.pg_pte = 0;
	MAPPT64(KVTMPPT2, ptpp, &tpte);
	pt = (hatpgt64_t *)KVTMPPT2;
	ptep = pt->hat_pte;
	for (i = 0; i < PAE_HAT_EPPT; ptep++, i++) {
		if (ptep->pg_pte != 0)
			aec++;
		if (ptep->pg_pte & PG_LOCK)
			lckcnt++;
	}
	if (HATPT_AEC(ptpp) != aec)
		/*
		 *+ Mismatch in active page table entries.
		 */
		cmn_err(CE_PANIC,
			"pae_hat_checkptcnt: hatpt_aec=%x, aec=%x\n",
			HATPT_AEC(ptpp), aec);

	if (HATPT_LOCKS(ptpp) != lckcnt)
		/*
		 *+ Mismatch in active page table entries.
		 */
		cmn_err(CE_PANIC,
			"pae_hat_checkptcnt: hatpt_locks=%x, lckcnt=%x\n",
			HATPT_LOCKS(ptpp), lckcnt);


	UNMAPPT64((vaddr_t)KVTMPPT2, &tpte);
	ENABLE();
}

#else

#define	HAT_LOG(as, ptpp, addr, endaddr, flags, func, lckcnt)
#define pae_hat_checkptinit(XXX, YYY)
#define	pae_hat_checkptcnt(XXX)

#endif	/* DEBUG */

/*
 * The following variables are maintained by hat_online/hat_offline. 
 */
extern int maxonlinecpu;		/* to limit searches in TLB code */
extern int minonlinecpu;		/* to limit searches in TLB code */


/*
 * boolean_t
 * pae_hat_remptel(struct as *as, vaddr_t addr, page_t *pp, uint_t dontneed)
 *	Internal hat subroutine to remove a PTE from the mapping
 *	linked list for its page.  This must be a visible mapping.
 *	When a mapping is found, the translation's modify bit
 *	is ORed into the page structure and the page is freed
 *	if this is the last mapping.
 *
 * Calling/Exit State:
 *	The caller must own the hat_resourcelock.
 *	The page's p_vpglock must also be held.
 *	Because of the lock order of these two locks, the caller must
 *	do the LOCKing.
 *
 *	Both the locks are held on return.
 *
 *	Returns B_TRUE if the page is freed, else returns B_FALSE. 
 *	The caller is responsible for syncing the pte's mod bit into
 *	the page structure.
 */
STATIC boolean_t
pae_hat_remptel(struct as *as, vaddr_t addr, page_t *pp, uint_t dontneed)
{
	ASSERT(PAGE_VPGLOCK_OWNED(pp));

	if (as == &kas) {
		PAGE_KAS_UNLOAD(addr, pp);
	} else {
		PAGE_UAS_UNLOAD(as, addr, pp);
	}

	if (!IS_VALID_REPLICA(pp) && !PAGE_IN_USE(pp)) {
		ASSERT(pp->p_hat_refcnt == 0);
		if (as != &kas)
			hat_uas_shootdown_l(&as->a_hat);
		page_free(pp, dontneed);
		return B_TRUE;
	}

	return B_FALSE;
}

/*
 * MACRO
 * PRIVL1_LOAD(hat_t *hatp, pte64_t *pdep, int ptndx, int pdptndx)
 *	Set the level 1 pte of a process with a dedicated L1 page table. 
 *
 * Calling/Exit State:
 *	None.
 */
#define	PRIVL1_LOAD(hatp, pdep, ptndx, pdptndx) { \
		hatprivl164_t *hpl1; \
		pte64_t *ptep; \
					\
		hpl1 = HAT_PRIVL164_LIST((hatp)); \
		ASSERT(hpl1); \
		ptep = hpl1->hp_privl1virt[pdptndx] + (ptndx); \
		pae_setpte(pdep, ptep);	/* ptep->pg_pte = (pdep)->pg_pte; */ \
		ASSERT(pdep->pg_pte == ptep->pg_pte); \
}

/*
 * MACRO
 * PRIVL1_UNLOAD(hat_t *hatp, int ptndx, int pdptndx)
 *	Clear the level 1 pte of a process with a dedicated L1 page table. 
 *
 * Calling/Exit State:
 *	None.
 */
#define	PRIVL1_UNLOAD(hatp, ptndx, pdptndx) { \
		hatprivl164_t *hpl1; \
		pte64_t *ptep; \
					\
		hpl1 = HAT_PRIVL164_LIST((hatp)); \
		ASSERT(hpl1); \
		ptep = hpl1->hp_privl1virt[pdptndx] + (ptndx); \
		pae_clrpte(ptep);	/*  ptep->pg_pte = 0; */ \
		ASSERT(ptep->pg_pte == 0); \
}

/*
 * void
 * pae_hat_kas_zerol1ptes(vaddr_t addr, pte64_t *locall1ptep)
 *	Zaps the level 1 ptes for all the engines.
 *
 * Calling/Exit State:
 *	The hat resource lock is held on entry and is still held on exit.
 *
 * Remarks:
 *	Global so that it can be called from pse_hat.c
 */
void
pae_hat_kas_zerol1ptes(vaddr_t addr, pte64_t *locall1ptep)
{
	int i, index, pdptx;
	pte64_t *l1ptep;

	index = locall1ptep - (pte64_t *)pagerounddown((vaddr_t)locall1ptep);
	ASSERT(index < PAE_NPGPT);
	pdptx = PDPTNDX64(addr);

	for (i = 0; i < Nengine; i++) {
		l1ptep = &engine[i].e_local_pae->pp_kl1pt[pdptx][index];
		pae_clrpte(l1ptep);
	}
}

/*
 * void
 * pae_hat_zerol1pte(hat_t *hatp, pte64_t *locall1ptep)
 *	Zaps a level 1 pte for all active engines.
 *
 * Calling/Exit State:
 *	The hat resource lock is held on entry and is still held on exit.
 *
 * Remarks:
 *	Global so that it can be called from pse_hat.c
 */
void
pae_hat_zerol1pte(hat_t *hatp, vaddr_t addr, pte64_t *locall1ptep)
{
	int i, index, pdptndx;
	hatprivl164_t *hpl1p;

	ASSERT(HATRL_OWNED(hatp));

	index = locall1ptep - (pte64_t *)pagerounddown((vaddr_t)locall1ptep);
	ASSERT(index < PAE_NPGPT);
	pdptndx = PDPTNDX64(addr);

	/*
	 * If there's a private L1 for this address space, then zero out
	 * the entries in the private L1.
	 */
        if ((hpl1p = HAT_PRIVL164_LIST(hatp)) != (hatprivl164_t *)NULL) {
		if (pdptndx < (PDPTSZ - 1)) {
			/*
			 * If the L1 entry was in one of the first three
			 * L1s, then just return after zeroing it.  The
			 * first three L1s are shared, so the change will
			 * be visible to all cpus active in this address
			 * space.
			 */
			PRIVL1_UNLOAD(hatp, index, pdptndx);
			return;
		} else if (!hpl1p->hp_kl1busy) {
			/*
			 * The fourth L1 (if present) of a private L1 isn't
			 * shared, so we have to fall through to the loop
			 * below which zeros out the entry on all cpus
			 * active in the address space.
			 */
			PRIVL1_UNLOAD(hatp, index, pdptndx);
        	}
        }

	/*
	 * No need to zero out loaded copies if the address space isn't
	 * active on any cpu
	 */
	if (hatp->hat_activecpucnt == 0)
		return;

	/*
	 * Zero out loaded copies of the L1 on all cpus except this one.
	 *
	 * Races on minonlinecpu and maxonlinecpu are benign.
	 */
	for (i = minonlinecpu; i <= maxonlinecpu; i++) {
		if (i == l.eng_num)
			continue;
		if (EMASK_TEST1(&hatp->hat_activecpubits, i)) {
			pae_clrpte(pae_hat_kvtol1ptep64(addr, index, i));
		}
	}
}

/*
 * void
 * pae_hat_kas_l1pteload(vaddr_t addr, pte64_t *locall1ptep, uint_t cnt)
 *	Reloads the level 1 ptes zapped earlier; copies the l1pte from the
 *	current engine to other engines' page directories.
 *
 * Calling/Exit State:
 *	The hat resource lock is held on entry and is still held on exit.
 *
 * Remarks:
 *	Global so that it can be called from pse_hat.c
 */
void
pae_hat_kas_l1pteload(vaddr_t addr, pte64_t *locall1ptep) 
{
	int i, index, pdptx;
	pte64_t *l1ptep;

	index = locall1ptep - (pte64_t *)pagerounddown((vaddr_t)locall1ptep);
	pdptx = PDPTNDX64(addr);

	for (i = 0; i < Nengine; i++) {
		l1ptep = &engine[i].e_local_pae->pp_kl1pt[pdptx][index];
		pae_setpte(locall1ptep, l1ptep);
	}
}

/*
 * STATIC pte64_t *
 * pae_hat_findfirstpt_l1(hat_t *hatp, vaddr_t *addrp, vaddr_t endaddr)
 *	Find the first non-zero pte in the address range <*addrp, endaddr>.
 *
 * Calling/Exit State:
 *	hat resource lock is held.
 */
STATIC pte64_t *
pae_hat_findfirstpt_l1(pte64_t *privl1virt, vaddr_t *addrp, vaddr_t endaddr)
{
	pte64_t *hpl1ep, *ehpl1ep;
#ifdef DEBUG
	vaddr_t savaddr = *addrp;
#endif

	hpl1ep = privl1virt + pae_ptnum(*addrp);
	ehpl1ep = privl1virt + pae_ptnum(endaddr - 1);

	while (hpl1ep <= ehpl1ep) {
		if (hpl1ep->pg_pte != 0)
			return (pte64_t *)hpl1ep;
		if (hpl1ep < ehpl1ep)
			*addrp = (((vaddr_t)*addrp + PAE_VPTSIZE) & PAE_VPTMASK);
		else
			/* no page table in the range */
			return (pte64_t *)NULL;
		hpl1ep++;
	}

	ASSERT(*addrp >= savaddr);
	return hpl1ep;
}

/*
 * MACRO pte64_t *
 * pae_hat_findpt_l1(pte64_t *privl1virt, vaddr_t addr)
 *	Find the page directory entry corresponding to the addr.
 *
 * Calling/Exit State:
 *	hat resource lock is held.
 */
#define	pae_hat_findpt_l1(privl1virt, addr) \
		((privl1virt) + pae_ptnum((addr)))

/*
 * MACRO
 * FREE_PT_L1(struct as *as, pte64_t *pdtep, page_t *ptpp)
 *
 * Calling/Exit State:
 *	None.
 */
#define	FREE_PT_L1(as, pdtep, ptpp) { \
	ASSERT(HATPT_LOCKS(ptpp) == 0); \
	if ((as) == u.u_procp->p_as) \
		pae_clrpte(pdtep);	/* pdtep->pg_pte = 0; */ \
	hat_pgfree((ptpp)); \
}

/*
 * STATIC void
 * pae_hat_l1pteload(hat_t *hatp, int ptndx, pte64_t *ptep)
 *	Load a level 1 pte for the specified hat.
 *              hatp = pointer to hat
 *		ptndx = index of level 1 pte
 *		ptep = pte value pointer to be loaded
 *
 * Calling/Exit State:
 *	The hat resource lock is held on entry and is still held on exit.
 *
 * Description:
 *	Load the level 1 pte in the private level 1 page table, if any,
 *	and also in the level 1 page table of any processors which have
 *	this address space loaded.
 */
STATIC void
pae_hat_l1pteload(hat_t *hatp, vaddr_t addr, pte64_t *l1ptep)
{
        int i;
	int pdptndx, ptndx;
	hatprivl164_t *hpl1p;

        ASSERT(HATRL_OWNED(hatp));

	pdptndx = PDPTNDX64(addr);
	ptndx = pae_ptnum(addr);

	/*
	 * Check to see if there's a private L1 for this address space.
	 * If there isn't a private L1, then we just fall through to
	 * the loop below to load the level 1 pte on active CPUs.
	 */
        if ((hpl1p = HAT_PRIVL164_LIST(hatp)) != (hatprivl164_t *)NULL) {
		if (pdptndx < (PDPTSZ - 1)) {
	        	/*
		         * There's a private L1 for this address space, and
			 * the L1 entry to be loaded is in the first three
			 * page directories, so just change the private L1
			 * and return.  The change will be visible to all
			 * cpus active in this address space, since the
			 * first three L1s of the private L1s are shared.
			 */
			PRIVL1_LOAD(hatp, l1ptep, ptndx, pdptndx);
			return;
		} else if (!hpl1p->hp_kl1busy) {
			/*
			 * There's a private L1 for this address space, the
			 * L1 entry to be loaded is in the last page
			 * directory, and the last page directory is not
			 * loaded on any CPU, so change the private page
			 * directory and then fall through to the loop
			 * below to load the L1 on any cpus active in this
			 * address space.
			 *
			 * Note that if the last page directory is loaded on
			 * a CPU, then we skip this part, and just go to the
			 * loop below to load the L1 on cpus active in this
			 * address space.
			 */
			PRIVL1_LOAD(hatp, l1ptep, ptndx, pdptndx);
		}
        }

	/*
	 * If there are CPUs active in this address space, then load
	 * the l1 pte for all active CPUs, including the calling CPU.
	 */
	if (hatp->hat_activecpucnt != 0) {
		for (i = minonlinecpu; i <= maxonlinecpu; i++) {
			if (EMASK_TEST1(&hatp->hat_activecpubits, i)) {
				ASSERT(pae_hat_kvtol1ptep64(addr, ptndx, i)->pg_pte == 0 || pae_hat_kvtol1ptep64(addr, ptndx, i)->pg_pte == l1ptep->pg_pte);
				pae_setpte(l1ptep, pae_hat_kvtol1ptep64(addr, ptndx, i)); 
			}
		}
	}
}

/*
 * boolean_t
 * pae_hat_prep(struct as *as, vaddr_t *addr, vaddr_t endaddr, uint_t flags,
 *		pte64_t **pdtepp, pte64_t **epdtepp, pte64_t **ptepp, 
 *		page_t **ptppp, vaddr_t *kvaddr)
 *	Compute the address of page directory entry for the 
 *	page. Search the hatpt list to find a page table that
 *	has a matching pde pointer. If it is not found, it means
 *	the page table not present. Allocate and set up the page
 *	table. Then get the page table entry for the page.
 *
 *	It maps the L2 page table into the per-engine address space if
 *	it is an out-of-context access.
 *
 *	Allocate an L2 page table if does not exist.
 *
 *	It loads the L1 pte if the process is active on any of the engines.
 * 
 *	The function does not distinguish between the uas and kas.
 *
 * Calling/Exit State:
 *
 *	pdtepp:
 *	  It contains the reference to an L1 entry corressponding to 
 *	  address withing the range <addr, endaddr>. It is used to zero
 *	  zero L1 pte if the L2 page table is freed in hat_unprep.
 *
 *	  We should probably return the value of the pde. How about if we 
 *	  are in context, then we return the hatpt_pdtep, otherwise we
 *	  return the value of the pde.
 *
 *	epdtepp:
 *	  We should also return the endvpdte to the caller since the 
 *	  virtual for the private L1 are different.
 *
 *	flags:
 *	  HAT_ALLOCPT: allocate a page table if one does not exist. It
 *	  implies that it is load/map operation.
 *
 *	  HAT_CANWAIT: indicating the preference of the caller to sleep for
 *	  page table allocation.
 *
 *	  0: find an absolute match for the address range beginning at <addr>.
 *
 *	  HAT_FIRSTPT: find the first mapped address within the range 
 *	  <addr, endaddr>.
 *
 *	kvaddr:
 *	  It is a hint at which to establish the temporary mapping, but
 *	  pae_hat_prep may choose to assign any virtual address or keep a pool
 *	  virtual addresses to implement a lazy shootdown policy.
 *
 * 	  Ideally hat_prep should transparently keep account of per-engine 
 *	  mappings used for handling page table mappings, but for now the 
 * 	  caller explicitly requests the virtual address at which to setup
 *	  the translation.
 *
 *	  If we do not use the kvaddr, then we must reset it to 0 so that
 *	  pae_hat_unprep may not do an unmap if it is out-of-context.
 *
 *	Locking:
 *	  The hat_resourcelock is held on entry and remains held on exit.
 *
 *	PSE mapping:
 *	  If it is a PSE mapping, then ptepp contains the pde entry and
 *	  pdtepp and ptppp are null.
 *
 *	B_FALSE return value:
 *	  If addr is greater than endaddr. ptepp, pdepp are set to NULL.  
 *
 *	  If HAT_ALLOCPT is specified and HAT_CANWAIT is not specified, then
 *	  it returns B_FALSE if page table was not successfully allocated.
 *
 * TODO:
 *	Remove epdtep. It is not being used anymore.
 */
boolean_t
pae_hat_prep(struct as *as, vaddr_t *addr, vaddr_t endaddr, uint_t flags,
		pte64_t **pdtepp, pte64_t **epdtepp, pte64_t **ptepp, 
		page_t **ptppp, vaddr_t *kvaddr, vaddr_t *savaddr)
{
	hatpt_t *ptap, *eptap, *newptap, *savptap;
	hat_t *hatp;
	pte64_t *vpdte, *evpdte, *privl1virt;
	boolean_t inctx;
	page_t *newptpp, *ptpp;
	uint_t pndx, pdptndx, epdptndx, npdptes, ptlen;
	pte64_t *hpl1ep, newpde;
	vaddr_t ptendaddr;
	hatprivl164_t *hpl1p;
	 
	newptap = (hatpt_t *)NULL;
	newptpp = (page_t *)NULL;
	*ptppp = (page_t *)NULL;
	*pdtepp = (pte64_t *)NULL;
	*epdtepp = (pte64_t *)NULL;
	*ptepp = (pte64_t *)NULL;
	newpde.pg_pte = 0;

	if (*addr >= endaddr)
		return B_FALSE;

	hatp = &as->a_hat;
	vpdte = KPDTE(*addr);
	evpdte = KPDTE(endaddr - 1);

tryagain:
	inctx = (u.u_procp && u.u_procp->p_as == as &&
		EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask));
	ASSERT(HATRL_OWNED(hatp));

	if ((hpl1p = HAT_PRIVL164_LIST(hatp)) != NULL) {
		ASSERT(hatp->hat_pts == (hatpt_t *)NULL);
		ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);

		pdptndx = PDPTNDX64(*addr);
		epdptndx = PDPTNDX64(endaddr - 1); 
		npdptes = epdptndx - pdptndx;
		if (pdptndx < (PDPTSZ - 1)) {
			ptendaddr = (pdptndx + 1) * pae_ptbltob(PAE_NPGPT);
			privl1virt = hpl1p->hp_privl1virt[pdptndx];
		} else {
			ptendaddr = uvend;
			if (inctx)
				privl1virt = l.kl1ptp64[pdptndx];
			else
				privl1virt = hpl1p->hp_privl1virt[pdptndx];
		}

		/*
		 * TODO: if an L1 is not allocated then allocate one.
		 * For now all L1's are allocated when we switch to
		 * a private L1 page table.
		 */
		if (flags & HAT_FIRSTPT) {
			if (npdptes) {
				do {
					if (endaddr > ptendaddr)
						ptlen = ptendaddr - *addr;
					else
						ptlen = endaddr - *addr;
					hpl1ep = pae_hat_findfirstpt_l1(
						privl1virt, addr, *addr+ptlen);
					if (hpl1ep != NULL)
						break;
					pdptndx = PDPTNDX(*addr);
					if ((pdptndx < (PDPTSZ - 1)) || !inctx)
						privl1virt =
						hpl1p->hp_privl1virt[pdptndx];
					else
						privl1virt =
							l.kl1ptp64[pdptndx];
					ASSERT(uvend > pae_ptbltob(PAE_NPGPT));
					if (ptendaddr <= (uvend -
							pae_ptbltob(PAE_NPGPT)))
						ptendaddr +=
							pae_ptbltob(PAE_NPGPT);
					else
						ptendaddr = uvend;
				} while (npdptes-- != 0);
			} else {
				hpl1ep = pae_hat_findfirstpt_l1(privl1virt, addr, endaddr);
			}
		} else {
			hpl1ep = pae_hat_findpt_l1(privl1virt, *addr);
		}

		if (hpl1ep && hpl1ep->pg_pte != 0) {
			if (PG_ISPSE(hpl1ep)) {
				*ptepp = hpl1ep;
				*savaddr = *addr;
				return B_TRUE;
			}
#ifdef DEBUG
			if (inctx) {
				vpdte = KPDTE(*addr);
				if (PFN64(vpdte) != PFN64(hpl1ep))
					/*
					 *+ Mismatched page frame numbers. 
					 *+ This should never happen while
					 *+ in context.	
					 */
					cmn_err(CE_PANIC, 
						"vpdte=%x hpl1ep=%x pte=%Lx, hp_pte=%Lx\n", 
						vpdte, hpl1ep, vpdte->pg_pte, hpl1ep->pg_pte);
				ASSERT(PFN64(vpdte) == PFN64(hpl1ep));
			}
#endif /* DEBUG */
			*ptppp = page_numtopp(PFN64(hpl1ep));
			*pdtepp = hpl1ep;
			ASSERT(*ptppp);
			goto found_ptap;
		}

		if (hpl1ep && hpl1ep->pg_pte == 0 && !(flags & HAT_ALLOCPT)) {
			if (!(flags & HAT_ALLOCPSE))
				return B_FALSE;
			*savaddr = *addr;
			*ptepp = hpl1ep;
			return B_TRUE;
		}

		if (!hpl1ep && !(flags & HAT_ALLOCPT))
			return B_FALSE;

		ASSERT(flags & HAT_ALLOCPT);
		if (newptpp == (page_t *)NULL) {
			newptpp = hat_pgalloc(HAT_POOLONLY);
			if (newptpp)
				goto new_pt; 

			if ((flags & HAT_CANWAIT) == 0)
				return B_FALSE;

			/*
			 * Back out of locks and allocate a page table.
			 * Then back to square 1.
			 */
			HATRL_UNLOCK_SVDPL(hatp);
			newptpp = hat_pgalloc(HAT_CANWAIT);
			ASSERT(newptpp);
			HATRL_LOCK_SVPL(hatp);
			goto tryagain;
		}
new_pt:
		ASSERT(newptpp);
		ptpp = newptpp;
		newptpp = (page_t *)NULL;
		newpde.pg_pte = pae_mkpte(PTE_RW|PG_V, page_pptonum(ptpp));
		pae_hat_l1pteload(hatp, *addr, &newpde);
		*ptppp = ptpp;
		ASSERT(hpl1ep->pg_pte == newpde.pg_pte);
		*pdtepp = hpl1ep;
		goto found_ptpp;
	} /* end if hat_privl1_list */

	ASSERT(HAT_PRIVL164_LIST(hatp) == (hatprivl164_t *)NULL);
	*epdtepp = evpdte;

	/*
	 * Find the page table corresponding to the L1 entry. If the
	 * ptap is NULL or it is the prevptap for link_ptap() and the
	 * desired entry is absent or it is the first ptap beginning
	 * at addr.
	 */
	if (flags & HAT_FIRSTPT) {
		ptap = pae_hat_findfirstpt(addr, hatp);
		if (*addr >= endaddr)
			return B_FALSE;
	} else if (flags == 0) {
		eptap = ptap = hatp->hat_ptlast;
		if (ptap == (hatpt_t *)NULL)
			return B_FALSE;
		for (; ptap->hatpt_pdtep != vpdte;) {
			ptap = ptap->hatpt_forw;
			if (ptap == eptap)
				return B_FALSE;
		}
	} else {
		ASSERT(flags & HAT_ALLOCPT);
		ptap = pae_hat_findpt(hatp, vpdte);
	}

	if (ptap && ((ptap->hatpt_pdtep == vpdte) || !(flags & HAT_ALLOCPT))) {
		if (PG_ISPSE(&ptap->hatpt_pde)) {
			*ptepp = &ptap->hatpt_pde;
			*savaddr = *addr;
			return B_TRUE;
		}
#ifdef DEBUG
		if (inctx) {
			vpdte = KPDTE(*addr);
			if (PFN64(vpdte) != PFN64(&ptap->hatpt_pde))
				/*
				 *+ Mismatched page frame numbers. This
				 *+ should never happen while in context.	
				 */
				cmn_err(CE_PANIC, 
				"vpdte=%x pdtep=%x, pte=%Lx, ptap_pte=%Lx\n", 
				vpdte, ptap->hatpt_pdtep, vpdte->pg_pte, ptap->hatpt_pdtep->pg_pte);
			ASSERT(PFN64(vpdte) == PFN64(&ptap->hatpt_pde));
		}
#endif /* DEBUG */
		ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
		*ptppp = ptap->hatpt_ptpp;
		*pdtepp = ptap->hatpt_pdtep;
		ASSERT(*pdtepp >= KPDTE(0));
		goto found_ptap;
	}

	if (!ptap && !(flags & HAT_ALLOCPT))
		return B_FALSE;

	ASSERT(flags & HAT_ALLOCPT);
	savptap = ptap;
	if (newptap == (hatpt_t *) NULL) {
		newptap = hat_ptalloc(HAT_POOLONLY);
		if (newptap)
			goto new_ptap;
		if ((flags & HAT_CANWAIT) == 0)
			return B_FALSE;

		HATRL_UNLOCK_SVDPL(hatp);
		newptap = hat_ptalloc(HAT_CANWAIT);
		ASSERT(newptap);
		HATRL_LOCK_SVPL(hatp);
		goto tryagain;
	}

	/*
	 * We will reach here if we have allocated a page table page.
	 */
new_ptap:
	ASSERT(newptap);
	ptap = newptap;
	newptap = (hatpt_t *)NULL;
	ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
	ptap->hatpt_pdtep = vpdte;
	/* Load the new L2 page table in the L1 page directory table */
	hat_link_ptap(as, savptap, ptap);
	hatp->hat_ptlast = ptap;
	*ptppp = ptap->hatpt_ptpp;
	*pdtepp = ptap->hatpt_pdtep;
	pae_hat_l1pteload(hatp, *addr, &ptap->hatpt_pde);

found_ptap:
	if (newptap)
		hat_ptfree(newptap);
found_ptpp:
	if (newptpp)
		hat_pgfree(newptpp);
	ASSERT(*ptppp);
	ASSERT(*pdtepp);
	pndx = pae_pnum(*addr);
	*savaddr = *addr;
	/* set ptep */
	SETPTRS64(pndx, *ptepp, *addr, *ptppp, *kvaddr, inctx);
	ASSERT(*ptepp);
	return B_TRUE;
}

/*
 * void
 * pae_hat_unprep(struct as *as, page_t **ptppp, pte64_t *pdtep, vaddr_t kvaddr)
 *
 * Calling/Exit State: 
 *	pdtep:
 *	  zero the pdtep in the hatpt chains/private L1 entry if the
 *	  active entry count is zero.
 *
 *	kvaddr:
 *	  unmap a temporary mapping if it is an out-of-context access.
 */
void
pae_hat_unprep(struct as *as, vaddr_t addr, page_t **ptppp, 
		pte64_t *pdtep, vaddr_t kvaddr)
{
	hat_t *hatp;
	int flush = 0;
	boolean_t inctx;
	pte64_t tpte;

	hatp = &as->a_hat;
	inctx = (u.u_procp && u.u_procp->p_as == as &&
		EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask));
	ASSERT(HATRL_OWNED(hatp));

	if (HAT_PRIVL164_LIST(hatp)) {
		ASSERT(hatp->hat_pts == (hatpt_t *)NULL);
		ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);
		if (*ptppp && HATPT_AEC(*ptppp) == 0) {
			ASSERT(HATPT_LOCKS(*ptppp) == 0);
			HAT_LOG(as, *ptppp, addr, addr + PAGESIZE, 
				0, "pae_hat_unprep", HATPT_LOCKS(*ptppp));
			pae_hat_checkptinit(pdtep, *ptppp);
			pae_hat_zerol1pte(hatp, addr, pdtep);
			flush |= hat_uas_shootdown_l(hatp);
			/* zero the pde and free the page table page */
			FREE_PT_L1(as, pdtep, *ptppp);
			*ptppp = (page_t *)NULL;
		}
		goto done;
	}

	/*
	 * It is possible that a page table is freed multiple times because
	 * of the generality of the interface. When a page table is freed
	 * its hatpt_as is set to NULL, and the check is added to ensure
	 * that ptap is not already freed and has a valid "as" associated.
	 */
	if (*ptppp && HATPT_AEC(*ptppp) == 0 && HATPT_PTAP(*ptppp)->hatpt_as == as) {
		ASSERT(HAT_PRIVL164_LIST(hatp) == (hatprivl164_t *)NULL);
		ASSERT(HATPT_LOCKS(*ptppp) == 0);
		ASSERT(HATPT_PTAP(*ptppp)->hatpt_pdtep == pdtep);
		ASSERT(KPDTE(addr) == pdtep);
		HAT_LOG(as, *ptppp, addr, addr + PAGESIZE, 
				0, "pae_hat_unprep", HATPT_LOCKS(*ptppp));
		pae_hat_checkptinit(pdtep, *ptppp);
		pae_hat_zerol1pte(hatp, addr, pdtep);
		flush |= hat_uas_shootdown_l(hatp);
		/* unlink and free the page table page */
		FREE_PT(HATPT_PTAP(*ptppp), as);
		*ptppp = (page_t *)NULL;
		goto done;
	}

	ASSERT(*ptppp == NULL || 
		(HATPT_LOCKS(*ptppp) != 0 || HATPT_AEC(*ptppp) != 0));

done:
	if (!inctx) {
		ASSERT(kvaddr);
		tpte.pg_pte = 0;
		UNMAPPT64(kvaddr, &tpte);
	}

	if (flush)
		TLBSflushtlb();
}

/*
 * STATIC int
 * pae_hat_loadpte(struct as *as, pte64_t *ptep, uint_t mode, page_t *ptpp,
 *		page_t *pp, uint_t pf, vaddr_t addr, uint_t flag)
 *	Load the ptep with the pf page frame number and mode protections.
 *	Also adjust all the accounting state.
 *
 * Calling/Exit State:
 *	The hat resource lock and the vpage lock are held across the function.
 *
 *	On success, returns non-zero. On failure, returns zero.
 */
STATIC int
pae_hat_loadpte(struct as *as, pte64_t *ptep, uint_t mode, page_t *ptpp,
		page_t *pp, uint_t pf, vaddr_t addr, uint_t flag)
{
	pte64_t tpte;
#ifdef CCNUMA
	page_t *opp;
	hat_t *hatp = &as->a_hat;
#endif

	if (ptep->pg_pte != 0) {
		ASSERT(HATPT_AEC(ptpp));

		if ((uint_t)PFN64(ptep) == pf) {
			/*
			 * When redoing a PTE, we never downgrade permissions.
			 * Therefore, shootdown is never necessary.
			 */
#ifdef	UNSAFE_ASSERT
			ASSERT(!HAT_REDUCING_PERM(ptep->pg_pte, mode));
			ASSERT((ptep->pg_pte & ~mode & (PTE_PROTMASK|PG_V)) == 0);
#endif
			if ((ptep->pg_pte & mode) != mode) {
				/*
				 * Or in new mode bits without changing any
				 * other bits.
				 */
#ifdef	UNSAFE_ASSERT
				ASSERT((mode & ~(PTE_PROTMASK|PG_V)) == 0);
#endif
				atomic_or(&ptep->pte32.pg_low, mode);
				ASSERT(ptep->pg_pte != 0);
			}
			if (flag & HAT_LOCK) {
				if ((ptep->pg_pte & PG_LOCK) == 0) {
					INCR_LOCK_COUNT(ptpp, as);
					ptep->pg_pte |= PG_LOCK;
				}
			}
		} else
#ifdef CCNUMA
		{
			/*
			 * May be a replica page from another CG is
			 * mapped here. If so rewrite the pte and
			 * adjust the mapcount on both pages.
			 */
			opp = pteptopp(ptep);
			if (opp->p_vnode != pp->p_vnode ||
			    opp->p_offset != pp->p_offset) {
				/*
				 *+ Trying to change existing mapping.
				 */
				cmn_err(CE_PANIC,
					"pae_hat_loadpte - changing existing mapping:pte = %Lx\n",
					ptep->pg_pte);
				/* NOTREACHED */
			}
			ASSERT(opp->p_offset == pp->p_offset);
			/*
			 * Do not reload the page frame if the page
			 * is already locked.
			 */
			if (ptep->pg_pte & PG_LOCK) {
				if ((ptep->pg_pte & mode) != mode) {
					/*
					 * Or in new mode bits without changing
					 * any other bits.
					 */
					atomic_or(&ptep->pte32.pg_low, mode);
					ASSERT(ptep->pg_pte != 0);
				}
				return 0;
			}
			/*
			 * Replicated page is being loaded. Decrement the aec
			 * and rss, since we are overlaying the translation
			 * with the cloned page.
			 */
			PAGE_UAS_UNLOAD(as, addr, opp);
			if (ptep->pg_pte & PG_WASREF) {
				DECR_PAGE_REFCNT(opp);
			}
			pae_clrpte(ptep);
			BUMP_RSS(as, -1);
			BUMP_AEC(ptpp, -1);
			if (hat_uas_shootdown_l(hatp))
				TLBSflushtlb();
			goto setup_xlation;
                }
#else /* !CCNUMA */
		{
			/* 
			 *+ Trying to change existing mapping. 
			 */
			cmn_err(CE_PANIC,
			  "pae_hat_loadpte - changing existing mapping:pte = %Lx\n",
					ptep->pg_pte);
			/* NOTREACHED */
		}
#endif /* CCNUMA */

		ASSERT(pp == NULL || pteptopp(ptep) == pp);
		return 1;
	}

setup_xlation:

	/* ptep->pg_pte = pae_mkpte(mode, pf); */
	tpte.pg_pte = pae_mkpte(mode, pf);
	pae_setpte(&tpte, ptep);
	ASSERT(ptep->pg_pte != 0 && tpte.pg_pte == ptep->pg_pte);

	if (flag & HAT_LOCK) {
		if ((ptep->pg_pte & PG_LOCK) == 0) {
			INCR_LOCK_COUNT(ptpp, as);
			ptep->pg_pte |= PG_LOCK;
		}
	}

	/*
	 * Increment the map count on the page.
	 */
	if (pp) {
		ASSERT(!(flag & HAT_HIDDEN));
		PAGE_UAS_LOAD(as, addr, pp);
		ptep->pg_pte |= PG_WASREF;
		pp->p_hat_refcnt++;
		BUMP_RSS(as, 1);
	} 
	BUMP_AEC(ptpp, 1);
	return 1;
}
	
/* 
 * int 
 * pae_hat_load(struct seg seg, vaddr_t addr, page_t *pp, uint_t pf,
 *		uint_t prot, uint_t flag)
 *	Does the work for hat_memload() and hat_devload().
 *
 * Calling/Exit State:
 *	There may or may not be a pp, depending on who calls it.
 *	The hat_memload() "Calling/Exit State" comments apply here.
 *	This is a STATIC hat private function. This function may block,
 *	so no spin locks should be held on entry.
 *
 *	This function sets the protections to the "max" of the protections
 *	in the pte and those in the argument ``prot''. Thus, protections
 *	are never downgraded.
 *
 *	/proc calls this function out of context.
 *
 *	PAGE_VPGLOCK() acquired on the page and returned held.
 *
 *	On success, returns non-zero. On failure, returns zero.
 *
 * Description:
 *	The code has been somewhat reorganized and the mapping chunk
 *	allocation primitives have been revised from the uniprocessor
 *	version of the code. The key reason is to optimize lock holding
 *	and lock dropping cases for resource allocation situations.
 *	The page table and mapping chunk allocation cases now have
 *	two levels. First, an attempt is made to allocate without
 *	sleeping to keep from dropping spinlocks. The code eliminates
 *	artificial holds on resources. Artificial holds on resources
 *	would be poorly bounded because of LWPs and the hat code should
 *	not impose an artificial limit on the number of LWPs. The code
 *	actually becomes cleaner as a result of doing all of this
 *	as more code became common code that had been replicated and
 *	the extra code for artificially holding the resources was removed.
 *	Now only special bulk processing that has a guaranteed
 *	single-threaded environment is allowed to artificially hold
 *	resources. Faults are inherently parallel, and hat_pteload()
 *	does the translation work for faults.
 *
 *	In addition, once it is found that a page table is needed,
 *	The code also preallocates a mapping chunk in order to
 *	insure that there is at most only one case of needing to
 *	drop the locks and to insure that there is no interruption
 *	of lock holding once the code starts the setup of the new
 *	page table. This is part of avoiding artificial holds of page
 *	tables. The requirement of a pre-existing readlock on a pp
 *	eliminates the artificial hold of the page.
 *	The existence of the readlock is already checked by an ASSERT
 *	in hat_memload(), so no additional checks are performed here.
 *	The new "preallocation" methods necessitated the revision of
 *	of the mapping chunk allocation/initialization service routines
 *	to allow allocation prior to establishing page table context
 *	needed to initialize the chunks accounting.
 *
 * History:
 *	If loading a read-only translation, then must wait if any LWP 
 *	other than the current context has acquired the HAT_RDONLY_LOCK, 
 *	until that LWP has released it. The current context does not 
 *	actually need to acquire the HAT_RDONLY_LOCK, since it will be 
 *	holding the controlling hat_resourcelock during the loading.
 *
 * TODO:
 *	Handle walking the page list and loading multiple pages at a time.
 */
int
pae_hat_load(struct seg *seg, vaddr_t addr, page_t *pp, uint_t pf,
		size_t npages, uint_t prot, uint_t flag)
{
	hat_t *hatp;
	page_t *ptpp = NULL;
	pte64_t *ptep, *pdtep, *epdtep;
	uint_t mode; 
	uint_t pteflags = 0;
	struct as *as = seg->s_as;
	int num_vpglocks = 0;
	vnode_t *vp;
	vnode_t *vpglocks[HAT_MAX_VPGLOCKS] = {NULL, NULL};
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	vaddr_t savaddr;
	int err;

	ASSERT(getpl() == PLBASE);
	/* addr must be page aligned */
	ASSERT((addr & POFFMASK) == 0);

	if (flag & HAT_STATPT) {
		ASSERT(as == &kas);
		ASSERT(KADDR(addr));
		if (flag & HAT_HIDDEN)
			(void) pae_hat_statpt_devload(addr, npages, pf, prot);
		else
			(void) pae_hat_statpt_memload(addr, npages, pp, prot);
		return 1;
	}

	if (as == &kas) {
		pae_hat_kas_memload(addr, pp, prot);
		return 1;
	}

	if (flag & HAT_HIDDEN) {
		/*
		 * Disable caching for anything outside of the
		 * mainstore memory.
		 */
		ASSERT(pp == (page_t *)NULL);
		if (!mainstore_memory(mmu_ptob(pf)))
			pteflags |= PG_CD;
	} else {
		ASSERT_PP_IN_RANGE(pp);
		ASSERT(IS_VALID_REPLICA(pp) || PAGE_IS_LOCKED(pp));
		pf = page_pptonum(pp);
#ifdef CC_PARTIAL
		/*
		 * The following section of code is added to treat a
		 * potential covert channel threat in the use of the page
		 * cache to transmit covert information.
		 *
		 * Initially, p_lid is cleared in page_get().
		 * When a page is physically (i/o) read in (sfs_getpageio(),
		 * spec_getpageio()), the level of the calling process
		 * faulting the page in is registered in p_lid.
		 * Anonymous pages are skipped. Shared memory is not an
		 * issue because shared memory operations are only
		 * allowed at a single level (unprivileged use).
		 * If at this time the caller's level is not the same as
		 * the level at which the page was faulted in, count the
		 * event.  Note that once counted, this page need not
		 * be counted again.  The p_lid field is zeroed to
		 * indicate this fact.
		 */
		if (pp->p_lid &&
		    MAC_ACCESS(MACEQUAL, pp->p_lid, CRED()->cr_lid)) {
			pp->p_lid = (lid_t)0;
			CC_COUNT(CC_CACHE_PAGE, CCBITS_CACHE_PAGE);
		}
#endif /* CC_PARTIAL */
	}
		
	/*
	 * kas use of this mechanism is not valid, since seg_map
	 * has special code it is the only visible mapper is kas.
	 */
	ASSERT(as != &kas);

	hatp = &as->a_hat;
	mode = hat_vtop_prot(prot) | pteflags;

tryagain:
	/*
	 * The hat_resourcelock is needed to add a translation.
	 * If pp non-NULL, the PAGE_VPGLOCK also must have been held.
	 * The worst case occurs only when there is a memory bind
	 * and sleeps are required to allocate resources.
	 * When that happens, the resources are "preallocated"
	 * and then processing starts over at this point.
	 */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(CURRENT_LWP() != NULL);

	HATRL_LOCK_SVPL(hatp);

	if (pae_hat_prep(as, &addr, addr + PAGESIZE, HAT_ALLOCPT|HAT_CANWAIT,
			&pdtep, &epdtep, &ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		/*
		 *+ Cannot fail to load the translation, since we wait
		 *+ for the page table page to be available.
		 */
		cmn_err(CE_PANIC,
			"pae_hat_load: Failed to load the translation." );
	}

	ASSERT(pdtep && ptep && ptpp);

	/*
	 * Acquire vpglock trylock. If we could not acquire then
	 * goto tryagain, but first free the page table by calling
	 * pae_hat_unprep().
	 */
	if (pp) {
		vp = pp->p_vnode;
		ASSERT(vp != NULL);
		ASSERT(VN_IS_HELD(vp));
		if (!HAT_VPGLOCK_EXISTS(vp, vpglocks) &&
		    !HAT_TRYVPGLOCK(vp, num_vpglocks, vpglocks)) {
			pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
			HATRL_UNLOCK_SVDPL(hatp);
			goto tryagain;
		}
	}

	err = pae_hat_loadpte(as, ptep, mode, ptpp, pp, pf, addr, flag);

	HAT_LOG(as, ptpp, addr, addr + PAGESIZE, 
			flag, "pae_hat_load", HATPT_LOCKS(ptpp));

	pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

done:
	if (pp) {
		HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
	}
	HATRL_UNLOCK_SVDPL(hatp);

	if (IS_TRIM_NEEDED(as))
		POST_AGING_EVENT_SELF();

	return (err);
}

/*
 * STATIC boolean_t
 * pae_hat_unloadpte(pte64_t *ptep, page_t *ptpp, struct as *as, 
 *		vaddr_t addr, uint_t flags,
 *		int *numvpg_locks, vnode_t **vpglocks, vnode_t **failed_vpp)
 *	Unload a virtual address translation.
 *
 * Calling/Exit State:
 *	The hat resource lock is held by the caller and is returned
 *	held.  However, it may be dropped in the course of processing because
 *	the processing needs page VPGLOCKs and the order is wrong.
 *	Returns true if freed the page (used for stat collection).
 *	Argument numpt is relevant only if doTLBS is B_TRUE. 
 *
 *	This function does not block.
 *
 *	May be called from out of context.
 *
 *	The function is called from hat_unload.
 */
STATIC boolean_t
pae_hat_unloadpte(pte64_t *ptep, page_t *ptpp, struct as *as, 
		vaddr_t addr, uint_t flags,
		int *numvpg_locks, vnode_t **vpglocks, vnode_t **failed_vpp)
{
	page_t *pp = (page_t *)NULL;
	boolean_t ret = B_FALSE;
	uint_t pteval;

	if ((flags & HAT_HIDDEN) == 0) {
		pp = pteptopp(ptep);
		ASSERT(pp != NULL);
		ASSERT(pp->p_vnode != NULL);
		ASSERT(VN_IS_HELD(pp->p_vnode));

		/*
		 * Need the VPGLOCK to go on. See if the vpglock
		 * already exists.
		 */
		if (intrpend(PLBASE) ||
		    (!HAT_VPGLOCK_EXISTS(pp->p_vnode, vpglocks) &&
		     !HAT_TRYVPGLOCK(pp->p_vnode, *numvpg_locks, vpglocks))) {
			*failed_vpp = pp->p_vnode;
			return ret;
		}

		/* finally, reset the pte after all the accounting is done */
		pteval = atomic_fnc(&ptep->pte32.pg_low);
		ptep->pte32.pg_high = 0;

		/*
		 * A historical note -
		 *
		 * p_hat_refcnt used to be potentially inaccurate in the
		 * old model, where the per-page spin lock (the old 
		 * page_uselock) would have been needed to really keep
		 * it accurate; and to avoid the cost of lock trips, we
		 * had chosen to maintain it without accuracy. This lack 
		 * of accuracy in p_hat_refcnt was then made up for by
		 * explicitly sync'ing up the count under the cover of
		 * the spinlock. With locking reductions achieved by 
		 * moving the protection of the page mapping chain up
		 * a level to the vnode layer (i.e., the substitution of
		 * page_uselock by the per-vnode-pagelock, we can maintain
		 * p_hat_refcnt exactly, as is the case in the few lines
		 * below. This removed the need for periodic recomputation
		 * of p_hat_refcnt just for accuracy.
		 */
		if (pteval & PG_WASREF)
			/* clear the was reference bit */
			pp->p_hat_refcnt--;

		if (pteval & PG_M) {
			PAGE_SETMOD(pp);
		}

		/*
		 * Must remove pte from its p_hat_mapping list.
		 */
		if (pae_hat_remptel(as, addr, pp, (flags & HAT_DONTNEED)))
			ret = B_TRUE;

		BUMP_RSS(as, -1);
	} else {
		pteval = atomic_fnc(&ptep->pte32.pg_low);
		ptep->pte32.pg_high = 0;
	}

	ASSERT((flags & HAT_UNLOCK) || ((pteval & PG_LOCK) == 0));

	if (flags & HAT_UNLOCK) {
		if (pteval & PG_LOCK)
			DECR_LOCK_COUNT(ptpp, as);
	}

	BUMP_AEC(ptpp, -1);

	return ret;
}

/*
 ************************************************************************
 * External dynamic pt hat interface routines
 ************************************************************************
 */

/*
 * void
 * pae_hat_pageunload(page_t *pp, struct as *as, vaddr_t addr)
 *	Unload all of the hardware translations that map page pp
 *	visibly, i.e., by appearing on the p_hat_mapping list.
 *
 * Calling/Exit State:
 *	The function is called with the V_PGLOCK spinlock
 *	of the page held (checked by ASSERT).
 *
 *	This lock is never dropped by the function and
 *	is still held on return. This lock provides
 *	permission to access and change the p_hat_mapping list,
 *	which is purged as a natural part of removing
 *	the translations.
 *
 * Description:
 *	This function must also grab the hat_resourcelock spinlock
 *	for the as of each translation in the p_hat_mapping chain.
 *	The hat lock is later in the locking order explicitly
 *	to allow the required semantics of never dropping the VPGLOCK.
 *	To find the AS for the translation, the hat resources must
 *	be accessed (but not changed yet). The VPGLOCK, by
 *	protecting the mapping guarantees that read access is OK
 *	for all resources related to the translation. To change
 *	any of the hat information for a translation (including
 *	all contents of a PTE) both the page VPGLOCK and the hat lock
 *	must be held.
 *
 * TODO:
 *	Should we return a flag indicating a shootdown.
 */
void
pae_hat_pageunload(page_t *pp, struct as *as, vaddr_t addr)
{
	pte64_t *ptep, *pdtep, *epdtep;
	hat_t *hatp = &as->a_hat;
	int flush = 0;
	uint_t pteval;
	struct seg *seg;
	page_t *ptpp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT2;
	vaddr_t savaddr = addr;
	vaddr_t endaddr = addr + PAGESIZE;
#ifdef DEBUG
	int modset = 0;
#endif

	ASSERT_PP_IN_RANGE(pp);
	ASSERT(PAGE_VPGLOCK_OWNED(pp));

	if (as == &kas) {
		/*
		 * It is a visible kernel mapping. Kernel page tables
		 * don't move around so no special tricks needed. Even
		 * the hat resource lock can be skipped (no linked list
		 * to protect and the VPGLOCK serializes what needs to
		 * be serialized).
		 */
		ASSERT(KADDR(addr) && !KADDR_PER_ENG(addr));

#ifndef CCNUMA
		ptep = kvtol2ptep64(addr);
		ASSERT(ptep->pg_pte != 0);
		ASSERT(pteptopp(ptep) == pp);
		pteval = atomic_fnc(&ptep->pte32.pg_low);
#else
		pteval = 0;
		FOR_EACH_CG_PTE(addr)
			ASSERT(cg_ptep->pg_pte != 0);
			ASSERT(pteptopp(cg_ptep) == pp);
			pteval |= atomic_fnc(&cg_ptep->pte32.pg_low);
		END_FOR_EACH_CG_PTE
#endif /* CCNUMA */

#ifdef DEBUG
		modset = ptep->pg_pte & PG_M;
#endif
		ASSERT(pteval != 0);
		if (pteval & PG_M)
			pp->p_mod = 1;

		seg = as_segat(as, addr);  
		ASSERT(seg);
		if (!SOP_LAZY_SHOOTDOWN(seg, addr))
			hat_shootdown((TLBScookie_t)0, HAT_NOCOOKIE);

		PAGE_KAS_UNLOAD(addr, pp);
		return;
	}

	hatp = &as->a_hat;

	HATRL_LOCK_SVPL(hatp);

	if (pae_hat_prep(as, &addr, endaddr, HAT_FIRSTPT, &pdtep, &epdtep,
			&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		ASSERT(pdtep == (pte64_t *)NULL);
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}
	if (ptep->pg_pte == 0 || PFN64(ptep) != pp->p_pfn) {
		pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}
	ASSERT(pdtep && ptep && ptpp);
	ASSERT(ptep->pg_pte != 0);
	ASSERT(savaddr == addr);
	ASSERT(pteptopp(ptep) == pp);

	/*
	 * A virtual address could be calculated here,
	 * but the simpler full flush scheme is being used
	 * for the initial implementation. This does not
	 * require a virtual address. It also works for
	 * newer CPU chips (486, ...) than the 386, making
	 * the hat code intel generic.
	 * Although a mechanism is available on the 386 to
	 * flush individual virtual addresses, it is 386-specific.
	 * A different mechanism is provided on the 486.
	 * NOTE: there is a good side to the full flush.
	 * All flushes count for TLB Shootdown avoidance.
	 */

#ifdef DEBUG
	modset = ptep->pg_pte & PG_M;
#endif
	if (ptep->pg_pte & PG_V) {
		flush |= pae_hat_uas_shield(hatp, addr, addr + PAGESIZE, ptep);
	}
	pteval = ptep->pte32.pg_low;
	ptep->pg_pte = 0;
	
	PAGE_UAS_UNLOAD(as, addr, pp);

	if (pteval & PG_M) {
		PAGE_SETMOD(pp);
		HAT_SET_MODSTATS(hatp, pdtep, pae_pnum(addr));
	}
	if (pteval & PG_LOCK)
		DECR_LOCK_COUNT(ptpp, as);

	if (pteval & PG_WASREF)
		pp->p_hat_refcnt--;

	/*
	 * The hat resource spin lock must protect a_rss.
	 */
	BUMP_RSS(as, -1);
	BUMP_AEC(ptpp, -1);

	pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

	HATRL_UNLOCK_SVDPL(hatp);

	ASSERT(!modset || pp->p_mod);

	if (flush)
		TLBSflush1(addr);
}

#ifndef UNIPROC
/*
 * int
 * pae_hat_fastfault(struct as *as, vaddr_t addr, enum seg_rw rw)
 *	Wait for the hat resource lock.
 *
 * Calling/Exit State:
 *      Called from early in fault processing.
 *      Grabs and release the hat resource lock.
 *
 * Remarks:
 *	UAS L1 or L2 ptes are temporarily made invalid when
 *	hat_uas_shield is used (by pageunload, hat_unload, or
 *      chgprot), then revalidated, while the hat resource lock
 *	is still held. Therefore, it sufficies to grab the hat
 *      resource lock and then release it. However, it is
 *      necessary to check that the access was valid so that
 *      we can still obtain clean kernel panics.
 *
 *	This routine does not have to deal with PSE mappings.
 *	hat_fastfault is called only if the kernel takes a fault
 *	on a valid user address and there are no catch fault flags
 *	in effect.  Specifically, if the page is softlocked or
 *	memory locked, drivers can access this page without
 *	calling CATCH_FAULTS(), which could result in a fault
 *	because of the shootdown optimization of unloading the L1
 *	entries (only in MP kernels).  For PSE segments, L1 entries
 *	are unloaded only when the segment itself is being unmapped.
 *	There should never be a case in which a PSE segment has been
 *	softlocked or memory locked and unloaded.
 */
int
pae_hat_fastfault(struct as *as, vaddr_t addr, enum seg_rw rw)
{
        hat_t *hatp;
        pte64_t *vpdtep, *ptep, *pdtep, *epdtep;
        uint_t mode;
	page_t *ptpp;
	vaddr_t eaddr = addr + PAGESIZE;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	vaddr_t savaddr;

        vpdtep = KPDTE(addr);

        hatp = &as->a_hat;
        mode = ((rw == S_WRITE) ? (PG_V | PG_RW) : PG_V) | PG_LOCK;

        HATRL_LOCK_SVPL(hatp);
	if (pae_hat_prep(as, &addr, eaddr, HAT_FIRSTPT, &pdtep, &epdtep, 
			&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		ASSERT(pdtep == (pte64_t *)NULL);
                HATRL_UNLOCK_SVDPL(hatp);
                return(0);
        }
        if ((ptep->pg_pte & mode) == mode) {
		pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
                HATRL_UNLOCK_SVDPL(hatp);
                return(1);
        }
	pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
        HATRL_UNLOCK_SVDPL(hatp);
        return(0);

}
#endif /* UNIPROC */

/*
 * void
 * pae_hat_pagesyncmod(page_t *pp, struct as *as, vaddr_t addr)
 *	Sync all visible modify bits back to the page structure,
 *	while clearing them in the PTEs.
 *
 * Calling/Exit State:
 *	The function is called with the VPGLOCK spinlock
 *	of the page held (checked by ASSERT).
 *
 *	This lock is never dropped by the function and
 *	is still held on return. This lock provides
 *	permission to access the p_hat_mapping list, and part of the
 *	permission to change the PTEs (clearing mod bits).
 *	This function is key in establishing a commit point for
 *	writing an active, but dirty, page to secondary media.
 *	Such a commit point is needed to insure that later modifications
 *	(after commencement of I/O) are not lost.
 *
 * Description:
 *	The VPGLOCK on the page guarantees stable hat resources
 *	for all visible translations for the page, and so allows
 *	looking at a PTE to see if it has a modify bit set.
 *	If a modify bit is found set, this function must also grab
 *	the hat_resourcelock spinlock for the as of that translation
 *	in order to have permission to clear that modify bit.
 *	The hat lock is later in the locking order explicitly 
 *	to allow accomplishing this sort of task without dropping
 *	the page VPGLOCK.
 */
void
pae_hat_pagesyncmod(page_t *pp, struct as *as, vaddr_t addr)
{
	pte64_t *ptep, *pdtep, *epdtep;
	int flush = 0;
	struct hat *hatp;
	struct seg *seg;
	vaddr_t savaddr = addr;
	vaddr_t endaddr = addr + PAGESIZE;
#ifdef DEBUG
	int modset = 0;
#endif
#ifdef CCNUMA
	uint_t pteval;
#endif /* CCNUMA */

	page_t *ptpp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT2;

	ASSERT_PP_IN_RANGE(pp);
	ASSERT(PAGE_VPGLOCK_OWNED(pp));

	if (as == &kas) {
		ASSERT(!KADDR_PER_ENG(addr));
		/*
		 * It is a visible kernel mapping.
		 */
#ifndef CCNUMA
		ptep = kvtol2ptep64(addr);
		ASSERT(pteptopp(ptep) == pp);

		/* 
		 * It's okay to clear the mod bit here. If somebody
		 * sets the mod bit at a later stage, it will not
		 * be missed because the knowledge of dirtyness of
		 * the page is already cached in the page structure.
		 */	

		if (ptep->pg_pte & PG_M) {
#ifdef DEBUG
			modset++;
#endif
			pp->p_mod = 1;
			atomic_and(&ptep->pte32.pg_low, ~PG_M);
			seg = as_segat(&kas, addr);  
			ASSERT(seg);
			if (!SOP_LAZY_SHOOTDOWN(seg, addr))
				hat_shootdown((TLBScookie_t)0, HAT_NOCOOKIE);
		} /* if (pg_pte & PG_M) */
#else
		pteval = 0;
		FOR_EACH_CG_PTE(addr)
			ASSERT(cg_ptep->pg_pte != 0);
			ASSERT(pteptopp(cg_ptep) == pp);
			pteval |= cg_ptep->pte32.pg_low;
		END_FOR_EACH_CG_PTE
		if (pteval & PG_M) {
#ifdef DEBUG
			modset++;
#endif
			pp->p_mod = 1;
			FOR_EACH_CG_PTE(addr)
				atomic_and(&cg_ptep->pte32.pg_low, ~PG_M);
			END_FOR_EACH_CG_PTE
			seg = as_segat(&kas, addr);  
			ASSERT(seg);
			if (!SOP_LAZY_SHOOTDOWN(seg, addr))
				hat_shootdown((TLBScookie_t)0, HAT_NOCOOKIE);
		} /* if (pg_pte & PG_M) */
#endif /* !CCNUMA */
		return;
	} /* as == kas? */

	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);

	if (pae_hat_prep(as, &addr, endaddr, HAT_FIRSTPT, &pdtep, &epdtep,
			&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		ASSERT(pdtep == (pte64_t *)NULL);
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}

	if (ptep->pg_pte == 0 || PFN64(ptep) != pp->p_pfn) {
		pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}

	ASSERT(ptep && pdtep && ptpp);
	ASSERT(addr == savaddr);
	ASSERT(ptep->pg_pte != 0);
	ASSERT(pteptopp(ptep) == pp);

	/*
	 * A virtual address could be calculated here,
	 * but the simpler full flush scheme is being used
	 * for the initial implementation. This does not
	 * require a virtual address. It also works for
	 * newer CPU chips (486, ...) than the 386, making
	 * the hat code intel generic.
	 */

	if (ptep->pg_pte & PG_M) {
#ifdef DEBUG
		modset++;
#endif
		atomic_and(&ptep->pte32.pg_low, ~PG_M);
		ASSERT(pteptopp(ptep) == pp);
		PAGE_SETMOD(pp);
		HAT_SET_MODSTATS(hatp, pdtep, pae_pnum(addr));
		flush |= hat_uas_shootdown_l(hatp);
	}

	pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

	HATRL_UNLOCK_SVDPL(hatp);

	ASSERT(!modset || pp->p_mod);	
	if (flush)
		TLBSflushtlb();
}

/*
 * void
 * pae_hat_pagegetmod(page_t *pp, struct as *as, vaddr_t addr)
 *	Look for a modify bit either in the page structure or a PTE.
 *	If found in a PTE, set the p_mod field. Never change a PTE.
 *
 *	This routine is important for performance since it allows
 *	the determination of whether a page is dirty without causing
 *	TLB Shootdown or even needing the hat_resourcelock for any
 *	any of the translations. This is important for minimizing
 *	the cost of fsflush.
 *
 * Calling/Exit State:
 *	The function is called with the VPGLOCK spinlock
 *	of the page held (checked by ASSERT).
 *
 *	This lock is never dropped by the function and is held
 *	on return. This lock provides permission to access the
 *	the p_mod field of the page_t, and guarantees stable
 *	hat resources (though not PTE bits).
 *
 *	Stable PTE bits can be guaranteed only by unloading the page.
 *	But the instability is one way: ref and mod bits can be set
 *	at any time but are never cleared by MMUs.
 */
void
pae_hat_pagegetmod(page_t *pp, struct as *as, vaddr_t addr)
{
	pte64_t *ptep, *pdtep, *epdtep;
	hat_t *hatp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	page_t *ptpp;
	vaddr_t savaddr = addr;
	vaddr_t endaddr = addr + PAGESIZE;
#ifdef CCNUMA
	uint_t pteval;
#endif /* CCNUMA */

	ASSERT_PP_IN_RANGE(pp);
	ASSERT(PAGE_VPGLOCK_OWNED(pp));

	if (pp->p_mod)
		return;

	if (as == &kas) {
		/*
		 * It is a visible kernel mapping.
		 */
#ifndef CCNUMA
		ptep = kvtol2ptep64(addr);
		ASSERT(pteptopp(ptep) == pp);
		if (ptep->pg_pte & PG_M)
			pp->p_mod = 1;
#else
		pteval = 0;
		FOR_EACH_CG_PTE(addr)
			ASSERT(pteptopp(cg_ptep) == pp);
			pteval |= cg_ptep->pte32.pg_low;
		END_FOR_EACH_CG_PTE
		if (pteval & PG_M)
			pp->p_mod = 1;
#endif /* !CCNUMA */
	} else {
		hatp = &as->a_hat;
		HATRL_LOCK_SVPL(hatp);
		if (pae_hat_prep(as, &addr, endaddr, HAT_FIRSTPT, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
			ASSERT(pdtep == (pte64_t *)NULL);
			HATRL_UNLOCK_SVDPL(hatp);
			return;
		}
		if (ptep->pg_pte == 0 || PFN64(ptep) != pp->p_pfn) {
			pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
			HATRL_UNLOCK_SVDPL(hatp);
			return;
		}
			
		ASSERT(addr == savaddr);
		ASSERT(pdtep && ptep && ptpp);
		ASSERT(pteptopp(ptep) == pp);

		if (ptep->pg_pte & PG_M)
			pp->p_mod = 1;

		pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
		HATRL_UNLOCK_SVDPL(hatp);
	}
}

/*
 * void
 * pae_hat_kas_unload(vaddr_t addr, ulong_t nbytes, uint_t flags)
 *	Unload a range of visible kernel mappings.
 *
 * Calling/Exit State:
 *	The PAGE_VPGLOCKs will be grabbed and released.
 *	The caller assures there is no access and must do lazy
 *	TLB Shootdown processing.
 *	Valid values for the flags argument are HAT_DONTNEED which is
 *	passed onto page_free() and HAT_CLEARMOD which clears the
 *	mod bit in the page struct as well as in the translation.
 *
 * Remarks:
 *	VM_HASHLOCK() is obtained in this routine. This is to guard the
 *	identity of  the page(s) we are unloading so that we can obtain
 *	VPGLOCKs on them. The race condition exists between page aborts
 *	(hat_pageunloads) and segmap unloads. The VM_HASHLOCK will protect us
 *	from this race condtition since pvn acquires the VM_HASHLOCK when
 *	doing the abort.
 */
void
pae_hat_kas_unload(vaddr_t addr, ulong_t nbytes, uint_t flags)
{
	pte64_t ptesnap;
	pte64_t *ptep;
	page_t *pp;
	int num_vpglocks = 0;
	vnode_t *vpglocks[HAT_MAX_VPGLOCKS] = {NULL, NULL};
	vnode_t *vp;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((nbytes & POFFMASK) == 0);

	ptep = kvtol2ptep64(addr);

	VM_HASHLOCK();

	for (; nbytes != 0; ptep++, nbytes -= PAGESIZE, addr += PAGESIZE) {
		if ((ptesnap = *ptep).pg_pte == 0) {
			continue;
		}
		pp = pteptopp(&ptesnap);
		ASSERT(pp != NULL);
		vp = pp->p_vnode;

		if (!HAT_VPGLOCK_EXISTS(vp, vpglocks)) {
			if (!HAT_TRYVPGLOCK(vp, num_vpglocks, vpglocks)) {
				HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
 				HAT_VPGLOCK(vp, num_vpglocks, vpglocks);
			}
		}

		/*
		 * The page could have been aborted in the mean time
		 * and our translation unloaded by hat_pageunload.
		 * If so, skip it.
		 */
		if (ptep->pg_pte == 0)
			continue;

		ASSERT(pteptopp(ptep) == pp);
		/*
		 * Must remove pte from its p_hat_mapping list.
		 * Clear the mod bits if neccessary.
		 */
#ifndef CCNUMA
		if (flags & HAT_CLRMOD) {
			pp->p_mod = 0;
		} else if (ptep->pgm.pg_mod) {
			pp->p_mod = 1;
		}
		pae_clrpte(ptep);	/* ptep->pg_pte = 0; */
#else
		FOR_EACH_CG_PTE(addr)
			if (flags & HAT_CLRMOD) {
				pp->p_mod = 0;
			} else if (cg_ptep->pgm.pg_mod) {
				pp->p_mod = 1;
			}
			pae_clrpte(cg_ptep);	/* cg_ptep->pg_pte = 0; */
		END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */

		(void) pae_hat_remptel(&kas, addr, pp, (flags & HAT_DONTNEED));
	}

	HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
	VM_HASHUNLOCK();
}

/*
 * void
 * pae_hat_kas_valid(vaddr_t addr, ulong_t nbytes, boolean_t set)
 *      Validate or invalidate a range of visible kernel mappings.
 *
 * Calling/Exit State:
 *      Same as hat_kas_unload.
 *
 *      If `set' is B_TRUE, we turn on the PG_V bits in the PTEs;
 *      otherwise, we turn them off.
 *
 * Remarks:
 *      This is a service routine for segmap on P6 platforms.
 *      The TLB shootdown policy employed by segmap assumes that it is safe
 *      to leave unneeded mappings loaded until they are to be recycled,
 *      since following a shootdown, the only way a mapping can find its
 *      way back into the TLB is if the corresponding virtual address is
 *      referenced.  That assumption is violated by the speculative
 *      TLB loading that can occur on the P6.  To address that problem with
 *      minimal impact on the existing code, we simply invalidate such
 *      mappings to prevent the eager P6 from loading them back into the
 *      TLB behind our backs.  Later we may be able to turn the PG_V bits
 *      back on and avoid unnecessary faults or shootdowns.
 *
 *      This routine was cloned from hat_kas_unload.
 */
void
pae_hat_kas_valid(vaddr_t addr, ulong_t nbytes, boolean_t set)
{
	pte64_t ptesnap;
	pte64_t *ptep;
	page_t *pp;
	int num_vpglocks = 0;
	vnode_t *vpglocks[HAT_MAX_VPGLOCKS] = {NULL, NULL};
	vnode_t *vp;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((nbytes & POFFMASK) == 0);

	ptep = kvtol2ptep64(addr);

	VM_HASHLOCK();

	for (; nbytes != 0; ptep++, nbytes -= PAGESIZE, addr += PAGESIZE) {
		if ((ptesnap = *ptep).pg_pte == 0) {
			continue;
		}
		pp = pteptopp(&ptesnap);
		ASSERT(pp != NULL);
		vp = pp->p_vnode;

		if (!HAT_VPGLOCK_EXISTS(vp, vpglocks)) {
			if (!HAT_TRYVPGLOCK(vp, num_vpglocks, vpglocks)) {
				HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
 				HAT_VPGLOCK(vp, num_vpglocks, vpglocks);
			}
		}

		/*
		 * The page could have been aborted in the mean time
		 * and our translation unloaded by hat_pageunload.
		 * If so, skip it.
		 */
		if (ptep->pg_pte == 0)
			continue;

		ASSERT(pteptopp(ptep) == pp);
#ifndef CCNUMA
		if (set)
			atomic_or(&ptep->pte32.pg_low, PG_V);
		else
			atomic_and(&ptep->pte32.pg_low, ~PG_V);
#else
		FOR_EACH_CG_PTE(addr)
			if (set)
				atomic_or(&cg_ptep->pte32.pg_low, PG_V);	
			else
				atomic_and(&cg_ptep->pte32.pg_low, ~PG_V);
		END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */
	}

	HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
	VM_HASHUNLOCK();
}

/*
 * boolean_t
/*
 * boolean_t
 * pae_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len,
 *		uint_t prot, bolean_t doshoot)
 *	Change the protections for the virtual address range
 *	[addr,addr+len] to the protection prot.
 *
 * Calling/Exit State:
 *	The return value is useful only when argument doshoot is B_FALSE.
 *	It return B_TRUE, if the caller needs to perform a shootdown.
 *	It returns B_FALSE, if no shootdown is neccessary. 
 *
 *	No hat or page locks are held by the caller.
 *	No hat or page locks are held on return.
 *
 *	This is never used on the kas.
 *
 * Description:
 *	This changes only active PTEs, no mappings are added, and none
 *	are removed. So only the hat resource lock is needed.
 *
 *	TLB Shootdown is performed based on the doshoot flag. If the flag
 *	is B_FALSE, then the caller gurantees that there are no accesses
 *	to this range and the caller is reponsible to perfrom the shootdown.
 *	Since vtop operations must search the hatpt list anyway, the trick 
 *	of unloading level 1 entries is used to minimize the TLB Shootdown
 *	synchronization time (the time between TLBSsetup and TLBSteardown).
 *
 *	Always called from current process context.
 *
 * Remarks:
 *	Segments mapped via PSE mappings should not be calling
 *	this routine; such segments call pse_hat_chgprot instead.
 *
 *	We assume that the MMU atomically checks the protection bits 
 *	before writing the mod bits.
 *
 * History:
 *	Wait until anything depending on protections not becoming
 *	restrictive finishes before making the protections restrictive.
 */
boolean_t
pae_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
		boolean_t doshoot)
{
	struct hat *hatp = &seg->s_as->a_hat;
	vaddr_t endaddr, savaddr;
	uint_t pprot, curprot;
	vaddr_t shaddr = UNSHIELDED;
	boolean_t flush = B_FALSE;
	int pndx = 0;
	pte64_t *ptep, *pdtep, *epdtep;
	struct as *as = seg->s_as;
	page_t *ptpp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;

	/* addr must be page aligned */
	ASSERT(((u_long) addr & POFFMASK) == 0);
	ASSERT(seg->s_as != &kas);
	ASSERT(u.u_procp->p_as == seg->s_as);
	ASSERT(CURRENT_LWP() != NULL);


	if (prot == (unsigned)~PROT_WRITE)
		pprot = PG_US|PG_V;
	else
		pprot = hat_vtop_prot(prot);

	endaddr = addr + len;
	ASSERT(endaddr >= addr);

	HATRL_LOCK_SVPL(hatp);

	do {
		if (pae_hat_prep(as, &addr, endaddr, HAT_FIRSTPT, &pdtep,
				&epdtep, &ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE)
			goto done;

		ASSERT(HATPT_AEC(ptpp));

		pndx = pae_pnum(addr);
		for (; pndx < PAE_HAT_EPPT; addr += PAGESIZE, ptep++, pndx++) {
			if (addr >= endaddr) {
				pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				goto done;
			}
#ifdef DEBUG
			savaddr = addr;
#endif /* DEBUG */
			if (ptep->pg_pte == 0)
				continue;
			curprot = (ptep->pte32.pg_low & (PG_US|PG_RW|PG_V));
			ASSERT((curprot & PG_US) && (curprot != (PG_US|PG_RW)));
			if (prot == ~PROT_WRITE) {
				/*
				 * In this case we must avoid adding PG_V to
				 * a PROT_NONE page, and not waste time on a
				 * page which is already write protected; but
				 * must restore the PG_V which might be
				 * removed by shielding.
				 */
				if (!(curprot & PG_RW))
					continue;
				if (shaddr == UNSHIELDED)
					flush |= pae_hat_uas_shield(hatp,
								    shaddr = addr,
								    endaddr,
								    ptep);
			} else {
				/*
				 * The operation below on ptep->pg_pte is not
				 * atomic: 
				 * it's not fatal if it loses volatile
				 * PG_REF, but it must never lose volatile
				 * PG_M: hence this check & call to shield
				 */
				if (curprot == pprot)
					continue;
				if ((curprot & PG_RW) && (shaddr == UNSHIELDED))
					flush |= pae_hat_uas_shield(hatp, shaddr = addr, endaddr, ptep);
			}
			ptep->pte32.pg_low = pprot | (ptep->pte32.pg_low & ~(PG_US|PG_RW|PG_V));
		} /* pndx loop */

		pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

		ASSERT(savaddr != addr);

	} while (addr < endaddr);

done:
	if (shaddr != UNSHIELDED)
		pae_hat_uas_unshield(hatp, shaddr, endaddr);
	if (flush && (doshoot || hatp->hat_activecpucnt > 1)) {
		/*
		 * In the pae_hat_uas_shield() scheme, we cannot delay shooting
		 * other cpus, and have already shot them.  If flush &&
		 * !doshoot,  is it better to delay our TLBSflushtlb() or do
		 * it now?  Since the callers will do hat_uas_shootdown() if
		 * we return B_TRUE, yet the other cpus don't need shooting
		 * down, do it now and return B_FALSE, if currently there are
		 * other cpus active  (of course, that can easily change -
		 * just a strategy). 
		 * Later we could change pae_hat_chgprot()'s callers.
		 */
		TLBSflushtlb();
		flush = B_FALSE;
	}
	HATRL_UNLOCK_SVDPL(hatp);
	return flush;
}

/*
 * void
 * pae_hat_kas_memload(vaddr_t addr, page_t *pp, uint_t prot)
 *	Load a visible (on p_hat_mapping chain) kernel translation.
 *
 * Calling/Exit State:
 *	Assumes that combination of any locks by caller combined with
 *	the PAGE_VPGLOCK provide all synchronization that is needed.
 *	All PTEs for visible kernel mappings must be virtually
 *	contiguous.
 *	REMEMBER that fact if a second kernel segment driver needs
 *	visible mappings (seg_map is only one for now).
 */
void
pae_hat_kas_memload(vaddr_t addr, page_t *pp, uint_t prot)
{
	pte64_t *ptep, tpte;
	uint_t mode;
	uint_t pf, pteval;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT(pp);

	mode = hat_vtop_prot(prot);
	pf = page_pptonum(pp);
	ptep = kvtol2ptep64(addr);

	PAGE_VPGLOCK(pp);
	if (ptep->pg_pte != 0) {
		if ((uint)PFN64(ptep) == pf) {
			/*
			 * Redoing an existing PTE is a TLB shootdown case.
			 */
			pteval = ptep->pte32.pg_low & (PTE_PROTMASK | PG_V);
			if (pteval == mode) {
				/*
			 	 * Mode is the same, then we don't do anything.
				 */
				PAGE_VPGUNLOCK(pp);
				return;
			} else if ((pteval | PG_V) == mode) {
				/*
				 * Mode is the same except that we're turning
				 * the VALID bit on.  No need for shootdown.
				 * (This case can arise from the P6 workaround
				 * in segmap_release.)
				 */
#ifndef CCNUMA
				ptep->pg_pte |= PG_V;
#else
				pteval = 0;
				FOR_EACH_CG_PTE(addr)
					ASSERT(cg_ptep->pg_pte != 0);
					pteval = cg_ptep->pte32.pg_low | PG_V;
					cg_ptep->pte32.pg_low = pteval;
				END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */
				PAGE_VPGUNLOCK(pp);
				return;
			}

			ASSERT((mode & ~(PTE_PROTMASK | PG_V)) == 0);

#ifndef CCNUMA
			/* pteval holds the low 32-bits of the 64-bit pte */
			pteval = atomic_fnc(&ptep->pte32.pg_low);
#else
			pteval = 0;
			FOR_EACH_CG_PTE(addr)
				pteval |= atomic_fnc(&cg_ptep->pte32.pg_low);
			END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */
			hat_shootdown((TLBScookie_t)0, HAT_NOCOOKIE);
			/* reloading the pte with new protections */
			pteval = mode | (pteval & ~(PTE_PROTMASK|PG_V));

#ifndef CCNUMA
			ptep->pte32.pg_low = pteval;
			ASSERT(ptep->pg_pte != 0);
			ASSERT(pteptopp(ptep) == pp);
#else
			FOR_EACH_CG_PTE(addr)
				cg_ptep->pte32.pg_low = pteval;
				ASSERT(cg_ptep->pg_pte != 0);
				ASSERT(pteptopp(cg_ptep) == pp);
			END_FOR_EACH_CG_PTE
#endif /* CCNUMA */

			PAGE_VPGUNLOCK(pp);
			return;
		} else {
			/*
			 *+ Trying to change existing mapping.
			 */
			cmn_err(CE_PANIC,
				"hat_kas_memload - changing mapping:pte= %Lx\n",
				ptep->pg_pte);
		}
	}

	/* ptep->pg_pte = pae_mkpte(mode, pf); */
	tpte.pg_pte = pae_mkpte(mode, pf);
#ifndef CCNUMA
	pae_setpte(&tpte, ptep);
	ASSERT(ptep->pg_pte != 0);
#else
	FOR_EACH_CG_PTE(addr)
		pae_setpte(&tpte, cg_ptep);
		ASSERT(cg_ptep->pg_pte != 0);
		ASSERT(pteptopp(cg_ptep) == pp);
	END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */

	/* increment page mapping count */
	PAGE_KAS_LOAD(addr, pp);

	PAGE_VPGUNLOCK(pp);
}

/*
 * STATIC uint_t
 * pae_hat_map_vplist(struct seg *seg, vnode_t *vp, off64_t vp_base,
 *		uint_t mode, uint_t flags)
 *	This routine is invoked by hat_map() if it decides to scan the vpages
 *	list of the vnode to map in the pages.
 *
 * Calling/Exit State:
 *	Same conditions as hat_map().
 *	Returns the number of successful mappings.
 *	If it has scanned the entire vpages list or if v_pages is NULL,
 *	this routine returns the size of the seg.
 */
STATIC uint_t
pae_hat_map_vplist(struct seg *seg, vnode_t *vp, off64_t vp_base, uint_t mode,
		uint_t flags)
{
	vaddr_t	addr;
	page_t	*pp, *epp, *mpp, *ptpp, *opp;
	pte64_t	*ptep, *pdtep, *epdtep;
	pte64_t tpte;
	hat_t	*hatp;
	struct	as *as = seg->s_as;
	int	seg_size;
	off64_t	vp_off;		/* offset of page in file */
	off64_t	vp_end;		/* end of VM segment in file */
	long	addr_off;	/* loop invariant; see comments below. */
	unsigned long fail_count = 0, map_count = 0;
	vaddr_t eaddr, savaddr;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;

	addr = seg->s_base;
	seg_size = seg->s_size;
	eaddr = addr + seg_size - 1;
	ASSERT(((u_long)addr & POFFMASK) == 0);	/* page aligned */

	hatp = &(as->a_hat);

	if (((as->a_isize + seg->s_size) >= hat_privl1_size) &&
			(HAT_PRIVL164_LIST(hatp) == NULL) &&
			(hat_privl1_size != 0))
		pae_hat_switch_l1(as);

	/*
	 * Remove invariant from the loop. To compute the virtual
	 * address of the page, we use the following computation:
	 *
	 *	addr = seg->s_base + (vp_off - vp_base)
	 *
	 * Since seg->s_base and vp_base do not vary for this segment:
	 *
	 *	addr = vp_off + (addr_off = (seg->s_base - vp_base))
	 *
	 * Note: currently addr = seg->s_base;
	 */
	addr_off = addr - vp_base;
	vp_end = vp_base + seg_size;

	/*
	 * Walk down the list of pages associated with the vnode,
	 * setting up the translation tables if the page maps into
	 * addresses in this segment.
	 *
	 * This is not an easy task because the pages on the list
	 * can disappear as we traverse it. Holding the VM_PGIDLK protects
	 * against this, but sometimes the lock must be dropped.
	 * It is dropped only to allocate page tables or mapping chunks
	 * in NOSLEEP mode. To avoid too much dropping, an attempt
	 * is first made to allocate from the hat's local free caches
	 * of these resources (the page layer locks can be kept while doing
	 * that). To allow that, the hat free cache locks occur later in the
	 * lock hierarchy.
	 *
	 * Use a dummy page as a marker of our start point to terminate our
	 * search of the page list. Code that scans this list must skip
	 * strange pages. We need another marker page to insert in the
	 * middle of the list when we call PAGE_RECLAIM_L. If the page cannot
	 * cannot be reclaimed from the free list, the page's identity is
	 * destroyed by PAGE_RECLAIM_L. We move to the next page on the list
	 * by a macro (ADVANCE_FROM_MARKER_PAGE) that removes the marker page
	 * page from the list.
	 */

	/* allocate page markers */
	CREATE_2_MARKER_PAGES_NS(mpp, epp, pae_hat_map);

	if (mpp == (page_t *)NULL)
	    return 0;

	VM_HASHLOCK();
	ASSERT(VN_IS_HELD(vp));
	VN_PGLOCK(vp);

	if ((pp = vp->v_pages) == NULL) {
	    VN_PGUNLOCK(vp);
	    VM_HASHUNLOCK();
	    DESTROY_2_MARKER_PAGES(mpp, epp);
	    return btop(seg->s_size);
	}

	INSERT_END_MARKER_PAGE(epp, vp);

	HATRL_LOCK_SVPL(hatp);
	VM_PAGEFREELOCK();

	do {
	    /*
	     * If there are any pending interrupts, then insert the
	     * marker page in front of the next page to process, and drop
	     * the locks we're holding.
	     */
	    if (intrpend(PLBASE)) {
		INSERT_MARKER_PAGE(pp->p_vpprev, mpp);
		VM_PAGEFREEUNLOCK(); 
		HATRL_UNLOCK_SVDPL(hatp); 
		VN_PGUNLOCK(vp); 
		VM_HASHUNLOCK();
		VM_HASHLOCK();
		VN_PGLOCK(vp); 
		HATRL_LOCK_SVPL(hatp); 
		VM_PAGEFREELOCK(); 
		ADVANCE_FROM_MARKER_PAGE(pp, mpp, vp);
		continue;
	    }

	    /*
	     * See if the vp offset is within the range mapped
	     * by the segment.  Also skip any marker pages.
	     *
	     * If page is being paged in, ignore it. (There may be 
	     * races if we try to use it.)
	     *
	     * NOTE: We do these checks here, before possibly taking
	     * the page off the freelist, to avoid unecessarily
	     * reclaiming the page. As a consequence, we must check
	     * the vnodes.
	     */
	    vp_off = pp->p_offset;
	    if (pp->p_vnode != vp || pp->p_invalid) {
		pp = pp->p_vpnext;
		continue;
	    }
	    if (vp_off < vp_base || vp_off >= vp_end) {
		if (++fail_count / (map_count + 1) >= HAT_MAP_VPLIST_TRIES)
		    break;
		pp = pp->p_vpnext;
		continue;
	    }

	    map_count++;

	    /* cache the value of leader here */
	    opp = pp; 

	    if (pp->p_free != 0 && !page_reclaim_l(pp, P_LOOKUP)) {
		/*
		 * The page was free and the attempt to allocate
		 * ran into freemem limits, so we must give up
		 * on this page.
		 */
		pp = opp->p_vpnext;
		continue;
	    }

#ifdef CCNUMA
	    /* find a page on the local cg */
	    if (vp->v_place.pl_type == PP_REPLICATED) {
		VM_PAGEFREEUNLOCK();
		HATRL_UNLOCK_SVDPL(hatp);
		pp = page_find_closest(seg, pp, 0/*flag*/);
		ASSERT(pp->p_free == 0);
		HATRL_LOCK_SVPL(hatp);
		VM_PAGEFREELOCK();
	    }
#endif /* CCNUMA */

	    /*
	     * At this point, we have a page (not on any free list)
	     * with an effective reader lock (i.e., count not bumped,
	     * but status say it could be and that cannot change
	     * while the VN_PGLOCK is held.
	     */

	    ASSERT(pp->p_vnode == vp && pp->p_offset == vp_off);
	    addr = (vaddr_t) (addr_off + vp_off);
	    ASSERT((vp_off & POFFMASK) == 0);
	    ASSERT((addr & POFFMASK) == 0);
	    ASSERT(addr <= eaddr);

	    if (pae_hat_prep(as, &addr, eaddr, HAT_ALLOCPT, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		/*
		 * That is as hard as we are willing to try.
		 * Give up the whole show. Going on is likely
		 * run into more of same.
		 */
		if (!IS_VALID_REPLICA(pp) && !PAGE_IN_USE(pp)) {
		    page_free_l(pp, 0);
		}
		break;
	    }
	    ASSERT(ptep->pg_pte == 0); 
	    ASSERT(pp->p_vnode == vp && pp->p_offset == vp_off);

	    /* ptep->pg_pte = pae_mkpte(mode, page_pptonum(pp)); */
	    tpte.pg_pte = pae_mkpte(mode, page_pptonum(pp));
	    pae_setpte(&tpte, ptep);
	    ASSERT(tpte.pg_pte == ptep->pg_pte);

	    MET_PREATCH(1);

	    /*
	     * Finally, add this reference to the page. If the
	     * page is on the freelist and we mapped it into
	     * the process address space, take it off the freelist.
	     * Then get the next page on the vnode page list.
	     */
	    PAGE_UAS_LOAD(as, addr, pp);
	    BUMP_RSS(as, 1);
	    BUMP_AEC(ptpp, 1);

	    if (flags & HAT_LOCK) {
		INCR_LOCK_COUNT(ptpp, as);
		ptep->pg_pte |= PG_LOCK;
	    }

	    pp = opp->p_vpnext;

	    pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

	} while (pp != epp);

	VM_PAGEFREEUNLOCK();
	HATRL_UNLOCK_SVDPL(hatp);
	REMOVE_MARKER_PAGE(epp, vp);
	VN_PGUNLOCK(vp);
	VM_HASHUNLOCK();

	ASSERT(getpl() == PLBASE);

	DESTROY_2_MARKER_PAGES(mpp, epp);
	if (pp == epp)		/* scanned the entire vpages list? */
	    return btop(seg->s_size);

	return map_count;
}

/*
 * uint_t
 * pae_hat_map(struct seg *seg)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
uint_t
pae_hat_map(struct seg *seg)
{
	return 0;
}

/*
 * uint_t 
 * pae_hat_preload(struct seg *seg, vnode_t *vp, off64_t vp_base, uint_t prot,
 *		uint_t flags)
 *	Allocate any hat resources needed for a new segment.
 *
 *	This routine is invoked by the seg_create() routines 
 *	in the segment drivers.
 *
 *	For vnode type of segments, we load up translations 
 *	to any pages of the segment that are already in core.
 *	But this is done only in NOSLEEP mode. Also, it does not
 *	try too hard for locks on pages. Memory exhaustion
 *	causes it to stop trying.
 *
 * Calling/Exit State:
 *	No spin locks that would interfere with kmem_alloc or kpg_alloc
 *	can be held by the caller. This function will cause the context
 *	to acquire (and later release) these locks- VM_PAGEFREELOCK,
 *	VN_PGLOCK, and HAT_RESOURCE_LOCK.
 *
 *	Always called from current process context.
 *
 * Description:
 *	This function may look up pages by their page identity, if the
 *	scanning the v_pages does not harvest enough pages, under certain
 *	conditions.
 */
uint_t
pae_hat_preload(struct seg *seg, vnode_t *vp, off64_t vp_base, 
		uint_t prot, uint_t flags)
{
	vaddr_t	addr, eaddr;
	page_t	*pp, *ptpp;
	pte64_t	*ptep, *pdtep, *epdtep;
	pte64_t tpte;
	hat_t	*hatp;
	struct	as *as = seg->s_as;
	int	pndx;
	off64_t	soff, eoff;
	uint_t  mode;
	boolean_t got_pp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	vaddr_t savaddr;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(((u_long)seg->s_base & POFFMASK) == 0);	/* page aligned */

	if (vp == NULL || (vp->v_pages == NULL) || 
	   ((flags & HAT_PRELOAD) == 0) || (as != u.u_procp->p_as))
		return(0);

	/*
	 * Set up as many of the hardware address translations as we can.
	 * If the segment is a vnode-type segment and it has a non-NULL vnode, 
	 * we walk down the list of incore pages in the given range.
	 *
	 * Note: the file system that owns the vnode is not informed about the
	 * new references we have made to the page.
	 */

	/*
	 * Calculate the protections we need for the pages.
	 * (i.e. whether to set fault on write bit or not).
	 */
	if (!((mode = hat_vtop_prot(prot)) & PG_V))
		return(0);		/* don't bother */

	/*
	 * First, prime the hat free pools.
	 */
	hat_refreshpools();

	/*
	 * If the size we are trying to map is extremely small (as governed
	 * by HAT_MAP_MIN_SIZE), we skip traversing the vpages list and
	 * directly go to the page identity method.
	 */
	if (btop(seg->s_size) > HAT_MAP_MIN_SIZE) {
		pae_hat_map_vplist(seg, vp, vp_base, mode, flags);

		ASSERT(KS_HOLD0LOCKS());
		ASSERT(getpl() == PLBASE);

		return 0;
	}

	hatp = &(as->a_hat);
	soff = vp_base;
	eoff = vp_base + seg->s_size;

	VM_HASHLOCK();
	VN_PGLOCK(vp);

	if (vp->v_pages == NULL) {
		VN_PGUNLOCK(vp);	
		VM_HASHUNLOCK();
		ASSERT(getpl() == PLBASE);
		return 0;
	}

	HATRL_LOCK_SVPL(hatp);
	VM_PAGEFREELOCK();

	while (soff < eoff) {
		/*
		 * If there are any pending interrupts drop the locks we
		 * are holding.
		 */
		if (intrpend(PLBASE)) {
			{ HAT_MAP_INTRPEND(hatp, vp); }
		}

		HAT_MAP_FIND_PAGE(seg, vp, soff, pp, hatp);
		if (pp == NULL) {
			soff += PAGESIZE;
			continue;
		}

		/*
		 * At this point, we have a page (not on any free list)
		 * with an effective reader lock (i.e., count not bumped,
		 * but status says it could be and that cannot change)
		 * while the VPGLOCK is held.
		 */

		ASSERT(pp->p_vnode == vp && pp->p_offset == soff);

		addr = (vaddr_t) (seg->s_base + (soff - vp_base));
		eaddr = (vaddr_t) (seg->s_base + (eoff - vp_base));
		ASSERT(addr <= seg->s_base + seg->s_size);

		if (pae_hat_prep(as, &addr, eaddr, HAT_ALLOCPT, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
			/*
			 * That is as hard as we are willing to try.
			 * Give up the whole show. Going on is likely
			 * run into more of same.
			 */
			if (!IS_VALID_REPLICA(pp) && !PAGE_IN_USE(pp)) {
				page_free_l(pp, 0);
			}
			break;
		}
		ASSERT(ptep->pg_pte == 0); 
		ASSERT(soff < eoff);

		got_pp = B_TRUE;
		pndx = pae_pnum(addr);
		for (; pndx < PAE_HAT_EPPT; soff += PAGESIZE, ptep++, pndx++) {

		    	if (soff >= eoff)
				goto bye;

#ifdef NOTYET
			/*
			 * TODO: We probably should break out of the loop
			 * and start all over again.
			 */
		    	if (intrpend(PLBASE) && !got_pp) {
				BUMP_AEC(ptpp, 1);
				HAT_MAP_INTRPEND(hatp, vp);
				BUMP_AEC(ptpp, -1);
		    	}
#endif /* NOTYET */

		    	ASSERT(!got_pp || pp != NULL);

			ASSERT(ptep->pg_pte == 0); 

		    	if (!got_pp) {
				HAT_MAP_FIND_PAGE(seg, vp, soff, pp, hatp);
				if (pp == NULL) 
			    		continue;
		    	}
		    	/*
		     	 * Mark  got_pp to B_FALSE so that next time around,
		     	 * we go ahead and find the page for the new offset.
		     	 */
		    	got_pp = B_FALSE;

		    	ASSERT(ptep->pg_pte == 0); 
	
		    	/* ptep->pg_pte = pae_mkpte(mode, page_pptonum(pp)); */
			tpte.pg_pte = pae_mkpte(mode, page_pptonum(pp));
			pae_setpte(&tpte, ptep);
			ASSERT(tpte.pg_pte == ptep->pg_pte);

		    	MET_PREATCH(1);

		    	/*
		     	 * Finally, add this reference to the page mapping list.
		     	 */
	    		addr = (vaddr_t) (seg->s_base + (soff - vp_base));
			PAGE_UAS_LOAD(as, addr, pp);
		    	BUMP_RSS(as, 1);
		    	BUMP_AEC(ptpp, 1);

		    	if (flags & HAT_LOCK) {
				INCR_LOCK_COUNT(ptpp, as);
				ptep->pg_pte |= PG_LOCK;
		    	}

		} /* for (; pndx < PAE_HAT_EPPT; soff += PAGESIZE, ...) */

		pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

	} /* while (soff < eoff ) */

bye:
	VM_PAGEFREEUNLOCK();
	HATRL_UNLOCK_SVDPL(hatp);
	VN_PGUNLOCK(vp);
	VM_HASHUNLOCK();
	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	return 0;
}


/*
 * void
 * pae_hat_unlock(struct seg *seg, vaddr_t addr)
 *	Unlock translation at addr. 
 *	Translation might either not exist (page_abort) or must be locked.
 *	Undo lock bit and counts.
 *
 * Calling/Exit State:
 *	Will grab the hat_resourcelock and release it.
 *
 * Remarks:
 *	Should not be called by segments using PSE mappings.
 */
void
pae_hat_unlock(struct seg *seg, vaddr_t addr)
{
	struct hat *hatp;
	struct as *as = seg->s_as;
	pte64_t *ptep, *pdtep, *epdtep;
	page_t *ptpp;
	vaddr_t eaddr = addr + PAGESIZE;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	vaddr_t savaddr;

	hatp = &as->a_hat;
	/* addr is page aligned */
	ASSERT(((int)addr & POFFMASK) == 0);
	ASSERT(as != &kas);

	HATRL_LOCK_SVPL(hatp);

	/*
	 * Find the hatpt struct.
	 */
	if (pae_hat_prep(as, &addr, eaddr, 0, &pdtep, &epdtep, 
			&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}

	if (ptep->pg_pte == 0)
		goto done;

	HAT_LOG(as, ptpp, addr, eaddr, HAT_UNLOCK, 
			"pae_hat_unlock", HATPT_LOCKS(ptpp));
	ASSERT(ptep->pg_pte & PG_LOCK);

	/*
	 * Assume no TLB Shootdown needed since uniprocessor code
	 * said no TLBSflushtlb was needed.
	 */
	ptep->pg_pte &= ~PG_LOCK;
	DECR_LOCK_COUNT(ptpp, as);
	pae_hat_checkptcnt(ptpp);

done:
	pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * ppid_t
 * pae_kvtoppid(caddr_t vaddr)
 *	Return the physical page ID corresponding to the virtual 
 *	address vaddr. This is a required interface defined by DKI.
 *	For the 80x86, we use the page frame number as the physical page ID.
 *
 * Calling/Exit State:
 *	Kernel page table lookups need no locks.
 *
 * Remarks:
 *	Works properly for PSE mappings.
 */
ppid_t
pae_kvtoppid(caddr_t vaddr)
{
	pte64_t *ptep;

	ptep = kvtol1ptep64((vaddr_t)vaddr);
	/*
	 * Handle most frequent case first: VALID but not PSE.
	 */
	if ((ptep->pg_pte & (PG_V|PG_PS)) == PG_V) {
		if (PG_ISVALID(ptep = kvtol2ptep64((vaddr_t)vaddr)))
			return (ppid_t)PFN64(ptep);
		else
			return NOPAGE;
	} else if (PG_ISVALID(ptep))
		return (ppid_t)PFN64(ptep) + pae_pnum(vaddr);
	else
		return NOPAGE;
}

/*
 * void
 * pae_hat_vtopte_l(struct hat *hatp, vaddr_t vaddr, 
 *		pte64_t *pdtepa, pte64_t *ptepa)
 *	Find the pte and pde for a particular user virtual address.
 *
 * Calling/Exit State:
 *	The HAT resource lock is held on entry and still held on return.
 *
 *	On exit, the return value ptepa and pdepa are set as follows:
 *
 *		If there is a valid, non-PSE PTE for the virtual
 *		address, the return is in pte and the corressponding 
 *		pde for the page table.
 *
 *		If there is a valid PSE PDE for the virtual address,
 *		and the process has a private level 1 page table,
 *		then the ptepa is a pointer to the pde, and
 *		*pdepa is seto to NULL.
 *
 *		If there is a valid PSE PDE for the virtual address,
 *		and the process does not have a private level 1 page
 *		table, then the ptepa contains NULL, and *pdepa is
 *		the pointer to hatpt_pde structure for the hatpt_t.
 *
 *		If there is no valid PTE for the virtual address, then
 *		the ptepa and pdepa are both NULL.
 *
 * Remarks:
 *	This is global only so that it can be accessed from the i386
 *	specific begin_user_write code, and also from kdb db_uvtop
 *	code.
 *
 *	For PSE mappings, returns NULL, but also sets *ptapp to the
 *	proper hatpt_t pointer.  Therefore, callers uninterested in
 *	PSE mappings can ignore the NULL return value; other callers
 *	can find PSE mappings by checking *ptapp in the face of a NULL
 *	return value.
 *
 * Note:
 *	We have changed the interface to return the pde instead of
 *	ptapp since it is being used only to get the pde for the
 *	PSE pages. It should also return the pte instead of the 
 *	pointer to pte. This is to enable an unmap of the temporary
 *	mapping.
 */
void
pae_hat_vtopte_l(struct as *as, vaddr_t addr, pte64_t *pdtepa, pte64_t *ptepa)
{
	pte64_t *ptep, *pdtep, *epdtep;
	page_t *ptpp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT2;
	vaddr_t eaddr = addr + PAGESIZE;
	vaddr_t savaddr;

	ASSERT(!KADDR(addr));
	pdtepa->pg_pte = 0;
	ptepa->pg_pte = 0;

	if (pae_hat_prep(as, &addr, eaddr, 0, &pdtep, &epdtep, 
			&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		return;
	}

        if (pdtep == (pte64_t *)NULL && ptep != (pte64_t *)NULL) {
		if (PG_ISPSE(ptep)) {
			pdtepa->pg_pte = ptep->pg_pte;
			ptepa->pg_pte = 0;
		}
	} else {
		pdtepa->pg_pte = pdtep->pg_pte;
		ptepa->pg_pte = ptep->pg_pte;
	}

	pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

        return;
}

/*
 * uint_t
 * pae_hat_xlat_info(struct as *as, vaddr_t addr)
 *	Returns whether or not a translation is loaded at the given address,
 *	which must be a user address in the user address space, as.
 *
 * Calling/Exit State:
 *	The return value is stale unless the caller stabilizes the
 *	access to this address.
 */
uint_t
pae_hat_xlat_info(struct as *as, vaddr_t addr)
{
	hat_t *hatp;
	uint_t flags = 0;
	pte64_t pte, pde;
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

	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	pae_hat_vtopte_l(as, addr, &pde, &pte);
	if (pte.pg_pte != 0)
		flags = xlat_flags[pte.pg_pte & (PG_US|PG_RW|PG_V)];
	HATRL_UNLOCK_SVDPL(hatp);

	return flags;
}

/*
 * ppid_t
 * pae_hat_vtoppid(struct as *as, vaddr_t vaddr)
 *	Routine to translate virtual addresses to physical page IDs.
 *
 * Calling/Exit State:
 *	If an invalid user address is received, hat_vtoppid returns NOPAGE.
 *	It is illegal to pass an invalid (unmapped) kernel address, and if
 *	done, the system may panic.
 *	hat_vtoppid will grab hat_resourcelock and release it if user address.
 *
 *	The return value is stale unless the caller stabilizes the conditions;
 *	this is typically done by having the page SOFTLOCKed or by virtue of
 *	using the kernel address space.
 *
 *	Can be called from out of context (e.g. drivers).
 *
 * Remarks:
 *	Handles PSE translations.
 */
ppid_t
pae_hat_vtoppid(struct as *as, vaddr_t vaddr)
{
	struct hat *hatp;
	pte64_t pte, pde;
	ppid_t retval;
	pl_t savpl;

	if (KADDR(vaddr))
		return (ppid_t)mmu_btop(kvtophys64(vaddr));

	/*
	 * It is a user address and the only way to find the virtual
	 * addresses of user page tables is to search the hatpt list.
	 */

	ASSERT(as != (struct as *)NULL);
	hatp = &as->a_hat;
	/*
	 * The following ugliness due to  the fact that wd driver
	 * calls this function and the wd lock is at PLHI whereas
	 * the HAT lock could be lower than that.
	 */
#ifdef _LOCKTEST	/* KLUDGE for now */
	do {
		savpl = TRYLOCK(&hatp->hat_resourcelock, PLHI);
	} while(savpl == (pl_t)INVPL);
	hatp->hat_lockpl = savpl;
#else
	savpl = LOCK(&hatp->hat_resourcelock, PLHI);
#endif
	pae_hat_vtopte_l(as, vaddr, &pde, &pte);
	if (pte.pg_pte != NULL)
		if (PG_ISPSE(&pte))
			retval = (ppid_t)PFN64(&pte) + pae_pnum(vaddr);
		else
			retval = (ppid_t)PFN64(&pte);
	else if ((pde.pg_pte != NULL) && PG_ISPSE(&pde))
		retval = PFN64(&pde) + pae_pnum(vaddr);
	else
		retval = NOPAGE;
	UNLOCK(&hatp->hat_resourcelock, savpl);
	return retval;
}

#define EBADDR	0

/*
 * paddr64_t
 * pae_vtop(caddr_t vaddr, proc_t *procp)
 *	Routine to translate virtual addresses to physical addresses
 *	Typically used by drivers that need physical addresses.
 *
 * Calling/Exit State:
 *	Returns the physical address corresponding to the given virtual
 *	address within the process, procp; if procp is NULL, vaddr must be
 *	a kernel address.
 *
 *	As an optimization, if a PSE mapping exists in the address space,
 *	it takes advantage of it.
 *
 *	If an invalid user address is received, vtop returns 0.
 *	It is illegal to pass an invalid (unmapped) kernel address, and if
 *	done, the system may panic.
 *
 *	The return value is stale unless the caller stabilizes the conditions;
 *	this is typically done by having the page SOFTLOCKed or by virtue of
 *	using the kernel address space.
 */
paddr64_t
pae_vtop(caddr_t vaddr, proc_t *procp)
{
	ppid_t ppid;
	struct as *as;
	pte64_t *pseck;

	if (procp == NULL || procp == uprocp) {
		pseck = KPDTE(vaddr);
		if (PG_ISPSE(pseck))
			return((pseck->pg_pte & MMU_PAGEMASK)|PSE_PAGOFF(vaddr));
	}
	as = (procp == (proc_t *)NULL ? (struct as *)NULL : procp->p_as);

	if ((ppid = pae_hat_vtoppid(as, (vaddr_t)vaddr)) == NOPAGE)
		return (paddr64_t)EBADDR;
	return (paddr64_t)mmu_ptob64(ppid) + PAGOFF((vaddr_t)vaddr);
}

/*
 * page_t *
 * pae_hat_vtopp_l(struct as *as, vaddr_t vaddr, enum seg_rw rw)
 *	Routine to find the page mapped at a particular user virtual address.
 *
 * Calling/Exit State:
 *	VN_PGLOCK is held on entry to the function.
 */
page_t *
pae_hat_vtopp_l(struct as *as, vaddr_t vaddr)
{
	hat_t *hatp;
	pte64_t pte, pde;
	page_t *pp;

	ASSERT(as != (struct as *)NULL);
	ASSERT(!KADDR(vaddr));

	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	pae_hat_vtopte_l(as, vaddr, &pde, &pte);
	if (pte.pg_pte == NULL || PG_ISPSE(&pde) ||
	    (pp = page_numtopp(PFN64(&pte))) == (page_t *)NULL) {
		HATRL_UNLOCK_SVDPL(hatp);
		return ((page_t *)NULL);
	}

	ASSERT(PAGE_VPGLOCK_OWNED(pp));
	ASSERT(PAGE_IN_USE(pp));        /* since we found a mapping */

	HATRL_UNLOCK_SVDPL(hatp);
	return pp;
}

/*
 * page_t *
 * pae_hat_vtopp(struct as *as, vaddr_t vaddr, enum seg_rw rw)
 *	Routine to find the page mapped at a particular user virtual address.
 *
 * Calling/Exit State:
 *	If there is a valid visible mapping from the given virtual address
 *	to a page, the translation allows the specified access (rw), and
 *	that page can be read-locked without spinning or blocking, that page
 *	is returned read-locked.
 *	Otherwise, NULL is returned.
 */
page_t *
pae_hat_vtopp(struct as *as, vaddr_t vaddr, enum seg_rw rw)
{
	hat_t *hatp;
	pte64_t pte, pde;
	page_t *pp;
	page_t *lpp;		/* leader page */

	ASSERT(as != (struct as *)NULL);
	ASSERT(!KADDR(vaddr));

	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	pae_hat_vtopte_l(as, vaddr, &pde, &pte);
	if (pte.pg_pte == NULL || PG_ISPSE(&pde) ||
	    (rw == S_WRITE && !(pte.pg_pte & PG_RW)) ||
	    (pp = page_numtopp(PFN64(&pte))) == (page_t *)NULL)
		goto nopage;

	if (PAGE_TRYVPGLOCK(pp) == INVPL)
		goto nopage;

	if (PAGE_IS_WRLOCKED(pp))
		goto nopage_useunlock;

	ASSERT(PAGE_IN_USE(pp));	/* since we found a mapping */

	/*
	 * Hold a reader lock on the leader, but return the replica
	 * so that optimization in as_prmapin could still be applied.
	 */
	lpp = pp;
#ifdef CCNUMA
	while (lpp->p_replica) {
		ASSERT(IS_VALID_REPLICA(lpp));
		lpp = lpp->p_set;
	}
#endif /* CCNUMA */
	lpp->p_activecnt++;	/* Acquire a read lock */
	PAGE_VPGUNLOCK_PL(pp, VM_HAT_RESOURCE_IPL);
	HATRL_UNLOCK_SVDPL(hatp);
	return pp;

nopage_useunlock:
	PAGE_VPGUNLOCK_PL(pp, GL_PAGE_IPL);
nopage:
	HATRL_UNLOCK_SVDPL(hatp);
	return (page_t *)NULL;
}

/*
 * int
 * pae_hat_exec(struct as *oas, vaddr_t ostka, ulong_t stksz, struct as *nas,
 *		vaddr_t nstka, uint_t hatflag)
 *	Move page tables and hat structures for the new stack image
 *	from the old address space to the new address space.
 *
 * Calling/Exit State:
 *	Single engine environment during exec.
 *	Moves page tables but does not flush (one is coming soon)
 *	unless no more page tables are left (very unlikely).
 *
 * Notes:
 *	Should we create a L1 page table or ptap chains? All processes
 *	begin with hatpt chains. However, if the process that is doing
 *	an exec has a dedicated L1, then we allocate a hatpt structure
 *	and link it to the nas.
 */
int
pae_hat_exec(struct as *oas, vaddr_t ostka, ulong_t stksz, struct as *nas,
		vaddr_t nstka, uint_t hatflag)
{
	ASSERT(PAGOFF(stksz) == 0);
	ASSERT(PAGOFF(ostka) == 0);
	ASSERT(PAGOFF(nstka) == 0);
	ASSERT(nas->a_segs->s_next == nas->a_segs);
	ASSERT(nas->a_hat.hat_pts == (hatpt_t *)NULL);
	ASSERT(oas == u.u_procp->p_as);

	/*
	 * Move the page tables themselves as the flag
	 * states that they contain only pages to be moved
	 * and the pages are properly aligned in the table.
	 */
	if (hatflag) {
		hatpt_t *ptap, *nptap;
		pte64_t *ovpdte, *nvpdte, *ptep, *pdtep, *epdtep;
		struct hat *hatp, *ohatp;
		pl_t trypl;
		int delta;
		vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
		page_t *ptpp;
		vaddr_t addr, endaddr;
		vaddr_t savaddr;

		ovpdte = KPDTE(ostka);
		nvpdte = KPDTE(nstka);
		delta = nvpdte - ovpdte;
		ohatp = &oas->a_hat;
		hatp = &nas->a_hat;
		ASSERT(ohatp->hat_activecpucnt == 1);
		addr = ostka;
		endaddr = ostka + stksz - 1;
		nptap = (hatpt_t *)kmem_zalloc(sizeof(hatpt_t), KM_NOSLEEP);
		HATRL_LOCK_SVPL(ohatp);
		trypl = HATRL_TRYLOCK(hatp);
		ASSERT(trypl != (pl_t)INVPL);

		while (addr < endaddr) {
			if (pae_hat_prep(oas, &addr, endaddr, HAT_FIRSTPT, &pdtep,
				&epdtep, &ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
				/* 
				 * The following condition can occur in rare
				 * circumstances: We can get swapped out in 
				 * exec() during as_alloc() or extractargs().
				 * Then when we get swapped back in none of 
				 * the page tables for oas will be loaded. In
				 * this case we just quit and let the nas 
				 * fault in the pages.
				 */
				ASSERT(pdtep == (pte64_t *)NULL);
				break;
			}

			if (HAT_PRIVL164_LIST(ohatp)) {
				if (nptap == (hatpt_t *)NULL) {
					pae_hat_unprep(oas, savaddr, &ptpp, pdtep, kvaddr);
					HATRL_UNLOCK(hatp, trypl);
					HATRL_UNLOCK_SVDPL(ohatp);
					goto unload;
				}
				nptap->hatpt_pde.pg_pte = pdtep->pg_pte;
				pdtep->pg_pte = 0;
				/* adjust the index for new stack address */
				nptap->hatpt_pdtep = nvpdte + 
						(KPDTE(addr) - ovpdte);
				nptap->hatpt_as = nas;
				nptap->hatpt_ptpp = ptpp;
				hat_link_ptap(nas, hatp->hat_ptlast, nptap);
				HATPT_PTAP(ptpp) = nptap;
				nptap = (hatpt_t *)NULL;
			} else {
				pdtep->pg_pte = 0;
				ptap = HATPT_PTAP(ptpp);
				hat_unlink_ptap(oas, ptap);
				ptap->hatpt_pdtep += delta;
				hat_link_ptap(nas, hatp->hat_ptlast, ptap);
			}
			oas->a_rss -= HATPT_AEC(ptpp);
			nas->a_rss += HATPT_AEC(ptpp);
			ASSERT(HATPT_LOCKS(ptpp) == 0);
			pae_hat_unprep(oas, savaddr, &ptpp, pdtep, kvaddr);
			addr = ((vaddr_t)addr + PAE_VPTSIZE) & PAE_VPTMASK;
		}
#ifdef DEBUG
		{
		pte64_t *sl1ptep, *el1ptep;

		sl1ptep = kvtol1ptep64(ostka);
		el1ptep = kvtol1ptep64(endaddr);
		for (; sl1ptep < el1ptep; sl1ptep++) {
			ASSERT(sl1ptep->pg_pte == 0);
		}
		}
#endif /* DEBUG */

		HATRL_UNLOCK(hatp, trypl);
		/*
		 * If there are any page tables left, the unload will flush
		 * the TLB. So, cover the unlikely case (swapping might cause
		 * it) that moving the new stack took the last PT.
		 */
		if (ohatp->hat_pts == (hatpt_t *)NULL)
			TLBSflushtlb();
		HATRL_UNLOCK_SVDPL(ohatp);
		if (nptap)
			kmem_free(nptap, sizeof(hatpt_t));
		return(0);
	}

	/*
	 * In the case of non-aligned PTEs, the PTEs would have to be
	 * copied to new page table(s).	 Since this is an extremely
	 * complex operation, for a case which is so rare that it will
	 * probably never occur, we just unload (and swap out) the old
	 * mapping and let references via the new address space fault
	 * the pages back in.
	 */
unload:

#ifdef DEBUG
	cmn_err(CE_NOTE, "hat_exec: couldn't do special case - unload instead");
#endif
	pae_hat_unload(oas, ostka, stksz, AS_UNLOAD, HAT_UNLOCK);
	return(0);
}

/*
 * void
 * pae_hat_asload(struct as *as)
 *	Add engine to active engine accounting in hat structure.
 *	Load the current process's address space into the MMU
 *	(page directory).  
 *
 * Calling/Exit State:
 *	Must grab and release hat_resourcelock.
 *
 *	Called by as_exec() after setting up the stack.
 *	Called during context switch to load new as.
 *
 * Remarks:
 *	PSE mappings are represented in the linked list of hatpt
 *	structures in the same way that page tables are.  This
 *	allows hat_asload to load PSE mappings along with other
 *	page directory entries without any special PSE code
 *	required or any performance penalty.
 */
void
pae_hat_asload(struct as *as)
{
	hatpt_t	*ptap, *eptap;
	hat_t *hatp;
	int i, j;
	pte64_t *l1ptep;

	ASSERT(as != (struct as *)NULL);
	ASSERT(as == u.u_procp->p_as);
	hatp = &as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	hatp->hat_activecpucnt++;
	EMASK_SETS(&hatp->hat_activecpubits, &l.eng_mask);

#ifdef DEBUG
	/*
	 * Ensure all user L1 page table entries are zero after
	 * unloading an address space.
	 */
	for (i = 0; i < PDPTSZ ; i++) {
		uint_t npde;

		l1ptep = l.kl1ptp64[i];
		if (i < (PDPTSZ - 1))
			npde = PAE_HAT_EPPT;
		else
			npde = pae_btoptbl(uvend - MINUVEND);
		for (j = 0; j < npde; j++, l1ptep++) {
			ASSERT(l1ptep->pg_pte == 0);	
		}
	}
#endif /* DEBUG */

        /*
         * See if we can just switch L1 page tables.
         */
	if (HAT_PRIVL164_LIST(hatp)) {
		pte64_t *kl2pt, *kl1pt;
		hatprivl164_t *hpl1p;
		vaddr_t kveng_l1pt;
		uint_t npdp;

		hpl1p = HAT_PRIVL164_LIST(hatp);
		kl2pt = &engine[l.eng_num].e_local_pae->pp_pmap[0][0];
		if (uvend == MINUVEND) {
			npdp = PDPTSZ - 1;
			kl1pt = &engine[l.eng_num].e_local_pae->pp_kl1pt[3][0];
		} else if (hpl1p->hp_kl1busy) {
			npdp = PDPTSZ - 1;
			kl1pt = &engine[l.eng_num].e_local_pae->pp_kl1pt[3][0];
			bcopy(hpl1p->hp_privl1virt[PDPTSZ - 1], KPDTE(MINUVEND),
				pae_ptnum(uvend - MINUVEND) * sizeof(pte64_t));
		} else {
			kl1pt = hpl1p->hp_privl1virt[PDPTSZ - 1];
			npdp = PDPTSZ;
			if (hpl1p->hp_cgnum != mycg) {
				/*
				 * We've switched CGs, so copy the local
				 * page tables (from this L1) into our
				 * private L1.  This also picks up the
				 * mapping of the kernel stack extension
				 * page, which is mapped prior to the
				 * hat_asunload (i.e., using this CG's
				 * ptes) and thus may not be mapped by
				 * the private L1 for this process.
				 */
				bcopy(KPDTE(kvbase), &kl1pt[pae_ptnum(kvbase)],
					(PAE_HAT_EPPT - pae_ptnum(kvbase))
						* sizeof(pte64_t));
				hpl1p->hp_cgnum = mycg;
				hpl1p->hp_engnum = l.eng_num;
			} else if (hpl1p->hp_engnum != l.eng_num) {
				/*
				 * We're on the same CG, but we switched
				 * engines, so we have to switch to
				 * KVPER_ENG L2 for this engine
				 */
				kl1pt[pae_ptnum(KVPER_ENG_STATIC_PAE)].pg_pte =
					pae_mkpte(PG_US | PG_RW | PG_V,
						pae_pfnum(kvtophys64((vaddr_t)kl2pt)));
				hpl1p->hp_engnum = l.eng_num;
			}
			hpl1p->hp_kl1busy = B_TRUE;
		}
		for (i = 0; i < npdp; i++) {

			if (hpl1p->hp_privl1[i]) {
				kveng_l1pt = KVENG_L1PT + (i * MMU_PAGESIZE);
				/* Switch kpd0 */
				kl2pt[pae_pgndx(kveng_l1pt)].pg_pte =
					pae_mkpte(PG_RW | PG_V,
						pae_pfnum(hpl1p->hp_privl1[i]));
				l.kl1ptp64[i] = hpl1p->hp_privl1virt[i];
				/* self reference an L1 */
				kl1pt[pae_ptnum(l.kvpte64[i])].pg_pte =
					pae_mkpte(PG_RW | PG_V,
						pae_pfnum(hpl1p->hp_privl1[i]));
				l.pdpte[i].pg_pte = pae_mkpte(PG_V, 
					pae_pfnum(hpl1p->hp_privl1[i])); 
			}
			/*
			 * Should we zero the KVENG_L1PT if an L1 does
			 * not exist.
			 */
		}
		TLBSflushtlb();
	} else {
		ptap = eptap = hatp->hat_pts;
		if (ptap != (hatpt_t *)NULL) {
			do {
				ptap->hatpt_pdtep->pg_pte = ptap->hatpt_pde.pg_pte;
				ptap = ptap->hatpt_forw;
			} while (ptap != eptap);
		}
	}

	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * void
 * pae_hat_asunload(struct as *as, boolean_t doflush)
 *	Unload level 1 entries of as and take engine out of active engine
 *	accounting of hat structure.
 *
 * Calling/Exit State:
 *	This is called on context switch when switching LWPs and from relvm().
 *	It does a TLB flush if the doflush is B_TRUE.
 *
 *	Must grab and release hat_resource lock.
 *
 * Remarks:
 *	PSE mappings appear in the linked list of hatpt structures,
 *	thus this routine transparently unloads such mappings.
 */
void
pae_hat_asunload(struct as *as, boolean_t doflush)
{
	struct hat *hatp;
	hatpt_t	*ptap, *eptap;
	pl_t savpl;
	boolean_t do_free;
	int i, j;
#ifdef DEBUG
	pte64_t *l1ptep;
#endif /* DEBUG */

	ASSERT(as != (struct as *)NULL);
	hatp = &as->a_hat;
	savpl = LOCK(&hatp->hat_resourcelock, PLHI);

	/*
	 * See if we just need to switch L1 page tables.
	 */
	if (HAT_PRIVL164_LIST(hatp)) {
		pte64_t *kl2pt, *kl1pt, *ul1pt;
		hatprivl164_t *hpl1p;
		paddr_t physul1pt;
		vaddr_t kveng_l1pt;
		uint_t npdp;

		hpl1p = HAT_PRIVL164_LIST(hatp);
		kl2pt = &engine[l.eng_num].e_local_pae->pp_pmap[0][0];
		kl1pt = &engine[l.eng_num].e_local_pae->pp_kl1pt[3][0];
		if (uvend == MINUVEND) {
			npdp = PDPTSZ - 1;
		} else if (hpl1p->hp_kl1busy &&
				(hpl1p->hp_engnum == l.eng_num)) {
			npdp = PDPTSZ;
			hpl1p->hp_kl1busy = B_FALSE;
		} else {
			npdp = PDPTSZ - 1;
			bzero(KPDTE(MINUVEND),
				pae_ptnum(uvend - MINUVEND) * sizeof(pte64_t));
		}
		for (i = 0; i < npdp; i++) {
			if (hpl1p->hp_privl1[i]) {
				kveng_l1pt = KVENG_L1PT + (i * MMU_PAGESIZE);
				ul1pt = &engine[l.eng_num].e_local_pae->pp_kl1pt[i][0];
				physul1pt = kvtophys64((vaddr_t)ul1pt);
				/* Switch kpd0 */
				kl2pt[pae_pgndx(kveng_l1pt)].pg_pte =
					 pae_mkpte(PG_RW | PG_V, pae_pfnum(physul1pt));

				l.kl1ptp64[i] = ul1pt;
				/* self reference an L1 */
				kl1pt[pae_ptnum(l.kvpte64[i])].pg_pte =
					pae_mkpte(PG_RW | PG_V, pae_pfnum(physul1pt));
				l.pdpte[i].pg_pte = pae_mkpte(PG_V, pae_pfnum(physul1pt));
			}
		}
		TLBSflushtlb();
	} else {
		ptap = eptap = hatp->hat_pts;
		if (ptap != (hatpt_t *)NULL) {
			do {
				ptap->hatpt_pdtep->pg_pte = 0;
				ptap = ptap->hatpt_forw;
			} while (ptap != eptap);
		}
	}

#ifdef DEBUG
	/*
	 * Ensure all user L1 page table entries are zero after
	 * unloading an address space.
	 */
	for (i = 0; i < PDPTSZ; i++) {
		uint_t npde;

		l1ptep = l.kl1ptp64[i];
		if (i < (PDPTSZ - 1))
			npde = PAE_HAT_EPPT;
		else
			npde = pae_btoptbl(uvend - MINUVEND);
		for (j = 0; j < npde; j++, l1ptep++) {
			ASSERT(l1ptep->pg_pte == 0);	
		}
	}
#endif /* DEBUG */

	if (doflush)
		TLBSflushtlb();

	/*
	 * Remove the CPU from the hat accounting.
	 */
	ASSERT(EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask));
	EMASK_CLRS(&hatp->hat_activecpubits, &l.eng_mask);
	do_free = (--hatp->hat_activecpucnt == 0 && as->a_free);
	if (SV_BLKD(&hatp->hat_sv)) {
		UNLOCK(&hatp->hat_resourcelock, savpl);
		SV_SIGNAL(&hatp->hat_sv, 0);
	} else {
		UNLOCK(&hatp->hat_resourcelock, savpl);
        }
	ASSERT(hatp->hat_activecpucnt >= 0);

	if (do_free) {
		/*
		 * We are releasing the last reference to the AS
		 * struture now. So therefore it is time to tear it
		 * down altogether.
		 */
		HAT_AS_FREE64(as, hatp);
	}
}

/*
 * boolean_t
 * pae_hat_kas_agerange(vaddr_t addr, vaddr_t endaddr)
 * 	Function to age address range in kernel visible mapping space.
 *
 * Calling/Exit State:
 *	No locks need be held on entry and no locks are held on exit.
 *	The caller guarantees that the specified range is not being
 *	accessed for the duration of this routine. The page vpglock
 *	is acquired and dropped within the function in order to traverse
 *	the mapping chain of the page.
 *
 *	This function does not protect against races with hat_pageunload,
 *	and therefore is only appropriate for anon pages.
 *
 *	Return value indicates if TLB will be required before the memory
 *	may be accessed.
 *
 * Description:
 *	This function goes through this address range and unloads the entries
 *	for whom the reference bit in the PTE is cleared  and clears the
 *	reference bit for the entries whose reference bit is set.
 *
 * Remarks:
 *	Called only for kvn mappings. We don't need to worry about any
 *	race conditions against aborts. 
 *
 *	Does not handle PSE mappings.
 */
boolean_t
pae_hat_kas_agerange(vaddr_t addr, vaddr_t endaddr)
{
	pte64_t *ptep;
	page_t *pp;
	boolean_t doflush = B_FALSE;
	int num_vpglocks = 0, i;
	vnode_t *vpglocks[HAT_MAX_VPGLOCKS] = {NULL, NULL};
	vnode_t *vp;
	uint_t ptesval;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ptep = kvtol2ptep64(addr);

	for (; addr < endaddr; addr += PAGESIZE, ptep++) {
	    ptesval = ptep->pte32.pg_low;
	    if (ptep->pg_pte == 0) {
		continue;
	    }
	    ASSERT(!(ptep->pg_pte & PG_LOCK));
	    doflush = B_TRUE;

#ifdef CCNUMA
	    FOR_EACH_CG_PTE(addr)
		ptesval |= cg_ptep->pte32.pg_low;
	    END_FOR_EACH_CG_PTE
#endif /* CCNUMA */

	    if (!(ptesval & PG_REF)) {
	        pp = pteptopp(ptep);
		ASSERT(pp != NULL);
		vp = pp->p_vnode;
		if (!HAT_VPGLOCK_EXISTS(vp, vpglocks)) {
			if (!HAT_TRYVPGLOCK(vp, num_vpglocks, vpglocks)) {
 				for (i = num_vpglocks; i > 0; i--) {
 				    if (i == 1)
					VN_PGUNLOCK_PL(vpglocks[i-1], PLBASE);
				    else
					VN_PGUNLOCK_PL(vpglocks[i-1], GL_PAGE_IPL);
				}
 				HAT_VPGLOCK(vp, num_vpglocks, vpglocks);
			}
		}
	 	ASSERT(pteptopp(ptep) == pp);

 		if (ptesval & PG_M) {
			/*
			 * This page is dirty. We don't want to recycle it
			 * if there is already a buildup of dirty pages the
			 * pageout daemon is backlogged on. We can get this
			 * page freed in a future trip, if the backlog clears
			 * by then.
			 */
			if (need_clean_pages)
				continue;
			pp->p_mod = 1;
		}

		/*
		 * Must remove pte from its p_hat_mapping list.
		 */
		(void) pae_hat_remptel(&kas, addr, pp, HAT_NOFLAGS);
#ifndef CCNUMA
		pae_clrpte(ptep);	/* ptep->pg_pte = 0; */
#else
		FOR_EACH_CG_PTE(addr)
			pae_clrpte(cg_ptep);
		END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */

	    } else /* PG_REF bit set */
#ifndef CCNUMA
		ptep->pg_pte &= ~PG_REF;
#else
		FOR_EACH_CG_PTE(addr)
			cg_ptep->pg_pte &= ~PG_REF;
		END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */
	}

	for (i = num_vpglocks; i > 0; i--) {
		if (i == 1)
			VN_PGUNLOCK_PL(vpglocks[i-1], PLBASE);
		else
			VN_PGUNLOCK_PL(vpglocks[i-1], GL_PAGE_IPL);
	}

	ASSERT(getpl() == PLBASE);
	return (doflush);
}

/*
 * vaddr_t
 * pae_hat_unload(struct as *as, vaddr_t addr, ulong_t len,
 *			enum age_type howhard, uint_t flags)
 *	Age the range [addr, endaddr] in specified as.
 *
 *	Never touch locked translations (page tables containing only
 *	locked translations can be skipped).
 *
 *	If howhard is AS_AGE, unreferenced (and unlocked) translations
 *	are removed and the page is freed, if appropriate. Referenced
 *	translations are made unreferenced.
 *
 *	If howhard is AS_SWAP, all unlocked translations are removed.
 *
 *	If howhard is AS_TRIM, MAXTRIM unlocked translations starting
 *	from addr are removed. The addr returned is the last removed
 *	addr + PAGESIZE.
 *
 * Calling/Exit State:
 *	This routine should not be called for hidden mappings.
 *
 *	The return addr is valid only in the case of AS_TRIM and it would be
 *	the next unloaded pte. The return would be endaddr if we have no more
 *	translations loaded for the as and trimcnt < MAXTRIM.
 *
 *	This function does not block.
 *
 *	This function may be called from out-of-context.
 *
 * Remarks:
 *	Refer to the design doc. for the design and implementation of the
 *	shootdown algorithm used in this function.
 *
 *	In case of AS_AGE and AS_TRIM, some translations that fit the
 *	criteria to get thrown out would be skipped if the trylock of the
 *	page the tranlation is pointing to is unsuccessful. This is 
 *	tolerable since will get them the next time around.
 *
 *	Segments mapped via PSE mappings should not be calling
 *	this routine; such segments call pse_hat_agerange instead.
 *
 *	VM_HASHLOCK is not necessary to protect the p_vnode fields of the
 *	pages found mapped; this is because the address space is expected
 *	to have the necessary holds on the vnodes. The address space 
 *	itself is stable because of the local_age_lock.
 *
 * Historical Note:
 *	p_hat_refcnt used to be potentially inaccurate in the old model
 *	where the per-page spin lock (the old page_uselock) would have
 *	have been needed to really keep it; and to avoid the cost of lock
 *	trips, we had chosen to maintain it without accuracy. This lack
 *	of accuracy in p_hat_refcnt was then made up for by explicitly
 *	sync'ing up the count under the cover of the spinlock. With
 *	locking reductions achieved by moving the protection of the page
 *	mapping chain up a level to the vnode layer (i.e., the substitution
 *	of page_uselock by the per-vnode-pagelock, we can maintain 
 *	p_hat_refcnt exactly, as is the case in the few lines below. This
 *	removed the need for periodic recomputation of p_hat_refcnt just
 *	for accuracy.
 *
 * AS_UNLOAD case:
 *	This routine does not block. The locks acquired by this routine are
 *	the hat resource lock, the VPGLOCK spinlocks for any of the pages,
 *	and the global vm_pgfreelk spinlock (gota free some pages).
 *
 *	User address spaces:
 *	The state on completion is that the level 1 PTEs that cover the
 *	range have been unloaded (minimizes TLBS hold time and there
 *	is a fast fault path to reload them), any translations in
 *	the range have been unloaded, any hat resources that are no longer
 *	needed have been freed, and any notinuse page has been freed.
 *	Any locks that were held by hat_unload are released.
 *
 *	Kernel address space:
 *	New visible kernel mapping interface obseletes this.
 *
 *	Historical note: we used to play a trick here, looking for
 *	fully unloaded page tables to hide under a temporary AS, in
 *	order to cut back on hat resource lock hold time. This job
 *	is now done a differnt way: hat_unloadpte calls intrpend
 *	to detect the need to drop. If so detected, then all locks
 *	dropped and we try again. Contention on the HAT resource lock
 *	this not an issue because:
 *	    => The AS write lock is held by the caller, and it
 *	       blocks all concurrent activity with two exceptions:
 *	       (a) out of context hat_agerange, and (b) hat_fastfault.
 *          => Out of context hat_agerange runs with the process seized,
 *             and therefore does not content with hat_unload (which
 *             always runs in context).
 *          => Hat_fastfault can only occur during driver access to
 *             softlocked data (i.e. during physio). Few applications
 *             generate physio concurrently with unmapping from
 *             another LWP. This is not viewed as an important
 *             performance case.
 *      On the other hand, it is essential for hat_unload to restore
 *      all L1 ptes before dropping locks, to allow hat_vtopte_l to
 *      use a dedicated L1 page table for optimized L2 lookup. This is
 *      an important optimization for physio.
 *
 *	Segments mapped PSE mappings should not be calling
 *	this routine; such segments call pse_hat_unload instead.
 */
vaddr_t
pae_hat_unload(struct as *as, vaddr_t addr_arg, ulong_t len,
	     enum age_type howhard, uint_t flags)
{
	pte64_t *ptep, *pdtep, *epdtep;
	hat_t *hatp;
	uint_t freedphys_pgs = 0; 		/* SAR counter */
	uint_t freedvirt_pgs = 0;		/* SAR counter */ 
	uint_t scanned_pgs = 0;			/* SAR counter */
	pl_t origpl;
	int num_vpglocks = 0;
	page_t *pp, *ptpp;
	vnode_t *vpglocks[HAT_MAX_VPGLOCKS] = {NULL, NULL};
	vnode_t *failed_vp = NULL;
	vnode_t *vp;
	boolean_t trylock_failed = B_FALSE;
	boolean_t flush = B_FALSE;
	int pndx;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	vaddr_t addr = addr_arg;
	vaddr_t endaddr = addr + len;
	vaddr_t savaddr;
	vaddr_t	shaddr;
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(((getpl() == PLBASE) && (howhard != AS_UNLOAD)) ||
		(howhard == AS_UNLOAD));
	ASSERT(endaddr <= (vaddr_t)uvend);
	ASSERT(as != &kas);
	ASSERT(endaddr >= addr);

	hatp = &as->a_hat;
	origpl = HATRL_LOCK(hatp);

top:
	shaddr = UNSHIELDED;
	if (pae_hat_prep(as, &addr, endaddr, HAT_FIRSTPT, &pdtep, &epdtep,
			&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE) {
		ASSERT(pdtep == (pte64_t *)NULL);
		HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
		HATRL_UNLOCK(hatp, origpl);
		ASSERT(getpl() == PLBASE);
		return endaddr;
	}

	ASSERT(HATPT_AEC(ptpp));
	ASSERT(pdtep);

	do {
		ASSERT(addr < endaddr);
		ASSERT(HATPT_AEC(ptpp) != 0);
		ASSERT(HATPT_AEC(ptpp) >= HATPT_LOCKS(ptpp)); 

		if (howhard == AS_SWAP) {

		    if (HATPT_AEC(ptpp) == HATPT_LOCKS(ptpp)) {
			pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
			addr = ((vaddr_t)addr + PAE_VPTSIZE) & PAE_VPTMASK;
			goto nextpt;
		    }

		    pndx = pae_pnum(addr);
		    for (; pndx < PAE_HAT_EPPT; pndx++, addr += PAGESIZE, ptep++) {
			    if (addr >= endaddr) {
				pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				goto bye;
			    }
			    if (ptep->pg_pte == 0 || ptep->pg_pte & PG_LOCK)
				continue;

			    freedvirt_pgs++;

			    if (ptep->pgm.pg_mod) {
				HAT_SET_MODSTATS(hatp, pdtep, pndx);
			    }

			    /*
			     * since howhard == AS_SWAP, we free the pte with
			     * 		flags == HAT_DONTNEED, below.
			     */
			    if (pae_hat_unloadpte(ptep, ptpp, 
					as, addr, flags|HAT_DONTNEED,
					&num_vpglocks, vpglocks, &failed_vp)) {
				ASSERT(failed_vp == NULL);
				freedphys_pgs++;		 
			    } else if (failed_vp) {
				/*
				 * hat_unloadpte failed. We must drop all
				 * locks and start over again.
				 */
				HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
				pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				HATRL_UNLOCK(hatp, origpl);
				ASSERT(VN_IS_HELD(failed_vp));
				HAT_VPGLOCK(failed_vp, num_vpglocks, vpglocks);
				failed_vp = NULL;
				(void) HATRL_LOCK(hatp);
				goto top;
			    }

			    if (HATPT_AEC(ptpp) == 0) {
				/*
				 * The zeroing and flushing of L1 ptes is done
				 * by the pae_hat_unprep function.
				 */
				pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				goto nextpt;
			    }
		    } /* for pndx loop */

		    pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

		} else if (howhard == AS_AGE || howhard == AS_TRIM) {

		    if (HATPT_AEC(ptpp) == HATPT_LOCKS(ptpp)) {
			pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
			addr = ((vaddr_t)addr + PAE_VPTSIZE) & PAE_VPTMASK;
			goto nextpt;
		    }

		    pndx = pae_pnum(addr);
		    for (; pndx < PAE_HAT_EPPT; pndx++, addr += PAGESIZE, ptep++) {
			    if (addr >= endaddr) {
				pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				goto bye;
			    }
			    if (ptep->pg_pte == 0 || ptep->pg_pte & PG_LOCK)
				continue;

			    pp = pteptopp(ptep);
			    ASSERT(pp);
			    ASSERT(pp->p_vnode != NULL);
			    vp = pp->p_vnode;
			    ASSERT(vp != NULL);
			    ASSERT(VN_IS_HELD(vp));

			    /*
			     * Acquire the vpglock
			     */
			    if (!HAT_VPGLOCK_EXISTS(vp, vpglocks)) {
				if (num_vpglocks == HAT_MAX_VPGLOCKS ||
				    !HAT_TRYVPGLOCK(vp, num_vpglocks, 
				    vpglocks)) {
					HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
					pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
					if (shaddr != UNSHIELDED) {
						pae_hat_uas_unshield(hatp,
								     shaddr,
								     endaddr);
						if (flush) {
							TLBSflushtlb();
							flush = B_FALSE;
						}
					}
					HATRL_UNLOCK(hatp, origpl);
					HAT_VPGLOCK(vp, num_vpglocks, vpglocks);
					HATRL_LOCK(hatp);
					goto top;
				}
			    }

			    if (howhard != AS_AGE || !(ptep->pg_pte & PG_REF)) {
				/*
				 * MAXTRIM ptes removed for AS_TRIM ? 
				 */
				if (howhard == AS_TRIM && freedvirt_pgs == MAXTRIM) {

				    pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				    goto bye;
				}

				scanned_pgs++;

				/*
				 * See historical note in commentary.
				 */
				if (ptep->pg_pte & PG_WASREF) {
				    ptep->pg_pte &= ~PG_WASREF;
				    pp->p_hat_refcnt--;
				}

				/*
				 * Shared page with outstanding reference count.
				 */
				if (pp->p_hat_refcnt > 0 && howhard != AS_TRIM)
				    continue;

				/*
				 * Unreferenced page.
				 */
				
				if (ptep->pgm.pg_mod) {
					if (need_clean_pages) {
						/* 
						 * There is already a buildup of dirty 
						 * pages to be flushed. Skip this page, 
						 * since it is only the age/trim case, 
						 * and this page cannot be freed 
						 * without a cleaning I/O, anyway.
						 */
						continue;
					}
					PAGE_SETMOD(pp);
					HAT_SET_MODSTATS(hatp, pdtep, pndx);
				}
				
				if (shaddr == UNSHIELDED &&
				    (ptep->pg_pte & PG_V))
					flush |= pae_hat_uas_shield(hatp,	
								    shaddr = addr,
								    endaddr, ptep);
				if (pae_hat_remptel(as, addr, pp, HAT_NOFLAGS))
					++freedphys_pgs;
				freedvirt_pgs++;

				if (flags & HAT_UNLOCK) {
					if (ptep->pg_pte & PG_LOCK)
						DECR_LOCK_COUNT(ptpp, as);
				}
				
				BUMP_RSS(as, -1);
				pae_clrpte(ptep);
				BUMP_AEC(ptpp, -1);
				
				if (HATPT_AEC(ptpp) == 0) {
					pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
					goto nextpt;
				}
			    } else { /* howhard == AS_AGE && PG_REF */
				if (shaddr == UNSHIELDED)
					atomic_and(&ptep->pte32.pg_low,	~PG_REF);
				else
					ptep->pte32.pg_low &= ~PG_REF;
				
				scanned_pgs++;
				if (!(ptep->pg_pte & PG_WASREF)) {
				    ASSERT(PAGE_VPGLOCK_OWNED(pp));
				    ptep->pg_pte |= PG_WASREF;
				    pp->p_hat_refcnt++;
				}
			    }
		    }  /* for pndx loop */
		    pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

		} else { 

		    ASSERT(howhard == AS_UNLOAD);
		    HAT_LOG(as, ptpp, addr, endaddr, 
			flags, "pae_hat_unload", HATPT_LOCKS(ptpp));

		    pndx = pae_pnum(addr);
		    for (; pndx < PAE_HAT_EPPT; pndx++, addr += PAGESIZE, ptep++) {
			    if (addr >= endaddr) {
				pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				goto bye;
			    }
			    if (ptep->pg_pte == 0)
				continue;

			    if (flags & HAT_HIDDEN) {
				if (flags & HAT_UNLOCK) {
				    if (ptep->pg_pte & PG_LOCK)
					DECR_LOCK_COUNT(ptpp, as);
				}
				
				if (shaddr == UNSHIELDED &&
				    (ptep->pg_pte & PG_V)) {
					flush |= pae_hat_uas_shield(hatp,	
								shaddr = addr,
								endaddr, ptep);
				}
				
				pae_clrpte(ptep);	/* ptep->pg_pte = 0; */
				BUMP_AEC(ptpp, -1);
				continue;
			    }
			    pp = pteptopp(ptep);
			    ASSERT(pp);
			    ASSERT(pp->p_vnode != NULL);
			    vp = pp->p_vnode;
			    ASSERT(vp != NULL);
			    ASSERT(VN_IS_HELD(vp));

			    /*
			     * Acquire the vpglock and if it fails,
			     * drop locks & start over
			     */
			    if (!HAT_VPGLOCK_EXISTS(vp, vpglocks)) {
				if (num_vpglocks == HAT_MAX_VPGLOCKS ||
				    !HAT_TRYVPGLOCK(vp, num_vpglocks, 
				    vpglocks)) {
					HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
					pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
					if(shaddr != UNSHIELDED) {
						pae_hat_uas_unshield(hatp,
								 shaddr, endaddr);
						if (flush) {
							TLBSflushtlb();
							flush = B_FALSE;
						}
					}
					HATRL_UNLOCK(hatp, origpl);
					HAT_VPGLOCK(vp, num_vpglocks, vpglocks);
					HATRL_LOCK(hatp);
					goto top;
				}
			    }
			    
			    if (shaddr == UNSHIELDED &&
				(ptep->pg_pte & PG_V)) {
				    flush |= pae_hat_uas_shield(hatp,	
							    shaddr = addr,
							    endaddr, ptep);
			    }
			    
			    /*
			     * See historical note in commentary.
			     */
			    if (ptep->pg_pte & PG_WASREF) {
				pp->p_hat_refcnt--;
			    }

			    /*
			     * Unreferenced page.
			     */
			    if (ptep->pgm.pg_mod) {
				    PAGE_SETMOD(pp);
				    HAT_SET_MODSTATS(hatp, pdtep, pndx);
			    }
	
			    if (pae_hat_remptel(as, addr, pp, HAT_NOFLAGS))
				    ++freedphys_pgs;
			    freedvirt_pgs++;

			    if (flags & HAT_UNLOCK) {
				    if (ptep->pg_pte & PG_LOCK)
					    DECR_LOCK_COUNT(ptpp, as);
			    }
			    BUMP_RSS(as, -1);
			    /* ptep->pg_pte = 0; */
			    pae_clrpte(ptep);
			    BUMP_AEC(ptpp, -1);
	
			    if (HATPT_AEC(ptpp) == 0) {
				    pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
				    goto nextpt;
			    }

		    }  /* for pndx loop */

		    pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);

		} /* howhard != AS_UNLOAD */

nextpt:
		if (pae_hat_prep(as, &addr, endaddr, HAT_FIRSTPT, &pdtep, &epdtep,
				&ptep, &ptpp, &kvaddr, &savaddr) == B_FALSE)
			goto bye;

	} while (addr < endaddr);

	/* for the AS_TRIM case. */
	ASSERT(howhard == AS_TRIM);
	addr = endaddr;

bye:
	HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);

	if (shaddr != UNSHIELDED)
		pae_hat_uas_unshield(hatp, shaddr, endaddr);

	if (flush)
		TLBSflushtlb();

	HATRL_UNLOCK(hatp, origpl);

	ASSERT(((getpl() == PLBASE) && (howhard != AS_UNLOAD)) ||
		(howhard == AS_UNLOAD));

	/* update sar counters */
	if (howhard == AS_AGE) {
		MET_VIRSCAN(scanned_pgs);
		MET_PHYSFREE(freedphys_pgs);
		MET_VIRFREE(freedvirt_pgs);
	} else if (howhard == AS_SWAP) {
		MET_PSWPOUT(freedphys_pgs);
		MET_VPSWPOUT(freedvirt_pgs);
	}
	return addr;
}

#ifdef OLD

/*
 * void
 * hat_start_stats(struct as *as, vaddr_t addr, ulong_t len)
 *	Start keeping mod bit history information for the given address
 *	range.
 *
 * Calling/Exit State:
 *	This function is always called in context.
 *
 * Remarks:
 *	There should be a matching pair of hat_start_stats() and 
 *	hat_stop_stats() for the given range.
 */
void
hat_start_stats(struct as *as, vaddr_t addr, ulong_t len)
{
	hat_t *hatp = &as->a_hat;
	pte64_t *vpdtep, *evpdtep;
	hat_stats_t *hat_modstatsp, *modstatsp = NULL;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((len & POFFMASK) == 0);
	ASSERT(u.u_procp->p_as == as);

	vpdtep = KPDTE(addr);
	evpdtep = KPDTE(addr + len - 1);

	HATRL_LOCK_SVPL(hatp);
	do {
tryagain:
		/* Returns the predecessor stats structure */
		hat_modstatsp = pae_hat_findstats(hatp, vpdtep);
		if (hat_modstatsp != NULL && hat_modstatsp->stats_pdtep == vpdtep) {
			hat_modstatsp->stats_refcnt++;
		} else {
			if (modstatsp == NULL) {
				HATRL_UNLOCK_SVDPL(hatp);
				modstatsp = kmem_alloc(
						sizeof(hat_stats_t), KM_SLEEP);
				modstatsp->stats_modinfo = kmem_zalloc(
						PAE_NPGPT, KM_SLEEP);
				HATRL_LOCK_SVPL(hatp);
				goto tryagain;
			}
			if (hat_modstatsp == NULL) {
				modstatsp->stats_next = hatp->hat_modstatsp;
				hatp->hat_modstatsp = modstatsp;
			} else {
				modstatsp->stats_next = hat_modstatsp->stats_next;
				hat_modstatsp->stats_next = modstatsp;
			}

			modstatsp->stats_refcnt = 1;
			modstatsp->stats_pdtep = vpdtep;
			/* Used up both the structures */
			modstatsp = NULL;
		}
	} while (++vpdtep <= evpdtep);

	HATRL_UNLOCK_SVDPL(hatp);
	if (modstatsp != NULL) {
		kmem_free(modstatsp->stats_modinfo, PAE_NPGPT);
		kmem_free(modstatsp, sizeof(hat_stats_t));
	}
}

/*
 * void
 * hat_stop_stats(struct as *as, vaddr_t addr, ulong_t len)
 *	Stop the stats collection of the mod bit history info.
 *	for the given range.
 *
 * Calling/Exit State:
 *	This function is always called in context.
 *
 * Remarks:
 *	There should be a matching pair of hat_start_stats() and 
 *	hat_stop_stats() for the given range.
 */
void
hat_stop_stats(struct as *as, vaddr_t addr, ulong_t len)
{
	hat_t *hatp = &as->a_hat;
	pte64_t *vpdtep, *evpdtep;
	hat_stats_t *prev_modstatsp, *hat_modstatsp, *tmp_modstatsp;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((len & POFFMASK) == 0);
	ASSERT(u.u_procp->p_as == as);

	vpdtep = KPDTE(addr);
	evpdtep = KPDTE(addr + len - 1);

	HATRL_LOCK_SVPL(hatp);
	/*
	 * Temporary hack for close on exec problem. If not for this
	 * problem, we should be able to assert this condition is not true.
	 */
	if (hatp->hat_modstatsp == NULL) {
		HATRL_UNLOCK_SVDPL(hatp);
		return;
	}

	hat_modstatsp = hatp->hat_modstatsp;
	if (hat_modstatsp->stats_pdtep == vpdtep)
		prev_modstatsp = NULL;
	else {
		while (hat_modstatsp->stats_next->stats_pdtep != vpdtep) {
			hat_modstatsp = hat_modstatsp->stats_next;
			ASSERT(hat_modstatsp != NULL);
		}
		prev_modstatsp = hat_modstatsp;
		hat_modstatsp = hat_modstatsp->stats_next;
	}
	do {
		ASSERT(hat_modstatsp->stats_pdtep == vpdtep);
		ASSERT(hat_modstatsp->stats_refcnt > 0);
		ASSERT(prev_modstatsp == NULL || 
		prev_modstatsp->stats_next == hat_modstatsp);

		if (--hat_modstatsp->stats_refcnt == 0) {
			kmem_free(hat_modstatsp->stats_modinfo, PAE_NPGPT);
			if (prev_modstatsp == NULL) {
				ASSERT(hatp->hat_modstatsp == hat_modstatsp);
				hatp->hat_modstatsp =  hat_modstatsp->stats_next;
			} else {
				prev_modstatsp->stats_next = hat_modstatsp->stats_next;
			}
			tmp_modstatsp = hat_modstatsp->stats_next;
			kmem_free(hat_modstatsp, sizeof(hat_stats_t));
			hat_modstatsp = tmp_modstatsp;
		} else {
			prev_modstatsp = hat_modstatsp;
			hat_modstatsp = hat_modstatsp->stats_next;
		}
	} while(++vpdtep <= evpdtep);

	HATRL_UNLOCK_SVDPL(hatp);
}

#endif /* OLD */


/*
 * void
 * pae_hat_check_stats(struct as *as, vaddr_t addr, ulong_t len, uint_t *vec,
 *			boolean_t clear)
 *	Check the modify bit information for the given address range.
 *
 * Calling/Exit State:
 *	This function is always called in context.
 *	Argument vec is an outarg, which on return, would contain a
 *	bit vector corresponding to the address range. If the corresponding
 *	bit is on, the page has been modified.
 *	vec is not assumed to be initialized by this function.
 *
 * Remarks:
 *	A preceding hat_start_stats() for the address range should have been 
 *	called.
 *
 * Description:
 *	Checks both the page table entry (if present) as well as the
 * 	stats structure for the mod bit information. Clears the stats
 *	mod bit information and the pte mod bit after recording, if the
 *	clear flag is set.
 */
void
pae_hat_check_stats(struct as *as, vaddr_t addr, ulong_t len, uint_t *vec, 
		boolean_t clear)
{
	hat_t *hatp = &as->a_hat;
	pte64_t *vpdtep, *evpdtep, *ptep, *pdtep, *epdtep;
	hat_stats_t *modstatsp;
	int indx, vec_indx, i;
	int ptlen;
	boolean_t flush = B_FALSE;
	page_t *pp;
	int  num_vpglocks = 0;
	vnode_t *vpglocks[HAT_MAX_VPGLOCKS] = {NULL, NULL};
	vaddr_t endaddr;
	page_t *ptpp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	vaddr_t savaddr;

#ifdef DEBUG
	ulong_t savlen = btop(len);
#endif

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((len & POFFMASK) == 0);
	ASSERT(u.u_procp->p_as == as);

	endaddr = addr + len - 1;
	vpdtep = KPDTE(addr);
	evpdtep = KPDTE(endaddr);

	HATRL_LOCK_SVPL(hatp);

	modstatsp = pae_hat_findstats(hatp, vpdtep);
	ASSERT(modstatsp != NULL);
	ASSERT(modstatsp->stats_pdtep == vpdtep);

	indx = pae_pnum(addr);
	len = btop(len);
	ASSERT(len != 0);
	vec_indx = 0;
	do {
		ASSERT(modstatsp != NULL);
		ASSERT(modstatsp->stats_pdtep == vpdtep);
		ASSERT(modstatsp->stats_refcnt >= 1);

		pae_hat_prep(as, &addr, endaddr, 0,
			&pdtep, &epdtep, &ptep, &ptpp, &kvaddr, &savaddr);
		if (pdtep == NULL || pdtep != vpdtep) {
		    ptlen = min(len, PAE_NPGPT);
		    for (i = 0; i < ptlen; i++, indx++, vec_indx++) {
			if (BITMASKN_TEST1(modstatsp->stats_modinfo, indx))
			    BITMASKN_SET1(vec, vec_indx);
			else
			    BITMASKN_CLR1(vec, vec_indx);
	
			if (clear)
			    BITMASKN_CLR1(modstatsp->stats_modinfo, indx);
		    }
		    len -= ptlen;
		} else {	/* page table exists for this address */
		    /*
		     * Reset addr when entering a new page table.
		     */
		    if (indx == 0)
			addr = (vaddr_t)addr & PAE_VPTMASK;

		    for (; indx < PAE_HAT_EPPT && len != 0; ptep++, indx++, vec_indx++, len--) {
			    if (ptep->pg_pte == 0) {
				if (BITMASKN_TEST1(modstatsp->stats_modinfo, indx))
				    BITMASKN_SET1(vec, vec_indx);
				else
				    BITMASKN_CLR1(vec, vec_indx);
			    } else {
				if (ptep->pg_pte & PG_M) {
				    BITMASKN_SET1(vec, vec_indx);
				    if (clear) {
					pp = pteptopp(ptep);
					ASSERT(pp != NULL);
					ASSERT(VN_IS_HELD(pp->p_vnode));
			
					if (!HAT_VPGLOCK_EXISTS(pp->p_vnode, 
					    vpglocks)) {
					    if (!HAT_TRYVPGLOCK(pp->p_vnode, 
						num_vpglocks, vpglocks))
						continue;
					}
					atomic_and(&ptep->pte32.pg_low, ~PG_M);
				        pp->p_mod = 1;
				        PAGE_SETMOD(pp);
				        flush = B_TRUE;
				        hat_uas_shootdown_l(hatp);
				    }
				} else {
				    if (BITMASKN_TEST1(modstatsp->stats_modinfo, indx))
					BITMASKN_SET1(vec, vec_indx);
				    else
					BITMASKN_CLR1(vec, vec_indx);
				}
			    }
			    if (clear)
				BITMASKN_CLR1(modstatsp->stats_modinfo, indx);
		    }
		}
		indx = 0;
		modstatsp = modstatsp->stats_next;
		addr = ((vaddr_t)addr + PAE_VPTSIZE) & PAE_VPTMASK;
		pae_hat_unprep(as, savaddr, &ptpp, pdtep, kvaddr);
	} while (++vpdtep <= evpdtep && len != 0);

	ASSERT(vec_indx == savlen);

	if (flush)
		TLBSflushtlb();

	HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * vaddr_t
 * pae_hat_dup(struct seg *pseg, struct seg *cseg, uint_t flags) 
 *	Preload child page tables based on parent PT entries and flags.
 *
 * Calling/Exit State:
 *	No locks need be held on entry and none are held on exit.
 *	The cseg is pre-established. Calls to kmem_zalloc and kpg_alloc are
 *	possible (constraint on hierarchy of any spinlocks relative to KMA
 *	locks held by caller of hat_dup). The child AS's hat resource lock
 *	must be available to grab, since hat_dup must grab both the
 *	parent's and the child's lock and there can be contention for the
 *	parent's lock. Alas, there CAN also be contention for the child's
 *	lock after the first hat_dup (i.e., after the first segment), since
 *	hat_pageunload() (and hat_pagesyncmod()) can grab the lock. This is
 *	one of the few cases where a LOCK_SH is permissible (No possibility
 *	of an AB/BA deadlock). Parent's pages are also PAGE_TRYVPGLOCKed by
 *	hat_dupmc() which is called by hat_dup. Returns the first addr not
 *	processed.
 *
 * Description:
 *	This function calls hat_dupmc() for each mapping chunk that is
 *	being dup'ed. No sleeping is done in this execution path. We 
 *	skip any pages that cannot be tryvpglocked and we exit on resource
 *	exhaustion.
 *
 * Remarks:
 *	Segments mapped via PSE mappings should not be calling
 *	this routine; such segments call pse_hat_dup instead.
 */
vaddr_t
pae_hat_dup(struct seg *pseg, struct seg *cseg, uint_t flags)
{
	vaddr_t addr = pseg->s_base;
	vaddr_t endaddr = pseg->s_base + pseg->s_size;
	hat_t *phatp = &pseg->s_as->a_hat;
	hat_t *chatp = &cseg->s_as->a_hat;
	pte64_t *pptep, *ppdtep, *pepdtep;
	int ppndx;
	enum hat_cont_type res;
	int num_vpglocks = 0;
	vnode_t *vpglocks[HAT_MAX_VPGLOCKS] = {NULL, NULL};
#ifdef DEBUG
	pl_t entry_pl = getpl();
	int entry_locks = hier_lockcount(0);
#endif /* DEBUG */
	struct as *pas = pseg->s_as;
	page_t *pptpp;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT1;
	vaddr_t savaddr;

	ASSERT(pseg->s_base == cseg->s_base);
	ASSERT(pseg->s_size == cseg->s_size);

start:
	ASSERT(entry_pl == getpl());
	ASSERT(entry_locks == hier_lockcount(0));

	/*
	 * Instead of pre-allocating the page table page for the 
	 * child in order not to drop locks later, a refresh of 
	 * the pagepool is done, so that the probability of failing
	 * page table allocation is significantly reduced.
	 */
	hat_refreshpools();

	HATRL_LOCK_SVPL(phatp);

	/*
	 * The child hat lock is locked by LOCK_SH primitive 
	 * because the parent lock is held at this time. LOCK_SH
	 * o.k. here since deadlock should not happen here. If
	 * that is found to be FALSE, then hat_dup must stop
	 * after one TRYLOCK and return(addr).
	 */
	HATRL_LOCK_SH(chatp);

next:
locks_dropped:
	/*
	 * Find the page table in the address range that is left
	 * in the seg being duped.
	 */
	if (pae_hat_prep(pas, &addr, endaddr, HAT_FIRSTPT, &ppdtep,
			&pepdtep, &pptep, &pptpp, &kvaddr, &savaddr) == B_FALSE)
		goto nomoreret;

	ASSERT(addr >= pseg->s_base);

	ppndx = pae_pnum(addr);
	for (; ppndx < PAE_HAT_EPPT; ppndx++, pptep++, addr += PAGESIZE) {
		if (addr >= endaddr) {
			pae_hat_unprep(pas, savaddr, &pptpp, ppdtep, kvaddr);
			goto nomoreret;
		}

		if (pptep->pg_pte == 0)
			continue;

		res = pae_hat_dup_range(cseg, pptep, &addr, endaddr, 
				ppndx, &num_vpglocks, vpglocks, flags);
		ASSERT(res == NOMORE || res == ALLOCPT ||
			res == DROPLOCKS || res == TRYLOCK_FAIL ||
			res == NEXTPT);

		switch (res) {
		case NOMORE:
			pae_hat_unprep(pas, savaddr, &pptpp, ppdtep, kvaddr);
			goto nomoreret;

		case ALLOCPT:
			HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
			vpglocks[0] = NULL;
			pae_hat_unprep(pas, savaddr, &pptpp, ppdtep, kvaddr);
			HATRL_UNLOCK(chatp, VM_HAT_RESOURCE_IPL);
			HATRL_UNLOCK_SVDPL(phatp);
			goto start;

		case DROPLOCKS:
			HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
			pae_hat_unprep(pas, savaddr, &pptpp, ppdtep, kvaddr);
			HATRL_UNLOCK(chatp, VM_HAT_RESOURCE_IPL);
			HATRL_UNLOCK_SVDPL(phatp);
			HATRL_LOCK_SVPL(phatp);
			HATRL_LOCK_SH(chatp);
			goto locks_dropped;

		case TRYLOCK_FAIL:
			pae_hat_unprep(pas, savaddr, &pptpp, ppdtep, kvaddr);
			HATRL_UNLOCK(chatp, VM_HAT_RESOURCE_IPL);
			HATRL_UNLOCK_SVDPL(phatp);
			HAT_VPGLOCK(vpglocks[0], num_vpglocks, vpglocks);
			HATRL_LOCK(phatp);
			phatp->hat_lockpl = l.vpglockpl;
			HATRL_LOCK_SH(chatp);
			goto locks_dropped;
		case NEXTPT:
			pae_hat_unprep(pas, savaddr, &pptpp, ppdtep, kvaddr);
			if (addr >= endaddr)
				goto nomoreret;
			goto next;
		} /* end switch */
		/* NOTREACHED */
	} /* ppndx loop */
	goto next;
	/* NOTREACHED */

nomoreret:
	HAT_VPGUNLOCK_ALL(num_vpglocks, vpglocks);
	UNLOCK(&chatp->hat_resourcelock, VM_HAT_RESOURCE_IPL);
	HATRL_UNLOCK_SVDPL(phatp);
	ASSERT(entry_pl == getpl());
	ASSERT(entry_locks == hier_lockcount(0));
	return(addr);
}

/*
 * STATIC enum hat_cont_type
 * pae_hat_dup_range(struct seg *cseg, pte64_t *pptep,
 *		vaddr_t *addrp, vaddr_t endaddr, int pndx,
 *		int *num_vpglocks, vnode_t **vpglocks, uint_t flags)
 * 	Loop over the parent's chunk and copy the parent's translations
 *	to the child. Skip copying for translations that have the write
 *	permission in the parent's AS if the flags argument is set to
 *	HAT_NOPROTW, meaning it is a MAP_PRIVATE segment.
 *
 * Calling/Exit State:
 *	The caller of this function is expected to hold the hat resource
 *	locks of the parent and the child. This function TRYVPGLOCKs the
 * 	parent's pages and if it fails returns. It does not block.
 *	There are four returns possible from this function:
 *
 *	ALLOCPT tells the caller that we failed to allocate pages
 * 	for page table.
 *
 *	DROPLOCKS tells the caller that there's an interrupt pending
 *	and it should drop and re-take all locks.
 *
 *	TRYLOCK_FAIL tells the caller that a TRYVPGLOCK failed for the
 *	last page we tried to process.  The VPGLOCKs have been dropped,
 *	but the p_vnode on which we failed is passed back in vpglocks[0]
 *	so that the caller may lock it the hard way if so desired.
 *
 *	NOMORE tells the caller that it's time to bail out because
 *	we have finished dup'ing the parent.
 *
 * Remarks:
 *	The parent's permissions are never changed. If HAT_HIDDEN is specified
 *	in flags, the translations are inherently shared, but do not appear on
 *	any possible p_hat_mapping lists. If HAT_HIDDEN is not in flags,
 *	there must be page_t's for each translation. In the latter case,
 *	processing depends on whether pseg is shared or private. Either pseg is
 *	shared (HAT_NOPROTW not specified in flags), which means no permissions
 *	checks are needed, or it is private (HAT_NOPROTW is in flags) and we
 *	immediately need to skip dup'ing writable translations into the child.
 */
STATIC enum hat_cont_type
pae_hat_dup_range(struct seg *cseg, pte64_t *pptep, 
	  vaddr_t *addrp, vaddr_t endaddr, int pndx,
	  int *num_vpglocks, vnode_t **vpglocks, uint_t flags)
{
	pte64_t *cptep, cpteval, *cpdtep, *cepdtep;
	page_t *cpp;
	struct as *cas = cseg->s_as;
	boolean_t more_pts;
	int skip_writable;
	vaddr_t addr = *addrp;
	enum hat_cont_type ret = NEXTPT;
	vaddr_t kvaddr = (vaddr_t)KVTMPPT2;
	vaddr_t savaddr;
	page_t *cptpp;

	/*
	 * If we consume new_ptap or new_mc, must we allocate another?
	 */
	more_pts = (pae_ptnum(endaddr - 1) > pae_ptnum(addr));

	/*
	 * flag that tells if there are any cowable pages in this range
	 * in pseg.
	 */
	skip_writable = flags & HAT_NOPROTW;

	/*
	 * Now that parent is set up with something to work on,
	 * see what state the child is in.
	 */
	if (pae_hat_prep(cas, &addr, endaddr, HAT_ALLOCPT, &cpdtep, &cepdtep,
			&cptep, &cptpp, &kvaddr, &savaddr) == B_FALSE) {
		return (more_pts ? ALLOCPT : NOMORE);
	}

	/*
	 * Now cptap is either NULL (no entries in chatp yet),
	 * it is the desired ptap (cptap->hatpt_pdtep == vpdtep),
	 * or it is the prevptap for link_ptap() and the desired 
	 * entry is absent.
	 */
	if (flags & HAT_HIDDEN) {
		for (; pndx < PAE_HAT_EPPT; addr += PAGESIZE, pptep++, cptep++, pndx++) {
			if (addr >= endaddr) {
				*addrp = addr;
				pae_hat_unprep(cas, savaddr, &cptpp, cpdtep, kvaddr);
				return NOMORE;
			}
			if (pptep->pg_pte & PG_V) {
				cptep->pg_pte = pptep->pg_pte &
					~(PG_LOCK | PG_WASREF);
				BUMP_AEC(cptpp, 1);
			}
		}
		goto bye;	
	} /* flags & HAT_HIDDEN */

	/*
	 * Only visible mappings are left (pages backed by real
	 * memory as opposed to magic memory, like a mapped I/O bus).
	 * Translations appear on page_t p_hat_mapping chains.
	 */

	for (; pndx < PAE_HAT_EPPT; addr += PAGESIZE, pptep++, cptep++, pndx++) {
		if (addr >= endaddr) {
			*addrp = addr;
			pae_hat_unprep(cas, savaddr, &cptpp, cpdtep, kvaddr);
			return NOMORE;
	    	}
		if ((pptep->pg_pte & PG_V) == 0)
			continue;

		/* Don't propagate lock or wasref bits to child.  */
		cpteval.pg_pte = pptep->pg_pte & ~(PG_LOCK | PG_WASREF);
		cpp = pteptopp(pptep);
		ASSERT(cpp != NULL);
		if (skip_writable && (pptep->pg_pte & PG_RW))
			continue;
		/*
		 * We found a loaded page, and have no instructions to skip it.
		 * We are going to just copy the translation over to the child's
		 * address space. We need to VPGLOCK the page ourselves to do
		 * this. Because we hold the HAT lock, however, we can't get the
		 * vpglock w/o violating the lock hierarchy. We trylock it; if
		 * that doesn't work, return and let the caller deal with it.
		 */

		ASSERT(VN_IS_HELD(cpp->p_vnode));

		if (!HAT_VPGLOCK_EXISTS(cpp->p_vnode, vpglocks)) {
			if (!HAT_TRYVPGLOCK(cpp->p_vnode, *num_vpglocks, vpglocks)) {
				HAT_VPGUNLOCK_ALL(*num_vpglocks, vpglocks);
				vpglocks[0] = cpp->p_vnode;
				ret = TRYLOCK_FAIL;
				goto bye;
			}
		}

		ASSERT(pteptopp(&cpteval) == cpp);

		/*
		 * Load the translation into the child using the
		 * the same permissions we found in the parent.
		 * Link into p_hat_mapping.
		 */
		/* cptep->pg_pte = cpteval.pg_pte; */
		pae_setpte(&cpteval, cptep);
		PAGE_UAS_LOAD(cas, addr, cpp);

		ASSERT(cpteval.pg_pte);
		ASSERT((cpteval.pg_pte & PG_LOCK) == 0);

		MET_PREATCH(1);
		BUMP_RSS(cas, 1);
		BUMP_AEC(cptpp, 1);

		if (intrpend(PLBASE)) {
			/*
			 * We will return DROPLOCKS code to the caller. The
			 * caller knows to release vpglocks and other locks.
			 */
			addr += PAGESIZE;
			ret = DROPLOCKS;
			goto bye;
		}
	} /* end pndx < PAE_HAT_EPPT */

bye:
	*addrp = addr;
	pae_hat_unprep(cas, savaddr, &cptpp, cpdtep, kvaddr);

	return ret;
}

/*
 * STATIC hatpt_t *
 * pae_hat_findpt(hat_t *hatp, pte64_t *vpdtep)
 *	Find the page table corresponding to the l1 entry.
 *
 * Calling/Exit State:
 *	The hat resource lock for hatp is held and is returned held.
 *	This routine doesn't block. If desired pt exists, return the
 *	pt. If not, return the preceding page table. If no page table
 *	exists for the address space, return NULL.
 *
 * Remarks:
 * 	Global so that it can be called from pse_hat.c
 */
STATIC hatpt_t *
pae_hat_findpt(hat_t *hatp, pte64_t *vpdtep)
{
	hatpt_t *eptap, *ptap;

	ptap = hatp->hat_ptlast;
	if ((ptap == (hatpt_t *)NULL) || (ptap->hatpt_pdtep == vpdtep)) {
		return ptap;
	}
	eptap = ptap;
	if (ptap->hatpt_pdtep > vpdtep) {
		do {
			ptap = ptap->hatpt_back;
		} while ((ptap->hatpt_pdtep > vpdtep) &&
			 (ptap->hatpt_pdtep < eptap->hatpt_pdtep));
		/*
		 * Either we found the ptap desired or it is missing and
		 * we found the ptap that would be the predecessor in the
		 * ptap list.
		 */
	} else {
		do {
			ptap = ptap->hatpt_forw;
		} while ((ptap->hatpt_pdtep < vpdtep) &&
			 (ptap->hatpt_pdtep > eptap->hatpt_pdtep));
		/*
		 * If all are before the desired place, we stop
		 * at one too far. If not, we back up one.
		 */
		if (ptap->hatpt_pdtep != vpdtep)
			ptap = ptap->hatpt_back;
	}
	return ptap;
}

/*
 * STATIC hat_stats_t *
 * pae_hat_findstats(hat_t *hatp, pte64_t *vpdtep)
 *	Find the stats structure corresponding to the l1 entry.
 *
 * Calling/Exit State:
 *	The hat resource lock for hatp is held and is returned held.
 *	This routine doesn't block. If desired stats struct. exists,
 *	return the struct. If not, return the preceding structure.
 *	If the l1 pointer is less than any existing stats structs or
 *	if there are no existing structs for the address space,
 *	returns NULL.
 */
hat_stats_t *
pae_hat_findstats(hat_t *hatp, pte64_t *vpdtep)
{
	hat_stats_t *modstatsp;

	if ((modstatsp = hatp->hat_modstatsp) == NULL ||
			vpdtep < modstatsp->stats_pdtep)
		return NULL;

	 while (modstatsp->stats_next != NULL) {
		ASSERT(vpdtep >= modstatsp->stats_pdtep);
		if (modstatsp->stats_pdtep == vpdtep ||
			vpdtep > modstatsp->stats_next->stats_pdtep)
			return modstatsp;

		modstatsp = modstatsp->stats_next;
	}

	return modstatsp;
}


/*
 * STATIC hatpt_t *
 * pae_hat_findfirstpt(vaddr_t *addr, hat_t *phatp)
 *	Find the first existing page table in an address space starting
 *	from addr.
 *
 * Calling/Exit State:
 *	The hat resource lock for phatp is held and is returned held.
 */
STATIC hatpt_t *
pae_hat_findfirstpt(vaddr_t *addr, hat_t *hatp)
{
	hatpt_t *ptap;
	pte64_t *vpdtep;
#ifdef DEBUG
	vaddr_t savaddr = *addr;
#endif

	ptap = hatp->hat_ptlast;
	if (ptap == (hatpt_t *)NULL) {
		ASSERT(hatp->hat_pts ==(hatpt_t *) NULL);
		return NULL;
	}

	vpdtep = KPDTE(*addr);

	/* check if the l1 entry is what we are looking for */
	if (ptap->hatpt_pdtep == vpdtep)
		return ptap;

	/*
	 * The following complex looking condition just checks if
	 * the addr passed in is beyond the last page table for
	 * this AS.
	 */
	if (hatp->hat_pts->hatpt_back->hatpt_pdtep < vpdtep)
	  	return ((hatpt_t *)NULL);

	/*
	 * Start from hat_pts if ptlast is beyond the pt
	 * we are looking for.
	 */
	if (ptap->hatpt_pdtep > vpdtep) {
		ptap = hatp->hat_pts;
		hatp->hat_ptlast = ptap;
	}

	while (ptap->hatpt_pdtep < vpdtep) {
		ptap = ptap->hatpt_forw;
		ASSERT(ptap != hatp->hat_pts);
	}

	hatp->hat_ptlast = ptap;

	ASSERT(ptap->hatpt_pdtep >= vpdtep);

	if (ptap->hatpt_pdtep != vpdtep)
		*addr = (vaddr_t)((ptap->hatpt_pdtep - KPDTE(0)) * PAE_VPTSIZE);

	ASSERT(*addr >= savaddr);

	return ptap;
}

/* The following are for debugging only */

/*
 * void
 * print_pae_page_table(pte64_t *ptep)
 *	Print page table.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_pae_page_table(pte64_t *ptep)
{
	int i;

	for (i = 0; i < PAE_HAT_EPPT; i++, ptep++) {
		if (ptep->pg_pte == 0)
			continue;
		debug_printf("%x: PG_PFN64=%x, PG_US=%x, PG_RW=%x, PG_V=%x\n", i,
			PFN64(ptep), ptep->pgm.pg_us, ptep->pgm.pg_rw, ptep->pgm.pg_v);
	}
}

/*
 * pte64_t *
 * print_kvtol1ptep64(vaddr_t addr)
 *      Debug routine.
 *
 * Calling/Exit State:
 *      None.
 */
pte64_t *
print_kvtol1ptep64(vaddr_t addr)
{
	pte64_t *ptep;

	ptep = kvtol1ptep64(addr);
	debug_printf("addr=%x, ptep=%x, pte=%Lx\n", addr, ptep, ptep->pg_pte);
	return ptep;
}

/*
 * pte64_t *
 * print_kvtol2ptep64(vaddr_t addr)
 *      Debug routine.
 *
 * Calling/Exit State:
 *      None.
 */
pte64_t *
print_kvtol2ptep64(vaddr_t addr)
{
	pte64_t *ptep;

	ptep = kvtol2ptep64(addr);
	debug_printf("addr=%x, ptep=%x, pte=%Lx\n", addr, ptep, ptep->pg_pte);
	return ptep;
}

/*
 * void
 * print_kvtophys64(vaddr_t addr)
 *      Debug routine.
 *
 * Calling/Exit State:
 *      None.
 */
void
print_kvtophys64(vaddr_t addr)
{
	paddr_t paddr;

	paddr = kvtophys64(addr);
	debug_printf("vaddr=%x, paddr=%x\n", addr, paddr);
}

/*
 * pte64_t *
 * print_kvtol2ptep64_g(vaddr_t addr, cgnum_t cgnum)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
pte64_t *
print_kvtol2ptep64_g(vaddr_t addr, cgnum_t cgnum)
{
	pte64_t *ptep = kvtol2ptep64_g(addr, cgnum);

	debug_printf("addr=%x, cgnum=%d, ptep=%x, pte=%Lx\n", 
			addr, cgnum, ptep, ptep->pg_pte);
	return ptep;
}

/*
 * pte64_t *
 * print_kvtol2pteptep64_gvaddr_t addr, cgnum_t cgnum)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
pte64_t *
print_kvtol2pteptep64_g(vaddr_t addr, cgnum_t cgnum)
{
        pte64_t *ptep = kvtol2pteptep64_g(addr, cgnum);

        debug_printf("addr=%x, cgnum=%d, ptep=%x pte=%Lx\n", 
			addr, cgnum, ptep, ptep->pg_pte);
	return ptep;
}

/*
 * void
 * pae_hat_page_wt(void *, uint_t)
 *	Change the mapped pages from cache write-back to cache write-through.
 *
 * Inputs:
 *	addr: Virtual address of the starting page.
 * 	nbytes: span of the pages
 *
 * Note:
 *	The following is for some bus-mastering device which may need
 *	to have the pages to be cache write-through
 */
void
pae_hat_page_wt(void *addr, uint_t nbytes)
{
	uint_t	i, npg;
	vaddr_t	va;
	pte64_t	*ptep;

	npg = btopr(nbytes);
	va = (vaddr_t)addr;
	va &= PAGEMASK;
	for (i = 0 ; i < npg ; ++i) {
#ifndef CCNUMA
		ptep = kvtol2ptep64(va);
		ptep->pgm.pg_wt = 1;
#else
		FOR_EACH_CG_PTE(va)
			cg_ptep->pgm.pg_wt = 1;
		END_FOR_EACH_CG_PTE
#endif /* !CCNUMA */
		va += PAGESIZE;
	}

	TLBSflushtlb();

	/* flushtlb across processors */
	hat_shootdown((TLBScookie_t)0, HAT_NOCOOKIE);
}

/*
 * void
 * pae_hat_switch_l1(struct as *as)
 *	If address space is large enough, then switch to a private
 *	L1 page table.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Steps to switch to a dedicated L1:
 *
 *		- Allocate a page to use as our private L1 page table.
 *		- Copy the current L1PT into our private copy.
 *		- The KVPER_ENG_STATIC entry is already set up.
 *		- Unload the entries in the original L1PT.
 *		- Save the current L1 page table so we can restore it
 *		  restore it in hat_asunload.
 *		- Set up the KVPTE entry to refer back to the new L1.
 *		- Load the cr3 register and release the locks.
 */
void
pae_hat_switch_l1(struct as *as)
{
        struct hat      *hatp;
        hatpt_t         *ptap, *nptap;
        pl_t            savpl;
        hatprivl164_t   *privp;
        pte64_t         *kl1pt;
        pte64_t         *kl2pt;
        pte64_t         *tmp;
	uint_t		ptndx, i, pdptndx, npdp;
	boolean_t	inctx;
	vaddr_t		addr3, kveng_l1pt, addr;

        ASSERT(as != (struct as *)NULL);

        hatp = &as->a_hat;
	inctx = (u.u_procp->p_as == as);

	ASSERT(HAT_PRIVL164_LIST(hatp) == (hatprivl164_t *)NULL);

	npdp = (uvend == MINUVEND) ? PDPTSZ - 1 : PDPTSZ;

        tmp = (pte64_t *)kpg_alloc(npdp, PROT_READ|PROT_WRITE, P_NODMA|SLEEP);
        privp = kmem_zalloc(sizeof(hatprivl164_t), KM_SLEEP);
        savpl = LOCK(&hatp->hat_resourcelock, PLHI);

	/*
	 * In a multi-lwp case, avoid switching to an private L1,
	 * if a lwp is currently running on a remote engine.
	 */
	i = minonlinecpu;
	while (i <= maxonlinecpu) {
		if (i != l.eng_num && EMASK_TEST1(&hatp->hat_activecpubits, i)) {
			SV_WAIT(&hatp->hat_sv, PRIMEM, &hatp->hat_resourcelock);
			(void) LOCK(&hatp->hat_resourcelock, PLHI);
			i = minonlinecpu;
		} else {
			i++;
		}
	}

	DISABLE_PRMPT();

	for (i = 0, addr3 = (vaddr_t)tmp; i < npdp; 
			i++, addr3 += PAGESIZE) {
		if (inctx) {
		        bcopy((char *)l.kpd64[i], 
				(char *)addr3, PAE_NPGPT * sizeof(pte64_t));
		} else if (i < (PDPTSZ - 1)) {
			bzero((char *)addr3, PAE_NPGPT * sizeof(pte64_t));
		} else {
			bcopy((char *)l.kpd64[i],
				(char *)addr3, PAE_NPGPT * sizeof(pte64_t));
			bzero((char *)addr3, pae_btoptbl(uvend - MINUVEND) *
				sizeof(pte64_t));
		}
	}

        /*
         * Another lwp could have allocated the private L1PT
         * while we slept.  If so just give up because multiple
	 * lwps share the private L1.
         */
	if (HAT_PRIVL164_LIST(hatp)) {
		UNLOCK(&hatp->hat_resourcelock, savpl);
		ENABLE_PRMPT();
		kpg_free(tmp, npdp);
		kmem_free(privp, sizeof(hatprivl164_t));
		return;
	}
	ENABLE_PRMPT();

	for (i = 0, addr3 = (vaddr_t)tmp; i < npdp; 
			i++, addr3 += PAGESIZE) {
		privp->hp_privl1virt[i] = (pte64_t *)addr3;
		privp->hp_privl1[i] = kvtophys64((vaddr_t)privp->hp_privl1virt[i]);
	}
	privp->hp_kl1busy = B_FALSE;
	privp->hp_cgnum = mycg;
	privp->hp_engnum = l.eng_num;
	HAT_PRIVL164_LIST(hatp) = privp;
	ptap = hatp->hat_pts;
	if (ptap != (hatpt_t *)NULL) {
		ASSERT(hatp->hat_ptlast != (hatpt_t *)NULL);
                do {
                        nptap = ptap->hatpt_forw;
			addr = (ptap->hatpt_pdtep - KPDTE(0)) * PAE_VPTSIZE;
			pdptndx = PDPTNDX64(addr);
			ptndx = pae_ptnum(addr);
			if (inctx) {
				ASSERT((privp->hp_privl1virt[pdptndx] + ptndx)->pg_pte == ptap->hatpt_pdtep->pg_pte); 
				/* already copied to an L1 above */
				ptap->hatpt_pdtep->pg_pte = 0;
			} else {
				/*
				 * No other LWPs are active on remote cpus.
				 * It implies that the stores can be in
				 * any order (low, high or high then low).
				 */
				(privp->hp_privl1virt[pdptndx] + ptndx)->pg_pte = 
							ptap->hatpt_pde.pg_pte;
			}
			hat_unlink_ptap(as, ptap);
			kmem_free(ptap, sizeof(hatpt_t));
                        ptap = nptap;
                } while (hatp->hat_pts != (hatpt_t *)NULL);
        }

	ASSERT(hatp->hat_pts == (hatpt_t *)NULL);
	ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);

        /*
         * Switch to using the private level 1 now rather than
         * waiting for the next context switch.  Switching now
         * simplifies the context switch code (which runs more
         * frequently) at the cost of some added complexity
         * here (which runs much less frequently).
         */

	if (inctx) {
		kl2pt = &engine[l.eng_num].e_local_pae->pp_pmap[0][0];
		if (npdp < PDPTSZ)
			kl1pt = &engine[l.eng_num].e_local_pae->pp_kl1pt[3][0];
		else {
			kl1pt = privp->hp_privl1virt[3];
			privp->hp_kl1busy = B_TRUE;
		}
		for (i = 0; i < npdp; i++) {
			if (privp->hp_privl1[i]) {
				kveng_l1pt = KVENG_L1PT + (i * MMU_PAGESIZE);
				/* Switch kpd0 */
				kl2pt[pae_pgndx(kveng_l1pt)].pg_pte =
					pae_mkpte(PG_RW | PG_V,
						pae_pfnum(privp->hp_privl1[i]));
				l.kl1ptp64[i] = privp->hp_privl1virt[i];
				/* self reference an L1 */
				kl1pt[pae_ptnum(l.kvpte64[i])].pg_pte =
					pae_mkpte(PG_RW | PG_V,
						pae_pfnum(privp->hp_privl1[i]));
				l.pdpte[i].pg_pte = pae_mkpte(PG_V, 
					pae_pfnum(privp->hp_privl1[i])); 
			}
		}
		TLBSflushtlb();
		ASSERT(hatp->hat_activecpucnt >= 0);
	}

        UNLOCK(&hatp->hat_resourcelock, savpl);
}
#endif PAE_MODE

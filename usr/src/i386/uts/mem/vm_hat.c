#ident	"@(#)kern-i386:mem/vm_hat.c	1.39.26.2"
#ident	"$Header$"

/*
 * The hat layer manages the address translation hardware as a cache
 * driven by calls from the higher levels in the VM system.  Nearly
 * all the details of how the hardware is managed should not be visible
 * above this layer except for miscellaneous machine specific functions
 * (e.g. mapin/mapout) that work in conjunction with this code.	 Other
 * than a small number of machine specific places, the hat data
 * structures seen by the higher levels in the VM system are opaque
 * and are only operated on by the hat routines.  Each address space
 * contains a struct hat and each page structure contains an opaque pointer
 * which is used by the hat code to hold a list of active translations to
 * that page.
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
#include <fs/fs_hier.h>
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
#include <proc/cg.h>
#include <svc/systm.h>
#include <mem/kmem.h>
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

/* Temporary, for debugging only */
#undef STATIC
#define STATIC

#define HAT_PRIVL132_LIST(hatp) ((hatp)->hat_privl1_list)
#define HAT_PRIVL1_LIST HAT_PRIVL132_LIST

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

extern struct seg *segkmap;	/* kernel generic mapping segment */

hat_t *kas_hatp = &kas.a_hat;

void hat_free_modstats(struct as *as);

/*
 * This lkinfo structure is used for all hat_resourcelock instances.
 */
STATIC LKINFO_DECL(hatresource_lkinfo, "hat resource lock of an as", 0);


/*
 ************************************************************************
 * hat resource allocation and free pool management code:
 *
 * This section of code contains all of the routines for managing
 * PT and MC allocation and free pools.
 *
 * The code is designed to work with a daemon to help with pool management.
 * Resources are always freed into the corresponding pool.  The daemon
 * trims the pool if it grows too big.
 * Allocation always tries the pool first.  If that fails, and flags permit,
 * an attempt is made to create a new instance of the desired resource.
 * If that fails (resource exhaustion via kpg_alloc (for pages) or
 * kmem_zalloc (for companion structures)), and the flag allows sleeping,
 * a reserve is placed and the code sleeps, waiting for the daemon to
 * replenish the desired pool and wake the waiters.
 * The daemon is expected to run about once a second.
 * Pool maintenance is a non-sleeping chore.
 * Waiting for a second or so is OK here since resource exhaustion will
 * be dominated by page pool exhaustion.  That, in turn, causes swapping
 * which creates worse delays while "solving" the problem.
 ************************************************************************
 */

/*
 * The following could be tunables.
 */
long hat_minptfree = 12;
long hat_maxptfree = 36;
long hat_minpgfree = 12;
long hat_maxpgfree = 36;

lock_t hat_ptpoollock;
lock_t hat_pgpoollock;
STATIC LKINFO_DECL(hat_ptfree_lkinfo, "mem: hat_ptpoollock: hat pt free list lock", 0);
STATIC LKINFO_DECL(hat_pgfree_lkinfo, "mem: vm_mcpoollock: hat pg free list lock",0);

extern pl_t ptpool_pl;
pl_t pgpool_pl;
STATIC sv_t hat_ptwait;
STATIC sv_t hat_pgwait;

STATIC hatpt_t *hatpt_freelist;
STATIC long nptfree;
#ifdef DEBUG
STATIC long nptneeded;		/* unwoken up reservers */
#endif
STATIC long nptreserved;	/* untaken reserved page tables */

STATIC long npgfree;
STATIC page_t *pgfreelist;	/* list of pages free pages used for */
				/* dedicated L1 */
STATIC long npgreserved;        /* untaken reserved chunks */
#ifdef DEBUG
STATIC long npgneeded;		/* unwoken up reservers */
#endif

extern hat_stats_t *hat_findstats(hat_t *, pte_t *);

#ifndef UNIPROC
xshield_t *xshield;
#endif

#ifdef DEBUG
#define	CHECK_PGFREELIST() { \
	page_t *tpp; \
	int tpgfree; \
		\
	tpp = pgfreelist; \
	tpgfree = npgfree; \
	do { \
		ASSERT(tpp != NULL); \
		tpgfree--; \
	} while ((tpp = tpp->p_next) != NULL); \
	ASSERT(tpp == (page_t *)NULL && tpgfree == 0); \
}
#else
#define	CHECK_PGFREELIST()
#endif /* DEBUG */

/*
 * STATIC hatpt_t *
 * allocptfrompool(void)
 *	Attempts to allocate a page table from the pool.
 *	If the pool is empty, it returns NULL.
 *
 * Calling/Exit State:
 *	No pool locks are held by caller.
 *	Pt pool lock used to access pool, but is dropped before return.
 */
STATIC hatpt_t *
allocptfrompool(void)
{
	hatpt_t *ptap;

	PTPOOL_LOCK();
	ASSERT(nptfree >= 0);
	ASSERT(nptreserved >= 0);
	if (nptfree <= nptreserved) {
		PTPOOL_UNLOCK();
		return((hatpt_t *)NULL);
	}
	ASSERT(hatpt_freelist != (hatpt_t *)NULL);
	ASSERT(nptfree > 0);
	ptap = hatpt_freelist;
	hatpt_freelist = ptap->hatpt_forw;
	nptfree--;
	ASSERT(nptfree >= 0);
	PTPOOL_UNLOCK();

	HOP_CHECKPTINIT((void *)ptap->hatpt_pdtep, ptap->hatpt_ptpp);
	return(ptap);
}

/*
 * page_t *
 * allocpg(void)
 *	This routine allocates a physical page.
 *
 * Calling/Exit State:
 *	- caller prepares to block if !NOSLEEP 
 *	- no HAT translations are set 
 *	- npages is always equal to 1 
 *	- kernel virtual heap space is allocated by caller 
 */
page_t *
allocpg(void)
{
	page_t *pp;
	uint_t flag = P_NODMA|NOSLEEP;
#ifdef NO_RDMA
	const mresvtyp_t mtype = M_KERNEL_ALLOC;
#else /* !NO_RDMA */
	extern mresvtyp_t kpg_mtypes[];
	mresvtyp_t mtype;

	mtype = kpg_mtypes[(flag & P_DMA)];
#endif /* NO_RDMA */

	if (flag & NOSLEEP) {
		if (!mem_resv(1, mtype))
			return((void *)NULL);
	} else {
		mem_resv_wait(1, mtype, B_FALSE);
	}

	/*
	 * Now actually allocate the physical pages we need
	 * using page_get.
	 */
	pp = page_get(ptob(1), flag, mycg);
	if (pp == NULL) {
		ASSERT(flag & NOSLEEP);
		mem_unresv(1, mtype);
		return((void *)NULL);
	}
	ASSERT(pp != NULL);
	return (pp);
}

/*
 * page_t *
 * allocnewpg(void)
 *	Allocate a new page and put it on the free list.
 *
 * Calling/Exit State:
 *	None.
 */
page_t *
allocnewpg(void)
{
	page_t *pp;

trytoalloc:
	pp = allocpg();
	if (pp == (page_t *)NULL)
		return (page_t *)NULL;
	
	/*
	 * Allocate temporary mapping to zero the page table page.
	 * Note that the hatpt_ptva is initialized by the caller.
	 */
	pagezero(pp, 0, PAGESIZE);
	HATPT_AEC(pp) = 0;
	HATPT_LOCKS(pp) = 0;
	HATPT_PTAP(pp) = (hatpt_t *)NULL;
	ASSERT(pp != NULL);

	if (npgreserved <= npgfree) {
		return pp;
	} else {
		PGPOOL_LOCK();
		ASSERT(npgreserved >= 0);
		ASSERT(npgfree >= 0);
		ASSERT(npgneeded >= 0);
                ASSERT(npgfree + npgneeded == npgreserved);
		pp->p_next = pgfreelist;
		pgfreelist = pp;
		if (npgfree++ < npgreserved) {
			ASSERT(npgneeded-- != 0);
			ASSERT(npgfree + npgneeded == npgreserved);
			SV_SIGNAL(&hat_pgwait, 0);
		}
		PGPOOL_UNLOCK();
		goto trytoalloc;
        }
	/* NOTREACHED */
}

/*
 * void
 * freepg(page_t *pp)
 *
 * Calling/Exit State:
 *	None.
 */
void
freepg(page_t *pp)
{
#ifdef NO_RDMA
        const mresvtyp_t mtype = M_KERNEL_ALLOC;
#else
        static mresvtyp_t mtype_page[] = {
                        M_KERNEL_ALLOC,         /* STD_PAGE */
                        M_DMA,                  /* DMA_PAGE */
                        M_KERNEL_ALLOC          /* PAD_PAGE */
        };
        mresvtyp_t mtype;
#endif

#ifndef NO_RDMA
	mtype = mtype_page[pp->p_type];
#endif /* NO_RDMA */

	ASSERT(pp != (page_t *)NULL);
	page_unlock(pp);

        mem_unresv(1, mtype);
}

/*
 * void
 * hat_pgfree(page_t *pp)
 *      Need to adjust to the beginning of the page free list.
 *      The chunk should be zeroed (running zeroing is better for
 *      sparse use) already.
 *
 * Calling/Exit State:
 *      No pool locks are held by the caller, the mc pool lock is used.
 *      Only free to the pool (let the daemon worry about trimming the pool)
 *      in order to minimize lock constraints on the caller.
 */
void
hat_pgfree(page_t *pp)
{
	HOP_CHECKPTINIT((void *)0, pp);
	PGPOOL_LOCK();
	ASSERT(pgfreelist != pp);
	pp->p_next = pgfreelist;
	pgfreelist = pp;
	ASSERT(npgfree >= 0);
	ASSERT(npgneeded >= 0);
	ASSERT(npgreserved >= 0);
	ASSERT(npgfree + npgneeded >= npgreserved);
        if (npgfree++ < npgreserved) {
                ASSERT(npgneeded-- != 0);
                ASSERT(npgfree + npgneeded == npgreserved);
                SV_SIGNAL(&hat_pgwait, 0);
        }
	PGPOOL_UNLOCK();
}

/*
 * STATIC page_t *
 * allocpgfrompool(void)
 *      Attempts to allocate a mapping chunk from the pool.
 *      If the pool is empty, it returns NULL.
 *
 * Calling/Exit State:
 *      No pool locks are held by caller.
 *      mc lock used to access pool, but is dropped before return.
 */
STATIC page_t *
allocpgfrompool(void)
{
	page_t *pp = (page_t *)NULL;

	PGPOOL_LOCK();
	ASSERT(npgfree >= 0);
	ASSERT(npgneeded >= 0);
	ASSERT(npgreserved >= 0);
	ASSERT(npgfree + npgneeded >= npgreserved);
	if (npgfree <= npgreserved) {
		ASSERT(npgfree + npgneeded == npgreserved);
		PGPOOL_UNLOCK();
		return((page_t *)NULL);
	}
	CHECK_PGFREELIST();
	pp = pgfreelist;
	pgfreelist = pp->p_next;
	npgfree--;
	ASSERT(npgfree >= 0);
        PGPOOL_UNLOCK();
	HOP_CHECKPTINIT(0, pp);
        return(pp);
}

/*
 * page_t *
 * waitforpg(void)
 *	Waits for a pg to become available (either freed or allocated by
 *	the pool refresh daemon or by another allocating lwp) and then 
 *	allocates it.
 *
 * Calling/Exit State:
 *      This code sleeps, so the caller can hold no spin locks.
 *
 */
STATIC page_t *
waitforpg(void)
{
        page_t *pp;

        PGPOOL_LOCK();
        ASSERT(npgfree >= 0);
        ASSERT(npgneeded >= 0);
        ASSERT(npgreserved >= 0);
        ASSERT(npgfree + npgneeded >= npgreserved);
        if (npgfree <= npgreserved) {
                npgreserved++;
                ASSERT(npgfree + ++npgneeded == npgreserved);
                SV_WAIT(&hat_pgwait, PRIMEM, &hat_pgpoollock);
                PGPOOL_LOCK();
                ASSERT(npgfree > 0);
                npgreserved--;
                ASSERT(npgreserved >= 0);
        }
	CHECK_PGFREELIST();
        ASSERT(npgfree > 0);
	pp = pgfreelist;
	pgfreelist = pp->p_next;
	npgfree--;
        ASSERT(npgfree >= 0);
        PGPOOL_UNLOCK();
	HOP_CHECKPTINIT(0, pp);

        return(pp);
}

/*
 * page_t *
 * hat_pgalloc(uint_t flag)
 *      Allocate a mapping chunk, insuring that it is zeroed.
 *      There is an internal mc free pool. Access to it is controlled
 *      by the hat_mcpoollock spin lock.
 *
 * Calling/Exit State:
 *      The calling state depends on the flag passed in.
 *      The flag can have the following values:
 *      HAT_POOLONLY    -> use local free mc pool only.
 *              Any collection of locks (except TLBS) can be held.
 *              The free pool locks are after VM_PAGELOCK to allow this.
 *              Pool locks are only ever held by the low level pool code.
 *      HAT_NOSLEEP     -> return immediately if no memory
 *              Try the local mc free pool first. if that is exhausted
 *              allocate a new mc page via kpg_alloc and zero it,
 *              but pass them NOSLEEP in the appropriate form.
 *              The pool lock is dropped while calling outside of hat.
 *              No locks are held by the calling hat code. The outside
 *              code that calls into the hat may only hold locks earlier
 *              in the hierarchy than the earliest of VPGLOCK and
 *              locks used in seg_kmem via kpg_alloc.
 *      HAT_REFRESH     -> do a HAT_NOSLEEP-style allocation and if that
 *              is successful, refresh the pools if they need it.
 *              This is used by the bulk loaders (hat_map and hat_dup)
 *              to get maximum mileage out of having to drop all of
 *              the locks.
 *      HAT_CANWAIT     -> wait if no memory currently available
 *              Do a NOSLEEP allocation. If that fails, bump a reserve
 *              count and sleep in the mcalloc code. The pool daemon
 *              runs about every second and tries to refresh the pools.
 *              To the extent that it succeeds, it wakes up waiters.
 *              This is flag is used only in hat_pteload, which does
 *              only a single PTE.
 *
 */
page_t *
hat_pgalloc(uint_t flag)
{
        page_t *pp;

        pp = allocpgfrompool();
        if (pp) {
                if (flag == HAT_REFRESH)
                        hat_refreshpools();
                return(pp);
        }
        if (flag == HAT_POOLONLY)
                return((page_t *) NULL);
        pp = allocnewpg();
        if (pp) {
                if (flag == HAT_REFRESH)
                        hat_refreshpools();
                return(pp);
        }
        if (flag & (HAT_NOSLEEP|HAT_REFRESH)) {
                ASSERT(flag == HAT_NOSLEEP || flag == HAT_REFRESH);
                return((page_t *) NULL);
        }
        ASSERT(flag == HAT_CANWAIT);
        pp = waitforpg();
        ASSERT(pp != (page_t *)NULL);
        return(pp);
}

/*
 * STATIC hatpt_t *
 * allocnewpt(void)
 *	Allocates a pt using kmem_zalloc and kpg_alloc.
 *	Field that are fixed at allocation time are initialized
 *	in the hatpt_t structure.
 *	It never sleeps.
 *
 * Calling/Exit State:
 *	The caller should have no spin locks that interfere
 *	(hierarchy-wise) with kmem_zalloc and kpg_alloc.
 *	The lock for the pt pool is used when bumping the count.
 */
STATIC hatpt_t *
allocnewpt(void)
{
	hatpt_t *ptap;
	page_t *ptpp;

	/*
	 * Right now MMU_PAGESIZE == PAGESIZE.
	 * If that changes, much hat code must change to loop
	 * over multiple physical pages per logical page.
	 */
#ifdef DEBUG
	/*LINTED*/
	ASSERT(MMU_PAGESIZE == PAGESIZE);
#endif
trytoalloc:
	ptap = (hatpt_t *)kmem_zalloc(sizeof (hatpt_t), KM_NOSLEEP);
	if (ptap == (hatpt_t *)NULL)
		return((hatpt_t *)NULL);
	ptap->hatpt_ptpp = (page_t *) allocpg();
	if (ptap->hatpt_ptpp == (page_t *)NULL) {
		kmem_free(ptap, sizeof (hatpt_t));
		return((hatpt_t *)NULL);
	}

	/*
	 * Allocate temporary mapping to zero the page table page.
	 * Note that the hatpt_ptva is initialized by the caller.
	 */
	ptpp = ptap->hatpt_ptpp;
	ASSERT(ptpp != NULL);
	pagezero(ptpp, 0, PAGESIZE);
	HATPT_PTAP(ptpp) = ptap;
	HATPT_AEC(ptpp) = 0;
	HATPT_LOCKS(ptpp) = 0;
#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		ptap->hatpt_pde64.pg_pte = 
			pae_mkpte(PTE_RW|PG_V, page_pptonum(ptpp));
	} else
#endif /* PAE_MODE */
	{
		ptap->hatpt_pde.pg_pte = 
			(uint_t)mkpte(PTE_RW|PG_V, page_pptonum(ptpp));
	}
	/* stale check o.k. here w/o holding the ptpool lock */
	if (nptreserved <= nptfree)
		return (ptap);
	else {
		PTPOOL_LOCK();
		ASSERT(nptreserved >= 0);
		ASSERT(nptfree >= 0);
		ASSERT(nptneeded >= 0);
		ASSERT(nptfree + nptneeded >= nptreserved);
		ptap->hatpt_forw = hatpt_freelist;
		hatpt_freelist = ptap;

		if (nptfree++ < nptreserved) {
			ASSERT(nptneeded-- != 0);
			ASSERT(nptfree + nptneeded == nptreserved);
			SV_SIGNAL(&hat_ptwait, 0);
	  	}

		PTPOOL_UNLOCK();
		goto trytoalloc;
	}
	/* NOTREACHED */
}

/*
 * STATIC hatpt_t *
 * waitforpt(void)
 *	Waits for a pt to become available (either freed or allocated by
 *	the pool refresh daemon or other allocating lwps) and then allocates it.
 *
 * Calling/Exit State:
 *	This code sleeps, so the caller can hold no spin locks.
 */
STATIC hatpt_t *
waitforpt(void)
{
	hatpt_t *ptap;

	PTPOOL_LOCK();
	ASSERT(nptfree >= 0);
	ASSERT(nptneeded >= 0);
	ASSERT(nptreserved >= 0);
	ASSERT(nptfree + nptneeded >= nptreserved);
	if (nptfree <= nptreserved) {
		nptreserved++;
		ASSERT(nptfree + ++nptneeded == nptreserved);
		SV_WAIT(&hat_ptwait, PRIMEM, &hat_ptpoollock);
		PTPOOL_LOCK();
		ASSERT(nptfree > 0);
		nptreserved--;
		ASSERT(nptreserved >= 0);
	}
	ASSERT(nptfree > 0);
	ASSERT(hatpt_freelist != (hatpt_t *)NULL);
	ptap = hatpt_freelist;
	hatpt_freelist = ptap->hatpt_forw;
	nptfree--;
	ASSERT(nptfree >= 0);
	PTPOOL_UNLOCK();
	HOP_CHECKPTINIT(ptap->hatpt_pdtep, ptap->hatpt_ptpp);
	return(ptap);
}

/*
 * hatpt_t *
 * hat_ptalloc(uint_t flag)
 *	Allocate a page table with its hatpt_t structure,
 *	set up pte for page table in the hatpt_t, zero the page table,
 *	and return a pointer to its hatpt_t structure.
 *	There is an internal pt free pool. Access to it is controlled
 *	by the hat_ptpoollock spin lock.
 *
 * Calling/Exit State:
 *	The calling state depends on the flag passed in.
 *	The flag can have the following values:
 *	HAT_POOLONLY	-> use local free pt pool only.
 *		Any collection of locks (except TLBS) can be held.
 *		The free pool locks are after the global page layer locks
 *		(pgidlk and pgfreelk) to allow this. Pool locks are only
 *		ever held by the low level pool code.
 *	HAT_NOSLEEP	-> return immediately if no memory
 *		Try the local pt free pool first. if that is exhausted
 *		allocate a new pt via kpg_alloc and kmem_zalloc,
 *		but pass them NOSLEEP in the appropriate form.
 *		The pool lock is dropped while calling outside of hat.
 *		No locks are held by the calling hat code. The outside
 *		code that calls into the hat may only hold locks earlier
 *		in the hierarchy than the earliest of VPGLOCK and
 *		locks used in seg_kmem via kpg_alloc and locks in KMA.
 *	HAT_REFRESH	-> do a HAT_NOSLEEP-style allocation and if that
 *		is successful, refresh the pools if they need it.
 *		This is used by the bulk loaders (hat_map and hat_dup)
 *		to get maximum mileage out of having to drop all of
 *		the locks.
 *	HAT_CANWAIT	-> wait if no memory currently available
 *		Do a NOSLEEP allocation. If that fails, bump a reserve
 *		count and sleep in the ptalloc code. The pool daemon
 *		runs about every second and tries to refresh the pools.
 *		To the extent that it succeeds, it wakes up waiters.
 *		This is flag is used only in hat_pteload, which does
 *		only a single PTE.
 *
 */
hatpt_t *
hat_ptalloc(uint_t flag)
{
	hatpt_t *ptap;

	ptap = allocptfrompool();
	if (ptap) {
		if (flag == HAT_REFRESH)
			hat_refreshpools();
		return(ptap);
	}
	if (flag == HAT_POOLONLY)
		return((hatpt_t *) NULL);
	ptap = allocnewpt();
	if (ptap) {
		if (flag == HAT_REFRESH)
			hat_refreshpools();
		return(ptap);
	}
	if (flag & (HAT_NOSLEEP|HAT_REFRESH)) {
		ASSERT(flag == HAT_NOSLEEP || flag == HAT_REFRESH);
		return((hatpt_t *) NULL);
	}
	ASSERT(flag == HAT_CANWAIT);
	ptap = waitforpt();
	ASSERT(ptap != (hatpt_t *)NULL);
	return(ptap);
}

/*
 * void
 * hat_ptfree(hatpt_t *ptap)
 *	Return page table to the free pt pool.
 *
 * Calling/Exit State:
 *	Any combination of locks through MV_PAGELOCK in the hierarchy
 *	can be held by the caller. This code uses only the spin lock
 *	for the free pt pool. It lets the daemon return excess pages
 *	from this pool back to kmem_free and kpg_free.
 *
 * Remarks:
 *	If called to free a PSE ptap, then it calls pse_hat_ptfree.
 */
void
hat_ptfree(hatpt_t *ptap)
{
#ifdef DEBUG
	if (PAE_ENABLED()) {
		ASSERT(!PG_ISPSE(&ptap->hatpt_pde64));
	} else
	{
		ASSERT(!PG_ISPSE(&ptap->hatpt_pde));
	}
#endif /* DEBUG */
	HOP_CHECKPTINIT(ptap->hatpt_pdtep, ptap->hatpt_ptpp);
	PTPOOL_LOCK();
	ptap->hatpt_forw = hatpt_freelist;
	hatpt_freelist = ptap;
	ptap->hatpt_as = (struct as *)NULL;
	ASSERT(nptfree >= 0);
	ASSERT(nptneeded >= 0);
	ASSERT(nptreserved >= 0);
	ASSERT(nptfree + nptneeded >= nptreserved);
	if (nptfree++ < nptreserved) {
		ASSERT(nptneeded-- != 0);
		ASSERT(nptfree + nptneeded == nptreserved);
		SV_SIGNAL(&hat_ptwait, 0);
	}
	PTPOOL_UNLOCK();
}

/*
 * void
 * hat_refreshpools(void)
 *	If pools are too small, try to grow them (NOSLEEP, of course)
 *	and wake up waiters if successful.
 *	If pools are too large, free resources to trim them if possible.
 *	It is is always possible for the pt pool, but mc page fragmentation
 *	limits action for the mc pool.
 *	This is called from the pool daemon and internally.
 *
 * Calling/Exit State:
 *	The caller can hold no spin locks that interfere with kmem_zalloc
 *	or kpg_alloc calls.
 *	The caller cannot hold any of the pool locks.
 */
void
hat_refreshpools(void)
{
	hatpt_t *ptap, *nptap;
	int i, tofree;
	page_t *pp, *npp;

	/*
	 * The allocation must happen without the locks held
	 * because of lock hierarchy.
	 * So there is no reason to grab the lock to check,
	 * since the operation is inherently (but benignly) stale.
	 */
	if (nptfree - nptreserved < hat_minptfree) {
		do {
			ptap = allocnewpt();
			if (ptap == (hatpt_t *)NULL)
				return;
			hat_ptfree(ptap);
		} while (nptfree - nptreserved < hat_minptfree);
	} else if (nptfree - nptreserved > hat_maxptfree) {
		PTPOOL_LOCK();
		tofree = nptfree - nptreserved - hat_maxptfree;
		if (tofree <= 0) {
			PTPOOL_UNLOCK();
			goto checkpgpool;
		}
		/*
		 * Take all off now since lock must be dropped
		 * to give them back.
		 */
		ptap = nptap = hatpt_freelist;
		for (i = tofree; i--; nptap = nptap->hatpt_forw) {
			ASSERT(nptap);
		}
		ASSERT(nptap);
		hatpt_freelist = nptap;
		nptfree -= tofree;
		PTPOOL_UNLOCK();
		while (tofree--) {
			nptap = ptap->hatpt_forw;
			/*
			 * Note: We do not need to release the virtual space 
			 * for the page table page, since we never allocate
			 * it because we have per-address-space page table
			 * kernel virtual.
			 */
			freepg(ptap->hatpt_ptpp);
			kmem_free(ptap, sizeof (hatpt_t));
			ptap = nptap;
		}
	}

checkpgpool:
        if (npgfree - npgreserved < hat_minpgfree) {
                do {
                        pp = allocnewpg();
                        if (pp == (page_t *)NULL)
                                return;
			hat_pgfree(pp);
                } while (npgfree - npgreserved < hat_minpgfree);
        } else if (npgfree - npgreserved > hat_maxpgfree && npgfree) {
                PGPOOL_LOCK();
                ASSERT(npgfree >= 0);
                ASSERT(npgreserved >= 0);
                tofree = npgfree - npgreserved - hat_maxpgfree;
                if (tofree <= 0 || npgfree == 0) {
                        PGPOOL_UNLOCK();
                        return;
                }
                if (tofree > npgfree)
                        tofree = npgfree;
                ASSERT(tofree > 0);
                pp = npp = pgfreelist;
                for (i = tofree; i--; npp = npp->p_next) {
                        ASSERT(npp);
                }
		ASSERT(npp);
                pgfreelist = npp;
                npgfree -= tofree;
                PGPOOL_UNLOCK();
                while (tofree--) {
                        npp = pp->p_next;
			freepg(pp);
                        pp = npp;
                }
		CHECK_PGFREELIST();
        }
}

/*
 * Declarations for TLB shootdown.
 */

STATIC LKINFO_DECL(TLBSlock_lkinfo, "TLBS lock", 0);
lock_t TLBSlock;		/* serialize all tlb shootdowns */
extern pl_t TLBSoldpl;

volatile TLBScookie_t *TLBcpucookies;	/* used by resume and use_private */

/* the following variables are maintained by hat_online/hat_offline. */
int maxonlinecpu;		/* to limit searches in TLB code */
int minonlinecpu;	/* to limit searches in TLB code */

STATIC volatile TLBScookie_t TLBSgoodguess;	/* guess of timestamp last TLBS occured */
STATIC emask_t TLBShootdownbits;	/* for lazy shootdown */


/*
 * void
 * hat_online(void)
 *	Initialize plocal, kas, and TLBS as needed to put this
 *	engine online.
 *
 * Calling/Exit State:
 *	Called late in bringing an engine online.
 *	It is called by that engine.
 *	Calling hat_online() signifies that the engine is ready
 *	to have itself counted in TLB/hat activity.
 *	CR3 must have been loaded.
 *	For the first one, hat_lockinit must have been called already.
 *	It uses both the kas hat resource lock and the TLBSlock to
 *	Protect the fields it has to change.  This lets other code
 *	skip the kas hat resource lock (might even be able to skip
 *	it here) and still get accurate online bit maps.
 */
void
hat_online(void)
{
	TLBSoldpl = TLBS_LOCK();
	if (maxonlinecpu < l.eng_num)
		maxonlinecpu = l.eng_num;
	if (minonlinecpu > l.eng_num)
		minonlinecpu = l.eng_num;
	kas_hatp->hat_activecpucnt++;
	EMASK_SETS(&kas_hatp->hat_activecpubits, &l.eng_mask);
	TLBSflushtlb();
	TLBS_UNLOCK(TLBSoldpl);
}

/*
 * void
 * hat_offline(void)
 *	Called by an engine when it is going offline.
 *
 * Calling/Exit State:
 *	The engine will do no more TLB/hat-related operations.
 */
void
hat_offline(void)
{
	int i;

	TLBSoldpl = TLBS_LOCK();
	kas_hatp->hat_activecpucnt--;
	EMASK_CLRS(&kas_hatp->hat_activecpubits, &l.eng_mask);
	if (minonlinecpu == l.eng_num) {
		i = l.eng_num + 1;
		while (!EMASK_TEST1(&kas_hatp->hat_activecpubits, i)) {
			i++;
		}
		minonlinecpu = i;
	}
	if (maxonlinecpu == l.eng_num) {
		i = l.eng_num - 1;
		while (!EMASK_TEST1(&kas_hatp->hat_activecpubits, i)) {
			i--;
		}
		maxonlinecpu = i;
	}
	TLBS_UNLOCK(TLBSoldpl);
}

/*
 * void TLBSasync(ulong_t dummy)
 *	Stub with argument to call TLBSflushtlb (which has none).
 *	Used as the action subroutine for the engine mechanism.
 *
 * Calling/Exit State:
 *	Called via the engine signal mechanism during lazy shootdown.
 */
/* ARGSUSED */
STATIC void
TLBSasync(ulong_t dummy)
{
	TLBSflushtlb();
}

/*
 * int
 * hat_uas_shootdown_l(hat_t *hatp)
 *	Perform a batched shootdown for a given user address space.
 *
 * Calling/Exit State:
 *	The HAT lock is held on entry and remains held on exit.
 *	Returns to the caller an indication if the local tlb flush
 *	is required.
 *
 * Remarks:
 *	Global so that it can be called from pse_hat.c
 */
int
hat_uas_shootdown_l(hat_t *hatp)
{
	int flush;
 
	ASSERT(HATRL_OWNED(hatp));
	ASSERT(hatp != kas_hatp);

	if (hatp->hat_activecpucnt == 0)
		return(0);

	flush = EMASK_TESTS(&hatp->hat_activecpubits, &l.eng_mask);

	if (flush != 0 && hatp->hat_activecpucnt == 1) 
		return flush;

	(void)TLBS_LOCK();
	TLBS_CPU_SIGNAL(&hatp->hat_activecpubits, TLBSasync, 0);	
	TLBS_UNLOCK(VM_HAT_RESOURCE_IPL);

	return flush;
}

/*
 * void
 * hat_uas_shootdown(struct as *as)
 *	Perform a batched shootdown for a given user address space.
 *
 * Calling/Exit State:
 *	The HAT lock is acquired and released by this function. The hat lock
 *	is acquired to stabilize the activecpu bit vector. This function
 *	is a wrapper around hat_uas_shootdown_l().
 */
void
hat_uas_shootdown(struct as *as)
{
	hat_t *hatp = &as->a_hat;

	HATRL_LOCK_SVPL(hatp);

	if (hat_uas_shootdown_l(hatp))
		TLBSflushtlb();

	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * void
 * hat_shootdown(TLBScookie_t cookie, uint_t flag)
 *	For the lazy TLB shootdown aficionados (kernel segment drivers),
 *	if flag is HAT_NOCOOKIE, assume all engines need a flush.
 *	Otherwise, flush all engines that have not been flushed prior
 *	to the supplied cookie value.
 *
 * Calling/Exit State:
 *	No spinlocks that preclude grabbing the TLBSlock can be held
 *	by the caller.
 *	Avoid using the kas hat resource lock to minimize constraints
 *	on the callers and for performance.
 *	The races for info protected by the kas lock are all benign
 *	(trust me).
 */
void
hat_shootdown(TLBScookie_t cookie, uint_t flag)
{
	int i;
	int cnt;
	TLBScookie_t newguess, curcookie;

	if (flag == HAT_NOCOOKIE) {
		TLBSoldpl = TLBS_LOCK();
		newguess = hat_getshootcookie();
		TLBS_CPU_SIGNAL(&kas_hatp->hat_activecpubits, TLBSasync, 0);
		TLBSflushtlb();
		TLBSgoodguess = newguess;
		TLBS_UNLOCK(TLBSoldpl);
		return;
	}

	if (TICKS_LATER(TLBSgoodguess, cookie))
                return;

	cnt = 0;
	newguess = hat_getshootcookie();

	/*
	 * While we search, prepare for the worst, so we
	 * need exclusive use of the TLBShootdownbits.
	 */
	TLBSoldpl = TLBS_LOCK();
	EMASK_CLRALL(&TLBShootdownbits);
	for (i = minonlinecpu; i <= maxonlinecpu; i++) {
		if (!EMASK_TEST1(&kas_hatp->hat_activecpubits, i))
			continue;
		curcookie = TLBcpucookies[i];
		if (TICKS_LATER(curcookie, cookie)) {
			if (TICKS_LATER(newguess, curcookie))
				newguess = curcookie;
			continue;
		}
		if (i == l.eng_num) 
			TLBSflushtlb();
		else {
			cnt++;
			EMASK_SET1(&TLBShootdownbits, i);
		}
	}
	if (cnt)
		TLBS_CPU_SIGNAL(&TLBShootdownbits, TLBSasync, 0);
	TLBSgoodguess = newguess;
	TLBS_UNLOCK(TLBSoldpl);
}

/*
 ************************************************************************
 * hat initialization (called once at startup)
 ************************************************************************
 */

/*
 * void
 * hat_lockinit(void)
 *	Initialize all hat lock-related structures.
 *
 * Calling/Exit State:
 *	This is called once during system initialization from hat_init().
 *	There is only one engine online.
 *	The page pool and KMA are available.
 */
STATIC void
hat_lockinit(void)
{
	if (mycg == BOOTCG) {
	    LOCK_INIT(&kas.a_hat.hat_resourcelock, VM_HAT_RESOURCE_HIER,
			VM_HAT_RESOURCE_IPL, &hatresource_lkinfo, 0);
	    LOCK_INIT(&hat_ptpoollock, VM_HAT_PTPOOL_HIER, VM_HAT_PTPOOL_IPL,
		&hat_ptfree_lkinfo, 0);
	    LOCK_INIT(&hat_pgpoollock, VM_HAT_PGPOOL_HIER, VM_HAT_PGPOOL_IPL,
		&hat_pgfree_lkinfo, 0);
	    LOCK_INIT(&TLBSlock, VM_HAT_TLBS_HIER, VM_HAT_TLBS_IPL,
		&TLBSlock_lkinfo, 0);

	    SV_INIT(&hat_ptwait);
	    SV_INIT(&hat_pgwait);
	}
}

/*
 * void
 * hat_init(void)
 *	This is called once during startup.
 *	Initializes the dynamic hat information.
 *	The page pool is available.
 *
 * Calling/Exit State:
 *	Single CPU environment.
 *	No dynamic hat calls prior to this point.
 *
 * Description:
 *	Initialize hatpt and mapping chunk page freelists.
 *	Initialize hat resource locks.
 */
void
hat_init(void)
{
#ifdef DEBUG
	/*LINTED*/
	ASSERT(PAGESIZE == MMU_PAGESIZE);
#endif
	if ((TLBcpucookies = kmem_zalloc(sizeof(TLBcpucookies[0]) * Nengine,
			KM_NOSLEEP)) == NULL) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not allocate space for TLBcpucookies\n");
	}
#ifndef UNIPROC
	if ((xshield = kmem_zalloc(sizeof(xshield_t) * Nengine,
				    KM_NOSLEEP)) == NULL) {
		/*
		 *+ Kernel failed to allocate memory at boot time.  System
		 *+ is probably underconfigured.
		 */
		cmn_err(CE_PANIC, "Could not allocate space for	xshield[]\n");
	}
#endif	
	hatpt_freelist = (hatpt_t *)NULL;
	pgfreelist = (page_t *)NULL;
	hat_lockinit();
	hat_refreshpools();
}

/*
 * void
 * hat_kmadv(void)
 *	Call kmem_advise() for HAT data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
hat_kmadv(void)
{
	kmem_advise(sizeof(hatpt_t));
}

/*
 ************************************************************************
 * Static hat utility functions
 ************************************************************************
 */

/*
 * void
 * hat_link_ptap(struct as *as, hatpt_t *prevptap, hatpt_t *ptap)
 *	Insert ptap into ordered chain of "as" after prevptap.
 *
 * Calling/Exit State:
 *	The hat resource spinlock is owned by the caller and is not dropped.
 *
 * Remarks:
 *	Global so that it can be called from pse_hat.c
 */
void
hat_link_ptap(struct as *as, hatpt_t *prevptap, hatpt_t *ptap)
{
	hat_t *hatp = &as->a_hat;

#ifdef DEBUG
	if (PAE_ENABLED())
		ASSERT(ptap->hatpt_pdtep64);
	else
		ASSERT(ptap->hatpt_pdtep);
#endif /* DEBUG */

	ASSERT(ptap != prevptap);

	ptap->hatpt_as = as;
	hatp->hat_ptlast = ptap;

	if (hatp->hat_pts == (hatpt_t *)NULL) {
		ASSERT(prevptap == (hatpt_t *)NULL);
		ptap->hatpt_forw = ptap->hatpt_back = ptap;
		hatp->hat_pts = ptap;
	} else {
		ASSERT(prevptap);

		/*
		 * Although prevptap is "before" ptap,
		 * the list is circular and prevptap may be the upper end.
		 */

		ptap->hatpt_back = prevptap;
		ptap->hatpt_forw = prevptap->hatpt_forw;
		prevptap->hatpt_forw->hatpt_back = ptap;
		prevptap->hatpt_forw = ptap;
#ifdef PAE_MODE
		if (PAE_ENABLED()) {
			if (prevptap->hatpt_pdtep64 > ptap->hatpt_pdtep64) {
				ASSERT(hatp->hat_pts->hatpt_pdtep64 > ptap->hatpt_pdtep64);
				hatp->hat_pts = ptap;
			}
		} else
#endif /* PAE_MODE */
		{
			if (prevptap->hatpt_pdtep > ptap->hatpt_pdtep) {
				ASSERT(hatp->hat_pts->hatpt_pdtep > ptap->hatpt_pdtep);
				hatp->hat_pts = ptap;
			}
		}
	}
}

/*
 * void
 * hat_unlink_ptap(struct as *as, hatpt_t *ptap)
 *	Unlink hatpt structure from the specified address space.
 *
 * Calling/Exit State:
 *	Caller owns the hat_resourcelock.
 *
 * Remarks:
 *	Global so that it can be called from pse_hat.c
 */
void
hat_unlink_ptap(struct as *as, hatpt_t *ptap)
{
	struct hat *hatp = &(as->a_hat);

	if (ptap->hatpt_forw == ptap) {
		hatp->hat_pts = hatp->hat_ptlast = (hatpt_t *)NULL;
	} else {
		ptap->hatpt_back->hatpt_forw = ptap->hatpt_forw;
		ptap->hatpt_forw->hatpt_back = ptap->hatpt_back;
		if (hatp->hat_pts == ptap) 
			hatp->hat_pts = ptap->hatpt_forw;
		if (hatp->hat_ptlast == ptap) 
			hatp->hat_ptlast = hatp->hat_pts;
	}
}

/*
 ************************************************************************
 * External dynamic pt hat interface routines
 ************************************************************************
 */

/*
 * void
 * hat_alloc(struct as *as)
 *	The general definition of hat_alloc is to allocate
 *	and initialize the basic hat structure(s) of an address space.
 *	Called from as_alloc() when address space is being set up.
 *	In this hat implementation, the hat structure is in the as
 *	structure and is assumed to be zeroed by as_alloc when
 *	as_alloc zeroes the as structure.
 *	So, only special MP initialization is needed.
 *
 * Calling/Exit State:
 *	There is only single engine access to the as at this point.
 *	On exit, the as is ready to use.
 */
void
hat_alloc(struct as *as)
{
	/*
	 * No page tables allocated as yet.
	 * as structure zeroed by as_alloc().
	 */
	struct hat *hatp = &as->a_hat;

	ASSERT(hatp->hat_pts == (hatpt_t *)NULL);
	ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);
	ASSERT(hatp->hat_privl1_list == (hatprivl1_t *)NULL);
	LOCK_INIT(&hatp->hat_resourcelock, VM_HAT_RESOURCE_HIER,
			VM_HAT_RESOURCE_IPL, &hatresource_lkinfo, KM_SLEEP);
	SV_INIT(&hatp->hat_sv);
}

/*
 * void
 * hat_free(struct as *as)
 *	Free all of the translation resources for the specified address space.
 *	Then deinit the lock.
 *	Called from as_free when the address space is being destroyed.
 *
 * Calling/Exit State:
 *	If it is called in context, hat_asunload for this as should
 *	have been done before by the calling context. The context is single
 *	threaded at this point.
 *
 * Remarks:
 *	At this point in time, all hat resources would have been freed by
 *	the segment drivers via seg_unmap(). So we just assert that there
 *	are no page tables left in the address space.
 */
/* ARGSUSED */
void
hat_free(struct as *as)
{
	struct hat   *hatp;
	extern hatops_t pae_hatops;

	ASSERT(as != (struct as *)NULL);
	ASSERT(as != &kas);
	ASSERT(as->a_rss == 0);
	ASSERT(getpl() == PLBASE);

	hatp = &as->a_hat;

	ASSERT(hatp->hat_pts == (hatpt_t *)NULL);
	ASSERT(hatp->hat_ptlast == (hatpt_t *)NULL);

	/*
	 * At this point the freeing process is single threaded, and has
	 * blocked access to /proc. Thus, we have an effective write lock on
	 * the AS.
	 */
	(void) LOCK(&hatp->hat_resourcelock, PLHI);
	as->a_free = 1;
	if (hatp->hat_activecpucnt != 0) {
		/*
		 * Some engine still has the address space loaded.
		 * Delay the deallocation of the AS structure until
		 * this engine gives up the AS.
		 */
		UNLOCK(&hatp->hat_resourcelock, PLBASE);
		return;
	}
	UNLOCK(&hatp->hat_resourcelock, PLBASE);
#ifdef PAE_MODE
	if (hatp->hat_privl164_list && l.hatops == &pae_hatops) {
		HAT_AS_FREE64(as, hatp);
	} else
#endif /* PAE_MODE */
	{
		HAT_AS_FREE(as, hatp);
	}
}

/*
 * boolean_t
 * local_age_lock(proc_t *procp)
 *	Obtain the necessary serialization for in-context aging.
 *	For MMUs that can support transparent maintenance of modify
 *	bits [see broader description in vm_hat.h], multiple LWPs 
 *	within the same address space can be actively referencing 
 *	the address space even as one of them ages the address space. 
 *	In this case, it suffices for the aging LWP to hold the address
 *	space read-locked, since this will guarantee that the layout of
 *	the address space does not change during aging.
 *
 * Calling/Exit State:
 *	Called with process p_mutex held, which is dropped before return.
 * 	The function can block. If the current context loses the race to
 *	age the address space during the time that it blocks, then it
 *	will return B_FALSE to its caller. Otherwise, B_TRUE will be
 *	returned, and the necessary serialization for aging the address
 *	space would be achieved. 
 *	It is assumed that the caller will set the P_LOCAL_AGE flag, 
 *	under the same cover of p_mutex just before calling this function. 
 *	If B_TRUE is returned, the flag will remain set. 
 *
 * Remarks:
 *	The only serialization necessary is the obtainment of the address
 *	space read lock. First, an attempt is made to acquire the lock
 *	without blocking. If this fails, then the current context must
 *	block. During the time that it blocks, the LWP must declare the
 *	process available for aging/swapping by other contexts, in order
 *	to prevent resource deadlocks.
 *
 *	This is the only hat layer routine that contests the AS sleep lock.
 *	For this reason, it may be appropriate to locate it to some another 
 *	family specific file.
 */
boolean_t
local_age_lock(proc_t *procp)
{
	struct as *asp = procp->p_as;
	
	ASSERT(KS_HOLD1LOCK());
	ASSERT((procp->p_flag & P_LOCAL_AGE) != 0);

	if (as_tryrdlock(asp)) {
		UNLOCK(&procp->p_mutex, PLBASE);
		return(B_TRUE);
	}
	/*
	 * Must block for the AS read lock. Expose process to selection 
	 * by swapper in the interim.
	 */
	procp->p_flag &= ~P_LOCAL_AGE;
	UNLOCK(&procp->p_mutex, PLBASE);
	as_rdlock(asp);
	
	(void) LOCK(&procp->p_mutex, PLHI);
	/*
	 * Did someone beat us to the punch ?
 	 */
	if (CAN_SKIP_AGING(procp) || !AGING_EVENT_POSTED()) {
		UNLOCK(&procp->p_mutex, PLBASE);
		as_unlock(asp);
		return(B_FALSE);
	}

	procp->p_flag |= P_LOCAL_AGE;
	UNLOCK(&procp->p_mutex, PLBASE);
	return(B_TRUE);
}

/*
 * void
 * hat_wait_asunload(struct as *as)
 *	Wait till the seized LWPs are unloaded.
 *
 * Calling/Exit State:
 *	It is called from remove_proc after the process has seized.
 */
void
hat_wait_asunload(struct as *as)
{
	hat_t *hatp = &as->a_hat;
	pl_t opl;

	opl = LOCK(&hatp->hat_resourcelock, PLHI);
	while (hatp->hat_activecpucnt > 1) {
		SV_WAIT(&hatp->hat_sv, PRIMEM, &hatp->hat_resourcelock);
		(void) LOCK(&hatp->hat_resourcelock, PLHI);
	}
	UNLOCK(&hatp->hat_resourcelock, opl);
}

/*
 * void
 * hat_free_modstats(struct as *as)
 * 	Free the modstats structure.
 *
 * Calling/Exit State:
 *	The context is exiting and hence single threaded.
 *	The caller has already verified that this process has the
 *	modbit stat structure allocated.
 *
 * Remarks:
 *	Since we are single threaded at this point, we do not acquire the
 *	hat resource lock. 
 */
void
hat_free_modstats(struct as *as)
{
	hat_t *hatp = &as->a_hat;
	hat_stats_t *modstatsp, *nmodstatsp;

	ASSERT(hatp->hat_modstatsp != NULL);
	modstatsp = hatp->hat_modstatsp;
	while (modstatsp != NULL) {
		nmodstatsp = modstatsp->stats_next;
		ASSERT(modstatsp->stats_refcnt != 0);
		kmem_free(modstatsp->stats_modinfo, NPGPT);
		kmem_free(modstatsp, sizeof(hat_stats_t));
		modstatsp = nmodstatsp;
	}
}

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
#ifdef NOTYET
	hat_t *hatp = &as->a_hat;
	pte_t *vpdtep, *evpdtep;
	hat_stats_t *hat_modstatsp, *modstatsp = NULL;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((len & POFFMASK) == 0);
	ASSERT(u.u_procp->p_as == as);

	vpdtep = kpd0 + ptnum(addr);
	evpdtep = kpd0 + ptnum(addr + len - 1);

	HATRL_LOCK_SVPL(hatp);
	do {
tryagain:
		/* Returns the predecessor stats structure */
		hat_modstatsp = hat_findstats(hatp, vpdtep);
		if (hat_modstatsp != NULL && hat_modstatsp->stats_pdtep == vpdtep) {
			hat_modstatsp->stats_refcnt++;
		} else {
			if (modstatsp == NULL) {
				HATRL_UNLOCK_SVDPL(hatp);
				modstatsp = kmem_alloc(
						sizeof(hat_stats_t), KM_SLEEP);
				modstatsp->stats_modinfo = kmem_zalloc(
						NPGPT, KM_SLEEP);
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
		kmem_free(modstatsp->stats_modinfo, NPGPT);
		kmem_free(modstatsp, sizeof(hat_stats_t));
	}
#endif /* NOTYET */
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
#ifdef NOTYET
	hat_t *hatp = &as->a_hat;
	pte_t *vpdtep, *evpdtep;
	hat_stats_t *prev_modstatsp, *hat_modstatsp, *tmp_modstatsp;

	ASSERT((addr & POFFMASK) == 0);
	ASSERT((len & POFFMASK) == 0);
	ASSERT(u.u_procp->p_as == as);

	vpdtep = kpd0 + ptnum(addr);
	evpdtep = kpd0 + ptnum(addr + len - 1);

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
			kmem_free(hat_modstatsp->stats_modinfo, NPGPT);
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
#endif /* NOTYET */
}

/*
 * ppid_t
 * kvtoppid(caddr_t vaddr)
 *	Return the physical page ID corresponding to the virtual
 *	address vaddr. This is a required interface defined by DKI.
 *	For the 80x86, we use the page frame number as the physical page ID.
 *
 * Calling/Exit State:
 *      Kernel page table lookups need no locks.
 */
ppid_t
kvtoppid(caddr_t vaddr)
{
	extern ppid_t pae_kvtoppid(caddr_t);
	extern ppid_t kvtoppid32(caddr_t);

#ifdef PAE_MODE
	if (PAE_ENABLED())
		return pae_kvtoppid(vaddr);
	else
#endif /* PAE_MODE */
		return kvtoppid32(vaddr); 
}

extern paddr64_t pae_vtop(caddr_t, proc_t *);
extern paddr_t _Compat_vtop(caddr_t, proc_t *);

/*
 * paddr_t
 * vtop(caddr_t vaddr, proc_t *procp)
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
paddr_t
vtop(caddr_t vaddr, proc_t *procp)
{
	paddr64_t phys;

#ifdef PAE_MODE
	if (PAE_ENABLED()) {
		phys = pae_vtop(vaddr, procp);
		if (phys > ULONG_MAX)
			/*
			 *+ Compatiblity drivers (< DDI8) cannot address
			 *+ memory above 4G.
			 */
			cmn_err(CE_PANIC,
				"vtop: memory cannot be above 4G %Lx", phys);
		return (paddr_t)phys;
	} else
#endif /* PAE_MODE */
	{
		return _Compat_vtop(vaddr, procp);
	}
}

/*
 * paddr64_t
 * vtop64(caddr_t vaddr, proc_t *procp)
 *	Routine to translate virtual addresses to physical addresses.
 *	Typically used by AIO that need physical addresses above 4G.
 *
 * Calling/Exit State:
 *	See comments above in vtop.
 */
paddr64_t
vtop64(caddr_t vaddr, proc_t *procp)
{
#ifdef PAE_MODE
	if (PAE_ENABLED())
		return (paddr64_t)pae_vtop(vaddr, procp);
	else
#endif /* PAE_MODE */
		return (paddr64_t)_Compat_vtop(vaddr, procp);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/* The following are for debugging only */

/*
 * void
 * print_hatpt(struct as *as)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_hatpt(struct as *as)
{
	hat_t *hatp;
	hatpt_t *ptap, *eptap;
	page_t *ptpp;

	hatp = &as->a_hat;
	ptap = eptap = hatp->hat_pts;

	debug_printf("hat_pts=%x\n", ptap);
	do {
		if (!ptap)
			return;
		ptpp = ptap->hatpt_ptpp;
		if (PAE_ENABLED()) 
			debug_printf("hatpt_pde=%Lx, hatpt_pdtep=%x, hatpt_ptpp=%x\n",
				ptap->hatpt_pde64, ptap->hatpt_pdtep64, ptap->hatpt_ptpp);
		else
			debug_printf("hatpt_pde=%x, hatpt_pdtep=%x, hatpt_ptpp=%x\n",
				ptap->hatpt_pde, ptap->hatpt_pdtep, ptap->hatpt_ptpp);
		debug_printf("hatpt_as=%x, hatpt_aec=%x, hatpt_locks=%x\n",
			ptap->hatpt_as, HATPT_AEC(ptpp), HATPT_LOCKS(ptpp));
		debug_printf("hatpt_forw=%x, hatpt_back=%x\n\n",
			ptap->hatpt_forw, ptap->hatpt_back);
	} while ((ptap = ptap->hatpt_forw) != eptap);
}

/*
 * void
 * print_ptpp(page_t *ptpp)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_ptpp(page_t *ptpp)
{
	debug_printf("hatpt_aec=%x, hatpt_locks=%x\n",
			HATPT_AEC(ptpp), HATPT_LOCKS(ptpp));
	debug_printf("p_hat_refcnt=%x\n", ptpp->p_hat_refcnt);
}

/*
 * void
 * print_kvtopp(vaddr_t addr)
 *	Debug routine.
 *
 * Calling/Exit State:
 *	None.
 */
void
print_kvtopp(vaddr_t addr)
{
	debug_printf("addr=%x, pp=%x\n", addr, kvtopp(addr));
}

#endif /* DEBUG || DEBUG_TOOLS */
#ifdef DEBUG

#define	HAT_LOG_SIZE	100

typedef struct hat_log_record {
	struct as *hl_as;		/* address space requesting an op */
	vaddr_t hl_addr;		/* start address */
	vaddr_t  hl_eaddr;		/* end address of an operation */
	struct page *hl_ptpp;		/* page table page pointer */
	ulong_t hl_lockcnt;		/* lock count */
	clock_t	hl_stamp;		/* time at which the op executed */
	uint_t	hl_flags;		/* flags of the op */
	char	*hl_func;		/* operation or function name */
	int	hl_line;		/* line number */
} hat_log_record_t;

hat_log_record_t hat_log_record[MAXNUMCPU][HAT_LOG_SIZE];
uint_t hat_logndx[MAXNUMCPU];

/*
 * void
 * hat_log(struct as *, struct page *, vaddr_t, vaddr_t, 
 *		uint_t, char *func, int line)
 *
 * Calling/Exit State:
 *	None.
 */
void
hat_log(struct as *as, struct page *ptpp, vaddr_t addr, vaddr_t endaddr, 
		uint_t flags, char *func, ulong_t lockcnt, int line)
{ 
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_addr = (addr);
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_ptpp = (ptpp);
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_as = (as);
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_eaddr = (endaddr);
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_stamp = TICKS();
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_flags = (flags);
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_func = (func);
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_line = (line);
	hat_log_record[myengnum][hat_logndx[myengnum]].hl_lockcnt = (lockcnt);
	hat_logndx[myengnum] = ++hat_logndx[myengnum] % HAT_LOG_SIZE;
}


/*
 * void
 * print_hat_log_record(uint_t)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_hat_log(uint_t engnum)
{
	int i;

	for (i = 0; i < HAT_LOG_SIZE; i++) {
		debug_printf("%x:as=%x, addr=%x,%x stamp=%x, lcnt=%x, f=%x, %s,%d\n", i,
			hat_log_record[engnum][i].hl_as,
			hat_log_record[engnum][i].hl_addr,
			hat_log_record[engnum][i].hl_eaddr,
			hat_log_record[engnum][i].hl_stamp,
			hat_log_record[engnum][i].hl_lockcnt,
			hat_log_record[engnum][i].hl_flags,
			hat_log_record[engnum][i].hl_func,
			hat_log_record[engnum][i].hl_line);
	}
}

#endif /* DEBUG */

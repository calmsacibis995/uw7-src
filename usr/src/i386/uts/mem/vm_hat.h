#ifndef _MEM_VM_HAT_H	/* wrapper symbol for kernel use */
#define _MEM_VM_HAT_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/vm_hat.h	1.43.10.3"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/param.h>	 /* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */
#include <mem/immu.h> /* REQUIRED */
#include <mem/immu64.h>	/* REQUIRED */
#include <mem/mem_hier.h> /* REQUIRED */
#include <util/emask.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/param.h>	 /* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */
#include <sys/types.h> /* REQUIRED */
#include <sys/immu.h> /* REQUIRED */
#include <sys/emask.h> /* REQUIRED */
#include <vm/mem_hier.h> /* REQUIRED */
#include <vm/immu64.h>	/* REQUIRED */

#else

#include <sys/immu.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the contents of the machine specific
 * hat data structures and the machine specific hat procedures.
 * The machine independent interface is described in <vm/hat.h>.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* TLB Shootdown cookie type: */
/*
 * A clock_t is a long.  We want unsigned so we have to declare it
 * unsigned long instead of unsigned clock_t to keep lint quiet.
 */
typedef unsigned long TLBScookie_t;
extern volatile TLBScookie_t *TLBcpucookies;

/*
 * Special TLB flush interfaces for single- or double-page directed flushes.
 */
#ifdef _KERNEL
void TLBSflush1(vaddr_t);	/* Flush one page at the given address */
void TLBSflush2(vaddr_t);	/* Flush two pages at the given address */

extern lock_t hat_ptpoollock;
extern lock_t hat_pgpoollock;
extern lock_t TLBSlock;		/* serialize all tlb shootdowns */
extern pl_t ptpool_pl;
extern pl_t pgpool_pl;
/*
 * Pool locking macros.
 */
#if (!VM_INTR_IPL_IS_PLMIN)

#define PTPOOL_LOCK()	(ptpool_pl = LOCK(&hat_ptpoollock, VM_HAT_PTPOOL_IPL))
#define PTPOOL_UNLOCK()	UNLOCK(&hat_ptpoollock, ptpool_pl)
#define PGPOOL_LOCK()	(pgpool_pl = LOCK(&hat_pgpoollock, VM_HAT_PGPOOL_IPL))
#define PGPOOL_UNLOCK() UNLOCK(&hat_pgpoollock, pgpool_pl)
#define TLBS_LOCK()	LOCK(&TLBSlock, VM_HAT_TLBS_IPL)
#define TLBS_UNLOCK(oldpl)	UNLOCK(&TLBSlock, oldpl)

#else	/* VM_INTR_IPL_IS_PLMIN */

#define PTPOOL_LOCK()	(ptpool_pl = LOCK_PLMIN(&hat_ptpoollock))
#define PTPOOL_UNLOCK()	UNLOCK_PLMIN(&hat_ptpoollock, ptpool_pl)
#define	PGPOOL_LOCK()	(pgpool_pl = LOCK_PLMIN(&hat_pgpoollock))
#define PGPOOL_UNLOCK()	(UNLOCK_PLMIN(&hat_pgpoollock, pgpool_pl))
#define TLBS_LOCK()	LOCK_PLMIN(&TLBSlock)
#define TLBS_UNLOCK(oldpl)	UNLOCK_PLMIN(&TLBSlock, oldpl)

#endif /* !VM_INTR_IPL_IS_PLMIN */

#define TLBS_CPU_SIGNAL(EMASKP, FUNCP, ARG) \
		xcall(EMASKP, NULL, FUNCP, (void *)(ARG))

#endif /* _KERNEL */

#define UNSHIELDED	((vaddr_t)(-1))

#define NO_WAIT_ENG	(-1)

typedef struct xshield {
	vaddr_t	xs_vaddr;		/* user virt addr */
	vaddr_t xs_eaddr;		/* end user virt addr */
	atomic_int_t	xs_wait_eng;	/* last eng_num or NO_WAIT_ENG */
	atomic_int_t	xs_wait_cnt;	/* number of engines to wait for */
} xshield_t;

extern xshield_t *xshield;

/*
 * History:
 * The x86 HAT design is based on dividing a page table into
 * chunks that are a power of 2 long. These chunks allow the
 * mapping pointer overhead to be reduced for the typical
 * small process. The number of chunks times the number of 
 * entries equal 1024. The initial design uses 32 chunks of 32
 * entries. If translation caching (stealable translation accounting)
 * is implemented, this may change to 16 chunks of 64 entries.
 * This allows four accounting words to be available per chunk
 * (instead of 1 word) in the accounting chunk (chunk 0) of
 * a mapping chunk page. The extra words would be used for a
 * time stamp and pointers for a doubly linked list.
 *
 * The mapping chunk page declarations:
 * A mapping chunk is allocated from a physical page divided into
 * HAT_MCPP chunks of HAT_EPMC mapping pointers. The first chunk
 * of the physical page is used for free chunk accounting and
 * page table chunk linkage pointers.  The latter use generates
 * the requirement that HAT_EPMC >= HAT_MCPP so that there enough
 * room for the PTE chunk pointers. The second chunk of the page 
 * is used for keeping reference history information for each
 * mapping pointer. Each word acts as a bitmask for a mapping
 * chunk with each bit in the word denoting the reference 
 * information for a mapping pointer.
 *
 * The following union describes a page table chunk pointer. The
 * low order bits of the pointer would always be zero because of the
 * physical alignment rules of chunks.  Those implied zero bits
 * have been used for a needed count field that counts active slots
 * (NOTE a mapping pointer slot can contain NULL and still be active).
 * The mask HATPTC_ADDR is useful for using the value as a pointer
 * to a PTE chunk as in the statement:
 *
 *	ptep = (pte_t *)(ptcp->hat_ptpv & HATPTC_ADDR) + mcnx;
 *
 * There are situations where the count can be higher than the actual
 * number of active entries since it is artificially bumped in some
 * special cases (artificial bumps exclude each other by other means)
 * to allow much simpler scan code that might sleep during the scan.
 *
 * Current:
 * In the current design, p_mapping chains are eliminated from the 
 * HAT layer. The p_mapping chains were used to link the mappings
 * to the shared page. This instead is replaced by segment chaining.
 * Segment chaining links the segments sharing a vnode and hence the
 * translations to the pages belonging to the vnode. This simplifies
 * the design of the HAT layer and removes the complexity associated
 * with saving space due to mapping chunks and linkages. 
 */

#define HAT_EPPT	NPGPT	/* number of entries per page table */
#define PAE_HAT_EPPT	PAE_NPGPT	/* number of entries per page table */

/*
 * stat structure to keep the mod bit info for Locus Merge.
 */
typedef struct hat_stats {
	int	stats_refcnt;
	uint_t	*stats_modinfo;
	union {
		pte_t	*stats_pdtep;
		pte64_t *stats_pdtep64;
	} hsu;
#define	stats_pdtep	hsu.stats_pdtep
#define	stats_pdtep64	hsu.stats_pdtep64
	struct hat_stats *stats_next;
} hat_stats_t;

/*
 * The hat structure is the machine dependent hardware address translation
 * structure kept in the address space structure to show the translation.
 *
 * History:
 * The next two fields, hat_rdonly_owner and hat_rdonly_sv, are
 * used in conjunction with the hat_resourcelock to provide a
 * specific type of synchronization between the LWPs that share 
 * the same address space. The need for the synchronization is specific
 * to the Intel386(tm) processor. The purpose of the synchronization
 * is to ensure that while an LWP is performing write accesses in the
 * kernel mode to a range in the encompassing address space it is 
 * guaranteed that none of the translation protections over that 
 * range are (or become) restricted to read-only values. 
 *
 * This requirement is Intel386 specific because it arises from the
 * fact that the processor does not take a protection fault when it is
 * in supervisor mode. Code that relies on correct recognition of
 * the protection violation (such as algorithms that do backing store 
 * allocation at the time of the first write access to certain pages,
 * or the copy-on-write semantics of sharing) must be supported by
 * explicit simulation of the faults before actual accesses are
 * generated in kernel mode. 
 *
 * After an LWP has performed this type of pre-faulting, it needs to
 * co-ordinate with other LWPs, by signalling its intention that the
 * translations thus instantiated remain writeable until the original
 * LWP has completed write accesses (in supervisor mode).
 *
 * The two fields, hat_rdonly_owner and hat_rdonly_sv, provide this
 * signalling mechanism as follows. The LWP that needs to prevent the
 * loading of read-only translations (or modification of existing ones
 * to read-only) acquires an effective lock by recording its LWP 
 * pointer into hat_rdonly_owner. Any other LWP that either needs to
 * acquire the same lock or needs to load a read-only translation,
 * check that the hat_rdonly_owner field is NULL -- and if it is not,
 * they must block until the owner signals release of lock through
 * the hat_rdonly_sv synchronization object. One exception to this rule
 * is that the owner LWP is permitted to load read-only translations,
 * as otherwise, it can deadlock against itself. 
 *
 * The hat_resourcelock is the spin lock which guards the acquistion
 * and release of the read-only pseudo lock. This works because the
 * loading of any translation is accomplished entirely within the
 * coverage of hat_resourcelock.
 *	struct 	lwp *hat_rdonly_owner;
 *	sv_t	hat_rdonly_sv;	
 */
typedef struct hat {
	lock_t	hat_resourcelock;	/* spinlock for all hat resouce use
					 * including this structure.
					 */
	sv_t	hat_sv;			/* wait for as unload in mult-lwp case
					 * during exec and swtch to private L1
					 */
	pl_t	hat_lockpl;		/* pl for current hat lock holder */
	struct	hatpt *hat_pts;		/* current page table list */
	struct	hatpt *hat_ptlast;	/* last page table to fault */
	union {
		struct hatprivl1 *hat_privl1_list;
		struct hatprivl164 *hat_privl164_list;
	} hatu;
#define hat_privl1_list		hatu.hat_privl1_list
#define	hat_privl164_list	hatu.hat_privl164_list
	
	hat_stats_t *hat_modstatsp;	/* mod bit stats for merge */
	int	hat_activecpucnt;	/* count of CPUs currently active
					 * in this address space.
					 */
	emask_t	hat_activecpubits;	/* bitmap of active CPUs */
} hat_t;

typedef struct hatshpt {
	lock_t	hat_shptlock;		/* shared page table spin lock */
	uint_t	hat_rpt[4][4 * PAE_HAT_EPPT];
} hatshpt_t;	

/*
 * Gemini:
 * We keep the linked list of the private L1PTs per-process
 * and not across address spaces. We avoid the rare case of
 * dynamically changing the kernel address space.
 *
 * UW2.1.x:
 * We need to keep a linked list of all the private L1PTs
 * for the rare case that we make a change to the kernel's
 * address space that affects the L1PT.  The routines
 * hat_kas_zerol1ptes and hat_kas_reload_l1ptes will walk
 * this list and make the change in all the private L1PTs.
 */
typedef struct hatprivl1 {
	struct hatprivl1 *hp_forw;	/* next L1 page table on list */
	struct hatprivl1 *hp_back;	/* prev L1 page table on list */
	pte_t	*hp_origl1;		/* original L1 page table phys addr */
	pte_t	*hp_privl1;		/* private  L1 page table phys addr */
	pte_t	*hp_privl1virt;		/* private  L1 page table virt addr */
} hatprivl1_t;

typedef struct hatprivl164 {
	boolean_t hp_kl1busy;		/* kernel L1 is busy */
	cgnum_t	hp_cgnum;		/* cg whose L2s are used by this L1 */
	uint_t	hp_engnum;		/* engine whose per-engine L2 is used */
	paddr_t	hp_privl1[4];		/* private  L1 page table phys addr */
	pte64_t	*hp_privl1virt[4];	/* private  L1 page table virt addr */
} hatprivl164_t;

/*
 * The following structure describes a page table.
 */
typedef struct hatpgt {
	pte_t hat_pte[NPGPT];
} hatpgt_t;

typedef struct hatpgt64 {
	pte64_t hat_pte[PAE_NPGPT];
} hatpgt64_t;


/*
 * The hatpt structure is the machine dependent page table accounting
 * structure internal to the x86 HAT layer. It links an address space
 * to a currently resident page table. It constains hatpt pointers
 * for a doubly linked, circular linked list of active page tables
 * for an address space.
 */
typedef struct hatpt {
	struct	hatpt *hatpt_forw;	/* forward page table list ptr */
	struct	hatpt *hatpt_back;	/* backward page table list ptr */
	union {
	  struct {
		pte_t	hatpt_pde;		/* PDE for page table */
		pte_t	*hatpt_pdtep;		/* PDT entry pointer */
	  } hatpt32;
	  struct {
		pte64_t	hatpt_pde64;		/* PDE for page table */
		pte64_t	*hatpt_pdtep64;		/* PDT entry pointer */
	  } hatpt64;
	} ptu;
#define	hatpt_pde	ptu.hatpt32.hatpt_pde
#define	hatpt_pdtep	ptu.hatpt32.hatpt_pdtep
#define	hatpt_pde64	ptu.hatpt64.hatpt_pde64
#define	hatpt_pdtep64	ptu.hatpt64.hatpt_pdtep64

	struct page *hatpt_ptpp;	/* physical page of page table */
	struct	as *hatpt_as;		/* pointer back to containing as */
} hatpt_t;

struct hatpt_pgprv {
	ulong_t	hatpt_aec;		/* active entry count */
	ulong_t	hatpt_locks;		/* count of locked PTEs */
	hatpt_t	*hatpt_ptap;		/* hatpt pointer */
};

#define	HATPT_PGPRV(pp)		((struct hatpt_pgprv *)(pp)->p_pgprv_data)
#define	HATPT_AEC(pp)		(HATPT_PGPRV(pp)->hatpt_aec)
#define	HATPT_LOCKS(pp)		(HATPT_PGPRV(pp)->hatpt_locks)
#define	HATPT_PTAP(pp)		(HATPT_PGPRV(pp)->hatpt_ptap)


/*
 * HAT-specific parts of a page structure.
 */
union phat {
	volatile long _p_hat_refcnt;	/*
					 * # of contexts that have "recently"
					 * referenced the page. This is a hint.
					 */
};

#define p_hat_refcnt    p_hat._p_hat_refcnt

#define HAT_SYNC_THRESH	25	/* used in hat_agerange() */

/*
 * Flags to pass to hat_ptalloc() or hat_pgalloc().
 */
#define	HAT_NOSLEEP	0x1	/* return immediately if no memory */
#define HAT_CANWAIT	0x2	/* wait if no memory currently available */
#define HAT_POOLONLY	0x3	/*
				 * Internal flag for pt and chunk alloc.
				 * Attempt to service the request from
				 * the resource free pool only, never
				 * call kpg_alloc() or kmem_zalloc().
				 * Must be used when any page or hat
				 * locks are held.
				 * Used by hat_map and hat_pteload.
				 * Will be used by hat_dup, when it is coded.
				 */
#define HAT_REFRESH	0x4	/*
				 * Allocate the requested resource (NOSLEEP)
				 * and if successful, try to refresh the
				 * free pools (also a NOSLEEP operation).
				 * This allows optimal lock handling in
				 * hat_map and probably hat_dup.
				 */
#define	HAT_FIRSTPT	0x100
#define HAT_ALLOCPT	0x800
#define HAT_ALLOCPSE	0x1000

#endif	/* _KERNEL || _KMEMUSER */

/* some HAT-specific macros: */

#ifdef _KERNEL

/*
 * hat_getshootcookie()
 *	Give the caller the current cookie.
 *
 * Calling/Exit State:
 *	Nothing special, no locking (or much else) involved.
 */

#define hat_getshootcookie()	((TLBScookie_t)TICKS())

/*
 * The following lock is used in conjunction with the page layer locks
 * due the page layer / hat layer interactions; thus the lock is keyed off
 * the VM_HAT_PAGE_HIER_MIN hierarchy 
 */ 
#define VM_HAT_RESOURCE_HIER	VM_HAT_PAGE_HIER_MIN	/* hat resource lock */
#define	VM_HAT_RESOURCE_IPL		VM_INTR_IPL

/*
 * The following locks are used only internal to the hat layer. Thus these
 * locks are keyed off the VM_HAT_LOCAL_HIER_MIN hierarchy.
 */
#define VM_HAT_TLBS_HIER	VM_HAT_LOCAL_HIER_MIN	/* TLBSlock */
#define VM_HAT_TLBS_IPL		VM_INTR_IPL

#define VM_HAT_PTPOOL_HIER	(VM_HAT_LOCAL_HIER_MIN + 5) /* ptpoollock */
#define VM_HAT_PTPOOL_IPL	VM_INTR_IPL

#define VM_HAT_PGPOOL_HIER	VM_HAT_PTPOOL_HIER	/* hat_pgpoollock */
#define VM_HAT_PGPOOL_IPL	VM_INTR_IPL

#if (!VM_INTR_IPL_IS_PLMIN)

#define HATRL_LOCK_SVPL(HATP)	\
	(HATP->hat_lockpl = LOCK(&HATP->hat_resourcelock, VM_HAT_RESOURCE_IPL))
#define HATRL_LOCK(HATP)	\
	LOCK(&HATP->hat_resourcelock, VM_HAT_RESOURCE_IPL)

/* release hat resource lock */
#define HATRL_UNLOCK_SVDPL(HATP)	\
	UNLOCK(&HATP->hat_resourcelock, HATP->hat_lockpl)
#define HATRL_UNLOCK(HATP, PL)	\
	UNLOCK(&HATP->hat_resourcelock, PL)

#define HATRL_TRYLOCK(HATP)	\
	TRYLOCK(&HATP->hat_resourcelock,  VM_HAT_RESOURCE_IPL)

#define HATRL_LOCK_SH(HATP)	\
	LOCK_SH(&HATP->hat_resourcelock,  VM_HAT_RESOURCE_IPL)

#else 	/* VM_INTR_IPL_IS_PLMIN */ 

#define HATRL_LOCK_SVPL(HATP)	\
	(HATP->hat_lockpl = LOCK_PLMIN(&HATP->hat_resourcelock))

#define HATRL_LOCK(HATP)	\
	LOCK_PLMIN(&HATP->hat_resourcelock)

/* release hat resource lock */
#define HATRL_UNLOCK_SVDPL(HATP)	\
	UNLOCK_PLMIN(&HATP->hat_resourcelock, HATP->hat_lockpl)

#define HATRL_UNLOCK(HATP, PL)	\
	UNLOCK_PLMIN(&HATP->hat_resourcelock, PL)

#define HATRL_TRYLOCK(HATP)	\
	TRYLOCK_PLMIN(&HATP->hat_resourcelock)

#define HATRL_LOCK_SH(HATP)	\
	LOCK_SH_PLMIN(&HATP->hat_resourcelock)

#endif	/* !VM_INTR_IPL_IS_PLMIN */

/* check the hat resource lock */
#define HATRL_OWNED(HATP)	LOCK_OWNED(&HATP->hat_resourcelock)

/*
 * MACRO
 * SETPTRS(pndx, ptep, addr, ptpp, kvaddr, inctx)
 *	Temporary mappings are set up at the <kvaddr> if the access
 *	to the page table is not within the context. It sets ptep.
 *
 * Calling/Exit State:
 *	It is called from hat_prep().
 */
#define SETPTRS(pndx, ptep, addr, ptpp, kvaddr, inctx) { \
	hatpgt_t *pt = (hatpgt_t *)vtoptkv(addr); \
				\
	if ((inctx)) { \
		(ptep) = pt->hat_pte + (pndx); \
		ASSERT((ptep)); \
	} else { \
		kvtol2ptep((kvaddr))->pg_pte = mkpte(PG_RW|PG_V, \
			(ptpp)->p_pfn);\
		TLBSflush1((kvaddr)); \
		(ptep) = (pte_t *)((kvaddr)) + (pndx); \
	} \
}

#define SETPTRS64(pndx, ptep, addr, ptpp, kvaddr, inctx) { \
	hatpgt64_t *pt = (hatpgt64_t *)vtoptkv64(addr); \
				\
	if ((inctx)) { \
		(ptep) = pt->hat_pte + (pndx); \
		ASSERT((ptep)); \
	} else { \
		kvtol2ptep64((kvaddr))->pg_pte = pae_mkpte(PG_RW|PG_V, (ptpp)->p_pfn);\
		TLBSflush1((kvaddr)); \
		(ptep) = (pte64_t *)((kvaddr)) + (pndx); \
	} \
}

/*
 * MACRO
 * FREE_PT(ptap, as)
 *
 * Calling/Exit State:
 *	It is called from hat_unprep().
 */
#define FREE_PT(ptap, as) { \
	ASSERT(HATPT_LOCKS((ptap)->hatpt_ptpp) == 0); \
	if ((as) == u.u_procp->p_as) \
	    (ptap)->hatpt_pdtep->pg_pte = 0; \
	hat_unlink_ptap((as), (ptap)); \
	hat_ptfree((ptap)); \
}

/*
 * Bump the rss size for the as.
 */ 
#ifdef lint
#define BUMP_RSS(as, count) { \
		(as)->a_rss += (count); \
}
#else	/* lint */
#define BUMP_RSS(as, count) { \
		ASSERT((count) > 0 || (as)->a_rss > 0);	\
		(as)->a_rss += (count); \
}
#endif	/* lint */

/*
 * boolean_t
 * HAT_REDUCING_PERM(pte, prot)
 *	Determines whether protection is being downgraded for the given pte.
 *
 * Calling/Exit State:
 *	Called with hat_resourcelock held.
 *
 * Remarks:
 *	There are three possible scenarios of downgrading protection:
 *	1) Going from write to read
 *	2) Going from read to prot_none
 *	3) Going from write to prot_none
 */
#define HAT_REDUCING_PERM(pte, prot) \
		(((pte) & PG_RW && !((prot) & PG_RW)) || \
		 ((pte) & PG_V && !((prot) & PG_V)))

/*
 * Bump the active pte count for chunk, and the active pte count for
 * page table.
 */
#ifdef DEBUG
#ifdef lint
#define BUMP_AEC(ptpp, count) { \
		HATPT_AEC((ptpp)) += (count);   \
		if (PAE_ENABLED()) \
			pae_hat_checkptcnt((ptpp)); \
}
#else   /* lint */
#define BUMP_AEC(ptpp, count) { \
		ASSERT((count) > 0 || HATPT_AEC((ptpp)));       \
		HATPT_AEC((ptpp)) += (count);   \
		if (PAE_ENABLED()) \
			pae_hat_checkptcnt((ptpp)); \
}
#endif	/* lint */
#else
#ifdef lint
#define BUMP_AEC(ptpp, count) { \
		HATPT_AEC((ptpp)) += (count);	\
}
#else 	/* lint */ 
#define BUMP_AEC(ptpp, count) { \
		ASSERT((count) > 0 || HATPT_AEC((ptpp)));	\
		HATPT_AEC((ptpp)) += (count);	\
}
#endif 	/* lint */
#endif	/* DEBUG */

#define INCR_LOCK_COUNT(ptpp, as) { \
		HATPT_LOCKS((ptpp))++; \
		ASSERT(HATPT_LOCKS((ptpp)) <= NPGPT); \
		(as)->a_lockedrss++; \
}

#define DECR_LOCK_COUNT(ptpp, as) { \
		ASSERT(HATPT_LOCKS((ptpp)) > 0); \
		--HATPT_LOCKS((ptpp)); \
		ASSERT((as)->a_lockedrss != 0); \
		(as)->a_lockedrss--; \
}

#define INCR_PAGE_REFCNT(pp) { \
		(pp)->p_hat_refcnt++; \
}

#define DECR_PAGE_REFCNT(pp) { \
		(pp)->p_hat_refcnt--; \
}

/*
 * void
 * HAT_AS_FREE(struct as *as, struct hat *hat)
 * HAT_AS_FREE64(struct as *as, struct hat *hat)
 *	Deallocate a totally unreferenced as structure.
 *
 * Remarks:
 *	The call to hat_free_modstats() is temporary until the
 *	close-on-exec problem is fixed to close the file in the
 *	context of the old AS.
 */
#define HAT_AS_FREE(as, hatp) {                                         \
	ASSERT(hatp == &(as)->a_hat); \
	if (HAT_PRIVL1_LIST((hatp))) { \
		hatprivl1_t *hpl1p, *nexthpl1p; \
						\
		hpl1p = HAT_PRIVL1_LIST((hatp)); \
		do { \
			ASSERT(hpl1p->hp_origl1 == (pte_t *)NULL); \
			nexthpl1p = hpl1p->hp_forw; \
			if (hpl1p->hp_privl1) { \
				hat_unlink_privl1((hatp), hpl1p); \
				kpg_free(hpl1p->hp_privl1virt, (ulong_t)1); \
				kmem_free(hpl1p, sizeof(hatprivl1_t)); \
			} \
			hpl1p = nexthpl1p; \
		} while (HAT_PRIVL1_LIST((hatp)) != (hatprivl1_t *)NULL); \
		ASSERT(HAT_PRIVL1_LIST((hatp)) == (hatprivl1_t *)NULL); \
	} \
			\
	if ((hatp)->hat_modstatsp != NULL)                              \
		hat_free_modstats(as);                                  \
	LOCK_DEINIT(&(hatp)->hat_resourcelock);                         \
	as_dealloc(as);                                                 \
}

extern void hat_unlink_privl1(hat_t *, hatprivl1_t *);

#define HAT_AS_FREE64(as, hatp) {                                         \
	ASSERT(hatp == &(as)->a_hat); \
	if ((hatp)->hat_privl164_list) { \
		hatprivl164_t *hpl1p; \
						\
		hpl1p = (hatp)->hat_privl164_list; \
		kpg_free(hpl1p->hp_privl1virt[0], \
			(uvend == MINUVEND) ? PDPTSZ - 1 : PDPTSZ); \
		kmem_free(hpl1p, sizeof(hatprivl164_t)); \
	} \
			\
	if ((hatp)->hat_modstatsp != NULL)                              \
		hat_free_modstats(as);                                  \
	LOCK_DEINIT(&(hatp)->hat_resourcelock);                         \
	as_dealloc(as);                                                 \
}

extern void hat_unlink_privl1(hat_t *, hatprivl1_t *);

/*
 * MACRO
 * MAPPT(kvaddr, pp)
 * MAPPT64(kvaddr, pp)
 *      Create mapping to the page <pp> at temporary kernel
 *      virtual address <addr>.
 *
 * Calling/Exit State:
 *      None.
 */
#define MAPPT(kvaddr, pp, tptep) { \
	pte_t *ptep1; \
	/* flush the TLB before setting the mapping */ \
	ptep1 = kvtol2ptep((kvaddr)); \
        if(ptep1->pgm.pg_pfn != page_pptonum((pp))) { \
	   ptep1->pg_pte = mkpte(PG_RW|PG_V, page_pptonum((pp))); \
	   TLBSflush1((vaddr_t)kvaddr); \
	}\
	ASSERT(kvtopp((kvaddr)) == pp); \
}

#define MAPPT64(kvaddr, pp, tptep) { \
	pte64_t *ptep1; \
	/* flush the TLB before setting the mapping */ \
	ptep1 = kvtol2ptep64((kvaddr)); \
	(tptep)->pg_pte = ptep1->pg_pte; \
	ptep1->pg_pte = pae_mkpte(PG_RW|PG_V, page_pptonum((pp))); \
	TLBSflush1((vaddr_t)kvaddr); \
	ASSERT(kvtopp64((kvaddr)) == pp); \
}

#define SHPT_MAPPT64(kvaddr, ppid, tptep) { \
	pte64_t *ptep1; \
	/* flush the TLB before setting the mapping */ \
	ptep1 = kvtol2ptep64((kvaddr)); \
	(tptep)->pg_pte = ptep1->pg_pte; \
	ptep1->pg_pte = pae_mkpte(PG_RW|PG_V, (ppid)); \
	TLBSflush1((vaddr_t)kvaddr); \
}

/*
 * MACRO
 * UNMAPPT(kvaddr, pp)
 * UNMAPPT64(kvaddr, pp)
 *      Remove mapping to the page <pp> at temporary per-engine
 *      kernel virtual address <addr>.
 *
 * Calling/Exit State:
 *      None.
 */
#define UNMAPPT(kvaddr, tptep)

#define UNMAPPT64(kvaddr, tptep) { \
	pte64_t *ptep1; \
	ptep1 = kvtol2ptep64((kvaddr)); \
	ptep1->pg_pte = (tptep)->pg_pte; \
	TLBSflush1((vaddr_t)(kvaddr)); \
}

#define	SHPT_UNMAPPT64(kvaddr, tptep)	UNMAPPT64((kvaddr), (tptep))

enum hat_cont_type {
	NEXTPT,		/* process next page table */
	ALLOCPT,	/* allocate page table */
	DROPLOCKS,	/* drop the locks that are held */
	TRYLOCK_FAIL,	/* trylock of the vnode failed */
	NOMORE		/* all done, exit */
};

/*
 * 		Notes on synchronization needed for local aging 
 *			of address spaces 
 *
 * One way to ensure that in a multithreaded address space, a single LWP
 * can undertake the address space aging step without causing other LWPs
 * to experience MMU inconsistencies is to deactivate them while the
 * aging step is in progress; this is accomplished via the 
 * vm_seize()/vm_unseize() interfaces with the process/LWP subsystems.
 *
 * However, it may be possible to support the local aging of address spaces 
 * without requiring that all LWPs in the address space be seized. 
 * This can happen, trivially, if there is only one LWP in the address 
 * space --i.e., the one that is aging the AS.
 *
 * When there are more than one LWPs in the AS, it is still possible,
 * depending upon processor MMU characteristics, to proceed with local
 * aging without arresting other LWPs. The requirement is that an MMU
 * access that can cause the MODIFY bit to become set also generates
 * a write-through update of the non-cached (i.e., not cached in MMU)
 * copy of the translation. (It is assumed that should this update fail
 * to find the translation valid, full fault processing would be
 * initiated on the missing translation -- eventhough the MMU itself
 * had not registered a miss immediately prior to the update. 
 *
 * The property described above permits the asynchronous removal of
 * page table entries provided the agent doing the removal holds the
 * page tables locked. Any concurrent modifying accesses by other
 * LWPs would be held off due to the need for full fault processing.
 * Note that removal of the entries by itself does not yet free the
 * associated pages; the page freeing is deferred to occur until after
 * TLB entries are globally removed. 
 *
 * In the asynchronous aging approach, the LWP that performs the aging step
 * must hold the address space read locked in order to guarantee segment
 * chain consistency. 
 *
 * This approach of not deactivating (not vm_seize'ing) the LWPs during
 * local aging trades off the vm_seize overhead against the possiblility
 * of busy-waiting that can result on other processors if the other LWPs 
 * fault and then busy wait for hat lock(s).
 */

struct proc;
extern	boolean_t		local_age_lock(struct proc *);
#define	LOCAL_AGE_LOCK(procp)	local_age_lock((procp))
#define	LOCAL_AGE_UNLOCK(procp)	as_unlock((procp)->p_as)

/*
 * Functions/macros used by vm_hat.c, vm_hatstatic.c, and/or pse_hat.c
 */
struct page;
extern void hat_pgfree(struct page *);
extern void hat_ptfree(hatpt_t *);
extern void hat_link_ptap(struct as *, hatpt_t *, hatpt_t *);
extern void hat_unlink_ptap(struct as *, hatpt_t *);
extern void hat_vprotinit(void);
extern void shpt_hat_deinit(hatshpt_t *);
extern hatshpt_t *shpt_hat_init(uint_t );
extern void shpt_hat_load(struct as *, hatshpt_t *, vaddr_t, uint_t,
			  ppid_t, uint_t);
extern void shpt_hat_unload(struct as *, hatshpt_t *, uint_t);
extern void shpt_hat_attach(struct as *, vaddr_t, size_t, hatshpt_t *, cgnum_t);
extern void shpt_hat_detach(struct as *, vaddr_t, size_t, hatshpt_t *);
extern uint_t shpt_hat_xlat_info(struct as *, hatshpt_t *, vaddr_t);

#define hat_vtop_prot(prot)	vprot_array[(prot) & PROT_ALL]

extern uint_t vprot_array[];

/*
 * The following constants:
 *
 *	HAT_MAP_MAX_SIZE
 *	HAT_MAP_VPLIST_TRIES
 *	HAT_MAP_THRESH
 *	HAT_MAP_MIN_SIZE
 *
 * are used to direct the method by which hat_map finds
 * the vnode pages that may be premapped as an optimization.
 *
 * The ordinary search method would be to walk the "unsorted"
 * vpages list for the vnode and select pages that happen to
 * be within the segment range. For large lists, this method
 * can be burdensome. The alternative would be to search by
 * identity, off the page hash chains; but here the added cost 
 * of hash search must be weighed.
 *
 * To implement suitable tradeoff points, we start with the
 * vpage list method. When we see too many failures there, we
 * stop and return a count back to hat_map. hat_map then uses
 * this count, and the knowledge of segment size, to decide 
 * whether to proceed with the identity search method or not.
 * 
 * HAT_MAP_VPLIST_TRIES specifies how large a failure-to-success ratio
 * should the vpage list method continue to tolerate as it marches
 * down the vpages list.
 *
 * If the vpages list method has covered x pages (either successfully
 * mapped them or decided it was possible to map them but not ideal 
 * to do) then, HAT_MAP_THRESH specifies how small can x be for
 * it to make sense to go onto the identity search method.
 *
 * Finally, if a segment is very large, we may want to suspend going
 * to the identity search method altogether, once the vpages list
 * method has been applied. HAT_MAP_MAX_SIZE provides this threshold.
 */
#define HAT_MAP_VPLIST_TRIES	10
#define HAT_MAP_THRESH		10
#define HAT_MAP_MAX_SIZE	32
#define HAT_MAP_MIN_SIZE	3

/*
 * void
 * HAT_MAP_INTRPEND(struct hat *hatp, struct page *pp)
 *
 * 	If a pending interrupt is discovered, drop all the 
 *	spinlocks held (vm_pagefreelock, hat_resourcelock, 
 *	vnode page lock, and the vm_hashlock) during hat_map, 
 *	and then reacquire them. 
 *
 * Calling/Exit State:
 *	Not called from interrupt level. The context holds exactly
 *	the three spinlocks described. 
 */
#define HAT_MAP_INTRPEND(hatp, vp)  { \
	VM_PAGEFREEUNLOCK(); \
	HATRL_UNLOCK_SVDPL(hatp); \
	VN_PGUNLOCK(vp); \
	VM_HASHUNLOCK(); \
	ASSERT(KS_HOLD0LOCKS()); \
	VM_HASHLOCK(); \
	VN_PGLOCK(vp); \
	HATRL_LOCK_SVPL(hatp); \
	VM_PAGEFREELOCK(); \
}

/*
 * struct page *
 * HAT_MAP_FIND_PAGE(vp, off, pp) 
 *
 * 	Find the page with this identity and if it is on the free list,
 * 	try to get it from the free list. 
 *
 * 	Return pp as NULL if the page does not exist.
 *
 * 	If the page exists, but -
 *		- it is write-locked   
 *		or,
 *		- it is free but cannot be reclaimed from freelist
 * 	then return pp as NULL too.
 *
 * REMARKS: 
 *	Caller is expected to have ensured that the vnode vpages
 *	list is somehow stabilized. This would be expected to be
 *	done by holding the vnode vpglock.
 */
#ifndef CCNUMA
#define HAT_MAP_FIND_PAGE(seg, vp, off, pp, hatp) { \
	pp = page_exists((vp), (off)); \
	if (pp) { \
		if ( ((pp)->p_invalid) || \
				(((pp)->p_free != 0) && \
				 (!page_reclaim_l((pp), P_LOOKUP)) ) ) { \
			pp = (page_t *)NULL; \
		} \
	} \
}
#else /* CCNUMA */
#define HAT_MAP_FIND_PAGE(seg, vp, off, pp, hatp) { \
	pp = page_exists((vp), (off)); \
	if (pp) { \
		if ( ((pp)->p_invalid) || \
				(((pp)->p_free != 0) && \
				 (!page_reclaim_l((pp), P_LOOKUP)) ) ) { \
			pp = (page_t *)NULL; \
		} else if (vp->v_place.pl_type == PP_REPLICATED) { \
			VM_PAGEFREEUNLOCK(); \
			HATRL_UNLOCK_SVDPL(hatp); \
			pp = page_find_closest(seg, pp, 0); \
			HATRL_LOCK_SVPL((hatp)); \
			VM_PAGEFREELOCK(); \
		} \
	} \
}
#endif /* CCNUMA */


/*
 * This constant should not be changed lightly. Code dealing with 
 * vpglock array will be affected.
 */
#define HAT_MAX_VPGLOCKS	2

#define HAT_VPGLOCK_EXISTS(vp, vpglocks) \
	((((vpglocks)[0]) == (vp) || ((vpglocks)[1]) == (vp)) ? B_TRUE : B_FALSE)

/*
 * HAT_VPGUNLOCK_ALL(nvpg_locks, vpglocks)
 *	All locks will be dropped at VM_INTR_IPL level. Thus, there should
 * 	be atleast 1 lock held at this point.
 */
#define HAT_VPGUNLOCK_ALL(nvpg_locks, vpglocks) {			\
	ASSERT(KS_HOLDLOCKS());						\
	while ((nvpg_locks) > 0) {					\
		--(nvpg_locks);						\
		VN_PGUNLOCK_PL((vpglocks)[(nvpg_locks)], VM_INTR_IPL);	\
		vpglocks[(nvpg_locks)] = NULL;				\
	}								\
}

#define HAT_VPGLOCK(vp, nvpg_locks, vpglocks) {	\
	ASSERT((nvpg_locks) == 0);		\
	VN_PGLOCK(vp);				\
	(vpglocks)[0] = (vp);			\
	(nvpg_locks) = 1;			\
}

/*
 * boolean_t
 * HAT_TRYVPGLOCK(vnode_t *vp, int *numvpg_locks, vnode_t *vpglocks)
 *
 *	The PAGE_VPGLOCK() will be attempted to trylocked.
 *
 * Calling/Exit State:
 *	The page's vnode is stabilized by the caller.
 *	The caller adjusts the pl value (l.vpglockpl) for the vpglock
 *	trylocked.
 *	This routine may unlock a previously held vpage lock. The caller
 *	should tolerate this.
 *	Caller should have already called HAT_VPGLOCK_EXISTS() macro to
 *	check if the lock for this page exists and only then call this
 *	routine.
 *
 *	Returns TRUE if the lock is held on return. FALSE otherwise.
 *	
 * Remarks:
 *	If the number of vpglocks held is greater than HAT_MAX_VPGLOCKS,
 *	this routine unlocks the first lock in the vpglock array if the
 *	trypglock for the current page succeeds.
 */
#define HAT_TRYVPGLOCK(vp, nvpg_locks, vpglocks) \
	(ASSERT((vp) != NULL), VN_PGTRYLOCK((vp)) == INVPL ? B_FALSE : \
		((nvpg_locks) < HAT_MAX_VPGLOCKS ? 		\
			(vpglocks)[(nvpg_locks)++] = (vp) :	\
			(VN_PGUNLOCK_PL((vpglocks)[HAT_MAX_VPGLOCKS - 1], VM_INTR_IPL),	\
 			  ((vpglocks)[HAT_MAX_VPGLOCKS - 1] = (vp))), \
				B_TRUE))

/*
 * void
 * flushtlb(void)
 *	TLB flush asm call. 
 *
 * Calling/Exit State:
 *	None.
 */
asm void flushtlb(void)
{
	movl	%cr3, %eax
	movl	%eax, %cr3
}
#pragma asm partial_optimization flushtlb

/*
 * void
 * flushtlb1(vaddr_t addr)
 *	TLB flush one page asm call.
 *
 * Calling/Exit State:
 *	None.
 */
asm void flushtlb1(vaddr_t addr)
{
%reg	addr
	invlpg	(addr)
%mem	addr
	movl	addr, %eax
	invlpg	(%eax)
}
#pragma asm partial_optimization flushtlb1

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_VM_HAT_H */


#ifndef _MEM_VMPARAM_H	/* wrapper symbol for kernel use */
#define _MEM_VMPARAM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/vmparam.h	1.60.9.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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
 * 
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

#ifdef _KERNEL_HEADERS
#include <proc/user.h>		/* PORTABILITY */
#include <svc/cpu.h>		/* PORTABILITY */
#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/param.h>		/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/user.h>		/* PORTABILITY */
#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* PORTABILITY */

/*
 * cpu.h not installed in usr/include, but is only needed by VM itself,
 * which is always the _KERNEL_HEADERS case.
 */

#ifdef	_KERNEL

#include <sys/ksynch.h>		/* REQUIRED */

#endif

#endif /* _KERNEL_HEADERS */

/*
 * This file contains architecture family specific VM definitions.
 * Platform specific VM definitions are contained in <mem/vm_mdep.h>.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Lower bound of end of user virtual address range as guaranteed
 * by the ABI
 */
#define MINUVEND ((vaddr_t)0xC0000000L)

#ifdef	_KERNEL

/*
 * Division of virtual addresses between user and kernel.
 */

#define UVBASE	 ((vaddr_t)0x00000000L)	/* base user virtual address */
extern const vaddr_t uvend;		/* end of user virtual address range */
extern const vaddr_t kvbase;		/* base of kernel virtual range */
extern char _end[];
#define KVEND	 ((vaddr_t)0x00000000L) /* end of kernel virtual range */

/*
 * Determine whether vaddr is a kernel virtual address.
 */
#define KADDR(vaddr)		((vaddr_t)(vaddr) >= kvbase)
/*
 * Macro to execute a statement for each cg. The statement
 * should be formulated in terms of ptep. This macro is
 * useful for manipulating replicated kernel page tables.
 */
#endif	/* _KERNEL */

#ifdef CCNUMA

#define FOR_EACH_CG_PTE(addr) {					\
	cgnum_t cgnum;						\
	pte64_t *cg_ptep;					\
								\
	ASSERT(!hat_static_callocup);				\
	ASSERT(!KADDR_PER_ENG(addr));				\
								\
	for (cgnum = 0; cgnum < Ncg; cgnum++) {			\
		cg_ptep = kvtol2ptep64_g(addr, cgnum);	\

#define END_FOR_EACH_CG_PTE	\
	}			\
}

#endif /* CCNUMA */

/*
 * Functions which return a physical page ID return NOPAGE if
 * there is no valid physical page ID.
 * This is required for the DDI.
 */

#define NOPAGE		((ppid_t)-1)

/*
 * This is to maintain backward compatibility for DDI function hat_getppfnum.
 * PSPACE_MAINSTORE is the code for the physical address space which
 * includes, at least, "mainstore" system memory, which is the memory that
 * programs (and the kernel) execute out of.  See hat_getppfnum(D3K).
 */

#define PSPACE_MAINSTORE	0

#define	SYSDAT_PAGES	1	/* for now */

/*
 * Fixed kernel virtual addresses.
 *
 * These addresses are all mapped by a level 2 page table which is
 * replicated for each engine to allow some of these mappings to
 * be different on a per-engine basis.  The rest of these mappings
 * are statically defined for the life of the system at boot.
 *
 * Each of these addresses must begin on a page boundary.
 *
 * WARNING: Since USER_DS must include UVUVWIN, any pages below this
 * address will be visible to the user if they have user page permissions.
 */

#define KVPER_ENG_END	((vaddr_t)0xFFFFF000)
	/* high end of per-engine range (inclusive) */

#define KVTMPPG3	KVPER_ENG_END
#define KVTMPPG2	(KVTMPPG3 - MMU_PAGESIZE)
	/* per-engine temporary page slot for pagezero(), ppcopy() */

#define KVTMPPG1	(KVTMPPG2 - MMU_PAGESIZE)
	/* per-engine temporary page slot for pagezero(), ppcopy();
	 * KVTMPPG1 and KVTMPPG2 must be contiguous, with KVTMPPG1 first */

#define KVTMPPT2	(KVTMPPG1 - MMU_PAGESIZE)
	/* per-engine temporary page slot for page table mappings */

#define KVTMPPT1	(KVTMPPT2 - MMU_PAGESIZE)
	/* per-engine temporary page slot for page table mappings */

#define KVPHYSMAP1	(KVTMPPT1 - MMU_PAGESIZE)
	/* per-engine temporary page slot for physmap1() */

#define KVMET		(KVPHYSMAP1 - (MET_PAGES * PAGESIZE))
	/* global kernel metrics */

#define KVPLOCALMET	(KVMET - (PLMET_PAGES * PAGESIZE))
	/* per-engine struct plocalmet; also mapped at e_local->pp_localmet  */

#define KVSYSDAT	(KVPLOCALMET - (SYSDAT_PAGES * PAGESIZE))
	/* system read-only data: eg; hrt timer */

#define KVPLOCAL	(KVSYSDAT - (PL_PAGES * MMU_PAGESIZE))
	/* per-engine struct plocal; also mapped at e_local->pp_local;
	 * must be beyond USER_DS */

#define KVCGLOCAL       (KVPLOCAL - (CGL_PAGES * MMU_PAGESIZE))
        /* per-CG struct cglocal */
#define KVENG_L2PT      (KVCGLOCAL - MMU_PAGESIZE)
	/* per-engine level 2 page table; also mapped at e_local->pp_pmap */

#define	PDPTSHFT	2
#define	PDPTSZ		(1 << PDPTSHFT)
#define KVENG_L1PT	(KVENG_L2PT - PDPTSZ * MMU_PAGESIZE)
	/* per-engine level 1 page table; also mapped at e_local->pp_kl1pt */

#define FPEMUL_PAGES	16	/* max # pages for FPU emulator */

#define KVFPEMUL	(KVENG_L1PT - (FPEMUL_PAGES * MMU_PAGESIZE))
	/* FPU emulator code/data; must be beyond USER_DS */

#define KVUENG		(KVFPEMUL - (USIZE * PAGESIZE))
	/* per-engine idle ublock; also mapped at e_local->pp_ublock;
	 * must be beyond USER_DS */

#define KVUENG_EXT	(KVUENG - (KSE_PAGES * MMU_PAGESIZE))
	/* stack extension page for per-engine ublock */

#define KVUENG_REDZONE	(KVUENG_EXT - MMU_PAGESIZE)
	/* red zone (unmapped) to catch per-engine kernel stack overflow */

#define KVUVWIN		(KVUENG_REDZONE - MMU_PAGESIZE)
	/* kernel-writeable mapping to UVUVWIN page
	 * also mapped at e_local->pp_uvwin */

#define UVUVWIN		(KVUVWIN - MMU_PAGESIZE)
	/* per-engine page to contain user-visible read-only data;
	 * included in USER_DS so user progs can access directly */

#define KVENG_PAGES	(UVUVWIN - SZPPRIV_PAGES_PAE * MMU_PAGESIZE)
	/* all of per-engine pages of a SBSP mapped at this address for
	 * later use in scg_sysinit */

#define KVLAST_ARCH	KVENG_PAGES
	/* KVLAST_ARCH is the last fixed kernel virtual address (working 
	 * down from high memory) allocated by architecture-specific but
	 * platform-independent code.  This symbol can be used by
	 * platform-specific code to begin allocating additional
	 * fixed kernel virtual addresses.
	 */

	/* start of 4 Meg per-engine static area */
#define KVPER_ENG_STATIC	((vaddr_t)0xFFC00000)

	/* start of 2 Meg per-engine static area */
#define KVPER_ENG_STATIC_PAE	(KVPER_ENG_STATIC + pae_ptbltob(1))

extern vaddr_t	kvper_eng;
#define	KVPER_ENG		kvper_eng

/*
 * Space for per-CG KL2PTEs in the CCNUMA kernel.
 */
extern vaddr_t			kl2ptes;
#define KL2PTES			kl2ptes

/*
 * The following defines indicate the limits of allocatable kernel virtual.
 * That is, the range [KVAVBASE,KVAVEND) is available for general use.
 */
#define KVAVBASE	kvbase
#define KVAVEND		KL2PTES

/*
 * Other misc virtual addresses.
 */

#define UVSTACK	 ((vaddr_t)0x80000000L)	/* default user stack location */
#define UVMAX_STKSIZE	0x1000000

/*
 * Determine whether kernel address vaddr is in the per-engine range.
 */

#define KADDR_PER_ENG(vaddr)	((vaddr_t)(vaddr) >= KVPER_ENG)


/*
 * Determine whether [addr, addr+len) are valid user address.
 */

#define VALID_USR_RANGE(addr, len) \
	((vaddr_t)(addr) + (len) > (vaddr_t)(addr) && \
	 (vaddr_t)(addr) >= UVBASE && (vaddr_t)(addr) + (len) <= uvend)

/*
 * Given an address, addr, which is in or just past a valid user range,
 * return the (first invalid) address just past that user range.
 */
#define VALID_USR_END(addr)	uvend

#ifdef _KERNEL

/*
 * WRITEABLE_USR_RANGE() checks that an address range is within the
 * valid user address range, and that it is user-writeable.
 * On machines where read/write page permissions are enforced on kernel
 * accesses as well as user accesses, this can be simply defined as
 * VALID_USR_RANGE(addr, len), since the appropriate checks will be done
 * at the time of the actual writes.  Otherwise, this must also call a
 * routine to simulate a user write fault on the address range.
 */

#define WRITEABLE_USR_RANGE(addr, len) \
		VALID_USR_RANGE(addr, len)

/*
 * END_USER_WRITE() is called after writing to user address space
 * to perform any necessary clean-up from the previous WRITEABLE_USR_RANGE().
 */

#define END_USER_WRITE(addr, len)

/*
 * Function prototypes for entry points visible to FSKI-conformant fstypes.
 */

#ifdef _FSKI
extern void map_addr(vaddr_t *, uint_t, off_t, int);
#else
extern void map_addr(vaddr_t *, uint_t, off64_t, int);
#endif

#endif /* _KERNEL */

/*
 * KVPTE:
 *
 * The level 2 ptes are now accessed by reserving a fixed kernel virtual
 * that references back to the L1. In an out of context access temporary
 * mappings are setup to access the L2 ptes. This allows us to save the
 * precious kernel virtual needed for large memory systems.
 *
 * (See the plocal structure for pointers into KVPTEs).
 *
 * For the ccNUMA kernel:
 *
 * KL2PTES is a two dimensional array of all the CG-local kernel level 2 ptes
 * (the PTEs for the [KVAVBASE,KVAVEND) range of kernel virtual addresses). It
 * provides kernel virtual addresses for these PTEs so that they can be
 * read/written easily by the kernel when it is retrieving/changing page
 * mappings.
 * 
 * Each CG maps it own level2 page tables into KL2PTES by establishing
 * a second back pointer to the L1. Each CG maps the other CGs into
 * KL2PTES by actually allocating level 2 page tables. This allocation
 * of memory is required by the assymetric physical model.
 *
 * For simplicity, we just use the first available virtual address for
 * both KVPTE. KL2PTES immediately follows this.
 */

/* num. (virtual) bytes in KL2PTES */
#define KL2PTES_SIZE(num_cg)	(PTESPERCG * sizeof(pte64_t) * (num_cg))

/* Index into KL2PTES[] for a kernel virtual address */

#define kl2ptesndx(va)	pae_ptnum((vaddr_t)(va) - KVAVBASE)

#define	PDPTNDXSHFT	30
#define	PDPTNDX(addr)	0
#define	PDPTNDX64(addr)		((vaddr_t)(addr) >> PDPTNDXSHFT)

/*
 * pte_t *
 * kvtol1ptep(vaddr_t addr)
 *	Return pointer to level 1 pte for the given kernel virtual address.
 *
 * Calling/Exit State:
 *	None.
 */
#define	vtol1ptep(addr) \
	(kvtol2ptep((l.kvpte[PDPTNDX((addr))]) + (ptnum((addr)) << PNUMSHFT)))
#define kvtol1ptep	vtol1ptep

#define	vtol1ptep64(addr) \
	(kvtol2ptep64((l.kvpte64[PDPTNDX64((addr))]) + (pae_ptnum((addr)) << PNUMSHFT)))
#define kvtol1ptep64	vtol1ptep64

/*
 * vaddr_t
 * vtoptkv(vaddr_t addr)
 *	Convert virtual address to corressponding page table kernel virtual. 
 *
 * Calling/Exit State:
 *	This is only called from the hat layer to optimize usage of
 *	the kernel virtual (per-address space page tables).
 */
#define	vtoptkv(addr)	((l.kvpte[PDPTNDX((addr))]) + (ptnum((addr)) << PNUMSHFT))
#define	vtoptkv64(addr)	((l.kvpte64[PDPTNDX64((addr))]) + (pae_ptnum((addr)) << PNUMSHFT))


/*
 * pte_t *
 * kvtol2ptep(vaddr_t addr)
 *	Return pointer to level 2 pte for the given kernel virtual address.
 *
 * Description:
 *	If addr is in the range mapped by the per-engine level 2 page table,
 *	KVENG_L2PT, return a pointer into it.  Otherwise, return a pointer
 *	to level 2 page table on this CG into the global set of level 2 pages.
 */
/* We need not determine if the address is in a per-engine range */
#define	vtol2ptep(addr) \
	(&((pte_t *)l.kvpte[PDPTNDX((addr))])[pfnum((vaddr_t)(addr))])
#define kvtol2ptep(addr)	vtol2ptep(addr)

#define	vtol2ptep64(addr) \
	(&((pte64_t *)l.kvpte64[PDPTNDX64((addr))])[pae_pfndx((vaddr_t)(addr))])
#define kvtol2ptep64	vtol2ptep64

/*
 * one page of PTEs per CG in the KL2PTES
 *
 * XXX: This formula is derived from the pkl2ptes based code in mmu.c
 *	(which requires each CGs KL2PTES to begin with a new page table),
 *	not from the actual requirement to cover [KVAVBASE, KVAVEND).
 */
#define	PTESPERCG	(pae_ptbltob(1) / sizeof(pte64_t))

#define kvtol2ptep64_g(addr, cgnum) \
		(ASSERT(KADDR(addr)), \
		 ASSERT(!KADDR_PER_ENG(addr)), \
		 &((pte64_t *)KL2PTES)[pae_pfnum((vaddr_t)(addr) - \
			KVAVBASE) + cgnum * PTESPERCG])
#define	kvtol2ptepCG	kvtol2ptep64_g

/*
 * vaddr_t
 * pteptokv(ptep)
 *	It is used for kernel visible mapping for lazy shootdown
 *	evaluation.
 *
 * Calling/Exit State:
 *	None.
 */
#define	pteptokv(ptep) \
      (((ulong_t)((ptep) - (pte_t *)l.kvpte[PDPTNDX((vaddr_t)(ptep))]) << MMU_PAGESHIFT))

#define	pteptokv64(ptep) \
      (((ulong_t)((ptep) - (pte64_t *)l.kvpte64[PDPTNDX64((vaddr_t)(ptep))]) << MMU_PAGESHIFT))

#define kvtol2pteptep64_g(addr, cgnum) \
		kvtol2ptep64(kvtol2ptep64_g(addr, cgnum))
#define	kvtol2pteptepCG	kvtol2ptepte64_g

#define kvtol2pteptep64(addr) kvtol2pteptep64_g(addr, mycg)
#define	kvtol2pteptep	kvtol2pteptep64


/*
 * paddr_t
 * kvtophys(vaddr_t addr)
 *	Return the physical address equivalent of given kernel
 *	virtual address.
 *
 * Calling/Exit State:
 *	The caller must ensure that the given kernel virtual address
 *	is currently mapped.
 */

#define _KVTOPHYS(addr) \
		(ASSERT(PG_ISVALID(kvtol1ptep(addr))), \
		(kvtol1ptep(addr)->pgm.pg_ps ? \
		(paddr_t) ((kvtol1ptep(addr)->pg_pte & PAGE4MASK) + \
			((vaddr_t)(addr) & PAGE4OFFSET)) : \
		 (ASSERT(PG_ISVALID(kvtol2ptep(addr))), \
		(paddr_t) ((kvtol2ptep(addr)->pg_pte & MMU_PAGEMASK) + \
			   ((vaddr_t)(addr) & MMU_PAGEOFFSET)))))
#define kvtophys(addr)		_KVTOPHYS(addr)

#define _KVTOPHYS64(addr) \
		(ASSERT(PG_ISVALID(kvtol1ptep64(addr))), \
		(kvtol1ptep64(addr)->pgm.pg_ps ? \
		(paddr_t) ((kvtol1ptep64(addr)->pg_pte & PAGE4MASK) + \
			((vaddr_t)(addr) & PAGE4OFFSET)) : \
		 (ASSERT(PG_ISVALID(kvtol2ptep64(addr))), \
		(paddr_t) (((kvtol2ptep64(addr))->pg_pte & MMU_PAGEMASK) + \
			   ((vaddr_t)(addr) & MMU_PAGEOFFSET)))))
#define kvtophys64(addr)	_KVTOPHYS64(addr)

/*
 * struct page *
 * kvtopp(vaddr_t addr)
 *	Return the page struct for the physical page corresponding to
 *	a given kernel virtual address.
 *
 * Calling/Exit State:
 *	The caller must ensure that the given kernel virtual address
 *	is currently mapped to a page with a page struct.
 */

#define kvtopp32(addr)	 (ASSERT(!PG_ISPSE(kvtol1ptep(addr))), \
				pteptopp(kvtol2ptep(addr)))

#define kvtopp64(addr)	 (ASSERT(!PG_ISPSE(kvtol1ptep64(addr))), \
				pteptopp(kvtol2ptep64(addr)))

#define kvtopp(addr)	 ((PAE_ENABLED() ? kvtopp64((addr)) : kvtopp32((addr))))


/*
 * vaddr_t
 * STK_LOWADDR(proc_t *p)
 *	Return the lowest address which is part of the autogrow stack.
 *
 * Calling/Exit State:
 *	Caller must insure that p is stabilized and cannot be deallocated
 *	out from underneath us. This is usually guaranteed by the fact that
 *	this p is that of the caller. Of the fields referenced, p_stkbase
 *	never changes and p_stksize is stabilized by the AS lock for p->p_as,
 *	which the caller must hold.
 */

#define STK_LOWADDR(p)	((p)->p_stkbase - (p)->p_stksize)

/*
 * vaddr_t
 * STK_HIGHADDR(proc_t *p)
 *	Return the highest address which is part of the autogrow stack.
 *
 * Calling/Exit State:
 *	Caller must insure that p is stabilized and cannot be deallocated
 *	out from underneath us. This is usually guaranteed by the fact that
 *	this p is that of the caller. Although this implemenation doesn't
 *	require it, the caller must hold the AS lock for p->p_as, since
 *	for some implementations this may be the grow end of the stack.
 */

#define STK_HIGHADDR(p)	((p)->p_stkbase)

#define HI_TO_LOW	0
#define LO_TO_HI	1

#define STK_GROWTH_DIR	HI_TO_LOW

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_VMPARAM_H */

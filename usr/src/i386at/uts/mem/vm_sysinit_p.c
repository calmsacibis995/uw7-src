/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ident	"@(#)kern-i386at:mem/vm_sysinit_p.c	1.26.9.1"
#ident	"$Header$"

/*
 * VM - system initialization.
 */

#include <fs/memfs/memfs_mnode.h>
#include <mem/hatstatic.h>
#include <mem/page.h>
#include <mem/pmem.h>
#include <mem/rzbm.h>
#include <mem/seg_kmem.h>
#include <mem/tuneable.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/cg.h>
#include <svc/cpu.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/var.h>

extern vaddr_t ksym_base;
extern size_t mod_obj_size;
extern vaddr_t mod_obj_kern;
extern page_t *mod_obj_plist;
extern size_t resmgr_rdata_size;
extern size_t resmgr_size;
extern size_t license_rdata_size;
extern size_t license_size;
extern page_t *resmgr_obj_plist;
extern page_t *license_obj_plist;
extern size_t memfs_obj_npages;
extern size_t pages_total;

extern page_t	**memfs_getparm( struct pmem_extent *, vaddr_t *, size_t *);

STATIC page_t	**memfsroot_plistp;

extern struct pmem_extent *memNOTused[PMEM_TYPES][MAXNUMCG];

extern size_t totalmem;		
static uint_t	last_pfn;	/* need these for the page slice computation */

/*
 * STATIC void
 * pagepool_scan_kobject(void (*func)(), vaddr_t vaddr, size_t objsize, 
 *			 void *arg)
 *
 * 	Scan the kernel object, breaking it up into physically contiguous
 *	chunks.
 *
 * Calling/Exit State:
 *	``func'' will be called for each identified chunk.
 *	''vaddr'' is virtual address to start scan.
 *	''objsize'' is size in bytes of of area.
 *	``arg'' is an opaque argument which is passed onto chunk.
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 *
 * Description:
 *	In the restricted DMA case, the object is typically allocated
 *	from pages whose physically addresses are in reverse order from
 *	their virtual addresses. Therefore, this function can identify chunks
 *	that are either in low to high or in high to low physical address order.
 */
 
STATIC void
pagepool_scan_kobject(void (*func)(), vaddr_t objvaddr, size_t objsize,
		      void *arg)
{
	
	paddr_t paddr, lower_paddr, higher_paddr;
	size_t size;
	boolean_t obj_reversed;

	lower_paddr = (PAE_ENABLED() ?
		       kvtophys64(objvaddr)
		       : kvtophys(objvaddr));
	higher_paddr = lower_paddr + PAGESIZE;
	size = PAGESIZE;

	obj_reversed = B_FALSE;
	for (;;) {
		if (size >= objsize) {
			(*func)(lower_paddr, higher_paddr - lower_paddr,
				arg, obj_reversed);
			break;
		}
		objvaddr += PAGESIZE;
		size += PAGESIZE;
		paddr = (PAE_ENABLED() ?
			 kvtophys64(objvaddr) :
			 kvtophys(objvaddr));
		if (paddr == higher_paddr) {
			higher_paddr += PAGESIZE;
#ifndef NO_RDMA
			ASSERT(!obj_reversed);
		} else if (paddr + PAGESIZE == lower_paddr) {
			lower_paddr = paddr;
			ASSERT(size == 2 * PAGESIZE || obj_reversed);
			obj_reversed = B_TRUE;
#endif /* NO_RDMA */
		} else {
			(*func)(lower_paddr, higher_paddr - lower_paddr,
				arg, obj_reversed);
			lower_paddr = paddr;
			higher_paddr = lower_paddr + PAGESIZE;
		}
	}
}

/*
 * STATIC void
 * pagepool_memfsroot_chunk(paddr_t chunk_base, size_t chunk_size, void *arg,
 *	boolean_t obj_reversed)
 *	Initialize a pagepool chunk for the memfs meta data.
 *
 * Calling/Exit State:
 *	``arg'' is treated as a pointer to a (page_t **). *arg is treated
 *	as the current pointer into the pages table.  It is incremented
 *	following the chunk initialization.
 * 
 *	``chunk_base'' and ``chunk_size'' describe the physical chunk to be
 *	initialized.
 *
 *	``obj_reversed'' signifies that chunk had been copied (in reversed
 *	order) to high memory.
 *
 *	Called at system initialization time, when all activity is single
 *	threaded.
 *
 * Description:
 *	This function is called by pagepool_scan_kobject on behalf
 *	of pagepool_init.
 */

STATIC void
pagepool_memfsroot_chunk(paddr_t chunk_base, size_t chunk_size, void *arg,
			 boolean_t obj_reversed)
{
	page_t **ppp = arg;
	uint_t num_pp = btop(chunk_size);

	*ppp = page_init_chunk(*ppp, num_pp, chunk_base, &last_pfn, 
			       memfsroot_plistp,  obj_reversed);
}

#ifdef PSLICE_MINALIGN
/*
 * asm int
 * most_significant(ulong_t value)
 *
 * Calling/Exit State:
 *
 *  None.
 *
 * Description:
 * 
 * Returns the position of the most significant "1" bit.
 */
asm int
most_significant(ulong_t value)
{
%mem	value;

	bsrl value, %eax;
}

/* Doubles as a log base 2 function */
#define logbase2 most_significant

/*
 * asm int
 * least_significant(ulong_t value)
 *
 * Calling/Exit State:
 *
 *  None.
 *
 * Description:
 * 
 * Returns the position of the least significant "1" bit. This is also the
 * "alignment" of the value.
 */
asm int
least_significant(ulong_t value)
{
%mem	value;

	bsfl value, %eax;
}

/*
 * STATIC ulong_t
 * roundup2pow(ulong_t value)
 *
 * Calling/Exit State:
 *
 *  None.
 *
 * Description
 *
 *	Rounds the value up to the next power of two.
 */
STATIC ulong_t
roundup2pow(ulong_t value)
{
	if (value == 0)
		return 0;
	
	return (most_significant(value) == least_significant(value)) ?
		most_significant(value)
		: most_significant(value) + 1;
	
}

/*
 * int
 * min_alignment(paddr64_t start, paddr64_t end, uint_t *min_align_value)
 *
 * Calling/Exit State:
 *
 *  None.
 *
 * Description:
 * 
 * Updates the value of the min_hole_alignment after taking the hole from
 * start to end into account. Assumes that addresses are 64 bit unsigned
 * entities.
 * 
 */
STATIC void
min_alignment(paddr64_t start, paddr64_t end, uint_t *min_align_value)
{
	ulong_t 	lower_bits;
	ulong_t		upper_bits;
	int		align;
	
	lower_bits = start & 0xffffffff;
	if (lower_bits) {
		align = least_significant(lower_bits);
		/* common case */
		if (align < *min_align_value)
			*min_align_value = align; 
	} else if (start) {
		/* we have a true 64 bit address here */
		upper_bits = start >> 32;
		align = least_significant(upper_bits);
		align += 32;

		if (align < *min_align_value)
			*min_align_value = align; 
	} /* else start == 0, don't do anything */

	/* repeat the same for the end hole */
	lower_bits = end & 0xffffffff;
	if (lower_bits) {
		align = least_significant(lower_bits);
		/* common case */
		if (align < *min_align_value)
			*min_align_value = align; 
	} else if (end) {
		/* we have a true 64 bit address here */
		upper_bits = end >> 32;
		align = least_significant(upper_bits);
		align += 32;

		if (align < *min_align_value)
			*min_align_value = align; 
	} /* else end == 0, don't do anything */
}

/*
 * STATIC int
 * pslice_min_align(struct pmem_extent *mnotused)
 *	Calculate the minimum hole alignment of the free list
 *
 * Calling/Exit State:
 *	Called during system initialization when all processing
 *	is single threaded.
 */
STATIC int
pslice_min_align(struct pmem_extent *mnotused, uint_t *min_align)
{
	paddr64_t start_hole, end_hole;

	/* Do nothing if the list is null */
	if (mnotused == NULL)
		return 0;

	start_hole = mnotused->pm_extent + mnotused->pm_base;
	mnotused = mnotused->pm_next;
	while (mnotused != NULL) {
		end_hole = mnotused->pm_base;
		min_alignment(start_hole, end_hole, min_align);
		start_hole = end_hole + mnotused->pm_extent;
		mnotused = mnotused->pm_next;
	}
}
#endif

/*
 * STATIC uint_t
 * pslice_hole_overhead(paddr64_t start_hole, paddr64_t end_hole)
 *
 * 	Compute the dummy page structure overhead for this hole.
 */
STATIC uint_t
pslice_hole_overhead(paddr64_t start_hole, paddr64_t end_hole)
{
	uint_t 			overhead;
	uint_t			start_overhead, end_overhead;

	/*
	 * Make sure that the free lists are in the 
	 * increasing order of paddrs.
	 */
	ASSERT(end_hole >= start_hole);

	start_overhead = pslice_pages - (btop64(start_hole) % pslice_pages);
	/* overhead can't be greater than pslice_pages */
	start_overhead = start_overhead % pslice_pages;
	end_overhead = btop64(end_hole) % pslice_pages;

	if (PFN_TO_SLICE(btop64(start_hole)) ==
	    PFN_TO_SLICE(btop64(end_hole)))  {
		overhead = btop64(end_hole - start_hole);
	} else {
		overhead = start_overhead + end_overhead;
	}

	return overhead;
}

/*
 * STATIC int
 * pslice_overhead()
 *
 *	Compute the total number of dummy page structures required for this
 *	CG. 
 *
 *	*nchunkp will be updated with the number of chunks of paged mainstore
 *	memory.
 *
 * Calling/Exit State:
 *	Called during system initialization when all processing
 *	is single threaded.
 */
STATIC uint_t
pslice_overhead()
{
	uint_t 			overhead = 0, start_overhead;
	paddr64_t 		start_hole = 0, end_hole;
	pmem_extent_t 		*pmem_p;

	for(pmem_p = totalMem; pmem_p; pmem_p = pmem_p->pm_next) {

		/*
		 * Skip the chunks belonging to other CGs and non-paged
		 * chunks.
		 */
		if ((pmem_p->pm_cg != mycg)
		    || ((pmem_p->pm_use != PMEM_RESMGR)
			&& (pmem_p->pm_use != PMEM_LICENSE)
			&& (pmem_p->pm_use != PMEM_MEMFSROOT_META)
			&& (pmem_p->pm_use != PMEM_MEMFSROOT_FS)
			&& (pmem_p->pm_use != PMEM_KSYM)
			&& (pmem_p->pm_use != PMEM_FREE))) {
			continue;
		}

		end_hole = pmem_p->pm_base;
		overhead += pslice_hole_overhead(start_hole, end_hole);
		start_hole = end_hole + pmem_p->pm_extent;
	}

	/* Add the overhead for the last hole */
	start_overhead = SLICE_TO_PFN(PFN_TO_SLICER(btop64(start_hole))) 
				      - btop64(start_hole); 
	overhead += start_overhead;
	
	return overhead;
}

/*
 * uint_t
 * get_pagepool_size(uint_t *nchunkp)
 *
 *  Find total pages left in memory not unused by kernel text and data.
 *  The memory used by the symbol table is also included in the page pool.
 *
 * Calling/Exit State:
 *
 *  None.
 *
 * Description:
 *
 *   Loop through the memNOTused lists to find free chunks.
 *   Then loop through the memused array to find chunks used by
 *   symbol table.
 *   
 *   This only calculates an upper bound on the number of pages
 *   that the page pool will need to manage, since subsequent
 *   allocations could reduce this before we initialize the page pool.
 */
uint_t
get_pagepool_size(uint_t *nchunkp)
{
	uint_t 		totalpages = 0, pslice_dummy = 0;
	int		i;
	vaddr_t		addr;
	size_t		size;
	paddr64_t	slice_start;
	extern		uchar_t mem_width;
	pmem_extent_t	*pmem_p;
		
	segkmem_pse_pagepool_init();

	pslice_table_entries = MAX_SLICE_ENTRIES;
	pslice_page_shift = mem_width - MAX_SLICE_BITS - PAGESHIFT;

	/* Initialize the masks */
	pslice_pages = 1 << pslice_page_shift;
	pslice_page_mask = ~(pslice_pages - 1);
	
	/* No dummy pages for IDF free lists */
	pslice_dummy = pslice_overhead();

	/* compute the total number of page structures necessary */
	totalpages = 0;
	*nchunkp = 0;
	for(pmem_p = totalMem; pmem_p; pmem_p = pmem_p->pm_next) {

		/*
		 * Skip the chunks belonging to other CGs and non-paged
		 * chunks.
		 */
		if ((pmem_p->pm_cg != mycg)
		    || ((pmem_p->pm_use != PMEM_RESMGR)
			&& (pmem_p->pm_use != PMEM_LICENSE)
			&& (pmem_p->pm_use != PMEM_KSYM)
			&& (pmem_p->pm_use != PMEM_MEMFSROOT_META)
			&& (pmem_p->pm_use != PMEM_MEMFSROOT_FS)
			&& (pmem_p->pm_use != PMEM_FREE))) {
			continue;
		}

		totalpages += btop64(pmem_p->pm_extent);
		(*nchunkp)++;
	}

	/* Temporarily initialize the global_memsize structure. */
	global_memsize.tm_cg[mycg].tm_general = totalpages;
	
	/* epages - pages is not the number of pages anymore! */
	totalpages += pslice_dummy;

#ifdef DEBUG
	cmn_err(CE_NOTE,
		"pslice_pages = 0x%x\n"
		"pslice_table_entries = 0x%x\n"
		"pslice_dummy = 0x%x\n"
		"totalpages = 0x%x\n",
		pslice_pages,
		pslice_table_entries,
		pslice_dummy,
		totalpages);
#endif	
		
	return totalpages;
}

/*
 * void
 * pagepool_init(page_t *pp, int pgarraysize)
 *
 * 	Allocate page structures for the unused memory pages.
 *
 * Calling/Exit State:
 *
 * 	pp is the calloc'ed page array.
 *	pgarraysize is the size of the page array.
 *
 * Description:
 *
 * Determine which memory controller clicks are present,
 * group them into physically contiguous runs, and
 * allocate page structures for each such run.
 */
void
pagepool_init(page_t *pp, int pgarraysize)
{
	page_t *savpp = pp, *old_pp;
	int	i;
	uint_t	num_pp;
	vaddr_t	addr;
	size_t	size;
	pmem_extent_t 		*pmem_p;
 	uint_t end_slice_pfn;
	
#ifdef CCNUMA
	if (mycg == 0)
		last_pfn = 0;
	else {
		/*
		 * Look for the first PFN that has mycg as the cg and has a
		 * pageable type as the use field.
		 */
		for(pmem_p = totalMem; pmem_p; pmem_p = pmem_p->pm_next) {
			if ((pmem_p->pm_cg == mycg) &&
			    (pmem_p->pm_use == PMEM_FREE))
				break;
		}

		if (pmem_p)
			last_pfn = pmem_p->pm_base;
		else
			/* No paged memory on this cg ? */
			last_pfn = 0;
	}
#else
	last_pfn = 0;
#endif

	for(pmem_p = totalMem; pmem_p; pmem_p = pmem_p->pm_next) {

#ifdef DEBUG
		cmn_err(CE_NOTE, "pagepool_init: %Lx - %Lx (%d, %d)\n",
			pmem_p->pm_base, pmem_p->pm_base + pmem_p->pm_extent, 
			pmem_p->pm_use, pmem_p->pm_cg);
#endif
		/*
		 * Skip the chunks belonging to other CGs and non-paged
		 * chunks.
		 */
		if ((pmem_p->pm_cg != mycg)
		    || ((pmem_p->pm_use != PMEM_RESMGR)
			&& (pmem_p->pm_use != PMEM_LICENSE)
			&& (pmem_p->pm_use != PMEM_MEMFSROOT_META)
			&& (pmem_p->pm_use != PMEM_MEMFSROOT_FS)
			&& (pmem_p->pm_use != PMEM_KSYM)
			&& (pmem_p->pm_use != PMEM_FREE))) {
			continue;
		}

		num_pp = btop64(pmem_p->pm_extent);
		
		switch(pmem_p->pm_use) {
		case PMEM_FREE:
			pp = page_init_chunk(pp, num_pp,
					     pmem_p->pm_base,
					     &last_pfn, NULL, B_FALSE);
			break;
		case PMEM_KSYM:
			pp = page_init_chunk(pp, num_pp,
					     pmem_p->pm_base,
					     &last_pfn, &mod_obj_plist,
					     B_FALSE);
			break;
		case PMEM_RESMGR:
			pp = page_init_chunk(pp, num_pp, 
					     pmem_p->pm_base,
					     &last_pfn, &resmgr_obj_plist,
					     B_FALSE); 
			resmgr_size = pmem_p->pm_extent;
			resmgr_rdata_size = ptob(btopr(resmgr_size));
			break;
		case PMEM_LICENSE:
			pp = page_init_chunk(pp, num_pp, 
					     pmem_p->pm_base,
					     &last_pfn, &license_obj_plist,
					     B_FALSE); 
			license_size = pmem_p->pm_extent;
			license_rdata_size = ptob(btopr(license_size));
			break;
		case PMEM_MEMFSROOT_META:
			memfsroot_plistp = memfs_getparm(pmem_p, &addr, &size); 
			memfs_obj_npages += btop(size);
			pp = page_init_chunk(pp, num_pp, pmem_p->pm_base,
					     &last_pfn, memfsroot_plistp,
					     B_FALSE); 
			break;
		case PMEM_MEMFSROOT_FS:
			while (memfsroot_plistp =
			       memfs_getparm(pmem_p, &addr, &size)) {
				if (size > 0) {	
					memfs_obj_npages += btop(size);
					pagepool_scan_kobject(
						pagepool_memfsroot_chunk,
						addr, size, &pp);
				}
			}
			break;
		default:
			break;
		}

	}

	end_slice_pfn = SLICE_TO_PFN(PFN_TO_SLICER(last_pfn));
	
	/* Initialize any remaining pages as dummy */
	for (; last_pfn < end_slice_pfn; last_pfn++, pp++) {
		pp->p_pfn = PAGE_PFN_EMPTY;
		pp->p_cg = mycg;
	}

	epages = pp;
	
	/*
	 * Union the PMEM_NODMA list into the PMEM_DMA list.
	 */
#ifndef NO_RDMA
	pmem_list_union(&memNOTused[PMEM_DMA][mycg],
		        &memNOTused[PMEM_NODMA][mycg]);
	pmem_list_free(&memNOTused[PMEM_NODMA][mycg]);
#endif /* NO_RDMA */

	/*
	 * ASSERT:  We didn't run out of page structures
	 *          (and hence run into the page hash structures).
	 */
	if ((char *)pp > (char *)(savpp + pgarraysize)) {
		/*
		 *+ A problem was found during system initialization
		 *+ of virtual memory data structures.  This indicates
		 *+ a kernel software error.  Corrective action:  none.
		 */
		cmn_err(CE_PANIC, "Invalid page struct and page hash tables");
	}
}

/*
 * void
 * idf_pagepool_init(void)
 *	Initialize "identityfree" memory pagepool.
 *
 * Calling/Exit State:
 *	None.
 */
void
idf_pagepool_init(void)
{
	struct pmem_extent *mnup;

	idf_init();
	mnup = memNOTused[PMEM_IDF][mycg];
	while (mnup != NULL) {
		idf_init_chunk(btop64(mnup->pm_base),
			       btop64(mnup->pm_extent), mycg); 
		mnup = mnup->pm_next;
	}
}

uchar_t mem_width;

/*
 * void
 * calc_memory_width(void)
 *
 * Calling/Exit State:
 *	Called from kvm_init. Runs on each CG (sequentially).
 *	Currently, real action only takes place on CG0.
 *
 * Remarks:
 *	This is a temporary hack which must be fixed with the
 *	new boot and topology information parsing. 
 */
void
calc_memory_width(void)
{
	int i;
	pmem_type_t type;
	paddr_t base, extent;
	uchar_t cg_mem_width = 0;
	paddr64_t j, maxpaddr;

	if (mycg != BOOTCG)
		return;
	
	/*
	 * If we support DMA to device memory, then the
	 * memory width is given by the width which the
	 * machine can address (determined by PAE/non-PAE mode).
	 */
	if (tune.t_devnondma) {
		mem_width = using_pae ? 36 : 32;
		return;
	}

	/*
	 * Find highest physical address.
	 */
	maxpaddr = pmem_list_highest(&totalMem, PMEM_RESERVED);

	/*
	 * mem_width = log2(maxpaddr) rounded up to the next integer.
	 */
	j = 1;
	while (j < maxpaddr) {
		++cg_mem_width;
		j <<= 1;
	}

	if (cg_mem_width > mem_width)
		mem_width = cg_mem_width;
}

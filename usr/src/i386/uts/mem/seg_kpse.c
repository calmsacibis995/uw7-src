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

#ident	"@(#)kern-i386:mem/seg_kpse.c	1.2.5.1"
#ident	"$Header$"

#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/pmem.h>
#include <mem/pse.h>
#include <mem/pse_hat.h>
#include <mem/seg.h>
#include <mem/seg_pse.h>
#include <mem/seg_kmem.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/map.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * Kernel segment for PSE mappings.  Allows large physmap requests to
 * be mapped via PSE mappings.
 #
 * segkpse is a kernel segment driver for creating PSE mappings.
 * It is derived, in part, from seg_kmem.
 *
 * Allocation of virtual space in the segment is controlled by
 * the resource map kpse_rmap.  Each unit in the resource map
 * represents PSE_PAGESIZE bytes.  kpse_rbase and kpse_rsize
 * are the base and size of the resource map.  Thus:
 *
 *	base virtual address of segment = kpse_rbase * PSE_PAGESIZE
 *	size segment in bytes = kpse_rsize * PSE_PAGESIZE
 *
 * kpse_cookie is a dynamically allocated area of TLBScookies
 * used to do shootdowns when an address is recycled.
 */
STATIC struct map *kpse_rmap = NULL;
STATIC uint_t kpse_rbase = 0;
STATIC uint_t kpse_rsize = 0;
STATIC TLBScookie_t *kpse_cookie = NULL;

extern ulong_t segkmem_bytes;	/* Minimum size of KMA region */

/*
 * The following code is used to implement
 * the PSE-mapped pool used for KMA.
 */

/* 
 * We link the free chunks together using this header, which we keep
 * in the free chunk itself.  This assumes that the chunks are bigger
 * than this header.  segkmem_pse_freelist is the head of this list.
 * It is a singly-linked list, and is null terminated.
 */

typedef struct sp_header {
	struct sp_header 	*sp_next;	/* next free chunk */
	vaddr_t			sp_addr;	/* address of free chunk */
	uint_t			sp_size;	/* size of chunk in bytes */
} sph_t;

STATIC ppid_t		*segkmem_pse_ppid;	/* ppid of our first PSE page */
STATIC int		segkmem_pse_nfree;	/* number of PSE pages        */
STATIC vaddr_t		segkmem_pse_base;	/* base address of our range  */
STATIC vaddr_t		segkmem_pse_end;	/* end address of our range   */
uint_t			segkmem_pse_size;	/* size in bytes of our range */
STATIC sph_t		segkmem_pse_freelist;	/* head of freelist           */
STATIC lock_t		segkmem_pse_lock;	/* lock for our allocator     */

STATIC LKINFO_DECL(segkmem_pse_lkinfo, "VM:pse:segkmem_pse_lock", 0);

/*
 * We use the following structure to record 
 * statistics for the allocator.  All requests
 * come in in pagesize multiples, but we keep
 * the statistics in bytes.
 *
 * Protected by segkmem_pse_lock;
 */
struct {
	ulong_t	si_mem;		/* bytes in pool */
	ulong_t	si_alloc;	/* bytes currently allocated */
	ulong_t	si_max_alloc;	/* max bytes ever allocated */
	ulong_t	si_fail;	/* bytes requested but not allocated */
	ulong_t	si_toohard;	/* bytes requested with alignment or */
				/* boundary requests that we could not meet */
} segkmem_pse_info;

extern uint_t segkmem_pse_bytes;

/*
 * void
 * segkmem_pse_pagepool_init(void)
 *	Move PSE pages from boot time allocator into our KMA allocator.
 *
 * Calling/Exit State:
 *	Called during startup, while calloc is still available
 *	and startup code is still mapped in.
 *	Called after pse_pagepool_init has taken its PSE pages.
 *
 * Remarks:
 *	Calls pse_palloc, which is part of the pre-sysinit
 *	startup code, i.e., in mmu.c.  The PSE pages in its list
 *	are in order, but it hands us pages from the end of the list,
 *	i.e., in the wrong order.  We compensate for this here,
 *	so that when we map in the pages in segkmem_pse_create, 
 *	we ensure that the pages are physically contiguous.
 *
 *	The boot time allocator may not have been able
 * 	to reserve all the PSE pages we wanted. 
 *	It also might have allocated more than what
 *	segkmem will hold (segkmem_bytes). We ignore 
 *	the fact that segkmem_bytes may get adjusted upward
 *	by carve_kvspace. We want to have at least NONPSE_MIN
 *	bytes of non-PSE KMA (for DMA and aligned requests),
 *	and adjust segkmem_pse_bytes down if necessary.
 */
void
segkmem_pse_pagepool_init(void)
{
	size_t size = btopser(segkmem_pse_bytes);
	paddr64_t paddr;
	ppid_t ppid;
	cgnum_t xcg;

	/*
	 * if PSE is not supported, or if no physical memory was requested
	 *	don't create the pool
	 */
	if (!PSE_SUPPORTED() || (size == 0))
		return;

	ppid = 0;
	segkmem_pse_ppid = calloc(size * sizeof(ppid_t));

	/*
	 * Allocate PSE pages until either get as many as requested
	 *	(via segkmem_pse_bytes) or can't get any more.
	 */
	for (segkmem_pse_nfree = 0 ; segkmem_pse_nfree < size ;
						++segkmem_pse_nfree) {
		paddr = pse_palloc(&xcg, KERNEL_PHYS_LIMIT);
		if (paddr == PALLOC_FAIL)
			break;
		ASSERT((paddr & PSE_PAGEOFFSET) == 0);
		ppid = phystopfn64(paddr);
		ASSERT((ppid & PSE_NPGOFFSET) == 0);
		segkmem_pse_ppid[segkmem_pse_nfree] = ppid;
	}
	segkmem_pse_size = psetob(segkmem_pse_nfree);
}

/*
 * void
 * segkmem_pse_calloc(void)
 *	Perform any necessary callocs.
 *
 * Calling/Exit State:
 *	Called during kvm_init/kpg_calloc to allocate
 *	space if necessary.
 *	Currently unused.
 */
void
segkmem_pse_calloc(void)
{
}

/*
 * void
 * segkmem_pse_create(vaddr_t base, size_t size)
 *	Map the PSE pages into segkmem.
 *
 * Calling/Exit State:
 *	Called during kvm_init/segkmem_create to establish
 *	mappings to our PSE pages.
 *
 * Remarks:
 *	We map in segkmem_pse_nfree PSE pages 
 *	beginning at ppid segkmem_pse_ppid.
 */
void
segkmem_pse_create(vaddr_t base, size_t size)
{
	int i;

	ASSERT(segkmem_pse_size);
	ASSERT(segkmem_pse_size == size);
	ASSERT((size & PSE_PAGEOFFSET) == 0);
	ASSERT((base & PSE_PAGEOFFSET) == 0);
	ASSERT(segkmem_pse_nfree);
	ASSERT(segkmem_pse_ppid);

	segkmem_pse_base = base;
	segkmem_pse_end = base + size - 1;

	/*
	 * Map in PSE pages.
	 */
	for (i = 0; i < segkmem_pse_nfree; i++, base += PSE_PAGESIZE)
		HOP_PSE_STATPT_DEVLOAD(base, 1, segkmem_pse_ppid[i],
			PROT_READ|PROT_WRITE);
}


/*
 * void
 * segkmem_pse_init(vaddr_t base, size_t size)
 *	Initialize the PSE allocator.
 *
 * Calling/Exit State:
 *	Called during kvm_init/kpg_init to initialize
 *	our private allocator.
 *
 * Remarks:
 *	Free the entire PSE range to initialize our allocator.
 */
void
segkmem_pse_init(vaddr_t base, size_t size)
{

	ASSERT(segkmem_pse_size);
	ASSERT(segkmem_pse_size == size);
	ASSERT((size & PSE_PAGEOFFSET) == 0);
	ASSERT((base & PSE_PAGEOFFSET) == 0);
	ASSERT(segkmem_pse_base == base);
	ASSERT(segkmem_pse_end == base + size - 1);

	LOCK_INIT(&segkmem_pse_lock, VM_KMA_HIER, PLHI, &segkmem_pse_lkinfo,
		KM_NOSLEEP);

	segkmem_pse_info.si_mem = size;
	segkmem_pse_info.si_alloc = size;
	segkmem_pse_free((void *)base, btop(size));
}

/*
 * vaddr_t
 * segkmem_pse_alloc(ulong_t npages, uint_t prot, uint_t flag)
 *	This routine allocates "npages" contiguous pages of kernel heap,
 *	backed by PSE physical memory. The HAT translations are already
 *	set up.
 *
 * Calling/Exit State:
 *	We are called from kpg_alloc to try to allocate from the
 *	PSE memory pool first.  If we can't satisfy the request, we
 *	return NULL, and kpg_alloc will try to satisfy the request.
 *	We return the starting virtual address for the contiguous
 *	kernel address range, if we succeed.
 *
 * Remarks:
 *	This is a first-fit allocator.  The free list is headed
 *	by segkmem_pse_freelist.  Each chunk on the freelist
 *	contains a segkmem_pse_header structure that contains
 *	the address of this free chunk, the size of this chunk,
 *	and a pointer to the next chunk.
 *
 *	We never sleep. We ignore the page prot arg.
 *	We can't supply DMA-able memory so we let kpg_alloc
 *	satisfy the request if the KM_REQ_DMA or KM_DMA
 *	flag was given to kmem_*alloc.
 *
 *	No locks are required on entry or held on exit.
 */
/*ARGSUSED*/
vaddr_t
segkmem_pse_alloc(ulong_t npages, uint_t prot, uint_t flag)
{
	pl_t oldpri;
	sph_t *prev, *this, *new;
	vaddr_t addr;
	uint_t size;
	uint_t nbytes;

	ASSERT(npages != 0);
	if (segkmem_pse_size == 0 || !(flag & P_NODMA))
		return NULL ;

	nbytes = ptob(npages);
	oldpri = LOCK(&segkmem_pse_lock, PLHI);
	prev = &segkmem_pse_freelist;
	this = prev->sp_next;
	while (this != NULL) {
		ASSERT(this->sp_size >= PAGESIZE);
		ASSERT(this->sp_addr == (vaddr_t)this);
		if (this->sp_size >= nbytes) {
			addr = this->sp_addr;
			size = this->sp_size - nbytes;
			/*
			 * If this chunk is now empty, remove it.
			 * Otherwise we will return the first part
			 * of this chunk, and we need to add the
			 * remaining part of the chunk to the freelist.
			 */
			if (size == 0) 
				prev->sp_next = this->sp_next;
			else {
				vaddr_t naddr = addr + nbytes;

				new = (sph_t *)naddr;
				new->sp_addr = naddr;
				new->sp_size = size;
				new->sp_next = this->sp_next;
				prev->sp_next = new;
			}
			if ((segkmem_pse_info.si_alloc += nbytes) > segkmem_pse_info.si_max_alloc)
				segkmem_pse_info.si_max_alloc = segkmem_pse_info.si_alloc;
			UNLOCK(&segkmem_pse_lock, oldpri);
			return addr;
		}
		prev = this;
		this = this->sp_next;
	}

	segkmem_pse_info.si_fail += nbytes;
	UNLOCK(&segkmem_pse_lock, oldpri);
	return NULL;
}

/*
 * boolean_t
 * segkmem_pse_free(void *vaddr, ulong_t npages)
 * 	Free kernel memory mapped via PSE mappings
 *
 * Calling/Exit State:
 *	Called from kpg_free. We check to see if this
 *	range of virtual addresses came from our PSE pool.
 *	If it is one of ours we add it to our freelist,
 *	and return B_TRUE.  We return B_FALSE otherwise.
 *
 * Remarks:
 *	Called with vaddr page-aligned.
 *	No locks are required on entry or held on exit.
 */
boolean_t
segkmem_pse_free(void *vaddr, ulong_t npages)
{
	pl_t oldpri;
	sph_t *prev, *new, *next;
	vaddr_t addr;
	uint_t nbytes;

	ASSERT(npages != 0);
	ASSERT(((vaddr_t)vaddr & POFFMASK) == 0);

	if (!(((vaddr_t)vaddr <  segkmem_pse_end) &&
	      ((vaddr_t)vaddr >= segkmem_pse_base)))
		return B_FALSE;


	addr = (vaddr_t)vaddr;
	nbytes = ptob(npages);
	oldpri = LOCK(&segkmem_pse_lock, PLHI);
	segkmem_pse_info.si_alloc -= nbytes;

	/*
	 * Find the proper place in the list for addr.
	 * We maintain two pointers into the list, 
	 * prev and next, such that when we emerge 
	 * from the loop below, 
	 *		prev < addr <= next
	 * 1.  prev is the head of the list (segkmem_pse_freelist)
	 *	or prev is < addr, and
	 * 2.  next is NULL, or addr <= next.
	 * Note addr == next is an error condition that 
	 * will result in a panic.
	 *
	 * There are 4 normal situations to deal with below:
	 * 1. addr can be merged with both prev and next, or
	 * 2. addr can only be merged with prev, or
	 * 3. addr can only be merged with next, or
	 * 4. addr can not be merged with either.
	 *
	 * We also need to watch out for the cases
	 * where any of the range [addr, addr+nbytes) 
	 * is already free.  We panic in these cases.
	 */

	prev = &segkmem_pse_freelist;
	next = prev->sp_next;
	while (next != NULL && next->sp_addr < addr) {
		ASSERT(next->sp_size >= PAGESIZE);
		ASSERT(next->sp_addr == (vaddr_t)next);
		prev = next;
		next = next->sp_next;
	}

	ASSERT(prev == &segkmem_pse_freelist || prev->sp_addr < addr);
	ASSERT(next == NULL || addr <= next->sp_addr);

	if (prev != &segkmem_pse_freelist
	  && prev->sp_addr + prev->sp_size >= addr) {
		if (prev->sp_addr + prev->sp_size > addr) {
			/*
			 *+ The segkmem PSE free routine detected that it is
			 *+ freeing a chunk of memory that was already free.
			 *+ This indicates a kernel software problem.
			 *+ Corrective action: none.
			 */
			cmn_err(CE_PANIC, "segkmem_pse_free - freeing free chunk");
			/* NOTREACHED */
		}
		/*
		 * Case 2 (or possibly case 1 below).
		 * Merge addr with previous chunk.
		 */
		prev->sp_size += nbytes;
		/*
		 * See if we can merge with next segment too.
		 */
		if (next != NULL 
		  && prev->sp_addr + prev->sp_size >= next->sp_addr) {
			if (prev->sp_addr + prev->sp_size > next->sp_addr) {
				/*
				 *+ The segkmem PSE free routine detected that it is
				 *+ freeing a chunk of memory that was already free.
				 *+ This indicates a kernel software problem.
				 *+ Corrective action: none.
				 */
				cmn_err(CE_PANIC, "segkmem_pse_free - freeing free chunk");
				/* NOTREACHED */


			}
			/*
			 * Case 1 - merge with next segment too.
			 */
			prev->sp_size += next->sp_size;
			prev->sp_next = next->sp_next;
		}

	} else if (next == NULL || addr + nbytes < next->sp_addr) {
		/*
		 * Case 4 - addr cannot be combined with any chunk.
		 * Add in addr as a new chunk.
		 */
		new = (sph_t *)addr;
		new->sp_addr = addr;
		new->sp_size = nbytes;
		new->sp_next = next;
		prev->sp_next = new;
	} else {
		/*
		 * Case 3 - merge addr with next chunk
		 */
		if (addr + nbytes > next->sp_addr) {
			/*
			 *+ The segkmem PSE free routine detected that it is
			 *+ freeing a chunk of memory that was already free.
			 *+ This indicates a kernel software problem.
			 *+ Corrective action: none.
			 */
			cmn_err(CE_PANIC, "segkmem_pse_free - freeing free chunk");
			/* NOTREACHED */
		}
		ASSERT(addr + nbytes == next->sp_addr);
		new = (sph_t *)addr;
		new->sp_addr = addr;
		new->sp_size = next->sp_size + nbytes;
		new->sp_next = next->sp_next;
		prev->sp_next = new;
	}

	UNLOCK(&segkmem_pse_lock, oldpri);
	return B_TRUE;
}


/*
 * vaddr_t
 * segkmem_pse_alloc_physreq(ulong_t npages, const physreq_t *physreq, int flags)
 *	Try to satisfy a KMA request by obtaining PSE-mapped memory.
 *
 * Calling/Exit State:
 *
 *	We are called from kmem_alloc_oversize to try to allocate from
 *	the PSE memory pool first.  If we can't satisfy the request, we
 *	return NULL, and kmem_alloc_oversize will try to satisfy the request.
 *
 * Remarks:
 *	We can't supply DMA-able memory so we let kpg_alloc
 *	satisfy the request if the KM_REQ_DMA or KM_DMA
 *	flag was given to kmem_*alloc.
 *	We never sleep.  The allocated memory 
 *	will always be physically contiguous.
 *
 *	If physreq is non-NULL, and specifies an alignment or boundary
 *	greater than PAGESIZE, we just return NULL, and let
 *	kmem_alloc_oversize satisfy the request.
 *
 *	No locks are required on entry or held on exit.
 */
vaddr_t
segkmem_pse_alloc_physreq(ulong_t npages, const physreq_t *physreq, int flags)
{
	pl_t oldpri;

	ASSERT(npages != 0);
	if (!(flags & P_NODMA))
		return NULL;

	if (physreq && (physreq->phys_align > PAGESIZE
		     || physreq->phys_boundary > PAGESIZE)) {

		oldpri = LOCK(&segkmem_pse_lock, PLHI);
		segkmem_pse_info.si_toohard += ptob(npages);
		UNLOCK(&segkmem_pse_lock, oldpri);
		return NULL;
	}

	return segkmem_pse_alloc(npages, NULL, flags);
}

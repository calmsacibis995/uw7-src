#ifndef _MEM_SEG_KMEM_F_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_KMEM_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/seg_kmem_f.h	1.1.1.1"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * VM - Kernel Segment Driver Family specific code
 *
 * The following code provides an interface for KMA
 * to allocate memory backed by PSE (4 Meg) pages.
 * It does this by providing a layer between KMA and KPG.
 * The kpg allocator is unchanged, as some callers of
 * kpg_alloc (the hat, for example) need memory pages
 * that have associated page structures, and cannot
 * deal with PSE pages that do not have page structures.
 *
 * In order to enable the 4 Meg KMA feature, the PSE
 * driver must be enabled and the SEGKMEM_PSE_BYTES
 * tunable must be nonzero.  SEGKMEM_PSE_BYTES will be
 * rounded up to the next 4 Meg boundary.  SEGKMEM_PSE_BYTES
 * should be less than SEGKMEM_BYTES, since some KMA
 * requests (for DMA-accessible buffers, or buffers with 
 * special alignment or boundary restrictions) can only
 * be satisfied with non-PSE backed KMA.  The system will
 * automatically reduce SEGKMEM_PSE_BYTES so that at least
 * NONPSE_MIN (2 Meg) of non-PSE backed KMA is available.
 *
 * If 4 Meg KMA is enabled, segkmem_addr, the starting address
 * of the kpgseg segment, is rounded up to a 4 Meg boundary. 
 * The 4 Meg PSE pages will be mapped in at the beginning of 
 * the kpgseg range, and kpg/zbm will manage the remainder 
 * of the kpgseg range. 
 *
 * kmem_alloc will try to allocate from the PSE range first,
 * and only call kpg_alloc if we can't get memory from
 * the PSE range.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <mem/kmem.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <sys/kmem.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

extern uint_t segkmem_pse_size;
extern vaddr_t segkmem_pse_alloc_physreq(ulong_t, const physreq_t *, int);
extern vaddr_t segkmem_pse_alloc(ulong_t, uint_t, uint_t);
extern boolean_t segkmem_pse_free(void *, ulong_t);
extern void segkmem_pse_pagepool_init(void);
extern void segkmem_pse_calloc(void);
extern void segkmem_pse_create(vaddr_t, size_t);
extern void segkmem_pse_init(vaddr_t, size_t);

/*
 * If we are using PSE pages for KMA, then we need to
 * ensure that segkmem starts on a PSE_PAGESIZE boundary.
 */
#define NEXTVADDR_F(lo_kvaddr)						\
		((segkmem_pse_size) ?					\
			psetob(btopser(lo_kvaddr)) :			\
			ptob(btopr(lo_kvaddr)))

/*
 * If we are using PSE pages for KMA, then we don't need
 * to have a bitmap for the PSE part of the segkmem range.
 */
#define KPG_CALLOC_F(zbm, size, cellsize)				    \
	{								    \
		if (segkmem_pse_size)  {				    \
			ASSERT(segkmem_pse_size < size);		    \
			zbm_calloc(zbm, size - segkmem_pse_size, cellsize); \
			segkmem_pse_calloc(); 		   		    \
		} else							    \
			zbm_calloc(zbm, size, cellsize);		    \
	}

/*
 * If we are using PSE pages for KMA, then we don't need to
 * allocate L2 page tables for the PSE part of the segkmem range. 
 * Adjust the base and size to skip the PSE pages.
 */
#define KPG_CREATE_F(base, size)					\
	{								\
		if (segkmem_pse_size) {					\
			ASSERT(segkmem_pse_size < size);		\
			hat_statpt_alloc(base + segkmem_pse_size,	\
					 size - segkmem_pse_size);	\
			segkmem_pse_create(base, segkmem_pse_size);	\
		} else							\
			hat_statpt_alloc(base, size);			\
	}

/*
 * If we are using PSE pages for KMA, then we don't need
 * zbm to manage the PSE part of the segkmem range. 
 * Adjust the base and size to skip the PSE pages.
 */
#define KPG_INIT_F(zbm, base, size, func)				\
	{								\
		if (segkmem_pse_size) {					\
			ASSERT(segkmem_pse_size < size);		\
			zbm_init(zbm, base + segkmem_pse_size,		\
				      size - segkmem_pse_size, func);	\
			segkmem_pse_init(base, segkmem_pse_size);	\
		} else							\
			zbm_init(zbm, base, size, func);		\
	}

/*
 * If we are using PSE pages for KMA, then we try to
 * allocate from the PSE part of the segkmem range first.
 */
#define KPG_ALLOC_F(npages, prot, flag)					\
	{								\
		vaddr_t addr;						\
		addr = segkmem_pse_alloc(npages, prot, flag);		\
		if (addr != NULL)					\
			return((void *)addr);				\
	}

/*
 * If we are using PSE pages for KMA, then we try to
 * free to the PSE part of the segkmem range first.
 */
#define KPG_FREE_F(vaddr, npages)					\
	{								\
		ASSERT(((vaddr_t)vaddr & POFFMASK) == 0);		\
		if (segkmem_pse_free(vaddr, npages))			\
			return;						\
	}

/*
 * If we are using PSE pages for KMA, then we try to
 * allocate from the PSE part of the segkmem range first.
 */
#define KPG_ALLOC_PHYSREQ_F(npages, physreq, pflags)			   \
	{								   \
		vaddr_t addr;						   \
		addr = segkmem_pse_alloc_physreq(npages, physreq, pflags); \
		if (addr != NULL)					   \
			return addr;					   \
	}


#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_KMEM_F_H */

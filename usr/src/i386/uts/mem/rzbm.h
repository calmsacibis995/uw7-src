#ifndef _MEM_RZBM_H	/* wrapper symbol for kernel use */
#define _MEM_RZBM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/rzbm.h	1.1.1.3"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * VM - Zoned Bit Map (ZBM) Allocation for Identityless physical memory.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)
extern void rzbm_free(ppid_t, uint_t);
extern ppid_t rzbm_alloc(cgnum_t, uint_t);
extern void rzbm_unresv(uint_t);
extern int rzbm_resv(uint_t, uint_t);
extern int rzbm_init_chunk(ppid_t, uint_t, cgnum_t);
extern void rzbm_init();
#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_KMEMUSER)
/*
 * Statistical Information for a Zoned Bit Map
 */
struct rzbm_stats {
	ulong_t		zbs_npg_allocs;		/* # of rzbm_allocs */
	ulong_t		zbs_npg_free;		/* # of rzbm_free */
	ulong_t		zbs_nchunks;		/* # of chunks searched */
	ulong_t		zbs_npages;		/* # of pages requested */
	ulong_t		zbs_nallocated;		/* # of pages allocated */
	ulong_t		zbs_npse_allocated;	/* # of pse pages allocated */
	ulong_t		zbs_nfreed;		/* # of pages freed */
	ulong_t		zbs_npse_freed;		/* # of pse pages freed */
	ulong_t		zbs_nfail;		/* # of allocation failures */
	ulong_t		zbs_npse_fail;		/* # of pse allocation failures */
} rzbm_stats;


/*
 * Bit map to control allocation of a cell of zbm_cellsize pages
 * within the address range
 */
struct rzbm_cell {
	ushort_t	zbc_nfree;		/* free page count */
	uchar_t		zbc_cursor;		/* allocation scan pointer */
	/*
	 * list links
	 */
	struct rzbm_cell *zbc_flink;		/* forward link */
	struct rzbm_cell *zbc_blink;		/* backward link */
};

/*
 * The rzbm structure: locus of activity for a ZONED BIT MAP.
 */
typedef struct rzbm {
	struct rzbm		*zbm_flink;
	struct rzbm		*zbm_blink;

	/*
	 * pointers to calloced storage
	 */
	uint_t			*zbm_bitmap;	/* allocation map */
	struct rzbm_cell	*zbm_cell;	/* cells */

	/*
	 * the free pool
	 */
	struct rzbm_cell	*zbm_pool;	/* 4k page size free pool */
	struct rzbm_cell	*zbm_psepool;	/* 4M page size free pool */

	/*
	 * Miscellaneous data. An int is sufficient to cover 2^31 * PAGESIZE
	 * size of physical memory.
	 */

	cgnum_t			zbm_home;
	int			zbm_unavail;	/* unavail pages in lastcell */
	int			zbm_cellwidth;	/* words of bitmap per cell */
	int			zbm_ncell;	/* number of cells */
	int			zbm_mapsize;	/* # bits in the bit map */
	int			zbm_mapwidth;	/* # words in the bitmap */
	int			zbm_cwshft;	/* log2 zbm_cellwidth */
	int			zbm_cellmask;	/* for % by zbm_cellwidth */
	int 			zbm_cellsize;	/* number of pages per cell */

	/*
	 * Physical address range from which we are allocating.
	 */
	ppid_t			zbm_base;	/* base page id */
	uint_t			zbm_size;	/* size in unit of pages */
} rzbm_t;

/*
 * List of all the contiguous chunk of physical memory.
 */
typedef struct rzbm_list {
	/*
	 * Identityless memory pool.
	 */
	struct rzbm		*zbm_chunk;
	int			zbm_nchunk;

	int			zbm_maxfree;	/* maximnum free pages avail. */
	int			zbm_resv;	/* no. of pages reserved */

	/*
	 * statistics
	 */
	struct rzbm_stats	zbm_stats;

	/*
	 * rzbm_lock mutexes all non-constant fields in this structure
	 */
	lock_t			zbm_lock;	/* spin lock */
	sv_t			zbm_sv; 	/* for waiting */

} rzbm_list_t;

#define	idf_init()			rzbm_init()
#define	idf_init_chunk(base, sz, cg)	rzbm_init_chunk((base), (sz), (cg))
#define	idf_resv(npgs, flags)		rzbm_resv((npgs), (flags))
#define	idf_unresv(npgs)		rzbm_unresv((npgs))
#define	idf_page_get(cg, pagesz)	rzbm_alloc((cg), (pagesz))
#define	idf_page_free(pf, pagesz)	rzbm_free((pf), (pagesz))
#define	idf_deinit_chunk(base, sz, cg)	
#define	idf_memsize()			rzbm_memsize()

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_RZBM_H */

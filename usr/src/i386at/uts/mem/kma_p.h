#ifndef _MEM_KMA_P_H	/* wrapper symbol for kernel use */
#define _MEM_KMA_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:mem/kma_p.h	1.12.4.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KMEM_HIST) && defined(_KMEM_C)

#ifdef _KERNEL_HEADERS

#include <svc/clock.h>		/* PORTABILITY */
#include <util/param.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#endif /* _KMEM_HIST && _KMEM_C */


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Machine-dependent parameters for dynamic kernel
 * memory allocator.
 */

/*
 * Number of pages to allocate per chunk for each fixed buffer size.
 * The actual buffer sizes, and the number of such sizes is fixed,
 * due to dependencies in the generic code.
 *
 * The following numbers are found to be most suitable for 8Meg
 * configurations. Larger memory configurations are scaled proportionately
 * in kma_init().
 *
 */
#define NPAGE16		1
#define NPAGE32		1
#define NPAGE64		1
#define NPAGE128	1
#define NPAGE256	1
#define NPAGE512	1
#define NPAGE1024	1
#define NPAGE2048	1
#define NPAGE4096	2
#define NPAGE8192	4

#define KMA_POOL_ALLOC_MAX	4	/* in pages */
#define KMA_GRADIENT		2048	/* in pages */

/*
 * For cache-aligned KMA and other purposes, this is a default worst-case
 * cache line size to be used for this platform if no machine-specific
 * information can be obtained via the PSM or elsewhere.
 */
#define CACHE_LINE_DEFAULT	32	/* 32-byte L1/L2 cache line on x86 */

/*
 * MINBUFSZ and the related MINSZSHIFT establish a minimum granularity for
 * freelist buffer sizes.  This allows us to reduce the size of kmlistp[].
 * This size is equal to the smallest buffer size listed above, and can't be
 * changed without changing generic kma.c code.
 */
#define MINBUFSZ	16	/* minimum buffer size (fixed) */
#define MINSZSHIFT	4	/* log2(MINBUFSZ) */

/*
 * MAXBUFSZ is equal to the largest buffer size listed above, and can't be
 * changed without changing generic kma.c code.
 */
#define MAXBUFSZ	8192	/* maximum buffer size (fixed) */

#define MINVBUFSZ	32	/* minimum "variable" buffer size;
				   kmem_advise() ignores sizes <= MINVBUFSZ;
				   this also controls the minimum spacing
				   between buffer pool sizes */
#define MINVIDX		1	/* log2(MINVBUFSZ / MINBUFSZ) */

#ifdef _KMEM_HIST

/*
 * On the AT we just use lbolt for timing
 */
#define KMA_HIST_STAMP(t)	{ t = TICKS() * (1000000 / HZ); }

#endif /* _KMEM_HIST */

#define NFIXEDBUF	10	/* # of fixed buffer sizes above (fixed) */
#define NVARBUF		8	/* # of extra buffer sizes which can be
				   created by kmem_advise(). */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_KMA_P_H */

#ifndef _MEM_TUNEABLE_H	/* wrapper symbol for kernel use */
#define _MEM_TUNEABLE_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/tuneable.h	1.11.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

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

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct tune {
	int	t_gpgslo;	/* If freemem < t_getpgslow, then start	*/
				/* to steal pages from processes.	*/
	int	t_fsflushr;	/* The time interval in seconds  at     */
				/* which fsflush is run.		*/
				/* seconds.				*/
	int	t_minamem;	/* The minimum available memory to	*/
				/* maintain in order to avoid deadlock.	*/
				/* Reserves both real (resident) and	*/
				/* anonymous (swap) memory.  In pages.	*/
	int	t_kmem_resv;	/* The pool of memory to reserve for	*/
				/* kmem usage, in order to avoid	*/
				/* deadlock.  Reserves both real	*/
				/* (resident) and anonymous (swap)	*/
				/* memory.  In pages.			*/
	int	t_flckrec;	/* max number of active frlocks		*/
	int	t_dmalimit;	/* Last (exclusive) DMAable page number */
	int	t_dmabase;	/* First (inclusive) DMAable page num	*/
	int	t_devnondma;	/* Non-zero => some device memory is	*/
				/* non-DMAable				*/
	int	t_dma_percent;	/* Percent of RDMA_LARGE system 	*/
				/* devoted to the DMAable memory pool.	*/
} tune_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL
extern tune_t tune;
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_TUNEABLE_H */

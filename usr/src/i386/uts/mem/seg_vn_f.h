#ifndef _MEM_SEG_VN_F_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_VN_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/seg_vn_f.h	1.9.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Tuning options and constants for seg_vn.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Allow segs to expand in place. Really an optimization for stack growth.
 * See segvn_concat for more comment.
 */
#define ANON_SLOP	(16 * PAGESIZE)

/*
 * SEGVN_STORE_ORDER
 *	Allows segvn to take advantage of store ordering optimizations.
 *
 *	Set to one for families implementing store order access to memory.
 *	Otherwise, set to 0.
 */
#define	SEGVN_STORE_ORDER	1

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_VN_F_H */

#ifndef _IO_STRMDEP_H	/* wrapper symbol for kernel use */
#define _IO_STRMDEP_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/strmdep.h	1.9"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

/*
 * This file contains all machine-dependent declarations
 * in STREAMS.
 */

/*
 * save the address of the calling function to
 * enable tracking of who is allocating message blocks
 */

#define saveaddr(sp, val) {	char *vp; \
				vp = (char *) sp; \
				vp -= 4; \
				val = *(long *) vp; \
			   }

/*
 * macro to check pointer alignment
 * (true if alignment is sufficient for worst case)
 */

#define str_aligned(X)	(((uint)(X) & (sizeof(int) - 1)) == 0)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_STRMDEP_H */

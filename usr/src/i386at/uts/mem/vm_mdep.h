#ifndef _MEM_VM_MDEP_H	/* wrapper symbol for kernel use */
#define _MEM_VM_MDEP_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:mem/vm_mdep.h	1.17.3.1"
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

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * This file contains platform specific VM definitions.
 * Architecture family specific VM definitions are contained
 * in <mem/vmparam.h>.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Fixed kernel virtual addresses.
 *
 * These addresses are all mapped by a level 2 page table which
 * is replicated for each engine to allow some mappings to
 * be different on a per-engine basis.  Any other mappings 
 * that go in this page table must be statically defined for
 * the life of the system at boot.
 *
 * The mappings defined in this file are all of the
 * "statically defined" category.
 *
 * These addresses are all in the top 4 Meg of kernel virtual.
 * Each of these addresses must begin on a page boundary.
 *
 * Note:
 *
 * KVLAST_ARCH is the last fixed kernel virtual address
 * (working down from high memory) allocated by architecture
 * family specific but platform-independent code.  This symbol
 * can be used by platform-specific code to begin allocating
 * additional fixed kernel virtual addresses.
 */

#define NP_DISP_MONO	1
#define NP_DISP_COLOR	1

#define KVDISP_MONO	(KVLAST_ARCH - NP_DISP_MONO * MMU_PAGESIZE)
#define KVDISP_COLOR	(KVDISP_MONO - NP_DISP_COLOR * MMU_PAGESIZE)
#define	KVPAGE0		(KVDISP_COLOR - MMU_PAGESIZE)

#define KVLAST_PLAT	KVPAGE0

/*
 * Support for _Compat_phystokv(), the extended driver compatibility
 * phystokv/xphystokv function.
 */

/*
 * end of minimal phystokv range: 1 MB;
 * most drivers which used [x]phystokv limited use to the first 1MB.
 */
#define PHYSTOKV_PARTIAL_COMPAT_RANGE	0x100000

/* 
 * base of phystokv virtual
 *
 * Prior releases used 0xC0000000; some ddi4 drivers have inline versions
 * of phystokv, and thus require the identical virtual address.
 * ddi4 drivers are no longer supported.
 */
#define KVPHYSTOKV	kvphysbase	/* base of phystokv virtual */

extern vaddr_t kvphysbase;

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_VM_MDEP_H */

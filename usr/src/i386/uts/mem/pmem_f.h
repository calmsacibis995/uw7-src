#ifndef _MEM_FMEM_F_H	/* wrapper symbol for kernel use */
#define _MEM_FMEM_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/pmem_f.h	1.1"
#ident	"$Header$"
#ifdef _KERNEL_HEADERS

#include <mem/pmem_p.h>		/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/pmem_p.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#endif /* _MEM_FMEM_F_H */

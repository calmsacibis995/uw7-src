#ifndef	_MEM_PHYSMAP_F_H	/* wrapper symbol for kernel use */
#define	_MEM_PHYSMAP_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/physmap_f.h	1.3.1.1"
#ident	"$Header$"

/*
 * family-specific macros for physmap and physmap_free
 */

extern vaddr_t physmap0(paddr_t, size_t);
extern boolean_t physmap0_free(vaddr_t, size_t);

#define	PHYSMAP_F(physaddr, nbytes, flags)	\
		{ \
		caddr_t va; \
		if ((va = (caddr_t)physmap0(physaddr, nbytes)) != NULL) \
			return va; \
		}

#define	PHYSMAP_FREE_F(vaddr, nbytes, flags)	\
		{ \
		if (physmap0_free((vaddr_t)vaddr, nbytes) == B_TRUE) \
			return; \
		}

#endif	/* _MEM_PHYSMAP_F_H */

#ifndef _UTIL_MOD_MOD_OBJMD_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_OBJMD_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/mod/mod_objmd.h	1.3.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <proc/obj/elftypes.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined (_KMEMUSER)

#include <sys/elftypes.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define MOD_OBJ_MACHTYPE	EM_386
#define MOD_OBJ_VALRELOC(x)	((x) == SHT_REL)
#define MOD_OBJ_ERRRELOC(x)	((x) == SHT_RELA)
extern int mod_obj_relone(const struct modobj *, const Elf32_Rel *, 
				unsigned int, size_t, const Elf32_Shdr *);

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MOD_OBJMD_H */

#ident	"@(#)kern-i386:util/mod/mod_objmd.c	1.3.3.1"
#ident	"$Header$"

#include <proc/obj/elf.h>
#include <proc/obj/elf_386.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/mod/mod_obj.h>
#include <util/mod/mod_objmd.h>
#include <util/param.h>
#include <util/types.h>

/*
 * int mod_obj_relone(const struct module *mp, const Elf32_Rel *reltbl, 
 *	unsigned int nreloc, size_t relocsize, const Elf32_Shdr *shp)
 *
 *	Perform family specific symbol relocation for one relocation 
 *	table in the module.
 *
 * Calling/Exit State:
 *	Returns 0 if the symbol relocation is successful;
 *	otherwise returns -1.
 */
int
mod_obj_relone(const struct modobj *mp, const Elf32_Rel *reltbl, 
	unsigned int nreloc, size_t relocsize, const Elf32_Shdr *shp)
{

	Elf32_Rel *rend;
	unsigned int offset;
	unsigned int stndx;
	unsigned long value;

	/* LINTED pointer alignment */
	rend = (Elf32_Rel *)((char *)reltbl + nreloc * relocsize);

	while (reltbl < rend) {
		offset = reltbl->r_offset + shp->sh_addr;
		value = 0;
		switch (ELF32_R_TYPE(reltbl->r_info)) {

		case R_386_NONE:
		case R_386_BASEOFF:
			break;
		case R_386_PC32:
			value = - (int)offset;
		/* FALLTHROUGH */
		case R_386_32:
			stndx = ELF32_R_SYM(reltbl->r_info);
			if (stndx >= mp->md_nsyms) {
				/*
				 *+ Bad symbol table index in 
				 *+ relocation in the module.
				 */
				cmn_err(CE_WARN, 
	        "!MOD: Bad symbol table index %d in relocation in module %s.\n",
					stndx,mp->md_path);
				return (-1);
			}
			value += (stndx == STN_UNDEF ? 0 :
		       		 	((Elf32_Sym *)(mp->md_symspace + 
				 	mp->md_symentsize * stndx))->st_value);

			*(unsigned long *)offset = value + 
						   *(unsigned long *)offset;
			break;
		default:
			/*
			 *+ Illegal relocation type in the module.
			 */
			cmn_err(CE_WARN,
			     "!MOD: Illegal relocation type %d in module %s.\n",
				ELF32_R_TYPE(reltbl->r_info), 
				mp->md_path);
			return (-1);
		}
		/* LINTED pointer alignment */
		reltbl = (Elf32_Rel *)((char *)reltbl + relocsize);
	}

	return (0);
}

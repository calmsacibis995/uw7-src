#ifndef _ELF_F_H
#define _ELF_F_H

#ident	"@(#)stand:i386/boot/h/elf_f.h	1.1"
#ident	"$Header$"

/*
 * IA32 family-specific ELF object support
 */

#define MACHTYPE EM_386
#define IS_RELOC(shtype) ((shtype) == SHT_REL)

int relocate1(const Elf32_Rel *relp, const Elf32_Shdr *shp,
	      const Elf32_Shdr *symhdr);

#endif /* _ELF_F_H */

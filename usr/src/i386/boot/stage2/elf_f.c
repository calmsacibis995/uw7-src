#ident	"@(#)stand:i386/boot/stage2/elf_f.c	1.1"
#ident	"$Header$"

/*
 * ELF object file support -- iA32 family specific pieces.
 */

#include <boot.h>
#include <elf.h>
#include <sys/elf_386.h>
#include <link.h>

int
relocate1(const Elf32_Rel *relp, const Elf32_Shdr *shp,
	  const Elf32_Shdr *symhdr)
{
	Elf32_Sym *sym;
	unsigned stndx;
	ulong_t offset, value = 0;

	offset = relp->r_offset + shp->sh_addr;

	switch (ELF32_R_TYPE(relp->r_info)) {
	case R_386_PC32:
		value = -(int)offset;
#ifdef DEBUG3
printf("$");
#endif
		/* FALLTHROUGH */
	case R_386_32:
		stndx = ELF32_R_SYM(relp->r_info);
		ASSERT(stndx < symhdr->sh_size / symhdr->sh_entsize);
		if (stndx != STN_UNDEF) {
			sym = (Elf32_Sym *)(void *)
				(symhdr->sh_addr + stndx * symhdr->sh_entsize);
			value += sym->st_value;
#ifdef DEBUG3
printf("R%d", value);
#endif
		}
		*(ulong_t *)offset += value;
#ifdef DEBUG3
printf("=%d@%u  ", *(ulong_t *)offset, offset);
#endif
		break;
	default:
		ASSERT(ELF32_R_TYPE(relp->r_info) == R_386_NONE);
	}

	return 0;
}

Elf32_Half *_REL = 0; /* Patched by "mkldimg" to point to relocation table. */

void
relocate_self(ulong_t reloff)
{
	ulong_t fixaddr;
	Elf32_Half *relp;
	
	/*
	 * Find relocation table.
	 */
	relp = (Elf32_Half *)
		((char *)*(Elf32_Half **)((char *)&_REL + reloff) + reloff);

	/*
	 * Perform the relocation fixups needed to self-relocate.
	 * We only deal with R_386_RELATIVE-style relocations here,
	 * though they've been converted to a more compact form by "mkldimg".
	 * These should be sufficient for self-relocation, because of the
	 * way we were linked (ld -G -Bsymbolic).
	 */
	while (*relp != 0) {
		/*
		 * Find the address of the fixup location.
		 */
		fixaddr = (ulong_t)*relp++ + reloff;

		/*
		 * Add the relocation offset to the fixup location.
		 */
		/* THE FOLLOWING TAKES ADVANTAGE OF UNALIGNED ACCESSES */
		/* LINTED: pointer alignment */
		*(ulong_t *)fixaddr += reloff;	
	}
}

#ident	"@(#)nas:i386/target.h	1.1"
/*
* i386/target.h - i386 target machine specification
*
* Depends on:
*	"common/as.h"
*
* Only included from "common/objf.c"
*/

#define TARGET_MACHINE	EM_386
#define TARGET_DATA	ELFDATA2LSB
#define TARGET_EFLAGS	0

#define TARGET_RELNMSZ	4	/* use ".rel" */

#define TARGET_SYMREL_ALIGN	4	/* for .symtab & .rela sections */

#define TARGET_ADDR_BIT		32	/* number of bits in an Elf32_Addr */
#define TARGET_WORD_BIT		32	/* number of bits in an Elf32_Word */
#define TARGET_BIND_BIT		4	/* num. of bits allowed for binding */
#define TARGET_SYMT_BIT		4	/* num. bits allowed for symbol type */
#define TARGET_RELT_BIT		8	/* num. bits allowed for reloc type */
#define TARGET_RELS_BIT		24	/* num. bits allowed for rel. symndx */

#define TARGET_WORD_BIT		32	/* basic size of information */

	/*
	* So that the common code is independent of the "size"
	* of the target elf object file, the macros EMKF, EMKM,
	* and EMKT build the appropriate elf function, macro,
	* and type names, respectively.
	*/
#ifdef __STDC__
#  define EMKF(suf)	elf32_##suf
#  define EMKM(suf)	ELF32_##suf
#  define EMKT(suf)	Elf32_##suf
#else
#  define EMKF(suf)	elf32_/**/suf
#  define EMKM(suf)	ELF32_/**/suf
#  define EMKT(suf)	Elf32_/**/suf
#endif

#ident	"@(#)sgs-inc:common/sys/elf_386.h	1.5"

#ifndef _SYS_ELF_I386_H
#define _SYS_ELF_I386_H

#define R_386_NONE		0	/* relocation type */
#define R_386_32		1
#define R_386_PC32		2
#define R_386_GOT32		3
#define R_386_PLT32		4
#define R_386_COPY		5
#define R_386_GLOB_DAT		6
#define R_386_JMP_SLOT		7
#define R_386_RELATIVE		8
#define R_386_GOTOFF		9
#define R_386_GOTPC		10
#define R_386_BASEOFF		255

#define R_386_NUM		11

#define ELF_386_MAXPGSZ		0x1000	/* maximum page size */

#endif

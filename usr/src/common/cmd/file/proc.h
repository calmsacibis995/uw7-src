#ifndef _proc_h_
#define _proc_h_

#ident	"@(#)file:common/cmd/file/proc.h	1.1"

#include <sys/types.h>
#include <sys/elf.h>
#include <sys/procfs.h>

/* machine dependent ELF definitions */
#define Elf_Ehdr	Elf32_Ehdr
#define Elf_Phdr	Elf32_Phdr
#define Elf_Addr	Elf32_Addr
#define Elf_Dyn		Elf32_Dyn
#define Elf_Off		Elf32_Off
#define Elf_Half	Elf32_Half
#define MY_CLASS	ELFCLASS32
#define MY_DATA		ELFDATA2LSB
#define MY_MACHINE	EM_386

typedef struct {
	int		fd;
	Elf_Ehdr	*ehdr;
	Elf_Phdr	*phdr;
	pstatus_t	*pstatus;
	psinfo_t	*psinfo;
} Core_info;

extern Core_info  *core;

extern void *myalloc(size_t);

#endif

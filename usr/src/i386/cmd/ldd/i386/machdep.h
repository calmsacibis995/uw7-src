#ident	"@(#)ldd:i386/machdep.h	1.2"

/* 386 machine dependent definitions */

#define M_TYPE EM_386
#define M_DATA ELFDATA2LSB
#define M_CLASS ELFCLASS32
#define ELF_EHDR Elf32_Ehdr
#define ELF_PHDR Elf32_Phdr
#define elf_getehdr elf32_getehdr
#define elf_getphdr elf32_getphdr

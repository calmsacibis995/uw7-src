#ifndef _machdep_h_
#define _machdep_h_
#ident	"@(#)rtld:i386/machdep.h	1.11"

/* i386 machine dependent macros, constants and declarations */

/* object file macros */
#define ELF_TARGET_386	1
#define M_MACH		EM_386
#define M_CLASS		ELFCLASS32
#define M_DATA		ELFDATA2LSB
#define M_FLAGS		_rt_flags

/* page size */

#define PAGESIZE	_rt_syspagsz

/* segment boundary */

#ifdef	ELF_386_MAXPGSZ
#define SEGSIZE		ELF_386_MAXPGSZ
#else
#define SEGSIZE		0x1000	/* 4k */
#endif

/* macro to truncate to previous page boundary */

#define PTRUNC(X)	((X) & ~(PAGESIZE - 1))

/* macro to truncate to previous segment boundary */

#define STRUNC(X)	((X) & ~(SEGSIZE - 1))

/* macro to round to next page boundary */

#define PROUND(X)	(((X) + PAGESIZE - 1) & ~(PAGESIZE - 1))

/* macro to round to next segment boundary */

#define SROUND(X)	(((X) + SEGSIZE - 1) & ~(SEGSIZE - 1))

/* macro to round to next double word boundary */

#define DROUND(X)	(((X) + sizeof(double) - 1) & ~(sizeof(double) - 1))

/* macro to determine if relocation is a PC-relative type */

#define PCRELATIVE(T)	((T) == R_386_PC32)

/* default library search directory */

#define DEF_LIBDIR	"/usr/lib"
#define DEF_LIBDIRLEN	8

#endif

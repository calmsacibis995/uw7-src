#ifndef	_MACHINE_H
#define _MACHINE_H
#ident	"@(#)debugger:inc/i386/Machine.h	1.26"

/* NOTE: uses C style comments so it may be included by C code */

/* Machine dependent manifest constants. */
#include <limits.h>
#include <sys/signal.h>
#include <sys/elf.h>
#include "filehdr.h"

#define TEXT_BYTE_SWAP  

#define GENERIC_CHAR_SIGNED  	1
#define COFFMAGIC	 	I386MAGIC

/* valid signals range from 1 to NUMBER_OF_SIGS, inclusive */
#ifndef GEMINI_ON_OSR5
#define NUMBER_OF_SIGS	MAXSIG
#else
#define NUMBER_OF_SIGS	31
#endif

#if defined(PTRACE) || defined(OLD_PROC)
#define USES_SVR3_CORE		1
#else
#undef USES_SVR3_CORE
#endif

#if PTRACE
/* signal that informs us of process state change */
#define SIG_INFORM		SIGALRM 
#undef HAS_SYSCALL_TRACING
#undef PROVIDES_SEGMENT_MAP

#else
/* signal that informs us of process state change */
#define SIG_INFORM		SIGUSR1 
#define HAS_SYSCALL_TRACING	1
#define PROVIDES_SEGMENT_MAP	1
#endif

#ifdef GEMINI_ON_OSR5
#define HAS_PACCESS		1
#define NO_BIDIR_PIPES		1

#else
#undef HAS_PACCESS
#endif

#define BKPTSIZE		1
#define BKPTTEXT		"\314"


#define ERRBIT			0x1
#define EXEC_FAILED()		(goal2 == sg_run && getreg(REG_EAX) != 0)
#define FORK_FAILED()		(getreg(REG_EFLAGS) & ERRBIT)
#define LWP_CREATE_FAILED()	(getreg(REG_EFLAGS) & ERRBIT)
#define SYS_RETURN_VAL()	(getreg(REG_EAX))

#define MACHINE		EM_386

#define Elf_Ehdr	Elf32_Ehdr
#define Elf_Phdr	Elf32_Phdr
#define Elf_Shdr	Elf32_Shdr
#define Elf_Sym		Elf32_Sym
#define Elf_Dyn		Elf32_Dyn
#define Elf_Addr	Elf32_Addr
#define Elf_Half	Elf32_Half

/* constants for use in TYPE */
#define	CHAR_SIZE	1
#define	SHORT_SIZE	sizeof(short)
#define	INT_SIZE	sizeof(int)
#define	LONG_SIZE	sizeof(long)
#define	PTR_SIZE	sizeof(long)
#define	SFLOAT_SIZE	sizeof(float)
#define	LFLOAT_SIZE	sizeof(double)
/* 386/387 use 10 bytes for extended floats, even though they
 * are stored in memory as 12 bytes - the final bytes are ignored
 */
#define	XFLOAT_SIZE	sizeof(long double)
#define EXTENDED_SIZE	10
#if LONG_LONG
#define LONG_LONG_SIZE	sizeof(long long)
#endif

#define SIZEOF_TYPE	ft_sint
#define PTRDIFF_TYPE	ft_int
#define PTRDIFF		int

#define ROUND_TO_WORD(x)	(((x) + sizeof(int) - 1) & ~(sizeof(int)-1))
/* last valid syscall number - NOTE: OS release dependent */

#ifdef HAS_SYSCALL_TRACING
#include <sys/syscall.h>

#if defined(SYS_invlpg)	/* Gemini */
#define lastone SYS_invlpg
#elif defined(SYS_lwpcontinue)   /* ES/MP */
#define lastone SYS_lwpcontinue
#elif defined(SYS_getksym)	/* ES */
#define lastone SYS_getksym
#else	/* 4.0 */
#define lastone SYS_seteuid
#endif
#else
#define lastone	0
#endif

/* Number of digits in string representation of a number */
#define	MAX_INT_DIGITS	10
#define	MAX_LONG_DIGITS	10

/* Number of lines in the register pane in the Disassembly window */
#define REG_DISP_LINE	6

#ifdef SDE_SUPPORT
#define BROWSER_CLASS	ELFCLASS32
#define BROWSER_DATA	ELFDATA2LSB
#endif

/* macros to truncate to previous page or round to next page
 * assume pagesize is power of 2
 */
#define PTRUNC(X) ((X) & ~(pagesize - 1))
#define PROUND(X) (((X) + pagesize - 1) & ~(pagesize - 1))


#if defined(GEMINI_ON_OSR5)
#define ALTERNATE_ON_NO_NOTE
#else
#define ALTERNATE_ON_HAS_NOTE
#endif

#endif /* Machine.h */

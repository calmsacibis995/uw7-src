#ifndef _PROC_REGSET_H	/* wrapper symbol for kernel use */
#define _PROC_REGSET_H	/* subject to change without notice */

#ident	"@(#)kern-i386:proc/regset.h	1.10.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <svc/fp.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/fp.h>		/* REQUIRED */

#else

#include <sys/fp.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/* General register access (386) */

typedef	int	greg_t;

/* This header is included by an XPG4 V2 header file and
 * thus has namespace restrictions.
 */

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)
#define	NGREG	19
typedef	greg_t	gregset_t[NGREG];
#else
#define	_NGREG	19
typedef	greg_t	gregset_t[_NGREG];
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)*/

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)
#define R_SS	18
#define R_ESP	17
#define R_EFL	16
#define R_CS	15
#define R_EIP	14
#define R_EAX	11
#define R_ECX	10
#define R_EDX	9
#define R_EBX	8
#define R_EBP	6
#define R_ESI	5
#define R_EDI	4
#define R_DS	3
#define R_ES	2
#define R_FS	1
#define R_GS	0

/*
 * The following defines are only for compatibility. New code should use
 * the R_XX namespace defined above.
 */

#ifndef _KERNEL

#define SS	R_SS
#define UESP	R_ESP
#define EFL	R_EFL
#define CS	R_CS
#define EIP	R_EIP
#define ERR	13
#define TRAPNO	12
#define EAX	R_EAX
#define ECX	R_ECX
#define EDX	R_EDX
#define EBX	R_EBX
#define ESP	7
#define EBP	R_EBP
#define ESI	R_ESI
#define EDI	R_EDI
#define DS	R_DS
#define ES	R_ES
#define FS	R_FS
#define GS	R_GS

#endif /* !_KERNEL */

#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)*/

/*
 * Floating-point register access
 *  fpregset_t.fp_reg_set is 387 state.
 *  fpregset_t.f_wregs is Weitek state.
 */

/* Protect Namespace for XPG4 V2 */

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
typedef struct fpregset {
#else
typedef struct __fpregset {
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/
    union {
#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
	struct fp_chip_ste		/* fp extension state */
#else
	struct __fp_chip_ste		/* fp extension state */
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/
	{
#ifndef state
            int state[27];		/* 287/387 saved state */
#else
            int __state[27];		/* 287/387 saved state */
#endif
#ifndef status
            int status;			/* status word saved at exception */
#else
            int __status;			/* status word saved at exception */
#endif
#ifndef fpchip_state
	} fpchip_state;
#else
	} __fpchip_state;
#endif
#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)
	struct fpemul_state fp_emul_space;  /* for emulator(s) */
#else
	struct __fpemul_state __fp_emul_space;  /* for emulator(s) */
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)*/
#ifndef f_fpregs
	int f_fpregs[62];		/* union of the above */
#else
	int __f_fpregs[62];		/* union of the above */
#endif
#ifndef fp_reg_set
    } fp_reg_set;
#else
    } __fp_reg_set;
#endif
#ifndef f_wregs
    long    f_wregs[33];		/* saved weitek state */
#else
    long    __f_wregs[33];		/* saved weitek state */
#endif

} fpregset_t;

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)
/* Hardware debug register access (386) */

#define NDEBUGREG	8

typedef struct dbregset {
	unsigned	debugreg[NDEBUGREG];
} dbregset_t;

typedef struct regs {
	union {
		unsigned int eax;
		struct {
			unsigned short ax;
		} word;
		struct {
			unsigned char al;
			unsigned char ah;
		} byte;
	} eax;

	union {
		unsigned int ebx;
		struct {
			unsigned short bx;
		} word;
		struct {
			unsigned char bl;
			unsigned char bh;
		} byte;
	} ebx;

	union {
		unsigned int ecx;
		struct {
			unsigned short cx;
		} word;
		struct {
			unsigned char cl;
			unsigned char ch;
		} byte;
	} ecx;

	union {
		unsigned int edx;
		struct {
			unsigned short dx;
		} word;
		struct {
			unsigned char dl;
			unsigned char dh;
		} byte;
	} edx;

	union {
		unsigned int edi;
		struct {
			unsigned short di;
		} word;
	} edi;

	union {
		unsigned int esi;
		struct {
			unsigned short si;
		} word;
	} esi;

	unsigned int eflags;

} regs;
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >= 1)*/

#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_REGSET_H */

#ifndef _PROC_REGSET_H	/* wrapper symbol for kernel use */
#define _PROC_REGSET_H	/* subject to change without notice */

/* our own copy to get around the problems with C++ name spaces
 * and to represent the general and floating point regs
 * in a convenient form 
 */

#ident	"@(#)debugger:inc/i386/sys/regset.h	1.5"

#include "i_87fp.h"

/* General register access (386) */

typedef	int	greg_t;
#define	NGREG	19

typedef struct {
	greg_t greg[NGREG];
} gregset_t;

#ifdef OLD_PROC
#include <sys/reg.h>

#else

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

#endif

/*
 * Floating-point register access
 *  fpregset_t.fp_reg_set is 387 state.
 *  fpregset_t.f_wregs is Weitek state.
 */
typedef struct fpregset {
    union {
	struct 	/* fp extension state - changed for C++*/
	{
            int state[27];		/* 287/387 saved state */
            int status;			/* status word saved at exception */
	} fpchip_state;
	struct fpstate_t fstate;	/* debugger's representation */
	struct /* for emulator(s) - changed for C++ */
	{
            char    fp_emul[246];
            char    fp_epad[2];
	} fp_emul_space;
	int f_fpregs[62];		/* union of the above */
    } fp_reg_set;
#ifndef GEMINI_ON_OSR5
    long    f_wregs[33];		/* saved weitek state */
#endif
} fpregset_t;

#define fp_state	fp_reg_set.fstate
/* Hardware debug register access (386) */

#define NDEBUGREG	8

typedef struct dbregset {
	unsigned	debugreg[NDEBUGREG];
} dbregset_t;

#endif /* _PROC_REGSET_H */

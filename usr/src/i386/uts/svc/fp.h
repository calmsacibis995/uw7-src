#ifndef _SVC_FP_H	/* wrapper symbol for kernel use */
#define _SVC_FP_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/fp.h	1.11.3.1"
#ident	"$Header$"

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(__cplusplus)
extern "C" {
#endif

#if !(defined (_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
/*
 * 80287/80387 floating point processor definitions
 */

/*
 * masks for 80387 control word
 */
#define FPINV   0x00000001      /* invalid operation                    */
#define FPDNO   0x00000002      /* denormalized operand                 */
#define FPZDIV  0x00000004      /* zero divide                          */
#define FPOVR   0x00000008      /* overflow                             */
#define FPUNR   0x00000010      /* underflow                            */
#define FPPRE   0x00000020      /* precision                            */
#define FPPC    0x00000300      /* precision control                    */
#define FPRC    0x00000C00      /* rounding control                     */
#define FPIC    0x00001000      /* infinity control                     */
#define WFPDE   0x00000080      /* data chain exception                 */

/*
 * precision, rounding, and infinity options in control word
 */
#define FPSIG24 0x00000000      /* 24-bit significand precision (short) */
#define FPSIG53 0x00000200      /* 53-bit significand precision (long)  */
#define FPSIG64 0x00000300      /* 64-bit significand precision (temp)  */
#define FPRTN   0x00000000      /* round to nearest or even             */
#define FPRD    0x00000400      /* round down                           */
#define FPRU    0x00000800      /* round up                             */
#define FPCHOP  0x00000C00      /* chop (truncate toward zero)          */
#define FPP     0x00000000      /* projective infinity                  */
#define FPA     0x00001000      /* affine infinity                      */
#define WFPB17  0x00020000      /* bit 17                               */
#define WFPB24  0x01000000      /* bit 24                               */

/*
 * masks for 80387 status word
 */
#define FPS_ES	0x00000080      /* error summary bit                    */

/*
 * values that go into fp_kind
 */
#define FP_NO   0       /* no fp chip, no emulator (no fp support)      */
#define FP_SW   1       /* no fp chip, using software emulator          */
#define FP_HW   2       /* chip present bit                             */
#define FP_287  2       /* 80287 chip present                           */
#define FP_387  3       /* 80387 chip present                           */

#endif /* !(defined (_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/

#if defined(_KERNEL)

extern int fp_kind;	/* kind of fp support */

#endif /* _KERNEL */

/* Per-context floating-point emulator state */

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >=1) 
struct fpemul_state {
	char		fp_emul[246];
	char		fp_epad[2];
};
#else
struct __fpemul_state {
	char		__fp_emul[246];
	char		__fp_epad[2];
};
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED -0 >=1) */

#if !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)
/* State needed to allow FP emulator to be restartable (see fpeclean()) */
struct fpemul_restart {
	struct fpemul_state fr_fpestate;	/* Saved FPU emulator state */
	unsigned	fr_esp;			/* Saved user ESP */
	unsigned	fr_eip;			/* Saved user EIP */
};

/* Per-engine info shared between kernel and FP emulator */
struct fpemul_kstate {
	struct fpemul_state	fpe_state;	/* Current state */
	struct fpemul_restart	fpe_restart;	/* Saved prior state */
};
#endif /* !(defined(_XOPEN_SOURCE) && _XOPEN_SOURCE_EXTENDED - 0 >= 1)*/

#if defined(_KERNEL)

extern boolean_t using_fpu;

/* Floating-point emulator support functions */
extern void fpesetvec(void);
extern boolean_t fpeclean(void);

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif
#endif /* _SVC_FP_H */

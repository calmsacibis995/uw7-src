#ifndef FPEMU_H
#define FPEMU_H

#ident	"@(#)fpemu:common/fpemu.h	1.6"

/* Declarations for floating point emulation package. */

#include "fpemu_md.h"		/* pick up machine-dependent overrides */
/* Emulated FP double-extended values are kept in an array
** of unsigned chars.
*/

/* These definitions characterize the external representation. */
#ifndef	X_REP_LEN
#define	X_REP_LEN	10		/* representation takes 10 bytes */
#endif

#ifdef	RTOLBYTES
#  ifndef X_EXP_LOW
#  define X_EXP_LOW	8		/* byte # for low-order exponent */
#  endif
#  ifndef X_EXP_HIGH
#  define X_EXP_HIGH	(X_EXP_LOW+1)	/* byte # for high-order exponent */
#  endif
#  ifndef X_SIG_LOW
#  define X_SIG_LOW	0		/* byte # of low-order significand */
#  endif
#else	/* not RTOLBYTES */
#  ifndef X_EXP_LOW
#  define X_EXP_LOW	1		/* byte # for low-order exponent */
#  endif
#  ifndef X_EXP_HIGH
#  define X_EXP_HIGH	0		/* byte # for high-order exponent */
#  endif
#  ifndef X_SIG_LOW
#  define X_SIG_LOW	9		/* byte # of low-order significand */
#  endif
#endif	/* def RTOLBYTES */

#ifndef	D_REP_LEN
#define	D_REP_LEN	8		/* representation takes 8 bytes */
#endif

#ifdef	RTOLBYTES
#  ifndef D_EXP_LOW
#  define D_EXP_LOW	6		/* byte # for low-order exponent */
#  endif
#  ifndef D_EXP_HIGH
#  define D_EXP_HIGH	(D_EXP_LOW+1)	/* byte # for high-order exponent */
#  endif
#  ifndef D_SIG_LOW
#  define D_SIG_LOW	0		/* byte # of low-order significand */
#  endif
#else	/* not RTOLBYTES */
#  ifndef D_EXP_LOW
#  define D_EXP_LOW	1		/* byte # for low-order exponent */
#  endif
#  ifndef D_EXP_HIGH
#  define D_EXP_HIGH	0		/* byte # for high-order exponent */
#  endif
#  ifndef D_SIG_LOW
#  define D_SIG_LOW	7		/* byte # of low-order significand */
#  endif
#endif	/* def RTOLBYTES */

#ifndef	F_REP_LEN
#define	F_REP_LEN	4		/* representation takes 4 bytes */
#endif

#ifdef	RTOLBYTES
#  ifndef F_EXP_BYTE
#  define F_EXP_BYTE	3		/* byte # for exponent */
#  endif
#  ifndef F_SIG_LOW
#  define F_SIG_LOW	0		/* byte # of low-order significand */
#  endif
#else	/* not RTOLBYTES */
#  ifndef F_EXP_BYTE
#  define F_EXP_BYTE	0		/* byte # for exponent */
#  endif
#  ifndef F_SIG_LOW
#  define F_SIG_LOW	3		/* byte # of low-order significand */
#  endif
#endif	/* def RTOLBYTES */

#if defined(__cplusplus) || defined(c_plusplus)
struct fpemu_x_t {
    unsigned char ary[X_REP_LEN];
};

/* Emulated "float" */
struct fpemu_f_t {
    unsigned char ary[F_REP_LEN];
};

/* Emulated "double" */
struct fpemu_d_t {
    unsigned char ary[D_REP_LEN];
} ;

#else
typedef struct _fpemu_x_t {
    unsigned char ary[X_REP_LEN];
} fpemu_x_t;

/* Emulated "float" */
typedef struct _fpemu_f_t {
    unsigned char ary[F_REP_LEN];
} fpemu_f_t;

/* Emulated "double" */
typedef struct _fpemu_d_t {
    unsigned char ary[D_REP_LEN];
} fpemu_d_t;
#endif

/* Target machine bounds for integers. */
#ifndef	T_LONG_MAX
#define T_LONG_MAX	((long) ((~(unsigned long) 0) >> 1))
#endif
#ifndef	T_LONG_MIN
#define T_LONG_MIN	(-T_LONG_MAX-1)
#endif
#ifndef	T_ULONG_MAX
#define	T_ULONG_MAX	(~((unsigned long) 0))
#endif

/* Return values for fpemu_compare().
** The classic <0,0,>0 results are assumed internally
** (after eliminating the UNORDERED case).
*/
#define FPEMU_CMP_UNORDERED	(-2)
#define FPEMU_CMP_LESSTHAN	(-1)
#define FPEMU_CMP_EQUALTO	0
#define FPEMU_CMP_GREATERTHAN	1

#if defined(__STDC__) || defined(__cpluplus) || defined(c_plusplus)
#ifdef __cplusplus
extern "C" {
#endif
fpemu_x_t fpemu_nop(fpemu_x_t);
fpemu_x_t fpemu_add(fpemu_x_t,fpemu_x_t);
fpemu_x_t fpemu_mul(fpemu_x_t,fpemu_x_t);
fpemu_x_t fpemu_div(fpemu_x_t,fpemu_x_t);
fpemu_x_t fpemu_neg(fpemu_x_t);
fpemu_x_t fpemu_xtofp(fpemu_x_t);
fpemu_x_t fpemu_xtodp(fpemu_x_t);
long   fpemu_xtol(fpemu_x_t);
unsigned long fpemu_xtoul(fpemu_x_t);
fpemu_x_t fpemu_ltox(long);
fpemu_x_t fpemu_ultox(unsigned long);
fpemu_x_t fpemu_atox(const char *);
int fpemu_iszero(fpemu_x_t);
int fpemu_ispow2(fpemu_x_t);
int fpemu_compare(fpemu_x_t,fpemu_x_t);
fpemu_f_t fpemu_xtof(fpemu_x_t);
fpemu_d_t fpemu_xtod(fpemu_x_t);
char * fpemu_xtoa(fpemu_x_t);
char * fpemu_xtoh(fpemu_x_t);
#ifdef __cplusplus
}
#endif
#else
fpemu_x_t fpemu_nop();
fpemu_x_t fpemu_add();
fpemu_x_t fpemu_mul();
fpemu_x_t fpemu_div();
fpemu_x_t fpemu_neg();
fpemu_x_t fpemu_xtofp();
fpemu_x_t fpemu_xtodp();
long   fpemu_xtol();
unsigned long fpemu_xtoul();
fpemu_x_t fpemu_ltox();
fpemu_x_t fpemu_ultox();
fpemu_x_t fpemu_atox();
int fpemu_iszero();
int fpemu_ispow2();
int fpemu_compare();
fpemu_f_t fpemu_xtof();
fpemu_d_t fpemu_xtod();
char * fpemu_xtoa();
char * fpemu_xtoh();
#endif

#endif

#ident	"@(#)libc-i386:gen/fpemu.h	1.3"
/* from @(#)fpemu:common/fpemu.h	1.1" 		*/
/* fpemu.h */

/* Declarations for floating point emulation package. 	*/

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
#  define X_SIG_LOW	2		/* byte # of low-order significand */
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

typedef union _fp_x_t {
    unsigned char ary[X_REP_LEN];
    long double ld;
} fp_x_t;

/* Emulated "float" */
typedef struct _fp_f_t {
    unsigned char ary[F_REP_LEN];
} fp_f_t;

/* Emulated "double" */
typedef struct _fp_d_t {
    unsigned char ary[D_REP_LEN];
} fp_d_t;

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

#ifdef __STDC__
fp_x_t fp_nop(fp_x_t);
fp_x_t fp_add(fp_x_t,fp_x_t);
fp_x_t fp_mul(fp_x_t,fp_x_t);
fp_x_t fp_div(fp_x_t,fp_x_t);
fp_x_t fp_neg(fp_x_t);
fp_x_t fp_xtofp(fp_x_t);
fp_x_t fp_xtodp(fp_x_t);
long   fp_xtol(fp_x_t);
unsigned long fp_xtoul(fp_x_t);
fp_x_t fp_ltox(long);
fp_x_t fp_ultox(unsigned long);
fp_x_t fp_atox(const char *);
int fp_iszero(fp_x_t);
int fp_compare(fp_x_t,fp_x_t);
fp_f_t fp_xtof(fp_x_t);
fp_d_t fp_xtod(fp_x_t);
char * fp_xtoa(fp_x_t);
#else
fp_x_t fp_nop();
fp_x_t fp_add();
fp_x_t fp_mul();
fp_x_t fp_div();
fp_x_t fp_neg();
fp_x_t fp_xtofp();
fp_x_t fp_xtodp();
long   fp_xtol();
unsigned long fp_xtoul();
fp_x_t fp_ltox();
fp_x_t fp_ultox();
fp_x_t fp_atox();
int fp_iszero();
int fp_compare();
fp_f_t fp_xtof();
fp_d_t fp_xtod();
char * fp_xtoa();
#endif

#ident	"@(#)fpemu:common/fpemu.c	1.10"
/* fpemu.c */

/* Package to emulate IEEE floating point operations.
**
** All objects passed into and out of the package are in some
** target machine format with target machine byte order, and
** operands are double-extended precision.
**
** Within the package, however, all operations are done in
** an internal format.
**
** Assumptions about the host machine:
**	
**	- shorts, ints are at least 16 bits; unsigned longs are
**		at least 32 bits
**
** Assumptions about the target machine:
**	- one of two byte orders applies:
**	  little-endian (RTOLBYTES defined):
**
**     79 78	      63				   0
**	+-+-----------+-------------------------------------+
**	| |	      |					    |
**	|S| Exponent  |		Significand		    |
**	| |	      |					    |
**	+-+-----------+-------------------------------------+
**
**	  big-endian (RTOLBYTES not defined):
**
**      0  1	      16				  79
**	+-+-----------+-------------------------------------+
**	| |	      |					    |
**	|S| Exponent  |		Significand		    |
**	| |	      |					    |
**	+-+-----------+-------------------------------------+
**
*/


/* Machine-dependent definitions can alter fp.h definitions. */
#include "fpemu.h"
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>

/* Define internal form for operands. */

typedef struct {
    int i_sign;				/* 1 - plus, -1 - minus */
    int i_exp;				/* biased exponent */
    unsigned char i_flag;		/* flags (see below) */
    unsigned long i_mag[5];		/* magnitude bits, including enough
					** room for guard bits
					** i_mag[0] contains special (low-order)
					** bits; i_mag[1] contains the low-order
					** magnitude bits
					*/
} fpemu_i_t;

/* Macro to define ordinary positive floating point number in internal form.
** m4-m0 are the significant (mantissa) bits in high-to-low order,
** including the guard word.
*/
#define FP_INUM(exp, m4, m3, m2, m1, m0) \
	{ SIGN_PLUS, exp, IFL_NONE, m0, m1, m2, m3, m4 }


/* Flag values, describe the operand. */

#define IFL_NONE	0		/* Nothing special */
#define	IFL_NaN		1		/* Encodes NaN */
#define IFL_INF		2		/* Encodes infinity */
#define	IFL_ZERO	3		/* Encodes zero */
#define	IFL_DENORM	4		/* Denormalized number */

/* Assumptions are made about these values! */
#define	SIGN_PLUS	(1)		/* Plus i_sign value */
#define	SIGN_MINUS	(-1)		/* Minus i_sign value */

#define	BITS_PER_MAG	16		/* Each internal magnitude word holds
					** 16 bits
					*/
#define	RB_HIBIT	(((unsigned long) 1) << (BITS_PER_MAG-1))
#define	RB_HIPART	((~((unsigned long) 0)) << BITS_PER_MAG)
#define	RB_LOPART	(~RB_HIPART)
#define	RB_CARRY	(RB_HIBIT << 1) /* Carry in high-order signficand */

/* Bits in guard word (i_mag[0]) */
#define	RB_ROUND	RB_HIBIT
#define	RB_STICKY	(RB_HIBIT>>1)
#define	RB_GBIT		(RB_STICKY>>1)
#define	RB_GUARD	(RB_ROUND|RB_STICKY|RB_GBIT)

#define	ISQNaN(x)	((x)->i_flag == IFL_NaN && ((x)->i_mag[4] & (RB_HIBIT >> 1)) != 0)
#define	ISSNaN(x)	((x)->i_flag == IFL_NaN && ((x)->i_mag[4] & (RB_HIBIT >> 1)) == 0)
#define SETQNaN(x)	((x)->i_mag[4] |= (RB_HIBIT >> 1))

/* Description of double-extended layout. */

#define	SIGN_BIT	0x80	/* sign bit in exponent byte of external
				** representation for dbl-ext, dbl, flt
				*/

#define	X_FRACSIZE	64	/* bits of fraction */
#define	X_EXPSIZE	15	/* bits of exponent */
#define	X_EXPMAX	((1 << X_EXPSIZE) - 1)
#define	X_EXPBIAS	((1 << (X_EXPSIZE-1))-1)
/* Some machines (80387) bias denormalized numbers an extra number of
** bits so the high-order bit(s) of the significand are zero.
*/
#ifndef	X_DENORM_BIAS
#define	X_DENORM_BIAS	0
#endif

/* Description of float layout. */

#define	F_FRACSIZE	23	/* bits of fraction */
#define	F_EXPSIZE	8	/* bits of exponent */
#define	F_EXPMAX	((1 << F_EXPSIZE) - 1)
#define	F_EXPBIAS	((1 << (F_EXPSIZE-1))-1)

/* Description of double layout. */

#define	D_FRACSIZE	52	/* bits of fraction */
#define	D_EXPSIZE	11	/* bits of exponent */
#define	D_EXPMAX	((1 << D_EXPSIZE) - 1)
#define	D_EXPBIAS	((1 << (D_EXPSIZE-1))-1)

/* Significand encoding for infinity (low to high order) */
#ifndef	X_INFINITY
#define	X_INFINITY	0, 0, 0, 0
#endif

/* Encoding of "indefinite" result */
#ifndef	X_INDEFINITE
#define	X_INDEFINITE	1, X_EXPMAX, IFL_NaN, 0, ONES, ONES, ONES, ONES
#endif

#define ONES	((1 << BITS_PER_MAG) - 1)
/* Special internal values for various conditions.
** These are obviously dependent on the internal representation.
*/
static fpemu_i_t v_plusinf =  {  1, X_EXPMAX, IFL_INF, 0, X_INFINITY };
static fpemu_i_t v_minusinf = { -1, X_EXPMAX, IFL_INF, 0, X_INFINITY };
static fpemu_i_t v_indefinite =  {  X_INDEFINITE };
static fpemu_i_t v_pluszero = {  1, 0,        IFL_ZERO, 0, 0, 0, 0, 0 };
static fpemu_i_t v_minuszero ={ -1, 0,        IFL_ZERO, 0, 0, 0, 0, 0 };

#define FOR_HIGH_TO_LOW(var) for (var = 4; var >= 0; --var)
#define FOR_LOW_TO_HIGH(var) for (var = 0; var <= 4; ++var)

/* Internal operators, for convenience. */
#define	FP_NOP_OP	1		/* copy */
#define	FP_ADD_OP	2		/* add (subtract) */
#define FP_MUL_OP	3		/* multiply */
#define	FP_DIV_OP	4		/* divide */
#define FP_XTOFP_OP	5		/* convert ext. to float prec. */
#define FP_XTODP_OP	6		/* convert ext. to dbl. prec. */

static fpemu_i_t * fpemu_NaN();
static fpemu_i_t * fpemu_addmag();
static int	fpemu_compmag();
static void	fpemu_ex2in();
static fpemu_i_t * fpemu_final();
static fpemu_x_t	fpemu_in2ex();
static fpemu_i_t * fpemu_lshift();
static fpemu_i_t * fpemu_renorm();
static fpemu_i_t * fpemu_round();
static fpemu_i_t * fpemu_rshift();
static fpemu_i_t * fpemu_special();

/* Permit const declarations by nulling out for non-ANSI C. */
#if !defined(const) && !defined(__STDC__)
#define const
#endif


static void
fpemu_ex2in(ex,in)
fpemu_x_t *ex;
fpemu_i_t *in;
/* Convert external double-extended representation to internal. */
{
    in->i_sign =	(ex->ary[X_EXP_HIGH] & SIGN_BIT) ? SIGN_MINUS : SIGN_PLUS;
    in->i_exp =		(ex->ary[X_EXP_HIGH] & 0x7f) << 8 | ex->ary[X_EXP_LOW];
    in->i_mag[0] =	0;
#ifdef RTOLBYTES
    in->i_mag[1] =	ex->ary[X_SIG_LOW+0] | (ex->ary[X_SIG_LOW+1] << 8);
    in->i_mag[2] =	ex->ary[X_SIG_LOW+2] | (ex->ary[X_SIG_LOW+3] << 8);
    in->i_mag[3] =	ex->ary[X_SIG_LOW+4] | (ex->ary[X_SIG_LOW+5] << 8);
    in->i_mag[4] =	ex->ary[X_SIG_LOW+6] | (ex->ary[X_SIG_LOW+7] << 8);
#else
    in->i_mag[1] =	ex->ary[X_SIG_LOW-0] | (ex->ary[X_SIG_LOW-1] << 8);
    in->i_mag[2] =	ex->ary[X_SIG_LOW-2] | (ex->ary[X_SIG_LOW-3] << 8);
    in->i_mag[3] =	ex->ary[X_SIG_LOW-4] | (ex->ary[X_SIG_LOW-5] << 8);
    in->i_mag[4] =	ex->ary[X_SIG_LOW-6] | (ex->ary[X_SIG_LOW-7] << 8);
#endif

    switch(in->i_exp) {
    case 0:
	if (in->i_mag[1] | in->i_mag[2] | in->i_mag[3] | in->i_mag[4])
	    in->i_flag = IFL_DENORM;
	else
	    in->i_flag = IFL_ZERO;
	break;
    case X_EXPMAX:
	if (   in->i_mag[4] == v_plusinf.i_mag[4]
	    && in->i_mag[3] == v_plusinf.i_mag[3]
	    && in->i_mag[2] == v_plusinf.i_mag[2]
	    && in->i_mag[1] == v_plusinf.i_mag[1]
	    )
	    in->i_flag = IFL_INF;
	else
	    in->i_flag = IFL_NaN;
	break;
    default:
	if ((in->i_mag[4] & RB_HIBIT) != 0)
	    in->i_flag = IFL_NONE;
	else
	    *in = v_indefinite;		/* bad input value */
	break;
    }
    return;
}


static fpemu_x_t
fpemu_in2ex(in)
fpemu_i_t *in;
{
/* Convert internal double-extended representation to external. */
    fpemu_x_t ex;

    if (in->i_exp == X_EXPMAX) {		/* NaN or infinity */
	if (in->i_flag != IFL_NaN && in->i_flag != IFL_INF)
	    FPEFATAL(gettxt(":556","fpemu_in2ex:  bad exponent(1)"));
    }
    else if ((in->i_mag[4] & RB_HIBIT) == 0) {	/* Denormalized result */
	if (in->i_exp != 0)
	    FPEFATAL(gettxt(":557","fpemu_in2ex:  bad exponent (2)"));
    }
    else if (in->i_exp < 0)
	FPEFATAL(gettxt(":558","fpemu_in2ex:  bad exponent (3)"));

    if (in->i_exp > X_EXPMAX) {
	in = in->i_sign > 0 ? &v_plusinf : &v_minusinf;
	errno = ERANGE;
    }
    else if (in->i_exp < -X_FRACSIZE) {
	in = &v_pluszero;
	errno = ERANGE;
    }

    ex.ary[X_EXP_LOW] = in->i_exp;
    ex.ary[X_EXP_HIGH] = (in->i_exp >> 8);
    if (in->i_sign == SIGN_MINUS)
	ex.ary[X_EXP_HIGH] |= SIGN_BIT;
#ifdef RTOLBYTES
    ex.ary[X_SIG_LOW+0] =  in->i_mag[1];
    ex.ary[X_SIG_LOW+1] = (in->i_mag[1] >> 8);
    ex.ary[X_SIG_LOW+2] =  in->i_mag[2];
    ex.ary[X_SIG_LOW+3] = (in->i_mag[2] >> 8);
    ex.ary[X_SIG_LOW+4] =  in->i_mag[3];
    ex.ary[X_SIG_LOW+5] = (in->i_mag[3] >> 8);
    ex.ary[X_SIG_LOW+6] =  in->i_mag[4];
    ex.ary[X_SIG_LOW+7] = (in->i_mag[4] >> 8);
#else
    ex.ary[X_SIG_LOW-0] =  in->i_mag[1];
    ex.ary[X_SIG_LOW-1] = (in->i_mag[1] >> 8);
    ex.ary[X_SIG_LOW-2] =  in->i_mag[2];
    ex.ary[X_SIG_LOW-3] = (in->i_mag[2] >> 8);
    ex.ary[X_SIG_LOW-4] =  in->i_mag[3];
    ex.ary[X_SIG_LOW-5] = (in->i_mag[3] >> 8);
    ex.ary[X_SIG_LOW-6] =  in->i_mag[4];
    ex.ary[X_SIG_LOW-7] = (in->i_mag[4] >> 8);
#endif
    /* Zero padding bytes in external format. */
#if X_PAD_BYTES
#  if (X_PAD_BYTES & 0x001) != 0
    ex.ary[0] = 0;
#  endif
#  if (X_PAD_BYTES & 0x002) != 0
    ex.ary[1] = 0;
#  endif
#  if (X_PAD_BYTES & 0x004) != 0
    ex.ary[2] = 0;
#  endif
#  if (X_PAD_BYTES & 0x008) != 0
    ex.ary[3] = 0;
#  endif
#  if (X_PAD_BYTES & 0x010) != 0
    ex.ary[4] = 0;
#  endif
#  if (X_PAD_BYTES & 0x020) != 0
    ex.ary[5] = 0;
#  endif
#  if (X_PAD_BYTES & 0x040) != 0
    ex.ary[6] = 0;
#  endif
#  if (X_PAD_BYTES & 0x080) != 0
    ex.ary[7] = 0;
#  endif
#  if (X_PAD_BYTES & 0x100) != 0
    ex.ary[8] = 0;
#  endif
#  if (X_PAD_BYTES & 0x200) != 0
    ex.ary[9] = 0;
#  endif
#  if (X_PAD_BYTES & 0x400) != 0
    ex.ary[10] = 0;
#  endif
#  if (X_PAD_BYTES & 0x800) != 0
    ex.ary[11] = 0;
#  endif
#endif /* X_PAD_BYTES */
#if X_PAD_BYTES > 0x800
#include "can't handle padding"
#endif

    if (   (in->i_mag[1] & RB_HIPART) != 0
        || (in->i_mag[2] & RB_HIPART) != 0
        || (in->i_mag[3] & RB_HIPART) != 0
        || (in->i_mag[4] & RB_HIPART) != 0
    )
	FPEFATAL(gettxt(":559","fpemu_in2ex:  junk in high part of word"));

    return( ex );
}

int
fpemu_ispow2(x)
fpemu_x_t x;
/* Return non-zero if x is a normalized power of 2 */
{
    fpemu_i_t ix;
    int i;
    fpemu_ex2in(&x, &ix);
    if (ix.i_flag != IFL_NONE && ix.i_flag != IFL_ZERO)
	return 0;
    if ( 	(ix.i_mag[0] & RB_LOPART) != 0 || 
		(ix.i_mag[1] & RB_LOPART) != 0 || 
		(ix.i_mag[2] & RB_LOPART) != 0 || 
		(ix.i_mag[3] & RB_LOPART) != 0 || 
		(ix.i_mag[4] & RB_LOPART) != RB_HIBIT
       )
	return 0;
    return 1;
}

fpemu_x_t
fpemu_add(x,y)
fpemu_x_t x;
fpemu_x_t y;
/* Add x, y, return result. */
{
    fpemu_i_t ix, iy;		/* internal versions of each arg. */
    fpemu_i_t *result;

    fpemu_ex2in(&x, &ix);
    fpemu_ex2in(&y, &iy);

    /* Renormalize denormalized operands. */
    if (ix.i_flag == IFL_DENORM)
	(void) fpemu_renorm(&ix, X_DENORM_BIAS);
    if (iy.i_flag == IFL_DENORM)
	(void) fpemu_renorm(&iy, X_DENORM_BIAS);

    if ((result = fpemu_special(FP_ADD_OP, &ix, &iy)) == 0)
	result = fpemu_addmag(&ix, &iy);
    
    result = fpemu_final(result);
    return( fpemu_in2ex(result) );
}

static void
fpemu_mulsub(px, py, pr)
fpemu_i_t *px;
fpemu_i_t *py;
fpemu_i_t *pr;
/* Multiply two internal-form double-extendeds, px*py, produce
** like result, pr.  Preserve low-order precision word.
** The result value may share space with either of the input values.
*/
{
    unsigned long t;
    unsigned long partials[10];	/* for partial products */

    /* Renormalize operands. */
    if (px->i_flag == IFL_DENORM)
	(void) fpemu_renorm(px, X_DENORM_BIAS);
    if (py->i_flag == IFL_DENORM)
	(void) fpemu_renorm(py, X_DENORM_BIAS);
    
    pr->i_flag = IFL_NONE;
    pr->i_sign = px->i_sign * py->i_sign;
    pr->i_exp = px->i_exp + py->i_exp - X_EXPBIAS + 1;

    /* Calculate partial products.  Each partial[n] is the low-order
    ** product of x.mag[i] * y.mag[j], for each i+j == n, including
    ** carries from n-1.
    */
    partials[0] = 0;
    partials[1] = 0;
    partials[2] = 0;
    partials[3] = 0;
    partials[4] = 0;
    partials[5] = 0;
    partials[6] = 0;
    partials[7] = 0;
    partials[8] = 0;
    partials[9] = 0;

#define M(x,y) { t = px->i_mag[x] * py->i_mag[y]; \
		    partials[x+y] += t & RB_LOPART; \
		    partials[x+y+1] += ((t & RB_HIPART) >> BITS_PER_MAG); }
#define C(x)    { partials[x+1] += (partials[x] & RB_HIPART) >> BITS_PER_MAG; \
	      partials[x] &= RB_LOPART; }

    M(0,0);
    C(0);

    M(1,0); M(0,1);
    C(1);

    M(2,0); M(1,1); M(0,2);
    C(2);

    M(3,0); M(2,1); M(1,2); M(0,3);
    C(3)

    M(4,0); M(3,1); M(2,2); M(1,3); M(0,4);
    C(4);

    M(4,1); M(3,2); M(2,3); M(1,4);
    C(5);

    M(4,2); M(3,3); M(2,4);
    C(6);

    M(4,3); M(3,4);
    C(7);

    M(4,4);
    C(8);

    pr->i_mag[4] = partials[9];
    pr->i_mag[3] = partials[8];
    pr->i_mag[2] = partials[7];
    pr->i_mag[1] = partials[6];
    pr->i_mag[0] = partials[5];
    if (partials[4] | partials[3] | partials[2] | partials[1] | partials[0])
	pr->i_mag[0] |= 1;		/* don't lose low order info */

    /* May have to shift left one place to normalize. */
    if ((pr->i_mag[4] & RB_HIBIT) == 0)
	(void) fpemu_lshift(pr, 1);
    return;
}


fpemu_x_t
fpemu_mul(x,y)
fpemu_x_t x;
fpemu_x_t y;
/* Multiply x, y, return result. */
{
    fpemu_i_t ix, iy;		/* internal versions of each arg. */
    fpemu_i_t *result;
    fpemu_i_t r;

    fpemu_ex2in(&x, &ix);
    fpemu_ex2in(&y, &iy);

    if ((result = fpemu_special(FP_MUL_OP, &ix, &iy)) == 0) {
	fpemu_mulsub(&ix, &iy, &r);
	result = fpemu_final(&r);
    }
    return( fpemu_in2ex(result) );
}


fpemu_x_t
fpemu_div(x,y)
fpemu_x_t x;
fpemu_x_t y;
/* Divide x by y, return result. */
{
    fpemu_i_t ix, iy;		/* internal versions of each arg. */
    fpemu_i_t *result;
    fpemu_i_t r;

    fpemu_ex2in(&x, &ix);
    fpemu_ex2in(&y, &iy);

    if ((result = fpemu_special(FP_DIV_OP, &ix, &iy)) == 0) {
	/* Renormalize operands. */
	if (ix.i_flag == IFL_DENORM)
	    (void) fpemu_renorm(&ix, X_DENORM_BIAS);
	if (iy.i_flag == IFL_DENORM)
	    (void) fpemu_renorm(&iy, X_DENORM_BIAS);
	
	/* Both operands are non-zero valid numbers. */
	result = &r;
	r.i_flag = IFL_NONE;
	r.i_sign = ix.i_sign * iy.i_sign;
	r.i_exp = ix.i_exp - iy.i_exp
			+ X_EXPBIAS + X_FRACSIZE + BITS_PER_MAG - 1;
	r.i_mag[0] = 0;
	r.i_mag[1] = 0;
	r.i_mag[2] = 0;
	r.i_mag[3] = 0;
	r.i_mag[4] = 0;

	/* Compare, subtract, shift loop. */
	do {
	    if (fpemu_compmag(&ix,&iy) >= 0) {
		int i;
		int carry = 0;

		r.i_mag[0] |= 1;
		FOR_LOW_TO_HIGH(i) {
		    unsigned long *pres = &ix.i_mag[i];
		    *pres -= (iy.i_mag[i] + carry);
		    carry = 0;
		    if (*pres & RB_HIPART)
			carry = 1;
		    *pres &= RB_LOPART;
		}
	    }
	    (void) fpemu_lshift(&ix, 1);
	    (void) fpemu_lshift(&r, 1);
	} while ((r.i_mag[4] & RB_HIBIT) == 0);

	if (ix.i_mag[4] | ix.i_mag[3] | ix.i_mag[2] | ix.i_mag[1] | ix.i_mag[0])
	    r.i_mag[0] |= 1;

	result = fpemu_final(&r);
    }
    return( fpemu_in2ex(result) );
}



fpemu_x_t
fpemu_neg(arg1)
fpemu_x_t arg1;
/* Negate arg1 */
{
    /* Nevermind decoding and encoding value.  Just invert sign. */
    arg1.ary[X_EXP_HIGH] ^= SIGN_BIT;
    return( arg1 );
}


fpemu_x_t
fpemu_nop(x)
fpemu_x_t x;
{
    fpemu_i_t y;
    fpemu_i_t * pr = &y;

    fpemu_ex2in(&x,pr);
    if (pr->i_flag == IFL_NaN)
	pr = fpemu_NaN(FP_NOP_OP, pr, pr);
    return( fpemu_in2ex(pr) );
}


unsigned long
fpemu_xtoul(x)
fpemu_x_t x;
/* Convert double extended to unsigned long.  Rounding mode == chop.
** For invalid values, return EDOM and max/min value.  Assume 2's
** complement!
*/
{
    fpemu_i_t ix;
    int shift;

    fpemu_ex2in(&x, &ix);

    switch( ix.i_flag ){
    case IFL_DENORM:
    case IFL_ZERO:
	return( 0 );
    case IFL_NONE:
	/* Arrange to shift significand within the two high-order
	** magnitude words.
	*/
	if (ix.i_sign == SIGN_MINUS) {	/* no negative numbers <= 1.0 */
	    static init;
	    static fpemu_x_t minus1;
	    if (!init) {
		init = 1;
		minus1 = fpemu_ltox(-1L);
	    }
	    if (fpemu_compare(x, minus1) > 0) /* range OK */
		return 0;	
	    errno = EDOM;
	    return 0;
	}
	shift = ix.i_exp - X_EXPBIAS - (2*BITS_PER_MAG - 1);
	if (shift <= 0) {
	    unsigned long res;
	    /* Fits in host unsigned long. */
	    (void) fpemu_rshift(&ix, -shift);
	    res = ix.i_mag[4] << BITS_PER_MAG | ix.i_mag[3];
	    if (res <= T_ULONG_MAX)
		return( res );
	}
	/* Everything else is too big. */
	/*FALLTHRU*/
    case IFL_INF:
    case IFL_NaN:
	errno = EDOM;
	return( ix.i_sign > 0 ? T_ULONG_MAX : 0 );
    default:
	FPEFATAL(gettxt(":560","fpemu_xtol:  bad number"));
    }
    /*NOTREACHED*/
}


long
fpemu_xtol(x)
fpemu_x_t x;
/* Convert double extended to long.  Rounding mode == chop.  For
** invalid values, return EDOM and max/min value.  Assume 2's
** complement!
*/
{
    unsigned long res;
    int sign = 0;

    /* Determine sign of result, make sign positive, use fpemu_xtoul
    ** to do most of the work.
    */
    if (x.ary[X_EXP_HIGH] & SIGN_BIT) {
	sign = 1;
	x.ary[X_EXP_HIGH] &= ~SIGN_BIT;
    }
    res = fpemu_xtoul(x);
    if (sign == 0) {
	if (res > T_LONG_MAX) {
	    errno = EDOM;
	    res = T_LONG_MAX;
	}
	return( res );
    }
    /* Negative values. */
    if (res > (- (unsigned long) T_LONG_MIN)) {
	errno = EDOM;
	return( T_LONG_MIN );
    }
    return( (long) - (unsigned long) res );
}

fpemu_x_t
fpemu_ultox(x)
unsigned long x;
/* Convert unsigned long to double-extended. */
{
    fpemu_i_t res;

    if (x == 0)
	return( fpemu_in2ex( &v_pluszero ) );

    res.i_sign = SIGN_PLUS;
    res.i_exp = X_EXPBIAS - 1 + BITS_PER_MAG;
    res.i_flag = IFL_NONE;
    res.i_mag[0] = 0;
    res.i_mag[1] = 0;
    res.i_mag[2] = 0;
    res.i_mag[3] = 0;
    res.i_mag[4] = 0;

    for (;;) {
	res.i_mag[4] = x & RB_LOPART;
	x >>= BITS_PER_MAG;
	if (x == 0)
	    break;
	(void) fpemu_rshift(&res, BITS_PER_MAG);
    }
    return( fpemu_in2ex( fpemu_renorm(&res, 0) ) );
}

	
fpemu_x_t
fpemu_ltox(x)
long x;
/* Convert long to double-extended. */
{
    unsigned long ul = x;
    fpemu_x_t res;

    if (x >= 0)
	return( fpemu_ultox(ul) );
    
    ul = -ul;
    res = fpemu_ultox(ul);
    res.ary[X_EXP_HIGH] |= SIGN_BIT;
    return( res );
}


fpemu_x_t
fpemu_xtofp(x)
fpemu_x_t x;
/* Truncate double-extended value to precision of a float.
** Returned value remains double-extended.
*/
{
    fpemu_i_t ix;
    fpemu_i_t * res = &ix;
    int exp;

    fpemu_ex2in(&x, res);

    switch( ix.i_flag ){
    case IFL_NONE:			/* Non-zero ordinary value. */
	/* Remove low order bits, round.  Allow one extra position
	** for bit that's hidden on float, explicit in double-extended.
	** If the resulting float exponent would be non-positive ("exp"),
	** result would be denormalized.  Remove some extra bits that
	** would be lost.
	*/
	exp = res->i_exp - X_EXPBIAS + F_EXPBIAS;
	res = fpemu_rshift(res, X_FRACSIZE-F_FRACSIZE - (exp <= 0 ? exp : 1));

	res = fpemu_round(res);

	if (exp >= -F_FRACSIZE)
	    res = fpemu_renorm(res, 0);	/* there are bits to recover */

	if (exp >= F_EXPMAX) {		/* == F_EXPMAX is infinity, NaN */
	    res = res->i_sign > 0 ? &v_plusinf : &v_minusinf;
	    errno = EDOM;
	}
	else if (exp < 0) {
	    /* Would be denormalized float; make sure it would fit. */
	    if (exp < -F_FRACSIZE) {
		res = res->i_sign > 0 ? &v_pluszero : &v_minuszero;
		errno = ERANGE;
	    }
	    else
		res = fpemu_final(res);	/* to denormalize */
	}
	break;
    case IFL_NaN:
	res = fpemu_NaN(FP_XTOFP_OP, res, res);
	/*FALLTHRU*/
    case IFL_INF:
	break;
    case IFL_DENORM:
	/* All significand bits will be lost. */
	errno = EDOM;
	res = res->i_sign > 0 ? &v_pluszero : &v_minuszero;
	break;
    }
    return( fpemu_in2ex(res) );
}

fpemu_x_t
fpemu_xtodp(x)
fpemu_x_t x;
/* Truncate double-extended value to precision of a double.
** Returned value remains double-extended.
*/
{
    fpemu_i_t ix;
    fpemu_i_t * res = &ix;
    int exp;

    fpemu_ex2in(&x, res);

    switch( ix.i_flag ){
    case IFL_NONE:			/* Non-zero ordinary value. */
	/* Remove low order bits, round.  Allow one extra position
	** for bit that's hidden on double, explicit in double-extended.
	** If the resulting double exponent would be non-positive ("exp"),
	** result would be denormalized.  Remove some extra bits that
	** would be lost.
	*/
	exp = res->i_exp - X_EXPBIAS + D_EXPBIAS;
	res = fpemu_rshift(res, X_FRACSIZE-D_FRACSIZE - (exp <= 0 ? exp : 1));

	res = fpemu_round(res);

	if (exp >= -D_FRACSIZE)
	    res = fpemu_renorm(res, 0);	/* there are bits to recover */

	if (exp >= D_EXPMAX) {		/* == D_EXPMAX is infinity, NaN */
	    res = res->i_sign > 0 ? &v_plusinf : &v_minusinf;
	    errno = EDOM;
	}
	else if (exp < 0) {
	    /* Would be denormalized float; make sure it would fit. */
	    if (exp < -D_FRACSIZE) {
		res = res->i_sign > 0 ? &v_pluszero : &v_minuszero;
		errno = ERANGE;
	    }
	    else
		res = fpemu_final(res);	/* to denormalize */
	}
	break;
    case IFL_NaN:
	res = fpemu_NaN(FP_XTODP_OP, res, res);
	/*FALLTHRU*/
    case IFL_INF:
	break;
    case IFL_DENORM:
	/* All significand bits will be lost. */
	errno = EDOM;
	res = res->i_sign > 0 ? &v_pluszero : &v_minuszero;
	break;
    }
    return( fpemu_in2ex(res) );
}


static fpemu_i_t *
fpemu_special(op, l, r)
int op;
fpemu_i_t * l;
fpemu_i_t * r;
/* Determine whether op applied to operands l and r produces a
** special result.  If so, return the special result.  Otherwise
** return a null pointer.
*/
{
    int lflag = l->i_flag;
    int rflag = r->i_flag;

    /* Check NaN's */
    if (lflag == IFL_NaN || rflag == IFL_NaN)
	return( fpemu_NaN(op, l, r) );

    switch( op ){
    case FP_ADD_OP:
	/* Check zeroes. */
	if (lflag == IFL_ZERO)
	    return( r );
	if (rflag == IFL_ZERO)
	    return( l );
	/* Check infinities. */
	if (lflag == IFL_INF || rflag == IFL_INF) {
	    if (lflag == rflag)		/* both infinities */
		return ( l->i_sign == r->i_sign ? l : &v_indefinite );
	    return( lflag == IFL_INF ? l : r );
	}
	return ( 0 );
    
    case FP_MUL_OP:
	/* Check infinities. */
	if (lflag == IFL_INF || rflag == IFL_INF) {
	    if (lflag != rflag) {		/* both infinities */
		if (   lflag == IFL_INF  && rflag == IFL_ZERO
		    || lflag == IFL_ZERO && rflag == IFL_INF
		)
		    return( &v_indefinite );
	    }
	    return( l->i_sign == r->i_sign ? &v_plusinf : &v_minusinf );
	}

	/* Check zeroes. */
	if (lflag == IFL_ZERO || rflag == IFL_ZERO)
	    return( l->i_sign == r->i_sign ? &v_pluszero : &v_minuszero );
	return( 0 );
    
    case FP_DIV_OP:
	if (rflag == IFL_INF) {
	    if (lflag == IFL_INF)
		return( &v_indefinite );
	    else
		return( l->i_sign == r->i_sign ? &v_pluszero : &v_minuszero );
	}

	if (lflag == IFL_INF)
	    return( l->i_sign == r->i_sign ? &v_plusinf : &v_minusinf );

	if (rflag == IFL_ZERO)
	    return(	lflag == IFL_ZERO ? &v_indefinite
		     :  (l->i_sign == r->i_sign ? &v_plusinf : &v_minusinf)
		  );


	if (lflag == IFL_ZERO)
	    return( l->i_sign == r->i_sign ? &v_pluszero : &v_minuszero );
	return( 0 );
    }
    FPEFATAL(gettxt(":561","fpemu_special:  odd op %d"), op);
    /*NOTREACHED*/
}


/*ARGSUSED*/
static fpemu_i_t *
fpemu_NaN(op, px, py)
int op;
fpemu_i_t * px;
fpemu_i_t * py;
/* Figure out appropriate NaN result for computation.
** At least one of the operands is a NaN.  The behavior
** is implementation-dependent.
*/
{
#ifdef	FP_NAN_PROC
	FP_NAN_PROC
#else
    return &v_indefinite;
#endif
}


static fpemu_i_t *
fpemu_addmag(px, py)
fpemu_i_t * px;
fpemu_i_t * py;
/* Carry out addition of two valid numbers of possibly different signs.
** Return pointer to result value.
*/
{
    static fpemu_i_t res;
    int samesign = px->i_sign == py->i_sign;
    int shift;
    fpemu_i_t * result = &res;

    /* Arrange numbers so px has larger exponent.
    ** If subtraction is involved, make sure px is larger |value|.
    */
    if (   px->i_exp < py->i_exp
	|| (!samesign && px->i_exp == py->i_exp && fpemu_compmag(px,py) < 0)
    ){
	fpemu_i_t * t = px;
	px = py;
	py = t;
    }

    /* Check relative (positive) difference of exponents. */
    shift = px->i_exp - py->i_exp;
    if (shift > X_FRACSIZE+2)		/* allow for guard bits */
	res = *px;
    else {
	int i;
	unsigned long * pres;
	int carry = 0;

	if (shift)
	    (void) fpemu_rshift(py, shift);

	res.i_flag = IFL_NONE;

	/* px is larger magnitude, so result has same sign, exponent
	** (before normalization)
	*/
	res.i_sign = px->i_sign;
	res.i_exp = px->i_exp;

	if (samesign) {
	    FOR_LOW_TO_HIGH(i) {
		pres = &res.i_mag[i];
		*pres = px->i_mag[i] + py->i_mag[i] + carry;
		carry = 0;
		if (*pres & RB_HIPART)
		    carry = 1;
	    }
	}
	else {				/* different signs */
	    FOR_LOW_TO_HIGH(i) {
		pres = &res.i_mag[i];
		*pres = px->i_mag[i] - py->i_mag[i] - carry;
		carry = 0;
		if (*pres & RB_HIPART)
		    carry = 1;
	    }
	}
	res.i_mag[0] &= RB_GUARD;
	res.i_mag[1] &= RB_LOPART;
	res.i_mag[2] &= RB_LOPART;
	res.i_mag[3] &= RB_LOPART;

	if (samesign) {
	    if (res.i_mag[4] & RB_CARRY)
		(void) fpemu_rshift(&res, 1);
	}
	else {
	    if (   res.i_mag[0] == 0 && res.i_mag[1] == 0 && res.i_mag[2] == 0
		&& res.i_mag[3] == 0 && res.i_mag[4] == 0
	    ) {
		/* Make result true zero. */
		res.i_sign = SIGN_PLUS;
		res.i_exp = 0;
		res.i_flag = IFL_ZERO;
	    }
	    else {
		while (res.i_exp > 0 && (res.i_mag[4] & RB_HIBIT) == 0)
		    (void) fpemu_lshift(&res, 1);
	    }
	}
    }
    return( result );
}


static int
fpemu_compmag(l, r)
fpemu_i_t * l;
fpemu_i_t * r;
/* Compare magnitudes of two operands.
** Return appropriate FPEMU_CMP_... value, but not UNORDERED.
*/
{
    int i;
    
    FOR_HIGH_TO_LOW(i) {
	if (l->i_mag[i] > r->i_mag[i])
	    return( FPEMU_CMP_GREATERTHAN );
	else if (l->i_mag[i] < r->i_mag[i])
	    return( FPEMU_CMP_LESSTHAN );
    }
    return( FPEMU_CMP_EQUALTO );
}


static fpemu_i_t *
fpemu_renorm(px, bias)
fpemu_i_t * px;
int bias;
/* Normalize px so the high order significand bit is one.
** Add bias to resulting exponent.
*/
{
    /* Figure out how far left to shift. */
    int sh = 0;
    int i;
    unsigned long mag;

    /* (Denormalized number guaranteed to have at least one non-zero
    ** magnitude word.)
    */
    FOR_HIGH_TO_LOW(i) {
	if (px->i_mag[i] != 0) break;
	sh += BITS_PER_MAG;
    }
    
    mag = px->i_mag[i];
    while ((mag & RB_HIBIT) == 0) {
	++sh;
	mag <<= 1;
    }
    px->i_exp += bias;
    return( fpemu_lshift(px, sh) );
}


static fpemu_i_t *
fpemu_final(px)
fpemu_i_t * px;
/* Finish up operation:  round result, fix up denormalized numbers.
** Set errors on bad values.
*/
{
    if (px->i_exp >= X_EXPMAX && px->i_flag == IFL_NONE)
	px->i_exp = X_EXPMAX+1;		/* force error from fpemu_in2ex */
    else if (px->i_exp < -X_FRACSIZE + X_DENORM_BIAS) {
	errno = ERANGE;
	return( px->i_sign > 0 ? &v_pluszero : &v_minuszero );
    }
    else if (px->i_exp <= 0) {
	/* Shift into denormalized position. */
	(void) fpemu_rshift(px, -px->i_exp + X_DENORM_BIAS);
	px->i_exp = 0;
	px->i_flag = IFL_DENORM;
    }

    px = fpemu_round(px);

    return( px );
}


static fpemu_i_t *
fpemu_round(px)
fpemu_i_t * px;
/* Round (to nearest) px. */
{
    unsigned long guard = px->i_mag[0];

    px->i_mag[0] = 0;

    /* First adjust sticky bit by bits below. */
    if ((guard & ~(RB_ROUND|RB_STICKY)) != 0)
	guard |= RB_STICKY;
    
    switch( guard & (RB_ROUND|RB_STICKY) ){
    /* There are assumptions here about the relative values of these bits. */
    case RB_ROUND:
	if ((px->i_mag[1] & 1) == 0)
	    break;
	/*FALLTHRU*/
    case RB_ROUND|RB_STICKY:
	/* Propagate carry through magnitude words. */
	{
	    int i;
	    int carry = 0;
	    ++px->i_mag[1];
	    /* Yes, there's an extra iteration here... */
	    FOR_LOW_TO_HIGH(i) {
		px->i_mag[i] += carry;
		carry = 0;
		if (px->i_mag[i] & RB_HIPART)
		    carry = 1;
	    }
	    px->i_mag[1] &= RB_LOPART;	/* i_mag[0] should be zero already */
	    px->i_mag[2] &= RB_LOPART;
	    px->i_mag[3] &= RB_LOPART;
	    if (carry)
		(void) fpemu_rshift(px, 1);
	}
    }
    return( px );
}


static fpemu_i_t *
fpemu_lshift(px, n)
fpemu_i_t * px;
int n;
{
    px->i_exp -= n;

    /* Shift magnitude part of px left n bits. */
    for ( ; n > BITS_PER_MAG; n -= BITS_PER_MAG) {
	if (px->i_mag[4] != 0)
	    FPEFATAL(gettxt(":562","fpemu_lshift:  losing bits"));
	px->i_mag[4] = px->i_mag[3];
	px->i_mag[3] = px->i_mag[2];
	px->i_mag[2] = px->i_mag[1];
	px->i_mag[1] = px->i_mag[0];
	px->i_mag[0] = 0;
    }

    if (n > 0) {
	unsigned long himask = (~(RB_LOPART >> n)) & RB_LOPART;
						/* keep high part */
	unsigned long piece = 0;
	int i;

	FOR_LOW_TO_HIGH(i) {
	    unsigned long newpiece = (px->i_mag[i] & himask);
	    px->i_mag[i] =
		  ((px->i_mag[i] << n) & RB_LOPART)
			| (piece >> BITS_PER_MAG - n);
	    piece = newpiece;
	}
	/* Restore high order bits of high-order magnitude. */
	px->i_mag[4] |= piece << n;
    }
    return px;
}


static fpemu_i_t *
fpemu_rshift(px, n)
fpemu_i_t * px;
int n;
{
    px->i_exp += n;

    /* Shift magnitude part of px right n bits, with sticky guard bits. */
    for ( ; n > BITS_PER_MAG; n -= BITS_PER_MAG) {
	px->i_mag[0] = px->i_mag[1] | (px->i_mag[0] != 0);
	px->i_mag[1] = px->i_mag[2];
	px->i_mag[2] = px->i_mag[3];
	px->i_mag[3] = px->i_mag[4];
	px->i_mag[4] = 0;
    }

    if (n > 0) {
	unsigned long lomask = (~((~0L) << n));		/* keep low part */
	unsigned long piece = 0;
	int i;

	FOR_HIGH_TO_LOW(i) {
	    unsigned long newpiece = (px->i_mag[i] & lomask);
	    px->i_mag[i] = (px->i_mag[i] >> n) | piece;
	    piece = newpiece << (BITS_PER_MAG - n);
	}
	if (piece)
	    px->i_mag[0] |= 1;
    }
    if ((px->i_mag[0] & ~RB_GUARD) != 0)
	px->i_mag[0] = (px->i_mag[0] & RB_GUARD) | RB_GBIT;

    return px;
}

int
fpemu_iszero(x)
fpemu_x_t x;
/* Return 1 if x is zero.  Otherwise return 0. */
{
    fpemu_i_t val;

    fpemu_ex2in(&x, &val);
    return( val.i_flag == IFL_ZERO );
}


int
fpemu_compare(x,y)
fpemu_x_t x;
fpemu_x_t y;
/* Compare x::y.  Return appropriate FPEMU_CMP_... value. */
{
    fpemu_i_t ix;
    fpemu_i_t iy;

    fpemu_ex2in(&x, &ix);
    fpemu_ex2in(&y, &iy);

    /* If either operand is a NaN, return "unordered". */
    if (ix.i_flag == IFL_NaN || iy.i_flag == IFL_NaN)
	return FPEMU_CMP_UNORDERED;

    if (ix.i_sign != iy.i_sign) {
	/* Special case for comparing 0.0 and -0.0 */
	if(ix.i_flag == IFL_ZERO && iy.i_flag == IFL_ZERO)
		return 0;
	if (ix.i_sign < 0)
	    return FPEMU_CMP_LESSTHAN;
	return FPEMU_CMP_GREATERTHAN;
    }
    /* Operands have same sign.  Account for negative values! */
    if (ix.i_exp < iy.i_exp)
	return FPEMU_CMP_LESSTHAN * ix.i_sign;
    else if (ix.i_exp > iy.i_exp)
	return FPEMU_CMP_GREATERTHAN * ix.i_sign;
    
    /* Same sign, same exponent.  Compare magnitudes. */
    return( fpemu_compmag(&ix, &iy) * ix.i_sign );
}

fpemu_f_t
fpemu_xtof(x)
fpemu_x_t x;
/* Convert double-extended to float. */
{
    fpemu_i_t ix;
    fpemu_f_t res;
    int exp;
    unsigned short t;

    x = fpemu_xtofp(x);			/* truncate to float precision */
    fpemu_ex2in(&x, &ix);

    /* If result is infinity/NaN, produce suitable exponent; truncate
    ** significand.
    */
    if ((exp = ix.i_exp) == X_EXPMAX)
	exp = F_EXPMAX;
    else if (exp != 0)
	exp += F_EXPBIAS - X_EXPBIAS;
    
    /* Make a denormalized number.  Shift an extra position for
    ** the hidden bit that isn't hidden.
    */
    if (exp <= 0) {
	(void) fpemu_rshift(&ix, -exp+1);
	exp = 0;
    }

    /* Move significand into convenient position (open up 8 high-order bits),
    ** discard to-be-hidden bit.
    */
    if (exp != 0)
	ix.i_mag[4] &= ~RB_HIBIT;
    (void) fpemu_rshift(&ix, 2*BITS_PER_MAG - F_FRACSIZE - 1);

    /* Construct high-order short from exponent, high-order significand. */
    t = (exp << (BITS_PER_MAG - F_EXPSIZE - 1)) | ix.i_mag[4];

#ifdef RTOLBYTES
    res.ary[F_SIG_LOW+0] =  ix.i_mag[3];
    res.ary[F_SIG_LOW+1] = (ix.i_mag[3] >> 8);
    res.ary[F_SIG_LOW+2] =  t;
#else
    res.ary[F_SIG_LOW-0] =  ix.i_mag[3];
    res.ary[F_SIG_LOW-1] = (ix.i_mag[3] >> 8);
    res.ary[F_SIG_LOW-2] =  t;
#endif
    /* Assume exponent fits in one byte. */
    res.ary[F_EXP_BYTE] = t >> 8;
    if (ix.i_sign < 0)
	res.ary[F_EXP_BYTE] |= SIGN_BIT;

    /* Zero padding bytes in external format. */
#if F_PAD_BYTES
#  if (F_PAD_BYTES & 0x001) != 0
    ex.ary[0] = 0;
#  endif
#  if (F_PAD_BYTES & 0x002) != 0
    ex.ary[1] = 0;
#  endif
#  if (F_PAD_BYTES & 0x004) != 0
    ex.ary[2] = 0;
#  endif
#  if (F_PAD_BYTES & 0x008) != 0
    ex.ary[3] = 0;
#  endif
#  if (F_PAD_BYTES & 0x010) != 0
    ex.ary[4] = 0;
#  endif
#  if (F_PAD_BYTES & 0x020) != 0
    ex.ary[5] = 0;
#  endif
#  if (F_PAD_BYTES & 0x040) != 0
    ex.ary[6] = 0;
#  endif
#  if (F_PAD_BYTES & 0x080) != 0
    ex.ary[7] = 0;
#  endif
#  if (F_PAD_BYTES & 0x100) != 0
    ex.ary[8] = 0;
#  endif
#  if (F_PAD_BYTES & 0x200) != 0
    ex.ary[9] = 0;
#  endif
#  if (F_PAD_BYTES & 0x400) != 0
    ex.ary[10] = 0;
#  endif
#  if (F_PAD_BYTES & 0x800) != 0
    ex.ary[11] = 0;
#  endif
#endif /* F_PAD_BYTES */
#if F_PAD_BYTES > 0x800
#include "can't handle padding"
#endif

    return( res );
}


fpemu_d_t
fpemu_xtod(x)
fpemu_x_t x;
/* Convert double-extended to double. */
{
    fpemu_i_t ix;
    fpemu_d_t res;
    int exp;

    x = fpemu_xtodp(x);			/* truncate to double precision */
    fpemu_ex2in(&x, &ix);

    /* If result is infinity/NaN, produce suitable exponent; truncate
    ** significand.
    */
    if ((exp = ix.i_exp) == X_EXPMAX)
	exp = D_EXPMAX;
    else if (exp != 0)
	exp += D_EXPBIAS - X_EXPBIAS;
    
    /* Make a denormalized number.  Shift an extra position for
    ** the hidden bit that isn't hidden.
    */
    if (exp <= 0) {
	(void) fpemu_rshift(&ix, -exp+1);
	exp = 0;
    }

    /* Adjust exponent for position relative to magnitude. */
    exp <<= (BITS_PER_MAG - D_EXPSIZE - 1);

    res.ary[D_EXP_HIGH] = (exp >> 8);
    res.ary[D_EXP_LOW] = exp;
    if (ix.i_sign < 0)
	res.ary[D_EXP_HIGH] |= SIGN_BIT;

    /* Move significand into convenient position, discard to-be-hidden bit. */
    if (exp != 0)
	ix.i_mag[4] &= ~RB_HIBIT;
    (void) fpemu_rshift(&ix, 4*BITS_PER_MAG - D_FRACSIZE - 1);

    /* Assume overlap of D_EXP_LOW, high part of significand. */
#ifdef RTOLBYTES
    res.ary[D_SIG_LOW+0]  =  ix.i_mag[1];
    res.ary[D_SIG_LOW+1]  = (ix.i_mag[1] >> 8);
    res.ary[D_SIG_LOW+2]  =  ix.i_mag[2];
    res.ary[D_SIG_LOW+3]  = (ix.i_mag[2] >> 8);
    res.ary[D_SIG_LOW+4]  =  ix.i_mag[3];
    res.ary[D_SIG_LOW+5]  = (ix.i_mag[3] >> 8);
    res.ary[D_SIG_LOW+6] |=  ix.i_mag[4];
#else
    res.ary[D_SIG_LOW-0]  =  ix.i_mag[1];
    res.ary[D_SIG_LOW-1]  = (ix.i_mag[1] >> 8);
    res.ary[D_SIG_LOW-2]  =  ix.i_mag[2];
    res.ary[D_SIG_LOW-3]  = (ix.i_mag[2] >> 8);
    res.ary[D_SIG_LOW-4]  =  ix.i_mag[3];
    res.ary[D_SIG_LOW-5]  = (ix.i_mag[3] >> 8);
    res.ary[D_SIG_LOW-6] |=  ix.i_mag[4];
#endif
    /* Zero padding bytes in external format. */
#if D_PAD_BYTES
#  if (D_PAD_BYTES & 0x001) != 0
    ex.ary[0] = 0;
#  endif
#  if (D_PAD_BYTES & 0x002) != 0
    ex.ary[1] = 0;
#  endif
#  if (D_PAD_BYTES & 0x004) != 0
    ex.ary[2] = 0;
#  endif
#  if (D_PAD_BYTES & 0x008) != 0
    ex.ary[3] = 0;
#  endif
#  if (D_PAD_BYTES & 0x010) != 0
    ex.ary[4] = 0;
#  endif
#  if (D_PAD_BYTES & 0x020) != 0
    ex.ary[5] = 0;
#  endif
#  if (D_PAD_BYTES & 0x040) != 0
    ex.ary[6] = 0;
#  endif
#  if (D_PAD_BYTES & 0x080) != 0
    ex.ary[7] = 0;
#  endif
#  if (D_PAD_BYTES & 0x100) != 0
    ex.ary[8] = 0;
#  endif
#  if (D_PAD_BYTES & 0x200) != 0
    ex.ary[9] = 0;
#  endif
#  if (D_PAD_BYTES & 0x400) != 0
    ex.ary[10] = 0;
#  endif
#  if (D_PAD_BYTES & 0x800) != 0
    ex.ary[11] = 0;
#  endif
#endif /* D_PAD_BYTES */
#if D_PAD_BYTES > 0x800
#include "can't handle padding"
#endif

    return( res );
}
    

/* The ideas for, and outline of, this code are shamelessly stolen from
** Andrew Koenig.
*/

/* Multi-precision integer for input conversion.  mp_int[0] is low order. */

typedef struct {
    int mp_usize;		/* size used (unsigned longs) */
    int mp_asize;		/* actual size (unsigned longs) */
    unsigned long * mp_int;	/* pointer to array of components of value */
} mp_t;

#define MCHUNK	4		/* Allocate mp_int in hunks this big. */
/* MAXEXP, in this context, is an exponent so ridiculously big that
** any input number with this exponent would be too large.
*/
#define MAXEXP	99999999

/* X_MAX_DEC_EXP is a guess at the decimal exponent value that would be
** ridiculously large.  Numbers with exponents larger than this overflow.
** If smaller, they underflow.
*/
#define X_MAX_DEC_EXP	((X_EXPMAX - X_EXPBIAS) * 10)

#ifdef	__STDC__
typedef void myvoid;
#else
typedef char myvoid;
#endif

myvoid *malloc();
myvoid *realloc();

static void
mp_resize(mp, newsize)
mp_t * mp;
int newsize;
/* Resize a multi-precision int.  Pad high-order with zeroes if needed. */
{
    /* Round up desired size. */
    int want = ((newsize + MCHUNK + 1) / MCHUNK) * MCHUNK;

    if (want > mp->mp_asize) {
	/* Need to allocate memory. */
	unsigned int size = want * sizeof(mp->mp_int[0]);

	mp->mp_int = (unsigned long *)
			(mp->mp_int
			    ? realloc((myvoid *) mp->mp_int, size)
			    : malloc(size));
	mp->mp_asize = want;
    }
    while (mp->mp_asize < want)
	mp->mp_int[mp->mp_asize++] = 0;
    
    mp->mp_usize = newsize;
    return;
}


static void
mp_init(mp)
mp_t * mp;
/* Initialize an mp_t. */
{
    mp_resize(mp, 0);
    return;
}


static void
mp_seti(mp, n)
mp_t * mp;
int n;
/* Initialize an mp_t to a small integral value. */
{
    mp_resize(mp, 1);			/* assume fits in one word */
    mp->mp_int[0] = n;
    return;
}


static void
mp_imul(mp, n)
mp_t * mp;
int n;
/* Multiply an mp_t by a small integer. */
{
    int i;
    unsigned long carry = 0;
    unsigned long * mag = &mp->mp_int[0];

    for (i = 0; i < mp->mp_usize; ++i) {
	carry = carry + *mag * n;
	*mag = carry & RB_LOPART;
	carry >>= BITS_PER_MAG;
	++mag;
    }

    if (carry) {
	mp_resize(mp, i+1);
	mp->mp_int[i] = carry;		/* don't use "mag" -- ptr changed */
    }
    return;
}

static void
mp_iadd(mp, n)
mp_t * mp;
int n;
/* Add small integer to mp. */
{
    int i;
    unsigned long carry = n;

    for (i = 0; i < mp->mp_usize; ++i) {
	carry += mp->mp_int[i];
	mp->mp_int[i] = carry & RB_LOPART;
	if ((carry >>= BITS_PER_MAG) == 0)
	    return;
    }

    if (carry) {
	mp_resize(mp, i+1);
	mp->mp_int[i] = carry;
    }
    return;
} 


static void
mp_mul3(mp, mq, mr)
mp_t * mp;
mp_t * mq;
mp_t * mr;
/* Produce mr = mp * mq.  Result can be same as an operand. */
{
    mp_t res;				/* local result */
    unsigned long * r;			/* point at res value part */
    int i, j;

    res.mp_int = 0;
    res.mp_asize = 0;
    res.mp_usize = 0;
    /* Set presumed result size. */
    mp_resize(&res, mp->mp_usize + mq->mp_usize);

    /* The simple algorithm is:  zero all the result words, then
    ** do the multiplies.  It's cheaper to unroll the loop so
    ** the first iteration just sets the product values, and
    ** successive ones add to them, plus zero the next highest
    ** result word.
    */
    /* for (i = 0;;) */
    r = &res.mp_int[0+0];
    r[0] = 0;
    {
	unsigned long prod = 0;
	unsigned long x = mp->mp_int[0];

	for (j = 0; j < mq->mp_usize; ++j) {
	    prod += x * mq->mp_int[j];
	    *r = prod & RB_LOPART;
	    prod >>= BITS_PER_MAG;
	    ++r;
	}
	*r = prod;
    }

    /* Multiply remaining pieces. */
    for (i = 1; i < mp->mp_usize; ++i) {
	unsigned long x = mp->mp_int[i];

	r = &res.mp_int[i+0];
	r[mq->mp_usize] = 0;		/* zero highest carry */
	for (j = 0; j < mq->mp_usize; ++j) {
	    unsigned long prod = x * mq->mp_int[j] + *r;

	    /* Propagate product, carries.  Note that the first operation
	    ** cannot lose bits, even if both operands are 0xffff and the
	    ** other addition operand is 0xffff.
	    */
	    *r = prod & RB_LOPART;
	    ++r;
	    *r += prod >> BITS_PER_MAG;
	}
    }
    /* "Normalize" result to remove leading zero words. */
    for (r = &res.mp_int[res.mp_usize-1]; *r == 0; --r)
	--res.mp_usize;

    /* Pass back result. */
    if (mr->mp_int)
	free((myvoid *) mr->mp_int);
    
    *mr = res;
    return;
}

static void
mp_sub(x, y)
mp_t * x;
mp_t * y;
/* x -= y, for x >= y */
{
    int i;
    unsigned long borrow = 0;
    unsigned long *px = &x->mp_int[0];

    for (i = 0; i < x->mp_usize; ++i, ++px) {
	int new = 0;
	if (i < y->mp_usize)
	    new = y->mp_int[i];
	*px -= new + borrow;
	borrow = 0;
	if (*px & RB_HIPART) {
	    *px &= RB_LOPART;
	    borrow = 1;
	}
    }
    if (borrow)
	FPEFATAL(gettxt(":563","mp_sub:  lingering carry"));
    return;
}

static int
mp_iszero(mp)
mp_t * mp;
/* Is mp zero? */
{
    int i;

    for (i = 0; i < mp->mp_usize; ++i) {
	if (mp->mp_int[i] != 0)
	    return( 0 );
    }
    return( 1 );
}


static int
mp_cmpmag(x, y)
mp_t * x;
mp_t * y;
/* Compare x and y.  Return -1 for x<y, 0 for x==y, 1 for x>y */
{
    int n;

    n = x->mp_usize;
    if (y->mp_usize > n)
	n = y->mp_usize;
    
    for (; n >= 0; --n) {
	unsigned long tx = 0;
	unsigned long ty = 0;
	if (n < x->mp_usize)
	    tx = x->mp_int[n];
	if (n < y->mp_usize)
	    ty = y->mp_int[n];
	
	if (tx < ty)
	    return -1;
	if (tx > ty)
	    return 1;
    }
    return 0;
}


static void
mp_shift(x, n)
mp_t * x;
int n;
/* Effectively multiply x by 2^(n*BITS_PER_MAG).  That is
** move x's magnitude part n words to the "left".
*/
{
    int oldsize = x->mp_usize;
    unsigned long * from;
    unsigned long * to;
    unsigned long * limit;

    if (n <= 0)
	FPEFATAL(gettxt(":564","mp_shift: bad n"));
    
    mp_resize(x, oldsize+n);

    from = &x->mp_int[oldsize-1];
    to = &x->mp_int[oldsize-1+n];
    limit = &x->mp_int[0];
    while (from >= limit) {
	*to = *from;
	--to;
	--from;
    }
    while (to >= limit)			/* zero low order part */
	*to-- = 0;
    return;
}


static void
mp_div(n, d, r)
mp_t * n;
mp_t * d;
fpemu_i_t * r;
/* Compute r = n / d.  (d != 0)  Note that r is an internal-
** form value.  Produce 5 words of precision, then rely on
** the usual rounding to fix things up.
*/
{
    int i;
    unsigned long nh, dh;

    r->i_mag[0] = 0;
    r->i_mag[1] = 0;
    r->i_mag[2] = 0;
    r->i_mag[3] = 0;
    r->i_mag[4] = 0;
    r->i_sign = SIGN_PLUS;
    r->i_exp = X_EXPBIAS - 1 + 5 * BITS_PER_MAG;
    r->i_flag = IFL_NONE;

    /* Arrange n, d so:  d <= n < 2*d
    ** First step is to make numbers same order of magnitude.
    ** This avoids a lot of wasted motion.
    */
    if (d->mp_usize > n->mp_usize) {
	int s = d->mp_usize - n->mp_usize;
	mp_shift(n, s);
	r->i_exp -= s * BITS_PER_MAG;
    }
    else if (n->mp_usize > d->mp_usize) {
	int s = n->mp_usize - d->mp_usize;
	mp_shift(d, s);
	r->i_exp += s * BITS_PER_MAG;
    }

    /* Numerator and denominator have same number of magnitude words.
    ** Thus the position of their high order bits can differ by, at
    ** most, BITS_PER_MAG.  Arrange to get high order bits in matching
    ** positions as quickly as possible.  Eventually we want the
    ** numerator > denominator.
    */
    nh = n->mp_int[n->mp_usize-1];
    dh = d->mp_int[d->mp_usize-1];

    /* i will contain offset of n's high order bit with respect
    ** to d's.  If i > 0, n > d.
    */
    for (i = 0; nh != 0; ++i)
	nh >>= 1;
    for (     ; dh != 0; --i)
	dh >>= 1;
    
    if (i > 0) {
	r->i_exp += i;
	mp_imul(d, 1 << i);
    }
    else if (i < 0) {
	i = -i;
	r->i_exp -= i;
	mp_imul(n, 1 << i);
    }

    /* High order bits now match.  If n < d, it can only be by a
    ** factor of 2.
    */
    if (mp_cmpmag(n, d) < 0) {
	mp_imul(n, 2);
	--r->i_exp;
    }

    /* Now get the quotient. */
    do {
	if (mp_cmpmag(n, d) >= 0) {
	    mp_sub(n, d);
	    r->i_mag[0] |= 1;
	}
	(void) fpemu_lshift(r, 1);
	mp_imul(n, 2);
    } while ((r->i_mag[4] & RB_HIBIT) == 0);

    /* Account for lingering remainder. */
    if (! mp_iszero(n))
	r->i_mag[0] |= 1;
    return;
}


static fpemu_i_t *
mp_toi(mp, r)
mp_t * mp;
fpemu_i_t * r;
/* Convert internal multiple-precision int into floating point form. */
{
    int first1;
    int i;
    unsigned long himag;
    unsigned long guard = 0;

    /* Find first word that actually has bits. */
    for (first1 = mp->mp_usize-1; first1 >= 0; --first1) {
	if (mp->mp_int[first1] != 0)
	    break;
    }
    if (first1 < 0)
	FPEFATAL(gettxt(":565","mp_toi:  no bits"));
    
    himag = mp->mp_int[first1];

    /* Initial exponent:  -1 for non-hidden high bit. */
    r->i_exp = X_EXPBIAS - 1 + (first1 + 1) * BITS_PER_MAG;
    r->i_flag = IFL_NONE;

    /* Copy up to five significand words. */
    FOR_HIGH_TO_LOW(i) {
	r->i_mag[i] = (first1 >= 0 ? mp->mp_int[first1] : 0);
	--first1;
    }
    
    /* Figure out how many leading zero bits are in the first magnitude
    ** word.  We'll grab the next magnitude word and fill in missing
    ** guard bits after "renormalizing".
    */
    guard = (first1 >= 0 ? mp->mp_int[first1] : 0);
    while (himag != 0) {
	guard >>= 1;
	himag >>= 1;
    }

    r = fpemu_renorm(r, 0);
    r->i_mag[0] |= guard;
    return r;
}


static mp_t *
mp_pow(x)
int x;
/* Return 5^|x|, a multi-word int.  Because this can get
** expensive to do on the fly, we retain an array of powers
** we've computed already and reuse them.  This also reduces
** the number of multiplications.
*/
{
    static mp_t exp;
    static mp_t pow[32];		/* pow[i] is 5^(2^i)
					** let's hope int's have fewer than
					** 32 bits.
					*/
    mp_t * curpow = pow;		/* start at 5^1 */
    static int firsttime = 1;

    /* For first time, pre-compute some easy values. */
    if (firsttime) {
	mp_seti(&pow[0], 5);
	mp_seti(&pow[1], 5*5);
	mp_seti(&pow[2], 5*5*5*5);
	firsttime = 0;
    }

    mp_seti(&exp, 1);			/* start our result */

    if (x < 0)
	x = -x;

    while (x != 0) {
	/* Fixed point:  all mp_t's up to curpow have correct values. */
	if (curpow->mp_int == 0)	/* compute next value */
	    mp_mul3(curpow-1, curpow-1, curpow);
	if ((x & 1) != 0)
	    mp_mul3(curpow, &exp, &exp);
	x = (unsigned int) x >> 1;
	++curpow;
    }
    return( &exp );
}


fpemu_x_t
fpemu_atox(s)
const char * s;
/* Convert a floating point number to the external form of a
** double-extended.  Accept leading spaces, leading sign.
*/
{
    static mp_t frac;		/* static:  initialize for
				** free, retain space
				*/
    fpemu_i_t res;
    fpemu_i_t *pr = &res;
    int sign = SIGN_PLUS;	/* presumed */
    int sawdot = 0;		/* saw . in number if 1 */
    int digs = 0;		/* number of significant digits */
    int expoffset = 0;		/* exponent offset (digits after .) */
    int exp;

    /* Initialize the result to 0. */
    mp_init(&frac);
    while (isspace(*s)) /* skip leading space */
	++s;
    switch (*s) {
    case '-':
	sign = SIGN_MINUS;
	/*FALLTHRU*/
    case '+':
	s++;
    }
    /* Handle leading 0's specially. */
    if (*s == '0') {
#ifndef NO_NCEG
	if (s[1] == 'x' || s[1] == 'X') {
	    /* Hexadecimal floating constant. */
	    s++;
	    while (*++s == '0') /* skip leading 0's */
		;
	    if (*s == '.') {
		sawdot = 4;
		while (*++s == '0') /* fractional 0's, too */
		    expoffset -= 4;
	    }
	    if (!isxdigit(*s)) /* 'p' and 'P', too */
		goto zero;
	    /* At the first nonzero hex digit; accumulate until exponent. */
	    for (;;) {
		expoffset -= sawdot;
		if (isdigit(*s))
		    digs = *s - '0';
		else if (isupper(*s))
		    digs = *s - 'A' + 10;
		else
		    digs = *s - 'a' + 10;
		mp_imul(&frac, 16);
		mp_iadd(&frac, digs);
		if (!isxdigit(*++s)) {
		    if (sawdot != 0 || *s != '.')
			break;
		    sawdot = 4;
		    if (!isxdigit(*++s))
			break;
		}
	    }
	    /* Collect exponent part. */
	    if (*s != 'p' && *s != 'P')
		exp = expoffset;
	    else {
		int expsign = SIGN_PLUS;

		switch (*++s) {
		case '-':
		    expsign = SIGN_MINUS;
		    /*FALLTHRU*/
		case '+':
			s++;
			break;
		}
		exp = 0;
		while (isdigit(*s)) {
		    if (exp < MAXEXP)
			exp = exp * 10 + (*s - '0');
		    s++;
		}
		/* Apply expoffset (zero or negative) to exp. */
		if (exp < MAXEXP) {
		    if (expsign > 0)
			exp += expoffset;
		    else if ((exp += -expoffset) < MAXEXP)
			exp = -exp;
		}
	    }
	    goto setfrac; /* no scale changes */
	}
#endif /*NO_NCEG*/
	while (*++s == '0') /* skip leading 0's */
	    ;
    }
    if (*s == '.') {
	sawdot = 1;
	while (*++s == '0') /* fractional 0's, too */
	    expoffset--;
    }
    if (!isdigit(*s)) /* 'e' and 'E', too */
	goto zero;
    /* At the first nonzero digit; accumulate until exponent. */
    for (;;) {
	digs++;
	expoffset -= sawdot;
	mp_imul(&frac, 10);
	mp_iadd(&frac, *s - '0');
	if (!isdigit(*++s)) {
	    if (sawdot != 0 || *s != '.')
		break;
	    sawdot = 1;
	    if (!isdigit(*++s))
		break;
	}
    }
    /* Collect exponent part. */
    if (*s != 'e' && *s != 'E')
	exp = expoffset;
    else {
	int expsign = SIGN_PLUS;

	switch (*++s) {
	case '-':
	    expsign = SIGN_MINUS;
	    /*FALLTHRU*/
	case '+':
	    s++;
	    break;
	}
	exp = 0;
	while (isdigit(*s)) {
	    if (exp < MAXEXP)
		exp = exp * 10 + (*s - '0');
	    s++;
	}
	/* Apply expoffset (zero or negative) to exp. */
	if (exp < MAXEXP) {
	    if (expsign > 0)
		exp += expoffset;
	    else if ((exp += -expoffset) < MAXEXP)
		exp = -exp;
	}
    }
    /* Now form the number from its pieces. */
    if (exp + digs > X_MAX_DEC_EXP) {
	errno = ERANGE;
	pr = (sign > 0) ? &v_plusinf : &v_minusinf;
    }
    else if (exp + digs < -X_MAX_DEC_EXP) {
	errno = ERANGE;
zero:	pr = (sign < 0) ? &v_minuszero : &v_pluszero;
    }
    else {
	/* Get a multi-word int whose value is 5^exp.
	** This is cheaper than 10^exp:  remember that
	** 10^exp = 2^exp + 5^exp.  We can just add the
	** value of exp to the FP exponent.
	*/
	mp_t *expval = mp_pow(exp);

	if (exp >= 0) {
	    mp_mul3(expval, &frac, &frac);
setfrac:    pr = mp_toi(&frac, &res);
	}
	else
	    mp_div(&frac, expval, &res);
	pr->i_exp += exp;
	pr->i_sign = sign;
	pr = fpemu_final(pr);
    }
    return fpemu_in2ex(pr);
}


/* Routines, data to support internal double-extended to ASCII.  The
** conversion process uses "decimal floating point" (dfp) to do the
** calculations.  The representation consists a fixed number of chunks
** for the magnitude, and an exponent.  A DFP number then consists of
** a bunch of digits, right-justified, 4 (a parameter) per chunk,
** with an implicit decimal point to the right of the low-order digit.
** Because it's DECIMAL floating point, each chunk has a value 0-9999.
** An exponent gives the actual position of the decimal point.  Thus
** 117.3 would be represented by the lower-order chunk containing
** 1173, and an exponent of -1.
*/

/* Number of digits in X_FRACSIZE bits.  The actual number is 20, but
** empirically we have to get another digit to produce enough precision
** so fpemu_atox and fpemu_xtoa are inverses of each other.
*/
#define ODIGITS 21

/* Assume we can fit 4 digits in an unsigned short. */
#define DFP_DIGITS 4			/* digits per chunk */
#define	DFP_MAXVAL 10000		/* 10^DFP_DIGITS */

/* Number of fraction words needed. */
#define DFP_NFRAC (((ODIGITS+1+DFP_DIGITS-1)/DFP_DIGITS)+1)

typedef struct {
    int exp;				/* decimal exponent */
    unsigned short frac[DFP_NFRAC];	/* DFP_DIGITS chunks:  frac[0] is
					** low order part
					*/
} dfpemu_t;


static void
dfpemu_init(pfp)
dfpemu_t * pfp;
/* Initialize decimal FP:  zero fraction, set exponent assuming
** fraction digits are added via dfpemu_adddigs().
*/
{
    int i;

    for (i = DFP_NFRAC-1; i >= 0; --i)
	pfp->frac[i] = 0;
    
    pfp->exp = -(DFP_NFRAC * DFP_DIGITS);
    return;
}


static void
dfpemu_adddigs(pfp, n)
dfpemu_t * pfp;
unsigned short n;
/* Add high-order digits word to decimal FP number.  Round if we're
** about to throw away a low-order word > DFP_MAXVAL/2.
*/
{
    int i;
    int carry = (int) pfp->frac[0] >= DFP_MAXVAL/2;

    /* Move fraction words down one, increment exponent. */
    for (i = 0; i < DFP_NFRAC-1; ++i) {
	pfp->frac[i] = pfp->frac[i+1] + carry;
	carry = 0;
	if (pfp->frac[i] >= DFP_MAXVAL) {
	    carry = 1;
	    pfp->frac[i] = 0;
	}
    }
    pfp->frac[DFP_NFRAC-1] = n;			/* set new high part */
    pfp->exp += DFP_DIGITS;
    return;
}


static void
dfpemu_mul3(px, py, pr)
dfpemu_t * px;
dfpemu_t * py;
dfpemu_t * pr;
/* Multiply decimal FP numbers *px and *py, giving result *pr.
** The result may share space with the operands.
*/
{
    int resexp = px->exp + py->exp;	/* anticipated result exponent */
    unsigned long partials[2*DFP_NFRAC];
    int i, j;

    /* Get partial products for result.  For 4-digit by 4-digit
    ** multiplies, there is room to hold the sum of 42 such
    ** products in a 32-bit unsigned long.  Therefore, collect
    ** partial products, then propagate carries.
    ** Collect the complete set of partial products because
    ** some leading words of the decimal FP values can be zero.
    ** We'll locate the significant part of the product later.
    */

    /* Unroll first iteration so the initialization of the result
    ** array gets folded into it.
    */
    /* for (i = 0; ...) */
    for (j = 0; j < DFP_NFRAC; ++j)
	partials[0+j] = px->frac[0] * py->frac[j];

    for (i = 1; i < DFP_NFRAC; ++i) {
	partials[i+DFP_NFRAC-1] = 0;	/* initialize highest product */
	for (j = 0; j < DFP_NFRAC; ++j)
	    partials[i+j] += px->frac[i] * py->frac[j];
    }
    partials[DFP_NFRAC-1 + DFP_NFRAC-1 + 1] = 0;

    /* Distribute carries. */
    for (i = 0; i <= DFP_NFRAC*2-2; ++i) {
	if (partials[i] >= DFP_MAXVAL) {
	    unsigned long quo = partials[i] / DFP_MAXVAL;
	    partials[i+1] += quo;
	    partials[i] -= quo * DFP_MAXVAL;
	}
    }

    /* Find first non-zero word, but leave DFP_NFRAC words. */
    for (i = DFP_NFRAC*2-1; i >= DFP_NFRAC; --i)
	if (partials[i] != 0) break;
    
    /* Write result to destination.  "i" is index of first word,
    ** and is >= DFP_NFRAC-1.
    */
    for (j = DFP_NFRAC-1; j >= 0; --j, --i)
	pr->frac[j] = partials[i];

    /* Adjust exponent for words not copied. */
    resexp += (i+1) * DFP_DIGITS;
    pr->exp = resexp;

    return;
}



/* Tables of 2^(2^n) and 5^(2^n) for quick exponentiation.  The ones that
** aren't filled in explicitly get filled in by code when they're needed.
*/

static dfpemu_t two2n[X_EXPSIZE+1] = {
    { 0, { 2 } },			/* 2^(2^0) */
    { 0, { 4 } },			/* 2^(2^1) */
    { 0, { 16 } },			/* 2^(2^2) */
    { 0, { 256 } },			/* 2^(2^3) */
    { 0, { 5536, 6 } },			/* 2^(2^4) */
    { 0, { 7296, 9496, 42, } },		/* 2^(2^5) */
};

static dfpemu_t five2n[X_EXPSIZE+1] = {
    { 0, { 5 } },			/* 5^(2^0) */
    { 0, { 25 } },			/* 5^(2^1) */
    { 0, { 625 } },			/* 5^(2^2) */
    { 0, { 625, 39 } },			/* 5^(2^3) */
};


static void
dfpemu_exp(pfp, n, e)
dfpemu_t * pfp;
int n;
unsigned int e;
/* Multiply decimal FP by n ^ e for positive e. */
{
    /* The algorithm does decimal FP multiplies by n ^ (2^i)
    ** for each "i" bit in "e".  We start with low exponents and
    ** work upward, building the exponent table if necessary.
    */
    dfpemu_t *ep;

    switch( n ){
    case 2:	ep = two2n; break;
    case 5:	ep = five2n; break;
    default:	FPEFATAL(gettxt(":566","dfpemu_exp(): unknown factor %d"), n);
    }

    for ( ; e != 0; e >>= 1) {
	if (ep->frac[0] == 0)		/* must compute exponential first */
	    dfpemu_mul3(&ep[-1], &ep[-1], ep);
	if (e & 1)
	    dfpemu_mul3(ep, pfp, pfp);
	++ep;
    }
}


static void
dfpemu_round(pfp, ndig)
dfpemu_t * pfp;
int ndig;
/* Round decimal floating point into ndig-th digit.  Digits are
** counted beginning with 1.
*/
{
    static unsigned short digs[DFP_DIGITS+1] = { 1, 10, 100, 1000, 10000 };
    int dig;
    int wd;

    /* Have to locate where the ndig+1-th digit is, check
    ** it for >= 5, increment ndig-th digit, propagate
    ** carry.  Find "dig" between 1 and DFP_DIGITS.
    */
    for (dig = 1; pfp->frac[DFP_NFRAC-1] >= digs[dig]; ++dig)
	;

    /* High-order word has "dig" digits.  Determine in which word we'll
    ** find the ndig+1-th digit.
    */
    wd = DFP_NFRAC-1 - (ndig + DFP_DIGITS-dig)/DFP_DIGITS;
    dig = DFP_DIGITS-1 - (ndig + DFP_DIGITS-dig) % DFP_DIGITS;
    /* 0 <= wd < DFP_NFRAC; 0 <= dig < DFP_DIGITS */

    /* See if we need to round. */
    if (((unsigned int) (pfp->frac[wd] / digs[dig])) % 10 >= 5) {
	/* Need to round.  Find place to round, propagate carries. */
	if (++dig >= 4) {
	    dig = 0;
	    ++wd;
	}
	if ((pfp->frac[wd] += digs[dig]) >= DFP_MAXVAL) {
	    /* Rounding produced carry. */
	    do {
		pfp->frac[wd] -= DFP_MAXVAL;
		if (++wd >= DFP_NFRAC) break;
		++pfp->frac[wd];
	    } while (pfp->frac[wd] >= DFP_MAXVAL);
	    if (wd >= DFP_NFRAC)	/* carried out of high-order */
		dfpemu_adddigs(pfp, 1);
	}
    }
    return;
}


char *
fpemu_xtoa(x)
fpemu_x_t x;
/* Convert double-extended value to E-format string in memory,
** null terminated.
*/
{
    fpemu_i_t ix;
    char * retval;
    /* Buffer:  DFP_NFRAC*DFP_DIGITS digits, plus:
    **			1 for sign, 1 for '.', 1 for 'e', 5 for exp (-eeee),
    **			1 for null
    */
    static char lbuf[(DFP_NFRAC*DFP_DIGITS)+1+1+5+1];
    char *lbufp;
    int lbsize = 0;

    fpemu_ex2in(&x, &ix);

    switch (ix.i_flag) {
    default:	FPEFATAL(gettxt(":567","fpemu_xtoa():  confused number")); /*NOTREACHED*/
    case IFL_INF:
	retval = ix.i_sign == SIGN_PLUS ? "inf" : "-inf";
	break;
    case IFL_NaN:
	retval = ix.i_sign == SIGN_PLUS ? "NaN" : "-NaN";
	break;
    case IFL_ZERO:
	retval = ix.i_sign == SIGN_PLUS ? "0" : "-0";
	break;
    case IFL_NONE:
    case IFL_DENORM:
    {
	/* Must convert number to digits. */
	int i;
	int bexp;			/* binary exponnent */
	int dexp = 0;			/* decimal exponent */
	static dfpemu_t signif;		/* significand decimal FP */


	dfpemu_init(&signif);

	/* Convert to raw digits, worry about exponent later. */
	for (i = (ODIGITS+1+DFP_DIGITS-1)/DFP_DIGITS; i >= 0; --i) {
	    int j;
	    unsigned long rem = 0;

	    for (j = 4; j >= 0; --j) {
		unsigned long val;

		val = (rem << BITS_PER_MAG) + ix.i_mag[j];
		ix.i_mag[j] = val / DFP_MAXVAL;
		rem = val % DFP_MAXVAL;
	    }
	    dfpemu_adddigs(&signif, (unsigned short) rem);
	}

	/* Find correct exponents. */
	bexp = ix.i_exp - X_EXPBIAS - (X_FRACSIZE + BITS_PER_MAG - 1);
#if X_DENORM_BIAS != 0
	if (ix.i_flag == IFL_DENORM)	/* Adjust for denormalized range. */
	    bexp += X_DENORM_BIAS;
#endif

	/* Multiply significand by suitable exponential to scale the result. */
	if (bexp != 0) {
	    if (bexp > 0)
		dfpemu_exp(&signif, 2, (unsigned int) bexp);
	    else if (bexp < 0) {
		/* x * 2^(-n) = (x * 5^n)/10^n:  multiply by powers of 5,
		** reduce exponent.
		*/
		dexp += bexp;		/* (bexp < 0) */
		dfpemu_exp(&signif, 5, (unsigned int) -bexp);
	    }
	}

	dfpemu_round(&signif, ODIGITS);	/* Round result to ODIGITS digits. */

	/* Convert "signif" to digits in buffer. */
	lbufp = &lbuf[3];		/* Leave room for sign, decimal point,
					** carry
					*/
	lbsize = 0;
	for (i = DFP_NFRAC-1; i > 0; --i) /* skim off zero high-order chunks */
	    if (signif.frac[i] != 0) break;

	for ( ; i >= 0; --i)
	    lbsize +=
		sprintf(&lbufp[lbsize], "%.*d", DFP_DIGITS, signif.frac[i]);

	dexp += signif.exp;

	/* "dexp" now gives exponent relative to the number in the buffer
	** with a decimal point to the right of the right-most digit.
	*/

	/* Remove leading zeroes in buffer. */
	while (lbufp[0] == '0') {
	    ++lbufp;
	    --lbsize;
	}

	/* Rounding has already been done.  Truncate to ODIGITS digits. */
	if (lbsize > ODIGITS) {
	    dexp += lbsize - ODIGITS;
	    lbsize = ODIGITS;
	}

	/* Remove trailing zeroes. */
	while (lbufp[lbsize-1] == '0') {
	    --lbsize;
	    ++dexp;
	}

	/* At this point, "lbufp" points to a buffer of "lbsize" digits,
	** '0'-'9', and "dexp" is the exponent of the number.  Place a
	** sign, a decimal point, and an exponent, if necessary.  Also
	** stick in null termination.
	*/
	if (lbsize > 1) {
	    --lbufp;
	    lbufp[0] = lbufp[1];
	    lbufp[1] = '.';
	    dexp += lbsize - 1;
	    ++lbsize;
	}
	if (ix.i_sign == SIGN_MINUS) {
	    --lbufp;
	    lbufp[0] = '-';
	    ++lbsize;
	}
	if (dexp != 0)
	    (void) sprintf(&lbufp[lbsize], "e%d", dexp);
	else
	    lbufp[lbsize] = '\0';
	retval = lbufp;
	break;
    }
    } /*end switch*/
    return retval;
}


char *
fpemu_xtoh(x)
fpemu_x_t x;
/* Convert double-extended to hexadecimal floating string in memory,
** null terminated.
*/
{
    /* 1 [-], 2 "0x", hex digits, 1 ".", 1 "p", 6 exp (-ddddd), 1 null */
    static char lbuf[1+2+(X_FRACSIZE+3)/4+1+1+6+1] = "-";
    static const char tohex[] = "0123456789abcdef";
    fpemu_i_t ix;
    char *p;
    int i, j;

    fpemu_ex2in(&x, &ix);
    switch (ix.i_flag) {
    default:
	FPEFATAL(gettxt(":0","fpemu_xtoh():  confused number"));
	/*NOTREACHED*/
    case IFL_INF:
	return (ix.i_sign < 0) ? "-inf" : "inf";
    case IFL_NaN:
	return (ix.i_sign < 0) ? "-NaN" : "NaN";
    case IFL_ZERO:
	return (ix.i_sign < 0) ? "-0" : "0";
    case IFL_NONE:
    case IFL_DENORM:
	break;
    }
    /* Generate hex digits from high to low order, leaving room for '.'. */
    lbuf[1] = '0';
    lbuf[2] = 'x';
    p = &lbuf[3];
    for (i = 4; i != 0; i--) {
	/* This loop assumes that BITS_PER_MAG is a multiple of 4. */
	for (j = BITS_PER_MAG - 4; j >= 0; j -= 4)
	    *++p = tohex[(ix.i_mag[i] >> j) & 0xf];
    }
    /* Insert '.' after high order digit. */
    lbuf[3] = lbuf[4];
    lbuf[4] = '.';
    /* Trim off low-order zeroes.  Radix char guarantees this loop stops. */
    for (p = &lbuf[1 + 2 + (X_FRACSIZE + 3) / 4]; *p == '0'; p--)
	;
    /* Append exponent.  +1: true binary point, -4: after first hex digit. */
    sprintf(&p[1], "p%d", ix.i_exp - X_EXPBIAS + 1 - 4);
    if (ix.i_sign == SIGN_MINUS)
	return &lbuf[0];
    return &lbuf[1];
}

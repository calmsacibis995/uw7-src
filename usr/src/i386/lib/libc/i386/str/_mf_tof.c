#ident	"@(#)libc-i386:str/_mf_tof.c	1.4"
/*LINTLIBRARY*/
/*
* _mf_tof.c - Build float value from MkFlt object, x86 specific.
*/

#include "synonyms.h"
#include <stddef.h>
#include <limits.h>
#include <float.h>
#include <errno.h>
#include <math.h>
#include "mkflt.h"

#define BIT(n)		((unsigned long)1 << (n))
#define LOW(n)		(~(~(unsigned long)0 << (n)))

/*
*		x86 float
* _____________________
* |0                31|		f=23 bits + implied bit
* |0      15|0      15|		e= 8 bits, bias=127
* |0  7|0  7|0  7|0  7|			e-max=127
*  <-----f-----><-e->s			e-min=-126 (normalized)
*					e-min=-126-23 (subnormal)
*
* The hardware performs all floating operations in long double.
* This code assumes that [L]DBL_DIG <= 2 * ULDIGS.
*
* Enhanced asm's are used to guarantee that the result of the
* final division or multiplication is correctly rounded to the
* target precision.  This prevents double rounding errors.
*/

#ifndef NO_LONG_DOUBLE

#define TYPE	long double
#define P5MAX	LOG2TO5(LDBL_MANT_DIG)
#define DIGMAX	LDBL_DIG

asm long double
#ifdef __STDC__
divf(long double n, const long double *d, int *cw) /* n/d with single prec. */
#else
divf(n, d, cw)long double n; const long double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	andb	$0xfc, 1(%eax)	// set computation precision to single
	fldcw	(%eax)
	fldt	n
	fldt	(%edx)
	fdivr
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization divf

asm long double
#ifdef __STDC__
mulf(long double n, const long double *d, int *cw) /* n*d with single prec. */
#else
mulf(n, d, cw)long double n; const long double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	andb	$0xfc, 1(%eax)	// set computation precision to single
	fldcw	(%eax)
	fldt	n
	fldt	(%edx)
	fmul
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization mulf

#else /*!NO_LONG_DOUBLE*/

#define TYPE	double
#define P5MAX	LOG2TO5(DBL_MANT_DIG)
#define DIGMAX	DBL_DIG

asm double
#ifdef __STDC__
divf(double n, const double *d, int *cw) /* n/d with single prec. */
#else
divf(n, d, cw)double n; const double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	andb	$0xfc, 1(%eax)	// set computation precision to single
	fldcw	(%eax)
	fldl	n
	fldl	(%edx)
	fdivr
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization divf

asm double
#ifdef __STDC__
mulf(double n, const double *d, int *cw) /* n*d with single prec. */
#else
mulf(n, d, cw)double n; const double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	andb	$0xfc, 1(%eax)	// set computation precision to single
	fldcw	(%eax)
	fldl	n
	fldl	(%edx)
	fmul
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization mulf

#endif /*NO_LONG_DOUBLE*/

static const TYPE fltmax = FLT_MAX;

void
#ifdef __STDC__
_mf_tof(MkFlt *mfp)
#else
_mf_tof(mfp)MkFlt *mfp;
#endif
{
	register BigInt *bp = mfp->bp;
	register unsigned long val;
	register int i;

again:;
	switch (mfp->kind)
	{
	case MFK_REGULAR:
		/*
		* Use native floating hardware when conditions permit
		* a correctly rounded computation.
		*/
		if (mfp->exp < 0)
		{
			if (mfp->exp >= -P5MAX && mfp->ndig <= DIGMAX)
			{
				TYPE x;
				int cw;

				if ((i = mfp->ndig - ULDIGS) > 0)
				{
					x = (_mf_pow10[i] * (long)bp->pkt[0])
						+ (long)bp->pkt[1];
				}
				else
					x = (long)bp->pkt[0];
				x = divf(x, &_mf_pow10[-mfp->exp], &cw);
				if (x > fltmax)
					goto overflow;
				mfp->res.f = x;
				break;
			}
		}
		else if (mfp->ndig <= DIGMAX)
		{
			if (mfp->exp <= P5MAX)
			{
				TYPE x;
				int cw;

				if ((i = mfp->ndig - ULDIGS) > 0)
				{
					x = (_mf_pow10[i] * (long)bp->pkt[0])
						+ (long)bp->pkt[1];
				}
				else
					x = (long)bp->pkt[0];
				if (mfp->exp != 0)
					x = mulf(x, &_mf_pow10[mfp->exp], &cw);
				if (x > fltmax)
					goto overflow;
				mfp->res.f = x;
				break;
			}
			if (mfp->exp + mfp->ndig <= P5MAX + DIGMAX)
			{
				TYPE x;
				int cw;

				if ((i = mfp->ndig - ULDIGS) > 0)
				{
					x = (_mf_pow10[i] * (long)bp->pkt[0])
						+ (long)bp->pkt[1];
				}
				else
					x = (long)bp->pkt[0];
				i = DIGMAX - mfp->ndig;
				x *= _mf_pow10[i];
				x = mulf(x, &_mf_pow10[mfp->exp - i], &cw);
				if (x > fltmax)
					goto overflow;
				mfp->res.f = x;
				break;
			}
		}
		/*
		* Significand or exponent (or both) are too big for a safe use
		* of the native floating hardware.  One Ulong is sufficient.
		*/
		mfp->want = 1;
		if ((bp = _mf_10to2(mfp)) == 0)
		{
			if (mfp->kind != MFK_REGULAR)
				goto again;
			goto error;
		}
#ifndef NO_NCEG_FPE
		/*FALLTHROUGH*/
	case MFK_HEXSTR:
#endif
		/*
		* Adjust binary point to just below the high order bit.
		*/
		val = bp->next * ULBITS - 1;
		if (mfp->exp <= 0)
			mfp->exp += val;
		else if ((val += mfp->exp) < mfp->exp)
			goto overflow;
		else
			mfp->exp = val;
		/*
		* Check for sure overflow or underflow.
		*
		* Set val so that (bp->pkt[0] & val) is the 1/2 ulp bit.
		*/
		if (mfp->exp > 127)
			goto overflow;
		if (mfp->exp < -126)
		{
			if (mfp->exp < -126 - 24) /* round may add 1 to exp */
				goto underflow;
			i = -mfp->exp - 127 + 8;
			val = BIT(i);
		}
		else /* normalized range */
		{
			val = BIT(7);
		}
		/*
		* Round as indicated by the value and FLT_ROUNDS.
		*/
#ifndef NO_FLT_ROUNDS
		if (FLT_ROUNDS != 1)	/* not to nearest */
		{
			/*
			* 2 = round toward +inf, 3 = round toward -inf.
			*/
			if (FLT_ROUNDS - 2 == mfp->sign)
			{
				if (bp->pkt[0] & val)
					goto round;
				goto rnd_ifnonzero;
			}
		}
		else
#endif
		if (bp->pkt[0] & val)	/* 1/2 ulp bit is set */
		{
			register unsigned long ul = val << 1;

			if (ul == 0)	/* val is BIT(31) */
				goto rnd_ifnonzero;
			else if (bp->pkt[0] & ul)	/* currently odd */
			{
			round:;
				if ((bp->pkt[0] += val) < val)	/* overflow */
				{
					bp->pkt[0] = BIT(31);
					mfp->exp++;
				}
			}
			else
			{
				register int n;

			rnd_ifnonzero:;
				if (bp->pkt[0] & (val - 1))
					goto round;
				n = i;
				while (++n < bp->next)
				{
					if (bp->pkt[n] != 0)
						goto round;
				}
			}
		}
		/*
		* Rounding complete.  Jam in the bits.
		* Must recheck for overflow and underflow.
		*/
		if (mfp->exp > 127)
			goto overflow;
		if (mfp->exp < -126)
		{
			if (mfp->exp < -126 - 23)
				goto underflow;
			i = -mfp->exp - 127 + 9;
			mfp->res.ul[0] = bp->pkt[0] >> i;
		}
		else /* normalized range */
		{
			mfp->res.ul[0] = ((mfp->exp + 127) << 23)
				| ((bp->pkt[0] >> 8) & LOW(23));
		}
		break;
	case MFK_OVERFLOW:
	overflow:;
		errno = ERANGE;
#ifndef NO_CI4
		if (_lib_version == c_issue_4)
		{
			mfp->res.f = FLT_MAX;
			break;
		}
#endif
#ifndef NO_NCEG_FPE
		/*FALLTHROUGH*/
	case MFK_INFINITY:
#endif
		mfp->res.ul[0] = LOW(8) << 23;
		break;
	case MFK_UNDERFLOW:
	underflow:;
		errno = ERANGE;
	error:;
		mfp->sign = 0;
	default:
	case MFK_ZERO:
		mfp->res.ul[0] = 0;
		break;
#ifndef NO_NCEG_FPE
	case MFK_DEFNAN:
		bp->pkt[0] = 0;
		/*FALLTHROUGH*/
	case MFK_VALNAN:
		val = bp->pkt[0] | BIT(31);	/* force quiet NaN */
		mfp->res.ul[0] = (val >> 9) | (LOW(8) << 23);
		break;
#endif
	}
	if (mfp->sign)
		mfp->res.uc[3] |= BIT(7);
	if (bp != 0 && bp->allo)
		free(bp);
}

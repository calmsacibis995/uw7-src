#ident	"@(#)libc-i386:str/_mf_tod.c	1.3"
/*LINTLIBRARY*/
/*
* _mf_tod.c - Build double value from MkFlt object, x86 specific.
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
*		x86 double
* _________________________________________
* |0                31|0                31|
* |0      15|0      15|0      15|0      15|
* |0  7|0  7|0  7|0  7|0  7|0  7|0  7|0  7|
*  <--------------f---------------><--e->s
*
*	f=52 bits + implied bit
*	e=11 bits, bias=1023
*		e-max=1023
*		e-min=-1022 (normalized)
*		e-min=-1022-52 (subnormal)
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
divd(long double n, const long double *d, int *cw) /* n/d with double prec. */
#else
divd(n, d, cw)long double n; const long double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	orb	$0x02, 1(%eax)	// set computation
	andb	$0xfe, 1(%eax)	// precision to double
	fldcw	(%eax)
	fldt	n
	fldt	(%edx)
	fdivr
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization divd

asm long double
#ifdef __STDC__
muld(long double n, const long double *d, int *cw) /* n*d with double prec. */
#else
muld(n, d, cw)long double n; const long double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	orb	$0x02, 1(%eax)	// set computation
	andb	$0xfe, 1(%eax)	// precision to double
	fldcw	(%eax)
	fldt	n
	fldt	(%edx)
	fmul
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization muld

#else /*!NO_LONG_DOUBLE*/

#define TYPE	double
#define P5MAX	LOG2TO5(DBL_MANT_DIG)
#define DIGMAX	DBL_DIG

asm double
#ifdef __STDC__
divd(double n, const double *d, int *cw) /* n/d with double prec. */
#else
divd(n, d, cw)double n; const double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	orb	$0x02, 1(%eax)	// set computational
	andb	$0xfe, 1(%eax)	// precision to double
	fldcw	(%eax)
	fldl	n
	fldl	(%edx)
	fdivr
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization divd

asm double
#ifdef __STDC__
muld(double n, const double *d, int *cw) /* n*d with double prec. */
#else
muld(n, d, cw)double n; const double *d; int *cw;
#endif
{
%mem n, d, cw;
	movl	cw, %eax
	movl	d, %edx
	fstcw	(%eax)
	movl	(%eax), %ecx
	orb	$0x02, 1(%eax)	// set computational
	andb	$0xfe, 1(%eax)	// precision to double
	fldcw	(%eax)
	fldl	n
	fldl	(%edx)
	fmul
	movl	%ecx, (%eax)
	fldcw	(%eax)		// restore previous
}
 #pragma asm full_optimization muld

#endif /*NO_LONG_DOUBLE*/

static const TYPE dblmax = DBL_MAX;

void
#ifdef __STDC__
_mf_tod(MkFlt *mfp)
#else
_mf_tod(mfp)MkFlt *mfp;
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
				x = divd(x, &_mf_pow10[-mfp->exp], &cw);
				if (x > dblmax)
					goto overflow;
				mfp->res.d = x;
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
					x = muld(x, &_mf_pow10[mfp->exp], &cw);
				if (x > dblmax)
					goto overflow;
				mfp->res.d = x;
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
				x = muld(x, &_mf_pow10[mfp->exp - i], &cw);
				if (x > dblmax)
					goto overflow;
				mfp->res.d = x;
				break;
			}
		}
		/*
		* Significand or exponent (or both) are too big for a safe use
		* of the native floating hardware.  Two Ulongs are sufficient.
		*/
		mfp->want = 2;
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
		* Set val and i so that (bp->pkt[i] & val) is the 1/2 ulp bit.
		*/
		if (mfp->exp > 1023)
			goto overflow;
		if (mfp->exp < -1022)
		{
			if (mfp->exp < -1022 - 53) /* round may add 1 to exp */
				goto underflow;
			if ((i = -mfp->exp - 1023 + 11) > 31)	/* one pkt */
			{
				val = BIT(i - 32);
				i = 0;
			}
			else /* two pkts */
			{
				val = BIT(i);
				i = 1;
			}
		}
		else /* normalized range */
		{
			val = BIT(10);
			i = 1;
		}
		/*
		* Round as indicated by the value and FLT_ROUNDS.
		* This code knows that i is either 0 or 1.
		*/
#ifndef NO_FLT_ROUNDS
		if (FLT_ROUNDS != 1)	/* not to nearest */
		{
			/*
			* 2 = round toward +inf, 3 = round toward -inf.
			*/
			if (FLT_ROUNDS - 2 == mfp->sign)
			{
				if (bp->pkt[i] & val)
					goto round;
				goto rnd_ifnonzero;
			}
		}
		else
#endif
		if (bp->pkt[i] & val)	/* 1/2 ulp bit is set */
		{
			register unsigned long ul = val << 1;

			if (ul == 0)	/* val is BIT(31) */
			{
				if (i != 0 && bp->pkt[0] & BIT(0)) /* is odd */
					goto round;
				goto rnd_ifnonzero;
			}
			else if (bp->pkt[i] & ul)	/* currently odd */
			{
			round:;
				if ((bp->pkt[i] += val) < val)	/* overflow */
				{
					if (--i < 0 || ++bp->pkt[0] == 0)
					{
						bp->pkt[0] = BIT(31);
						mfp->exp++;
					}
				}
			}
			else
			{
				register int n;

			rnd_ifnonzero:;
				if (bp->pkt[i] & (val - 1))
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
		if (mfp->exp > 1023)
			goto overflow;
		if (mfp->exp < -1022)
		{
			if (mfp->exp < -1022 - 52)
				goto underflow;
			if ((i = -mfp->exp - 1023 + 12) > 31)	/* one pkt */
			{
				mfp->res.ul[1] = 0;
				mfp->res.ul[0] = bp->pkt[0] >> (i - 32);
			}
			else /* two pkts */
			{
				val = bp->pkt[0];
				mfp->res.ul[1] = val >> i;
				mfp->res.ul[0] = (val << (32 - i))
					| (bp->pkt[1] >> i);
			}
		}
		else /* normalized range */
		{
			val = bp->pkt[0];
			mfp->res.ul[1] = ((mfp->exp + 1023) << 20)
				| ((val >> 11) & LOW(20));
			mfp->res.ul[0] = (val << 21) | (bp->pkt[1] >> 11);
		}
		break;
	case MFK_OVERFLOW:
	overflow:;
		errno = ERANGE;
#ifndef NO_CI4
		if (_lib_version == c_issue_4)
		{
			mfp->res.d = FLT_MAX;
			break;
		}
#endif
#ifndef NO_NCEG_FPE
		/*FALLTHROUGH*/
	case MFK_INFINITY:
#endif
		mfp->res.ul[0] = 0;
		mfp->res.ul[1] = LOW(11) << 20;
		break;
	case MFK_UNDERFLOW:
	underflow:;
		errno = ERANGE;
	error:;
		mfp->sign = 0;
	default:
	case MFK_ZERO:
		mfp->res.ul[0] = 0;
		mfp->res.ul[1] = 0;
		break;
#ifndef NO_NCEG_FPE
	case MFK_DEFNAN:
		bp->next = 1;
		bp->pkt[0] = 0;
		/*FALLTHROUGH*/
	case MFK_VALNAN:
		val = bp->pkt[0] | BIT(31);	/* force quiet NaN */
		mfp->res.ul[1] = (val >> 12) | (LOW(11) << 20);
		mfp->res.ul[0] = val << 20;
		if (bp->next > 1)
			mfp->res.ul[0] |= bp->pkt[1] >> 12;
		break;
#endif
	}
	if (mfp->sign)
		mfp->res.uc[7] |= BIT(7);
	if (bp != 0 && bp->allo)
		free(bp);
}

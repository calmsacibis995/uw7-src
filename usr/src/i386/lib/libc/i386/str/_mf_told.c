#ident	"@(#)libc-i386:str/_mf_told.c	1.4"
/*LINTLIBRARY*/
/*
* _mf_told.c - Build long double value from MkFlt object, x86 specific.
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
*		x86 long double
* _____________________________________________________________
* |0                31|0                31|0                31|
* |0      15|0      15|0      15|0      15|0      15|0      15|
* |0  7|0  7|0  7|0  7|0  7|0  7|0  7|0  7|0  7|0  7|0  7|0  7|
*  <------------------f------------------> <---e-->s <padding>
*
*	f=64 bits (no implied bit)
*	e=15 bits, bias=16383
*		e-max=16383
*		e-min=-16382 (normalized)
*		e-min=-16382-63 (subnormal)
*
* The hardware performs all floating operations in long double.
*
* This code uses symbolic values and expressions, but frequently
* knows the actual answers or key relationships.  Some are:
*
*	LDBL_DIG <= 2*ULDIGS
*	LOG2TO5(LDBL_MANT_DIG) == 27
*/

#ifndef NO_LONG_DOUBLE

void
#ifdef __STDC__
_mf_told(MkFlt *mfp)
#else
_mf_told(mfp)MkFlt *mfp;
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
		* correctly rounded computation.
		*/
		if (mfp->exp < 0)
		{
			if (mfp->exp >= -LOG2TO5(LDBL_MANT_DIG)
				&& mfp->ndig <= LDBL_DIG)
			{
				long double ld;

				if ((i = mfp->ndig - ULDIGS) > 0)
				{
					ld = (_mf_pow10[i] * (long)bp->pkt[0])
						+ (long)bp->pkt[1];
				}
				else
					ld = (long)bp->pkt[0];
				mfp->res.ld = ld / _mf_pow10[-mfp->exp];
				break;
			}
		}
		else if (mfp->ndig <= LDBL_DIG)
		{
			if (mfp->exp <= LOG2TO5(LDBL_MANT_DIG))
			{
				long double ld;

				if ((i = mfp->ndig - ULDIGS) > 0)
				{
					ld = (_mf_pow10[i] * (long)bp->pkt[0])
						+ (long)bp->pkt[1];
				}
				else
					ld = (long)bp->pkt[0];
				if (mfp->exp != 0)
					ld *= _mf_pow10[mfp->exp];
				mfp->res.ld = ld;
				break;
			}
			else if (mfp->exp + mfp->ndig
				<= LOG2TO5(LDBL_MANT_DIG) + LDBL_DIG)
			{
				long double ld;

				if ((i = mfp->ndig - ULDIGS) > 0)
				{
					ld = (_mf_pow10[i] * (long)bp->pkt[0])
						+ (long)bp->pkt[1];
				}
				else
					ld = (long)bp->pkt[0];
				i = LDBL_DIG - mfp->ndig;
				mfp->res.ld = (ld * _mf_pow10[i])
					* _mf_pow10[mfp->exp - i];
				break;
			}
		}
		/*
		* Significand or exponent (or both) are too big for a safe use
		* of the native floating hardware.  Three Ulongs are sufficient.
		*/
		mfp->want = 3;
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
		if (mfp->exp > 16383)
			goto overflow;
		if (mfp->exp < -16382)
		{
			if (mfp->exp < -16382 - 64) /* round may add 1 to exp */
				goto underflow;
			if ((i = -mfp->exp - 16383) > 31)	/* one pkt */
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
			val = BIT(31);
			i = 2;
		}
		/*
		* Round as indicated by the value and FLT_ROUNDS.
		* This code knows that i is [0,2].
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
				if (i != 0 && bp->pkt[i - 1] & BIT(0)) /* odd */
					goto round;
				goto rnd_ifnonzero;
			}
			else if (bp->pkt[i] & ul)	/* currently odd */
			{
			round:;
				if ((bp->pkt[i] += val) < val)	/* overflow */
				{
					if (--i < 0 || (++bp->pkt[i] == 0 &&
						(--i < 0 || ++bp->pkt[0] == 0)))
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
		if (mfp->exp > 16383)
			goto overflow;
		if (mfp->exp < -16382)
		{
			if (mfp->exp < -16382 - 63)
				goto underflow;
			if ((i = -mfp->exp - 16383 + 1) > 31)	/* one pkt */
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
			mfp->res.us[4] = 0;
		}
		else /* normalized range */
		{
			mfp->res.us[4] = mfp->exp + 16383;
			mfp->res.ul[1] = bp->pkt[0];
			mfp->res.ul[0] = bp->pkt[1];
		}
		break;
	case MFK_OVERFLOW:
	overflow:;
		errno = ERANGE;
#ifndef NO_CI4
		if (_lib_version == c_issue_4)
		{
			mfp->res.ld = FLT_MAX;
			break;
		}
#endif
#ifndef NO_NCEG_FPE
		/*FALLTHROUGH*/
	case MFK_INFINITY:
#endif
		mfp->res.ul[0] = 0;
		mfp->res.ul[1] = BIT(31);
		mfp->res.us[4] = LOW(15);
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
		mfp->res.us[4] = 0;
		break;
#ifndef NO_NCEG_FPE
	case MFK_DEFNAN:
		bp->next = 1;
		bp->pkt[0] = 0;
		/*FALLTHROUGH*/
	case MFK_VALNAN:
		bp->pkt[0] |= BIT(31) | BIT(30); /* force quiet NaN */
		mfp->res.us[4] = LOW(15);
		mfp->res.ul[1] = bp->pkt[0];
		mfp->res.ul[0] = 0;
		if (bp->next > 1)
			mfp->res.ul[0] = bp->pkt[1];
		break;
#endif
	}
	if (mfp->sign)
		mfp->res.uc[9] |= BIT(7);
	if (bp != 0 && bp->allo)
		free(bp);
}

#endif /*NO_LONG_DOUBLE*/

#ident	"@(#)libc-i386:gen/ecvt.c	1.12.2.1"
/*LINTLIBRARY*/
/*
 *	ecvt converts to decimal
 *	the number of digits is specified by ndigit
 *	decpt is set to the position of the decimal point
 *	sign is set to 0 for positive, 1 for negative
 *
 */
#ifndef DSHLIB
#ifdef __STDC__
	#pragma weak ecvt = _ecvt
	#pragma weak fcvt = _fcvt
#endif
#endif
#include "synonyms.h"
#include <math.h>
#include <nan.h>
#include <values.h>


/* definitions needed on M32 if unix 5.0 standard file is used */
#ifndef BITS
#define BITS(type)	(BITSPERBYTE * (int)sizeof(type))
#endif
#ifndef MINDOUBLE
#define MINDOUBLE	4.94065645841246544e-324
#endif
#ifndef MINFLOAT
#define MINFLOAT	((float)1.40129846432481707e-45)
#endif


#define	NMAX	((DSIGNIF * 3 + 19)/10) /* restrict max precision */
#define	NDIG	((NMAX/4)*4 + 8)

/* The following macro converts a binary exponent to a decimal one. */
#define E2to10(x)	(((x)*301029L + 500000000L)/1000000L - 500)

/* LONGBITS is the number of bits in a long NOT including the sign bit */
#define	LONGBITS	(BITS(long) - 1)
#define	LONGDIGITS	E2to10(LONGBITS)
#define LOW(x)		((1 << (x)) - 1)

#if i386
#define EXP_SIZE	11 
#define EXP_BIAS	1024
typedef union { 
	double d;
	long l1[2];
} dlcast;
/*	Precisely why BINEXP must be off by 2 is locked in the mind
	of a former developer.  Nonetheless, it breaks if changed */
#define BINEXP(X)	(((((dlcast *)&(X))->l1[1] >> 20)\
				& LOW(EXP_SIZE)) - EXP_BIAS + 2)
#define UNDER(X)	(BINEXP(X) == -EXP_BIAS + 2)
#endif

extern char *cvt(), *memset();
extern char *_iba_ltostr();

char *
ecvt(value, ndigit, decpt, sign)
double	value;
int	ndigit, *decpt, *sign;
{
	return (cvt(value, ndigit, decpt, sign, 0));
}

char *
fcvt(value, ndigit, decpt, sign)
double	value;
int	ndigit, *decpt, *sign;
{
	return (cvt(value, ndigit, decpt, sign, 1));
}

static char buf[NDIG];

static char *
cvt(value, ndigit, decpt, sign, f_flag)
double value;
register int ndigit;
int	*sign, f_flag;
int	*decpt;
{
	register char *end_ptr;
	char *begin_ptr;
	int tdecpt, adjustment;
	long long1, long2;
#if i386
	long pval1 = ((dlcast *)&value)->l1[1];
	long pval2 = ((dlcast *)&value)->l1[0];
#endif

	/* The test below precisely immitates the test if (value == 0.0)   */
	/* The reason for the awkward code is performance. Two integer     */
	/* compare are much faster than floating point trap. 		   */
	/* Note that tests for positive and negative			   */
	/* representation of zero must be made.				   */

#if i386
	if ((pval2 == 0) && ((pval1 == 0) || (pval1 == 0x80000000))) {
#endif
		*sign = 0;
		*decpt = !f_flag;
		if (ndigit > NMAX)
			ndigit = NMAX;
		(void) memset(buf, '0', ndigit);
		buf[ndigit] = '\0';
		return(buf);
	}
	/*
	   Non-zero value.  Estimate "tdecpt", the number of digits
	   before the decimal point, erring on the low side.  This
	   value may be negative.
	*/
	if (UNDER(value))  {
		/* if denormalized number */
		(void)frexp(value, &tdecpt);
		tdecpt = E2to10(tdecpt - 1) + 1;
	} else {
		int temp = BINEXP(value);
		tdecpt = E2to10(temp - 1) + 1;
	}
	if (f_flag)	/* Fortran f format conversion */
		ndigit += tdecpt;
	if (ndigit < 0)
		ndigit = 0;

	/*
	   Decide how many digits to try to convert
	   ("length_wanted").  This will be the number
	   of digits desired plus a rounding digit (i.e. ndigit+1).
	   If that is too much to fit in a long (allowing one
	   bit margin since "tvalue" may be up to one bit larger
	   than revealed by "tdecpt"), "length_wanted" is reduced
	   so it will fit in a long and the remaining number of
	   digits needed is left in "excess".

	   Next compute "shift", the number of decimal places by
	   which tvalue must be shifted left so that "length_wanted"
	   digits (and possibly one more) will be in front of the
	   decimal point.

	   Convert the number into one or two longs.
	*/

	{   register int length_wanted, excess;
	    {	register int shift;

		if ((excess = (length_wanted = ndigit + 1) -
				E2to10(LONGBITS - 1)) > 0) {
			length_wanted = E2to10(LONGBITS - 1);
			if (excess > LONGDIGITS)
				excess = LONGDIGITS;
		}
		shift = length_wanted - tdecpt;

		long1 = _iba_dtop((long *) &value, shift, excess, &long2, sign);
	    }

	/*
	   Convert the number in "long1", and make the
	   necessary adjustments if we underestimated the number
	   of digits before the decimal point.
	*/

	    {

		end_ptr = &buf[4] + length_wanted;
		adjustment = end_ptr -
			(begin_ptr = _iba_ltostr(long1, end_ptr));

		if ((adjustment -= length_wanted) > 0) {
			tdecpt += adjustment;
			if (f_flag)
				ndigit += adjustment;
		}
	    }


	/*
	   Convert the second long, and fill out with leading
	   zeroes until "excess" digits are obtained.
	*/

	    if (excess > 0) {
		register char *p;

		p = _iba_ltostr(long2, end_ptr + excess);

		while (end_ptr < p)
			*--p = '0';
	    }
	}

	/*
	   Round result.  If the rightmost converted digit is 5 or
	   more, 1 is added to the next digit to the left; the carry
	   propagates as long as 9's are encountered; if the number
	   is all 9's, it turns to 100000....., and the position of
	   the decimal point is adjusted.
	*/

	{	register char *p, *bp = begin_ptr;

		if (ndigit > NMAX)
			ndigit = NMAX;
		p = end_ptr = bp + ndigit;

		if (*p > '5' || (*p == '5' && 
			((((int)*(p - 1)) % 2) || adjustment > 0)))
			do {
				*p = '0';
				if (p == bp) { /* all 9's */
					*--bp = '1';
					++tdecpt;
					if (!f_flag || ndigit == NMAX)
						--end_ptr;
					break;
				}
			} while (++*--p > '9');

		*end_ptr = '\0';

		*decpt = tdecpt;
		return(bp);
	}
}

#ident	"@(#)libc-i386:gen/ldexp.c	1.8"
/*LINTLIBRARY*/
/*
 *	double ldexp (value, exp)
 *		double value;
 *		int exp;
 *
 *	Ldexp returns value * 2**exp, if that result is in range.
 *	
 *	If value is a NaN, returns error EDOM and value NaN.  If value is
 *	also a signalling NaN, raises an invalid op exception.
 *
 *	On underflow, return error ERANGE and value 0.
 *
 *	In -Xt mode,
 *	On overflow, return error ERANGE and value +-HUGE.
 *
 *	In -Xa and -Xc modes,
 *	On overflow, return error ERANGE and value +-HUGE_VAL.
 */

#include "synonyms.h"
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"
/* Largest signed long int power of 2 */
#define MAXSHIFT	(BITSPERBYTE * sizeof(long) - 2)

asm	double
xldexp(x,n)
{
%mem	x; reg n;
	pushl	n
	fildl	(%esp)
	popl	%eax
	fldl	x
	fscale
	fstp	%st(1)
%mem	x,n;
	fildl	n
	fldl	x
	fscale
	fstp	%st(1)
}

#pragma partial_optimization xfrexp

extern double frexp();

double
ldexp(value, exp)
double value;
int exp;
{
	int old_exp;

	if (NANorINF(value)) {
		if (!INF(value)){        /* ie x is NaN */

			/* raise an exception if not a quiet NaN */
			SigNAN(value);

			errno=EDOM;
			return (value);
		} else {
			/* inf */
			errno = ERANGE;
			if (_lib_version == c_issue_4)
				return (value < 0 ? -HUGE : HUGE);
			else
				return (value < 0 ? -HUGE_VAL : HUGE_VAL);
		}
	}
	if (exp == 0 || value == 0.0) /* nothing to do for zero */
		return (value);
	(void) frexp(value, &old_exp);
	if (exp > 0) {
		if ( (exp > (MAXINT-MAXBEXP))
		/* guard against integer overflow in next addition */
	    	|| ((exp + old_exp) > MAXBEXP) ) {
		/* we have floating point overflow condition */

			errno = ERANGE;
			if (_lib_version == c_issue_4)
				return (value < 0 ? -HUGE : HUGE);
			else
				return (value < 0 ? -HUGE_VAL : HUGE_VAL);
		}
		return (xldexp(value,exp));
	}
	if ( (exp < -(MAXINT-MAXBEXP)) ||
		/* guard against integer overflow in next addition */
		(exp + old_exp < MINBEXP) ) { /* underflow */
		errno = ERANGE;
		return (0.0);
	}
	return (xldexp(value,exp));
}

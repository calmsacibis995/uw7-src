#ident	"@(#)libc-i386:gen/ldexpl.c	1.4"
/*LINTLIBRARY*/
/*
 *	long double ldexpl (value, exp)
 *		long double value;
 *		int exp;
 *
 *	Ldexp returns value * 2**exp, if that result is in range.
 * 
 *      If value is a NaN, returns error EDOM and value NaN.  If value is
 *      also a signalling NaN, raises an invalid op exception.
 *
 *      On underflow, return error ERANGE and value 0.
 *
 *      In -Xt mode,
 *      On overflow, return error ERANGE and value +-HUGE.
 *
 *      In -Xa and -Xc modes,
 *      On overflow, return error ERANGE and value +-HUGE_VAL.
 */

#include "synonyms.h"
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"
/* Largest signed long int power of 2 */
#define MAXSHIFT	(BITSPERBYTE * sizeof(long) - 2)

asm	long double
xldexpl(x,n)
{
%mem	x; reg n;
	pushl	n
	fildl	(%esp)
	popl	%eax
	fldt	x
	fscale
	fstp	%st(1)
%mem	x,n;
	fildl	n
	fldt	x
	fscale
	fstp	%st(1)
}

#pragma partial_optimization xldexpl

extern long double frexpl();

long double
ldexpl(value, exp)
long double value;
int exp;
{
	int old_exp;
	long double ret;

        if (NANorINFLD(value)) {
		if (!INFLD(value)){        /* ie x is NaN */

                	/* raise an exception if not a quiet NaN */
	                SigNANLD(value);

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
	(void) frexpl(value, &old_exp);
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
		return (xldexpl(value,exp));
	}
	if ( (exp < -(MAXINT-MAXBEXP)) ||
		/* guard against integer overflow in next addition */
		(exp + old_exp < MINBEXP) ) { /* underflow */
		errno = ERANGE;
		return (0.0);
	}
	return (xldexpl(value,exp));
}

#ident	"@(#)libm:i386/sqrtf.c	1.4"

/*LINTLIBRARY*/

/*
 * sqrtf returns the square root of its single-precision argument
 * using the 80387 processor or the 80387 emulator.
 * If x is a NaN, returns error EDOM and value NaN.  If x is also a
 * signalling NaN, raise an invalid op exception.
 *
 * In -Xt mode,
 * if x < 0, raise an invalid op exception, return error EDOM and value 0.
 *
 * In -Xa and -Xc modes,
 * if x < 0, raise an invalid op exception, return error EDOM and value NaN.
 */

#include "synonyms.h"
#include <errno.h>
#include <math.h>			/* temporary location	*/
#include <values.h>
#include "fpparts.h"

asm	float xsqrtf(float x)
{
%mem	x;
	flds	x
	fsqrt
}

#pragma partial_optimization xsqrtf

float
sqrtf(float x)
{
	float xsqrtf(float);

	if (FNANorINF(x)) {
		if (FINF(x)) {
			if (x>0)	/* x == +inf */
				return x;
			/* x == -inf - fall through to x <=0 check below */
		} else			/* x == NaN */
			return _float_domain(x,0.0F,x,"sqrtf",5);
	}

	if (x <= 0.0F) {
		float zero = 0.0F;
		float ret;
		if (!x)		/* x == 0 */
			return x;
		/* raise invalid op exception */
		zero /= zero;

		if (_lib_version == c_issue_4)
			ret = 0.0F;
		else
			FMKNAN(ret);

		return _float_domain(x,0.0F,ret,"sqrtf",5);
	}
	return(xsqrtf(x));
}

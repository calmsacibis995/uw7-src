#ident	"@(#)libm:i386/sqrt.c	1.7"

/*LINTLIBRARY*/

/*
 * sqrt returns the square root of its double-precision argument
 *
 * using the [34]87 processor or the [34]87 emulator.
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
#include <math.h>
#include <values.h>
#include "fpparts.h"

asm	double xsqrt(x)
{
%mem	x;
	fldl	x
	fsqrt
}

#pragma partial_optimization xsqrt

double
sqrt(x)
double x;
{

	if (NANorINF(x)) {
		if (INF(x)) {
			if (x>0)	/* x == +oo */
				return x;
			/* x == -oo - fall through to x <=0 check below */
		} else			/* x == NaN */
			return _domain_err(x,0.0,x,"sqrt",4);
	}

	if (x <= 0.0) {
		double ret=x;
		double q1=0.0;
		double q2=0.0;
		if (x == 0.0)
			return x;

		/* raise invalid op exception */
		q1 /= q2;
		if (_lib_version == c_issue_4)
			ret = 0.0;
		else
			MKNAN(ret);

		return _domain_err(x,0.0,ret,"sqrt",4);
	}

	return(xsqrt(x));
}

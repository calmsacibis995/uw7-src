#ident	"@(#)libm:i386/fmod.c	1.6"
/*LINTLIBRARY*/

/* fmod(x,y)
 * Return f =x-n*y, for some integer n, where f has same sign
 * as x and |f| < |y|
 *
 * If x or y are NaN, return error EDOM and value NaN.  If x or y are also
 * signalling NaNs, raise an invalid op exception.
 *
 * If x is +-inf raise an invalid op exception and return error EDOM
 * and value NaN.
 *
 * If y is +-inf or x is 0, return x.
 *
 * In -Xt mode,
 * If y is 0, raise the invalid op exception and return error EDOM and value
 * x.
 *
 * In -Xa and -Xc modes,
 * if y is 0, raise the invalid op exception and return error EDOM and value
 * NaN.
 */

#include "synonyms.h"
#include <math.h>
#include <values.h>
#include "fpparts.h"

asm double xfmod(x, y)
{
%mem	x,y; lab loop;
	fldl	y
	fldl	x
loop:
	fprem
	fstsw	%ax
	testl	$0x400,%eax
	jne	loop
	ffree	%st(1)
}

#pragma partial_optimization	xfmod

double	fmod(x, y)
double	x, y;
{

	double xfmod();
	double ret;

	if (NANorINF(x)|| (NANorINF(y) && !INF(y)) || y == 0.0 ) {
		double q1 = 0.0;
		double q2 = 0.0;

		if (NANorINF(x) && !INF(x)) 		/* x is a NaN */
			ret = x;
		else if (NANorINF(y) && !INF(y))	/* y is a NaN */
			ret = y;
		else {
			/* raise exception */
			q1 /= q2;
			ret = x;
			if (_lib_version != c_issue_4 || y != 0.0)
				MKNAN(ret);
		}

		return _domain_err(x,y,ret,"fmod",4);

	} else if (x == 0.0 || NANorINF(y))
		return x;

	return xfmod(x,y);
}

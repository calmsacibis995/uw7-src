#ident	"@(#)libm:i386/fmodf.c	1.5"
/*LINTLIBRARY*/

/* fmodf(x,y) - single precision version
 * Return f =x-n*y, for some integer n, where f has same sign
 * as x and |f| < |y|
 *
 *  for y == 0, f = 0 
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

asm float xfmodf(float x, float y)
{
%mem	x,y;
	flds	y
	flds	x
.dorem:
	fprem
	fstsw	%ax
	testl	$0x400,%eax
	jne	.dorem
	ffree	%st(1)
}

#pragma partial_optimization	xfmodf

float	fmodf(float x, float y)
{

	float xfmodf(float,float);
	if (FNANorINF(x)|| (FNANorINF(y) && !FINF(y)) || !y ) {
		float q1 = 0.0F;
		float q2 = 0.0F;
		float ret;

		if (FNANorINF(x) && !FINF(x)) 		/* x is a NaN */
			ret = x;
		else if (FNANorINF(y) && !FINF(y))	/* y is a NaN */
			ret = y;
		else {
			/* raise exception */
			q1 /= q2;
			ret = x;
			if (_lib_version != c_issue_4 || y)
				FMKNAN(ret);
		}

		return _float_domain(x,y,ret,"fmodf",5);

	} else if (!x || NANorINF(y))
		return x;
	return xfmodf(x,y);
}

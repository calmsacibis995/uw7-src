#ident	"@(#)libm:i386/rem.c	1.5"
/*LINTLIBRARY*/


/* remainder(x,y)
 * Return x rem y =x-n*y, n=[x/y] rounded (rounded to even 
 * in the half way case)
 *
 *  Domain error occurs for y == 0 or NaN, x == inf or NaN
 */

#ifdef __STDC__
	#pragma weak remainder = _remainder
#endif

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>
#include <sys/fp.h>

#include "fpparts.h"
#define P754_NOFAULT 1
#include <ieeefp.h>

asm double xrem(x, y)
{
%mem	x,y;
	fldl	y
	fldl	x
.dorem1:
	fprem1
	fstsw	%ax
	testl	$0x400,%eax
	jne	.dorem1
	ffree	%st(1)
}


double	
remainder(x, y)
double	x, y;
{

	double	hy, y2, t, t1;
	double	xrem();
	short	k;
	long	n;
	fp_except	mask;
	unsigned short	xexp, yexp, nx, nf, sign;

	extern int _fp_hw;  /* detect hardware presence */

	if (NANorINF(x)|| (NANorINF(y) && !INF(y)) || !y ) {
		double ret;
		double q1 = 0.0;
		double q2 = 0.0;

		if (NANorINF(x) && !INF(x)) 		/* x is a NaN */
			ret = x;
		else if (NANorINF(y) && !INF(y))	/* y is a NaN */
			ret = y;
		else {
			/* raise exception */
			/* x inf or y == 0 */
			q1 /= q2;
			MKNAN(ret);
		}
		return _domain_err(x,y,ret,"remainder",9);

	} else if (!x || NANorINF(y))
		return x;

	if (_fp_hw == FP_387) { /* 80387 present - use machine instruction */
		return xrem(x,y);
	}

	xexp = EXPONENT(x);
	yexp = EXPONENT(y);
	sign = SIGNBIT(x);

	/* subnormal number */
	mask = fpsetmask(0);  /* mask all exceptions */
	nx = 0;
	if (yexp == 0) {
		t = 1.0, EXPONENT(t) += 57;
		y *= t; 
		nx = 57;
		yexp = EXPONENT(y);
	}

	/* if y is tiny (biased exponent <= 57), scale up y to y*2**57 */
	else if (yexp <= 57) {
		EXPONENT(y) += 57; 
		nx += 57; 
		yexp += 57;
	}

	nf = nx;
	SIGNBIT(x) = 0;
	SIGNBIT(y) = 0;
	/* mask off the least significant 27 bits of y */
	t = y; 
	LOFRACTION(t) &= 0xf8000000;
	y2 = t;

	/* LOOP: argument reduction on x whenever x > y */
loop:
	while ( x > y ) {
		t = y;
		t1 = y2;
		xexp = EXPONENT(x);
		k = xexp - yexp - 25;
		if (k > 0) 	/* if x/y >= 2**26, scale up y so that x/y < 2**26 */ {
			EXPONENT(t) += k;
			EXPONENT(t1) += k;
		}
		n = x / t; 
		x = (x - n * t1) - n * (t - t1);
	}
	/* end while (x > y) */

	if (nx != 0) {
		t = 1.0; 
		EXPONENT(t) += nx; 
		x *= t; 
		nx = 0; 
		goto loop;
	}

	/* final adjustment */

	hy = y / 2.0;
	if (x > hy || ((x == hy) && n % 2 == 1)) 
		x -= y;
	SIGNBIT(x) ^= sign;
	if (nf != 0) { 
		t = 1.0; 
		EXPONENT(t) -= nf; 
		x *= t;
	}
	(void)fpsetmask(mask);  /* reset exception masks */
	return(x);
}

#ident	"@(#)libm:i386/logf.c	1.4"

/*LINTLIBRARY*/

/*
 *	logf returns the natural logarithm of its single-precision argument.
 *	log10f returns the base-10 logarithm of its single-precision argument.
 *	Algorithm and coefficients from Cody and Waite (1980).
 *	Calls frexp.
 *      If argument is NaN, return EDOM error and value NaN.  If argument is
 *      also a signalling NaN, raise the invalid op exception.
 *
 *      In -Xt mode,
 *      If x < 0 raise the invalid op exception and return error EDOM and
 *      value -HUGE.
 *      If x = 0 raise the divide by zero exception and return error EDOM
 *      and value -HUGE.
 *
 *      In -Xa and -Xc modes,
 *      If x < 0 raise the invalid op exception and return error EDOM and
 *      value NaN.
 *      If x = 0 raise the divide by zero exception and return error ERANGE
 *      and value -HUGE_VAL.
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

static float log_error(float, const char*, int);

asm	float xlogf(float x)
{
%mem	x;
	fldln2				/ load log(e)2
	flds	x
	fyl2x
}

#pragma partial_optimization xlogf

asm	float xlog10f(float x)
{
%mem	x;
	fldlg2				/ load log(10)2
	flds	x
	fyl2x
}

#pragma partial_optimization xlog10f

float
logf(float x)
{
	float xlogf(float);
	if (FNANorINF(x)) {
		if (FINF(x))	/* x is inf */
			return(FSIGNBIT(x) ? log_error(x,"logf",4) : x);
		else 		/* x is NaN */
			return _float_domain(x,0.0F,x,"logf",4);
	} 
	if (x <= 0.0F)
		return(log_error(x,"logf",4));

	return(xlogf(x));
}

float
log10f(float x)
{
	float xlog10f(float);
	if (FNANorINF(x)) {
		if (FINF(x))	/* x is inf */
			return(FSIGNBIT(x) ? log_error(x,"log10f",6) : x);
		else 		/* x is NaN */
			return _float_domain(x,0.0F,x,"log10f",6);
	} 
	if (x <= 0.0F)
		return(log_error(x,"log10f",6));

	return(xlog10f(x));
}

static float
log_error(float x, const char *f_name, int name_len)
{
	register int zflag = 0;
	float ret;
	float q1=0.0F;
	float q2=0.0F;


	if (!x) {
		/* raise divide-by-zero exception */
		q1 = 1.0F / q1;
		zflag = 1;
		ret = (float)-HUGE_VAL;
	} else {		/* x < 0 */
		/* raise invalid op exception */
		q1 /= q2;
		FMKNAN(ret);
	}

	if (_lib_version != c_issue_4) {
		errno = zflag ? ERANGE : EDOM;
		return ret;
	}
	else {
		struct exception exc;
		exc.arg1 = (float)x;
		exc.name = (char *)f_name;
		exc.retval = -HUGE;
		exc.type = zflag ? SING : DOMAIN;

		if (!matherr(&exc)) {
			(void) write(2, f_name, name_len);
			if (zflag)
				(void) write(2,": SING error\n",13);
			else
				(void) write(2,": DOMAIN error\n",15);
			errno = EDOM;
		}
		return ((float)exc.retval);
	}
}

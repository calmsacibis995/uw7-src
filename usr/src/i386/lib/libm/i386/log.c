#ident	"@(#)libm:i386/log.c	1.7"

/*LINTLIBRARY*/

/*
 *	log returns the natural logarithm of its double-precision argument.
 *	log10 returns the base-10 logarithm of its double-precision argument.
 *	Algorithm and coefficients from Cody and Waite (1980).
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
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"

extern int write();
static double log_error();

asm	double xlog(x)
{
%mem	x;
	fldln2				/ load log(e)2
	fldl	x
	fyl2x
}

#pragma partial_optimization xlog

asm	double xlog10(x)
{
%mem	x;
	fldlg2				/ load log(10)2
	fldl	x
	fyl2x
}
#pragma partial_optimization xlog10

double
log(x)
double x;
{
	if (NANorINF(x)) {
		if (INF(x))     
			return(x < 0 ? log_error(x,"log",3) : x);
		else            /* x is NaN */
			return _domain_err(x,0.0,x,"log",3);
	} else if (x <= 0.0)
		return(log_error(x,"log",3));

	return(xlog(x));
}

double
log10(x)
double x;
{
	if (NANorINF(x)) {
		if (INF(x))     
			return(x < 0 ? log_error(x,"log10",5) : x);
		else            /* x is NaN */
			return _domain_err(x,0.0,x,"log10",5);
	} else if (x <= 0.0)
		return(log_error(x,"log10",5));

	return(xlog10(x));
}


static double
log_error(x, f_name, name_len)
double x;
char *f_name;
unsigned int name_len;
{
	register int zflag = 0;
	struct exception exc;
	double q1=0.0;
	double q2=0.0;
	int err=EDOM;

	exc.arg1 = x;
	exc.name = f_name;

	if (x == 0.0) {
		/* raise divide-by-zero exception */
		q1 = 1.0 / q1;
		exc.retval = -HUGE_VAL;
		err=ERANGE;
		zflag = 1;
	} else {		/* x < 0 */
		/* raise invalid op exception */
		q1 /= q2;
		MKNAN(exc.retval);	/* make a NaN for the return value */
	}

	if (_lib_version != c_issue_4)
		errno = err;
	else {
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
	}
	return (exc.retval);
}

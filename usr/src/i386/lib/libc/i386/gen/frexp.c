#ident	"@(#)libc-i386:gen/frexp.c	1.5"
/*LINTLIBRARY*/
/*
 * frexp(value, eptr)
 * returns a double x such that x = 0 or 0.5 <= |x| < 1.0
 * and stores an integer n such that value = x * 2 ** n
 * indirectly through eptr.
 *
 * If value is a NaN returns error EDOM and value NaN.  If value is also 
 * a signalling NaN, raise an invalid op exception.
 */
#include "synonyms.h"
#include <errno.h>
#include <values.h>
#include "fpparts.h"

asm	double
xfrexp(val,contwo,ptr)
{
%mem	val,contwo,ptr;
	movl	ptr,%eax
	fldl	val
	fxtract
	fxch	%st(1)
	fistpl	(%eax)
	fidiv	contwo
	incl	(%eax)
}

#pragma partial_optimization xfrexp

double
frexp(value, eptr)
double value; 
int *eptr;
{
	static int contwo = 2;

	if (NANorINF(value)){
		if (!INF(value)){	/* value is NaN */

			/* Cause exception if not quiet */
			SigNAN(value);

			errno=EDOM;
		}
			
		return value;
	}

	*eptr = 0;
	if (value == 0.0) /* nothing to do for zero */
		return (value);
	return(xfrexp(value,contwo,eptr));
}

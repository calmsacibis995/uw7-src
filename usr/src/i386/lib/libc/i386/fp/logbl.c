#ident	"@(#)libc-i386:fp/logbl.c	1.4"
/*LINTLIBRARY*/

/* IEEE recommended functions */

#ifdef __STDC__
	#pragma weak logbl = _logbl
	#pragma weak nextafterl = _nextafterl
#endif

#include <values.h>
#include <float.h>
#include <errno.h>
#include "fpparts.h"
#include "synonyms.h"

/* LOGBL(X)
 * logbl(x) returns the unbiased exponent of x as a long double precision
 * number, except that logbl(NaN) is NaN, logbl(infinity) is +infinity,
 * logbl(0) is -infinity and raises the divide by zero exception,
 * errno = EDOM.
 * Signalling NaNs raise the invalid op exception
 */
long double logbl(x)
long double x;
{
	register int iexp = EXPONENTLD(x);

	if (iexp == MAXEXPLD) { /* infinity  or NaN */
		SIGNBITLD(x) = 0;
		errno = EDOM;
		if ((LOFRACTIONLD(x) || HIFRACTIONLD(x)) &&
			!QNANBITLD(x))
			/* signalling nan - raise invalid op */
			return(x + 1.0);
		else
			return x;
	}
	if (iexp == 0)  {  /* de-normal  or 0*/
		if ((HIFRACTIONLD(x) == 0) && (LOFRACTIONLD(x) == 0)) { /*zero*/
			long double zero = 0.0L;
			errno = EDOM;
			return(-1.0L/zero); /* return -inf - raise div by 
					    * zero exception
					    */
		}
		else  /* de-normal */
			return(-16382.0L);
	}
	return((long double)(iexp - 16383.0L)); /* subtract bias */
}

/* NEXTAFTERL(X,Y)
 * nextafterl(x,y) returns the next representable neighbor of x
 * in the direction of y
 * Special cases:
 * 1) if either x or y is a NaN then the result is one of the NaNs, errno
 * 	=EDOM
 * 2) if x is +-inf, x is returned and errno = EDOM
 * 3) if x == y the results is x without any exceptions being signalled
 * 4) overflow  and inexact are signalled when x is finite,
 *	but nextafterl(x,y) is not, errno = ERANGE
 * 5) underflow and inexact are signalled when nextafterl(x,y)
 * 	lies between +-(2**-16382), errno = ERANGE
 */
 long double nextafterl(x,y)
 long double x,y;
 {
	if (NANorINFLD(x)) {
		errno = EDOM;
		if (!INFLD(x)) {  /* NaN */

			SigNANLD(x);
		}
		return x; 
	}
	if (NANorINFLD(y) && !INFLD(y)) {
		errno = EDOM;
		SigNANLD(y);
		return y;
	}
	if (x == y)
		return x;
	if (((y > x) && (x >= 0.0)) || ((y < x) && (x <= 0.0))) {
		/* y>x, x pos or y<x, x negative */

		if ((x == 0.0) && (y < 0.0))
			SIGNBITLD(x) = 1;

		if (LOFRACTIONLD(x) != (unsigned)0xffffffff)
			LOFRACTIONLD(x) += 0x1;
		else {
			LOFRACTIONLD(x) = 0x0;
			if ((HIFRACTIONLD(x) & 0x7fffffff) != 
				(unsigned)0x7fffffff)
				HIFRACTIONLD(x) += 0x1; 
			else {
				HIFRACTIONLD(x) = 0x80000000; 
				EXPONENTLD(x) += 0x1;
			}
		}
	}
	else { /* y<x, x pos or y>x, x neg */

		if (LOFRACTIONLD(x) != 0x0)
			LOFRACTIONLD(x) -= 0x1;
		else {
			LOFRACTIONLD(x) = (unsigned)0xffffffff;
			if ((HIFRACTIONLD(x) & 0x7fffffff) != 0x0)
				HIFRACTIONLD(x) -=0x1;
			else {
				HIFRACTIONLD(x) = (unsigned)0xffffffff;
				EXPONENTLD(x) -= 0x1;
				if (EXPONENTLD(x) == 0)
					/* becomes denormal */
					HIFRACTIONLD(x) = 0x7fffffff;
			}
		}
	}
	if (EXPONENTLD(x) == MAXEXPLD) { /* signal overflow and inexact */
		long double z = LDBL_MAX;
		errno = ERANGE;
		z *= 3.0;
	}
	if (EXPONENTLD(x) == 0) {
	/* de-normal - signal underflow and inexact */
		long double z = MINLONGDOUBLE;
		errno = ERANGE;
		z /= 2.0;
	}
	return x;
}

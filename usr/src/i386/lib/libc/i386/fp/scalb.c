#ident	"@(#)libc-i386:fp/scalb.c	1.9"
/*LINTLIBRARY

/* SCALB(X,N)
 * return x * 2**N without computing 2**N - this is the standard
 * C library ldexp() routine except that signaling NANs generate
 * invalid op exception - errno = EDOM
 */

#ifdef __STDC__
	#pragma weak scalb = _scalb
#endif
#include "synonyms.h"
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"
#include <limits.h>

double scalb(x,n)
double	x, n;
{
	if (NANorINF(x)) { /* check for NAN or INF (IEEE only) */
		if (!INF(x)) {	/* is a NaN */
			/* Raise an exception on non-quiet NaN. */
			SigNAN(x);
			errno=EDOM;
			return x;
		}
		else {
			errno = ERANGE;
			if (_lib_version == c_issue_4)
				return(x > 0.0 ? HUGE : -HUGE);
			else
				return(x);
		}
	}

	if (NANorINF(n)) /* check for NAN or INF (IEEE only) */
		if (!INF(n)) {	/* is a NaN */
			/* Raise an exception on non-quiet NaN. */
			SigNAN(n);
			errno=EDOM;
			return n;
		}

	/* 0.0 * 2**n = x or x * 2**0 = x * 1 = x	*/
	else if (x == 0.0 || n == 0.0)
		return x;

	if ((n >= (double)INT_MAX) || (n <= (double)INT_MIN)) 
	{
		/* over or underflow	*/
		errno = ERANGE;

		/* lim n -> -Inf of x * 2**n = 0 		*/
		if(n < 0.0) return(0.0); 	/* underflow	*/

		else { 	
			/* lim n -> Inf of x * 2**n = -Inf or +Inf 	*/
			if (_lib_version == c_issue_4)
				return(x > 0.0 ? HUGE : -HUGE);
			else
				return(x > 0.0 ? HUGE_VAL : -HUGE_VAL);
		}
	}
	return(ldexp(x, (int)n));
}

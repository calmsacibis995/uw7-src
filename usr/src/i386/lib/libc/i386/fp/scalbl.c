#ident	"@(#)libc-i386:fp/scalbl.c	1.2"
/*LINTLIBRARY

/* SCALBL(X,N)
 * return x * 2**N without computing 2**N - this is the standard
 * C library ldexpln() routine except that signaling NANs generate
 * invalid op exception - errno = EDOM
 */

#ifdef __STDC__
	#pragma weak scalbl = _scalbl
#endif
#include "synonyms.h"
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"
#include <limits.h>

long double 
scalbl(x,n)
long double	x, n;
{
	long double ret;

	if (NANorINFLD(x)) { /* check for NAN or INF (IEEE only) */
		if (!INFLD(x)) {	/* is a NaN */
			/* Raise an exception on non-quiet NaN. */
			SigNANLD(x);
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
	/* 0.0 * 2**n = x or x * 2**0 = x * 1 = x	*/
	else if ((x == 0.0) || (n == 0.0)) return x;

	else if ((n >= (long double)INT_MAX) || (n <= (long double)INT_MIN)) 
	{
		if(n < 0.0) return(0.0); 
			/* lim n -> -Inf of x * 2**n = 0 		*/
		else { 	
			/* lim n -> Inf of x * 2**n = -Inf or +Inf 	*/
			if (_lib_version == c_issue_4)
				return(x > 0.0 ? HUGE : -HUGE);
			else
				return(x > 0.0 ? HUGE_VAL : -HUGE_VAL);
		}
	}

	return(ldexpl(x, (int)n));
}

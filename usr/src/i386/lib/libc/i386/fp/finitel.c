#ident	"@(#)libc-i386:fp/finitel.c	1.2"
/*LINTLIBRARY*/

/*	IEEE recommended functions */

#ifdef __STDC__
	#pragma weak finitel = _finitel
	#pragma weak fpclassl = _fpclassl
	#pragma weak unorderedl = _unorderedl
#endif

#include <values.h>
#include "fpparts.h"
#include "synonyms.h"

#define P754_NOFAULT 1		/* avoid generating extra code */
#include "ieeefp.h"

/* FINITEL(X)
 * finitel(x) returns 1 if x > -inf and x < +inf and 0 otherwise 
 * NaN returns 0
 */

int finitel(x)
long double	x;
{	
	return(!NANorINFLD(x));
}

/* UNORDEREDL(x,y)
 * unorderedl(x,y) returns 1 if x is unordered with y, otherwise
 * it returns 0; x is unordered with y if either x or y is NAN
 */

int unorderedl(x,y)
long double	x,y;
{	
	if (NANorINFLD(x) && !INFLD(x))
		return 1;
	if (NANorINFLD(y) && !INFLD(y))
		return 1;
	return 0;
}	

/* FPCLASSL(X)
 * fpclassl(x) returns the floating point class x belongs to 
 */

fpclass_t	fpclassl(x)
long double	x;
{	
	register int	sign, exp;

	exp = EXPONENTLD(x);
	sign = SIGNBITLD(x);
	if (exp == 0) { /* de-normal or zero */
		if (HIFRACTIONLD(x) || LOFRACTIONLD(x)) /* de-normal */
			return(sign ? FP_NDENORM : FP_PDENORM);
		else
			return(sign ? FP_NZERO : FP_PZERO);
	}
	if (exp == MAXEXPLD) { /* infinity or NaN */
		if (INFLD(x))
			return(sign ? FP_NINF : FP_PINF);
		else {
			if (QNANBITLD(x))
			/* hi-bit of mantissa set - quiet nan */
				return(FP_QNAN);
			else	return(FP_SNAN);
		}
	}
	/* if we reach here we have non-zero normalized number */
	return(sign ? FP_NNORM : FP_PNORM);
}

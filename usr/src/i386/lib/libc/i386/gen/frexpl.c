#ident	"@(#)libc-i386:gen/frexpl.c	1.3"
/*LINTLIBRARY*/
/*
 * frexpl(value, eptr)
 * returns a long double x such that x = 0 or 0.5 <= |x| < 1.0
 * and stores an integer n such that value = x * 2 ** n
 * indirectly through eptr.
 *
 * If value is a NaN returns error EDOM and value NaN.  If value is also
 * a signalling NaN, raise an invalid op exception.
 *
 */
#include "synonyms.h"
#include <values.h>
#include "fpparts.h"
#include <errno.h>

asm	long double
xfrexpl(val,contwo,ptr)
{
%mem	val,contwo,ptr;
	movl	ptr,%eax
	fldt	val
	fxtract
	fxch	%st(1)
	fistpl	(%eax)
	fidiv	contwo
	incl	(%eax)
}

#pragma partial_optimization xfrexpl

long double
frexpl(value, eptr)
long double value;
int *eptr;
{
	static int contwo = 2;

        if (NANorINFLD(value)){
                if (!INFLD(value)){       /* value is NaN */

                        /* Cause exception if not quiet */
                        SigNANLD(value);

                        errno=EDOM;
                }

                return value;
        }

	*eptr = 0;
	if (value == 0.0) /* nothing to do for zero */
		return (value);
	return(xfrexpl(value,contwo,eptr));
}

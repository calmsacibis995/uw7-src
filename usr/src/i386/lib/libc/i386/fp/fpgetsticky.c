#ident	"@(#)libc-i386:fp/fpgetsticky.c	1.4"

#include <ieeefp.h>
#include "fp.h"
#include "synonyms.h"

#ifdef __STDC__
	#pragma weak fpgetsticky = _fpgetsticky
#endif

fp_except
fpgetsticky()
{
	struct _sw87 sw;

	_getsw(&sw);
	return (fp_except)sw.excp;
}

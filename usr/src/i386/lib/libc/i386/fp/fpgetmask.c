#ident	"@(#)libc-i386:fp/fpgetmask.c	1.4"

#include <ieeefp.h>
#include "fp.h"
#include "synonyms.h"

#ifdef __STDC__
	#pragma weak fpgetmask = _fpgetmask
#endif

fp_except
fpgetmask()
{
	struct _cw87 cw;

	_getcw(&cw);
	return (fp_except)(~cw.mask & EXCPMASK);
}





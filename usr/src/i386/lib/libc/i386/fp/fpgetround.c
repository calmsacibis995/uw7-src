#ident	"@(#)libc-i386:fp/fpgetround.c	1.4"

#include <ieeefp.h>
#include "fp.h"
#include "synonyms.h"

#ifdef __STDC__
	#pragma	weak fpgetround = _fpgetround
#endif

fp_rnd
fpgetround()
{
	struct _cw87 cw;

	_getcw(&cw);
	return (fp_rnd)cw.rnd;
}

#ident	"@(#)ksh93:src/lib/libast/sfio/_sferror.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sferror(reg Sfio_t* f)
#else
static __sferror(f)
reg Sfio_t	*f;
#endif
{
	return sferror(f);
}

#undef sferror

#if __STD_C
sferror(reg Sfio_t* f)
#else
sferror(f)
reg Sfio_t	*f;
#endif
{
	return __sferror(f);
}

#ident	"@(#)ksh93:src/lib/libast/sfio/_sfclrerr.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfclrerr(reg Sfio_t* f)
#else
static __sfclrerr(f)
reg Sfio_t	*f;
#endif
{
	return sfclrerr(f);
}

#undef sfclrerr

#if __STD_C
sfclrerr(reg Sfio_t* f)
#else
sfclrerr(f)
reg Sfio_t	*f;
#endif
{
	return __sfclrerr(f);
}

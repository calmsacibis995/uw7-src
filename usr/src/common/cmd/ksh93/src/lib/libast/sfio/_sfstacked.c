#ident	"@(#)ksh93:src/lib/libast/sfio/_sfstacked.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfstacked(reg Sfio_t* f)
#else
static __sfstacked(f)
reg Sfio_t	*f;
#endif
{
	return sfstacked(f);
}

#undef sfstacked

#if __STD_C
sfstacked(reg Sfio_t* f)
#else
sfstacked(f)
reg Sfio_t	*f;
#endif
{
	return __sfstacked(f);
}

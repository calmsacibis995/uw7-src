#ident	"@(#)ksh93:src/lib/libast/sfio/_sfeof.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfeof(reg Sfio_t* f)
#else
static __sfeof(f)
reg Sfio_t	*f;
#endif
{
	return sfeof(f);
}

#undef sfeof

#if __STD_C
sfeof(reg Sfio_t* f)
#else
sfeof(f)
reg Sfio_t	*f;
#endif
{
	return __sfeof(f);
}

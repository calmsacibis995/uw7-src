#ident	"@(#)ksh93:src/lib/libast/sfio/_sffileno.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sffileno(reg Sfio_t* f)
#else
static __sffileno(f)
reg Sfio_t	*f;
#endif
{
	return sffileno(f);
}

#undef sffileno

#if __STD_C
sffileno(reg Sfio_t* f)
#else
sffileno(f)
reg Sfio_t	*f;
#endif
{
	return __sffileno(f);
}

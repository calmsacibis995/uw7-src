#ident	"@(#)ksh93:src/lib/libast/sfio/_sfputl.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfputl(reg Sfio_t* f, reg long v)
#else
static __sfputl(f,v)
reg Sfio_t	*f;
reg long	v;
#endif
{
	return sfputl(f,v);
}

#undef sfputl

#if __STD_C
sfputl(reg Sfio_t* f, reg long v)
#else
sfputl(f,v)
reg Sfio_t	*f;
reg long	v;
#endif
{
	return __sfputl(f,v);
}

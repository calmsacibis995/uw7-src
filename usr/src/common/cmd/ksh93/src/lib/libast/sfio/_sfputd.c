#ident	"@(#)ksh93:src/lib/libast/sfio/_sfputd.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfputd(reg Sfio_t* f, reg double v)
#else
static __sfputd(f,v)
reg Sfio_t	*f;
reg double	v;
#endif
{
	return sfputd(f,v);
}

#undef sfputd

#if __STD_C
sfputd(reg Sfio_t* f, reg double v)
#else
sfputd(f,v)
reg Sfio_t	*f;
reg double	v;
#endif
{
	return __sfputd(f,v);
}

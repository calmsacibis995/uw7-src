#ident	"@(#)ksh93:src/lib/libast/sfio/_sfputc.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfputc(reg Sfio_t* f, reg int c)
#else
static __sfputc(f,c)
reg Sfio_t	*f;
reg int		c;
#endif
{
	return sfputc(f,c);
}

#undef sfputc

#if __STD_C
sfputc(reg Sfio_t* f, reg int c)
#else
sfputc(f,c)
reg Sfio_t	*f;
reg int		c;
#endif
{
	return __sfputc(f,c);
}

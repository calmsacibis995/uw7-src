#ident	"@(#)ksh93:src/lib/libast/sfio/_sfgetc.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfgetc(reg Sfio_t* f)
#else
static __sfgetc(f)
reg Sfio_t	*f;
#endif
{
	return sfgetc(f);
}

#undef sfgetc

#if __STD_C
sfgetc(reg Sfio_t* f)
#else
sfgetc(f)
reg Sfio_t	*f;
#endif
{
	return __sfgetc(f);
}

#ident	"@(#)ksh93:src/lib/libast/sfio/_sfputu.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfputu(reg Sfio_t* f, reg ulong v)
#else
static __sfputu(f,v)
reg Sfio_t	*f;
reg ulong	v;
#endif
{
	return sfputu(f,v);
}

#undef sfputu

#if __STD_C
sfputu(reg Sfio_t* f, reg ulong v)
#else
sfputu(f,v)
reg Sfio_t	*f;
reg ulong	v;
#endif
{
	return __sfputu(f,v);
}

#ident	"@(#)ksh93:src/lib/libast/sfio/_sfulen.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfulen(reg ulong v)
#else
static __sfulen(v)
reg ulong	v;
#endif
{
	return sfulen(v);
}

#undef sfulen

#if __STD_C
sfulen(reg ulong v)
#else
sfulen(v)
reg ulong	v;
#endif
{
	return __sfulen(v);
}

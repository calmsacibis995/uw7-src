#ident	"@(#)ksh93:src/lib/libast/sfio/_sfslen.c	1.1"
#include	"sfhdr.h"

#if __STD_C
static __sfslen(void)
#else
static __sfslen()
#endif
{
	return sfslen();
}

#undef sfslen

#if __STD_C
sfslen(void)
#else
sfslen()
#endif
{
	return __sfslen();
}

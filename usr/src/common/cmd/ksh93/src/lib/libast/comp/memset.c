#ident	"@(#)ksh93:src/lib/libast/comp/memset.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_memset

NoN(memset)

#else

void*
memset(void* asp, register int c, register size_t n)
{
	register char*	sp = (char*)asp;

	while (n-- > 0)
		*sp++ = c;
	return(asp);
}

#endif

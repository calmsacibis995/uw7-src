#ident	"@(#)ksh93:src/lib/libast/comp/memcmp.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_memcmp

NoN(memcmp)

#else

int
memcmp(const void* ab1, const void* ab2, size_t n)
{
	register const unsigned char*	b1 = (const unsigned char*)ab1;
	register const unsigned char*	b2 = (const unsigned char*)ab2;
	register const unsigned char*	e = b1 + n;

	while (b1 < e)
		if (*b1++ != *b2++)
			return(*--b1 - *--b2);
	return(0);
}

#endif

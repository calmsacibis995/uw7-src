#ident	"@(#)ksh93:src/lib/libast/comp/memcpy.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_memcpy

NoN(memcpy)

#else

#undef	memcpy

#ifdef _lib_bcopy

extern void	bcopy(void*, void*, size_t);

void*
memcpy(void* s1, void* s2, size_t n)
{
	bcopy(s2, s1, n);
	return(s1);
}

#else

void*
memcpy(void* as1, const void* as2, register size_t n)
{
	register char*		s1 = (char*)as1;
	register const char*	s2 = (const char*)as2;

	while (n-- > 0)
		*s1++ = *s2++;
	return(as1);
}

#endif

#endif

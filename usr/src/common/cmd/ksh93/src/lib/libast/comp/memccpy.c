#ident	"@(#)ksh93:src/lib/libast/comp/memccpy.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_memccpy

NoN(memccpy)

#else

/*
 * Copy s2 to s1, stopping if character c is copied. Copy no more than n bytes.
 * Return a pointer to the byte after character c in the copy,
 * or 0 if c is not found in the first n bytes.
 */

void*
memccpy(void* as1, const void* as2, register int c, size_t n)
{
	register char*		s1 = (char*)as1;
	register const char*	s2 = (char*)as2;
	register const char*	ep = s2 + n;

	while (s2 < ep)
		if ((*s1++ = *s2++) == c)
			return(s1);
	return(0);
}

#endif

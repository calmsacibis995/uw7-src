#ident	"@(#)ksh93:src/lib/libast/comp/memchr.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_memchr

NoN(memchr)

#else

/*
 * Return the ptr in sp at which the character c appears;
 * 0 if not found in n chars; don't stop at \0.
 */

void*
memchr(const void* asp, register int c, size_t n)
{
	register const char*	sp = (char*)asp;
	register const char*	ep = sp + n;

	while (sp < ep)
		if (*sp++ == c)
			return(--sp);
	return(0);
}

#endif

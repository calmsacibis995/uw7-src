#ident	"@(#)ksh93:src/lib/libast/string/memdup.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_memdup

NoN(memdup)

#else

/*
 * return a copy of s of n chars using malloc
 */

void*
memdup(register const void* s, register size_t n)
{
	register void*	t;

	return((t = (void*)newof(0, char, n, 0)) ? memcpy(t, s, n) : 0);
}

#endif

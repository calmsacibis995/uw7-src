#ident	"@(#)ksh93:src/lib/libast/hash/memsum.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * return a running 32 bit checksum of buffer b of length n
 *
 * c is the return value from a previous
 * memsum() or strsum() call, 0 on the first call
 *
 * the result is the same on all implementations
 */

unsigned long
memsum(const void* ap, int n, register unsigned long c)
{
	register const unsigned char*	p = (const unsigned char*)ap;
	register const unsigned char*	e = p + n;

	while (p < e) HASHPART(c, *p++);
#if LONG_MAX > 2147483647
	return(c & 0xffffffff);
#else
	return(c);
#endif
}

#ident	"@(#)ksh93:src/lib/libast/hash/strsum.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * return a running 32 bit checksum of string s
 *
 * c is the return value from a previous
 * memsum() or strsum() call, 0 on the first call
 *
 * the result is the same on all implementations
 */

unsigned long
strsum(const char* as, register unsigned long c)
{
	register const unsigned char*	s = (const unsigned char*)as;
	register int			n;

	while (n = *s++) HASHPART(c, n);
#if LONG_MAX > 2147483647
	return(c & 0xffffffff);
#else
	return(c);
#endif
}

#ident	"@(#)ksh93:src/lib/libast/hash/strhash.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * hash table library
 */

#include "hashlib.h"

/*
 * return the hash of the null terminated string s
 */

unsigned int
strhash(const char* as)
{
	register const unsigned char*	s = (const unsigned char*)as;
	register unsigned int		i = 0;
	register unsigned int		c;

	while (c = *s++) HASHPART(i, c);
	return(i);
}

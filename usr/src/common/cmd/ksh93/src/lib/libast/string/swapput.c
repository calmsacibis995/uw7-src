#ident	"@(#)ksh93:src/lib/libast/string/swapput.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * internal representation conversion support
 */

#include <ast.h>
#include <swap.h>

/*
 * put v of n chars into b according to op
 */

void*
swapput(int op, void* b, int n, int_max v)
{
	register char*	p = (char*)b + n;

	while (p > (char*)b)
	{
		*--p = v;
		v >>= CHAR_BIT;
	}
	if (op ^= int_swap) swapmem(op, p, p, n);
	return(b);
}

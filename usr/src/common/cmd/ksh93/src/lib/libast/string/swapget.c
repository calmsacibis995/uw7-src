#ident	"@(#)ksh93:src/lib/libast/string/swapget.c	1.1"
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
 * get int_n from b according to op
 */

int_max
swapget(int op, const void* b, int n)
{
	register unsigned char*	p;
	register unsigned char*	d;
	int_max			v;
	unsigned char		tmp[sizeof(int_max)];

	if (op ^= int_swap)
	{
		if (n > sizeof(int_max)) n = sizeof(int_max);
		swapmem(op, b, d = tmp, n);
	}
	else d = (unsigned char*)b;
	p = d + n;
	v = 0;
	while (d < p)
	{
		v <<= CHAR_BIT;
		v |= *d++;
	}
	return(v);
}

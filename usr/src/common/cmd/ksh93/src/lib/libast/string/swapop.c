#ident	"@(#)ksh93:src/lib/libast/string/swapop.c	1.1"
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
 * return the swap operation for external to internal conversion
 */

int
swapop(const void* internal, const void* external, int size)
{
	register int	op;
	char		tmp[sizeof(int_max)];

	if (size <= 1)
		return(0);
	if (size <= sizeof(int_max))
		for (op = 0; op < size; op++)
			if (!memcmp(internal, swapmem(op, external, tmp, size), size))
				return(0);
	return(-1);
}

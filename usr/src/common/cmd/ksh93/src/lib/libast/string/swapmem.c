#ident	"@(#)ksh93:src/lib/libast/string/swapmem.c	1.1"
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
 * swap n bytes according to op
 * from==to is ok
 */

void*
swapmem(int op, const void* from, void* to, register size_t n)
{
	register char*	f = (char*)from;
	register char*	t = (char*)to;
	register int	c;

	switch ((op ^ int_swap) & (n - 1))
	{
	case 0:
		if (t != f) memcpy(t, f, n);
		break;
	case 1:
		for (n >>= 1; n--; f += 2, t += 2)
		{
			c = f[0]; t[0] = f[1]; t[1] = c;
		}
		break;
	case 2:
		for (n >>= 2; n--; f += 4, t += 4)
		{
			c = f[0]; t[0] = f[2]; t[2] = c;
			c = f[1]; t[1] = f[3]; t[3] = c;
		}
		break;
	case 3:
		for (n >>= 2; n--; f += 4, t += 4)
		{
			c = f[0]; t[0] = f[3]; t[3] = c;
			c = f[1]; t[1] = f[2]; t[2] = c;
		}
		break;
	case 4:
		for (n >>= 3; n--; f += 8, t += 8)
		{
			c = f[0]; t[0] = f[4]; t[4] = c;
			c = f[1]; t[1] = f[5]; t[5] = c;
			c = f[2]; t[2] = f[6]; t[6] = c;
			c = f[3]; t[3] = f[7]; t[7] = c;
		}
		break;
	case 5:
		for (n >>= 3; n--; f += 8, t += 8)
		{
			c = f[0]; t[0] = f[5]; t[5] = c;
			c = f[1]; t[1] = f[4]; t[4] = c;
			c = f[2]; t[2] = f[7]; t[7] = c;
			c = f[3]; t[3] = f[6]; t[6] = c;
		}
		break;
	case 6:
		for (n >>= 3; n--; f += 8, t += 8)
		{
			c = f[0]; t[0] = f[6]; t[6] = c;
			c = f[1]; t[1] = f[7]; t[7] = c;
			c = f[2]; t[2] = f[4]; t[4] = c;
			c = f[3]; t[3] = f[5]; t[5] = c;
		}
		break;
	case 7:
		for (n >>= 3; n--; f += 8, t += 8)
		{
			c = f[0]; t[0] = f[7]; t[7] = c;
			c = f[1]; t[1] = f[6]; t[6] = c;
			c = f[2]; t[2] = f[5]; t[5] = c;
			c = f[3]; t[3] = f[4]; t[4] = c;
		}
		break;
	}
	return(to);
}

#ident	"@(#)ksh93:src/lib/libast/string/modei.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * mode_t representation support
 */

#include "modelib.h"

/*
 * convert external mode to internal
 *
 * NOTE: X_IFMT ignored
 */

#undef	modei

int
modei(register int x)
{
#if _S_IDPERM
	return(x & X_IPERM);
#else
	register int	i;
	register int	c;

	i = 0;
	for (c = 0; c < PERMLEN; c += 2)
		if (x & permmap[c + 1])
			i |= permmap[c];
	return(i);
#endif
}

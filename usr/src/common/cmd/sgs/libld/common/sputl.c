#ident	"@(#)libld:common/sputl.c	1.5"
/*
 * The intent here is to provide a means to make the value of
 * bytes in an io-stream correspond to the value of the long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 *
 */
#include <values.h>
#include "synsyms.h"

void
sputl(w, buffer)
register long w;
register char *buffer;
{
	register int i = BITSPERBYTE * sizeof(long);

	while ((i -= BITSPERBYTE) >= 0)
		*buffer++ = (char) (w >> i);
}

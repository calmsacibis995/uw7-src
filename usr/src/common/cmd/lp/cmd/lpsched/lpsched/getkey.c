/*		copyright	"%c%" 	*/

#ident	"@(#)getkey.c	1.2"
#ident	"$Header$"

#include "sys/types.h"
#include "time.h"
#include "stdlib.h"

#include "lpsched.h"

long
#ifdef	__STDC__
getkey (
	void
)
#else
getkey ()
#endif
{
	DEFINE_FNNAME (getkey)

	static int		seeded = 0;

	if (!seeded) {
		srand48 (time((time_t *)0));
		seeded = 1;
	}
	return (lrand48());
}

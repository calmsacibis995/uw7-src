#ident	"@(#)ksh93:src/lib/libast/obsolete/slowio.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use sfslowio */

#include <ast.h>
#include <sfdisc.h>

int
slowio(Sfio_t* sp)
{
	return(sfslowio(sp));
}

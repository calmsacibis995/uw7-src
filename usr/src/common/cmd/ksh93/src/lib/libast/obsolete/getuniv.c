#ident	"@(#)ksh93:src/lib/libast/obsolete/getuniv.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use astconf() */

#include <ast.h>

char*
getuniv(void)
{
	return(astconf("UNIVERSE", NiL, NiL));
}

#ident	"@(#)ksh93:src/lib/libast/obsolete/setuniv.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use astconf() */

#include <ast.h>

char*
setuniv(const char* universe)
{
	return(astconf("UNIVERSE", NiL, universe));
}

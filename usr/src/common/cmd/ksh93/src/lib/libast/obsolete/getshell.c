#ident	"@(#)ksh93:src/lib/libast/obsolete/getshell.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use pathshell */

#include <ast.h>

char*
getshell(void)
{
	return(pathshell());
}

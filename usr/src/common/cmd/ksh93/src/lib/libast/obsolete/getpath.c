#ident	"@(#)ksh93:src/lib/libast/obsolete/getpath.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use pathbin */

#include <ast.h>

char*
getpath(void)
{
	return(pathbin());
}

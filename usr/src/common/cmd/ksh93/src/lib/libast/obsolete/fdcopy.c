#ident	"@(#)ksh93:src/lib/libast/obsolete/fdcopy.c	1.1"
#pragma prototyped
/*
 * OBSOLETE 950401 -- use astcopy
 */

#include <ast.h>

off_t
fdcopy(int rfd, int wfd, off_t n)
{
	return(astcopy(rfd, wfd, n));
}

#ident	"@(#)ksh93:src/lib/libast/obsolete/lpstat.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use pathstat */

#include <ast.h>
int
lpstat(const char* path, struct stat* st)
{
	return(pathstat(path, st));
}

#ident	"@(#)ksh93:src/lib/libast/obsolete/hsort.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use strsort */

#include <ast.h>

void
hsort(char** argv, int n, int(*fn)(const char*, const char*))
{
	strsort(argv, n, fn);
}

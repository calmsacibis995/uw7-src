#ident	"@(#)ksh93:src/lib/libast/obsolete/getwinsize.c	1.1"
#pragma prototyped

/* OBSOLETE 950401 -- use astwinsize */

#include <ast.h>

void
getwinsize(int fd, register int* rows, register int* cols)
{
	astwinsize(fd, rows, cols);
}

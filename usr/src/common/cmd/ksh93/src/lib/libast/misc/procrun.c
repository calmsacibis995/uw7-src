#ident	"@(#)ksh93:src/lib/libast/misc/procrun.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * procopen() + procclose()
 * no env changes
 * no modifications
 * effective=real
 */

#include "proclib.h"

int
procrun(const char* path, char** argv)
{
	return(procclose(procopen(path, argv, NiL, NiL, PROC_GID|PROC_UID)));
}

#ident	"@(#)ksh93:src/lib/libcmd/cmdinit.c	1.1"
#pragma prototyped
/*
 * command initialization
 */

#include <cmdlib.h>

void
cmdinit(register char** argv)
{
	register char*	cp;

	if (cp = strrchr(argv[0], '/')) cp++;
	else cp = argv[0];
	error_info.id = cp;
}

#ident	"@(#)ksh93:src/lib/libast/misc/ftwflags.c	1.1"
#pragma prototyped

/*
 * return default FTW_* flags conditioned by astconf()
 */

#include <ast.h>
#include <ftwalk.h>

int
ftwflags(void)
{
	register char*	s;
	
	s = astconf("PATH_RESOLVE", NiL, NiL);
	if (streq(s, "physical"))
		return(FTW_PHYSICAL);
	if (streq(s, "metaphysical"))
		return(FTW_META|FTW_PHYSICAL);
	return(0);
}

#ident	"@(#)ksh93:src/lib/libast/comp/system.c	1.1"
#pragma prototyped
/*
 * ast library system(3)
 */

#include <ast.h>
#include <proc.h>

int
system(const char* cmd)
{
	char*	sh[4];

	sh[0] = "sh";
	sh[1] = "-c";
	sh[2] = (char*)cmd;
	sh[3] = 0;
	return(procrun(NiL, sh));
}

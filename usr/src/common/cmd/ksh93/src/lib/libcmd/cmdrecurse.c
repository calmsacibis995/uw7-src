#ident	"@(#)ksh93:src/lib/libcmd/cmdrecurse.c	1.1"
#pragma prototyped
/*
 * use tw to recurse on argc,argv with pfxc,pfxv prefix args
 */

#include <cmdlib.h>
#include <proc.h>
#include <ftwalk.h>

int
cmdrecurse(int argc, char** argv, int pfxc, char** pfxv)
{
	register char**	v;
	register char**	a;
	int		resolve = 'L';
	char		arg[16];

	if (!(a = (char**)stakalloc((argc + pfxc + 4) * sizeof(char**))))
		error(ERROR_exit(1), gettxt(":231","out of space"));
	v = a;
	*v++ = "tw";
	*v++ = arg;
	*v++ = *(argv - opt_info.index);
	while (*v = *pfxv++)
	{
		if (streq(*v, "-H"))
			resolve = 'H';
		else if (streq(*v, "-P"))
			resolve = 'P';
		v++;
	}
	while (*v++ = *argv++);
	sfsprintf(arg, sizeof(arg), "-%cc%d", resolve, pfxc + 2);
	procopen(*a, a, NiL, NiL, PROC_OVERLAY);
	return(-1);
}

#ident	"@(#)ksh93:src/lib/libcmd/rmdir.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * rmdir
 */

static const char id[] = "\n@(#)rmdir (AT&T Bell Laboratories) 05/09/95\0\n";

#include <cmdlib.h>

int
b_rmdir(int argc, char** argv)
{
	register char*	dir;
	register int	n;

	NoP(argc);
	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, " directory ...")) switch (n)
	{
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	argv += opt_info.index;
	if (error_info.errors || !*argv)
		error(ERROR_usage(2), optusage(NiL));
	while (dir = *argv++)
		if (rmdir(dir) < 0)
			error(ERROR_system(0), gettxt(":317","%s: cannot remove"), dir);
	return(error_info.errors != 0);
}

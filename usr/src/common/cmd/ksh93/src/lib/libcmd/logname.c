#ident	"@(#)ksh93:src/lib/libcmd/logname.c	1.1"
#pragma prototyped
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * logname
 */

static const char id[] = "\n@(#)logname (AT&T Bell Laboratories) 04/01/92\0\n";

#include <cmdlib.h>

#include "FEATURE/ids"

#if _lib_getlogin
extern char*		getlogin(void);
#endif

int
b_logname(int argc, char *argv[])
{
	register int	n;
	register char*	logname;

	NoP(argc);
	NoP(id[0]);
	cmdinit(argv);
	while (n = optget(argv, " ")) switch (n)
	{
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	if (error_info.errors)
		error(ERROR_usage(2), optusage(NiL));
#if _lib_getlogin
	if (!(logname = getlogin()))
#endif
	logname = fmtuid(getuid());
	sfputr(sfstdout, logname, '\n');
	return(0);
}

#ident	"@(#)ksh93:src/lib/libcmd/getconf.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * getconf [ -prw ] [ name [ [ path [ value ] ] ... ]
 */

static const char id[] = "\n@(#)getconf (AT&T Bell Laboratories) 05/09/95\0\n";

#include <cmd.h>

int
b_getconf(int argc, char** argv)
{
	register char*	name;
	register char*	path;
	register char*	value;
	register char*	s;
	register int	n;
	int		flags;

	static char	empty[] = "-";

	NoP(argc);
	NoP(id[0]);
	cmdinit(argv);
	error_info.flags |= ERROR_LIBRARY;
	flags = 0;
	while (n = optget(argv, gettxt(":374", "prw [ name [ path [ value ] ] ... ]"))) switch (n)
	{
	case 'p':
		flags |= X_OK;
		break;
	case 'r':
		flags |= R_OK;
		break;
	case 'w':
		flags |= W_OK;
		break;
	case ':':
		error(2, opt_info.arg);
		break;
	case '?':
		error(ERROR_usage(2), opt_info.arg);
		break;
	}
	if (error_info.errors)
		error(ERROR_usage(2), optusage(NiL));
	argv += opt_info.index;
	name = *argv;
	do
	{
		if (!name)
		{
			path = 0;
			value = 0;
		}
		else
		{
			if (streq(name, empty))
				name = 0;
			if (!(path = *++argv))
				value = 0;
			else
			{
				if (streq(path, empty))
					path = 0;
				if ((value = *++argv) && (streq(value, empty)))
					value = 0;
			}
		}
		if (!name)
			astconflist(sfstdout, path, flags);
		else if (!(s = astconf(name, path, value)))
			break;
		else if (!value)
		{
			if (flags & X_OK)
			{
				sfputr(sfstdout, name, ' ');
				sfputr(sfstdout, path ? path : empty, ' ');
			}
			sfputr(sfstdout, *s ? s : "undefined", '\n');
		}
	} while (*argv && (name = *++argv));
	return(error_info.errors != 0);
}

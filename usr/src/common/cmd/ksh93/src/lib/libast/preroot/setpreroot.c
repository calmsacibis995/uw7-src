#ident	"@(#)ksh93:src/lib/libast/preroot/setpreroot.c	1.1"
#pragma prototyped
/*
 * AT&T Bell Laboratories
 * force current command to run under dir preroot
 */

#include <ast.h>
#include <preroot.h>

#if FS_PREROOT

#include <option.h>

void
setpreroot(register char** argv, const char* dir)
{
	register char*	s;
	register char**	ap;
	int		argc;
	char*		cmd;
	char**		av;
	char		buf[PATH_MAX];

	if ((argv || (argv = opt_info.argv)) && (dir || (dir = getenv(PR_BASE)) && *dir) && !ispreroot(dir) && (*(cmd = *argv++) == '/' || (cmd = pathpath(buf, cmd, NiL, PATH_ABSOLUTE|PATH_REGULAR|PATH_EXECUTE))))
	{
		argc = 3;
		for (ap = argv; *ap++; argc++);
		if (av = newof(0, char*, argc, 0))
		{
			ap = av;
			*ap++ = PR_COMMAND;
			*ap++ = dir;
			*ap++ = cmd;
			while (*ap++ = *argv++);
			if (!(s = getenv(PR_SILENT)) || !*s)
			{
				sfprintf(sfstderr, "+");
				ap = av;
				while (s = *ap++)
					sfprintf(sfstderr, " %s", s);
				sfprintf(sfstderr, "\n");
				sfsync(sfstderr);
			}
			execv(*av, av);
			free(av);
		}
	}
}

#else

NoN(setpreroot)

#endif

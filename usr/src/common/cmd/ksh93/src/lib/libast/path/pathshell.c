#ident	"@(#)ksh93:src/lib/libast/path/pathshell.c	1.1"
#pragma prototyped
/*
 * G. S. Fowler
 * D. G. Korn
 * AT&T Bell Laboratories
 *
 * shell library support
 */

#include <ast.h>

/*
 * return pointer to the full path name of the shell
 *
 * SHELL is read from the environment and must start with /
 *
 * if set-uid or set-gid then the executable and its containing
 * directory must not be writable by the real user
 *
 * astconf("SHELL",NiL,NiL) is returned by default
 *
 * NOTE: csh is rejected because the bsh/csh differentiation is
 *       not done for `csh script arg ...'
 */

char*
pathshell(void)
{
	register char*	s;
	register char*	sh;
	register int	i;

	static char*	val;

	if ((sh = getenv("SHELL")) && *sh == '/' && strmatch(sh, "*/(sh|*[!cC]sh)"))
	{
		if (!(i = getuid()))
		{
			if (!strmatch(sh, "?(/usr)?(/local)/?(l)bin/?([a-z])sh")) goto defshell;
		}
		else if (i != geteuid() || getgid() != getegid())
		{
			if (!access(sh, W_OK)) goto defshell;
			s = strrchr(sh, '/');
			*s = 0;
			i = access(sh, W_OK);
			*s = '/';
			if (!i) goto defshell;
		}
		return(sh);
	}
 defshell:
	if (!(sh = val))
	{
		if (!*(sh = astconf("SHELL", NiL, NiL)) || !(sh = strdup(sh)))
			sh = "/bin/sh";
		val = sh;
	}
	return(sh);
}

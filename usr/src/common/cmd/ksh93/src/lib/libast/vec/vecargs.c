#ident	"@(#)ksh93:src/lib/libast/vec/vecargs.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * string vector argv insertion
 */

#include <ast.h>
#include <vecargs.h>
#include <ctype.h>

/*
 * insert the string vector vec between
 * (*argvp)[0] and (*argvp)[1], sliding (*argvp)[1] ... over
 * null and blank args are deleted
 *
 * vecfree always called
 *
 * -1 returned if insertion failed
 */

int
vecargs(register char** vec, int* argcp, char*** argvp)
{
	register char**	argv;
	register char**	oargv;
	char**		ovec;
	char*		s;
	int		num;

	if (!vec) return(-1);
	if ((num = (char**)(*(vec - 1)) - vec) > 0)
	{
		if (!(argv = newof(0, char*, num + *argcp + 1, 0)))
		{
			vecfree(vec, 0);
			return(-1);
		}
		oargv = *argvp;
		*argvp = argv;
		*argv++ = *oargv++;
		ovec = vec;
		while (s = *argv = *vec++)
		{
			while (isspace(*s)) s++;
			if (*s) argv++;
		}
		vecfree(ovec, 1);
		while (*argv = *oargv++) argv++;
		*argcp = argv - *argvp;
	}
	else vecfree(vec, 0);
	return(0);
}

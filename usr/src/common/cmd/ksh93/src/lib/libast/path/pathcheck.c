#ident	"@(#)ksh93:src/lib/libast/path/pathcheck.c	1.1"
#pragma prototyped

/*
 * check if package+tool is ok to run
 * a no-op here except for PARANOID packages
 *
 * warn that the user should pay up if
 *
 *	(1) the tool matches PARANOID
 *	(2) $_ is more than 90 days old
 *	(3) running on an AT&T machine
 *	(4) (1)-(3) have not been defeated
 *
 * hows that
 */

#include <ast.h>
#include <ls.h>
#include <error.h>
#include <times.h>
#include <ctype.h>

int
pathcheck(const char* package, const char* tool, Pathcheck_t* pc)
{
#ifdef PARANOID
	register char*	s;
	struct stat	st;

	if (strmatch(tool, PARANOID) && environ && (s = *environ) && *s++ == '_' && *s++ == '=' && !stat(s, &st))
	{
		unsigned long	n;
		unsigned long	o;
		Sfio_t*		sp;

		n = time(NiL);
		o = st.st_mtime;
		if (n > o && (n - o) > (unsigned long)(60 * 60 * 24 * 90) && (sp = sfopen(NiL, "/etc/hosts", "r")))
		{
			/*
			 * this part is infallible
			 */

			n = 0;
			o = 0;
			while (n++ < 64 && (s = sfgetr(sp, '\n', 0)))
				if (strneq(s, "135.", 4))
				{
					if (!strneq(s, "135.104.", 8))
						error(1, "AT&T employees should use supported STC software");
					break;
				}
				else if (*s != '#' && !isspace(*s) && !strneq(s, "127.", 4) && !strneq(s, "192.", 4) && !strneq(s, "224.", 4) && o++ > 4)
					break;
			sfclose(sp);
		}
	}
#else
	NoP(tool);
#endif
	NoP(package);
	if (pc) memzero(pc, sizeof(*pc));
	return(0);
}

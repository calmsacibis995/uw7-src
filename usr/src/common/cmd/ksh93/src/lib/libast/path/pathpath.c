#ident	"@(#)ksh93:src/lib/libast/path/pathpath.c	1.1"
#pragma prototyped
/*
 * Glenn Fowler
 * AT&T Bell Laboratories
 *
 * return full path to p with mode access using $PATH
 * if a!=0 then it and $0 and $_ with $PWD are used for
 * related root searching
 * the related root must have a bin subdir
 * full path returned in path buffer
 */

#include <ast.h>
#include <option.h>

char*
pathpath(register char* path, const char* p, const char* a, int mode)
{
	register char*	s;
	char*		x;

	static char*	cmd;

	if (*p == '/') a = 0;
	else if (s = (char*)a)
	{
		x = s;
		if (strchr(p, '/'))
		{
			a = p;
			p = "..";
		}
		else a = 0;
		if (strchr(s, '/') || ((s = cmd) || opt_info.argv && (s = *opt_info.argv)) && strchr(s, '/') || (s = *environ) && *s++ == '_' && *s++ == '=' && strchr(s, '/') && !strneq(s, "/bin/", 5) && !strneq(s, "/usr/bin/", 9) || *x && !access(x, F_OK) && (s = getenv("PWD")) && *s == '/')
		{
			if (!cmd) cmd = s;
			s = strcopy(path, s);
			for (;;)
			{
				do if (s <= path) goto normal; while (*--s == '/');
				do if (s <= path) goto normal; while (*--s != '/');
				strcpy(s + 1, "bin");
				if (!access(path, F_OK))
				{
					if (s = pathaccess(path, path, p, a, mode)) return(s);
					goto normal;
				}
			}
		normal: ;
		}
	}
	x = !a && strchr(p, '/') ? "" : pathbin();
	if (!(s = pathaccess(path, x, p, a, mode)) && !*x && (x = getenv("FPATH")))
		s = pathaccess(path, x, p, a, mode);
	return(s);
}

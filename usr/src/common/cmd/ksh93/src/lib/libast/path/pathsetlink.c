#ident	"@(#)ksh93:src/lib/libast/path/pathsetlink.c	1.1"
#pragma prototyped
/*
* Glenn Fowler
* AT&T Bell Laboratories
*/

#include "univlib.h"

/*
 * create symbolic name from external representation text in buf
 * the arg order matches link(2)
 */

int
pathsetlink(const char* buf, const char* name)
{
	register char*	t = (char*)buf;
#ifdef UNIV_MAX
	register char*	s = (char*)buf;
	register char*	v;
	int		n;
	char		tmp[PATH_MAX];

	while (*s)
	{
		if (*s++ == univ_cond[0] && !strncmp(s - 1, univ_cond, univ_size))
		{
			s--;
			t = tmp;
			for (n = 0; n < UNIV_MAX; n++)
				if (*univ_name[n])
			{
				*t++ = ' ';
#ifdef ATT_UNIV
				*t++ = '1' + n;
				*t++ = ':';
#else
				for (v = univ_name[n]; *t = *v++; t++);
				*t++ = '%';
#endif
				for (v = (char*)buf; v < s; *t++ = *v++);
				for (v = univ_name[n]; *t = *v++; t++);
				for (v = s + univ_size; *t = *v++; t++);
			}
			t = tmp;
			break;
		}
	}
#endif
	return(symlink(t, name));
}

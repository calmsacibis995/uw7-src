/*		copyright	"%c%" 	*/

#ident	"@(#)syntax.c	1.2"
#ident	"$Header$"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "ctype.h"
#include "string.h"

#include "lp.h"
#include "sys/statvfs.h"

u_long Namemax = 0;

int
#if	defined(__STDC__)
syn_name (
	char *			str
)
#else
syn_name (str)
	char			*str;
#endif
{
	register char		*p;
	statvfs_t		buf;

	if (!*str)
	  	return(0);
	if (!Namemax)
		if (statvfs(ETCDIR, &buf) < 0)
			return (0);
		else
			Namemax = buf.f_namemax;

	if ((u_long)strlen(str) > Namemax)
		return (0);

	for (p = str; *p; p++)
		if (!isprint(*p) || *p == ' ' || *p == '/' || *p == '\\' ||
		       *p == ':' || *p == ';' || *p == ',' || *p == '*' ||
		       *p == '?' || *p == '~')
			return (0);

	return (1);
}

int
#if	defined(__STDC__)
syn_type (
	char *			str
)
#else
syn_type (str)
	char			*str;
#endif
{
	register char		*p;

	if ((int)strlen(str) > 14)
		return (0);

	for (p = str; *p; p++)
		if (!isalnum(*p) && *p != '-')
			return (0);

	return (1);
}

int
#if	defined(__STDC__)
syn_text (
	char *			str
)
#else
syn_text (str)
	char			*str;
#endif
{
	register char		*p;

	for (p = str; *p; p++)
		if (!isgraph(*p) && *p != '\t' && *p != ' ')
			return (0);

	return (1);
}

int
#if	defined(__STDC__)
syn_comment (
	char *			str
)
#else
syn_comment (str)
	char			*str;
#endif
{
	register char		*p;

	for (p = str; *p; p++)
		if (!isgraph(*p) && *p != '\t' && *p != ' ' && *p != '\n')
			return (0);

	return (1);
}

int
#if	defined(__STDC__)
syn_machine_name (
	char *			str
)
#else
syn_machine_name (str)
	char			*str;
#endif
{
	if ((int)strlen(str) > 8)
		return (0);

	return (1);
}

int
#if	defined(__STDC__)
syn_option (
	char *			str
)
#else
syn_option (str)
	char			*str;
#endif
{
	register char		*p;

	for (p = str; *p; p++)
		if (!isprint(*p))
			return (0);

	return (1);
}

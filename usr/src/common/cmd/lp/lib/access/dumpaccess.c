/*		copyright	"%c%" 	*/

#ident	"@(#)dumpaccess.c	1.2"
#ident	"$Header$"

#include "stdio.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"
#include "access.h"
#include "printers.h"

#if	defined(__STDC__)
static int		_dumpaccess ( char * , char ** );
#else
static int		_dumpaccess();
#endif

/**
 ** dumpaccess() - DUMP ALLOW OR DENY LISTS
 **/

int
#if	defined(__STDC__)
dumpaccess (
	char *			dir,
	char *			name,
	char *			prefix,
	char ***		pallow,
	char ***		pdeny
)
#else
dumpaccess (dir, name, prefix, pallow, pdeny)
	char			*dir,
				*name,
				*prefix,
				***pallow,
				***pdeny;
#endif
{
	register char		*allow_file	= 0,
				*deny_file	= 0;

	int			ret;

	if (
		!(allow_file = getaccessfile(dir, name, prefix, "allow"))
	     || _dumpaccess(allow_file, *pallow) == -1 && errno != ENOENT
	     || !(deny_file = getaccessfile(dir, name, prefix, "deny"))
	     || _dumpaccess(deny_file, *pdeny) == -1 && errno != ENOENT
	)
		ret = -1;
	else
		ret = 0;

	if (allow_file)
		Free (allow_file);
	if (deny_file)
		Free (deny_file);

	return (ret);
}

/**
 ** _dumpaccess() - DUMP ALLOW OR DENY FILE
 **/

static int
#if	defined(__STDC__)
_dumpaccess (
	char *			file,
	char **			list
)
#else
_dumpaccess (file, list)
	char			*file,
				**list;
#endif
{
	register char		**pl;

	register int		ret;

	FILE			*fp;

	level_t			lid;

	int			n;

	if (list) {
		if (!(fp = open_lpfile(file, "w", MODE_READ)))
			return (-1);
		for (pl = list; *pl; pl++)
			(void) fprintf (fp, "%s\n", *pl);
		if (ferror(fp))
			ret = -1;
		else
			ret = 0;
		close_lpfile (fp);
		lid = PR_SYS_PUBLIC;
		while ((n=lvlfile (file, MAC_SET, &lid)) < 0 &&
		       errno == EINTR)
		    continue;

		if (n < 0 && errno != ENOSYS)
		    ret = -1;

	} else
		ret = Unlink(file);

	return (ret);
}

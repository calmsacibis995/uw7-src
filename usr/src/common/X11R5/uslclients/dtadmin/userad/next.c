/*		copyright	"%c%" 	*/

#ident	"@(#)dtadmin:userad/next.c	1.1"

#include "string.h"
#include "errno.h"
#include "findlocales.h"

#if	defined(__STDC__)
static int		is ( char *, char *, unsigned int );
#else
static int		is();
#endif

/**
 ** next_x() - GO TO NEXT ENTRY UNDER PARENT DIRECTORY
 **/

char *
#if	defined(__STDC__)
next_x (
	char *			parent,
	long *			lastdirp,
	unsigned int		what
)
#else
next_x (parent, lastdirp, what)
	char			*parent;
	long			*lastdirp;
	unsigned int		what;
#endif
{
	DIR			*dirp;

	register char		*ret = 0;

	struct dirent		*direntp;


	if (!(dirp = opendir(parent)))
		return (0);

	if (*lastdirp != -1)
		seekdir (dirp, *lastdirp);

	do
		direntp = readdir(dirp);
	while (
		direntp
	     && (
			STREQU(direntp->d_name, ".")
		     || STREQU(direntp->d_name, "..")
		     || !is(parent, direntp->d_name, what)
		)
	);

	if (direntp) {
		if (!(ret = strdup(direntp->d_name)))
			errno = ENOMEM;
		*lastdirp = telldir(dirp);
	} else {
		errno = ENOENT;
		*lastdirp = -1;
	}

	closedir (dirp);

	return (ret);
}

static int
#if	defined(__STDC__)
is (
	char *			parent,
	char *			name,
	unsigned int		what
)
#else
is (parent, name, what)
	char			*parent;
	char			*name;
	unsigned int		what;
#endif
{
	char			*path;

	struct stat		statbuf;

	if (!(path = makepath(parent, name, (char *)0)))
		return (0);
	if (stat(path, &statbuf) == -1) {
		free (path);
		return (0);
	}
	free (path);
	return ((statbuf.st_mode & S_IFMT) == what);
}

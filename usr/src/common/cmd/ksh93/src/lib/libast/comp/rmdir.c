#ident	"@(#)ksh93:src/lib/libast/comp/rmdir.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_rmdir

NoN(rmdir)

#else

#include <ls.h>
#include <error.h>

int
rmdir(const char* path)
{
	register int	n;
	struct stat	st;
	char*		av[3];

	static char*	cmd[] = { "/bin/rmdir", "/usr/5bin/rmdir", 0 };

	if (stat(path, &st) < 0) return(-1);
	if (!S_ISDIR(st.st_mode))
	{
		errno = ENOTDIR;
		return(-1);
	}
	av[0] = "rmdir";
	av[1] = path;
	av[2] = 0;
	for (n = 0; n < elementsof(cmd); n++)
		if (procclose(procopen(cmd[n], av, NiL, NiL, 0)) != -1)
			break;
	n = errno;
	if (access(path, F_OK) < 0)
	{
		errno = n;
		return(0);
	}
	errno = EPERM;
	return(-1);
}

#endif

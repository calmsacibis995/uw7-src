#ident	"@(#)ksh93:src/lib/libast/comp/mkdir.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_mkdir

NoN(mkdir)

#else

#include <ls.h>
#include <wait.h>
#include <error.h>

int
mkdir(const char* path, mode_t mode)
{
	register int	n;
	char*		av[3];

	static char*	cmd[] = { "/bin/mkdir", "/usr/5bin/mkdir", 0 };


	n = errno;
	if (!access(path, F_OK))
	{
		errno = EEXIST;
		return(-1);
	}
	if (errno != ENOENT) return(-1);
	errno = n;
	av[0] = "mkdir";
	av[1] = path;
	av[2] = 0;
	for (n = 0; n < elementsof(cmd); n++)
		if (procclose(procopen(cmd[n], av, NiL, NiL, 0)) != -1)
			break;
	return(chmod(path, mode));
}

#endif

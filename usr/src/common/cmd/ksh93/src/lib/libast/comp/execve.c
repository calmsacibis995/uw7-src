#ident	"@(#)ksh93:src/lib/libast/comp/execve.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_execve

NoN(execve)

#else

#include <sig.h>
#include <wait.h>
#include <error.h>

static pid_t		childpid;

static void
execsig(int sig)
{
	kill(childpid, sig);
	signal(sig, execsig);
}

int
execve(const char* path, char* const argv[], char* const arge[])
{
	int	status;

	if ((childpid = spawnve(path, argv, arge)) < 0)
		return(-1);
	for (status = 0; status < 64; status++)
		signal(status, execsig);
	while (waitpid(childpid, &status, 0) == -1)
		if (errno != EINTR) return(-1);
	if (WIFSIGNALED(status))
	{
		signal(WTERMSIG(status), SIG_DFL);
		kill(getpid(), WTERMSIG(status));
		pause();
	}
	else status = WEXITSTATUS(status);
	exit(status);
}

#endif

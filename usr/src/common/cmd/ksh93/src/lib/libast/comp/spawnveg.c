#ident	"@(#)ksh93:src/lib/libast/comp/spawnveg.c	1.1"
#pragma prototyped

/*
 * spawnveg -- spawnve with process group or session control
 *
 *	pgid	<0	setsid()	[session group leader]
 *		 0	nothing		[retain session and process group]
 *		 1	setpgid(0,0)	[process group leader]
 *		>1	setpgid(0,pgid)	[join process group]
 */

#include <ast.h>

#if _lib_spawnveg

NoN(spawnveg)

#else

#include <error.h>

#ifndef ENOSYS
#define ENOSYS	EINVAL
#endif

#if _lib_vfork
#if _hdr_vfork
#include	<vfork.h>
#endif
#if _sys_vfork
#include	<sys/vfork.h>
#endif
#endif

#if !_lib_spawnve
#ifdef _map_spawnve
#undef	spawnve
#define spawnve	_map_spawnve
extern pid_t	spawnve(const char*, char* const[], char* const[]);
#undef	_lib_spawnve
#define _lib_spawnve	1
#endif
#endif

/*
 * fork+exec+(setsid|setpgid) with script check to avoid shell double fork
 */

pid_t
spawnveg(const char* cmd, char* const argv[], char* const envv[], pid_t pgid)
{
#if _lib_fork || _lib_vfork
	int	n;
	pid_t	pid;
#if !_real_vfork
	int	err[2];
#endif
#endif

#if _lib_spawnve
#if _lib_fork || _lib_vfork
	if (!pgid)
#endif
		return(spawnve(cmd, argv, envv));
#endif
#if _lib_fork || _lib_vfork
	n = errno;
#if _real_vfork
	errno = 0;
#else
	if (pipe(err) < 0) err[0] = -1;
	else
	{
		fcntl(err[0], F_SETFD, FD_CLOEXEC);
		fcntl(err[1], F_SETFD, FD_CLOEXEC);
	}
#endif
	sigcritical(1);
#if _lib_vfork
	pid = vfork();
#else
	pid = fork();
#endif
	sigcritical(0);
	if (!pid)
	{
		if (pgid < 0)
			setsid();
		else if (pgid > 0)
		{
			if (pgid == 1)
				pgid = 0;
			if (setpgid(0, pgid) < 0 && pgid && errno == EPERM)
				setpgid(0, 0);
		}
		execve(cmd, argv, envv);
		if (errno == ENOEXEC)
		{
			register char**	o;
			register char**	p;
			register char**	v;

			for (p = o = (char**)argv; *p; p++);
			if (v = newof(0, char*, p - o + 2, 0))
			{
				p = v;
				if (*p = *o) o++;
				else *p = (char*)cmd;
				*++p = (char*)cmd;
				while (*++p = *o++);
				execve(pathshell(), v, envv);
				free(v);
			}
#ifdef ENOMEM
			else errno = ENOMEM;
#endif
		}
#if !_real_vfork
		if (err[0] != -1)
		{
			n = errno;
			write(err[1], &n, sizeof(n));
		}
#endif
		_exit(errno == ENOENT ? EXIT_NOTFOUND : EXIT_NOEXEC);
	}
	else if (pid != -1)
	{
#if _real_vfork
		if (errno) pid = -1;
		else
#endif
		{
			if (pgid > 0)
			{
				/*
				 * parent and child are in a race here
				 */

				if (pgid == 1)
					pgid = pid;
				if (setpgid(pid, pgid) < 0 && pid != pgid && errno == EPERM)
					setpgid(pid, pid);
			}
			errno = n;
		}
	}
#if !_real_vfork
	if (err[0] != -1)
	{
		close(err[1]);
		if (read(err[0], &n, sizeof(n)) == sizeof(n))
		{
			while (waitpid(pid, NiL, 0) == -1 && errno == EINTR);
			pid = -1;
			errno = n;
		}
		close(err[0]);
	}
#endif
	return(pid);
#else
	errno = ENOSYS;
	return(-1);
#endif
}

#endif

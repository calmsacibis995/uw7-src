#ident	"@(#)ksh93:src/lib/libast/comp/setsid.c	1.1"
#pragma prototyped

#include <ast.h>

#ifdef _lib_setsid

NoN(setsid)

#else

#include <ast_tty.h>
#include <error.h>

/*
 * become new process group leader and drop control tty
 */

pid_t
setsid(void)
{
	int	pg;
#ifdef TIOCNOTTY
	int	fd;
#endif

	/*
	 * become a new process group leader
	 */

	if ((pg = getpid()) == getpgrp())
	{
		errno = EPERM;
		return(-1);
	}
	setpgid(pg, pg);
#ifdef TIOCNOTTY

	/*
	 * drop the control tty
	 */

	if ((fd = open("/dev/tty", 0)) >= 0)
	{
		ioctl(fd, TIOCNOTTY, 0);
		close(fd);
	}
#else

	/*
	 * second child in s5 to avoid reacquiring the control tty
	 */

#if _lib_fork && HUH920711 /* some s5's botch this */
	switch (fork())
	{
	case -1:
		exit(1);
	case 0:
		break;
	default:
		exit(0);
	}
#endif

#endif
	return(pg);
}

#endif

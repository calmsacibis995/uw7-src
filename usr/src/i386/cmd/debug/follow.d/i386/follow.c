#ident	"@(#)debugger:follow.d/i386/follow.c	1.2"

/* Minimal follower process. 
 * Sends SIGUSR1 to debugger when subject stops. 
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/procset.h>
#include <sys/procfs.h>
#include <unistd.h>

#ifndef OLD_PROC
#include <limits.h>
#endif

#ifdef FDEBUG
#define	DPRINT(M)	printf(M)
#else
#define	DPRINT(M)
#endif

static int	debugger;
static char	*filename;

static void
self_exit(int sig)
{
	kill(debugger, SIGUSR1);
	exit(0);
}

static void
myintr(int sig)
{
	if (sig == SIGUSR2)
		exit(0);
}

#if OLD_PROC

static int	fd = -1;

static int
open_proc()
{
	do {
		errno = 0;
		fd = open( filename, O_RDONLY );
	} while ( errno == EINTR );
	return ( errno == 0 );
}

static int 
err_handle(int err)
{
	int	nfd;

	if (err != EAGAIN)
		return 0;
	
	do {
		errno = 0;
		nfd = open( filename, O_RDONLY);
	} while (errno == EINTR);
	if (errno)
		return 0;
	close(fd);
	fd = nfd;
	return 1;
} 

static int
wait_for()
{
	do {
		errno = 0;
		ioctl( fd, PIOCWSTOP, 0 );
	} while ( errno == EINTR || err_handle(errno));
	return ( errno == 0 );
}

static int
get_status()
{
	struct prstatus	prstat;
	do {
		errno = 0;
		ioctl( fd, PIOCSTATUS, &prstat );
	} while ( errno == EINTR );
	return ( errno == 0 );
}

#else 	/* new /proc */

static int	ctl_fd = -1;
static int	status_fd = -1;

static int 
err_handle(int err)
{
	int	nctl_fd;
	int	nstatus_fd;
	char	fname[PATH_MAX];

	if (err != EAGAIN)
		return 0;

	/* process has exec'd a setuid or setgid program - 
	 * try to regain control
	 */
	sprintf(fname, "%s/%s", filename, "ctl");
	do {
		errno = 0;
		nctl_fd = open( fname, O_WRONLY );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	
	sprintf(fname, "%s/%s", filename, "status");
	do {
		errno = 0;
		nstatus_fd = open( fname, O_RDONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		close(nctl_fd);
		return 0;
	}
	close(ctl_fd);
	close(status_fd);
	ctl_fd = nctl_fd;
	status_fd = nstatus_fd;
	return 1;
} 

static int
wait_for()
{
	int	msg = PCWSTOP;
	DPRINT(("follow: wait_for\n"));
	do {
		errno = 0;
		write(ctl_fd, (char *)&msg, sizeof(int));
	} while(errno == EINTR || err_handle(errno));
	return (errno == 0);
}
	
static int
get_status()
{
	pstatus_t	prstat;
	DPRINT(("follow: get_status\n"));
	do {
		errno = 0;
		read(status_fd, (char *)&prstat, sizeof(pstatus_t));
	} while(errno == EINTR);
	return(errno == 0);
}
	
static int
open_proc()
{
	char	fname[PATH_MAX];

	/* open ctl and status files */
	DPRINT(("follow: open_proc - filename = %s\n", filename));
	sprintf(fname, "%s/%s", filename, "ctl");
	do {
		errno = 0;
		ctl_fd = open( fname, O_WRONLY );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	
	sprintf(fname, "%s/%s", filename, "status");
	do {
		errno = 0;
		status_fd = open( fname, O_RDONLY );
	} while ( errno == EINTR );
	if (errno)
	{
		close(ctl_fd);
		return 0;
	}
	return 1;
}
#endif

main(int argc, char **argv)
{
	sigset_t		set, nset;
	struct sigaction	act;

	sigset(SIGINT, SIG_IGN);
	
	if (argc != 3) 
	{
		fprintf(stderr, "%s: invalid argument list.\n",argv[0]);
		exit(1);
	}

	filename = argv[1];
	if (open_proc() == 0)
	{
		fprintf(stderr, "%s: couldn't open %s: %s\n",argv[0], argv[1], strerror(errno));
		exit(2);
	}

	(void)sscanf(argv[2], "%d", &debugger);
	if (debugger == 0) 
	{
		fprintf(stderr, "%s: couldn't find debugger\n", argv[0]);
		exit(1);
	}


	premptyset(&set);
	premptyset(&nset);
	/* block SIGUSR1 when not suspended */
	praddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, 0);

	act.sa_handler = myintr;
	premptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGUSR1, &act, 0);
	sigaction(SIGUSR2, &act, 0);


	for(;;) 
	{
		sigsuspend(&nset);

		(void)wait_for();

		sigsend(P_PID, debugger, SIGUSR1);

		if (!get_status())
		{
			DPRINT(("follow: get_status failed\n"));
			sigset(SIGALRM, self_exit);
			alarm(2);
		}
	}
}

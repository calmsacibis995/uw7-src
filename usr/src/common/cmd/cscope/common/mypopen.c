#ident	"@(#)cscope:common/mypopen.c	1.4"
/* static char Sccsid[] = "@(#)popen.c	5.3 u370 source"; */
/*	@(#)popen.c	1.3	*/
/*	3.0 SID #	1.4	*/
/*LINTLIBRARY*/
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include "global.h"	/* pid_t, SIGTYPE, shell, and basename() */

#define	tst(a,b) (*mode == 'r'? (b) : (a))
#define	RDR	0
#define	WTR	1
#define CLOSE_ON_EXEC	1

static pid_t popen_pid[20];
static SIGTYPE (*tstat)();

myopen(path, flag, mode)
char *path;
int flag;
int mode;
{
	/* opens a file descriptor and then sets close-on-exec for the file */
	int fd;

	if(mode)
		fd = open(path, flag, mode);
	else
		fd = open(path, flag);

	if(fd != -1 && (fcntl(fd, F_SETFD, CLOSE_ON_EXEC) != -1))
		return(fd);

	else return(-1);
}

FILE *
myfopen(path, mode)
char *path;
char *mode;
{
	/* opens a file pointer and then sets close-on-exec for the file */
	FILE *fp;

	fp = fopen(path, mode);

	if(fp && (fcntl(fileno(fp), F_SETFD, CLOSE_ON_EXEC) != -1))
		return(fp);

	else return(NULL);
}

FILE *
mypopen(cmd, mode)
char	*cmd, *mode;
{
	int	p[2];
	register pid_t *poptr;
	register int myside, yourside;
	register pid_t pid;

	if(pipe(p) < 0)
		return(NULL);
	myside = tst(p[WTR], p[RDR]);
	yourside = tst(p[RDR], p[WTR]);
	if((pid = fork()) == 0) {
		/* myside and yourside reverse roles in child */
		int	stdio;

		/* close all pipes from other popen's */
		for (poptr = popen_pid; poptr < popen_pid+20; poptr++) {
			if(*poptr)
				(void) close(poptr - popen_pid);
		}
		stdio = tst(0, 1);
		(void) close(myside);
		(void) close(stdio);
#if V9
		(void) dup2(yourside, stdio);
#else
		(void) fcntl(yourside, F_DUPFD, stdio);
#endif
		(void) close(yourside);
		(void) execlp(shell, basename(shell), "-c", cmd, 0);
		_exit(1);
	} else if (pid > 0)
		tstat = signal(SIGTSTP, SIG_DFL);
	if(pid == -1)
		return(NULL);
	popen_pid[myside] = pid;
	(void) close(yourside);
	return(fdopen(myside, mode));
}

int
pclose(ptr)
FILE	*ptr;
{
	register int f;
	register pid_t r;
	int status;
	SIGTYPE (*hstat)(), (*istat)(), (*qstat)();

	f = fileno(ptr);
	(void) fclose(ptr);
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	hstat = signal(SIGHUP, SIG_IGN);
	while((r = wait(&status)) != popen_pid[f] && r != -1)
		;
	if(r == -1)
		status = -1;
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	(void) signal(SIGHUP, hstat);
	(void) signal(SIGTSTP, tstat);
	/* mark this pipe closed */
	popen_pid[f] = 0;
	return(status);
}

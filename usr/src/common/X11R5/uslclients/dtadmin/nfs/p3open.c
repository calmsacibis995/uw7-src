#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/p3open.c	1.11"
#endif

/* This file was appropriated from dtmail.
 * Its ident string in that component was "@(#)dtmail:p3open.c	1.1"
 */
/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	p3open.c      open/close pipes to a shell command
 */

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <stropts.h>
#include <errno.h>

#include "proc.h"

XtInputId Id_read, Id_except;

extern FILE	*fdopen();
extern int	close(),
		execl(),
		fork(),
		pipe();
void	_exit();
static pid_t	popen_pid[256];

int
p3open(cmd, fp)
const char	*cmd;
FILE	*fp[2]; /* file pointer array to cmd stdin and stdout */
{
/*	int		tocmd[2];
*/
        int		fromcmd[2];
	register pid_t	pid;

	if(/* pipe(tocmd) < 0 || */ pipe(fromcmd) < 0)
		return  -1;
	if(/* tocmd[1] >= 256 || */ fromcmd[0] >= 256) {
/*		(void) close(tocmd[0]);
**		(void) close(tocmd[1]);
*/
	        (void) close(fromcmd[0]);
		(void) close(fromcmd[1]);
		return -1;
	}
	if((pid = fork()) == 0) {
/*		(void) close( tocmd[1] );
*/
	        (void) close( 0 );
/*		(void) fcntl( tocmd[0], F_DUPFD, 0 );
**		(void) close( tocmd[0] );
*/
		(void) close( fromcmd[0] );
		(void) close( 1 );
		(void) fcntl( fromcmd[1], F_DUPFD, 1 );
		(void) close( 2 );
		(void) fcntl( fromcmd[1], F_DUPFD, 2 );
		(void) close( fromcmd[1] );
/*		if (ioctl(0, I_PUSH, "ptem") < 0) {
			fprintf (stderr, "can't push ptem on 0\n");
			perror ("ptem");
			exit (1);
		}
		if (ioctl(1, I_PUSH, "ptem") < 0) {
			fprintf (stderr, "can't push ptem on 1\n");
			perror ("ptem");
			exit (1);
		}
*/		(void) execl("/sbin/sh", "sh", "-c", cmd, 0);
		perror ("child");
		_exit(1);
	}
	if(pid == (pid_t)-1)
	{
	    (void) close(fromcmd[0]);
            (void) close(fromcmd[1]);
	    return  -1;
	}
/*	popen_pid[ tocmd[1] ] = pid;
*/
	popen_pid[ fromcmd[0] ] = pid;
/*	(void) close( tocmd[0] );
*/
	(void) close( fromcmd[1] );
/*	fp[0] = fdopen( tocmd[1], "w" );
*/
	fp[1] = fdopen( fromcmd[0], "r" );
/*	setbuf (fp[0], NULL);
*/
	setbuf (fp[1], NULL);
	return  0;
}

int
p3close(fp, pid_in)
FILE	*fp[2];
pid_t    pid_in;
{
        extern XtAppContext	AppContext;
	int		status;
	pid_t		waitpid();
	void		(*hstat)(),
			(*istat)(),
			(*qstat)();
	pid_t pid, r;

	if (pid_in == BAD_PID)	/* close for p3open */
	{
	    pid = popen_pid[fileno(fp[1])];
	    popen_pid[fileno(fp[1])] = 0;
	}
	else 			/*  close for popenAddInput */
	    pid = pid_in;

	(void) fclose(fp[1]);
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	hstat = signal(SIGHUP, SIG_IGN);
	while ((r = waitpid(pid, &status, 0)) == (pid_t)-1 
		&& errno == EINTR)
			;
	if (r == (pid_t)-1)
	{
	    status = -1;
	}
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	(void) signal(SIGHUP, hstat);
	return  status;
}

extern int
popenAddInput(char * cmd, XtInputCallbackProc proc,
	      XtInputCallbackProc proc2, XtPointer client_data) 
{
        extern XtAppContext AppContext;
	int		fromcmd[2];
	register pid_t	pid;
	inputProcData   *data = (inputProcData *)client_data;

	if(pipe(fromcmd) < 0)
		return  -1;
	if(fromcmd[0] >= 256) {
		(void) close(fromcmd[0]);
		(void) close(fromcmd[1]);
		return -1;
	}
	if((pid = fork()) == 0) {
		(void) close( 0 );
		(void) close( fromcmd[0] );
		(void) close( 1 );
		(void) fcntl( fromcmd[1], F_DUPFD, 1 );
		(void) close( 2 );
		(void) fcntl( fromcmd[1], F_DUPFD, 2 );
		(void) close( fromcmd[1] );
/*
                if (ioctl(0, I_PUSH, "ptem") < 0) {
			fprintf (stderr, "can't push ptem on 0\n");
			perror ("ptem");
			exit (1);
		}
		if (ioctl(1, I_PUSH, "ptem") < 0) {
			fprintf (stderr, "can't push ptem on 1\n");
			perror ("ptem");
			exit (1);
		}
*/
		(void) execl("/sbin/sh", "sh", "-c", cmd, 0);
		perror ("child");
		_exit(1);
	}
	if(pid == (pid_t)-1)
	{
	    (void) close(fromcmd[0]);
            (void) close(fromcmd[1]);
	    return  -1;
	}
	data-> pid = pid;
	(void) close( fromcmd[1] );
	data-> fp[1] = fdopen( fromcmd[0], "r" );
	setbuf (((inputProcData *)client_data)-> fp[1], NULL);
	data-> exceptId = XtAppAddInput(AppContext, fromcmd[0],
				     (XtPointer)XtInputExceptMask,
				     proc2, client_data); 
	data-> readId   = XtAppAddInput(AppContext, fromcmd[0],
				     (XtPointer)XtInputReadMask, 
				     proc, client_data); 
	return  0;
}

#ident "@(#)pipe.c	26.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1995-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifndef _KMEMUSER
#define _KMEMUSER    /* for queue_t.  Note this prevents including
		      * the threads synch.h header file here due to rwlock_t
		      * clashes -- 
		      */
#endif

#include "include.h"

int errorFd;

static
CEdisplay(fmt, a,b,c,d,e,f,g,h)
char *fmt;
{
	static char CEbuf[256];
	sprintf(CEbuf, fmt,a,b,c,d,e,f,g,h);

	write(errorFd,CEbuf,strlen(CEbuf));
}

CommandResult(fmt, a,b,c,d,e,f,g,h)
char *fmt;
{
	static char CEbuf[256];

	sprintf(CEbuf, fmt,a,b,c,d,e,f,g,h);
	CEdisplay("OK %s\n", CEbuf);
}

void
pipeHandler(int fd)
{
	extern int errorFd;
	int n;
	static char buf[256];
	char *x,*y;
	char *argv[30];
	char pid[24];
	int argc=0;

	errorFd = fd;
#ifdef DEBUG_PIPE
	Log("pipeHandler(%d) STARTS", fd);
#endif
	n=read(fd, buf, sizeof(buf));

	switch (n) {
	case -1:
		SystemError(4, "dlpid: pipeHandler: Read on pipe failed\n");
		exit(1);
	case 0:
		break;
	default:
		buf[n]=0;
#ifdef DEBUG_PIPE
		Log("pipeHandler: Read '%s'", buf);
#endif
		if ( (x = strchr(buf, '\n')) ) {
			*x=0;
		}
		x=buf;
		while ( (y = strchr(x, ' ')) ) {
			argv[argc++] = x;
			*y++=0;
			x=y;
		}
		argv[argc++] = x;

		if ( strcmp(argv[0], "restart") == 0 ) {
			if ( argc == 1 ) {
				RestartAllInterfaces();
			} else {
				RestartInterface(argv[1]);
			}
		} else if ( strcmp(argv[0], "start") == 0 ) {
			StartInterface(argv[1]);
		} else if ( strcmp(argv[0], "stop") == 0 ) {
			StopInterface(argv[1]);
		} else if ( strcmp(argv[0], "terminate") == 0 ) {
			Log("User sent 'terminate' command.  Exiting");
			sprintf(pid, "%d\n", getpid());
			CEdisplay(pid);
			StopAllInterfaces();
			sleep(1);
			exit(0);
		} else if ( strcmp(argv[0], "add") == 0 ) {
			AddInterface(argv[1], argv[2]);
			DumpIfStructs();
		} else if ( strcmp(argv[0], "remove") == 0 ) {
			RemoveInterface(argv[1], argv[2]);
			DumpIfStructs();
		} else if ( strcmp(argv[0], "ayt") == 0 ) {
			CEdisplay("OK \n");
		} else if ( strcmp(argv[0], "add_backup") == 0 ) {
			AddBackupInterface(argv[1], argv[2]);
			DumpIfStructs();
		} else if ( strcmp(argv[0], "failover") == 0 ) {
			FailoverInterface(argv[1]);
			DumpIfStructs();
		} else {
			Error(5, "dlpid: Unknown command '%s'\n", argv[0]);
		}
	}
	CEdisplay("EOF\n");
	errorFd = -1;
#ifdef DEBUG_PIPE
	Log("pipeHandler(%d) ENDS", fd);
#endif
}

OpenPipes(char *PipeName)
{
	struct stat st;
	mode_t m;
	int fd1,fd2;

	struct strfdinsert ins;
	queue_t *q;

	if (PipeName == NULL) {
		SystemError(49, "dlpid: openPipes: null pipe file name\n");
		exit(1);
	}

	if (strlen(PipeName) == 0) {
		SystemError(49, "dlpid: openPipes: null pipe file name\n");
		exit(1);
	}

	errorFd = 2;
#ifdef DEBUG_PIPE
	Log("OpenPipes STARTS");
	Log("OpenPipes: Openning %s", STREAMS_PIPE_DEVICE);
#endif
	if ( ( fd1 = open (STREAMS_PIPE_DEVICE, O_RDWR) ) == -1 ) {
		SystemError(6, "dlpid: openPipes: Unable to open pipe(%s)\n", STREAMS_PIPE_DEVICE);
		exit(1);
	}
	if ( fstat(fd1, &st) == -1 ) {
		SystemError(7, "dlpid: openPipes: fstat failed on open pipe(%s)\n", STREAMS_PIPE_DEVICE);
		exit(1);
	}
#ifdef DEBUG_PIPE
	Log("OpenPipes: Openned fd=%d, maj/min=%x", fd1, st.st_rdev);
#endif
	m=umask(0);
	unlink(PipeName);
	if ( mknod(PipeName, S_IFCHR | 0600, st.st_rdev) == -1 ) {
		SystemError(8, "dlpid: openPipes: Unable to mknod pipe(%s)\n", PipeName);
		exit(1);
	}
	umask(m);

#ifdef DEBUG_PIPE
	Log("OpenPipes: Openning#2 %s ", STREAMS_PIPE_DEVICE);
#endif
	if ( ( fd2 = open (STREAMS_PIPE_DEVICE, O_RDWR) ) == -1 ) {
		SystemError(9, "dlpid: openPipes: Unable to open pipe#2(%s)\n", STREAMS_PIPE_DEVICE);
		exit(1);
	}

	if ( fstat(fd2, &st) == -1 ) {
		SystemError(10, "dlpid: openPipes: fstat failed on open pipe#2(%s)\n", STREAMS_PIPE_DEVICE);
		exit(1);
	}
#ifdef DEBUG_PIPE
	Log("OpenPipes: Openned #2 fd=%d, maj/min=%x", fd2, st.st_rdev);
#endif
	ins.ctlbuf.buf = (char *)&q;
	ins.ctlbuf.maxlen = ins.ctlbuf.len = sizeof(q);

	ins.databuf.buf = (char *)0;
	ins.databuf.len = -1;
	ins.databuf.maxlen = 1024;

	ins.fildes = fd1;
	ins.flags = 0;
	ins.offset = 0;

#ifdef DEBUG_PIPE
	Log("OpenPipes: Sending FDINSERT(fd=%d) to fd=%d", fd1, fd2);
#endif
	if ( ioctl(fd2, I_FDINSERT, (char *)&ins) == -1 ) {
		SystemError(11, "dlpid: openPipes: ioctl(I_FDINSERT) failed on open pipe(%s)\n", STREAMS_PIPE_DEVICE);
		exit(1);
	}

	AddInput(fd2, pipeHandler);

#ifdef DEBUG_PIPE
	Log("OpenPipes ENDS");
#endif
	errorFd = -1;
}

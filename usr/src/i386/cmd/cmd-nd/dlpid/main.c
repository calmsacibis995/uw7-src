#ident "@(#)main.c	26.2"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <unistd.h>
#include "include.h"
#include "errno.h"

struct sigaction alrmhandler, alrmnohandler, intnohandler, hupnohandler;

#ifdef _REENTRANT
#include <synch.h>
mutex_t mutex;
#endif

#define NPOLL	200

int set_id = 1;
int netisl = 0;

#define NUMISLCHARS 20
char NetIslInterface[NUMISLCHARS];

static void (*poll_handlers[NPOLL])();
static struct pollfd pollfds[NPOLL];
static int npollfd = 0;

/* rather than catch every signal and exit, use atexit instead */
void
RemovePidFile(void)
{
	extern char *PidFile;

	/* It may not be there -- we don't care */
	(void) unlink(PidFile);
}

/* This function is called once all the LLI modules have been openned and linked
 * its job is to wait for messages from the LLI module which signal that the
 * MDI driver has failed. */
ServiceFds() {
#ifdef DEBUG_PIPE
	Log("ServiceFds STARTS npollfd %d", npollfd);
#endif
	while (npollfd) {
		int x,i;

#ifdef DEBUG_PIPE
		Log("ServiceFds: Poll(0x%x,%d,-1)", pollfds,npollfd);
#endif
		switch (x = poll(pollfds, npollfd, -1)) {
		case 0:
			break;
		case -1:
			SystemError(1, "dlpid: ServiceFds: Poll returned -1\n");
			if (errno != EINTR)
				exit(1);
			break;
		default:
			for (i=0; i<npollfd; i++) {
				int revent = pollfds[i].revents;
				int fd = pollfds[i].fd;

				if (revent) {
					if (revent & (POLLNVAL|POLLERR|POLLHUP)) {
						SystemError(2, "dlpid: ServiceFds: Poll returned an error (fd=%d, revent=0x%x)\n", fd, revent);
						exit(1);
					}
#ifdef DEBUG_PIPE
					Log("ServiceFds: Event(0x%x) on fd=%d", revent, fd);
#endif
					(*poll_handlers[i])(fd, revent);
				}
			}
		}
	}
#ifdef DEBUG_PIPE
	Log("ServiceFds ENDS");
#endif
}

void
AddInput(int fd, void (*service_function)())
{
	struct pollfd *pp;

	poll_handlers[npollfd] = service_function;

	pp = &pollfds[npollfd];
	pp->fd = fd;
	pp->events = POLLIN|POLLPRI;
	pp->revents = 0;

#ifdef DEBUG_PIPE
	Log("AddInput(%d,0x%x) Adds entry %d",fd,service_function,npollfd);
#endif
	npollfd++;
}

void
RemoveInput(int fd)
{
	int i;

	for (i=0; i<npollfd; i++) {
		if (pollfds[i].fd == fd) {
			memcpy( (char *)&pollfds[i], (char *)&pollfds[i+1],
					(npollfd-i) * sizeof(struct pollfd) );
			memcpy( (char *)&poll_handlers[i], (char *)&poll_handlers[i+1],
					(npollfd-i) * sizeof(void (*)()));
			npollfd--;
			return;
		}
	}
}

static int logfd = -1;
static char logbuf[256];
static int loggingLevel = 0;
static char *LogFile;
static char *PidFile = _PATH_DLPIDPID;

Log(fmt,a,b,c,d,e,f,g,h)
char *fmt;
{
	if ( loggingLevel == 0 )
		return;

	if ( logfd == -1 ) {
		logfd = open (LogFile, O_WRONLY|O_CREAT|O_TRUNC|O_SYNC, 0666);
	}
	sprintf(logbuf,fmt,a,b,c,d,e,f,g,h);
	if ( logbuf[strlen(logbuf)-1] != '\n'  ) {
		strcat(logbuf,"\n");
	}
	write(logfd, logbuf, strlen(logbuf));
}

main(int argc, char **argv)
{
	int c;
	int	do_fork = 1;
	int numfds;
	extern char *optarg;
	extern int optind;
	FILE *fp;

#ifdef _REENTRANT
	if (mutex_init(&mutex, USYNC_THREAD, NULL) != 0) {
		perror("mutex_init failed");
		exit(1);
	}
#endif

	/* we don't check for EINTR in any of dlpid code so we must rely on
	 * SA_RESTART functionality (which is part of the reason why we 
	 * aren't using signal any more...
	 */
	alrmhandler.sa_handler = restartHandler;
	sigfillset(&alrmhandler.sa_mask);
	alrmhandler.sa_flags = SA_RESTART;

	alrmnohandler.sa_handler = ignoreHandler;
	sigemptyset(&alrmnohandler.sa_mask);
	alrmnohandler.sa_flags = SA_RESTART;

	intnohandler.sa_handler = ignoreHandler;
	sigemptyset(&intnohandler.sa_mask);
	intnohandler.sa_flags = SA_RESTART;

	hupnohandler.sa_handler = ignoreHandler;
	sigemptyset(&hupnohandler.sa_mask);
	hupnohandler.sa_flags = SA_RESTART;

	while ( (c = getopt(argc, argv, "dl:i:")) != -1 ) {
		switch (c) {
		case 'd':
			do_fork = 0;
			break;
		case 'l':
			loggingLevel = 1;
			LogFile = optarg;
			/* optarg is global and can be changed with next call to getopt,
			 * so call Log() now while it's still set correctly.
			 */
			Log("Starting logfile");
			break;
		case 'i':
			netisl = 1;
			strncpy(NetIslInterface, optarg, NUMISLCHARS);
			break;
		}
	}

	if (do_fork) {   /* become a daemon */
		switch (fork()) {
		case -1:
			SystemError(3, "dlpid: Unable to fork()\n");
			_exit(1);
			/* NOTREACHED */
			break;
		case 0:		/* Child */
			/* become process group & session group leader.  since a controlling
			 * terminal is associated with a session, and this new session has
			 * not yet acquired a controlling terminal our process now has no
			 * controlling terminal, which is a Good Thing(tm) for daemons.
		 	 */
			(void) setsid(); 
			/* now fork again so the parent (the session group leader) can exit
			 * means that the new child, as a non-session group leader, can never
			 * regain a controlling terminal.
			 */
			switch(fork()) {
				case -1:
					SystemError(3, "dlpid: unable to fork()\n");
					_exit(1);  /* use _exit so we don't flush stdio twice */
					/* NOTREACHED */
					break;
				case 0:  /* grandchild */
					chdir("/");
					/* we haven't created our pid file yet */
					umask(022);/* to get control over perms of anything we create */
					fclose(stdin);
					fclose(stdout);
					/* if ((numfds = (int)sysconf(_SC_OPEN_MAX)) == -1) numfds = 60;
					 * don't fclose(stderr) as any call to Error should go to
					 * console.  don't close logfd as Logging might be enabled
					 * Unfortunately, catfprintf routines use fds on their own
					 * and we shouldn't mess with them here.
					 * for (c=3; c<numfds; c++) {
					 * 		extern int __catfd;   from message catalog
					 * 		if ((c == fileno(stderr)) || 
					 * 			(c == logfd) ||
					 * 			(c == __catfd)) continue;
					 * 		close(c);
					 * 	}
					 */
					break;
				default:   /* child */ 
					_exit(0);  /* use _exit so we don't flush stdio twice */
					/* NOTREACHED */
					break;
			}
			break;
		default:	/* Parent */
			_exit(0);  /* use _exit so we don't flush stdio twice */
			/* NOTREACHED */
			break;
		}
	}

	sigaction(SIGHUP, &hupnohandler, NULL); /* signal(SIGHUP, SIG_IGN); */
	sigaction(SIGINT, &intnohandler, NULL); /* signal(SIGINT, SIG_IGN); */

	if (netisl == 0) {
		OpenPipes(argv[optind]);
	} else {
		if (AddInterface("net0", NetIslInterface) == (struct dlpidev *)NULL) {
			Log("main: AddInterface fails - exit 1");
			exit(1);
		}
		DumpIfStructs();
		if (StartInterface("net0") == 0) {
			Log("main: StartInterface fails - exit 1");
			exit(1);
		}
	}
	atexit(RemovePidFile);

	/* tuck process id away */
	fp = fopen(PidFile, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	ServiceFds();
	exit(0);
}

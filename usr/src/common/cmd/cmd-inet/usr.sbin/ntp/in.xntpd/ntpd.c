#ident "@(#)ntpd.c	1.3"

/*
 * ntpd.c - main program for the fixed point NTP daemon
 */
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#ifndef SYS_WINNT
#if !defined(VMS)	/*wjm*/
#include <sys/param.h>
#endif /* VMS */
#include <sys/signal.h>
#if !defined(VMS)	/*wjm*/
#include <sys/ioctl.h>
#endif /* VMS */
#include <sys/time.h>
#if !defined(VMS)	/*wjm*/
#include <sys/resource.h>
#endif /* VMS */
#else
#include <signal.h>
#include <process.h>
#endif /* SYS_WINNT */
#if defined(SYS_HPUX)
#include <sys/lock.h>
#include <sys/rtprio.h>
#define HAVE_SETSID
#endif

#if defined(SYS_SVR4) || defined (SYS_UNIXWARE2)
#include <termios.h>
#endif

#if (defined(SYS_SOLARIS)&&!defined(bsd)) || defined(__svr4__)
#include <termios.h>
#endif

#ifdef SYS_DOMAINOS
#include <apollo/base.h>
#endif /* SYS_DOMAINOS */

#include "ntpd.h"
#include "ntp_select.h"
#include "ntp_io.h"
#include "ntp_stdlib.h"

#ifdef LOCK_PROCESS
#ifdef SYS_SOLARIS
#include <sys/mman.h>
#else
#include <sys/lock.h>
#endif
#endif

/*
 * Signals we catch for debugging.  If not debugging we ignore them.
 */
#define	MOREDEBUGSIG	SIGUSR1
#define	LESSDEBUGSIG	SIGUSR2

/*
 * Signals which terminate us gracefully.
 */
#ifndef SYS_WINNT
#define	SIGDIE1		SIGHUP
#endif /* SYS_WINNT */
#define	SIGDIE2		SIGINT
#ifndef SYS_WINNT
#define	SIGDIE3		SIGQUIT
#endif /* SYS_WINNT */
#define	SIGDIE4		SIGTERM

#ifdef SYS_WINNT
	/* handles for various threads, process, and objects */
	extern HANDLE hServDoneEvent;
	HANDLE 	process_handle = NULL, WorkerThreadHandle = NULL,
			ResolverThreadHandle = NULL, TimerThreadHandle = NULL,
			hMutex = NULL;
	/* variables used to inform the Service Control Manager of our current state */
	SERVICE_STATUS ssStatus;
	SERVICE_STATUS_HANDLE   sshStatusHandle;
	int was_stopped = 0;
#endif /* SYS_WINNT */

/*
 * Scheduling priority we run at
 */
#define	NTPD_PRIO	(-12)

/*
 * Debugging flag
 */
int debug;

/*
 * -x and -g flags
*/
extern int allow_set_backward;
int correct_any;
/*
 * Initializing flag.  All async routines watch this and only do their
 * thing when it is clear.
 */
int initializing;

/*
 * Version declaration
 */
#if !defined(SYS_WINNT) || defined(EXTERNAL_VERSION)
extern char *Version;
#else
char *Version = "version=<UNDEFINED> (NMAKE build fix needed)";
#endif /* SYS_WINNT */

/*
 * Alarm flag.  Imported from timer module
 */
extern int alarm_flag;

int was_alarmed;

#if !defined(SYS_386BSD) && !defined(SYS_BSDI) && !defined(SYS_44BSD)
/*
 * We put this here, since the argument profile is syscall-specific
 */
extern int syscall	P((int, struct timeval *, struct timeval *));
#endif /* !SYS_386BSD */

#ifdef	SIGDIE2
static	RETSIGTYPE	finish		P((int));
#endif	/* SIGDIE2 */

#ifdef	DEBUG
static	RETSIGTYPE	moredebug	P((int));
static	RETSIGTYPE	lessdebug	P((int));
#endif	/* DEBUG */

/*
 * Main program.  Initialize us, disconnect us from the tty if necessary,
 * and loop waiting for I/O and/or timer expiries.
 */
#if !defined(VMS)
void
#endif /* VMS */
main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp;
	struct recvbuf *rbuflist;
	struct recvbuf *rbuf;

	initializing = 1;	/* mark that we are initializing */
	debug = 0;		/* no debugging by default */

	getstartup(argc, argv);	/* startup configuration, may set debug */

#if !defined(VMS)
#ifndef NODETACH
	/*
	 * Detach us from the terminal.  May need an #ifndef GIZMO.
	 */
#ifdef	DEBUG
	if (!debug) {
#endif /* DEBUG */
#ifndef SYS_WINNT
#undef BSD19906
#if defined(BSD)&&!defined(sun)&&!defined(SYS_SINIXM)&&!defined(SYS_MIPS)
#if defined(BSD4_4) || (BSD >= 199006 && !defined(i386)) || defined(SYS_BSDI)
#define  BSD19906
#endif /* BSD... */
#endif /* BSD sun */
#if defined(BSD19906)
		daemon(0, 0);
#else /* BSD19906 */
		if (fork())
			exit(0);

		{
                        u_long s;
			int max_fd;
#if defined(NTP_POSIX_SOURCE) && !defined(SYS_386BSD)
    			max_fd = sysconf(_SC_OPEN_MAX);
#else /* NTP_POSIX_SOURCE */
			max_fd = getdtablesize();
#endif /* NTP_POSIX_SOURCE */
			for (s = 0; s < max_fd; s++)
				(void) close(s);
			(void) open("/", 0);
			(void) dup2(0, 1);
			(void) dup2(0, 2);
#ifdef SYS_DOMAINOS
			{
				uid_$t puid;
				status_$t st;

				proc2_$who_am_i(&puid);
				proc2_$make_server(&puid, &st);
			}
#endif /* SYS_DOMAINOS */
#if defined(NTP_POSIX_SOURCE) || defined(HAVE_SETSID)
#ifndef SYS_ULTRIX
			(void) setsid();
#else
			(void) setpgid(0, 0);
#endif
#else /* NTP_POSIX_SOURCE || HAVE_SETSID */
			{
				int fid;

				fid = open("/dev/tty", 2);
				if (fid >= 0) {
					(void) ioctl(fid, (u_long) TIOCNOTTY,
						(char *) 0);
					(void) close(fid);
				}
#ifdef HAVE_ATT_SETPGRP
				(void) setpgrp();
#else /* HAVE_ATT_SETPGRP */
				(void) setpgrp(0, getpid());
#endif /* HAVE_ATT_SETPGRP */
			}
#endif /* NTP_POSIX_SOURCE || HAVE_SETSID */
		}
#endif /* BSD19906 */
#else /* SYS_WINNT */

    {
	SERVICE_TABLE_ENTRY dispatchTable[] = {
        	{ TEXT("NetworkTimeProtocol"), (LPSERVICE_MAIN_FUNCTION)service_main },
        	{ NULL, NULL }
    	};

	/* daemonize */
    	if (!StartServiceCtrlDispatcher(dispatchTable)) {
		if (!was_stopped) {
			syslog(LOG_ERR, "StartServiceCtrlDispatcher: %m");
			ExitProcess(2);
		} else {
			NLOG(NLOG_SYSINFO) /* conditional if clause for conditional syslog */
			  syslog(LOG_INFO, "StartServiceCtrlDispatcher: service stopped");
			ExitProcess(0);
		}
    	}
    }
#endif /* SYS_WINNT */
#ifdef	DEBUG
	}
#endif /* DEBUG */
#endif /* NODETACH */
#if defined(SYS_WINNT) && !defined(NODETACH)
#if defined(DEBUG)
	else
	   service_main(argc, argv);
#endif
} /* end main */

/*
 * If this runs as a service under NT, the main thread will block at StartServiceCtrlDispatcher()
 * and another thread will be started by the Service Control Dispatcher which will begin execution 
 * at the routine specified in that call (viz. service_main) 
 */
void
service_main(argc, argv)
	DWORD argc;
	LPTSTR *argv;
{
    	char *cp;
	DWORD dwWait;

	if(!debug) {
    	/* register our service control handler */
		if (!(sshStatusHandle = RegisterServiceCtrlHandler( TEXT("NetworkTimeProtocol"),
                        			(LPHANDLER_FUNCTION)service_ctrl))) {
			syslog(LOG_ERR, "RegisterServiceCtrlHandler failed: %m");
			return;
		}

		/* report pending status to Service Control Manager */
		ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		ssStatus.dwCurrentState = SERVICE_START_PENDING;
		ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		ssStatus.dwWin32ExitCode = NO_ERROR;
    		ssStatus.dwServiceSpecificExitCode = 0;
		ssStatus.dwCheckPoint = 1;
		ssStatus.dwWaitHint = 5000;
		if (!SetServiceStatus(sshStatusHandle, &ssStatus)) {
			syslog(LOG_ERR, "SetServiceStatus: %m");
        		ssStatus.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(sshStatusHandle, &ssStatus);
			return;
    	}

		/* create an event object that the control handler function
	 	* will signal when it receives the "stop" control code */
		if (!(hServDoneEvent = CreateEvent(
        				NULL,    /* no security attributes */
        				TRUE,    /* manual reset event */
        				FALSE,   /* not-signalled */
        				NULL))){ /* no name */
				syslog(LOG_ERR, "CreateEvent failed: %m");
        			ssStatus.dwCurrentState = SERVICE_STOPPED;
				SetServiceStatus(sshStatusHandle, &ssStatus);
				return;
    		}
	}  /* debug */
#endif /* defined(SYS_WINNT) && !defined(NODETACH) && defined(DEBUG) && !debug */
#endif /* VMS */

	/*
	 * Logging.  This may actually work on the gizmo board.  Find a name
	 * to log with by using the basename of argv[0]
	 */
	cp = strrchr(argv[0], '/');
	if (cp == 0)
		cp = argv[0];
	else
		cp++;

	debug = 0; /* will be immediately re-initialized 8-( */
	getstartup(argc, argv);	/* startup configuration, catch logfile this time */

#if !defined(SYS_WINNT) && !defined(VMS)

#ifndef	LOG_DAEMON
	openlog(cp, LOG_PID);
#else

#ifndef	LOG_NTP
#define	LOG_NTP	LOG_DAEMON
#endif
	openlog(cp, LOG_PID | LOG_NDELAY, LOG_NTP);
#ifdef	DEBUG
	if (debug)
		setlogmask(LOG_UPTO(LOG_DEBUG));
	else
#endif	/* DEBUG */
		setlogmask(LOG_UPTO(LOG_DEBUG)); /* @@@ was INFO */
#endif	/* LOG_DAEMON */

#endif  /* SYS_WINNT || VMS */

	NLOG(NLOG_SYSINFO) /* conditional if clause for conditional syslog */
	  syslog(LOG_NOTICE, "%s", Version);

#ifdef SYS_WINNT
	/* TODO: lock the process in memory using SetProcessWorkingSetSize() and VirtualLock() functions */
#endif /* SYS_WINNT */

#if defined(SYS_HPUX)
	/*
	 * Lock text into ram, set real time priority
	 */
	if (plock(TXTLOCK) < 0)
	    syslog(LOG_ERR, "plock() error: %m");
	if (rtprio(0, 120) < 0)
	    syslog(LOG_ERR, "rtprio() error: %m");
#else
#if defined(LOCK_PROCESS)
#if defined(MCL_CURRENT) && defined(MCL_FUTURE)
	/*
	 * lock the process into memory
	 */
	if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0)
	    syslog(LOG_ERR, "mlockall(): %m");
#else
#if defined(PROCLOCK)
	/*
	 * lock the process into memory
	 */
	if (plock(PROCLOCK) < 0)
	    syslog(LOG_ERR, "plock(): %m");
#endif
#endif
#endif
#if defined(NTPD_PRIO) && NTPD_PRIO != 0
	/*
	 * Set the priority.
	 */
#ifdef	HAVE_ATT_NICE
	nice (NTPD_PRIO);
#endif /* HAVE_ATT_NICE */
#ifdef  HAVE_BSD_NICE
	(void) setpriority(PRIO_PROCESS, 0, NTPD_PRIO);
#endif /* HAVE_BSD_NICE */

#endif /* !PROCLOCK || !LOCK_PROCESS */
#endif /* SYS_HPUX */

#ifdef SYS_WINNT
	process_handle = GetCurrentProcess();
	if (!SetPriorityClass(process_handle, (DWORD) REALTIME_PRIORITY_CLASS)) {
			syslog(LOG_ERR, "SetPriorityClass: %m");
	}
#endif /* SYS_WINNT */

	/*
	 * Set up signals we pay attention to locally.
	 */
#ifdef SIGDIE1
	(void) signal_no_reset(SIGDIE1, finish);
#endif	/* SIGDIE1 */
#ifdef SIGDIE2
	(void) signal_no_reset(SIGDIE2, finish);
#endif	/* SIGDIE2 */
#ifdef SIGDIE3
	(void) signal_no_reset(SIGDIE3, finish);
#endif	/* SIGDIE3 */
#ifdef SIGDIE4
	(void) signal_no_reset(SIGDIE4, finish);
#endif	/* SIGDIE4 */

#if !defined(SYS_WINNT) && !defined(VMS)
#ifdef DEBUG
	(void) signal_no_reset(MOREDEBUGSIG, moredebug);
	(void) signal_no_reset(LESSDEBUGSIG, lessdebug);
#else
	(void) signal_no_reset(MOREDEBUGSIG, SIG_IGN);
	(void) signal_no_reset(LESSDEBUGSIG, SIG_IGN);
#endif 	/* DEBUG */
#endif /* SYS_WINNT || VMS */

	/*
	 * Set up signals we should never pay attention to.
	 */
#ifdef SIGPIPE
	(void) signal_no_reset(SIGPIPE, SIG_IGN);
#endif	/* SIGPIPE */

	/*
	 * Call the init_ routines to initialize the data structures.
	 * Note that init_systime() may run a protocol to get a crude
	 * estimate of the time as an NTP client when running on the
	 * gizmo board.  It is important that this be run before
	 * init_subs() since the latter uses the time of day to seed
	 * the random number generator.  That is not the only
	 * dependency between these, either, be real careful about
	 * reordering.
	 */
	init_auth();
	init_util();
	init_restrict();
	init_mon();
	init_systime();
	init_timer();
	init_lib();
	init_random();
	init_request();
	init_control();
	init_leap();
	init_peer();
#ifdef REFCLOCK
	init_refclock();
#endif
	init_proto();
	init_io();
	init_loopfilter();

	mon_start(MON_ON);      /* monitor on by default now      */
				/* turn off in config if unwanted */

	/*
	 * Get configuration.  This (including argument list parsing) is
	 * done in a separate module since this will definitely be different
	 * for the gizmo board.
	 */
	getconfig(argc, argv);
	initializing = 0;


#if defined(SYS_WINNT) && !defined(NODETACH)
#if defined(DEBUG)
	if(!debug) {
#endif

	/* 
	 * the service_main() thread will have to wait for start/stop/pause/continue requests
	 * from the services icon in the Control Panel or from any WIN32 application
         * start a new thread to perform all the work of the NTP service 
         */
        if (!(WorkerThreadHandle = (HANDLE)_beginthread(
                    worker_thread,
                    0,       /* stack size			*/
                    NULL))){    /* argument to thread	*/
			syslog(LOG_ERR, "_beginthread: %m");
			if (hServDoneEvent != NULL)
				CloseHandle(hServDoneEvent);
			if (ResolverThreadHandle != NULL)
				CloseHandle(ResolverThreadHandle);
        		ssStatus.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(sshStatusHandle, &ssStatus);
			return;
	}

    	/* report to the service control manager that the service is running */
		ssStatus.dwCurrentState = SERVICE_RUNNING;
		ssStatus.dwWin32ExitCode = NO_ERROR;
		if (!SetServiceStatus(sshStatusHandle, &ssStatus)) {
			syslog(LOG_ERR, "SetServiceStatus: %m");
			if (hServDoneEvent != NULL)
				CloseHandle(hServDoneEvent);
			if (ResolverThreadHandle != NULL)
				CloseHandle(ResolverThreadHandle);
        		ssStatus.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(sshStatusHandle, &ssStatus);
			return;
		}

    	/* wait indefinitely until hServDoneEvent is signaled */
    	dwWait = WaitForSingleObject(hServDoneEvent,INFINITE);
		if (hServDoneEvent != NULL)
			CloseHandle(hServDoneEvent);
		if (ResolverThreadHandle != NULL)
			CloseHandle(ResolverThreadHandle);
		if (WorkerThreadHandle != NULL)
			CloseHandle(WorkerThreadHandle);
		if (TimerThreadHandle != NULL)
			CloseHandle(TimerThreadHandle);
		/* restore the clock frequency back to its original value */
		if (!SetSystemTimeAdjustment((DWORD)0, TRUE))
			syslog(LOG_ERR, "Failed to reset clock frequency, SetSystemTimeAdjustment(): %m");
    	ssStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(sshStatusHandle, &ssStatus);
		return;
#if defined(DEBUG)
	} else 
	    worker_thread();
#endif
} /* end service_main() */


/*
 * worker_thread - perform all remaining functions after initialization and and becoming a service
 */
void
worker_thread(notUsed)
	void *notUsed;
{
 	struct recvbuf *rbuflist;
	struct recvbuf *rbuf;

#endif /* defined(SYS_WINNT) && !defined(NODETACH) && defined(DEBUG) && !debug */

	/*
	 * Report that we're up to any trappers
	 */
	report_event(EVNT_SYSRESTART, (struct peer *)0);

	/*
	 * Use select() on all on all input fd's for unlimited
	 * time.  select() will terminate on SIGALARM or on the
	 * reception of input.  Using select() means we can't do
	 * robust signal handling and we get a potential race
	 * between checking for alarms and doing the select().
	 * Mostly harmless, I think.
	 */
	/*
	 * Under NT, a timer periodically invokes a callback function
	 * on a different thread. This callback function has no way
	 * of interrupting a winsock "select" call on a different
	 * thread. A mutex is used to synchronize access to clock
	 * related variables between the two threads (one blocking
	 * on a select or processing the received packets and the
	 * other that calls the timer callback function, timer(),
	 * every second). Due to this change, timer() routine can
	 * be invoked between  processing two or more received
	 * packets, or even during processing a single received
	 * packet before entering the clock_update routine (if
	 * needed). The potential race condition is also avoided.
	 */
	/* On VMS, I suspect that select() can't be interrupted
	 * by a "signal" either, so I take the easy way out and 
	 * have select() time out after one second. 
	 * System clock updates really aren't time-critical, 
	 * and - lacking a hardware reference clock - I have 
	 * yet to learn about anything else that is.
	 */
	was_alarmed = 0;
	rbuflist = (struct recvbuf *)0;
	for (;;) {
#ifndef HAVE_SIGNALED_IO
		extern fd_set activefds;
		extern int maxactivefd;

		fd_set rdfdes;
		int nfound;
#else
		block_io_and_alarm();
#endif


		rbuflist = getrecvbufs();	/* get received buffers */
		if (alarm_flag) {		/* alarmed? */
			was_alarmed = 1;
			alarm_flag = 0;
		}

		if (!was_alarmed && rbuflist == (struct recvbuf *)0) {
			/*
			 * Nothing to do.  Wait for something.
			 */
#ifndef HAVE_SIGNALED_IO
			rdfdes = activefds;
#if defined(VMS)
			/* make select() wake up after one second */
			{
			    struct timeval t1;
			    t1.tv_sec = 1; t1.tv_usec = 0;
			    nfound = select(maxactivefd+1, &rdfdes, (fd_set *)0,
					    (fd_set *)0, &t1);
			}
#else
			nfound = select(maxactivefd+1, &rdfdes, (fd_set *)0,
					(fd_set *)0, (struct timeval *)0);
#endif /* VMS */
			if (nfound > 0) {
				l_fp ts;

        			get_systime(&ts);
        			(void)input_handler(&ts);
#ifndef SYS_WINNT
			} else if (nfound == -1 && errno != EINTR)
#else /* SYS_WINNT */
			} else if (nfound == SOCKET_ERROR)
#endif /* SYS_WINNT */
				syslog(LOG_ERR, "select() error: %m");
#else
			wait_for_signal();
#endif
			if (alarm_flag) {		/* alarmed? */
				was_alarmed = 1;
				alarm_flag = 0;
			}
			rbuflist = getrecvbufs();  /* get received buffers */
		}
#ifdef HAVE_SIGNALED_IO
		unblock_io_and_alarm();
#endif

		/*
		 * Out here, signals are unblocked.  Call timer routine
		 * to process expiry.
		 */
#ifndef SYS_WINNT
		/*
		 * under WinNT, the timer() routine is directly called
		 * by the timer callback function (alarming)
		 * was_alarmed should have never been set, but don't
		 * want to risk timer() being accidently called here
		 */
		if (was_alarmed) {
			timer();
			was_alarmed = 0;
		}
#endif /* SYS_WINNT */

		/*
		 * Call the data procedure to handle each received
		 * packet.
		 */
		while (rbuflist != (struct recvbuf *)0) {
			rbuf = rbuflist;
			rbuflist = rbuf->next;
			(rbuf->receiver)(rbuf);
			freerecvbuf(rbuf);
		}
		/*
		 * Go around again
		 */
	}
}


#ifdef SIGDIE2
/*
 * finish - exit gracefully
 */
static RETSIGTYPE
finish(sig)
int sig;
{

	/*
	 * Log any useful info before exiting.
	 */
#ifdef notdef
	log_exit_stats();
#endif
#ifdef SYS_WINNT
	/* with any exit(0)'s in the worker_thread, the service_main()
	 * thread needs to be informed to quit also
	 */
	SetEvent(hServDoneEvent);
#endif /* SYS_WINNT */
	exit(0);
}
#endif	/* SIGDIE2 */


#ifdef DEBUG
/*
 * moredebug - increase debugging verbosity
 */
static RETSIGTYPE
moredebug(sig)
int sig;
{
	if (debug < 255) {
		debug++;
		syslog(LOG_DEBUG, "debug raised to %d", debug);
	}
}

/*
 * lessdebug - decrease debugging verbosity
 */
static RETSIGTYPE
lessdebug(sig)
int sig;
{
	if (debug > 0) {
		debug--;
		syslog(LOG_DEBUG, "debug lowered to %d", debug);
	}
}
#endif	/* DEBUG */

#ifdef SYS_WINNT
/* service_ctrl - control handler for NTP service
 * signals the service_main routine of start/stop requests
 * from the control panel or other applications making
 * win32API calls
 */
void
service_ctrl(dwCtrlCode)
	DWORD dwCtrlCode;
{
    DWORD  dwState = SERVICE_RUNNING;

    /* Handle the requested control code */
    switch(dwCtrlCode) {

        case SERVICE_CONTROL_PAUSE:
		/* see no reason to support this */
		break;

        case SERVICE_CONTROL_CONTINUE:
 		/* see no reason to support this */
  		break;

        case SERVICE_CONTROL_STOP:
            	dwState = SERVICE_STOP_PENDING;
            	/* Report the status, specifying the checkpoint and waithint,
             	 *  before setting the termination event.
             	 */
			ssStatus.dwCurrentState = dwState;
			ssStatus.dwWin32ExitCode = NO_ERROR;
			ssStatus.dwWaitHint = 3000;
			if (!SetServiceStatus(sshStatusHandle, &ssStatus)) {
				syslog(LOG_ERR, "SetServiceStatus: %m");
			}
			was_stopped = 1;
            	SetEvent(hServDoneEvent);
            	return;

        case SERVICE_CONTROL_INTERROGATE:
            	/* Update the service status */
			break;

        default:
        	/* invalid control code */
            	break;

    }

	ssStatus.dwCurrentState = dwState;
	ssStatus.dwWin32ExitCode = NO_ERROR;
	if (!SetServiceStatus(sshStatusHandle, &ssStatus)) {
		syslog(LOG_ERR, "SetServiceStatus: %m");
	}
}
#endif /* SYS_WINNT */

#ident	"@(#)xdm:dm.c	1.22"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: dm.c,v 1.64 91/07/31 16:55:01 keith Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * display manager
 */

# include	"dm.h"

# include	<stdio.h>
# include	<sys/types.h>
# include	<time.h>
#if defined(X_NOT_POSIX) || defined(_POSIX_SOURCE)
# include	<signal.h>
#else
#define _POSIX_SOURCE
# include	<signal.h>
#undef _POSIX_SOURCE
#endif

#ifndef sigmask
#define sigmask(m)  (1 << ((m - 1)))
#endif

# include	<sys/stat.h>
# include	<errno.h>
# include	<X11/Xfuncproto.h>
#if NeedVarargsPrototypes
# include <stdarg.h>
# define Va_start(a,b) va_start(a,b)
#else
# include <varargs.h>
# define Va_start(a,b) va_start(a)
#endif

#ifndef F_TLOCK
#ifndef X_NOT_POSIX
# include	<unistd.h>
#endif
#endif

extern int	errno;
extern int	Ptty_fd;
extern int	Slaveptty_fd;

int	loginProblem = 0;	/* don't mess with this */

#ifdef SVR4
extern FILE    *fdopen();
#endif

extern int	AnyDisplaysLeft ();
extern int	makepttypair ();
extern void	ShellFail ();
extern void	NoHome ();

static int	serverTrys;
static time_t	maxTime;
static void	RescanServers ();
int		Rescan;
static long	ServersModTime, ConfigModTime, AccessFileModTime;
SIGVAL		StopAll ();
static SIGVAL	RescanNotify ();
void		StopDisplay ();
static void	RestartDisplay ();
void		Sys_Shutdown ();

#ifndef NOXDMTITLE
static char *Title;
static int TitleLen;
#endif

static char	*serverPidFile = "/dev/X/xdm-serverPid";
static char	*userPidFile = "/dev/X/xdm-userPid";
static int	userPidFd;

#ifndef UNRELIABLE_SIGNALS
static SIGVAL ChildNotify ();
#endif

main (argc, argv)
int	argc;
char	**argv;
{
    int	oldpid, oldumask;

    /* create master slave pair */
    if (makepttypair () < 0)
	{
	Debug ("Could not create pty for use in login");
	exit (1);
	}

    /* make sure at least world write access is disabled */
    if (((oldumask = umask(022)) & 002) == 002)
	(void) umask (oldumask);
#ifndef NOXDMTITLE
    Title = argv[0];
    TitleLen = (argv[argc - 1] + strlen(argv[argc - 1])) - Title;
#endif

    /*
     * Step 1 - load configuration parameters
     */
    InitResources (argc, argv);
    SetConfigFileTime ();
    LoadDMResources ();
    /*
     * Only allow root to run in non-debug mode to avoid problems
     */
    if (debugLevel == 0 && geteuid() != 0)
    {
	fprintf (stderr, "Only root is allowed to start graphical login: %s\n", argv[0]);
	exit (1);
    }
    if (debugLevel == 0 && daemonMode)
	BecomeOrphan ();
    /* SUPPRESS 560 */
    if (oldpid = StorePid ())
    {
	if (oldpid == -1)
	    LogError ("Can't create/lock pid file %s\n", pidFile);
	else
	    LogError ("Can't lock pid file %s, another xdm is running (pid %d)\n",
		 pidFile, oldpid);
	exit (1);
    }
    if (debugLevel == 0 && daemonMode)
/*
 *	WIPRO : Kumar K.V
 *	CHANGE # UNKNOWN
 *	FILE # dm.c
 * 
 *  The error log is initialised before becoming a daemon so that
 *  it is not necessary to dup the standard file descriptors to
 *  the root inode (as originally done in daemon.c)
 * 
 *	ENDCHANGE # UNKNOWN
 */
	/*
	BecomeDaemon ();
    InitErrorLog ();
	*/
    InitErrorLog ();
	BecomeDaemon ();
#ifdef XDMCP
    CreateWellKnownSockets ();
#else
    Debug ("xdm: not compiled for XDMCP\n");
#endif
    (void) Signal (SIGTERM, StopAll);
    (void) Signal (SIGINT, StopAll);
    /*
     * Step 2 - Read /etc/Xservers and set up
     *	    the socket.
     *
     *	    Keep a sub-daemon running
     *	    for each entry
     */
    SetAccessFileTime ();
#ifdef XDMCP
    ScanAccessDatabase ();
#endif
    ScanServers ();

    /*
     * initialize values for server start try errors
     */
    serverTrys = 0;
    StartDisplays ();
    (void) Signal (SIGHUP, RescanNotify);
#ifndef UNRELIABLE_SIGNALS
    (void) Signal (SIGCHLD, ChildNotify);
#endif

    while (
#ifdef XDMCP
	   AnyWellKnownSockets() ||
#endif
	   AnyDisplaysLeft ())
    {
	if (Rescan)
	{
	    RescanServers ();
	    Rescan = 0;
	}
#if defined(UNRELIABLE_SIGNALS) || !defined(XDMCP)
	WaitForChild ();
#else
	WaitForSomething ();
#endif
    }
    Debug ("Nothing left to do, exiting\n");
    exit(0);
    /*NOTREACHED*/
}

/* ARGSUSED */
static SIGVAL
RescanNotify (n)
    int n;
{
    Debug ("Caught SIGHUP\n");
    Rescan = 1;
#ifdef SIGNALS_RESET_WHEN_CAUGHT
    (void) Signal (SIGHUP, RescanNotify);
#endif
}

ScanServers ()
{
    char	lineBuf[10240];
    int		len;
    FILE	*serversFile;
    struct stat	statb;
    static DisplayType	acceptableTypes[] =
	    { { Local, Permanent, FromFile },
	      { Foreign, Permanent, FromFile },
	    };

#define NumTypes    (sizeof (acceptableTypes) / sizeof (acceptableTypes[0]))

    if (servers[0] == '/')
    {
	serversFile = fopen (servers, "r");
	if (serversFile == NULL)
 	{
	    LogError ("cannot access servers file %s\n", servers);
	    return;
	}
	if (ServersModTime == 0)
	{
	    fstat (fileno (serversFile), &statb);
	    ServersModTime = statb.st_mtime;
	}
	while (fgets (lineBuf, sizeof (lineBuf)-1, serversFile))
	{
	    len = strlen (lineBuf);
	    if (lineBuf[len-1] == '\n')
		lineBuf[len-1] = '\0';
	    ParseDisplay (lineBuf, acceptableTypes, NumTypes);
	}
	fclose (serversFile);
    }
    else
    {
	ParseDisplay (servers, acceptableTypes, NumTypes);
    }
}

static void
MarkDisplay (d)
struct display	*d;
{
    d->state = MissingEntry;
}

static void
RescanServers ()
{
    Debug ("rescanning servers\n");
    LogInfo ("Rescanning both config and servers files\n");
    ForEachDisplay (MarkDisplay);
    SetConfigFileTime ();
    ReinitResources ();
    LoadDMResources ();
    ScanServers ();
    SetAccessFileTime ();
#ifdef XDMCP
    ScanAccessDatabase ();
#endif
    StartDisplays ();
}

SetConfigFileTime ()
{
    struct stat	statb;

    if (stat (config, &statb) != -1)
	ConfigModTime = statb.st_mtime;
}

SetAccessFileTime ()
{
    struct stat	statb;

    if (stat (accessFile, &statb) != -1)
	AccessFileModTime = statb.st_mtime;
}

static
RescanIfMod ()
{
    struct stat	statb;

    if (config && stat (config, &statb) != -1)
    {
	if (statb.st_mtime != ConfigModTime)
	{
	    Debug ("Config file %s has changed, rereading\n", config);
	    LogInfo ("Rereading configuration file %s\n", config);
	    ConfigModTime = statb.st_mtime;
	    ReinitResources ();
	    LoadDMResources ();
	}
    }
    if (servers[0] == '/' && stat(servers, &statb) != -1)
    {
	if (statb.st_mtime != ServersModTime)
	{
	    Debug ("Servers file %s has changed, rescanning\n", servers);
	    LogInfo ("Rereading servers file %s\n", servers);
	    ServersModTime = statb.st_mtime;
	    ForEachDisplay (MarkDisplay);
	    ScanServers ();
	}
    }
#ifdef XDMCP
    if (accessFile && accessFile[0] && stat (accessFile, &statb) != -1)
    {
	if (statb.st_mtime != AccessFileModTime)
	{
	    Debug ("Access file %s has changed, rereading\n", accessFile);
	    LogInfo ("Rereading access file %s\n", accessFile);
	    AccessFileModTime = statb.st_mtime;
	    ScanAccessDatabase ();
	}
    }
#endif
}

/*
 * catch a SIGTERM, kill all displays and exit
 */

/* ARGSUSED */
SIGVAL
StopAll (n)
    int n;
{
    Debug ("StopAll: Shutting down entire manager\n");
#ifdef XDMCP
    Debug("calling DestroyWellKnownSockets\n");
    DestroyWellKnownSockets ();
#endif
    ForEachDisplay (StopDisplay);
#ifdef SIGNALS_RESET_WHEN_CAUGHT
    /* to avoid another one from killing us unceremoniously */
    Debug("Signals reset when caught\n");
    (void) Signal (SIGTERM, StopAll);
    (void) Signal (SIGINT, StopAll);
#endif
}

/*
 * notice that a child has died and may need another
 * sub-daemon started
 */

int	ChildReady;

#ifndef UNRELIABLE_SIGNALS
/* ARGSUSED */
static SIGVAL
ChildNotify (n)
    int n;
{
    ChildReady = 1;
}
#endif

WaitForChild ()
{
    int		pid;
    struct display	*d;
    waitType	status;
#ifndef X_NOT_POSIX
    sigset_t mask, omask;
#else
    int		omask;
#endif

#ifdef UNRELIABLE_SIGNALS
    /* XXX classic System V signal race condition here with RescanNotify */
    if ((pid = wait (&status)) != -1)
#else
#ifndef X_NOT_POSIX
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGHUP);
    sigprocmask(SIG_BLOCK, &mask, &omask);
#else
    omask = sigblock (sigmask (SIGCHLD) | sigmask (SIGHUP));
#endif
    Debug ("signals blocked, mask was 0x%x\n", omask);
    if (!ChildReady && !Rescan)
#ifndef X_NOT_POSIX
	sigsuspend(&omask);
#else
	sigpause (omask);
#endif
    ChildReady = 0;
#ifndef X_NOT_POSIX
    sigprocmask(SIG_SETMASK, &omask, (sigset_t *)NULL);
#else
    sigsetmask (omask);
#endif
#ifndef X_NOT_POSIX
    while ((pid = waitpid (-1, &status, WNOHANG)) > 0)
#else
    while ((pid = wait3 (&status, WNOHANG, (struct rusage *) 0)) > 0)
#endif
#endif
    {
	Debug ("Manager wait returns pid: %d sig %d core %d code %d\n",
	       pid, waitSig(status), waitCore(status), waitCode(status));

	/*
	 * clean up the utmp entry
	 */
	if (cleanut (pid, status))
		Debug ("Could not cleanup utmpx entry\n");

	/*
	 * Remove user login/lock file
	 */
	if (RemovePidFile ())
		Debug ("Could not remove user's pid file\n");

	if (autoRescan)
	    RescanIfMod ();
	/* SUPPRESS 560 */
	if (d = FindDisplayByPid (pid)) {
	   Debug("SUPPRESS 560 set d->pid to -1 status is %d\n",status);
	    d->pid = -1;
	    switch (waitVal (status)) {
	    case BADSHELL:
		ShellFail ();
		d->startTries = 0;
		Debug ("Display exited with BADSHELL\n");
		if (d->displayType.lifetime != Permanent ||
		    d->status == zombie)
		    StopDisplay (d);
		else
		    RestartDisplay (d, FALSE);
		break;
	    case NOHOME:
		NoHome ();
		d->startTries = 0;
		Debug ("Display exited with NOHOME\n");
		if (d->displayType.lifetime != Permanent ||
		    d->status == zombie)
		    StopDisplay (d);
		else
		    RestartDisplay (d, FALSE);
		break;
	    case SHUTDOWN_FLAG:
	      Debug ("Display Exited with SYSTEM_SHUTDOWN\n");
	      StopAll ();
	      if (cleanut (pid, status))
		Debug ("Could not cleanup utmpx entry\n");
	      exit (SHUTDOWN_FLAG);
	    case EXIT_XDM:
		Debug ("Display exited with EXIT_XDM\n");
		Debug("EXIT_XDM d->pid=%d d->status=%d\n",d->pid, d->status);
		sleep(1);
		StopDisplay (d);
		if (!(AnyDisplaysLeft ()))
			{
			Debug ("No Displays Left\n");
			StopAll ();
			close (Ptty_fd);
			close (Slaveptty_fd);
		    	if (cleanut (pid, status))
				Debug ("Could not cleanup utmpx entry\n");
			exit (0);
			}
		break;
	    case UNMANAGE_DISPLAY:
		Debug ("Display exited with UNMANAGE_DISPLAY\n");
		StopDisplay (d);
		break;
	    case OBEYSESS_DISPLAY:
		loginProblem = 0;
		d->startTries = 0;
		Debug ("Display exited with OBEYSESS_DISPLAY\n");
		if (d->displayType.lifetime != Permanent ||
		    d->status == zombie)
		    StopDisplay (d);
		else
		    RestartDisplay (d, FALSE);
		break;
	    default:
		Debug ("Display exited with unknown status %d\n", waitVal(status));
		LogError ("Unknown session exit code %d from process %d\n",
			  waitVal (status), pid);
		StopAll ();
		close (Ptty_fd);
		close (Slaveptty_fd);
		if (cleanut (pid, status))
			Debug ("Could not cleanup utmpx entry\n");
		exit (0);
	    case OPENFAILED_DISPLAY:
		Debug ("Display exited with OPENFAILED_DISPLAY, try %d of %d\n",
		       d->startTries, d->startAttempts);
		LogError ("Display %s cannot be opened\n", d->name);
		/*
 		 * no display connection was ever made, tell the
		 * terminal that the open attempt failed
 		 */
#ifdef XDMCP
		if (d->displayType.origin == FromXDMCP)
		    SendFailed (d, "Cannot open display");
#endif
		if (d->displayType.origin == FromXDMCP ||
		    d->status == zombie ||
		    ++d->startTries >= d->startAttempts)
		{
		    LogError ("Display %s is being disabled\n", d->name);
		    StopDisplay (d);
		}
		else
		{
		    RestartDisplay (d, TRUE);
		}
		break;
	    case RESERVER_DISPLAY:
		loginProblem = 0;
		d->startTries = 0;
		Debug ("Display exited with RESERVER_DISPLAY\n");
		if (d->displayType.origin == FromXDMCP || d->status == zombie)
		    StopDisplay(d);
		else
		    RestartDisplay (d, TRUE);
		break;
	    case waitCompose (SIGTERM,0,0):
		loginProblem = 0;
		d->startTries = 0;
		Debug ("Display exited on SIGTERM\n");
		if (d->displayType.origin == FromXDMCP || d->status == zombie)
		    StopDisplay(d);
		else
		    RestartDisplay (d, TRUE);
		break;
	    case REMANAGE_DISPLAY:
		loginProblem = 0;
		d->startTries = 0;
		Debug ("Display exited with REMANAGE_DISPLAY\n");
		/*
 		 * XDMCP will restart the session if the display
		 * requests it
		 */
		if (d->displayType.origin == FromXDMCP || d->status == zombie)
		    StopDisplay(d);
		else
		    RestartDisplay (d, FALSE);
		break;
	    }
	}
	/* SUPPRESS 560 */
	else if (d = FindDisplayByServerPid (pid))
	{
	Debug("setting d->serverPid=-1 d->status=%d\n",d->status);
	    d->serverPid = -1;
	    switch (d->status)
	    {
	    case zombie:
		Debug ("Zombie server reaped, removing display %s\n", d->name);
		RemoveDisplay (d);
		break;
	    case phoenix:
		Debug ("Phoenix server arises, restarting display %s\n", d->name);
		d->status = notRunning;
		break;
	    case running:
		Debug ("Server for display %s terminated unexpectedly, status %d\n", d->name, waitVal (status));
		LogError ("Server for display %s terminated unexpectedly: %d\n", d->name, waitVal (status));
		/*
		 * handle thrashing server here
		 */
		serverTrys++;
		if (serverTrys == d->trashThreshold) {
			if (time (NULL) < maxTime) {
				Debug ("Exiting due to thrashing server\n");
				LogError ("Exiting due to thrashing server\n");
				StopAll ();
				close (Ptty_fd);
				close (Slaveptty_fd);
			    	if (cleanut (pid, status))
					Debug ("Could not cleanup utmpx entry\n");
				exit (EXIT_XDM);
			} else {
				/* reinitialize values */
				serverTrys = 0;
				maxTime = time (NULL) + (time_t)d->thrashThreshold;
			}
		}

		if (d->pid != -1)
		{
		    Debug ("Terminating session pid %d\n", d->pid);
		    TerminateProcess (d->pid, SIGTERM);
		}		
		break;
	    case notRunning:
		Debug ("Server exited for notRunning session on display %s\n", d->name);
		break;
	    }
	}
	else
	{
	    Debug ("Unknown child termination, status %d\n", waitVal (status));
	}
    }
    StartDisplays ();
}

static void
CheckDisplayStatus (d)
struct display	*d;
{
    if (d->displayType.origin == FromFile)
    {
	switch (d->state) {
	case MissingEntry:
	    StopDisplay (d);
	    break;
	case NewEntry:
	    LoadServerResources (d);
	    maxTime = time (NULL) + (time_t) d->thrashThreshold;
	    d->state = OldEntry;
	    StartDisplay (d, 0);
	    break;
	case OldEntry:
	    if (d->status == notRunning)
		StartDisplay (d, 1);
	    break;
	}
    }
}

StartDisplays ()
{
    ForEachDisplay (CheckDisplayStatus);
}

StartDisplay (d, notFirst)
struct display	*d;
int		notFirst;
{
    int	pid;

    Debug ("StartDisplay %s\n", d->name);
    if (notFirst)
	LoadServerResources (d);
    if (d->displayType.location == Local)
    {
	/* don't bother pinging local displays; we'll
	 * certainly notice when they exit
	 */
	d->pingInterval = 0;
    	if (d->authorize)
    	{
	    Debug ("SetLocalAuthorization %s, auth %s\n",
		    d->name, d->authNames[0]);
	    SetLocalAuthorization (d);
	    /*
	     * reset the server after writing the authorization information
	     * to make it read the file (for compatibility with old
	     * servers which read auth file only on reset instead of
	     * at first connection)
	     */
	    if (d->serverPid != -1 && d->resetForAuth && d->resetSignal) {
		Debug("sending resetSignal=%d for serverPid=%d\n",d->resetSignal,d->serverPid);
		kill (d->serverPid, d->resetSignal);
		}
    	}
	if (d->serverPid == -1 && !StartServer (d))
	{
	    Debug("Server for display %s can't be started\n",d->name);
	    LogError ("Server for display %s can't be started, session disabled\n", d->name);
	    RemoveDisplay (d);
	    return;
	}
    }
    else
    {
	/* this will only happen when using XDMCP */
	if (d->authorizations)
	    SaveServerAuthorizations (d, d->authorizations, d->authNum);
    }
    switch (pid = fork ())
    {
    case 0:
	CleanUpChild ();
	(void) Signal (SIGPIPE, SIG_IGN);
	LoadSessionResources (d);
	SetAuthorization (d);
	if (!WaitForServer (d))
	    exit (OPENFAILED_DISPLAY);
#ifdef XDMCP
	if (d->useChooser)
	    RunChooser (d);
	else
#endif
	    ManageSession (d);
	exit (REMANAGE_DISPLAY);
    case -1:
	break;
    default:
	Debug ("Default Case setting d->status to running d->pid to: %d\n", pid);
	d->pid = pid;
	d->status = running;
	break;
    }
}

TerminateProcess (pid, signal)
{
int i;
Debug("TerminateProcess for pid=%d with signal=%d\n", pid, signal);
    i=kill (pid, signal);
Debug("TerminateProcess result of kill = %d %ld\n",i,errno);
#ifdef SIGCONT
    Debug("TerminateProcess with SIGCONT\n");
    i=kill (pid, SIGCONT);
    Debug("TerminateProcess result of kill SIGCONT = %d %ld\n",i,errno);
#endif
}

/*
 * transition from running to zombie or deleted
 */

void
StopDisplay (d)
    struct display	*d;
{
Debug("StopDisplay: d->serverPid=%d d->status=%d d->pid=%d\n", d->serverPid, d->status,d->pid);

    if (d->serverPid != -1)
	d->status = zombie; /* be careful about race conditions */
    if (d->pid != -1)
	TerminateProcess (d->pid, SIGTERM);
    if (d->serverPid != -1)
	TerminateProcess (d->serverPid, d->termSignal);
    RemoveDisplay (d);
}

/*
 * transition from running to phoenix or notRunning
 */

static void
RestartDisplay (d, forceReserver)
    struct display  *d;
    int		    forceReserver;
{
Debug("RestartDisplay d->serverPid=%d\n",d->serverPid);
    if (d->serverPid != -1 && (forceReserver || d->terminateServer))
    {
	Debug("Call TerminateProcess and set status to phoenix\n");
	TerminateProcess (d->serverPid, d->termSignal);
	d->status = phoenix;
    }
    else
    {
	Debug("set status to notRunning\n");
	d->status = notRunning;
    }
}

static FD_TYPE	CloseMask;
static int	max;

RegisterCloseOnFork (fd)
int	fd;
{
    FD_SET (fd, &CloseMask);
    if (fd > max)
	max = fd;
}

ClearCloseOnFork (fd)
int	fd;
{
    FD_CLR (fd, &CloseMask);
    if (fd == max) {
	while (--fd >= 0)
	    if (FD_ISSET (fd, &CloseMask))
		break;
	max = fd;
    }
}

CloseOnFork ()
{
    int	fd;

    for (fd = 0; fd <= max; fd++)
	if (FD_ISSET (fd, &CloseMask))
	    close (fd);
    FD_ZERO (&CloseMask);
    max = 0;
}

static int  pidFd;
static FILE *pidFilePtr;

StorePid ()
{
    int		oldpid;

    Debug("StorePid ");
    if (pidFile[0] != '\0') {
	pidFd = open (pidFile, 2);
	if (pidFd == -1 && errno == ENOENT)
	    pidFd = creat (pidFile, 0666);
	if (pidFd == -1 || !(pidFilePtr = fdopen (pidFd, "w")))
	{
	    LogError ("process-id file %s cannot be opened\n",
		      pidFile);
	    return -1;
	}
	if (fscanf (pidFilePtr, "%d\n", &oldpid) != 1)
	    oldpid = -1;
	fseek (pidFilePtr, 0l, 0);
	if (lockPidFile)
	{
#ifdef LOCK_EX
	    if (flock (pidFd, LOCK_EX|LOCK_NB) == -1)
	    {
		if (errno == EWOULDBLOCK)
		    return oldpid;
		else
		    return -1;
	    }
#else
	    if (lockf (pidFd, F_TLOCK, 0) == -1)
	    {
		if (errno == EACCES)
		    return oldpid;
		else
		    return -1;
	    }
#endif
	}
	fprintf (pidFilePtr, "%-5d\n", getpid ());
	(void) fflush (pidFilePtr);
	RegisterCloseOnFork (pidFd);
    }
    return 0;
}

int
StoreServerPid (pid)
int	pid;
{
    FILE	*spidFilePtr;

    Debug("StoreServerPid %d\n",pid);
    if (!(spidFilePtr = fopen (serverPidFile, "w"))) {
	Debug ("Could not open %s\n", serverPidFile);
	return 1;
    }
    fprintf (spidFilePtr, "%-5d\n", pid);
    Debug ("In StoreServerPid: pid = %d\n", pid);
    fclose (spidFilePtr);
    return 0;
}

int
StoreUserPid (pid)
int	pid;
{
    FILE	*spidFilePtr;

    Debug("StoreUserPid %d\n", pid);
    if (!(spidFilePtr = fopen (userPidFile, "w"))) {
	Debug ("Could not open %s\n", userPidFile);
	return 1;
    }
    fprintf (spidFilePtr, "%-5d\n", pid);
    Debug ("In StoreUserPid: pid = %d\n", pid);
    fclose (spidFilePtr);
    return 0;
}

int
RemovePidFile ()
{
	return (unlink (userPidFile));
}

UnlockPidFile ()
{
    if (lockPidFile)
#ifdef F_ULOCK
	lockf (pidFd, F_ULOCK, 0);
#else
	flock (pidFd, LOCK_UN);
#endif
    close (pidFd);
    fclose (pidFilePtr);
}

#if NeedVarargsPrototypes
SetTitle (char *name, ...)
#else
/*VARARGS*/
SetTitle (name, va_alist)
char *name;
va_dcl
#endif
{
#ifndef NOXDMTITLE
    char	*p = Title;
    int	left = TitleLen;
    char	*s;
    va_list	args;

    Va_start(args,name);
    *p++ = '-';
    --left;
    s = name;
    while (s)
    {
	while (*s && left > 0)
	{
	    *p++ = *s++;
	    left--;
	}
	s = va_arg (args, char *);
    }
    while (left > 0)
    {
	*p++ = ' ';
	--left;
    }
    va_end(args);
#endif	
}

void
Sys_Shutdown ()
{
	char	shutdown[512];
	int	status;

	strcpy (shutdown, DEFAULT_SHUTDOWN);
	Debug ("shutdown = %s\n", shutdown);
	status = system (shutdown);
	Debug ("Status = %d\n", status);
}

/*		copyright	"%c%" 	*/


#ident	"@(#)B2.c	1.2"
/***************************************************************************
 * Command: B2
 * Inheritable Privileges: P_SETUID, P_SYSOPS
 *       Fixed Privileges: -
 * Notes:  Print a job with ensured B2 level markings.
 *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/secsys.h>
#include <wait.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <priv.h>
#include <string.h>
#include <libgen.h>
#include <debug.h>

#define	BANNER_PATH	"/usr/lib/lp/model/B2.banntrail"
#define	JOB_PATH	"/usr/lib/lp/model/B2.job"

/*
**  Selected exit codes from 'lpsched.h'.
**  These are the things lpsched expects to receive.
*/
#define	EXIT_OKAY		0
#define	EXIT_SIGTERM		0200
#define	EXIT_GOT_SIGNAL		0202
#define	EXIT_PRINTER_FAULT	0201
#define	EXIT_NO_EXEC		0341
#define	EXIT_NO_FORK		0343

static	char *	PrinterNamep;
static	char *	RequestIdp;
static	char *	UserNamep;
static	char *	Titlep;
static	char *	Copiesp;
static	char *	OptionListp;
static	char **	FileListpp;
static	char **	Argv;

static	pid_t	ChildPid;
static	uid_t	lpUID;
static	uid_t	lpGID;
static	uid_t	UserUID;
static	uid_t	UserGID;

typedef	enum
{
	TYPE_BANNER,
	TYPE_JOB,
	TYPE_TRAILER
}
jobType;

struct	jobControl
{
	char *	envp;
	char *	execPathp;
	int	status;
};

static	struct jobControl JobControl [] =
{
	"B2_CONTROL=banner",	BANNER_PATH,	0,
	"B2_CONTROL=job",	JOB_PATH,	0,
	"B2_CONTROL=trailer",	BANNER_PATH,	0,
};

#ifdef	__STDC__
static	void	Usage (FILE *);
static	void	ParseOptions (int, char *[]);
static	void	PrintJob (jobType);
static	void	SignalTrap (int);
#else
static	void	Usage ();
static	void	ParseOptions ();
static	void	PrintJob ();
static	void	SignalTrap ();
#endif

typedef	struct
{

	int	signo;
	void	(*disp) ();
	void	(*odisp) ();
}
signal_ent;

static	signal_ent	SignalTable [] =
{
  SIGHUP,    SignalTrap, (void (*)()) 0, /*  Exit   Hangup               */
  SIGINT,    SignalTrap, (void (*)()) 0, /*  Exit   Interupt             */
  SIGQUIT,   SignalTrap, (void (*)()) 0, /*  Core   Quit                 */
  SIGILL,    SIG_DFL,    (void (*)()) 0, /*  Core   Illegal instruction  */
  SIGTRAP,   SIG_DFL,    (void (*)()) 0, /*  Core   Breakpoint trap      */
  SIGABRT,   SIG_DFL,    (void (*)()) 0, /*  Core   Abort                */
  SIGEMT,    SIG_DFL,    (void (*)()) 0, /*  Core   Emulation trap       */
  SIGFPE,    SIG_DFL,    (void (*)()) 0, /*  Core   Arithmetic exception */
  SIGBUS,    SIG_DFL,    (void (*)()) 0, /*  Core   Bus error            */
  SIGSEGV,   SIG_DFL,    (void (*)()) 0, /*  Core   Segmentation         */
  SIGSYS,    SIG_DFL,    (void (*)()) 0, /*  Core   Bad system call      */
  SIGPIPE,   SIG_DFL,    (void (*)()) 0, /*  Exit   Broken pipe          */
  SIGALRM,   SIG_DFL,    (void (*)()) 0, /*  Exit   Alarm clock          */
  SIGTERM,   SignalTrap, (void (*)()) 0, /*  Exit   Terminated           */
  SIGUSR1,   SIG_DFL,    (void (*)()) 0, /*  Exit   User signal 1        */
  SIGUSR2,   SIG_DFL,    (void (*)()) 0, /*  Exit   User signal 2        */
  SIGCHLD,   SIG_DFL,    (void (*)()) 0, /*  Ignore Child status changed */
  SIGPWR,    SIG_DFL,    (void (*)()) 0, /*  Ignore Power fail/restart   */
  SIGWINCH,  SIG_DFL,    (void (*)()) 0, /*  Ignore Window size change   */
  SIGURG,    SIG_DFL,    (void (*)()) 0, /*  Ignore Urgent socket cond.  */
  SIGPOLL,   SIG_DFL,    (void (*)()) 0, /*  Exit   Pollable event       */
  SIGTSTP,   SIG_DFL,    (void (*)()) 0, /*  Stop   Stopped user         */
  SIGCONT,   SIG_DFL,    (void (*)()) 0, /*  Ignore Continue             */
  SIGTTIN,   SIG_DFL,    (void (*)()) 0, /*  Stop   TTY input            */
  SIGTTOU,   SIG_DFL,    (void (*)()) 0, /*  Stop   TTY output           */
  SIGVTALRM, SIG_DFL,    (void (*)()) 0, /*  Exit   Virtual timer exp.   */
  SIGPROF,   SIG_DFL,    (void (*)()) 0, /*  Exit   Profiling timer exp. */
  SIGXCPU,   SIG_DFL,    (void (*)()) 0, /*  Core   CPU time exceeded    */
  SIGXFSZ,   SIG_DFL,    (void (*)()) 0, /*  Core   File size exceeded   */
};
/*
**
*/
static	void
#ifdef	_STDC__
Usage (FILE *filep)
#else
Usage (filep)

FILE	*filep;
#endif
{
	(void)	fprintf (filep, "%s\n", "Usage:");
	(void)  fprintf (filep, "\t%s", "printer-name");
	(void)  fprintf (filep,
		"request-id user-name title copies options files ...\n");
	return;
}
/*
**
*/
int
#ifdef	__STDC__
main (int argc, char **argv)
#else
main (argc, argv)

int	argc;
char	*argv[];
#endif
{
	DEFINE_FNNAME (<B2>main)
	int	i,
		exitCode,
		waitStatus;
	char *	sp = (char *) 0;
	struct
	passwd *passwdp;
	struct
	utsname	uts_name;

	OPEN_DEBUG_FILE ("/tmp/B2.debug")
	Argv = argv;
	/*
	**
	*/
	for (i=0; i < sizeof (SignalTable)/sizeof (signal_ent); i++)
	{
		if (SignalTable [i].disp != SIG_DFL)
		{
			SignalTable [i].odisp =
				signal (SignalTable [i].signo,
					SignalTable [i].disp);
		}
	}
	ParseOptions (argc, argv);
	/*
	**  Find out who lp is.
	**
	*/
	passwdp = getpwnam ("lp");

	if (! passwdp)
	{
		endpwent ();
		exit (EXIT_NO_EXEC);
	}
	lpUID = passwdp->pw_uid; TRACEd (lpUID);
	lpGID = passwdp->pw_gid; TRACEd (lpGID);
	/*
	**  Given:
	**  We are a setuid 'root' program.
	**
	**  If we are NOT in an ID based privilege mechanism
	**  then we want to run as 'lp' to avoid anyone logged
	**  in as 'root' from being able to 'kill' us.
	**  Logging in as 'lp' in the B2 env. is disallowed.
	**  Furthermore, we do not lose any privs. by changing
	**  our uid.
	**
	**  Setting uid to 'lp' in a ID based priv. mechanism will
	**  result in lost privs.  So, we set our effective uid to
	**  'lp' which results in lost WORKING privs. but we immediately
	**  turn these back on.
	*/
	if (secsys (ES_PRVID, 0) < 0)
	{
		if (setuid (lpUID) < 0)
		{
			exit (EXIT_NO_EXEC);
		}
		if (setgid (lpGID) < 0) 
		{
			exit (EXIT_NO_EXEC);
		}
	}
	else
	{
		if (setuid (0) < 0)
		{
			exit (EXIT_NO_EXEC);
		}
		if (seteuid (lpUID) < 0)
		{
			exit (EXIT_NO_EXEC);
		}
		if (setegid (lpGID) < 0) 
		{
			exit (EXIT_NO_EXEC);
		}
		(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t) 0);
	}
	if (sp = strchr (UserNamep, '!'))
	{
		(void)	uname (&uts_name);

		*sp = '\0';

		if (strcmp (UserNamep, uts_name.nodename) != 0)
		{
			UserUID = lpUID;
			UserGID = lpGID;
		}
		else
		{
			setpwent ();
			passwdp = getpwnam (sp+1);

			if (! passwdp)
			{
				endpwent ();
				exit (EXIT_NO_EXEC);
			}
			UserUID = passwdp->pw_uid;
			UserGID = passwdp->pw_gid;
		}
		*sp = '!';
	}
	else
	{
		setpwent ();
		passwdp = getpwnam (UserNamep);

		if (! passwdp)
		{
			endpwent ();
			exit (EXIT_NO_EXEC);
		}
		UserUID = passwdp->pw_uid;
		UserGID = passwdp->pw_gid;
	}
	TRACEd (UserUID)
	TRACEd (UserGID)
	endpwent ();
	/*
	**
	*/
	switch (ChildPid = fork ()) {
	case	-1:
		TRACEP ("Fork for banner failed.")
		exit (EXIT_NO_FORK);
		/*NOTREACHED*/

	case	0:	/*  Child  */
		PrintJob (TYPE_BANNER);
		/*NOTREACHED*/

	default:	/*  Parent */
		/*
		**  Assume the banner will print in case we receive
		**  a SIGTERM signal.  The B2.banntrail script ignores
		**  SIGTERM.  Any other signal will prevent the trailer
		**  from printing.
		*/
		JobControl [TYPE_BANNER].status++;
		if (wait (&waitStatus) < 0)
		{
			exit (EXIT_NO_EXEC);
		}
		if (WIFSIGNALED (waitStatus))
		{
			TRACEP ("Banner terminated due to signal.")
			exit (EXIT_GOT_SIGNAL);
		}
		else
		if (WEXITSTATUS (waitStatus))
		{
			/*
			**  Assume banner did not print
			**  and pass on the exit code.
			*/
			exit (WEXITSTATUS (waitStatus));
		}
	}
	/*
	**
	*/
	switch (ChildPid = fork ()) {
	case	-1:
		/*
		**  Break here and fall through to try to print
		**  trailer.  Not likely if fork() is failing.
		*/
		TRACEP ("Fork for job failed.")
		break;

	case	0:	/*  Child  */
		PrintJob (TYPE_JOB);
		/*NOTREACHED*/

	default:	/*  Parent */
		if (wait (&waitStatus) < 0)
		{
			exit (EXIT_NO_EXEC);
		}
		if (WIFSIGNALED (waitStatus))
		{
			exitCode = EXIT_GOT_SIGNAL;
		}
		else
		{
			exitCode = WEXITSTATUS (waitStatus);
		}
	}
	JobControl [TYPE_JOB].status++;
	/*
	**
	*/
trailer:
	switch (ChildPid = fork ()) {
	case	-1:
		TRACEP ("Fork for trailer failed.")
		exit (EXIT_NO_FORK);
		/*NOTREACHED*/

	case	0:	/*  Child  */
		PrintJob (TYPE_TRAILER);
		break;

	default:	/*  Parent */
		/*
		**  Assume the trailer prints in case we get a
		**  SIGTERM.  This will prevent two trailers from
		**  printing.
		*/
		JobControl [TYPE_TRAILER].status++;
		if (wait (&waitStatus) < 0)
		{
			exit (EXIT_NO_EXEC);
		}
		if (WIFSIGNALED (waitStatus))
		{
			exitCode = EXIT_GOT_SIGNAL;
		}
		else
		{
			exitCode = WEXITSTATUS (waitStatus);
		}
	}
	return	exitCode;
}
/*
**
*/
static	void
#ifdef	__STDC__
ParseOptions (int argc, char **argv)
#else
ParseOptions (argc, argv)

int	argc;
char	**argv;
#endif
{
	if (argc < 6)
	{
		Usage (stderr);
		exit (225);
	}
	PrinterNamep	= strdup (basename (argv [0]));
	RequestIdp	= argv [1];
	UserNamep	= argv [2];
	Titlep		= argv [3];
	Copiesp		= argv [4];
	OptionListp	= argv [5];
	FileListpp	= &argv [6];

	return;
}
/*
**
*/
static	void
#ifdef	__STDC__
PrintJob (jobType type)
#else
PrintJob (type)

jobType	type;
#endif
{
	DEFINE_FNNAME (PrintJob)
	ENTRYP

	(void)	putenv (JobControl [type].envp);

	if (type != TYPE_JOB)
		goto	exec;

	if (setgid (UserGID) < 0)
	{
		exit (EXIT_NO_EXEC);
	}
	if (setuid (UserUID) < 0)
	{
		exit (EXIT_NO_EXEC);
	}
	if (procprivl (CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0) < 0)
	{
		exit (EXIT_NO_EXEC);
	}
exec:
	TRACEP ("Before ``execv''.")
	if (execv (JobControl [type].execPathp, Argv) < 0)
	{
		TRACEP ("``execv'' failed.")
		exit (EXIT_NO_EXEC);
	}
	/*NOTREACHED*/
}

static void
#ifdef	__STDC__
SignalTrap (int signo)
#else
SignalTrap (signo)

int	signo;
#endif
{
	DEFINE_FNNAME (SignalTrap)

	int	waitStatus;

	static	int	SignalTrapFlag	= 0;

	if (SignalTrapFlag++)
		return;

	(void)	sigignore (SIGHUP);
	(void)	sigignore (SIGINT);
	(void)	sigignore (SIGQUIT);
	(void)	sigignore (SIGTERM);

	ENTRYP
	TRACEd (signo)
	TRACEd (JobControl [TYPE_BANNER].status)
	TRACEd (JobControl [TYPE_JOB].status)
	TRACEd (JobControl [TYPE_TRAILER].status)

	/*
	**  Wait for any child processes to complete.
	*/
	(void)	wait (&waitStatus);

	if (signo == SIGTERM &&
	    JobControl [TYPE_BANNER].status &&
	    !JobControl [TYPE_TRAILER].status)
	{
		(void)	putenv ("CANCEL_FLAG=1");

		switch (ChildPid = fork ()) {
		case	-1:
			TRACEP ("Fork for trailer failed.");
			exit (EXIT_NO_FORK);
			/*NOTREACHED*/
		case	0:
			PrintJob (TYPE_TRAILER);
			/*NOTREACHED*/
		default:
			/*
			**  Update 'JobControl [TYPE_TRAILER].status'
			**  now in case of another signal.
			*/
			JobControl [TYPE_TRAILER].status++;
			if (wait (&waitStatus) < 0)
			{
				exit (EXIT_NO_EXEC);
			}
		}
	}
	switch (signo) {
	case	SIGHUP:
	case	SIGINT:
	case	SIGQUIT:
		exit (EXIT_PRINTER_FAULT);
		/*NOTREACHED*/

	case	SIGTERM:
		exit (0);
		/*NOTREACHED*/

	default:
		exit (EXIT_GOT_SIGNAL);
		/*NOTREACHED*/
	}
	/*NOTREACHED*/
}

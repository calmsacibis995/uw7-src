/*		copyright	"%c%" 	*/

/*	Portions Copyright(c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)cron:common/cmd/cron/cron.c	1.17.7.16"

/***************************************************************************
 * Command: cron
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_SETPLEVEL,P_SETUID,
 *			   P_AUDIT,P_SYSOPS,P_DACREAD
 *       Fixed Privileges: None
 * Inheritable Authorizations: None
 *       Fixed Authorizations: None
 * Notes:
 *	Cron runs as a daemon process and executes commands
 *	at specified dates and times.  Regularly scheduled
 *	commands can be specified according to instructions
 *	found in crontab files under /var/spool/cron/crontabs.
 *	Users may submit their own crontab files via the
 *	crontab(1) command, if permitted.
 *	Commands that are to be executed only once are
 *	submitted via the at(1) or batch(1) commands and
 *	reside under /var/spool/cron/atjobs until their
 *	desired execution time arrives.
 *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <varargs.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <deflt.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <priv.h>
#include <mac.h>
#include <pfmt.h>
#include <string.h>
#include <sys/secsys.h>
#include "cron.h"
#include <ia.h>
#include <audit.h>
#include <ulimit.h>
#include <limits.h>

#define MAIL		"/usr/bin/mail"	/* mail program to use */
#define CONSOLE		"/dev/console"	/* where to write error messages when cron dies	*/

#define TMPINFILE	"/tmp/crinXXXXXX"  /* file to put stdin in for cmd */
#define	TMPDIR		"/tmp"		/* dir for stdout, stderr tempfile */
#define	PFX		"crout"		/* stdout, stderr tempfile prefix */

#define INMODE		00400		/* mode for stdin file	*/
#define OUTMODE		00600		/* mode for stdout file */
#define ISUID		06000		/* mode for verifying at jobs */

#define INFINITY	LONG_MAX	/* upper bound on time	*/
#define CUSHION		120L		/* to detect change of system clock */
#define	MAXRUN		25		/* max total jobs allowed in system */
#define ZOMB		100		/* proc slot used for mailing output */

/*
 * Queue parameters that can be specified
 * in the QUEDEFS file.
 */
#define	JOBF		'j'		/* job limit for this queue */
#define	NICEF		'n'		/* niceness value for execution */
#define	USERF		'u'		/* no longer used; will be ignored */
#define WAITF		'w'		/* wait time between exec attempts */

/*
 * First character of entries in log file.
 * This character identifies the type of log entry
 * (beginning or end of command execution).
 */
#define BCHAR		'>'		/* beginning of command execution */
#define	ECHAR		'<'		/* end of command execution */

/*
 * Action argument to quedefs().
 */
#define	DEFAULT		0		/* set queue parameters to defaults */
#define	LOAD		1		/* load parameters from QUEDEFS file */

/*
 * Action argument flags to crabort(), may be ORed together.
 */
#define	NO_ACTION	0x00		/* nothing special required */
#define	REMOVE_NPIPE	0x01		/* remove NPIPE */
#define	CONSOLE_MSG	0x02		/* write error message on console */

/*
 * Action argument to routines that optionally
 * return an errno or simply abort on failure.
 */
#define ABORT_ON_ERR	0		/* abort on usually-fatal errors */
#define RETURN_ERRNO	1		/* just return an errno */

/*
 * Messages, must match message database.
 */
static const char
	GETCRONDERR[] =	":602:Cannot get level for /etc/cron.d.\n",
	LPMERR[] =	":587:Process terminated to enforce least privilege\n",
	NOLVLPROC[] =	":588:lvlproc() failed\n",
	MLDMODERR[] =	":604:Cannot change multi-level directory mode.\n",
	NOREADMLD[] =	":607:Cannot read the \"%s\" multi-level directory.\n",
	NOREADEFF[] =	":608:Cannot read a \"%s\" effective directory.\n",
	BADCD[] =	":609:Cannot change directory to \"%s\" multi-level directory.\n",
	LCKERR[] =	":611:Cannot open lockfile.\n",
	SLCKERR[] =	":612:Cannot set lock on file.\n",
	ILCKERR[] =	":613:File descriptor for lockfile.\n",
	FLCKERR[] =	":614:flock or its data is invalid.\n",
	ULCKERR[] =	":615:Unknown error in attempt to lock file.\n",
	NOREADDIR[] =	":73:Canot read the crontab directory.",
	BADJOBOPEN[] =	":74:Unable to read your at job.\n",
	BADSHELL[] =	":9:Because your login shell is not /usr/bin/sh, you cannot use %s\n",
	BADSTAT[] =	":75:Cannot access your crontab file.  Resubmit it.\n",
	CANTCDHOME[] =	":76:Cannot change directory to your home directory.\nYour commands will not be executed.\n",
	CANTEXECSH[] =	":77:Unable to exec the shell for one of your commands.\n",
	EOLN[] =	":78:Unexpected end of line.\n",
	NOREAD[] =	":79:Cannot read your crontab file.  Resubmit it.\n",
	NOSTDIN[] =	":80:Unable to create a standard input file for one of your crontab commands.\nThat command was not executed\n.",
	OUTOFBOUND[] =	":81:Number too large or too small for field\n",
	UNEXPECT[] =	":84:Unexpected symbol found\n",
	ERRORWAS[] = 	":85:The error was \"%s\"\n",
	ERRMSG[] =	":37:%s: %s\n",
	CANTRUNJOB[] =	":96:Couldn't run your \"%s\" job\n\n",
	YOURJOB[] =	":104:Your \"%s\" job",
	BADLVLPROC[] =	":616:lvlproc() failed: %s\n",
	BADSECADVIS[] =	":684:secadvise() failed",
	RESCHEDAT[] =	":114:Rescheduling \"%s\" job",
	RESCHEDCRON[] =	":746:Rescheduling cron job: \"%s\"",
	LOGFERR[] =	":685:Unable to stat or open cron logfile.",
	OUTCRON[] =	":103:Output from \"cron\" command\n\n",
	OUTAT[] =	":105:Output from \"at\" job\n\n";

/*
 * The following messages are no longer used,
 * but are preserved here for historical interest.
 */

#if 0

	LTDBERR[] =	":603:lvlin() on SYS_ALL failed.\n",
	NOMLD[] =	":605:\"%s\" directory is not a multi-level directory.\n",
	MLDERR[] =	":606:\"%s\" multi-level directory is corrupt.\n",
	SETERR[] =	":610:Cannot set required privilege.\n",
	STDERRMSG[] =	":82:\n\n*************************************************\nCron: The previous message is the standard output\n      and standard error of one of your cron commands.\n",
	STDOUTERR[] =	":83:One of your commands generated output or errors, but cron was unable to mail you this output.\nRemember to redirect standard output and standard error for each of your commands.",

#endif

/*
 * Format parameter to mail(),
 * determines the type of mail message to be sent.
 */
#define	ERR_CRONTABENT	0	/* error in crontab file entry */
#define	ERR_UNIXERR	1	/* error in some system call */
#define	ERR_EXECCRON	2	/* error in setting up cron job environment*/
#define	ERR_EXECAT	3	/* error in setting up at job environment */
#define OUT_CRON	4	/* output from a cron job */
#define OUT_AT		5	/* output from an at job */

/*
 * cftime() format for log messages,
 * and the corresponding message database ID.
 */
static const char FORMAT[] = "%a %b %e %H:%M:%S %Y";
static const char FORMATID[] = ":22";
static char timebuf[80];	/* buffer for cftime() */

/*
 * If a UID-based privilege mechanism (such as SUM)
 * is in use, this will be set to the privileged userid;
 * otherwise it will be set to -1, indicating that a
 * file-based privilege mechanism (such as LPM) is in use.
 */
static uid_t privid;		/* privileged userid or -1 */

/*
 * Structure used to maintain lists of
 * cron and at jobs to be performed.
 */
struct event {	
	time_t	time;		/* time of the event */
	short	etype;		/* type of event, e.g. CRONEVENT */
	char	*cmd;		/* command for cron, job name for at */
	struct usr *u;		/* pointer to the owner (usr) of this event */
	struct event *link; 	/* pointer to another event for this user */
	union { 
		struct {		/* for crontab events */
			char *minute;	/*  (these	*/
			char *hour;	/*   fields	*/
			char *daymon;	/*   are	*/
			char *month;	/*   from	*/
			char *dayweek;	/*   crontab)	*/
			char *input;	/* ptr to stdin	*/
		} ct;
		struct {		/* for at events */
			int eventid;	/* for el_remove-ing at events */
		} at;
	} of; 
};

/*
 * Structure used to maintain list of users
 * that we know about.
 */
struct usr {	
	char	*name;		/* name of user (e.g. "root") */
	char	*home;		/* home directory for user */
	char	*shell;		/* shell used to execute user's jobs */
	uid_t	uid;		/* user id */
	gid_t	gid;		/* group id */
	level_t	lid;		/* MAC level identifier */
#ifdef ATLIMIT
	int	aruncnt;	/* counter for running at jobs per uid */
#endif
#ifdef CRONLIMIT
	int	cruncnt;	/* counter for running cron jobs per uid */
#endif
	int	ctid;		/* for el_remove-ing crontab events */
	struct event *ctevents;	/* list of this usr's crontab events */
	struct event *atevents;	/* list of this usr's at events */
	struct usr *nextusr; 	/* pointer to next user */
};

/*
 * Structure to maintain job queue info.
 */
struct queue {
	int	njob;		/* job limit */
	int	nice;		/* nice value for execution */
	int	nwait;		/* wait time to next execution attempt */
	int	nrun;		/* number running */
};

static struct queue qt[NQUEUE];	/* job queue info for each queue */

/*
 * Default values for queue defs.
 */
static const struct queue qd = {
	100,			/* job limit */
	2,			/* nice value for execution */
	60,			/* wait time to next execution attempt */
	0,			/* number running */
};

/*
 * Information on currently-running jobs.
 * We replicate from the usr structure any information we might need
 * to handle the completion of a running job, in case the corresponding
 * usr structure gets freed before the job completes.  This can happen
 * if we re-initialize due to the system clock being changed.
 */
static struct runinfo {
	pid_t	pid;		/* process id */
	short	que;		/* queue number */
	uid_t	uid;		/* user id, replicated from usr struct */
	level_t	lid;		/* MAC level, replicated from usr struct */
	char	*name;		/* name of user, replicated from usr struct */
	char 	*outfile;	/* file where stdout & stderr are trapped */
	short	jobtype;	/* what type of event, e.g. CRONEVENT */
	char	*jobname;	/* command for cron, jobname for at */
	int	mailwhendone;	/* 1 = send mail even if no output */
} rt[MAXRUN];

/*
 * These variables don't all have to be global,
 * but it is handy for debugging purposes.
 */
static int	mac_installed;	/* flag to see if MAC is installed */
static level_t	cron_lid;	/* cron's level identifier saved at startup */
static level_t	low_lid;	/* low lid == lid of /etc/cron.d */
static uid_t	cron_uid;	/* cron's effective uid saved at startup */
static int	msgfd;		/* file descriptor for receiving pipe end */
static int	notexpired;	/* time for next job has not come */
static int	running;	/* zero when no jobs are executing */
static struct event *next_event;/* the next event to execute */
static struct usr *uhead;	/* ptr to the list of users */
static struct usr *ulast;	/* previous usr list entry from find_usr() */
static struct flock lck;	/* file and record locking structure */
static int	lckfd;		/* file descriptor for lock file */
static time_t	init_time;	/* time at which we last did initialize() */
static int	wait_time = 60;	/* max alarm time while waiting for child */
static int	ecid = 1;	/* for giving event classes distinguishable id 
				   names for el_remove'ing them.
				   MUST be initialized to 1 */
/*
 * User's default environment for the shell.
 */
static char homedir[MAXPATHLEN + 6] =	"HOME=";
static char shellpath[MAXPATHLEN + 7] =	"SHELL=";
static char logname[UNAMESIZE + 9] =	"LOGNAME=";
static char tzone[100] =		"TZ=";
static char posix[100] =		"";
static char path[MAXPATHLEN + 6] = "PATH=";
static char *envinit[] = {
	homedir,
	logname,
	path,
	shellpath,
	tzone,
	posix,
	0
};
extern char **environ;

/* added for xenix */
#define DEFTZ	"EST5EDT"	/* default time zone */
static int log = 0;		/* set to 1 to enable logging */
/* end of xenix */

/*
 * Log file info.
 */
static FILE *logf;		/* file pointer to ACCTFILE from fopen() */
static char logf_backup[MAXPATHLEN];	/* pathname for old log file */
static int logf_size = 0;	/* max bytes in log file before aging it */
static int logf_lines = 0;	/* last # of lines to remain in log file */
				/* after aging handling is performed */

#define QUE(x)		('a'+(x))	/* queue number to queue name (a-z) */
#define JTYP(x)		((x)-'a')	/* queue name to queue # (job type) */
#define RCODE(x)	(((x)>>8)&0377)	/* child return code from wait(2) */
#define TSTAT(x)	((x)&0377)	/* termination status from wait(2) */
#define NOW()		time((time_t *)0)
#define MIN(x, y)	((x) < (y) ? (x) : (y))
#define MAX(x, y)	((x) < (y) ? (y) : (x))
#define DOTDIR(x)	(x[0]=='.' && (x[1]=='\0' || (x[1]=='.' && x[2]=='\0')))

static void	add_atevent();	/* add an at job to event lists */
static void	cleanup();	/* clean up after completion of an event */
static void	crabort();	/* termination routine */
static void	cronend();	/* handler for termination (SIGTERM) signal */
static void	defaults();	/* read defaults from the DEFFILE file */
static void	del_atjob();	/* delete an at job from event lists */
static void	del_ctab();	/* delete a user's crontab from event lists */
static void	dscan();	/* apply func to all files in a directory */
static int	ex();		/* execute an event */
static struct usr *find_usr();	/* find the usr structure for a given user */
static void	idle();		/* wait for something to happen */
static time_t	idletime();	/* returns time remaining until next event */
static void	initialize();	/* initialization routine */
static void	logit();	/* log beginning or end of event to log file */
static int	mail();		/* send mail message to a user */
static void	mod_atjob();	/* process a user's new at job */
static void	mod_ctab();	/* process a user's new or revised crontab */
static void	mod_usr();	/* update a usr struct with new passwd info */
static void	msg();		/* format and print message to log file */
static int	msg_wait();	/* await message from at, batch, crontab */
static struct usr *new_usr();	/* allocate and initialize a usr structure */
static char *	next_field();	/* get next field of line in crontab file */
static int	next_ge();	/* find next number in list >= current */
static time_t	next_time();	/* compute time of recurrence of cron event */
static FILE *	open_userfile();/* open a user's crontab or atjob file */
static void	parseqdef();	/* interpret a line from the QUEDEFS file */
static void	quedefs();	/* initialize job queue information */
static void	read_dirs();	/* read the crontabs and atjobs directories */
static void	read_mldir();	/* find all files in a multi-level directory */
static void	read_sldir();	/* find all files in an ordinary directory */
static void	readcron();	/* read crontab file and add events to lists */
static void	resched();	/* schedule an event to run at a later time */
static void	rm_ctevents();	/* remove a user's cron events from lists */
static int	rm_userfile();	/* unlink a user's jobfile or tempfile */
static int	setlevel();	/* set process level */
static void	timeout();	/* handler for alarm clock (SIGALRM) signals */

main(argc, argv)
	int	argc;
	char	**argv;
{
	time_t	t;
	time_t	t_old;
	time_t	last_time;
	time_t	ne_time;	/* amt of time until next event execution */
	time_t	lastmtime = 0L;
	int	tz_old;		/* to check if TZ changed since last time */
	int	tz_new;		/* ditto */
	struct	event *e, *eprev;
	struct	stat buf;
	pid_t	rfork;
	int	done;

	/*
	 * Clear working privileges.
	 */
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	/*
	 * Make process exempt from auditing.
	 */
	(void) procprivc(SETPRV, AUDIT_W, (priv_t)0);
	auditevt(ANAUDIT, NULL, sizeof(aevt_t));
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel("UX:cron");

	if (lvlproc(MAC_GET, &cron_lid) == -1) {
		if (errno == ENOPKG)
			mac_installed = 0;
		else
			crabort(NOLVLPROC, CONSOLE_MSG, "", "", "");
	} else {
		mac_installed = 1;
		/* start off with MLD virtual mode */
		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, CONSOLE_MSG, "", "", "");
		/* get level of /etc/cron.d directory */
		if (lvlfile(CROND, MAC_GET, &low_lid) == -1)
			crabort(GETCRONDERR, CONSOLE_MSG, "", "", "");
	}

	/*
	 * Determine whether we are running under a file-based privilege
	 * mechanism (such as LPM) or an id-based privilege mechanism
	 * (such as SUM).  The secsys call returns the privileged userid
	 * if we're running under an id-based mechanism, and -1 otherwise.
	 */
	privid = (uid_t)secsys(ES_PRVID, 0);

	/* remember cron's id */
	cron_uid = geteuid();

	/*
	 * Normally we fork a child to run in the background
	 * and the parent process just exits.  This may be
	 * overridden by supplying the command-line arg "nofork".
	 */
	if (argc <= 1 || strcmp(argv[1],"nofork")) {
		for (;;) {
			(void) procprivc(SETPRV, SYSOPS_W, (priv_t)0);
			rfork = fork();
			(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

			if (rfork == (pid_t)-1)
				sleep(30);	/* try again later */
			else if (rfork != 0)
				exit(0);	/* parent exits */
			else {
				setpgrp();	/* child detaches */
				break;
			}
		}
	}

	umask(022);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, cronend);
	signal(SIGALRM, timeout);

	defaults();
	initialize(1);

	/* 
	 * If opening log file for first time, chmod to 664
	 * so that a login besides 'root' is able to open
	 * this file if cron has to be manually restarted
	 * at some point in the future.
	 */
	if (stat(ACCTFILE,&buf) < 0) {
		if (errno != ENOENT)
			crabort(LOGFERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		/*
		 * File does not exist.  To create it, we need
		 * MAC write privilege because the directory is
		 * at SYS_PUBLIC and we're running at SYS_PRIVATE.
		 */
		(void) procprivc(SETPRV, MACWRITE_W, (priv_t)0);
		logf = fopen(ACCTFILE, "w");
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		if (logf == NULL) 
			crabort(LOGFERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		if (chmod(ACCTFILE, 0664) < 0)
			pfmt(stderr, MM_ERROR, ":686:chmod failed\n");
	} else {
		/* append to log file */
		if ((logf = fopen(ACCTFILE,"a")) == NULL)
			pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",ACCTFILE,
				strerror(errno));
	}

	quedefs(DEFAULT);	/* load default queue definitions */
	msg(":86:*** cron started ***   pid = %d", getpid());

	/*
	 * Record the current time and the daylight-savings-time
	 * state so that we can detect a change in the system clock,
	 * which would necessitate re-initializing.
	 */
	t_old = NOW();
	tz_old = localtime(&t_old)->tm_isdst;
	last_time = t_old;

	/*
	 * This is the main processing loop.
	 * Everything from here to the end of main()
	 * is contained within this loop.
	 *
	 * loop
	 *	check system clock and daylight-savings-time state
	 *	if either has changed
	 *		re-initialize
	 *	get the chronologically next event
	 *	if it is in the future
	 *		idle()
	 *		repeat loop
	 *	if QUEDEFS file has been modified
	 *		load new QUEDEFS info
	 *	if log file has grown too big
	 *		age the log file
	 *
	 *	execute the event
	 *
	 *	if it was a crontab event
	 *		add it back to event list for next occurrence
	 *	else it was an at or batch event
	 *		remove it since it is once-only
	 *	repeat loop
	 */

	for (;;) {
		t = NOW();
		tz_new = localtime(&t)->tm_isdst;

		if (t_old > t || t-last_time > CUSHION || tz_new != tz_old) {
			/*
			 * The time was set backward or forward, or the
			 * daylight-savings-time state has changed.
			 * Clear out and reload all our event information.
			 */
			initialize(0);
			t = NOW(); 
			tz_old = tz_new;
		}
		t_old = t;

		if (next_event == NULL && !el_empty())
			next_event = (struct event *)el_first();
		ne_time = idletime(t);	/* time until next_event->time */
#ifdef DEBUG
		if (next_event == NULL)
			fprintf(stderr, "next_time=infinity\n");
		else {
			cftime(timebuf, FORMAT, &next_event->time);
			fprintf(stderr, "next_time=%ld (%s), idle=%ld\n",
				next_event->time, timebuf, ne_time);
		}
#endif
		if (ne_time > 0) {
			idle(MAX(ne_time, 2));
			last_time = (notexpired? INFINITY : next_event->time);
			notexpired = 0;
			continue;
		}

		last_time = next_event->time;	/* save execution time */

		/*
		 * If the QUEDEFS file has changed since we last
		 * looked at it, go read it again.
		 */
		if (stat(QUEDEFS,&buf) < 0)
			msg(":87:Cannot stat QUEDEFS file");
		else if (lastmtime != buf.st_mtime) {
			quedefs(LOAD);
			lastmtime = buf.st_mtime;
		}

		/*
		 * In case log file has grown too big (>= logf_size bytes),
		 * keep only the last logf_lines lines and copy the rest
		 * of the file to an aged log file.
		 */
		if (stat(ACCTFILE, &buf) < 0 ||
			buf.st_size == 0 || buf.st_size < logf_size) 
			;
		else {
			fclose(logf);
			copylog(ACCTFILE, logf_backup, logf_lines);
			/* Reopen log file */
			if ((logf = fopen(ACCTFILE,"a")) == NULL)
				pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",ACCTFILE,
					strerror(errno));
		}

		/*
		 * Execute event at the correct level.
		 * Don't execute if not at the right level.
		 */
		if (setlevel(&next_event->u->lid, RETURN_ERRNO) == 0) {
			done = ex(next_event);
			setlevel(&cron_lid, ABORT_ON_ERR);
		} else
			done = 1;	/* actually, skipped */

		switch (next_event->etype) {
		case CRONEVENT:
			/*
			 * Add the cron event back into the main event list
			 * for its next scheduled occurrence.
			 * If the job could not be run and has been rescheduled
			 * for a later time via resched(), we do nothing here.
			 */
			if (done) {
				next_event->time = next_time(next_event);
				el_add((int *)next_event, next_event->time,
					next_event->u->ctid); 
			}
			break;
		default:
			/* remove at or batch job from system */
			for (eprev = NULL, e = next_event->u->atevents;
				e != NULL; eprev = e, e = e->link)
				if (e == next_event) {
					if (eprev == NULL)
						e->u->atevents = e->link;
					else
						eprev->link = e->link;
					FREE(e->cmd, M_ATCMD);
					FREE(e, M_ATEVENT);
					break;	
				}
			break;
		}
		next_event = NULL; 
	}
}

/*
 * Initialization routine, called at start time
 * (with firstpass TRUE) and whenever we detect
 * that the system clock has been changed
 * (with firstpass FALSE).
 */

static void
initialize(firstpass)
	int firstpass;
{
	int omask;
	int fd;
	int retval;
	int errval;
	static int fds[2];

#ifdef DEBUG
	fprintf(stderr,"in initialize\n");
#endif
	if (firstpass) {
		/*
		 * For mail(1), make sure messages appear to come from root.
		 * This works only for old mailers; newer ones ignore the
		 * LOGNAME.  For the latter, mail will appear to come from
		 * the user that owns the job.
		 */
		(void) putenv("LOGNAME=root");

		/* 
		 * Since we're creating a file in a directory
		 * whose level is not the same as this process's,
		 * we need MAC write privilege.
		 */
		omask = umask(0);
		(void) procprivc(SETPRV, MACWRITE_W, (priv_t)0);
		lckfd = creat(LCK_CRON, 0660); errval = errno;
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		(void) umask(omask);
		if (lckfd < 0) {
			pfmt(stderr, MM_ERROR, ERRMSG, LCK_CRON, strerror(errval));
			crabort(LCKERR, CONSOLE_MSG, "", "", "");
		}

		/* 
		 * Setup so that we lock the entire file.
		 */
		lck.l_type = F_WRLCK;   /* set write lock */
		lck.l_whence = 0;       /* from start of file */
		lck.l_start = 0L;
		lck.l_len = 0L;         /* till end of max file */

		/*
		 * Set lock on the file.
		 */
		if (fcntl(lckfd, F_SETLK, &lck) < 0) {
			switch (errno) {
			case EACCES:
			case EAGAIN:
				crabort(SLCKERR, CONSOLE_MSG, "", "", "");
				/* NOTREACHED */
			case EBADF:
				crabort(ILCKERR, CONSOLE_MSG, "", "", "");
				/* NOTREACHED */
			case EINVAL:
			case EFAULT:
				crabort(FLCKERR, CONSOLE_MSG, "", "", "");
				/* NOTREACHED */
			default:
				crabort(ULCKERR, CONSOLE_MSG, "", "", "");
				/* NOTREACHED */
			}
		}
	} else {
		/*
		 * Re-initializing because system clock was changed.
		 * Free all the old usr and event data structures.
		 */
		register struct usr	*u, *unext;
		register struct event	*e, *enext;

		el_delete();

		for (u = uhead; u != NULL; u = unext) {
			FREE(u->name, M_UNAME);
			FREE(u->home, M_HOME);
			FREE(u->shell, M_SHELL);
			rm_ctevents(u);

			for (e = u->atevents; e != NULL; e = enext) {
				FREE(e->cmd, M_ATCMD);
				enext = e->link;
				FREE(e, M_ATEVENT);
			}
			unext = u->nextusr;
			FREE(u, M_USR);
		}
		uhead = NULL;

		/*
		 * Close pipe; we're going to set it up again below.
		 */
		close(fds[0]);
		close(fds[1]);
		fdetach(NPIPE);
	}

	/*
	 * Set process level to the level of /etc/cron.d directory.
	 */
	setlevel(&low_lid, ABORT_ON_ERR);

	if (firstpass)
		if (access(NPIPE,F_OK) == -1) {
			/* create mount point at low level */

			omask = umask(0);
			if ((fd = creat(NPIPE, 0660)) < 0)
				crabort(":618:Cannot create %s",
					REMOVE_NPIPE|CONSOLE_MSG, NPIPE,
					strerror(errno), "");
			close(fd);
			(void) umask(omask);
		}

	/*
	 * Create pipe at low level,
	 * attach name to pipe, and
	 * push connld on mounted end.
	 */

	if (pipe(fds) < 0)
		crabort(":619:pipe() failed: %s\n",
			REMOVE_NPIPE|CONSOLE_MSG, strerror(errno), "", "");

	if (fattach(fds[1], NPIPE) != 0)
		crabort(":620:Cannot attach pipe: %s\n",
			REMOVE_NPIPE|CONSOLE_MSG, strerror(errno), "", "");

	/*
	 * For connld to run at multiple levels requires that we have
	 * MAC write privilege at the time it is pushed.
	 */
	(void) procprivc(SETPRV, MACWRITE_W, (priv_t)0);
	retval = ioctl(fds[1], I_PUSH, "connld"); errval = errno;
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
	if (retval != 0)
		crabort(":621:Cannot push connld: %s\n",
			REMOVE_NPIPE|CONSOLE_MSG, strerror(errval), "", "");

	/*
	 * Reset process level to original level.
	 */
	setlevel(&cron_lid, ABORT_ON_ERR);

	msgfd = fds[0];
	init_time = NOW();
	el_init(8, init_time, DAY, 10);

	/*
	 * Read directories, create user list,
	 * and add events to the main event list.
	 */
	read_dirs();
	next_event = NULL;
}

/*
 * read_dirs() reads the crontabs and atjobs directories for jobs to
 * include to its event list at initialization time.  If these directories
 * are multi-level directories, each effective directory is examined.
 */

static void
read_dirs()
{
	struct stat buf;

	if (mac_installed) {
		/* turn on real mode to test for MLDs */
		if (mldmode(MLD_REAL) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		(void) stat(CRONDIR, &buf);
		if (S_ISMLD & buf.st_flags)
			read_mldir(CRONDIR, "crontabs", mod_ctab);
		else
			read_sldir(CRONDIR, "crontabs", mod_ctab);

		(void) stat(ATDIR, &buf);
		if (S_ISMLD & buf.st_flags)
			read_mldir(ATDIR, "atjobs", mod_atjob);
		else
			read_sldir(ATDIR, "atjobs", mod_atjob);

		/* return to virtual mode */
		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	} else {
		/* this is the base system */
		read_sldir(CRONDIR, "crontabs", mod_ctab);
		read_sldir(ATDIR, "atjobs", mod_atjob);
	}
}

/*
 * read_mldir() is called in MLD real mode.
 * It performs the multi-level directory reading of a spool directory.
 */

static void
read_mldir(dname, cname, func)
	char	*dname;
	char	*cname;
	void	(*func)();
{
	DIR	*dir;
	DIR	*mldir;
	level_t lid;
	int	retval;
	register struct	dirent *dp;

	if (chdir(dname) == -1)
		crabort(BADCD, REMOVE_NPIPE|CONSOLE_MSG, cname, "", "");
	if ((mldir = opendir(".")) == NULL)
		crabort(NOREADMLD, REMOVE_NPIPE|CONSOLE_MSG, cname, "", "");

	while ((dp = readdir(mldir)) != NULL) {
		/* skip the dot entries */
		if (DOTDIR(dp->d_name))
			continue;

		/* skip entry if its LID cannot be retrieved */
		(void) procprivc(SETPRV, MACREAD_W, (priv_t)0);
		retval = lvlfile(dp->d_name, MAC_GET, &lid);
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		if (retval == -1) 
			continue;

		/* skip entry if its LID is not valid */
		if (setlevel(&lid, RETURN_ERRNO) != 0)
			continue;

		/* skip entry if it is not a directory or is inaccessible */
		if (chdir(dp->d_name) == -1)
			continue;

		if ((dir=opendir("."))==NULL)
			crabort(NOREADEFF, REMOVE_NPIPE|CONSOLE_MSG,
				cname, "", "");

		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		dscan(dir, func);

		if (mldmode(MLD_REAL) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		closedir(dir);

		if (chdir("..") == -1)
			crabort(BADCD, REMOVE_NPIPE|CONSOLE_MSG, cname, "", "");
	} /* end-while */

	/* reset cron's level */
	setlevel(&cron_lid, ABORT_ON_ERR);

	closedir(mldir);
}

/*
 * read_sldir() is called in MLD real mode, if MAC is installed.
 */

static void
read_sldir(dname, cname, func)
	char	*dname;
	char	*cname;
	void	(*func)();
{
	DIR	*dir;

	/* turn on MLD virtual mode */
	if (mac_installed)
		if (mldmode(MLD_VIRT) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	if (chdir(dname) == -1)
		crabort(BADCD, REMOVE_NPIPE|CONSOLE_MSG, cname, "", "");
	if ((dir = opendir("."))==NULL)
		crabort(NOREADDIR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	dscan(dir, func);
	closedir(dir);

	/* reset MLD real mode */
	if (mac_installed)
		if (mldmode(MLD_REAL) == -1)
			crabort(MLDMODERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
}

/*
 * For every entry in the specified directory,
 * call the specified function, passing it the
 * directory entry name and corresponding LID
 * as arguments.  If MAC is not installed, a
 * default LID value of zero is supplied.
 */

static void
dscan(df, func)
	DIR	*df;
	void	(*func)();
{
	register struct dirent *dp;
	level_t lid = 0;	/* defaults to 0 if MAC not installed */

	while ((dp = readdir(df)) != NULL) {
		/* skip the dot entries */
		if (DOTDIR(dp->d_name))
			continue;

		/* skip entries whose LID cannot be retrieved */
		if (mac_installed) {
			if (lvlfile(dp->d_name, MAC_GET, &lid) == -1)
				continue;
		}
		(*func) (dp->d_name, lid);
	}
}

/*
 * Process the named user's new or modified crontab file.
 * This may involve creating a usr structure for this user
 * if one did not already exist.
 */

static void
mod_ctab(name, lid)
	char	*name;		/* file name, same as user name */
	level_t	lid;		/* level id of file */
{
	struct	passwd	*pw;
	struct	stat	buf;
	struct	usr	*u;
	char	path[MAXPATHLEN];

	if ((pw = getpwnam(name)) == NULL)
		return;
	strcat(strcat(strcpy(path, CRONDIR), "/"), name);

	/*
	 * A warning message is given by the crontab command so there is
	 * no need to give one here......use this code if you only want users
	 * with a login shell of /usr/bin/sh to use cron.
	 */
#if 0
	if ((strcmp(pw->pw_shell,"")!=0) && (strcmp(pw->pw_shell,SHELL)!=0)){
			(void) mail(name, BADSHELL, ERR_EXECCRON, "cron");
			unlink(path);
			return;
	}
#endif
	if (stat(path, &buf) < 0) {
		(void) mail(name,BADSTAT,ERR_UNIXERR);
		unlink(path);
		return;
	}
	if ((u = find_usr(name, lid)) == NULL) {
#ifdef DEBUG
		fprintf(stderr,"new user (%s) with a crontab\n",name);
#endif
		u = new_usr(pw, lid);
		u->ctid = ecid++;
		readcron(u, path);
	} else {
		mod_usr(u, pw);
		if (u->ctid == 0) {
#ifdef DEBUG
			fprintf(stderr,"%s now has a crontab\n",u->name);
#endif
			/* user didn't have a crontab last time */
			u->ctid = ecid++;
			readcron(u, path);
		} else {
#ifdef DEBUG
			fprintf(stderr,"%s has revised his crontab\n",u->name);
#endif
			rm_ctevents(u);
			el_remove(u->ctid,0);
			readcron(u, path);
		}
	}
}

/*
 * Process a new at or batch jobfile.
 * This may involve creating a usr structure for this user
 * if one did not already exist.
 */

static void
mod_atjob(name, lid)
	char	*name;		/* file name */
	level_t	lid;		/* level id of file */
{
	char	*ptr;
	time_t	tim;
	int	jobtype;
	struct	passwd	*pw;
	struct	stat	buf;
	struct	usr	*u;
	struct	event	*e;
	char	path[MAXPATHLEN];

	/*
	 * Extract the job type and start time from the filename.
	 * If the filename is not of the appropriate form (e.g.
	 * "8139660.a.5"), ignore it.
	 */
	ptr = name;
	if ((tim = num(&ptr)) == 0 || *ptr++ != '.')
		return;
	if ((jobtype = JTYP(*ptr)) < 0 || jobtype >= NQUEUE)
		return;

	strcat(strcat(strcpy(path, ATDIR), "/"), name);

	/*
	 * If the file's setid bits are off, the file may have been
	 * chowned.  Executing it would risk a security violation.
	 */
	if (stat(path, &buf) < 0)
		return;
	if ((buf.st_mode & ISUID) == 0) {
		(void) rm_userfile(path, buf.st_uid, RETURN_ERRNO);
		return;
	}

	if ((pw = getpwuid(buf.st_uid)) == NULL)
		return;

	/*
	 * A warning message is given by the at command so there is no
	 * need to give one here......use this code if you only want users
	 * with a login shell of /usr/bin/sh to use at.
	 */
#if 0
	if ((strcmp(pw->pw_shell,"")!=0) && (strcmp(pw->pw_shell,SHELL)!=0)){
			(void) mail(pw->pw_name, BADSHELL, ERR_EXECAT, "at");
			unlink(path);
			return;
	}
#endif
	if ((u = find_usr(pw->pw_name, lid)) == NULL) {
#ifdef DEBUG
		fprintf(stderr,"new user (%s) with an at job = %s\n",pw->pw_name,name);
#endif
		u = new_usr(pw, lid);
		add_atevent(u, name, tim, jobtype);
	} else {
		mod_usr(u, pw);
		/*
		 * There's no interface for modifying an existing at job,
		 * but we want to be sure we don't somehow add one twice.
		 */
		for (e = u->atevents; e != NULL; e = e->link)
			if (strcmp(e->cmd,name) == 0)
				break;
		if (e == NULL) {
#ifdef DEBUG
			fprintf(stderr,"%s has a new at job = %s\n",u->name,name);
#endif
			add_atevent(u, name, tim, jobtype);
		}
	}
}

/*
 * Add an at job to the given user's event list
 * for execution at the specified time.
 */

static void
add_atevent(u, job, tim, jobtype)
	struct usr *u;
	char *job;
	time_t tim;
	int jobtype;
{
	struct event *e;

	e = (struct event *)MALLOC(sizeof(struct event), M_ATEVENT);
	e->etype = (short)jobtype;
	e->cmd = MALLOC(strlen(job)+1, M_ATCMD);
	strcpy(e->cmd,job);
	e->u = u;
#ifdef DEBUG
	fprintf(stderr,"add_atevent: user=%s, job=%s, time=%ld, type=%d\n",
		u->name, e->cmd, e->time, e->etype);
#endif
	e->link = u->atevents;
	u->atevents = e;
	e->of.at.eventid = ecid++;
	if (tim < init_time)		/* old job */
		e->time = init_time;
	else
		e->time = tim;
	el_add((int *)e, e->time, e->of.at.eventid); 
}

/*
 * readcron reads in a crontab file for a user (u).
 * The list of events for user u is built, and 
 * u->events is made to point to this list.
 * Each event is also entered into the main event list.
 */

static char line[CTLINESIZE];	/* holds a line from a crontab file */
static int cursor;		/* cursor for the above line */

#define TFADMIN	"TFADMIN=/sbin/tfadmin export TFADMIN; "

static void
readcron(u, path)
	struct usr *u;
	char *path;
{
	FILE *cf;		/* cf will be a user's crontab file */
	struct event *e;
	int start,i;

	if ((cf = open_userfile(path, u->uid)) == NULL) {
		(void) mail(u->name,NOREAD,ERR_UNIXERR);
		return; 
	}
	while (fgets(line,CTLINESIZE,cf) != NULL) {
		/* process a line of a crontab file */
		cursor = 0;
		while (line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if (line[cursor] == '#')
			continue;
		e = (struct event *) MALLOC(sizeof(struct event), M_CREVENT);
		e->etype = CRONEVENT;

		if ((e->of.ct.minute = next_field(0,59,u)) == NULL) goto badf1;
		if ((e->of.ct.hour = next_field(0,23,u)) == NULL) goto badf2;
		if ((e->of.ct.daymon = next_field(1,31,u)) == NULL) goto badf3;
		if ((e->of.ct.month = next_field(1,12,u)) == NULL) goto badf4;
		if ((e->of.ct.dayweek = next_field(0,6,u)) == NULL) goto badf5;

		if (line[++cursor] == '\0') {
			(void) mail(u->name,EOLN,ERR_CRONTABENT);
			goto badf6; 
		}
		/* get the command to execute	*/
		start = cursor;
again:
		while (line[cursor] != '%' &&  line[cursor] != '\n'
		    && line[cursor] != '\0' && line[cursor] != '\\')
			cursor++;
		if (line[cursor] == '\\') {
			cursor += 2;
			goto again;
		}

		/* 
		 * Check if file based privilege mechanism; if yes prefix
		 * the command to be executed with 
		 *	TFADMIN=/sbin/tfadmin export TFADMIN; 
		 * so that commands run from crontab will run via tfadmin
		 * if the crontab entry is prefixed with "$TFADMIN".
		 */
		if (privid < 0) {
			/* file based privilege mechanism */
		       e->cmd = MALLOC(cursor - start + sizeof(TFADMIN), M_CRCMD);
		       strcpy(e->cmd, TFADMIN);
		       strncat(e->cmd, line + start, cursor - start);
		       e->cmd[cursor - start + sizeof(TFADMIN) - 1] = '\0';
		} else {
			/* ID based privilege mechanism */
			e->cmd = MALLOC(cursor - start + 1, M_CRCMD);
			strncpy(e->cmd, line + start, cursor - start);
			e->cmd[cursor - start] = '\0';
		}

		/* see if there is any standard input */
		if (line[cursor] == '%') {
			i = strlen(line) - cursor;
			e->of.ct.input = MALLOC(i, M_IN);
			strcpy(e->of.ct.input,line+cursor+1);
			while (--i >= 0)
				if (e->of.ct.input[i] == '%')
					e->of.ct.input[i] = '\n'; 
		} else
			e->of.ct.input = NULL;

		/* have the event point to its owner */
		e->u = u;
		/* insert this event at the front of this user's event list */
		e->link = u->ctevents;
		u->ctevents = e;
		/* set the time for the first occurrence of this event */
		e->time = next_time(e);
		/* finally, add this event to the main event list */
		el_add((int *)e, e->time, u->ctid);
#ifdef DEBUG
		cftime(timebuf, FORMAT, &e->time);
		fprintf(stderr,"inserting cron event %s at %ld (%s)\n",
			e->cmd,e->time,timebuf);
#endif
		continue;

badf6:		FREE(e->of.ct.dayweek, M_CRFIELD);
badf5:		FREE(e->of.ct.month, M_CRFIELD);
badf4:		FREE(e->of.ct.daymon, M_CRFIELD);
badf3:		FREE(e->of.ct.hour, M_CRFIELD);
badf2:		FREE(e->of.ct.minute, M_CRFIELD);
badf1:		FREE(e, M_CREVENT); 
	}

	fclose(cf);
}


/*
 * Send a mail message to the specified user.
 * Returns the process-id of the child process
 * that was forked; -1 if the fork failed.
 */

static int
mail(usrname, message, format, a1, a2, a3)
	char	*usrname, *message, *a1, *a2, *a3;
	int	format;
{
	char	iobuf[BUFSIZ];
	FILE	*mailpipe;
	FILE	*st;
	int	nbytes;
	char	*temp, *i;
	struct	passwd *ruser_ids;
	pid_t	fork_val;
	int	saveerrno = errno;

#ifdef TESTING
	return(-1);
#endif
		
	(void) procprivc(SETPRV, SYSOPS_W, (priv_t)0);
	fork_val = fork();
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	if (fork_val == (pid_t)-1) {
		/* fork failed, forget it */
		return(-1);
	} else if (fork_val != 0) {
		/* parent, just note that we have a child running */
		running++;
		return(fork_val);
	}

	/* child */

	/*
	 * Get uid for real user and change to that uid.
	 * We do this so that mail won't always come from
	 * root, since that could be a security hole.
	 * If we can't get the real uid, quit -- don't
	 * send mail as root.
	 */
	if ((ruser_ids = getpwnam(usrname)) == NULL)
		exit(0);

	(void) procprivc(SETPRV, SETUID_W, (priv_t)0);
	setuid(ruser_ids->pw_uid);
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	temp = xmalloc(strlen(MAIL) + strlen(usrname) + 2);
	(void) strcpy(temp, MAIL);
	(void) strcat(temp, " ");
	(void) strcat(temp, usrname);

	(void) procprivc(SETPRV, SYSOPS_W, (priv_t)0);
	mailpipe = popen(temp, "w");
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	if (mailpipe == NULL)
		exit(127);

	fprintf(mailpipe,"To: %s\nSubject: ", usrname);

	switch (format) {
	case ERR_CRONTABENT:
		pfmt(mailpipe, MM_NOSTD,
			":94:Your crontab file has an error in it");
		fputs("\n\n", mailpipe);
		i = strrchr(line,'\n');
		if (i != NULL)
			*i = ' ';
		fprintf(mailpipe, "\t%s\n\t", line);
		pfmt(mailpipe, MM_NOSTD, message, a1, a2, a3);
		putc('\n', mailpipe);
		pfmt(mailpipe, MM_NOSTD, ":95:This entry has been ignored.\n"); 
		break;	

	case ERR_UNIXERR:
		pfmt(mailpipe, MM_NOSTD, message, a1, a2, a3);
		fputs("\n\n", mailpipe);
		pfmt(mailpipe, MM_NOSTD, ERRORWAS, strerror(saveerrno));
		break;

	case ERR_EXECCRON:
		pfmt(mailpipe, MM_NOSTD, CANTRUNJOB, "cron");
		pfmt(mailpipe, MM_NOSTD, message, a1, a2, a3);
		putc('\n', mailpipe);
		pfmt(mailpipe, MM_NOSTD, ERRORWAS, strerror(saveerrno));
		break;

	case ERR_EXECAT:
		pfmt(mailpipe, MM_NOSTD, CANTRUNJOB, "at");
		pfmt(mailpipe, MM_NOSTD, message, a1, a2, a3);
		putc('\n', mailpipe);
		pfmt(mailpipe, MM_NOSTD, ERRORWAS, strerror(saveerrno));
		break;

	case OUT_CRON:
	case OUT_AT:
		/* a1 is jobname, a2 is job output filename or NULL */
		pfmt(mailpipe, MM_NOSTD, message);
		if (format == OUT_CRON) {
			pfmt(mailpipe, MM_NOSTD, YOURJOB, "cron");
			fprintf(mailpipe, "\n\n%s\n\n", a1);
		} else {
			pfmt(mailpipe, MM_NOSTD, YOURJOB, "at");
			fprintf(mailpipe, " \"%s\"", a1);
		}
		fprintf(mailpipe, " ");
		if (a2 == NULL || (st = fopen(a2, "r")) == NULL)
			pfmt(mailpipe, MM_NOSTD, ":107:completed.\n");
		else {
			pfmt(mailpipe, MM_NOSTD,
				":106:produced the following output:\n\n");
			while ((nbytes = fread(iobuf, 1, BUFSIZ, st)) != 0)
				(void) fwrite(iobuf, 1, nbytes, mailpipe);
			fclose(st);
		}
		break;

	default:
		msg(":178:Program error %s\n", "(cron)");
		break;
	}

	(void) pclose(mailpipe); 
	exit(0);
}

/*
 * next_field returns a pointer to a string which holds 
 * the next field of a line of a crontab file.
 * If numbers in this field are out of range (lower..upper),
 * or there is a syntax error, then a mail message is sent
 * to the user telling him which line the error was in,
 * and NULL is returned.
 */

static char *
next_field(lower,upper,u)
	int lower,upper;
	struct usr *u;
{
	char *s;
	int num,num2,start;

	while (line[cursor] == ' ' || line[cursor] == '\t')
		cursor++;
	start = cursor;
	if (line[cursor] == '\0') {
		(void) mail(u->name,EOLN,ERR_CRONTABENT);
		return(NULL); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			(void) mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
		s = MALLOC(2, M_CRFIELD);
		strcpy(s,"*");
		return(s); 
	}
	for (;;) {
		if (!isdigit(line[cursor])) {
			(void) mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		} while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			(void) mail(u->name,OUTOFBOUND,ERR_CRONTABENT);
			return(NULL); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				(void) mail(u->name,UNEXPECT,ERR_CRONTABENT);
				return(NULL); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			} while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				(void) mail(u->name,OUTOFBOUND,ERR_CRONTABENT);
				return(NULL); 
			}
		}
		if (line[cursor] == ' ' || line[cursor] == '\t')
			break;
		if (line[cursor]=='\0') {
			(void) mail(u->name,EOLN,ERR_CRONTABENT);
			return(NULL); 
		}
		if (line[cursor++]!=',') {
			(void) mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
	}
	s = MALLOC(cursor-start+1, M_CRFIELD);
	strncpy(s,line+start,cursor-start);
	s[cursor-start] = '\0';
	return(s);
}

/*
 * Returns the integer time for the next occurrence of event e.
 * The following fields have ranges as indicated:
 *
 *	PRGM  | min	hour	day of month	mon	day of week
 *	------|-------------------------------------------------------
 *	cron  | 0-59	0-23	    1-31	1-12	0-6 (0=sunday)
 *	time  | 0-59	0-23	    1-31	0-11	0-6 (0=sunday)
 */

static time_t
next_time(e)
	struct event *e;
{
	struct tm *tm;
	int tm_mon,tm_mday,tm_wday,wday,m,min,h,hr,carry,day,days,
	d1,day1,carry1,d2,day2,carry2,daysahead,mon,yr,db,wd,today;
	time_t t;

	t = NOW();
	tm = localtime(&t);

	tm_mon = next_ge(tm->tm_mon+1,e->of.ct.month) - 1;	/* 0-11 */
	tm_mday = next_ge(tm->tm_mday,e->of.ct.daymon);		/* 1-31 */
	tm_wday = next_ge(tm->tm_wday,e->of.ct.dayweek);	/* 0-6  */

	today = TRUE;
	if ( (strcmp(e->of.ct.daymon,"*")==0 && tm->tm_wday!=tm_wday)
	    || (strcmp(e->of.ct.dayweek,"*")==0 && tm->tm_mday!=tm_mday)
	    || (tm->tm_mday!=tm_mday && tm->tm_wday!=tm_wday)
	    || (tm->tm_mon!=tm_mon))
		today = FALSE;

	m = tm->tm_min+1;
	if ((tm->tm_hour + 1) <= next_ge(tm->tm_hour%24, e->of.ct.hour)) {
		m = 0;
	}
	min = next_ge(m%60,e->of.ct.minute);
	carry = (min < m) ? 1:0;
	h = tm->tm_hour+carry;
	hr = next_ge(h%24,e->of.ct.hour);
	carry = (hr < h) ? 1:0;
	if ((!carry) && today) {
		/* this event must occur today	*/
		if (tm->tm_min>min)
			t +=(time_t)(hr-tm->tm_hour-1)*HOUR + 
			    (time_t)(60-tm->tm_min+min)*MINUTE;
		else t += (time_t)(hr-tm->tm_hour)*HOUR +
			(time_t)(min-tm->tm_min)*MINUTE;
		return(t-(long)tm->tm_sec); 
	}

	min = next_ge(0,e->of.ct.minute);
	hr = next_ge(0,e->of.ct.hour);

	/*
	 * Calculate the date of the next occurrence of this event,
	 * which will be on a different day than the current day.
	 */

	/* check monthly day specification */
	d1 = tm->tm_mday+1;
	day1 = next_ge((d1-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1,e->of.ct.daymon);
	carry1 = (day1 < d1) ? 1:0;

	/* check weekly day specification */
	d2 = tm->tm_wday+1;
	wday = next_ge(d2%7,e->of.ct.dayweek);
	if (wday < d2)
		daysahead = 7 - d2 + wday;
	else
		daysahead = wday - d2;
	day2 = (d1+daysahead-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1;
	carry2 = (day2 < d1) ? 1:0;

	/*
	 * Based on their respective specifications,
	 * day1, and day2 give the day of the month
	 * for the next occurrence of this event.
	 */

	if (strcmp(e->of.ct.daymon,"*")==0 && strcmp(e->of.ct.dayweek,"*")!=0) {
		day1 = day2;
		carry1 = carry2; 
	}
	if (strcmp(e->of.ct.daymon,"*")!=0 && strcmp(e->of.ct.dayweek,"*")==0) {
		day2 = day1;
		carry2 = carry1; 
	}

	yr = tm->tm_year;
	if ((carry1 && carry2) || (tm->tm_mon != tm_mon)) {
		/* event does not occur in this month */
		m = tm->tm_mon+1;
		mon = next_ge(m%12+1,e->of.ct.month)-1;		/* 0..11 */
		carry = (mon < m) ? 1:0;
		yr += carry;
		/* recompute day1 and day2 */
		day1 = next_ge(1,e->of.ct.daymon);
		db = days_btwn(tm->tm_mon,tm->tm_mday,tm->tm_year,mon,1,yr) + 1;
		wd = (tm->tm_wday+db)%7;
		/* wd is the day of the week of the first of month mon */
		wday = next_ge(wd,e->of.ct.dayweek);
		if (wday < wd) day2 = 1 + 7 - wd + wday;
		else day2 = 1 + wday - wd;
		if (strcmp(e->of.ct.daymon,"*")!=0 && strcmp(e->of.ct.dayweek,"*")==0)
			day2 = day1;
		if (strcmp(e->of.ct.daymon,"*")==0 && strcmp(e->of.ct.dayweek,"*")!=0)
			day1 = day2;
		day = (day1 < day2) ? day1:day2; 
	} else {
		/* event occurs in this month */
		mon = tm->tm_mon;
		if (!carry1 && !carry2)
			day = (day1 < day2) ? day1 : day2;
		else if (!carry1)
			day = day1;
		else
			day = day2;
	}

	/*
	 * Now that we have the min, hr, day, mon, yr of the next
	 * event, figure out what time that turns out to be.
	 */

	days = days_btwn(tm->tm_mon,tm->tm_mday,tm->tm_year,mon,day,yr);
	t += (time_t)(23-tm->tm_hour)*HOUR + (time_t)(60-tm->tm_min)*MINUTE
	    + (time_t)hr*HOUR + (time_t)min*MINUTE + (time_t)days*DAY;
	return(t-(long)tm->tm_sec);
}

/*
 * list is a character field as in a crontab file;
 * for example: "40,20,50-10".
 * next_ge returns the next number in the list that is
 * greater than or equal to current.
 * If no numbers of list are >= current, the smallest
 * element of list is returned.
 * NOTE: current must be in the appropriate range.
 */

#define	DUMMY	100

static int
next_ge(current,list)
	int current;
	char *list;
{
	char *ptr;
	int n,n2,min,min_gt;

	if (strcmp(list,"*") == 0)
		return(current);
	ptr = list;
	min = DUMMY; 
	min_gt = DUMMY;
	for (;;) {
		if ((n=(int)num(&ptr))==current) return(current);
		if (n<min) min=n;
		if ((n>current)&&(n<min_gt)) min_gt=n;
		if (*ptr=='-') {
			ptr++;
			if ((n2=(int)num(&ptr))>n) {
				if ((current>n)&&(current<=n2))
					return(current); 
			}
			else {	/* range that wraps around */
				if (current>n) return(current);
				if (current<=n2) return(current); 
			}
		}
		if (*ptr=='\0') break;
		ptr += 1; 
	}
	if (min_gt!=DUMMY) return(min_gt);
	else return(min);
}

/*
 * Remove the specified atjob from the given user's list of events.
 * If there is no crontab for this user either, then we can dispense
 * with the usr structure as well.
 */

static void
del_atjob(jobname, usrname, lid)
	char	*jobname;
	char	*usrname;
	level_t lid;
{
	struct	event	*e, *eprev;
	struct	usr	*u;

	if ((u = find_usr(usrname, lid)) == NULL)
		return;

	for (eprev = NULL, e = u->atevents; e != NULL; eprev = e, e = e->link)
		if (strcmp(jobname, e->cmd) == 0) {
			if (next_event == e)
				next_event = NULL;
			if (eprev == NULL)
				u->atevents = e->link;
			else
				eprev->link = e->link;
			el_remove(e->of.at.eventid, 1);
			FREE(e->cmd, M_ATCMD);
			FREE(e, M_ATEVENT);
			break;
		}

	if (u->ctid == 0 && u->atevents == NULL) {
#ifdef DEBUG
		fprintf(stderr,"%s removed from usr list\n",usrname);
#endif
		if (ulast == NULL)
			uhead = u->nextusr;
		else
			ulast->nextusr = u->nextusr;
		FREE(u->name, M_UNAME);
		FREE(u->home, M_HOME);
		FREE(u->shell, M_SHELL);
		FREE(u, M_USR);
	}
}

/*
 * The specified user no longer has a crontab;
 * remove the associated data structures.
 * If there are no atjobs for this user either,
 * then we can dispense with the usr structure
 * as well.
 */

static void
del_ctab(name, lid)
	char	*name;
	level_t lid;
{
	struct	usr	*u;

	if ((u = find_usr(name, lid)) == NULL)
		return;
	rm_ctevents(u);
	el_remove(u->ctid, 0);
	u->ctid = 0;
	if (u->atevents == NULL) {
#ifdef DEBUG
		fprintf(stderr,"%s removed from usr list\n",name);
#endif
		if (ulast == NULL)
			uhead = u->nextusr;
		else
			ulast->nextusr = u->nextusr;
		FREE(u->name, M_UNAME);
		FREE(u->home, M_HOME);
		FREE(u->shell, M_SHELL);
		FREE(u, M_USR);
	}
}


/*
 * Remove the specified user's list of crontab events,
 * freeing the associated data structures.
 */

static void
rm_ctevents(u)
	struct usr *u;
{
	register struct event *e;
	register struct event *enext;

	/*
	 * See if the next event (to be run by cron)
	 * is a cronevent owned by this user.
	 */
	if (next_event != NULL && next_event->etype == CRONEVENT
		&& next_event->u == u)
		next_event = NULL;

	for (e = u->ctevents; e != NULL; e = enext) {
		FREE(e->cmd, M_CRCMD);
		FREE(e->of.ct.minute, M_CRFIELD);
		FREE(e->of.ct.hour, M_CRFIELD);
		FREE(e->of.ct.daymon, M_CRFIELD);
		FREE(e->of.ct.month, M_CRFIELD);
		FREE(e->of.ct.dayweek, M_CRFIELD);
		if (e->of.ct.input != NULL)
			FREE(e->of.ct.input, M_IN);
		enext = e->link;
		FREE(e, M_CREVENT);
	}
	u->ctevents = NULL;
}

/*
 * Find the usr structure for the supplied user name
 * and return a pointer to it.  The user's level id
 * must match as well.  Return NULL if no match.
 *
 * Side effect:  ulast is left pointing to the previous
 * usr structure in the list as a convenience to our
 * caller, who may wish to delete the found structure
 * from the list.
 */

static struct usr *
find_usr(uname, lid)
	char	*uname;
	level_t	lid;
{
	struct usr *u;

	for (ulast = NULL, u = uhead; u != NULL; ulast = u, u = u->nextusr)
		if (strcmp(u->name, uname) == 0 && u->lid == lid)
			break;
	return(u);
}

/*
 * Allocate a new usr structure, initialize it from the
 * supplied password file entry and LID, and link it into
 * the uhead list.  Return a pointer to the new structure.
 */

static struct usr *
new_usr(pw, lid)
	struct passwd *pw;
	level_t lid;
{
	register struct usr *u;
	char *s;

	u = (struct usr *)MALLOC(sizeof(struct usr), M_USR);
	u->name = MALLOC(strlen(pw->pw_name)+1, M_UNAME);
	strcpy(u->name, pw->pw_name);
	u->home = MALLOC(strlen(pw->pw_dir)+1, M_HOME);
	strcpy(u->home, pw->pw_dir);
	s = select_shell(pw->pw_shell);
	u->shell = MALLOC(strlen(s)+1, M_SHELL);
	strcpy(u->shell, s);
	u->uid = pw->pw_uid;
	u->gid = pw->pw_gid;
	u->lid = lid;
#ifdef ATLIMIT
	u->aruncnt = 0;
#endif
#ifdef CRONLIMIT
	u->cruncnt = 0;
#endif
	u->ctid = 0;
	u->ctevents = NULL;
	u->atevents = NULL;
	u->nextusr = uhead;
	uhead = u;
	return(u);
}

/*
 * Update the specified usr structure with
 * information from the passwd structure supplied.
 */

static void
mod_usr(u, pw)
	register struct usr	*u;
	register struct passwd	*pw;
{
	char *s;
	u->uid = pw->pw_uid;
	u->gid = pw->pw_gid;

	if (strcmp(u->home, pw->pw_dir) != 0) {
		FREE(u->home, M_HOME);
		u->home = MALLOC(strlen(pw->pw_dir)+1, M_HOME);
		strcpy(u->home, pw->pw_dir);
	}
	s = select_shell(pw->pw_shell);
	if (strcmp(u->shell, s) != 0) {
		FREE(u->shell, M_SHELL);
		u->shell = MALLOC(strlen(s)+1, M_SHELL);
		strcpy(u->shell, s);
	}
}

/*
 * Execute an event.
 * If the event could not be executed now and instead was
 * rescheduled for a later time via resched(), we return 0.
 * If the event was executed now, or was discarded because
 * of an error, we return 1.
 */

static int
ex(e)
	struct event *e;
{
	register int i;
	register struct usr *u;
	short sp_flag;
	int fd;
	pid_t rfork;
	FILE *atfp;
	char atline[BUFSIZ];
	int ulim;
	char mailvar[4];
	char *cron_infile;
	struct stat buf;
	struct queue *qp;
	struct runinfo *rp;
	int retval;
	int errval;
	uinfo_t uinfo = NULL;
	actl_t	actl;
	aevt_t	aevt;
	arec_t	admp;
	acronrec_t acron_rec;
	char d_path[MAXPATHLEN], s[MAXPATHLEN];
	char save_path[MAXPATHLEN];
	char *d;
	char *path_tok;
	int no_match = 0;

	qp = &qt[e->etype];	/* set pointer to queue defs */
	if (qp->nrun >= qp->njob) {
		msg(":97:%c queue max run limit reached", QUE(e->etype));
		resched(qp->nwait);
		return(0);
	}
	for (rp=rt; rp < rt+MAXRUN; rp++) {
		if (rp->pid == 0)
			break;
	}
	if (rp >= rt+MAXRUN) {
		msg(":98:MAXRUN (%d) procs reached",MAXRUN);
		resched(qp->nwait);
		return(0);
	}

	u = e->u;

#ifdef ATLIMIT
	if (u->uid != 0 && u->aruncnt >= ATLIMIT) {
		msg(":99:ATLIMIT (%d) reached for uid %d", ATLIMIT, u->uid);
		resched(qp->nwait);
		return(0);
	}
#endif
#ifdef CRONLIMIT
	if (u->uid != 0 && u->cruncnt >= CRONLIMIT) {
		msg(":100:CRONLIMIT (%d) reached for uid %d",CRONLIMIT,u->uid);
		resched(qp->nwait);
		return(0);
	}
#endif

	rp->outfile = TEMPNAM(TMPDIR, PFX, M_OUTFILE);	/* may be NULL */
	rp->jobtype = e->etype;
	if (e->etype == CRONEVENT) {
		rp->jobname = MALLOC(strlen(e->cmd)+1, M_JOB);
		(void) strcpy(rp->jobname, e->cmd);
		rp->mailwhendone = 0;	/* "cron" jobs only produce mail if there's output */
	} else {
		rp->jobname = MALLOC(strlen(ATDIR) + strlen(e->cmd) + 2, M_JOB);
		strcat(strcat(strcpy(rp->jobname, ATDIR), "/"), e->cmd);
		if ((atfp = open_userfile(rp->jobname, u->uid)) == NULL) {
			(void) mail(u->name, BADJOBOPEN, ERR_EXECAT);
			rm_userfile(rp->jobname, u->uid, ABORT_ON_ERR);
			FREE(rp->jobname, M_JOB);
			if (rp->outfile != NULL)
				FREE(rp->outfile, M_OUTFILE);
			return(1);
		}

		/*
		 * Skip over the first two lines.
		 */
		fscanf(atfp,"%*[^\n]\n");
		fscanf(atfp,"%*[^\n]\n");

		/*
		 * Check to see if we should always send mail
		 * to the owner.
		 */
		if (fscanf(atfp,": notify by mail: %3s%*[^\n]\n",mailvar) == 1)
			rp->mailwhendone = (strcmp(mailvar, "yes") == 0);
		else
			rp->mailwhendone = 0;

		/*
		 * Read in user's ulimit from atjob file.  After forking
		 * a child process to exec the at job, set the ulimit for
		 * that child process to the ulimit obtained here.
		 */
		ulim = 0;
		while ((fgets(atline, sizeof(atline), atfp)) != NULL)
			if (strncmp(atline, "ulimit ", 6) == 0) {
				(void) sscanf(atline, "ulimit %d", &ulim);
				break;
			}
		fclose(atfp);
	}
	(void) procprivc(SETPRV, SYSOPS_W, (priv_t)0);
	rfork = fork(); errval = errno;
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	if (rfork == (pid_t)-1) {
		msg(":101:fork() failed: %s", strerror(errval));
		resched(wait_time);
		sleep(30);
		FREE(rp->jobname, M_JOB);
		if (rp->outfile != NULL)
			FREE(rp->outfile, M_OUTFILE);
		return(0);
	}
	if (rfork) {
		/*
		 * This is the parent process.
		 * Record info we'll need later to deal with
		 * the completion of the child in cleanup().
		 */
		++qp->nrun;
		++running;
		rp->pid = rfork;
		rp->que = e->etype;
		rp->uid = u->uid;
		rp->lid = u->lid;
		rp->name = MALLOC(strlen(u->name) + 1, M_RNAME);
		strcpy(rp->name, u->name);
#ifdef ATLIMIT
		if (e->etype != CRONEVENT)
			u->aruncnt++;
#endif
#ifdef CRONLIMIT
		if (e->etype == CRONEVENT)
			u->cruncnt++;
#endif
		logit((char)BCHAR,rp,0);
		return(1);
	}

	/*
	 * This is the child.  If the child must be aborted, just
	 * exit.  Don't call crabort() which is only applicable to
	 * the parent.
	 */
	{
		int fds;
		fds = sysconf(_SC_OPEN_MAX);
	for (i = 0; i < fds; i++)
		close(i);
	}

	if (e->etype != CRONEVENT ) {
		/*
		 * It's an at job - if we read in a value for user's
		 * ulimit from the at job file, we set child process'
		 * ulimit to that value.
		 */
		if (ulim) {
			(void) procprivc(SETPRV, SYSOPS_W, (priv_t)0);
			(void) ulimit(UL_SETFSIZE, ulim);
			(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		}

		/*
		 * If the file's setid bits are off, the file may have been
		 * chowned.  Executing it would risk a security violation.
		 */
		if (stat(rp->jobname, &buf) < 0)
			exit(1);
		if ((buf.st_mode & ISUID) == 0) { 
			(void) rm_userfile(rp->jobname, buf.st_uid, RETURN_ERRNO);
			exit(1); 
		}

		/*
		 * Open jobfile as stdin to shell.
		 * Because we closed all our open files, the
		 * open() below will assign file descriptor 0.
		 */
		(void) procprivc(SETPRV, SETUID_W, (priv_t)0);
		(void) seteuid(u->uid);
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		retval = open(rp->jobname, O_RDONLY);
		unlink(rp->jobname); 
		if (seteuid(cron_uid) == -1)
			exit(1);
		if (retval == -1) {
			(void) mail(u->name, BADJOBOPEN, ERR_EXECCRON);
			exit(1); 
		}
	}

	(void) procprivc(SETPRV, AUDIT_W, (priv_t)0);
	retval = auditctl(ASTATUS, &actl, sizeof(actl_t));
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
	if (retval == 0) {
		/*
		 * Retrieve the default user mask, of the user associated with
		 * the cron job being processed.  Since /etc/security/ia/index
		 * is read only for root, set DACREAD priv for when cron is
		 * started by administrator.  MACREAD is required as well
		 * because we are running at the user's lid here, and the file
		 * is at SYS_PRIVATE.
		 */
		(void) procprivc(SETPRV, DACREAD_W, MACREAD_W, (priv_t)0);
		retval = ia_openinfo(u->name, &uinfo);
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		if (retval != 0 || uinfo == NULL)
			exit(1);
		ia_get_mask(uinfo, aevt.emask);
		ia_closeinfo(uinfo);

		/* Populate the audit cron record */
		acron_rec.uid = u->uid;
		acron_rec.gid = u->gid;
		acron_rec.lid = u->lid;
		if (strlen(e->cmd) < (size_t)ADT_CRONSZ)
			strcpy(acron_rec.cronjob,e->cmd);
		else {
			strncpy(acron_rec.cronjob,e->cmd,ADT_CRONSZ-1);
			acron_rec.cronjob[ADT_CRONSZ-1]='\0';
		}
		admp.rtype = ADT_CRON;
		/*
		 * Note: the 0 (success) value does not indicate the success
		 * of the cron job.
		 */
		admp.rstatus = 0;
		admp.rsize = sizeof(acronrec_t);
		admp.argp = (char *)&acron_rec;

		/*
		 * Note that the following audit system calls all require
		 * AUDIT privilege and are bracketed together.  Do not
		 * insert here any calls not requiring the privilege.
		 */
		(void) procprivc(SETPRV, AUDIT_W, (priv_t)0);

		/*
		 * Set the user mask of the current process, cron, to the mask
		 * retrieved earlier, unless the user is alreay logged in.
		 * Cron is exempt for auditing so there is no net effect
		 * for the cron process. The soon to be exec'ed cron job
		 * will inherit this new user mask.
		 */

		/* make process auditable */
		auditevt(AYAUDIT, NULL, sizeof(aevt_t));

		aevt.uid = u->uid;
		if (auditevt(AGETUSR, &aevt, sizeof(aevt_t))) {
			if (errno != ESRCH) 
				exit(1);
		}
		if (auditevt(ASETME, &aevt, sizeof(aevt_t)) == -1)
			exit(1);

		/* write the audit cron record */
		auditdmp(&admp, sizeof(arec_t));

		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
	}


	/*
	 * Set correct user and group identification and initialize
	 * the supplementary group access list.  These calls all
	 * require SETUID privilege; do not insert here any calls
	 * not requiring the privilege.
	 */
	(void) procprivc(SETPRV, SETUID_W, (priv_t)0);

	if (setgid(u->gid) == -1
		|| initgroups(u->name, u->gid) == -1
		|| setuid(u->uid) == -1)
		exit(1);

	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	sp_flag = FALSE;
	if (e->etype == CRONEVENT)
		/* check for standard input to command */
		if (e->of.ct.input != NULL) {
			cron_infile = mktemp(TMPINFILE);
			if ((fd = creat(cron_infile,INMODE)) == -1) {
				(void) mail(u->name, NOSTDIN, ERR_EXECCRON);
				exit(1); 
			}
			i = strlen(e->of.ct.input);
			if (write(fd, e->of.ct.input, i) != i) {
				(void) mail(u->name, NOSTDIN, ERR_EXECCRON);
				unlink(cron_infile);
				exit(1); 
			}
			close(fd);
			/* open tmp file as stdin input to sh */
			if (open(cron_infile,O_RDONLY)==-1) {
				(void) mail(u->name, NOSTDIN, ERR_EXECCRON);
				unlink(cron_infile);
				exit(1); 
			}
			unlink(cron_infile); 
		}
		else if (open("/dev/null",O_RDONLY)==-1) {
			open("/",O_RDONLY);
			sp_flag = TRUE; 
		}

	/* redirect stdout and stderr for the shell */
	if (rp->outfile != NULL && creat(rp->outfile, OUTMODE) != -1)
		dup(1);
	else if (open("/dev/null",O_WRONLY) != -1)
		dup(1);
	if (sp_flag)
		close(0);
	strcpy(d_path, "/sbin:/usr/bin:/usr/sbin:/usr/lbin:");
	strcpy(save_path, d_path);
	strcpy(s, u->shell);
	d = strrchr(s, '/');
	*d = '\0';
	path_tok = strtok(d_path,":");
	if (strcmp(path_tok, s)!=0) 
		no_match = 1;
	if (no_match) {
		while ((path_tok = strtok(NULL,":")) != NULL) {
			if (strcmp(path_tok, s)==0) {
				no_match = 0;
				break;
			}
		}
	}
	if (no_match) {
		strcat(s,":");
		strcat(s, save_path);
	}
	else
		strcpy(s, save_path);
			
	if (strcmp("/u95/bin/sh", u->shell)==0) 
		sprintf(posix, "POSIX2=TRUE");
	strncat(path, s, sizeof(path) - sizeof("PATH="));
	strncat(homedir, u->home, sizeof(homedir) - sizeof("HOME="));
	strncat(logname, u->name, sizeof(logname) - sizeof("LOGNAME="));
	strncat(shellpath, u->shell, sizeof(shellpath) - sizeof("SHELL="));
	environ = envinit;
	if (chdir(u->home) == -1) {
		(void) mail(u->name, CANTCDHOME,
			    e->etype == CRONEVENT ? ERR_EXECCRON : ERR_EXECAT);
		exit(1); 
	}
#ifdef TESTING
	exit(1);
#endif
	if (u->uid != 0)
		nice(qp->nice);

	(void) procprivc(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);

	if (e->etype == CRONEVENT)
		execl(u->shell,"sh","-c",e->cmd,0);
	else /* type == ATEVENT */
		execl(u->shell,"sh",0);

	(void) mail(u->name, CANTEXECSH,
		    e->etype == CRONEVENT ? ERR_EXECCRON : ERR_EXECAT);
	exit(1);
}

/*
 * There's nothing to do for the next 'tyme' seconds.
 * Delay by waiting for completion of child processes,
 * if we have any, or by awaiting receipt of a message
 * from at(1) or crontab(1).
 */

static void
idle(tyme)
	long	tyme;
{
	long	t;
	pid_t	pid;
	int	prc;
	long	alm;
	struct	runinfo	*rp;

	for (t = tyme; t > 0; t = idletime(NOW())) {
		if (running == 0) {
			(void) msg_wait();
			return;
		}
		alm = MIN(t, wait_time);
#ifdef DEBUG
		fprintf(stderr, "in idle - setting alarm for %ld sec\n", alm);
#endif
		alarm((unsigned) alm);
		pid = wait(&prc);
		alarm(0);
#ifdef DEBUG
		fprintf(stderr,"wait returned %x\n",prc);
#endif
		if (pid == (pid_t)-1) {
			if (msg_wait())
				return;
		} else {
			for (rp=rt;rp < rt+MAXRUN; rp++)
				if (rp->pid == pid)
					break;
			if (rp >= rt+MAXRUN) {
				msg(":102:Unexpected pid returned %d (ignored)",pid);
				/* incremented in mail() */
				running--;
			} else {
				setlevel(&rp->lid, ABORT_ON_ERR);

				if (rp->que == ZOMB) {
					/*
					 * Child was forked by mail() to send
					 * user the job output.  The task of
					 * removing the output tempfile was
					 * left to us, since it could not be
					 * removed safely until the child was
					 * finished with it.  See cleanup().
					 */
					running--;
					rp->pid = 0;
					rm_userfile(rp->outfile, rp->uid,
						ABORT_ON_ERR);
					FREE(rp->outfile, M_OUTFILE);
				} else
					cleanup(rp,prc);

				setlevel(&cron_lid, ABORT_ON_ERR);
			}
		}
	}
}

/*
 * Clean up after completion of a job.
 * The main thing to do here, besides bookkeeping,
 * is to (optionally) send a mail message to the
 * user indicating that the job has completed.
 */

static void
cleanup(rp, rc)
	struct runinfo	*rp;
	int		rc;
{
	struct stat	buf;
	char		*f;
#if ATLIMIT || CRONLIMIT
	struct usr	*u;

	/*
	 * We used to keep a back-pointer to the usr struct from the runinfo
	 * struct, which made this easy but meant we could never free the
	 * usr structures without risking a dangling pointer.  This way is
	 * clumsy and can be fooled if re-initialization takes place while
	 * there are running jobs, but on the whole seems preferable.
	 */
	if ((u = find_usr(rp->name, rp->lid)) != NULL && u->uid == rp->uid) {
#ifdef ATLIMIT
		if (rp->que != CRONEVENT && u->aruncnt > 0)
			--u->aruncnt;
#endif
#ifdef CRONLIMIT
		if (rp->que == CRONEVENT && u->cruncnt > 0)
			--u->cruncnt;
#endif
	}
#endif

	logit((char)ECHAR, rp, rc);
	--qt[rp->que].nrun;
	rp->pid = 0;
	--running;

	/*
	 * If there was stdout or stderr output from the job,
	 * or the user requested mail on job completion, send mail.
	 */
	if ((f = rp->outfile) != NULL && (stat(f, &buf) || buf.st_size == 0))
		f = NULL;
	if (f != NULL || rp->mailwhendone) {
		if (rp->jobtype == CRONEVENT)
			rp->pid = mail(rp->name, OUTCRON, OUT_CRON, rp->jobname, f);
		else
			rp->pid = mail(rp->name, OUTAT, OUT_AT, rp->jobname, f);

		if (rp->pid < 0)	/* fork failed */
			rp->pid = 0;
	}

	/*
	 * If we successfully forked a child process to send mail,
	 * we don't want to remove the job output file at this point;
	 * the child process may need it.  Instead, we flag the child
	 * as a "zombie" process; idle() will remove the output file
	 * when it notes that the child has terminated.
	 */
	if (rp->pid > 0)
		rp->que = ZOMB;
	else if (rp->outfile != NULL) {
		rm_userfile(rp->outfile, rp->uid, ABORT_ON_ERR);
		FREE(rp->outfile, M_OUTFILE);
	}

	FREE(rp->name, M_RNAME);
	FREE(rp->jobname, M_JOB);
}

/*
 * While we're waiting for the next scheduled event,
 * see if we get any messages from at(1) or crontab(1).
 * Returns 0 if there are no messages waiting and we have
 * child processes running; this allows idle(), our caller,
 * to see if any child processes have terminated.
 * Otherwise returns 1.
 *
 * Side effect:  sets the 'notexpired' flag to prevent
 * main() from thinking that the time to the next event
 * has been reached.
 */

static int
msg_wait()
{
	long	t;
	struct	passwd *pw;
	int 	subsize;
	int	retval;
	int	errval;
	struct obj_attr obj;
	static struct pollfd pollfd;
	static struct s_strrecvfd *recbuf = NULL;
	static struct message msgbuf;
	level_t level = 0;	/* Set default for when MAC is not installed */

	/*
	 * If this is the first time through, perform one-time
	 * initialization of our static data structures.
	 */
	if (recbuf == NULL) {
		/* get size of the subject attributes structure */
		if ((subsize = secadvise(0, SA_SUBSIZE, 0)) < 0)
			crabort(BADSECADVIS, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

		/* allocate buffer for ioctl(I_S_RECVFD), never freed */
		recbuf = (struct s_strrecvfd *) MALLOC(
				sizeof(struct s_strrecvfd) +
				subsize - sizeof(struct sub_attr), M_RECBUF);

		/* initialize argument structure for poll() */
		pollfd.fd = msgfd;
		pollfd.events = POLLIN;
	}

	if (poll(&pollfd, 1, 0) <= 0 && running)
		return(0);

	t = idletime(NOW());
	t = MAX(t, 1);
#ifdef DEBUG
	fprintf(stderr,"in msg_wait - setting alarm for %ld sec\n", t);
#endif
	alarm((unsigned) t);
	recbuf->fd = 0;

	/*
	 * Retrieve credentials from the sending process.
	 */
	(void) procprivc(SETPRV, MACWRITE_W, (priv_t)0);
	retval = ioctl(msgfd, I_S_RECVFD, recbuf); errval = errno;
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
	if (retval != 0) {
		if (errval != EINTR) {
			pfmt(stderr, MM_ERROR, ":622:ioctl() I_S_RECVFD failed: %s\n",
				strerror(errval));
			notexpired = 1;
		}
		if (next_event == NULL)
			notexpired = 1;
		return(1);
	}

	if (read(recbuf->fd, (char *)&msgbuf, sizeof msgbuf) != sizeof msgbuf) {
		if (errno != EINTR) {
			msg(":108:Read error: %s", strerror(errno));
			notexpired = 1;
		}
		if (next_event == NULL)
			notexpired = 1;
		close(recbuf->fd);
		return(1);
	}
	if (mac_installed) {
		(void) procprivc(SETPRV, MACREAD_W, (priv_t)0);
		retval = flvlfile(recbuf->fd, MAC_GET, &level);
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);
		if (retval != 0)
			crabort(":623:Cannot get level for unique pipe\n",
				REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
	}
	close(recbuf->fd);

	/*
	 * Get information on login whose jobs are to be manipulated.
	 * This is done by getting information on login after which
	 * the crontab file is named or, in the case of at, the owner
	 * of the at job file.
	 */
	if ((pw = getpwnam(msgbuf.logname)) == NULL) {
		msg(":747:Cannot get data on login %s: %s", msgbuf.logname, strerror(errno));
		notexpired = 1;
		return(1);
	}
	obj.uid = pw->pw_uid;	
	obj.gid = pw->pw_gid;
	obj.mode = 0700;
	obj.lid = level;	/* Level of process that sent msg to cron */

	if (secadvise(&obj, SA_WRITE, &(recbuf->s_attrs)) < 0)
		if (errno == EFAULT)
			crabort(":625:Can't determine privileges of sending process\n",
				REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		else {
			switch (msgbuf.action) {
			case DELETE:
				msg(":626:Attempt by uid %d to delete at/cron job rejected",
					recbuf->uid);
				break;
			case ADD:
				msg(":627:Attempt by uid %d to add/change at/cron job rejected",
					recbuf->uid);
				break;
			default:
				msg(":628:Attempt by uid %d to manipulate at/cron job rejected",
					recbuf->uid);
				break;
			}
			notexpired = 1;
			return(1);
		}

	alarm(0);

	if (msgbuf.etype != '\0')
		if (setlevel(&level, RETURN_ERRNO) != 0)
			msgbuf.etype = '\0';

	switch (msgbuf.etype) {
	case AT:
		if (msgbuf.action == DELETE) {
			if (log)
				msg(":629:Deleting at job %s for login %s",
					msgbuf.fname, msgbuf.logname);
			del_atjob(msgbuf.fname, msgbuf.logname, level);
		} else {
			if (log)
				msg(":630:Adding/modifying at job %s for login %s",
					msgbuf.fname, msgbuf.logname);
			mod_atjob(msgbuf.fname, level);
		}
		break;
	case CRON:
		if (msgbuf.action == DELETE) {
			if (log)
				msg(":631:Deleting cron job(s) for %s", msgbuf.fname);
			del_ctab(msgbuf.fname, level);
		} else {
			if (log)
				msg(":632:Adding/modifying cron job(s) for %s", msgbuf.fname);
			mod_ctab(msgbuf.fname, level);
		}
		break;
	default:
		msg(":109:Message received - bad format");
		break;
	}

	if (msgbuf.etype != '\0')
		setlevel(&cron_lid, ABORT_ON_ERR);

	if (next_event != NULL) {
		if (next_event->etype == CRONEVENT)
			el_add((int *)next_event, next_event->time,
				next_event->u->ctid);
		else /* etype == ATEVENT */
			el_add((int *)next_event, next_event->time,
				next_event->of.at.eventid);
		next_event = NULL;
	}
	fflush(stdout);
	notexpired = 1;
	return(1);
}

/*
 * Handler for alarm clock (SIGALRM) signals.
 * There's nothing for it to do besides re-prime the signal.
 * Catching the signal will cause interrupted system calls
 * to return EINTR, which is what we want.
 */

/* ARGSUSED */
static void
timeout(s)
	int s;
{
	signal(SIGALRM, timeout);
}

/*
 * Handler for termination (SIGTERM) signal.
 */

/* ARGSUSED */
static void
cronend(s)
	int s;
{
	crabort(":110:SIGTERM", REMOVE_NPIPE, "", "", "");
}

/*
 * crabort() - handle exits out of cron.
 */

static void
crabort(mssg, action, a1, a2, a3)
	char	*mssg;
	int	action;
	char	*a1, *a2, *a3;
{
	FILE	*cons;
	int	errval;

	if (action & REMOVE_NPIPE) {
		/*
		 * NPIPE should vanish when cron finishes so detach
		 * and unlink it.
		 */
		if ((errval = setlevel(&low_lid, RETURN_ERRNO)) != 0)
			pfmt(stderr, MM_ERROR, BADLVLPROC, strerror(errval));

		(void) fdetach(NPIPE);

		if (unlink(NPIPE) < 0)
			pfmt(stderr, MM_ERROR, ":633:Cannot unlink %s: %s\n",
				NPIPE, strerror(errno));
	}

	if ((errval = setlevel(&cron_lid, RETURN_ERRNO)) != 0)
		pfmt(stderr, MM_ERROR, BADLVLPROC, strerror(errval));

	if (action & CONSOLE_MSG) {
		/*
		 * Write error message to console.
		 */
		if ((cons = fopen(CONSOLE,"w")) != NULL) {
			pfmt(cons, MM_NOSTD, ":112:cron aborted: ");
			pfmt(cons, MM_NOSTD, mssg, a1, a2, a3);
			if (mssg[strlen(mssg) - 1] != '\n')
				putc('\n', cons);
			fclose(cons); 
		}

		/*
		 * Write error message to stderr.
		 */
		pfmt(stderr, MM_NOSTD, ":112:cron aborted: ");
		pfmt(stderr, MM_NOSTD, mssg, a1, a2, a3);
		if (mssg[strlen(mssg) - 1] != '\n')
			putc('\n', stderr);
	}

	/* always log the message */
	msg(mssg, a1, a2, a3);

	msg(":113:******* CRON ABORTED ********");
	exit(1);
}

/*
 * Format and print a message to the log file.
 */

static void
msg(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;
	time_t	t;

	if (logf == NULL)
		return;

	t = NOW();

	va_start(args);
	fmt = va_arg(args, char *);
	(void) vpfmt(logf, MM_NOSTD, fmt, args);
	va_end(args);

	cftime(timebuf, gettxt(FORMATID, FORMAT), &t);
	fprintf(logf, " %s\n", timebuf);
	fflush(logf);
}

/*
 * Log the beginning (cc == BCHAR) or end (cc == ECHAR)
 * of a job.  In the latter case rc is the job return code
 * from wait(2); for the beginning of a job it will be 0.
 */

static void
logit(cc,rp,rc)
	char	cc;
	struct	runinfo	*rp;
	int	rc;
{
	time_t t;
	int    ret;

	if (!log)
		return;

	t = NOW();
	if (cc == BCHAR)
		fprintf(logf, "%c  CMD: %s\n",cc, next_event->cmd);
	cftime(timebuf, gettxt(FORMATID, FORMAT), &t);
	fprintf(logf,"%c  %s %u %c %s",
		cc, rp->name, rp->pid, QUE(rp->que), timebuf);
	if ((ret=TSTAT(rc)) != 0)
		fprintf(logf," ts=%d",ret);
	if ((ret=RCODE(rc)) != 0)
		fprintf(logf," rc=%d",ret);
	putc('\n', logf);
	fflush(logf);
}

/*
 * Reschedule the current job (next_event) to run at a later time.
 * Called from ex() if the job could not be run because some
 * queue limit, job limit or process limit was exceeded.
 */

static void
resched(delay)
	int	delay;
{
	time_t	nt;

	nt = next_event->time + delay;
	if (next_event->etype == CRONEVENT) {
		next_event->time = next_time(next_event);
		if (nt < next_event->time)
			next_event->time = nt;
		el_add((int *)next_event, next_event->time, next_event->u->ctid);
		msg(RESCHEDCRON, next_event->cmd);
		return;
	}
	add_atevent(next_event->u, next_event->cmd, nt, next_event->etype);
	msg(RESCHEDAT, next_event->cmd);
}

/*
 * Initialize the job queue info structures.
 * If action is DEFAULT, the default values are used;
 * otherwise (action is LOAD) the QUEDEFS file is
 * read and its values override the defaults.
 */

#define	QBUFSIZ	80	/* max length of line in QUEDEFS file */

static void
quedefs(action)
	int	action;
{
	register int i;
	char	qbuf[QBUFSIZ];
	FILE	*fd;

	/* set up default queue definitions */
	for (i = 0; i < NQUEUE; i++) {
		qt[i].njob = qd.njob;
		qt[i].nice = qd.nice;
		qt[i].nwait = qd.nwait;
	}
	if (action == DEFAULT)
		return;
	if ((fd = fopen(QUEDEFS,"r")) == NULL) {
		msg(":115:Cannot open quedefs file: %s", strerror(errno));
		msg(":116:Using default queue definitions");
		return;
	}
	while (fgets(qbuf, QBUFSIZ, fd) != NULL) {
		if ((i = JTYP(qbuf[0])) < 0 || i >= NQUEUE || qbuf[1] != '.')
			continue;	/* skip bad entry */
		parseqdef(&qbuf[2], &qt[i]);
	}
	fclose(fd);
}

/*
 * Interpret a line from the QUEDEFS file and set fields
 * of the supplied job queue info structure accordingly.
 *
 * Each line in the QUEDEFS file starts with a queue identifier
 * (a letter from a to z corresponding to a queue number from
 * 0 to 25) followed by a ".", followed by zero or more instances
 * of a parameter specification consisting of a number (parameter
 * value) and a letter (parameter name).  For example,
 *
 *	b.2j12n90w
 *
 * We are given a pointer to the third character of the line,
 * where the parameter specifications start, and a pointer to
 * the corresponding job queue info structure.
 * Unrecognized parameter specifications are ignored.
 */

static void
parseqdef(parms, qp)
	char *parms;
	struct queue *qp;
{
	register int i;

	for (;;) {
		i = 0;
		while (isdigit(*parms)) {
			i *= 10;
			i += *parms++ - '0';
		}
		switch (*parms++) {
		case JOBF:
			qp->njob = i;
			break;
		case NICEF:
			qp->nice = i;
			break;
		case WAITF:
			qp->nwait = i;
			break;
		case '\n':
		case '\0':
			return;
		}
	}
}

/*
 * Read program defaults from DEFFILE (/etc/default/cron).
 * Also initialize the TZ (timezone) environment variable
 * for the default user environment.
 */

static void
defaults()
{
	FILE *def_fp;
	char *tz;
	register char *def;

	/*
	 * Get TZ value for environment.
	 */
	sprintf(tzone, "TZ=%s", ((tz = getenv("TZ")) != NULL) ? tz : DEFTZ);

	if ((def_fp = defopen(DEFFILE)) != NULL) {
		if ((def = defread(def_fp, "CRONLOG")) == NULL
			|| *def == 'N' || *def == 'n')
			log = 0;
		else
			log = 1;

		if ((def = defread(def_fp, "BACKUP")) == NULL)
			strcpy(logf_backup, BACKUP);
		else
			strcpy(logf_backup, def);

		if ((def = defread(def_fp, "LINES")) == NULL) 
			logf_lines = LINES;
		else
			logf_lines = atoi(def);

		if ((def = defread(def_fp, "SIZE")) == NULL)
			logf_size = SIZE;
		else
			logf_size = atoi(def);

		(void) defclose(def_fp);
	}
}

/*
 * Set the process level using the specified lid.
 * The action to be taken on failure is determined
 * by the second argument, either RETURN_ERRNO
 * or ABORT_ON_ERR.
 *
 * Returns 0 on success and if MAC is not installed.
 */

static int
setlevel(lidp, failure_action)
	level_t	*lidp;
	int	failure_action;
{
	int	retval;
	int	errval;

	if (mac_installed) {
		(void) procprivc(SETPRV, SETPLEVEL_W, (priv_t)0);
		retval = lvlproc(MAC_SET, lidp); errval = errno;
		(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

		if (retval == -1) {
			if (failure_action != RETURN_ERRNO)
				crabort(NOLVLPROC, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
			return(errval);
		}
	}
	return(0);
}

/*
 * Open a crontab or at command file owned by a specified user.
 * To avoid having to use more powerful privileges, we change our
 * effective uid to that of the user to perform the open operation,
 * then change it back again.  If the attempt to change it back
 * fails, we abort unconditionally.
 *
 * Returns the (possibly NULL) file pointer from fopen().
 */

static FILE *
open_userfile(path, uid)
	char	*path;
	uid_t	uid;
{
	FILE	*fp;

	(void) procprivc(SETPRV, SETUID_W, (priv_t)0);
	(void) seteuid(uid);
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	fp = fopen(path, "r");

	if (seteuid(cron_uid) == -1)
		crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");

	return(fp);
}

/*
 * Remove a command file or temporary file owned by a specified user.
 * To avoid having to use more powerful privileges, we change our
 * effective uid to that of the user to perform the unlink operation,
 * then change it back again.
 *
 * The action to be taken on failure is determined by the third
 * argument, either RETURN_ERRNO or ABORT_ON_ERR.  We care only about
 * failure to set our uid back to that of cron.
 */

static int
rm_userfile(path, uid, failure_action)
	char	*path;
	uid_t	uid;
	int	failure_action;
{
	(void) procprivc(SETPRV, SETUID_W, (priv_t)0);
	(void) seteuid(uid);
	(void) procprivc(CLRPRV, PRIVS_W, (priv_t)0);

	(void) unlink(path);

	if (seteuid(cron_uid) == -1) {
		if (failure_action != RETURN_ERRNO)
			crabort(LPMERR, REMOVE_NPIPE|CONSOLE_MSG, "", "", "");
		return(errno);
	}
	return(0);
}

/*
 * Return the time remaining until the next event.
 * Caller supplies us with the current time.
 * We never return a negative number.
 */

static time_t
idletime(now)
	time_t now;
{
	time_t t;

	if (next_event == NULL)
		return(INFINITY);

	t = next_event->time - now;
	return(MAX(t, 0));
}

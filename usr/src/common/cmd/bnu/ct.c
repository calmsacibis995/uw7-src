#ident	"@(#)ct.c	1.7"

/*
 *
 *	ct [-h] [-v] [-w n] [-x n] [-s speed] telno ...
 *
 *	dials the given telephone number, waits for the
 *	modem to answer, and initiates a login process.
 *
 *	ct uses the connection server via the dial() interface.
 *	dials() returns a valid file descriptor or a negative
 *	error value.
 */

#include "uucp.h"
#include "sysfiles.h"
#include <dial.h>
#include <pwd.h>
#include <utmp.h>
#include <wait.h>
#include <sys/stat.h>

#define ROOT	0
#define SYS	3
#define TTYGID	(gid_t) 7		/* group id for terminal */
#define TTYMOD	(mode_t) 0622
#define DEV	"/dev/"
#define TELNOSIZE	32		/* maximum phone # size is 31 */
#define LEGAL	"0123456789-*#="
#define USAGE	"[-h] [-v] [-w n] [-x n] [-s speed] telno ..."
#define LOG	"/var/adm/ctlog"
#define	TTYMON	"/usr/lib/saf/ttymon"
#define TRUE	1
#define FALSE	0

#ifdef GETTY
static char	*Dc = NULL;		/* device name for GETTY */
#endif

static int	_Status;		/* exit status of child */

static pid_t	_Pid = 0;		/* process id of child */

static char	_Tty[sizeof DEV+12] = "";  /* /dev/... for connection device */

static char	*_Num;			/* pointer to a phone number */

static time_t	_Log_on, _Log_elpsd;

static FILE	*_Fdl;

extern int  optind;
extern char *optarg;
extern void cleanup();
extern struct passwd  *getpwuid ();
extern struct utmp *getutid (), *getutent(), *pututline();
extern void setutent(), endutent();

static int logproc(), exists();
static void startat(), stopat(), disconnect(), zero();

/*
 * These two dummy routines are needed because the uucp routines
 * used by ct reference them, but they will never be
 * called when executing from ct
 */

/*VARARGS*/
/*ARGSUSED*/
void
assert (s1, s2, i1, s3, i2)
char *s1, *s2, *s3;
int i1, i2;
{ 
	fprintf(stderr,"ct: %s %s %d\n",s2,s1,i1);  /* for ASSERT in uucp.h*/
}

/*ARGSUSED*/
void
logent (s1, s2)
char *s1, *s2;
{ }		/* for ASSERT in rwioctl.c */

jmp_buf Sjbuf;			/* used by uucp routines */

static calls_t	call;	/* structure for dials() */
calls_t	*ret_call;	/* structure for returned calls from dials() */
static dial_status_t dstatus; /* dialer status returned by dials() */

static char	*char_speed;

static int	fdl;

main (argc, argv)
char   *argv[];
{
	register int    c;
	int	errors = 0;

	int     count,
		wait_count,
	        hangup = 1,	/* hangup by default */
	        minutes = 0;	/* number of minutes to wait for dialer */

	struct termios   termio;

	typedef void (*save_sig)();
	save_sig	save_hup,
			save_quit,
			save_int;

	save_hup = signal (SIGHUP, cleanup);
	save_quit = signal (SIGQUIT, cleanup);
	save_int = signal (SIGINT, cleanup);
	(void) signal (SIGTERM, cleanup);
	(void) strcpy (Progname, "ct");

	call.attr = NULL;
	call.speed = 1200;
	char_speed = "1200";
	call.device_name = NULL;
	call.telno = NULL;
	call.caller_telno = NULL;
	call.sysname = NULL;
	call.function = FUNC_NULL;
	call.class = "ACU";
	call.protocol = NULL;
	call.pinfo_len = 0;
	call.pinfo = NULL;

	while ((c = getopt (argc, argv, "hvw:s:x:")) != EOF) {
		switch (c) {
		case 'h': 
			hangup = 0;
			break;

		case 'v': 
			Verbose = 1;
			break;

		case 'w': 
			minutes = atoi (optarg);
			if (minutes < 1) {
			    (void) fprintf(stderr,
				"\tusage: %s %s\n", Progname, USAGE);
			    (void) fprintf(stderr,
				"(-w %s) Wait time must be > 0\n", optarg);
			    cleanup(101);
			}
			break;

		case 's': 
			call.speed = atoi(optarg);
			char_speed = optarg;
			break;

		case 'x':
			Debug = atoi(optarg);
			if (Debug < 0 || Debug > 9) {
			    (void) fprintf(stderr,
				"\tusage: %s %s\n", Progname, USAGE);
			    (void) fprintf(stderr,
				"(-x %s) value must be 0-9\n", optarg);
			    cleanup(101);
			}
			break;

		case '?': 
			(void) fprintf(stderr, "\tusage: %s %s\n", Progname, USAGE);
			cleanup(101);
			/* NOTREACHED */
		}
	}

	if (optind == argc) {
		(void) fprintf(stderr, "\tusage: %s %s\n", Progname, USAGE);
		(void) fprintf(stderr, "No phone numbers specified!\n");
		cleanup(101);
	}

	/* check for valid phone number(s) */
	for (count = argc - 1; count >= optind; --count) {
		_Num = argv[count];
		if (strlen(_Num) >= (size_t)(TELNOSIZE - 1)) {
		    (void) fprintf(stderr,
			"ct: phone number too long -- %s\n", _Num);
		    ++errors;
		}
		if (strspn(_Num, LEGAL) < strlen(_Num)) {
		    (void) fprintf(stderr,
			"ct: bad phone number -- %s\n", _Num);
		    ++errors;
		}
	}

	if (errors)
		cleanup(101);

	(void) signal(SIGHUP, SIG_IGN);

	if (!isatty(0))
		hangup = 0;

	if (hangup) {  /* -h option not specified */
		do {
		    (void) fputs ("Confirm hang-up? (y/n): ", stdout);
		    switch (c=tolower(getchar())) {
		    case EOF:
		    case 'n':
			cleanup(101);
			break;
		    case 'y':
			break;
		    default:
			while ( c != EOF && c != '\n' )
				c=getchar();
			break;
		    } /* end of switch */
		} while (c != 'y');

		/* close stderr if it is not redirected */
		if ( isatty(2) ) {
			Verbose = 0;
			Debug = 0;
			(void) close (2);
		}

		(void) ioctl (0, TCGETS, &termio);
		termio.c_cflag = 0;			/* speed to zero ... */
		(void) ioctl (0, TCSETSW, &termio);	/* and hang up */
		(void) sleep (5);
	}

    for (wait_count = 0; ; wait_count++) {

	/* Try each phone number until a connection is made, or none work */

	for (count = optind; count < argc; count++) {
		/* call dial() to make connection via connection server */
		call.telno = argv[count];
		fdl = dials(&call, &ret_call, &dstatus);
		if (fdl >= 0) {
		    _Fdl = fdopen(fdl, "r+");
		    break;
		}
	}

	/* check why the loop ended (connected or no more numbers to try) */
	if (count != argc)	/* got connection */
		break;

	/* ct used to reserve a dialer. Since it is now using the
	 * the connection server, it has no knowledge of devices
	 * or  device allocation. Now, if dials() fails and either
	 * -h was specified (i.e. the user is still around) or
	 * -w was specified (i.e. the user said to wait), we will
	 * retry the connection.
	 */

	if (!minutes) { /* no -w supplied on command line */

		if (!isatty(0) )  {  /* not a terminal - get out */
		    cleanup(101);
		}

		/* Ask user if she/he wants to wait */
		(void) fputs("Do you want to wait for dialer? (y for yes): ", stdout);
		if ((c = getchar ()) == EOF || tolower (c) != 'y')
		    cleanup(101);
		while ( (c = getchar()) != EOF && c != '\n')
		    ;

		(void) fputs ("Time, in minutes? ", stdout);
		(void) scanf ("%d", &minutes);
		while ( (c = getchar()) != EOF && c != '\n')
		    ;

		if (minutes <= 0)
		    cleanup(101);

	}

	if (wait_count >= minutes) {
		if (wait_count)
			(void) fputs("*** TIMEOUT ***\n", stdout);
		cleanup(101);
	}

	(void) fprintf(stdout, "Waiting for a dialer\n");
	sleep(60);

    } /* end of for (wait_count ...) loop */

	/****** Successfully made connection ******/
	VERBOSE("Connected\n%s", "");

	/* ignore some signals if they were ignored upon invocation of ct */
	/* or else, have them go to graceful disconnect */

	(void) signal(SIGHUP, (save_hup  == SIG_IGN ? SIG_IGN : disconnect));
	(void) signal(SIGHUP, (save_int  == SIG_IGN ? SIG_IGN : disconnect));
	(void) signal(SIGHUP, (save_quit == SIG_IGN ? SIG_IGN : disconnect));

	(void) signal (SIGTERM, disconnect);
	(void) signal (SIGALRM, disconnect);

	(void) sleep (2);	/* allow time for line/modem to settle */

	_Log_on = time ((long *) 0);

	if (logproc(fdl)) {	/* logproc() also sets _Tty */
		(void) fputs("Hit carriage return ", _Fdl);
		(void) fclose(_Fdl);
		CDEBUG(4, "there is a login process; exit\n%s", "");
		exit(0);
	}

#ifdef GETTY
	if (isastream(fdl) != 1) {
		/* we can't do getty without a device name */
		if ((Dc = ttyname(fdl)) == NULL)
			cleanup(101);
		if (EQUALSN(Dc, DEV, strlen(DEV)))
			Dc += 5;
		CDEBUG(4, "start login process (%s ", GETTY);
		CDEBUG(4, "-h -t 60 %s ", Dc);
		CDEBUG(4, "%s)\n", char_speed);
	} else 
#endif
	{
	CDEBUG(4, "start login process (%s ", TTYMON);
	CDEBUG(4, "-g -h -t 60 -l %s)\n", char_speed);
	}

	for (;;) {
		pid_t w_ret;
		switch(_Pid = fork()) {
		case -1:	/* fork failed */
		    if ((!hangup || Verbose))
			(void) fputs ("ct: can't fork for login process\n", stderr);
		    cleanup(101);
		    /*NOTREACHED*/

		case 0:		/* child process */
		    startat ();
		    (void) close(2);
		    /* ttymon will use open fd 0 for connection */
		    if ( fdl != 0 ) {
			(void) close(0);
			dup(fdl);
		    }
		    (void) signal(SIGHUP, SIG_DFL);  /* so child will exit on hangup */
#ifdef GETTY
		    if (Dc) {
			setsid();
			(void) execl(GETTY, "getty", "-h", "-t", "60",
				Dc, char_speed, (char *) 0);
		    } else
#endif
			(void) execl(TTYMON, "ttymon", "-g", "-h", "-t", "60",
				"-l", char_speed, (char *) 0);
		    /* exec failed */
		    cleanup(101);
		    /*NOTREACHED*/

		default:	/* parent process */
		    break;
		}

		/* Parent process */

		while ((w_ret = wait(&_Status)) != _Pid)
		    if (w_ret == -1 && errno != EINTR) {
			VERBOSE("ct: wait failed errno=%d\n", errno);
			cleanup(101);
		    }
		if ((_Status & 0xff00) < 0) {
		    if (!hangup)
			VERBOSE("ct: can't exec login process\n%s", "");
		    cleanup(101);
		}

		stopat(call.telno);

	    rewind (_Fdl);	/* flush line */
	    (void) fputs ("\nReconnect? ", _Fdl);

	    rewind (_Fdl);
	    (void) alarm (20);
	    c = getc (_Fdl);

	    if (c == EOF || tolower (c) == 'n')
		    disconnect (0);	/* normal disconnect */
	    while ( (c = getc(_Fdl)) != EOF && c != '\n')
		    ;
	    (void) alarm (0);
	}
}

static void
disconnect (code)
{
	struct termios   termio;

	(void) alarm(0);
	(void) signal (SIGALRM, SIG_IGN);
	(void) signal (SIGINT, SIG_IGN);
	(void) signal (SIGTERM, SIG_IGN);

	_Log_elpsd = time ((long *) 0) - _Log_on;

	(void) Ioctl (fileno(_Fdl), TCGETS, &termio);
	termio.c_cflag &= ~CBAUD;			/* speed to zero ... */
	(void) Ioctl (fileno(_Fdl), TCSETSW, &termio);  /* hang up line */

	DEBUG(5, "Disconnect(%d)\n", code);
	VERBOSE("Disconnected\n%s", "");

	/* For normal disconnect or timeout on "Reconnect?" message,
	   we already cleaned up above */

	if ((code != 0) && (code != SIGALRM))
		stopat(call.telno);

	cleanup(code);
}

/*
 * clean and exit with "code" status
 */
void
cleanup (code)
register int    code;
{
	CDEBUG(5, "cleanup(%d)\n", code);

	if (fdl >=0 &&  (*_Tty != '\0') ) {
		CDEBUG(5, "chmod/chown %s\n", *_Tty ? _Tty : "<unknown>");
		if (fchown(fdl , UUCPUID, TTYGID) < 0 ) {
			CDEBUG(5, "Can't chown to uid=%ld, ", (long) UUCPUID);
			CDEBUG(5, "gid=%ld\n", (long) TTYGID);
		}
		if (fchmod(fdl , TTYMOD) < 0) {
			CDEBUG(5, "Can't chmod to %lo\n", (unsigned long) TTYMOD);
		}
	}

	undials(fdl, ret_call);

	if (_Pid) { /* kill the child process */
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
		(void) kill (_Pid, SIGKILL);
	}
	exit (code);
}

/*
 * Check if there is a login process active on this line.
 * Return:
 *	0 - there is no login process on this line
 *	1 - found a login process on this line
 */

static int
logproc(fd)
int fd;
{
	register struct utmp   *u;
	char *line, *slash;

	if ((line = ttyname(fd)) == NULL)
		return(0);

	for (;;) {
		if (strncmp(line, "/dev/", 5) == 0) {
			line +=5;
			break;
		}

		if (strlen(line) <= (size_t) 12)
			break;

		if ((slash = strchr(++line, '/')) == NULL)
			break;
		else
			line = slash;
	}

	(void) strcpy(_Tty, line);

	while ((u = getutent()) != NULL) {
		if (u->ut_type == LOGIN_PROCESS
		    && EQUALS(u->ut_line, line)
		    && EQUALS(u->ut_user, "LOGIN") ) {
			CDEBUG(7, "ut_line %s, ", u->ut_line);
			CDEBUG(7, "ut_user %s, ", u->ut_user);
			CDEBUG(7, "ut_id %.4s, ", u->ut_id);
			CDEBUG(7, "ut_pid %d\n", u->ut_pid);

			/* see if the process is still active */
			if (kill(u->ut_pid, 0) == 0 || errno == EPERM) {
			    CDEBUG(4, "process still active\n%s", "");
			    return(1);
			}
		}
	}
	return(0);
}

/*
 * Create an entry in utmp file if one does not already exist.
 */
static void
startat ()
{
	struct utmp utmpbuf;
	register struct utmp   *u;
	int fd;

/*	Set up the prototype for the utmp structure we want to write.	*/

	u = &utmpbuf;
	zero (&u -> ut_user[0], sizeof (u -> ut_user));
	zero (&u -> ut_line[0], sizeof (u -> ut_line));

/*	Fill in the various fields of the utmp structure.		*/

	u -> ut_id[0] = 'c';
	u -> ut_id[1] = 't';
	u -> ut_id[2] = _Tty[strlen(_Tty)-2];
	u -> ut_id[3] = _Tty[strlen(_Tty)-1];
	u -> ut_pid = getpid ();

	u -> ut_exit.e_termination = 0;
	u -> ut_exit.e_exit = 0;
	u -> ut_type = INIT_PROCESS;
	time (&u -> ut_time);
	setutent ();		/* Start at beginning of utmp file. */

/*	For INIT_PROCESSes put in the name of the program in the	*/
/*	"ut_user" field.						*/

	strncpy (&u -> ut_user[0], "ttymon", sizeof (u -> ut_user));
	strncpy (&u -> ut_line[0], _Tty, sizeof (u -> ut_line));

/*	Write out the updated entry to utmp file.			*/
	pututline (u);

/*	Now attempt to add to the end of the wtmp file.  Do not create	*/
/*	if it doesn't already exist. Do not overwrite any info already	*/
/*	in file.							*/

	if ((fd = open(WTMP_FILE, O_WRONLY | O_APPEND)) != -1) {
		(void) write(fd, u, sizeof(*u));
		(void) close(fd);
	}
	endutent ();
	return;
}

/*
 * Change utmp file entry to "dead".
 * Make entry in ct log.
 */

static void
stopat (num)
char   *num;
{
	struct utmp utmpbuf;
	register struct utmp   *u;
	int fd;
	FILE * fp;

/*	Set up the prototype for the utmp structure we want to write.	*/

	setutent();
	u = &utmpbuf;
	zero (&u -> ut_user[0], sizeof (u -> ut_user));
	zero (&u -> ut_line[0], sizeof (u -> ut_line));

/*	Fill in the various fields of the utmp structure.		*/

	u -> ut_id[0] = 'c';
	u -> ut_id[1] = 't';
	u -> ut_id[2] = _Tty[strlen(_Tty)-2];
	u -> ut_id[3] = _Tty[strlen(_Tty)-1];
	u -> ut_pid = (pid_t) _Pid;
	u -> ut_type = USER_PROCESS;

/*	Find the old entry in the utmp file with the user name and	*/
/*	copy it back.							*/

	if (u = getutid (u)) {
		utmpbuf = *u;
		u = &utmpbuf;
	}

	u -> ut_exit.e_termination = _Status & 0xff;
	u -> ut_exit.e_exit = (_Status >> 8) & 0xff;
	u -> ut_type = DEAD_PROCESS;
	time (&u -> ut_time);

/*	Write out the updated entry to utmp file.			*/

	pututline (u);

/*	Now attempt to add to the end of the wtmp file.  Do not create	*/
/*	if it doesn't already exist. Do not overwrite any info already	*/
/*	in file.							*/

	if ((fd = open(WTMP_FILE, O_WRONLY | O_APPEND)) != -1) {
		(void) write(fd, u, sizeof(*u));
		(void) close(fd);
	}
	endutent ();

/*	Do the log accounting 					*/

	if (exists (LOG) && (fp = fopen (LOG, "a")) != NULL) {
		char   *aptr;
		int     hrs,
		        mins,
		        secs;

 	/* ignore user set TZ for logfile purposes */
		if ( (aptr = getenv ("TZ")) != NULL )
			*aptr = '\0';

		(aptr = ctime (&_Log_on))[16] = '\0';
		hrs = _Log_elpsd / 3600;
		mins = (_Log_elpsd %= 3600) / 60;
		secs = _Log_elpsd % 60;
		(void) fprintf(fp, "%-8s ", getpwuid (getuid ()) -> pw_name);
		(void) fprintf(fp, "(%4s)  %s ", char_speed, aptr);
		if (hrs)
		    (void) fprintf(fp, "%2d:%.2d", hrs, mins);
		else
		    (void) fprintf(fp, "   %2d", mins);
		(void) fprintf(fp, ":%.2d  %s\n", secs, num);
		(void) fclose (fp);
	}
	return;
}

static int
exists (file)
char   *file;
{
	struct stat statb;

	if (stat (file, &statb) == -1 && errno == ENOENT)
		return (0);
	return (1);
}

static void
zero (adr, size)
register char  *adr;
register int    size;
{
	while (size--)
		*adr++ = '\0';
	return;
}

#ident  "@(#)in.login.c	1.12"
#ident  "$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1987, 1988 Microsoft Corporation */
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 * Command:	in.login
 *
 * * Usage: in.login -H [[-p] [-v level] [-h level] name [env-var ... ]]
 * Usage: in.login -R [ -L ] name
 * Usage NOTE:	-H replaces the login SVR4.0 -h option (telnet), and
 *		-R replaces the login SVR4.0 -r option (rlogin).
 *		-L causes ruserok() to skip the .rhosts check
 *
 * Level:	SYS_PRIVATE
 *
 * The in.login.c source is based on login:login.c but is not designed
 * to run in an LPM environment.  It is assumed that the inheritable
 * privilege set from UNIX SVR4.1ES is sufficient.
 *
 * Inheritable Privileges:	P_AUDIT,P_DACWRITE,P_MACWRITE,P_SETUID,P_DEV
 *				P_MACREAD,P_DACREAD,P_SYSOPS,P_SETFLEVEL,
 *				P_OWNER,P_SETPLEVEL
 *
 *       Fixed Privileges:	None
 *
 * Files:	/etc/utmp
 *		/etc/wtmp
 * 		/etc/dialups
 *		/etc/d_passwd
 *	 	/var/adm/lastlog
 *		/var/adm/loginlog
 *		/etc/default/login
 *		/etc/security/ia/index
 *		/etc/security/ia/master
 *
 * Notes:	Conditional assemblies:
 *
 *		NO_MAIL	causes the MAIL environment variable not to be set
 *
 *		LIMITED calls the "limit_user" routine to determine if the
 *			maximum number of allowable users has been reached.
 *		REMOTE_LOGIN code regions should be the only difference
 *			between this program and login:login.c.  The actual
 *			change in text may include { brackets } and
 *			whitespace.  The intent is to provide an alternate
 *			flow of control in the remote login case and
 *			record the telnet terminal.  The remote user
 *			identity is also store in utmp.
 */
#define	REMOTE_LOGIN

#define NO_MAIL

/* LINTLIBRARY */
#include <sys/types.h>
#include <utmpx.h>
#include <signal.h>
#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>			/* For logfile locking */
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <utime.h> 
#include <termio.h>
#include <sys/stropts.h>
#include <shadow.h>			/* shadow password header file */
#include <time.h>
#include <sys/param.h> 
#include <sys/fcntl.h>
#include <deflt.h>
#include <grp.h>
#include <mac.h>
#include <ia.h>
#include <sys/vnode.h>
#include <audit.h>
#include <errno.h>
#include <lastlog.h>
#include <iaf.h>
#include <priv.h>
#include <sys/secsys.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/stream.h>
#include <sys/tp.h>
#include <limits.h>
#include <dlfcn.h>
#include "copyright.h"
#include "sl_ipc.h"

/*
 * The following defines are macros used throughout login.
 */

#define	SCPYN(a, b)		(void) strncpy((a), (b), (sizeof((a))-1))
#define	EQN(a, b)		(!strncmp((a), (b), strlen((a))-1))
#define	ENVSTRNCAT(to, from)	{int deflen; deflen = strlen(to);\
				(void) strncpy((to) + deflen, (from),\
				 sizeof(to) - (1 + deflen));}
/*
 * The following defines are for different files.
 */

#define	SHELL		"/usr/bin/sh"
#define	SHELL2		"/sbin/sh"
#define	LASTLOG		"/var/adm/lastlog"
#define	LOGINLOG	"/var/adm/loginlog"	/* login log file */
#define	DIAL_FILE	"/etc/dialups"
#define	DPASS_FILE	"/etc/d_passwd"

/*
 * The following defines are for MAXIMUM values.
 */

#define	MAXLINE		      256
#define	MAXTIME		       60	/* default */
#define	MAXTRYS		        5	/* default */
#define	MAXARGS		       63
#define	MAX_TIMEOUT	(15 * 60)
#define	MAX_FAILURES 	       20	/* MAX value LOGFAILURES */
#define	MAX_DELAY	       10

/*
 * The following defines are for DEFAULT values.
 */

#define	DEF_TZ				      "EST5EDT"
#define	DEF_HZ				          "100"
#define	DEFUMASK				   077
#define	DEF_PATH			     "/usr/bin"
#define	DEF_SUPATH	"/sbin:/usr/sbin:/usr/bin:/etc"
#define	DEF_TIMEOUT				    60
#define	DEF_DELAY				     0	/* sleep before exit */

/*
 * The following defines don't fit into the MAXIMUM or DEFAULT
 * categories listed above.
 */

#define	PBUFSIZE	   128	/* max significant chars in a password */
#define	SLEEPTIME	   1	/* sleeptime before login incorrect msg */
#define	LNAME_SIZE	  32	/* size of logname */
#define	TTYN_SIZE	  15	/* size of logged tty name */
#define	TIME_SIZE	  30	/* size of logged time string */
#define	L_WAITTIME	   5	/* waittime for log file to unlock */
#define	DISABLETIME	  20	/* Seconds login disabled after LOGFAILURES or 
				   MAXTRYS unsuccesful attempts. */
#define	LOGFAILURES	   5 	/* default */

#define	ENT_SIZE	  (LNAME_SIZE + TTYN_SIZE + TIME_SIZE + 3)

#define UFAIL	7

/*
 *      The following defines are for Single Login
 */

#define SYS_SLOGIN      "/etc/.slogin"  /* system single login file */
#define USER_SLOGIN     "/.slogin"      /* user single login file */

extern	void	free(),
		setbuf(),
		*malloc();

extern	int 	errno,
		atoi(),
		optind,
		atexit(),
		getopt(),
		putenv(),
		islower(),
		isupper(),
		lvlproc(),
		fdevstat(),
		auditctl(),
		auditdmp(),
		auditevt(),
		isastream(),
		limit_user(),
		tp_fgetinf();

extern	long	atol(),
		wait();

extern  FILE	*defopen();

extern  char	*bigcrypt(),
		*getava(),
		*getenv(),
		*strdup(),
		*strchr(),
		*strcat(),
		*ttyname(),
		*defread(),
		*basename(),
		*sttyname(),
		*argvtostr(),
		**strtoargv();

extern	struct	utmpx	*getutxent(),
			*pututxline();

extern	time_t	time();

int nischeck(char *,char *);

static	FILE	*err_iop = stderr;

static	char	**getargs(),
		**chk_args(),
		*get_passwd(),
		*fget_passwd(),
		*findttyname(),
		*findrttyname();

static	char	Xtty[25],
		Xpasswd[128],
		Xstr[MAXLINE],
		term[256] = {""},
		u_name[LNAME_SIZE],
		hertz[10] = { "HZ=" },
		timez[100] = { "TZ=" },
		path[256] = { "PATH=" },
		home[256] = { "HOME=" },
		shell[256] = { "SHELL=" },
		def_lvl[LVL_MAXNAMELEN + 1],
		usr_lvl[LVL_MAXNAMELEN + 1],
		*BINPASSWD = "/usr/bin/passwd",
		logname[LNAME_SIZE + 8] = {"LOGNAME="},
		xlogname[LNAME_SIZE + 9] = {"XLOGNAME="},
#ifndef	NO_MAIL
		mail[LNAME_SIZE + 15] = { "MAIL=/var/mail/" },
#endif
		*envinit[9 + MAXARGS] = {home, path, logname, hertz, timez, term, 0, 0};

static const char *incorrectmsg = "Login incorrect\n",
		*incorrectmsgid = ":309",
		*truncatedwarning = "Your password was truncated to 8 \
characters during an operating system upgrade. Please re-enter it for \
conversion to the new long password standard.\n",
        *truncatedwarningid = ":1093",
        *notmatched = "The Passwords did not match please re-enter.\n",
        *notmatchedid = ":1094",
        *repeatpassword = "Repeat ",
        *repeatpasswordid = ":1095",
		*unexpectedfail = ":Unexpected failure. Password file(s) unchanged.",
        *unexpectedfailid = ":1096";



static	char 	*ttyn		= NULL,
		*rttyn		= NULL,
	 	*Def_tz		= NULL,
		*Console	= NULL,
		*Users		= NULL,
		*Passreq	= NULL,
		*Altshell	= NULL,
		*Mandpass	= NULL,
		*opt_fpm	= NULL,
		*Def_path	= NULL,
		*Def_term	= NULL,
	 	*Def_hertz	= NULL,
		*Def_supath	= NULL;

static	unsigned Def_timeout	= DEF_TIMEOUT,
		 Def_exit_time	= DEF_DELAY;

static	mode_t	Umask		= DEFUMASK;

static	long	Def_ulimit	= 0,
		Def_maxtrys	= MAXTRYS,
		Def_slptime	= SLEEPTIME,
		Def_distime	= DISABLETIME,
		Def_failures	= LOGFAILURES;

static	int	gpass(),
		quotec(),
		prt_alt(),
		dialpass(),
		ck_xlogin(),
		exec_pass(),
		read_pass(),
		on_console(),
		do_lastlog(),
		get_options(),
		init_badtry(),
		legalenvvar(),
		no_utmp_entry(),
		verify_macinfo();

static	long	get_logoffval();

static	int	pflag		=  0,
		hflag		=  0,
		vflag		=  0,
		intrupt		=  0,
		ret			=  0,
#ifndef	REMOTE_LOGIN
	/***Remote Login Zone: Never assign graphics to true***/
		graphics	=  0,
#else
#define	graphics	(0)
#endif
		Idleweeks	= -1;

	int	uinfo_open = 0;

static	void	usage(),
		catch(),
		do_ava(),
		do_put(),
		execute(),
		exitcmd(),
		pr_msgs(),
		adumprec(),
		badlogin(),
		donothing(),
		logbadtry(),
		do_exit(),
		verify_pwd(),
		update_utmp(),
		setup_environ(),
		init_defaults(),
		uppercaseterm();

static	uinfo_t	uinfo, s_uinfo;

static	uid_t	ia_uid;
static	gid_t	ia_gid;

static struct lastlog ll;

static	level_t *ia_lvlp,
		lvlv, lvlh,
		o_def_level,
		level		= 0,
		proc_lid	= 0,
		max_level	= 0,
		def_lvllow	= 0,
		def_lvlhigh	= 0;

static	actl_t	actl;

/*
 *      The following defines are for Single Login
 */

static SL_USER_INFO_T   SLUserInfo;
static char             slogin_config [1024];   /* user .login file path */

/*
 *      and a prototype
 */

static int       chk_slogin_active ( void );


#ifdef REMOTE_LOGIN
	/***Remote Login Zone: globals***/
	/* The Inet remote login applications
	* /usr/sbin/in.rlogind and /usr/sbin/in.telnetd
	* invoke the in.login (NOT login) scheme.  The programs
	* are so similar that they are based on the same program
	* but conditionally compiled based on the REMOTE_LOGIN macro.
	*
	* The remote login program in.rlogind invokes in.login
	*	with the -R option (This was -r in SVR4.0),
	* the telnet program in.telnetd invokes in.login
	*	with the -H option (This was -h in SVR4.0).
	*
	* These variables only used from the in.login scheme REMOTE_LOGIN
	* sections.
	*/
static	int	Hflag_telnetd = 0,	/* telnetd identification flag */
		Rflag_rlogind = 0,	/* rlogind identification flag */
		valid_luser = 0,	/* passwd entry for user rlogin */
		rlogin_firstime = 1,	/* first time through rlogin loop */
		usererr = -1;		/* when not -1, rlogin validated */

	struct	utmpx	utmpx_used_for_NMAX_macro;	/* for NMAX */
#define NMAX	sizeof(utmpx_used_for_NMAX_macro.ut_user)
static	char	rusername[NMAX+1],	/* remote user's login name */
		lusername[NMAX+1],	/* local  user's login name */
		terminal[256],		/* Remote $TERM value */
		*zero = (char *)0;	/* for pointer-to-pointer-to-null */
#endif  /* REMOTE_LOGIN */

/*
 * Procedure:	main
 */
main(argc, argv)
	char	**argv;
{
	register uid_t	priv_uid;
	register int	mac = 0,
			trys = 0,		/* value for login attempts */
			lim_user,
#ifndef	REMOTE_LOGIN
	/***Remote Login Zone: Never assign clist to true***/
			clist = 0,
#else
#define	clist	(0)
#endif
			lastlogok,
			writelog = 0,
			firstime = 1,
			login_cmd = 0;

	struct	utmpx	utmp;

	char	**envp,
		*ia_dirp,
		*ia_shellp,
		*pshell = NULL,
		*ttyprompt = NULL,
		*pwdmsgid = ":308",
		*pwdmsg = "Password:",
		inputline[MAXLINE],
		*log_entry[MAX_FAILURES],
		*loginmsg = "login: ",
		*loginmsgid = ":307",
		*limusermsg = ":784:This system has been configured to support %d users.\nThere are currently this many users logged in.\n\nPlease try again later.\n";

	long	ia_expire,
		log_attempts = 0;
	int	log_trys = 0,		/* value for writing to logfile */
		nopassword = 1;

	struct	spwd	noupass = { "", "no:password" };

	(void) atexit(do_exit);

	errno = 0;

	Xpasswd[0] = '\0';
	inputline[0] = '\0';

	envp = (char **)NULL;

	u_name[0] = utmp.ut_user[0] = '\0';

	/*
	 * Ignore the quit and interrupt signals early so
	 * no strange interrupts can occur.
	 */
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel("UX:in.login");

#ifdef REMOTE_LOGIN
	/***Remote Login Zone: Level Change***/
	/* If we are kicked off from the inetd daemon running at
	 * level USER_LOGIN, we need to change level to SYS_PRIVATE
	 * Allow the lvlproc to return -1 if ENOPKG == errno
	*/
	{
		level_t	lvl_proc_ret, sys_level, get_level, user_level;

		lvl_proc_ret = lvlproc(MAC_GET, &get_level);

		if (	(0 == lvl_proc_ret) &&
			(0 == lvlin(SYS_PRIVATE, &sys_level)) &&
			(0 == lvlin(USER_LOGIN,  &user_level)) &&
			(user_level != sys_level)	) {
			if ((user_level == get_level) &&
				(0 != lvlproc(MAC_SET, &sys_level)) ) {
				exit(3);
			}
		} else {
			if (!((-1 == lvl_proc_ret) && (ENOPKG == errno)))
				exit(2);
		}
	}
#endif  /* REMOTE_LOGIN */

	tzset();

	/*
	 * These variables may be set if MAC is installed.  They
	 * need to be initialized to 0 in case the routine
	 * "adumprec()" is called before they can be set so they
	 * don't contain garbage data.
	 */
 	o_def_level = lvlv = lvlh = 0;

	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) < 0) {
		actl.auditon = 0;
		if (errno != ENOPKG) {
			exit(1);
		}
	} else {
		/*
		 * Make this process exempt from auditing,
		 * and enable flag for setting audit event masks.
		 */
		(void) auditevt(ANAUDIT, NULL, sizeof(aevt_t));
		actl.auditon = 1;
	}

	/*
	 * Determine if this command was executed as "login" instead
	 * of being invoked as the I&A scheme.
	 */
#ifdef REMOTE_LOGIN
	/* The usr.sbin.mk file defines INSTALLPATH as the location
	 * that this binary is installed.  If an application intends
	 * to port this internal interface to in.telnetd and in.rlogind
	 * it may need to adjust this INSTALLPATH concept.
	 */
	if (strcmp(argv[0], INSTALLPATH) != 0) {
		char buffer[256];

		sprintf(&buffer[0], ":894:Must execute from %s.\n",
			INSTALLPATH);
		/*
		 * If not the "scheme", don't allow it.
		 */
		pfmt(err_iop, MM_ERROR, &buffer[0]);
		adumprec(ADT_LOGIN, 1, ADT_LOGINMSG);
		exit(1);
	}
#else
	if (strcmp(basename(argv[0]), "login") == 0) {
		/*
		 * If it is the "login" command, check if
		 * it was called in a sequence similar to:
		 *
		 *	init->getty->login
		 *
		 * or from the command line in the form:
		 *
		 *	$ exec login
		 *
		 * If not, don't allow it.
		 */
		if (no_utmp_entry()) {
			pfmt(err_iop, MM_ERROR,
				":314:You must exec \"login\" from the lowest level \"sh\".\n");
			adumprec(ADT_LOGIN, 1, ADT_LOGINMSG);
			exit(1);
		}
		/*
		 * Indicate that this is, in fact, the "login" command.
		 */
		login_cmd = 1;
	}
#endif  /* REMOTE_LOGIN */

	/*
	 * Call the secsys system call to see if we are running
	 * with a privilege mechanism that supports ID-based
	 * functionality.  If so, "priv_uid" will be >= 0,
	 * otherwise -1.
	 */
	priv_uid = (uid_t) secsys(ES_PRVID, 0);

	/* Check if MAC is installed */

	if (lvlproc(MAC_GET, &proc_lid) == 0) {
		mac = 1;
		(void) lvlin("SYS_RANGE_MAX", &max_level);
	}
	else {
		proc_lid = 0;
		if (errno != ENOPKG) {
			exit(1);
		}
	}

	init_defaults(mac);

#ifdef REMOTE_LOGIN
	/***Remote Login Zone: Not graphical port monitor***/
	/*
	 * Call ck_xlogin() to determine if login was called by the
	 * graphical port monitor.  If so, then abort.  
	 * If an application intends
	 * to port this internal interface to in.telnetd and in.rlogind
	 * it may need to adjust this ck_xlogin concept.
	 */

	if (ck_xlogin()) {
		pfmt(err_iop, MM_ERROR,
		     ":893:\"in.login\" must not be used from the GUI.\n");
		adumprec(ADT_LOGIN, 1, ADT_LOGINMSG);
		exit(1);
	}
#else
	/*
	 * Call ck_xlogin() to determine if login was called by the
	 * graphical port monitor.  If so, then all communication
	 * via stdin, stdout, and stderr is avoided.
	 */
	graphics = ck_xlogin();
#endif  /* REMOTE_LOGIN */

	/*
	 * Set the alarm to timeout in Def_timeout seconds if
	 * the user doesn't respond.  Also, set process priority.
	 */
	if (!graphics) {
		(void) sigset(SIGALRM, exitcmd);
		(void) alarm(Def_timeout);
	}
	(void) nice(0);

	if (!graphics && get_options(argc, argv) == -1) {
		usage(mac);
		adumprec(ADT_LOGIN, 1, ADT_LOGINMSG);
		exit(1);
	}
	/*
	 * If devicename is not passed as argument, call findttyname(0)
	 * and findrttyname(0).
	 */
	if (!graphics) {
		if (ttyn == NULL) {
			if (mac) {
				/*
				 * Temporarily set the level of this process to
				 * SYS_RANGE_MAX so the login scheme can determine
				 * the name of any device in the "/dev" directory.
				 */
				(void) lvlproc(MAC_SET, &max_level);
			}
			ttyn = findttyname(0);
			if (ttyn == NULL)
				ttyn = "/dev/???";
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: We have no Trusted Path Data Channels***/
			rttyn = ttyn;
#else
			if (strcmp(ttyn, "/dev/console") == 0) {
				rttyn = ttyn;
			}
			else {
				rttyn = findrttyname(0);
				if (rttyn == NULL)
					rttyn = "/dev/???";
			}
#endif  /* REMOTE_LOGIN */
			if (mac)
				(void) lvlproc(MAC_SET, &proc_lid);
		}
		else
			rttyn = ttyn;
	}
	else {
		ttyn = Xtty;
		rttyn = Xtty;
	}

	writelog = init_badtry(log_entry);

	/*
	 * Determine the number of login attempts to allow.
	 * A value of 0 is infinite.
	 */
	log_attempts = get_logoffval();

#ifdef REMOTE_LOGIN
	/***Remote Login Zone: doremotelogin or get TERM cmdline arg***/
	/* /usr/sbin/in.rlogind and /usr/sbin/in.telnetd remote login
	*  stores the user@machine-name for later identification.
	*  Also do remote login validation.
	*/
	if (Rflag_rlogind) {
		/* Determine that this remote login can procede */
		usererr = doremotelogin(argv[optind]);
		if ((usererr != -1) && !valid_luser)
			usererr = -1;
		SCPYN(utmp.ut_host, argv[optind++]);
		/* Store the remote location and uid */
		doremoteterm(terminal);
	}
	else if (Hflag_telnetd) {
		/* Store the remote location and uid */
		SCPYN(utmp.ut_host, argv[optind++]);
		if (argv[optind][0] != '-')
			SCPYN(terminal, argv[optind]);
		optind++;
	}
#endif  /* REMOTE_LOGIN */

	/*
	 * Get the prompt set by the character port monitor.
	 */
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: rlogin no prompt for input***/
	/* In the verified remote login case,
	* the password should not be set since
	* no passwd was gotten.
	*
	* in the in.login case, "graphics" is never true, and
	* since the source files are so tightly coupled we can
	* substitute that test for it.
	*/
	if ((!Rflag_rlogind) || (Rflag_rlogind && (usererr == -1))) {
	/***indented and added { } - macro to control flow***/
#else
	if (!graphics) {
#endif  /* REMOTE_LOGIN */
		ttyprompt = getenv("TTYPROMPT");
		if ((ttyprompt != NULL) && (*ttyprompt != '\0')) {
			/*
			 * If ttyprompt is set, there should be data on
			 * the stream already. 
			 */
			if ((envp = getargs(inputline)) != (char**)NULL) {
				/*
				 * Call chk_args to process options.
				 */
				envp = chk_args(envp, mac);
				uppercaseterm(*envp);
				SCPYN(utmp.ut_user, *envp);
				SCPYN(u_name, *envp++);
			}
		}
		else if (optind < argc) {
			SCPYN(utmp.ut_user, argv[optind]);
			SCPYN(u_name, argv[optind]);
			(void) strcpy(inputline, u_name);
			(void) strcat(inputline, "   \n");
			envp = &argv[optind + 1];
		}
	}
#ifndef REMOTE_LOGIN
	/***Remote Login Zone: don't do !graphics else case***/
	else {
		envp = strtoargv(Xstr);
		/*
		 * Call chk_args to process options.
		 */
		envp = chk_args(envp, mac);
		uppercaseterm(*envp);
		SCPYN(utmp.ut_user, *envp);
		SCPYN(u_name, *envp++);
		ENVSTRNCAT(xlogname, u_name);
	}
#endif  /* REMOTE_LOGIN */
	/*
	 * Enter an infinite loop.  This loop will terminate on one of
	 * three conditions:
	 *
	 *	1) a successful login,
	 *
	 *	2) number of failed login attempts is greater than log_attempts,
	 *
	 *	3) an error occured and the loop exits.
	 *
	 *
	 */
	for (;;) {
		/*
		 * Reset/clear uid, gid and level for audit.
		 */
		level = 0;
		ia_uid = ia_gid = -1;
		/*
		 * Free the storage for the master file
		 * information if it was previously allocated.
		 */
		if (uinfo_open) {
			uinfo_open = 0;
			ia_closeinfo(uinfo);
		}
		if ((pshell != NULL) && (*pshell != '\0')) {
			free(pshell);
			pshell = NULL;
		}
		/*
		 * If logging is turned on and there is an unsuccessful
		 * login attempt, put it in the string storage area.
		 */
	
		if (writelog && (Def_failures > 0)) {
			logbadtry(log_trys, log_entry);
			if (log_trys == Def_failures) {
				/*
				 * write "log_trys" number of records out
				 * to the log file and reset log_trys to 1.
				 */
				badlogin(log_trys, log_entry);
				log_trys = 1;
			}
			else {
				++log_trys;
			}
		}
                /*
                 *      clean up single login items
                 */

                SLUserInfo.userName [ 0 ] = '\0';       /* no name */
                SLUserInfo.password [ 0 ] = '\0';       /* no password */

		/*
		 * Don't do this the first time through.  Do it EVERY
		 * time after that, though.
		 */
		if (!firstime) {
			u_name[0] = utmp.ut_user[0] = '\0';
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: invalidate usererr***/
	/* On the first pass through, we did the rhost & equiv processing.
	 * If this is not the first pass, that information is stale.
	 */
			usererr = -1;
#endif  /* REMOTE_LOGIN */
		}
		firstime = 0;

		if (!graphics)
			(void) fflush(stdout);
		/*
		 * One of the loop terminators.  If either of these
		 * conditions exists, exit when "trys" is greater
		 * than log_attempts and Def_maxtrys isn't 0.
		 */
		if (log_attempts && Def_maxtrys) {
			if (++trys > log_attempts) {
				/*
				 * If logging is turned on, output the string
			 	 * storage area to the log file, and sleep for
			 	 * DISABLETIME seconds before exiting.
				 */
				if (log_trys) {
					badlogin(log_trys, log_entry);
				}
				(void) sleep(Def_distime);
				adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
				exit(1);
			}
		}
		/*
		 * Keep prompting until the user enters something.
		 */
		if (!graphics) {
			while (utmp.ut_user[0] == '\0') {
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: rlogin set local user name***/
	/* This conditional decides weather to "login:" prompt
	 * on the network.  If this is not the first rlogin request
	 * for a valid user, we must prompt.
	 * Since rlogin_firstime is a "Remote Login" defined variable,
	 * we must invalidate it in the next zone.
	 */
			if (Rflag_rlogind && lusername[0] && 
			    rlogin_firstime) {
				SCPYN(utmp.ut_user, lusername);
				SCPYN(u_name, lusername);
				envp = &zero;
				rlogin_firstime = 0;
			} else
	/***indented and added { } - macro to control flow***/
#endif  /* REMOTE_LOGIN */
				{
					/*
					 * If TTYPROMPT is not set, print out our own
					 * prompt otherwise, print out ttyprompt.
					 */
					if ((ttyprompt == NULL) || (*ttyprompt == '\0')) {
						/* do not translate login 
						 * prompt because uucp scripts
						 * depend on it
						pfmt(stdout, MM_NOSTD|MM_NOGET,
							gettxt(loginmsgid, loginmsg));
						 */
						(void)fputs(loginmsg, stdout);
					}
					else
						(void)fputs(ttyprompt, stdout);
					(void) fflush(stdout);
					if ((envp = getargs(inputline)) != (char**)NULL) {
						envp = chk_args(envp, mac);
						SCPYN(utmp.ut_user, *envp);
						SCPYN(u_name, *envp++);
					}
				}
			}
			/*
			 * If any of the common login messages was the input, we must be
			 * looking at a tty that is running login.  We exit because
			 * they will chat at each other until one times out.
			 */
			if (EQN(loginmsg, inputline) || EQN(pwdmsg, inputline) ||
					EQN(incorrectmsg, inputline)) {
				pfmt(err_iop, MM_ERROR, ":311:Looking at a login line.\n");
				adumprec(ADT_LOGIN, 8, ADT_LOGINMSG);
				exit(8);
			}
		}

                /*
                 *      save the user name away
                 */
                strcpy( SLUserInfo.userName, u_name );

		if (ia_openinfo(u_name, &uinfo) || (uinfo == NULL)) {
			adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: ia_openinfo passwd control***/
		/* In all but the verified remote login case,
		* the password should be gotten
		*/
		if ((!Rflag_rlogind) || (Rflag_rlogind && (usererr == -1)))
	/***indented and added { } - macro to control flow***/
#endif  /* REMOTE_LOGIN */
			{
			if (!graphics) {
				/* do not translate password prompt because 
				 * uucp scripts depend on it.
				(void) gpass(gettxt(pwdmsgid, pwdmsg), noupass.sp_pwdp, priv_uid, 1);
				 */
				ret = gpass(pwdmsg, noupass.sp_pwdp, priv_uid, 1);
				(void) dialpass("/sbin/sh", priv_uid);
				continue;
			}
			else
				exit(1);
			}
		}
		/*
		 * Set "uinfo_open" to 1 to indicate that the information
		 * from the master file needs to be freed if we go back to
		 * the top of the loop, or we exit.
		 */
		uinfo_open = 1;

		ia_get_sh(uinfo, &ia_shellp);
		pshell = strcpy((char *)malloc((unsigned int)
				(strlen(ia_shellp) + 1)), ia_shellp);
		/*
		 * Save a copy of the pointer if -v is given
	 	 * we have to update the master and level file.
		 */
		if (vflag)
			s_uinfo = uinfo;
	
		/*
		 * Get uid and gid info early for AUDIT.
		 */
		ia_get_uid(uinfo, &ia_uid);
		ia_get_gid(uinfo, &ia_gid);

		/*
		 * If this privilege mechanism supports ID-based functionality
		 * and the user has the privileged ID but is NOT on the system
		 * console, exit!
		 */
		if (!graphics && (priv_uid >= 0) && !on_console(priv_uid)) {
			exit(10);
		}

		/*
		 * Get the user's password.
		 */
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: main() passwd access***/
		/* In all but the verified remote login case,
		* get the user's password
		* Since rlogin_firstime is a "Remote Login" defined variable,
		* we had to invalidate it in this zone since it was used
		* in the previous one.
		*/
		rlogin_firstime = 0;
		if ((!Rflag_rlogind) || (Rflag_rlogind && (usererr == -1)))
	/***indented - macro to control flow***/
#endif  /* REMOTE_LOGIN */
			if (read_pass(priv_uid, &nopassword)) {
				if (!graphics)
					(void) dialpass(pshell, priv_uid);
				continue;
			}
	
		/*
		 * Get dialup password, if necessary.
		 */
		if (!graphics && dialpass(pshell, priv_uid))
			continue;

		if (mac) {
			if (!verify_macinfo()) {
				/* could not verify MAC info */
				if (graphics) {
					adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
					exit(1);
				}
				else {
					continue;
				}
			}
		}
		/*
		 * Check for login expiration.
		 */
		ia_get_logexpire(uinfo, &ia_expire);
		if (ia_expire > 0) {
			if (ia_expire < DAY_NOW) {
				if (graphics) {
					adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
					exit(3);
				}
				pfmt(err_iop, MM_ERROR|MM_NOGET,
					gettxt(incorrectmsgid, incorrectmsg));
				adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
				exit(1);
			}
		}
		/*
	 	 * Get the information for the last time this user logged
	 	 * in, and set up the information to be recorded for this
	 	 * session.
		 */
		lastlogok = do_lastlog(mac, ia_uid, &utmp);

		ia_get_dir(uinfo, &ia_dirp);
                /*
                 *      create directory for single login
                 */
                if ( ia_dirp )
                        sprintf ( slogin_config,"%s%s",ia_dirp,USER_SLOGIN );
                else
                        sprintf ( slogin_config,"%s",USER_SLOGIN );

		break;		/* break out of loop */
	}			/* end of infinite loop */
#ifdef LIMITED

/* THIS CODE SEGMENT TO BE REPLACED BY A NEW LICENSING SCHEME FOR ESMP */
	/*
	 * If you have a limited binary license, then
	 * we only allow a LIMITED number of concurrent users.
	 */
	if ((lim_user = limit_user(pshell, ia_uid, priv_uid)) > 0) {
		pfmt(err_iop, MM_ERROR, limusermsg, lim_user);
		adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
		exit(1);
	}
#endif
	/*
	 * Check if the password has expired, the user wants to
	 * change password, etc.
	 */
	verify_pwd(nopassword, priv_uid);

	/*
	 * Update the utmp and wtmp file entries.
	 */
	update_utmp(&utmp, login_cmd);

	/*
	 * This routine sets up the basic environment.
	 */
	setup_environ(envp, ia_dirp, pshell);

	/*
	 * If the named file descriptor is a stream, send the appropriate
	 * information back up the stream to the process that called this
	 * command.
	 */
	if (isastream(0))
		do_ava(mac, actl.auditon);
	else {
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: Fail SVR4 STREAMS networking assertion!***/
		pfmt(err_iop, MM_ERROR,
		     ":895:Networking STREAMS detection failure.\n");
		adumprec(ADT_LOGIN, 1, ADT_LOGINMSG);
		exit(1);
#else
		clist = 1;
#endif  /* REMOTE_LOGIN */
	}
	/*
	 * Print advisory messages such as the Copyright messages,
	 * level name (if MAC is installed), and last login information.
	 */
	if (!graphics)
		pr_msgs(mac, lastlogok);

        /*
         *      check and register single login
         */

#ifdef REMOTE_LOGIN
	/***Remote Login Zone: Rlogin/no passwd => no Single Login"***/
	/* In the verified remote login case,
	* the password should not be sent to
	* single login database when a passwd is not provided
	*
	* (Not_Rlogin) OR (.rhosts did not let us in)
	*/
    	if (!Rflag_rlogind || usererr == -1)
	/***indented and added { } - macro to control flow***/
#endif  /* REMOTE_LOGIN */
        if ( chk_slogin_active () ) {
                /*
                 *      single login is active -- register
                 *      with the nuc spiritual guide
                 *
                 *      ia_uid          user id
                 *      ia_gid          group id
                 *      SLUserInfo is already filled out
                 */
                SLUserInfo.uid = ia_uid;
                SLUserInfo.gid = ia_gid;
                if( SLRegisterUser( &SLUserInfo ) ){
                        /*
                         *      silently fail
                         */
                }
        }

	/*
	 * Release the information held by the different "ia_"
	 * routines since that information is no longer needed.
	 */
	ia_closeinfo(uinfo);

#ifndef REMOTE_LOGIN
	/***Remote Login Zone: No clist or login assignments.***/
	/*
	 * If either the "clist" or "login_cmd" indicator was set,
	 * then this execution should be handled as if there is no
	 * IAF facility available or desired.  This means that this
	 * process has the responsibility to change the user's ID,
	 * change to the home directory, and execute the "shell" found
	 * in the user's I&A entry.
	 *
	 * Note:	For future releases this compound "if" statement
	 *		should really be an "and" (&&) to force the use
	 *		of "ttymon" (and possibly "trusted path") with
	 *		streams.
	 */
	if (clist || login_cmd) {
		/*
		 * The "execute" routine never returns!
		 */
		execute(pshell, ia_dirp);
	}
#endif  /* REMOTE_LOGIN */
	exit(0);
	/* NOTREACHED */
}


/*
 * Procedure:	dialpass
 *
 * Notes:	Opens either the DIAL_FILE or DPASS_FILE to determine
 *		if there is a dialup password on this system.
 */
static	int
dialpass(shellp, priv_uid)
	char	*shellp;
	uid_t	priv_uid;
{
	register FILE *fp;
	char defpass[30];
	char line[80];
	register char *p1, *p2;


	if ((fp = fopen(DIAL_FILE, "r")) == NULL) {
		return 0;
	}
	while ((p1 = fgets(line, sizeof(line), fp)) != NULL) {
		while (*p1 != '\n' && *p1 != ' ' && *p1 != '\t')
			p1++;
		*p1 = '\0';
		if (strcmp(line, rttyn) == 0)
			break;
	}
	(void) fclose(fp);
	if (p1 == NULL || (fp = fopen(DPASS_FILE, "r")) == NULL) {
		return 0;
	}

	defpass[0] = '\0';
	p2 = 0;
	while ((p1 = fgets(line, sizeof(line)-1, fp)) != NULL) {
		while (*p1 && *p1 != ':')
			p1++;
		*p1++ = '\0';
		p2 = p1;
		while (*p1 && *p1 != ':')
			p1++;
		*p1 = '\0';
		if (strcmp(shellp, line) == 0)
			break;

		if (strcmp(SHELL, line) == 0)
			SCPYN(defpass, p2);
		p2 = 0;
	}
	(void) fclose(fp);
	if (!p2)
		p2 = defpass;
	if (*p2 != '\0')
		return gpass(gettxt(":332", "Dialup Password:"), p2, priv_uid, 0);
	return 0;
}


/*
 * Procedure:	gpass
 *
 * Notes:	get_passwd() fails if it cannot open /dev/tty.
 *		If this happens, and the real UID is privileged,
 *		(provided the privilege mechanism supports ID-based
 *		functionality) then use the current stdin and stderr.
 *
 *		This allows login to work with network connections
 *		and other non-ttys.
 */

/* Error code returned to xdm, when a login is attempted and an upgrade has 
 * taken place. 
 */

static	int
gpass(prmt, pswd, priv_uid, ptype)
	char *prmt, *pswd;
	uid_t	priv_uid;
{
	char *enteredpassword;
    char passwordstore1[PASS_MAX];
    char passwordstore2[PASS_MAX];
    char passwordtobeencrypted[PASS_MAX];
	char nis_passwd[14];
	char *pw;

	static  struct  index   index, *indxp = &index;
    static  struct  master  *mastp;
	int nisuser = 0;

	if (graphics) {
		enteredpassword = Xpasswd;
	}
	else if (((enteredpassword = get_passwd(prmt)) == (char *)0) && 
				(getuid() == priv_uid)) {
		enteredpassword = fget_passwd(stdin, stderr, prmt);
	}
        if ( ptype == 1)
                strcpy( SLUserInfo.password, enteredpassword );

	nisuser = nischeck(u_name,nis_passwd);

	if( ptype == 1) strcpy( SLUserInfo.password, enteredpassword );

	if (nisuser)
	{
		if (!enteredpassword || strcmp(crypt(enteredpassword, nis_passwd),
			 nis_passwd)) {
			if (!graphics) {
				(void) sleep (Def_slptime);
				pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
					incorrectmsg));
			}
			/*
			 *  invalid password -- clear the password ptr ( just in case )
			 */
			if ( ptype == 1 )
				SLUserInfo.password[0] = '\0';
			return 1;
		}
   	}
	else
	{
		if (!enteredpassword || strcmp(bigcrypt(enteredpassword, pswd),
			pswd)) {
			if (!graphics) {
				(void) sleep (Def_slptime);	
				pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
					incorrectmsg));
		/*
		 *  invalid password -- clear the password ptr ( just in case )
		 */
			if ( ptype == 1 )
				SLUserInfo.password[0] = '\0';
			return 1;
			}
		}
	}
	return 0;
}

int
nischeck(namep,nis_passwd)
char    *namep;
char	*nis_passwd;
{
	char password_file[] = "/etc/passwd";
	FILE *fd;
	struct passwd *ent = NULL;

	if ((fd = fopen (password_file, "r")) == NULL)
	{
		/* If the passwd file has dissapeared we are in a bad state. However,
		 * returning 0 will send us back throught the authentication scheme
		 * that has checked the ia database for passwords earlier.
		 */
		return(0);
	}
	
	/* fgetpwent() only reads from password file, so we know for certain that
	 * the user is local.
	 */

	while ((ent = fgetpwent(fd)) != NULL)
	{
		if (strcmp (ent->pw_name, namep) == 0)
		{
			/* Local user */
			fclose (fd);
			return(0);
		}
	}
	
	fclose (fd);

	ent = NULL;

	setpwent();

	while ((ent = getpwent()) != NULL)
	{
		if (strcmp (ent->pw_name, namep) == 0)
		{
			/* NIS user */
			(void) strncpy (nis_passwd,ent->pw_passwd,14);
			endpwent();
			return(1);
		}
	}

	ent = NULL;
	return (0); /* The default return will make us go through the long passwd
				 * check. If we get here we are dealing with an invalid userid,
				 * so who cares.
				 */
}


/*
 * Procedure:	chk_args
 *
 * Notes:	Checks the options associated with the MAC feature.
 *		The scheme fails if the information is invalid.
 */
static	char **
chk_args(pp, mac)
	char	**pp;
	int	mac;
{
	char	*p,
		*badopt = ":668:Cannot specify -%c more than once\n",
		*invalidopt = ":669:Invalid options -h, -v\n",
		*badservice = ":593:System service not installed\n";

	pflag = vflag = hflag = 0;
	def_lvl[0] = NULL;
	usr_lvl[0] = NULL;

	while (*pp) {
		p = *pp;

		if (*p++ != '-') {
			return pp;
		}
		else {
			pp++;
more:
			switch(*p++) {
			case 'v':
				if (mac) {
					if (vflag++) {
						pfmt(err_iop, MM_ERROR, badopt, 'v');
						exit(1);
					}
					if (!*p)
						p = *pp++;
					(void) strcpy(def_lvl, p);
				}
				else {
					pfmt(err_iop, MM_ERROR, invalidopt);
					pfmt(err_iop, MM_ERROR, badservice);
					exit(1);
				}
				break;
			case 'h':
				if (mac) {
					if (hflag++) {
						pfmt(err_iop, MM_ERROR, badopt, 'h');
						exit(1);
					}
					if (!*p)
						p = *pp++;
					(void) strcpy(usr_lvl, p);
				}
				else {
					pfmt(err_iop, MM_ERROR, invalidopt);
					pfmt(err_iop, MM_ERROR, badservice);
					exit(1);
				}
				break;
			case 'p':
				if (pflag++) {
					pfmt(err_iop, MM_ERROR, badopt, 'p');
					exit(1);
				}
				if (*p++) {
					pp++;
					goto more;
				}
				break;
			}
		}
	}
	return pp;
}


/*
 * Procedure:	getargs
 *
 * Notes:	Scans the data enetered at the prompt and stores the
 *		information in the argument passed.  Exits if EOF is
 *		enetered.
 */
static char **
getargs(inline)
	char	*inline;
{
	int	c,
		llen = MAXLINE - 1;
	static	char	*args[MAXARGS],	/* pointer to arguments in envbuf[] */
			envbuf[MAXLINE];/* storage for argument text */
	char	*ptr = envbuf,
		**reply = args;
	enum {
		WHITESPACE, ARGUMENT
	} state = WHITESPACE;

	while ((c = getc(stdin)) != '\n') {
		/*
		 * Check ``llen'' to avoid overflow on ``inline''.
		 */
		if (llen > 0) {
			--llen;
			/*
			 * Save a literal copy of the input in ``inline''.
			 * which is checked in main() to determine if
			 * this login process "talking" to another login
			 * process.
			 */
			*(inline++) = (char) c;
		}
		switch (c) {
		case EOF:
			/*
			 * If the user enters an EOF character, exit
			 * immediately with the value of one (1) so it
			 * doesn't appear as if this login was successful.
			 */
			exit(1);
			/* FALLTHROUGH */
		case ' ':
		case '\t':
			if (state == ARGUMENT) {
				*ptr++ = '\0';
				state = WHITESPACE;
			}
			break;
		case '\\':
			c = quotec();
			/* FALLTHROUGH */
		default:
			if (state == WHITESPACE) {
				*reply++ = ptr;
				state = ARGUMENT;
			}
			*ptr++ = (char) c;
		}
		/*
		 * Check if either the "envbuf" array or the "args"
		 * array is overflowing.
		 */
		if (ptr >= envbuf + MAXLINE - 1
			|| reply >= args + MAXARGS - 1 && state == WHITESPACE) {
				(void) putc('\n', stdout);
			break;
		}
	}
	*ptr = '\0';
	*inline = '\0';
	*reply = NULL;

	return ((reply == args) ? NULL : args);
}


/*
 * Procedure:	quotec
 *
 * Notes:	Reads from the "standard input" of the tty. It is
 *		called by the routine "getargs".
 */
static int
quotec()
{
	register int c, i, num;

	switch (c = getc(stdin)) {
	case 'n':
		c = '\n';
		break;
	case 'r':
		c = '\r';
		break;
	case 'v':
		c = '\013';
		break;
	case 'b':
		c = '\b';
		break;
	case 't':
		c = '\t';
		break;
	case 'f':
		c = '\f';
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		for (num = 0, i = 0; i < 3; i++) {
			num = num * 8 + (c - '0');
			if ((c = getc(stdin)) < '0' || c > '7')
				break;
		}
		(void) ungetc(c, stdin);
		c = num & 0377;
		break;
	default:
		break;
	}
	return c;
}


static char *illegal[] = {
		"SHELL=",
		"HOME=",
		"LOGNAME=",
#ifndef	NO_MAIL
		"MAIL=",
#endif
		"CDPATH=",
		"IFS=",
		"PATH=",
		0
};


/*
 * Procedure:	legalenvvar
 *
 * Notes:	Determines if it is legal to insert environment variable
 *		passed as an argument.
 */
static int
legalenvvar(s)
	char *s;
{
	register char **p;

	for (p = illegal; *p; p++)
		if (!strncmp(s, *p, strlen(*p)))
			return 0;
	return 1;
}


/*
 * Procedure:	badlogin
 *
 * Notes:	Log to the log file after "trys" unsuccessful attempts.
 */
static	void
badlogin(trys, log_entry)
	int	trys;
	char	**log_entry;
{
	int retval, count, fildes;

	/*
	 * Tries to open the log file. If success, lock it and write
	 * in the failed attempts.
	 */
	if ((fildes = open (LOGINLOG, O_APPEND|O_WRONLY)) == -1)
		return;
	else {
		(void) sigset(SIGALRM, donothing);
		(void) alarm(L_WAITTIME);
		retval = lockf(fildes, F_LOCK, 0L);
		(void) alarm(0);
		(void) sigset(SIGALRM, SIG_DFL);
		if (retval == 0) {
			for (count = 0 ; count < trys ; count++) {
			   (void) write(fildes, log_entry[count],
				(unsigned) strlen (log_entry[count]));
				*log_entry[count] = '\0';
			}
			(void) lockf(fildes, F_ULOCK, 0L);
			(void) close(fildes);
		}
		return;
	}
}


/*
 * Procedure:	donothing
 *
 * Notes:	Called by "badlogin" routine when SIGALRM is
 *		caught.  The intent is to do nothing when the
 *		alarm is caught.
 */
static	void
donothing() {}

/*
 * Procedure:	exitcmd
 *
 * Notes:	Called by main routine if non-graphical login times out.
 */
static  void
exitcmd()
{
	(void) alarm(0);
	(void) sigset(SIGALRM, SIG_DFL);
	exit(1);
}


/*
 * Procedure:	get_passwd
 *
 * Notes:	Calls "fget_passwd" to read the user's password entry.
 */
static	char *
get_passwd(prompt)
	char *prompt;
{
	char *p;
	FILE	*fi;

	(void) lvlproc(MAC_SET, &max_level);
	/*
	 * Open the generic tty.
	 */
	fi = fopen("/dev/tty", "r");
	(void) lvlproc(MAC_SET, &proc_lid);

	if (fi == NULL) {
		return (char*)NULL;
	}
	setbuf(fi, (char*)NULL);
	p = fget_passwd(fi, stderr, prompt);
	if (fi != stdin)
		(void) fclose(fi);
	return p;
}


/*
 * Procedure:	fget_passwd
 * 
 * Notes:	Issues the "Password: " prompt and reads the input
 *		after turning off character echoing.
 */
static	char *
fget_passwd(fi, fo, prompt)
	FILE *fi, *fo;
	char *prompt;
{
	struct termio ttyb;
	unsigned short flags;
	register char *p;
	register int c;
	static char pbuf[PBUFSIZE + 1];
	void (*sig)(), catch();

	sig = signal(SIGINT, catch);
	intrupt = 0;
	(void) ioctl(fileno(fi), TCGETA, &ttyb);
	flags = ttyb.c_lflag;
	ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	(void) ioctl(fileno(fi), TCSETAF, &ttyb);
	(void) fputs(prompt, fo);
	tcflush(fileno(fo), TCIFLUSH);
	for (p = pbuf; !intrupt && (c = getc(fi)) != '\n' && c != EOF;) {
		if (p < &pbuf[PBUFSIZE])
			*p++ = (char) c;
	}
	*p = '\0';
	(void) putc('\n', fo);
	ttyb.c_lflag = flags;
	(void) ioctl(fileno(fi), TCSETAW, &ttyb);
	(void) signal(SIGINT, sig);

	/*
	 * Security hole: at this time signal SIGINT can be on SIG_IGN !
	 * In this case kill(getpid(), SIGINT) would effect nothing and 
	 * login could continue without a valid password.
	 */
	if (intrupt)
		exit(1);

	return pbuf;
}


/*
 * Procedure:	catch
 *
 * Notes:	Called by fget_passwd if the process catches an
 *		INTERRUPT signal.
 */
static void
catch()
{
	++intrupt;
}


/*
 * Procedure:	uppercaseterm
 *
 * Notes:	If all input characters are upper case set the
 *		corresponding termio so ALL input and output is
 *		UPPER case.
 */
static	void
uppercaseterm(strp)
	char	*strp;
{
	int 	upper = 0;
	int 	lower = 0;
	char	*sp;
	struct	termio	termio;

	for (sp = strp; *sp; sp++) {
		if (islower(*sp)) 
			lower++;
		else if (isupper(*sp))
			upper++;
	}

	if (upper > 0 && lower == 0) {
		(void) ioctl(0,TCGETA,&termio);
		termio.c_iflag |= IUCLC;
		termio.c_oflag |= OLCUC;
		termio.c_lflag |= XCASE;
		(void) ioctl(0,TCSETAW,&termio);
		for (sp = strp; *sp; sp++) 
			if (*sp >= 'A' && *sp <= 'Z' ) *sp += ('a' - 'A');
	}
}


/*
 * Procedure:	findttyname
 *
 * Notes:	Call ttyname(), but do not return syscon, systty,
 *		or sysconreal do not use syscon or systty if console
 *		is present, assuming they are links.
 */
static	char *
findttyname(fd)
	int	fd;
{
	char	*lttyn;

	if ((lttyn = (ttyname(fd))) != NULL)
	   {
		if (((strcmp(lttyn, "/dev/syscon") == 0) ||
		     (strcmp(lttyn, "/dev/sysconreal") == 0) ||
		     (strcmp(lttyn, "/dev/systty") == 0)) &&
		     (access("/dev/console", F_OK) == 0))
			lttyn = "/dev/console";
	   }
	return lttyn;
}


/*
 * Procedure:	findrttyname
 *
 * Notes:	Get the real/physical tty device special file name
 *		if the given file descriptor is a Trusted Path
 *		Data channel, otherwise call findttyname().
 *		Return /dev/console if name returned is /dev/systty,
 *		/dev/syscon, or /dev/sysconreal.
 */
static	char *
findrttyname(fd)
	int	fd;
{
	char		*lttyn;
	struct tp_info	tpinf;
	struct stat	statbuf;

    
	if (tp_fgetinf(fd, &tpinf) == -1) {
		return findttyname(fd);
	}
	statbuf.st_rdev = tpinf.tpinf_rdev;
	statbuf.st_dev = tpinf.tpinf_rdevfsdev;
	statbuf.st_ino = tpinf.tpinf_rdevino;
	statbuf.st_mode = tpinf.tpinf_rdevmode;
	lttyn = sttyname(&statbuf);

	if (((strcmp(lttyn, "/dev/syscon") == 0) ||
	     (strcmp(lttyn, "/dev/sysconreal") == 0) ||
	     (strcmp(lttyn, "/dev/systty") == 0)) &&
	     (access("/dev/console", F_OK) == 0))
		lttyn = "/dev/console";

	return lttyn;
}


/*
 * Procedure:	init_defaults
 *
 * Notes:	Reads the "login" default file in "/etc/defaults"
 *		directory.  Also initializes other variables used
 *		throughout the code.
 */
static	void
init_defaults(mac)
	register int	mac;
{
	FILE *defltfp;
	register char	*ptr,
			*Pndefault = "login";

	if ((defltfp = defopen(Pndefault)) != NULL) {
		if ((Console = defread(defltfp, "CONSOLE")) != NULL)
			if (*Console)
				Console = strdup(Console);
			else
				Console = NULL;
		if ((Users = defread(defltfp, "USERS")) != NULL)
			if (*Users)
				Users = strdup(Users);
			else
				Users = NULL;
		if ((Altshell = defread(defltfp, "ALTSHELL")) != NULL)
			if (*Altshell)
				Altshell = strdup(Altshell);
			else
				Altshell = NULL;
		if ((Passreq = defread(defltfp, "PASSREQ")) != NULL)
			if (*Passreq)
				Passreq = strdup(Passreq);
			else
				Passreq = NULL;
		if ((Mandpass = defread(defltfp, "MANDPASS")) != NULL)
			if (*Mandpass)
				Mandpass = strdup(Mandpass);
			else
				Mandpass = NULL;
		if ((opt_fpm = defread(defltfp, "OPT_FPM")) != NULL)
			if (*opt_fpm)
				opt_fpm = strdup(opt_fpm);
			else	
				opt_fpm = NULL;
		if ((Def_tz = defread(defltfp, "TIMEZONE")) != NULL)
			if (*Def_tz)
				Def_tz = strdup(Def_tz);
			else
				Def_tz = NULL;
		if ((Def_hertz = defread(defltfp, "HZ")) != NULL)
			if (*Def_hertz)
				Def_hertz = strdup(Def_hertz);
			else
				Def_hertz = NULL;
		if ((Def_path = defread(defltfp, "PATH")) != NULL)
			if (*Def_path)
				Def_path = strdup(Def_path);
			else
				Def_path = NULL;
		if ((Def_supath = defread(defltfp, "SUPATH")) != NULL)
			if (*Def_supath)
				Def_supath = strdup(Def_supath);
			else
				Def_supath = NULL;

		if ((ptr = defread(defltfp, "ULIMIT")) != NULL)
		    Def_ulimit = atol(ptr);
		if ((ptr = defread(defltfp, "TIMEOUT")) != NULL)
		    Def_timeout = (unsigned) atoi(ptr);
		if ((ptr = defread(defltfp, "SLEEPTIME")) != NULL)
		    Def_slptime = (unsigned) atoi(ptr);
		if ((ptr = defread(defltfp, "DISABLETIME")) != NULL)
		    Def_distime = (unsigned) atoi(ptr);
		if ((ptr = defread(defltfp, "MAXTRYS")) != NULL)
		    Def_maxtrys = (unsigned) atoi(ptr);
		if ((ptr = defread(defltfp, "LOGFAILURES")) != NULL)
		    Def_failures = (unsigned) atoi(ptr);
		if ((ptr = defread(defltfp, "UMASK")) != NULL)
			if (sscanf(ptr, "%lo", &Umask) != 1)
				Umask = DEFUMASK;
		if ((ptr = defread(defltfp, "IDLEWEEKS")) != NULL)
			Idleweeks = atoi(ptr);
		if ((ptr = defread(defltfp, "DELAYEDEXIT")) != NULL)
			Def_exit_time = (unsigned) atoi(ptr);

		if (mac) {
			if ((ptr = defread(defltfp, "SYS_LOGIN_LOW")) != NULL) {
				if (*ptr) {
					if (lvlin(ptr, &def_lvllow) != 0)
						def_lvllow = 0;

					else if(lvlvalid(&def_lvllow))
						def_lvllow = 0;
				}
			}
			if ((ptr = defread(defltfp, "SYS_LOGIN_HIGH")) != NULL) {
				if (*ptr) {
					if (lvlin(ptr, &def_lvlhigh) != 0)
						def_lvlhigh = 0;

					else if(lvlvalid(&def_lvlhigh))
						def_lvlhigh = 0;
				}
			}
		}
		(void) defclose(defltfp);
	}

	if (Umask < (unsigned)0 || ((mode_t) 0777) < Umask)
		Umask = DEFUMASK;

	(void) umask(Umask);

	if ((!Def_tz || !*Def_tz) && ((Def_tz = getenv("TZ")) == NULL)) {
		(void) strcat(timez, DEF_TZ);
	}
	else {
		ENVSTRNCAT(timez, Def_tz);
	}
	(void) putenv(timez);

	if (Def_timeout > MAX_TIMEOUT)
		Def_timeout = MAX_TIMEOUT;
	if (Def_slptime > DEF_TIMEOUT)
		Def_slptime = DEF_TIMEOUT;
	if (Def_distime > DEF_TIMEOUT)
		Def_distime = DEF_TIMEOUT;
	if (Def_failures < 0 ) 
		Def_failures = LOGFAILURES;
	if (Def_failures > MAX_FAILURES)
		Def_failures = MAX_FAILURES;
	if (Def_maxtrys < 0 )
		Def_maxtrys = MAXTRYS;
	if (Def_exit_time > MAX_DELAY)
		Def_exit_time = MAX_DELAY;

	return;
}

/*
 * Procedure:	exec_pass
 *
 * Notes:	This routine forks, changes the uid of the forked process
 *		to the user logging in, and execs the "/usr/bin/passwd"
 *		command.  It returns the status of the "exec" to the
 *		parent process.  All "working" privileges of the forked
 *		(child) process are cleared.  Also, P_SYSOPS is cleared
 *		from the maximum set to indicate to "passwd" that this
 *		"exec" originated from the login scheme.
 */
static int
exec_pass(usernam)
	char *usernam;
{
	int	status, w;
	pid_t	pid;

	if ((pid = fork()) == 0) {
		if (setuid(MAXUID) == -1) {
			pfmt(err_iop, MM_ERROR, ":321:Bad user id.\n");
			adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
			exit(127);
		}
		(void) procprivl(CLRPRV, ALLPRIVS_W, pm_max(P_SYSOPS), 0);
		(void) execl(BINPASSWD, BINPASSWD, usernam, (char *)NULL);
		(void) procprivl(SETPRV, ALLPRIVS_W, 0);
		exit(127);
	}

	while ((w = (int) wait(&status)) != pid && w != -1)
		;
	return (w == -1) ? w : status;
}


/*
 * Procedure:	get_options
 *
 * Notes:	Get_options parses the command line.  It returns 0
 *		if successful, -1 if failed.
 */
static	int
get_options(argc, argv)
	int	argc;
	char	*argv[];
{
	int	c;
	int	errflg = 0;

#ifdef REMOTE_LOGIN
	/***Remote Login Zone: conditionally compile OPTSTRING***/
	/* the in.login scheme uses -R and -H for rlogin and telnet
	*  sessions, respectively.  In SVR4.0, they were -r and -h,
	*  respectively.
	*/
	int	Lflag = 0;

#define OPTSTRING "d:ru:l:s:M:U:S:RHL"
#else
	/* the login scheme does not use -H */
#define OPTSTRING "d:ru:l:s:M:U:S:"
#endif  /* REMOTE_LOGIN */

	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		switch (c) {
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: R and H flag***/
		case 'R':
			/* the in.login in.rlogind option flag. */
			if (Hflag_telnetd || Rflag_rlogind) {
				pfmt(stderr, MM_ERROR, ":682:Only one of -R and -H allowed\n");
				errflg++;
			}
			Rflag_rlogind++;
			break;
		case 'H':
			/* the in.login in.telnetd option flag. */
			if (Hflag_telnetd || Rflag_rlogind) {
				pfmt(stderr, MM_ERROR, ":682:Only one of -R and -H allowed\n");
				errflg++;
			}
			Hflag_telnetd++;
			break;
		case 'L': {
			extern int _check_rhosts_file;

			Lflag++;
			_check_rhosts_file = 0;
			break;
		}
#endif  /* REMOTE_LOGIN */
		/*
		 * No need to continue login since the -r option
		 * is not allowed.
		 */
		case 'r':
			return -1;
		/*
		 * The ability to specify the -d option at the "login: "
		 * prompt with an argument is still supported however it
		 * has no effect.
		 */
		case 'd':
			/* ignore the following options for IAF reqts */
		case 'u':
		case 'l':
		case 's':
		case 'M':
		case 'U':
		case 'S':
			break;
		default:
			errflg++;
			break;
		} /* end switch */
	} /* end while */
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: verify R, H, L flags***/
	if ((!Hflag_telnetd && !Rflag_rlogind) ||
	    (Lflag && !Rflag_rlogind)) {
		return -1;
	}
#endif  /* REMOTE_LOGIN */
	if (errflg)
		return -1;
	return 0;
}


/*
 * Procedure:	usage
 *
 * Notes:	Prints the usage message.
 */
static	void
usage(mac)
	int	mac;
{
	char	*u_msg = ":785:Usage: login [[ -p ] name [ env-var ... ]]\n";

#ifdef REMOTE_LOGIN
	/***Remote Login Zone: Usage Message***/
	/* in.login has -R for remote login OR -H for telnet.
	 * They were -r and -h, respectively, in SVR4.0 */
	pfmt(stderr, MM_ACTION,
		":683:Usage: in.login -R [ -L ]\nUsage: in.login -H [ -p ] [ -v def_level ] [ -h level ] [ name ] [ env-var ... ]]\n");
#else
	if (mac)
		u_msg = ":670:Usage: login [[ -p ]  [ -v def_level ] [ -h level ] name [ env-var ... ]]\n";

	pfmt(err_iop, MM_ACTION, u_msg);
#endif  /* REMOTE_LOGIN */
}


/*
 * Procedure:	adumprec
 *
 * Notes:	Only writes the record if auditing is turned on.
 *		It determines this by checking the "auditon" flag
 *		in the audit structure.
 */
static	void
adumprec(rtype, status, msg)
	int rtype;	/* event types login, bad_auth, bad_lvl and def_lvl */
	int status;	/* event exit status */
	char *msg;	/* bad_auth message */
{
        arec_t		rec;		/* auditdmp(2) structure */
        alogrec_t	alogrec;	/* login record structure */

	if (actl.auditon) {
		rec.rtype = rtype;
	        rec.rstatus = status;
	        rec.rsize = sizeof(struct alogrec);

		alogrec.uid = ia_uid;
		alogrec.gid = ia_gid;

		alogrec.ulid = o_def_level;
		alogrec.hlid = lvlh;
		alogrec.vlid = lvlv;

		(void) sprintf(alogrec.bamsg, "%s", msg);
		(void) sprintf(alogrec.tty, "%s", rttyn);

	        rec.argp = (char *)&alogrec;

	        (void) auditdmp(&rec, sizeof(arec_t));
	}
        return;
}


/*
 * Procedure:	do_lastlog
 *
 * Notes:	Gets the information for the last time the user logged
 *		on and also sets up the information for this login
 *		session so it can be reported at a subsequent login.
 */
static	int
do_lastlog(mac, uid, utmp)
	register int	mac;
	register uid_t	uid;
	register struct	utmpx	*utmp;
{
	int	fd1,
		lastlogok = 0;
	long	ia_inact;
	struct	stat	f_buf;
	struct	lastlog	newll;

	if (stat(LASTLOG, &f_buf) < 0) {
		(void) close(creat(LASTLOG, (mode_t) 0));
		(void) chmod(LASTLOG, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
	}

	if ((fd1 = open(LASTLOG, O_RDWR)) >= 0) {
		(void) lseek(fd1, (long)uid * sizeof(struct lastlog), 0);
		if (read(fd1, (char *)&ll, sizeof(ll)) == sizeof(ll) &&
			ll.ll_time != 0)
			lastlogok = 1;
		(void) lseek(fd1, (long)uid * sizeof(struct lastlog), 0);
		(void) time(&newll.ll_time);
		SCPYN(newll.ll_line, (rttyn + sizeof("/dev/")-1));
		SCPYN(newll.ll_host, utmp->ut_host);
		newll.ll_level = level;

		/* Check for login inactivity. */

		ia_get_loginact((uinfo_t)uinfo, &ia_inact);
		if ((ia_inact > 0) && ll.ll_time)
			if((( ll.ll_time / DAY ) + ia_inact) < DAY_NOW ) {
					if (graphics) {
						adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
						exit(2);
					}
	                                pfmt(err_iop, MM_ERROR|MM_NOGET,
	                                	gettxt(incorrectmsgid, incorrectmsg));
	                                (void) write(fd1, (char * )&ll, sizeof(ll));
	                                (void) close(fd1);
					adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
	                                exit(1);
			}

		(void) write(fd1, (char * )&newll, sizeof(newll));
		(void) close(fd1);
	}

	return lastlogok;
}

#define	NENVINIT	(sizeof (envinit) / sizeof (envinit[0]))

/*
 * Procedure:	setup_environ
 *
 * Notes:	Set up the basic environment for the exec.  This
 *		includes HOME, PATH, LOGNAME, SHELL, TERM, HZ, TZ,
 *		and MAIL.
 */
static	void
setup_environ(envp, dirp, shellp)
	register char	**envp,
			*dirp,
			*shellp;
{
	static int basicenv;
	register int	i, k,
			l_index, length;
	char	*ptr, *endptr, **senvp = (char **)0;
	extern char **environ;

#ifdef REMOTE_LOGIN
	/***Remote Login Zone: setup TERM with terminal***/
	if (Rflag_rlogind) {
		(void) strcat(term, "TERM=");
		ENVSTRNCAT(term, terminal);
	} else if (Hflag_telnetd) {
		if (*terminal == '\0') /* initialize for null term type */
			(void) strcat(term, "TERM=");
		ENVSTRNCAT(term, terminal);
	} else
	/***indented and added { } - macro to control flow***/
#endif	/* REMOTE_LOGIN */
	{
		/*
		 * Login will only set the environment variable "TERM" if it
		 * already exists in the environment.  This allows features
		 * such as doconfig with the port monitor to work correctly
		 * if an administrator specifies a particular terminal for a
		 * particular port.
		 *
		 * NOTE: If the TERM variable is NOT in the current environment,
		 *	 it won't be included in the basic environment.
		 */
		if ((!Def_term || !*Def_term) && ((Def_term = getenv("TERM")) != NULL)) {
			(void) strcat(term, "TERM=");
			ENVSTRNCAT(term, Def_term);
		}
	}

	if ((!Def_hertz || !*Def_hertz) && ((Def_hertz = getenv("HZ")) == NULL))
		(void) strcat(hertz, DEF_HZ);
	else			
		ENVSTRNCAT(hertz, Def_hertz);

	if (!Def_path || !*Def_path)
		(void) strcat(path, DEF_PATH);
	else
		ENVSTRNCAT(path, Def_path);

	ENVSTRNCAT(home, dirp);
	ENVSTRNCAT(logname, u_name);

	/* Find the end of the basic environment. */

	for (basicenv = 0; envinit[basicenv] != NULL; basicenv++);

	if (shellp[0] == '\0') { 
		/*
		 * If possible, use the primary default shell,
		 * otherwise, use the secondary one.
		 */
		if (access(SHELL, X_OK) == 0)
			shellp = SHELL;
		else
			shellp = SHELL2;
			
	} else 
		if (Altshell && *Altshell && (strcmp(Altshell, "YES") == 0))
			envinit[basicenv++] = shell;

	ENVSTRNCAT(shell, shellp);

#ifndef	NO_MAIL
	envinit[basicenv++] = mail;
	(void) strcat(mail,u_name);
#endif
	/*
	 * When called by telnetd add in the inherited environment first.
	 */
	if (Hflag_telnetd) {
		senvp = envp;
		envp = environ;
	}

	/*
	 * Add in all the environment variables picked up from the
	 * argument list to "login" or from the user response to the
	 * "login" request.
	 */
	k = 0;
	l_index = 0;
again:
	for (; *envp; envp++) {
		/*
		 * Scan each string provided.  If it doesn't have the format
		 * xxx=yyy, then add the string "Ln=" to the beginning.
		 */
		if ((endptr = strchr(*envp,'=')) == (char*)NULL) {
			if ((basicenv+k) < (NENVINIT-1)) {
				ptr = (char *)malloc(strlen(*envp) + 7);
				if (ptr != (char *)0) {
					envinit[basicenv+k] = ptr;
					(void) sprintf(ptr,"L%d=%s",l_index,*envp);
					k++;
					l_index++;
				}
			}
		}
		/* Is this an environmental variable we permit? */
		else if (legalenvvar(*envp)) {
			/*
			 * Check to see whether this string replaces any
			 * previously-defined string.
			 */
			for (i = 0, length = endptr+1-*envp; i < basicenv+k; i++ ) {
				if (strncmp(*envp, envinit[i], length) == 0) {
					envinit[i] = *envp;
					break;
				}
			}
			/*
			 * If it doesn't, place it at the end of
			 * the environment array.
			 */
			if ((i == basicenv+k) && ((basicenv+k) < (NENVINIT-1))){
				envinit[basicenv+k] = *envp;
				k++;
			}
		}
	}
	/*
	 * If we were processing the inherited environment, now do the
	 * environment we were called to process in the first place.
	 */
	if (senvp) {
		envp = senvp;
		senvp = (char **)0;
		goto again;
	}
}


/*
 * Procedure:	pr_msgs
 *
 * Notes:	Prints any advisory messages such as the Copyright
 *		message, level name (if MAC is installed), and last
 *		login information.
 */
static	void
pr_msgs(mac_msg, lastlog_msg)
	int	mac_msg;
	int	lastlog_msg;
{
	register int	i;
	struct utsname un;
	char	*bufp;

	(void) alarm(0);
	(void) sigset(SIGALRM, SIG_DFL);

	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGINT, SIG_DFL);
	(void) uname(&un);

	bufp = '\0';

	COPYRIGHT(un);

	/* display mac level for this login session */
	if (mac_msg) {
		bufp = (char *)malloc((unsigned int)(i = lvlout(&level, bufp,
			0, LVL_FULL)));
		if (bufp != NULL) {
			if (lvlout(&level, bufp, i, LVL_FULL) == 0)
				pfmt(stdout, MM_INFO, ":671:Current Level: %s\n",
					bufp);
		}
	}
	/*	
	 * Advise the user the time and date that this login-id
	 * was last used. 
 	 */
	if (lastlog_msg) {
		char	timebuf[256];
		size_t	timelen;
		struct	tm	*ltime;

		ltime = localtime(&ll.ll_time);
		timelen = strftime(timebuf, sizeof timebuf, (char *)0, ltime);
		if (mac_msg) {
			lvlout(&ll.ll_level, def_lvl, MAXNAMELEN, LVL_ALIAS);
			pfmt(stdout, MM_NOSTD,
				":672:Last login: %.*s on %.*s at level %s\n", timelen,
			timebuf, sizeof(ll.ll_line), ll.ll_line, def_lvl);
		} else {
			pfmt(stdout, MM_NOSTD,
				":330:Last login: %.*s on %.*s\n", timelen,
			timebuf, sizeof(ll.ll_line), ll.ll_line);
		}
	}
}


/*
 * Procedure:	update_utmp
 *
 * Notes:	Updates the utmpx and wtmpx files.
 */
static	void
update_utmp(utmp, cur_pid)
	register struct	utmpx	*utmp;
	int	cur_pid;
{
	register struct	utmpx	*u;

	(void) time(&utmp->ut_tv.tv_sec);
	if (cur_pid)
		utmp->ut_pid = getpid();
	else {
		utmp->ut_pid = getppid();
	}

	/*
	 * Find the entry for this pid in the utmp file.
	 */
	while ((u = getutxent()) != NULL) {
		if (((u->ut_type == INIT_PROCESS ||
			u->ut_type == LOGIN_PROCESS)  &&
			(u->ut_pid == utmp->ut_pid)) ||
			((u->ut_type == USER_PROCESS) &&
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: verify parent via utmp "."***/
			/* The inetd spawned in.rlogind and in.telnetd
			 * processes create utmp entries with
			 * .rlogind and .telnetd names, respectively;
			 * verify ancestor process.
			 */
			(u->ut_pid  == utmp->ut_pid) &&
			((Rflag_rlogind &&
			  !strncmp(u->ut_user, ".rlogin", sizeof(u->ut_user))) ||
			 (Hflag_telnetd &&
			  !strncmp(u->ut_user, ".telnet", sizeof(u->ut_user))) ||
			 (Hflag_telnetd &&
			  !strncmp(u->ut_user, ".nvt", sizeof(u->ut_user)))))) 
 {
#else
			(u->ut_pid  == utmp->ut_pid))) {
#endif  /* REMOTE_LOGIN */
	/*
	 * Copy in the name of the tty minus the "/dev/", the id, and set
	 * the type of entry to USER_PROCESS.
	 */
			SCPYN(utmp->ut_line,(ttyn + sizeof("/dev/")-1));
			utmp->ut_id[0] = u->ut_id[0];
			utmp->ut_id[1] = u->ut_id[1];
			utmp->ut_id[2] = u->ut_id[2];
			utmp->ut_id[3] = u->ut_id[3];
			utmp->ut_type = USER_PROCESS;

	/* Write the new updated utmp file entry. */

			pututxline(utmp);
			break;
		}
	}
	endutxent();		/* Close utmp file */

	/*
	 * Now attempt to write out this entry to the wtmp file
	 * if we were successful in getting it from the utmp file
	 * and the wtmp file exists.
	 */
	updwtmpx(WTMPX_FILE, utmp);

	return;
}


/*
 * Procedure:	verify_macinfo
 *
 * Notes:	Used to check any user supplied MAC information if
 *		MAC is installed.  If any check fails, it returns
 *		0.  On success, 1.
 */
static	int
verify_macinfo()
{
	struct	devstat	devstat;
	register int	i, good = 0;
	level_t	*lvlp, tmplvl;
	long	lvlcnt;
	char	*badlvl = ":673:Invalid default login security level specified\n",
		*busylvl = ":674:Level file busy, default level unchanged\n",
		*ng = ":675:Update of I&A files failed, default level unchanged.\n",
		*nodo = ":676:Login for %s not allowed on this terminal\n";

	if (ia_get_lvl((uinfo_t)uinfo, &ia_lvlp, &lvlcnt)) {
		pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
			incorrectmsg));
		adumprec(ADT_LOGIN, 11, ADT_LOGINMSG);
		exit(11);
	}
		
	o_def_level = *ia_lvlp;

	if (hflag) {
		if (lvlin(usr_lvl, &lvlh) != 0) {
			lvlh = 0;
			adumprec(ADT_BAD_LVL, 0, ADT_LOGINMSG);
			(void) sleep (Def_slptime) ;
			pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
				incorrectmsg));
			return 0;
		}
		lvlp = ia_lvlp;
		for (i = 0; i < lvlcnt; i++, lvlp++) {
			if (lvlequal(&lvlh, lvlp) > 0) {
				good++;
				break;
			}
		}
		if (!good) {
			adumprec(ADT_BAD_LVL, 0, ADT_LOGINMSG);
			(void) sleep (Def_slptime) ;
			pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
				incorrectmsg));
			return 0;
		}
	}
	if (vflag) {
		if (lvlin(def_lvl, &lvlv) != 0) {
			lvlv = 0;
			level = *ia_lvlp;
			adumprec(ADT_DEF_LVL, 1, ADT_LOGINMSG);
			pfmt(err_iop, MM_ERROR, badlvl);
			vflag = 0;
		}
	}

	if (hflag)
		level = lvlh;
	else
		level = *ia_lvlp;

	/*
	 * Check new default level.
	 */
	good = 0;
	if (vflag) {
		lvlp = ia_lvlp;
		for (i = 0; i < lvlcnt; i++, lvlp++) {
			if (lvlequal(&lvlv, lvlp) > 0) {
				good++;
				break;
			}
		}
		if (good) {
			tmplvl = *ia_lvlp;
			*ia_lvlp = lvlv;
			*lvlp = tmplvl;

			if (lckpwdf() != 0) {
				pfmt(err_iop, MM_WARNING, busylvl);
				adumprec(ADT_DEF_LVL, 1, ADT_LOGINMSG);
				good = 0;
			} else {
				if ((lvlia(IA_WRITE, (level_t **) ia_lvlp, u_name,
					&lvlcnt) != 0) || 
					(putiaent(u_name, s_uinfo) != 0)) {
					pfmt(err_iop, MM_WARNING, ng);
					adumprec(ADT_DEF_LVL, 1, ADT_LOGINMSG);
					good = 0;
				}
				(void) ulckpwdf();
			}
		}
			
		else {
			pfmt(err_iop, MM_ERROR, badlvl);
			adumprec(ADT_DEF_LVL, 1, ADT_LOGINMSG);
		}
		
	}
	if (lvlvalid(&level)) {
		adumprec(ADT_BAD_LVL, 0, ADT_LOGINMSG);
		(void) sleep (Def_slptime) ;
		pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
			incorrectmsg));
		return 0;
	}

	/* check level against device range */

	if (fdevstat(0, DEV_GET, &devstat) == 0) {
		if ((lvldom(&devstat.dev_hilevel, &level) <= 0) ||
			(lvldom(&level, &devstat.dev_lolevel) <= 0)) {
			adumprec(ADT_BAD_LVL, 0, ADT_LOGINMSG);
			(void) sleep (Def_slptime) ;
			pfmt(err_iop, MM_ERROR, nodo, u_name);
			return 0;
		}
	}

	/* check level against login range - if set */

	if (def_lvlhigh) {
		if (lvldom(&def_lvlhigh, &level) <= 0) {
			adumprec(ADT_BAD_LVL, 0, ADT_LOGINMSG);
			(void) sleep (Def_slptime) ;
			pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
				incorrectmsg));
			return 0;
		}
	}
	if (def_lvllow) {
		if (lvldom(&level, &def_lvllow) <= 0) {
			adumprec(ADT_BAD_LVL, 0, ADT_LOGINMSG);
			(void) sleep (Def_slptime) ;
			pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
				incorrectmsg));
			return 0;
		}
	}
	if (vflag && good)
		adumprec(ADT_DEF_LVL, 0, ADT_LOGINMSG);
	return 1;
}


/*
 * Procedure:	verify_pwd
 *
 * Notes:	Execute "/usr/bin/passwd" if passwords are required
 *		for the system, the user does not have a password,
 *		AND the user's NULL password can be changed accord-
 *		ing to its password aging information.
 *
 *		It also calls the program "/usr/bin/passwd" if the
 *		"-p" flag is present on the input line indicating the
 *		user wishes to modify their password.
 */
static	void
verify_pwd(nopass, priv_uid)
	register int	nopass;
	uid_t	priv_uid;
{
	time_t	now;
	long	ia_lstchg, ia_min,
			ia_max, ia_warn;
	register int	n,
			paschg = 0;
	char	*badpasswd = ":148:Cannot execute %s: %s\n";

	(void) alarm(0);	/* give user time to come up with new password */
	(void) sigset(SIGALRM, SIG_DFL);

	now = DAY_NOW;

	/* get the aging info */

	ia_get_logmin((uinfo_t)uinfo, &ia_min);
	ia_get_logmax((uinfo_t)uinfo, &ia_max);
	ia_get_logchg((uinfo_t)uinfo, &ia_lstchg);
	ia_get_logwarn((uinfo_t)uinfo, &ia_warn);

	if (nopass && (ia_uid != priv_uid)) {
		if (Passreq && *Passreq && (strcmp("YES", Passreq) == 0) &&
				((ia_max == -1) || (ia_lstchg > now) ||
				((now >= ia_lstchg + ia_min) &&
	 			(ia_max >= ia_min)))) {
#ifdef REMOTE_LOGIN
	/***Remote Login Zone: rlogin can't change "No Password"***/
			/* In the verified remote login case,
			* the password should not be set since
			* no passwd was gotten
			*/
			if ((!Rflag_rlogind) || (Rflag_rlogind && (usererr == -1)))
	/***indented and added { } - macro to control flow***/
#endif  /* REMOTE_LOGIN */
			{
				if (graphics) {
					do_put();
					exit(6);
				}
				pfmt(err_iop, MM_ERROR, ":322:You don't have a password.\n");
				if (!prt_alt(opt_fpm))
					(void) pfmt(err_iop, MM_ACTION, ":323:Choose one.\n");
				(void) fflush(stderr);
				n = exec_pass(u_name);
				if (n > 0) {
					adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
					exit(9);
				}
				if (n < 0) {
					pfmt(err_iop, MM_ERROR, badpasswd, BINPASSWD, strerror(errno));
					adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
					exit(9); 
				}
				paschg = 1;
				ia_lstchg = now;
			}
		}
	}

	/* Is the age of the password to be checked? */

	if ((ia_lstchg == 0) || (ia_lstchg > now) || ((ia_max >= 0)		
		&& (now > (ia_lstchg + ia_max)) && (ia_max >= ia_min))) {
		if ((Idleweeks == 0) || ((Idleweeks > 0) &&
			(now > (ia_lstchg + (7 * Idleweeks))))) {

			if (graphics) {
				adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
				exit(4);
			}
			pfmt(err_iop, MM_ERROR,
				":324:Your password has been expired for too long\n");
			pfmt(err_iop, MM_ACTION,
				":133:Consult your system administrator\n");
			adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
			exit(1);
		}
		else {
			if (graphics) {
				do_put();
				exit(6);
			}
			pfmt(err_iop, MM_ERROR, ":325:Your password has expired.\n");
			pfmt(err_iop, MM_ACTION, ":326:Choose a new one\n");
			n = exec_pass(u_name);
			if (n > 0) {
				adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
				exit(9);
			}
			if (n < 0) {
				pfmt(err_iop, MM_ERROR, badpasswd, BINPASSWD,
					strerror(errno));
				adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
				exit(9);
			}
		}
		paschg = 1;
		ia_lstchg = now;
	}

	if (pflag) {
		if (graphics) {
			do_put();
			exit(11);
		}
		if (!paschg) {
			n = exec_pass(u_name);
			if (n > 0)
				pfmt(err_iop, MM_WARNING, ":677:Password unchanged\n");
			if (n < 0)
				pfmt(err_iop, MM_WARNING, badpasswd, BINPASSWD,
					strerror(errno));
		}
		ia_lstchg = now;
	}

	/* Warn user that password will expire in n days. */

	if ((ia_warn > 0) && (ia_max > 0) &&
	            (now + ia_warn) >= (ia_lstchg + ia_max)) {

		int	xdays = (ia_lstchg + ia_max) - now;

		if (xdays <= 0) {
			if (graphics) {
				do_put();
				exit(6);
			}
			pfmt(err_iop, MM_ERROR, ":325:Your password has expired.\n");
			pfmt(err_iop, MM_ACTION, ":326:Choose a new one\n");
			n = exec_pass(u_name);
			if (n > 0) {
				adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
				exit(9);
			}
			if (n < 0) {
				pfmt(err_iop, MM_ERROR, badpasswd, BINPASSWD,
					strerror(errno));
				adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG);
				exit(9);
			}
			paschg = 1;
		}
		else {
			if (!graphics) {
				if (xdays == 1)
					(void) pfmt(err_iop, MM_INFO,
					":678:Your password will expire in 1 day\n");
				else
					(void) pfmt(err_iop, MM_INFO,
					":327:Your password will expire in %d days\n", xdays);
			}
		}
	}
}


/*
 * Procedure:	do_ava
 *
 * Notes:	Do the work to send the appropriate information up the
 *		stream to the process that called the login scheme.
 */
static	void
do_ava(mac, auditon)
	register int	mac,
		 	auditon;
{
	register int	i;
	gid_t	gidcnt,
		*ia_sgidp;
	char	*ptr,
		**avaptr,
		Uid[16] = { "UID=" },
		Gid[16] = { "GID=" },
		tty[32] = { "TTY=" },
		env[1024] = { "ENV=" },
		Sgid[256] = { "SGID=" },
		maclevel[32] = { "LID=" },
		Gidcnt[16] = { "GIDCNT=" },
		Ulimit[16] = { "ULIMIT=" },
		adtmask[128] = { "AUDITMASK=" },
		badputava[] = ":679:putava() failed for \"%s %s\"\n";

	avaptr = retava(0);

	if ((avaptr = putava(logname, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "logname", logname);
		exit(1);
	}
	(void) sprintf(Uid + strlen(Uid), "%d", (int) ia_uid);
	if ((avaptr = putava(Uid, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "uid", Uid);
		exit(1);
	}

	(void) sprintf(Gid + strlen(Gid), "%d", (int) ia_gid);
	if ((avaptr = putava(Gid, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "gid", Gid);
		exit(1);
	}

	(void) sprintf(Ulimit + strlen(Ulimit), "%d", (int) Def_ulimit);
	if ((avaptr = putava(Ulimit, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "ulimit", Ulimit);
		exit(1);
	}

	(void) strcat(tty, rttyn);
	if ((avaptr = putava(tty, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "tty", tty);
		exit(1);
	}

	if (auditon) {
		adtemask_t	ia_amask;

		ia_get_mask((uinfo_t)uinfo, ia_amask);
		for (i = 0; i < ADT_EMASKSIZE; i++) { 
			(void) sprintf(adtmask + strlen(adtmask), "%u", (unsigned) ia_amask[i]);
			(void) strcat(adtmask, ",");
		}
		if ((avaptr = putava(adtmask, avaptr)) == NULL) {
			adumprec(ADT_BAD_AUTH, 0, ADT_AUDITMSG);
			pfmt(err_iop, MM_ERROR, badputava, "adtmask,", adtmask);
			exit(1);
		}
	}
	if (mac) {
		(void) sprintf(maclevel + strlen(maclevel), "%ul", (unsigned) level);
		if ((avaptr = putava(maclevel, avaptr)) == NULL) {
			pfmt(err_iop, MM_ERROR, badputava, "maclevel", maclevel);
			exit(1);
		}
	}
	ia_get_sgid((uinfo_t)uinfo, &ia_sgidp, &gidcnt);
	if (gidcnt) {
		(void) sprintf(Gidcnt + strlen(Gidcnt), "%d", (int) gidcnt);
		if ((avaptr = putava(Gidcnt, avaptr)) == NULL) {
			pfmt(err_iop, MM_ERROR, badputava, "gidcnt", Gidcnt);
			exit(1);
		}
		for (i = 0; i < gidcnt; i++) { 
			(void) sprintf(Sgid + strlen(Sgid), "%d", (int) *ia_sgidp++);
			(void) strcat(Sgid, ",");
		}
		if ((avaptr = putava(Sgid, avaptr)) == NULL) {
			pfmt(err_iop, MM_ERROR, badputava, "sgid", Sgid);
			exit(1);
		}
	}
	if ((avaptr = putava(shell, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "shell", shell);
		exit(1);
	}
	if ((avaptr = putava(home, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "home", home);
		exit(1);
	}

	if ((ptr = argvtostr(envinit)) == NULL) {
		pfmt(err_iop, MM_ERROR, ":680:argvtostr() failed\n");
		exit(1);
	}

	(void) strcat(env, ptr);
	if ((avaptr = putava(env, avaptr)) == NULL) {
		pfmt(err_iop, MM_ERROR, badputava, "env", env);
		exit(1);
	}

	if (setava(0, avaptr) != 0) {
		pfmt(err_iop, MM_ERROR, ":681:setava() failed; *avaptr = %s\n",*avaptr);
		exit(1);
	}
}


/*
 * Procedure:	init_badtry
 *
 * Notes:	If the logfile exist, turn on attempt logging and
 *	 	initialize the string storage area.
 */
static	int
init_badtry(log_entry)
	char	**log_entry;
{
	register int	i, dolog = 0;
	struct	stat	dbuf;

	if (stat(LOGINLOG, &dbuf) == 0) {
		dolog = 1;
		for (i = 0; i < Def_failures; i++) {
			if (!(log_entry[i] = (char *) malloc((unsigned)ENT_SIZE))) {
				dolog = 0 ;
				break ;
			}
			*log_entry[i] = '\0';
		}	
	}
	return dolog;
}


/*
 * Procedure:	logbadtry
 *
 * Notes:	Writes the failed login attempt to the storage area.
 */
static	void
logbadtry(trys, log_entry)
	int	trys;
	char	**log_entry;
{
	long	timenow;

	if (trys && (trys <= Def_failures)) {
		(void) time(&timenow);
		(void) strncat(log_entry[trys-1], u_name, LNAME_SIZE);
		(void) strncat(log_entry[trys-1], ":", (size_t) 1);
		(void) strncat(log_entry[trys-1], rttyn, TTYN_SIZE);
		(void) strncat(log_entry[trys-1], ":", (size_t) 1);
		(void) strncat(log_entry[trys-1], ctime(&timenow), TIME_SIZE);
	}
}


/*
 * Procedure:	on_console
 *
 * Notes:	If the "priv_uid" is equal to the user's uid or the user
 *		is one of those listed in the USERS parameter in
 *		/etc/default/login, the login will be disallowed if the
 *		user is NOT on a terminal specified in the CONSOLE
 *		parameter in /etc/default/login.
 */
static	int
on_console(priv_uid)
	register uid_t	priv_uid;
{
	int	n,r1,r2,match=0;
	char	*p,s1[PATH_MAX],s2[PATH_MAX],*sp;

	if (!(match=(ia_uid==priv_uid)) && Users && *Users && (p=strtok(Users,","))) {
		while (p) {
			n=sscanf(p,"%d-%d",&r1,&r2);

			if (n==0 || (n==1 && sscanf(p,"%d%s",&r1,s1)>1) || (n==2 && sscanf(p,"%d-%d%s",&r1,&r2,s1)>2))
				match=(strcmp(uinfo->ia_name,p)==0);
			else if (n==1)
				match=(ia_uid==r1);
			else if (n==2)
				match=(r1<=ia_uid && ia_uid<=r2);

			if (match)
				break;

			p=strtok(NULL,",");
		}
	}

	if (match) {
		if (Console && *Console && (p=strtok(Console,","))) {
			match=0;
			while (p) {
				if ((sp=strchr(p,'-')))
					*sp=' ';
				n=sscanf(p,"%s%s",s1,s2);

				if (n==1)
					match=(strcmp(rttyn,s1)==0);
				else if (n==2)
					match=((strcmp(s1,rttyn)<=0) && (strcmp(rttyn,s2)<=0));

				if (match)
					break;

				p=strtok(NULL,",");
			}
		}
		if (!match) {
			pfmt(err_iop,MM_ERROR,":1097:Login not allowed on %s\n",rttyn);
			adumprec(ADT_BAD_AUTH, 0, ADT_LOGINMSG);
			return 0;
		}

		if (Def_supath != NULL)
			Def_path = Def_supath;
		else
			Def_path = DEF_SUPATH;
	}
	return 1;
}


/*
 * Procedure:	read_pass
 *
 * Notes:	Gets user password and checks if MANDPASS is required.
 *		returns 1 on failure, 0 on success.
 */
static	int
read_pass(priv_uid, nopass)
	uid_t	priv_uid;
	int	*nopass;
{
	char	*ia_pwdp,
		*pwdmsgid = ":308",
		*pwdmsg = "Password:";
	int	mandatory = 0;

	ia_get_logpwd((uinfo_t)uinfo, &ia_pwdp);

	/*
	 * If the user doesn't have a password check if the privilege
	 * mechanism supports ID-based functionality.  If so, its OK
	 * for a user with the privileged ID not to have a password.
	 *
	 * If, however, the MANDPASS flag is set and this user doesn't
	 * have a password, set a flag and continue on to get the
	 * user's password.  Otherwise, return success because its OK
	 * not to have a password.
	 */
	if (*ia_pwdp == '\0') {
		if (ia_uid == priv_uid)
			return 0;
		if (Mandpass && *Mandpass && (strcmp("YES", Mandpass) == 0)) {
			mandatory = 1;
		}
		else {
			return 0;
		}
	}
	/*
	 * Get the user's password, turning off echoing.
	 */

	/* do not translate password prompt because 
	 * uucp scripts depend on it.
	if (gpass(gettxt(pwdmsgid, pwdmsg), ia_pwdp, priv_uid, 1)) {
	 */
	ret = gpass(pwdmsg, ia_pwdp, priv_uid, 1) ;
	if (ret) {
                SLUserInfo.password[0] = '\0';  /* clear single user password */
		if (!mandatory) {	/* true password failure, log it */
			adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG); 
			if (graphics)
				exit(ret);
		}
		return 1;
	}
	/*
	 * Doesn't matter if the user entered No password.  Since MANDPASS
	 * was set, make it look like a bad login attempt.
	 */
	if (mandatory) {
		if (graphics) {
			adumprec(ADT_BAD_AUTH, 0, ADT_PASWDMSG); 
			do_put();
			exit(5);
		}
		(void) sleep(Def_slptime);
		pfmt(err_iop, MM_ERROR|MM_NOGET, gettxt(incorrectmsgid,
			incorrectmsg));
		return 1;
	}
	/*
	 * Everything went fine, so indicate that the user had a password
	 * and return success.
	 */
	else {
		*nopass = 0;
		return 0;
	}
}


/*
 * Procedure:	get_logoffval
 *
 * Notes:	The following is taken directly from the SVR4.1 require-
 *		ments relating to the functionality of the MAXTRYS and
 *		LOGFAILURES tunables:
 *
 *		1.  Users will be allowed LOGFAILURES (will be set  to  5)
 *		    attempts  to successfully log in at each invocation of
 *		    login.
 *
 *		2.  If  the  file  LOGINLOG  (will  be   defined   to   be
 *		    /var/adm/loginlog) exists, all LOGFAILURES consecutive
 *		    unsuccessful  login  attempts  will   be   logged   in
 *		    LOGINLOG.   After  LOGFAILURES  unsuccessful attempts,
 *		    login will sleep for DISABLETIME before  dropping  the
 *		    line.  In  other  words, if a person tried five times,
 *		    unsuccessfully, to log in  at  a  terminal,  all  five
 *		    attempts  will  be  logged  in /var/adm/loginlog if it
 *		    exists.   The  login  command  will  then  sleep   for
 *		    DISABLETIME  seconds  and  drop the line. On the other
 *		    hand,  if  a  person  has  one  or  two   unsuccessful
 *		    attempts, none of them will be logged.
 *
 *		      => Note: Since LOGFAILURES can now  be  set  by  the
 *		      administrator, it may be set to 1 so that any number
 *		      of failed login attempts are recorded.  When  either
 *		      MAXTRYS  or  LOGFAILURES  is reached login will exit
 *		      and the user will be disconnected from  the  system.
 *		      The   difference   being   that   in   the  case  of
 *		      LOGFAILURES, a record  of  bad  login  attempts  are
 *		      recorded in the system logs.
 *
 *		3.  By default, MAXTRYS and LOGFAILURES will be set to 5.
 *
 *		4.  If set, MAXTRYS  must  be  >=  0.   If  MAXTRYS=0  and
 *		    LOGFAILURES  is  not set, then login will not kick the
 *		    user  off  the  system  (unlimited  attempts  will  be
 *		    allowed).
 *
 *		5.  If set, LOGFAILURES must be within the range of  0-20.
 *		    If  LOGFAILURES  is  = 0, and MAXTRYS is not set, then
 *		    login will not kick off the user  (unlimited  attempts
 *		    will be allowed).
 *
 *		6.  If  LOGFAILURES  or  MAXTRYS   are   not   set,   then
 *		    respectively,  the  effect will be as if the item were
 *		    set to 0.
 *
 *		7.  The lowest positive number of MAXTRYS and  LOGFAILURES
 *		    will  be  the  number of failed login attempts allowed
 *		    before  the  appropriate  action  is  taken  (e.g.  1,
 *		    MAXTRYS  =  3  and LOGFAILURES=6 then the user will be
 *		    kicked off the system after 3 bad login  attempts  and
 *		    IN  NO  CASE  shall  bad  login  records end up in the
 *		    system log file (var/adm/loginlog).  e.g. 2, MAXTRYS=6
 *		    and  LOGFAILURES=3,  then  the user will be kicked off
 *		    the system after 3 bad  login  attempts  and  at  that
 *		    point  in  time,  3  records  will  be recorded in the
 *		    system log file).
 *
 *		8.  In the case when  both  values  are  equal,  then  the
 *		    action of LOGFAILURES will dominate (i.e., a record of
 *		    the bad login attempts will be  recorded  in  the  log
 *		    files).
 */
static	long
get_logoffval()
{
	if (Def_maxtrys == Def_failures)		/* #8 */
		return Def_failures;

	if (!Def_maxtrys && (Def_failures < 2))		/* #4, #5, and #6 */
		return 0;

	if (Def_maxtrys < Def_failures) {		/* #7, example 1 */
		Def_failures = 0;
		return Def_maxtrys;
	}
	/*
	 * Def_maxtrys MUST be greater than Def_failures so
	 * return Def_failures!
	 */
	return Def_failures;				/* #7, example 2 */
}


/*
 * Procedure:	ck_xlogin
 *
 * Notes:	Checks to see if the IAF module was pushed and if there is
 *		any data on the stream.  Specifically checks for the terms
 *		XLOGNAME and XPASSWD.  Both MUST be present to consider
 *		this a graphical login scheme.  Also expects XTTY to be
 *		on the stream. If it isn't, something else is used instead.
 *		 If an application intends
 *		 to port this internal interface to in.telnetd and in.rlogind
 *		 it may need to adjust this ck_xlogin concept.
 */
static	int
ck_xlogin()
{
	char	*p;
	char	**avap;

	if ((avap = retava(0)) == NULL)
		return 0;

	if ((p = getava("XLOGNAME", avap)) == NULL)
		return 0;

	SCPYN(Xstr, p);

	if ((p = getava("XPASSWD", avap)) == NULL)
		return 0;

	SCPYN(Xpasswd, p);

	if ((p = getava("XTTY", avap)) == NULL)
		SCPYN(Xtty, "/dev/vt");
	else
		SCPYN(Xtty, p);
#ifndef REMOTE_LOGIN
	/***Remote Login Zone: Do not send errors to "/dev/null".***/
	/*
	 * Call ck_xlogin() to determine if login was called by the
	 * graphical port monitor.  If so, then abort.
	 */
	/*
	 * Set the file descriptor for error messages to "/dev/null".
	 */
	err_iop = fopen("/dev/null", "a+");
	/*
	 * No need to sleep on bad login attempt with graphical login.
	 */
	Def_slptime = 0;
#endif  /* REMOTE_LOGIN */

	return 1;
}


/*
 * Procedure:	prt_alt
 *
 * Notes:	Prints alternate message to user if they don't have
 *		a password and one is required.
 */
static	int
prt_alt(fnamep)
	char	*fnamep;
{
	char	*p;
	char	line[BUFSIZ];
	struct	stat	sbuf;
	FILE	*iop;

	/*
	 * If file name pointer is NULL, return 0.
	 */
	if (!fnamep)
		return 0;

	/*
	 * If file does not exist, return 0.
	 */
	if (stat(fnamep, &sbuf) < 0)
		return 0;

	/*
	 * File existed, check to see if it contains any data.
	 * If not, return 0.
	 */
	if (sbuf.st_size == 0)
		return 0;

	/*
	 * If the file is not a regular file or any of the execute
	 * mode bits are turned on, return 0.
	 */
	if (((sbuf.st_mode & S_IFMT) != S_IFREG) || (sbuf.st_mode & 0111))
		return 0;

	/*
	 * File existed and contained data, now see if file is
	 * readable.  If not, return 0;
	 */
	if ((iop = fopen(fnamep, "r")) == NULL)
		return 0;

	/*
	 * Now print whatever data is contained in the file.
	 */
	while ((p = fgets(line, sizeof(line), iop)) != NULL)
		(void) printf("%s", p);

	/*
	 * Flush the stdout, and close the file.
	 */
	(void) fflush(stdout);
	(void) fclose(iop);

	return 1;	/* success */
}


/*
 * Procedure:	do_exit
 *
 * Notes:	Can do several things here.  For instance, if the master
 *		file is open, close it before exiting.  Also, sleeps before
 *		exiting to give the user a chance to read any error messages
 *		before the screen is cleared.  Virtual terminals, for example,
 *		disappear immediately.
 */
static	void
do_exit()
{
	if (uinfo_open) {
		ia_closeinfo(uinfo);
	}
	if (!graphics && Def_exit_time)
		(void) sleep(Def_exit_time);
	return;
}


/*
 * Procedure:	execute
 *
 * Notes:	Emulates the "login" command processing that was common
 *		in previous versions of UNIX(TM).  This routine will
 *		never return since it will try to exec the user's shell
 *		or exit on error.
 */
static	void
execute(u_shellp, dirp)
	char	*u_shellp;
	char	*dirp;
{
		FILE	*defltfp;
		char	**envp,
			*endptr,
			minusnam[16] = "-",
			*noexec   = ":331:No shell\n",
			tmout_env_buf[128] = "TIMEOUT=0",
			*badset_id = ":846:set_id() failed: %s",
			*nodir 	  = ":735:unable to change directory to %s\n";

		extern	char	**environ;

		/*
		 * Make this process auditable again.
		 */
		(void) auditevt(AYAUDIT, NULL, sizeof(aevt_t));
		/*
		 * Write an audit record indicating successful
		 * I&A.  This doesn't mean that the user got a
		 * prompt, it just means that they entered a
		 * recognized login name with the correct matching
		 * password.
		 */
		adumprec(ADT_LOGIN, 0, ADT_LOGINMSG);

		if (set_id(u_name) != 0) {
			pfmt(err_iop, MM_ERROR, badset_id, strerror(errno));
			exit(1);
		}
		if (chdir(dirp) != 0) {
			pfmt(err_iop, MM_ERROR, nodir, dirp);
			exit(1);
		}
		environ = (char **) envinit;
		(void) strcat (minusnam, basename(u_shellp));
		/*
		 * Read the /etc/defaults/sh file to obtain shell TIMEOUT
		 * value, pass this value through the environment.
		 */
		if ((defltfp = defopen("sh")) != NULL) {
			register char	*ptr;
	
			if ((ptr = defread(defltfp, "TIMEOUT")) != NULL) {
				(void) strcpy(&tmout_env_buf[8], (const char *)ptr);
			}
			(void) defclose(defltfp);
		}
		(void) putenv(tmout_env_buf);
	
		(void) execl(u_shellp, minusnam, (char *) 0);
	
		if (access(u_shellp, R_OK|X_OK) == 0) {
			/*
			 * "u_shellp" was not an executable object file.
			 * Maybe its a shell procedure or command line with
			 * arguments.  If so, clear the SHELL= environment
			 * variable.
			 */
			envp = environ;
			for (; *envp != NULL; envp++) {
				if (!(strncmp(*envp, "SHELL=", 6)) &&
		    		   ((endptr = strchr(*envp, '=')) != NULL)) {
					(*++endptr) = '\0';
				}
			}
			(void) execl(SHELL, "sh", u_shellp, (char *) 0);
		}
		/*
		 * Neither "exec" worked so issue a diagnostic and exit.
		 */
		pfmt(err_iop, MM_ERROR, noexec);
		exit(1);
}

/*
 * Procedure:	do_put
 *
 * Notes:	Puts the value in XLOGNAME back on the stream for XDM
 *		to use in changing passwords.
 *
 * WARNING:	The desktop metaphor needs to be modified to handle any
 *		MAC options when the MAC feature is supported.
 */
static	void
do_put()
{
	char	**avaptr;

	avaptr = retava(0);

	(void) getava("XLOGNAME", avaptr);

	avaptr = putava(xlogname, avaptr);

	(void) setava(0, avaptr);

	return;
}


/*
 * Procedure:	no_utmp_entry
 *
 * Notes:	Determine if a valid "utmp" entry exists for this
 *		invocation of "login".  If a preceeding process did
 *		not set this up correctly, don't continue!
*/
static	int
no_utmp_entry()
{
	register struct	utmpx	*u;
	pid_t			tmppid;

	tmppid = getpid();

	/*
	 * Find the entry for this pid in the utmp file.
	 */
	while ((u = getutxent()) != NULL) {
		if (((u->ut_type == INIT_PROCESS ||
			u->ut_type == LOGIN_PROCESS) &&
			u->ut_pid == tmppid) ||
			(u->ut_type == USER_PROCESS &&
			u->ut_pid  == tmppid)) {

			break;
		}
	}
	endutxent();		/* close utmp file */

	if (u == (struct utmpx *)NULL)
		return 1;
	 
	return 0;
}

/*
 *      chk_slogin_active ( void )
 *
 *      Check to see if single login is active -- this assumes
 *      that the name is active and the user's directory has
 *      been read in
 *
 *      Entry :
 *              Nothing
 *
 *      Exit :
 *              0               single login NOT active
 *              1               single login active
 *
 *      Notes :
 */

int
chk_slogin_active ( void )

{
        FILE                    *fp;

        /*
         *      first check for SYSTEM single login active
         */

        if ( (fp = fopen ( SYS_SLOGIN, "r") ) == NULL) {
                /*
                 *      system single login not active
                 */
                return ( 0 );
        }
        (void) fclose ( fp );

        /*
         *      SYSTEM single login is active -- check user
         */

        if ( (fp = fopen ( slogin_config, "r") ) != NULL) {

                 /*     user single login not active */

                return ( 0 );
        }
        (void) fclose ( fp );
        /*
         *      user single login is  active
         */

        return ( 1 );
}


#ifdef REMOTE_LOGIN
	/***Remote Login Zone: Private Functions***/
	/* These functions are only used in the in.login scheme
	*  for remote login and telnet sessions.
	*	doremotelogin(host)
	*	getstr(buf, cnt, err)
	*	doremoteterm(term)
	*/

/*
 * Procedure:	doremotelogin(host)
 * Restrictions:	defined only when compiled with the REMOTE_LOGIN macro
 * Notes:		Validate an internet remote login session
*/
static int
doremotelogin(host)
	char *host;
{
	static struct passwd *pwd;
	struct	passwd *getpwnam();
	void getstr();

	getstr(rusername, sizeof (rusername), "remuser");
	getstr(lusername, sizeof (lusername), "locuser");
	getstr(terminal, sizeof(terminal), "Terminal type");

	if (getuid()) {
		return(-1);
	}
	pwd = getpwnam(lusername);
	if (pwd == NULL) {
		return(-1);
	}
	valid_luser = 1;	/* have entry in passwd file for this user */
	return(ruserok(host, (pwd->pw_uid == 0), rusername, lusername));
}

/*
 * Procedure:	getstr
 * Restrictions:	defined only when compiled with the REMOTE_LOGIN macro
 * Notes:		gets null-terminated string with length restriction
 * and exit with failure when buffer overflows
*/
static void
getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
	} while ((--cnt > 1) && (c != 0));
	*buf = 0;

	if ((0 == cnt) && (c != 0)) {
		printf("%s too long\r\n", err);
		exit(1);
	}

}
/*
 * Procedure:	doremoteterm(term)
 * Restrictions:	defined only when compiled with the REMOTE_LOGIN macro
 * Notes:		Set up remote device characteristics
*/
static int
doremoteterm(term)
	char *term;
{
	char *strchr();
	struct termios tp;
	register char *cp = strchr(term, '/'), **cpp;
	char *speed;
#ifdef __STDC__
	char	*speeds[] =
	{ "0", "50", "75", "110", "134", "150", "200", "300",
	"600", "1200", "1800", "2400", "4800", "9600", "19200", "38400" };
#else
	char	*speeds[16];

	speeds[0] = "0";
	speeds[1] = "50";
	speeds[2] = "75";
	speeds[3] = "110";
	speeds[4] = "134";
	speeds[5] = "150";
	speeds[6] = "200";
	speeds[7] = "300";
	speeds[8] = "600";
	speeds[9] = "1200";
	speeds[10] = "1800";
	speeds[11] = "2400";
	speeds[12] = "4800";
	speeds[13] = "9600";
	speeds[14] = "19200";
	speeds[15] = "38400";
#endif

#define	NSPEEDS	(sizeof (speeds) / sizeof (speeds[0]))

	ioctl(0, TCGETS, &tp);

	if (cp) {
		*cp++ = '\0';
		speed = cp;
		cp = strchr(speed, '/');
		if (cp)
			*cp++ = '\0';
		for (cpp = speeds; cpp < &speeds[NSPEEDS]; cpp++)
			if (strcmp(*cpp, speed) == 0) {
				tp.c_cflag = ( (tp.c_cflag & ~CBAUD) |
					       ((cpp-speeds) & CBAUD) );
				break;
			}
	}
	tp.c_lflag |= ECHO|ICANON;
	tp.c_oflag |= XTABS;
	tp.c_iflag |= IGNPAR|ICRNL;
	tp.c_cc[VEOF] = CEOF;
	tp.c_cc[VEOL] = CEOL;

	ioctl(0, TCSETS, &tp);

}
#endif  /* REMOTE_LOGIN */

#ifdef	LIMITED

/*
**  If you have a limited binary license, then we only allow a LIMITED
**  number of concurrent users.
*/
int
license_limit()
{}
#endif	/* LIMITED */

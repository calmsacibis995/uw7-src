/*		copyright	"%c%" 	*/


#ident	"@(#)lpsched.c	1.3"
#ident  "$Header$"

/*******************************************************************************
 *
 * FILENAME:    lpsched.c
 *
 * DESCRIPTION: This is the main module for the "lpsched" command, it 
 *              schedule print requests.
 *
 * SCCS:	lpsched.c 1.3  7/21/97 at 14:50:58
 *
 * CHANGE HISTORY:
 *
 * 21-07-97  Paul Cunningham        ul97-19724
 *           Added in call to function scanSuspList() to scan for any items
 *           on the Suspend_List that have timed out. This was added to cope
 *           with the problem where a printer client does a "askforstatus" on
 *           a remote print server that is no responding (eg. has been mis-
 *           configured).
 *
 *
 * Command: lpsched
 * Inheritable Privileges: P_SETPLEVEL,P_SETFLEVEL,P_OWNER,P_SYSOPS,
 *                         P_MACREAD,P_MACWRITE,P_AUDIT,P_COMPAT,
 *                         P_DACREAD,P_DACWRITE,P_DEV,P_SETUID
 * Fixed Privileges:       None
 *
 *******************************************************************************
 */

#include <sys/utsname.h>
#include <sys/stat.h>
#include <limits.h>
#include <ulimit.h>
#include <priv.h>

#include "lpsched.h"
#include "debug.h"

#include <sys/time.h>
#include <sys/resource.h>

#define WHO_AM_I        I_AM_LPSCHED
#include "oam.h"

#ifdef	MALLOC_3X
#include "malloc.h"
#endif

int			Starting	= 0;
int			Shutdown	= 0;
int			DoneChildren	= 0;
int			Sig_Alrm	= 0;
int			OpenMax		= OPEN_MAX;
int			Reserve_Fds	= 0;

char			*Local_System	= 0;
char			*SHELL		= DEFAULT_SHELL;

gid_t			Lp_Gid;
uid_t			Lp_Uid;
level_t			Lp_Lid;

#ifdef	DEBUG
unsigned long		debug = 0;
int			signals = 0;
#endif

extern int		errno;
extern void		shutdown_messages();
extern void		scanSuspList();

int			am_in_background	= 0;

static void		disable_signals();
static void		startup();
static void		process();
static void		ticktock();
static void		background();
static void		usage();
static void		Exit();
static void		disable_signals();

/**
 ** main()
 **/

#ifdef	__STDC__
main (
	int			argc,
	char *			argv[]
)
#else
main (argc, argv)
    int		argc;
    char	*argv[];
#endif
{
    int		c;
#ifdef	MALLOC_3X
    extern int	optind;
#endif
    extern char	*optarg;
    extern int	optopt;
    extern int	opterr;
#ifdef	DEBUG
    char *	cp;
#endif
    struct rlimit rlp;
    int		fd_ct; 

    DEFINE_FNNAME (main)

    SET_DEBUG_PATH  ("/tmp/lpsched.debug")
    OPEN_DEBUG_FILE ("/tmp/lpsched.debug")
    PrintProcPrivs ();

    /*
    **  Get the current maximun number of open file descriptors that
    **  the process can have and then close all of them before starting
    **  the LP daemon.
    */
    if (getrlimit(RLIMIT_NOFILE, &rlp) != -1)  {
	    for (fd_ct=3; fd_ct<rlp.rlim_cur; fd_ct++)
		(void)close(fd_ct);
    }

#if	defined(MDL)
# ident "lpsched has MDL"
    /*
    **	Set the output of MDL to be MDL-LOG
    */
    mdl(SET_FILE_STREAM, 0, 0, 0, "MDL-LOG");
    /*
    **	Set the toggle Flag to cause the file to be opened
    **	and closed as needed rather than opened once and kept.
    **	(ie, it saves a file descriptor at the cost or performance).
    */
    mdl(TOGGLE_OPEN, 0, 0, 0, 0);
#endif

#ifdef	MALLOC_3X

# if	defined(DEF_MXFAST)
    mallopt (M_MXFAST, DEF_MXFAST);
# endif
# if	defined(DEF_NLBLKS)
    mallopt (M_NLBLKS, DEF_NLBLKS);
# endif
# if	defined(DEF_GRAIN)
    mallopt (M_GRAIN, DEF_GRAIN);
# endif
    mallopt (M_KEEP, 0);

#endif

    opterr = 0;
    while((c = getopt(argc, (char * const *)argv, "D:dsf:n:r:M:")) != EOF)
        switch(c)
        {
# if defined (DEBUG)
	    case 'd':
		debug = (unsigned int) DB_ALL;
		goto SkipD;
	    case 'D':
		if (*optarg == '?') {
			note ("\
-D flag[,flag...]    (all logs \"foo\" are in /var/lp/logs,\n\
                      although \"lpsched\" goes to stdout if SDB)\n\
\n\
  EXEC               (log all exec's in \"exec\")\n\
  DONE               (log just exec finishes in \"exec\")\n\
  INIT               (log initialization info in \"lpsched\" or stdout)\n\
  ABORT              (issue abort(2) on fatal error)\n\
  SCHEDLOG           (log additional debugging info in \"lpsched\")\n\
  SDB                (don't start lpsched as background process)\n\
  MESSAGES           (log all message traffic in \"messages\")\n"
			);
#if	defined(TRACE_MALLOC)
			note ("\
  MALLOC             (track malloc use; dump on SIGUSR1 in \"lpsched\")\n"
			);
#endif
			note ("\
  ALL                (all of the above; equivalent to -d)\n"
			);
			exit (0);
		}
		while ((cp = strtok(optarg, ", "))) {
#define IFSETDB(P,S,F)	if (STREQU(P, S)) debug |= F
			IFSETDB (cp, "EXEC", DB_EXEC);
			else IFSETDB (cp, "DONE", DB_DONE);
			else IFSETDB (cp, "INIT", DB_INIT);
			else IFSETDB (cp, "ABORT", DB_ABORT);
			else IFSETDB (cp, "SCHEDLOG", DB_SCHEDLOG);
			else IFSETDB (cp, "SDB", DB_SDB);
			else IFSETDB (cp, "MESSAGES", DB_MESSAGES);
			else IFSETDB (cp, "MALLOC", DB_MALLOC);
			else IFSETDB (cp, "ALL", DB_ALL);
			else {
				note ("-D flag not recognized; try -D?\n");
				exit (1);
			}
			optarg = 0;
		}
SkipD:
		(void) OPEN_DEBUG_FILE ("/tmp/lpsched.debug")

#   if	defined(TRACE_MALLOC)
		if (debug & DB_MALLOC) {
#ifdef	__STDC__
			extern void	(*mdl_logger)( char * , ... );
#else
			extern void	(*mdl_logger)();
#endif
			mdl_logger = note;
		}
#   endif
		break;

	    case 's':
		signals++;
		break;
# endif /* DEBUG */

	    case 'f':
		if ((ET_SlowSize = atoi(optarg)) < 1)
		    ET_SlowSize = 1;
		break;

	    case 'n':
		if ((ET_NotifySize = atoi(optarg)) < 1)
		    ET_NotifySize = 1;
		break;

	    case 'r':
		if ((Reserve_Fds = atoi(optarg)) < 1)
		    Reserve_Fds = 0;
		break;

#ifdef	MALLOC_3X
	    case 'M':
		{
			int			value;

			value = atoi(optarg);
			printf ("M_MXFAST set to %d\n", value);
			mallopt (M_MXFAST, value);

			value = atoi(argv[optind++]);
			printf ("M_NLBLKS set to %d\n", value);
			mallopt (M_NLBLKS, value);

			value = atoi(argv[optind++]);
			printf ("M_GRAIN set to %d\n", value);
			mallopt (M_GRAIN, value);
		}
		break;
#endif

	    case '?':
		if (optopt == '?')
		{
		    usage ();
		    exit (0);
		}
		else
		{
		    lpfail (ERROR, E_SCH_BADOP, optopt); /* abs s19.4 */
		}
	}
    
    lp_alloc_fail_handler = mallocfail;

    startup();

    process();

    lpshut(1);	/* one last time to clean up */
    /*NOTREACHED*/
}

/*
 * Procedure:     startup
 *
 * Restrictions:
 *               lvlproc(2): None
 *               setuid(2): None
 *               mopen: None
 *               ulimit(2): None
*/

static void
startup()
{
    DEFINE_FNNAME (startup)

    int			n;
    struct passwd	*p;
    struct utsname	utsbuf;

    Starting = 1;
    getpaths();

    /*
     * There must be a user named "lp".
     */
    if ((p = lp_getpwnam(LPUSER)) == NULL)
	lpfail (ERROR, E_SCH_CANTFIND);
    lp_endpwent();

    Lp_Uid = p->pw_uid;
    Lp_Gid = p->pw_gid;
    TRACEd (Lp_Uid)
    TRACEd (Lp_Gid)
    /*
    **  ES Note:
    **  Our MAC-level should be SYS_PRIVATE and we explicitly enforce this.
    **  LP_DEFAULT_LID == SYS_PRIVATE
    */
    while ((n = lvlproc (MAC_GET, &Lp_Lid)) < 0 && errno == EINTR)
		continue;
    if (n < 0)
	if (errno == ENOPKG)
		Lp_Lid = LPSCHED_PROC_LID;
    	else
		lpfail (ERROR, E_SCH_CNOTMACL);

    if (Lp_Lid != LPSCHED_PROC_LID)
    {
	Lp_Lid = LPSCHED_PROC_LID;
   	while ((n = lvlproc (MAC_SET, &Lp_Lid)) < 0 && errno == EINTR)
		continue;
	if (n < 0 && errno != ENOPKG)
		lpfail (ERROR, E_SCH_CNOTMACL);
    }
    /*
    **  Only "root" and "lp" are allowed to run us.
    **
    **  ES Note:
    **  Not quite true anymore.
    **  We no longer determine who can start us.  The tfadmin
    **  DB determines who can start us.  In the case of ES not
    **  being installed then whoever can exec us can run us.
    **  But we should be able to change our uid and gid to lp
    **  (for consistency).
    */
    /*
    **if (getuid() && getuid() != Lp_Uid)
    **	fail ("You must be \"lp\" or \"root\" to run this program.\n");
    */
	/*
	**  We set our uid to root because in the SUM env root
	**  is the privileged process and will allow us to retain
	**  privs when doing 'seteuid's.
	*/
	if (setuid (0) < 0)
		lpfail (ERROR, E_SCH_CANTSET);
	TRACEd (getuid())
	TRACEd (getgid())

	(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t)0);

/*
	if (seteuid (Lp_Uid) < 0)
		lpfail (ERROR, E_SCH_CANTSETE);
	TRACEd (geteuid())

	(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t)0);

	if (setegid (Lp_Gid) < 0)
		lpfail (ERROR, E_SCH_CANTSETG);
	TRACEd (getegid())

	(void)	procprivl (SETPRV, ALLPRIVS_W, (priv_t)0);

*/
	(void)	mldmode (MLD_VIRT);

    (void) uname(&utsbuf);
    Local_System = Strdup(utsbuf.nodename);

    /*
    **  If mopen() succeeds (returns 0) then a scheduler must already
    **  be running.  If it fails then no harm is done.
    */
    if (!mopen())
	    lpfail (WARNING, E_SCH_PRACTIVE);

    /*
     * Make sure that all critical directories are present and that 
     * symbolic links are correct.
     */
    lpfsck();
    
    background();
    /*
     * We are the child process now.
#ifdef	DEBUG
     * (That is, unless the debug flag is set.)
#endif
     */

    (void) Close (0);
    (void) Close (2);
    if (am_in_background)
	(void) Close (1);

    if ((OpenMax = ulimit(4, 0L)) == -1)
	OpenMax = OPEN_MAX;

    disable_signals();

    init_messages();

    init_network();

    init_memory();

    lpnote (INFO, E_SCH_PRSTART);
    Starting = 0;
}

/*
 * Procedure:     lpshut
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
void
lpshut (int immediate)
#else
void
lpshut (immediate)

int	immediate;
#endif
{
	DEFINE_FNNAME (lpshut)

	int			i;

	extern MESG *		Net_md;


	/*
	 * If this is the first time here, stop all running
	 * child processes, and shut off the alarm clock so
	 * it doesn't bug us.
	 */
	if (!Shutdown) {
		(void) mputm (Net_md, S_SHUTDOWN, 1);
		for (i = 0; i < ET_Size; i++)
			terminate (&Exec_Table[i]);
		(void) alarm (0);
		Shutdown = (immediate? 2 : 1);
	}

	/*
	 * If this is an express shutdown, or if all the
	 * child processes have been cleaned up, clean up
	 * and get out.
	 */
	if (Shutdown == 2) {

		/*
		 * We don't shut down the message queues until
		 * now, to give the children a chance to answer.
		 * This means an LP command may have been snuck
		 * in while we were waiting for the children to
		 * finish, but that's OK because we'll have
		 * stored the jobs on disk (that's part of the
		 * normal operation, not just during shutdown phase).
		 */
		shutdown_messages();
    
		lpnote (INFO, E_SCH_PRSTOP);
		exit (0);
		/*NOTREACHED*/
	}
}

static void
process()
{
    register FSTATUS	*pfs;
    register PWSTATUS	*ppws;

    DEFINE_FNNAME (process)


    /*
     * Call the "check_..._alert()" routines for each form/print-wheel;
     * we need to do this at this point because these routines
     * short-circuit themselves while we are in startup mode.
     * Calling them now will kick off any necessary alerts.
     */
    for (pfs = walk_ftable(1); pfs; pfs = walk_ftable(0))
	check_form_alert (pfs, (_FORM *)0);
    for (ppws = walk_pwtable(1); ppws; ppws = walk_pwtable(0))
	check_pwheel_alert (ppws, (PWHEEL *)0);
    
    /*
     * Clear the alarm, then schedule an EV_ALARM. This will clear
     * all events that had been scheduled for later without waiting
     * for the next tick.
     */
    (void) alarm (0);
    schedule (EV_ALARM);

    /*
     * Start the ball rolling.
     */
    schedule (EV_INTERF, (PSTATUS *)0);
    schedule (EV_NOTIFY, (RSTATUS *)0);
    schedule (EV_SLOWF, (RSTATUS *)0);

#if	defined(CHECK_CHILDREN)
    schedule (EV_CHECKCHILD);
#endif

    schedule (EV_POLLPRINTER, (PSTATUS *)0);

    for (EVER)
    {
	TRACEP ("Begin loop")

        /* check on the request status suspended list for timed out items
	 */
	scanSuspList();

	take_message ();

	if (Sig_Alrm)
		schedule (EV_ALARM);

	if (DoneChildren)
		dowait ();

	if (Shutdown)
		check_children();
	if (Shutdown == 2)
		break;
    }
}

/* ARGSUSED0 */
static void
#ifdef	__STDC__
ticktock (
	int			sig
)
#else
ticktock(sig)
	int			sig;
#endif
{
	DEFINE_FNNAME (ticktock)

	Sig_Alrm = 1;
	(void) signal (SIGALRM, ticktock);
	return;
}
			    
/*
 * Procedure:     background
 *
 * Restrictions:
 *               fork(2): None
*/
static void
background()
{
    DEFINE_FNNAME (background)

#ifdef	DEBUG
    if (debug & DB_SDB)
	return;
#endif
    
    switch(fork())
    {
	case -1:
	    lpfail (ERROR, E_SCH_FFORKCHILD, PERROR);
	    /*NOTREACHED*/

	case 0:
	    (void) setpgrp();
	    am_in_background = 1;
	    return;
	    
	default:
	    lpnote (INFO, E_SCH_PRSTART);
	    exit(0);
	    /* NOTREACHED */
    }
}

static void
usage()
{
	DEFINE_FNNAME (usage)

	lpnote (INFO, E_SCH_USAGE);

#ifdef	DEBUG
	
        lpnote(INFO|MM_NOSTD, E_SCH_USAGE1);
  
#endif

#ifdef	MALLOC_3X
	lpnote (INFO|MM_NOSTD, E_SCH_USAGE2);
#endif

	lpnote (WARNING, E_SCH_USAGE3);

	return;
}

static void
Exit(n)
    int		n;
{
    DEFINE_FNNAME (Exit)

    lpfail (ERROR, E_SCH_UESIGNAL, n);
}

static void
disable_signals()
{
    DEFINE_FNNAME (disable_signals)

    int		i;

# if defined(DEBUG)
    if (!signals)
# endif
	for (i = 0; i < NSIG; i++)
		if (signal(i, SIG_IGN) != SIG_IGN)
			(void) signal (i, Exit);
    
    (void) signal(SIGHUP, SIG_IGN);
    (void) signal(SIGINT, SIG_IGN);
    (void) signal(SIGQUIT, SIG_IGN);
    (void) signal(SIGALRM, ticktock);
    (void) signal(SIGTERM, lpshut);	/* needs arg, but sig# OK */
    (void) signal(SIGCLD, SIG_IGN);
    (void) signal(SIGTSTP, SIG_IGN);
    (void) signal(SIGCONT, SIG_DFL);
    (void) signal(SIGTTIN, SIG_IGN);
    (void) signal(SIGTTOU, SIG_IGN);
    (void) signal(SIGXFSZ, SIG_IGN);	/* could be a problem */

#ifdef	DEBUG
    if (debug & DB_ABORT)
	(void) signal(SIGABRT, SIG_DFL);
#endif

#if	defined(TRACE_MALLOC)
    if (debug & DB_MALLOC) {
# if	defined(	__STDC__)
	static void		sigusr1( int );
# else
	static void		sigusr1();
# endif
	(void) signal(SIGUSR1, sigusr1);
    }
#endif
}

#if	defined(TRACE_MALLOC)
static void
# if	defined(	__STDC__)
sigusr1 ( int sig )
# else
sigusr1()
# endif
{
	(void) signal(SIGUSR1, sigusr1);
	mdl_dump ();
}
#endif

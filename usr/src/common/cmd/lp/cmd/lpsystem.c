/*		copyright	"%c%" 	*/


#ident	"@(#)lpsystem.c	1.2"
#ident  "$Header$"

#include	<sys/utsname.h>
#include	<stdio.h>
#include	<tiuser.h>
#include	<netconfig.h>
#include	<netdir.h>

#include	"lp.h"
#include	"systems.h"
#include	"msgs.h"
#include	"boolean.h"
#include	"debug.h"

#define WHO_AM_I	I_AM_LPSYSTEM
#include "oam.h"

static	int	Timeout;
static	int	Retry;
static	char	*Sysnamep;
static	char	*Protocolp;
static	char	*Timeoutp;
static	char	*Retryp;
static	char	*Commentp;
static	boolean	SchedulerRequired;

#ifdef	__STDC__
static	int	NotifyLpsched (int, char *);
static	void	SecurityCheck (void);
static	void	TcpIpAddress (void);
static	void	ListSystems (char * []);
static	void	RemoveSystems (char * []);
static	void	AddModifySystems (char * []);
static	void	formatsys (SYSTEM *);
#else
static	int	NotifyLpsched ();
static	void	SecurityCheck ();
static	void	TcpIpAddress ();
static	void	ListSystems ();
static	void	RemoveSystems ();
static	void	AddModifySystems ();
static	void	formatsys ();
#endif

extern	int	errno;

#ifdef	__STDC__
int
main (int argc, char * argv [])
#else
int
main (argc, argv)
int	argc;
char	*argv [];
#endif
{
	int c;
	boolean	lflag = False,rflag = False,Aflag = False,badOptions = False;
	extern int opterr,optind;
	extern char *optarg;
 
	while ((c = getopt(argc, argv, "t:T:R:y:lrA?")) != EOF)
	switch (c & 0xFF) {
	case 't':
		if (Protocolp) {
                    LP_ERRMSG1(ERROR, E_SYS_MANYOPT, "t");
                    return 1;
		}
		Protocolp = optarg;
		if (!STREQU(NAME_S5PROTO, Protocolp) &&
		    !STREQU(NAME_BSDPROTO, Protocolp)&&
		    !STREQU(NAME_NUCPROTO, Protocolp)) {
			LP_ERRMSG3(ERROR, E_SYS_SUPPROT2,
			   	NAME_S5PROTO, NAME_BSDPROTO, NAME_NUCPROTO);
			return	1;
		}
		break;
	case 'T':
		if (Timeoutp) {
                        LP_ERRMSG1(ERROR, E_SYS_MANYOPT, "T");
			return	1;
		}
		Timeoutp = optarg;
		if (*Timeoutp == 'n')
			Timeout = -1;
		else if (sscanf(Timeoutp,"%d",&Timeout) != 1 || Timeout < 0) {
                        LP_ERRMSG1(ERROR, E_SYS_BADTIMEOUT, Timeoutp);
			return	1;
		}
		break;
	case 'R':
		if (Retryp) {
                        LP_ERRMSG1(ERROR, E_SYS_MANYOPT, "R");
			return	1;
		}
		Retryp = optarg;
		if (*Retryp == 'n')
			Retry = -1;
		else if (sscanf (Retryp, "%d", &Retry) != 1 || Retry < 0) {
                        LP_ERRMSG1(ERROR, E_SYS_BADRETRY, Retryp);
			return	1;
		}
		break;
	case 'y':
		if (Commentp) {
                        LP_ERRMSG1(ERROR, E_SYS_MANYOPT, "y");
			return	1;
		}
		Commentp = optarg;
		break;
	case 'l':
		if (lflag) {
                        LP_ERRMSG1(ERROR, E_SYS_MANYOPT, "l");
			return	1;
		}
		lflag++;
		break;
	case 'r':
		if (rflag) {
                        LP_ERRMSG1(ERROR, E_SYS_MANYOPT, "r");
			return	1;
		}
		rflag++;
		break;
	case 'A':
		if (Aflag) {
                        LP_ERRMSG1(ERROR, E_SYS_MANYOPT, "A");
			return	1;
		}
		Aflag++;
		break;
	default:
                LP_ERRMSG1(ERROR, E_SYS_OPTION, c & 0xFF);
		return	1;
	case '?':
                LP_OUTMSG(INFO, E_SYS_USAGE);
		return	0;
	}
	/*
	**  Check for valid option combinations.
	**
	**  The '-A' option is mutually exclusive.
	**  The '-l' option is mutually exclusive.
	**  The '-r' option is mutually exclusive.
	**
	**  Allow combination of Aflag and Protocolp if IPXAddress() 
	*/
	if (Aflag && (Protocolp || Timeoutp || Retryp || Commentp))
		badOptions = True;
	if (lflag&&(Protocolp||Timeoutp||Retryp||Commentp||rflag||Aflag))
		badOptions = True;
	if (rflag && (Protocolp || Timeoutp || Retryp || Commentp || Aflag))
		badOptions = True;
	if (badOptions) {
		LP_ERRMSG(ERROR, E_SYS_IMPRUS);
		LP_OUTMSG(INFO, E_SYS_USAGE);
		return	1;
	}
	/*
	**	Lets do some processing.
	**	We'll start with the flags.
	*/
	if (Aflag) {
		TcpIpAddress ();
		/*NOTREACHED*/
	}
	if (lflag) {
		ListSystems (&argv [optind]);
		/*NOTREACHED*/
	}
	if (rflag) {
		RemoveSystems (&argv [optind]);
		/*NOTREACHED*/
	}
	AddModifySystems (&argv [optind]);
	return	0;
}

#ifdef	__STDC__
static	void
SecurityCheck (void)
#else
static	void
SecurityCheck ()
#endif
{
	/*
	**  ES Note:
	**  This is no longer applicable.
	**
	if (geteuid () != 0) {
                LP_ERRMSG(ERROR, E_SYS_BEROOT);
		(void)	exit (1);
	}
	*/
	return;
}

/* Do we need a IPXAddress function? */
#ifdef	__STDC__
static	void
TcpIpAddress (void)
#else
static	void
TcpIpAddress ()
#endif
{
	int	i;
	struct	utsname		utsbuf;
	struct	netconfig	*configp;
	struct	nd_hostserv	hostserv;
	struct	nd_addrlist	*addrsp;
	struct	netconfig	*getnetconfigent ();

	(void)	uname (&utsbuf);
	configp = getnetconfigent ("tcp");
	if (! configp) {
		LP_ERRMSG (ERROR, E_SYS_NOTCPIP);
		(void)	exit (2);
	}
	hostserv.h_host = utsbuf.nodename;
	hostserv.h_serv = "printer";
	if (netdir_getbyname (configp, &hostserv, &addrsp)) {
		int	save = errno;
		(void)	pfmt (stderr, ERROR, NULL);
                errno = save;
		(void)	perror ("netdir_getbyname");
		(void)	exit (2);
	}
	for (i=0; i < addrsp->n_addrs->len; i++)
		(void)printf("%02x",(unsigned char)addrsp->n_addrs->buf [i]);
	(void)	printf ("\n");
	(void)	exit (0);
}
#ifdef	__STDC__
static	void
ListSystems (char *syslistp [])
#else
static	void
ListSystems (syslistp)

char *syslistp [];
#endif
{
	char	*sysnamep;
	SYSTEM	*systemp;
	int	exit_val = 0;

	if (! *syslistp) {
		while ((systemp = getsystem (NAME_ALL)) != NULL)
			formatsys (systemp);
	}
	else for (sysnamep = *syslistp; sysnamep; sysnamep = *++syslistp) {
		if (STREQU(NAME_ANY,sysnamep) || STREQU(NAME_NONE,sysnamep) ||
		    STREQU(NAME_ALL, sysnamep)) {
                        LP_ERRMSG1(WARNING, E_SYS_RESWORD, sysnamep);
			continue;
		}
		if ((systemp = getsystem (sysnamep)) == NULL) {
                        LP_ERRMSG1(WARNING, E_SYS_NOTFOUND, sysnamep);
			exit_val = 1;
			continue;
		}
		formatsys (systemp);
	}
	(void)	exit (exit_val);
}

#ifdef	__STDC__
static	void
RemoveSystems (char *syslistp [])
#else
static	void
RemoveSystems (syslistp)
char	*syslistp [];
#endif
{
	char	*sysnamep;
	SYSTEM	*systemp;
	int	exit_val = 0,status=0;

	SecurityCheck ();
	if (! syslistp || ! *syslistp) {
                LP_ERRMSG(ERROR, E_SYS_IMPRUS);
		LP_OUTMSG(INFO, E_SYS_USAGE);
		(void)	exit (1);
	}
	SchedulerRequired = True;
	for (sysnamep = *syslistp; sysnamep; sysnamep = *++syslistp) {
		if (STREQU(NAME_ANY, sysnamep)||STREQU(NAME_NONE, sysnamep)||
		    STREQU(NAME_ALL, sysnamep))	{
                        LP_ERRMSG1(WARNING, E_SYS_RESWORD, sysnamep);
			continue;
		}
		if (! (systemp = getsystem (sysnamep))) {
                        LP_ERRMSG1(WARNING, E_SYS_NOTFOUND, sysnamep);
			exit_val = 1;
			continue;
		}
		if ((status=NotifyLpsched (S_UNLOAD_SYSTEM, sysnamep))!=MOK ||
		    delsystem (sysnamep)) {
			switch(status) {
			case MBUSY:
				LP_ERRMSG1(ERROR,E_SYS_BUSY,sysnamep);
				break;
			case MNODEST:
	                        LP_ERRMSG1(WARNING,E_SYS_NOTFOUND,sysnamep);
				break;
			case MNOOPEN:
	                        LP_ERRMSG(ERROR,E_LP_NEEDSCHED);
				break;
			default:
	                        LP_ERRMSG1(ERROR,E_SYS_NOTREM,sysnamep);
			}
			(void)	exit (2);
		}
		else
                        LP_OUTMSG1(INFO, E_SYS_REMOVED, sysnamep);
	}
	(void)	exit (exit_val);
}
#ifdef	__STDC__
static	void
AddModifySystems (char *syslistp [])
#else
static	void
AddModifySystems (syslistp)
char	*syslistp [];
#endif
{
	char	*sysnamep;
	SYSTEM	*systemp,sysbuf;
	boolean	modifiedFlag;
	int status;
	static	SYSTEM	DefaultSystem =	{ NULL, NULL, NULL, S5_PROTO, NULL,
					  DEFAULT_TIMEOUT, DEFAULT_RETRY,
					  NULL, NULL, NULL
					}; 
	SecurityCheck ();
	if (! syslistp || ! *syslistp) {
                LP_ERRMSG(ERROR, E_SYS_IMPRUS);
		LP_OUTMSG(INFO, E_SYS_USAGE);
		(void)	exit (1);
	}
	for (sysnamep = *syslistp; sysnamep; sysnamep = *++syslistp) {
		modifiedFlag = False;
		if (systemp = getsystem (sysnamep)) {
			sysbuf = *systemp;
			modifiedFlag = True;
		}
		else {
			sysbuf = DefaultSystem;
			sysbuf.name = sysnamep;
		}
		if (Protocolp) {
			if (STREQU (NAME_S5PROTO, Protocolp))
				sysbuf.protocol = S5_PROTO;
			else if (STREQU (NAME_BSDPROTO, Protocolp))
				sysbuf.protocol = BSD_PROTO;
			else sysbuf.protocol = NUC_PROTO;
		}
		if (Timeoutp) 
			sysbuf.timeout = Timeout;
		if (Retryp)
			sysbuf.retry = Retry;
		if (Commentp)
			sysbuf.comment = Commentp;
		if (putsystem (sysnamep, &sysbuf)) {
                        LP_ERRMSG2(ERROR, E_SYS_COULDNT,
				modifiedFlag ? "modify" : "add", sysnamep);
			(void)	exit (2);
		}
		if ((status=NotifyLpsched (S_LOAD_SYSTEM, sysnamep)) != MOK) {
			/*
			**  Try to put the old system back.
			*/
			if (systemp)
				(void)	putsystem (sysnamep, systemp);
			switch(status) {
			case MBUSY:
				LP_ERRMSG1(ERROR,E_SYS_BUSY,sysnamep);
				break;
			case MNODEST:
	                        LP_ERRMSG1(WARNING,E_SYS_NOTFOUND,sysnamep);
				break;
			case MNOOPEN:
	                        LP_ERRMSG(ERROR,E_LP_NEEDSCHED);
				break;
			default:
	                        LP_ERRMSG2(ERROR,E_SYS_COULDNT,
				modifiedFlag ? "modify" : "add", sysnamep);
			}
			(void)	exit (2);
		}
		if (modifiedFlag)
                        LP_OUTMSG1(INFO, E_SYS_MODIFIED, sysnamep);
		else
                        LP_OUTMSG1(INFO, E_SYS_ADDED, sysnamep);
	}
	(void)	exit (0);
}

#ifdef	__STDC__
static	int
NotifyLpsched (int msgType, char *sysnamep)
#else
static	int
NotifyLpsched ()
int	msgType;
char	*sysnamep;
#endif
{
	char	msgbuf [MSGMAX];
	ushort	status;
	static	boolean	OpenedChannel	= False;
	static	char	FnName []	= "NotifyLpsched";

	if (! OpenedChannel) {
		if (mopen () < 0) {
		/*
		**  In some cases if the scheduler is down
		**  we can do what we want.  Other cases
		**  require the scheduler to be present.
		*/
		return	SchedulerRequired ? MNOOPEN : MOK;
		}
		OpenedChannel = True;
	}
	if (putmessage (msgbuf, msgType, sysnamep) < 0) {
                int	save = errno;
		(void)	pfmt (stderr, ERROR, NULL);
                errno = save;
		(void)	perror ("putmessage");
		(void)	exit (2);
	}
	if (msend (msgbuf) < 0) {
                int	save = errno;
		(void)	pfmt (stderr, ERROR, NULL);
                errno = save;
		(void)	perror ("putmessage");
		(void)	exit (2);
	}
	if (mrecv (msgbuf, sizeof (msgbuf)) < 0) {
                int	save = errno;
		(void)	pfmt (stderr, ERROR, NULL);
                errno = save;
		(void)	perror ("mrecv");
		(void)	exit (2);
	}
	if (getmessage (msgbuf, mtype (msgbuf), &status) < 0) {
                int	save = errno;
		(void)	pfmt (stderr, ERROR, NULL);
                errno = save;
		(void)	perror ("mrecv");
		(void)	exit (2);
	}
	return	(int)	status;
}

#ifdef	__STDC__
static	void
formatsys (SYSTEM * sys)
#else
static	void
formatsys (sys)
SYSTEM	*sys;
#endif
{
	char *systype;
	
	if (sys->protocol == S5_PROTO)
		systype = NAME_S5PROTO;
	else if (sys->protocol == BSD_PROTO)
		systype = NAME_BSDPROTO;
	else systype = NAME_NUCPROTO;

        LP_OUTMSG1(INFO|MM_NOSTD, E_SYS_SYS, sys->name);
	LP_OUTMSG1(INFO|MM_NOSTD, E_SYS_TYPE, systype);
	if (sys->timeout == -1)
                LP_OUTMSG(INFO|MM_NOSTD, E_SYS_NEVER2);
	else
                LP_OUTMSG1(INFO|MM_NOSTD, E_SYS_MINUTES2, sys->timeout);
	if (sys->retry == -1)
                LP_OUTMSG(INFO|MM_NOSTD, E_SYS_NORETRY2);
	else
                LP_OUTMSG1(INFO|MM_NOSTD, E_SYS_MINRETRY2, sys->retry);
        LP_OUTMSG1(INFO|MM_NOSTD, E_SYS_COMMENT2,
		(sys->comment == NULL ? "none" : sys->comment));
}

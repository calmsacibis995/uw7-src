/*		copyright	"%c%" 	*/

#ident	"@(#)uucp.h	1.3"
#ident  "$Header$"

#ifndef _UUCP_H
#define _UUCP_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "parms.h"

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <sys/param.h>

#include <sys/termios.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include <utime.h>
#include <dirent.h>

#include <time.h>

#include <sys/times.h>
#include <errno.h>

#include <sys/mkdev.h>

/* If big command line is needed for uux, increase the BUFSIZ */
#if defined(BIGCMDLINE)
# if defined(BUFSIZ)
#  undef BUFSIZ
# endif                 /* BUFSIZ */
#define BUFSIZ 5000
#endif                  /* BIGCMDLINE */

/* what mode should user files be allowed to have upon creation? */
/* NOTE: This does not allow setuid or execute bits on transfer. */
#define LEGALMODE (mode_t) 0666

/* what mode should public files have upon creation? */
#define PUB_FILEMODE (mode_t) 0666

/* what mode should log files have upon creation? */
#define LOGFILEMODE (mode_t) 0644

/* what mode should C. files have upon creation? */
#define CFILEMODE (mode_t) 0664

/* what mode should D. files have upon creation? */
#define DFILEMODE (mode_t) 0660

/* define the value of PUBMASK, used for creating "public" directories */
#define PUBMASK (mode_t) 0000

/* what mode should public directories have upon creation? */
#define PUB_DIRMODE (mode_t) 01777

/* define the value of DIRMASK, used for creating "system" subdirectories */
#define DIRMASK (mode_t) 0002

#define MAXSTART	300	/* how long to wait on startup */

/* define the last characters for ACU  (used for 801/212 dialers) */
#define ACULAST "<"

/*  caution - the fillowing names are also in Makefile 
 *    any changes here have to also be made there
 *
 * it's a good idea to make directories .foo, since this ensures
 * that they'll be ignored by processes that search subdirectories in SPOOL
 *
 *  XQTDIR=/var/uucp/.Xqtdir
 *  CORRUPT=/var/uucp/.Corrupt
 *  LOGDIR=/var/uucp/.Log
 *  SEQDIR=/var/uucp/.Sequence
 *  STATDIR=/var/uucp/.Status
 *  
 */

/* where to put the STST. files? */
#define STATDIR		"/var/uucp/.Status"

/* where should logfiles be kept? */
#define LOGUUX		"/var/uucp/.Log/uux"
#define LOGUUXQT	"/var/uucp/.Log/uuxqt"
#define LOGUUCP		"/var/uucp/.Log/uucp"
#define LOGCICO		"/var/uucp/.Log/uucico"
#define CORRUPTDIR	"/var/uucp/.Corrupt"

/* some sites use /var/uucp/.XQTDIR here */
/* use caution since things are linked into there */
#define XQTDIR		"/var/uucp/.Xqtdir"

/* how much of a system name can we print in a [CX]. file? */
/* MAXBASENAME - 1 (pre) - 1 ('.') - 1 (grade) - 4 (sequence number) */
#define SYSNSIZE (MAXBASENAME - 7)

#ifdef USRSPOOLLOCKS
#define LOCKPRE		"/var/spool/locks/LCK."
#else
#define LOCKPRE		"/var/spool/uucp/LCK."
#endif /* USRSPOOLLOCKS */

#define SQFILE		"/etc/uucp/SQFILE"
#define SQTMP		"/etc/uucp/SQTMP"
#define SLCKTIME	5400	/* system/device timeout (LCK.. files) */
#define DIALCODES	"/etc/uucp/Dialcodes"
#define PERMISSIONS	"/etc/uucp/Permissions"

#define SPOOL		"/var/spool/uucp"
#define SEQDIR		"/var/uucp/.Sequence"

#define X_LOCKTIME	3600
#ifdef USRSPOOLLOCKS
#define SEQLOCK		"/var/spool/locks/LCK.SQ."
#define SQLOCK		"/var/spool/locks/LCK.SQ"
#define X_LOCK		"/var/spool/locks/LCK.X"
#define S_LOCK		"/var/spool/locks/LCK.S"
#define L_LOCK		"/var/spool/locks/LK"
#define X_LOCKDIR	"/var/spool/locks"	/* must be dir part of above */
#else
#define SEQLOCK		"/var/spool/uucp/LCK.SQ."
#define SQLOCK		"/var/spool/uucp/LCK.SQ"
#define X_LOCK		"/var/spool/uucp/LCK.X"
#define S_LOCK		"/var/spool/uucp/LCK.S"
#define L_LOCK		"/var/spool/uucp/LK"
#define X_LOCKDIR	"/var/spool/uucp"	/* must be dir part of above */
#endif /* USRSPOOLLOCKS */
#define X_LOCKPRE	"LCK.X"		/* must be last part of above */

#define PUBDIR		"/var/spool/uucppublic"
#define ADMIN		"/var/uucp/.Admin"
#define ERRLOG		"/var/uucp/.Admin/errors"
#define SYSLOG		"/var/uucp/.Admin/xferstats"
#define RMTDEBUG	"/var/uucp/.Admin/audit"
#define CLEANUPLOGFILE	"/var/uucp/.Admin/uucleanup"
#define CMDLOG		"/var/uucp/.Admin/command"
#define PERFLOG		"/var/uucp/.Admin/perflog"
#define ACCOUNT		"/var/uucp/.Admin/account"
#define SECURITY	"/var/uucp/.Admin/security"

#define	WORKSPACE	"/var/uucp/.Workspace"

#define SQTIME		60
#define TRYCALLS	2	/* number of tries to dial call */
#define MINULIMIT	(1L<<11)	/* minimum reasonable ulimit */
#define	MAX_LOCKTRY	5	/* number of attempts to lock device */

/*
 * CDEBUG is for communication line debugging 
 * DEBUG is for program debugging 
 * #define SMALL to compile without the DEBUG code
 */

#ifndef SMALL
#define  DEBUG(l, f, s) if (Debug >= l) fprintf(stderr, f, s)
#define CDEBUG(l, f, s) if (Debug >= l) fprintf(stderr, f, s)
#else
#define  DEBUG(l, f, s) 
#define CDEBUG(l, f, s) 
#endif /* SMALL */

/*
 * VERBOSE is used by cu and ct to inform the user of progress
 * In other programs, the Value of Verbose is always 0.
 */
#define VERBOSE(f, s) { if (Verbose > 0) fprintf(stderr, f, s); }

#define PREFIX(pre, str)	(strncmp((pre), (str), strlen(pre)) == SAME)
#define BASENAME(str, c) ((Bnptr = strrchr((str), c)) ? (Bnptr + 1) : (str))
#define EQUALS(a,b)	((a != CNULL) && (b != CNULL) && (strcmp((a),(b))==SAME))
#define EQUALSN(a,b,n)	((a != CNULL) && (b != CNULL) && (strncmp((a),(b),(n))==SAME))
#define LASTCHAR(s)	(s+strlen(s)-1)

#define SAME 0
#define FAIL -1
#define SUCCESS 0
#define NULLCHAR	'\0'
#define CNULL (char *) 0
#define STBNULL (struct sgttyb *) 0
#define MASTER 1
#define SLAVE 0
#define MAXBASENAME 14 /* should be DIRSIZ but that is now fs dependent */
#define MAXFULLNAME BUFSIZ
#define MAXNAMESIZE	64	/* /var/spool/uucp/<14 chars>/<14 chars>+slop */
#define CONNECTTIME 30
#define EXPECTTIME 45
#define MSGTIME 60
#define NAMESIZE MAXBASENAME+1
#define	SIZEOFPID	10		/* maximum number of digits in a pid */
#define EOTMSG "\004\n\004\n"
#define CALLBACK 1

/* manifests for sysfiles.c's sysaccess()	*/
/* check file access for REAL user id */
#define	ACCESS_SYSTEMS	1
#define	ACCESS_DEVICES	2
#define	ACCESS_DIALERS	3
/* check file access for EFFECTIVE user id */
#define	EACCESS_SYSTEMS	4
#define	EACCESS_DEVICES	5
#define	EACCESS_DIALERS	6

/* replace eaccess() local routine with access() with new flag */
#define eaccess(a,b)	access(a, EFF_ONLY_OK | b)

/* CHDIR() will make intermediate directories before doing a chdir() */
#define CHDIR(a)	((void) mkdirs(a, DIRMASK), chdir(a))

/* manifest for chkpth flag */
#define CK_READ		0
#define CK_WRITE	1

/* MINimum macro */
#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

/*
 * commands
 */
#define SHELL		"/usr/bin/sh"
#define MAIL		"mail"
#define UUCICO		"/usr/lib/uucp/uucico"
#define UUCP		"/usr/bin/uucp"
#define UUX		"/usr/bin/uux"
#define UUXCMD		"/usr/lib/uucp/uuxcmd"
#define UUXQT		"/usr/lib/uucp/uuxqt"


/* system status stuff */
#define SS_OK			0
#define SS_NO_DEVICE		1
#define SS_TIME_WRONG		2
#define SS_INPROGRESS		3
#define SS_CONVERSATION		4
#define SS_SEQBAD		5
#define SS_LOGIN_FAILED		6
#define SS_DIAL_FAILED		7
#define SS_BAD_LOG_MCH		8
#define SS_LOCKED_DEVICE	9
#define SS_ASSERT_ERROR		10
#define SS_BADSYSTEM		11
#define SS_CANT_ACCESS_DEVICE	12
#define SS_DEVICE_FAILED	13	/* used for interface failure */
#define SS_WRONG_MCH		14
#define SS_CALLBACK		15
#define SS_RLOCKED		16
#define SS_RUNKNOWN		17
#define SS_RLOGIN		18
#define SS_UNKNOWN_RESPONSE	19
#define SS_STARTUP		20
#define SS_CHAT_FAILED		21
#define SS_CALLBACK_LOOP	22
#define SS_INVOKE_FAILED	23
#define SS_CS_PROB		24


#define MAXPH	60	/* maximum phone string size */
#define	MAXC	BUFSIZ

#define	TRUE	1
#define	FALSE	0
#define NAMEBUF	32

/* The call structure is used by ct.c, cu.c, and dial.c.	*/

static struct call {
	char *speed;		/* transmission baud rate */
	char *line;		/* device name for outgoing line */
	char *telno;		/* ptr to tel-no digit string */
	char *type;		/* type of device to use for call. */
};

/* structure of an Systems file line */
#define F_MAX	50	/* max number of fields in Systems file line */
#define F_NAME 0
#define F_TIME 1
#define F_TYPE 2
#define F_CLASS 3	/* an optional prefix and the speed */
#define F_PHONE 4
#define F_LOGIN 5

/* structure of an Devices file line */
#define D_TYPE 0
#define D_LINE 1
#define D_CALLDEV 2
#define D_CLASS 3
#define D_CALLER 4
#define D_ARG 5
#define D_MAX	50	/* max number of fields in Devices file line */

#define D_ACU 1
#define D_DIRECT 2
#define D_PROT 4

#define GRADES "/etc/uucp/Grades"

#define	D_QUEUE	'Z'	/* default queue */

/* past here, local changes are not recommended */
#define CMDPRE		'C'
#define DATAPRE		'D'
#define XQTPRE		'X'

/*
 * stuff for command execution
 */
#define X_RQDFILE	'F'
#define X_STDIN		'I'
#define X_STDOUT	'O'
#define X_STDERR	'E'
#define X_CMD		'C'
#define X_USER		'U'
#define X_BRINGBACK	'B'
#define X_MAILF		'M'
#define X_RETADDR	'R'
#define X_COMMENT	'#'
#define X_NONZERO	'Z'
#define X_SENDNOTHING	'N'
#define X_SENDZERO	'n'
#define X_AUTH		'A'


/* This structure describes call routines */
struct caller {
	char	*CA_type;
	int	(*CA_caller)();
};

/* structure for a saved C file */

static struct cs_struct {
	char	file[NAMESIZE];
	char	sys[NAMESIZE+5];
	char	sgrade[NAMESIZE];
	char	grade;
	long	jsize;
};

/* This structure describes dialing routines */
struct dialer {
	char	*DI_type;
	int	(*DI_dialer)();
};

struct nstat {
	pid_t	t_pid;		/* process id				*/
	time_t	t_start;	/* start time				*/
	time_t	t_scall;	/* start call to system			*/
	time_t	t_ecall;	/* end call to system			*/
	time_t	t_tacu;		/* acu time				*/
	time_t	t_tlog;		/* login time				*/
	time_t	t_sftp;		/* start file transfer protocol		*/
	time_t	t_sxf;		/* start xfer 				*/
	time_t	t_exf;		/* end xfer 				*/
	time_t	t_eftp;		/* end file transfer protocol		*/
	time_t	t_qtime;	/* time file queued			*/
	int	t_ndial;	/* # of dials				*/
	int	t_nlogs;	/* # of login trys			*/
	struct tms t_tbb;	/* start execution times		*/
	struct tms t_txfs;	/* xfer start times			*/
	struct tms t_txfe;	/* xfer end times 			*/
	struct tms t_tga;	/* garbage execution times		*/
};

/* This structure describes the values from Limits file */
struct limits {
	int	totalmax;	/* overall limit */
	int	sitemax;	/* limit per site */
	char	mode[64];	/* uucico mode */
};

struct MsgNo {
	char	*msgno;		/* message no. of UerrorText[] */
	char	*msgs;		/* message of UerrorText[] */
};

/* external declarations */

extern int Read(), Write();
#if defined(__STDC__)
/*
extern int Ioctl(int,int,int);
*/
extern Ioctl();
#else
extern int Ioctl();
#endif
extern int Ifn, Ofn;
extern int Debug, Verbose;
extern uid_t Uid, Euid;		/* user-id and effective-uid */
extern uid_t Gid, Egid;		/* group-id and effective-gid */
extern long Ulimit;
extern mode_t Dev_mode;		/* save device mode here */
extern char Wrkdir[];
extern long Retrytime;
extern char **Env;
extern char Uucp[];
extern char Pchar;
extern struct nstat Nstat;
extern char Dc[];			/* line name			*/
extern int Seqn;			/* sequence #			*/
extern int Role;
extern int Sgrades;		/* flag for administrator defined service grades */
extern char Grade;
extern char Logfile[];
extern char Rmtname[];
extern char JobGrade[];
extern char User[];
extern char Loginuser[];
extern char *Spool;
extern char *Pubdir;
extern char Myname[];
extern char Keys[];
extern char Progname[];
extern char RemSpool[];
extern char *Bnptr;		/* used when BASENAME macro is expanded */
extern char *sys_errlist[];
extern int SizeCheck;		/* ulimit check supported flag */
extern long RemUlimit;		/* remote ulimit if supported */
extern int Restart;		/* checkpoint restart supported flag */

extern char Jobid[];		/* Jobid of current C. file */
extern int Uerror;		/* global error code */
extern struct MsgNo UerrorText[]; /* text for error code */

/*	Some global I need for section 2 and section 3 routines */
extern char *optarg;	/* for getopt() */
extern int optind;	/* for getopt() */

#define UERRORTEXT		gettxt(UerrorText[Uerror].msgno,\
					UerrorText[Uerror].msgs)
#define UTEXT(x)		gettxt(UerrorText[x].msgno,\
					UerrorText[x].msgs)

extern struct stat __s_;
#define F_READANY(f)	((fstat((f),&__s_)==0) && ((__s_.st_mode&(0004))!=0) )
#define READANY(f)	((stat((f),&__s_)==0) && ((__s_.st_mode&(0004))!=0) )

#define WRITEANY(f)	((stat((f),&__s_)==0) && ((__s_.st_mode&(0002))!=0) )
#define DIRECTORY(f)	((stat((f),&__s_)==0) && ((__s_.st_mode&(S_IFMT))==S_IFDIR) )
#define NOTEMPTY(f)	((stat((f),&__s_)==0) && (__s_.st_size!=0) )

/* standard functions used */

extern char	*strcat(), *strcpy(), *strncpy(), *strrchr();
extern char	*strchr(), *strpbrk();
extern char	*getlogin(), *ttyname();
extern long	times(), lseek(), atol();
extern time_t	time();
extern int	pipe(), close(), getopt();
extern struct tm	*localtime();
extern FILE	*popen();

/* uucp functions and subroutine */
extern int	iswrk(), gtwvec();			/* anlwrk.c */
extern void	findgrade();				/* grades.c */
extern void	chremdir(), mkremdir();			/* chremdir.c */
extern void	toCorrupt();				/* cpmv.c  */
extern int	xmv();					/* cpmv.c  */

extern int	getargs();				/* getargs.c */
extern void	bsfix();				/* getargs.c */
extern char	*getprm();				/* getprm.c */

extern char	*next_token();				/* permission.c */
extern char	*nextarg();				/* permission.c */
extern int	getuline();				/* permission.c */

extern void	logent(), syslog(), closelog();		/* logent.c */
extern void	commandlog();				/* logent.c */
extern time_t	millitick();				/* logent.c */

extern unsigned long	getfilesize();			/* statlog.c */
extern void 		putfilesize();			/* statlog.c */

extern char	*protoString();				/* permission.c */
extern		logFind(), mchFind();			/* permission.c */
extern		chkperm(), chkpth();			/* permission.c */
extern		cmdOK(), switchRole();			/* permission.c */
extern		callBack(), requestOK();		/* permission.c */
extern		noSpool(), AuthReq(), CryptType();	/* permission.c */
extern void	myName(), keys();			/* permission.c */

extern int	mkdirs();				/* expfile.c */
extern int	scanlimit();				/* limits.c */
extern void	systat();				/* systat.c */
extern int	mklock(), cklock(), mlock();		/* ulockf.c */
extern void	delock(), rmlock();			/* ulockf.c */
extern char	*timeStamp();				/* utility.c */
extern void	assert(), errent();			/* utility.c */
extern void	uucpname();				/* uucpname.c */
extern int	versys();				/* versys.c */
extern void	xuuxqt(), xuucico();			/* xqt.c */
extern void	cleanup();				/* misc main.c */

#define ASSERT(e, s1, s2, i1) if (!(e)) {\
	assert(s1, s2, i1, __FILE__, __LINE__);\
	cleanup(21);};

/* messages */
extern char *Ct_OPEN;
extern char *Ct_WRITE;
extern char *Ct_READ;
extern char *Ct_CREATE;
extern char *Ct_ALLOCATE;
extern char *Ct_LOCK;
extern char *Ct_STAT;
extern char *Ct_CHOWN;
extern char *Ct_CHMOD;
extern char *Ct_LINK;
extern char *Ct_CHDIR;
extern char *Ct_UNLINK;
extern char *Wr_ROLE;
extern char *Ct_CORRUPT;
extern char *Ct_FORK;
extern char *Ct_CLOSE;
extern char *Ct_BADOWN;
extern char *Fl_EXISTS;

/* defines for temporarily included des encryption (see sum.c) */

#define KEYLEN		8

typedef char Key[8];

#endif

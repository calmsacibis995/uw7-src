/*		copyright	"%c%" 	*/

#ident	"@(#)cron:common/cmd/cron/cron.h	1.7.5.4"

/*
 * Definitions for cron and related programs.
 */

#define FALSE		0
#define TRUE		1

#define MINUTE		60L		/* seconds in a minute */
#define HOUR		(60L*60L)	/* seconds in an hour */
#define DAY		(24L*60L*60L)	/* seconds in a day */

#define	NQUEUE		26		/* number of queues available (a-z) */

#define	ATEVENT		0		/* internal event type for at jobs */
#define BATCHEVENT	1		/* internal event type for batch jobs */
#define CRONEVENT	2		/* internal event type for cron jobs */

#define ADD		'a'		/* message action to add a job */
#define DELETE		'd'		/* message action to delete a job */
#define	AT		'a'		/* message etype for at jobs */
#define CRON		'c'		/* message etype for cron jobs */

#define UNAMESIZE	20		/* max chars in a user name */
/*
 * Structure used for passing messages from the
 * at and crontab commands to the cron daemon.
 */
struct message {
	level_t	lid;			/* reserved, currently unused */
	char	etype;			/* type of job, AT or CRON */
	char	action;			/* action requested, ADD or DELETE */
	char	fname[FILENAME_MAX];	/* job file name */
	char	logname[UNAMESIZE + 1];	/* user's login name */
};

#ifdef MDEBUG				/* for xmalloc/free debugging */

#define MALLOC(size, type)		xxmalloc((size), (type))
#define FREE(addr, type)		xxfree((addr), (type))
#define TEMPNAM(dir, pfx, type)		xxtempnam((dir), (pfx), (type))

extern char *	xxmalloc();
extern void	xxfree();
extern char *	xxtempnam();

#define M_ATCMD		1		/* at cmd */
#define M_ATEVENT	2		/* at event struct */
#define M_CRCMD		3		/* cron cmd */
#define M_CREVENT	4		/* cron event struct */
#define M_CRFIELD	5		/* cron time field */
#define M_ELM		6		/* elm struct */
#define M_HOME		7		/* home dir */
#define M_IN		8		/* cron input */
#define M_JOB		9		/* runinfo jobname */
#define M_OUTFILE	10		/* runinfo outfile */
#define M_RECBUF	11		/* msg_wait recbuf */
#define M_RNAME		12		/* runinfo user name */
#define M_UNAME		13		/* user name */
#define M_USR		14		/* usr struct */
#define M_SHELL		15		/* shell field */

#else

#define MALLOC(size, type)		xmalloc(size)
#define FREE(addr, type)		free(addr)
#define TEMPNAM(dir, pfx, type)		tempnam((dir), (pfx))

#endif

#ifdef __STDC__

struct dirent;
extern void	copylog(char *, char *, int);
extern int	days_btwn(int, int, int, int, int, int);
extern int	days_in_mon(int, int);
extern time_t	num(char **);
extern char *	xmalloc(int);
extern char *	select_shell (const char *);
extern void	el_add(int *, time_t, int);
extern void	el_delete(void);
extern int	el_empty(void);
extern int *	el_first(void);
extern void	el_init(int, time_t, time_t, int);
extern void	el_remove(int, int);
extern int	ascandir(char *, struct dirent *(*[]), int (*)(), int (*)());
extern int	filewanted(struct dirent *);
extern int	sendmsg(int, char *, char *, int, gid_t);
extern int	allowed(char *, char *, char *);
extern char *	getuser(uid_t, char **);

#else /* __STDC__ */

extern void	copylog();		/* copylog.c */
extern int	days_btwn();		/* cronfuncs.c */
extern int	days_in_mon();		/* cronfuncs.c */
extern time_t	num();			/* cronfuncs.c */
extern char *	xmalloc();		/* cronfuncs.c */
extern char *	select_shell ();	/* cronfuncs.c */
extern void	el_add();		/* elm.c */
extern void	el_delete();		/* elm.c */
extern int	el_empty();		/* elm.c */
extern int *	el_first();		/* elm.c */
extern void	el_init();		/* elm.c */
extern void	el_remove();		/* elm.c */
extern int	ascandir();		/* funcs.c */
extern int	filewanted();		/* funcs.c */
extern int	sendmsg();		/* funcs.c */
extern int	allowed();		/* permit.c */
extern char *	getuser();		/* permit.c */

#endif /* __STDC__ */

/* anything below here can be changed */

#define CRONDIR		"/var/spool/cron/crontabs"
#define ATDIR		"/var/spool/cron/atjobs"
#define ACCTFILE	"/var/cron/log"
#define CRONALLOW	"/etc/cron.d/cron.allow"
#define CRONDENY	"/etc/cron.d/cron.deny"
#define ATALLOW		"/etc/cron.d/at.allow"
#define ATDENY		"/etc/cron.d/at.deny"
#define PROTO		"/etc/cron.d/.proto"
#define	QUEDEFS		"/etc/cron.d/queuedefs"
#define	NPIPE		"/etc/cron.d/NPIPE"
#define LCK_CRON	"/etc/cron.d/LCK_CRON"
#define DEFFILE		"/etc/default/cron"
#define	CROND		"/etc/cron.d"

/* definitions if not defined in DEFFILE */ 
#define BACKUP		"/var/cron/olog"
#define	LINES		100		/* # of last lines to keep in log */
#define SIZE		500000L		/* max size of log file before backup */ 

#define SHELL		"/usr/bin/sh"	/* shell to execute */

#define CTLINESIZE	1000		/* max chars in a crontab line */

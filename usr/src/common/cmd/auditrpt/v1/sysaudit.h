/*		copyright	"%c%" 	*/

#ident	"@(#)sysaudit.h	1.2"
#ident	"$Header$"

#ifndef _ACC_AUDIT_AUDIT_H	/* wrapper symbol for kernel use */
#define _ACC_AUDIT_AUDIT_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>         /* REQUIRED */
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h> 	/* REQUIRED */
#endif

#ifndef _SVC_SYSTM_H
#include <svc/systm.h> 		/* REQUIRED */
#endif

#ifndef _PROC_PROC_H
#include <proc/proc.h> 		/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h> 		/* REQUIRED */
#endif

#ifndef _ACC_MAC_MAC_H
#include <acc/mac/mac.h> 	/* REQUIRED */
#endif

#ifndef _IO_STROPTS_H
#include <io/stropts.h> 	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>          /* REQUIRED */
#include <sys/param.h> 		/* REQUIRED */
#include <sys/systm.h> 		/* REQUIRED */
#include <sys/proc.h> 		/* REQUIRED */
#include <sys/time.h> 		/* REQUIRED */
#include <sys/mac.h> 		/* REQUIRED */
#include <sys/stropts.h> 	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/* Global Function Prototypes External To AUDIT Module */
#if defined (__STDC__)
extern	int	adt_admin(int,int,int,int,long,int,int,int,int);
extern	int	adt_auditme(int,int, lid_t);
extern	int	adt_parmsset(int,int,int,pid_t,int,long,long);
extern	void	adt_auditchk(int,int *); 
extern	int	adt_cc(long,long);
extern	int	adt_dupaproc(proc_t *pp,proc_t *cp);
extern	void	adt_exit(int);
extern	void	adt_filenm(char *,int,struct vnode *);
extern	void	adt_freeaproc(proc_t *pp); 
extern	int	adt_installed(void);
extern	void	adt_kill(int,pid_t *,int);
extern	void	adt_modload(int,int,int, char *);
extern	void	adt_moduload(int,int,int);
extern	void	adt_pathupd(char *);
extern	int	adt_priv(int, int, struct cred *);
extern	void	adt_record(int,int,int *,rval_t *); 
extern	void	adt_recvfd(struct adtrecvfd *);
extern	void	adt_symlink(char *,char *);
extern	void	adtflush();

#else	/* !defined (__STDC__) */

extern	int	adt_admin();
extern	int	adt_auditme();
extern	int	adt_parmsset();

extern	void	adt_auditchk(); 	/* systrap() */
extern	int	adt_cc();		/* cc_limiter() */
extern	int	adt_dupaproc();   	/* fork() */
extern	void	adt_exit();		/* exit() */
extern	void	adt_filenm();		/* lookuppn() */
extern	void	adt_freeaproc();	/* exit() */
extern	void	adt_kill();		/* kill() dotoprocs() */
extern	int	adt_installed();	/* main() */
extern	void	adt_modload();		/* modload() */
extern	void	adt_moduload();		/* moduload() */
extern	void	adt_pathupd();		/* lookuppn() */
extern	int	adt_priv();		/* pm_denied() */
extern	void	adt_record();		/* systrap() */
extern	void	adt_recvfd();		/* strioctl() */
extern	void	adt_symlink();		/* symlink() */
extern	void	adtflush();		/* main() */

#endif		/* defined (__STDC__) */

/* record the use of privilege, when not using sys_cred */
#define ADT_PRIV(a,b,c)	\
(((audit_on) && ((c) != sys_cred)) ? adt_priv((a), (b), (c)) : 0)

/* record the administrative use of the system scheduler */
#define ADT_ADMIN(a,b,c,d,e,f,g,h,i)	(audit_on ? adt_admin((a),(b),(c),(d),\
					(e),(f),(g),(h),(i)) : 0)

/* record any modification to the scheduler's parameters */
#define ADT_PARMSSET(a,b,c,d,e,f,g)	(audit_on ? adt_parmsset((a),\
					(b),(c),(d),(e),(f),(g)) : 0)

#endif /* _KERNEL */

/* Length of string containing the "audit magic number" */
#define ADT_BYORDLEN		16

/* Define an "audit magic number" for each machine type. */
/* The current machine type must then be assigned to the */
/* ADT_BYORD define in the auditmdep.h file.             */
#define ADT_3B2_BYORD		"AR32W"
#define ADT_386_BYORD		"AR32WR"
#define ADT_XDR_BYORD		"XDR"


#ifdef _KERNEL_HEADERS

#ifndef _ACC_AUDIT_AUDITMDEP_H
#include <acc/audit/auditmdep.h>
#endif	/* _ACC_AUDIT_AUDITMDEP_H */

#elif defined(_KERNEL)

#include <sys/auditmdep.h>

#else

#include <sys/auditmdep.h>

#endif	/* _KERNEL_HEADERS */


/* return >1 if event e is to be audited in emask E, 0 otherwise. */
#define	EVENTCHK(e,E) (((unsigned int)0x80000000 >> ((e)&0x1F)) & (E)[(e)>>5])

/* set the event in the event vector to 1 (= to be audited) */
#define	EVENTADD(e,E) (E)[(e)>>5] |= ((unsigned int)0x80000000 >> ((e)&0x1F)) 

/* set the event in the event vector to 0 (= NOT to be audited) */
#define	EVENTDEL(e,E) (E)[(e)>>5] &= ~((unsigned int)0x80000000 >> ((e)&0x1F)) 

/* audit: signal sent from kernel to kick off handling program	*/
#define ADT_PROG	SIGUSR1

/* Audit event mask bit positions */
#define ADT_NULL		0
#define ADT_ACCESS		1	/* access(2) 	  */
#define ADT_ACCT_OFF		2	/* sysacct(2) 	  */
#define ADT_ACCT_ON		3	/* sysacct(2)	  */
#define ADT_ACCT_SW		4	/* sysacct(2)	  */
#define ADT_ADD_GRP		5	/* groupadd(1M)	  */
#define ADT_ADD_USR		6	/* useradd(1M)	*/
#define ADT_ADD_USR_GRP		7	/* addgrpmem(1M)  */
#define ADT_ASSIGN_LID		8	/* lvlname(1M)	  */
#define ADT_ASSIGN_NM		9	/* lvlname(1M)    */
#define ADT_AUDIT_BUF		10	/* auditbuf(2)    */
#define ADT_AUDIT_CTL		11	/* auditctl(2)	  */
#define ADT_AUDIT_DMP		12	/* auditdmp(2)    */
#define ADT_AUDIT_EVT		13	/* auditevt(2)    */
#define ADT_AUDIT_LOG		14	/* auditlog(2)	  */
#define ADT_AUDIT_MAP		15	/* auditmap(1M)   */
#define ADT_BAD_AUTH		16	/* bad passwd	  */
#define ADT_BAD_LVL		17	/* bad login lvl  */
#define ADT_CANCEL_JOB		18	/* lp		  */
#define ADT_CHG_DIR		19	/* chg_dir	  */
#define ADT_CHG_NM		20	/* rename(2)	  */
#define ADT_CHG_ROOT		21	/* chroot(2)	  */
#define ADT_CHG_TIMES		22	/* utime(2)	  */
#define ADT_COV_CHAN_1		23	/* covert channel */
#define ADT_COV_CHAN_2		24	/* covert channel */
#define ADT_COV_CHAN_3		25	/* covert channel */
#define ADT_COV_CHAN_4		26	/* covert channel */
#define ADT_COV_CHAN_5		27	/* covert channel */
#define ADT_COV_CHAN_6		28	/* covert channel */
#define ADT_COV_CHAN_7		29	/* covert channel */
#define ADT_COV_CHAN_8		30	/* covert channel */
#define ADT_CREATE		31	/* creat(2)	  */
#define ADT_CRON		32	/* cron(1M)	  */
#define ADT_DAC_MODE		33	/* chmod(2)	  */
#define ADT_DAC_OWN_GRP		34	/* chown(2)	  */
#define ADT_DATE		35	/* stime(2) adj_time(2) */
#define ADT_DEACTIVATE_LID	36	/* lvldelete(1M)  */
#define ADT_DEF_LVL		37	/* login level    */
#define ADT_DEL_NM		38	/* lvldelete(1M)  */
#define ADT_DISP_ATTR		39	/* devstat(2) fdevstat(2) */
#define ADT_EXEC		40	/* exec(2)	  */
#define ADT_EXIT		41	/* exit(2)	  */
#define ADT_FCNTL		42	/* fcntl(2)	  */
#define ADT_FILE_ACL		43	/* acl(2)         */
#define ADT_FILE_LVL		44	/* lvl_file(2)    */
#define ADT_FILE_PRIV		45	/* filepriv(2)    */
#define ADT_FORK		46	/* fork(2)	  */
#define ADT_INIT		47	/* init(1M)	  */
#define ADT_IOCNTL		48	/* ioctl(2)	  */
#define ADT_IPC_ACL		49	/* aclipc(2)	  */
#define ADT_KILL		50	/* kill(2) sigsendset(2) */
#define ADT_LINK		51	/* link(2)	  */
#define ADT_LOGIN		52	/* success login  */
#define ADT_LP_ADMIN		53	/* lp		  */
#define ADT_LP_MISC		54	/* lp misc        */
#define ADT_MISC		55	/* miscellaneous  */
#define ADT_MK_DIR		56	/* mkdir(2)	  */
#define ADT_MK_MLD		57	/* mkmld(2)	  */
#define ADT_MK_NODE		58	/* mknod(2)	  */
#define ADT_MOD_GRP		59	/* groupmod(1M)   */
#define ADT_MOD_USR		60	/* usermod(1M)	*/
#define ADT_MOUNT		61	/* mount(2) umount(2) */
#define ADT_MSG_CTL		62	/* IPC message controls	  */
#define ADT_MSG_GET		63	/* IPC message gets	  */
#define ADT_MSG_OP		64	/* IPC message operations */
#define ADT_OPEN_RD		65	/* open(2) RD_ONLY	  */
#define ADT_OPEN_WR		66	/* open(2) WR_ONLY or RDWR*/
#define ADT_PAGE_LVL		67	/* printer page	 level    */
#define ADT_PASSWD		68	/* passwd(1)	*/
#define ADT_PIPE		69	/* pipe(2)	*/
#define ADT_PM_DENIED		70	/* adt_priv()	*/
#define ADT_PROC_LVL		71	/* lvlproc(2)	*/
#define ADT_PRT_JOB		72	/* printer job	*/
#define ADT_PRT_LVL		73	/* printer level*/
#define ADT_RECVFD		74	/* receive FD	*/
#define ADT_RM_DIR		75	/* rmdir(2)	*/
#define ADT_SCHED_LK		76	/* priocntl(2)	*/
#define ADT_SCHED_RT		77	/* priocntl(2)	*/
#define ADT_SCHED_TS		78	/* priocntl(2)	*/
#define ADT_SEM_CTL		79	/* IPC semaphore controls   */
#define ADT_SEM_GET		80	/* IPC semaphore gets	    */
#define ADT_SEM_OP		81	/* IPC semaphore operations */
#define ADT_SET_ATTR		82	/* devstat(2) fdevstat(2)   */
#define ADT_SET_GID		83	/* setgid(2)	*/
#define ADT_SET_GRPS		84	/* setgroups(2)	*/
#define ADT_SET_LVL_RNG		85	/* lvlvfs(2)	*/
#define ADT_SET_PGRPS		86	/* setpgrp(2),setpgid(2) */
#define ADT_SET_SID		87	/* setsid(2)   */
#define ADT_SET_UID		88	/* setuid(2)	*/
#define ADT_SETRLIMIT		89	/* setrlimit(2)	*/
#define ADT_SHM_CTL		90	/* IPC shared-memory controls   */
#define ADT_SHM_GET		91	/* IPC shared-memory gets	*/
#define ADT_SHM_OP		92	/* IPC shared-memory operations */
#define ADT_STATUS		93	/* stat(2)	*/
#define ADT_SYM_CREATE		94	/* symlink(2)	*/
#define ADT_SYM_STATUS		95	/* symlink(2)	*/
#define ADT_TFADMIN		96	/* tfadmin(1M)	*/
#define ADT_TRUNC_LVL		97	/* lp           */
#define ADT_ULIMIT		98	/* ulimit(2)	*/
#define ADT_UMOUNT		99	/* umount(2)	*/
#define ADT_UNLINK		100	/* unlink(2)	*/
#define ADT_MODPATH		101	/* modpath(2)	*/
#define ADT_MODADM		102	/* modadm(2)	*/
#define ADT_MODLOAD		103	/* load module  */
#define ADT_MODULOAD		104	/* unload module*/

/*
 * NOTE:  adding new event types above affects the following define
 */
#define ADT_NUMOFEVTS		104	/* number of auditable events */
#define ADT_ZNUMOFEVTS		255	/* number of auditable events */

/*
 * NOTE:  number assigned to record types must be beyond mask range 
 */
#define	ADT_PROC		129	/* New process data */
#define	ADT_GROUPS		130	/* New multiple groups list */

/* length of auditable event names within the adtevt[] table */
#define ADT_EVTNAMESZ		14	

/* define for pathname processing */
#define A_SLASH			'/'

/* Default path to primary audit log file directory */
#define ADT_DEFPATH		"/var/audit"
#define ADT_DEFPATHLEN		10

/* Defines for dynamic loadable modules */
#define ADT_MOD_AUTO	1
#define ADT_MOD_DEMAND	2

/* Defines for process class specific information */
#define ADT_RT_NEW		1
#define ADT_TS_NEW		2
#define ADT_RT_PARMSSET		3
#define ADT_TS_PARMSSET		4
#define ADT_RT_SETDPTBL 	5
#define ADT_TS_SETDPTBL		6

/* cmd values for the auditbuf(2) system call */
#define ABUFGET		0x0		/* get audit buffer attributes */
#define ABUFSET		0x1		/* set audit buffer attributes */

/* cmd values for the auditctl(2) system call */
#define AUDITOFF	0x0		/* turn auditing off now */
#define AUDITON		0x1		/* turn auditing on now */
#define ASTATUS		0x2		/* get status of auditing */

/* flag values for the auditdmp(2) system call */
#define ADT_ERR_IN_PROGRESS 0x1		/* audit error being processed */

/* cmd values for the auditevt(2) system call */
#define AGETSYS		0x1		/* get system event mask */
#define ASETSYS		0x2		/* set system event mask */
#define AGETUSR		0x4		/* get user's event mask */
#define ASETME		0x8		/* set process event mask */
#define ASETUSR		0x10		/* set user's event mask */
#define ANAUDIT		0x20		/* do not audit invoking process */
#define AYAUDIT		0x40		/* audit invoking process */
#define ACNTLVL		0x80		/* get the size of the level table */
#define AGETLVL		0x100		/* get object level event mask */
#define ASETLVL		0x200		/* set object level event mask */
#define AGETME		0x400		/* get process event mask */

/* cmd values for the auditlog(2) system call */
#define ALOGGET		0x0		/* get audit log-related attributes */
#define ALOGSET		0x1		/* set audit log-related attributes */

/* flag values for the auditset(1M) command line arguments */
#define ADT_DMASK	0x1		/* display audit masks */
#define ADT_AMASK	0x2		/* all users masks */
#define ADT_UMASK	0x4		/* specific user[s] masks */
#define ADT_EMASK	0x8		/* event masks */
#define ADT_SMASK	0x10		/* system wide event mask */
#define ADT_MMASK	0x20		/* display object mask and levels*/
/* 
 * flag values for both the auditset(1M) command line arguments
 * and the auditevt(2) system call. 
 * NOTE: Gap in numbers is reserved for growth.
 */
#define ADT_OMASK	0x1000		/* object level event mask */
#define ADT_LMASK	0x2000		/* single level criteria */
#define ADT_RMASK	0x4000		/* level range criteria */
/*
 * temporary flags for trusted applications event criteria
 */
#define ADT_DZMASK	0x10000000	/* display extended event mask bits */
#define ADT_SZMASK	0x20000000	/* set extended event mask bits */

/* flags for auditlog(2) system call */
#define PPATH		0x1		/* primary log path */
#define PNODE		0x2		/* primary log node name */
#define APATH		0x4		/* alternate log path */
#define ANODE		0x8		/* alternate log node name */
#define PSIZE		0x10		/* maximum size for primary log */
#define PSPECIAL	0x20		/* primary log is character special*/
#define ASPECIAL	0x40		/* alternate log is character special */
#define LOGFILE		0x100		/* FULL pathname to audit log file */

/* flags for auditlog a_onfull */
#define ASHUT		0x1		/* shut down on log-full */
#define ADISA		0x2		/* disable auditing on log-full */
#define AALOG		0x4		/* switch to alternate on log-full */
#define APROG		0x8		/* run a program on log-full */

/* flags to check if this event is to be audited */
#define	AUDITME		0x1		/* this event is audited */
#define	AEXEMPT		0x2		/* do not audit this process */

/* flags for adt_auditme() */
#define A_SYSCHK	0x0		/* audit check in systrap */
#define A_OBJCHK	0x1		/* audit check in lookup */
#define A_IPCCHK	0x2		/* audit check in IPC code */
#define A_FDCHK		0x4		/* audit check for file descriptors */

#define ACWD		0		/* current working directory flag */
#define AROOT		1		/* current root directory flag */
#define ADT_CWDPATHLEN	128		/* current working directory length */

#define ADT_BAMSG	6		/* length of bad_auth message field */
#define ADT_LOGINMSG	"LOGIN"		/* logname message for bad_auth */
#define ADT_PASWDMSG	"PASWD"		/* password message for bad_auth */
#define ADT_AUDITMSG	"AUDIT"		/* putava message for bad_auth */
#define ADT_TTYSZ	32		/* length of real tty device field */

/* auditdmp(2) interface structure for login */
typedef struct alogrec {
	uid_t uid;			/* user's effective uid */
	gid_t gid;			/* user's effective gid */
	level_t ulid;			/* user's default MAC level */
	level_t hlid;			/* user's current (-h) MAC level */
	level_t vlid;			/* user's new default (-v) MAC level */
	char bamsg[ADT_BAMSG];		/* bad_auth error message */
	char tty[ADT_TTYSZ];		/* user's ttyname */
} alogrec_t;

/* auditdmp(2) interface structure for passwd */
typedef struct apasrec {
	uid_t nuid;			/* user's uid cmdline arg */
} apasrec_t;

#define ADT_CRONSZ	128		/* length of cron description field */

/* auditdmp(2) interface structure for cron */
typedef struct acronrec {
	uid_t uid;			/* cron user's effective uid */
	gid_t gid;			/* cron user's effective gid */
	level_t lid;			/* cron user's MAC level */
	char cronjob[ADT_CRONSZ];	/* cron job invoked for user */
} acronrec_t;


/* auditbuf(2) structure */
typedef struct abuf {	
	int vhigh;			/* audit buffer high_water_mark */
	int bsize;			/* audit buffer size */
} abuf_t;

/* auditctl(2) structure */
#define ADT_VERLEN 8
typedef struct actl {		
	int	auditon;		/* audit status variable */
	char	version[ADT_VERLEN];	/* audit version */
	long	gmtsecoff;		/* GMT offset in seconds */
} actl_t;

/* auditevt(2) structure */
typedef struct aevt {	
	adtemask_t	emask;    	/* event mask to be set or retrieved */
	uid_t		uid;      	/* user event mask is to be set */
	uint_t		flags;   	/* event mask flags */
	uint_t		nlvls;    	/* number of individual object levels */
	level_t		*lvl_minp;	/* minimum level range criteria */
	level_t		*lvl_maxp;	/* maximum level range criteria */
	level_t		*lvl_tblp;	/* pointer to object level table */
} aevt_t;

/*
 * There is ONE available audit level range(%l%l)
 * and a configurable number of levels ADT_NLVLS. 
 * 	(see /etc/master.d/audit)
 */
struct adt_lvlrange {
        level_t	a_lvlmin;		/* minimum level range */
        level_t	a_lvlmax;		/* maximum level range */
};
extern struct adt_lvlrange adt_lvlrange[];

#define ADT_MAXSEQ		999	/* maximum number of logs per day */

/* 
 * Maximum length of pathname to audit log files ADT_MAXPATHLEN = 
 * [ MAXPATHLEN - (A_SLASH + ADT_DATESZ + ADT_DATESZ + ADT_SEQSZ + ADT_NODESZ) ]
 *     1024     - (   1    +     2      +     2      +     3     +     7     )
 * 
 * Do not want to force include <limits.h> or <sys/param.h> 
 */
#define ADT_MAXPATHLEN		1009	/* Maximum length for pathnames */
#define ADT_NODESZ		7	/* length of event log node name */
#define ADT_DATESZ		2	/* length of date field */
#define ADT_SEQSZ		3	/* length of sequence number field  */

/* auditlog(2) structure */
typedef struct alog {
	int	flags;			/* log file attributes */
	int	onfull;			/* action on log-full */
	int	onerr;			/* action on error */
	int	maxsize;		/* maximum log size */
	int	seqnum;			/* log sequence number 001-999 */
	char	mmp[ADT_DATESZ];	/* current month time stamp */
	char	ddp[ADT_DATESZ];	/* current day time stamp */
	char	pnodep[ADT_NODESZ];	/* optional primary node name */
	char	anodep[ADT_NODESZ];	/* optional alternate node name */
	char	*ppathp;		/* primary log pathname */
	char	*apathp;		/* alternate log pathname */
	char	*progp;			/* program run during log switch */
	char	*defpathp;		/* default log path name */
	char	*defnodep;		/* default log node name */
	char	*defpgmp;		/* default log switch program */
	int	defonfull;		/* default action on log-full */
} alog_t;

/* auditdmp(2) structure */
typedef struct arec {
	int rtype;			/* audit record event type */
	int rsize;			/* audit record size of argp */
	int rstatus;			/* udit record event status */
	char *argp;			/* audit record data */
} arec_t;

typedef struct adtc {
 	char	*a_path;		/* pathname of cur dir */
 	uint_t	a_ref;			/* reference count */
 	uint_t	a_len;			/* current length of string */
 	uint_t	a_olen;			/* original malloc'd length */
 	char	*a_lastcomp;		/* last component of cur dir */
 	char	*a_cwdend;		/* end of current dir pathn */
} adtcwd_t;

/* System Call Function Prototypes */

#if defined (__STDC__) && !defined (_KERNEL)

extern	int	auditbuf(int, abuf_t *, int);
extern	int	auditctl(int, actl_t *, int);
extern	int	auditdmp(arec_t *, int);
extern	int	auditevt(int, aevt_t *, int);
extern	int	auditlog(int, alog_t *, int);

#endif		/* defined (__STDC__) */


#if defined (_KERNEL) || defined (_KMEMUSER)

extern	int	audit_on;

/* Values for the changed flag */
#define	ADTCHGBAS	1		/* process attributes changed */	
#define	ADTCHGGRP	2		/* multiple groups changed */

/* process audit structure */
typedef struct aproc {
	ushort		a_flags;	/* AUDITME or AEXEMPT */
	ushort		a_event;	/* auditable event number */
	unsigned long	a_seqnum;	/* event sequence number */
	int		a_changed;	/* changed flag */
	timestruc_t	a_time;		/* starting time of the event */
	adtemask_t	a_procemask;	/* process event mask */
	adtemask_t	a_useremask;	/* user event mask */
 	adtcwd_t	*a_cwd;		/* current working directory ptr */
 	adtcwd_t	*a_root;	/* chroot directory ptr */
} aproc_t;

/* audit buffer structure */
typedef struct acbufs {
        uint_t    b_inuse;		/* amount of data in buffer */
        uint_t    b_flag;		/* buffer flags */
} acbufs_t;


/* audit buffer control structure */
typedef struct abuf_ctl {	
	uint_t a_vhigh;			/* "high water mark" */
	uint_t a_bsize;			/* audit buffer size */
	uint_t a_curbuf;			/* current buffer index*/
	char *a_wptr;			/* write pointer into buffer */
	char *a_bp;			/* pointer to start of buffer */
	acbufs_t *a_cp;			/* pointer to control structures */
	char *a_ap;			/* read pointer for  write thru */
	uint_t a_asize;			/* size of buffer for write thru */
} abufctl_t;

/* 
 * flags for signalling buffer action when calling audit daemon
 * to perform some kind of write
 * 	ADTLOCK   - buffer is locked (must be)
 * 	ADTWANT   - someone is waiting for buffer to write
 * 	ADTNOBUF  - bypass buffer and take data from pointer
 * 		    and write directly to file
 * 	ADTBUFWR  - buffer is being flushed by daemon
 * 	ADTCLEAR  - called by AUDITOFF case of auditctl()
 */
#define ADTLOCK			0x001
#define ADTWANT			0x002
#define ADTNOBUF		0x004
#define ADTBUFWR		0x008
#define ADTCLEAR		0x010


/*
 * macros for manipulating buffer structures
 */
extern	uint_t			adt_nbuf;
#define ADT_LASTBUF		(adt_nbuf-1)
#define ADT_BUFPTR		adt_bufctl.a_bp
#define ADT_CTLPTR		adt_bufctl.a_cp
#define ADT_WPTR		adt_bufctl.a_wptr
#define ADT_CURBUF		adt_bufctl.a_curbuf
#define ADT_CURBUFLAG		ADT_CTLPTR[ADT_CURBUF].b_flag
#define ADT_GETBUF(wp)		(int)(((wp) - ADT_BUFPTR)/adt_bsize)
#define ADT_GETPTR(bn)		(ADT_BUFPTR + ((bn) * adt_bsize))
#define ADT_NXTBUF(bn)		(int)(((bn) == ADT_LASTBUF) ? 0 : ((bn)+1))
#define ADT_SET_BUFWR(bn)    	(ADT_CTLPTR[(bn)].b_flag |= ADTBUFWR)
#define ADT_SET_CLEAR(bn)	(ADT_CTLPTR[(bn)].b_flag |= ADTCLEAR)
#define ADT_SET_NOBUF(bn)	(ADT_CTLPTR[(bn)].b_flag |= ADTNOBUF)
#define ADT_VHIGH(bn,size)	((ADT_CTLPTR[(bn)].b_inuse + (size)) > adt_bufctl.a_vhigh)
#define ADT_BUFULL(bn,size)	((size) >= (adt_bsize - ADT_CTLPTR[(bn)].b_inuse))
#define ADT_RESET_INUSE(bn)	(ADT_CTLPTR[(bn)].b_inuse = 0)
#define ADT_INC_INUSE(bn,size)	(ADT_CTLPTR[(bn)].b_inuse += (size))

/* macros for manipulating audit log file structure */
#define ADT_LOG_VP		adt_logctl.a_vp
#define ADT_LOG_SIZEUPD(size)	(adt_logctl.a_logsize += (size))


/* audit control structure */
typedef struct actl_ctl {
	uint_t a_auditon;			/* audit status */
	char a_version[ADT_VERLEN];		/* audit version */
	long a_gmtsecoff;			/* GMT offset in seconds */
	uint_t a_seqnum;			/* next sequence number */
} actlctl_t;


/* audit log control structure */
typedef struct alog_ctl {	
	uint_t	a_flags;		/* log file attributes */
	uint_t	a_onfull;		/* action on log-full */
	uint_t	a_onerr;		/* action on error */
	uint_t	a_maxsize;		/* maximum log size */
	uint_t	a_seqnum;		/* log sequence number 001-999 */
	char	a_mmp[ADT_DATESZ];	/* current month time stamp */
	char	a_ddp[ADT_DATESZ];	/* current day time stamp */
	char	*a_pnodep;		/* optional primary node name */
	char	*a_anodep;		/* optional alternate node name */
	char	*a_ppathp;		/* primary-log pathname */
	char	*a_apathp;		/* alternate-log pathname */
	char	*a_progp;		/* log-switch user program pathname */
	char    *a_defpathp;		/* default log path name */
	char    *a_defnodep;		/* default log node name */
	char    *a_defpgmp;		/* default log switch program */
	int     a_defonfull;   	        /* default action on log-full */
	struct vnode *a_vp;		/* log vnode pointer */
	uint_t	a_logsize;		/* current log size */
	int	a_savedd;		/* saved integer value of DD */
	char	*a_logfile;		/* FULL pathname to audit log file */
} alogctl_t;



/* Get a pure record number (high 8 bits) from a sequence/record pair */
#define	EXTRACTREC(s)		(((s) & 0xff000000) >> 24)

/* Get a pure sequence number (low 24 bits) from a sequence/record pair */
#define	EXTRACTSEQ(s)		((s) & 0x00ffffff)

#endif 		/* _KERNEL || _KMEMUSER */

#endif 		/* _ACC_AUDIT_AUDIT_H */

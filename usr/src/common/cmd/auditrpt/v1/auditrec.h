/*		copyright	"%c%" 	*/

#ident	"@(#)auditrec.h	1.3"
#ident	"$Header$"

#ifndef _ACC_AUDIT_AUDITREC_H	/* wrapper symbol for kernel use */
#define _ACC_AUDIT_AUDITREC_H	/* subject to change without notice */

#ifdef _KERNEL_HEADERS

#ifndef _ACC_AUDIT_AUDIT_H
#include <acc/audit/audit.h>	/* REQUIRED */
#endif

#ifndef _ACC_MAC_MAC_H
#include <acc/mac/mac.h>	/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>		/* REQUIRED */
#endif

#ifndef _SVC_UTSNAME_H
#include <svc/utsname.h>	/* REQUIRED */
#endif

#ifndef _ACC_PRIV_PRIVILEGE_H
#include <acc/priv/privilege.h> /* REQUIRED */
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h>		/* REQUIRED */
#endif

#ifndef _FS_FCNTL_H
#include <fs/fcntl.h>		/* REQUIRED */
#endif

#ifndef _SVC_RESOURCE_H
#include <svc/resource.h>	/* REQUIRED */
#endif

#ifndef _UTIL_MOD_MOD_H
#include <util/mod/mod.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include "sysaudit.h"		/* REQUIRED */
#include <sys/mac.h>		/* REQUIRED */
#include <sys/vnode.h>		/* REQUIRED */
#include <sys/utsname.h>	/* REQUIRED */
#include <sys/privilege.h> 	/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/fcntl.h>		/* REQUIRED */
#include <sys/resource.h>	/* REQUIRED */
#include <sys/mod.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/* kernel audit record types */
#define FILENAME_R	1
#define FORK_R		2
#define FILE_R		3
#define CMN_R		4
#define EXEC_R		5
#define SETUID_R	6
#define INIT_R		7
#define KILL_R		8
#define CHOWN_R		9
#define CHMOD_R		10
#define IPCR_R		11
#define TIME_R		12
#define SETGRPS_R	13
#define SETPGRP_R	14
#define LOGIN_R		15
#define SYS_R		16
#define PRIV_R		17
#define USER_R		18
#define ACL_R		19
#define MAC_R		20
#define DEV_R		21
#define ABUF_R		22
#define ACTL_R		23
#define AEVT_R		24
#define ALOG_R		25
#define	PROC_R		26
#define	GROUPS_R	27
#define	PLOCK_R		28
#define MOUNT_R		29
#define PASSWD_R	30
#define CRON_R		31
#define FCHDIR_R	32
#define FCHMOD_R	33
#define FCHOWN_R	34
#define FMAC_R		35
#define FSTAT_R		36
#define FDEV_R		37
#define RECVFD_R	38
#define CC_R		39
#define FPRIV_R		40
#define PIPE_R		41
#define IOCTL_R		42
#define FCNTL_R		43
#define FCNTLK_R	44
#define ADMIN_R		45
#define PARMS_R		46
#define ADMP_R		47
#define RLIM_R		48
#define MCTL_R		49
#define ZMISC_R		50
#define MODPATH_R	51
#define MODADM_R	52
#define MODLD_R		53

#define A_TBL		100
#define A_FILEID	101
#define A_GIDMAP	102
#define A_IDMAP		103
#define A_CLASSMAP	104
#define A_TYPEMAP	105
#define A_LVLMAP	106
#define A_CATMAP	107
#define A_PRIVMAP	108
#define A_SYSMAP	109

#define ADT_UID		1
#define ADT_SID		2
#define ADT_GID		3
#define ADT_PGRP	4

	
#define	SEMCTL	0
#define	SEMGET	1
#define	SEMOP	2
#define	SHMAT	0
#define	SHMCTL	1
#define	SHMDT	2
#define	SHMGET	3
#define	MSGGET	0
#define	MSGCTL	1
#define	MSGRCV	2
#define	MSGSND	3

/* round up size to be a multiple of NBPW to obtain a full word */
#define ROUND2WORD(size)     (roundup(size,NBPW))

/* public functions */
extern void adt_filenm();

/*
 *	COMMON record:
 */
typedef struct cmn_rec {
	ushort		c_rtype;	/* record type */
	ushort		c_event;	/* event subtype */
	unsigned long	c_size;		/* record size */
	unsigned long	c_seqnm;	/* sequence # */
	pid_t		c_pid;		/* process id */
	time_t		c_time;		/* time */
	long		c_status;	/* success / failure */
} cmnrec_t;


/*
 *	TRAILER record: 
 */
typedef struct trail_rec  {
	unsigned long	t_size;		/* record size */
	ushort		t_rtype;	/* record type */
	ushort		t_event;	/* event type */
} trailrec_t;


/* 
 *	FILENAME record : contains the pathname and pid
 */
struct	fname_r {
	vtype_t	f_type;
	dev_t	f_fsid;
	dev_t	f_dev;
	ino_t	f_inum;
	lid_t	f_lid;
};
typedef struct filnm_rec {
	cmnrec_t	cmn;		/* common block*/
	struct	fname_r	spec;		/* type specific data */
} filnmrec_t;


/*
 *	USER record:
 */
typedef struct user_rec {
	cmnrec_t	cmn;		/* common block */
} userrec_t;


/*
 *	FORK record : dumped once for each NEW PROCESS
 */
struct	fork_r {
	pid_t		f_cpid;	/* child process id */
};
typedef struct fork_rec {
	cmnrec_t	cmn;		/* common block */
	struct	fork_r	spec;		/* type specific data */
} forkrec_t;


/*
 *	PROC record : dumped whenever the state of a process changes
 */
struct	proc_r {
	lid_t		pr_lid;
	uid_t		pr_uid;
	gid_t		pr_gid;
	uid_t		pr_ruid;
	gid_t		pr_rgid;
	long		pr_sid;
};
typedef	struct	proc_rec {
	cmnrec_t	cmn;		/* common block */
	struct	proc_r	spec;		/* type specific data */
} procrec_t;

/*
 *	GROUPS record : dumped whenever multiple groups are added to a process
 */
struct	groups_r {
	long		gr_ngroups;
};
typedef	struct	groups_rec {
	cmnrec_t	cmn;		/* common block */
	struct	groups_r spec;		/* type specific data */
	gid_t		data[1];	/*variable list of groups*/
} grprec_t;


/*
 *	KILL record:
 */
struct	kill_r {
	int		k_sig;		/* signal sent */
	int		k_entries;	/* number of pids */
};
typedef struct kill_rec {
	cmnrec_t	cmn;		/* common block */
	struct	kill_r	spec;		/* type specific data */
} killrec_t;


/*
 *	FILE record: dumped by all file-bound syscalls (open,close,link ...)
 */
struct file_r {
	int	f_fd;			/* file descriptor */
};
typedef struct file_rec {
	cmnrec_t	cmn;		/* common block */
	struct	file_r	spec;		/* type specific data */
} filerec_t;


/*
 *	FCHDIR record:
 */
struct fchdir_r {
	int	f_fd;			/* file descriptor */
};
typedef struct fchdir_rec {
	cmnrec_t	cmn;		/* common block */
	struct fchdir_r spec;		/* type specific data */
} fchdirec_t;


/*
 *	CHMOD record:
 */
struct	chmod_r {
	mode_t		c_nmode;	/* new mode */
};
typedef struct chmod_rec {
	cmnrec_t	cmn;		/* common block */
	struct chmod_r	spec;		/* type specific data */
} chmodrec_t;


/*
 *	FCHMOD record:
 */
struct fchmod_r {
	struct	chmod_r	f_chmod;	/* chmod struct(new mode) */
	int	f_fd;			/* file descriptor */
};
typedef struct fchmod_rec {
	cmnrec_t	cmn;		/* common block */
	struct fchmod_r spec;		/* type specific data */
} fchmodrec_t;


/*
 *	CHOWN record:
 */
struct	chown_r {
	id_t		c_uid;		/* new owner */
	id_t		c_gid;		/* new group */
};
typedef struct chown_rec {
	cmnrec_t	cmn;		/* common block */
	struct chown_r	spec;		/* type specific data */
} chownrec_t;


/*
 *	FCHOWN record:
 */
struct fchown_r {
	struct	chown_r	f_chown;	/* chown stuct(new uid,gid) */
	int	f_fd;			/* file descriptor */
};
typedef struct fchown_rec {
	cmnrec_t	cmn;		/* common block */
	struct fchown_r spec;		/* type specific data */
} fchownrec_t;


/*
 *	FDEVSTAT record:
 */
struct	fdev_r {
	struct	devstat devstat;
	int	l_fd;			/* file descriptor */
};
typedef struct fdev_rec {
	cmnrec_t	cmn;		/* common block */
	struct	fdev_r	spec;		/* type specific data */
} fdevrec_t;


/*
 *	LVLFILE record:
 */
struct mac_r {
	lid_t		l_lid;		/* MAC level id */
};
typedef struct mac_rec {
	cmnrec_t	cmn;		/* common block */
	struct	mac_r	spec;		/* type specific data */
} macrec_t;


/*
 *	FLVLFILE record:
 */
struct fmac_r {
	struct	mac_r	l_mac;		/* lvlfile struct(MAC lid) */
	int	l_fd;			/* file descriptor */
};
typedef struct fmac_rec {
	cmnrec_t	cmn;		/* common block */
	struct	fmac_r	spec;		/* type specific data */
} fmacrec_t;


/*
 *	FSTAT & FXSTAT records:
 */
struct fstat_r {
	int	s_fd;			/* file descriptor */
};
typedef struct fstat_rec {
	cmnrec_t	cmn;		/* common block */
	struct	fstat_r	spec;		/* type specific data */
} fstatrec_t;


/*
 *	RECVFD record:
 */
struct	recvfd_r {
	pid_t	r_spid;			/* senders pid */
	int	r_sfd;			/* senders file descriptor */
	pid_t	r_rpid;			/* receivers pid */
	int	r_rfd;			/* receivers file descriptor */
};
typedef struct recvfd_rec {
	cmnrec_t	cmn;		/* common block */
	struct recvfd_r spec;		/* type specific data */
} recvfdrec_t;


/*
 *	TIME record:
 */
struct	time_r {
	long		t_time;/* access time -- passed from calling routine */
};
typedef struct time_rec {
	cmnrec_t	cmn;		/* common block */
	struct	time_r	spec;		/* type specific data */
} timerec_t;


/*
 *	EXEC record:
 */
typedef struct exec_rec {
	cmnrec_t	cmn;		/* common block */
} execrec_t;


/*
 *	SETUID record: setuid(2) and setgid(2)
 */
struct	setid_r {
	id_t		s_nid;		/* new id */
	int		s_type;		/* type of action ADT_GID or ADT_UID */
};
typedef struct setuid_rec {
	cmnrec_t	cmn;		/* common block */
	struct	setid_r	spec;		/* type specific data */
} setuidrec_t;


/*
 *	SETPGRP record: setpgid(2), setpgrp(2) and setsid(2)
 */
struct	setpgrp_r {
	int		s_flag;		/* 1=SETPGRP, 3=SETSID, 5=SETPGID */
	int		s_pid;		/* process id */
	int		s_pgid;		/* process group id */
};
typedef struct setpgrp_rec {
	cmnrec_t	cmn;		/* common block */
	struct setpgrp_r spec;		/* type specific data */
} setpgrprec_t;


/*
 *	IPC record:
 */
struct ipc_r {
	int		i_op;		/* opcode */
	int		i_id;		/* id */
	int		i_flag;		/* flag */
	int		i_cmd;		/* cmd */
};
typedef struct ipc_rec {
	cmnrec_t	cmn;		/* common block */
	struct	ipc_r	spec;		/* type specific data */
} ipcrec_t;


/* 
 *	LOGIN record:
 */
struct login_r {
	alogrec_t	l_alogrec;
};
typedef struct login_rec {
	cmnrec_t	cmn;		/* common block */
	struct	login_r	spec;		/* type specific data */
} loginrec_t;


/* 
 *	PASSWD record:
 */
struct passwd_r {
	apasrec_t	p_apasrec;
};
typedef struct passwd_rec {
	cmnrec_t	 cmn;		/* common block */
	struct	passwd_r spec;		/* type specific data */
} passwdrec_t;


/* 
 *	CRON record:
 */
struct cron_r {
	acronrec_t	c_acronrec;
};
typedef struct cron_rec {
	cmnrec_t	cmn;		/* common block */
	struct cron_r spec;		/* type specific data */
} cronrec_t;


/*
 *	INIT record:
 */
typedef struct init_rec {
	cmnrec_t	cmn;		/* common block */
} initrec_t;


/*
 *	IOCTL record:
 */
struct	ioctl_r {
	int	i_fdes;			/* file descriptor */
	int	i_cmd;			/* command id */
	int	i_flag;			/* flags found for file table entry */
};
typedef struct ioctl_rec {
	cmnrec_t	cmn;		/* common block */
	struct	ioctl_r	spec;		/* type specific data */
} ioctlrec_t;

/*
 *	FCNTL record:
 */
struct	fcntl_r {		/* cmd = F_DUPFD, F_SETFD or F_SETFL */
	int	f_fdes;		/* file descriptor */
	int	f_cmd;		/* command id */
	int 	f_arg;		/* dupfd, close_on_exec  or status flag */
};
typedef struct fcntl_rec {
	cmnrec_t	cmn;	/* common block */
	struct	fcntl_r	spec;	/* type specific data */
} fcntlrec_t;

struct	fcntlk_r {		/* cmd = F_ALLOCSP, F_FREESP, F_SETLK
					 F_SETLKW,  F_RSETLK or F_RSETLKW */
	int	f_fdes;		/* file descriptor */
	int	f_cmd;		/* command id */
	flock_t	f_flock;	/* File segment locking set data type */
};
typedef struct fcntlk_rec {
	cmnrec_t 	 cmn;		/* common block */
	struct	fcntlk_r spec;		/* type specific data */
} fcntlkrec_t;


/*
 *	RLIMIT record:
 */
struct	rlim_r {
	int		r_rsrc;		/* resource */
	rlim_t		r_soft;		/* current soft limit */
	rlim_t		r_hard;		/* current hard limit */
};
typedef struct rlim_rec {
	cmnrec_t	cmn;		/* common block */
	struct	rlim_r	spec;		/* type specific data */
} rlimrec_t;

/*
 *	ULIMIT record:
 */
struct	sys_r {
	int		s_cmd;		/* command id */
	long		s_arg;		/* newlimit */
};
typedef struct sys_rec {
	cmnrec_t	cmn;		/* common block */
	struct	sys_r	spec;		/* type specific data */
} sysrec_t;


/*
 *	PRIVILEGE record:
 */
struct	priv_r {
	int		p_scall;	/* syscall # */
	int		p_req;		/* requested privilege */
	priv_t		p_max;		/* current maximum privilege set*/
};
typedef struct priv_rec {
	cmnrec_t	cmn;		/* common block */
	struct	priv_r	spec;		/* type specific data */
} privrec_t;


/*
 *	ACL record:
 */
struct	acl_r	{
	int		a_nentries;	/* number of entries */
	int		a_ityp;		/* for ipc identification */
	int		a_iid;		/* for ipc identification */
};
typedef struct acl_rec {
	cmnrec_t	cmn;		/* common block */
	struct	acl_r	spec;		/* type specific data */
	/* array of acls follows here (size includes this)        */
	/* acl format						 */
	/* int a_type;   USER/USER_OBJ/GROUP/GROUP_OBJ/OTHER_OBJ */
	/* uid_t a_id; 						 */
	/* short a_perm; 					 */
} aclrec_t;

/* acl structures at end of acl record, used by auditrpt */
typedef struct tmpacl{
	int   ttype;
	uid_t tid;
	short tperm;
}tacl_t;

/*
 *	DEVSTAT record:
 */
struct	dev_r {
	struct	devstat devstat;
};
typedef struct dev_rec {
	cmnrec_t	cmn;		/* common block */
	struct	dev_r	spec;		/* type specific data */
} devrec_t;


/*
 *	AUDITBUF record:
 */
struct	abuf_r	{
	int	 a_vhigh;	
};
typedef struct abufsys_rec {
	cmnrec_t	cmn;
	struct	abuf_r	spec;
} abufsysrec_t;


/*
 *	AUDITCTL record:
 */
struct	actl_r {
	int	 a_cmd;	
};
typedef struct actlsys_rec {
	cmnrec_t	cmn;
	struct	actl_r	spec;
} actlsysrec_t;


/*
 *	AUDITEVT record: This structure is similar to the aevt_t structure in
 *			 audit.h.  It shoud be kept in sync, fixed length fields.
 */
struct	aevt_r {
	int		cmd;	  /* command value passed to system call */
	adtemask_t	emask;    /* event mask to be set or retrieved */
	uid_t		uid;      /* user id whose event mask is to be set */
	uint		flags;    /* event mask flags */
	uint		nlvls;    /* number of individual object levels */
	level_t		lvlmin; /* minimum specified level range criteria */
	level_t		lvlmax; /* maximum specified level range criteria */
};
typedef struct aevtsys_rec {
	cmnrec_t	cmn;
	struct	aevt_r	spec;
} aevtsysrec_t;


/*
 *	AUDITLOG record: This structure is similar to the aevt_t structure in
 *			 audit.h.  It shoud be kept in sync, fixed length fields.
 */
struct alog_r {
	uint	flags;			/* log file attributes */
	uint	onfull;			/* action on log-full */
	uint	onerr;			/* action on error */
	uint	maxsize;		/* maximum log size */
	uint	seqnum;			/* log sequence number 001-999 */
	char	mmp[ADT_DATESZ+1];	/* current month time stamp */
	char	ddp[ADT_DATESZ+1];	/* current day time stamp */
	char	pnodep[ADT_NODESZ+1];   /* optional primary node name */
	char	anodep[ADT_NODESZ+1];   /* optional alternate node name */
	char	ppathp[MAXPATHLEN+1];	/* primary log pathname */
	char	apathp[MAXPATHLEN+1];	/* alternate log pathname */
	char	progp[MAXPATHLEN+1];	/* program run during log switch */
};
typedef struct alogsys_rec {
	cmnrec_t 	cmn;
	struct	alog_r	spec;
} alogsysrec_t;

/*
 *	AUDITDMP record:  only records auditdmp(2) failures
 */
struct admp_r {
	int	a_event;		/* event attempted to be dump */
	int	a_status;		/* status of attempted event */
};
typedef struct admp_rec {
	cmnrec_t	cmn;
	struct admp_r	spec;
} admprec_t;

/*
 *	MEMCNTL record:
 */
struct	mctl_r	{
	int	 m_attr;	
};
typedef struct mctl_rec {
	cmnrec_t cmn;
	struct mctl_r	 spec;
} mctlrec_t;

/*
 *	PLOCK record:
 */
struct	plock_r	{
	int	 p_op;	
};
typedef struct plock_rec {
	cmnrec_t cmn;
	struct plock_r	 spec;
} plockrec_t;


/*
 *	ID RECORD record:
 */
struct	id_r {
	char	i_aver[ADT_VERLEN];	/* audit version # */
	char	i_mmp[ADT_DATESZ+1];	/* audit month created */
	char	i_ddp[ADT_DATESZ+1];	/* audit day created */
	int	i_mac;			/* 1=mac_installed */
};
typedef struct id_rec {
	cmnrec_t	cmn;		/* common block */
	struct	id_r	spec;		/* type specific data */
} idrec_t;


/*
 *	MOUNT event record:
 */
struct	mount_r	{
	int	 m_flags;	
};
typedef struct mount_rec {
	cmnrec_t cmn;
	struct mount_r	 spec;
} mountrec_t;

/*
 *	COVERT CHANNEL record:
 */
struct cc_r {
	long		cc_event;	/* CC specific event */
	long		cc_bps;		/* bits per second */
};

typedef struct cc_rec {
	cmnrec_t	cmn;		/* common record */
	struct cc_r	spec;		/* specific data */
} ccrec_t;

/*
 *	FILEPRIV record:
 */
struct fpriv_r {
	int	f_count;		/* number of privileges */
};
typedef struct fpriv_rec {
	cmnrec_t	cmn;		/* common record */
	struct fpriv_r	spec;		/* specific data */
} fprivrec_t;

/*
 *	Unamed PIPE record:
 */
struct pipe_r {
	int	p_fd0;			/* parent FD */
	int	p_fd1;			/* child FD */
};
typedef struct pipe_rec {
	cmnrec_t	cmn;		/* common record */
	struct pipe_r	spec;		/* specific data */
} piperec_t;

/*
 *	SETGRPS record:
 */
struct setgroup_r {
	int	s_ngroups;		/* number of supplementary groups */
};
typedef struct setgroup_rec {
	cmnrec_t	  cmn;		/* common record */
	struct setgroup_r spec;		/* specific data */
	/* list of gids follows here */
} setgrouprec_t;

/*
 *	ADMIN_R record: used by rt_admin() and ts_admin() via adt_admin()
 */
struct	admin_r {
	int	a_func;		/* internal function */
	int	a_globpri;	/* global (class independent) priority */
	long	a_quantum;	/* time quantum given to procs at this level */
	short	a_tqexp;	/* ts_umdpri assigned when proc at this level
					exceeds its time quantum */
	short	a_slpret;	/* ts_umdpri assigned when proc at this level					returns to usermode after sleeping */
	short	a_maxwait;	/* bumped to ts_lwait if more than ts_maxwait
					secs elapse before receiving full quantum */
	short	a_lwait;	/* ts_umdpri assigned if ts_dispwait exceeds ts_maxwait */
						
};
typedef struct admin_rec {
	cmnrec_t	cmn;		/* common block */
	struct	admin_r	spec;		/* type specific data */
} adminrec_t;

/*
 *	PARMSSET_R record: used by rt_enterclass(), rt_parmsset(),
 *		ts_enterclass() and ts_parmsset() via adt_parmsset()
 */
struct	parms_r {
	int	p_func;		/* internal function */
	pid_t	p_procid;	/* process id */
	short	p_upri;		/* user priority */
	long	p_uprisecs;	/* uprilim for ADT_SCHED_TS */
				/* tqsecs for ADT_SCHED_RT */
	long	p_tqnsecs;	/* ZERO for ADT_SCHED_TS */
				/* tqnsecs for ADT_SCHED_RT */
};
typedef struct parms_rec {
	cmnrec_t	cmn;	/* common block */
	struct	parms_r	spec;	/* type specific data */
} parmsrec_t;

/*
 *	ZMISC record:	Trusted Applications
 */
typedef struct zmisc_rec {
	cmnrec_t	cmn;	/* common block */
} zmiscrec_t;

/*
 *	MODPATH_R record
 */
typedef struct modpath_rec {
	cmnrec_t	cmn;		/* common block */
} modpath_t;

/*
 *	MODADM_R record
 */
struct modadm_r {
	unsigned int type;	/* module type */
	unsigned int cmd;	/* command */
	int typedata;		/* used for CDEV and BDEV module types*/
};
typedef struct modadm_rec {
	cmnrec_t	cmn;		/* common block */
	struct	modadm_r spec;	        /* type specific data */
} modadm_t;

/*
 *	MODLD_R record
 */
struct modld_r {
	int action;	/* action - AUTO or DEMAND */
	int id;		/* module id */
};

typedef struct modld_rec {
	cmnrec_t	cmn;	/* common block */
	struct	modld_r spec;	/* type specific data */
}modld_t;


/* constants for sizes of audit record structures */
#define SIZ_ABUF 	sizeof(struct abuf_r)
#define SIZ_ACL 	sizeof(struct acl_r)
#define SIZ_ACTL 	sizeof(struct actl_r)
#define SIZ_ADMIN 	sizeof(struct admin_r)
#define SIZ_ADMP	sizeof(struct admp_r)
#define SIZ_AEVT 	sizeof(struct aevt_r)
#define SIZ_ALOG 	sizeof(struct alog_r)
#define SIZ_CC		sizeof(struct cc_r)
#define SIZ_CHMOD 	sizeof(struct chmod_r)
#define SIZ_CHOWN 	sizeof(struct chown_r)
#define SIZ_CRON 	sizeof(struct cron_r)
#define SIZ_DEV 	sizeof(struct dev_r)
#define SIZ_FCHDIR 	sizeof(struct fchdir_r)
#define SIZ_FCHMOD 	sizeof(struct fchmod_r)
#define SIZ_FCHOWN 	sizeof(struct fchown_r)
#define SIZ_FCNTL 	sizeof(struct fcntl_r)
#define SIZ_FCNTLK 	sizeof(struct fcntlk_r)
#define SIZ_FDEV 	sizeof(struct fdev_r)
#define SIZ_FILE 	sizeof(struct file_r)
#define SIZ_FMAC 	sizeof(struct fmac_r)
#define SIZ_FNAME 	sizeof(struct fname_r)
#define SIZ_FORK 	sizeof(struct fork_r)
#define SIZ_FPRIV 	sizeof(struct fpriv_r)
#define SIZ_FSTAT 	sizeof(struct fstat_r)
#define SIZ_GROUPS 	sizeof(struct groups_r)
#define SIZ_ID		sizeof(struct id_r)
#define SIZ_IPC 	sizeof(struct ipc_r)
#define SIZ_IOCTL 	sizeof(struct ioctl_r)
#define SIZ_KILL 	sizeof(struct kill_r)
#define SIZ_LOGIN 	sizeof(struct login_r)
#define SIZ_MAC 	sizeof(struct mac_r)
#define SIZ_MODADM 	sizeof(struct modadm_r)
#define SIZ_MODLD	sizeof(struct modld_r)
#define SIZ_MOUNT 	sizeof(struct mount_r)
#define SIZ_PARMS 	sizeof(struct parms_r)
#define SIZ_PIPE 	sizeof(struct pipe_r)
#define SIZ_PLOCK 	sizeof(struct plock_r)
#define SIZ_PRIV 	sizeof(struct priv_r)
#define SIZ_PROC 	sizeof(struct proc_r)
#define SIZ_RECVFD 	sizeof(struct recvfd_r)
#define SIZ_SETGROUP 	sizeof(struct setgroup_r)
#define SIZ_SETID 	sizeof(struct setid_r)
#define SIZ_SETPGRP 	sizeof(struct setpgrp_r)
#define SIZ_PASSWD 	sizeof(struct passwd_r)
#define SIZ_SYS 	sizeof(struct sys_r)
#define SIZ_TIME 	sizeof(struct time_r)
#define SIZ_RLIM	sizeof(struct rlim_r)
#define SIZ_MCTL	sizeof(struct mctl_r)

#define SIZ_CMNREC	sizeof(cmnrec_t)
#define SIZ_TRAILREC	sizeof(trailrec_t)
#define SIZ_USERREC	sizeof(userrec_t)
#define SIZ_LOGINREC	sizeof(loginrec_t)
#define SIZ_PASREC	sizeof(passwdrec_t)
#define SIZ_CRONREC	sizeof(cronrec_t)
#define SIZ_ZMISCREC	sizeof(zmiscrec_t)

#endif	/* defined(_KERNEL) || defined(_KMEMUSER) */

#endif	/* _ACC_AUDIT_AUDITREC_H */


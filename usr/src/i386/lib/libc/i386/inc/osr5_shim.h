#ifndef _OSR5_SHIM_H_
#define	_OSR5_SHIM_H_
#ident	"@(#)libc-i386:inc/osr5_shim.h	1.7"

/* "shim" code for Gemini on OpenServer.
 * This code translates various values and data types between
 * those given by a Gemini program and those expected by
 * the OpenServer kernel.  It also translates values and
 * data types returned by the OpenServer kernel to those
 * expected by the Gemini program.
 */

/* some OSR5 system call numbers */
#define _SCO_ACCESS	33
#define _SCO_EACCESS	9512
#define _SCO_FCNTL	62
#define _SCO_FPATHCONF	12072
#define _SCO_FXSTAT	125
#define	_SCO_GETGROUPS	11048
#define _SCO_INFO	12840
#define _SCO_IOCTL	54
#define _SCO_LXSTAT	124
#define _SCO_MKNOD	14
#define _SCO_MMAP	115
#define _SCO_MSGSYS	49
#define _SCO_PATHCONF	11816
#define _SCO_PTRACE	26
#define	_SCO_SELECT	9256
#define	_SCO_SEMSYS	53
#define	_SCO_SETGROUPS	11304
#define _SCO_SHMSYS	52
#define _SCO_SIGPENDING	10536
#define _SCO_SIGPROCMASK	10280
#define _SCO_SIGSUSPEND	10792
#define _SCO_SYSCONF	11560
#define _SCO_WAITID	107
#define _SCO_XSTAT	123

/* old dev_t invalid value */
#define O_NODEV (ushort_t)-1

/* used by getgroups and setgroups */
#define _SCO_NGROUPS_MAX	8

/* sco version id for xstat/lsxtat/fxstat */
#define _SCO_STAT_VER	51

/* definitions for semaphore/shared memory/message commands */
#define	_SCO_SEMCTL		0
#define _SCO_SHMCTL		1
#define	_SCO_MSGCTL		1
#define _SCO_IPC_RMID		0
#define _SCO_IPC_SET		1
#define _SCO_IPC_STAT		2

/* definition for fcntl */
#define _SCO_F_GETLK		5	/* SVR3 Get file lock */

/* definitions for sa_flags member of struct sigaction */
#define _SCO_SA_NOCLDSTOP	0x00001
#define _SCO_SA_ONSTACK		0x00200
#define _SCO_SA_NOCLDWAIT	0x10000

/* definitions for the first argument to sigprocmask */
#define _SCO_SIG_SETMASK	0
#define _SCO_SIG_BLOCK		1
#define _SCO_SIG_UNBLOCK	2

struct osr5_ipc_perm {
        ushort_t	uid;    /* owner's user id */
        ushort_t	gid;    /* owner's group id */
        ushort_t	cuid;   /* creator's user id */
        ushort_t	cgid;   /* creator's group id */
        ushort_t	mode;   /* access modes */
        ushort_t	seq;    /* slot usage sequence number */
        long		key;    /* key */
};

struct osr5_semid_ds {
        struct osr5_ipc_perm sem_perm; /* operation permission struct */
        struct sem	*sem_base; /* ptr to first semaphore in set */
        ushort_t 	sem_nsems;/* # of semaphores in set */
        char		fill[2];  /* struct lockb not used in UW */
        time_t		sem_otime;/* last semop time */
        time_t		sem_ctime;/* last change time */
};


struct osr5_shmid_ds {
	struct osr5_ipc_perm shm_perm;       /* operation permission struct */
        int		shm_segsz;      /* size of segment in bytes */
        char            *__pad1;        /* for namespace protection */
        char            __pad2[4];      /* for swap compatibility */

        short           shm_lpid;       /* pid of last shmop */
        short           shm_cpid;       /* pid of creator */

        ushort_t        shm_nattch;     /* used only for shminfo */
        ushort_t 	shm_cnattch;    /* used only for shminfo */

        time_t          shm_atime;      /* last shmat time */
        time_t          shm_dtime;      /* last shmdt time */
        time_t          shm_ctime;      /* last change time */
};

struct msg;

struct osr5_msqid_ds {
	struct osr5_ipc_perm	msg_perm; /* operation permission struct */
	struct msg	*msg_first;	/* ptr to first message on q */
	struct msg	*msg_last;	/* ptr to last message on q */
	ushort_t 	msg_cbytes;	/* current # bytes on q */
	ushort_t	msg_qnum;	/* # of messages on q */
	ushort_t	msg_qbytes;	/* max # of bytes on q */
	short		msg_lspid;	/* pid of last msgsnd */
	short		msg_lrpid;	/* pid of last msgrcv */
	time_t		msg_stime;	/* last msgsnd time */
	time_t		msg_rtime;	/* last msgrcv time */
	time_t		msg_ctime;	/* last change time */
};

typedef long    osr5_sigset_t;

/* definitions for sigaction */
struct  osr5_sigaction {
        union{
#if defined(__STDC__) && !defined(_NO_PROTOTYPE)
                void    (*_sa_handler)(int);    /* signal handler */
                void    (*_sa_sigaction)(int, siginfo_t *, void *);     /* signa
l handler */
#else
                void    (*_sa_handler)();       /* signal handler */
                void    (*_sa_sigaction)();     /* signal handler */
#endif /* defined(__STDC__) && !defined(_NO_PROTOTYPE) */
        } _sa_function;
        osr5_sigset_t sa_mask;               /* signal mask to apply */
        int     sa_flags;               /* see signal options below */
};

/* definitions for uname */
struct osr5_utsname {
        char    sysname[9];
        char    nodename[9];
        char    release[9];
        char    version[9];
        char    machine[9];
};

/* definitions for stat/lstat/xstat */
#define _ST_FSTYPSZ 16
struct osr5_st_stat32 {              /* New 32-bit stat (always defined)     */
        short   st_dev;         /* id of device containing dir entry    */
        long    st_pad1[3];
        uint_t st_ino;         /* i-node number (32 bits-worth)        */
        ushort_t  st_mode;        /* file mode (permissions and type)     */
        short st_nlink;       /* number of links                      */
        ushort_t   st_uid;         /* user id of the file's owner          */
        ushort_t   st_gid;         /* group id of the file's owner         */
        short   st_rdev;        /* id of block/character special file   */
        long    st_pad2[2];
        off_t   st_size;        /* file size in bytes                   */
        long    st_pad3;
        time_t  st_atime;       /* time of last access                  */
        time_t  st_mtime;       /* time of last data modification       */
        time_t  st_ctime;       /* time of last file status change      */
        long    st_blksize;     /* preferred I/O block size in bytes    */
        long    st_blocks;      /* number st_blksize blocks allocated   */
        char    st_fstype[_ST_FSTYPSZ]; /* containing filesystem's type */
        long    st_pad4[7];
        long    st_sco_flags;   /* flags (_STAT_SCO_*)                  */
};

/* definitions for sysconf */
#define _SCO_SC_ARG_MAX     0       /* index to ARG_MAX */
#define _SCO_SC_CHILD_MAX   1       /* index to CHILD_MAX */
#define _SCO_SC_CLK_TCK     2       /* index to CLK_TCK */
#define _SCO_SC_NGROUPS_MAX 3       /* index to NGROUPS_MAX */
#define _SCO_SC_OPEN_MAX    4       /* index to OPEN_MAX */
#define _SCO_SC_JOB_CONTROL 5       /* index to _POSIX_JOB_CONTROL */
#define _SCO_SC_SAVED_IDS   6       /* index to _POSIX_SAVED_IDS */
#define _SCO_SC_VERSION     7       /* index to _POSIX_VERSION */
#define _SCO_SC_PASS_MAX    8       /* index to _PASS_MAX */
#define _SCO_SC_XOPEN_VERSION 9     /* index to _XOPEN_VERSION */
#define _SCO_SC_STREAM_MAX          21      /* index to STREAM_MAX */
#define _SCO_SC_PAGESIZE            34      /* index to PAGESIZE */
#define _SCO_LOGNAME_MAX	8
#define _SCO_STREAM_MAX		128
#define _SCO_BC_BASE_MAX	2147483647 /* INT_MAX */
#define _SCO_TZNAME_MAX		50
#define _SCO_SC_IOV_MAX		37
#define _SCO_SI86MEM		65

/* definitions for pathconf/fpathconf */
#define _SCO_PC_LINK_MAX	0
#define _SCO_MAX_CANON		1
#define _SCO_MAX_INPUT		2
#define _SCO_NAME_MAX		3
#define _SCO_PATH_MAX		4
#define _SCO_PIPE_BUF		5
#define _SCO_CHOWN_RESTRICTED	6
#define _SCO_NO_TRUNC		7
#define _SCO_VDISABLE		8

/* definitions for waitid */
#define _SCO_WSTOPPED	0004	
#define _SCO_WCONTINUED	0010
#define _SCO_WTRACED	0020	
#define _SCO_WNOWAIT	0200
#define _SCO_WEXITED	0400

/* definitions for waitpid */
#define	_SCO_WNOHANG	0001	
#define	_SCO_WUNTRACED	0002	

#define	_SCO_SI_MAXSZ	128
#define	_SCO_SI_PAD		((SI_MAXSZ / sizeof(int)) - 3)

/* osr5 siginfo struct */
typedef	struct	osr5_siginfo	{
	int	si_signo;		/* signal from signal.h */
	int	si_code;		/* code from above	*/
	int	si_errno;		/* error from errno.h	*/

	union {
		int	_pad[_SCO_SI_PAD];/* for future growth	*/

		struct {		/* kill(), SIGCLD	*/
			short	_pid;	/* process ID		*/
			union {
				struct {
					ushort_t	_uid;
				} _kill;
				struct {
					clock_t	_utime;
					int	_status;
					clock_t	_stime;
				} _cld;
			} _pdata;
		} _proc;

		struct {/* SIGSEGV, SIGBUS, SIGILL and SIGFPE	*/
			caddr_t	_addr;	/* faulting address	*/
		} _fault;

		struct {		/* SIGPOLL, SIGXFSZ	*/
			int	_fd;	/* file descriptor	*/
			long	_band;
		} _sifile;

	} _data;
} osr5_siginfo_t;

/* OSR5 flock struct */
typedef struct osr5_flock {
        short   l_type;
        short   l_whence;
        off_t   l_start;
        off_t   l_len;          /* len = 0 means until end of file */
	short   l_pid;
        short   l_sysid;
} osr5_flock_t;

/* definitions used for streamio and termio[s] */
struct osr5_strrecvfd {
	int		fd;
	ushort_t	uid;
	ushort_t	gid;
	char		fill[8];
};

struct osr5_termio {
        ushort_t	c_iflag;        /* input modes */
        ushort_t	c_oflag;        /* output modes */
        ushort_t	c_cflag;        /* control modes */
        ushort_t	c_lflag;        /* line discipline modes */
        char		c_line;         /* line discipline */
        uchar_t		c_cc[8];       /* control chars */
};

struct osr5_termios {
        ulong_t		c_iflag;        /* input modes */
        ulong_t		c_oflag;        /* output modes */
        ulong_t		c_cflag;        /* control modes */
        ulong_t		c_lflag;        /* local (line discipline) modes */
        uchar_t		c_cc[21];       /* control chars */
};

#define _SCO_IEXTEN	0000400
#define _SCO_TOSTOP	0001000
#define _SCO_ECHOCTL	0100000
#define _SCO_VSTART	11
#define _SCO_VSTOP	12
#define _SCO_VDSUSP	16
#define _SCO_VREPRINT	17
#define _SCO_TIOCSPGRP	(('T'<<8)|118)
#define _SCO_TIOCGPGRP	(('T'<<8)|119)
#define _SCO_TIOCGSID	(('T'<<8)|122)

/* definition used by siglongjmp */
#define _SCO_JBLEN	6

/* definitions used by OSR5 emulation of sysinfo */
struct osr5_socksysreq {
	int	args[7];
};

#define _SCO_PATH_SOCKSYS	"/dev/socksys"
#define _SCO_SO_GETIPDOMAIN	16
#define _SCO_IOCPARM_MASK	0x7f
#define _SCO_IOC_IN		0x80000000
#define _SCO_IOSW(x,y,t)	(_SCO_IOC_IN|((sizeof(t)&_SCO_IOCPARM_MASK)<<16)|(x<<8)|y)
#define _SCO_SIOSOCKSYS		_SCO_IOSW('I', 66, struct osr5_socksysreq)
#define _SCO_MAXHOSTNAMELEN	256
#define _SCO_NIOCSETDOMNAM	4
#define _SCO_NIOCGETDOMNAM	5
#define _SCO_SETNAME		56

struct domnam_args {
	char	*name;
	int	namelen;
};

#endif

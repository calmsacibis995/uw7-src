/*		copyright	"%c%" 	*/


#ident	"@(#)libcmd:i386/lib/libcmd/systbl.c	1.2.3.4"
#ident "$Header$"

/* These routines map system call number <=> system call name*/

#include	<string.h>
#include	<sys/types.h>
#include	<sys/syscall.h>

extern	char	*strcpy();
extern	int	strcmp();

struct	sysdef {
	const char	*name;
	int	number;
};
const static	struct	sysdef	sdefs[] = {
	{ "exit",		SYS_exit},
	{ "fork",		SYS_fork},
	{ "read",		SYS_read},
	{ "write",		SYS_write},
	{ "open",		SYS_open},
	{ "close",		SYS_close},
	{ "wait",		SYS_wait},
	{ "creat",		SYS_creat},
	{ "link",		SYS_link},
	{ "unlink",		SYS_unlink},
	{ "exec",		SYS_exec},
	{ "chdir",		SYS_chdir},
	{ "time",		SYS_time},
	{ "mknod",		SYS_mknod},
	{ "chmod",		SYS_chmod},
	{ "chown",		SYS_chown},
	{ "brk",		SYS_brk	},
	{ "stat",		SYS_stat},
	{ "lseek",		SYS_lseek},
	{ "getpid",		SYS_getpid},
	{ "mount",		SYS_mount},
	{ "umount",		SYS_umount},
	{ "setuid",		SYS_setuid},
	{ "getuid",		SYS_getuid},
	{ "stime",		SYS_stime},
	{ "ptrace",		SYS_ptrace},
	{ "alarm",		SYS_alarm},
	{ "fstat",		SYS_fstat},
	{ "pause",		SYS_pause},
	{ "utime",		SYS_utime},
	{ "stty",		SYS_stty},
	{ "gtty",		SYS_gtty},
	{ "access",		SYS_access},
	{ "nice",		SYS_nice},
	{ "statfs",		SYS_statfs},
	{ "sync",		SYS_sync},
	{ "kill",		SYS_kill},
	{ "fstatfs",		SYS_fstatfs},
	{ "pgrpsys",		SYS_pgrpsys},
	{ "xenix",		SYS_xenix},
	{ "dup",		SYS_dup},
	{ "pipe",		SYS_pipe},
	{ "times",		SYS_times},
	{ "profil",		SYS_profil},
	{ "plock",		SYS_plock},
	{ "setgid",		SYS_setgid},
	{ "getgid",		SYS_getgid},
	{ "signal",		SYS_signal},
	{ "msgsys",		SYS_msgsys},
	{ "sysi86",		SYS_sysi86},
	{ "acct",		SYS_acct},
	{ "shmsys",		SYS_shmsys},
	{ "semsys",		SYS_semsys},
	{ "ioctl",		SYS_ioctl},
	{ "uadmin",		SYS_uadmin},
	{ "",			-1        },	/*56*/
	{ "utssys",		SYS_utssys},
	{ "fsync",		SYS_fsync},
	{ "execve",		SYS_execve},
	{ "umask",		SYS_umask},
	{ "chroot",		SYS_chroot},
	{ "fcntl",		SYS_fcntl},
	{ "ulimit",		SYS_ulimit},
	{ "",			-1        },	/*64-77*/
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "",			-1        },
	{ "rfsys",		SYS_rfsys},
	{ "rmdir",		SYS_rmdir},
	{ "mkdir",		SYS_mkdir},
	{ "getdents",		SYS_getdents},
	{ "",			-1        },	/*82-83*/
	{ "",			-1        },
	{ "sysfs",		SYS_sysfs},
	{ "getmsg",		SYS_getmsg},
	{ "putmsg",		SYS_putmsg},
	{ "poll",		SYS_poll},
	{ "lstat",		SYS_lstat},
	{ "symlink",		SYS_symlink},
	{ "readlink",		SYS_readlink},
	{ "setgroups",		SYS_setgroups},
	{ "getgroups",		SYS_getgroups},
	{ "fchmod",		SYS_fchmod},
	{ "fchown",		SYS_fchown},
	{ "sigprocmask"	,	SYS_sigprocmask},
	{ "sigsuspend",		SYS_sigsuspend},
	{ "sigaltstack"	,	SYS_sigaltstack},
	{ "sigaction",		SYS_sigaction},
	{ "sigpending",		SYS_sigpending},
	{ "context",		SYS_context},
	{ "evsys",		SYS_evsys},
	{ "evtrapret",		SYS_evtrapret},
	{ "statvfs",		SYS_statvfs},
	{ "fstatvfs",		SYS_fstatvfs},
	{ "",			-1        },	/*105*/
	{ "nfssys",		SYS_nfssys},
	{ "waitsys",		SYS_waitsys},
	{ "sigsendsys",		SYS_sigsendsys},
	{ "hrtsys",		SYS_hrtsys},
	{ "acancel",		SYS_acancel},
	{ "async",		SYS_async},
	{ "priocntlsys"	,	SYS_priocntlsys},
	{ "pathconf",		SYS_pathconf},
	{ "mincore",		SYS_mincore},
	{ "mmap",		SYS_mmap},
	{ "mprotect",		SYS_mprotect},
	{ "munmap",		SYS_munmap},
	{ "fpathconf",		SYS_fpathconf},
	{ "vfork",		SYS_vfork},
	{ "fchdir",		SYS_fchdir},
	{ "readv",		SYS_readv},
	{ "writev",		SYS_writev},
	{ "xstat",		SYS_xstat},
	{ "lxstat",		SYS_lxstat},
	{ "fxstat",		SYS_fxstat},
	{ "xmknod",		SYS_xmknod},
	{ "clocal",		SYS_clocal},
	{ "setrlimit",		SYS_setrlimit},
	{ "getrlimit",		SYS_getrlimit},
	{ "lchown",		SYS_lchown},
	{ "memcntl",		SYS_memcntl},
	{ "getpmsg",		SYS_getpmsg},
	{ "putpmsg",		SYS_putpmsg},
	{ "rename",		SYS_rename},
	{ "uname",		SYS_uname},
	{ "setegid",		SYS_setegid},
	{ "sysconfig",		SYS_sysconfig},
	{ "adjtime",		SYS_adjtime},
	{ "systeminfo",		SYS_systeminfo},
	{ "",			-1        },	/*140*/
	{ "seteuid",		SYS_seteuid},
	{ "",			-1        },	/*142*/
	{ "keyctl",		SYS_keyctl},
	{ "secsys",		SYS_secsys},
	{ "filepriv",		SYS_filepriv},
	{ "procpriv",		SYS_procpriv},
	{ "devstat",		SYS_devstat},
	{ "aclipc",		SYS_aclipc},
	{ "fdevstat",		SYS_fdevstat},
	{ "flvlfile",		SYS_flvlfile},
	{ "lvlfile",		SYS_lvlfile},
	{ "",			-1        },	/*152*/
	{ "lvlequal",		SYS_lvlequal},
	{ "lvlproc",		SYS_lvlproc},
	{ "",			-1        },	/*155*/
	{ "lvlipc",		SYS_lvlipc},
	{ "acl",		SYS_acl},
	{ "auditevt",		SYS_auditevt},
	{ "auditctl",		SYS_auditctl},
	{ "auditdmp",		SYS_auditdmp},
	{ "auditlog",		SYS_auditlog},
	{ "auditbuf",		SYS_auditbuf},
	{ "lvldom",		SYS_lvldom},
	{ "lvlvfs",		SYS_lvlvfs},
	{ "mkmld",		SYS_mkmld},
	{ "mldmode",		SYS_mldmode},
	{ "secadvise",		SYS_secadvise},
	{ "online",		SYS_online},
	{ "setitimer",		SYS_setitimer},
	{ "getitimer",		SYS_getitimer},
	{ "gettimeofday",	SYS_gettimeofday},
	{ "settimeofday",	SYS_settimeofday},
	{ "_lwp_create",	SYS_lwpcreate},
	{ "_lwp_exit",		SYS_lwpexit},
	{ "_lwp_wait",		SYS_lwpwait},
	{ "__lwp_self",		SYS_lwpself},
	{ "_lwp_info",		SYS_lwpinfo},
	{ "__lwp_private",	SYS_lwpprivate},
	{ "processor_bind",	SYS_processor_bind},
	{ "processor_exbind",	SYS_processor_exbind},
	{ "",			-1        },	/*181-182*/
	{ "",			-1        },
	{ "prepblock",		SYS_prepblock},
	{ "block",		SYS_block},
	{ "rdblock",		SYS_rdblock},
	{ "unblock",		SYS_unblock},
	{ "cancelblock",	SYS_cancelblock},
	{ "",			-1        },	/*188*/
	{ "pread",		SYS_pread},
	{ "pwrite",		SYS_pwrite},
	{ "truncate",		SYS_truncate},
	{ "ftruncate",		SYS_ftruncate},
	{ "_lwp_kill",		SYS_lwpkill},
	{ "sigwait",		SYS_sigwait},
	{ "fork1",		SYS_fork1},
	{ "forkall",		SYS_forkall},
	{ "modload",		SYS_modload},
	{ "moduload",		SYS_moduload},
	{ "modpath",		SYS_modpath},
	{ "modstat",		SYS_modstat},
	{ "modadm",		SYS_modadm},
	{ "getksym",		SYS_getksym},
	{ "_lwp_suspend",	SYS_lwpsuspend},
	{ "_lwp_continue",	SYS_lwpcontinue},
	{ "priocntllst",	SYS_priocntllst},
	{ "__sleep",		SYS_sleep},
	{ "_lwp_sema_wait",	SYS_lwp_sema_wait},
	{ "_lwp_sema_post",	SYS_lwp_sema_post},
	{ "_lwp_sema_trywait",	SYS_lwp_sema_trywait},
};

#define	NUM_SYSCALL (sizeof(sdefs)/sizeof(struct sysdef))
/* Store number of sys calls in a global variable, so it can be accessed externally */
int num_syscall = NUM_SYSCALL;

/*Given the system call number return the system call name*/
/*If the system call number is not defined then a NULL pointer is returned*/ 
char	*
scallnam(buf, p)
char	*buf;
int	p;
{
	register int	i;

	for(i = 0; i < NUM_SYSCALL; ++i) {
		if (sdefs[i].number == p) {
			return strcpy(buf, sdefs[i].name);
		}
	}
	*buf='\0';
	return (char *)0;
}

/*Given the system call name return the system call number*/
/*If the system call name is not defined then a -1 is returned*/
int
scallnum(name)
char	*name;
{
	register int	i;

	if (!(*name))
		return -1;
	for(i = 0; i < NUM_SYSCALL; ++i){
		if (!strcmp(name, sdefs[i].name)) {
			return sdefs[i].number;
		}
	}
	return -1;
}

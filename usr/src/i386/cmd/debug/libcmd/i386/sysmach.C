#ident	"@(#)debugger:libcmd/i386/sysmach.C	1.10"

#include "Parser.h"
#include "Machine.h"
#include <sys/signal.h>

#ifdef HAS_SYSCALL_TRACING
#include "TSClist.h"
#include <sys/syscall.h>
#endif

// machine specific signal and system call information

const char *sigtable[] = {	// OS release dependent!
	"hup",
	"int",
	"quit",
	"ill",
	"trap",
	"abrt",
	"emt",
	"fpe",
	"kill",
	"bus",
	"segv",
	"sys",
	"pipe",
	"alrm",
	"term",
	"usr1",
	"usr2",
	"cld",
	"pwr",
	"winch",
	"urg",
	"poll",
	"stop",
	"tstp",
	"cont",
	"ttin",
	"ttou",
	"vtalrm",
	"prof",
	"xcpu",
	"xfsz",
#ifdef SIGWAITING
	"waiting",
	"lwp",
	"aio",
#endif
	0
};

// if a user does a syscall stat (or fstat etc.) create an event
// on both stat and xstat - don't know which one they really want

int
getsys_mach(int entry)
{
#ifdef HAS_SYSCALL_TRACING
	switch(entry)
	{
		case SYS_stat:	return SYS_xstat;
		case SYS_lstat:	return SYS_lxstat;
		case SYS_fstat:	return SYS_fxstat;
		case SYS_mknod:	return SYS_xmknod;
		case SYS_uname: return SYS_utssys;
		default:
			break;
	}
#endif
	return 0;
}

/*
 *	system call names -  OS release dependent!
 *	this file and Machine.h must both be updated 
 *	if new system calls are added
 */

#ifdef HAS_SYSCALL_TRACING
syscalls systable[] =
{
	{ "exit",	SYS_exit },
	{ "fork",	SYS_fork },
	{ "read",	SYS_read },
	{ "write",	SYS_write },
	{ "open",	SYS_open },
	{ "close",	SYS_close },
	{ "wait",	SYS_wait },
	{ "creat",	SYS_creat },
	{ "link",	SYS_link },
	{ "unlink",	SYS_unlink },
	{ "exec",	SYS_exec },
	{ "chdir",	SYS_chdir },
	{ "time",	SYS_time },
	{ "mknod",	SYS_mknod },
	{ "chmod",	SYS_chmod },
	{ "chown",	SYS_chown },
	{ "brk",	SYS_brk },
	{ "stat",	SYS_stat },
	{ "lseek",	SYS_lseek },
	{ "getpid",	SYS_getpid },
	{ "mount",	SYS_mount },
	{ "umount",	SYS_umount },
	{ "setuid",	SYS_setuid },
	{ "getuid",	SYS_getuid },
	{ "stime",	SYS_stime },
	{ "ptrace",	SYS_ptrace },
	{ "alarm",	SYS_alarm },
	{ "fstat",	SYS_fstat },
	{ "pause",	SYS_pause },
	{ "utime",	SYS_utime },
	{ "stty",	SYS_stty },
	{ "gtty",	SYS_gtty },
	{ "access",	SYS_access },
	{ "nice",	SYS_nice },
	{ "statfs",	SYS_statfs },
	{ "sync",	SYS_sync },
	{ "kill",	SYS_kill },
	{ "fstatfs",	SYS_fstatfs },
	{ "pgrpsys",	SYS_pgrpsys },
	{ "getpgrp",	SYS_pgrpsys },
	{ "setpgrp",	SYS_pgrpsys },
	{ "getsid",	SYS_pgrpsys },
	{ "setsid",	SYS_pgrpsys },
	{ "getpgid",	SYS_pgrpsys },
	{ "setpgid",	SYS_pgrpsys },
	{ "xenix",	SYS_xenix },
	{ "dup",	SYS_dup },
	{ "pipe",	SYS_pipe },
	{ "times",	SYS_times },
	{ "profil",	SYS_profil },
	{ "plock",	SYS_plock },
	{ "setgid",	SYS_setgid },
	{ "getgid",	SYS_getgid },
	{ "signal",	SYS_signal },
	{ "sigset",	SYS_signal },
	{ "sighold",	SYS_signal },
	{ "sigrelse",	SYS_signal },
	{ "sigignore",	SYS_signal },
	{ "sigpause",	SYS_signal },
	{ "msgsys",	SYS_msgsys },
	{ "msgget",	SYS_msgsys },
	{ "msgctl",	SYS_msgsys },
	{ "msgrcv",	SYS_msgsys },
	{ "msgsnd",	SYS_msgsys },
	{ "sysi86",	SYS_sysi86 },
	{ "acct",	SYS_acct },
	{ "shmsys",	SYS_shmsys },
	{ "shmat",	SYS_shmsys },
	{ "shmctl",	SYS_shmsys },
	{ "shmdt",	SYS_shmsys },
	{ "shmget",	SYS_shmsys },
	{ "semsys",	SYS_semsys },
	{ "semctl",	SYS_semsys },
	{ "semget",	SYS_semsys },
	{ "semop",	SYS_semsys },
	{ "ioctl",	SYS_ioctl },
	{ "uadmin",	SYS_uadmin },
	{ "utssys",	SYS_utssys },
	{ "ustat",	SYS_utssys },
	{ "fusers",	SYS_utssys },
	{ "fsync",	SYS_fsync },
	{ "execve",	SYS_execve },
	{ "umask",	SYS_umask },
	{ "chroot",	SYS_chroot },
	{ "fcntl",	SYS_fcntl },
	{ "ulimit",	SYS_ulimit },

#if SYS_cg_ids	/* Gemini - NUMA-related system calls */
	{ "cg_ids",		SYS_cg_ids },
	{ "cg_processors",	SYS_cg_processors },
	{ "cg_info",		SYS_cg_info },
	{ "cg_bind",		SYS_cg_bind },
	{ "cg_current",		SYS_cg_current },
	{ "cg_memloc",		SYS_cg_memloc },
#endif
	{ "rfsys",	SYS_rfsys },
	{ "rmdir",	SYS_rmdir },
	{ "mkdir",	SYS_mkdir },
	{ "getdents",	SYS_getdents },
	{ "sysfs",	SYS_sysfs },
	{ "getmsg",	SYS_getmsg },
	{ "putmsg",	SYS_putmsg },
	{ "poll",	SYS_poll },
	{ "lstat",	SYS_lstat },
	{ "symlink",	SYS_symlink },
	{ "readlink",	SYS_readlink },
	{ "setgroups",	SYS_setgroups },
	{ "getgroups",	SYS_getgroups },
	{ "fchmod",	SYS_fchmod },
	{ "fchown",	SYS_fchown },
	{ "sigprocmask",	SYS_sigprocmask },
	{ "sigsuspend",	SYS_sigsuspend },
	{ "sigaltstack",	SYS_sigaltstack },
	{ "sigaction",	SYS_sigaction },
	{ "sigpending",	SYS_sigpending },
	{ "sigfillset",	SYS_sigpending },
	{ "getcontext",	SYS_context },
	{ "setcontext",	SYS_context },
	{ "evsys",	SYS_evsys },
	{ "evtrapret",	SYS_evtrapret },
	{ "statvfs",	SYS_statvfs },
	{ "fstatvfs",	SYS_fstatvfs },
	{ "nfssys",	SYS_nfssys },
	{ "waitsys",	SYS_waitsys },
	{ "sigsendsys",	SYS_sigsendsys },
	{ "hrtsys",	SYS_hrtsys },
	{ "acancel",	SYS_acancel },
	{ "async",	SYS_async },
	{ "priocntlsys",	SYS_priocntlsys },
	{ "pathconf",	SYS_pathconf },
	{ "mincore",	SYS_mincore },
	{ "mmap",	SYS_mmap },
	{ "mprotect",	SYS_mprotect },
	{ "munmap",	SYS_munmap },
	{ "fpathconf",	SYS_fpathconf },
	{ "vfork",	SYS_vfork },
	{ "fchdir",	SYS_fchdir },
	{ "readv",	SYS_readv },
	{ "writev",	SYS_writev },
	{ "xstat",	SYS_xstat },
	{ "lxstat",	SYS_lxstat },
	{ "fxstat",	SYS_fxstat },
	{ "xmknod",	SYS_xmknod },
	{ "clocal",	SYS_clocal },
	{ "setrlimit",	SYS_setrlimit },
	{ "getrlimit",	SYS_getrlimit },
	{ "lchown",	SYS_lchown },
	{ "memcntl",	SYS_memcntl },
	{ "getpmsg",	SYS_getpmsg },
	{ "putpmsg",	SYS_putpmsg },
	{ "rename",	SYS_rename },
	{ "uname",	SYS_uname },
	{ "setegid",	SYS_setegid },
	{ "sysconfig",	SYS_sysconfig },
	{ "adjtime",	SYS_adjtime },
	{ "systeminfo",	SYS_systeminfo },
	{ "seteuid",	SYS_seteuid },

#ifdef SYS_getksym 	/* ES and beyond */

	{ "secsys",	SYS_secsys },
	{ "filepriv",	SYS_filepriv },
	{ "procpriv",	SYS_procpriv },
	{ "devstat",	SYS_devstat },
	{ "aclipc",	SYS_aclipc },
	{ "fdevstat",	SYS_fdevstat },
	{ "flvlfile",	SYS_flvlfile },
	{ "lvlfile",	SYS_lvlfile },
	{ "lvlequal",	SYS_lvlequal },
	{ "lvlproc",	SYS_lvlproc },
	{ "lvlipc",	SYS_lvlipc },
	{ "acl",	SYS_acl	 },
	{ "auditevt",	SYS_auditevt },
	{ "auditctl",	SYS_auditctl },
	{ "auditdmp",	SYS_auditdmp },
	{ "auditlog",	SYS_auditlog },
	{ "auditbuf",	SYS_auditbuf },
	{ "lvldom",	SYS_lvldom },
	{ "lvlvfs",	SYS_lvlvfs },
	{ "mkmld",	SYS_mkmld },
	{ "mldmode",	SYS_mldmode },
	{ "secadvise",	SYS_secadvise },
	{ "online",	SYS_online },
	{ "setitimer",	SYS_setitimer },
	{ "getitimer",	SYS_getitimer },
	{ "gettimeofday ",	SYS_gettimeofday },
	{ "settimeofday ",	SYS_settimeofday },
	{ "_lwp_create",	SYS_lwpcreate },
	{ "_lwp_exit",	SYS_lwpexit },
	{ "_lwp_wait",	SYS_lwpwait },
	{ "_lwp_self",	SYS_lwpself },
	{ "_lwp_info",	SYS_lwpinfo },
	{ "_lwp_private",	SYS_lwpprivate },
	{ "processor_bind",	SYS_processor_bind },
	{ "processor_exbind",	SYS_processor_exbind },
	{ "prepblock",	SYS_prepblock },
	{ "block",	SYS_block },
	{ "rdblock",	SYS_rdblock },
	{ "unblock",	SYS_unblock },
	{ "cancelblock",	SYS_cancelblock },
	{ "pread",	SYS_pread },
	{ "pwrite",	SYS_pwrite },
	{ "truncate",	SYS_truncate },
	{ "ftruncate",	SYS_ftruncate },
	{ "_lwp_kill",	SYS_lwpkill },
	{ "sigwait",	SYS_sigwait },
	{ "fork1",	SYS_fork1 },
	{ "forkall",	SYS_forkall },
	{ "modload",	SYS_modload },
	{ "moduload",	SYS_moduload },
	{ "modpath",	SYS_modpath },
	{ "modstat",	SYS_modstat },
	{ "modadm",	SYS_modadm },
	{ "getksym",	SYS_getksym },

#ifdef SYS_lwpcontinue /* 4.2 */

	{ "_lwp_suspend",	SYS_lwpsuspend },
	{ "_lwp_continue",	SYS_lwpcontinue },
#ifdef SYS_priocntllst	/* ES/MP */
	{ "priocntllist",	SYS_priocntllst },
	{ "sleep",		SYS_sleep },
	{ "_lwp_sema_wait",	SYS_lwp_sema_wait },
	{ "_lwp_sema_post",	SYS_lwp_sema_post },
	{ "_lwp_sema_trywait",	SYS_lwp_sema_trywait },

#ifdef SYS_invlpg /* Gemini */	
	{ "fstatvfs64",		SYS_fstatvfs64 },
	{ "statvfs64",		SYS_statvfs64 },
	{ "ftruncate64",	SYS_ftruncate64 },
	{ "truncate64",		SYS_truncate64 },
	{ "getrlimit64",	SYS_getrlimit64 },
	{ "setrlimit64",	SYS_setrlimit64 },
	{ "lseek64",		SYS_lseek64 },
	{ "mmap64",		SYS_mmap64 },
	{ "pread64",		SYS_pread64 },
	{ "pwrite64",		SYS_pwrite64 },
	{ "creat64",		SYS_creat64 },
	{ "dshmsys",		SYS_dshmsys },
	{ "invlpg",		SYS_invlpg },
	{ "sendv",		SYS_sendv },
	{ "sendv64",		SYS_sendv64 },

#endif
#endif
#endif
#endif
	{ 0, 0 }
};
#endif

#ident	"@(#)libc-i386:gen/shim.c	1.17"

/* "shim" code for Gemini on OpenServer (and a bit
 * for Gemini on UnixWare 2.1.x.
 * This code translates various values and data types between
 * those given by a Gemini program and those expected by
 * the OpenServer kernel.  It also translates values and
 * data types returned by the OpenServer kernel to those
 * expected by the Gemini program.
 */

#include "synonyms.h"
#include <errno.h>
#include <unistd.h>

#ifdef GEMINI_ON_OSR5

/* functions aliased to _acl all return ENOPKG */

#pragma weak access = _access
#pragma weak acl = _acl
#pragma weak aclipc = _acl
#pragma weak _aclipc = _acl
#pragma weak auditbuf = _acl
#pragma weak _auditbuf = _acl
#pragma weak auditctl = _acl
#pragma weak _auditctl = _acl
#pragma weak auditdmp = _acl
#pragma weak _auditdmp = _acl
#pragma weak auditevt = _acl
#pragma weak _auditevt = _acl
#pragma weak auditlog = _acl
#pragma weak _auditlog = _acl
#pragma weak devstat = _acl
#pragma weak _devstat = _acl
#pragma weak fcntl = _fcntl
#pragma weak ioctl = _ioctl
#pragma weak flvlfile = _acl
#pragma weak _flvlfile = _acl
#pragma weak fdevstat = _acl
#pragma weak _fdevstat = _acl
#pragma weak fpathconf = _fpathconf
#pragma weak getgroups = _getgroups
#pragma weak gettimeofday = _gettimeofday
#pragma weak _abi_gettimeofday = _gettimeofday
#pragma weak lvldom = _acl
#pragma weak _lvldom = _acl
#pragma weak lvlequal = _acl
#pragma weak _lvlequal = _acl
#pragma weak lvlfile = _acl
#pragma weak _lvlfile = _acl
#pragma weak lvlipc = _acl
#pragma weak _lvlipc = _acl
#pragma weak lvlproc = _acl
#pragma weak _lvlproc = _acl
#pragma weak lvlvfs = _acl
#pragma weak _lvlvfs = _acl
#pragma weak mkmld = _acl
#pragma weak _mkmld = _acl
#pragma weak mldmode = _acl
#pragma weak _mldmode = _acl
#pragma weak mmap = _mmap
#pragma weak mmap32 = _mmap /*for now*/
#pragma weak _mmap32 = _mmap /*for now*/
#pragma weak msgctl = _msgctl
#pragma weak nuname = _nuname
#pragma weak pathconf = _pathconf
#pragma weak ptrace = _ptrace
#pragma weak select = _select
#pragma weak _abi_select = _select
#pragma weak semctl = _semctl
#pragma weak setcontext = _setcontext
#pragma weak setgroups = _setgroups
#pragma weak shmctl = _shmctl
#pragma weak sigaction = _sigaction
#pragma weak sigpending = _sigpending
#pragma weak sigprocmask = _sigprocmask
#pragma weak sigsuspend = _sigsuspend
#pragma weak sleep = _sleep
#pragma weak sysconf = _sysconf
#pragma weak sysinfo = _sysinfo
#pragma weak _abi_sysinfo = _sysinfo
#pragma weak waitid = _waitid
#pragma weak waitpid = _waitpid
#pragma weak xmknod = _xmknod

#include <stdarg.h>
#include <time.h>
#include <siginfo.h>
#include <signal.h>
#include <limits.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <ucontext.h>
#include "osr5_shim.h"
#include <sys/stat.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/procset.h>
#include <sys/wait.h>
#include <sys/siginfo.h>
#include <sys/mkdev.h>
#include <sys/fcntl.h>
#include <sys/msg.h>
#include <sys/termio.h>
#include <sys/termios.h>
#include <sys/stropts.h>
#include <sys/select.h>
#include <sys/systeminfo.h>

void * 
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	/* UW silently ignores this flag - OSR5 rejects it */
	if (flags & MAP_NORESERVE)
		flags &= ~MAP_NORESERVE;
	return((void *)syscall(_SCO_MMAP, addr, len, 
		prot, flags, fd, off));
}

extern int _osr5_setcontext(const ucontext_t *);

int 
setcontext(const ucontext_t *ucp)
{
	ucontext_t	*sucp = (ucontext_t *)ucp;

	/* correct arbitrary junk */
        sucp->uc_sigmask.sa_sigbits[1] = 0;  
        sucp->uc_sigmask.sa_sigbits[2] = 0;
        sucp->uc_sigmask.sa_sigbits[3] = 0;
        sucp->uc_privatedatap = 0;

        return _osr5_setcontext(sucp);    /* success never returns */
}

int 
getgroups(int gsize, gid_t *glist)
{
        register int	i;
        int		save_ret;
        ushort_t	sa[_SCO_NGROUPS_MAX];

	if ((save_ret = syscall(_SCO_GETGROUPS, gsize, sa)) > 0)
        {
                i = save_ret;
                while (i--)
                        glist[i] = sa[i];
        }
        return save_ret;
}

int 
setgroups(int ngroups, const gid_t *glist)
{
        register int	i;
        ushort_t	sa[_SCO_NGROUPS_MAX];
	ushort_t	*sptr = sa;

	for(i = 0; i < ngroups; i++)
	{
		*sptr++ = *glist++;
	}
	return(syscall(_SCO_SETGROUPS, ngroups, sa));
}

static void
fixup_ipc(struct ipc_perm *uw_ipc, struct osr5_ipc_perm *osr5_ipc, 
	int flag)
{
	if (flag == 0)
	{
		osr5_ipc->uid = uw_ipc->uid;
		osr5_ipc->gid = uw_ipc->gid;
		osr5_ipc->cuid = uw_ipc->cuid;
		osr5_ipc->cgid = uw_ipc->cgid;
		osr5_ipc->mode = uw_ipc->mode;
		osr5_ipc->seq = uw_ipc->seq;
		osr5_ipc->key = uw_ipc->key;
	}
	else
	{
		uw_ipc->uid = osr5_ipc->uid;
		uw_ipc->gid = osr5_ipc->gid;
		uw_ipc->cuid = osr5_ipc->cuid;
		uw_ipc->cgid = osr5_ipc->cgid;
		uw_ipc->mode = osr5_ipc->mode;
		uw_ipc->seq = osr5_ipc->seq;
		uw_ipc->key = osr5_ipc->key;
	}

}

union semun
{
	int		val;
	struct semid_ds	*buf;
	ushort_t	*array;
};

int
semctl(int id, int num, int cmd, ...)
{
	int val, retry;
	va_list ap;
	struct osr5_semid_ds shim_semid;
	struct semid_ds	 *uw_semid;
	int ret;
	int ocmd;

	/*
	* Due to an unfortunate documentation miscue, the intended use of
	* semctl() changed from an optional 4th argument with varying
	* scalar types to a union--one that wasn't declared in any header!
	* For most existing implementations, this wasn't a real problem
	* since a union of int and a couple of pointers was passed the
	* same as a simple int or pointer.  However, other implementations
	* also have to be handled, and there is no "right" answer, given
	* that some code might exist (other than test suites) that passes
	* the 4th argument as a true union.
	*
	* This code takes an intermediate position.  Since taking the 4th
	* argument as a simple scalar is less likely to cause faults at
	* user level, we try that first.  The kernel won't dereference a
	* bad pointer value.  The hope is that if we guessed wrong, the
	* system call will fail, and we give it another try as if a union
	* was passed, but only for those cmd values that need a 4th arg.
	*/

	va_start(ap, cmd);
	ocmd = cmd;
	retry = 1;
	switch (cmd)
	{
	default:
		val = 0;
		retry = 0;
		break;
	case SETVAL:
		val = va_arg(ap, int);
		break;
	case GETALL:
	case SETALL:
		val = (int)va_arg(ap, ushort_t *);
		break;
	case IPC_STAT:
		cmd = _SCO_IPC_STAT;
		val = (int)&shim_semid;
		break;
	case IPC_SET:
		cmd = _SCO_IPC_SET;
		uw_semid = va_arg(ap, struct semid_ds *);
		fixup_ipc(&uw_semid->sem_perm, &shim_semid.sem_perm, 0);
		shim_semid.sem_nsems = uw_semid->sem_nsems;
		shim_semid.sem_otime = uw_semid->sem_otime;
		shim_semid.sem_ctime = uw_semid->sem_ctime;
		shim_semid.sem_base = uw_semid->sem_base;
		shim_semid.fill[0] = 0;
		shim_semid.fill[1] = 0;
		val = (int)&shim_semid;
		break;
	case 1: /*IPC_O_SET*/
	case 2: /*IPC_O_STAT*/
		val = (int)va_arg(ap, struct semid_ds *);
		break;
	}
	while ((ret = syscall(_SCO_SEMSYS, _SCO_SEMCTL, id, num, cmd, val)) == -1
		&& retry)
	{
		va_end(ap);
		retry = 0;
		va_start(ap, cmd);
		val = va_arg(ap, union semun).val;
	}
	if (ret > -1 && (ocmd == IPC_STAT))
	{
		uw_semid = (struct semid_ds *)val;
		fixup_ipc(&uw_semid->sem_perm, &shim_semid.sem_perm, 1);

                uw_semid->sem_nsems = shim_semid.sem_nsems;
                uw_semid->sem_otime = shim_semid.sem_otime;
                uw_semid->sem_ctime = shim_semid.sem_ctime;
                uw_semid->sem_base = shim_semid.sem_base;
	}

	va_end(ap);
	return ret;
}

int 
shmctl(int shmid, int cmd, struct shmid_ds *shmidptr)
{
	struct osr5_shmid_ds shim_shm;
	int ret;
	int ocmd = cmd;

	if (cmd == IPC_STAT)
	{
		cmd = _SCO_IPC_STAT;
	}
	else if (cmd == IPC_SET)
	{
		cmd = _SCO_IPC_SET;
		fixup_ipc(&shmidptr->shm_perm, &shim_shm.shm_perm, 0);

		shim_shm.shm_segsz   = shmidptr->shm_segsz;
		shim_shm.shm_lpid    = shmidptr->shm_lpid;
		shim_shm.shm_cpid    = shmidptr->shm_cpid;
		shim_shm.shm_nattch  = shmidptr->shm_nattch;
		shim_shm.shm_cnattch = shmidptr->shm_cnattch;
		shim_shm.shm_atime   = shmidptr->shm_atime;
		shim_shm.shm_dtime   = shmidptr->shm_dtime;
		shim_shm.shm_ctime   = shmidptr->shm_ctime;

	}


	if (((ret = syscall(_SCO_SHMSYS, _SCO_SHMCTL, shmid, cmd, 
		&shim_shm)) >= 0) && (ocmd == IPC_STAT))
        {
		fixup_ipc(&shmidptr->shm_perm, &shim_shm.shm_perm, 1);

                shmidptr->shm_segsz = shim_shm.shm_segsz;
                shmidptr->shm_lpid = shim_shm.shm_lpid;
                shmidptr->shm_cpid = shim_shm.shm_cpid;
                shmidptr->shm_nattch = shim_shm.shm_nattch;
                shmidptr->shm_cnattch = shim_shm.shm_cnattch;
                shmidptr->shm_atime = shim_shm.shm_atime;
                shmidptr->shm_dtime = shim_shm.shm_dtime;
                shmidptr->shm_ctime = shim_shm.shm_ctime;
        }
	return ret;
}

int
msgctl(int msqid, int cmd, struct msqid_ds *buf)
{
	int	ret;
	struct osr5_msqid_ds	obuf;
	int ocmd = cmd;

	if (cmd == IPC_STAT)
	{
		cmd = _SCO_IPC_STAT;
	}
	else if (cmd == IPC_SET)
	{
		cmd = _SCO_IPC_SET;
		fixup_ipc(&buf->msg_perm, &obuf.msg_perm, 0);

		obuf.msg_first = buf->msg_first;
		obuf.msg_last = buf->msg_last;
		obuf.msg_cbytes = buf->msg_cbytes;
		obuf.msg_qnum = buf->msg_qnum;
		obuf.msg_qbytes = buf->msg_qbytes;
		obuf.msg_lspid = buf->msg_lspid;
		obuf.msg_lrpid = buf->msg_lrpid;
		obuf.msg_stime = buf->msg_stime;
		obuf.msg_rtime = buf->msg_rtime;
		obuf.msg_ctime = buf->msg_ctime;
	}
	if (((ret = syscall(_SCO_MSGSYS, _SCO_MSGCTL, msqid, cmd,
		&obuf)) >= 0) && (ocmd == IPC_STAT))
	{
		fixup_ipc(&buf->msg_perm, &obuf.msg_perm, 1);

		buf->msg_first = obuf.msg_first;
		buf->msg_last = obuf.msg_last;
		buf->msg_cbytes = obuf.msg_cbytes;
		buf->msg_qnum = obuf.msg_qnum;
		buf->msg_qbytes = obuf.msg_qbytes;
		buf->msg_lspid = obuf.msg_lspid;
		buf->msg_lrpid = obuf.msg_lrpid;
		buf->msg_stime = obuf.msg_stime;
		buf->msg_rtime = obuf.msg_rtime;
		buf->msg_ctime = obuf.msg_ctime;
	}
	return ret;
}

int
sigaction(int sig, const struct sigaction *nact, struct sigaction *oact)
{
	struct osr5_sigaction osr5_oact, osr5_nact;
	struct osr5_sigaction *osr5_optr, *osr5_nptr;
	int ret;
	
	if (nact != 0)
	{
		int oflags = nact->sa_flags;
		int nflags = oflags & ~(SA_ONSTACK|SA_NOCLDWAIT|SA_NOCLDSTOP);
		if (oflags & SA_ONSTACK)
		{
			nflags |= _SCO_SA_ONSTACK;
		}
		if (oflags & SA_NOCLDWAIT)
		{
			nflags |= _SCO_SA_NOCLDWAIT;
		}
		if (oflags & SA_NOCLDSTOP)
		{
			nflags |= _SCO_SA_NOCLDSTOP;
		}
		osr5_nact.sa_flags = nflags;
		osr5_nact.sa_mask = nact->sa_mask.sa_sigbits[0];
						 /* OSR5 uses a union of a
						  * sa_handler and sa_sigaction
						  * let the kernel look at the
						  * flags to take care of this
						  */
		osr5_nact._sa_function._sa_handler = nact->sa_handler;
		osr5_nptr = &osr5_nact;
	}
	else
		osr5_nptr = 0;

	if (oact != 0)
		osr5_optr = &osr5_oact;
	else
		osr5_optr = 0;
	
	if ((ret = __sigaction(sig,
		(const struct sigaction*)osr5_nptr, osr5_optr)) == 0
		&& osr5_optr)
	{
		int oflags = osr5_optr->sa_flags;
		int nflags = oflags & ~(_SCO_SA_ONSTACK|_SCO_SA_NOCLDWAIT|_SCO_SA_NOCLDSTOP);
		if (oflags & _SCO_SA_ONSTACK)
		{
			nflags |= SA_ONSTACK;
		}
		if (oflags & _SCO_SA_NOCLDWAIT)
		{
			nflags |= SA_NOCLDWAIT;
		}
		if (oflags & _SCO_SA_NOCLDSTOP)
		{
			nflags |= SA_NOCLDSTOP;
		}
		oact->sa_flags = nflags;
		oact->sa_mask.sa_sigbits[0]  = osr5_optr->sa_mask;
		oact->sa_mask.sa_sigbits[1]  = 0;
		oact->sa_mask.sa_sigbits[2]  = 0;
		oact->sa_mask.sa_sigbits[3]  = 0;
		oact->sa_handler = osr5_optr->_sa_function._sa_handler;
	}
	return ret;
}

int 
sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	int	ohow;
	int	ret;
	switch(how)
	{
	case SIG_BLOCK:
		ohow = _SCO_SIG_BLOCK;
		break;
	case SIG_UNBLOCK:
		ohow = _SCO_SIG_UNBLOCK;
		break;
	case SIG_SETMASK:
		ohow = _SCO_SIG_SETMASK;
		break;
	}
	ret = syscall(_SCO_SIGPROCMASK, ohow, set, oset);
	if (ret != -1 && oset)
	{
		oset->sa_sigbits[1]  = 0;
		oset->sa_sigbits[2]  = 0;
		oset->sa_sigbits[3]  = 0;
	}
	return ret;
}

int 
sigpending(sigset_t *set)
{
	return syscall(_SCO_SIGPENDING, set);
}

int 
sigsuspend(const sigset_t *set)
{
	return syscall(_SCO_SIGSUSPEND, set);
}

extern int _osr5_uname(struct osr5_utsname *);

int
nuname(struct utsname *name)
{
	struct osr5_utsname osr5_name;
	int ret;

	if ((ret = _osr5_uname(&osr5_name)) > -1)
	{
		strcpy(name->sysname,  osr5_name.sysname);
		strcpy(name->nodename, osr5_name.nodename);
		strcpy(name->release,  osr5_name.release);
		strcpy(name->version,  osr5_name.version);
		strcpy(name->machine,  osr5_name.machine);
	}
	return ret;
}

#undef st_atime
#undef st_ctime
#undef st_mtime

/* convert OSR5 stat structure to Gemini stat structure */
static void
fixup_stat(struct stat *gbuf, struct osr5_st_stat32 *obuf)
{
	ushort_t	major, minor;

	if (obuf->st_dev == O_NODEV)
		gbuf->st_dev = NODEV;
	else
	{
		major = obuf->st_dev >> ONBITSMINOR;
		minor = obuf->st_dev & OMAXMIN;
		gbuf->st_dev = (major << NBITSMINOR) | minor;
	}

	gbuf->st_ino = obuf->st_ino;
	gbuf->st_mode = obuf->st_mode;
	gbuf->st_nlink = obuf->st_nlink;
	gbuf->st_uid = obuf->st_uid;
	gbuf->st_gid = obuf->st_gid;

	if (obuf->st_rdev == O_NODEV)
		gbuf->st_rdev = NODEV;
	else
	{
		major = obuf->st_rdev >> ONBITSMINOR;
		minor = obuf->st_rdev & OMAXMIN;
		gbuf->st_rdev = (major << NBITSMINOR) | minor;
	}
	gbuf->st_size = obuf->st_size;

	gbuf->st_atim.st__tim.tv_sec = obuf->st_atime;
	gbuf->st_mtim.st__tim.tv_sec = obuf->st_mtime;
	gbuf->st_ctim.st__tim.tv_sec = obuf->st_ctime;
	gbuf->st_atim.st__tim.tv_nsec = 0L;
	gbuf->st_mtim.st__tim.tv_nsec = 0L;
	gbuf->st_ctim.st__tim.tv_nsec = 0L;
	gbuf->st_blksize = obuf->st_blksize;
	gbuf->st_blocks = obuf->st_blocks;
	strncpy(gbuf->st_fstype, obuf->st_fstype, _ST_FSTYPSZ); 
	gbuf->st_aclcnt = 0;
	gbuf->st_level = 0;
	gbuf->st_flags = 0;
	gbuf->st_cmwlevel = 0;
}

int 
_fxstat(const int ver, int fd, struct stat *buf)
{
	struct osr5_st_stat32 osr5_buf;
	int ret;
	
	if ((ret = syscall(_SCO_FXSTAT, _SCO_STAT_VER, fd, &osr5_buf)) > -1)
	{
		fixup_stat(buf, &osr5_buf);
	}
	return ret;
}

int 
_lxstat(int vers, const char *path, struct stat *buf)
{
	struct osr5_st_stat32 osr5_buf;
	int ret;
	
	if ((ret = syscall(_SCO_LXSTAT, _SCO_STAT_VER, path, &osr5_buf)) > -1)
	{
		fixup_stat(buf, &osr5_buf);
	}
	return ret;
}

int 
_xstat(int vers, const char *path, struct stat *buf)
{
	struct osr5_st_stat32 osr5_buf;
	int ret;
	
	if ((ret = syscall(_SCO_XSTAT, _SCO_STAT_VER, path, &osr5_buf)) > -1)
	{
		fixup_stat(buf, &osr5_buf);
	}
	return ret;
}

int
access(const char *path, int amode)
{
	if (amode & EX_OK)
	{
		struct osr5_st_stat32	buf;
		if (syscall(_SCO_XSTAT, _SCO_STAT_VER, path, &buf) == -1)
		{
			return -1;
		}
		if ((buf.st_mode & S_IFMT) != S_IFREG)
		{
			errno = EACCES;
			return -1;
		}
		amode &= ~EX_OK;
		amode |= X_OK;
	}
	if (amode & EFF_ONLY_OK)
	{
		amode &= ~EFF_ONLY_OK;
		return(syscall(_SCO_EACCESS, path, amode));
	}
	else
		return(syscall(_SCO_ACCESS, path, amode));
}

static long
_k_sysconf(int name)
{
	return((long)syscall(_SCO_SYSCONF, name));
}

long
sysconf(int name)
{
	int	result;

	switch(name) 
	{
		default:
			errno = EINVAL;
			return(-1L);

		case _SC_ARG_MAX:
			return(_k_sysconf(_SCO_SC_ARG_MAX));

		case _SC_CLK_TCK:
			return(_k_sysconf(_SCO_SC_CLK_TCK));

		case _SC_JOB_CONTROL:
			return(_k_sysconf(_SCO_SC_JOB_CONTROL));

		case _SC_SAVED_IDS:
			return(_k_sysconf(_SCO_SC_SAVED_IDS));

		case _SC_CHILD_MAX:
			return(_k_sysconf(_SCO_SC_CHILD_MAX));

		case _SC_NGROUPS_MAX:
			return(_k_sysconf(_SCO_SC_NGROUPS_MAX));

		case _SC_OPEN_MAX:
			return(_k_sysconf(_SCO_SC_OPEN_MAX));

		case _SC_VERSION:
			return(_k_sysconf(_SCO_SC_VERSION));

		case _SC_PAGESIZE:
			return(_k_sysconf(_SCO_SC_PAGESIZE));
	
		case _SC_XOPEN_VERSION:
			return(_k_sysconf(_SCO_SC_XOPEN_VERSION));

		case _SC_PASS_MAX:
			return(_k_sysconf(_SCO_SC_PASS_MAX));

		case _SC_LOGNAME_MAX:
			return(_SCO_LOGNAME_MAX);


	/*
	 * The following variables are new to XPG4.
	 */

  		case _SC_TZNAME_MAX:	
			return(_SCO_TZNAME_MAX); 
  		case _SC_STREAM_MAX:
			return(_SCO_STREAM_MAX); 
  		case _SC_BC_BASE_MAX:	
#ifdef _SCO_BC_BASE_MAX
			return(_SCO_BC_BASE_MAX);
#else
			return(_POSIX2_BC_BASE_MAX);
#endif
  		case _SC_BC_DIM_MAX:	
			return(_POSIX2_BC_DIM_MAX);
  		case _SC_BC_SCALE_MAX:  
			return(_POSIX2_BC_SCALE_MAX);
  		case _SC_BC_STRING_MAX: 
			return(_POSIX2_BC_STRING_MAX);
  		case _SC_COLL_WEIGHTS_MAX: 
#ifdef COLL_WEIGHTS_MAX
			/* internal libc - use Gemini value */
			return(COLL_WEIGHTS_MAX);
#else
			return(_POSIX2_COLL_WEIGHTS_MAX);
#endif
  		case _SC_EXPR_NEST_MAX: 
			/* internal libc - use Gemini value */
			return(EXPR_NEST_MAX);
  		case _SC_RE_DUP_MAX:	
#ifdef RE_DUP_MAX
			/* internal libc - use Gemini value */
			return(RE_DUP_MAX);
#else
			return(_POSIX2_RE_DUP_MAX);
#endif
  		case _SC_LINE_MAX:	
			return(_POSIX2_LINE_MAX);

		case _SC_XOPEN_SHM:
#ifdef _SCO_XOPEN_SHM
			return(_SCO_XOPEN_SHM);
#else
			return(1);
#endif
		case _SC_IOV_MAX:
			return(_k_sysconf(_SCO_SC_IOV_MAX));
			
		case _SC_XOPEN_CRYPT:
			/*
			 * The encryption routines crypt(), encrypt() and
                         * setkey() are always provided, and hence the setting
			 * for this X/Open Feature Group is to return true.
			 * Note that in certain markets
                         * the decryption algorithm may not be exported
                         * and in that case, encrypt() returns ENOSYS for
                         * the decryption operation.
			 */
			return (1L);
		case _SC_ATEXIT_MAX:
			return(32L);

		case _SC_XOPEN_XCU_VERSION:
			return 3;

		case _SC_NPROCESSORS_CONF:
	        case _SC_NPROC_CONF:
		case _SC_MAX_CPUS_PER_CG:
		{
			struct scoutsname	scostr;
			if (syscall(_SCO_INFO, &scostr, 
				sizeof(scostr)) == -1)
				return -1;
			return scostr.numcpu;
		}
		case _SC_TOTAL_MEMORY:
		case _SC_USEABLE_MEMORY:
		case _SC_GENERAL_MEMORY:
		{
			long	msize;
			int	serrno = errno;
			if ((msize = sysi86(_SCO_SI86MEM)) == -1)
			{
				errno = serrno;
				return -1;
			}
			return msize;
		}

		case _SC_NCGS_CONF:
		case _SC_NCGS_ONLN:
			return 1L;

  		case _SC_2_C_DEV:
  		case _SC_2_C_BIND:
  		case _SC_2_C_VERSION:
  		case _SC_2_CHAR_TERM:
  		case _SC_2_FORT_DEV:
  		case _SC_2_FORT_RUN:
  		case _SC_2_SW_DEV:
  		case _SC_2_UPE:
		case _SC_XOPEN_ENH_I18N:
  		case _SC_2_LOCALEDEF:
  		case _SC_2_VERSION:
		case _SC_NACLS_MAX:
		case _SC_NPROCESSORS_ONLN:
  	        case _SC_NPROC_ONLN:
		case _SC_NPROCESSES:
		case _SC_XOPEN_UNIX:
		case _SC_DEDICATED_MEMORY:
		case _SC_CG_SIMPLE_IMPL:
			/* not supported */
				return(-1L);
				break;

	}
}

static int
fixup_pathconf(int name)
{
	switch(name)
	{
	case _PC_LINK_MAX:	return _SCO_PC_LINK_MAX;
	case _PC_MAX_CANON:	return _SCO_MAX_CANON;
	case _PC_MAX_INPUT:	return _SCO_MAX_INPUT;
	case _PC_NAME_MAX:	return _SCO_NAME_MAX;
	case _PC_PATH_MAX:	return _SCO_PATH_MAX;
	case _PC_PIPE_BUF:	return _SCO_PIPE_BUF;
	case _PC_CHOWN_RESTRICTED:	return _SCO_CHOWN_RESTRICTED;
	case _PC_NO_TRUNC:	return _SCO_NO_TRUNC;
	case _PC_VDISABLE:	return _SCO_VDISABLE;
	default:
		return name;
	}
	/*NOTREACHED*/
}

long
fpathconf(int fd, int name)
{
	int oname = fixup_pathconf(name);
	return(syscall(_SCO_FPATHCONF, fd, oname));
}

long 
pathconf(const char *path, int name)
{
	int oname = fixup_pathconf(name);
	return(syscall(_SCO_PATHCONF, path, oname));
}

int
_xmknod(int ver, const char *path, mode_t mode, dev_t dev)
{
	major_t major;
	minor_t minor;
	ushort_t odev;

	major = dev >> NBITSMINOR;
	minor = dev & MAXMIN;
	if (major > OMAXMAJ || minor > OMAXMIN)
		odev = O_NODEV;
	else
		odev = (((ushort_t)major) << ONBITSMINOR) 
			| (ushort_t)minor;
	return(syscall(_SCO_MKNOD, path, mode, odev));
}

int
waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
	osr5_siginfo_t    osr5_info;
	int osr5_options = 0;
	int ret;
	if (options & WEXITED)
		osr5_options |= _SCO_WEXITED;
	if (options & WSTOPPED)
		osr5_options |= _SCO_WSTOPPED;
	if (options & WCONTINUED)
		osr5_options |= _SCO_WCONTINUED;
	if (options & WNOWAIT)
		osr5_options |= _SCO_WNOWAIT;
	if (options & WNOHANG)
		/* does osr5 waitid support nohang? */
		osr5_options |= _SCO_WNOHANG;
	memset((char *)&osr5_info, 0, sizeof(osr5_info));
	if ((ret = syscall(_SCO_WAITID, idtype, id, &osr5_info, osr5_options)) != -1)
	{
		memset((char *)infop, 0, sizeof(siginfo_t));
		infop->si_signo = osr5_info.si_signo;
		infop->si_code = osr5_info.si_code;
		switch(osr5_info.si_signo)
		{
		case SIGCLD:
			infop->_data._proc._pid = osr5_info._data._proc._pid;
			infop->_data._proc._pdata._kill._uid = osr5_info._data._proc._pdata._kill._uid;
			infop->_data._proc._pdata._cld._status = osr5_info._data._proc._pdata._cld._status;
			break;
		case SIGSEGV:
		case SIGBUS:
		case SIGILL:
		case SIGFPE:
			infop->_data._fault._addr = osr5_info._data._fault._addr;
			break;
		case SIGPOLL:
		case SIGXFSZ:
			infop->_data._file._fd = osr5_info._data._sifile._fd;
			infop->_data._file._band = osr5_info._data._sifile._band;
			break;
		}
	}
	return(ret);
}

extern int _osr5_waitpid(short, int *, int);

pid_t
waitpid(pid_t pid, int *stat_loc, int options)
{
	int osr5_options = 0;
	if (options & WNOHANG)
		osr5_options |= _SCO_WNOHANG;
	if (options & WUNTRACED)
		osr5_options |= _SCO_WUNTRACED;
	if (options & WCONTINUED)
		osr5_options |= _SCO_WCONTINUED;
	if (options & WNOWAIT)
		osr5_options |= _SCO_WNOWAIT;
	return(_osr5_waitpid((short)pid, stat_loc, osr5_options));
}

int 
fcntl(int fd, int cmd, ...)
{
	va_list	ap;
	flock_t	*fptr;
	int	 ret;
	int	ocmd = cmd;

	union {
		int fd2;
		osr5_flock_t *ofptr;
		char *ptr;
	} osr5_fcu;

	osr5_flock_t ofc;

	va_start(ap, cmd);
	osr5_fcu.ptr = va_arg(ap, char *);
	fptr = (flock_t *)osr5_fcu.ptr;

	switch(cmd)
	{
		case F_DUP2:
			(void)close(osr5_fcu.fd2);
			ocmd = F_DUPFD;
			break;
		case F_GETLK:
			/* use old size flock_t structure */
			ocmd = _SCO_F_GETLK;
			ofc.l_type = fptr->l_type;
			ofc.l_whence = fptr->l_whence;
			ofc.l_start = fptr->l_start;
			ofc.l_len = fptr->l_len;
			ofc.l_sysid = fptr->l_sysid;
			ofc.l_pid = fptr->l_pid;
			osr5_fcu.ofptr = &ofc;
			break;
		default:
			break;
	}

	if ((ret = syscall(_SCO_FCNTL, fd, ocmd, osr5_fcu)) != -1)
	{
		if (cmd == F_GETLK)
		{
                        fptr->l_type = ofc.l_type;
                        fptr->l_whence = ofc.l_whence;
                        fptr->l_start = ofc.l_start;
                        fptr->l_len = ofc.l_len;
                        fptr->l_sysid = ofc.l_sysid;
                        fptr->l_pid = ofc.l_pid;
	        }
	}
	return ret;
}

/* ARGSUSED */
static void
awake(sig)
	int sig;
{
}

/* sleep for OSR5 - no system call available - we copy code
 * here rather than using code in port/gen because of problems
 * with i386/sys/sleep.s overwriting the port/gen/sleep.c.
 * we assume we have SVR4 style signals available
 */
unsigned
sleep(sleep_tm)
unsigned sleep_tm;
{
	int  alrm_flg;
	int  errsave;
	unsigned unslept, alrm_tm, left_ovr;

	struct osr5_sigaction oact, nact;
	sigset_t alrm_mask;
	sigset_t nset;
	sigset_t oset;

	if (sleep_tm == 0)
		return(0);

	alrm_tm = alarm(0);			/* prev. alarm time */
	nact._sa_function._sa_handler = awake;
	nact.sa_flags = 0;
	nact.sa_mask = 0;
	__sigaction(SIGALRM, &nact, &oact);

	alrm_flg = 0;
	left_ovr = 0;

	if (alrm_tm != 0) {	/* skip all this if no prev. alarm */
		if (alrm_tm > sleep_tm) {
			alrm_tm -= sleep_tm;
			++alrm_flg;
		} else {
			left_ovr = sleep_tm - alrm_tm;
			sleep_tm = alrm_tm;
			alrm_tm = 0;
			--alrm_flg;
			__sigaction(SIGALRM, &oact, (struct sigaction *)0);
		}
	}

	sigemptyset(&alrm_mask);
	sigaddset(&alrm_mask, SIGALRM);
	syscall(_SCO_SIGPROCMASK, _SCO_SIG_BLOCK, &alrm_mask, &oset);
	nset = oset;
	sigdelset(&nset,SIGALRM);

	/* preserve errno; sigsuspend MUST return EINTR */
	errsave = errno;
	(void)alarm(sleep_tm);
	sigsuspend(&nset);
	errno = errsave;

	unslept = alarm(0);
	if (!sigismember(&oset, SIGALRM))
		syscall(_SCO_SIGPROCMASK, _SCO_SIG_UNBLOCK, &alrm_mask, 0);
	if (alrm_flg >= 0)
		__sigaction(SIGALRM, &oact, (struct sigaction *)0);

	if(alrm_flg > 0 || (alrm_flg < 0 && unslept != 0))
		(void) alarm(alrm_tm + unslept);
	return(left_ovr + unslept);
}

int
_xioctl(unsigned long vers, int fd, int cmd, ...)
{
	va_list	ap;
	char	*arg;

	va_start(ap, cmd);
	arg = va_arg(ap, char *); /* mimics implementation below */
	va_end(ap);
	return ioctl(fd, cmd, arg);
}

int 
ioctl(int fd, int cmd, ...)
{
	struct osr5_strrecvfd	osr5_rcv;
	struct osr5_termios	osr5_tios;
	ushort_t		osr5_pid;
	char			*save_arg;
	int			ret;
	va_list			ap;
	tcflag_t		save_flags;
	tcflag_t		oflags, nflags;
	int			sco_cmd;

	union {
		int	arg;
		int	*int_ptr;
		ushort_t *osr5_pid_ptr;
		void	*ioptr;
	} osr5_ioctl; 

	sco_cmd = cmd;

	va_start(ap, cmd);
	save_arg = va_arg(ap, char *);
	va_end(ap);

	switch(cmd)
	{	
	case I_RECVFD:
		osr5_ioctl.ioptr = &osr5_rcv;
		break;

	case TCSETS:
	case TCSETSW:
	case TCSETSF:
		oflags = ((struct termios *)save_arg)->c_lflag;
		nflags = oflags & ~(TOSTOP|IEXTEN|ECHOCTL);
		memcpy(&osr5_tios, save_arg, sizeof(struct termios));
		if (oflags & (TOSTOP|IEXTEN|ECHOCTL))
		{
			if (oflags & TOSTOP)
			{
				nflags |= _SCO_TOSTOP;
			}
			if (oflags & IEXTEN)
			{
				nflags |= _SCO_IEXTEN;
			}
			if (oflags & ECHOCTL)
			{
				nflags |= _SCO_ECHOCTL;
			}
			osr5_tios.c_lflag = nflags;
		}
		osr5_tios.c_cc[_SCO_VSTART] = ((struct termios *)save_arg)->c_cc[VSTART];
		osr5_tios.c_cc[_SCO_VSTOP] = ((struct termios *)save_arg)->c_cc[VSTOP];
		osr5_tios.c_cc[_SCO_VDSUSP] = ((struct termios *)save_arg)->c_cc[VDSUSP];
		osr5_tios.c_cc[_SCO_VREPRINT] = ((struct termios *)save_arg)->c_cc[VREPRINT];
		osr5_tios.c_cc[19] = 0;
		osr5_tios.c_cc[20] = 0;
		/*FALLTHROUGH*/

	case TCGETS:
		osr5_ioctl.ioptr = &osr5_tios;
		break;

	case TCSETA:
	case TCSETAW:
	case TCSETAF:
		save_flags = ((struct termio *)save_arg)->c_lflag;
		nflags = save_flags & ~(TOSTOP|IEXTEN|ECHOCTL);
		if (save_flags & (TOSTOP|IEXTEN|ECHOCTL))
		{
			if (save_flags & TOSTOP)
			{
				nflags |= _SCO_TOSTOP;
			}
			if (save_flags & IEXTEN)
			{
				nflags |= _SCO_IEXTEN;
			}
			if (save_flags & ECHOCTL)
			{
				nflags |= _SCO_ECHOCTL;
			}
			((struct termio *)save_arg)->c_lflag = nflags;
		}
		osr5_ioctl.ioptr = (struct termio *)save_arg;
		break;

	case TIOCSPGRP:
		sco_cmd = _SCO_TIOCSPGRP;
		osr5_pid = *(pid_t *)save_arg;
		osr5_ioctl.osr5_pid_ptr = &osr5_pid;
		break;

	case TIOCGPGRP:
		sco_cmd = _SCO_TIOCGPGRP;
		osr5_ioctl.osr5_pid_ptr = &osr5_pid;
		break;

	case TIOCGSID:
		sco_cmd = _SCO_TIOCGSID;
		osr5_ioctl.osr5_pid_ptr = &osr5_pid;
		break;

	default:
		osr5_ioctl.arg = (int)save_arg;	/* catch all int cmds */
	}

	if ((ret = syscall(_SCO_IOCTL, fd, sco_cmd, osr5_ioctl.arg)) != -1)
	{
		switch(cmd)
		{
		case I_RECVFD:
			 ((struct strrecvfd *)save_arg)->fd = osr5_rcv.fd;
			 ((struct strrecvfd *)save_arg)->uid = osr5_rcv.uid;
			 ((struct strrecvfd *)save_arg)->gid = osr5_rcv.gid;
			 memcpy(((struct strrecvfd *)save_arg)->__fill, osr5_rcv.fill, 8);
			 break;

		case TCGETS:
			memcpy((struct termios *)save_arg, &osr5_tios, sizeof(struct termios));
			oflags = osr5_tios.c_lflag;
			nflags = oflags & ~(_SCO_TOSTOP|_SCO_IEXTEN|_SCO_ECHOCTL);
			if (oflags & (_SCO_TOSTOP|_SCO_IEXTEN|_SCO_ECHOCTL))
			{
				if (oflags & _SCO_TOSTOP)
				{
					nflags |= TOSTOP;
				}
				if (oflags & _SCO_IEXTEN)
				{
					nflags |= IEXTEN;
				}
				if (oflags & _SCO_ECHOCTL)
				{
					nflags |= ECHOCTL;
				}
				((struct termios *)save_arg)->c_lflag = nflags;
			}
			((struct termios *)save_arg)->c_cc[VSTART]  = osr5_tios.c_cc[_SCO_VSTART];
			((struct termios *)save_arg)->c_cc[VSTOP]  = osr5_tios.c_cc[_SCO_VSTOP];
			((struct termios *)save_arg)->c_cc[VDSUSP] = osr5_tios.c_cc[_SCO_VDSUSP];
			((struct termios *)save_arg)->c_cc[VREPRINT] = osr5_tios.c_cc[_SCO_VREPRINT];
			break;

		case TCGETA:
			oflags = ((struct termio *)save_arg)->c_lflag;
			nflags  = oflags & ~(_SCO_TOSTOP|_SCO_IEXTEN|_SCO_ECHOCTL);
			if (oflags & (_SCO_TOSTOP|_SCO_IEXTEN|_SCO_ECHOCTL))
			{
				if (oflags & _SCO_TOSTOP)
				{
					nflags |= TOSTOP;
				}
				if (oflags & _SCO_IEXTEN)
				{
					nflags |= IEXTEN;
				}
				if (oflags & _SCO_ECHOCTL)
				{
					nflags |= ECHOCTL;
				}
				((struct termio *)save_arg)->c_lflag = nflags;
			}
			break;

		case TCSETA:
		case TCSETAW:
		case TCSETAF:
			((struct termio *)save_arg)->c_lflag = save_flags;
			break;
			
		case TIOCGPGRP:
		case TIOCGSID:
			 *save_arg = (pid_t)osr5_pid;
		}
	}
	return ret;
}

int
ptrace(int request, pid_t pid, int addr, int data)
{
	errno = 0;
	return(syscall(_SCO_PTRACE, request, pid, addr, data));
}

extern int _sys_gettimeofday(struct timeval *, void *);

int
gettimeofday(struct timeval *tv, void *tz)
{
	return(_sys_gettimeofday(tv, tz));
}

int
select(int nfds, fd_set *rdfds, fd_set *wfds, fd_set *exfds,
	struct timeval *timeout)
{
	fd_set	except;
	fd_set	*eptr;
	int	i;
	int	ret;

	if (rdfds)
	{
		/* ensure that any bits set in rdfds are also set in
		 * exfds - OSR5 streams M_PROTO messages are handled
		 * as exceptions
		 */
		eptr = &except;
		if (exfds)
			memcpy((char *)eptr, (char *)exfds, 
				sizeof(fd_set));
		else
			memset((char *)eptr, 0, sizeof(fd_set));

		for(i = 0; i < nfds; i++)
		{
			if (FD_ISSET(i, rdfds))
				FD_SET(i, eptr);
		}
	}
	else
		eptr = exfds;

	ret = syscall(_SCO_SELECT, nfds, rdfds, wfds, eptr, timeout);

	if (!rdfds)
		return ret;

	if (ret > 0)
	{
		/* turn off all except bits that the user hasn't
		 * asked for - make them read bits, instead
		 */
		for(i = 0; i < nfds; i++)
		{
			if (FD_ISSET(i, &except))
			{
				if (!exfds || !FD_ISSET(i, exfds))
				{
					FD_CLR(i, &except);
					if (FD_ISSET(i, rdfds))
					{
						/* both read and except
						 * set
						 */
						 ret -= 1;
					}
					else
						FD_SET(i, rdfds);
				}
			}
		}
		if (exfds)
			memcpy((char *)exfds, (char *)&except,
				sizeof(fd_set));
	}
	else if ((ret == 0) && exfds)
	{
		memset((char *)exfds, 0, sizeof(fd_set));
	}
	return ret;
}

int 
__sigfillset(sigset_t *set)
{
	set->sa_sigbits[0]  = ~0;
	set->sa_sigbits[1]  = 0;
	set->sa_sigbits[2]  = 0;
	set->sa_sigbits[3]  = 0;
	return 0;
}

/* security calls - these are the ones that return ENOPKG
 * if you don't have the auditing or acl packages installed;
 * all the calls are aliased to _acl.
 */

int
acl()
{
	errno = ENOPKG;
	return -1;
}

/* get the yellow pages or secure rpc domain name */
static int
yp_domain(int cmd, char *buf, int count)
{
	struct domnam_args	domnam_args;
	int 			nfsdev;
	int 			nfscmd;
	int			retval;
	int			serrno;

	domnam_args.name = buf;
	domnam_args.namelen = count;
	if ((nfsdev = open("/dev/nfsd", O_RDONLY)) == -1)
		return -1;
	if (cmd == SI_SET_SRPC_DOMAIN)
		nfscmd = _SCO_NIOCSETDOMNAM;
	else
		nfscmd = _SCO_NIOCGETDOMNAM;
	retval = ioctl(nfsdev, nfscmd, (char *)&domnam_args);
	serrno = errno;
	close(nfsdev);
	errno = serrno;
	return retval;
}

/* get the IP domain name */
static int
ip_domain(char *buf, int len)
{
	struct osr5_socksysreq	sreq;
	int			sdev;
	int			retval;
	int			serrno;

	if ((sdev = open(_SCO_PATH_SOCKSYS, O_RDONLY)) == -1)
		return -1;
	sreq.args[0] = _SCO_SO_GETIPDOMAIN;
	sreq.args[1] = (int)buf;
	sreq.args[2] = len;
	retval = ioctl(sdev, _SCO_SIOSOCKSYS, &sreq);
	serrno = errno;
	close(sdev);
	errno = serrno;
	return retval;
}

int
sysinfo(int cmd, char *buf, long count)
{
	int			len;
	int			min;
	char			*p = 0;
	struct scoutsname	scostr;
	char			hostname[_SCO_MAXHOSTNAMELEN];
	struct osr5_utsname	osr5_name;

	static const char	arch[] = "IA32";
	static const char	old_arch[] = "x86at";
	static const char	provider[] = "SCO";
	static const char	release[] = "3.2";
	static const char	sysname[] = "SCO_SV";
	static const char	os_base[] = "UNIX_SVR3";
	static const char	m386[] = "386";
	static const char	m486[] = "486";
	static const char	ppro[] = "Pentium Pro";
	static const char	pII[] = "Pentium II";
	static const char	unlimited[] = "unlimited";
	static const char	gen_mp[] = "Generic MP";
	static const char	gen_at[] = "Generic AT";

	switch(cmd)
	{
#ifdef SI_OS_BASE
	case SI_OS_PROVIDER:
		p = (char *)provider;
		break;
	case SI_OS_BASE:
		p = (char *)os_base;
		break;
	case __O_SI_ARCHITECTURE:
		p = (char *)old_arch;
		break;
#endif
	case SI_ARCHITECTURE:
		p = (char *)arch;
		break;
	case SI_RELEASE:
		p = (char *)release;
		break;
	case SI_SYSNAME:
		p = (char *)sysname;
		break;
	case SI_HOSTNAME:
#ifdef SI_KERNEL_STAMP
	case SI_KERNEL_STAMP:
	case SI_USER_LIMIT:
	case __O_SI_HOSTNAME:
	case __O_SI_SYSNAME:
#endif
	case SI_HW_SERIAL:
	case SI_MACHINE:
	case SI_VERSION:
	case SI_HW_PROVIDER:
	{
		if (syscall(_SCO_INFO, &scostr, sizeof(scostr)) == -1)
			return -1;
		switch(cmd)
		{
		case SI_HOSTNAME:
			strcpy(hostname, scostr.nodename);
			len = strlen(hostname) + 1; /* 1 for . */
			p = hostname + len;
			*p = 0;
			if (ip_domain(p, sizeof(hostname) - len) != 0)
				return -1;
			if ((*p != 0) && (*p != '.') && 
				(strchr(p, '.')))
			{
				*(p-1) = '.';
			}
			p = hostname;
			break;
		case SI_HW_SERIAL:
			p = scostr.sysserial;
			break;
		case SI_MACHINE:
			if (strcmp(scostr.machine, "i80386") == 0)
				p = (char *)m386;
			else if (strcmp(scostr.machine, "i80486") == 0)
				p = (char *)m486;
			else if (strcmp(scostr.machine, "Pent Pro")
				== 0)
				p = (char *)ppro;
			else if (strcmp(scostr.machine, "Pent II") == 0)
				p = (char *)pII;
			else
				p = scostr.machine;
			break;
#ifdef SI_KERNEL_STAMP
		case SI_KERNEL_STAMP:
			p = scostr.kernelid;
			break;
		case __O_SI_HOSTNAME:
			p = scostr.nodename;
			break;
		case __O_SI_SYSNAME:
			p = scostr.sysname;
			break;
		case SI_USER_LIMIT:
			if (strcmp(scostr.numuser, "unlim") == 0)
				p = (char *)unlimited;
			else 
			{
				char	*ptr;
				p = scostr.numuser;
				/* value may be "nnn-user" */
				if ((ptr = strchr(p, '-')) != 0)
					*ptr = 0;
			}
			break;
#endif
		case SI_VERSION:
			p = scostr.release;
			/* expect "3.2v5.0.x" */
			if (strncmp(p, "3.2v", 4) == 0)
				p += 4;
			break;
		case SI_HW_PROVIDER:
			/* a best guess */
			if (scostr.numcpu > 1)
				p = (char *)gen_mp;
			else
				p = (char *)gen_at;
			break;
		}
		break;
	}

#ifdef __O_SI_MACHINE
	case __O_SI_MACHINE:
	case __O_SI_HW_PROVIDER:
		if (_osr5_uname(&osr5_name) == -1)
			return -1;
		p = osr5_name.machine;
		break;
#endif
	case SI_SET_SRPC_DOMAIN:
		if (yp_domain(cmd, buf, count) != 0)
			return -1;
		return len;

	case SI_SRPC_DOMAIN:
		p = hostname;
		*p = 0;
		if (yp_domain(cmd, hostname, sizeof(hostname)) != 0)
			return -1;
		break;

#ifdef __O_SI_SET_HOSTNAME
	case __O_SI_SET_HOSTNAME:
		if (strchr(buf, '.') != 0)
			/* we cannot set domain name */
			break;
		if (sysi86(_SCO_SETNAME, buf) == -1)
			return -1;
		return count;
#endif
	case SI_SET_HOSTNAME:
	case SI_INITTAB_NAME:
		break;
	default:
		errno = EINVAL;
		return -1;
	}
	if (!p)
	{
		errno = ENOSYS;
		return -1;
	}
	len = strlen(p);
	if (len < count)
		min = len;
	else
		min = count - 1;
	memcpy(buf, p, min);
	buf[min] = 0;
	return (len + 1);
}

#endif /*GEMINI_ON_OSR5*/

#if defined(GEMINI_ON_OSR5) || defined(GEMINI_ON_UW2)
#pragma weak lseek64 = _lseek64

long long
lseek64(int fd, long long off, int whence)
{
	if (off != (long)off)
	{
		errno = EOVERFLOW;
		return -1;
	}
	return lseek(fd, (long)off, whence);
}
#endif /*defined(GEMINI_ON_OSR5) || defined(GEMINI_ON_UW2)*/

#ifdef GEMINI_ON_UW2

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/syscall.h>
#include <sys/processor.h>
#include <sys/keyctl.h>
#include <netdb.h>

#define _SCO_INFO	12840

#pragma weak sysinfo = _sysinfo
#pragma weak _abi_sysinfo = _sysinfo

/* code taken form libsocket - used to get system ip domain name */

/* _dname_read(file_to_search, buffer_to_fill, size_of_buffer)
 * This code is intended to get the default domain name
 * from the indicated file in the spirit of code in
 * common/cmd/cmd-inet/usr.sbin/in.named/ns_init.c, since
 * we would rather not go to the network for it.
 */

static void
dname_read(const char *config_file, char *dname, uint_t size_dname)
{
	FILE	*fp;
	char	*cp;
	uint_t	dotchars;
	char	buf[BUFSIZ];

	if ((fp = fopen(config_file, "r")) == 0) 
		return;
	/* read the config file, looking for a line beginning with
	 * "domain"
	 */
	while (fgets(buf, BUFSIZ, fp) != 0) 
	{
		if (strncmp(buf, "domain", sizeof("domain") - 1) == 0) 
		{
			cp = buf + sizeof("domain") - 1;
			while (*cp == ' ' || *cp == '\t')
				cp++;
			if (*cp != 0) 
			{
				/* buf always null-terminated */
				memccpy(dname, cp, 0, size_dname);
				if ((cp = strchr(dname, '\n')) != 0)
					*cp = '\0';
				break;
			}
		}
	}
	fclose(fp);

	/* Since this is an administrator edited file, there
	 * may be a trailing "." for the root domain.
	 * Delete trailing '.'.
	 */
	dotchars = strlen(dname);
	while ((dotchars > 0) && (dname[--dotchars] == '.')) 
		dname[dotchars] = 0;
}


#ifndef	CONFFILE
#define	CONFFILE	"/etc/resolv.conf"
#endif

/* add domain name to hostname - local_name already
 * contains the node portion
 */
static void
get_domain(char *local_name, int local_size)
{
	uint_t		len;
	char		*p;
	const char	**filep;
	const char	*datafiles[] = {
			CONFFILE,
			"/etc/inet/named.boot",
			"/etc/named.boot",
			0 
		};


	len = strlen(local_name);
	p = local_name + len + 1;

	for (filep = datafiles; *filep; filep++)
	{
		/* extract domain name into correct place for it to be
		 * appended to uname string in local_name.  Don't put
		 * in the "." as a connector till after the domain name
		 * is there, so in abort case we won't have to mess with
		 * the string.  Make the domain name null
		 * terminated before we start.
		 */

		*p = 0;
		dname_read(*filep, p, local_size - len - 1);
		if ((*p != 0) && (*p != '.') && (strchr(p, '.'))) 
		{
			/* put the dot after the uname and before
			 * the domain name, joining the strings.
			 */
			*(p-1) = '.';
			return;
		}
	}
}


int
sysinfo(int cmd, char *buf, long count)
{
	int			len;
	int			min;
	char			*p = 0;
	struct scoutsname	scostr;
	struct utsname		utname;
	char			hostname[MAXHOSTNAMELEN];
	processor_info_t	pinfo;
	char			val[13];

	static const char	arch[] = "IA32";
	static const char	provider[] = "SCO";
	static const char	release[] = "4.2MP";
	static const char	sysname[] = "UnixWare";
	static const char	os_base[] = "UNIX_SVR4";
	static const char	m386[] = "386";
	static const char	m486[] = "486";
	static const char	mppro[] = "Pentium Pro";
	static const char	unlimited[] = "unlimited";
	static const char	gen_mp[] = "Generic MP";
	static const char	gen_at[] = "Generic AT";


	switch(cmd)
	{
#ifdef SI_OS_PROVIDER
	case SI_OS_PROVIDER:
		p = (char *)provider;
		break;
	case SI_OS_BASE:
		p = (char *)os_base;
		break;
#endif
	case SI_ARCHITECTURE:
		p = (char *)arch;
		break;
	case SI_RELEASE:
		p = (char *)release;
		break;
	case SI_SYSNAME:
		p = (char *)sysname;
		break;
	case SI_HOSTNAME:
	case SI_VERSION:
	{

		if (_nuname(&utname) == -1)
			return -1;
		if (cmd == SI_VERSION)
			p = utname.version;
		else
		{
			strcpy(hostname, utname.nodename);
			get_domain(hostname, sizeof(hostname));
			p = hostname;
		}
		break;
	}
	case SI_HW_SERIAL:
#ifdef SI_KERNEL_STAMP
	case SI_KERNEL_STAMP:
#endif
	{
		char	*ptr;

		if (syscall(_SCO_INFO, &scostr, sizeof(scostr)) == -1)
			return -1;
#ifdef SI_KERNEL_STAMP
		if (cmd == SI_KERNEL_STAMP)
			p = scostr.kernelid;
		else
#endif
			p = scostr.sysserial;
		break;
	}
	case SI_HW_PROVIDER:
	{
		long	nprocs;
		if ((nprocs = sysconf(_SC_NPROCESSORS_CONF)) == -1)
			return -1;
		if (nprocs > 1)
			p = (char *)gen_mp;
		else
			p = (char *)gen_at;
		break;
	}
	case SI_MACHINE:
		if (_processor_info(0, &pinfo) == -1)
			return -1;
		if (strcmp(pinfo.pi_processor_type, "i386") == 0)
			p = (char *)m386;
		else if (strcmp(pinfo.pi_processor_type, "i486") == 0)
			p = (char *)m486;
		else if (strcmp(pinfo.pi_processor_type, 
			"Pentium_Pro") == 0)
			p = (char *)mppro;
		else
			p = pinfo.pi_processor_type;
		break;
#ifdef SI_USER_LIMIT
	case SI_USER_LIMIT:
	{
		int	nusers;
		if ((nusers = syscall(SYS_keyctl, K_GETUSERLIMIT,
			0, 0)) < 0)
			return -1;
		if (nusers == K_UNLIMITED)
			p = (char *)unlimited;
		else
		{
			sprintf(val, "%d", nusers);
			p = val;
		}
		break;
	}
#endif
	case SI_SET_HOSTNAME:
		break;
#ifdef __O_SI_SET_HOSTNAME
	case __O_SI_SET_HOSTNAME:
		if (strchr(buf, '.') != 0)
			/* we cannot set domain name */
			break;
		/*FALLTHROUGH*/
#endif
	default:
		return(syscall(SYS_systeminfo, cmd, buf, count));
	}
	if (!p)
	{
		errno = ENOSYS;
		return -1;
	}
	len = strlen(p);
	if (len < count)
		min = len;
	else
		min = count - 1;
	memcpy(buf, p, min);
	buf[min] = 0;
	return (len + 1);
}

#endif /* GEMINI_ON_UW2 */

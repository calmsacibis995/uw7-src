#ident	"@(#)kern-i386:svc/sysent.c	1.39.6.1"
#ident	"$Header$"

#include <svc/systm.h>
#include <util/param.h>
#include <util/types.h>

/*
 * This table is the switch used to transfer to the appropriate
 * routine for processing a system call.  Each row contains the
 * number of arguments expected, and a pointer to the routine.
 */

int	nosys(), invsys(), sco_stub();

int	access(), alarm(), brk(), chdir(), chmod(), chown(), chroot();
int	close(), creat(), dup(), exec(), exece(), fcntl(), fork1(), fstat();
int	fsync(), getgid(), getpid(), getuid(), gtime(), gtty(), ioctl();
int	kill(), link(), lock_mem(), lseek(), mknod(), msgsys(); 
int	mount(), nice(), open(), pipe(), profil(), ptrace(), read(), rename();
int	semsys(), setgid(), setpgrp(), setuid(), shmsys();
int	ssig(), sigprocmask(), sigsuspend(), sigaltstack();
int	sigaction(), sigpending(), setcontext();
int	stat(), stime(), stty(), syssync(), sysacct(), times(), ulimit();
int	getrlimit(), setrlimit();
int	umask(), umount(), unlink(), utime(), utssys(), wait(), write();
int	readv(), writev();
int	online(), pause();

void	rexit();

int	rmdir(), mkdir(), getdents(), statfs(), fstatfs();
int	sysfs(), getmsg(), poll(), putmsg(), sysi86(), uadmin();
int	lstat(), symlink(), readlink();
int	setgroups(), getgroups(), fchdir(), fchown(), fchmod();
int	statvfs(), fstatvfs();

int	hrtsys();
int	priocntlsys();
int	waitsys();
int	sigsendsys();
int	mincore(), mmap(), mprotect(), munmap(), vfork();
int	xstat(), lxstat(), fxstat();
int	xmknod();
int	nuname(), lchown();
int	getpmsg(), putpmsg();
int	memcntl();
int	cxenix();
int	sysconfig();
int	adjtime();
int	systeminfo();
int	setegid(), seteuid();
int	nfssys();
int	pathconf(), fpathconf();

int	keyctl();

int     filepriv(), procpriv();
int     acl(), aclipc();
int     auditbuf(), auditctl(), auditdmp(), auditevt(), auditlog();
int     lvlproc(), lvlfile(), flvlfile(), lvldom(), lvlequal(), lvlipc();
int     devstat(), fdevstat(), lvlvfs(), mkmld(), mldmode();
int     secsys();
int	secadvise();

int	processor_bind(), processor_exbind();

int	setitimer(), getitimer(), settimeofday(), gettimeofday();
int	_lwp_create(), _lwp_wait(), _lwp_info(), __lwp_self(); 
void	_lwp_exit();
int	__lwp_private();
int	_lwp_kill();
int	pread(), pwrite(), truncate(), ftruncate();

int	prepblock(), block(), unblock(), rdblock();
int	cancelblock();
int 	forkall(), sigtimedwait();
int	_lwp_suspend(), _lwp_continue();
int	__sleep();
int	_lwp_sema_wait(), _lwp_sema_post(), _lwp_sema_trywait();
int	priocntllst();

int	modload(), moduload(), modpath(), modstat(), modadm(), getksym();

int	syscall_89(), syscall_90(), syscall_91(), syscall_92();

int	creat64();
int	fstatvfs64(), statvfs64();
int	ftruncate64(), truncate64();
int	getrlimit64(), setrlimit64();
int	lseek64();
int	mmap64();
int	pread64(), pwrite64(),dshmsys(), invlpg();
int	socketsys();
int	cg_ids(), cg_processors(), cg_info();
int	cg_bind(), cg_current(), cg_memloc();

int	pmregister();		/* licensing, and should not be public */
int	sendv(), sendv64();
/*
 * READ THIS:
 *	Any additions, modifications, or deletions
 *	to sysent[] should also be made to svc/sysentnm.c,
 *	adtent[], syscall.h, scallnam(3) and truss(1M).
 */
struct sysent sysent[] = {
	0, nosys,			/*  0 = indir */
	1, (int(*)())rexit,		/*  1 = exit */
	0, forkall,			/*  2 = fork */
	3, read,			/*  3 = read */
	3, write,			/*  4 = write */
	3, open,			/*  5 = open */
	1, close,			/*  6 = close */
	3, wait,			/*  7 = wait argnum 3 for sco compat */
	2, creat,			/*  8 = creat */
	2, link,			/*  9 = link */
	1, unlink,			/* 10 = unlink */
	2, exec,			/* 11 = exec */
	1, chdir,			/* 12 = chdir */
	0, gtime,			/* 13 = time */
	3, mknod,			/* 14 = mknod */
	2, chmod,			/* 15 = chmod */
	3, chown,			/* 16 = chown */
	1, brk,				/* 17 = brk */
	2, stat,			/* 18 = stat */
	3, lseek,			/* 19 = lseek */
	0, getpid,			/* 20 = getpid */
	6, mount,			/* 21 = mount */
	1, umount,			/* 22 = umount */
	1, setuid,			/* 23 = setuid */
	0, getuid,			/* 24 = getuid */
	1, stime,			/* 25 = stime */
	4, ptrace,			/* 26 = ptrace */
	1, alarm,			/* 27 = alarm */
	2, fstat,			/* 28 = fstat */
	0, pause,			/* 29 = pause */
	2, utime,			/* 30 = utime */
	2, stty,			/* 31 = stty */
	2, gtty,			/* 32 = gtty */
	2, access,			/* 33 = access */
	1, nice,			/* 34 = nice */
	4, statfs,			/* 35 = statfs */
	0, syssync,			/* 36 = sync */
	2, kill,			/* 37 = kill */
	4, fstatfs,			/* 38 = fstatfs */
	4, setpgrp,			/* 39 = setpgrp argnum 4 for sco */
	0, cxenix,			/* 40 = cxenix */
	1, dup,				/* 41 = dup */
	0, pipe,			/* 42 = pipe */
	1, times,			/* 43 = times */
	4, profil,			/* 44 = prof */
	1, lock_mem,			/* 45 = proc lock */
	1, setgid,			/* 46 = setgid */
	0, getgid,			/* 47 = getgid */
	2, ssig,			/* 48 = sig */
	6, msgsys,			/* 49 = IPC message */
	4, sysi86,			/* 50 = i386-specific system call */
	1, sysacct,			/* 51 = turn acct off/on */
	4, shmsys,            		/* 52 = shared memory */
	5, semsys,			/* 53 = IPC semaphores */
	3, ioctl,			/* 54 = ioctl */
	3, uadmin,			/* 55 = uadmin */
	0, nosys,			/* 56 = reserved for exch */
	4, utssys,			/* 57 = utssys */
	1, fsync,			/* 58 = fsync */
	3, exece,			/* 59 = exece */
	1, umask,			/* 60 = umask */
	1, chroot,			/* 61 = chroot */
	3, fcntl,			/* 62 = fcntl */
	2, ulimit,			/* 63 = ulimit */
	/*
	 * The following 6 entries were reserved for the UNIX PC.
	 * They have now been assigned to the NUMA-related system calls.
	 */
	3, cg_ids,			/* 64 = cg_ids */
	5, cg_processors,		/* 65 = cg_processors */
	3, cg_info,			/* 66 = cg_info */
	7, cg_bind,			/* 67 = cg_bind */
	2, cg_current,			/* 68 = cg_current */
	3, cg_memloc,			/* 69 = cg_memloc */
	0, nosys,			/* 70 = was advfs */
	0, sco_stub,			/* 71 = was unadvfs */
	0, sco_stub,			/* 72 = unused */
	0, sco_stub,			/* 73 = unused */
	0, sco_stub,			/* 74 = was rfstart */
	0, nosys,			/* 75 = reserved for rtxsys:
					 *      real-time and embedded UNIX
					 *      capabilities added by VenturCom
					 */
	0, sco_stub,			/* 76 = was rdebug */
	0, sco_stub,			/* 77 = was rfstop */
	0, sco_stub,			/* 78 = was rfsys */
	1, rmdir,			/* 79 = rmdir */
	2, mkdir,			/* 80 = mkdir */
	3, getdents,			/* 81 = getdents */
	0, nosys,			/* 82 = was libattach */
	0, nosys,			/* 83 = was libdetach */
	3, sysfs,			/* 84 = sysfs */
	4, getmsg,			/* 85 = getmsg */
	4, putmsg,			/* 86 = putmsg */
	3, poll,			/* 87 = poll */
	2, lstat,			/* 88 = lstat */
	2, syscall_89,			/* 89 = symlink (SCO=security) */
	3, syscall_90,			/* 90 = readlink (SCO=symlink) */
	2, syscall_91,			/* 91 = setgroups (SCO=lstat) */
	3, syscall_92,			/* 92 = getgroups (SCO=readlink sz=3 for it)*/
	2, fchmod,			/* 93 = fchmod */
	3, fchown,			/* 94 = fchown */
	3, sigprocmask,			/* 95 = sigprocmask */
	1, sigsuspend,			/* 96 = sigsuspend */
	2, sigaltstack,			/* 97 = sigaltstack  */
	4, sigaction,			/* 98 = sigaction */
	2, sigpending,			/* 99 = sigpending */
	2, setcontext,			/* 100 = setcontext */
	0, nosys,			/* 101 = evsys */
	0, nosys,			/* 102 = evtrapret */
	2, statvfs,			/* 103 = statvfs */
	2, fstatvfs,			/* 104 = fstatvfs */
	0, nosys,			/* 105 = reserved */
	2, nfssys,			/* 106 = nfssys */
	4, waitsys,			/* 107 = waitset */
	2, sigsendsys,			/* 108 = sigsendset */
	5, hrtsys,			/* 109 = hrtsys */
	0, nosys,			/* 110 = was acancel */
	0, nosys,			/* 111 = was async */
	4, priocntlsys,			/* 112 = priocntlsys */
	2, pathconf,			/* 113 = pathconf */
	3, mincore,			/* 114 = mincore */
	6, mmap,			/* 115 = mmap */
	3, mprotect,			/* 116 = mprotect */
	2, munmap,			/* 117 = munmap */
	2, fpathconf,			/* 118 = fpathconf */
	0, vfork,			/* 119 = vfork */
	1, fchdir,			/* 120 = fchdir */
	3, readv,			/* 121 = readv */
	3, writev,			/* 122 = writev */
	3, xstat,			/* 123 = xstat */
	3, lxstat,			/* 124 = lxstat */
	3, fxstat,			/* 125 = fxstat */
	4, xmknod,			/* 126 = xmknod */
	0, invsys,			/* 127 = reserved for clocal */
	2, setrlimit,			/* 128 = setrlimit */
	2, getrlimit,			/* 129 = getrlimit */
	3, lchown,			/* 130 = lchown */
	6, memcntl,			/* 131 = memcntl */
	5, getpmsg,			/* 132 = getpmsg */
	5, putpmsg,			/* 133 = putpmsg */
	2, rename,			/* 134 = rename */
	1, nuname,			/* 135 = nuname */
	1, setegid,			/* 136 = setegid */
	1, sysconfig,			/* 137 = sysconfig */
	2, adjtime,			/* 138 = adjtime */
	3, systeminfo,			/* 139 = systeminfo */
	1, socketsys,			/* 140 = OSR Socketsys */
	1, seteuid,			/* 141 = seteuid */
	0, nosys,			/* 142 = not used */
	3, keyctl,			/* 143 = keyctl */
	2, secsys,			/* 144 = secsys */
	4, filepriv,			/* 145 = filepriv */
	3, procpriv,			/* 146 = procpriv */
	3, devstat,			/* 147 = devstat */
	5, aclipc,			/* 148 = aclipc */
	3, fdevstat,			/* 149 = fdevstat */
	3, flvlfile,			/* 150 = flvlfile */
	3, lvlfile,			/* 151 = lvlfile */
	3, sendv,			/* 152 = sendv */
	2, lvlequal,			/* 153 = lvlequal */
	2, lvlproc,			/* 154 = lvlproc */
	0, nosys,			/* 155 = not used */
	4, lvlipc,			/* 156 = lvlipc */
	4, acl,				/* 157 = acl */
	3, auditevt,			/* 158 = auditevt */
	3, auditctl,			/* 159 = auditctl */
	2, auditdmp,			/* 160 = auditdmp */
	3, auditlog,			/* 161 = auditlog */
	3, auditbuf,			/* 162 = auditbuf */
	2, lvldom,			/* 163 = lvldom */
	3, lvlvfs,			/* 164 = lvlvfs */
	2, mkmld,			/* 165 = mkmld */
	1, mldmode,			/* 166 = mldmode */
	3, secadvise,			/* 167 = secadvise */
	2, online,			/* 168 = temporary online */
	3, setitimer,			/* 169 = setitimer */
	2, getitimer,			/* 170 = getitimer */
	1, gettimeofday,		/* 171 = gettimeofday */
	1, settimeofday,		/* 172 = settimeofday */
	2, _lwp_create,			/* 173 = lwp create */
	0, (int(*)())_lwp_exit,		/* 174 = lwp exit */
	2, _lwp_wait,			/* 175 = lwp wait */
	0, __lwp_self,			/* 176 = lwp self */
	1, _lwp_info,			/* 177 = lwp info */
	1, __lwp_private,		/* 178 = lwp private */
	4, processor_bind,		/* 179 = processor_bind */
	5, processor_exbind,		/* 180 = processor_exbind */
	0, nosys,			/* 181 = not used */
	3, sendv64,			/* 182 = sendv64 */
	3, prepblock,			/* 183 = prepblock */
	1, block,			/* 184 = block */
	1, rdblock,			/* 185 = rdblock */
	3, unblock,			/* 186 = unblock */
	0, cancelblock,			/* 187 = cancelblock */
	0, nosys,			/* 188 = not used */
	4, pread,			/* 189 = pread */
	4, pwrite,			/* 190 = pwrite */
	2, truncate,			/* 191 = truncate */
	2, ftruncate,			/* 192 = ftruncate */
	2, _lwp_kill,			/* 193 = lwp kill */
	3, sigtimedwait, 		/* 194 = sigwait */
	0, fork1,			/* 195 = fork1 */
	0, forkall,			/* 196 = forkall */
	1, modload,			/* 197 = modload */
	1, moduload,			/* 198 = moduload */
	1, modpath,			/* 199 = modpath */
	3, modstat,			/* 200 = modstat */
	3, modadm,			/* 201 = modadm */
	3, getksym,			/* 202 = getksym */
	1, _lwp_suspend,		/* 203 = lwpsuspend */
	1, _lwp_continue,		/* 204 = lwpcontinue */
	5, priocntllst,			/* 205 = priocntllist */
	1, __sleep,			/* 206 = sleep */
	1, _lwp_sema_wait,		/* 207 = _lwp_sema_wait */
	1, _lwp_sema_post,		/* 208 = _lwp_sema_post */
	1, _lwp_sema_trywait,		/* 209 = _lwp_sema_trywait */
	/*
	 * Licensing-related system calls.  Should not be public.
	 */
	2, pmregister,			/* 210 = pmregister */
	/*
	 * Reserved.
	 */
	0, nosys,			/* 211 = unused */
	0, nosys,			/* 212 = unused */
	0, nosys,			/* 213 = unused */
	0, nosys,			/* 214 = unused */
	0, nosys,			/* 215 = unused */
	/*
	 * 64 bit syscalls.
	 * syscalls which have a(n) argument(s) of type long long will have
	 * number of arguments increased by one for each long long argument.
	 */
	2, fstatvfs64,			/* 216 = fstatvfs64 */
	2, statvfs64,			/* 217 = statvfs64 */
	3, ftruncate64,			/* 218 = ftruncate64 */
	3, truncate64,			/* 219 = truncate64 */
	2, getrlimit64,			/* 220 = getrlimit64 */
	2, setrlimit64,			/* 221 = setrlimit64 */
	4, lseek64,			/* 222 = lseek64 */
	7, mmap64,			/* 223 = mmap64 */
	5, pread64,			/* 224 = pread64 */
	5, pwrite64,			/* 225 = pwrite64 */
	2, creat64,			/* 226 = creat64 */
	/*
	 * dshm related system calls.
	 */
	8, dshmsys,			/* 227 = dshmsys */
	2, invlpg,			/* 228 = invlpg */
};

unsigned sysentsize = sizeof(sysent)/sizeof(struct sysent);

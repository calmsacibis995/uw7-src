#ident	"@(#)kern-i386:svc/sysentnm.c	1.6.7.1"
#ident	"$Header$"
/*
 * svc/sysentnm.c:
 *	This file is used by the systraptrace debug facility
 *	and is included directly by svc/systrap.c when compiled
 *	with _SYSTRAPTRACE defined.
 *
 * Whenever the sysent table is modified in svc/sysent.c, this
 * table should be modified accordingly.
 */
STATIC char *sysentnames[] = {
	"nosys",		/*  0 = indir */
	"rexit",		/*  1 = exit */
	"forkall",		/*  2 = fork */
	"read",			/*  3 = read */
	"write",		/*  4 = write */
	"open",			/*  5 = open */
	"close",		/*  6 = close */
	"wait",			/*  7 = wait */
	"creat",		/*  8 = creat */
	"link",			/*  9 = link */
	"unlink",		/* 10 = unlink */
	"exec",			/* 11 = exec */
	"chdir",		/* 12 = chdir */
	"gtime",		/* 13 = time */
	"mknod",		/* 14 = mknod */
	"chmod",		/* 15 = chmod */
	"chown",		/* 16 = chown */
	"brk",			/* 17 = brk */
	"stat",			/* 18 = stat */
	"lseek",		/* 19 = lseek */
	"getpid",		/* 20 = getpid */
	"mount",		/* 21 = mount */
	"umount",		/* 22 = umount */
	"setuid",		/* 23 = setuid */
	"getuid",		/* 24 = getuid */
	"stime",		/* 25 = stime */
	"ptrace",		/* 26 = ptrace */
	"alarm",		/* 27 = alarm */
	"fstat",		/* 28 = fstat */
	"pause",		/* 29 = pause */
	"utime",		/* 30 = utime */
	"stty",			/* 31 = stty */
	"gtty",			/* 32 = gtty */
	"access",		/* 33 = access */
	"nice",			/* 34 = nice */
	"statfs",		/* 35 = statfs */
	"syssync",		/* 36 = sync */
	"kill",			/* 37 = kill */
	"fstatfs",		/* 38 = fstatfs */
	"setpgrp",		/* 39 = setpgrp */
	"cxenix",		/* 40 = cxenix */
	"dup",			/* 41 = dup */
	"pipe",			/* 42 = pipe */
	"times",		/* 43 = times */
	"profil",		/* 44 = prof */
	"lock_mem",		/* 45 = proc lock */
	"setgid",		/* 46 = setgid */
	"getgid",		/* 47 = getgid */
	"ssig",			/* 48 = sig */
	"msgsys",		/* 49 = IPC message */
	"sysi86",		/* 50 = i386-specific system call */
	"sysacct",		/* 51 = turn acct off/on */
	"shmsys",            	/* 52 = shared memory */
	"semsys",		/* 53 = IPC semaphores */
	"ioctl",		/* 54 = ioctl */
	"uadmin",		/* 55 = uadmin */
	"nosys",		/* 56 = reserved for exch */
	"utssys",		/* 57 = utssys */
	"fsync",		/* 58 = fsync */
	"exece",		/* 59 = exece */
	"umask",		/* 60 = umask */
	"chroot",		/* 61 = chroot */
	"fcntl",		/* 62 = fcntl */
	"ulimit",		/* 63 = ulimit */
	"cg_ids",		/* 64 = cg_ids */
	"cg_processors",	/* 65 = cg_processors */
	"cg_info",		/* 66 = cg_info */
	"cg_bind",		/* 67 = cg_bind */
	"cg_current",		/* 68 = cg_current */
	"cg_memloc"		/* 69 = cg_memloc */
	"nosys",		/* 70 = was advfs */
	"nosys",		/* 71 = was unadvfs */
	"nosys",		/* 72 = unused */
	"nosys",		/* 73 = unused */
	"nosys",		/* 74 = was rfstart */
	"rtxsys",		/* 75 = rtxsys() (stubbed in stubs) */
	"nosys",		/* 76 = was rdebug */
	"nosys",		/* 77 = was rfstop */
	"rfsys",		/* 78 = rfsys */
	"rmdir",		/* 79 = rmdir */
	"mkdir",		/* 80 = mkdir */
	"getdents",		/* 81 = getdents */
	"nosys",		/* 82 = was libattach */
	"nosys",		/* 83 = was libdetach */
	"sysfs",		/* 84 = sysfs */
	"getmsg",		/* 85 = getmsg */
	"putmsg",		/* 86 = putmsg */
	"poll",			/* 87 = poll */
	"lstat",		/* 88 = lstat */
	"symlink",		/* 89 = symlink */
	"readlink",		/* 90 = readlink */
	"setgroups",		/* 91 = setgroups */
	"getgroups",		/* 92 = getgroups */
	"fchmod",		/* 93 = fchmod */
	"fchown",		/* 94 = fchown */
	"sigprocmask",		/* 95 = sigprocmask */
	"sigsuspend",		/* 96 = sigsuspend */
	"sigaltstack",		/* 97 = sigaltstack  */
	"sigaction",		/* 98 = sigaction */
	"sigpending",		/* 99 = sigpending */
	"setcontext",		/* 100 = setcontext */
	"nosys",		/* 101 = evsys */
	"nosys",		/* 102 = evtrapret */
	"statvfs",		/* 103 = statvfs */
	"fstatvfs",		/* 104 = fstatvfs */
	"nosys",		/* 105 = reserved */
	"nfssys",		/* 106 = nfssys */
	"waitsys",		/* 107 = waitset */
	"sigsendsys",		/* 108 = sigsendset */
	"hrtsys",		/* 109 = hrtsys */
	"async_cancel",		/* 110 = acancel */
	"async",		/* 111 = async */
	"priocntlsys",		/* 112 = priocntlsys */
	"pathconf",		/* 113 = pathconf */
	"mincore",		/* 114 = mincore */
	"mmap",			/* 115 = mmap */
	"mprotect",		/* 116 = mprotect */
	"munmap",		/* 117 = munmap */
	"fpathconf",		/* 118 = fpathconf */
	"vfork",		/* 119 = vfork */
	"fchdir",		/* 120 = fchdir */
	"readv",		/* 121 = readv */
	"writev",		/* 122 = writev */
	"xstat",		/* 123 = xstat */
	"lxstat",		/* 124 = lxstat */
	"fxstat",		/* 125 = fxstat */
	"xmknod",		/* 126 = xmknod */
	"clocal",		/* 127 = clocal */
	"setrlimit",		/* 128 = setrlimit */
	"getrlimit",		/* 129 = getrlimit */
	"lchown",		/* 130 = lchown */
	"memcntl",		/* 131 = memcntl */
	"getpmsg",		/* 132 = getpmsg */
	"putpmsg",		/* 133 = putpmsg */
	"rename",		/* 134 = rename */
	"nuname",		/* 135 = nuname */
	"setegid",		/* 136 = setegid */
	"sysconfig",		/* 137 = sysconfig */
	"adjtime",		/* 138 = adjtime */
	"systeminfo",		/* 139 = systeminfo */
	"nosys",		/* 140 = reserved */
	"seteuid",		/* 141 = seteuid */
	"nosys",		/* 142 = not used */
	"keyctl",		/* 143 = not used */
	"secsys",		/* 144 = secsys */
	"filepriv",		/* 145 = filepriv */
	"procpriv",		/* 146 = procpriv */
	"devstat",		/* 147 = devstat */
	"aclipc",		/* 148 = aclipc */
	"fdevstat",		/* 149 = fdevstat */
	"flvlfile",		/* 150 = flvlfile */
	"lvlfile",		/* 151 = lvlfile */
	"sendv",		/* 152 = sendv */
	"lvlequal",		/* 153 = lvlequal */
	"lvlproc",		/* 154 = lvlproc */
	"nosys",		/* 155 = not used */
	"lvlipc",		/* 156 = lvlipc */
	"acl",			/* 157 = acl */
	"auditevt",		/* 158 = auditevt */
	"auditctl",		/* 159 = auditctl */
	"auditdmp",		/* 160 = auditdmp */
	"auditlog",		/* 161 = auditlog */
	"auditbuf",		/* 162 = auditbuf */
	"lvldom",		/* 163 = lvldom */
	"lvlvfs",		/* 164 = lvlvfs */
	"mkmld",		/* 165 = mkmld */
	"mldmode",		/* 166 = mldmode */
	"secadvise",		/* 167 = secadvise */
	"online",		/* 168 = temporary online */
	"setitimer",		/* 169 = setitimer */
	"getitimer",		/* 170 = getitimer */
	"gettimeofday",		/* 171 = gettimeofday */
	"settimeofday",		/* 172 = settimeofday */
	"_lwp_create",		/* 173 = lwp create */
	"_lwp_exit",		/* 174 = lwp exit */
	"_lwp_wait",		/* 175 = lwp wait */
	"__lwp_self",		/* 176 = lwp self */
	"_lwp_info",		/* 177 = lwp info */
	"__lwp_private",	/* 178 = lwp private */
	"processor_bind",	/* 179 = processor_bind */
	"processor_exbind",	/* 180 = processor_exbind */
	"nosys",		/* 181 = not used */
	"sendv64",		/* 182 = sendv64 */
	"prepblock",		/* 183 = prepblock */
	"block",		/* 184 = block */
	"rdblock",		/* 185 = rdblock */
	"unblock",		/* 186 = unblock */
	"cancelblock",		/* 187 = cancelblock */
	"setprmptoffset",	/* 188 = setprmptoffset */
	"pread",		/* 189 = pread */
	"pwrite",		/* 190 = pwrite */
	"truncate",		/* 191 = truncate */
	"ftruncate",		/* 192 = ftruncate */
	"_lwp_kill",		/* 193 = lwp kill */
	"sigwait", 		/* 194 = sigwait */
	"fork1",		/* 195 = fork1 */
	"forkall",		/* 196 = forkall */
	"modload",		/* 197 = modload */
	"moduload",		/* 198 = moduload */
	"modpath",		/* 199 = modpath */
	"modstat",		/* 200 = modstat */
	"modadm",		/* 201 = modadm */
	"getksym",		/* 202 = getksym */
	"_lwp_suspend",		/* 203 = lwpsuspend */
	"_lwp_continue",	/* 204 = lwpcontinue */
	"nosys",		/* 205 = reserved */
	"__sleep",		/* 206 = sleep */
	"_lwp_sema_wait",	/* 207 = _lwp_sema_wait */
	"_lwp_sema_post",	/* 208 = _lwp_sema_post */
	"_lwp_sema_trywait",	/* 209 = _lwp_sema_trywait */
	"pmregister",		/* 210 = pmregister */
	"nosys",		/* 211 = unused */
	"nosys",		/* 212 = unused */
	"nosys",		/* 213 = unused */
	"nosys",		/* 214 = unused */
	"nosys",		/* 215 = unused */
	"fstatvfs64",		/* 216 = fstatvfs64 */
	"statvfs64",		/* 217 = statvfs64 */
	"ftruncate64",		/* 218 = ftruncate64 */
	"truncate64",		/* 219 = truncate64 */
	"getrlimit64",		/* 220 = getrlimit64 */
	"setrlimit64",		/* 221 = setrlimit64 */
	"lseek64",		/* 222 = lseek64 */
	"mmap64",		/* 223 = mmap64 */
	"pread64",		/* 224 = pread64 */
	"pwrite64",		/* 225 = pwrite64 */
	"creat64",		/* 226 = creat64 */
	"dshmsys",		/* 227 = dshmsys */
	"invlpg",		/* 228 = invlpg */
};

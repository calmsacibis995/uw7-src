#ident  "@(#)auditent.c	1.9"
#ident  "$Header$"

#include <acc/audit/audit.h>
#include <acc/audit/audithier.h>
#include <acc/audit/auditmod.h>
#include <acc/audit/auditrec.h>
#include <acc/dac/acl.h>
#include <acc/priv/privilege.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <fs/vnode.h>
#include <mem/lock.h>
#include <proc/bind.h>
#include <proc/cred.h>
#include <proc/mman.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/ulimit.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <svc/syscall.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/processor.h>
#include <util/sysmacros.h>
#include <util/types.h>


/* System call check functions. */
STATIC	int	adt_acctchk();
STATIC	int	adt_aclchk();
STATIC	int	adt_aclipchk();
STATIC	int	adt_bindchk();
STATIC	int	adt_devchk();
STATIC	int	adt_fcevtchk();
STATIC	int	adt_cdevtchk();
STATIC	int	adt_evtchk();
STATIC	int	adt_fctlchk();
STATIC	int	adt_keychk();
STATIC	int	adt_lvlfchk();
STATIC	int	adt_lvlpchk();
STATIC	int	adt_lvlvchk();
STATIC	int	adt_mctlchk();
STATIC	int	adt_msgchk();
STATIC	int	adt_nochk();
STATIC	int	adt_onlinechk();
STATIC	int	adt_openchk();
STATIC	int	adt_pipechk();
STATIC	int	adt_plckchk();
STATIC	int	adt_privchk();
STATIC	int	adt_semchk();
STATIC	int	adt_setchk();
STATIC	int	adt_shmchk();
STATIC	int	adt_ulmchk();


/*
 * The adtent[] table which parallels the
 * sysent[] table, provides individual entry points
 * to check functions for audit events that correspond
 * to system calls. The A_CHKFUNC is passed the user
 * level arguments and the A_EVTNUM in order to decide 
 * whether or not to set the AUDITME flag.
 * NOTE: some check functions default to the event
 * number that is passed in from the adtent[]. 
 */

STATIC	struct adtent adtent[] =
{
/**********	********		*************
  A_CHKFUNC     A_EVTNUM		system call
  *********	********		*************/
  adt_nochk,	ADT_NULL,		/* 0 = unused */
  adt_nochk,	ADT_EXIT,		/* 1 = exit */
  adt_evtchk,	ADT_FORK,		/* 2 = fork */
  adt_nochk,	ADT_NULL,		/* 3 = read */
  adt_nochk,	ADT_NULL,		/* 4 = write */
  adt_openchk,	ADT_OPEN_RD,		/* 5 = open */
  adt_nochk,	ADT_NULL,		/* 6 = close */
  adt_nochk,	ADT_NULL,		/* 7 = wait */
  adt_fcevtchk,	ADT_CREATE,		/* 8 = creat */
  adt_evtchk,	ADT_LINK,		/* 9 = link */
  adt_evtchk,	ADT_UNLINK,		/* 10 = unlink */
  adt_evtchk,	ADT_EXEC,		/* 11 = exec */
  adt_cdevtchk,	ADT_CHG_DIR,		/* 12 = chdir */
  adt_nochk,	ADT_NULL,		/* 13 = time */
  adt_fcevtchk,	ADT_MK_NODE,		/* 14 = mknod */
  adt_evtchk,	ADT_DAC_MODE,		/* 15 = chmod */
  adt_evtchk,	ADT_DAC_OWN_GRP,	/* 16 = chown */
  adt_nochk,	ADT_NULL,		/* 17 = brk */
  adt_evtchk,	ADT_STATUS,		/* 18 = stat */
  adt_nochk,	ADT_NULL,		/* 19 = lseek */
  adt_nochk,	ADT_NULL,		/* 20 = getpid */
  adt_evtchk,	ADT_MOUNT,		/* 21 = mount */
  adt_evtchk,	ADT_UMOUNT,		/* 22 = umount */
  adt_evtchk,	ADT_SET_UID,		/* 23 = setuid */
  adt_nochk,	ADT_NULL,		/* 24 = getuid */
  adt_nochk,	ADT_DATE,		/* 25 = stime */
  adt_nochk,	ADT_NULL,		/* 26 = ptrace */
  adt_nochk,	ADT_NULL,		/* 27 = alarm */
  adt_evtchk,	ADT_STATUS,		/* 28 = fstat */
  adt_nochk,	ADT_NULL,		/* 29 = pause */
  adt_evtchk,	ADT_CHG_TIMES,		/* 30 = utime */
  adt_nochk,	ADT_NULL,		/* 31 = stty */
  adt_nochk,	ADT_NULL,		/* 32 = gtty */
  adt_evtchk,	ADT_ACCESS,		/* 33 = access */
  adt_nochk,	ADT_NULL,		/* 34 = nice */
  adt_nochk,	ADT_NULL,		/* 35 = statfs */
  adt_nochk,	ADT_NULL,		/* 36 = sync */
  adt_evtchk,	ADT_KILL,		/* 37 = kill */
  adt_nochk,	ADT_NULL,		/* 38 = fstatfs */
  adt_setchk,	ADT_SET_PGRPS,		/* 39 = setpgrp */
  adt_nochk,	ADT_NULL,		/* 40 = cxenix */
  adt_nochk,	ADT_NULL,		/* 41 = dup */
  adt_pipechk,	ADT_PIPE,		/* 42 = pipe */
  adt_nochk,	ADT_NULL,		/* 43 = times */
  adt_nochk,	ADT_NULL,		/* 44 = prof */
  adt_plckchk,	ADT_SCHED_LK,		/* 45 = proc lock */
  adt_evtchk,	ADT_SET_GID,		/* 46 = setgid */
  adt_nochk,	ADT_NULL,		/* 47 = getgid */
  adt_nochk,	ADT_NULL,		/* 48 = sig */
  adt_msgchk,	ADT_MSG_CTL,		/* 49 = IPC message */
  adt_nochk,	ADT_NULL,		/* 50 = 3b2-specific */
  adt_acctchk,	ADT_ACCT_ON,		/* 51 = sysacct */
  adt_shmchk,	ADT_SHM_CTL,		/* 52 = shared memory */
  adt_semchk,	ADT_SEM_CTL,		/* 53 = IPC semaphores */
  adt_evtchk,	ADT_IOCNTL,		/* 54 = ioctl */
  adt_nochk,	ADT_NULL,		/* 55 = uadmin */
  adt_nochk,	ADT_NULL,		/* 56 = reserved for exch */
  adt_nochk,	ADT_NULL,		/* 57 = utssys */
  adt_nochk,	ADT_NULL,		/* 58 = fsync */
  adt_evtchk,	ADT_EXEC,		/* 59 = exece */
  adt_nochk,	ADT_NULL,		/* 60 = umask */
  adt_cdevtchk,	ADT_CHG_ROOT,		/* 61 = chroot */
  adt_fctlchk,	ADT_FCNTL,		/* 62 = fcntl */
  adt_ulmchk,	ADT_ULIMIT,		/* 63 = ulimit */
  adt_nochk,	ADT_NULL,		/* 64 = cg_ids */
  adt_nochk,	ADT_NULL,		/* 65 = cg_processors */
  adt_nochk,	ADT_NULL,		/* 66 = cg_info */
  adt_nochk,	ADT_NULL,		/* 67 = cg_bind */
  adt_nochk,	ADT_NULL,		/* 68 = cg_current */
  adt_nochk,	ADT_NULL,		/* 69 = cg_memloc */
  adt_nochk,	ADT_NULL,		/* 70 = was advfs */
  adt_nochk,	ADT_NULL,		/* 71 = was unadvfs */
  adt_nochk,	ADT_NULL,		/* 72 = unused */
  adt_nochk,	ADT_NULL,		/* 73 = unused */
  adt_nochk,	ADT_NULL,		/* 74 = was rfstart */
  adt_nochk,	ADT_NULL,		/* 75 = rtxsys */
  adt_nochk,	ADT_NULL,		/* 76 = NCR vendor unique */
  adt_nochk,	ADT_NULL,		/* 77 = was rfstop */
  adt_nochk,	ADT_NULL,		/* 78 = rfsys */
  adt_evtchk,	ADT_RM_DIR,		/* 79 = rmdir */
  adt_fcevtchk,	ADT_MK_DIR,		/* 80 = mkdir */
  adt_nochk,	ADT_NULL,		/* 81 = getdents */
  adt_nochk,	ADT_NULL,		/* 82 = libattach */
  adt_nochk,	ADT_NULL,		/* 83 = libdetach */
  adt_nochk,	ADT_NULL,		/* 84 = sysfs */
  adt_nochk,	ADT_NULL,		/* 85 = getmsg */
  adt_nochk,	ADT_NULL,		/* 86 = putmsg */
  adt_nochk,	ADT_NULL,		/* 87 = poll*/
  adt_evtchk,	ADT_SYM_STATUS,		/* 88 = lstat */
  adt_fcevtchk,	ADT_SYM_CREATE,		/* 89 = symlink */
  adt_nochk,	ADT_NULL,		/* 90 = readlink */
  adt_evtchk,	ADT_SET_GRPS,		/* 91 = setgroups */
  adt_nochk,	ADT_NULL,		/* 92 = getgroups */
  adt_evtchk,	ADT_DAC_MODE,		/* 93 = fchmod */
  adt_evtchk,	ADT_DAC_OWN_GRP,	/* 94 = fchown */
  adt_nochk,	ADT_NULL,		/* 95 = sigprocmask */
  adt_nochk,	ADT_NULL,		/* 96 = sigsuspend */
  adt_nochk,	ADT_NULL,		/* 97 = sigaltstack */
  adt_nochk,	ADT_NULL,		/* 98 = sigaction */
  adt_nochk,	ADT_NULL,		/* 99 = sigpending */
  adt_nochk,	ADT_NULL,		/* 100 = setcontext */
  adt_nochk,	ADT_NULL,		/* 101 = evsys */
  adt_nochk,	ADT_NULL,		/* 102 = evtrapret */
  adt_nochk,	ADT_NULL,		/* 103 = statvfs */
  adt_nochk,	ADT_NULL,		/* 104 = fstatvfs */
  adt_nochk,	ADT_NULL,		/* 105 = reserved */
  adt_nochk,	ADT_NULL,		/* 106 = nfssys */
  adt_nochk,	ADT_NULL,		/* 107 = waitset */
  adt_evtchk,	ADT_KILL,		/* 108 = sigsendset */
  adt_nochk,	ADT_NULL,		/* 109 = hrtsys */
  adt_nochk,	ADT_NULL,		/* 110 = acancel */
  adt_nochk,	ADT_NULL,		/* 111 = async */
  adt_nochk,	ADT_NULL,		/* 112 = priocntlsys */
  adt_nochk,	ADT_NULL,		/* 113 = pathconf */
  adt_nochk,	ADT_NULL,		/* 114 = mincore */
  adt_nochk,	ADT_NULL,		/* 115 = mmap */
  adt_nochk,	ADT_NULL,		/* 116 = mprotect */
  adt_nochk,	ADT_NULL,		/* 117 = munmap */
  adt_nochk,	ADT_NULL,		/* 118 = fpathconf */
  adt_evtchk,	ADT_FORK,		/* 119 = vfork */
  adt_evtchk,	ADT_CHG_DIR,		/* 120 = fchdir */
  adt_nochk,	ADT_NULL,		/* 121 = readv */
  adt_nochk,	ADT_NULL,		/* 122 = writev */
  adt_evtchk,	ADT_STATUS,		/* 123 = xstat expanded stat */
  adt_evtchk,	ADT_SYM_STATUS,		/* 124 = lxstat expanded symlink stat */
  adt_evtchk,	ADT_STATUS,		/* 125 = fxstat expanded fd stat*/
  adt_fcevtchk,	ADT_MK_NODE,		/* 126 = xmknod for dev_t */
  adt_nochk,	ADT_NULL,		/* 127 = clocal */
  adt_evtchk,	ADT_SETRLIMIT,		/* 128 = setrlimit */
  adt_nochk,	ADT_NULL,		/* 129 = getrlimit */
  adt_evtchk,	ADT_DAC_OWN_GRP,	/* 130 = lchown */
  adt_mctlchk,	ADT_SCHED_LK,		/* 131 = memcntl */
  adt_nochk,	ADT_NULL,		/* 132 = getpmsg */
  adt_nochk,	ADT_NULL,		/* 133 = putpmsg */
  adt_evtchk,	ADT_CHG_NM,		/* 134 = rename */
  adt_nochk,	ADT_NULL,		/* 135 = nuname */
  adt_nochk,	ADT_NULL,		/* 136 = setegid */
  adt_nochk,	ADT_NULL,		/* 137 = sysconfig */
  adt_nochk,	ADT_DATE,		/* 138 = adjtime */
  adt_nochk,	ADT_NULL,		/* 139 = systeminfo */
  adt_nochk,	ADT_NULL,		/* 140 = reserved */
  adt_nochk,	ADT_NULL,		/* 141 = seteuid */
  adt_nochk,	ADT_NULL,		/* 142 = Computer Systems */
  adt_keychk,	ADT_KEYCTL,		/* 143 = keyctl */
  adt_nochk,	ADT_NULL,		/* 144 = secsys */
  adt_privchk,	ADT_FILE_PRIV,		/* 145 = filepriv */
  adt_nochk,	ADT_NULL,		/* 146 = procpriv */
  adt_devchk,	ADT_DISP_ATTR,		/* 147 = devstat */
  adt_aclipchk,	ADT_IPC_ACL,		/* 148 = aclipc */
  adt_devchk,	ADT_DISP_ATTR,		/* 149 = fdevstat */
  adt_evtchk,	ADT_FILE_LVL,		/* 150 = flvlfile */ 
  adt_lvlfchk,	ADT_FILE_LVL,		/* 151 = lvlfile */ 
  adt_nochk,	ADT_NULL,		/* 152 = sendv */
  adt_nochk,	ADT_NULL,		/* 153 = lvlequal */
  adt_lvlpchk,	ADT_PROC_LVL,		/* 154 = lvlproc */
  adt_nochk,	ADT_NULL,		/* 155 = unused */
  adt_nochk,	ADT_NULL,		/* 156 = lvlipc */
  adt_aclchk,	ADT_FILE_ACL,		/* 157 = acl */
  adt_nochk,	ADT_AUDIT_EVT,		/* 158 = auditevt */
  adt_nochk,	ADT_AUDIT_CTL,		/* 159 = auditctl */
  adt_nochk,	ADT_AUDIT_DMP,		/* 160 = auditdmp */
  adt_nochk,	ADT_AUDIT_LOG,		/* 161 = auditlog */
  adt_nochk,	ADT_AUDIT_BUF,		/* 162 = auditbuf */
  adt_nochk,	ADT_NULL,		/* 163 = lvldom */
  adt_lvlvchk,	ADT_SET_LVL_RNG,	/* 164 = lvlvfs */ 
  adt_fcevtchk,	ADT_MK_MLD,		/* 165 = mkmld */ 
  adt_nochk,	ADT_NULL,		/* 166 = mldmode */ 
  adt_nochk,	ADT_NULL,		/* 167 = secadvise */ 
  adt_onlinechk,ADT_ONLINE,		/* 168 = online */ 
  adt_nochk,	ADT_NULL,		/* 169 = setitimer */ 
  adt_nochk,	ADT_NULL,		/* 170 = getitimer */ 
  adt_nochk,	ADT_NULL,		/* 171 = gettimeofday */ 
  adt_nochk,	ADT_DATE,		/* 172 = settimeofday */ 
  adt_evtchk,	ADT_LWP_CREATE,		/* 173 = lwp create */ 
  adt_nochk,	ADT_LWP_EXIT,		/* 174 = lwp exit */ 
  adt_nochk,	ADT_NULL,		/* 175 = lwp wait */ 
  adt_nochk,	ADT_NULL,		/* 176 = lwp self */ 
  adt_nochk,	ADT_NULL,		/* 177 = lwp_info */ 
  adt_nochk,	ADT_NULL,		/* 178 = lwp private */ 
  adt_bindchk,	ADT_LWP_BIND,		/* 179 = processor_bind */ 
  adt_nochk,	ADT_NULL,		/* 180 = processor_exbind */ 
  adt_nochk,	ADT_NULL,		/* 181 = not used */ 
  adt_nochk,	ADT_NULL,		/* 182 = sendv64 */ 
  adt_nochk,	ADT_NULL,		/* 183 = prepblock */ 
  adt_nochk,	ADT_NULL,		/* 184 = block */ 
  adt_nochk,	ADT_NULL,		/* 185 = rdblock */ 
  adt_nochk,	ADT_NULL,		/* 186 = unblock */ 
  adt_nochk,	ADT_NULL,		/* 187 = cancelblock */ 
  adt_nochk,	ADT_NULL,		/* 188 = not used */ 
  adt_nochk,	ADT_NULL,		/* 189 = pread */ 
  adt_nochk,	ADT_NULL,		/* 190 = pwrite */ 
  adt_nochk,	ADT_NULL,		/* 191 = truncate */ 
  adt_nochk,	ADT_NULL,		/* 192 = ftruncate */ 
  adt_evtchk,	ADT_LWP_KILL,		/* 193 = lwp kill */ 
  adt_nochk,	ADT_NULL,		/* 194 = sigwait */ 
  adt_nochk,	ADT_FORK,		/* 195 = fork1 */ 
  adt_nochk,	ADT_FORK,		/* 196 = forkall */ 
  adt_nochk,	ADT_MODLOAD,		/* 197 = modload */ 
  adt_evtchk,	ADT_MODULOAD,		/* 198 = moduload */ 
  adt_evtchk,	ADT_MODPATH,		/* 199 = modpath */ 
  adt_nochk,	ADT_NULL,		/* 200 = modstat */ 
  adt_evtchk,	ADT_MODADM,		/* 201 = modadm */ 
  adt_nochk,	ADT_NULL,		/* 202 = modgetsym */ 
  adt_nochk,	ADT_NULL,		/* 203 = lwpsuspend */ 
  adt_nochk,	ADT_NULL,		/* 204 = lwpcontinue */ 
  adt_nochk,	ADT_NULL,		/* 205 = priocntllist */ 
  adt_nochk,	ADT_NULL,		/* 206 = sleep */ 
  adt_nochk,	ADT_NULL,		/* 207 = _lwp_sema_wait */ 
  adt_nochk,	ADT_NULL,		/* 208 = _lwp_sema_post */ 
  adt_nochk,	ADT_NULL,		/* 209 = _lwp_sema_trywait */ 
  adt_nochk,	ADT_NULL,		/* 210 = pmregister */ 
  adt_nochk,	ADT_NULL,		/* 211 = unused */ 
  adt_nochk,	ADT_NULL,		/* 212 = unused */ 
  adt_nochk,	ADT_NULL,		/* 213 = unused */ 
  adt_nochk,	ADT_NULL,		/* 214 = unused */ 
  adt_nochk,	ADT_NULL,		/* 215 = unused */ 
  adt_nochk,	ADT_NULL,		/* 216 = fstatvfs64 */
  adt_nochk,	ADT_NULL,		/* 217 = statvfs64 */
  adt_nochk,	ADT_NULL,		/* 218 = ftruncate64 */
  adt_nochk,	ADT_NULL,		/* 219 = truncate64 */
  adt_nochk,	ADT_NULL,		/* 220 = getrlimit64 */
  adt_evtchk,	ADT_SETRLIMIT64,	/* 221 = setrlimit64 */
  adt_nochk,	ADT_NULL,		/* 222 = lseek64 */
  adt_nochk,	ADT_NULL,		/* 223 = mmap64 */
  adt_nochk,	ADT_NULL,		/* 224 = pread64 */
  adt_nochk,	ADT_NULL,		/* 225 = pwrite64 */
  adt_fcevtchk,	ADT_CREATE64,		/* 226 = creat64 */
  adt_nochk,	ADT_NULL,		/* 227 = dshmsys */
  adt_nochk,	ADT_NULL		/* 228 = invlpg */
};

unsigned adtentsize = sizeof(adtent)/sizeof(struct adtent);

/*
 *
 * STATIC int adt_nochk(int *uap, int event)
 * 	Stub entry for check function for "unaudited" system calls.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
adt_nochk(int *uap, int event)
{
	return 0;
}

 
/*
 *
 * STATIC int adt_cdevtchk(int *uap, int event)
 * 	Standard entry for common system calls
 * 	which need pathname stored in alwp structure 
 * 	to update current/root  directories as well
 *	as to evaluate object level auditing.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
adt_cdevtchk(int *uap, int event)
{
	alwp_t *alwp;
	alwp = u.u_lwpp->l_auditp;
	alwp->al_event = (ushort_t)event;
	if (event && EVENTCHK(event, alwp->al_emask->ad_emask))
		SET_AUDITME(alwp);
	alwp->al_flags |= ADT_NEEDPATH;
	return 0;
}


/*
 *
 * STATIC int adt_fcevtchk(int *uap, int event)
 * 	Standard entry for common system calls which creates
 * 	creates object. Therefore stores pathname in 
 *	the alwp structure.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
adt_fcevtchk(int *uap, int event)
{
	alwp_t *alwp;
	alwp = u.u_lwpp->l_auditp;
	alwp->al_event = (ushort_t)event;
	if (event && EVENTCHK(event, alwp->al_emask->ad_emask))
		SET_AUDITME(alwp);
	alwp->al_flags |= ADT_OBJCREATE;
	return 0;
}


/*
 *
 * STATIC int adt_evtchk(int *uap, int event)
 * 	Standard entry for common system calls
 * 	which don't need their arguments evaluated
 * 	to determine event.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
adt_evtchk(int *uap, int event)
{
	return event;
}


struct opena {
	char *fnamep;
	int fmode;
	int cmode;
};
/*
 *
 * STATIC int adt_openchk(struct opena *uap, int event)
 * 	Check function for open(2) system call, 
 * 	arguments indicate whether this is an 
 * 	ADT_OPEN_RD or ADT_OPEN_WR event.
 *
 * Calling/Exit State:
 *	None.
 *
 */
STATIC int
adt_openchk(struct opena *uap, int event)
{

	if ((uap->fmode - FOPEN) & FWRITE)
		event = ADT_OPEN_WR;
	if ((int)(uap->fmode - FOPEN) & FCREAT)
		return adt_fcevtchk((int *) NULL, event);		
	else 
		return event;
}


struct setpgrpa {
	int	flag;
	int	pid;
	int	pgid;
};
/*
 *
 * STATIC int adt_setchk(struct setpgrpa *uap, int event)
 * 	Check function for setpgrp(2), setpgid(2) and setsid(2) system calls,
 * 	arguments indicate whether this is an ADT_SET_PGRPS or an
 * 	ADT_SET_SID event.  If the event was triggered by a setpgrp(2) or
 * 	a setpgid(2) call, the event is ADT_SET_PGRPS; if the event was
 * 	triggered by a setsid(2) call, the event is ADT_SET_SID.
 *
 * Calling/Exit State:
 *	None.
 *
 */
STATIC int
adt_setchk(struct setpgrpa *uap, int event)
{
	switch (uap->flag) {
	case 1: 			/* setpgrp() */
	case 5: 			/* setpgid() */
		event = ADT_SET_PGRPS;
		break;

	case 3: 			/* setsid() */
		event = ADT_SET_SID;
		break;

	default:
		event = 0;
		break;
	} 
	return event;
}


struct memcntla {
	caddr_t addr;
	size_t	len;
	int	cmd;
	caddr_t arg;
	int	attr;
	int	mask;
};
/*
 *
 * STATIC int adt_mctlchk(memcntla *uap, int event)
 * 	Check function for memcntl(2) system call,
 * 	arguments indicate whether this is an ADT_SCHED_LK event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_mctlchk(struct memcntla *uap, int event)
{
	switch (uap->cmd) {
	case MC_LOCK:
	case MC_LOCKAS:
		break;

	default:
		event = 0;
		break;
	}
	return event;
}


struct plocka {
	int	op;
};
/*
 *
 * STATIC int adt_plckchk(plocka *uap, int event)
 * 	Check function for plock(2) system call,
 * 	arguments indicate whether this is an ADT_SCHED_LK event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_plckchk(struct plocka *uap, int event)
{
	switch (uap->op) {
	case PROCLOCK:
	case TXTLOCK:
	case DATLOCK:
		break;

	default:
		event = 0;
		break;
	}
	return event;
}


struct ipca {
	int opcode;
	char *dp;
};
/*
 *
 * STATIC int adt_msgchk(ipca *uap, int event)
 * 	Check function for all IPC message system call entry points, 
 * 	determine which operation and return appropriate event.
 *
 * Calling/Exit State:
 *	None.
 *
 */
STATIC int
adt_msgchk(struct ipca *uap, int event)
{
	switch (uap->opcode) {
	case MSGGET:
		event = ADT_MSG_GET;
		break;

	case MSGRCV:
	case MSGSND:
		event = ADT_MSG_OP;
		break;

	case MSGCTL:
		break;

	default:
		event = 0;
		break;
	}
	return event;
}


struct accta {
        char *fname;
};
/*
 * STATIC int adt_acctchk(struct accta  *uargp, int event)
 * 	Check function for sysacct(2) system call,
 * 	arguments passed in is an ADT_ACCTON event.
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_acctchk(struct accta  *uargp, int event)
{
	if (uargp->fname == NULL)
		event = ADT_ACCT_OFF;
	return(event);  /* default = ADT_ACCT_ON */
}


/*
 *
 * STATIC int adt_shmchk(sttuct ipca *uap, int event)
 * 	Check function for all shared memory system call entry points, 
 * 	determine which operation and return appropriate event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_shmchk(struct ipca *uap, int event)
{
	switch (uap->opcode) {
	case SHMGET:
		event = ADT_SHM_GET;
		break;

	case SHMAT:
	case SHMDT:
		event = ADT_SHM_OP;
		break;

	case SHMCTL:
		break;

	default:
		event = 0;
		break;
	}
	return event;
}


/*
 *
 * STATIC int adt_semchk(struct ipca *uap, int event)
 * 	Check function for all IPC semaphore system call entry points, 
 * 	determine which operation and return appropriate event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_semchk(struct ipca *uap, int event)
{
	switch (uap->opcode) {
	case SEMGET:
		event = ADT_SEM_GET;
		break;

	case SEMOP:
		event = ADT_SEM_OP;
		break;

	case SEMCTL:
		break;

	default:
		event = 0;
		break;
	}
	return event;
}


struct ulimita {
	int	cmd;
	long  	arg;
};
/*
 *
 * STATIC int adt_ulmchk(struct ulimita *uap, int event)
 * 	Check function for ulimit(2) system call, 
 * 	arguments indicate whether this is an ADT_ULIMIT event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_ulmchk(struct ulimita *uap, int event)
{
	if (uap->cmd == UL_SFILLIM)
		return event;
	else
		return 0;
}


struct filepriva {
	char	*fname;
	int	cmd;
	priv_t	*privp;
	int	count;
};
/*
 *
 * STATIC int adt_privchk(struct filepriva *uap, int event)
 * 	Check function for filepriv(2) system call, 
 * 	arguments indicate whether this is an ADT_FILE_PRIV event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_privchk(struct filepriva *uap, int event)
{
	if (uap->cmd == PUTPRV) 
		return event;
	else
		return 0;
}
 

struct bind_args {
        idtype_t idtype;
        id_t    id;
        processorid_t processorid;
        processorid_t *obind;
};
/*
 *
 * STATIC int adt_bindchk(struct bind_args  *uap, int event)
 * 	Check function for processor_bind system call, arguments indicate
 * 	whether this is an ADT_LWP_BIND or ADT_LWP_UNBIND event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_bindchk(struct bind_args  *uap, int event)
{
	switch (uap->processorid) {
	case PBIND_NONE:
		event = ADT_LWP_UNBIND;
		break;
	case PBIND_QUERY:
		event = 0;
		break;
	default:
		break;
	}
	return event;
}

struct online_args {
        int processor;
        int flag;
};
/*
 *
 * STATIC int adt_onlinechk(struct online_args  *uap, int event)
 * 	Check function for online system call, arguments indicate
 * 	whether this is an P_ONLINE or P_OFFLINE.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_onlinechk(struct online_args  *uap, int event)
{
	switch (uap->flag) {
	case P_ONLINE:
	case P_OFFLINE:
		break;
	default:
		event = 0;
		break;
	}
	return event;
}

 
struct devstata {
	char 		*pathp;
	int		cmd;
	struct devstat  *devp;
};
/*
 *
 * STATIC int adt_devchk(struct devstata  *uap, int event)
 * 	Check function for devstat system call, arguments indicate
 * 	whether this is an ADT_SET_ATTR or ADT_DISP_ATTR event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_devchk(struct devstata  *uap, int event)
{
	switch (uap->cmd) {
	case DEV_SET:
		event = ADT_SET_ATTR;
		break;

	case DEV_GET:
		break;

	default:
		event = 0;
		break;	
	}
	return event;

}
 

struct aclipca {
	int type;			/* ipc object type */
	int id;				/* ipc object id   */
	int cmd;			/* aclipc cmd      */
	int nentries;			/* # of ACL entries */
	struct acl *aclbufp;		/* acl buffer ptr  */
};
/*
 *
 * STATIC int adt_aclipchk( struct aclipca  *uap, int event)
 * 	Check function for aclipc system call, 
 * 	arguments indicate whether this is an ADT_IPC_ACL event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_aclipchk(struct aclipca  *uap, int event)
{
	if (uap->cmd == ACL_SET)
		return event;
	else
		return 0;
}


struct lvlfilea {
	char 	*pathp;		/* used for filename record */
	int	cmd;
	lid_t	*lidp;
};
/*
 * STATIC int adt_lvlfchk(struct lvlfilea  *uap, int event)
 * 	Check function for lvlfile system call, 
 * 	arguments indicate whether this is an ADT_FILE_LVL event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_lvlfchk(struct lvlfilea  *uap, int event)
{
	if (uap->cmd == MAC_SET) 
		return event;
	else
		return 0;
}


struct lvlproca {
	int	cmd;
	lid_t  *lidp;
};
/*
 *
 * STATIC int adt_lvlpchk(struct lvlproca *uap, int event)
 * 	Check function for lvlproc(2) system call, 
 * 	arguments indicate whether this is an ADT_PROC_LVL event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_lvlpchk(struct lvlproca *uap, int event)
{
	if (uap->cmd == MAC_SET)
		return event;
	else
		return 0;
}


struct acla {
	char		*pathp;
	int  		cmd;
	int  		nentries;
	struct acl 	*aclbufp;
};
/*
 *
 * STATIC int adt_aclchk(struct acla *uap, int event)
 * 	Check function for acl(2) system call, 
 * 	arguments indicate whether this is an ADT_FILE_ACL event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_aclchk(struct acla *uap, int event)
{
	if (uap->cmd == ACL_SET) {
		return event;
	} else
		return 0;
}


struct lvlvfsa {
	char	*fname;
	int	cmd;
	level_t *hilevelp;
};
/*
 *
 * STATIC int adt_lvlvchk(struct lvlvfsa *uap, int event)
 * 	Check function for lvlvfs system call, 
 * 	arguments indicate whether this is an ADT_SET_LVL_RNG event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_lvlvchk(struct lvlvfsa *uap, int event)
{
	if (uap->cmd == MAC_SET)
		return event;
	else
		return 0;
}


struct fcntla {
	int	fdes;
	int	cmd;
	int	arg;
};
/*
 *
 * STATIC int adt_fctlchk(struct fcntla *uap,int event)
 * 	Check function for fcntl system call, 
 * 	arguments indicate whether this is an ADT_FCNTL event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_fctlchk(struct fcntla *uap,int event)
{

	switch (uap->cmd) {
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
	case F_ALLOCSP:
	case F_FREESP:
	case F_SETLK:
	case F_SETLKW:
	case F_RSETLK:
	case F_RSETLKW:
		break;

	default:
		event = 0;
		break;
	}
	return event;
}


struct pipea {
	int 	fildes[2];
};
/*
 *
 * STATIC int adt_pipechk(struct pipea *uap,int event)
 * 	Check function for pipe(2) system call, process lid indicates 
 *	whether this object level is to be audited.
 *
 * Calling/Exit State:
 *	This function is called after lwp updates its credentials.
 */
/* ARGSUSED */
STATIC int
adt_pipechk(struct pipea *uap, int event)
{
	return event;
}

struct keyctl_arg {
	int	cmd;
	void	*arg;
	int	nskeys;
};
/*
 *
 * STATIC int adt_keychk(struct keyctl_arg *uap, int event)
 * 	Check function for keyctl system call, 
 * 	arguments indicate whether this is an ADT_KEYCTL event.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adt_keychk(struct keyctl_arg *uap, int event)
{

	switch (uap->cmd) {
	case K_SETPROCLIMIT:
	case K_SETUSERLIMIT:
		break;
	default:
		event = 0;
		break;
	}
	return event;
}



/*
 * void adt_auditchk(int scall, int *argp)
 * 	If the specified system call maps to an auditable event,
 * 	setup the audit lwp structure so that an audit record
 * 	will be generated. 
 *
 * Calling/Exit State:
 *	Called from the systrap() to check if lwp is to be audited.
 *	No locks are held on entry and none held at exit.
 */ 
void
adt_auditchk(int scall, int *argp)
{
	ushort event;
	alwp_t *alwp;
	lwp_t *lwpp = u.u_lwpp;


	/* System call must be within adtent[]. */
	ASSERT(scall <= (sizeof(adtent) / sizeof(struct adtent)));

	/*
	 * Invoke the appropriate check function for the scall,
	 * if it returns a positive number, and the process is not 
	 * exempt, then check the process event mask to see if
	 * the event is to be audited.
	 */
	if (lwpp->l_auditp || (lwpp->l_auditp = adt_lwp())) {
		alwp = lwpp->l_auditp;
		alwp->al_event = 0;
		if (event = (ushort_t)(adtent[scall].a_chkfp)(argp,
			adtent[scall].a_evtnum)) 
			alwp->al_event = event;
			if (event && EVENTCHK(event, alwp->al_emask->ad_emask))
				SET_AUDITME(alwp);
	}
}


/*
 *
 * int adt_chk_ola(alwp_t *alwp, lid_t lid)
 * 	Check if specified level is part of the object level audit criteria
 *
 * Calling/Exit State:
 *	None.
 */
int
adt_chk_ola(alwp_t *alwp, lid_t lid)
{
	pl_t pl;
 	int i;
	int audit_obj = 0;

	ASSERT(mac_installed);
	if (alwp->al_flags & AUDITME)
		return 1;

	/*
	 * We currently have to call mac_lid_ops(),
	 * because mac_hold() is (void) and either
	 * a deadlock or PANIC(DEBUG) would occur
	 * when an invalid lid of 0 is passed.
	 */
	if (mac_lid_ops(lid, INCR) == 0) {
		pl =  RW_RDLOCK(&adt_lvlctl.lvl_mutex, PLAUDIT);
	        /* check if lid is within audit range of levels? */
		if (adt_lvlctl.lvl_flags & ADT_RMASK) {
		   if ((!(MAC_ACCESS(MACDOM,adt_lvlctl.lvl_range.a_lvlmax,lid)))
		    && (!(MAC_ACCESS(MACDOM,lid,adt_lvlctl.lvl_range.a_lvlmin)))
		    && EVENTCHK(alwp->al_event, adt_lvlctl.lvl_emask))
			audit_obj = 1;
		} else if (adt_lvlctl.lvl_flags & ADT_LMASK) {
			/*
			 * check if lid is within table of
			 * individual audit levels?
			 */
			lid_t *lvl_tbl = adt_lvlctl.lvl_tbl;
			for (i = 0; i < adt_nlvls; i++) {
				if (*lvl_tbl == 0)
					continue;
				if (((MAC_ACCESS(MACEQUAL, *lvl_tbl, lid)) == 0)
			 	    && EVENTCHK(alwp->al_event,
				    		adt_lvlctl.lvl_emask)) {
	                                audit_obj = 1;
	                                break;
				}
				lvl_tbl++;
			}
		}
		RW_UNLOCK(&adt_lvlctl.lvl_mutex, pl);
		mac_rele(lid);
		if (audit_obj)
			SET_AUDITME(alwp);
	}
	return audit_obj;
}

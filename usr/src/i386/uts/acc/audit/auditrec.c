#ident	"@(#)auditrec.c	1.4"
#ident  "$Header$"

#include <acc/audit/audit.h>
#include <acc/audit/auditmod.h>
#include <acc/audit/auditrec.h>
#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <proc/bind.h>
#include <proc/cred.h>
#include <proc/proc.h>	
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/ucontext.h>
#include <proc/user.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <svc/errno.h>
#include <proc/resource.h>
#include <proc/class/fc.h>
#include <proc/class/rt.h>
#include <proc/class/ts.h>
#include <svc/systm.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/mod/mod.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>



/* System call recording functions */
STATIC	void	adt_acl();
STATIC	void	adt_aclipc();
STATIC	void	adt_bind();
STATIC	void	adt_chmod();
STATIC	void	adt_chown();
STATIC	void	adt_devstat();
STATIC	void	adt_fchmod();
STATIC	void	adt_fchown();
STATIC	void	adt_fcntl();
STATIC	void	adt_fd();
STATIC	void	adt_fdevstat();
STATIC	void	adt_file();
STATIC	void	adt_flvlfile();
STATIC	void	adt_fork();
STATIC	void	adt_fpriv();
STATIC	void	adt_ioctl();
STATIC	void	adt_kill();
STATIC	void	adt_keyctl();
STATIC	void	adt_lwp_kill();
STATIC	void	adt_lvlfile();
STATIC	void	adt_lvlproc();
STATIC	void	adt_lwp_create();
STATIC	void	adt_memcntl();
STATIC	void	adt_modadm();
STATIC	void	adt_moduload();
STATIC	void	adt_modpath();
STATIC	void	adt_mount();
STATIC	void	adt_msg();
STATIC	void	adt_norec();
STATIC	void	adt_online();
STATIC	void	adt_pipe();
STATIC	void	adt_plock();
STATIC	void	adt_rlimit();
STATIC	void	adt_sem();
STATIC	void	adt_setgid();
STATIC	void	adt_setgroups();
STATIC	void	adt_setpgrp();
STATIC	void	adt_setuid();
STATIC	void	adt_shm();
STATIC	void	adt_ulimit();
STATIC	void	adt_utime();
void	adt_cmn();

/*
 * Following is the adtrec[] table which also parallels the
 * sysent[] table and provides individual entry points
 * to recording functions for audit events that correspond
 * to system calls. 
 */
STATIC	struct adtrec adtrec[] =
{
/**********		*************
  A_RECFUNC     	system call
  *********		*************/
  adt_norec,		/* 0 = unused */
  adt_norec,		/* 1 = exit */
  adt_fork,		/* 2 = fork */
  adt_norec,		/* 3 = read */
  adt_norec,		/* 4 = write */
  adt_file,		/* 5 = open */
  adt_norec,		/* 6 = close */
  adt_norec,		/* 7 = wait */
  adt_file,		/* 8 = creat */
  adt_file,		/* 9 = link */
  adt_file,		/* 10 = unlink */
  adt_file,		/* 11 = exec */
  adt_file,		/* 12 = chdir */
  adt_norec,		/* 13 = time */
  adt_file,		/* 14 = mknod */
  adt_chmod,		/* 15 = chmod */
  adt_chown,		/* 16 = chown */
  adt_norec,		/* 17 = brk */
  adt_file,		/* 18 = stat */
  adt_norec,		/* 19 = lseek */
  adt_norec,		/* 20 = getpid */
  adt_mount,		/* 21 = mount */
  adt_file,		/* 22 = umount */
  adt_setuid,		/* 23 = setuid */
  adt_norec,		/* 24 = getuid */
  adt_norec,		/* 25 = stime */
  adt_norec,		/* 26 = ptrace */
  adt_norec,		/* 27 = alarm */
  adt_fd,		/* 28 = fstat */
  adt_norec,		/* 29 = pause */
  adt_utime,		/* 30 = utime */
  adt_norec,		/* 31 = stty */
  adt_norec,		/* 32 = gtty */
  adt_file,		/* 33 = access */
  adt_norec,		/* 34 = nice */
  adt_norec,		/* 35 = statfs */
  adt_norec,		/* 36 = sync */
  adt_kill,		/* 37 = kill */
  adt_norec,		/* 38 = fstatfs */
  adt_setpgrp,		/* 39 = setpgrp */
  adt_norec,		/* 40 = cxenix */
  adt_norec,		/* 41 = dup */
  adt_pipe,		/* 42 = pipe */
  adt_norec,		/* 43 = times */
  adt_norec,		/* 44 = prof */
  adt_plock,		/* 45 = proc lock */
  adt_setgid,		/* 46 = setgid */
  adt_norec,		/* 47 = getgid */
  adt_norec,		/* 48 = sig */
  adt_msg,		/* 49 = IPC message */
  adt_norec,		/* 50 = 3b2-specific */
  adt_file,		/* 51 = sysacct */
  adt_shm,		/* 52 = shared memory */
  adt_sem,		/* 53 = IPC semaphores */
  adt_ioctl,		/* 54 = ioctl */
  adt_norec,		/* 55 = uadmin */
  adt_norec,		/* 56 = reserved for exch */
  adt_norec,		/* 57 = utssys */
  adt_norec,		/* 58 = fsync */
  adt_file,		/* 59 = exece */
  adt_norec,		/* 60 = umask */
  adt_file,		/* 61 = chroot */
  adt_fcntl,		/* 62 = fcntl */
  adt_ulimit,		/* 63 = ulimit */
  adt_norec,		/* 64 = unused */
  adt_norec,		/* 65 = unused */
  adt_norec,		/* 66 = unused */
  adt_norec,		/* 67 = file locking call */
  adt_norec,		/* 68 = local system calls */
  adt_norec,		/* 69 = inode open */
  adt_norec,		/* 70 = was advfs */
  adt_norec,		/* 71 = was unadvfs */
  adt_norec,		/* 72 = unused */
  adt_norec,		/* 73 = unused */
  adt_norec,		/* 74 = was rfstart */
  adt_norec,		/* 75 = rtxsys */
  adt_norec,		/* 76 = NCR vendor unique */
  adt_norec,		/* 77 = was rfstop */
  adt_norec,		/* 78 = rfsys */
  adt_file,		/* 79 = rmdir */
  adt_file,		/* 80 = mkdir */
  adt_norec,		/* 81 = getdents */
  adt_norec,		/* 82 = was libattach */
  adt_norec,		/* 83 = was libdetach */
  adt_norec,		/* 84 = sysfs */
  adt_norec,		/* 85 = getmsg */
  adt_norec,		/* 86 = putmsg */
  adt_norec,		/* 87 = poll*/
  adt_file,		/* 88 = lstat */
  adt_file,		/* 89 = symlink */
  adt_norec,		/* 90 = readlink */
  adt_setgroups,	/* 91 = setgroups */
  adt_norec,		/* 92 = getgroups */
  adt_fchmod,		/* 93 = fchmod */
  adt_fchown,		/* 94 = fchown */
  adt_norec,		/* 95 = sigprocmask */
  adt_norec,		/* 96 = sigsuspend */
  adt_norec,		/* 97 = sigaltstack */
  adt_norec,		/* 98 = sigaction */
  adt_norec,		/* 99 = sigpending */
  adt_norec,		/* 100 = setcontext */
  adt_norec,		/* 101 = evsys */
  adt_norec,		/* 102 = evtrapret */
  adt_norec,		/* 103 = statvfs */
  adt_norec,		/* 104 = fstatvfs */
  adt_norec,		/* 105 = reserved */
  adt_norec,		/* 106 = nfssys */
  adt_norec,		/* 107 = waitset */
  adt_kill,		/* 108 = sigsendset */
  adt_norec,		/* 109 = hrtsys */
  adt_norec,		/* 110 = acancel */
  adt_norec,		/* 111 = async */
  adt_norec,		/* 112 = priocntlsys */
  adt_norec,		/* 113 = pathconf */
  adt_norec,		/* 114 = mincore */
  adt_norec,		/* 115 = mmap */
  adt_norec,		/* 116 = mprotect */
  adt_norec,		/* 117 = munmap */
  adt_norec,		/* 118 = fpathconf */
  adt_fork,		/* 119 = vfork */
  adt_fd,		/* 120 = fchdir */
  adt_norec,		/* 121 = readv */
  adt_norec,		/* 122 = writev */
  adt_file,		/* 123 = xstat expanded stat */
  adt_file,		/* 124 = lxstat expanded symlink stat */
  adt_fd,		/* 125 = fxstat expanded fd stat */
  adt_file,		/* 126 = xmknod for dev_t */
  adt_norec,		/* 127 = clocal */
  adt_rlimit,		/* 128 = setrlimit */
  adt_norec,		/* 129 = getrlimit */
  adt_chown,		/* 130 = lchown */
  adt_memcntl,		/* 131 = memcntl */
  adt_norec,		/* 132 = getpmsg */
  adt_norec,		/* 133 = putpmsg */
  adt_file,		/* 134 = rename */
  adt_norec,		/* 135 = nuname */
  adt_norec,		/* 136 = setegid */
  adt_norec,		/* 137 = sysconfig */
  adt_norec,		/* 138 = adjtime */
  adt_norec,		/* 139 = systeminfo */
  adt_norec,		/* 140 = reserved */
  adt_norec,		/* 141 = seteuid */
  adt_norec,		/* 142 = Computer Systems */
  adt_keyctl,		/* 143 = keyctl */
  adt_norec,		/* 144 = secsys */
  adt_fpriv,		/* 145 = filepriv */
  adt_norec,		/* 146 = procpriv */
  adt_devstat,		/* 147 = devstat */
  adt_aclipc,		/* 148 = aclipc */
  adt_fdevstat,		/* 149 = fdevstat */
  adt_flvlfile,		/* 150 = flvlfile */ 
  adt_lvlfile,		/* 151 = lvlfile */ 
  adt_norec,		/* 152 = Sun Microsystems */
  adt_norec,		/* 153 = lvlequal */
  adt_lvlproc,		/* 154 = lvlproc */
  adt_norec,		/* 155 = unused */
  adt_norec,		/* 156 = lvlipc */
  adt_acl,		/* 157 = acl */
  adt_norec,		/* 158 = auditevt */
  adt_norec,		/* 159 = auditctl */
  adt_norec,		/* 160 = auditdmp */
  adt_norec,		/* 161 = auditlog */
  adt_norec,		/* 162 = auditbuf */
  adt_norec,		/* 163 = lvldom */
  adt_lvlfile,		/* 164 = lvlvfs */ 
  adt_file,		/* 165 = mkmld */ 
  adt_norec,		/* 166 = mldmode */ 
  adt_norec,		/* 167 = secadvise */ 
  adt_online,		/* 168 = online */ 
  adt_norec,		/* 169 = setitimer */ 
  adt_norec,		/* 170 = getitimer */ 
  adt_norec,		/* 171 = gettimeofday */ 
  adt_norec,		/* 172 = settimeofday */ 
  adt_lwp_create,	/* 173 = lwp create */ 
  adt_norec,		/* 174 = lwp exit */ 
  adt_norec,		/* 175 = lwp wait */ 
  adt_norec,		/* 176 = lwp self */ 
  adt_norec,		/* 177 = lwp info */ 
  adt_norec,		/* 178 = lwp private */ 
  adt_bind,		/* 179 = processor_bind */ 
  adt_norec,		/* 180 = processor_exbind */ 
  adt_norec,		/* 181 = not used */ 
  adt_norec,		/* 182 = sync_mailbox */ 
  adt_norec,		/* 183 = prepblock */ 
  adt_norec,		/* 184 = block */ 
  adt_norec,		/* 185 = rdblock */ 
  adt_norec,		/* 186 = unblock */ 
  adt_norec,		/* 187 = cancelclock */ 
  adt_norec,		/* 188 = not used */ 
  adt_norec,		/* 189 = pread */ 
  adt_norec,		/* 190 = pwrite */ 
  adt_norec,		/* 191 = truncate */ 
  adt_norec,		/* 192 = ftruncate */ 
  adt_lwp_kill,		/* 193 = lwp kill */ 
  adt_norec,		/* 194 = sigwait */ 
  adt_fork,		/* 195 = fork1 */ 
  adt_fork,		/* 196 = forkall */ 
  adt_norec,		/* 197 = modload */ 
  adt_moduload,		/* 198 = moduload */ 
  adt_modpath,		/* 199 = modpath */ 
  adt_norec,		/* 200 = modstat */ 
  adt_modadm,		/* 201 = modadm */ 
  adt_norec,		/* 202 = getksym */ 
  adt_norec,		/* 203 = lwpsuspend */ 
  adt_norec,		/* 204 = lwpcontinue */ 
  adt_norec,		/* 205 = priocntllist */ 
  adt_norec,		/* 206 = sleep */ 
  adt_norec,		/* 207 = _lwp_sema_wait */ 
  adt_norec,		/* 208 = _lwp_sema_post */ 
  adt_norec,		/* 209 = _lwp_sema_trywait */ 
};


/*
 * STATIC void adt_norec(int status, char *uap, rval_t *rvp)
 *	Stub entry for recording function for "non-audited" system calls.
 *
 * Calling/Exit State:
 *      This routine is called as part of the audit event table,
 *	for every system call that does not generate a record.
 *
 */
/* ARGSUSED */
STATIC void
adt_norec(int status, char *uap, rval_t *rvp)
{
 	return;
}


/*
 * STATIC void adt_record(int scall, int status,  int *argp, rval_t *rvp)
 *	Interface to syscall-specific auditing routines called 
	from systrap_cleanup().
 *
 * Calling/Exit State:
 *      This routine is called if the LWP is not EXEMPT from auditing,
 *	in order to index into the audit event table properly.
 *
 */
void
adt_record(int scall, int status, int *uap, rval_t *rvp)
{
	ASSERT(u.u_lwpp->l_auditp->al_frec1p->ar_bufp == NULL);

	if (u.u_lwpp->l_auditp->al_flags & AUDITME)
		/* 
		 * Invoke the recording function that corresponds 
		 * to the scall. 
		 */
		(adtrec[scall].a_recfp)(status, uap, rvp);
	else { 
		/* Check if privilege record needs to be generated */
		struct arecbuf *recp;
		cmnrec_t *bufp;
		recp = u.u_lwpp->l_auditp->al_bufp;
		bufp = recp->ar_bufp;

		if (bufp->c_rprivs) {
			bufp->c_rtype = CMN_R;
			bufp->c_event = ADT_PM_DENIED;
			bufp->c_size = recp->ar_inuse = SIZ_CMNREC;
			ADT_SEQNUM(bufp->c_seqnum); 
			bufp->c_pid = u.u_procp->p_pidp->pid_id;
			bufp->c_time = hrestime;
			bufp->c_status = 1;	/* EPERM */
			bufp->c_sid = u.u_procp->p_sid;
			bufp->c_lwpid = u.u_lwpp->l_lwpid;
			bufp->c_crseqnum = CRED()->cr_seqnum;
			adt_recwr(recp);

			bufp->c_rprivs = 0;
			bufp->c_rprvstat = 0;
		}
	}
	u.u_lwpp->l_auditp->al_flags &= ~(AUDITME | ADT_OBJCREATE| ADT_NEEDPATH);
}


/*
 * void adt_wrfilerec(arecbuf_t *recp, vnode_t *vp)
 *
 * Calling/Exit State:
 *    No locking on entry or exit.
 *
 * Description:
 *    Write pathname record.
 */
void
adt_wrfilerec(arecbuf_t *recp, vnode_t *vp)
{
	alwp_t *alwp = u.u_lwpp->l_auditp;
	filnmrec_t *bufp = recp->ar_bufp;
	vattr_t	vattr;
	
	ASSERT(recp);
	ASSERT(vp);
	ASSERT(alwp);

	bufp->f_rtype = FNAME_R;
	bufp->f_event = alwp->al_event;
	bufp->f_size = recp->ar_inuse;
	bufp->f_seqnum = CMN_SEQNM(alwp);

	bufp->f_vnode.v_type =  vp->v_type;
	bufp->f_vnode.v_lid = vp->v_lid;

	vattr.va_mask = AT_STAT;
	if (VOP_GETATTR(vp, &vattr, 0, CRED())) {
		bufp->f_vnode.v_fsid =  0;
		bufp->f_vnode.v_dev = 0;
		bufp->f_vnode.v_inum = 0;
	} else {
		bufp->f_vnode.v_fsid =  vattr.va_fsid;
		if (vp->v_type == VBLK || vp->v_type == VCHR)
			bufp->f_vnode.v_dev = vattr.va_rdev;
		else
			bufp->f_vnode.v_dev = vp->v_vfsp->vfs_dev;
		bufp->f_vnode.v_inum = vattr.va_nodeid;
	}

	adt_recwr(recp);
}


/*
 * void adt_filenm(pathname_t *apnp, vnode_t *vp, int error, uint_t cnt)
 * 	Construct a filename record,  and based on the flags 
 *	either dump or keep the record.
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
adt_filenm(pathname_t *apnp, vnode_t *vp, int error, uint_t cnt)
{
	char		*sbufp, *tbufp;
	size_t  	tsize;
	size_t  	apnp_sz = 0;
	size_t  	dpsz   = 0;
	alwp_t  	*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp = alwp->al_frec1p;

	ASSERT(recp->ar_bufp == NULL);

	sbufp = apnp->pn_buf;
	apnp_sz = strlen(apnp->pn_buf);

	/* compute the size for directories */
	if (*sbufp != A_SLASH) {
		if (alwp->al_cdp) {
			dpsz = alwp->al_cdp->a_len;
			cnt += alwp->al_cdp->a_cmpcnt;
		} else {
			dpsz = 1;
			cnt += 1;
		}
	} else if (alwp->al_rdp) {
		dpsz = alwp->al_rdp->a_len;
		cnt += alwp->al_rdp->a_cmpcnt;
	}

	/* Total size.  The three corresponds to 2 '/' and a NULL byte */
	tsize = dpsz + apnp_sz + 3;
	recp->ar_inuse = recp->ar_size = sizeof(filnmrec_t) + ROUND2WORD(tsize);
	tbufp = recp->ar_bufp = kmem_zalloc(recp->ar_size, KM_SLEEP);
	tbufp += sizeof(filnmrec_t);
	/* pathname does not start with A_SLASH */
	if (*sbufp != A_SLASH) {
		if (alwp->al_cdp) {
			bcopy(alwp->al_cdp->a_path, tbufp, alwp->al_cdp->a_len);
			tbufp += alwp->al_cdp->a_len - 1;
		} else 
			*tbufp++ = '*';
		*tbufp++ = '/';
	} else  if (alwp->al_rdp) {
		bcopy(alwp->al_rdp->a_path, tbufp, alwp->al_rdp->a_len);
		tbufp += alwp->al_rdp->a_len - 1;
	} 

	/* copy pathname from audit buffer */
	if (apnp_sz) {
		bcopy(sbufp, tbufp, apnp_sz);
		tbufp += apnp_sz;
	}
	*tbufp = '\0';


	((filnmrec_t *)recp->ar_bufp)->f_cmpcnt = cnt;

	if (alwp->al_flags & AUDITME) {
		if (error || !(alwp->al_flags & ADT_OBJCREATE))
			adt_wrfilerec(recp, vp);
	}
	/* The al_flags is set in the appropriate check functions. */
	if (!error && (alwp->al_flags & (ADT_NEEDPATH | ADT_OBJCREATE))) {
		alwp->al_cmpcnt = cnt;
	} else {
		kmem_free(recp->ar_bufp, recp->ar_size);
		recp->ar_bufp = NULL;
	}
}


/*
 * void adt_symrec(alwp_t *alwp, char *path, int status)
 *	Write symlink record.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 */
void
adt_symrec(alwp_t *alwp, char *path, int status)
{

	arecbuf_t *recp;
	filerec_t *bufp;
	pathname_t tpn;
	size_t size;
	int error;

	if ((error = pn_get(path, UIO_USERSPACE, &tpn)) == 0) {
		size = SIZ_FILEREC + tpn.pn_pathlen;
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
 		ADT_BUFOFLOW(alwp, size);
	}

 	recp = alwp->al_bufp;
 	bufp = recp->ar_bufp;
	bufp->cmn.c_rtype = FILE_R;
	bufp->cmn.c_size = recp->ar_inuse = SIZ_FILEREC;
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	if (error == 0) {
		bcopy(tpn.pn_buf, (caddr_t)bufp + SIZ_FILEREC, tpn.pn_pathlen);
		recp->ar_inuse += tpn.pn_pathlen;
		bufp->cmn.c_size = recp->ar_inuse;
		pn_free(&tpn);
	}
	adt_recwr(recp);
	ADT_BUFRESET(alwp);
}


/*
 * void adt_symlink(vnode_t *dvp, pathname_t *lpn, caddr_t path, int error)
 *	dump symlink record.
 *
 * Calling/Exit State:
 *	No locks are held at entry and none held at exit.
 */
void 
adt_symlink(vnode_t *dvp, pathname_t *lpn, caddr_t path, int error)
{
	alwp_t *alwp = u.u_lwpp->l_auditp;
	vnode_t *vp;

	if (error) {
		ASSERT(alwp->al_flags & AUDITME);
		ASSERT(alwp->al_frec1p->ar_bufp);
		adt_wrfilerec(alwp->al_frec1p, dvp);
		ADT_FREEPATHS(alwp);
		goto out;
	}
	ADT_FREEPATHS(alwp);
	alwp->al_flags &= ~(ADT_NEEDPATH | ADT_OBJCREATE);
        if (lookuppn(lpn, NO_FOLLOW, NULLVPP, &vp) == 0)
		VN_RELE(vp);
out:
        adt_symrec(alwp, path, error);
	alwp->al_flags &= ~AUDITME;
}


/*
 * int adt_cred(cred_t *crp, arecbuf_t **recp, lock_t *buf_mutex)
 * 	Function for attaching a new cred record to an 
 *	existing event record.
 *
 * Calling/Exit State:
 *	passed in buf_mutex must be locked. Return 0 and buf_mutex
 *	held when calling context does not need to sleep. Otherwise,
 *	function returns 1 and lock is released at PLBASE. 
 */
/* ARGSUSED */
int
adt_cred(cred_t *crp, arecbuf_t **recp, lock_t *buf_mutex)
{
	int sz;
	char *gp;
	cmnrec_t  *cmnbufp;
	credrec_t *bufp;
	alwp_t	 *alwp = u.u_lwpp->l_auditp;
	arecbuf_t *trecp = *recp;
	int ret = 0;

	sz = (crp->cr_ngroups * sizeof(gid_t));
	if (alwp && trecp->ar_size < (trecp->ar_inuse+sizeof(credrec_t)+sz)) {
		UNLOCK(buf_mutex, PLBASE);
		ret = 1;
		adt_getbuf(alwp, trecp->ar_inuse + sizeof(credrec_t) + sz);
		*recp = trecp  = alwp->al_bufp;
	}
	ASSERT((trecp->ar_inuse + sizeof(credrec_t) + sz) <= trecp->ar_size);
	cmnbufp = trecp->ar_bufp;
	cmnbufp->c_crseqnum = 0;	/* cred rec follows SPEC/FREE rec */
	bufp = (credrec_t *)((void*)((caddr_t)trecp->ar_bufp + trecp->ar_inuse));
	bufp->cr_crseqnum = CRED()->cr_seqnum;
	bufp->cr_lid = crp->cr_lid;
	bufp->cr_uid = crp->cr_uid;
	bufp->cr_gid = crp->cr_gid;
	bufp->cr_ruid = crp->cr_ruid;
	bufp->cr_rgid = crp->cr_rgid;
	bufp->cr_maxpriv = crp->cr_maxpriv;
	bufp->cr_ngroups = crp->cr_ngroups;
	gp = (char *)((caddr_t)trecp->ar_bufp + trecp->ar_inuse +
	     sizeof(credrec_t));
	bcopy(crp->cr_groups, gp, sz);
	trecp->ar_inuse += (sizeof(credrec_t) + sz);
	return ret;
}


/*
 * STATIC void adt_cmn(cmnrec_t *cmnp)
 * 	Called from each specific system call recording function to 
 * 	fill in the common area of each record.
 * Calling/Exit State:
 *	None.
 */
void
adt_cmn(cmnrec_t *cmnp)
{
	alwp_t 	*alwp; 

	alwp = u.u_lwpp->l_auditp;
	ASSERT(alwp);

	cmnp->c_event = alwp->al_event;
	cmnp->c_seqnum = CMN_SEQNM(alwp);
	cmnp->c_pid = u.u_procp->p_pidp->pid_id;
	cmnp->c_time = alwp->al_time;
	cmnp->c_sid = u.u_procp->p_sid;
	cmnp->c_lwpid = u.u_lwpp->l_lwpid;
	cmnp->c_crseqnum = CRED()->cr_seqnum;
}


/*
 * The following recording functions that are called from systrap_cleanup()
 * upon return from the auditable system calls.
 */

/*
 * STATIC void adt_fork(int status, char *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from fork().
 * 	This record is generated by PARENT process, 
 * 	so the child pid is in rvp->r_val1.
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_fork(int status, char *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	forkrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = FORK_R; 
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.f_cpid = rvp->r_val1; 	/* child process id. */
	if (status)
		bufp->spec.f_nlwp = 0; 		/* number of LWPs. */
	bufp->cmn.c_size = recp->ar_inuse = sizeof(forkrec_t) +
			 (bufp->spec.f_nlwp * sizeof(long));
	adt_recwr(recp);
	ADT_BUFRESET(u.u_lwpp->l_auditp);
}

/*
 * STATIC void adt_file(int status, char *uap, rval_t *rvp)
 * 	Called from systrap_cleanup() after return from open(), creat(),
 *	link(), unlink(), exec(), chdir(), mknod(), stat(), umount(), access(),
 *	sysacct(), exece(), chroot(), rmdir(), mkdir(), lstat(), xstat(), 
 *	lxstat(), xmknod(), rename() and mkmld().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_file(int status, char *uap, rval_t *rvp)
{
	/* These events should have a corresponding FNAME_R record */
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	filerec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = FILE_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(filerec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	adt_recwr(recp);
}



struct fchmoda {
	int	fd;
	int	fmode;
};
/*
 * STATIC void adt_fchmod(int status, struct fchmoda *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from fchmod().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_fchmod(int status, struct fchmoda *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	fchmodrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = FCHMOD_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(fchmodrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.f_chmod.c_nmode = uap->fmode;
	adt_recwr(recp);
}


struct fchowna {
	int fd;
	int uid;
	int gid;
};
/*
 * STATIC void adt_fchown(int status, struct fchowna *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from fchown().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_fchown(int status, struct fchowna *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	fchownrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = FCHOWN_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(fchownrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.f_chown.c_uid = uap->uid;
	bufp->spec.f_chown.c_gid = uap->gid;
	adt_recwr(recp);
}


struct flvlfilea {
	int fd;	
	int cmd;
	lid_t *lidp;
};
/*
 * STATIC void adt_flvlfile(int status, struct flvlfilea *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from flvlfile().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_flvlfile(int status, struct flvlfilea *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	fmacrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = FMAC_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(fmacrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	if (copyin(uap->lidp, &bufp->spec.f_lid.l_lid, sizeof(lid_t)))
		bufp->spec.f_lid.l_lid = 0;
	adt_recwr(recp);
}


struct fdevstata {
	int fd;
	int cmd;
	struct devstat *devp;
};
/*
 * STATIC void adt_fdevstat(int status, struct fdevstata *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from fdevstat().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_fdevstat(int status, struct fdevstata *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	fdevrec_t	*bufp = recp->ar_bufp;
	struct fdev_r	*specrec = &(bufp->spec);

	bufp->cmn.c_rtype = FDEV_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(fdevrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	if (copyin(uap->devp, &specrec->devstat, sizeof(struct devstat))) {
		specrec->devstat.dev_relflag = 0;
		specrec->devstat.dev_mode = 0;
		specrec->devstat.dev_hilevel = 0;
		specrec->devstat.dev_lolevel = 0;
		specrec->devstat.dev_state = 0;
		specrec->devstat.dev_usecount = 0;
	}
	adt_recwr(recp);
}


struct chmoda {		
	char *fnamep;
	int fmode;
};
/*
 * STATIC void adt_chmod(int status, struct chmoda *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from chmod().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_chmod(int status, struct chmoda *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	chmodrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = CHMOD_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(chmodrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.c_nmode = uap->fmode;
	adt_recwr(recp);
}


struct chowna {	
	char *fnamep;
	int uid;
	int gid;
};
/*
 * STATIC void adt_chown(int status, struct chowna *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from chown().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_chown(int status, struct chowna *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	chownrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = CHOWN_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(chownrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);
	bufp->spec.c_uid = uap->uid;

	bufp->spec.c_gid = uap->gid;
	adt_recwr(recp);
}


struct killa {	
	int pid;
	int sig;
};
/*
 * void adt_kill(int status, struct killa *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from kill().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
adt_kill(int status, struct killa *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	killrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = KILL_R;
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);
	bufp->spec.k_sig = uap->sig;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(killrec_t) + 
			(bufp->spec.k_entries * sizeof(pid_t));

	adt_recwr(recp);
	ADT_BUFRESET(u.u_lwpp->l_auditp);
}

struct lwp_killa {	
	lwpid_t lwpid;
	int sig;
};
/*
 * void adt_lwp_kill(int status, struct lwp_killa *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from lwp_kill().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
adt_lwp_kill(int status, struct lwp_killa *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	killrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = KILL_R;
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.k_sig = uap->sig;
	bufp->spec.k_entries = 1;
	*((caddr_t)bufp + sizeof(killrec_t)) = uap->lwpid;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(killrec_t) + sizeof(lwpid_t);
	adt_recwr(recp);
}


/*
 * void adt_pckill(pid_t pid, int sig, int error)
 * 	Called from prwritectl(), during the PCKILL case.
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
adt_pckill(pid_t pid, int sig, int err)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	killrec_t *bufp = recp->ar_bufp;
	uint_t	seqnum;

	bufp->cmn.c_rtype = KILL_R;
	bufp->cmn.c_event = ADT_KILL;
	ADT_SEQNUM(seqnum);
	bufp->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = err;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum; 

	bufp->spec.k_sig = sig;
	bufp->spec.k_entries = 1;
	*((caddr_t)bufp + sizeof(killrec_t)) = (lwpid_t)pid;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(killrec_t) + sizeof(lwpid_t);
	adt_recwr(recp);
}


/*
 * STATIC void adt_fd(int status, void *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from FD operations
 * 	fstat(), fchdir() and fxstat().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_fd(int status, void *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	fdrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = FD_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(fdrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	adt_recwr(recp);
}

/*
 * STATIC void adt_pipe(int status, void *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from pipe(2)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_pipe(int status, void *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	fdrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = FD_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(fdrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	if (status) {
		bufp->spec.f_fdinfo.v_type = 0;
		bufp->spec.f_fdinfo.v_fsid = 0;
		bufp->spec.f_fdinfo.v_dev = 0;
		bufp->spec.f_fdinfo.v_inum = 0;
		bufp->spec.f_fdinfo.v_lid = 0;
	} 
	adt_recwr(recp);
}


struct msga {
	union{
		struct msgsysa {
			int		opcode;
		}sysa;
		struct msgctla {
			int		opcode;
			int		msgid;
			int		cmd;
			struct msqid_ds	*buf;
		}ctla;
		struct msggeta {
			int		opcode;
			key_t		key;
			int		msgflg;
		}geta;
	}m;
};
/*
 * STATIC void adt_msg(int status, struct msga *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from ipc calls for messages.
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_msg(int status, struct msga *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	ipcrec_t  *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = IPCS_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(ipcrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.i_op = uap->m.sysa.opcode;
	switch (uap->m.sysa.opcode) {
	case MSGGET:
		bufp->spec.i_id = uap->m.geta.key;
		bufp->spec.i_flag = uap->m.geta.msgflg;
		bufp->spec.i_cmd = 0;
		break;

	case MSGCTL:
		bufp->spec.i_id = uap->m.ctla.msgid;
		bufp->spec.i_flag = 0;
		bufp->spec.i_cmd = uap->m.ctla.cmd;
		break;

	case MSGRCV:
	case MSGSND:
	default:
		bufp->spec.i_id = 0;
		bufp->spec.i_flag = 0;
		bufp->spec.i_cmd = 0;
		break;
	} 	/* end switch (uap->m.sysa.opcode) */
	adt_recwr(recp);
}


struct shma {
	union{
		struct shmsysa {
			int		opcode;
		}sysa;
		struct shmgeta {
			int		opcode;
			key_t		key;
			int		size;
			int		shmflg;
		}geta;
		struct shmctla {
			int		opcode;
			int		shmid;
			int		cmd;
			struct shmid_ds	*arg;
		}ctla;
	}m;
};
/*
 * STATIC void adt_shm(int status, struct shma *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from ipc calls
 *	for shared memory.
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_shm(int status, struct shma *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	ipcrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = IPCS_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(ipcrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.i_op = uap->m.sysa.opcode;
	switch (uap->m.sysa.opcode) {
	case SHMGET:
		bufp->spec.i_id = uap->m.geta.key;
		bufp->spec.i_flag = uap->m.geta.shmflg;
		bufp->spec.i_cmd = 0;
		break;

	case SHMCTL:
		bufp->spec.i_id = uap->m.ctla.shmid;
		bufp->spec.i_flag = 0;
		bufp->spec.i_cmd = uap->m.ctla.cmd;
		break;

	default:
		bufp->spec.i_id = 0;
		bufp->spec.i_flag = 0;
		bufp->spec.i_cmd = 0;
		break;
	} 	/* end switch (uap->m.sysa.opcode) */
	adt_recwr(recp);
}


struct sema {
	union{
		struct semsysa {
			int		opcode;
		}sysa;
		struct semctla {
			int		opcode;
			int		semid;
			uint		semnum;
			int		cmd;
			int		arg;
		}ctla;
		struct semgeta {
			int		opcode;
			key_t		key;
			int		nsems;
			int		semflg;
		}geta;
	}m;
};
/*
 * STATIC void adt_sem(int status, struct sema *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from ipc calls
 *	for semaphores.
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_sem(int status, struct sema *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	ipcrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = IPCS_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(ipcrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.i_op = uap->m.sysa.opcode;

	switch (uap->m.sysa.opcode) {
	case SEMGET:
		bufp->spec.i_id = uap->m.geta.key;
		bufp->spec.i_flag = uap->m.geta.semflg;
		bufp->spec.i_cmd = 0;
		break;

	case SEMCTL:
		bufp->spec.i_id = uap->m.ctla.semid;
		bufp->spec.i_flag = 0;
		bufp->spec.i_cmd = uap->m.ctla.cmd;
		break;

	case SEMOP:
	default:
		bufp->spec.i_id = 0;
		bufp->spec.i_flag = 0;
		bufp->spec.i_cmd = 0;
		break;
	} 	/* end switch (uap->m.sysa.opcode) */
	adt_recwr(recp);
}


/*
 * void adt_stime(int status, timestruc_t *timep)
 * 	Called directly from from stime(), adjtime(),
 * 	and settimeofday().
 * Calling/Exit State:
 *	None.
 */
void
adt_stime(int status, timestruc_t *timep)
{
        struct arecbuf *recp;
        timerec_t *bufp;
	alwp_t *alwp;
	uint_t seqnum;

	if ((alwp = u.u_lwpp->l_auditp) != NULL)
        	recp = alwp->al_bufp;
	else {
		ALLOC_RECP(recp, sizeof(timerec_t), ADT_DATE);
        }
	bufp = (timerec_t *)recp->ar_bufp;
	bufp->cmn.c_rtype = TIME_R;
	bufp->cmn.c_event = ADT_DATE;
	bufp->cmn.c_size = sizeof(timerec_t);
	ADT_SEQNUM(seqnum);
	bufp->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = status;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum; 

	if (status == 0)
		bcopy(timep, &bufp->spec.t_time, sizeof(timestruc_t));
	else {
		bufp->spec.t_time.tv_sec = -1;
		bufp->spec.t_time.tv_nsec = -1;
	}
		
	recp->ar_inuse = sizeof(timerec_t);
	adt_recwr(recp);
	if (!alwp)
		kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);
}


struct utimea {
	char *fnamep;
	timestruc_t *tptr;
};
/*
 * STATIC void adt_utime(int status, struct utimea *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from utime().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_utime(int status, struct utimea *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	timerec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = TIME_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(timerec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	/* if time passed is null, time set to current time */
	if (uap->tptr) {
		if (copyin(&uap->tptr,
		    &(bufp->spec.t_time), sizeof(timestruc_t))) {
			bufp->spec.t_time.tv_sec = -1;
			bufp->spec.t_time.tv_nsec = -1;
		}
	} else {
		bufp->spec.t_time = bufp->cmn.c_time;
	}
	adt_recwr(recp);
}


struct setuida {
	int uid;
};
/*
 * STATIC void adt_setuid(int status, struct setuida *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from setuid().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_setuid(int status, struct setuida *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	setidrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = SETID_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(setidrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.s_nid = uap->uid;
	adt_recwr(recp);
}


struct setpgrpa {
	int	flag;
	int	pid;
	int	pgid;
};
/*
 * STATIC void adt_setpgrp(int status, struct setpgrpa *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from setsid(),
 *	setpgrp(), setpgid().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_setpgrp(int status, struct setpgrpa *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	setpgrprec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = SETPGRP_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(setpgrprec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.s_flag = uap->flag;

	if (uap->flag == 5) { /* setpgid(2) */
		bufp->spec.s_pid = uap->pid;
		bufp->spec.s_pgid = uap->pgid;
	}
	adt_recwr(recp);
}

struct setgroupsa {
	int ngroups;
	gid_t *grouplistp;
};
/*
 * STATIC void adt_setgroups(int status, struct setgroupsa *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from setgroups().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_setgroups(int status, struct setgroupsa *uap, rval_t *rvp)
{
	alwp_t		*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp;
	setgrouprec_t	*bufp;
	ulong_t		sz = 0;
	size_t		size;

	if (status == 0 && uap->ngroups > 0) {
		sz = uap->ngroups * (sizeof(gid_t));
		size = sizeof(setgrouprec_t) + sz;
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
 		ADT_BUFOFLOW(alwp, size);
	}
	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->cmn.c_rtype = SETGRPS_R;
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->cmn.c_size = recp->ar_inuse = sizeof(setgrouprec_t) + sz;
	bufp->spec.s_ngroups = uap->ngroups;
	if (sz)  
		if (copyin(uap->grouplistp, (caddr_t)bufp +
		    sizeof(setgrouprec_t), sz))
			bufp->cmn.c_size= recp->ar_inuse= sizeof(setgrouprec_t);
	adt_recwr(recp);
	ADT_BUFRESET(alwp);
}


struct setgida {
	int gid;
};
/*
 * STATIC void adt_setgid(int status, struct setgida *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from setgid(). 
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_setgid(int status, struct setgida *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	setidrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = SETID_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(setidrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.s_nid = uap->gid;
	adt_recwr(recp);
}


struct rlimita {
	int	resource;
	struct rlimit *rlp;
};
/*
 * STATIC void adt_rlimit(int status, struct rlimita *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from setrlimit().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_rlimit(int status, struct rlimita *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	rlimrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = RLIM_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(rlimrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.r_rsrc = uap->resource;

	if (copyin(uap->rlp, &bufp->spec.r_soft, sizeof(rlim_t)*2)) {
		bufp->spec.r_soft = 0;
		bufp->spec.r_hard = 0;
	}
	adt_recwr(recp);
}


struct ulimita {
	int	cmd;
	long	arg;
};
/*
 * STATIC void adt_ulimit(int status, struct ulimita *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from ulimit().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_ulimit(int status, struct ulimita *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	ulimitrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = ULIMIT_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(ulimitrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.u_cmd = uap->cmd;
	bufp->spec.u_arg = uap->arg;
	adt_recwr(recp);
}


struct ioctla {
	int fdes;
	int cmd;
	int arg;
};
/*
 * STATIC void adt_ioctl(int status, struct ioctla *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from ioctl().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_ioctl(int status, struct ioctla *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	ioctlrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = IOCTL_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(ioctlrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.i_cmd = uap->cmd;
	adt_recwr(recp);
}

struct fcntla {
	int fdes;
	int cmd;
	int arg;
};
/*
 * STATIC void adt_fcntl(int status, struct fcntla *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from fcntl().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_fcntl(int status, struct fcntla *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	struct flock 	bf;

	switch (uap->cmd) {
		case F_DUPFD:
		case F_SETFD:
		case F_SETFL:
		{
			fcntlrec_t	*bufp = recp->ar_bufp;
			struct fcntl_r	*specp = &(bufp->spec);

			bufp->cmn.c_rtype = FCNTL_R; 
			bufp->cmn.c_size = recp->ar_inuse = sizeof(fcntlrec_t);
			bufp->cmn.c_status = status;
			adt_cmn((cmnrec_t *)bufp);

			specp->f_cmd = uap->cmd;

			if (uap->cmd == F_DUPFD) {
				if (rvp->r_val1 > 0)
					specp->f_arg = rvp->r_val1;
				else
					specp->f_arg = -1;
			} else
				specp->f_arg = uap->arg;
			break;
		}
		
		case F_ALLOCSP:
		case F_FREESP:
		case F_SETLK:
		case F_SETLKW:
		case F_RSETLK:
		case F_RSETLKW:
		{
			fcntlkrec_t	*bufp = recp->ar_bufp;
			struct fcntlk_r *specp = &(bufp->spec);

			bufp->cmn.c_rtype = FCNTLK_R; 
			bufp->cmn.c_size = recp->ar_inuse = sizeof(fcntlkrec_t);
			bufp->cmn.c_status = status;
			adt_cmn((cmnrec_t *)bufp);

			specp->f_cmd = uap->cmd;

			if (!(copyin(&uap->arg, &bf, sizeof(struct o_flock)))) {
				specp->f_flock.l_type = bf.l_type;
				specp->f_flock.l_whence = bf.l_whence;
				specp->f_flock.l_start = bf.l_start;
				specp->f_flock.l_len = bf.l_len;
				specp->f_flock.l_sysid = bf.l_sysid;
				specp->f_flock.l_pid = bf.l_pid;
			} else 
				specp->f_flock.l_type = -1;
			break;
		}

		default:
		{
			fcntlrec_t	*bufp = recp->ar_bufp;
			struct fcntl_r *specp = &(bufp->spec);

			bufp->cmn.c_rtype = FCNTL_R; 
			bufp->cmn.c_size = recp->ar_inuse = sizeof(fcntlrec_t);
			bufp->cmn.c_status = status;
			adt_cmn((cmnrec_t *)bufp);

			specp->f_cmd = uap->cmd;
			break;
		}
	} 	/* end switch (uap->cmd) */
	adt_recwr(recp);
}

struct acla{
	char		*pathp;
	int  		cmd;
	int  		nentries;
	struct acl 	*aclbufp;
};
/*
 * STATIC void adt_acl(int status, struct acla *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from acl().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_acl(int status, struct acla *uap, rval_t *rvp)
{
	alwp_t		*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp;
	aclrec_t	*bufp;
	ulong_t		sz = 0;
	size_t		size;

	if (status == 0 && uap->nentries > 0) {
       		sz = (uap->nentries * sizeof(struct acl));
		size = sizeof(aclrec_t) + sz;
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
 		ADT_BUFOFLOW(alwp, size);
	}
	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->cmn.c_rtype = ACL_R; 
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->cmn.c_size = recp->ar_inuse = sizeof(aclrec_t) + sz;
	bufp->spec.a_nentries = uap->nentries;
	if (sz) {
		if (copyin(uap->aclbufp, (caddr_t)bufp + sizeof(aclrec_t), sz))
			bufp->cmn.c_size = recp->ar_inuse = sizeof(aclrec_t);
	}
	adt_recwr(recp);
	ADT_BUFRESET(alwp);
}


struct aclipca {
	int type;			/* ipc object type */
	int id;				/* ipc object id   */
	int cmd;			/* aclipc cmd      */
	int nentries;			/* # of ACL entries */
	struct acl *aclbufp;		/* acl buffer ptr  */
};
/*
 * STATIC void adt_aclipc(int status, struct aclipca *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from aclipc().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_aclipc(int status, struct aclipca *uap, rval_t *rvp)
{
	alwp_t		*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp;
	ipcaclrec_t	*bufp;
	ulong_t		sz = 0;
	size_t		size;

	if (status == 0 && uap->nentries > 0) {
       		sz = (uap->nentries * sizeof(struct acl));
		size = sizeof(ipcaclrec_t) + sz;
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
 		ADT_BUFOFLOW(alwp, size);
	}
	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->cmn.c_rtype = IPCACL_R; 
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.i_id = uap->id;
	bufp->spec.i_nentries = uap->nentries;
	bufp->spec.i_type = uap->type;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(ipcaclrec_t) + sz;
	if (sz) {
		if (copyin(uap->aclbufp,(caddr_t)bufp+sizeof(ipcaclrec_t),sz))
			bufp->cmn.c_size = recp->ar_inuse = sizeof(ipcaclrec_t);
	}
	adt_recwr(recp);
	ADT_BUFRESET(alwp);
}


struct filepriva {
	char	*fname;
	int	cmd;
	priv_t	*privp;
	int	count;
};
/*
 * STATIC void adt_fpriv(int status, struct filepriva *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from filepriv().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_fpriv(int status, struct filepriva *uap, rval_t *rvp)
{
	alwp_t		*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp;
	fprivrec_t	*bufp;
	ulong_t		sz = 0;
	size_t		size;

	if (status == 0 && uap->count > 0) {
       		sz = (uap->count * sizeof(priv_t));
		size = sizeof(fprivrec_t) + sz;
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
		ADT_BUFOFLOW(alwp, size); 
	}
	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->cmn.c_rtype = FPRIV_R; 
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->cmn.c_size = recp->ar_inuse = sizeof(fprivrec_t) + sz;
	bufp->spec.f_count = uap->count;
	if (sz) {
		if (copyin(uap->privp, (caddr_t)bufp + sizeof(fprivrec_t), sz)) 
			bufp->cmn.c_size = recp->ar_inuse = sizeof(fprivrec_t);
	}
	adt_recwr(recp);
	ADT_BUFRESET(alwp);
}


struct devstata {
	char *pathp;
	int cmd;
	struct devstat *devp;
};
/*
 * STATIC void adt_devstat(int status, struct devstata *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from devstat().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_devstat(int status, struct devstata *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	devrec_t	*bufp = recp->ar_bufp;
	struct devstat	*specp = &(bufp->spec.devstat);

	bufp->cmn.c_rtype = DEV_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(devrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	if (copyin(uap->devp, specp, sizeof(struct devstat))) {
		specp->dev_relflag = 0;
		specp->dev_mode = 0;
		specp->dev_hilevel = 0;
		specp->dev_lolevel = 0;
		specp->dev_state = 0;
		specp->dev_usecount = 0;
	}
	adt_recwr(recp);
}


struct lvlfilea {
	char *pathp;		/* used to generate filename record */
	int cmd;
	lid_t *lidp;
};
/*
 * STATIC void adt_lvlfile(int status, struct lvlfilea *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from lvlfile(), lvlvfs().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_lvlfile(int status, struct lvlfilea *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	macrec_t  *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = MAC_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(macrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	if (copyin(uap->lidp, &bufp->spec.l_lid, sizeof(lid_t)))
		bufp->spec.l_lid = 0;
	adt_recwr(recp);
}


struct lvlproca {
	int cmd;
	lid_t *lidp;
};
/*
 * STATIC void adt_lvlproc(int status, struct lvlproca *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from lvlproc().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_lvlproc(int status, struct lvlproca *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	macrec_t  *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = MAC_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(macrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	if (copyin(uap->lidp, &bufp->spec.l_lid, sizeof(lid_t)))
		bufp->spec.l_lid = 0;
	adt_recwr(recp);
}


struct memcntla {
	caddr_t addr;
	size_t	len;
	int	cmd;
	caddr_t	arg;
	int	attr;
	int	mask;
};
/*
 * STATIC void adt_memcntl(int status, struct memcntla *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from memcntl().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_memcntl(int status, struct memcntla *uap, rval_t *rvp)
{
	arecbuf_t *recp  = u.u_lwpp->l_auditp->al_bufp;
	mctlrec_t  *bufp = recp->ar_bufp;

        bufp->cmn.c_rtype = MCTL_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(mctlrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.m_attr = uap->attr;
	adt_recwr(recp);
}

		
struct plocka {
	int op;
};
/*
 * STATIC void adt_plock(int status, struct plocka *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from plock().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_plock(int status, struct plocka *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	plockrec_t	*bufp = recp->ar_bufp;

        bufp->cmn.c_rtype = PLOCK_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(plockrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.p_op = uap->op;
	adt_recwr(recp);
}


struct modulda {
        int mod_id;
};
/*
 * STATIC void adt_moduload(int status, struct modulda *uap, rval_t *rvp)
 *      Called from systrap_cleanup(), after return from moduload().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_moduload(int status, struct modulda *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	modloadrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = MODLOAD_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(modloadrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.m_id = uap->mod_id;
	adt_recwr(recp);
	return;
}


struct modpatha {
	char *pathp;
};
/*
 * STATIC void adt_modpath(int status, struct modpatha *uap, rval_t *rvp)
 *      Called from systrap_cleanup(), after return from modpath().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_modpath(int status, struct modpatha *uap, rval_t *rvp)
{
	alwp_t		*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp;
	modpathrec_t	*bufp;
	size_t		size;
	uint_t 		len = 0;

	if (status == 0 && uap->pathp) {
		len = ROUND2WORD(MAXPATHLEN + 1);
		size = sizeof(modpathrec_t) + len;
		if (!(CRED()->cr_flags & CR_RDUMP))
			size += sizeof(credrec_t) +
				(CRED()->cr_ngroups * sizeof(gid_t));
		ADT_BUFOFLOW(alwp, size);
	}
	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->cmn.c_rtype = MODPATH_R;
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

        if (len) {
		if (copyinstr(uap->pathp, (caddr_t)bufp + sizeof(modpathrec_t),
		    MAXPATHLEN, &len))
			len = 0;
	}
	bufp->cmn.c_size = recp->ar_inuse = sizeof(modpathrec_t) + len;
        adt_recwr(recp);

        ADT_BUFRESET(alwp);
	return;
}

struct modadma {
	unsigned int type;	/* module type */
	unsigned int cmd;	/* command */
	void	*arg;		/* module type specific data */
};
/*
 * STATIC void adt_modadm(int status, struct modadma *uap, rval_t *rvp)
 *      Called from systrap_cleanup(), after return from modadm().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_modadm(int status, struct modadma *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	modadmrec_t	*bufp = recp->ar_bufp;
	struct mod_mreg mreg;
	struct mod_mreg *mregp = &mreg;

        bufp->cmn.c_rtype = MODADM_R;
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.m_type = uap->type;
	bufp->spec.m_cmd = uap->cmd;

	/*
	 * Calculate the size of the free-format data
	 */
	switch (uap->type) {
	case MOD_TY_FS: 
		if (copyin(uap->arg, mregp, sizeof(struct mod_mreg))) {
			bufp->cmn.c_size = recp->ar_inuse = sizeof(modadmrec_t);
			break;
		}
		bcopy(mregp, (caddr_t)bufp+sizeof(modadmrec_t), MODMAXNAMELEN);
		if (copyin(mregp->md_typedata, (caddr_t)bufp+sizeof(modadmrec_t)
		    + MODMAXNAMELEN, FSTYPSZ)) 
			bufp->cmn.c_size = recp->ar_inuse =
				sizeof(modadmrec_t) + ROUND2WORD(MODMAXNAMELEN);
		else
			bufp->cmn.c_size = recp->ar_inuse = sizeof(modadmrec_t)
				+ ROUND2WORD(MODMAXNAMELEN + FSTYPSZ);
		break;
	case MOD_TY_STR:
        case MOD_TY_CDEV:
        case MOD_TY_BDEV:
		if (copyin(uap->arg, (caddr_t)mregp, sizeof(struct mod_mreg))) {
			bufp->cmn.c_size = recp->ar_inuse = sizeof(modadmrec_t);
			break;
		}
		bcopy((caddr_t)mregp, (caddr_t)bufp + sizeof(modadmrec_t),
			MODMAXNAMELEN);
		bufp->cmn.c_size = recp->ar_inuse = sizeof(modadmrec_t)
			+ ROUND2WORD(MODMAXNAMELEN);
		break;
        default:
		bufp->cmn.c_size = recp->ar_inuse = sizeof(modadmrec_t);
		break;
	}

	adt_recwr(recp);
	return;
}


/*
 * STATIC void adt_mount(int status, struct mounta *uap, rval_t *rvp)
 * 	Called from systrap_cleanup(), after return from mount().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_mount(int status, struct mounta *uap, rval_t *rvp)
{
	arecbuf_t *recp  = u.u_lwpp->l_auditp->al_bufp;
	mountrec_t *bufp = recp->ar_bufp;

        bufp->cmn.c_rtype = MOUNT_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(mountrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.m_flags = uap->flags;
	adt_recwr(recp);
}


struct online_args {
        int     processor;
        int     flag;
};
/*
 * STATIC void adt_online(int status, struct online_args *uap, rval_t *rvp)
 *      Called from systrap_cleanup(), after return from online().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_online(int status, struct online_args *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	onlinerec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = ONLINE_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(onlinerec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.p_procid = uap->processor;
	bufp->spec.p_cmd = uap->flag;
	adt_recwr(recp);
	return;
}


struct bind_args {
        idtype_t idtype;
        id_t    id;
        processorid_t processorid;
        processorid_t *obind;
};
/*
 * STATIC void adt_bind(int status, struct bind_args *uap, rval_t *rvp)
 *      Called from systrap_cleanup(), after return from processor_bind().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_bind(int status, struct bind_args *uap, rval_t *rvp)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
	bindrec_t *bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = BIND_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(bindrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.b_idtype = uap->idtype;
	bufp->spec.b_id = uap->id;
	bufp->spec.b_cpuid = uap->processorid;
	if (uap->obind) {
		if (copyin(uap->obind, &bufp->spec.b_obind,
			sizeof(processorid_t)))
				bufp->spec.b_obind = ADT_DONT_CARE;
		bufp->spec.b_obind = *(uap->obind);
	} else
		bufp->spec.b_obind = ADT_DONT_CARE;
	adt_recwr(recp);
	return;
}

struct keyctl_arg {
        int	cmd;
        void	*arg;
        int	nskeys;
};
/*
 * STATIC void adt_keyctl(int status, struct keyctl_arg *uap, rval_t *rvp)
 *      Called from systrap_cleanup(), after return from keyctl().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_keyctl(int status, struct keyctl_arg *uap, rval_t *rvp)
{
	alwp_t		*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp;
	keyctlrec_t	*bufp;
	ulong_t		sz = 0;
	size_t		size;

	if (status == 0 && uap->nskeys > 0) {
		sz = uap->nskeys * (sizeof(k_skey_t));
		size = sizeof(keyctlrec_t) + sz;
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
 		ADT_BUFOFLOW(alwp, size);
	}
	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->cmn.c_rtype = KEYCTL_R;
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->cmn.c_size = recp->ar_inuse = sizeof(keyctlrec_t) + sz;
	bufp->spec.k_cmd = uap->cmd;
	bufp->spec.k_nskeys = uap->nskeys;
	if (sz)  
		if (copyin(uap->arg, (caddr_t)bufp + sizeof(keyctlrec_t), sz))
			bufp->cmn.c_size = recp->ar_inuse = sizeof(keyctlrec_t);
	adt_recwr(recp);
	ADT_BUFRESET(alwp);
}



/*
 * Following are special cases recording functions called directly from 
 * the system call triggering the event.
 */

/*
 * void adt_auditbuf(int vhigh, int status)
 * 	Called directly from auditbuf().
 * 	Recording function for vhigh modification
 * 	Check if current process is auditable.
 * 
 * Calling/Exit State:
 *	No locks must be held on entry and none held on exit. 
 */
void
adt_auditbuf(int vhigh, int status)
{
        struct arecbuf *recp;
        abufsysrec_t *bufp;
	alwp_t *alwp;
	uint_t seqnum;

	if ((alwp = u.u_lwpp->l_auditp) != NULL)
        	recp = alwp->al_bufp;
	else {
		ALLOC_RECP(recp, sizeof(abufsysrec_t), ADT_AUDIT_BUF);
        }
	bufp = (abufsysrec_t *)recp->ar_bufp;
	bufp->cmn.c_rtype = ABUF_R;
	bufp->cmn.c_event = ADT_AUDIT_BUF;
	bufp->cmn.c_size = sizeof(abufsysrec_t);
	ADT_SEQNUM(seqnum);
	bufp->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = status;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum; 

	bufp->spec.a_vhigh = vhigh; 
	recp->ar_inuse = sizeof(abufsysrec_t);
	adt_recwr(recp);
	if (!alwp)
		kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);
}


/*
 * void adt_auditctl(int cmd, int status)
 *      Called directly from auditctl().
 * 	Recording function for enable/disable auditing.
 * 	Check if current process is auditable.
 * 
 * Calling/Exit State:
 *	No locks must be held on entry and none held on exit. 
 */
void
adt_auditctl(int cmd, int status)
{
        struct arecbuf *recp;
        actlsysrec_t *bufp;
	alwp_t *alwp;
	uint_t seqnum;

	if ((alwp = u.u_lwpp->l_auditp) != NULL)
        	recp = alwp->al_bufp;
	else {
		ALLOC_RECP(recp, sizeof(actlsysrec_t), ADT_AUDIT_CTL);
        }
	bufp = (actlsysrec_t *)recp->ar_bufp;
	bufp->cmn.c_rtype = ACTL_R;
	bufp->cmn.c_event = ADT_AUDIT_CTL;
	bufp->cmn.c_size = sizeof(actlsysrec_t);
	ADT_SEQNUM(seqnum);
	bufp->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = status;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum; 

	bufp->spec.a_cmd = cmd; 
	recp->ar_inuse = sizeof(actlsysrec_t);
	adt_recwr(recp);
	if (!alwp)
		kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);
}

/*
 * void adt_auditdmp(arec_t *admp, int status)
 *      Called directly from auditdmp().
 * 	Recording function for failed auditdmp() event.
 * 	Check if current process is auditable.
 * 
 * Calling/Exit State:
 *	No locks must be held on entry and none held on exit. 
 */
void
adt_auditdmp(arec_t *admp, int status)
{
        struct arecbuf *recp;
        admprec_t *bufp;
	alwp_t *alwp;
	uint_t seqnum;

	if ((alwp = u.u_lwpp->l_auditp) != NULL)
        	recp = alwp->al_bufp;
	else {
		ALLOC_RECP(recp, sizeof(admprec_t), ADT_AUDIT_DMP);
        }
	bufp = (admprec_t *)recp->ar_bufp;
	bufp->cmn.c_rtype = ADMP_R;
	bufp->cmn.c_event = ADT_AUDIT_DMP;
	bufp->cmn.c_size = sizeof(admprec_t);
	ADT_SEQNUM(seqnum);
	bufp->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = status;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum; 

	if (admp) {
		bufp->spec.a_rtype = admp->rtype; 
		bufp->spec.a_status = admp->rstatus; 
	} else {
		bufp->spec.a_rtype = -1; 
		bufp->spec.a_status = -1; 
	}
	recp->ar_inuse = sizeof(admprec_t);
	adt_recwr(recp);
	if (!alwp)
		kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);
}

/*
 * void adt_auditevt(aevt_t *aevtp, lid_t * lvltblp, int status)
 *      Called directly from auditevt().
 * 	Recording function for enable/disable auditing event criteria.
 * 	Check if current process is auditable.
 * 
 * Calling/Exit State:
 *	No locks must be held on entry and none held on exit. 
 */
void
adt_auditevt(int cmd, aevt_t *aevtp, lid_t *lvltblp, int status)
{
        struct arecbuf *recp;
        aevtsysrec_t *bufp;
	alwp_t *alwp;
	uint_t seqnum;
	int sz = 0;

	if (lvltblp)
		sz = (adt_nlvls * (sizeof(lid_t)));
	if ((alwp = u.u_lwpp->l_auditp) != NULL) {
		size_t size;
		size = sizeof(aevtsysrec_t) + sz;
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
 		ADT_BUFOFLOW(alwp, size);
        	recp = alwp->al_bufp;
	} else {
		ALLOC_RECP(recp, sizeof(aevtsysrec_t) + sz, ADT_AUDIT_EVT);
        }
	bufp = (aevtsysrec_t *)recp->ar_bufp;
	bufp->cmn.c_rtype = AEVT_R;
	bufp->cmn.c_event = ADT_AUDIT_EVT;
	ADT_SEQNUM(seqnum);
	bufp->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = status;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum; 

	bufp->spec.a_cmd = cmd; 
	bufp->spec.a_nlvls = 0; 
	bufp->cmn.c_size = recp->ar_inuse = sizeof(aevtsysrec_t);
	switch (cmd) {
	case ANAUDIT:
	case AYAUDIT:
		break;

	case ASETSYS:
	case ASETME:
		if (aevtp)
			bcopy(aevtp->emask, bufp->spec.a_emask, sizeof(adtemask_t)); 
		else
			bzero(bufp->spec.a_emask, sizeof(adtemask_t)); 
		break;

	case ASETUSR:
		if (aevtp) {
			bcopy(aevtp->emask, bufp->spec.a_emask, sizeof(adtemask_t)); 
			bufp->spec.a_uid = aevtp->uid; 
		} else {
			bzero(bufp->spec.a_emask, sizeof(adtemask_t)); 
			bufp->spec.a_uid = -1; 
		}
		break;

	case ASETLVL:
		if (aevtp) {
			bufp->spec.a_flags = aevtp->flags; 
			bufp->spec.a_nlvls = adt_nlvls; 
			if (aevtp->flags & ADT_OMASK)
				bcopy(aevtp->emask, bufp->spec.a_emask,
					sizeof(adtemask_t)); 
			if (aevtp->flags & ADT_RMASK) {
				if (copyin(aevtp->lvl_minp,
				    &bufp->spec.a_lvlmin, sizeof(lid_t)))
				    	bufp->spec.a_lvlmin = 0;
				if (copyin(aevtp->lvl_maxp,
				    &bufp->spec.a_lvlmax, sizeof(lid_t)))
				    	bufp->spec.a_lvlmax = 0;
			}
			if (aevtp->flags & ADT_LMASK) {
				if (lvltblp) {
					if (copyin(lvltblp, (caddr_t)bufp +
					    sizeof(aevtsysrec_t), sz))
                        			bufp->cmn.c_size += sz;
						recp->ar_inuse += sz;
				}
			}

		} else {
			bzero(bufp->spec.a_emask, sizeof(adtemask_t)); 
			bufp->spec.a_uid = -1; 
			bufp->spec.a_flags = 0; 
			bufp->spec.a_lvlmin = NULL; 
			bufp->spec.a_lvlmax = NULL; 
		}
		break;

	default:
		break;

	}

	adt_recwr(recp);
	if (alwp)
		ADT_BUFRESET(alwp);
	else
		kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);
}

/*
 * void adt_auditlog(alog_t alogp, int status)
 *      Called directly from auditlog().
 * 	Recording function for audit log file manipulation.
 * 	Check if current process is auditable.
 * 
 * Calling/Exit State:
 *	No locks must be held on entry and none held on exit. 
 */
void
adt_auditlog(alog_t *alogp, int status)
{
        struct arecbuf *recp;
        alogsysrec_t *bufp;
	alwp_t *alwp;
	uint_t seqnum;

	if ((alwp = u.u_lwpp->l_auditp) != NULL) {
		size_t size;
		size = sizeof(alogsysrec_t);
		if (!(CRED()->cr_flags & CR_RDUMP)) 
			size += sizeof(credrec_t) + 
				(CRED()->cr_ngroups * sizeof(gid_t));
 		ADT_BUFOFLOW(alwp, size);
        	recp = alwp->al_bufp;
	} else {
		ALLOC_RECP(recp, sizeof(alogsysrec_t), ADT_AUDIT_LOG);
        }
	bufp = (alogsysrec_t *)recp->ar_bufp;
	bufp->cmn.c_rtype = ALOG_R;
	bufp->cmn.c_event = ADT_AUDIT_LOG;
	bufp->cmn.c_size = sizeof(alogsysrec_t);
	ADT_SEQNUM(seqnum);
	bufp->cmn.c_seqnum = ADT_SEQRECNUM(seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = status;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum; 

	if (alogp) {
		bufp->spec.a_flags = alogp->flags; 
		bufp->spec.a_onfull = alogp->onfull; 
		bufp->spec.a_onerr = alogp->onerr; 
		bufp->spec.a_maxsize = alogp->maxsize; 
		if (alogp->flags & PNODE)
			bcopy(alogp->pnodep, bufp->spec.a_pnodep, ADT_NODESZ); 
		else
			bufp->spec.a_pnodep[0] = NULL;
	
		if (alogp->flags & ANODE)
			bcopy(alogp->anodep, bufp->spec.a_anodep, ADT_NODESZ); 
		else
			bufp->spec.a_anodep[0] = NULL;

		if (alogp->flags & PPATH) {
			if (copyin(alogp->ppathp, bufp->spec.a_ppathp,
			    ADT_MAXPATHLEN))
				bufp->spec.a_ppathp[0] = NULL;
		} else
			bufp->spec.a_ppathp[0] = NULL; 

		if (alogp->flags & APATH) {
			if (copyin(alogp->apathp, bufp->spec.a_apathp,
			    ADT_MAXPATHLEN))
				bufp->spec.a_apathp[0] = NULL;
		} else
			bufp->spec.a_apathp[0] = NULL;

		if (alogp->flags & APROG) {
			if (copyin(alogp->progp, bufp->spec.a_progp, MAXPATHLEN))
				bufp->spec.a_progp[0] = NULL;
		} else
			bufp->spec.a_progp[0] = NULL;
	} else {
		bufp->spec.a_flags = 0; 
		bufp->spec.a_onfull = 0; 
		bufp->spec.a_onerr = 0; 
		bufp->spec.a_maxsize = 0; 
		bufp->spec.a_pnodep[0] = NULL;
		bufp->spec.a_anodep[0] = NULL;
		bufp->spec.a_ppathp[0] = NULL; 
		bufp->spec.a_apathp[0] = NULL;
		bufp->spec.a_progp[0] = NULL;
	}

	recp->ar_inuse = sizeof(alogsysrec_t);
	adt_recwr(recp);
	if (alwp)
		ADT_BUFRESET(alwp);
	else
		kmem_free(recp, sizeof(arecbuf_t) + recp->ar_size);
}

/*
 * void adt_exit(int status)
 * 	Recording function for when a process exits.
 * 	Check if current process is auditable.
 * 	Special case: called directly from exit().
 * Calling/Exit State:
 *	None.
 */
void
adt_exit(int status)
{
	lwp_t           *lwpp = u.u_lwpp;
        alwp_t		*alwp = lwpp->l_auditp;
        aproc_t		*ap = u.u_procp->p_auditp;
	arecbuf_t	*recp;
	cmnrec_t	*bufp;

	/* Its OK to check process audit event mask here */
	if (alwp && EVENTCHK(ADT_EXIT, ap->a_emask->ad_emask)) {
		recp = alwp->al_bufp;
		bufp = recp->ar_bufp;
		bufp->c_rtype = CMN_R;
		bufp->c_event = ADT_EXIT;
		bufp->c_size = recp->ar_inuse = SIZ_CMNREC;
		ADT_SEQNUM(bufp->c_seqnum); 
		bufp->c_pid = u.u_procp->p_pidp->pid_id;
		bufp->c_time = hrestime;
		bufp->c_status = status;
		bufp->c_sid = u.u_procp->p_sid;
		bufp->c_lwpid = lwpp->l_lwpid;
		bufp->c_crseqnum = CRED()->cr_seqnum;
		adt_recwr(recp);
	}
	if (alwp) {
		u.u_lwpp->l_auditp = NULL;
		adt_free(alwp);
	}
	adt_freeaproc(u.u_lwpp->l_procp);
	return;
}


struct lwp_createa {
        ucontext_t *ucp;
        u_long     flags;
};
/*
 * STATIC void adt_lwp_create(int status, struct lwp_createa *uap, rval_t *rvp)
 *      Called from systrap_cleanup(), after return from _lwp_create().
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
adt_lwp_create(int status, struct lwp_createa *uap, rval_t *rvp)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	lwpcreatrec_t	*bufp = recp->ar_bufp;

	bufp->cmn.c_rtype = LWPCREATE_R;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(lwpcreatrec_t);
	bufp->cmn.c_status = status;
	adt_cmn((cmnrec_t *)bufp);

	bufp->spec.l_newid = rvp->r_val1;
	adt_recwr(recp);
	return;
}


/*
 * void adt_lwp_exit(void)
 * 	Recording function for when an lwp exits.
 * 	Check if current process is auditable.
 * 	Special case: called directly from _lwp_exit().
 * Calling/Exit State:
 *	None.
 */
void
adt_lwp_exit(void)
{
	lwp_t           *lwpp = u.u_lwpp;
        alwp_t		*alwp = lwpp->l_auditp;
	arecbuf_t	*recp;
	cmnrec_t	*bufp;

	ASSERT(alwp != NULL);

	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->c_rtype = CMN_R;
	bufp->c_event = ADT_LWP_EXIT;
	bufp->c_size = recp->ar_inuse = SIZ_CMNREC;
	ADT_SEQNUM(bufp->c_seqnum); 
	bufp->c_pid = u.u_procp->p_pidp->pid_id;
	bufp->c_time = hrestime;
	bufp->c_status = 0;
	bufp->c_sid = u.u_procp->p_sid;
	bufp->c_lwpid = lwpp->l_lwpid;
	bufp->c_crseqnum = CRED()->cr_seqnum;
	adt_recwr(recp);

	return;
}

/*
 * void adt_modload(int status, struct modctl *mcp)
 *      Called directly from modload().
 * Calling/Exit State:
 *	None.
 */
void
adt_modload(int status, struct modctl *mcp)
{
	alwp_t		*alwp = u.u_lwpp->l_auditp;
	arecbuf_t	*recp;
	modloadrec_t	*bufp;
	struct module	*modp;
	struct vnode	*vp;

	if (!(alwp->al_flags & AUDITME) &&
	    EVENTCHK(ADT_MODLOAD, alwp->al_emask->ad_emask)) {
		alwp->al_event = ADT_MODLOAD;
		SET_AUDITME(alwp);
		alwp->al_flags &= ~ADT_NEEDPATH;
		if (status == 0) {
			modp = mcp->mc_modp;
			/* Need to generate a Filename record */
			if (lookupname(modp->mod_obj.md_path, UIO_SYSSPACE,
			    NO_FOLLOW, NULLVPP, &vp) == 0)
				VN_RELE(vp);
		}
	}
	
	if (alwp->al_flags & AUDITME) {
		recp = alwp->al_bufp;
		bufp = recp->ar_bufp;
		bufp->cmn.c_rtype = MODLOAD_R;
		bufp->cmn.c_status = status;
		adt_cmn((cmnrec_t *)bufp);
		
		bufp->spec.m_id = (status ? 0 : mcp->mc_id);
		bufp->cmn.c_size = recp->ar_inuse = sizeof(modloadrec_t);
		adt_recwr(recp);
	}	
	return;
}

/*
 * void adt_recvfd(struct adtrecvfd *recvfdp, struct vnode *vp, vattr_t *vap)
 * 	Recording function for when a process receives an fd via streams.
 * 	Check if current process is auditable.
 * 	Special case: called directly from strioctl().
 * Calling/Exit State:
 *	None.
 */
void
adt_recvfd(struct adtrecvfd *recvfdp, struct vnode *vp, vattr_t *vap)
{
	arecbuf_t	*recp = u.u_lwpp->l_auditp->al_bufp;
	recvfdrec_t	*bufp = recp->ar_bufp;
	struct vnode_r	*fdp = &(bufp->spec.r_fdinfo); 

	bufp->cmn.c_rtype = RECVFD_R;
	bufp->cmn.c_event = ADT_RECVFD;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(recvfdrec_t);
	ADT_SEQNUM(bufp->cmn.c_seqnum);
	bufp->cmn.c_pid = u.u_procp->p_pidp->pid_id;
	bufp->cmn.c_time = hrestime;
	bufp->cmn.c_status = 0;
	bufp->cmn.c_sid = u.u_procp->p_sid;
	bufp->cmn.c_lwpid = u.u_lwpp->l_lwpid;
	bufp->cmn.c_crseqnum = CRED()->cr_seqnum;

	fdp->v_type = vp->v_type;
	fdp->v_lid = vp->v_lid;

	/* Get the file attributes for the object. */
	if (vap) {
		fdp->v_fsid = vap->va_fsid;
		if (vp->v_type == VBLK || vp->v_type == VCHR)
			fdp->v_dev = vap->va_rdev;
		else
			fdp->v_dev = vp->v_vfsp->vfs_dev;
		fdp->v_inum = vap->va_nodeid;
	} else {
		vattr_t	vattr;
		vattr.va_mask = AT_STAT;
		if (VOP_GETATTR(vp, &vattr, 0, CRED())) {
			fdp->v_fsid = 0;
			fdp->v_dev = 0;
			fdp->v_inum = 0;
		} else {
			fdp->v_fsid = vattr.va_fsid;
			if (vp->v_type == VBLK || vp->v_type == VCHR)
	                      	fdp->v_dev = vattr.va_rdev;
			else
				fdp->v_dev = vp->v_vfsp->vfs_dev;
			fdp->v_inum = vattr.va_nodeid;
		} 
	}

	bufp->spec.r_spid = recvfdp->adtrfd_sendpid;
	bufp->spec.r_slwpid = recvfdp->adtrfd_sendlwpid;
	adt_recwr(recp);
	return;
}


/*
 * void adt_logoff(void)
 * 	Recording function for when a process logs off and
 *	the controlling tty is freed.
 * 	Special case: called directly from exit().
 * Calling/Exit State:
 *	None.
 */
void
adt_logoff(void)
{
	lwp_t           *lwpp = u.u_lwpp;
        alwp_t		*alwp = lwpp->l_auditp;
	arecbuf_t	*recp;
	cmnrec_t	*bufp;

	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->c_rtype = CMN_R;
	bufp->c_event = ADT_LOGOFF;
	bufp->c_size = recp->ar_inuse = SIZ_CMNREC;
	ADT_SEQNUM(bufp->c_seqnum); 
	bufp->c_pid = u.u_procp->p_pidp->pid_id;
	bufp->c_time = hrestime;
	bufp->c_status = 0;
	bufp->c_sid = u.u_procp->p_sid;
	bufp->c_lwpid = lwpp->l_lwpid;
	bufp->c_crseqnum = CRED()->cr_seqnum;
	adt_recwr(recp);
	return;
}


#ifdef CC_PARTIAL
/*
 * int adt_cc(long subevent, long bps)
 * 	Recording function for auditing covert channels.
 * 	Special case: called directly from covert channel event.
 * Calling/Exit State:
 *	None.
 */
int
adt_cc(long subevent, long bps)
{
	alwp_t    *alwp = u.u_lwpp->l_auditp;
	arecbuf_t *recp = alwp->al_bufp;
	ccrec_t   *bufp = recp->ar_bufp;
	cmnrec_t  *cmnp = recp->ar_bufp;
	ushort_t  event;

	/*
	 * adt_cc() may be called after the audit
	 * is turned off. Return 1 so that
	 * cc_limiter() can take appropriate action.
	 */

	event = bps ? ADT_COV_CHAN_1 : ADT_COV_CHAN_2;

	if (EVENTCHK(event, alwp->al_emask->ad_emask)) {
		SET_AUDITME(alwp);
		cmnp->c_event = event;
		cmnp->c_seqnum = CMN_SEQNM(alwp);
		cmnp->c_pid = u.u_procp->p_pidp->pid_id;
		cmnp->c_time = alwp->al_time;
		cmnp->c_sid = u.u_procp->p_sid;
		cmnp->c_lwpid = u.u_lwpp->l_lwpid;
		cmnp->c_crseqnum = CRED()->cr_seqnum;
		cmnp->c_rtype = CC_R;
		cmnp->c_size = recp->ar_inuse = sizeof(ccrec_t);
		cmnp->c_status = 0;

		bufp->spec.cc_event = subevent;
		bufp->spec.cc_bps = bps;

		return(adt_recwr(recp));
	
	}
	return(0);
}
#endif /* CC_PARTIAL */

/*
 * int adt_admin(int event, int status, int nentries, void *tbl) 
 * 	Recording function for administrative use of any of the
 * 	supported schedulers. Check if current process is audited.
 * Calling/Exit State:
 *	None.
 */
void
adt_admin(ushort_t event, int status, int nentries, void *tbl)
{
	alwp_t *alwp = u.u_lwpp->l_auditp;
	arecbuf_t *recp;
	adminrec_t *bufp;
	int sz, crsz;

	if (!(CRED()->cr_flags & CR_RDUMP))
                    crsz = sizeof(credrec_t)+(CRED()->cr_ngroups*sizeof(gid_t));

	switch(event) {
	case ADT_SCHED_TS:
		sz = nentries * sizeof(tsdpent_t);
		ADT_BUFOFLOW(alwp, (sizeof(adminrec_t) + sz + crsz)); 
		break;

	case ADT_SCHED_FP:
		sz = nentries * sizeof(rtdpent_t);
		ADT_BUFOFLOW(alwp, sizeof(adminrec_t) + sz + crsz); 
		break;

	case ADT_SCHED_FC:
		sz = nentries * sizeof(fcdpent_t);
		ADT_BUFOFLOW(alwp, sizeof(adminrec_t) + sz + crsz); 
		break;

	default:
		/*
		 *+ This case should never be called,
		 *+ if it is then one of the supported scheduling
		 *+ classes is passing garbage.
		 */ 
		cmn_err(CE_WARN, "unknown event %d\n", event);
	}

	recp = alwp->al_bufp;
	bufp = recp->ar_bufp;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(adminrec_t) + sz;
	bcopy(tbl, (caddr_t)bufp + sizeof(adminrec_t), sz);
	bufp->cmn.c_rtype = ADMIN_R;
	bufp->cmn.c_status = status;
	bufp->spec.nentries = nentries;
	adt_cmn((cmnrec_t *)bufp);

	bufp->cmn.c_event = event;
	adt_recwr(recp);
	ADT_BUFRESET(alwp);
}


/*
 * void adt_parmsset(int event, int status, long upri, long uprisecs)
 * 	Recording function for the use of either the supported schedulers.
 * 	Check if current process is audited.
 * Calling/Exit State:
 *	None.
 */
void
adt_parmsset(ushort_t event, int status, long upri, long uprisecs)
{
	arecbuf_t *recp = u.u_lwpp->l_auditp->al_bufp;
        parmsrec_t *bufp = recp->ar_bufp;

        bufp->cmn.c_rtype = PARMS_R;
	bufp->cmn.c_status = status;
        adt_cmn((cmnrec_t *)bufp);

	bufp->cmn.c_event = event;
	bufp->cmn.c_size = recp->ar_inuse = sizeof(parmsrec_t) + 
			(bufp->spec.nentries * sizeof(long));
	bufp->spec.p_upri = upri;
	bufp->spec.p_uprisecs = uprisecs;

        adt_recwr(recp);
	ADT_BUFRESET(u.u_lwpp->l_auditp);
}

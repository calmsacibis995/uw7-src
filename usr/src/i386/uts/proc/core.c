#ident	"@(#)kern-i386:proc/core.c	1.17.3.1"
#ident	"$Header$"

#include <fs/file.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/seg_kvn.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/fp.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/types.h>

extern struct execsw_info mod_execsw_info;
extern rwlock_t	mod_execsw_lock;

/*
 * int core(pid_t pid, struct cred *credp, rlim_t rlimit, int sig,
 *	    boolean_t destroyed)
 *	Generates a core file using the name "core.pid".
 *
 * Calling/Exit State:
 *	No locks should be held on entry and none will be held on return. If 
 *	"destroyed" is B_TRUE, then the caller is expected to have called
 *	post_destroy() prior to invoking core(). This would be the typical
 *	scenario when handling a defaulted signal that dumps core. However, 
 *	if the caller wishes to dump core for other reasons, the "destroyed"
 *	flag must be set to B_FALSE.
 */
int
core(pid_t pid, struct cred *credp, rlim_t rlimit, int sig, boolean_t destroyed)
{
	struct vnode 	*vp;
	struct vattr 	vattr;
	int 		error; 
	int		closerr;
	proc_t		*p = u.u_procp;
	lwp_t		*lwpp = u.u_lwpp;
	cred_t		*crp = lwpp->l_cred;
	struct module	*modp;
	struct execsw_info	*esi, *oesi;
	char		cfname[32];

	/*
	 * If the caller has not called post_destroy(), do the right thing
	 * now!
	 */
	if (!destroyed) {
		(void)LOCK(&p->p_mutex, PLHI);
		if ((lwpp->l_trapevf & EVF_PL_DESTROY) == 0) {
			/*
			 * Won a possible race with another LWP wanting to
			 * set the EVF_PL_DESTROY flag. Destroy all other
			 * LWPs in the process, but post the state to
			 * rendezvous first to generate the core file below.
			 */
			post_destroy(B_TRUE, EVF_PL_RENDEZV);
			UNLOCK(&p->p_mutex, PLBASE);
		} else {
			/*
			 * Somebody beat us to the punch, return failure. 
			 */
			UNLOCK(&p->p_mutex, PLBASE);
			return 1;
		}
	}

	ASSERT(p->p_flag & P_DESTROY);

	/*
	 * Force all LWPs to rendezvous for us to dump core. The call to 
	 * rendezvous() should not fail. This is because, by now post_destroy()
	 * must have been called to post the EVF_PL_DESTROY flag as well as
	 * as the EVF_PL_RENDEZ flag to all the lwps (except the core 
	 * dumping context). Further, racing calls to post_destroy() are 
	 * prevented by examining P_DESTROY flag. 
	 * 
	 * The checks incorporated into rendezvous() for the EVF_PL_DESTROY 
	 * trap event flag (preventing any other LWPs from performing 
	 * a destroy or a rendezvous operation on the process), guarantee 
	 * that this call to rendezvous() will be successful.
	 */
	if (!rendezvous()) {
		/*
		 *+ Call to rendezvous() from core() failed! User cannot take
		 *+ any corrective action.
		 */
		cmn_err(CE_PANIC, "rendezvous() failed from core()\n");
	}

	/*
	 * All of the LWPs in the process have rendezvoused;
	 * now decide if we really are going to dump core.
	 * (We have to complete and release the rendezvous even if we are
	 * going to bail out without dumping core, because otherwise we
	 * might leave other LWPs hanging in psig().)
	 */
	if (crp->cr_uid != crp->cr_ruid || !hasprocperm(crp, credp)) {
		error = EPERM;
		goto done2;	/* release rendezvous and return error */
	}

	/*
	 * POSIX says that an rlimit of 0 should prevent the creation
	 * of a core file, rather than create an empty one.
	 */
	if (rlimit == 0) {
	        error = EFBIG;
		goto done2;	/* release rendezvous and return error */
	}

	/*
	 * Generate filename for the core file (core.<pid>)
	 */
	bcopy("core.", cfname, 5);
	numtos(pid, &cfname[5]);
	 
	/*
	 * Acquire the process reader/write lock in the "read" mode - this
	 * will prevent all write operations on the process (via /proc).
	 */
	RWSLEEP_RDLOCK(&p->p_rdwrlock, PRIMED);

	/* Now we can safely dump core. */ 
	error = vn_open(cfname, UIO_SYSSPACE, FWRITE | FTRUNC | FCREAT | FSYNC,
		(0666 & ~p->p_cmask) & PERMMASK, &vp, CRCORE);
	if (error) 
		goto done;

	if (VOP_ACCESS(vp, VWRITE, 0, credp) || vp->v_type != VREG) 
		error = EACCES;
	else {
		DISABLE_PRMPT();
		if (using_fpu) {
			/*
			 * The current LWP is using the FPU. Save the FPU state.
			 */
			save_fpu();
		}
		ENABLE_PRMPT();
		vattr.va_size = 0;
		vattr.va_mask = AT_SIZE;
		(void) VOP_SETATTR(vp, &vattr, 0, 0, credp);

		if (p->p_execinfo != NULL) {
			(void)RW_RDLOCK(&mod_execsw_lock, PLDLM);

			esi = p->p_execinfo->ei_execsw->exec_info;
			oesi = esi;
			modp = esi->esi_modp;

			if (modp != NULL) {
				boolean_t unloading = MOD_IS_UNLOADING(modp);

				if (unloading) {
					RW_UNLOCK(&mod_execsw_lock, PLBASE);
					esi = &mod_execsw_info;
				} else {
					RW_UNLOCK(&mod_execsw_lock, PLDLM);
					MOD_HOLD_L(modp, PLBASE);
				}
			} else {
				RW_UNLOCK(&mod_execsw_lock, PLBASE);
			}

			error = (*esi->esi_core)
				    (vp, p, credp, rlimit, sig);

			/*
			 * The above call can change esi_modp.
			 */
			modp = oesi->esi_modp;

			if (modp && error != ENOLOAD) {
				MOD_RELE(modp);
			}
		}
	}

	closerr = VOP_CLOSE(vp, FWRITE, 1, 0, credp);
	VN_RELE(vp);

done:
	RWSLEEP_UNLOCK(&p->p_rdwrlock);
done2:
	release_rendezvous();
	return (error ? error : closerr);
}

/*
 * int core_seg(proc_t *p, vnode_t *vp, off64_t offset, vaddr_t addr, 
 *	       size_t size, rlim_t rlimit, struct cred *credp);
 *	Common code to dump process address space.
 *
 * Calling/Exit State: 
 *	No locks to be held entry and none will be held on return.
 *
 * Remarks:
 *	dumps all the the pages in the range [addr, addr+size] that are
 * 	backed by real memory to the file pointed to by vp, at the 
 *	specified offset. It is assumed that the process is single 
 *	threaded. More specifically, it is assumed that the structure of the
 *	address space does not change under us.
 */
int
core_seg(proc_t *p, vnode_t *vp, off64_t offset, vaddr_t addr, size_t size, 
	 rlim_t rlimit, struct cred *credp)
{
	vaddr_t 	eaddr;
	struct as	*asp = p->p_as;
	vaddr_t		base;
	u_int 		len;
	int 		err = 0;

	eaddr = addr + size;
	for (base = addr; base < eaddr; base += len) {
		len = eaddr - base;
		if (as_memory(asp, &base, &len) != 0)
			return 0;
		err = vn_rdwr(UIO_WRITE, vp, (caddr_t)base, (int)len,
			      offset + (base - addr), UIO_USERSPACE, 0,
			      (long)rlimit, credp, (int *)NULL);
		if (err)
			return err;
	}
	return 0;
}

#ident	"@(#)kern-i386:proc/grow.c	1.19"
#ident	"$Header$"

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <proc/resource.h>
#include <proc/exec.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <proc/proc.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <proc/mman.h>
#include <svc/reg.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/lock.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <acc/priv/privilege.h>

struct brka {
	caddr_t	nva;
};

/*
 * int
 * brk(struct brka *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *	All arguments passed to brk() are local to the calling LWP and require
 *	no special protection. Since the call effects the composition of
 *	the entire address space (which may be multithreaded) we need to
 *	insure that the brk() is done atomically with respect to other similar 
 *	requests. The AS read/write sleeplock is used for this purpose. The
 *	lock is also used to protect the p_brkbase, p_brksize, p_stkbase, and
 *	p_stksize fields of the proc structure.
 *
 *	On success, a 0 is returned to the caller and p_brksize is 
 *	appropriately updated. On failure, an non-zero errno is passed
 *	back to indicate the failure mode.
 */
/* ARGSUSED */
int
brk(struct brka *uap, rval_t *rvp)
{
	struct proc *p;
	vaddr_t nva;	/* new break address */
	vaddr_t ova;	/* old break address */
	size_t nsize;	/* new break size */

	p = u.u_procp;

	ASSERT(p->p_as != NULL);

	as_wrlock(p->p_as);

	nva = (vaddr_t)uap->nva;

        if (nva < p->p_brkbase
	 || ((nsize = nva - p->p_brkbase) > p->p_brksize
          && nsize > u.u_rlimits->rl_limits[RLIMIT_DATA].rlim_cur)) {
		as_unlock(p->p_as);
		return ENOMEM;
	}

	nva = (vaddr_t)roundup((u_int)nva, PAGESIZE);
	ova = (vaddr_t)roundup((u_int)(p->p_brkbase+p->p_brksize), PAGESIZE);

	if (nva > ova) {

		int error;
		int lckflag = 0;

		if (p->p_plock & (DATLOCK | PROCLOCK)) {
			if (p->p_as->a_paglck == 0) {
				 p->p_as->a_paglck = 1;
				 lckflag = 1;
			}
		}

		/*
		 * Add new zfod mapping to extend UNIX data segment
		 */

		error = as_map(p->p_as, ova, (u_int)(nva - ova), 
		 	       segvn_create, zfod_argsp); 

		if (lckflag)
			p->p_as->a_paglck = 0;

		if (error) {
			as_unlock(p->p_as);
			return error;
		}

	} else if (ova > nva) {
		(void) as_unmap(p->p_as, nva, (u_int)(ova - nva));
	}

	p->p_brksize = nsize;
	as_unlock(p->p_as);
	return 0;
}

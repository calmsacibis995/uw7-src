/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)kern-i386:mem/vm_mapfile.c	1.4.4.1"
#ident	"$Header$"

/*
 * vm_mapfile.c
 *
 *	i86-specific VM support routines, accessed via sysi86().
 *
 */

#include <fs/file.h>
#include <fs/pathname.h>
#include <fs/vnode.h>
#include <fs/xnamfs/xnamnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/sd.h>
#include <svc/sysi86.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

STATIC int maptfile(struct vnode *, struct mmf *, long , vaddr_t *);
STATIC int mapdfile(struct vnode *, struct mmf *, vaddr_t *);


/*
 * si86_chmfile(caddr_t ap, rval_t *rvp)
 *	For SI86PCHRGN:
 *	Write to a memory mapped file region.
 *	The term "region" is from SVR3.2; in SVR4, "region"
 *	generally means "segment".
 *
 * Calling/Exit State:
 *	No special requirement.  If the process is multi-threaded,
 *	the request is illegal and is rejected.
 */
si86_chmfile(caddr_t ap, rval_t *rvp)
{
	struct cmf cmf;
	struct segvn_data *svd;
	struct seg *seg;
	struct as *as = u.u_procp->p_as;
	vaddr_t naddr, saddr, eaddr;
	u_int prot;
	int error = 0;

	if (!SINGLE_THREADED())
		return EBUSY;

	if (copyin(ap, &cmf, sizeof cmf) < 0)
		return EFAULT;
	if ((rvp->r_val1 = cmf.cf_count) == 0)
		return 0;

	as_rdlock(as);

	seg = as_segat(as, (vaddr_t)cmf.cf_dstva);

	if (seg == NULL ||
	    seg != as_segat(as, (vaddr_t)cmf.cf_dstva + cmf.cf_count)) {
		as_unlock(as);
		return EFAULT;
	}

	svd = (struct segvn_data *) seg->s_data;
	if (svd->svd_type == MAP_SHARED) {
		as_unlock(as);
		return EFAULT;
	}

	saddr = (vaddr_t)cmf.cf_dstva;
	eaddr = (vaddr_t)cmf.cf_dstva + cmf.cf_count;
	do {
		prot = as_getprot(seg->s_as, saddr, &naddr);
		if (prot&PROT_WRITE || !(prot&PROT_EXEC)) {
			as_unlock(as);
			return EFAULT;
		}
		saddr = naddr;
	} while (naddr < eaddr);

	if (as_setprot(as, (vaddr_t)cmf.cf_dstva, cmf.cf_count, PROT_ALL))
		error = EFAULT;
	else {
		if (copyin(cmf.cf_srcva, cmf.cf_dstva, cmf.cf_count) < 0)
			error = EFAULT;
		if (as_setprot(as, (vaddr_t)cmf.cf_dstva, cmf.cf_count,
			       PROT_ALL & ~PROT_WRITE))
			error = EFAULT;
	}

	as_unlock(as);

	return error;
}


/*
 * int si86_mapfile(caddr_t ap, rval_t *rvp)
 *	For SI86SHFIL:
 *	Map a file into user address space.  If mmf.mf_flags & MAP_PRIVATE
 *	is set, the file is mapped into a copy-on-write segment.  Otherwise,
 *	it is mapped into a read-only shareable segment.
 *
 * Calling/Exit State:
 *	No special requirement.  If the process is multi-threaded,
 *	the request is illegal and is rejected.
 */
int si86_mapfile(caddr_t ap, rval_t *rvp)
{
	struct mmf mmf;
	vnode_t	*vp;
	struct vattr vattr;
	struct pathname	pn;
	int error = 0;
	vaddr_t addr;
	long filesz;

	if (!SINGLE_THREADED())
		return EBUSY;

	if (copyin(ap, &mmf, sizeof mmf) < 0)
		return EFAULT;

	/*
	 * Lookup path name.
	 */
	if (error = pn_get(mmf.mf_filename, UIO_USERSPACE, &pn))
		return error;
	error = lookuppn(&pn, FOLLOW, NULLVPP, &vp);
	pn_free(&pn);
	if (error)
		return error;

	if (error = VOP_ACCESS(vp, VEXEC, 0, CRED())) {
		VN_RELE(vp);
		return error;
	}

	vattr.va_mask = AT_SIZE;
	if (error = VOP_GETATTR(vp, &vattr, 0, CRED())) {
		VN_RELE(vp);
		return error;
	}

	if (error = VOP_OPEN(&vp, FREAD, CRED())) {
		VN_RELE(vp);
		return error;
	}

	switch (mmf.mf_flags) {
	case 0:						/* shareable segment */
		mmf.mf_filesz = mmf.mf_regsz = 0;

		if (vattr.va_size == 0) 
			break;
		/*
		 * For this Xenix compatibility API the interface uses an
		 * mf_filesz of type (long).  Constrain the mapping to fit.
		 */
		filesz = (long)MIN(vattr.va_size, LONG_MAX);
		error = maptfile(vp, &mmf, filesz, &addr);
		if (!error)
			rvp->r_val1 = addr;
		break;

	case MAP_PRIVATE:				/* copy-on-write */
		if (mmf.mf_filesz > mmf.mf_regsz) {
			error = EFAULT;
			break;
		}
		if (mmf.mf_regsz == 0)
			break;
		error = mapdfile(vp, &mmf, &addr);
		if (!error)
			rvp->r_val1 = addr;
		break;

	default:
		error = EINVAL;
		break;
	}

	VOP_CLOSE(vp, FREAD, 1, 0, CRED());
	VN_RELE(vp);
	if (!error && copyout(&mmf, ap, sizeof mmf) < 0)
		error = EFAULT;
	return error;
}


/*
 * STATIC vaddr_t findhole(proc_t *p, long nbytes)
 *	Find a hole of nbytes in process p's user address space.
 *
 * Calling/Exit State:
 *	No spinlocks are held on entry, none are held on return.
 *	The process is single threaded before calling this function.
 */
STATIC vaddr_t
findhole(proc_t *p, long nbytes)
{
	struct as *as = u.u_procp->p_as;
	vaddr_t base;
	uint_t len, slen;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(SINGLE_THREADED());

	if (nbytes <= 0)
		return NULL;

	/* Leave a PAGESIZE redzone on each side of the request. */
	len = (nbytes + 2 * PAGESIZE + PAGEOFFSET) & PAGEMASK;

	base = p->p_brkbase;
	slen = uvend - base;

	if (as_gap(as, len, &base, &slen, AH_HI, NULL) == 0) {
		/*
		 * as_gap() returns the 'base' address of a hole of
		 * 'slen' bytes.  We want to use the top 'len' bytes
		 * of this hole, and set '*addrp' accordingly.
		 * The addition of PAGESIZE is to allow for the redzone
		 * page on the low end.
		 */
		ASSERT(slen >= len);
		return base + (slen - len) + PAGESIZE;
	}

	/* no more virtual space available */
	return NULL;
}

/*
 * STATIC int mapdfile(struct vnode *vp, struct mmf *mp, vaddr_t *addrp)
 *	Map the vnode as copy-on-write and return the address used.
 *
 * Calling/Exit State:
 *	No spinlocks are held on entry, none are held on return.
 *	The process is single threaded before calling this function.
 */
STATIC int
mapdfile(struct vnode *vp, struct mmf *mp, vaddr_t *addrp)
{
	proc_t *p = u.u_procp;
	vaddr_t	vaddr;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(SINGLE_THREADED());

	/*
	 * Find a virtual address to which to attach the segment.
	 */
	if ((vaddr = findhole(p, mp->mf_regsz)) == NULL)
		return ENOMEM;

	RWSLEEP_WRLOCK(&p->p_rdwrlock, PRIMED);

	error = execmap(vp, vaddr, mp->mf_filesz,
			(mp->mf_regsz - mp->mf_filesz), 0, PROT_ALL);

	RWSLEEP_UNLOCK(&p->p_rdwrlock);

	if (!error)
		*addrp = vaddr;

	return error;
}

/*
 * STATIC int maptfile(struct vnode *vp, struct mmf *mp, long filesz,
 *		       vaddr_t *addrp)
 *	Map the vnode as shareable and return the address used.
 *
 * Calling/Exit State:
 *	No spinlocks are held on entry, none are held on return.
 *	The process is single threaded before calling this function.
 */
STATIC int
maptfile(struct vnode *vp, struct mmf *mp, long filesz, vaddr_t *addrp)
{
	proc_t *p = u.u_procp;
	struct as *as = p->p_as;
	struct seg *seg, *endseg;
	vaddr_t	vaddr;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(SINGLE_THREADED());
	ASSERT(as && as->a_segs);

	{
		seg = endseg = as->a_segs;
		do {
			struct segvn_data *svd;
			if (seg->s_ops != &segvn_ops)
				continue;
			svd = (struct segvn_data *) seg->s_data;

			if (svd->svd_vp == vp &&
			    svd->svd_offset == 0 &&
			    !(svd->svd_flags & SEGVN_PGPROT) &&
			    !(svd->svd_prot & S_WRITE) &&
			    (filesz <= seg->s_size)) {
#ifdef DEBUG
	cmn_err(CE_CONT,"maptfile: segment exits: vaddr = %x\n",seg->s_base);
#endif
				mp->mf_filesz = filesz;
				mp->mf_regsz = seg->s_size;
				*addrp = seg->s_base;
				return 0;
			}
		} while ((seg = seg->s_next) != endseg);
	}

	RWSLEEP_WRLOCK(&p->p_rdwrlock, PRIMED);

	if ((vaddr = findhole(p, filesz)) == NULL)
		error = ENOMEM;
	else
		error = execmap(vp, vaddr, filesz, 0, 0,
				PROT_ALL & ~PROT_WRITE);

	RWSLEEP_UNLOCK(&p->p_rdwrlock);

	if (error)
		return error;

	mp->mf_filesz = filesz;
	seg = as_segat(as, vaddr);
	ASSERT(seg != NULL);
	mp->mf_regsz = seg->s_size;

	*addrp = vaddr;
	return 0;
}

/*
 * void
 * xsdswtch(int tofrom)
 *	Called during process switch if special XENIX shared
 *	  data context switching is to be performed. 
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *		tofrom == 0 when switching from this proc.
 *		tofrom == 1 when switching to this proc.
 *	
 *
 * Description:
 *	This routine is only called at context switch time, and only if 
 *      BADVISE_XSDSWTCH is true.  The BADVISE_XSDSWTCH can only be affected
 *	by the SI86BADVISE subcommand to the sysi86() system call.
 *
 *	When context switching TO the proc, the shared data will be copied
 *	from the "real" shared data segment, which starts at sd_addr, to
 *	the 286 small data executable's private data (at sd_cpaddr). When
 *	context switching FROM the proc, the shared data is copied from
 *	the 286 private copy (starting at sd_cpaddr) to the "real" shared
 *	data segment (at sd_addr).  Note that we will not block during
 *	the copy because noswapcnt is greater than zero for both the
 *	region where sd_cpaddr lives and the shared memory region where
 *	sd_addr lives.  The regions' noswapcnt field is affected as follows:
 *
 *		1.  The SI86SHRGN subcommand to the sysi86() system call 
 *		    will increment the 286 private data and shmem regions' 
 *		    noswapcnt if the shared data segment's sd_cpaddr is 
 *		    changed from NULL to something other than NULL.
 *
 *		2.  The SI86SHRGN subcommand to the sysi86() system call 
 *		    will decrement the 286 private data and shmem regions' 
 *		    noswapcnt if the shared data segment's sd_cpaddr is 
 *	  	    changed from non-NULL to NULL.
 *
 *		3.  At fork time, xsdfork() will increment the 
 *		    regions' noswapcnt for each shared data segment whose
 *		    sd_cpaddr is non-NULL.
 *
 *		4.  When a shared data segment whose sd_cpaddr is non-NULL is
 *		    freed, the regions' noswapcnt is decremented.
 *
 * N.B.  We'd really like to skip the copyin() when tofrom==0 if the sd
 *	 seg is attached read-only.  However, this is not the way XENIX
 *	 worked.  Instead, we allow changes made by the 286 small data
 *	 model proc to read-only shared data to be reflected in the
 *	 "real" shared data segment.
 */
void
xsdswtch(int tofrom)
{
	int	len;
	struct	sd *sdp;
	proc_t	*pp = u.u_procp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void) LOCK(&pp->p_mutex, PLHI);
	for (sdp = pp->p_sdp; sdp != NULL; sdp = sdp->sd_link) {
		if ((sdp->sd_addr == NULL) || (sdp->sd_cpaddr == NULL))
			continue;
		len = sdp->sd_xnamnode->x_sd->xsd_len + 1;
		if (tofrom) {
			/* switch TO this proc */
			if (copyout(sdp->sd_addr, sdp->sd_cpaddr, len) == -1) {
				/*
				 *+ Could not copy bytes of XENIX shared
				 *+ data from sdp->sd_addr to sdp->sd_cpaddr.
				 */
				cmn_err(CE_WARN,
					"xsdswtch - couldn't copy %d bytes of XENIX shared data from 0x%x to 0x%x",
					len, sdp->sd_addr, sdp->sd_cpaddr);
			}
		} else {
			/* switch FROM this proc */
			if (copyin(sdp->sd_cpaddr, sdp->sd_addr, len) == -1) {
				/*
				 *+ Could not copy bytes of XENIX shared
				 *+ data from sdp->sd_cpaddr to sdp->sd_addr.
				 */
				cmn_err(CE_WARN,
					"xsdswtch - couldn't copy %d bytes of XENIX shared data from 0x%x to 0x%x",
			   		len, sdp->sd_cpaddr, sdp->sd_addr);
			}
		}
	}
	UNLOCK(&pp->p_mutex, PLBASE);
}


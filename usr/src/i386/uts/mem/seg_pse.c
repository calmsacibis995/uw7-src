/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)kern-i386:mem/seg_pse.c	1.3.7.1"
#ident	"$Header$"

/*
 * segpse is a user level segment driver for mapping character
 * devices with PSE mappings.  It is derived from seg_dev.
 *
 * PSE (page size extension) is a feature of some Intel
 * processors which allows page directory entries to
 * map memory, as opposed to standard mappings, in which
 * the page directory entry points to a page table, which
 * in turn maps the page.
 */
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/pse_hat.h>
#include <mem/pse.h>
#include <mem/seg.h>
#include <mem/seg_dev.h>
#include <mem/seg_pse.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/map.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <fs/specfs/specfs.h>

extern ppid_t pse_mmap(); 

/*
 * Number of PSE_PAGESIZE units in a segment
 */
#define	SEGPSE_PAGES(seg)	ptopser(seg_pages(seg))

/*
 * PSE_PAGESIZE index of a virtual address within a segment
 */
#define	SEGPSE_PAGE(seg, addr)	ptopse(seg_page(seg, addr))

/*
 * Private seg op routines.
 */
STATIC int segpse_dup(struct seg *, struct seg *);
STATIC int segpse_unmap(struct seg *, vaddr_t, uint_t);
STATIC void segpse_free(struct seg *);
STATIC faultcode_t segpse_fault(struct seg *, vaddr_t, uint_t, enum fault_type,
				enum seg_rw);
STATIC int segpse_setprot(struct seg *, vaddr_t, uint_t, uint_t);
STATIC int segpse_checkprot(struct seg *, vaddr_t, uint_t);
STATIC int segpse_incore(struct seg *, vaddr_t, uint_t, char *);
STATIC int segpse_getprot(struct seg *, vaddr_t, uint_t *);
STATIC off64_t segpse_getoffset(struct seg *, vaddr_t);
STATIC int segpse_gettype(struct seg *, vaddr_t);
STATIC int segpse_getvp(struct seg *, vaddr_t, vnode_t **);
STATIC void segpse_badop(void);
STATIC int segpse_nop(void);
STATIC void segpse_age(struct seg *, u_int);
STATIC int segpse_memory(struct seg *, vaddr_t *basep, u_int *lenp);

/* Other private functions */
STATIC segpse_page_t *segpse_vpage_init(struct seg *seg,
					struct segpse_data *sdp);
STATIC void segpse_convert(struct seg *seg);

	/*+ Per-segment lock to mutex F_SOFTLOCK and F_SOFTUNLOCK faults */
STATIC LKINFO_DECL(segpse_lkinfo, "MS:segpse:mutex", 0);

STATIC struct seg_ops segpse_ops = {
	segpse_unmap,
	segpse_free,
	segpse_fault,
	segpse_setprot,
	segpse_checkprot,
	(int (*)())segpse_badop,	/* kluster */
	(int (*)())segpse_nop,		/* sync */
	segpse_incore,
	(int (*)())segpse_nop,		/* lockop */
	segpse_dup,
	(void(*)())segpse_nop,		/* childload */
	segpse_getprot,
	segpse_getoffset,
	segpse_gettype,
	segpse_getvp,
	segpse_age,			/* age */
	(boolean_t (*)())segpse_nop,	/* lazy_shootdown */
	segpse_memory,
	(boolean_t(*)())segpse_nop,	/* xlat_op */
	(int (*)())segpse_nop,		/* memloc */
};

/*
 * int
 * segpse_create(struct seg *seg, void *argsp)
 *	Create a user segment for mapping device memory via PSE mappings.
 *
 * Calling/Exit State:
 *	Called with the AS write locked.
 *	Returns with the AS write locked.
 *
 * Description:
 *	Create a segpse type segment.
 */
int
segpse_create(struct seg *seg, void *argsp)
{
	struct segpse_data *sdp;
	struct segpse_crargs *a = argsp;
	int error;
	uint_t off;
	ppid_t ppid;

	ASSERT(seg->s_as != &kas);
	ASSERT((seg->s_base & PSE_PAGEOFFSET) == 0);
	ASSERT((seg->s_size & PSE_PAGEOFFSET) == 0);

	if (!PSE_SUPPORTED())
		return segdev_create(seg, argsp);
	sdp = kmem_alloc(sizeof(struct segpse_data), KM_SLEEP);
	sdp->offset = a->offset;
	sdp->prot = a->prot;
	sdp->maxprot = a->maxprot;
	sdp->vpage = NULL;

	/* Hold associated vnode -- segpse only deals with CHR devices */
	sdp->vp = specfind(a->dev, VCHR);
	ASSERT(sdp->vp != NULL);

	/* Inform the vnode of the new mapping */
	error = VOP_ADDMAP(sdp->vp, sdp->offset, seg->s_as, seg->s_base,
			   seg->s_size, sdp->prot, sdp->maxprot, MAP_SHARED,
			   u.u_lwpp->l_cred);
	if (error != 0) {
		VN_RELE(sdp->vp);
		kmem_free(sdp, sizeof(*sdp));
		return error;
	}

	/*
	 * Load mappings for the segment.
	 */
	for (off = 0 ; off < seg->s_size ; off += PSE_PAGESIZE) {
		ppid = pse_mmap(sdp->vp->v_rdev, sdp->offset + off, sdp->prot);
		ASSERT(ppid != NOPAGE);
		HOP_PSE_DEVLOAD(seg, seg->s_base + off, ppid, sdp->prot);
	}

	LOCK_INIT(&sdp->mutex, VM_SEGDEV_HIER, PLMIN, &segpse_lkinfo,
		  KM_SLEEP);

	seg->s_ops = &segpse_ops;
	seg->s_data = sdp;
	seg->s_as->a_isize += seg->s_size;

	return 0;
}

/*
 * STATIC void
 * segpse_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */
STATIC void
segpse_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ segpse segment driver.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segpse_badop");
	/*NOTREACHED*/
}

/*
 * STATIC void
 * segpse_nop(void)
 *	Do-nothing operation.
 *
 * Calling/Exit State:
 *	Always returns success w/o doing anything.
 */
STATIC int
segpse_nop(void)
{
	return 0;
}

/*
 * STATIC int
 * segpse_dup(struct seg *pseg, struct seg *cseg)
 *	Called from as_dup to replicate segment specific data structures,
 *	inform filesystems of additional mappings for vnode-backed segments,
 *	and, as an optimization to fork, pre-load copies of translations
 *	currently established in the parent segment.
 *
 * Calling/Exit State:
 *	The parent's address space is read locked on entry to the call and
 *	remains so on return.
 *
 *	The child's address space is not locked on entry to the call since
 *	there can be no active LWPs in it at this point in time.
 *
 *	On success, 0 is returned to the caller and s_data in the child
 *	generic segment stucture points to the newly created segvn_data.
 *	On failure, non-zero is returned and indicates the appropriate
 *	errno.
 */
STATIC int
segpse_dup(struct seg *pseg, struct seg *cseg)
{
	struct segpse_data *psdp = pseg->s_data;
	struct segpse_data *csdp;
	struct segpse_crargs a;
	segpse_page_t *vp, *evp;
	vaddr_t va;
	int error;

	a.dev = psdp->vp->v_rdev;
	a.offset = psdp->offset;
	a.prot = psdp->prot;
	a.maxprot = psdp->maxprot;

	if ((error = segpse_create(cseg, &a)) != 0)
		return error;

	csdp = cseg->s_data;
	if (psdp->vpage != NULL) {
		size_t nbytes = SEGPSE_PAGES(pseg) * sizeof(segpse_page_t);
		csdp->vpage = kmem_alloc(nbytes, KM_SLEEP);
		bcopy(psdp->vpage, csdp->vpage, nbytes);
		va = cseg->s_base;
		vp = csdp->vpage;
		evp = &vp[SEGPSE_PAGES(cseg)];
		while (vp < evp) {
#ifdef	DEBUG
			ASSERT(SOP_SETPROT(cseg, va, PSE_PAGESIZE,
				vp->dvp_prot) == 0);
#else
			(void)SOP_SETPROT(cseg, va, PSE_PAGESIZE, vp->dvp_prot);
#endif
			++vp;
			va += PSE_PAGESIZE;
		}
	}
	return 0;
}

/*
 * STATIC int
 * segpse_unmap(struct seg *seg, vaddr_t addr, size_t len)
 *	Unmap a portion (possibly all) of the specified segment.
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusively locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 *
 *	On success, 0 is returned and the request chunk of the address
 *	space has been deleted. On failure, non-zero is returned and
 *	indicates the appropriate errno.
 *
 * Remarks:
 *	If the range unmapped falls into the middle of a segment the
 *	result will be the creation of a hole in the address space and
 *	the creation of a new segment.
 *
 *	If a partial unmapping is requested, such that the segment
 *	(or segments, if there is a hole) would no longer be aligned
 *	on PSE_PAGESIZE boundaries, then the segment is first converted
 *	to a segdev type segment by calling segpse_convert, and then
 *	the operation is carried out (by calling SOP_UNMAP).  The
 *	process will still be able to access the pages, but the
 *	access will be through standard non-PSE mappings rather than
 *	through PSE mappings.
 *
 *	In the case of non-4MB-aligned partial unmappings, we always
 *	convert the entire segment to a segvdev mapping even though
 *	it may not always be necessary to do so.  For instance, if
 *	there is an 8MB segpse segment, and the region between 6 and
 *	7MB were unmapped, the first 4MB could remain a segpse segment,
 *	and new segdev segments could be created for the 4MB-6MB and
 *	7MB-8MB parts of the segment.  This optimization is omitted,
 *	for simplicity and because such unmappings are expected to
 *	be infrequent.
 */
STATIC int
segpse_unmap(struct seg *seg, vaddr_t addr, size_t len)
{
	struct segpse_data *sdp = seg->s_data;
	struct segpse_data *nsdp;
	struct seg *nseg;
	uint_t	opages,		/* old segment size in pages */
		npages,		/* new segment size in pages */
		dpages;		/* pages being deleted (unmapped)*/

	vaddr_t nbase;
	size_t nsize;

	/*
	 * Check for bad sizes
	 */
	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
	    (len & PAGEOFFSET) || (addr & PAGEOFFSET)) {
		/*
		 *+ A request was made to unmap segpse segment addresses
		 *+ which are outside of the segment.  This indicates a
		 *+ kernel software problem.
		 */
		cmn_err(CE_PANIC, "segpse_unmap");
	}

	if (((addr & PSE_PAGEOFFSET) != 0) || ((len & PSE_PAGEOFFSET) != 0)) {
		/* convert to segdev and then unmap */
		segpse_convert(seg);
		return SOP_UNMAP(seg, addr, len);
	}

	seg->s_as->a_isize -= len;
	/*
	 * Unload any hardware translations in the range to be taken out.
	 */
	HOP_PSE_UNLOAD(seg, addr, len);

	/* Inform the vnode of the unmapping. */
	ASSERT(sdp->vp != NULL);
	(void)VOP_DELMAP(sdp->vp, sdp->offset, seg->s_as, addr, len, sdp->prot,
		       sdp->maxprot, MAP_SHARED, u.u_lwpp->l_cred);

	/*
	 * Check for entire segment
	 */
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		return 0;
	}

	opages = SEGPSE_PAGES(seg);
	dpages = btopse(len);
	npages = opages - dpages;

	/*
	 * Check for beginning of segment
	 */
	if (addr == seg->s_base) {
		if (sdp->vpage != NULL) {
			size_t nbytes;
			segpse_page_t *ovpage;

			ovpage = sdp->vpage;	/* keep pointer to vpage */

			nbytes = npages * sizeof(segpse_page_t);
			sdp->vpage = kmem_alloc(nbytes, KM_SLEEP);
			bcopy(&ovpage[dpages], sdp->vpage, nbytes);

			/* free up old vpage */
			kmem_free(ovpage, opages * sizeof(segpse_page_t));
		}
		sdp->offset += len;

		seg->s_base += len;
		seg->s_size -= len;
		return 0;
	}

	/*
	 * Check for end of segment
	 */
	if (addr + len == seg->s_base + seg->s_size) {
		if (sdp->vpage != NULL) {
			size_t nbytes;
			segpse_page_t *ovpage;

			ovpage = sdp->vpage;	/* keep pointer to vpage */

			nbytes = npages * sizeof(segpse_page_t);
			sdp->vpage = kmem_alloc(nbytes, KM_SLEEP);
			bcopy(ovpage, sdp->vpage, nbytes);

			/* free up old vpage */
			kmem_free(ovpage, opages * sizeof(segpse_page_t));

		}
		seg->s_size -= len;
		return 0;
	}

	/*
	 * The section to go is in the middle of the segment,
	 * have to make it into two segments.  nseg is made for
	 * the high end while seg is cut down at the low end.
	 */
	nbase = addr + len;				/* new seg base */
	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
	seg->s_size = addr - seg->s_base;		/* shrink old seg */
	nseg = seg_alloc(seg->s_as, nbase, nsize);
	ASSERT(nseg != NULL);

	nseg->s_ops = seg->s_ops;
	nsdp = kmem_alloc(sizeof(struct segpse_data), KM_SLEEP);
	nseg->s_data = nsdp;
	nsdp->prot = sdp->prot;
	nsdp->maxprot = sdp->maxprot;
	nsdp->offset = sdp->offset + nseg->s_base - seg->s_base;
	nsdp->vp = sdp->vp;
	VN_HOLD(nsdp->vp);	/* Hold vnode associated with the new seg */

	LOCK_INIT(&nsdp->mutex, VM_SEGDEV_HIER, PLMIN, &segpse_lkinfo,
		  KM_SLEEP);

	if (sdp->vpage == NULL)
		nsdp->vpage = NULL;
	else {
		/* need to split vpage into two arrays */
		size_t nbytes;
		segpse_page_t *ovpage;

		ovpage = sdp->vpage;	/* keep pointer to vpage */

		npages = SEGPSE_PAGES(seg);	/* seg has shrunk */
		nbytes = npages * sizeof(segpse_page_t);
		sdp->vpage = kmem_alloc(nbytes, KM_SLEEP);

		bcopy(ovpage, sdp->vpage, nbytes);

		npages = SEGPSE_PAGES(nseg);
		nbytes = npages * sizeof(segpse_page_t);
		nsdp->vpage = kmem_alloc(nbytes, KM_SLEEP);

		bcopy(&ovpage[opages - npages], nsdp->vpage, nbytes);

		/* free up old vpage */
		kmem_free(ovpage, opages * sizeof(segpse_page_t));
	}

	return 0;
}

/*
 * STATIC void
 * segpse_free(struct seg *seg)
 *	Free a segment.
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusivley locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 */
STATIC void
segpse_free(struct seg *seg)
{
	struct segpse_data *sdp = seg->s_data;

	VN_RELE(sdp->vp);
	if (sdp->vpage != NULL)
		kmem_free(sdp->vpage, SEGPSE_PAGES(seg) *
			sizeof(segpse_page_t));

	LOCK_DEINIT(&sdp->mutex);

	kmem_free(sdp, sizeof(*sdp));
}

/*
 * STATIC faultcode_t
 * segpse_fault(struct seg *seg, vaddr_t addr, size_t len,
 *		enum fault_type type, enum seg_rw rw)
 *	Fault handler; called for both hardware faults and softlock requests.
 *
 * Calling/Exit State:
 *	Called with the AS lock held (in read mode) and returns the same.
 *
 *	Addr and len arguments have been properly aligned and rounded
 *	with respect to page boundaries by the caller (this is true of
 *	all SOP interfaces).
 *
 *	On success, 0 is returned and the requested fault processing has
 *	taken place. On error, non-zero is returned in the form of a
 *	fault error code.
 */
/*ARGSUSED*/
STATIC faultcode_t
segpse_fault(struct seg *seg, vaddr_t addr, size_t len, enum fault_type type,
	     enum seg_rw rw)
{
	uint_t protchk;

	/*
	 * SOFTLOCK and SOFTUNLOCK faults do nothing. 
	 */
	if (type == F_SOFTLOCK || type == F_SOFTUNLOCK)
		return 0;

	/*
	 * We should never get validity faults, because translations for
	 * segpse segments are loaded when the segment is created, and
	 * aren't loaded until the segment is unmapped
	 */
	ASSERT(type != F_INVAL);

	switch (rw) {
	case S_READ:
		protchk = PROT_READ;
		break;
	case S_WRITE:
		protchk = PROT_WRITE;
		break;
	case S_EXEC:
		protchk = PROT_EXEC;
		break;
	case S_READ_ACCESS:
		protchk = PROT_READ | PROT_EXEC;
		break;
	default:
		protchk = PROT_READ | PROT_WRITE | PROT_EXEC;
		break;
        }

	ASSERT(len == 1);

	if (segpse_checkprot(seg, addr, protchk) != 0)
		return FC_PROT;

	return (0);
}

/*
 * STATIC int
 * segpse_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
 *	Change the protections on a range of pages in the segment.
 *
 * Calling/Exit State:
 *	Called and exits with the address space exclusively locked.
 *
 *	Returns zero on success, returns a non-zero errno on failure.
 *
 * Remarks:
 *	If the requested protection change is not aligned on PSE_PAGESIZE
 *	boundaries, the segment is converted to a segdev segment,
 *	and then the operation is carried out.  The user will
 *	continue to access the physical pages, but the mappings
 *	will be standard, non-PSE mappings.
 *
 *	In the case of non-4MB-aligned partial unmappings, we always
 *	convert the entire segment to a segvdev mapping even though
 *	it may not always be necessary to do so.  For instance, if
 *	there is an 8MB segpse segment, and the region between 6 and
 *	7MB were unmapped, the first 4MB could remain a segpse segment,
 *	and new segdev segments could be created for the 4MB-6MB and
 *	7MB-8MB parts of the segment.  This optimization is omitted,
 *	for simplicity and because such unmappings are expected to
 *	be infrequent.
 */
STATIC int
segpse_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
{
	struct segpse_data *sdp = seg->s_data;
	segpse_page_t *vp, *evp;

	if ((sdp->maxprot & prot) != prot)
		return EACCES;		/* violated maxprot */

	if (addr == seg->s_base && len == seg->s_size && sdp->vpage == NULL) {
		if (sdp->prot == prot)
			return 0;			/* all done */
		sdp->prot = (uchar_t)prot;
	} else if (((addr & PSE_PAGEOFFSET) == 0) && ((len & PSE_PAGEOFFSET) == 0)) {
		if (sdp->vpage == NULL)
			sdp->vpage = segpse_vpage_init(seg, sdp);
		/*
		 * Now go change the needed vpages protections.
		 */
		evp = &sdp->vpage[SEGPSE_PAGE(seg, addr + len)];
		for (vp = &sdp->vpage[SEGPSE_PAGE(seg, addr)]; vp < evp; vp++)
			vp->dvp_prot = (uchar_t)prot;
	} else {
		segpse_convert(seg);
		return SOP_SETPROT(seg, addr, len, prot);
	}

	HOP_PSE_CHGPROT(seg, addr, len, prot, B_TRUE);

	return 0;
}

/*
 * STATIC int
 * segpse_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
 *	Determine that the vpage protection for addr  
 *	is at least equal to prot.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	On success, 0 is returned, indicating that the addr
 *	allow accesses indicated by the specified protection.
 *	Actual protection may be greater.
 *	On failure, EACCES is returned, to indicate that 
 *	the page does not allow the desired access.
 */
STATIC int
segpse_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
{
	struct segpse_data *sdp = seg->s_data;
	segpse_page_t *vp;

	ASSERT((addr & PAGEOFFSET) == 0);
	/*
	 * If segment protections can be used, simply check against them.
	 */
	if (sdp->vpage == NULL)
		return (((sdp->prot & prot) != prot) ? EACCES : 0);

	/*
	 * Have to check down to the vpage level.
	 */
	vp = &sdp->vpage[SEGPSE_PAGE(seg, addr)];
	if ((vp->dvp_prot & prot) != prot)
		return EACCES;

	return 0;
}

/*
 * STATIC int
 * segpse_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
 *	Return the protections on pages starting at addr for len.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	This function, which cannot fail, returns the permissions of the
 *	indicated pages in the protv array.
 */
STATIC int
segpse_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
{
 	struct segpse_data *sdp = seg->s_data;
	uint_t pgoff;

	ASSERT((addr & PAGEOFFSET) == 0);

	if (sdp->vpage == NULL) {
 		*protv = sdp->prot;
	} else {
		pgoff = SEGPSE_PAGE(seg, addr);
		*protv = sdp->vpage[pgoff].dvp_prot;
	}
	return 0;
}


/*
 * STATIC off64_t
 * segpse_getoffset(struct seg *seg, vaddr_t addr)
 *	Return the vnode offset mapped at the given address within the segment.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
STATIC off64_t
segpse_getoffset(struct seg *seg, vaddr_t addr)
{
	struct segpse_data *sdp = seg->s_data;

	return (addr - seg->s_base) + sdp->offset;
}

/*
 * STATIC int
 * segpse_gettype(struct seg *seg, vaddr_t addr)
 *	Return the segment type (MAP_SHARED||MAP_PRIVATE) to the caller.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC int
segpse_gettype(struct seg *seg, vaddr_t addr)
{
	return MAP_SHARED;
}

/*
 * STATIC int
 * segpse_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
 *	Return the vnode associated with the segment.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC int
segpse_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
{
	struct segpse_data *sdp = seg->s_data;

	ASSERT(sdp->vp != NULL);
	*vpp = sdp->vp;
	return 0;
}

/*
 * STATIC int
 * segpse_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
 *	Return an indication, in the array, vec, of whether each page
 *	in the given range is "in core".
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	"Pages" for segpse are always "in core", so set all to true.
 */
/*ARGSUSED*/
STATIC int
segpse_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
{
	size_t v = 0;

	for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE) {
		*vec++ = 1;
		v += PAGESIZE;
	}
	return (int)v;
}


/*
 * STATIC segpse_page_t *
 * segpse_vpage_init(struct seg *seg, struct segpse_data *sdp)
 *	Allocate and initialize a vpage array for the segment.
 *
 * Calling/Exit State:
 *	Called with exclusive access to the segment, either by holding
 *	the AS lock exclusively, or by holding sdp->mutex.
 *
 * Remarks:
 *	The vpage array is allocated one per PSE page.
 */
STATIC segpse_page_t *
segpse_vpage_init(struct seg *seg, struct segpse_data *sdp)
{
	segpse_page_t *vpage, *vp;

	/* round up segment size to a PSE boundary */
	/* Allocate an array of per-page structures. */
	vpage = kmem_alloc(SEGPSE_PAGES(seg) * sizeof(segpse_page_t), KM_SLEEP);

	/* Initialize all pages to the current segment-wide protections. */
	for (vp = &vpage[SEGPSE_PAGES(seg)]; vp-- != vpage;)
		vp->dvp_prot = sdp->prot;

	return vpage;
}

/*
 * STATIC void
 * segpse_age(struct seg *, u_int type)
 *	Age the translations for a segpse segment.
 *
 * Calling/Exit State:
 *	The process owning the AS which owns the argument segment has
 *	been seized.
 *
 *	This function does not block.
 *
 * Remarks:
 *	This is a no-op, but must be present in order to prevent
 *	hat_agerange from aging this segment.
 */
/*ARGSUSED*/
STATIC void
segpse_age(struct seg * seg, u_int type)
{

}

/*
 * STATIC int
 * segpse_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
 *	This is a no-op for segpse.
 *
 * Calling/Exit State:
 *	returns ENOMEM.
 */
/*ARGSUSED*/
STATIC int
segpse_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
{
	return ENOMEM;
}

/*
 * STATIC void
 * segpse_convert(struct seg *seg)
 *	Convert a segpse segment to a segdev segment
 *
 * Calling/Exit State:
 *	Called with the AS write locked.
 *	Returns with the AS write locked.
 */
STATIC void
segpse_convert(struct seg *seg)
{
 	struct segpse_data *sdp = seg->s_data;
	struct segdev_crargs args;
	segpse_page_t *vp, *evp;
	vaddr_t va;

	ASSERT(sdp->vp != NULL);
	/*
	 * unload translations in hat layer.  Note that there can be
	 * no pending softlocks because softlocks are held only
	 * so long as the as is read-locked, and the as is currently
	 * write-locked.
	 */
	HOP_PSE_UNLOAD(seg, seg->s_base, seg->s_size);
	seg->s_as->a_isize -= seg->s_size;

	/*
	 * Now, the segment is unmapped, and we can simply recreate
	 * it as a seg_dev segment
	 */
	seg->s_data = NULL;
	seg->s_ops = NULL;
	args.mapfunc = (ppid_t (*)(void *, channel_t, size_t, int))pse_mmap;
	args.chan = (channel_t)sdp->vp->v_rdev;	/* gag */
	args.idata = NULL;
	args.cfgp = NULL;
	args.offset = sdp->offset;
	args.prot = sdp->prot;
	args.maxprot = sdp->maxprot;
	segdev_create(seg, &args);

	/*
	 * Inform the vnode that the old mapping disappeared.  We
	 *	do this *after* the new mapping is created; otherwise,
	 *	there is a risk that the vnode would appear to have
	 *	no mappings, which could result in it going away
	 *	or something.
	 */
	(void)VOP_DELMAP(sdp->vp, sdp->offset, seg->s_as, seg->s_base,
			seg->s_size, sdp->prot, sdp->maxprot, MAP_SHARED,
			u.u_lwpp->l_cred);

	/*
	 * Go through old vpage array, and change protections
	 * and do soft-locks on new segment, if needed
	 */
	if (sdp->vpage != NULL) {
		va = seg->s_base;
		vp = sdp->vpage;
		evp = &vp[SEGPSE_PAGES(seg)];
		while (vp < evp) {
#ifdef	DEBUG
			ASSERT(SOP_SETPROT(seg, va, PSE_PAGESIZE,
				vp->dvp_prot) == 0);
#else
			(void)SOP_SETPROT(seg, va, PSE_PAGESIZE, vp->dvp_prot);
#endif
			++vp;
			va += PSE_PAGESIZE;
		}
		kmem_free(sdp->vpage,
			SEGPSE_PAGES(seg) * sizeof(segpse_page_t));
	}
	LOCK_DEINIT(&sdp->mutex);
	kmem_free(sdp, sizeof(*sdp));
}

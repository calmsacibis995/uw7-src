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

#ident	"@(#)kern-i386:mem/fgashm.c	1.2.1.2"
#ident	"$Header$"

/*
 * fgashm is a special type of shared memory which supports persistant
 * page granular affinity on a NUMA system.  fgashm segments are
 * mapped by seg_fga.
 *
 * The routines in this file are utility routines which can be called
 * from either the mainline shm code in proc/ipc or from the segfga
 * routines in mem/seg_fga.c.
 *
 * fgashm uses identityless memory (memory which has no associated page_t
 * structures, and thus no vnode,offset identity) and shared page tables.
 * The use of shared page tables imposes several externally visible
 * restrictions, such as strict alignment restrictions, the inability to
 * change protections on the pages (because they would be seen by all users
 * of the page), etc.  Identityless memory is also never paged, thus
 * creation of a fgashm segment requires PLOCK privilege.
 */
#include <io/conf.h>
#include <mem/as.h>
#include <mem/fgashm.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/rzbm.h>
#include <mem/seg.h>
#include <mem/seg_fga.h>
#include <mem/vmparam.h>
#include <proc/cg.h>
#include <proc/cguser.h>
#include <proc/ipc/shm.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/list.h>
#include <util/map.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifdef PAE_MODE

STATIC LKINFO_DECL(fgashm_lkinfo, "MS:FGA:fgashm mutex", 0);

/*
 * Shared page table interfaces (move to vm_hatshm.h???)
 */
extern hatshpt_t * shpt_hat_init(uint_t sz);
extern void shpt_hat_load(struct as *as, hatshpt_t *hshptp, vaddr_t baseaddr,
      		          uint_t off, ppid_t pf, uint_t prot);
extern void shpt_hat_unload(struct as *as, hatshpt_t *hshptp, uint_t off);
extern void shpt_hat_attach(struct as *as, vaddr_t addr, size_t sz,
               		    hatshpt_t *hshptp, cgnum_t cgnum);
extern void shpt_hat_detach(struct as *as, vaddr_t addr, size_t sz,
				hatshpt_t *hshptp);
extern void shpt_hat_deinit(hatshpt_t *hshptp);
extern uint_t shpt_hat_xlat_info(struct as *as, hatshpt_t *hshptp,
				 vaddr_t addr);

/*
 * TEMPORARY
 */
cgnum_t idf_place(ppid_t ppid);

cgnum_t idf_place(ppid_t ppid)
{
return(0);
}

extern uint_t fgashm_def_granularity;

#ifdef DEBUG
#define fgashm_bumpcnt(x)	((x)++)

static int fgashm_reaff_case1;
static int fgashm_reaff_case2;
static int fgashm_reaff_case3;
static int fgashm_del_mig_count;
static int fgashm_mig_count;

#else
#define fgashm_bumpcnt(x)
#endif

/*
 * int
 * shm_makefga(struct kshmid_ds *kshmp, size_t nbytes, int flag)
 *
 * Allocate and initialize a FGA SHM segment and all the data structures
 * necessary to manage it.
 *
 * Calling/Exit State:
 *
 * On success, 0 is returned and the FGA SHM segment is ready to be used.
 * On failure, an errno is returned to indicate the failure.
 */
int shm_makefga(struct kshmid_ds *kshmp, size_t nbytes, int flag)
{
	uint_t			index1;
	uint_t			index2;
	uint_t			num_pages;
	fgavpg_t *		vpgp;


	ASSERT(nbytes != 0);

/*
 * Validate the segment size.
 */
	if ((nbytes & (FGASHM_SIZE_QUANTA - 1)) != 0) {
		return(EINVAL);
	}

	num_pages = btopr(nbytes);

/*
 * Ensure that there is enough identityless memory to back the segment.
 * NOSLEEP is used here because the turnover of identityless memory is
 * anticipated to be low, so waiting for the memory to become available
 * would be a very lengthy process.  However, this was a decision made
 * without facts available...
 */
	if (!idf_resv(num_pages, NOSLEEP)) {
		return(ENOMEM);
	}


/*
 * Allocate the management data structures necessary for the segment.
 */
	kshmp->kshm_fgap = kmem_alloc(sizeof(struct fgashm_t), KM_SLEEP);

	kshmp->kshm_fgap->fshm_hatshptp = shpt_hat_init(nbytes);
	if (kshmp->kshm_fgap->fshm_hatshptp == NULL) {
		idf_unresv(num_pages);
		return(ENOMEM);
	}
	
	kshmp->kshm_align = FGASHM_ALIGN;
	kshmp->kshm_mapsize = nbytes;

	LOCK_INIT(&kshmp->kshm_fgap->fshm_vpglck, VM_FGA_HIER, PLMIN,
						&fgashm_lkinfo, KM_SLEEP);
	SV_INIT(&kshmp->kshm_fgap->fshm_vpgsv);

	INITQUE(&kshmp->kshm_fgap->fshm_aff_head.fh_anchor);
	FSPIN_INIT(&kshmp->kshm_fgap->fshm_aff_head.fh_lock);

/*
 * Establish the original affinity for the segment.  Could probably do
 * something more intelligent here, but, for now, simply choose a policy
 * and revisit later if necessary.
 */
	if (fgashm_def_granularity == 0) {
		fgashm_def_granularity = PAGESIZE;
		cmn_err(CE_NOTE,
			"fgashm_def_granularity has illegal 0 value, changing to PAGESIZE.");
	}

	if (flag & SHM_AFFINITY_MASK) {
		kshmp->kshm_fgap->fshm_aff_head.fh_def_aff = flag & SHM_AFFINITY_MASK;
	} else {
		kshmp->kshm_fgap->fshm_aff_head.fh_def_aff = SHM_PLC_DEFAULT;
	}
	kshmp->kshm_fgap->fshm_aff_head.fh_def_cgnum = mycg;
	kshmp->kshm_fgap->fshm_aff_head.fh_def_granularity = fgashm_def_granularity;

/*
 * Allocate the data structures which are used to manage the individual
 * pages in the shm segment.  These structures are managed in a two
 * dimensional array in order to avoid > 64K kmem_alloc() allocations.
 */
	kshmp->kshm_fgap->fshm_vpgpp =
		kmem_alloc(fshm_idx1(num_pages) * sizeof(fgavpg_t *),
			   KM_SLEEP);

	for (index1 = 0; index1 < fshm_idx1(num_pages); index1++) {
		kshmp->kshm_fgap->fshm_vpgpp[index1] =
			kmem_zalloc(FGA_LEVEL2_SZ * sizeof(fgavpg_t),
				   KM_SLEEP);

		vpgp = &kshmp->kshm_fgap->fshm_vpgpp[index1][0];
		for (index2 = 0; index2 < FGA_LEVEL2_SZ; index2++, vpgp++) {
			vpgp->fvp_ppid = VPG_NONEXISTENT;
		}
	}

	return(0);
} /* shm_makefga */

/*
 * void
 * shm_rm_fga(struct kshmid_ds *kshmp)
 *
 * Perform all necessary actions to remove a fga shm segment from the
 * system.  This basically involves releasing the identityless memory,
 * the shared page tables, and any memory allocations which have been
 * made.
 *
 * This routine should basically act like shm_makefga() in reverse.
 *
 * Calling/Exit State:
 */
void shm_rm_fga(struct kshmid_ds *kshmp)
{
	uint_t			index1;
	uint_t  		index2;
	fgavpg_t *		vpgp;
	uint_t			num_pages;


	num_pages = btopr(kshmp->kshm_mapsize);

/*
 * Release any instantiated pages back to the system.
 */
	for (index1 = 0; index1 < fshm_idx1(num_pages); index1++) {
		vpgp = &kshmp->kshm_fgap->fshm_vpgpp[index1][0];
		for (index2 = 0; index2 < FGA_LEVEL2_SZ; index2++, vpgp++) {
			ASSERT(vpgp->fvp_softlock_cnt == 0);
			ASSERT(vpgp->fvp_contended == 0);
			ASSERT((vpgp->fvp_flag & FVP_LOCK_FLAG) == 0);

			if (vpgp->fvp_ppid != VPG_NONEXISTENT) {
				idf_page_free(vpgp->fvp_ppid, PAGESIZE);
				vpgp->fvp_ppid = VPG_NONEXISTENT;
			}
		}
		kmem_free(kshmp->kshm_fgap->fshm_vpgpp[index1],
				(FGA_LEVEL2_SZ * sizeof(fgavpg_t)));
	}
	kmem_free(kshmp->kshm_fgap->fshm_vpgpp,
			(fshm_idx1(num_pages) * sizeof(fgavpg_t *)));

/*
 * Free any entries on the linked list of affinity information.
 */
	fgashm_destroy_affinity(&kshmp->kshm_fgap->fshm_aff_head);

	LOCK_DEINIT(&kshmp->kshm_fgap->fshm_vpglck);

/*
 * Return the shared page tables and the identityless memory reservation.
 */
	shpt_hat_deinit(kshmp->kshm_fgap->fshm_hatshptp);
	idf_unresv(num_pages);

	kmem_free(kshmp->kshm_fgap, sizeof(*kshmp->kshm_fgap));
	kshmp->kshm_fgap = NULL;

	kshmp->kshm_flag &= ~SHM_SUPPORTS_AFFINITY;

	return;
} /* shm_rm_fga */


/*
 * STATIC
 * int fgashm_reaffine(struct kshmid_ds *kshmp, struct shmid_ds *shmp)
 *	Change the affinity of the fga shm segment.
 *
 * Calling/Exit State:
 *
 *	The caller has insured that the fga shm segment is valid and will
 *	remain so for the duration of this call.  The caller has insured
 *	that there are no other concurrent callers of this function for
 *	this shared memory segment (currently done by holding SHM_BUSY for
 *	the segment).
 *
 *	No locks are held on entry or exit.
 *
 *	This function may block if:
 *		VPG locks are held on the fga shm segment
 *
 *	0 is returned on success, an errno on error.
 *
 * Remarks:
 */
int fgashm_reaffine(struct kshmid_ds *kshmp, struct shmid_ds *shmp)
{
	struct fgashm_t *		fgap;
	int				affinity;
	fgashm_affinity_t *		ins_eltp;
	fgashm_affinity_t *		aff_eltp;
	fgashm_affinity_t *		nextp;
	fgashm_affinity_t *		splitp;

	struct segfga_data 		fake_sfd;
	size_t				offset;
	size_t				endoffset;
	uint_t				pg_offset;
	fgavpg_t *			vpgp;
	cgnum_t				cgnum;


	fgap = kshmp->kshm_fgap;

	/*
	 * Validate the passed in affinity request makes sense and allocate
	 * and initialize the structure which will be placed on the linked
	 * list with the affinity.
	 */
	affinity = shmp->shm_placepolicy & SHM_AFFINITY_MASK;
	if ((affinity != SHM_BALANCED) && (affinity != SHM_CPUGROUP) &&
	    (affinity != SHM_FIRSTUSAGE) && (affinity != SHM_PLC_DEFAULT)) {
		return(EINVAL);
	}

	if (shmp->shm_placepolicy & ~(SHM_AFFINITY_MASK | SHM_MIGRATE)) {
		return(EINVAL);
	}

	if (affinity == SHM_CPUGROUP) {
		switch (cgnum = cgid2cgnum(shmp->shm_cg)) {
		case CG_NONE:
		case CG_CURRENT:
			cgnum = mycg;
			break;

		default:
			if (cgnum < 0 || !IsCGOnforU(cgnum)) {
				return(EINVAL);
			}
			break;
		}
	} else {
		cgnum = CG_NONE;
	}

	if ((affinity == SHM_BALANCED) && (shmp->shm_granularity == 0)) {
		shmp->shm_granularity = fgashm_def_granularity;
	}

	if (affinity == SHM_PLC_DEFAULT) {
		shmp->shm_granularity = fgashm_def_granularity;
	}

	/*
	 * Round both shm_nbytes and shm_granularity to be multiples of the
	 * page size.
	 */
	shmp->shm_nbytes = (shmp->shm_nbytes + PAGEOFFSET) & PAGEMASK;
	shmp->shm_granularity = (shmp->shm_granularity + PAGEOFFSET) & PAGEMASK;

	/*
	 * Validate that the passed in range is completely within the
	 * shared memory segment.  Remember that shm_off == 0 and
	 * shm_nbytes == 0 is a way to say "the entire shm segment".
	 */
	if ((shmp->shm_off & PAGEOFFSET) ||
		((shmp->shm_off + shmp->shm_nbytes) < shmp->shm_off) ||
		(shmp->shm_off >= kshmp->kshm_mapsize) ||
		((shmp->shm_off + shmp->shm_nbytes) > kshmp->kshm_mapsize)) {
		return(EINVAL);
	}

	if ((shmp->shm_nbytes == 0) && (shmp->shm_off != 0)) {
		return(EINVAL);
	}

	/*
	 * Handle the special case of the whole segment.
	 */
	if (((shmp->shm_off == 0) && (shmp->shm_nbytes == 0)) ||
		((shmp->shm_off == 0) &&
				(shmp->shm_nbytes == kshmp->kshm_mapsize))) {
		fgashm_destroy_affinity(&fgap->fshm_aff_head);

		fgap->fshm_aff_head.fh_def_aff = affinity;
		fgap->fshm_aff_head.fh_def_cgnum = cgnum;
		fgap->fshm_aff_head.fh_def_granularity = shmp->shm_granularity;

		goto fga_reaff_return;
	}

	/*
	 * Set up the element to be inserted and search through the list
	 * for the proper place to insert it (which may involve altering
	 * linked list ranges which overlap with the new range).
	 * The list is ordered by increasing fa_soffset.
	 */
	ins_eltp = kmem_alloc(sizeof(*ins_eltp), KM_SLEEP);
	ins_eltp->fa_soffset = shmp->shm_off;
	ins_eltp->fa_eoffset = (shmp->shm_off + shmp->shm_nbytes) - 1;
	ins_eltp->fa_affinity = affinity;
	ins_eltp->fa_cgnum = cgnum;
	ins_eltp->fa_granularity = shmp->shm_granularity;
	LS_INIT(ins_eltp);


	FSPIN_LOCK(&fgap->fshm_aff_head.fh_lock);
	for (aff_eltp = (fgashm_affinity_t *)
					fgap->fshm_aff_head.fh_anchor.flink;
		aff_eltp != (fgashm_affinity_t *)
					&fgap->fshm_aff_head.fh_anchor.flink;
		    aff_eltp = nextp) {

		nextp = (fgashm_affinity_t *) aff_eltp->fa_links.ls_next;

		/*
		 * Break out if we've found the correct insertion point -
		 * before aff_eltp.  Continue if this entry is entirely
		 * before the range to be inserted.
		 */
		if (aff_eltp->fa_soffset > ins_eltp->fa_eoffset) {
			break;
		} else if (aff_eltp->fa_eoffset < ins_eltp->fa_soffset) {
			continue;
		}

		/*
		 * Old entry is either entirely contained within the new
		 * entry (remove the old entry) or the beginning of the
		 * new entry is within the new entry (adjust the beginning
		 * of the new entry appropriately).
		 */
		if ((aff_eltp->fa_soffset >= ins_eltp->fa_soffset) &&
				(aff_eltp->fa_soffset <= ins_eltp->fa_eoffset)) {
		    fgashm_bumpcnt(fgashm_reaff_case1);
		    if (aff_eltp->fa_eoffset <= ins_eltp->fa_eoffset) {
			    LS_REMOVE(aff_eltp);
			    FSPIN_UNLOCK(&fgap->fshm_aff_head.fh_lock);
			    kmem_free(aff_eltp, sizeof(*aff_eltp));
			    FSPIN_LOCK(&fgap->fshm_aff_head.fh_lock);
		    } else {
			    aff_eltp->fa_soffset = ins_eltp->fa_eoffset + 1;
		    }

	/*
	 * The ending of the old entry is within the new entry - adjust the end
	 * of the old entry appropriately.
	 */
		} else if ((aff_eltp->fa_eoffset <= ins_eltp->fa_eoffset) &&
				(aff_eltp->fa_eoffset >= ins_eltp->fa_soffset)) {
			fgashm_bumpcnt(fgashm_reaff_case2);
			aff_eltp->fa_eoffset = ins_eltp->fa_soffset - 1;
	/*
	 * The new entry is entirely contained within the old entry - split the
	 * old entry into two pieces, one on either side of the new entry.
	 *
	 * During the kmem_alloc(), allow others to access the list.
	 * Since only fgashm_reaffine() will modify the list,
	 * we don't need to recheck our predicate when we awaken.
	 */
		} else if ((ins_eltp->fa_soffset >= aff_eltp->fa_soffset) &&
				(ins_eltp->fa_eoffset <= aff_eltp->fa_eoffset)) {
			fgashm_bumpcnt(fgashm_reaff_case3);
			FSPIN_UNLOCK(&fgap->fshm_aff_head.fh_lock);
			splitp = kmem_alloc(sizeof(*splitp), KM_SLEEP);

			splitp->fa_soffset = ins_eltp->fa_eoffset + 1;
			splitp->fa_eoffset = aff_eltp->fa_eoffset;

			splitp->fa_affinity = aff_eltp->fa_affinity;
			splitp->fa_cgnum = aff_eltp->fa_cgnum;
			splitp->fa_granularity = aff_eltp->fa_granularity;
			LS_INIT(splitp);

			FSPIN_LOCK(&fgap->fshm_aff_head.fh_lock);

			aff_eltp->fa_eoffset = ins_eltp->fa_soffset - 1;

			LS_INS_AFTER(aff_eltp, splitp);
			nextp = splitp;
		}
	} /* for */
	LS_INS_BEFORE(aff_eltp, ins_eltp);
	FSPIN_UNLOCK(&fgap->fshm_aff_head.fh_lock);


	/*
	 * Can't aggressively migrate when using firstusage policy.
	 */
	if ((affinity == SHM_FIRSTUSAGE) &&
				!(shmp->shm_placepolicy & SHM_MIGRATE)) {
		goto fga_reaff_return;
	}

	/*
	 * For each page in the range, aggressively affine the page
	 *
	 *   if the page does not exist, then "fault" it in, using the specified
	 *   affinity (regardless of the setting of the migrate flag)
	 *
	 *   if the page exists and the migrate flag is set, then either migrate
	 *   the page (if the softlock count is zero) or mark the page for a
	 *   delayed migration (to take place when the softlock count becomes
	 *   zero)
	 *
	 * Construct a fake segfga data structure for use on the fgashm_fault()
	 * call.
	 */
	fake_sfd.sfd_vpgpp = fgap->fshm_vpgpp;
	fake_sfd.sfd_vpgsvp = &fgap->fshm_vpgsv;
	fake_sfd.sfd_vpglckp = &fgap->fshm_vpglck;
	fake_sfd.sfd_hatshptp = fgap->fshm_hatshptp;
	fake_sfd.sfd_affp = &fgap->fshm_aff_head;

	endoffset = (shmp->shm_off + shmp->shm_nbytes) - 1;
	pg_offset = btop(shmp->shm_off);

	for (offset = shmp->shm_off;
		offset < endoffset;
		    offset += PAGESIZE, pg_offset++) {

	    vpgp = &fgap->fshm_vpgpp[fshm_idx1(pg_offset)][fshm_idx2(pg_offset)];
	    FGASHM_LOCK_VPG(&fgap->fshm_vpglck, &fgap->fshm_vpgsv, vpgp);

	    if (vpgp->fvp_ppid == VPG_NONEXISTENT) {
		    if (affinity != SHM_FIRSTUSAGE) {
			    fgashm_fault(vpgp, offset, &fake_sfd);
		    }
	    } else if (shmp->shm_placepolicy & SHM_MIGRATE) {
		    if (vpgp->fvp_softlock_cnt == 0) {
			    shpt_hat_unload(NULL,
					    fgap->fshm_hatshptp,
					    offset);

			    hat_shootdown((TLBScookie_t) 0, HAT_NOCOOKIE);

			    /*
			     * We don't know that the FVP_DEL_MIG_FLAG
			     * is set, but it might be, and it will be
			     * harmless to clear it, since MIGRATE is being
			     * set.
			     */
			    vpgp->fvp_flag &= ~FVP_DEL_MIGRATE_FLAG;
			    vpgp->fvp_flag |= FVP_MIGRATE_FLAG;

			    if (affinity != SHM_FIRSTUSAGE) {
				    fgashm_fault(vpgp, offset, &fake_sfd);
			    }
		    } else {
			    vpgp->fvp_flag |= FVP_DEL_MIGRATE_FLAG;
			    fgashm_bumpcnt(fgashm_del_mig_count);
		    }
	    }
	    FGASHM_UNLOCK_VPG(&fgap->fshm_vpglck, &fgap->fshm_vpgsv, vpgp);
	} /* for */


fga_reaff_return:

	return(0);
}

/*
 * STATIC void
 * fgashm_lockmem(struct kshmid_ds *kshmp)
 *	Fault in an entire fine grained affinity segment.
 *
 * Calling/Exit State:
 *
 *	No locks are held on entry or exit.
 *	This function will acquire and release the VPG_LOCK() for each
 *	page in the segment.
 *
 * Remarks:
 */
void fgashm_lockmem(struct kshmid_ds *kshmp)
{
	struct fgashm_t *		fgap;
	struct segfga_data 		fake_sfd;
	size_t				offset;
	uint_t				pg_offset;
	fgavpg_t *			vpgp;

	/*
	 * For each page in the range, fault in the page.
	 *
	 * WITHOUT the lock, we make a quick check - if the page is
	 * NONEXISTENT or needs to be migrated, we grab the lock and call
	 * the fault routine (note that the situation could have changed by
	 * the time the lock is acquired and the fault routine called, but
	 * in that case the fault routine will end up being a nop).
	 *
	 * If the page exists and doesn't need to be migrated, then it must
	 * have a valid translation in the page tables, so no action needs to
	 * be taken.  Granted, a migrate request could invalidate the
	 * translation immediately after this check has been made, but such
	 * a race condition exists whether we grab the lock or not.
	 *
	 * Construct a fake segfga data structure for use on the fgashm_fault()
	 * call.
	 */
	fgap = kshmp->kshm_fgap;

	fake_sfd.sfd_vpgpp = fgap->fshm_vpgpp;
	fake_sfd.sfd_vpgsvp = &fgap->fshm_vpgsv;
	fake_sfd.sfd_vpglckp = &fgap->fshm_vpglck;
	fake_sfd.sfd_hatshptp = fgap->fshm_hatshptp;
	fake_sfd.sfd_affp = &fgap->fshm_aff_head;

	pg_offset = 0;

	for (offset = 0;
		offset < kshmp->kshm_mapsize;
		    offset += PAGESIZE, pg_offset++) {

	    vpgp = &fgap->fshm_vpgpp[fshm_idx1(pg_offset)][fshm_idx2(pg_offset)];

	    if ((vpgp->fvp_ppid == VPG_NONEXISTENT) ||
				(vpgp->fvp_flag & FVP_MIGRATE_FLAG)) {

		    FGASHM_LOCK_VPG(&fgap->fshm_vpglck, &fgap->fshm_vpgsv, vpgp);
		    fgashm_fault(vpgp, offset, &fake_sfd);
		    FGASHM_UNLOCK_VPG(&fgap->fshm_vpglck, &fgap->fshm_vpgsv, vpgp);
	    }
	} /* for */

	return;
} 

/*
 * STATIC void
 * fgashm_fault(fgavpg_t *, size_t vpg_offset, struct segfga_data *sfdp)
 *	Fault in one page in a segfga segment.
 *
 * Calling/Exit State:
 *	The vpg lock is held (via FGASHM_LOCK_VPG()) when this function is
 *	called and is held when this function exits (and this function never
 *	releases the lock).
 *
 *	This function does not block.
 *
 * Remarks:
 *
 *	This function can be called both by the segfga driver and by
 *	a shmctl(SHM_SETPLACE) call.
 */
void fgashm_fault(fgavpg_t *vpgp, size_t vpg_offset,
		  struct segfga_data *sfdp)
{
	cgnum_t	place_cg;
	ppid_t	new_ppid;


	/*
	 * Check the state of the page.  If the fvp_ppid field is
	 * VPG_NONEXISTENT, then the page has never physically existed and
	 * the translation is invalid.  Get the page and install the
	 * translation.
	 *
	 * If fvp_ppid is not VPG_NONEXISTENT, then the page has existed.
	 * A fault is still possible if
	 * 	a) the translation was invalidated due to a shmctl(SHM_SETPLACE)
	 *	   migrate request.
	 *
	 *	b) two LWPs faulted simultaneously, and we are the loser of the
	 *	   race for the VPG lock (or we are a SOFTLOCK on a resident
	 * 	   segfga page).
	 *
 	 * If the FVP_MIGRATE_FLAG is set, we are case a).  If not, we are case
	 * b), and nothing needs to be done.
	 */
	if (vpgp->fvp_ppid == VPG_NONEXISTENT) {

		ASSERT((vpgp->fvp_softlock_cnt == 0) &&
				!(vpgp->fvp_flag & FVP_DEL_MIGRATE_FLAG));

		place_cg = fgashm_find_cg(sfdp->sfd_affp, vpg_offset);

		vpgp->fvp_ppid = idf_page_get(place_cg, PAGESIZE);
		pzero(vpgp->fvp_ppid);
		shpt_hat_load(NULL, sfdp->sfd_hatshptp, 0,
				vpg_offset, vpgp->fvp_ppid,
				PROT_USER | PROT_READ | PROT_WRITE);

	} else if (vpgp->fvp_flag & FVP_MIGRATE_FLAG) {

		ASSERT((vpgp->fvp_softlock_cnt == 0) &&
				!(vpgp->fvp_flag & FVP_DEL_MIGRATE_FLAG));

		vpgp->fvp_flag &= ~FVP_MIGRATE_FLAG;

		place_cg = fgashm_find_cg(sfdp->sfd_affp, vpg_offset);

		if (idf_resv(1, NOSLEEP)) {
			new_ppid = idf_page_get(place_cg, PAGESIZE);
			if (idf_place(new_ppid) == place_cg) {
				pcopy(vpgp->fvp_ppid, new_ppid);
				idf_page_free(vpgp->fvp_ppid, PAGESIZE);
				vpgp->fvp_ppid = new_ppid;

				fgashm_bumpcnt(fgashm_mig_count);
			} else {
				idf_page_free(new_ppid, PAGESIZE);
			}

			idf_unresv(1);
		}

/*
 * The loading of the translation here is necessary, regardless of whether
 * or not the page was actually migrated.  The FVP_MIGRATE_FLAG will only
 * be set on a page after the translation has been invalidated (to force
 * a fault, and thus the migration).  Therefore, in order for this routine
 * to complete correctly, it must load a translation, even if the physical
 * location of the page has not changed (in which this routine will simply
 * reload the translation which was originally loaded).
 */
		shpt_hat_load(NULL, sfdp->sfd_hatshptp, 0,
				vpg_offset, vpgp->fvp_ppid,
				PROT_USER | PROT_READ | PROT_WRITE);
	} /* else if MIGRATE */

	return;
}


/*
 * STATIC cgnum_t
 * fgashm_find_cg(fgashm_aff_head_t *, size_t offset)
 *	Find the cg where the specified page of the FGA shm segment
 *	should be placed.  offset is a byte offset into the shared memory
 *	segment.
 *
 * Calling/Exit State:
 *
 *	No locks are required to be held on entry, though the VPG lock
 *	will often be.  This routine will acquire and release a fast
 *	spin lock.
 *
 * Remarks:
 *
 */
STATIC
cgnum_t fgashm_find_cg(fgashm_aff_head_t *affp, size_t offset)
{
	fgashm_affinity_t *	aff_eltp;
	int				save_aff;
	cgnum_t				save_cg;
	uint_t				save_gran;
	cgnum_t				place_cg;


	/*
	 * Set up to use the default affinity.  If an entry appears in
	 * the list which contains the offset in question, the default
	 * affinity will be overridden.
	 */
	save_aff = affp->fh_def_aff;
	save_cg = affp->fh_def_cgnum;
	save_gran = affp->fh_def_granularity;

	FSPIN_LOCK(&affp->fh_lock);
	for (aff_eltp = (fgashm_affinity_t *) affp->fh_anchor.flink;
		aff_eltp != (fgashm_affinity_t *) &affp->fh_anchor.flink;
		    aff_eltp = (fgashm_affinity_t *)
						aff_eltp->fa_links.ls_next) {

		/*
		 * Since the list is sorted by offset, we can exit early
		 * if we've already past the point where the offset in
		 * question would appear.
		 */
		if (offset < aff_eltp->fa_soffset) {
			break;
		}

		if ((aff_eltp->fa_soffset <= offset) &&
					(offset <= aff_eltp->fa_eoffset)) {
			save_aff = aff_eltp->fa_affinity;
			save_cg = aff_eltp->fa_cgnum;
			save_gran = aff_eltp->fa_granularity;

			break;
		}

	} /* for */
	FSPIN_UNLOCK(&affp->fh_lock);

	/*
	 * Now that we have the affinity, apply the policy and return
	 * the cg where the page should be placed.
	 *
	 * What should SHM_PLC_DEFAULT do???
	 */
	switch (save_aff) {
		case SHM_BALANCED:
		case SHM_PLC_DEFAULT:
			ASSERT(save_gran != 0);

			/*
			 * Find an online cg where the page should be placed.
			 * This won't ensure that all cgs are hit with
			 * equal probability if there are holes in the cg
			 * numbering - anyone have a better algorithm?
			 */
			place_cg = ((offset / save_gran) % Ncg);
			while (!IsCGOnline(place_cg)) {
				place_cg = (place_cg + 1) % Ncg;
			}

			return(place_cg);
			break;

		case SHM_CPUGROUP:
			return(save_cg);
			break;

		case SHM_FIRSTUSAGE:
			return(mycg);
			break;

		default:
			/*
			 *+ An unknown affinity policy was used when placing
			 *+ shared memory pages.  This is a kernel internal
			 *+ software error.
			 */
			cmn_err(CE_PANIC, "Unknown affinity in fgashm_find_cg.");
			break;
	} /* switch */

} /* fgashm_find_cg */

/*
 * STATIC void
 * fgashm_destroy_affinity(fgashm_aff_head_t *head_affp)
 *	Remove all the affinity information on the passed in linked list.
 *
 * Calling/Exit State:
 *
 *	No locks are required to be held on entry.
 *	This routine will acquire and release a fast spin lock.
 *
 * Remarks:
 *
 */
STATIC
void fgashm_destroy_affinity(fgashm_aff_head_t *head_affp)
{
	fgashm_affinity_t *		affp;
	list_t				tmp_queue;

	/*
	 * Because kmem_free() cannot be called while holding a fast spin
	 * lock, and because the affinity queue is protected by a fast spin
	 * lock, we remove all entries from the affinity onto a temporary
	 * and private queue, under protection of the lock, then release the
	 * lock and destroy the entries on the private queue.
	 *
 	 * Alternatively, this could have been coded as remove an entry
	 * from the queue, release the lock, call kmem_free(), reacquire
	 * the lock, remove the next entry, etc.  This approach was not
	 * chosen because of the extra lock acquires and releases it would
	 * necessitate.
	 */
	INITQUE(&tmp_queue);
	FSPIN_LOCK(&head_affp->fh_lock);
	while ((affp = (fgashm_affinity_t *)
				LS_REMQUE(&head_affp->fh_anchor)) != NULL) {
		LS_INSQUE(&tmp_queue, affp);
	}
	FSPIN_UNLOCK(&head_affp->fh_lock);

	while ((affp = (fgashm_affinity_t *)
				LS_REMQUE(&tmp_queue)) != NULL) {
		kmem_free(affp, sizeof(*affp));
	}

	return;
}
#endif /* PAE_MODE */

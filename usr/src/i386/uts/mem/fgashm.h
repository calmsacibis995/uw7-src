#ifndef _MEM_FGASHM_H	/* wrapper symbol for kernel use */
#define _MEM_FGASHM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/fgashm.h	1.1.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

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
 * 
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

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h> /* REQUIRED */
#include <util/list.h> /* REQUIRED */
#include <util/param.h>	/* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h> /* REQUIRED */
#include <sys/list.h> /* REQUIRED */
#include <sys/param.h>	/* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */


/*
 * Implementation constants.  Used in the fgavpg_t flag field.
 *
 * For a particular VPG, the fvp_ppid, the softlock_count, and the fvp_flag
 * field will work together to determine if a migration is necessary for
 * the page.  The following combinations are possible:
 *
 *	fvp_ppid == VPG_NONEXISTENT
 *		implies that the page has never been physically backed
 *		and, therefore, that no translation exists for it.  Neither
 *		FVP_DEL_MIG_FLAG or FVP_MIGRATE_FLAG can be set.  softlock
 *		count must be zero.
 *	FVP_DEL_MIG_FLAG set, softlock_cnt > 0
 *		implies that the translation is valid and will remain so.
 *		page will be migrated after the softlock count has gone to
 *		0.
 *	FVP_DEL_MIG_FLAG set, softlock_cnt == 0
 *		implies that the translation is valid.  The page
 *		will be migrated when the next time that segfga_fault() is
 *		called for the page (i.e. regular fault or F_SOFTLOCK).
 *	FVP_MIGRATE_FLAG set
 *		implies that the translation is invalid and that no softlocks
 *		are held.  Page can either be aggressively migrated immediately
 *		or on next fault (either regular fault or F_SOFTLOCK).
 * 
 * Only one of FVP_MIGRATE_FLAG or FVP_DEL_MIG_FLAG can be set for any one
 * VPG at any one time.
 */
#define FVP_LOCK_FLAG		0x1	/* page is locked */
#define FVP_MIGRATE_FLAG	0x2	/* page to migrate on next fault */
#define FVP_DEL_MIGRATE_FLAG	0x4	/* mark as migratable when softlock count is 0 */

/*
 * Constants used when indexing in the fga shm virtual page (vpg) array.
 * These macros are for manipulating page offsets within the segfga segment,
 * NOT byte offsets.
 *
 * The sizes here were chosen to avoid > 64K allocations from kmem_alloc()
 * when creating a FGA SHM segment.  If changing these sizes, then the
 * allocations in shm_makefga() should be examined to ensure that they
 * are not too large.
 */
#define FGA_VPG_TO_IDX1_SHIFT		10
#define FGA_IDX1_MASK			0x3ff		/* 10 bits */
#define FGA_IDX2_MASK			0x3ff		/* 10 bits */
#define FGA_LEVEL2_SZ			0x400

#define fshm_idx1(vpg_num)		(((vpg_num) >> FGA_VPG_TO_IDX1_SHIFT) \
								& FGA_IDX1_MASK)

#define fshm_idx2(vpg_num)		((vpg_num) & FGA_IDX2_MASK)

/*
 * Constants used when waiting for events.  Chosen arbitrarily, based
 * on segvn wait priority.
 */
#define FGA_VPG_PRI   (PRIMEM - 1) /* pri of LWPs blocked on page locks */



/*
 * Structure Definitions.
 */

/*
 * There is one of these structures for each virtual page in the fga
 * shm segment.
 *
 * It is important to note that the softlock count can increase only when
 * the VPG lock is held.  Thus, if the VPG lock is held and the softlock count
 * is zero, the holder of the VPG lock is guaranteed that the softlock count
 * will remain zero and that it is safe to invalidate the translation and
 * perform a migration.
 */
typedef struct {
	uchar_t		fvp_flag;	/* useful flags - see #defines above */
	uchar_t		fvp_contended;	/* LWP awaiting lock on this VPG */
	uint_t		fvp_softlock_cnt;/* # of current softlocks on page */
	ppid_t		fvp_ppid;	/* phys addr where page is now, VPG_NONEXISTANT
					 * if non resident
					 */
} fgavpg_t;

#define VPG_NONEXISTENT		((ppid_t) -1)
					/* Mark a page in the fga shm segment
					 * as not yet existing.
					 */

#define VPG_MAX_SOFTLOCK_COUNT	(UINT_MAX)	/* Maximum softlock count */

/*
 * These are elements of a linked list which describes affinity for
 * different ranges of the fga shm.  Each element in the linked list
 * describes the affinity for one range where the affinity is identical
 * across all the pages in the segment.  fa_soffset and fa_eoffset are
 * in bytes.
 */
typedef struct {
	ls_elt_t	fa_links;
	size_t		fa_soffset;		/* start of range */
	size_t		fa_eoffset;		/* end of range */
	int		fa_affinity;		/* affinity policy in effect */
	cgnum_t		fa_cgnum;		/* cg for SHM_CPUGROUP */
	uint_t		fa_granularity;		/* for SHM_BALANCED */
} fgashm_affinity_t;

/*
 * This structure is used to manage the linked list and contains both
 * locks for accessing the linked list as well as the anchor for the list.
 */
typedef struct {
	list_t		fh_anchor;
	fspin_t		fh_lock;		/* linked list lock */
	int		fh_def_aff;		/* segment default affinity policy */
	cgnum_t		fh_def_cgnum;		/* used when SHM_CPUGROUP default affinity */
	uint_t		fh_def_granularity;	/* when SHM_BALANCED default affinity */
} fgashm_aff_head_t;


/*
 * There is one of these structures for each fga shm segment in the
 * system.
 *
 * The fshm_vpgpp is managed as a two dimensional array in order to
 * avoid kmem_alloc()s of > 64K.
 */
struct fgashm_t {
	fgavpg_t **		fshm_vpgpp;		/* array for pages in segment */
	hatshpt_t *		fshm_hatshptp;		/* shared page tables */
	lock_t			fshm_vpglck;		/* lock for pages in array */
	sv_t			fshm_vpgsv;		/* sv for page lockers */
	fgashm_aff_head_t	fshm_aff_head;		/* segment affinity info */
} ;



#ifdef _KERNEL


/*
 * Macros to lock and unlock a virtual page in a fga shm segment.
 * lockp is a pointer to the lock field in the fgashm_t structure.
 * svp is a pointer to the sv field in the fgashm_t structure.
 * vpgp is a pointer to the fgavpg_t structure to be locked.
 */

/*
 * Lock the virtual page of the fga segment.  Holding the page lock here
 * allows the holder to establish the physical identity of the page and
 * prevents anyone else from affecting the identity of the page.
 *
 * There is one page lock per virtual page in the fga segment.  All page
 * locks use one synchronization variable for the rare case when more than
 * one thread of control attempts to acquire the same lock.
 */
#define FGASHM_LOCK_VPG(lockp, svp, vpgp)				\
    {									\
	pl_t	opl;							\
									\
	opl = LOCK((lockp), VM_FGA_IPL);				\
	while ((vpgp)->fvp_flag & FVP_LOCK_FLAG) {			\
	    (vpgp)->fvp_contended = 1;					\
	    SV_WAIT((svp), FGA_VPG_PRI, (lockp));			\
	    opl = LOCK((lockp), VM_FGA_IPL);				\
	}								\
	(vpgp)->fvp_flag |= FVP_LOCK_FLAG;				\
	UNLOCK((lockp), opl);						\
    }

/*
 * Unlock the virtual page in the fga segment.  Only do a SV_BROADCAST()
 * if another thread of control is waiting for the lock on THIS page.
 */
#define FGASHM_UNLOCK_VPG(lockp, svp, vpgp)				\
    {									\
	pl_t	opl;							\
									\
	opl = LOCK((lockp), VM_FGA_IPL);				\
	(vpgp)->fvp_flag &= ~FVP_LOCK_FLAG;				\
	if ((vpgp)->fvp_contended) {					\
	    (vpgp)->fvp_contended = 0;					\
	    SV_BROADCAST((svp), 0);					\
	}								\
	UNLOCK((lockp), opl);						\
    }



struct kshmid_ds;
struct shmid_ds;

/*
 * Interface functions for fgashm.
 */
extern int	shm_makefga(struct kshmid_ds *kshmp, size_t nbytes, int flag);
extern void	shm_rm_fga(struct kshmid_ds *kshmp);
extern int	fgashm_reaffine(struct kshmid_ds *kshmp, struct shmid_ds *shmp);
extern void	fgashm_lockmem(struct kshmid_ds *kshmp);

/*
 * Helper functions for use by segfga and fgashm.
 */
struct segfga_data;

void fgashm_destroy_affinity(fgashm_aff_head_t *head_affp);
void fgashm_fault(fgavpg_t *vpgp, size_t vpg_offset,
			struct segfga_data *sfdp);
cgnum_t fgashm_find_cg(fgashm_aff_head_t *affp, size_t offset);


/*
 * Constants used for size and alignment of the fga shm.
 */
#define FGASHM_ALIGN		0x400000		/* 4 Megabytes */
#define FGASHM_SIZE_QUANTA	0x400000		/* 4 Megabytes */

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_FGASHM_H */

#ident	"@(#)kern-i386:proc/ipc/dshm.c	1.3.2.1"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */


#include <acc/audit/audit.h>
#include <acc/dac/acl.h>
#include <acc/priv/privilege.h>
#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/seg.h>
#include <mem/seg_umap.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/ipcsec.h>
#include <proc/ipc/shm.h>
#include <proc/ipc/dshm.h>
#include <proc/mman.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <acc/mac/mac.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

/*
 * Management of Dynamic Shared Memory Objects:
 *
 *	- There will be a maximum of dshminfo.dshmmni dynamic shared memory
 *	  objects configured at boot time.  Dshminfo is a tunable parameter.
 *
 *	- Each dynamic shared memory object will be represented by a 
 *	  kdshmid_ds structure.  An array of dshminfo.dshmmni kdshmid_ds 
 *	  structures will be allocated at boot time.
 *
 *	- Any object currently exists, will be attached to the global
 *	  dynamic shared memory directory dshmdir.  See the state table below.
 *
 *	- p_segacct will be protected by the address space writer lock of the
 *	  process.
 *
 *	- p_nshmseg will not be protected by any lock when its users are
 *	  proc_setup, remove_proc, and exit. They will make sure that they are
 *	  the only running context before using the field.
 *	  For other users, the field will be protected by the address space
 *	  writer lock of the process since we call either as_map or as_unmap
 *	  immediately after changing its value.  This will save us an
 *	  extra lock round-trip.
 */

STATIC int	dshmseqmax;	/* max ipcd_seq value we can use */
ipcdir_t	dshmdir;	/* dynamic shared memory ipc directory entry */
STATIC lock_t	dshmdir_lck;	/* protecting dshmdir manipulation,
				 * dshmdir.nactive, and the array
				 * dshmdir.ipcdir_entries[].
				 *
				 * Locking is not necessary for all accesses
				 * to dshmdir.nents and dshmdir.ipcdir_entries
				 * since these two fields are modified only in
				 * dshm_init during booting.
				 */
STATIC LKINFO_DECL(dshmdir_lks, "MS::dshmdir_lck", 0);

STATIC struct kdshmid_ds *kdshmids;	/* head of the kshmid_ds[] array
					 * Each cell, including struct as
					 * pointed to by kshm_asp,
					 * is protected by its own kshm_lck.
					 */
STATIC LKINFO_DECL(kdshmid_lks, "MS::kdshm_lck", 0);
/*
 * Locking order:
 *
 *	dshmdir_lck:
 *		kdshm_lck:
 */

/*
 * ipc directory entry and its lock must be accessible to DAC code
 *
 * Macros to protect dshmdir manipulation
 */
#define DSHMDIR_LOCK()		LOCK_PLMIN(&dshmdir_lck)
#define DSHMDIR_UNLOCK(pl)	UNLOCK_PLMIN(&dshmdir_lck, (pl))

/*
 * States of a dynamic shared memory object:
 *
 *	ipcd_ent	kdshm_vpage		Remarks
 *	========	========	======================================
 *	   0		   NULL		non-existent DSHM object
 *	 > 0		 non-NULL	the object exists and # of current
 *					users == kdshm_refcnt - 1
 *	 > 0		   NULL		in the middle of creation; not useable
 *	   0		 non-NULL	The object has been removed but there
 *					still some user(s) out there.  The
 *					object will be destroyed when the last
 *					user checks out.
 */
extern struct shminfo	shminfo;
extern struct dshminfo	dshminfo;	/* dynamic shared memory tunables info
					 * Since nobody will change this,
					 * accesses to this structure will
					 * not require locking.
					 */
/* Arguments for dshmsys() */
struct dshmsysa {
	int		opcode;
};

struct dshmgeta {
	int opcode;
	key_t key;
 	size_t buffer_size;
	ulong_t total_buffer_count;
	ulong_t appl_buffer_count;
        void * dshmaddr;
	size_t map_size; 
	int dshmflg;
};

struct dshmata {
	int	opcode;
	int	dshmid;
	int	dshmflg;
};

struct dshmctla {
	int		opcode;
	int		dshmid;
	int		cmd;
	struct dshmid_ds *arg;
};

struct dshmdta {
	int		opcode;
	caddr_t		addr;
};

struct dshmaligna {
	int		opcode;
};

struct dshmremapa {
	int		opcode;
	int 		dshmid;
	caddr_t 	uvaddr;
	ulong_t		buffer_index;
};

/*
 * STATICS
 */
STATIC int	dshmalloc(ipc_perm_t **);
STATIC void	_dshmdealloc(ipc_perm_t *);
STATIC void	dshmdealloc(struct kdshmid_ds *);
STATIC void	dshm_remove_id(const int);
STATIC int	dshmget(struct dshmgeta *, rval_t *);
STATIC int	dshmat(struct dshmata *, rval_t *);
STATIC int	dshmdt(struct dshmdta *, rval_t *);
STATIC int	dshmctl(struct dshmctla *, rval_t *);
STATIC int	dshmmmap(struct as *, vaddr_t, size_t, uchar_t,
			 struct kdshmid_ds *, int );
STATIC int	dshmkalignment(struct dshmaligna *uap, rval_t *);
STATIC int	dshmaddr(struct kdshmid_ds *, vaddr_t *, uint_t *);

/*
 * structures for ipcget interfaces
 */

STATIC ipcops_t		dshmops = {dshmalloc, _dshmdealloc};
STATIC ipcdata_t        dshmdata = {&dshmdir, &dshmops};/* 3rd parameter passed
							 * to ipcget.
							 */
/*
 * void
 * dshminit(void)
 * 	called by main (main.c) to initialize dynamic shared memory IPC.
 *
 * Calling/Exit State:
 *	KMA must be running when it is called.
 */

void
dshminit(void)
{
	int	i, size;
	const int n = dshminfo.dshmmni;
	struct kdshmid_ds *mp;
	struct ipc_sec *sp;	/* ptr to sec structs */

	LOCK_INIT(&dshmdir_lck, DSHMDIR_HIER, PLMIN, &dshmdir_lks, KM_NOSLEEP);

	/* allocate and initialize dynamic shared memory IPC directory */

	if ((dshmdir.ipcdir_nents = n) == 0) {
		/*
		 *+ The system configuration parameter dshminfo.dshmmni
		 *+ should be checked and set to a non-zero value to
		 *+ enable dynamic shared memory.
		 */
		cmn_err(CE_WARN,
		"dshminit: dshminfo.dshmmni is zero, DSHM disabled");
		return;
	}

	dshmdir.ipcdir_nactive = 0;

	size = n * (sizeof(struct ipcdirent) + sizeof(struct kdshmid_ds) +
		    sizeof(struct ipc_sec));

	dshmdir.ipcdir_entries = (ipcdirent_t *)kmem_zalloc(size, KM_NOSLEEP);

	if (dshmdir.ipcdir_entries == NULL) {
		/*
		 *+ Could not allocate memory for the SystemV
		 *+ dynamic shared memory IPC directory.
		 *+ Instead of PANIC'ing the system, this IPC mechanism
		 *+ is disabled and a warning message is printed.
		 *+ The system configuration parameter dshminfo.dshmmni
		 *+ should be checked to make sure that it is not
		 *+ inordinately large.
		 */
		cmn_err(CE_WARN,
		"dshminit: Can't allocate IPC directory, DSHM IPC disabled");
		/*
		 * Setting ipcdir_nents to zero will cause
		 * ipcget() to always fail with ENOSPC.
		 */
		dshmdir.ipcdir_nents = 0;
		return;
	}
	
	kdshmids = (struct kdshmid_ds *)(dshmdir.ipcdir_entries + n);

	sp = (struct ipc_sec *)(kdshmids + n);
	/*
	 * initialize kdshmids[] DAC entries, locks and sv
	 * in dynamic shared memory segments.
	 */
	for (i = n, mp = kdshmids; i > 0; i--, mp++, sp++) {
		mp->kdshm_ds.dshm_perm.ipc_secp = sp;
		LOCK_INIT(&mp->kdshm_lck, DSHMID_HIER, PLMIN,
			  &kdshmid_lks, KM_NOSLEEP);
		SV_INIT(&mp->kdshm_sv);
	}

	dshmseqmax = INT_MAX;
}

/*
 * int
 * dshmalloc(ipc_perm_t **ipcpp)
 *      Allocate and partially initialize a free dshmid_ds data
 *      structure from dshmids[].  A pointer to the encapsulated ipc_perm
 *      structure is returned via the out argument 'ipcpp'.
 *
 * Calling/Exit State:
 *      It should be called with dshmdir_lck held and return
 *	it held.  It will acquire kdshm_lck and drop the lock
 *	for eack kdshmids[] entry .  It will return the address of a free
 *	kdshmids[] in *ipcpp, if found.  Otherwise, returns NULL in *ipcpp.
 *	Since kdshmids[] are statically allocated, this routine will not
 *	block.  Conesquently, this function will always return 0.
 *	
 */
STATIC int
dshmalloc(ipc_perm_t **ipcpp)
{
	const int n = dshminfo.dshmmni;
	int i;
	struct kdshmid_ds *idp, *freep;
	pl_t opl;
	struct ipc_sec	*ipc_secp;

	ASSERT(LOCK_OWNED(&dshmdir_lck));

	/*
	 * look through kdshmids[] array for the first free entry:
	 *	- mark the entry IPC_ALLOC since ipcget will make the entry
	 *	  visible to the public.
	 *	- mark the entry DSHM_BUSY to block others since the
	 *	  segment setup procedure is not yet completed
 	 */

	freep = (struct kdshmid_ds *)NULL;
	for (i = 0, idp = kdshmids; i < n; i++, idp++) {
		opl = DSHMID_LOCK(idp);
		if ((idp->kdshm_ds.dshm_perm.mode & IPC_ALLOC) == 0
		    && (idp->kdshm_flag & DSHM_BUSY) == 0) {

			idp->kdshm_ds.dshm_perm.mode |= IPC_ALLOC;	
			idp->kdshm_flag |= DSHM_BUSY;

			ipc_secp = idp->kdshm_ds.dshm_perm.ipc_secp;
			struct_zero(ipc_secp, sizeof(struct ipc_sec));

			freep= idp;
        		DSHMID_UNLOCK(idp, opl);
			break;
		}
        	DSHMID_UNLOCK(idp, opl);
	}

	/*
	 * Since a kdshmid_ds encapsulates a dshmid_ds at the top, and the
	 * dshmid_ds in turn encapsulates an ipc_perm at the top, freep
	 * will effectively points to an ipc_perm.  If this relationship
	 * is changed in the future, a macro such as DSHMID_TO_IPC will
	 * be required.
	 */

	*ipcpp = (ipc_perm_t *)freep;
	return(0);
}

/* 
 * void
 * _dshmdealloc(ipc_perm_t *ipcp)
 *	Basically a wrapper for ipc.c level operations
 * 	acquire DSHMID lock and call dshmdealloc
 *	Wake any waiters & drop the DSHMID_LOCK
 *
 * Calling/Exit State:
 *	DSHMID_DIR LOCK held on entry returns the same way
 *	takes and releases the DSHMID_LOCK
 *	DSHM_BUSY in kdshm_flag owned; we need to take the
 *	DSHMID lock because dshmdealloc clears DSHM_BUSY
 */
STATIC void
_dshmdealloc(ipc_perm_t *ipcp)
{
	struct kdshmid_ds *kdshmp = (struct kdshmid_ds *)ipcp;
	pl_t opl;

	ASSERT(ipcp != NULL);
	ASSERT(kdshmp->kdshm_refcnt == 0);
	ASSERT(LOCK_OWNED(&dshmdir_lck));
	ASSERT(!LOCK_OWNED(&kdshmp->kdshm_lck));
	ASSERT(kdshmp->kdshm_flag & DSHM_BUSY);
	
	opl = DSHMID_LOCK(kdshmp);
	dshmdealloc(kdshmp);
	DSHMID_UNLOCK(kdshmp, opl);	
	if (SV_BLKD(&kdshmp->kdshm_sv))
		SV_BROADCAST(&kdshmp->kdshm_sv, 0);
}

/* 
 * void
 * dshmdealloc(struct kdshmid_ds *kdshmp)
 *	Free the passed in kdshmid_ds data structure.
 *
 * Calling/Exit State:
 *      The caller makes sure that we have exclusive access to
 *	the kdshmid_ds and its icp_perm prior to calling this function by
 * 	owning DSHM_BUSY on its kdshm_flag.
 *	On return, the kdshmid_ds and its icp_perm are freed.
 */
STATIC void
dshmdealloc(struct kdshmid_ds *kdshmp)
{
	ASSERT(kdshmp != NULL);
	ASSERT(kdshmp->kdshm_refcnt == 0);
	ASSERT(kdshmp->kdshm_flag & DSHM_BUSY);
	ASSERT(LOCK_OWNED(&kdshmp->kdshm_lck));
	
        /*
         * mac_rele is not called within this function because
         * this function is sometimes called with respect to
         * a segment for which there was not a prior mac_hold.
         * Instead, mac_rele is called selectively,
         * by functions which are dellocating segments for which
         * a mac_hold was done when the segments were established.
         *
         */

	kdshmp->kdshm_flag = 0;
	kdshmp->kdshm_vpage = NULL;
	kdshmp->kdshm_hatp= NULL;
	kdshmp->kdshm_pfn = NULL;
	kdshmp->kdshm_ds.dshm_perm.mode &= ~IPC_ALLOC;
	return;
}

/*
 * void
 * dshm_remove_id(const int dshmid)
 *	remove the specified dynamic shared memory segment from dshmdir.
 *
 * Calling/Exit State:
 *	The caller should be called with dshmdir_lck held and return the lock
 *	held.  The routine will not block.
 */
STATIC void
dshm_remove_id(const int dshmid)
{
	struct ipcdirent *direntp;

	ASSERT(LOCK_OWNED(&dshmdir_lck));

	/* remove the kdshmid_ds[] from dshmdir */

	ASSERT(dshmid >= 0 && dshmdir.ipcdir_nents >
	       (dshmid % dshminfo.dshmmni));
	ASSERT(dshmdir.ipcdir_nactive > 0);

	direntp = dshmdir.ipcdir_entries + (dshmid % dshminfo.dshmmni);

	dshmdir.ipcdir_nactive--;
	direntp->ipcd_ent = NULL;	/* make it invisible to the public */

	if (++direntp->ipcd_seq > dshmseqmax)
		direntp->ipcd_seq = 0;
}

/*
 * int
 * dshmconv(int, struct kdshmid_ds **)
 *	Convert user supplied dshm_id into a ptr to its corresponding
 *	kdshmid_ds[] entry in kdshmpp.
 *
 * Calling/Exit State:
 *	- This routine may block and should be called at PLBASE without
 *	  holding any lock.
 *	- This routine will garner dshmdir_lck and the kdshm_lck of the
 *	  kdshmid_ds[] entry and release them before returning to its caller. 
 *	- Return 0 on success and mark the dshm header DSHM_BUSY; otherwise,
 *	  return non-zero.
 *
 * Remarks:
 * 	This function cannot be static because it is called in ipcdac.c.
 */

int
dshmconv(int dshm_id, struct kdshmid_ds **kdshmpp)
{
	struct kdshmid_ds *kdshmp;	/* ptr to associated header */
	ipcdirent_t *direntp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	*kdshmpp = (struct kdshmid_ds *)NULL;

	if (dshm_id < 0)
		return EINVAL;

	DSHMDIR_LOCK();
	direntp = dshmdir.ipcdir_entries + (dshm_id % dshminfo.dshmmni);
	ASSERT(direntp != NULL);

	kdshmp = (struct kdshmid_ds *)direntp->ipcd_ent;

	if (kdshmp == NULL) {
		DSHMDIR_UNLOCK(PLBASE);
		return EINVAL;
	}
	DSHMDIR_UNLOCK(PLBASE);/* drop it; we might block for DSHM_BUSY */

	DSHMID_LOCK(kdshmp);
	while (kdshmp->kdshm_flag & DSHM_BUSY) {
		SV_WAIT(&kdshmp->kdshm_sv, DSHM_PRI, &kdshmp->kdshm_lck);
		DSHMID_LOCK(kdshmp);
	}

	/*
	 * hold DSHM_BUSY so that nobody can take it away
	 */

	kdshmp->kdshm_flag |= DSHM_BUSY;
	DSHMID_UNLOCK(kdshmp, PLBASE);

	/*
	 * the object may be gone or replaced while waiting for DSHM_BUSY
	 */

	DSHMDIR_LOCK();
	if (direntp->ipcd_ent != (ipc_perm_t *)kdshmp) {
		/*
		 * We lost the race to IPC_RMID, or the underlying kdshmid_ds[]
		 * has been changed while we were waiting for it.
		 */
		DSHMID_LOCK(kdshmp);
		kdshmp->kdshm_flag &= ~DSHM_BUSY;
		DSHMID_UNLOCK(kdshmp, PLMIN);
		DSHMDIR_UNLOCK(PLBASE);

		if (SV_BLKD(&kdshmp->kdshm_sv))
                	SV_BROADCAST(&kdshmp->kdshm_sv, 0);

		return EINVAL;
	}

	if (!(kdshmp->kdshm_ds.dshm_perm.mode & IPC_ALLOC)  
	  || dshm_id / dshminfo.dshmmni != direntp->ipcd_seq) {

		DSHMID_LOCK(kdshmp);
		kdshmp->kdshm_flag &= ~DSHM_BUSY;
		DSHMID_UNLOCK(kdshmp, PLMIN);
		DSHMDIR_UNLOCK(PLBASE);

		if (SV_BLKD(&kdshmp->kdshm_sv))
			SV_BROADCAST(&kdshmp->kdshm_sv, 0);

		return EINVAL;
	}
	DSHMDIR_UNLOCK(PLBASE);
	*kdshmpp = kdshmp;
	return 0;
}



/*
 * int
 * dshmget(struct dshmgeta *,  rval_t *)
 *	create a new dshm IPC. system call dshmget(2)
 *
 * Calling/Exit State:
 *	This routine may block and should be called at PLBASE without holding
 *	any lock.
 *
 * Remarks:
 *	- dynamic shared memory identifier =
 *	  current ipcd_seq * dshminfo.dshmmni + slot number
 *	  of the dshmdir.ipcdir_entries[] entry
 */
STATIC int
dshmget(struct dshmgeta *uap, rval_t *rvp)
{
	struct ipcdirent *direntp;	/* dshmdir[] entry returned by ipcget.
					 * It can be new or existing.
					 */
	boolean_t is_new = B_FALSE;	/* indicate whether a new
					 * kdshmid_ds and dshmdir
					 * is allocated by ipcget.
			 		 */
	int error = 0;
	struct kdshmid_ds *kdshmp;
	struct dshmid_ds *dsp;
	memsize_t size;
	lid_t lid;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

        /* Check for appropriate privileges */
        if (pm_denied(CRED(), P_PLOCK)){
		return EPERM;
	}
	
retry:
	DSHMDIR_LOCK();
	
	if (error = ipcget(uap->key, uap->dshmflg, &dshmdata,
	                   &is_new, &direntp)) {
		DSHMDIR_UNLOCK(PLBASE);
		return error;
	}

	ASSERT(direntp != NULL);
	kdshmp = (struct kdshmid_ds *)direntp->ipcd_ent;
	ASSERT(kdshmp != NULL);
	dsp = &kdshmp->kdshm_ds;
	lid = kdshmp->kdshm_ds.dshm_perm.ipc_secp->ipc_lid;
	DSHMID_LOCK(kdshmp);

	size = (ullong_t)uap->buffer_size * (ullong_t)uap->total_buffer_count;
	
#ifdef CC_PARTIAL
        MAC_ASSERT(dsp,MAC_SAME);
#endif

	if (is_new) {
	  	/*
		 * This is a new dynamic shared memory segment and
		 * dshmalloc marked it DSHM_BUSY.
		 */
		kdshmp->dmapsize = uap->map_size;
		kdshmp->dmapaddr = uap->dshmaddr;
		kdshmp->dtotalbufs = uap->total_buffer_count;
		kdshmp->dappbufs = uap->appl_buffer_count;
		kdshmp->dbuffersz = uap->buffer_size;

		if ( size < dshminfo.dshmmin ){

			error = EINVAL;
			goto errout;
		}

		if( kdshmp->dmapsize <= 0 ||
		    (kdshmp->dmapsize % DSHM_MIN_ALIGN )) { 

			error = EINVAL;
			goto errout;
		}

		/*
		 * Ensure buffer size is >=DSHM_MINBUFSZ & a power of 2
		 */
		if ( kdshmp->dbuffersz < DSHM_MINBUFSZ || 
		       (kdshmp->dbuffersz & ~(kdshmp->dbuffersz - 1) !=
			kdshmp->dbuffersz)){

			error = EINVAL;
			goto errout;
		}

		/*
		 * Ensure space for library management data
		 */
		if( !kdshmp->dtotalbufs ||
		    kdshmp->dappbufs > kdshmp->dtotalbufs ) {

			error = EINVAL;
			goto errout;
		}

		/*
		 * Ensure attach address is correctly aligned
		 */
		if( kdshmp->dmapaddr == NULL ||
		    (ulong_t)kdshmp->dmapaddr % DSHM_MIN_ALIGN ){

			error = EINVAL;
			goto errout;
		}

		/*
		 * drop both the lock on the newly created kdshmids[] entry
		 * and dshmdir_lck since we might be blocked below.
		 * It is OK to do this since dshmalloc already set DSHM_BUSY
		 * for us.  This will serialize accesses to the kdshmids[]
	         * entry. 
		 */

		DSHMID_UNLOCK(kdshmp, PLMIN);
		DSHMDIR_UNLOCK(PLBASE);
		
		/*
		 * allocate identityless pages, vpage[] & shared L2s
		 */
		if ( (error = segumap_create_obj(kdshmp)) != 0 ) {

                        /* decrement MAC levels reference */
                        mac_rele(lid);

			DSHMDIR_LOCK();
			DSHMID_LOCK(kdshmp);
			direntp->ipcd_ent = NULL;
                        dshmdir.ipcdir_nactive--;
			/* implicitly clears DSHM_BUSY */
                        dshmdealloc(kdshmp);
			DSHMID_UNLOCK(kdshmp, PLMIN);
			DSHMDIR_UNLOCK(PLBASE);

                        /* wake up kdshm_sv waiters */
                        if (SV_BLKD(&kdshmp->kdshm_sv))
                                SV_BROADCAST(&kdshmp->kdshm_sv, 0);

                        goto out;
		}

		/*
		 * Setting kdshm_refcnt without holding kdshm_lck is OK here.
		 * Nobody can attach/remove this object since it is not ready
		 * to be used yet.
		 */ 

		kdshmp->kdshm_refcnt = 1;

		dsp->dshm_atime = dsp->dshm_dtime = 0;
		dsp->dshm_ctime = hrestime.tv_sec;
		dsp->dshm_lpid = 0;
		dsp->dshm_cpid = u.u_procp->p_pidp->pid_id;
		dsp->dshm_perm.mode |= IPC_ALLOC;	

		/*
		 * the dynamic shared memory segment is ready!
		 * wake up all waiters for this segment
		 */

		DSHMID_LOCK(kdshmp);
		kdshmp->kdshm_flag &= ~DSHM_BUSY;
		DSHMID_UNLOCK(kdshmp, PLBASE);

		if (SV_BLKD(&kdshmp->kdshm_sv))
			SV_BROADCAST(&kdshmp->kdshm_sv, 0);

	} else {
		/*
		 * an existing kdshmid_ds[] was found wait for it,
		 * if necessary.
		 * We have to drop dshmdir_lck before waiting
		 */

		DSHMDIR_UNLOCK(PLMIN);
		while (kdshmp->kdshm_flag & DSHM_BUSY) {
			SV_WAIT(&kdshmp->kdshm_sv, DSHM_PRI, &kdshmp->kdshm_lck);
			DSHMID_LOCK(kdshmp);
		}

		/*
		 * we have to make sure that the object is the right one
		 * Hold DSHM_BUSY so that nobdy can take it away
		 */
		kdshmp->kdshm_flag |= DSHM_BUSY;
		DSHMID_UNLOCK(kdshmp, PLMIN);

		/*
		 * if the ipc was not set up successfully or was destroyed,
		 * we have to get it after all.
		 */
		DSHMDIR_LOCK();
		if (direntp->ipcd_ent == NULL) {
			DSHMID_LOCK(kdshmp);
			kdshmp->kdshm_flag &= ~DSHM_BUSY;
			DSHMID_UNLOCK(kdshmp, PLMIN);
			DSHMDIR_UNLOCK(PLBASE);

			if (SV_BLKD(&kdshmp->kdshm_sv)) 
				SV_BROADCAST(&kdshmp->kdshm_sv, 0);


			goto retry;
		}

		/*
		 * ensure dshmget parameters comply with existing
		 * fields.
		 */

		if ( (uap->buffer_size &&
		      uap->buffer_size != kdshmp->dbuffersz ) ||
		     (uap->total_buffer_count > kdshmp->dtotalbufs) ||
		     (uap->appl_buffer_count > kdshmp->dappbufs ) ||
		     (uap->map_size > kdshmp->dmapsize) ) {
			DSHMID_LOCK(kdshmp);
			kdshmp->kdshm_flag &= ~DSHM_BUSY;
			DSHMID_UNLOCK(kdshmp, PLMIN);
			DSHMDIR_UNLOCK(PLBASE);

			if (SV_BLKD(&kdshmp->kdshm_sv))
				SV_BROADCAST(&kdshmp->kdshm_sv, 0);
			
			error = EINVAL;
			goto out;
		}

		DSHMID_LOCK(kdshmp);
		kdshmp->kdshm_flag &= ~DSHM_BUSY;
		DSHMID_UNLOCK(kdshmp, PLMIN);
		DSHMDIR_UNLOCK(PLBASE);

		if (SV_BLKD(&kdshmp->kdshm_sv))
			SV_BROADCAST(&kdshmp->kdshm_sv, 0);
	}

	rvp->r_val1 = direntp->ipcd_seq * dshminfo.dshmmni +
	              (direntp - dshmdir.ipcdir_entries);
	
out:
        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	ADT_LIDCHECK(lid);

	return error;

errout:
	ASSERT(LOCK_OWNED(&dshmdir_lck));
	ASSERT(LOCK_OWNED(&kdshmp->kdshm_lck));
	/*
	 * make kdshm_ds invisible now and
	 * call dshmdealloc to free kdshmid_ds[] entry
	 */
	if(mac_installed){
		DSHMID_UNLOCK(kdshmp, PLMIN);
		DSHMDIR_UNLOCK(PLBASE);
	
		/* decrement MAC levels reference */
		mac_rele(lid);
	
		DSHMDIR_LOCK();
		DSHMID_LOCK(kdshmp);
	}
	direntp->ipcd_ent = NULL;
	dshmdir.ipcdir_nactive--;

	/* implicitly clears DSHM_BUSY */
	dshmdealloc(kdshmp);
	DSHMID_UNLOCK(kdshmp, PLMIN);
	DSHMDIR_UNLOCK(PLBASE);

	/* wake up kdshm_sv waiters */
	if (SV_BLKD(&kdshmp->kdshm_sv))
		SV_BROADCAST(&kdshmp->kdshm_sv, 0);

	goto out;
}

/*
 * int
 * dshmat(uap, rvp)
 * 	dshmat (attach dynamic shared segment) system call.
 *
 * Calling/Exit State:
 *	- This routine may block and should be called at PLBASE without
 *	  holding any lock
 *	- called in context and p_as will not be changed behind us.
 */
STATIC int
dshmat(struct dshmata *uap, rval_t *rvp)
{
	struct kdshmid_ds *kdshmp;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;
	vaddr_t	addr;
	uint	size;
	int	error = 0;
	lid_t	lid;

retry:
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * dshmconv will find the dshm header corresponding to uap->dshmid
	 * if it exists, and mark it DSHM_BUSY, which is a writer lock,
	 * on return.
	 * This will lock kdshmid_ds[] entry and serialize other accesses.
	 * See comments above for details.
	 */
	if (error = dshmconv(uap->dshmid, &kdshmp))
		return error;
	
	lid = kdshmp->kdshm_ds.dshm_perm.ipc_secp->ipc_lid;

	/*
	 * Write MAC access is needed since both read and write operations
	 * modify the object.
	 * DSHM_BUSY set by dshmconv provides the lock on its
	 * dshm_perm required by ipcaccess.
	 */
	if (error = ipcaccess(&kdshmp->kdshm_ds.dshm_perm,
			      DSHM_R|DSHM_W, IPC_MAC, CRED()))
		goto errret;

#ifdef CC_PARTIAL
        MAC_ASSERT(kdshmp->kdshm_ds, MAC_SAME);
#endif

	if (error = ipcaccess(&kdshmp->kdshm_ds.dshm_perm,
			      DSHM_R|DSHM_W, IPC_DAC, CRED()))
		goto errret;


#if defined CCNUMA
	/*
	 * DSHM_BUSY set by dshmconv provides the locking,
	 * no need to acquire the spin lock.
	 *
	 * if DSHM_AF_BUSY is set; the library has it attached
	 * for affinity, further attaches are prohibited.
	 */
	if(kdshmp->kdshm_flag & DSHM_AF_BUSY){
		/* don't jump to errret; it clears DSHM_AF_BUSY */
		DSHMID_LOCK(kdshmp);
		kdshmp->kdshm_flag &= ~DSHM_BUSY;
		DSHMID_UNLOCK(kdshmp, PLBASE);
		
		if (SV_BLKD(&kdshmp->kdshm_sv))
			SV_BROADCAST(&kdshmp->kdshm_sv, 0);

		/* 
		 * Check if the Object Level Audit Criteria
		 * pertains to this event and lid.
		 */
		ADT_LIDCHECK(lid);
		
		return EAGAIN;
	}

	if ( !(uap->dshmflg & _DSHM_PARTIAL)) {
		/*
		 * this is an application attach
		 * if DSHM_AF_DONE isn't set, we need to allocate
		 * the remaining pfns & set DSHM_AF_DONE.
		 */
		if(!(kdshmp->kdshm_flag & DSHM_AF_DONE)) {
			/*
			 * even if we fail to attach below
			 * the pfns will have been allocated
			 */
			segumap_alloc_mem(kdshmp, NULL, SUMAP_ALL);
			kdshmp->kdshm_flag |= DSHM_AF_DONE;
		}
	} else {
		/*
		 * the library is requesting an attach
		 * ensure pfns haven't already been allocated
		 * & set DSHM_AF_BUSY to prevent anyone else
		 * attaching.
		 */
		if(kdshmp->kdshm_flag & DSHM_AF_DONE) {
			/* Er.. affinity is done */
			error = EINVAL;
			goto errret;
		} else {
			/* indicate affinity in progress */
			kdshmp->kdshm_flag |= DSHM_AF_BUSY;
			/*
			 * ensure kernel memory allocated 
			 * no need to allocate idf pages
			 */
			segumap_alloc_mem(kdshmp, NULL, SUMAP_KMEM);
		}
	}
#endif

	/*
	 * No need to hold p_mutex
	 */
	if( pp->p_nlwp > 1 ) {
	  	error = EAGAIN;
		goto errret;
	}
	
	as_wrlock(asp);

	if (pp->p_nshmseg++ >= shminfo.shmseg) {
		error = EMFILE;
		--pp->p_nshmseg;
		as_unlock(asp);
		goto errret;
	}

	if ((error = dshmaddr(kdshmp, &addr, &size)) != 0) {
		pp->p_nshmseg--;
		as_unlock(asp);
		goto errret;
	}
	
	error = dshmmmap(asp, addr, size, PROT_ALL, kdshmp, uap->dshmid);

	if (error) {
		pp->p_nshmseg--;
		as_unlock(asp);
		goto errret;
	}

	/* record dshmem range for the detach */
	shm_add(pp, addr, (size_t)size, kdshmp, DSHM);

	as_unlock(asp);

	/*
	 * no race between testing p_nlwp & setting p_flag
	 * as we are guaranteed to be the only lwp after testing
	 */
	LOCK(&pp->p_mutex, PLHI);
	pp->p_flag |= P_HAS_DSHM;
	UNLOCK(&pp->p_mutex, PLBASE);
	
	rvp->r_val1 = (int) addr;
	kdshmp->kdshm_ds.dshm_atime = hrestime.tv_sec;
	kdshmp->kdshm_ds.dshm_lpid = pp->p_pidp->pid_id;
errret:
	DSHMID_LOCK(kdshmp);
	if (error == 0)
		++kdshmp->kdshm_refcnt;

#ifdef CCNUMA
	if( error && (kdshmp->kdshm_flag & DSHM_AF_BUSY) ) 
		kdshmp->kdshm_flag &= ~DSHM_AF_BUSY;
#endif

	kdshmp->kdshm_flag &= ~DSHM_BUSY;
	DSHMID_UNLOCK(kdshmp, PLBASE);

	if (SV_BLKD(&kdshmp->kdshm_sv))
		SV_BROADCAST(&kdshmp->kdshm_sv, 0);

	/* 
	 * Check if the Object Level Audit Criteria
	 * pertains to this event and lid.
	 */
	ADT_LIDCHECK(lid);

	return error;
}

#define ML_CACHE	64			/* internal result buffer */

/*
 * int
 * dshmctl(struct dshmctla *uap, rval_t *rvp)
 * 	Dshmctl system call.
 * 
 * Calling/Exit State:
 *	The routine should be called at PLBASE.
 */
/* ARGSUSED */
STATIC int
dshmctl(struct dshmctla *uap, rval_t *rvp)
{
	struct kdshmid_ds *kdshmp;
	struct dshmid_ds	*dsp;	/* dynamic shared memory header ptr */
	struct dshmid_ds	ds;	/* hold area for SVR4 IPC_SET */
	cred_t *credp = CRED();
	lid_t  lid;
	int error = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (error = dshmconv(uap->dshmid, &kdshmp))
		return error;
	dsp = &kdshmp->kdshm_ds;
	lid = kdshmp->kdshm_ds.dshm_perm.ipc_secp->ipc_lid;

	switch (uap->cmd) {

	case IPC_RMID:	/* Remove dynamic shared memory identifier. */
		/*
		 * DSHM_BUSY set by dshmconv provides the lock on its dshm_perm
		 * required by ipcaccess.
		 */
		if (error =
		    ipcaccess(&dsp->dshm_perm, DSHM_R|DSHM_W, IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif
		
		/* must have ownership */
		if (credp->cr_uid != dsp->dshm_perm.uid
		  && credp->cr_uid != dsp->dshm_perm.cuid
		  && pm_denied(credp, P_OWNER)) {
			error = EPERM;
			break;
		}

		/*
		 * we are committed to remove the segment from dshmdir
		 * and but not allow the kdshmid_ds[] to be reused yet.
		 * kdshmid_ds[] entry will be freed only after all current
		 * users have dshmdt'ed this dynamic shared memory object.
		 * But we will prevent new users from checking in.
		 */

		DSHMDIR_LOCK();
		dshm_remove_id(uap->dshmid);
		DSHMDIR_UNLOCK(PLBASE);

                if (--kdshmp->kdshm_refcnt == 0) {
                        /* decrement MAC levels reference */
                        mac_rele(lid);
                        FRIPCACL(dsp->dshm_perm.ipc_secp);
			segumap_delete_obj(kdshmp);
			DSHMID_LOCK(kdshmp);
			/* implicitly clears DSHM_BUSY */
			dshmdealloc(kdshmp);
			DSHMID_UNLOCK(kdshmp, PLBASE);
		} else {
			DSHMID_LOCK(kdshmp);
			kdshmp->kdshm_flag &= ~DSHM_BUSY;
			DSHMID_UNLOCK(kdshmp, PLBASE);
		}
		/* 
		 * wake up all sleepers
		 */
		if (SV_BLKD(&kdshmp->kdshm_sv))
			SV_BROADCAST(&kdshmp->kdshm_sv, 0);

                return (0);

	case IPC_SET:
		/*
		 * DSHM_BUSY set by dshmconv provides the lock on its dshm_perm
		 * required by ipcaccess.
		 */
		if (error = ipcaccess(&dsp->dshm_perm, DSHM_R|DSHM_W,
				      IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif
		
		/* must have ownership */
		if (credp->cr_uid != dsp->dshm_perm.uid
		    && credp->cr_uid != dsp->dshm_perm.cuid
		    && pm_denied(credp, P_OWNER)) {
			error = EPERM;
			break;
		}
		if (copyin(uap->arg, &ds, sizeof(ds))) {
			error = EFAULT;
			break;
		}
		if (ds.dshm_perm.uid < (uid_t)0 || ds.dshm_perm.uid > MAXUID ||
		    ds.dshm_perm.gid < (gid_t)0 || ds.dshm_perm.gid > MAXUID){
			error = EINVAL;
			break;
		}

		dsp->dshm_perm.uid = ds.dshm_perm.uid;
		dsp->dshm_perm.gid = ds.dshm_perm.gid;
		dsp->dshm_perm.mode = (ds.dshm_perm.mode & IPC_PERM) |
				     (dsp->dshm_perm.mode & ~IPC_PERM);
		dsp->dshm_ctime = hrestime.tv_sec;

		break;

	case IPC_STAT:
		/*
		 * DSHM_BUSY set by dshmconv provides the lock on its dshm_perm
		 * required by ipcaccess.
		 */
		if (error = ipcaccess(&dsp->dshm_perm, DSHM_R,
				      IPC_MAC|IPC_DAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_DOMINATES);
#endif
		
		dsp->dshm_nattch = kdshmp->kdshm_refcnt - 1;

		bcopy(dsp, &ds, sizeof(ds));
		ds.dshm_perm.ipc_secp = (struct ipc_sec *)NULL;

		if (copyout(&ds,  uap->arg, sizeof(ds)))
			error = EFAULT;

		break;
#ifdef CCNUMA
	case DSHM_SETPLACE:
	case _DSHM_MAP_SETPLACE:
		/*
		 * DSHM_BUSY set by dshmconv provides the lock on its dshm_perm
		 * required by ipcaccess.
		 */
		/*
		 * affinity already done
		 */
		if(kdshmp->kdshm_flag & DSHM_AF_DONE){
			error = EINVAL;
			break;
		}
		
		if (error = ipcaccess(&dsp->dshm_perm, DSHM_R|DSHM_W,
				      IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif
		
		/* must have ownership */
		if (credp->cr_uid != dsp->dshm_perm.uid
		    && credp->cr_uid != dsp->dshm_perm.cuid
		    && pm_denied(credp, P_OWNER)) {
			error = EPERM;
			break;
		}
		if (copyin(uap->arg, &ds, sizeof(ds))) {
			error = EFAULT;
			break;
		}
			
		if( error = segumap_alloc_mem(kdshmp, &ds,
					       uap->cmd == DSHM_SETPLACE ?
					       SUMAP_OBJ : SUMAP_KMEM_PRE ) )
			break;
		
		dsp->dshm_ctime = hrestime.tv_sec;

		break;
		
	case DSHM_CGMEMLOC:
	{

		ulong_t idx, count, ei, min;
		cgid_t	vec[ML_CACHE];
		cgid_t	*uvec;
		
		/*
		 * DSHM_BUSY set by dshmconv provides the lock on its dshm_perm
		 * required by ipcaccess.
		 */
		if (error = ipcaccess(&dsp->dshm_perm, DSHM_R,
				      IPC_MAC|IPC_DAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_DOMINATES);
#endif
		if (copyin(uap->arg, &ds, sizeof(ds))) {
			error = EFAULT;
			break;
		}
		
		idx = ds.dshm_extent.de_buf.deb_index;
		count = ds.dshm_extent.de_buf.deb_count;
		uvec = (cgid_t *)ds.dshm_mapaddr;
		
		if(idx + count > kdshmp->dtotalbufs ) {
			error = EINVAL;
			break;
		}
			
		for(ei = idx + count; idx < ei; idx += ML_CACHE ) {
			min = MIN(ML_CACHE, ei - idx);
			if( error = segumap_ctl_memloc(kdshmp, idx, min, vec) )
				break;
			
			if (copyout(vec, uvec, min * sizeof(cgid_t))) {
				error = EFAULT;
				break;
			}
			uvec += min;
		}
		
		break;
	}

#endif /* CCNUMA */
		
	default:
		error = EINVAL;
		break;
	}

	DSHMID_LOCK(kdshmp);
	kdshmp->kdshm_flag &= ~DSHM_BUSY;
	DSHMID_UNLOCK(kdshmp, PLBASE);

	if (SV_BLKD(&kdshmp->kdshm_sv))
		SV_BROADCAST(&kdshmp->kdshm_sv, 0);
        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	ADT_LIDCHECK(lid);

	return error;
}

/*
 * int
 * dshmkalignment(struct dshmaligna *uap, rval_t *rvp)
 * 	Dshm kalignment system call.
 * 
 * Calling/Exit State:
 *	The routine should be called at PLBASE.
 */
/* ARGSUSED */
STATIC int
dshmkalignment(struct dshmaligna *uap, rval_t *rvp)
{
	rvp->r_val1 = DSHM_MIN_ALIGN;
	return 0;
}

/*
 * various dshmsys() parameters
 */
#define	DSHMAT	0
#define	DSHMCTL	1
#define	DSHMDT	2
#define	DSHMGET	3
#define DSHMALGN 4
#define DSHMREMAP 5

/*
 * int 
 * dshmsys(uap, rvp)
 * 	System entry point for dshmat, dshmctl, dshmdt, and
 *	dshmget system calls.
 *
 * Calling/Exit State:
 *	Should be called and returned at PLBASE.
 */
int
dshmsys(struct dshmsysa *uap, rval_t *rvp)
{
	int error;
	ASSERT(getpl() == PLBASE);

	switch (uap->opcode) {
	case DSHMAT:
		error = dshmat((struct dshmata *)uap, rvp);
		break;
	case DSHMCTL:
		error = dshmctl((struct dshmctla *)uap, rvp);
		break;
	case DSHMDT:
		error = dshmdt((struct dshmdta *)uap, rvp);
		break;
	case DSHMGET:
		error = dshmget((struct dshmgeta *)uap, rvp);
		break;
	case DSHMALGN:
		error = dshmkalignment((struct dshmaligna *)uap, rvp);
		break;
	case DSHMREMAP:
	{
		/*
		 * XXX this is broken
		 * change libdshm.so, ..separate dshm_map
		 * and dshm_unmap
		 */
		struct dshmremapa *a = (struct dshmremapa *)uap;
		if( a->buffer_index == (ulong_t) -1  )
			error = segumap_unload_map(a->dshmid,
						   (vaddr_t)a->uvaddr);
		else
			error = segumap_load_map(a->dshmid, (vaddr_t)a->uvaddr
						 ,a->buffer_index);
		break;
	}
	default:
		error = EINVAL;
		break;
	}

	ASSERT(getpl() == PLBASE);
	return error;
}


/*
 * int 
 * dshmdt(struct dshmdta *uap, rval_t *rvp)
 * 	Detach dynamic shared memory segment system call.
 *
 * Calling/Exit State:
 *	The routine should be called in context at PLBASE without
 *	any lock held.
 *	The caller should prepare to block.
 *
 * Remarks:
 *	This dshmop that does not call dshmconv, which will serialize
 *	all manipulations on the same dynamic shared memory object
 *	kdshmid_ds[].
 *	Conseqently, we have to explicitly serialize with other accesses below.
 */
/* ARGSUSED */
STATIC int
dshmdt(struct dshmdta *uap, rval_t *rvp)
{
	segacct_t *sap;
	proc_t *pp = u.u_procp;
	struct as *asp;
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Does addr represent a dynamic shared memory segment
	 * currently attached to this address space?
	 */
	asp = pp->p_as;
	as_wrlock(asp);
	if (((sap = shm_find(pp, (vaddr_t)uap->addr)) == NULL) ||
	    sap->sa_type != DSHM ){
		as_unlock(asp);
		return EINVAL;
	}

	as_unlock(asp);

#ifdef CCNUMA
	/*
	 * Are we detaching a _DSHM_PARTIAL attach ?
	 *
	 * this could go in kdshmdt but we really only want to do it
	 * via a clean detach i.e. via a syscall not via exit
	 * Note DSHM_AF_BUSY just holds off further attaches
	 * via dshmat. It doesn't block attaches via fork,
	 * or deletion of the object via IPC_RMID
	 * 
	 */
	if( sap->sa_kdshmp->kdshm_flag & DSHM_AF_BUSY ) {

		DSHMID_LOCK(sap->sa_kdshmp);
		while ( sap->sa_kdshmp->kdshm_flag & DSHM_BUSY ) {
			SV_WAIT(&sap->sa_kdshmp->kdshm_sv,
				DSHM_PRI, &sap->sa_kdshmp->kdshm_lck);
			DSHMID_LOCK(sap->sa_kdshmp);
		}
		sap->sa_kdshmp->kdshm_flag &= ~DSHM_AF_BUSY;

		DSHMID_UNLOCK(sap->sa_kdshmp, PLBASE);
	}
#endif	

	return (kdshmdt(sap, B_TRUE));
}

/*
 * int
 * kdshmdt(segacct_t *sap, boolean_t unmap)
 *	detach the dynamic shared memory object descibed by sap
 *	from the current process.
 *
 *	If unmap is B_FALSE, the as is not modified (used by
 *	vfork child processes).
 *
 * Calling/Exit State:
 *	The routine should be called in context at PLBASE and no lock held.
 *	The caller should prepare to block.
 *	It will garner the following lock and release it before returning
 *	to its callers:
 *	- adress writer lock of the current process
 *	- kdshm_lck for each cell
 */
int
kdshmdt(segacct_t *sap, boolean_t unmap)
{
	proc_t *pp = u.u_procp;
	size_t len;
	struct kdshmid_ds *kdshmp;
	struct as *asp;
	vaddr_t addr;
	lid_t lid;
	pl_t opl;
	
	ASSERT(sap != NULL);

	/* garner address writer lock */
	asp = pp->p_as;
	as_wrlock(asp);

	kdshmp = sap->sa_kdshmp;
	len = sap->sa_len;
	addr = sap->sa_addr;

	/*
	 * remove our accounting record from this process
	 * and unmap the object
	 */
	shm_del(pp, addr);

	if (unmap)
		(void)as_unmap(asp, addr, len);

	ASSERT(pp->p_nshmseg > 0);

	/*
	 * Check to see if there are any dshm segments
	 * still attached & if not unset P_HAS_DSHM
	 */
	if(--pp->p_nshmseg == 0 ) {
		opl = LOCK(&pp->p_mutex, PLHI);
		pp->p_flag &= ~P_HAS_DSHM;
		UNLOCK(&pp->p_mutex, opl);
	} else {
		segacct_t *p = pp->p_segacct;

		while (p != NULL) {
			if (p->sa_type == DSHM)
				break;
			p = p->sa_next;
		}
		
		if( p == NULL ){
			/* No DSHM segments attached */
			opl = LOCK(&pp->p_mutex, PLHI);
			pp->p_flag &= ~P_HAS_DSHM;
			UNLOCK(&pp->p_mutex, opl);
		}
	}
	as_unlock(asp);

	
	DSHMID_LOCK(kdshmp);
	while (kdshmp->kdshm_flag & DSHM_BUSY) {
		SV_WAIT(&kdshmp->kdshm_sv, DSHM_PRI, &kdshmp->kdshm_lck);
		DSHMID_LOCK(kdshmp);
	}

	lid = kdshmp->kdshm_ds.dshm_perm.ipc_secp->ipc_lid;
	/*
	 * don't set DSHM_BUSY unless we have to drop
	 * the lock.
	 */
	if (--kdshmp->kdshm_refcnt == 0) {
		kdshmp->kdshm_flag |= DSHM_BUSY;
		DSHMID_UNLOCK(kdshmp, PLBASE);
                /* decrement MAC levels reference */
                mac_rele(lid);
                FRIPCACL(kdshmp->kdshm_ds.dshm_perm.ipc_secp);
		segumap_delete_obj(kdshmp);
		DSHMID_LOCK(kdshmp);
		/* implicitly clears DSHM_BUSY */
		dshmdealloc(kdshmp);
		DSHMID_UNLOCK(kdshmp, PLBASE);

		if (SV_BLKD(&kdshmp->kdshm_sv)) 
			SV_BROADCAST(&kdshmp->kdshm_sv, 0);
	        /* 
	         * Check if the Object Level Audit Criteria
	         * pertains to this event and lid.
	         */
		ADT_LIDCHECK(lid);

		return 0;
	}


	kdshmp->kdshm_ds.dshm_dtime = hrestime.tv_sec;
	kdshmp->kdshm_ds.dshm_lpid = pp->p_pidp->pid_id;

	DSHMID_UNLOCK(kdshmp, PLBASE);

        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	ADT_LIDCHECK(lid);
	return 0;
}

/*
 * STATIC int
 * dshmmmap(struct as *asp, vaddr_t addr, size_t size,
 * 	    uchar_t prot, struct kdshmid_ds * kdp, int dshmid)
 *
 *	Map the dynamic shared memory segment referenced by kdp
 *	into the specified address space at the specified address, size
 *	and protections.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error code on failure.
 */
STATIC int
dshmmmap(struct as *asp, vaddr_t addr, size_t size, uchar_t prot,
	 struct kdshmid_ds * kdp, int dshmid)
{
	struct segumap_crargs segumap_args;
  
	segumap_args.prot = segumap_args.maxprot = prot;
	segumap_args.dshmid = dshmid;
	segumap_args.dshm_hatp = kdp->kdshm_hatp;
	segumap_args.dshm_vpp = kdp->kdshm_vpage;
	segumap_args.dshm_pfn = kdp->kdshm_pfn;
	return as_map(asp, addr, size, segumap_create, &segumap_args);

}

/*
 * STATIC int
 * dshmaddr(struct kdshmid_ds *kdshmp, vaddr_t *addrp, uint_t *sizep)
 *	Validate the specified address & size  for attaching the
 *	specified dynamic shared memory segment to the current address space.
 *
 * Calling/Exit State:
 *	On both entry and exit, as lock is held in write mode.
 *	Returns 0 on success, error code on failure.
 *	On success, *addrp and *sizep contain the virtual address
 *		and size to be used for mapping the dynamic shared memory
 *		segment referenced by kdshmp into the address space.
 */
STATIC int
dshmaddr(struct kdshmid_ds *kdshmp, vaddr_t *addrp, uint_t *sizep)
{
	vaddr_t addr, base;
	uint_t size, len;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;

	size = len = kdshmp->dmapsize;
	addr = base = (vaddr_t)kdshmp->dmapaddr;

	/*
	 * Check that requested virtual address range is valid for
	 * user addresses.
	 */
	if (!VALID_USR_RANGE(base, len))
	  return EINVAL;

	ASSERT( addr % DSHM_MIN_ALIGN == 0 );

	/*
	 * Now check for conflicts with existing mappings
	 */
	if (as_gap(asp, len, (vaddr_t *)&base, &len, AH_LO, (vaddr_t)NULL)
			== 0) {
		ASSERT( addr == base );
		ASSERT( size == len );
		*addrp = addr;
		*sizep = size;
		return 0;
	}

	return EINVAL;
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)
void
print_dshm(const struct kdshmid_ds *kdp)
{


        debug_printf("Dump of kdshmid_ds  (0x%lx)\n\n",
                     (ulong_t)kdp);

        if (kdp == NULL) {
                debug_printf("kdp is NULL?\n\n");
                return;
        }	


        debug_printf("\t\tmaddr=%x bufsz=%d refcnt=%d flags=%x\n",
		     kdp->dmapaddr, kdp->dbuffersz,
		     kdp->kdshm_refcnt, kdp->kdshm_flag);
        debug_printf("\t\thatp=%x vpage=%x pfn=%x\n",
		     kdp->kdshm_hatp, kdp->kdshm_vpage, kdp->kdshm_pfn);
        debug_printf("\n\n");
}
#endif

#ident	"@(#)kern-i386:proc/ipc/shm.c	1.30.6.2"

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
#include <fs/memfs/memfs.h>
#include <fs/vnode.h>
#include <mem/anon.h>
#include <mem/as.h>
#include <mem/fgashm.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_dev.h>
#include <mem/seg_fga.h>
#include <mem/seg_pse.h>
#include <mem/seg_vn.h>
#include <mem/vpage.h>
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

extern vnode_t *pse_makevp(int, int *);
extern void pse_freevp(vnode_t *);

/*
 * Management of System V Shared Memory Objects:
 *
 *	- There will be a maximum of shminfo.shmmni shared memory objects
 *	  configured at boot time.  Shminfo is a tunable parameter.
 *
 *	- Each shared memory object will be represented by a kshmid_ds
 *	  structure.  An array of shminfo.shmmni kshmid_ds structures will
 *	  be allocated at boot time.
 *
 *	- Any object currently exists, will be attached to the global shared
 *	  memory directory shmdir.  See the state table below.
 *
 *	- p_segacct will be protected by the address space writer lock of the
 *	  process.
 *
 *	- p_nshmseg will not be protected by any lock when its users are
 *	  proc_setup, remove_proc, and exit. They will make sure that they are
 *	  the only running context before using the field.
 *	  For other users, the field will be protected by the address space
 *	  writer lock of the process since we call either as_map or as_unmap
 *	  immediately after changed its value.  This will save us extra lock
 *	  round-trip.
 */

STATIC int	shmseqmax;	/* max ipcd_seq value we can use */
ipcdir_t	shmdir;		/* shared memory ipc directory entry */
STATIC lock_t	shmdir_lck;	/* protecting shmdir manipulation,
				 * shmdir.nactive, and the array
				 * shmdir.ipcdir_entries[].
				 *
				 * Locking is not necessary for all accesses
				 * to shmdir.nents and shmdir.ipcdir_entries
				 * since these two fields are modified only in
				 * shm_init during booting.
				 */
STATIC LKINFO_DECL(shmdir_lks, "MS::shmdir_lck", 0);

STATIC struct kshmid_ds	*kshmids;	/* head of the kshmid_ds[] array
					 * Each cell, including struct as
					 * pointed to by kshm_asp,
					 * is protected by its own kshm_lck.
					 */
STATIC LKINFO_DECL(kshmid_lks, "MS::kshm_lck", 0);

/*
 * Locking order:
 *
 *	shmdir_lck:
 *		kshm_lck:
 */

/*
 * ipc directory entry and its lock must be accessible to DAC code
 *
 * Macros to protect shmdir manipulation
 */
#define SHMDIR_LOCK()		LOCK_PLMIN(&shmdir_lck)
#define SHMDIR_UNLOCK(pl)	UNLOCK_PLMIN(&shmdir_lck, (pl))

/*
 * States of a shared memory object:
 *
 *	ipcd_ent	kshm_mvp		Remarks
 *	========	========	======================================
 *	   0		   NULL		non-existent shared memory object
 *	 > 0		 non-NULL	the object exists and # of current
 *					users == kshm_refcnt - 1
 *	 > 0		   NULL		in the middle of creation; not useable
 *	   0		 non-NULL	The object has been removed but there
 *					still some user(s) out there.  The
 *					object will be destroyed when the last
 *					user checks out.
 */

extern struct shminfo	shminfo;	/* shared memory tunables info
					 * Since nobody will change this,
					 * accesses to this structure will
					 * not require locking.
					 */
/* XENIX 286/386 Binary Support */

/*
 * The structure 'xshmid_ds' is the XENIX 386 version of 'shmid_ds'.
 * All x.out executables use 'xshmid_ds'.  The shmctl() routine
 * must convert xshmid_ds <==> shmid_ds for x.out executables.
 */

struct	xshmid_ds {
	struct	o_ipc_perm shm_perm;	/* operation permission struct */
	int	shm_segsz;		/* segment size */
	ushort	shm_ptbl;		/* addr of sd segment */
	ushort	shm_lpid;		/* pid of last shared mem op */
	ushort	shm_cpid;		/* creator pid */
	ushort	shm_nattch;		/* current # attached */
	ushort	shm_cnattch;		/* in-core # attached */
	time_t	shm_atime;		/* last attach time */
	time_t	shm_dtime;		/* last detach time */
	time_t	shm_ctime;		/* last change time */
};		

#define INT16_MAX	32767		/* For 286 compatibility, shared mem
					 * identifiers should only use the low
					 * 15 bits.
					 */

/* End XENIX 286/386 Binary Support */

/*
 * Arguments for the various flavors of shmsys().
 */
struct shmgeta {
	int		opcode;
	key_t		key;
	int		size;
	int		shmflg;
};

struct shmata {
	int		opcode;
	int		shmid;
	caddr_t		addr;
	int		flag;
};

struct shmctla {
	int		opcode;
	int		shmid;
	int		cmd;
	struct shmid_ds	*arg;
};

struct shmdta {
	int		opcode;
	caddr_t		addr;
};

struct shmsysa {
	int		opcode;
};


/*
 * STATICS
 */
STATIC int	shmalloc(ipc_perm_t **);
STATIC void	_shmdealloc(ipc_perm_t *);
STATIC void	shmdealloc(ipc_perm_t *);
STATIC void	shm_remove_id(const int);
STATIC void	shm_rm_mvp(struct kshmid_ds *);
STATIC int	shm_lockmem(struct kshmid_ds *);
STATIC void	shm_unlockmem(struct as **);
STATIC int	shmget(struct shmgeta *, rval_t *);
STATIC int	shmat(struct shmata *, rval_t *);
STATIC int	shmdt(struct shmdta *, rval_t *);
STATIC int	kshmdt(segacct_t *, boolean_t);
STATIC int	shmctl(struct shmctla *, rval_t *);
STATIC int	shmmmap(struct as *, struct kshmid_ds *, vaddr_t, size_t, uchar_t);
STATIC int	shmaddr(struct kshmid_ds *, struct shmata *, vaddr_t *,
			uint_t *);

/*
 * structures for ipcget interfaces
 */

STATIC ipcops_t         shmops = {shmalloc, _shmdealloc};
STATIC ipcdata_t        shmdata = {&shmdir, &shmops};	/* 3rd parameter passed
							 * to ipcget.
							 */
/*
 * void
 * shminit(void)
 * 	called by main (main.c) to initialize System V shared memory IPC.
 *
 * Calling/Exit State:
 *	KMA must be running when it is called.
 */

void
shminit(void)
{
	int	i, size;
	const int n = shminfo.shmmni;
	struct kshmid_ds *mp;
	struct ipc_sec *sp;	/* ptr to sec structs */

	LOCK_INIT(&shmdir_lck, SHMDIR_HIER, PLMIN, &shmdir_lks, KM_NOSLEEP);

	/* allocate and initialize shared memory IPC directory */

	if ((shmdir.ipcdir_nents = n) == 0) {
		/*
		 *+ The system configuration parameter shminfo.shmmni
		 *+ should be checked and set to a non-zero value to
		 *+ enable shared memory.
		 */
		cmn_err(CE_WARN,
		"shminit: shminfo.shmmni is zero, shared memory IPC disabled");
		return;
	}

	shmdir.ipcdir_nactive = 0;

	size = n * (sizeof(struct ipcdirent) + sizeof(struct kshmid_ds) +
		    sizeof(struct ipc_sec));

	shmdir.ipcdir_entries = (ipcdirent_t *)kmem_zalloc(size, KM_NOSLEEP);

	if (shmdir.ipcdir_entries == NULL) {
		/*
		 *+ Could not allocate memory for the SystemV
		 *+ shared memory IPC directory.
		 *+ Instead of PANIC'ing the system, this IPC mechanism
		 *+ is disabled and a warning message is printed.
		 *+ The system configuration parameter shminfo.shmmni
		 *+ should be checked to make sure that it is not
		 *+ inordinately large.
		 */
		cmn_err(CE_WARN,
		"shminit: Can't allocate IPC directory, shared memory IPC disabled");
		/*
		 * Setting ipcdir_nents to zero will cause
		 * ipcget() to always fail with ENOSPC.
		 */
		shmdir.ipcdir_nents = 0;
		return;
	}
	
	kshmids = (struct kshmid_ds *)(shmdir.ipcdir_entries + n);

	sp = (struct ipc_sec *)(kshmids + n);
	/*
	 * initialize kshmids[] DAC entries, locks and sv
	 * in shared memory segments.
	 */
	for (i = n, mp = kshmids; i > 0; i--, mp++, sp++) {
		mp->kshm_ds.shm_perm.ipc_secp = sp;
		LOCK_INIT(&mp->kshm_lck, SHMID_HIER, PLMIN, &kshmid_lks, KM_NOSLEEP);
		SV_INIT(&mp->kshm_sv);
	}

	/*
	 * Compute the maximum ipcd_seq value that will generate a
	 * shmid <= INT16_MAX for any slot.  This constraint is for
	 * XENIX 286/386 Binary Support; without that requirement,
	 * the maximum permissible shmid would be INT_MAX.
	 */
	shmseqmax = (INT16_MAX - (n - 1)) / n;
}

/*
 * int
 * shmalloc(ipc_perm_t **ipcpp)
 *      Allocate and partially initialize a free shmid_ds data
 *      structure from shmids[].  A pointer to the encapsulated ipc_perm
 *      structure is returned via the out argument 'ipcpp'.
 *
 * Calling/Exit State:
 *      It should be called with shmdir_lck held and return
 *	it held.  It will acquire kshm_lck and drop the lock
 *	for eack kshmids[] entry .  It will return the address of a free
 *	kshmids[] in *ipcpp, if found.  Otherwise, returns NULL in *ipcpp.
 *	Since kshmids[] are statically allocated, this routine will not
 *	block.  Conesquently, this function will always return 0.
 *	
 */
STATIC int
shmalloc(ipc_perm_t **ipcpp)
{
	const int n = shminfo.shmmni;
	int i;
	struct kshmid_ds *idp, *freep;
	pl_t opl;
	struct ipc_sec	*ipc_secp;

	ASSERT(LOCK_OWNED(&shmdir_lck));

	/*
	 * look through kshmids[] array for the first free entry:
	 *	- mark the entry IPC_ALLOC since ipcget will make the entry
	 *	  visible to the public.
	 *	- mark the entry SHM_BUSY to block others since the
	 *	  segment setup procedure is not yet completed
 	 */

	freep = (struct kshmid_ds *)NULL;
	for (i = 0, idp = kshmids; i < n; i++, idp++) {
		opl = SHMID_LOCK(idp);
		if ((idp->kshm_ds.shm_perm.mode & IPC_ALLOC) == 0
		    && (idp->kshm_flag & SHM_BUSY) == 0) {

			idp->kshm_ds.shm_perm.mode |= IPC_ALLOC;	
			idp->kshm_flag |= SHM_BUSY;

			ipc_secp = idp->kshm_ds.shm_perm.ipc_secp;
			struct_zero(ipc_secp, sizeof(struct ipc_sec));

			freep= idp;
        		SHMID_UNLOCK(idp, opl);
			break;
		}
        	SHMID_UNLOCK(idp, opl);
	}

	/*
	 * Since a kshmid_ds encapsulates a shmid_ds at the top, and the
	 * shmid_ds in turn encapsulates an ipc_perm at the top, freep
	 * will effectively points to an ipc_perm.  If this relationship
	 * is changed in the future, a macro such as SHMID_TO_IPC will
	 * be required.
	 */

	*ipcpp = (ipc_perm_t *)freep;
	return(0);
}

/* 
 * void
 * _shmdealloc(ipc_perm_t *ipcp)
 *	Basically a wrapper for ipc.c level operations
 * 	acquire SHMID lock and call shmdealloc
 *	Wake any waiters & drop the SHMID_LOCK
 *
 * Calling/Exit State:
 *	DSHMID_DIR LOCK held on entry returns the same way
 *	takes and releases the SHMID_LOCK
 *	SHM_BUSY in kshm_flag owned; we need to take the
 *	SHMID lock because shmdealloc clears SHM_BUSY
 */
STATIC void
_shmdealloc(ipc_perm_t *ipcp)
{
	struct kshmid_ds *kshmp = (struct kshmid_ds *)ipcp;
	struct shmid_ds *dsp;
	pl_t opl;

	ASSERT(ipcp != NULL);
	ASSERT(kshmp->kshm_refcnt == 0);
	ASSERT(LOCK_OWNED(&shmdir_lck));
	ASSERT(!LOCK_OWNED(&kshmp->kshm_lck));

	opl = SHMID_LOCK(kshmp);
	shmdealloc(ipcp);
	SHMID_UNLOCK(kshmp, opl);
	if (SV_BLKD(&kshmp->kshm_sv)) 
		SV_BROADCAST(&kshmp->kshm_sv, 0);
}

/* 
 * void
 * shmdealloc(ipc_perm_t *ipcp)
 *	Free the passed in kshmid_ds data structure.
 *
 * Calling/Exit State:
 *      The caller makes sure that we have exclusive access to
 *	the kshmid_ds and its icp_perm prior to calling this function by
 * 	owning SHM_BUSY on its kshm_flag while holding the SHMID LOCK
 *
 *	On return, the kshmid_ds and its icp_perm are freed.
 *	SHMID lock still held
 */
STATIC void
shmdealloc(ipc_perm_t *ipcp)
{
	struct kshmid_ds *kshmidp = (struct kshmid_ds *)ipcp;
	struct shmid_ds *dsp;

	ASSERT(ipcp != NULL);
	ASSERT(kshmidp->kshm_refcnt == 0);
	ASSERT(LOCK_OWNED(&kshmidp->kshm_lck));
        /*
         * mac_rele is not called within this function because
         * this function is sometimes called with respect to
         * a segment for which there was not a prior mac_hold.
         * Instead, mac_rele is called selectively,
         * by functions which are dellocating segments for which
         * a mac_hold was done when the segments were established.
         *
         */


	dsp = &kshmidp->kshm_ds;
	kshmidp->kshm_flag = 0;
	kshmidp->kshm_mvp = NULL;
	kshmidp->kshm_fgap = NULL;
	dsp->shm_lkcnt = 0;
	dsp->shm_segsz = 0;
	ipcp->mode &= ~IPC_ALLOC;
	return;
}

/*
 * void
 * shm_remove_id(const int shmid)
 *	remove the specified shared memory segment from shmdir.
 *
 * Calling/Exit State:
 *	The caller should be called with shmdir_lck held and return the lock
 *	held.  The routine will not block.
 */
STATIC void
shm_remove_id(const int shmid)
{
	struct ipcdirent *direntp;

	ASSERT(LOCK_OWNED(&shmdir_lck));

	/* remove the kshmid_ds[] from shmdir */

	ASSERT(shmid >= 0 && shmdir.ipcdir_nents > (shmid % shminfo.shmmni));
	ASSERT(shmdir.ipcdir_nactive > 0);

	direntp = shmdir.ipcdir_entries + (shmid % shminfo.shmmni);

	shmdir.ipcdir_nactive--;
	direntp->ipcd_ent = NULL;	/* make it invisible to the public */

	if (++direntp->ipcd_seq > shmseqmax)
		direntp->ipcd_seq = 0;
}

/*
 * STATIC int
 * shm_makevp(struct kshimd_ds *kshmp, size_t nbytes, int flag)
 *	Allocate a vnode to provide backing store for shared memory segment
 *	Return 0 on success, errno for error.
 *
 * Calling/Exit State:
 *	kshmp is the address of an allocated kshmid_ds structure.
 *	nbytes is the requested size of backing store.
 *	flag is a set of validated flags which pertain to the segment creation.
 */
STATIC int
shm_makevp(struct kshmid_ds *kshmp, int nbytes, int flag)
{
	boolean_t psevp = B_FALSE;
	int error;

	if (kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY) {
		return(shm_makefga(kshmp, nbytes, flag));
	}

	/*
	 * Try a PSE segment unless specifically directed otherwise...
	 */
	if (!(kshmp->kshm_flag & SHM_PAGE_BACKED)) {
	    kshmp->kshm_mvp = pse_makevp(nbytes, &kshmp->kshm_mapsize);
	}

	if (kshmp->kshm_mvp != NULL) {
		kshmp->kshm_align = PSE_PAGESIZE;
		psevp = B_TRUE;
	} else if ((kshmp->kshm_mvp = memfs_create_unnamed(nbytes,
			MEMFS_FIXEDSIZE)) != NULL) {
		kshmp->kshm_mapsize = memfs_map_size(kshmp->kshm_mvp);
		kshmp->kshm_align = PAGESIZE;
	}
	if (kshmp->kshm_mvp == NULL)
		return(EINVAL);
	error = VOP_OPEN(&kshmp->kshm_mvp, FREAD | FWRITE, CRED());
	if (error) {
		if (psevp)
			pse_freevp(kshmp->kshm_mvp);
		else
			VN_RELE(kshmp->kshm_mvp);
	}
	return(error);
}

/*
 * void
 * shm_rm_mvp(struct kshmid_ds *kshmid_ds)
 *	Release the memory backing the shared memory segment, be it
 *	memfs backed, PSE backed, or identityless memory backed.
 *
 * Calling/Exit State:
 *	The caller should make sure that mvp is not accessible to others.
 *	This is true since this routine will be called only when it is
 *	the last user of the object.
 *
 * Remarks:
 *	The asp in this context is the address space pointer stored in kshmid_ds
 *	which is used to lock the shared memory object in core.  This address
 *	space is required since we use a_pglck flag to ask as_map to do the
 *	locking.
 *	
 */
STATIC void
shm_rm_mvp(struct kshmid_ds *kshmid_ds)
{
	vnode_t *mvp = kshmid_ds->kshm_mvp;

	/*
	 * FGA SHM (backed by identityless pages) is handled separately,
	 * because there is no vnode backing the segment.
	 */
	if (kshmid_ds->kshm_flag & SHM_SUPPORTS_AFFINITY) {
		shm_rm_fga(kshmid_ds);
		return;
	}

	/*
	 * If we are finally deleting the shared memory, and if no one did 
	 * the SHM_UNLOCK, we must do it now.
	 */

	if (kshmid_ds->kshm_ds.shm_lkcnt) {
		ASSERT(kshmid_ds->kshm_asp);
		shm_unlockmem(&kshmid_ds->kshm_asp);
	}

	VOP_CLOSE(mvp, FREAD | FWRITE, B_TRUE, 0, CRED());
	VN_RELE(mvp);
	return;
}

/*
 * void 
 * shm_add(proc_t *pp, vaddr_t addr, size_t len, void *p, shmacc_t type)
 * 	add a shm/dshm segment to the segacct list of the given process pp.
 *
 * Calling/Exit State:
 *	The routine should be called with as writer lock held.
 *	The callers should prepare to block.
 */
void
shm_add(proc_t *pp, vaddr_t addr, size_t len, void *sp, shmacc_t type)
{
	segacct_t *nsap, **sapp;

	ASSERT(pp != NULL);
	ASSERT(sp != NULL);

	nsap = kmem_alloc(sizeof(*nsap), KM_SLEEP);
	nsap->sa_addr = addr;
	nsap->sa_len  = len;
	nsap->sa_type = type;

	switch(type){
	case SHM:
		nsap->sa_kshmp = sp; 
		break;
	case DSHM:
		nsap->sa_kdshmp = sp;
		break;
	}
	
	/* add this to the sorted list in ascending order */
	sapp = &pp->p_segacct;
	while ((*sapp != NULL) && ((*sapp)->sa_addr < addr))
		sapp = &((*sapp)->sa_next);

	ASSERT((*sapp == NULL) || ((*sapp)->sa_addr >= addr + len ));
	nsap->sa_next = *sapp;
	*sapp = nsap;
	return;
}

/*
 * void
 * shm_del(proc_t *pp, vaddr_t addr)
 * 	Delete the segacct record attached at addr from the pp->p_segacct list.
 *
 * Calling/Exit State:
 *	The routine should be called with as writer lock held and will return
 *	with lock held.
 */
void
shm_del(proc_t *pp, vaddr_t addr)
{
	segacct_t *osap, **sapp;

	ASSERT(pp != NULL);

	sapp = &pp->p_segacct;
	while ((*sapp != NULL) && ((*sapp)->sa_addr < addr))
		sapp = &((*sapp)->sa_next);

	ASSERT((*sapp != NULL) && ((*sapp)->sa_addr == addr));
	
	osap = *sapp;	/* save pointer to structure for kmem_free */

	*sapp = (*sapp)->sa_next;

	kmem_free(osap, sizeof(*osap));

	return;
}

/*
 * segacct_t *
 * shm_find(proc_t *pp, vaddr_t addr)
 *	Find the segacct structure attached at addr in pp->p_segacct link list.
 * 
 * Calling/Exit State:
 *	The routine should be called holding as writer lock and will return
 *	with the lock held
 */ 
segacct_t *
shm_find(proc_t *pp, vaddr_t addr)
{

	segacct_t *sap;

	ASSERT(pp != NULL);

	sap = pp->p_segacct;
	while (sap != NULL) {
		if (sap->sa_addr == addr) {
			return(sap);
		}
		sap = sap->sa_next;
	}

	return NULL;
}

/*
 * STATIC int 
 * shm_lockmem(struct kshmid_ds *kshmp)
 *	Establish a memory lock on a shared memory segment.
 *
 * Calling/Exit State:
 *	This routine may block and no locks should held on entry but the
 *	caller must guarantee that, independent of the number of times
 *	this anon map is locked, only one shm_lockmem call is done for
 *	this anon map before shm_unlockmem is called.
 *
 *	On success, zero is returned and the pages are memory locked.
 *	On failure, a non-zero errno is returned and the pages remain unlocked 
 *	(at least by this caller).
 *
 *
 * Remarks:
 *	- The address space allocated in this routine will be stored in
 *	  kshmid_ds and used to lock the shared memory object in core. 
 *	  This address space is required since we use a_pglck flag to ask
 *	  as_map to do the locking.  It is NOT a real address space for a
 *	  process.
 */
STATIC int 
shm_lockmem(struct kshmid_ds *kshmp)
{
	struct as *asp;
	int err;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(kshmp);
	ASSERT((kshmp->kshm_mvp != NULL) || (kshmp->kshm_fgap != NULL));

	/*
	 * For affinity shared memory, make sure that entire segment has
	 * been made memory resident.
	 */
	if (kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY) {
		fgashm_lockmem(kshmp);
		return(0);
	}

	asp = kshmp->kshm_asp = as_alloc();
	ASSERT(kshmp->kshm_asp != NULL);

	kshmp->kshm_asp->a_paglck = 1;

	if (err = shmmmap(asp, kshmp, UVBASE, kshmp->kshm_mapsize,
			  PROT_ALL)) {
		kshmp->kshm_asp = NULL;
		kmem_free(asp, sizeof(struct as));
	}
	return (err);
}

/*
 * STATIC void
 * shm_unlockmem(struct as **aspp)
 *	Unlock the anonymous pages in the address space asp.
 *	All the pages in the address space will be unlocked
 *
 * Calling/Exit State:
 * 	No locks are held on entry but the caller must guarantee that 
 *	I have exclusive access to the address space asp by holding
 *	SHM_BUSY.  The caller further guarantees that a 
 *	corresponding and successful call to shm_lockmem was made for
 *	each call to this function. There is no way to ASSERT this here.
 *
 *	On return, the pages will have been unlocked (at least by the caller).
 *
 * Remarks:
 *	- The asp passed in is the address space pointer stored in kshmid_ds
 *	  which is used to lock the shared memory object in core.  This address
 *	  space is required since we use a_pglck flag to ask as_map to do the
 *	  locking.  It is NOT a real address space for a process.
 */
STATIC void
shm_unlockmem(struct as **aspp)
{
	ASSERT(*aspp);

	as_free(*aspp);
	*aspp = NULL;
	return;
}

/*
 * int
 * shmconv(int, strcut kshmid_ds **)
 *	Convert user supplied shm_id into a ptr to its corresponding kshmid_ds[]
 *	entry in kshmpp.
 *
 * Calling/Exit State:
 *	- This routine may block and should be called at PLBASE without
 *	  holding any lock.
 *	- This routine will garner  shmdir_lck and the kshm_lck of the
 *	  kshmid_ds[] entry and	release them before returning to its caller. 
 *	- This routine will not block.
 *	- Return 0 on success and mark the shm haeder SHM_BUSY; otherwise,
 *	  return non-zero.
 *
 * Remarks:
 * 	This function cannot be static because it is called in ipcdac.c.
 */

int
shmconv(int shm_id, struct kshmid_ds **kshmpp)
{
	struct kshmid_ds *kshmp;	/* ptr to associated header */
	ipcdirent_t *direntp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	*kshmpp = (struct kshmid_ds *)NULL;

	if (shm_id < 0)
		return EINVAL;

	SHMDIR_LOCK();
	direntp = shmdir.ipcdir_entries + (shm_id % shminfo.shmmni);
	ASSERT(direntp != NULL);

	kshmp = (struct kshmid_ds *)direntp->ipcd_ent;

	if (kshmp == NULL) {
		SHMDIR_UNLOCK(PLBASE);
		return EINVAL;
	}
	SHMDIR_UNLOCK(PLBASE);	/* drop it since we might block for SHM_BUSY */

	SHMID_LOCK(kshmp);
	while (kshmp->kshm_flag & SHM_BUSY) {
		SV_WAIT(&kshmp->kshm_sv, SHM_PRI, &kshmp->kshm_lck);
		SHMID_LOCK(kshmp);
	}

	/*
	 * hold SHM_BUSY so that nobody can take it away
	 */

	kshmp->kshm_flag |= SHM_BUSY;
	SHMID_UNLOCK(kshmp, PLBASE);

	/*
	 * the object may be gone or replaced while I am waiting for SHM_BUSY
	 */

	SHMDIR_LOCK();
	if (direntp->ipcd_ent != (ipc_perm_t *)kshmp) {
		/*
		 * We lost the race to IPC_RMID, or the underlying kshmid_ds[]
		 * has been changed while we were waiting for it.
		 */
		SHMID_LOCK(kshmp);
		kshmp->kshm_flag &= ~SHM_BUSY;
		SHMID_UNLOCK(kshmp, PLMIN);
		SHMDIR_UNLOCK(PLBASE);

		if (SV_BLKD(&kshmp->kshm_sv))
                	SV_BROADCAST(&kshmp->kshm_sv, 0);

		return EINVAL;
	}

	if (!(kshmp->kshm_ds.shm_perm.mode & IPC_ALLOC)  
	  || shm_id / shminfo.shmmni != direntp->ipcd_seq) {

		SHMID_LOCK(kshmp);
		kshmp->kshm_flag &= ~SHM_BUSY;
		SHMID_UNLOCK(kshmp, PLMIN);
		SHMDIR_UNLOCK(PLBASE);

		if (SV_BLKD(&kshmp->kshm_sv))
			SV_BROADCAST(&kshmp->kshm_sv, 0);

		return EINVAL;
	}
	SHMDIR_UNLOCK(PLBASE);
	*kshmpp = kshmp;
	return 0;
}

/*
 * int
 * shmget(struct shmgeta *,  rval_t *)
 *	create a new shmem IPC. system call shmget(2)
 *
 * Calling/Exit State:
 *	This routine may block and should be called at PLBASE without holding
 *	any lock.
 *
 * Remarks:
 *	- shared memory identifier = current ipcd_seq * shminfo.shmmni + slot
 *	  number of the shmdir.ipcdir_entries[] entry
 */
STATIC int
shmget(struct shmgeta *uap, rval_t *rvp)
{
	struct ipcdirent *direntp;	/* shmdir[] entry returned by ipcget.
					 * It can be new or existing.
					 */
	boolean_t is_new = B_FALSE;	/* indicate whether a new
					 * kshmid_ds and shmdir
					 * is allocated by ipcget.
			 		 */
	int error = 0;
	struct kshmid_ds *kshmp;
	struct shmid_ds *dsp;
	int size;
	lid_t lid;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Validate specified affinity, if any.
	 *
	 * Q: SHM_PLC_DEFAULT is not called out as a permissible argument to
	 * shmget() in the API.  Is that truly intended?
	 */
	if (uap->shmflg & SHM_AFFINITY_MASK) {
	    int affinity = uap->shmflg & SHM_AFFINITY_MASK;
	    if ((affinity != SHM_BALANCED) && (affinity != SHM_CPUGROUP) &&
		(affinity != SHM_FIRSTUSAGE)) {
		return EINVAL;
	    }
	}

	if (uap->shmflg & SHM_MIGRATE) {
	    return EINVAL;
	}

	/*
	 * Validate that the user has any necessary privilege to create/attach
	 * a shared memory segment.
	 */
	if ((uap->shmflg & IPC_SPECIAL) && pm_denied(CRED(), P_PLOCK)) {
	    return EPERM;
	}

retry:
	SHMDIR_LOCK();
	
	if (error = ipcget(uap->key, uap->shmflg, &shmdata,
	                   &is_new, &direntp)) {
		SHMDIR_UNLOCK(PLBASE);
		return error;
	}

	ASSERT(direntp != NULL);
	kshmp = (struct kshmid_ds *)direntp->ipcd_ent;
	ASSERT(kshmp != NULL);
	dsp = &kshmp->kshm_ds;
	lid = kshmp->kshm_ds.shm_perm.ipc_secp->ipc_lid;
	SHMID_LOCK(kshmp);

	size = uap->size;

#ifdef CC_PARTIAL
        MAC_ASSERT(dsp,MAC_SAME);
#endif

	if (is_new) {
		/*
		 * This is a new shared memory segment and shmalloc marked
		 * it SHM_BUSY.
		 *
		 * Allocate an anon_map structure and anon array and finish
		 * initialization.
		 */
		if (size < shminfo.shmmin || size > shminfo.shmmax) {
			/*
			 * make kshm_ds invisible now and
			 * call shmdealloc to free kshmid_ds[] entry
			 */
			if(mac_installed) {
				SHMID_UNLOCK(kshmp, PLMIN);
				SHMDIR_UNLOCK(PLBASE);
			
				/* decrement MAC levels reference */
				mac_rele(lid);
			
				SHMDIR_LOCK();
				SHMID_LOCK(kshmp);
			}
			direntp->ipcd_ent = NULL;
			shmdir.ipcdir_nactive--;

			/* implicitly clears SHM_BUSY */
			shmdealloc((ipc_perm_t *)dsp);
			SHMID_UNLOCK(kshmp, PLMIN);
			SHMDIR_UNLOCK(PLBASE);
			
			/* wake up kshm_sv waiters */
			if (SV_BLKD(&kshmp->kshm_sv))
				SV_BROADCAST(&kshmp->kshm_sv, 0);

			error = EINVAL;
			goto out;
		}

		if (uap->shmflg & IPC_SPECIAL) {
		    kshmp->kshm_flag |= SHM_SUPPORTS_AFFINITY;
		}

		/*
		 * drop both the lock on the newly created kshmids[] entry
		 * and shmdir_lck since we might blocked below.
		 * It is OK to do this since shmalloc already set SHM_BUSY
		 * for us.  This will serialize accesses to the kshmids[]
	         * entry. 
		 */

		SHMID_UNLOCK(kshmp, PLMIN);
		SHMDIR_UNLOCK(PLBASE);

		/*
		 * Reserve the swap space and get anon_map and anon[] for it
		 */
		if (shm_makevp(kshmp, size, uap->shmflg) != 0) {
			/*
			 * make kshm_ds invisible now and
			 * call shmdealloc to free kshmid_ds[] entry
			 */
                        /* decrement MAC levels reference */
                        mac_rele(lid);

			SHMDIR_LOCK();
			SHMID_LOCK(kshmp);

			direntp->ipcd_ent = NULL;
			shmdir.ipcdir_nactive--;
			
			/* implicitly clears SHM_BUSY & SHM_SUPPORTS_AFFINITY */
			shmdealloc((ipc_perm_t *)dsp);
			SHMID_UNLOCK(kshmp, PLMIN);
			SHMDIR_UNLOCK(PLBASE);

			/* wake up kshm_sv waiters  */
			if (SV_BLKD(&kshmp->kshm_sv))
				SV_BROADCAST(&kshmp->kshm_sv, 0);

			error = ENOMEM;
			goto out;
		}

		/*
		 * Store the original user's requested size, in bytes,
		 * rather than the page-aligned size.  The former is
		 * used for IPC_STAT and shmget() lookups.  The latter
		 * is saved in the anon_map structure and is used for
		 * calls to the vm layer.
		 */

		/*
		 * Setting kshm_refcnt without holding kshm_lck is OK here.
		 * Nobody can attach/remove this object since it is not ready
		 * to be used yet.
		 */ 

		kshmp->kshm_refcnt = 1;
		dsp->shm_segsz = size;
		dsp->shm_lkcnt = 0;
		dsp->shm_atime = dsp->shm_dtime = 0;
		dsp->shm_ctime = hrestime.tv_sec;
		dsp->shm_lpid = 0;
		dsp->shm_cpid = u.u_procp->p_pidp->pid_id;
		dsp->shm_perm.mode |= IPC_ALLOC;	

		/*
		 * the shared memory segment is ready!
		 * wake up all waiters for this segment
		 */

		SHMID_LOCK(kshmp);
		kshmp->kshm_flag &= ~SHM_BUSY;
		SHMID_UNLOCK(kshmp, PLBASE);

		if (SV_BLKD(&kshmp->kshm_sv))
			SV_BROADCAST(&kshmp->kshm_sv, 0);

	} else {
		/*
		 * an existing kshmid_ds[] was found and wait for it,
		 * if necessary.
		 * We have to drop shmdir_lck before waiting
		 */

		SHMDIR_UNLOCK(PLMIN);
		while (kshmp->kshm_flag & SHM_BUSY) {
			SV_WAIT(&kshmp->kshm_sv, SHM_PRI, &kshmp->kshm_lck);
			SHMID_LOCK(kshmp);
		}

		/*
		 * we have to make sure that the object is the right one
		 * Hold SHM_BUSY so that nobdy can take it away
		 */
		kshmp->kshm_flag |= SHM_BUSY;
		SHMID_UNLOCK(kshmp, PLBASE);

		/*
		 * if the ipc was not set up successfully or was destroyed,
		 * we have to get it after all.
		 */
		SHMDIR_LOCK();
		if (direntp->ipcd_ent == NULL) {
			SHMID_LOCK(kshmp);
			kshmp->kshm_flag &= ~SHM_BUSY;
			SHMID_UNLOCK(kshmp, PLMIN);
			SHMDIR_UNLOCK(PLBASE);

			if (SV_BLKD(&kshmp->kshm_sv))
				SV_BROADCAST(&kshmp->kshm_sv, 0);

			goto retry;
		}


		/*
		 * Only return a FGA shm segment to the caller if the
		 * caller is expecting one (indicated by specifying
		 * IPC_SPECIAL on the call to shmget()).
		 */
		if ((size && size > dsp->shm_segsz) ||
		    (((kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY) != 0) !=
		     			((uap->shmflg & IPC_SPECIAL) != 0))) {
			SHMID_LOCK(kshmp);
			kshmp->kshm_flag &= ~SHM_BUSY;
			SHMID_UNLOCK(kshmp, PLMIN);
			SHMDIR_UNLOCK(PLBASE);
				
			if (SV_BLKD(&kshmp->kshm_sv)) 
				SV_BROADCAST(&kshmp->kshm_sv, 0);
			
			error = EINVAL;
			goto out;
		}	

		SHMID_LOCK(kshmp);
		kshmp->kshm_flag &= ~SHM_BUSY;
		SHMID_UNLOCK(kshmp, PLMIN);
		SHMDIR_UNLOCK(PLBASE);

		if (SV_BLKD(&kshmp->kshm_sv))
			SV_BROADCAST(&kshmp->kshm_sv, 0);
	}

	rvp->r_val1 = direntp->ipcd_seq * shminfo.shmmni +
	              (direntp - shmdir.ipcdir_entries);

out:
        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	ADT_LIDCHECK(lid);
	return error;
}

/*
 * int
 * shmat(uap, rvp)
 * 	shmat (attach shared segment) system call.
 *
 * Calling/Exit State:
 *	- This routine may block and should be called at PLBASE without
 *	  holding any lock
 *	- called in context and p_as will not be changed behind us.
 */
STATIC int
shmat(struct shmata *uap, rval_t *rvp)
{
	struct kshmid_ds *kshmp;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;
	vaddr_t	addr;
	uint	size;
	int	error = 0;
	lid_t	lid;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * shmconv will find the shm header corresponding to uap->shmid
	 * , if exists, and mark it SHM_BUSY, which is a writer lock, on return.
	 * This will lock kshmid_ds[] entry and serialize other accesses.
	 * See comments above for details.
	 */

	if (error = shmconv(uap->shmid, &kshmp))
		return error;

	lid = kshmp->kshm_ds.shm_perm.ipc_secp->ipc_lid;

	/*
	 * Write MAC access is needed since both read and write operations
	 * modify the object.
	 * SHM_BUSY set by shmconv provides the lock on its shm_perm required
	 * by ipcaccess.
	 */
	if (error = ipcaccess(&kshmp->kshm_ds.shm_perm, SHM_R|SHM_W, IPC_MAC, CRED()))
		goto errret;

#ifdef CC_PARTIAL
        MAC_ASSERT(kshmp->kshm_ds, MAC_SAME);
#endif

	if (error = ipcaccess(&kshmp->kshm_ds.shm_perm, SHM_R, IPC_DAC, CRED()))
		goto errret;

	if ((uap->flag & SHM_RDONLY) == 0
	  && (error = ipcaccess(&kshmp->kshm_ds.shm_perm, SHM_W, IPC_DAC, CRED())))
		goto errret;

	as_wrlock(asp);
	if (pp->p_nshmseg++ >= shminfo.shmseg) {
		error = EMFILE;
		--pp->p_nshmseg;
		as_unlock(asp);
		goto errret;
	}

	/*
	 * I have to make sure that nobody can change my anon_map size
	 * while I am setting up my new address space mapping for this segment.
	 *
	 * No am_lck_cnt is required since this routine is always called in
	 * context and there is no way to change the size of an existing
	 * shared memory object.
	 */
	if ((error = shmaddr(kshmp, uap, &addr, &size)) != 0) {
		pp->p_nshmseg--;
		as_unlock(asp);
		goto errret;
	}
	
	error = shmmmap(asp, kshmp, addr, size,
		(uap->flag & SHM_RDONLY) ? (PROT_ALL & ~PROT_WRITE) : PROT_ALL);
	if (error) {
		pp->p_nshmseg--;
		as_unlock(asp);
		goto errret;
	}

	/* record shmem range for the detach */
	shm_add(pp, addr, (size_t)size, kshmp, SHM);

	as_unlock(asp);

	rvp->r_val1 = (int) addr;
	kshmp->kshm_ds.shm_atime = hrestime.tv_sec;
	kshmp->kshm_ds.shm_lpid = pp->p_pidp->pid_id;
errret:
	SHMID_LOCK(kshmp);

	if (error == 0)
		++kshmp->kshm_refcnt;

	kshmp->kshm_flag &= ~SHM_BUSY;
	SHMID_UNLOCK(kshmp, PLBASE);
	
	if (SV_BLKD(&kshmp->kshm_sv)) 
		SV_BROADCAST(&kshmp->kshm_sv, 0);
		
	/* 
	 * Check if the Object Level Audit Criteria
	 * pertains to this event and lid.
	 */
	ADT_LIDCHECK(lid);

	return error;
}

/*
 * int
 * shmctl(struct shmctla *uap, rval_t *rvp)
 * 	Shmctl system call.
 * 
 * Calling/Exit State:
 *	The routine should be called at PLBASE.
 */
/* ARGSUSED */
STATIC int
shmctl(struct shmctla *uap, rval_t *rvp)
{
	struct kshmid_ds *kshmp;
	struct shmid_ds	*dsp;	/* shared memory header ptr */
	struct o_shmid_ds ods;	/* hold area for SVR3 IPC_O_SET */
	struct shmid_ds	ds;	/* hold area for SVR4 IPC_SET */
	cred_t *credp = CRED();
	lid_t  lid;
	int error = 0;

	struct xshmid_ds xds;	/* hold for XENIX shmid_ds */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (error = shmconv(uap->shmid, &kshmp))
		return error;
	dsp = &kshmp->kshm_ds;
	lid = kshmp->kshm_ds.shm_perm.ipc_secp->ipc_lid;

	switch (uap->cmd) {

	case IPC_O_RMID:
	case IPC_RMID:	/* Remove shared memory identifier. */
		/*
		 * SHM_BUSY set by shmconv provides the lock on its shm_perm
		 * required by ipcaccess.
		 */
		if (error =
		    ipcaccess(&dsp->shm_perm, SHM_R|SHM_W, IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif
		
		/* must have ownership */
		if (credp->cr_uid != dsp->shm_perm.uid
		  && credp->cr_uid != dsp->shm_perm.cuid
		  && pm_denied(credp, P_OWNER)) {
			error = EPERM;
			break;
		}

		/*
		 * we are committed to remove the segment from shmdir
		 * and but not allow the kshmid_ds[] to be reused yet.
		 * kshmid_ds[] entry will be freed only after all current
		 * users have shmdt'ed this shared memory object.
		 * But we will prevent new users from checking in.
		 */

		SHMDIR_LOCK();
		shm_remove_id(uap->shmid);
		SHMDIR_UNLOCK(PLBASE);

                if (--kshmp->kshm_refcnt == 0) {
                        /* decrement MAC levels reference */
                        mac_rele(lid);
                        FRIPCACL(dsp->shm_perm.ipc_secp);
			shm_rm_mvp(kshmp);
			SHMID_LOCK(kshmp);
			/* implicitly clears SHM_BUSY */
			shmdealloc((ipc_perm_t *)kshmp);
			SHMID_UNLOCK(kshmp, PLBASE);

		} else {
			SHMID_LOCK(kshmp);
			kshmp->kshm_flag &= ~SHM_BUSY;
			SHMID_UNLOCK(kshmp, PLBASE);
		}
		/* 
		 * wake up all sleepers
		 */
		if (SV_BLKD(&kshmp->kshm_sv))
			SV_BROADCAST(&kshmp->kshm_sv, 0);
		
                return (0);

	case IPC_O_SET:	/* Set ownership and permissions. */
		/*
		 * SHM_BUSY set by shmconv provides the lock on its shm_perm
		 * required by ipcaccess.
		 */
		if (error =
		    ipcaccess(&dsp->shm_perm, SHM_R|SHM_W, IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif
		
		/* must have ownership */
		if (credp->cr_uid != dsp->shm_perm.uid
		  && credp->cr_uid != dsp->shm_perm.cuid
		  && pm_denied(credp, P_OWNER)) {
			error = EPERM;
			break;
		}
		/* XENIX 286/386 Binary Support */
		if (VIRTUAL_XOUT) {
			/* Copy in XENIX version of xshmid_ds.  Note that
			 * IPC_SET only really looks at fields in the
			 * ipc_perm portion of shmid_ds, and at that point
			 * the xshmid_ds and shmid_ds structs are the same.
			 * Don't need to kludge IPC_SET to use xds instead of
			 * ds, because the structs agree on shm_perm.  However,
			 * we DO want to ensure that we only copy sizeof(xds).
			 */
			if (copyin(uap->arg, &ods, sizeof(xds))) {
				error = EFAULT;
				break;
			}
		} else
		/* End XENIX 286/386 Binary Support */

		if (copyin(uap->arg, &ods, sizeof(ods))) {
			error = EFAULT;
			break;
		}

		if (ods.shm_perm.uid > MAXUID || ods.shm_perm.gid > MAXUID){
			error = EINVAL;
			break;
		}
		dsp->shm_perm.uid = ods.shm_perm.uid;
		dsp->shm_perm.gid = ods.shm_perm.gid;
		dsp->shm_perm.mode = (ods.shm_perm.mode & IPC_PERM) |
		                     (dsp->shm_perm.mode & ~IPC_PERM);
		dsp->shm_ctime = hrestime.tv_sec;

		break;

	case IPC_SET:
		/*
		 * SHM_BUSY set by shmconv provides the lock on its shm_perm
		 * required by ipcaccess.
		 */
		if (error = ipcaccess(&dsp->shm_perm, SHM_R|SHM_W, IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif
		
		/* must have ownership */
		if (credp->cr_uid != dsp->shm_perm.uid
		    && credp->cr_uid != dsp->shm_perm.cuid
		    && pm_denied(credp, P_OWNER)) {
			error = EPERM;
			break;
		}
		if (copyin(uap->arg, &ds, sizeof(ds))) {
			error = EFAULT;
			break;
		}
		if (ds.shm_perm.uid < (uid_t)0 || ds.shm_perm.uid > MAXUID ||
		    ds.shm_perm.gid < (gid_t)0 || ds.shm_perm.gid > MAXUID){
			error = EINVAL;
			break;
		}

		dsp->shm_perm.uid = ds.shm_perm.uid;
		dsp->shm_perm.gid = ds.shm_perm.gid;
		dsp->shm_perm.mode = (ds.shm_perm.mode & IPC_PERM) |
				     (dsp->shm_perm.mode & ~IPC_PERM);
		dsp->shm_ctime = hrestime.tv_sec;

		break;

	case IPC_O_STAT:	/* Get shared memory data structure. */
		/*
		 * SHM_BUSY set by shmconv provides the lock on its shm_perm
		 * required by ipcaccess.
		 */
		if (error = ipcaccess(&dsp->shm_perm, SHM_R, IPC_MAC|IPC_DAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp,MAC_DOMINATES);
#endif
		
		/* XENIX 286/386 Binary Support */
		if (VIRTUAL_XOUT) {
			/* Kludge up the XENIX version of shmid_ds. */
			xds.shm_perm.uid = (o_uid_t) dsp->shm_perm.uid;
			xds.shm_perm.gid = (o_gid_t) dsp->shm_perm.gid;
			xds.shm_perm.cuid = (o_uid_t) dsp->shm_perm.cuid;
			xds.shm_perm.cgid = (o_gid_t) dsp->shm_perm.cgid;
			xds.shm_perm.mode = (o_mode_t) dsp->shm_perm.mode;
			xds.shm_perm.seq = (ushort) dsp->shm_perm.seq;
			xds.shm_perm.key = dsp->shm_perm.key;
			xds.shm_segsz = dsp->shm_segsz;
			xds.shm_lpid = dsp->shm_lpid;
			xds.shm_cpid = dsp->shm_cpid;
			xds.shm_nattch = kshmp->kshm_refcnt;

			xds.shm_cnattch = xds.shm_nattch;
			xds.shm_atime = dsp->shm_atime;
			xds.shm_dtime = dsp->shm_dtime;
			xds.shm_ctime = dsp->shm_ctime;

			if (copyout(&xds, uap->arg, sizeof(xds)))
				error = EFAULT;
		} else
		/* End XENIX 286/386 Binary Support */

		{

			dsp->shm_nattch = kshmp->kshm_refcnt - 1;

			dsp->shm_cnattch = dsp->shm_nattch;

			/*
			 * copy expanded shmid_ds struct to SVR3 o_shmid_ds. 
			 * The o_shmid_ds data structure supports SVR3
			 * applications. EFT applications use struct shmid_ds.
			 */

			if (dsp->shm_perm.uid > USHRT_MAX || dsp->shm_perm.gid > USHRT_MAX ||
			    dsp->shm_perm.cuid > USHRT_MAX || dsp->shm_perm.cgid > USHRT_MAX ||
			    dsp->shm_perm.seq > USHRT_MAX || dsp->shm_lpid > SHRT_MAX ||
			    dsp->shm_cpid > SHRT_MAX || dsp->shm_nattch > USHRT_MAX || 
			    dsp->shm_cnattch > USHRT_MAX){
				error = EOVERFLOW;
				break;
			}

			ods.shm_perm.uid = (o_uid_t) dsp->shm_perm.uid;
			ods.shm_perm.gid = (o_gid_t) dsp->shm_perm.gid;
			ods.shm_perm.cuid = (o_uid_t) dsp->shm_perm.cuid;
			ods.shm_perm.cgid = (o_gid_t) dsp->shm_perm.cgid;
			ods.shm_perm.mode = (o_mode_t) dsp->shm_perm.mode;
			ods.shm_perm.seq = (ushort) dsp->shm_perm.seq;
			ods.shm_perm.key = dsp->shm_perm.key;
			ods.shm_segsz = dsp->shm_segsz;
			ods.shm_lkcnt = dsp->shm_lkcnt;
			ods.pad[0] = 0; 	/* initialize SVR3 reserve pad */
			ods.pad[1] = 0;
			ods.shm_lpid = (o_pid_t) dsp->shm_lpid;
			ods.shm_cpid = (o_pid_t) dsp->shm_cpid;
			ods.shm_nattch = (ushort) dsp->shm_nattch;
			ods.shm_cnattch = (ushort) dsp->shm_cnattch;
			ods.shm_atime = dsp->shm_atime;
			ods.shm_dtime = dsp->shm_dtime;
			ods.shm_ctime = dsp->shm_ctime;

			if (copyout(&ods, uap->arg,
			            sizeof(ods)))
				error = EFAULT;
		}

		break;

	case IPC_STAT:
		/*
		 * SHM_BUSY set by shmconv provides the lock on its shm_perm
		 * required by ipcaccess.
		 */
		if (error = ipcaccess(&dsp->shm_perm, SHM_R, IPC_MAC|IPC_DAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_DOMINATES);
#endif
		
		dsp->shm_nattch = kshmp->kshm_refcnt - 1;

		dsp->shm_cnattch = dsp->shm_nattch;

		bcopy(dsp, &ds, sizeof(ds));
		ds.shm_perm.ipc_secp = (struct ipc_sec *)NULL;

		if (copyout(&ds,  uap->arg, sizeof(ds)))
			error = EFAULT;

		break;

	case SHM_LOCK:	/* Lock segment in memory */
		/*
		 * The access check is done before the P_SYSOPS privilege
		 * check, since return must be EINVAL on MAC failure,
		 * regardless of P_SYSOPS privilege (which could possibly
		 * return EPERM).
		 */
		if (error = ipcaccess(&dsp->shm_perm, SHM_R|SHM_W, IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif
		
		if (pm_denied(credp, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		if (dsp->shm_lkcnt++ == 0) {
			ASSERT(kshmp->kshm_asp == NULL);
			if (error = shm_lockmem(kshmp)) {
				/* 
				 *+ Could not lock the memory in core.
				 *+ Print a note and continue.
				 */
				cmn_err(CE_NOTE,
				  "shmctl - couldn't lock %d pages into memory",
				   memfs_map_size(kshmp->kshm_mvp));
				error = ENOMEM;
				--dsp->shm_lkcnt;
			}
		}
		break;

	case SHM_UNLOCK:	/* Unlock segment */
		/*
		 * The access check is done before the P_SYSOPS privilege
		 * check, since return must be EINVAL on MAC failure,
		 * regardless of P_SYSOPS privilege (which could possibly
		 * return EPERM).
		 */
		if (error = ipcaccess(&dsp->shm_perm, SHM_R|SHM_W, IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif

		if (pm_denied(credp, P_SYSOPS)) {
			error = EPERM;
			break;
		}

		if (dsp->shm_lkcnt && (--dsp->shm_lkcnt == 0)) {
			if (!(kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY)) {
				ASSERT(kshmp->kshm_asp);
				shm_unlockmem(&kshmp->kshm_asp);
			}
		}
		break;

	case SHM_SETPLACE:	/* Change shared memory placement policy */
		/*
		 * The access check is done before the P_OWNER privilege
		 * check, since return must be EINVAL on MAC failure,
		 * regardless of P_OWNER privilege (which could possibly
		 * return EPERM).
		 */
		if (error = ipcaccess(&dsp->shm_perm, SHM_R|SHM_W, IPC_MAC, credp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(dsp,MAC_SAME);
#endif

		/* must have ownership */
		if (credp->cr_uid != dsp->shm_perm.uid
		  && credp->cr_uid != dsp->shm_perm.cuid
		  && pm_denied(credp, P_OWNER)) {
			error = EPERM;
			break;
		}

		if (!(kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY)) {
			error = EINVAL;
			break;
		}

		if (copyin(uap->arg, &ds, sizeof(ds))) {
			error = EFAULT;
			break;
		}

		error = fgashm_reaffine(kshmp, &ds);
		break;

	default:
		error = EINVAL;
		break;
	}

	SHMID_LOCK(kshmp);
	kshmp->kshm_flag &= ~SHM_BUSY;
	SHMID_UNLOCK(kshmp, PLBASE);

	if (SV_BLKD(&kshmp->kshm_sv)) 
		SV_BROADCAST(&kshmp->kshm_sv, 0);
        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	ADT_LIDCHECK(lid);

	return error;
}

/*
 * various shmsys() parameters
 */
#define	SHMAT	0
#define	SHMCTL	1
#define	SHMDT	2
#define	SHMGET	3

/*
 * int 
 * shmsys(uap, rvp)
 * 	System entry point for shmat, shmctl, shmdt, and shmget system calls.
 *
 * Calling/Exit State:
 *	Should be called and returned at PLBASE.
 */
int
shmsys(struct shmsysa *uap, rval_t *rvp)
{
	int error;
	ASSERT(getpl() == PLBASE);

	switch (uap->opcode) {
	case SHMAT:
		error = shmat((struct shmata *)uap, rvp);
		break;
	case SHMCTL:
		error = shmctl((struct shmctla *)uap, rvp);
		break;
	case SHMDT:
		error = shmdt((struct shmdta *)uap, rvp);
		break;
	case SHMGET:
		error = shmget((struct shmgeta *)uap, rvp);
		break;
	default:
		error = EINVAL;
		break;
	}

	ASSERT(getpl() == PLBASE);
	return error;
}

/* 
 * void
 * shmfork(proc_t *ppp, proc_t *cpp)
 * 	Duplicate parents, ppp, segacct records in child, cpp.
 *
 * Calling/Exit State:
 *	The caller should hold address space reader lock of ppp and
 *	make sure that no external accesses to cpp address space are possible.
 *	The callers should be prepared to block.
 */
void
shmfork(proc_t *ppp, proc_t *cpp)
{
	segacct_t *sap = ppp->p_segacct;

	ASSERT(getpl() == PLBASE);

	while (sap != NULL) {
		switch(sap->sa_type){
		case DSHM:
			shm_add(cpp, sap->sa_addr, sap->sa_len, sap->sa_kdshmp, DSHM);
			/* increment for every shmat */
			DSHMID_LOCK(sap->sa_kdshmp);
			while (sap->sa_kdshmp->kdshm_flag & DSHM_BUSY) {
				SV_WAIT(&sap->sa_kdshmp->kdshm_sv, DSHM_PRI, &sap->sa_kdshmp->kdshm_lck);
				DSHMID_LOCK(sap->sa_kdshmp);
			}

			ASSERT(sap->sa_kdshmp->kdshm_refcnt != 0);
			++sap->sa_kdshmp->kdshm_refcnt;
			DSHMID_UNLOCK(sap->sa_kdshmp, PLBASE);
			break;
		case SHM:
			shm_add(cpp, sap->sa_addr, sap->sa_len, sap->sa_kshmp, SHM);
			/* increment for every shmat */
			SHMID_LOCK(sap->sa_kshmp);
			while (sap->sa_kshmp->kshm_flag & SHM_BUSY) {
				SV_WAIT(&sap->sa_kshmp->kshm_sv, SHM_PRI, &sap->sa_kshmp->kshm_lck);
				SHMID_LOCK(sap->sa_kshmp);
			}
			ASSERT(sap->sa_kshmp->kshm_refcnt != 0);
			++sap->sa_kshmp->kshm_refcnt;
			SHMID_UNLOCK(sap->sa_kshmp, PLBASE);
			break;
		}
		sap = sap->sa_next;
	}
}

/*
 * int 
 * shmdt(struct shmdta *uap, rval_t *rvp)
 * 	Detach shared memory segment system call.
 *
 * Calling/Exit State:
 *	The routine should be called in context at PLBASE without any lock held.
 *	The caller should prepare to block.
 *
 * Remarks:
 *	This is the only shmop that does not call shmconv, which will serialize
 *	all manipulations on the same shared memory object kshmid_ds[].
 *	Conseqently, we have to explicitly serialize with other accesses below.
 */
/* ARGSUSED */
STATIC int
shmdt(struct shmdta *uap, rval_t *rvp)
{
	segacct_t *sap;
	proc_t *pp = u.u_procp;
	struct as *asp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Does addr represent a shared memory segment currently attached
	 * to this address space?
	 */
	asp = pp->p_as;
	as_wrlock(asp);
	if (((sap = shm_find(pp, (vaddr_t)uap->addr)) == NULL) ||
	    sap->sa_type != SHM){
		as_unlock(asp);
		return EINVAL;
	}

	as_unlock(asp);

	return (kshmdt(sap, B_TRUE));
}

/*
 * int
 * kshmdt(segacct_t *sap, boolean_t unmap)
 *	detach the shared memory object descibed by sap from the current
 *	process.  If unmap is B_FALSE, the as is not modified (used by
 *	vfork child processes).
 *
 * Calling/Exit State:
 *	The routine should be called in context at PLBASE and no lock held.
 *	The caller should prepare to block.
 *	It will garner the following lock and release it before returning
 *	to its callers:
 *	- adress writer lock of the current process
 *	- kshm_lck for each cell
 */
STATIC int
kshmdt(segacct_t *sap, boolean_t unmap)
{
	proc_t *pp = u.u_procp;
	size_t len;
	struct kshmid_ds *kshmp;
	struct as *asp;
	vaddr_t addr;
	lid_t lid;

	ASSERT(sap != NULL);

	/* garner address writer lock */
	asp = pp->p_as;
	as_wrlock(asp);

	kshmp = sap->sa_kshmp;
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
	--pp->p_nshmseg;

	as_unlock(asp);

	SHMID_LOCK(kshmp);
	while (kshmp->kshm_flag & SHM_BUSY) {
		SV_WAIT(&kshmp->kshm_sv, SHM_PRI, &kshmp->kshm_lck);
		SHMID_LOCK(kshmp);
	}

	lid = kshmp->kshm_ds.shm_perm.ipc_secp->ipc_lid;
	/*
	 * don't set SHM_BUSY unless we have to drop
	 * the lock.
	 */
	if (--kshmp->kshm_refcnt == 0) {
		kshmp->kshm_flag |= SHM_BUSY;
		SHMID_UNLOCK(kshmp, PLBASE);
                /* decrement MAC levels reference */
                mac_rele(lid);
                FRIPCACL(kshmp->kshm_ds.shm_perm.ipc_secp);
		shm_rm_mvp(kshmp);

		SHMID_LOCK(kshmp);
		/* implicitly clears SHM_BUSY */
		shmdealloc((ipc_perm_t *)kshmp);
		SHMID_UNLOCK(kshmp, PLBASE);
		
		if (SV_BLKD(&kshmp->kshm_sv))
			SV_BROADCAST(&kshmp->kshm_sv, 0);
		
	        /* 
	         * Check if the Object Level Audit Criteria
	         * pertains to this event and lid.
	         */
		ADT_LIDCHECK(lid);

		return 0;
	}

	kshmp->kshm_ds.shm_dtime = hrestime.tv_sec;
	kshmp->kshm_ds.shm_lpid = pp->p_pidp->pid_id;

	SHMID_UNLOCK(kshmp, PLBASE);

        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	ADT_LIDCHECK(lid);
	return 0;
}

/*
 * void
 * shmexit(proc_t *pp)
 * 	detach all shared memory segments from process, pp.
 *
 * Calling/Exit State:
 *	The caller should not hold any lock calling this routine.
 *	The caller should make sure that there will be no other agent to shmat
 *	or shmdt to this address space.
 *	It will garner address space lock and release it before returning
 *	to its callers.
 */
void
shmexit(proc_t *pp)
{
	segacct_t *sap;

	/*
	 * p_segacct is guaranteed not to change behind us
	 *
	 * The sap passed to kshmdt will be removed from the link list,
	 * and unmapped from the as unless we are a vfork child.
	 */

	while ((sap = (segacct_t *)pp->p_segacct) != NULL)
		switch(sap->sa_type){
		case DSHM:
			kdshmdt(sap, !(pp->p_flag & P_VFORK));
			break;
		case SHM:
			kshmdt(sap, !(pp->p_flag & P_VFORK));
			break;
		}
}

/*
 * void
 * shmexec(proc_t *)
 *	Detach shared memory segments from process doing exec.
 * 	We may need to do something different for different platforms.
 *
 * Calling/Exit State:
 *	See shmexit.
 */
void
shmexec(proc_t *pp)
{
	shmexit(pp);
}

extern ppid_t pse_mmap(dev_t, off64_t, uint_t);
extern int segpse_create(struct seg *, void *);
extern int segvn_create(struct seg *seg, const void * const);
extern int as_map(struct as *, vaddr_t, u_int, int (*)(), void *);

/*
 * STATIC int
 * shmmmap(struct as *asp, struct kshmid_ds *kshmp, vaddr_t addr, size_t size, uchar_t prot)
 *	Map the shared memory segment represented by the specified vnode
 *	into the specified address space at the specified address, size
 *	and protections.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error code on failure.
 */
STATIC int
shmmmap(struct as *asp, struct kshmid_ds *kshmp, vaddr_t addr, size_t size, uchar_t prot)
{
	struct segvn_crargs vn_args;
	struct segpse_crargs pse_args;
	struct segdev_crargs dev_args;
	struct segfga_crargs fga_args;
	vnode_t *vp = kshmp->kshm_mvp;
	int error;

	if (kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY) {
		/*
		 * fga shm segment, identityless memory backed
		 */
		fga_args.fgap = kshmp->kshm_fgap;
		fga_args.prot = fga_args.maxprot = prot;

		error = as_map(asp, (vaddr_t) addr, size, segfga_create,
			&fga_args);
	} else if (vp->v_type != VCHR) {
		/*
		 * memfs based shm segment
		 */
		vn_args.vp = vp;
		vn_args.offset = 0;
		vn_args.cred = NULL;
		vn_args.type = MAP_SHARED;
		vn_args.prot = vn_args.maxprot = prot;
		error = as_map(asp, addr, size, segvn_create,
			&vn_args);
	} else if ((uint)addr & PSE_PAGEOFFSET) {
		/*
		 * pse-based shm segment with address not rounded to 4MB
		 *	=> use segdev
		 */
		dev_args.mapfunc =
			(ppid_t (*)(void *, channel_t, size_t, int))pse_mmap;
		dev_args.chan = (channel_t)vp->v_rdev;	/* gag */
		dev_args.idata = NULL;
		dev_args.cfgp = NULL;	/* gag */
		dev_args.offset = 0;
		dev_args.prot = dev_args.maxprot = prot;
		error = as_map(asp, addr, size, segdev_create, &dev_args);
	} else {
		/*
		 * pse-based shm segment with 4MB rounded address
		 */
		pse_args.offset = 0;
		pse_args.dev = vp->v_rdev;
		pse_args.prot = pse_args.maxprot = prot;
		error = as_map(asp, addr, size, segpse_create,
			&pse_args);
	}
	return error;
}

/*
 * STATIC int
 * shmaddr(struct kshmid_ds *kshmp, struct shmata *uap, vaddr_t *addrp,
 *		uint_t *sizep)
 *	Obtain valid address/size for attaching the specified shared
 *	memory segment to the current address space.
 *
 * Calling/Exit State:
 *	On both entry and exit, as lock is held in write mode.
 *	Returns 0 on success, error code on failure.
 *	On success, *addrp and *sizep contain the virtual address
 *		and size to be used for mapping the shared memory
 *		segment referenced by kshmp into the address space.
 */
STATIC int
shmaddr(struct kshmid_ds *kshmp, struct shmata *uap, vaddr_t *addrp,
		uint_t *sizep)
{
	vaddr_t addr, base;
	uint_t align, size, len, sadj, eadj;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;
	struct seg *segp;
	segacct_t *sap;

	size = kshmp->kshm_mapsize;
	addr = (vaddr_t)uap->addr;
	align = kshmp->kshm_align;

	ASSERT((align == PSE_PAGESIZE) || (align == PAGESIZE) ||
						(align == FGASHM_ALIGN));
	/*
	 * If the address is zero, then let the system pick the
	 *	attach address
	 */
	if (addr == 0) {

		/*
		 * Let the system pick the attach address
		 */
		map_addr(&addr, size + align - PAGESIZE, 0, 1);

		/*
		 * If couldn't find an address, *and* the segment is
		 * PSE-aligned, then try again with just page alignment
		 */
		if ((addr == NULL) && !(align & PSE_PAGEOFFSET) &&
		    !(kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY)) {
			align = PAGESIZE;
			size = roundup(kshmp->kshm_ds.shm_segsz, PAGESIZE);
			map_addr(&addr, size, 0, 1);
		}

		/*
		 * No available space in the address space
		 */
		if (addr == NULL)
			return ENOMEM;

		addr = roundup(addr, align);
		*addrp = addr;
		*sizep = size;
		return 0;
	}

	/*
	 * Non-zero address specified, so try to use this user-supplied
	 *	attach address
	 */
	if (uap->flag & SHM_RND) {
		if (kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY) {
			addr = (vaddr_t) ((ulong) addr & ~(FGASHM_ALIGN - 1));
		} else {
			addr = (vaddr_t)((ulong)addr & ~(SHMLBA - 1));
		}
	}

	/*
	 * If the underlying segment is PSE, but the requested address
	 * is not PSE-aligned, then conserve virtual by using a PAGESIZE
	 * aligned size.
	 */
	if ((align == PSE_PAGESIZE) && (((uint_t)addr) & PSE_PAGEOFFSET) &&
				!(kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY)) {
		size = roundup(kshmp->kshm_ds.shm_segsz, PAGESIZE);
		align = PAGESIZE;
	}

	base = addr;
	len = size;		/* use aligned size */

	/*
	 * Check that user-supplied address is page aligned as required.
	 */
	if ((uint)base & PAGEOFFSET)
		return EINVAL;
	else if ((kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY) &&
				((uint) base & (FGASHM_ALIGN - 1)))
		return EINVAL;

	/*
	 * Check that requested virtual address range is valid for
	 * user addresses.
	 */
	if (!VALID_USR_RANGE(base, len)) {
		if ((align != PSE_PAGESIZE) ||
				(kshmp->kshm_flag & SHM_SUPPORTS_AFFINITY)) {
			return EINVAL;
		}

		/*
		 * The user-specified range was aligned to
		 * PSE_PAGESIZE; see if aligning to PAGESIZE
		 * gives a valid user range.
		 */
		len = size = roundup(kshmp->kshm_ds.shm_segsz, PAGESIZE);
		if (!VALID_USR_RANGE(base, len))
			return EINVAL;
		align = PAGESIZE;
	}
			
	/*
	 * Now check for conflicts with existing mappings
	 */
	if (as_gap(asp, len, (vaddr_t *)&base, &len, AH_LO, (vaddr_t)NULL)
			== 0) {
		*addrp = addr;
		*sizep = size;
		return 0;
	}

	/*
	 * We now have a requested address range which is valid for
	 * user-level mappings, but which conflicts with existing
	 * mappings.  We will still allow the mapping if either:
	 *
	 *	The segment is a PSE-segment and has conflicts at
	 *	the end, and truncating it down to just page alignment
	 *	at the end eliminates these conflicts.
	 *
	 *	The segment conflicts with the end of a PSE-segment,
	 *	and truncating that other segment to page alignment
	 *	eliminates the conflict.  We can do this truncation
	 *	by unmapping the appropriate address range.
	 */

	/*
	 * Set eadj, the "end-adjustment", to indicate how much
	 * to reduce the segment size if using PAGESIZE alignment
	 * instead of PSE_PAGESIZE alignment.
	 */
	eadj = size - roundup(kshmp->kshm_ds.shm_segsz, PAGESIZE);

	/*
	 * Set sadj, the "start-adjustment," to indicate how much
	 * to truncate PSE segment whose end might conflict with
	 * the beginning of the requested address range.
	 *
 	 *	(1) Find the segment (if any) mapped at the base address
	 *		of the requested mapping.
	 *	(2) Find the shared memory segment (if any) corresponding
	 *		to the conflicting segment
	 *	(3) See if using PAGESIZE rounding for the conflicting segment
	 *		eliminates the conflict
	 *
	 * If these conditions are all met, then set sadj to the amount of
	 * the preceding segment to unmap.  Otherwise set sadj to 0.
	 */
	if (((segp = as_segat(asp, base)) == NULL) ||
			((sap = shm_find(pp, segp->s_base)) == NULL) ||
			( sap->sa_type != SHM ) ||
			((sap->sa_addr +
				roundup(sap->sa_kshmp->kshm_ds.shm_segsz,
				PAGESIZE)) >= base))
		sadj = 0;
	else
		sadj = sap->sa_kshmp->kshm_mapsize -
			roundup(sap->sa_kshmp->kshm_ds.shm_segsz, PAGESIZE);

	/*
	 * If we found no possible adjustments, then the conflicts
	 * found cannot be eliminated
	 */
	if ((sadj == 0) && (eadj == 0))
		return EINVAL;

	/*
	 * If there will still be conflicts after we apply the adjustments,
	 * then return EINVAL.
	 */
	base = addr + sadj;
	len = size - eadj;
	if (as_gap(asp, len, (vaddr_t *)&base, &len, AH_LO, (vaddr_t)NULL)
			!= 0)
		return EINVAL;

	/*
	 * See if there is a conflict in the beginning of the segment;
	 * if so, then unmap the overlapping portion of the preceding
	 * PSE segment found previously.
	 */
	if (sadj != 0) {
		base = addr;
		len = sadj;
		if (as_gap(asp, len, (vaddr_t *)&base, &len, AH_LO,
				(vaddr_t)NULL) != 0)
			as_unmap(asp, addr, sadj);
	}

	/*
	 * If we found that we could use PAGESIZE alignment instead of
	 * PSE_PAGESIZE alignment, and if doing so eliminates a conflict,
	 * then do it.
	 */
	if (eadj != 0) {
		base = addr + size - eadj;
		len = eadj;
		if (as_gap(asp, len, (vaddr_t *)&base, &len, AH_LO,
				(vaddr_t)NULL) != 0)
			size -= eadj;
	}

#ifdef	DEBUG
	base = addr;
	len = size;
	ASSERT(as_gap(asp, len, (vaddr_t *)&base, &len, AH_LO, (vaddr_t)NULL)
			== 0);
#endif

	*addrp = addr;
	*sizep = size;
	return 0;
}

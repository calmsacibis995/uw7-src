#ident	"@(#)kern-i386:mem/seg_umap.c	1.4.5.6"

/*
 * segumap is a user level segment driver for mapping
 * shared memory segments with dynamic mappings. [DSHM]
 *
 * Dynamic mapping is a method of mapping pages of a large memory 
 * object into a smaller virtual address range.
 *
 * Access to the object data is protected by the DSHM lock in the ipc code
 * during creation. Subsequently a portion of the global data ( PFNs ) is 
 * read only and requires no locking. 
 * The object is protected from being deleted by 
 * DSHM reference counting at the IPC level, as we are guaranteed to
 * be single threaded we won't be deleting at the IPC level while
 * mapping/unmapping. Operations which remove the segment wait
 * for async IO to complete. 
 *
 * remaps & softlocks are serialized via the VPAGE spin locks.
 *
 *
 */
#include <acc/priv/privilege.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/seg.h>
#include <mem/seg_umap.h>
#include <mem/vmparam.h>
#include <mem/rzbm.h>
#include <proc/cg.h>
#include <proc/proc.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <proc/ipc/dshm.h>
#include <proc/cg.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/map.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef PAE_MODE

/*
 * Stubs for non-PAE kernels. Typically only used for
 * kstuff'ed kernels.
 */

int segumap_create(struct seg *seg, void *argsp) { return ENOSYS; }
int segumap_create_obj(struct kdshmid_ds *kdp) { return EUNATCH; }
void segumap_delete_obj(struct kdshmid_ds *kdp) { return; }
int segumap_load_map(const int dshmid, const vaddr_t uvaddr,
		     const ulong_t bufidx){ return 0; }
int segumap_unload_map(const int dshmid, vaddr_t uvaddr){ return 0; }
int segumap_alloc_mem(struct kdshmid_ds *kdp, struct dshmid_ds *udsp,
		      unsigned int flag){ return 0; }
int segumap_ctl_memloc(struct kdshmid_ds *kdp, ulong_t idx,
		       ulong_t len, cgid_t *vec){ return 0; }

struct seg_ops segumap_ops;

#else /* PAE_MODE */

STATIC LKINFO_DECL(dshmvpg_lks, "MS::dshmvp_lck", 0);

/*
 * Private seg op routines.
 */
STATIC int segumap_dup(struct seg *, struct seg *);
STATIC int segumap_unmap(struct seg *, vaddr_t, uint_t);
STATIC void segumap_free(struct seg *);
STATIC faultcode_t segumap_fault(struct seg *, vaddr_t, uint_t,
				 enum fault_type, enum seg_rw);
STATIC int segumap_setprot(struct seg *, vaddr_t, uint_t, uint_t);
STATIC int segumap_checkprot(struct seg *, vaddr_t, uint_t);
STATIC int segumap_incore(struct seg *, vaddr_t, uint_t, char *);
STATIC int segumap_getprot(struct seg *, vaddr_t, uint_t *);
STATIC off64_t segumap_getoffset(struct seg *, vaddr_t);
STATIC int segumap_gettype(struct seg *, vaddr_t);
STATIC int segumap_getvp(struct seg *, vaddr_t, vnode_t **);
STATIC void segumap_badop(void);
STATIC int segumap_nop(void);
STATIC void segumap_age(struct seg *, u_int);
STATIC int segumap_memory(struct seg *, vaddr_t *basep, u_int *lenp);
STATIC void segumap_alloc_vpages(struct kdshmid_ds *, cgnum_t);
STATIC void segumap_alloc_pdb(struct kdshmid_ds *, cgnum_t );
STATIC int segumap_alloc_kmem(struct kdshmid_ds *kdp,
			      struct dshmid_ds *udsp, unsigned int flag);
STATIC int segumap_memloc(struct seg *seg, vaddr_t addr, uint_t len, cgid_t
			 *vec); 

/*
 * NOT static because needed by as_aio_needs_prep(), as_aio_prep(), and
 * as_aio_unprep().
 */
struct seg_ops segumap_ops = {
	segumap_unmap,
	segumap_free,
	segumap_fault,
	segumap_setprot,
	segumap_checkprot,
	(int (*)())segumap_badop,	/* kluster */
	(int (*)())segumap_nop,		/* sync */
	segumap_incore,
	(int (*)())segumap_nop,		/* lockop */
	segumap_dup,
	(void(*)())segumap_nop,		/* childload */
	segumap_getprot,
	segumap_getoffset,
	segumap_gettype,
	segumap_getvp,
	segumap_age,			/* age */
	(boolean_t (*)())segumap_nop,	/* lazy_shootdown */
	segumap_memory,
	(boolean_t(*)())segumap_nop,    /* xlat_op */
	(int(*)())segumap_memloc,    	/* memloc */
};

/*
 * asm int
 * __log_base_2(int value)
 *	returns the log to base 2 of value
 * Calling/Exit State:
 *	No locks held on entry
 *
 * Remarks:
 * 	value is an exact power of 2
 */
asm int
__log_base_2(int value)
{
%mem    value;

        / bit scan forward is an easy way to do log to base 2
        bsfl value, %eax;
}
#pragma asm partial_optimization __log_base_2

/*
 * int
 * segumap_create_obj(struct kdshmid_ds *kdp)
 *	Create a DSHM memory object.
 *
 * Calling/Exit State:
 *	No spin locks held on entry
 *	kdshmid_ds lock held ( DSHM_BUSY ) on entry
 *	returns the same way.
 *
 * Remarks:
 *	This includes allocating the global vpage array,
 *	The array to hold the page frame numbers & the identityless
 *	pages themselves
 *	As an optimization, constants used later by the segment driver
 *	are calculated up front and stored in the vpage directory structure
 *
 */
int
segumap_create_obj(struct kdshmid_ds *kdp)
{
	dshm_vp_dir_t 	*vpdir;
	dshm_pfn_dir_t	*pfndp;
	ulong_t	j;
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(kdp->dmapsize != 0);
	ASSERT(kdp->dbuffersz != 0);
	ASSERT(kdp->dtotalbufs != 0);
	
	kdp->kdshm_objsz = ptob64(btopr64((ullong_t)kdp->dbuffersz
					  * (ullong_t)kdp->dtotalbufs));

	if( idf_resv(btop64(kdp->kdshm_objsz), NOSLEEP) == 0 )
		return ENOMEM;
	if( (kdp->kdshm_hatp = shpt_hat_init(kdp->dmapsize))==NULL){
		idf_unresv(btop64(kdp->kdshm_objsz));
		return ENODEV;
	}


	vpdir = kdp->kdshm_vpage = kmem_zalloc(sizeof(dshm_vp_dir_t),KM_SLEEP);
	vpdir->bufsize = kdp->dbuffersz;
	if( kdp->dbuffersz <= PAGESIZE ) {
		vpdir->vpagesize = PAGESIZE;
		vpdir->vpageshift = PAGESHIFT;
		vpdir->vpageoffset =  PAGEOFFSET;
		vpdir->vpagemask =  PAGEMASK;
		vpdir->pgsperslot =  1;
		vpdir->bufsperpage = PAGESIZE / kdp->dbuffersz;
		vpdir->bufshift = PAGESHIFT - __log_base_2(kdp->dbuffersz);
	} else {
		vpdir->bufsperpage = (uint_t)-1;
		vpdir->vpagesize = kdp->dbuffersz;
		vpdir->vpageshift = __log_base_2(kdp->dbuffersz);
		vpdir->vpageoffset = vpdir->vpagesize - 1;
		vpdir->vpagemask = ~vpdir->vpageoffset;
		vpdir->pgsperslot = kdp->dbuffersz / PAGESIZE;
                vpdir->bufshift = __log_base_2(kdp->dbuffersz) - PAGESHIFT;
	}
	vpdir->vpc =  kdp->dmapsize / vpdir->vpagesize;
	ASSERT(kdp->dmapsize % vpdir->vpagesize == 0 );

	kdp->kdshm_pfn = NULL;

	if( Ncg == 1) {
		/*
		 * allocate all the memory now if only 1 cg
		 */
		segumap_alloc_vpages(kdp, 0);
		segumap_alloc_pdb(kdp, 0);
	
		pfndp = kdp->kdshm_pfn;
		/* Allocate the identityless pages */
		for(j=0; j < pfndp->nent; j ++ ){
			SUMAP_BUFFER(pfndp,j) = idf_page_get(0,PAGESIZE);
			ASSERT(SUMAP_BUFFER(pfndp,j) != 0);
		}
	}
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	return 0;
}

/*
 * void
 * segumap_delete_obj(struct kdshmid_ds *kdp)
 *	Delete a DSHM memory object.
 *
 * Calling/Exit State:
 *	No spin locks held on entry
 *	kdshmid_ds lock held ( DSHM_BUSY ) on entry
 *	returns the same way.
 *
 * Remarks:
 *	Return all identityless memory to the allocator
 *	deinit all the locks and return memory to kma
 *
 */
void
segumap_delete_obj(struct kdshmid_ds *kdp)
{
        dshm_vp_dir_t	*vpdirp = kdp->kdshm_vpage;
        dshm_pfn_dir_t 	*pfndp = kdp->kdshm_pfn;
        ulong_t 	i, no_L1s;
  
	ASSERT(kdp != NULL);
	ASSERT( kdp->kdshm_flag & DSHM_BUSY );

#ifndef CCNUMA
	ASSERT(pfndp != NULL);
#endif	
	if( pfndp != NULL) {
		/*
		 * Return the identityless memory
		 */
		for (i=0; i < pfndp->nent; i ++ ){
			if (SUMAP_BUFFER(pfndp, i) != 0)
				idf_page_free(SUMAP_BUFFER(pfndp,i), PAGESIZE);
		}
		/*
		 * release the pfns, and the pfn directory
		 */
		no_L1s = SUMAP_PFNL1_COUNT(pfndp);
		for (i=0; i < no_L1s; i++ ){
			kmem_free(pfndp->pdb[i], SUMAP_PFN_CHUNK_SZ);
		}
		kmem_free(pfndp->pdb, sizeof(*pfndp->pdb) * no_L1s);
		kmem_free(pfndp,sizeof(dshm_pfn_dir_t));
	}
	
	/*
	 * Unreserve previously reserved memory
	 */
        idf_unresv((uint_t)btop64(kdp->kdshm_objsz));

#ifndef CCNUMA
	ASSERT(vpdirp->vpp != NULL);
#endif	
	if( vpdirp->vpp != NULL ) {
		/*
		 * deinit the vpage locks & free the memory
		 * used by the locks
		 */
		for(i=0; i< SUMAP_VP_BUCKETS; i++){
			LOCK_DEINIT(&vpdirp->vlockp[i]);
		}
		kmem_free(vpdirp->vlockp,
			  SUMAP_VP_BUCKETS * sizeof(*vpdirp->vlockp));

		/*
		 * free the vpages
		 */
#ifdef DEBUG
		/*
		 * assert no softlocks held
		 */
		for(i=0;i<vpdirp->vpc;i++)
			ASSERT(vpdirp->vpp[i >> SUMAP_VPSHIFT][i & SUMAP_VPOFFSET].lockcnt == 0 );
#endif		
		no_L1s = SUMAP_VPL1_COUNT(vpdirp);
		for(i=0; i < no_L1s; i++ )
			kmem_free(vpdirp->vpp[i], SUMAP_VP_CHUNK_SZ);
  
		kmem_free(vpdirp->vpp, no_L1s * sizeof(*vpdirp->vpp));
	}
	kmem_free(vpdirp, sizeof(dshm_vp_dir_t));
	/*
	 * inform the HAT that we're done with the shared L2s
	 */
        shpt_hat_deinit(kdp->kdshm_hatp);
}

/*
 * STATIC void
 * segumap_alloc_vpages(struct kdshmid_ds *kdp, cgnum_t cgnum)
 *	Allocate memory for the vpages & locks referenced by
 *	kdp->kdshm_vpage on CG <cgnum>
 *
 * Calling/Exit State:
 *	No locks held on entry, returns same.
 *	Callers should be prepared to block
 *
 * Remarks:
 *
 */
STATIC void
segumap_alloc_vpages(struct kdshmid_ds *kdp, cgnum_t cgnum)
{
	dshm_vp_dir_t *vpgdp = kdp->kdshm_vpage;
	ulong_t	i, no_L1s;
	
	/*
	 * calculate the number of SUMAP_VP_CHUNK_SZ chunks
	 * needed to hold the vpages
	 *
	 * We're using fixed sized chunks for the vpages,
	 * instead of variable for fast lookup
	 * Each chunk holds (SUMAP_VP_CHUNK_SZ/
	 *		     sizeof(segumap_vpage_t) vpages
	 */
	
	no_L1s = SUMAP_VPL1_COUNT(vpgdp);
	
	/*
	 * array of pointers to vpage chunks
	 */
	vpgdp->vpp = kmem_alloc( no_L1s * sizeof(*vpgdp->vpp), KM_SLEEP);

	for(i=0; i < no_L1s; i++) {
		/* chunk of vpages */
		vpgdp->vpp[i] = kmem_alloc_on_cg( SUMAP_VP_CHUNK_SZ,
						  KM_SLEEP,
						  cgnum );
		bzero(vpgdp->vpp[i], SUMAP_VP_CHUNK_SZ);
	}
	
	/*
	 * allocate space for the array of per vpage locks
	 * each lock covers a range of pages
	 * ( mapsize/pagesize * SUMAP_VP_BUCKETS )
	 */
	vpgdp->vlockp = kmem_alloc_on_cg( SUMAP_VP_BUCKETS
					  * sizeof(*vpgdp->vlockp),
					  KM_SLEEP,
					  cgnum );
	
	for(i=0; i < SUMAP_VP_BUCKETS; i++) {
		LOCK_INIT(&vpgdp->vlockp[i], VM_SEGUMAP_HIER,
			  VM_SEGUMAP_IPL, &dshmvpg_lks, KM_SLEEP);
	}	
}

/*
 * STATIC void
 * segumap_alloc_pdb(struct kdshmid_ds *kdp, cgnum_t cgnum)
 *	allocate memory for the page array referenced by <kdp>
 *	on CG <cgnum>
 *
 * Calling/Exit State:
 *	No locks held on entry, returns same.
 *	Callers should be prepared to block
 *
 * Remarks:
 *	Note this routine is only allocating storage for
 *	the ppids. The identityless pages are not
 *	allocated here.
 *
 */
STATIC void
segumap_alloc_pdb(struct kdshmid_ds *kdp, cgnum_t cgnum)
{
	dshm_pfn_dir_t *pfndp;
	ulong_t	i, no_L1s;
	
        /*
	 * Allocate space for the PFNs
	 *
	 * The number of entries ( nent ) is 
	 *  total buffer count if the buffer size is equal to the PAGESIZE
	 *  total buffer count * PAGES per buffer if buffer size is > PAGESIZE
	 *  total buffer count / buffers per page if buffer size is < PAGESIZE
	 */
	pfndp = kdp->kdshm_pfn =
		kmem_alloc_on_cg(sizeof(dshm_pfn_dir_t),KM_SLEEP, cgnum);

	pfndp->nent = kdp->kdshm_objsz / PAGESIZE;
	no_L1s = SUMAP_PFNL1_COUNT(pfndp);
	pfndp->pdb = kmem_alloc_on_cg(sizeof(*pfndp->pdb) *
				      no_L1s,
				      KM_SLEEP,
				      cgnum);

	bzero(pfndp->pdb, sizeof(*pfndp->pdb) * no_L1s);
		
	for(i=0; i < no_L1s; i++ ){
		pfndp->pdb[i] =	kmem_alloc_on_cg( SUMAP_PFN_CHUNK_SZ,
						  KM_SLEEP,
						  cgnum);
		bzero(pfndp->pdb[i], SUMAP_PFN_CHUNK_SZ);
	}		
}

/*
 * STATIC int
 * segumap_alloc_kmem(struct kdshmid_ds *kdp, struct dshmid_ds *udsp, unsigned int flag)
 *	allocate kernel memory for the DSHM object.
 *	as specified by flag..
 *		see segumap_alloc_mem below
 *	ccNUMA only
 *
 * Calling/Exit State:
 *	Callers shoud be prepared to block.
 *	No spinlocks held on entry, returns the same way.
 *	struct kdshmid_ds locked via DSHM_BUSY on entry
 *	further attaches prohibited by DSHM_AF_BUSY,
 *	returns same.
 * Remarks:
 *	udsp refers to the dshmid_ds copied in from user space
 *	and contains the affinity request
 */
STATIC int
segumap_alloc_kmem(struct kdshmid_ds *kdp, struct dshmid_ds *udsp, unsigned int flag)
{
#ifdef CCNUMA
	dshm_pfn_dir_t *pfndp;
	dshm_vp_dir_t *vpgdp;
	cgnum_t	cgnum;
	
	ASSERT( kdp != NULL );
	ASSERT( kdp->kdshm_flag & DSHM_BUSY);
	ASSERT( kdp->kdshm_objsz != 0 );
	
	vpgdp = kdp->kdshm_vpage;
	pfndp = kdp->kdshm_pfn;

	if( flag == SUMAP_KMEM_PRE ) {
		ASSERT( udsp != NULL );
		if(udsp->dshm_cg == CG_NONE || udsp->dshm_cg == CG_CURRENT) {
			cgnum = mycg;
		} else {
			if ((cgnum = cgid2cgnum(udsp->dshm_cg)) < 0)
				return EINVAL;
		}
	} else {
		cgnum = mycg;	
	}

/*
 * the current implementation waits for the first non _DSHM_PARTIAL
 * attach or the first dshmctl ( _DSHM_MAP_SETPLACE ) to allocate
 * the kernel virtual to hold the vpages and the pfn list
 * Note it doesn't allocate the identityless pages i.e. all
 * pfn entries are empty ( zero )
 *
 */
	if( vpgdp->vpp == NULL ) {
		segumap_alloc_vpages(kdp, cgnum);
	}
	
	if( pfndp == NULL) {
		segumap_alloc_pdb(kdp, cgnum);
	}
#endif	
	return 0;		
}


/*
 * int
 * segumap_alloc_mem(struct kdshmid_ds *kdp, struct dshmid_ds *udsp, unsigned int flags)
 *	allocate memory for the DSHM object referenced by kdp.
 *	as specified by flag & udsp. ( ccNUMA only )
 *	udsp refers to the dshmid_ds copied in from user space
 *   	possible values for flag..
 *   SUMAP_ALL allocate all remaining pages
 *	( identyless DSHM object pages + kernel virtual )
 *   SUMAP_OBJ allocate identityless object pages as specified in
 *	udsp->dshm_extent
 *   SUMAP_KMEM ensure sufficient kernel virtual to allow an attach to take
 *      place. i.e. vpages & pfn array ( pfn array may be empty )
 *   SUMAP_KMEM_PRE partially allocate kernel virtual for vpages &
 *      pfn array. i.e. pre attach
 *	as specified in udsp->dshm_extent ( currently ignored, all memory
 *	allocated )
 * Calling/Exit State:
 *	Callers shoud be prepared to block.
 *	On entry no spinlocks held, struct kdshmid_ds locked via DSHM_BUSY
 *	returns the same.
 * Remarks:
 *
 * The following syscalls may result in calls to segumap_alloc_mem
 * with the following values for flag..
 *
 *	  dshmctl( DSHM_SETPLACE ) flag = SUMAP_OBJ
 *	  dshmctl( DSHM_MAP_SETPLACE ) flag = SUMAP_KMEM_PRE
 *	  dshmat ( _DSHM_PARTIAL ) flag = SUMAP_KMEM
 *	  dshmat( ) flag = SUMAP_ALL
 *
 * The sequence of events is
 *	dshmget
 *	   reserve memory
 *
 *	multiple dshmctl calls as above
 *        SUMAP_KMEM_PRE is used to allocate memory prior to the
 *        first dshmat call for parts of the
 *        kernel managed data. multiple calls can be made
 *	  SUMAP_OBJ allocates identyless pages for specific buffers
 * 	 [Note a corresponding SUMAP_KMEM_PRE must be called prior to
 *	  SUMAP_OBJ to allocate kernel virtual to hold the pfns
 *	  allocated by SUMAP_OBJ.]
 *
 *	dshmat ( _DSHM_PARTIAL )
 *	    page tables, vpages & pfn array MUST exist
 *	    however the pfn array may be empty
 *	    a segumap_alloc_kmem SUMAP_KMEM call ensures this
 *
 *	dshmctl( DSHM_SETPLACE ) calls only (SUMAP_OBJ)
 *
 *	dshmdt()
 *
 *	possible further dshmctls ( DSHM_SETPLACE ) 
 *
 *	dshmat( ) ( no _DSHM_PARTIAL )
 *	    page tables, vpages & pfn array MUST exist
 *	    pfn array must be fully populated.
 *	    a segumap_alloc_kmem SUMAP_ALL call ensures this
 *
 *
 *	In practice the current implementation allocates all kernel
 *	memory for the vpages & pfn array on the first SUMAP_KMEM[_PRE]
 *	call. Further SUMAP_KMEM_PRE calls do nothing
 *
 */
int
segumap_alloc_mem(struct kdshmid_ds *kdp, struct dshmid_ds *udsp, unsigned int flag)
{
#ifdef CCNUMA
	dshm_vp_dir_t *vpgdp;
	ulong_t i;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT( kdp != NULL );
	ASSERT( kdp->kdshm_flag & DSHM_BUSY );
	ASSERT( kdp->kdshm_vpage != NULL);
	
	vpgdp = kdp->kdshm_vpage;
	
	/*
	 * Access via segumap_load_map from other LWPs
	 * to kdp->pfnd.pdb[][] is protected by DSHM_AF_BUSY
	 * i.e. no further attaches are allowed when DSHM_AF_BUSY is
	 * set. The library will have it attached, & will be able
	 * to load translations. If the library erroneously
	 * attempts to load a translation with no pfn,
	 * EAGAIN is returned.
	 *
	 * multiple affinity requests are serialised via DSHM_BUSY
	 * in the usual manner.
	 */
	switch(flag){
	/*
	 * Allocate all remaining pages
	 */ 
	case SUMAP_ALL:
	{
		dshm_pfn_dir_t **pfndpp;
		cgnum_t cgnum;
		ulong_t idx;
		uchar_t j;

		pfndpp = (dshm_pfn_dir_t **)&kdp->kdshm_pfn;
		
		/*
		 * allocate the kernel memory for the pfn array
		 * & vpages if not already allocated.  
		 * the pfn & vpage fields of kdp will be
		 * updated by segumap_alloc_kmem
		 * hence the ** type of pfndpp 
		 */
		if( (*pfndpp == NULL || vpgdp->vpp == NULL) &&
		    segumap_alloc_kmem(kdp, udsp, flag) !=0 ) {
			/*
			 *+ the pfn array was not already allocated
			 *+ and this allocation attempt failed
			 *+ this indicates a kernel software problem
			 *+ Note it is sometimes acceptable for
			 *+ segumap_alloc_kmem to fail, but not here.
			 */
				cmn_err(CE_PANIC, "segumap_alloc_mem kmem");
				/*NOTREACHED */
		}

		/*
		 * allocate page(s) from available cgs in a
		 * round robin fashion in buffer size units.
		 * i.e. stripe the buffers across online cgs.
		 */
		for(i=0, cgnum = 0; i < (*pfndpp)->nent; i+=vpgdp->pgsperslot){
			/*
			 * allocate n idf pages from cg cgnum
			 * where n = slotsize / PAGESIZE
			 */
			for (j=0, idx=i; j < vpgdp->pgsperslot &&
				     idx < (*pfndpp)->nent ; j++, idx++) {
				if (SUMAP_BUFFER((*pfndpp),idx) == 0) {
					SUMAP_BUFFER((*pfndpp),idx) =
						idf_page_get(cgnum, PAGESIZE);
				}
			}
			/* next cg; wrap if necessary */
			cgnum++;
			if(cgnum == Ncg)
				cgnum = 0;
		}
#ifdef DEBUG
		/* assert all pages allocated */
		for (i=0; i < (*pfndpp)->nent; i++) {
			ASSERT(SUMAP_BUFFER((*pfndpp),i) != 0);
		}
#endif		
		break;
	}
	
	/*
	 * Allocate identityless pages for DSHM object
	 * i.e. partially fill pfn[][]
	 */
	case SUMAP_OBJ:
	{
		dshm_pfn_dir_t *pfndp = kdp->kdshm_pfn;
		ulong_t b_idx, count;
		cgnum_t	cgnum;
		
		ASSERT( udsp != NULL );
		/*
		 * pfn array must be allocated first!!
		 */
		if(pfndp == NULL)
			return EAGAIN;
		
		/*
		 * convert buffers sized units to PAGESIZE
		 */
		b_idx = udsp->dshm_extent.de_buf.deb_index;
		count = udsp->dshm_extent.de_buf.deb_count;
			
		if( vpgdp->bufsize > PAGESIZE ) {
			b_idx <<= vpgdp->bufshift;
			count <<= vpgdp->bufshift;
		} else if ( vpgdp->bufsize < PAGESIZE ) {
			b_idx >>= vpgdp->bufshift;
			/* round up count before shifting */
			count += vpgdp->bufsperpage - 1;
			count >>= vpgdp->bufshift;
		}
		/*
		 * validate range
		 */
		if( b_idx + count > pfndp->nent)
			return EINVAL;
		
		if(udsp->dshm_cg == CG_NONE || udsp->dshm_cg == CG_CURRENT) {
			cgnum = mycg;
		} else {
			if((cgnum = cgid2cgnum(udsp->dshm_cg))< 0)
				return EINVAL;
		}
		/*
		 * for all specified buffers
		 * allocate a page from the specified CG
		 * unless the pfn L1 doesn't exist or
		 * it's already been allocated.
		 */
		for( i=0; i < count; i++, b_idx++){
			if(SUMAP_PFNL1ALLOCATED(pfndp, b_idx) &&
			   SUMAP_BUFFER(pfndp, b_idx) == 0) {
				SUMAP_BUFFER(pfndp, b_idx) =
					idf_page_get(cgnum, PAGESIZE);
				ASSERT(SUMAP_BUFFER(pfndp, b_idx) != 0);
			}
		}	
		break;
	}
	/*
	 * Allocate kernel virtual for vpages & pfn array
	 */	
	case SUMAP_KMEM:
	case SUMAP_KMEM_PRE:
		/*
		 * affinity request for kernel data
		 * i.e. page tables and pfn table
		 * page tables are replicated so do nothing for those
		 * if pfn array not already allocated then kmem_alloc
		 * it from appropriate cgs
		 */
		if( kdp->kdshm_pfn != NULL)
			break;
		
		return segumap_alloc_kmem(kdp, udsp, flag);

	default:
		/*
		 *+ An unknown flag was passed to us. 
		 *+ This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "segumap_alloc_mem");
		/* NOTREACHED */
	}
#endif
	return 0;
}

/*
 * int segumap_ctl_memloc(struct kdshmid_ds *, ulong_t, ulong_t, cgid_t *);
 *	Return location for <len> DSHM buffers  starting at buffer <idx>
 *
 * Calling/Exit State:
 *	On entry no spinlocks held, struct kdshmid_ds locked via DSHM_BUSY
 *	returns the same.
 * Remarks:
 *
 */
int
segumap_ctl_memloc(struct kdshmid_ds *kdp, ulong_t idx, ulong_t len, cgid_t *vec)
{
	dshm_pfn_dir_t *pfndp;
	ulong_t	i;
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT( kdp != NULL );
	ASSERT( kdp->kdshm_flag & DSHM_BUSY );
	ASSERT( idx + len <=  kdp->dtotalbufs );

#ifdef CCNUMA		
	pfndp = kdp->kdshm_pfn;

	if(pfndp == NULL)
		for( i=0; i< len; i++)
			vec[i] = CG_NONE;
	else
		for( i=0; i< len; i++, idx++)
			if(!SUMAP_PFNL1ALLOCATED(pfndp,idx) ||
			   !SUMAP_BUFFER(pfndp, idx))
				vec[i] = CG_NONE;
			else
				vec[i] = cgnum2cgid(pfntoCG(SUMAP_BUFFER(pfndp, idx)));
#else
	for( i=0; i< len; i++)
		vec[i] = cgnum2cgid(0);
#endif

	return 0;
}

/*
 * int
 * segumap_load_map(const int dshmid, const vaddr_t uvaddr, const ulong_t bufidx)
 *
 * 	Load translation(s) to buffer bufidx at address uvaddr.
 *	If the buffer size > PAGESIZE, multiple translations
 *	will be loaded.
 *
 * Calling/Exit State:
 *	No locks held on entry, returns the same way.
 *
 * Remarks:
 */
int
segumap_load_map(const int dshmid, const vaddr_t uvaddr, const ulong_t bufidx)
{
	pl_t opl;
	int i;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;
	struct seg *seg;
	struct segumap_data *sdp; 
	ppid_t pfn;
	ulong_t pfn_idx;
	vaddr_t addr;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Is there a segment attached at this address
	 */
        as_rdlock(asp);
	if((seg = as_segat(asp, uvaddr))== NULL) {
		as_unlock(asp);
		return EINVAL;
	}

	/*
	 * is this a DSHM segment ?
	 */
	if(!IS_SEGUMAP(seg)) {
		as_unlock(asp);
		return EINVAL;
	}

	sdp = seg->s_data;

	/*
	 * Validate dshmid.
	 * Note we don't use dshmconv. That would
	 * serialise remaps for all users of the dshm object
	 * We store the dshmid in the segment private area at
	 * attach time & rely on the fact that the dshmid object
	 * can't go away while we have the as_lock.
	 */
	if( dshmid != sdp->dshmid  ) {
		as_unlock(asp);
		return EINVAL;
	}
	
	/*
	 * Ensure uvaddr is vpage aligned
	 * vpagesize is a multiple of PAGESIZE
	 */
	if( uvaddr & SUMAP_VPAGEOFFSET(sdp) ) {
		as_unlock(asp);
		return EINVAL;
	}
	
	/*
	 * express pfn_idx in PAGESIZE pages ( offset into pfn array )
	 */

	pfn_idx = ( sdp->vpage->bufsize == PAGESIZE ) ? bufidx:
                (sdp->vpage->bufsize > PAGESIZE ) ?
                (bufidx << sdp->vpage->bufshift) :
                (bufidx >> sdp->vpage->bufshift);

	/*
	 * Buffer index out of bounds ?
	 */
	if( pfn_idx >= sdp->pfnd->nent ) {
		as_unlock(asp);
		return EINVAL;
	}
	
	addr = uvaddr - seg->s_base;
	opl = SUMAP_VPLOCK(sdp, addr);
	/*
	 * Fail if another LWP has softlocks
	 * This indicates an application error
	 */
	if( SUMAP_VPAGE(sdp,addr)->lockcnt != 0 ){
		SUMAP_VPUNLOCK(sdp, addr, opl);
		as_unlock(asp);
		return EAGAIN;
	}

#ifdef CCNUMA
	/*
	 * Assert pfn array exists ( doesn't matter if its empty )
	 *
	 * No attaches are allowed until it exists which is
	 * why we assert rather than test.
	 * 
	 */ 
	ASSERT( sdp->pfnd != NULL );
#endif

	/*
	 * Load a translation for PGSPERSLOT PAGESIZE pages in the buffer
	 */
	for(i=0; i < SUMAP_PGSPERSLOT(sdp); i++, pfn_idx++, addr += PAGESIZE) {
		pfn = SUMAP_BUFFER(sdp->pfnd, pfn_idx);
#ifdef CCNUMA
		/*
		 * check to see if idf page has been allocated
		 */
		if(!pfn) {
			while(i){
				/*
				 * if buffersize > pagesize only some pages
				 * may have been allocated we need to unload
				 * those translations already loaded before
				 * we return an error. 
				 */
				--i; addr -= PAGESIZE;
				shpt_hat_unload(asp, sdp->hatp, addr );
				TLBSflush1(seg->s_base + addr);
			}
			SUMAP_VPUNLOCK(sdp, (uvaddr - seg->s_base), opl);
			as_unlock(asp);
			return EAGAIN;
		}
#endif
		shpt_hat_load(asp, sdp->hatp, seg->s_base, addr, pfn,
			      sdp->prot );
	}

	SUMAP_VPUNLOCK(sdp, (uvaddr - seg->s_base), opl);
	as_unlock(asp);

	return 0;
}

/*
 * int
 * segumap_unload_map(const int dshmid, vaddr_t uvaddr)
 *
 * 	Unoad translation(s) for BUFFERSIZE pages for address uvaddr.
 *
 * Calling/Exit State:
 *	No locks held on entry, returns the same
 *
 * Remarks:
 */
int
segumap_unload_map(const int dshmid, vaddr_t uvaddr)
{
	pl_t opl;
	int i;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;
	struct seg *seg;
	struct segumap_data *sdp; 
	vaddr_t addr;
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Is there a segment attached at this address
	 */
        as_rdlock(asp);
	if((seg = as_segat(asp, uvaddr))== NULL) {
		as_unlock(asp);
		return EINVAL;
	}

	/*
	 * is this a DSHM segment ?
	 */
	if(!IS_SEGUMAP(seg)) {
		as_unlock(asp);
		return EINVAL;
	}

	sdp = seg->s_data;

	/*
	 * Validate dshmid.
	 * Note we don't use dshmconv. That would
	 * serialise remaps for all users of the dshm object
	 * We store the dshmid in the segment private area at
	 * attach time & rely on the fact that the dshmid object
	 * can't go away while we have the as_lock.
	 */
	if( dshmid != sdp->dshmid  ) {
		as_unlock(asp);
		return EINVAL;
	}

	/* round to vpagesize */
	uvaddr &= SUMAP_VPAGEMASK(sdp); 

	addr = uvaddr - seg->s_base;
	opl = SUMAP_VPLOCK(sdp, addr);
	/*
	 * Fail if another LWP has softlocks
	 * This indicates an application error
	 */
	if( SUMAP_VPAGE(sdp, addr)->lockcnt != 0 ){
		SUMAP_VPUNLOCK(sdp, addr, opl);
		as_unlock(asp);
		return EAGAIN;
	}
	
	/*
	 * unload translations for all pages
	 * mapped by the buffer
	 */
	for(i=0; i < SUMAP_PGSPERSLOT(sdp); i++, addr += PAGESIZE) {
		shpt_hat_unload(asp, sdp->hatp, addr );
		TLBSflush1(seg->s_base + addr);
	}
	
	SUMAP_VPUNLOCK(sdp, uvaddr - seg->s_base, opl);
	as_unlock(asp);

	return 0;
}

/*
 * int
 * segumap_create(struct seg *seg, void *argsp)
 *	Create a user segment for mapping DSHM memory.
 *
 * Calling/Exit State:
 *	Called with the AS write locked.
 *	Returns with the AS write locked.
 *
 * Description:
 *	Create a segumap type segment.
 *
 * Remarks:
 *	the segment private data contains
 *	references to the global DSHM data
 *
 *	Note also attach the shared L2s to the
 *	LWPs address space.
 */
int
segumap_create(struct seg *seg, void *argsp)
{
	struct segumap_data *sdp;
	struct segumap_crargs *a = argsp;

	ASSERT(seg->s_as != &kas);
	ASSERT((seg->s_base & PAGEOFFSET) == 0);
	ASSERT((seg->s_size & PAGEOFFSET) == 0);

	sdp = kmem_alloc(sizeof(struct segumap_data), KM_SLEEP);
	sdp->prot = a->prot;
	sdp->maxprot = a->maxprot;
	sdp->dshmid = a->dshmid;
	sdp->hatp = a->dshm_hatp;
	sdp->vpage = a->dshm_vpp;
	sdp->pfnd = a->dshm_pfn;
	
	seg->s_ops = &segumap_ops;
	seg->s_data = sdp;
	seg->s_as->a_isize += seg->s_size;
	shpt_hat_attach(seg->s_as, seg->s_base, seg->s_size, sdp->hatp, mycg);

	return 0;
}

/*
 * STATIC void
 * segumap_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */
STATIC void
segumap_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ segumap segment driver.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segumap_badop");
	/*NOTREACHED*/
}

/*
 * STATIC void
 * segumap_nop(void)
 *	Do-nothing operation.
 *
 * Calling/Exit State:
 *	Always returns success w/o doing anything.
 */
STATIC int
segumap_nop(void)
{
	return 0;
}

/*
 * STATIC int
 * segumap_dup(struct seg *umapg, struct seg *cseg)
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
 *	generic segment stucture points to the newly created segumap_data.
 *	On failure, non-zero is returned and indicates the appropriate
 *	errno.
 */
STATIC int
segumap_dup(struct seg *pseg, struct seg *cseg)
{
	struct segumap_data *psdp = pseg->s_data;
	struct segumap_crargs a;

	a.prot = psdp->prot;
	a.maxprot = psdp->maxprot;
	a.dshmid = psdp->dshmid;
	a.dshm_hatp = psdp->hatp;
	a.dshm_vpp = psdp->vpage;
	a.dshm_pfn = psdp->pfnd;	  

	return segumap_create(cseg, &a);
}

/*
 * STATIC int
 * segumap_unmap(struct seg *seg, vaddr_t addr, size_t len)
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
 *
 *	partial unmapping makes no sense for segumap and is prohibited
 */
STATIC int
segumap_unmap(struct seg *seg, vaddr_t addr, size_t len)
{
	struct segumap_data *sdp = seg->s_data;

	/*
	 * Check for bad sizes
	 */
	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
	    (len & PAGEOFFSET) || (addr & PAGEOFFSET)) {
		/*
		 *+ A request was made to unmap segumap segment addresses
		 *+ which are outside of the segment.  This indicates a
		 *+ kernel software problem.
		 */
		cmn_err(CE_PANIC, "segumap_unmap");
	}

	/*
	 * Check for entire segment
	 */
	if (addr == seg->s_base && len == seg->s_size) {
	  seg->s_as->a_isize -= seg->s_size;
	  shpt_hat_detach(seg->s_as, seg->s_base, seg->s_size, sdp->hatp);
	  seg_free(seg);

	  return 0;
	}

#ifdef DEBUG
	cmn_err(CE_WARN, "segumap_unmap partial unmapping requested");
#endif
	return EINVAL;
}

/*
 * STATIC void
 * segumap_free(struct seg *seg)
 *	Free a segment.
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusivley locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 */
STATIC void
segumap_free(struct seg *seg)
{
	struct segumap_data *sdp = seg->s_data;
	ASSERT(getpl() == PLBASE);
	ASSERT((seg->s_base & PAGEOFFSET) == 0);
	ASSERT((seg->s_size & PAGEOFFSET) == 0);

	kmem_free(sdp, sizeof(*sdp));
}

/*
 * STATIC faultcode_t
 * segumap_fault(struct seg *seg, vaddr_t addr, size_t len,
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
segumap_fault(struct seg *seg, vaddr_t addr, size_t len, enum fault_type type,
	     enum seg_rw rw)
{
	struct as *as = seg->s_as;
	struct segumap_data *sdp = seg->s_data; 
	segumap_vpage_t *vpp;
	pl_t 	opl;
	int 	j;
	vaddr_t seg_offset;
	
	switch(type) {
	case F_INVAL:
    /*
     * validity faults indicate an application error, because translations
     * for segumap segments are explicitly loaded by calls to dshm_remap
     * 
     */
		return FC_NOMAP; /* XXX find another return code */
	case F_PROT:
    /*
     * Since the seg_umap driver does not implement copy-on-write,
     * this means that a valid translation is already loaded,
     * but we got a fault trying to access the device.
     * Return an error here to prevent going in an endless
     * loop reloading the same translation...
     */
		return FC_PROT;
    /*
     * The only other faults we should get here are SOFTLOCK
     * and SOFTUNLOCK;
     */
	case F_MAXPROT_SOFTLOCK:
	case F_SOFTLOCK:

		for(seg_offset = (addr - seg->s_base) & SUMAP_VPAGEMASK(sdp),
			    len += (addr - seg->s_base);
		    seg_offset < len; seg_offset += SUMAP_VPAGESIZE(sdp)) {
			vpp = SUMAP_VPAGE(sdp, seg_offset);
			opl = SUMAP_VPLOCK(sdp, seg_offset);
			/*
			 * check for valid translation loaded 
			 * Note we're locking vpage size pages which
			 * may contain >1 PAGESIZE page
			 * However if any 1 PAGESIZE page has a valid translation
			 * they all will.
			 * Multiple softlocks are allowed, ergo there is no problem
			 * with multiple softlock requests to distinct ranges
			 * that lie within the same vpage.
			 *
			 * If we reach the max softlock count fail the operation
			 * we must do this to prevent deadlock.
			 */
			if((shpt_hat_xlat_info(as, sdp->hatp,
					       seg_offset + seg->s_base) &
			    HAT_XLAT_EXISTS) && vpp->lockcnt != SUMAP_MAXVPLOCK){
				/* Increment SOFTLOCK count, failing if at max. */
				vpp->lockcnt ++;
			} else {	
				/*	
				 * unlock those already locked  and return error
				 */
				int error;

				if(vpp->lockcnt == SUMAP_MAXVPLOCK)
					error = FC_MAKE_ERR(EAGAIN);
				else
					error = FC_NOMAP;
				
				SUMAP_VPUNLOCK(sdp, seg_offset, opl);
				for(j = (addr - seg->s_base) & SUMAP_VPAGEMASK(sdp);
				    j < seg_offset; j += SUMAP_VPAGESIZE(sdp)) {
					vpp = SUMAP_VPAGE(sdp, j);
					opl = SUMAP_VPLOCK(sdp, j);
					vpp->lockcnt--;
					SUMAP_VPUNLOCK(sdp, j, opl);
				}
				return error;
			}
			SUMAP_VPUNLOCK(sdp, seg_offset, opl);
		}	

		return 0;
		
	case F_SOFTUNLOCK:

		for(seg_offset = (addr - seg->s_base) & SUMAP_VPAGEMASK(sdp),
			    len += (addr - seg->s_base) ;
		    seg_offset < len; seg_offset += SUMAP_VPAGESIZE(sdp) ) {
			vpp = SUMAP_VPAGE(sdp, seg_offset);
			opl = SUMAP_VPLOCK(sdp, seg_offset);
			ASSERT( vpp->lockcnt > 0);
			vpp->lockcnt--;
			SUMAP_VPUNLOCK(sdp, seg_offset, opl);
		}
		
		return 0;
	default:
		return FC_PROT;
	}
}


/*
 * STATIC int
 * segumap_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
 *	Change the protections on a range of pages in the segment.
 *
 * Calling/Exit State:
 *	Called and exits with the address space exclusively locked.
 *
 *	Returns zero on success, returns a non-zero errno on failure.
 *
 * Remarks:
 *	protections on a per page basis make no sense for segumap
 * 	segments. Change the segment level protections.
 */
STATIC int
segumap_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
{
	struct segumap_data *sdp = seg->s_data;

	if ((sdp->maxprot & prot) != prot)
		return EACCES;		/* violated maxprot */

	if (sdp->prot != prot)
	  sdp->prot = (uchar_t)prot;

	return 0;
}

/*
 * STATIC int
 * segumap_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
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
segumap_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
{
	struct segumap_data *sdp = seg->s_data;

	ASSERT((addr & PAGEOFFSET) == 0);

	return (((sdp->prot & prot) != prot) ? EACCES : 0);

}

/*
 * STATIC int
 * segumap_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
 *	Return the protections on pages starting at addr for len.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	This function, which cannot fail, returns the permissions of the
 *	indicated pages in the protv array.
 */
STATIC int
segumap_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
{
 	struct segumap_data *sdp = seg->s_data;

	ASSERT((addr & PAGEOFFSET) == 0);

	*protv = sdp->prot;

	return 0;
}


/*
 * STATIC off64_t
 * segumap_getoffset(struct seg *seg, vaddr_t addr)
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
segumap_getoffset(struct seg *seg, vaddr_t addr)
{

	return (addr - seg->s_base);
}

/*
 * STATIC int
 * segumap_gettype(struct seg *seg, vaddr_t addr)
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
segumap_gettype(struct seg *seg, vaddr_t addr)
{
	return MAP_SHARED;
}

/*
 * STATIC int
 * segumap_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
 *	Return the vnode associated with the segment.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 * 	There is no vnode associated with this segment.
 *
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC int
segumap_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
{
	*vpp = (vnode_t *)NULL;
	return -1;
}

/*
 * STATIC int
 * segumap_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
 *	Return an indication, in the array, vec, of whether each page
 *	in the given range is "in core".
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	"Pages" for segumap are always "in core"
 *	return an indication of whether the pages from the object
 *	have mappings in the given range.
 */
/*ARGSUSED*/
STATIC int
segumap_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
{
	struct segumap_data *sdp = seg->s_data;
	const vaddr_t ea = addr + len;

	ASSERT(seg->s_base + seg->s_size >= ea );

	for( addr = addr & PAGEMASK; addr < ea ; addr += PAGESIZE ){
		if( shpt_hat_xlat_info(seg->s_as, sdp->hatp, addr) &
		    HAT_XLAT_EXISTS )
			*vec++ = 1;
		else
			*vec++ = 0;
	}

	return (int)len;
}


/*
 * STATIC void
 * segumap_age(struct seg *, u_int type)
 *	Age the translations for a segumap segment.
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
segumap_age(struct seg * seg, u_int type)
{

}

/*
 * STATIC int
 * segumap_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
 *	This is a no-op for segumap.
 *
 * Calling/Exit State:
 *	returns ENOMEM.
 */
/*ARGSUSED*/
STATIC int
segumap_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
{
	return ENOMEM;
}

/*
 * STATIC int
 * segumap_memloc(struct seg *seg, vaddr_t addr, uint_t len, cgid_t *vec)
 *
 * 	Determine the primary memory residency of the pages starting
 *	at `addr' for `len' bytes. The results are returned in the cgid_t
 *	array `vec.'
 *
 * Calling/Exit State:
 *	Called with AS read locked and returns the same way.
 *
 *	Returns its own argument `len' and does not have a notion of success
 *	or failure.
 *
 * Remarks:
 *
 * 	Current implementation grossly inefficient when
 *	buffersize > PAGESIZE. ( acquires & releases the same spin lock
 *	& makes redundant calls to shpt_hat_xlat_info )
 *	however this interface is not the specified route for dshm_memloc
 *	[should be using dshmctl, which takes no locks to get to the cg]
 *	
 */
STATIC int
segumap_memloc(struct seg *seg, vaddr_t addr, uint_t len, cgid_t *vec)
{
	struct as *as = seg->s_as;
	struct segumap_data *sdp = seg->s_data;
	uint_t	i;
	pl_t opl;
	vaddr_t ea;
	
	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);
	ea = len + addr;
	ASSERT( ea <= seg->s_base + seg->s_size);
	
	for(; addr < ea; addr += PAGESIZE ) {
		opl = SUMAP_VPLOCK(sdp, (addr - seg->s_base));
		if((shpt_hat_xlat_info(as, sdp->hatp, addr) &	
		    HAT_XLAT_EXISTS)) {
#ifdef CCNUMA			
			pte64_t *ptep = vtol2ptep64(addr);
			*vec++ = cgnum2cgid(pfntoCG(ptep->pgm.pg_pfn));
#else
			*vec++ = cgnum2cgid(0);
#endif			
		} else {
			*vec++ = CG_NONE;
		}
		SUMAP_VPUNLOCK(sdp, (addr - seg->s_base), opl);
	}

	return (int)len;
}		

#if defined(DEBUG) || defined(DEBUG_TOOLS)
void
print_segumap(const struct seg *seg)
{

        struct segumap_data *s = seg->s_data;

        debug_printf("Dump of SEGUMAP SEG (0x%lx)\n\n",
                     (ulong_t)seg);

        if (seg == NULL) {
                debug_printf("Seg pointer is NULL?\n\n");
                return;
        }	

        if(seg->s_ops != &segumap_ops){
                debug_printf("Not a segumap segment?\n\n");
                return;
        }
        debug_printf("\t\ts_base=%x s_size=%x s_as=%x\n",
                     seg->s_base, seg->s_size, seg->s_as);
        debug_printf("\t\tdshmid=%d hatp=%x vpage=%x\n",
                     s->dshmid, s->hatp, s->vpage);
        debug_printf("\t\tpfnd=%x bufsize=%d vlockp=%x\n",
                     s->pfnd,s->vpage->bufsize,s->vpage->vlockp);
        debug_printf("\t\tvpgspslt=%d vpagesize=%x vpagemask=%x\n",
                     s->vpage->pgsperslot,s->vpage->vpagesize,s->vpage->vpagemask);
        debug_printf("\t\tvpageshift=%d vpageoffset=%x bufshift=%d\n",
                     s->vpage->vpageshift,s->vpage->vpageoffset,s->vpage->bufshift);
        debug_printf("\t\tvpc=%x vpp=%x\n",s->vpage->vpc,s->vpage->vpp);
        debug_printf("\n\n");
}
#endif
#endif /* PAE_MODE */

#ifndef _MEM_SEG_UMAP_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_UMAP_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/seg_umap.h	1.1.2.2"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif


#ifdef _KERNEL_HEADERS

#include <util/ksynch.h> /* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h> /* REQUIRED */
#include <sys/types.h>	/* REQUIRED */
	
#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)
 
#define SUMAP_VP_CHUNK_SZ 32768 /* chunk of vpages */
#define SUMAP_PFN_CHUNK_SZ 32768 /* chunk of pfns */

#define SUMAP_VP_BUCKETS 128	/* number of locks protecting vpages */

extern struct seg_ops	segumap_ops;
	
#define IS_SEGUMAP(seg)	((seg)->s_ops == &segumap_ops)
/*
 * flags for ccnuma allocation of idf pages
 */
#define SUMAP_ALL	1	/* allocate all remaining pages idf + kmem*/
#define SUMAP_OBJ	2	/* allocate idf memory for object */
#define SUMAP_KMEM_PRE	3	/* allocate kernel memory pre 1st dshmat   */
#define SUMAP_KMEM	4      	/* allocate kernel data to allow attach */

/*
 * SUMAP_VPLOCK(sdp, addr)
 *
 * Acquire the spinlock for vpage referenced by addr in the
 * object referred to by sdp
 *
 * Notes:
 *
 * The vpage locks are held in an array.
 * Each vpage lock covers ( total vpages / SUMAP_VP_BUCKETS )
 * pages i.e. remaps to buffers within the same lock bucket will
 * be serialised.
 */
#define SUMAP_VPLOCK(sdp, addr) \
	LOCK(&(((sdp)->vpage->vlockp)\
		[((ulong_t)(addr) >> SUMAP_VPAGESHIFT((sdp))) \
	         % SUMAP_VP_BUCKETS]), VM_SEGUMAP_IPL)
/*
 * SUMAP_VPUNLOCK(sdp, addr)
 *
 * Release the spinlock for vpage referenced by addr in the
 * object referred to by sdp
 *
 */
#define SUMAP_VPUNLOCK(sdp, addr, pl) \
		UNLOCK(&(((sdp)->vpage->vlockp)[((ulong_t)(addr) >> \
			 SUMAP_VPAGESHIFT((sdp))) % SUMAP_VP_BUCKETS]), (pl))
		
/*
 * Constants used in buffer operations.
 * The following could be calculated on each use.
 * However as an optimisation they are calculated once and stored
 * along with the other vpage info in the vpage directory
 */	
#define SUMAP_VPAGESHIFT(sdp) 	((sdp)->vpage->vpageshift)
#define SUMAP_VPAGEOFFSET(sdp) 	((sdp)->vpage->vpageoffset)
#define SUMAP_VPAGESIZE(sdp)	((sdp)->vpage->vpagesize)
#define SUMAP_PGSPERSLOT(sdp) 	((sdp)->vpage->pgsperslot)
#define SUMAP_VPAGEMASK(sdp)	((sdp)->vpage->vpagemask)


/*
 * constants used to index the pfn table
 */
#define SUMAP_PFNSHIFT 13
#define SUMAP_PFNOFFSET 0x1fff
/*
 * constants used to index the vpage table
 */
#define SUMAP_VPSHIFT 15
#define SUMAP_VPOFFSET 0x7fff

/*
 * SUMAP_BUFFER(pfndp, idx)
 *
 * Return the ppid for the page at index idx in
 * the page directory referenced by pfndp
 *
 */
#define SUMAP_BUFFER(pfndp, idx) \
		((pfndp)->pdb[((ulong_t)(idx)) >> SUMAP_PFNSHIFT] \
		             [((ulong_t)(idx)) & SUMAP_PFNOFFSET])
/*
 * SUMAP_PFNL1ALLOCATED(pfndp, idx)
 *
 * Return non-zero if there is an L1 allocated
 * for the page at offset idx in the
 * page directory referenced by pfndp
 * 
 */
#define	SUMAP_PFNL1ALLOCATED(pfndp, idx) \
		((pfndp) && (pfndp)->pdb && \
		 (pfndp)->pdb[((ulong_t)(idx)) >> SUMAP_PFNSHIFT])
/*
 * SUMAP_PFNL1_COUNT(pfndp)
 *
 * Return the number of L1s allocated for the page directory pfndp
 */
#define SUMAP_PFNL1_COUNT(pfndp) ( 1 + ((pfndp)->nent >> SUMAP_PFNSHIFT))

/*
 * SUMAP_VPL1_COUNT(vpgdp)
 *
 * Return the number of L1s allocated for the vpages
 */		
#define SUMAP_VPL1_COUNT(vpgdp) ( 1 + ((vpgdp)->vpc >> SUMAP_VPSHIFT))

/*
 * SUMAP_VPAGE(sdp, addr)
 *
 * Return a pointer to the vpage referenced by addr in
 * sdp segumap_data.
 *
 */
#define SUMAP_VPAGE(sdp, addr) \
	( *((sdp)->vpage->vpp + \
	    (((ulong_t)(addr) >> SUMAP_VPAGESHIFT(sdp)) >> SUMAP_VPSHIFT))\
	  + (((ulong_t)(addr) >> SUMAP_VPAGESHIFT(sdp)) & SUMAP_VPOFFSET))
/*
 * Per-page data.
 *
 * The offset into the vpage table depends
 * on the size of this structure
 * SUMAP_VPSHIFT & SUMAP_VPOFFSET DEPEND ON THE SIZE OF segumap_page
 */
typedef struct segumap_vpage {
	unsigned char	lockcnt;	/* softlock count */ 
} segumap_vpage_t;
#define SUMAP_MAXVPLOCK 255	/* max softlock count */

/*
 * This structure contains the PFNs that make up the DSHM object
 *
 * So as not to allocate > 64K from kma
 * the pfns are held in a multi level table
 * *pdb points to an array of pointers
 * each pointer points to a chunk of pages
 * ( SUMAP_PFN_CHUNK_SZ / sizeof ppid )
 * The offset into the pfn array depends on the
 * size of *pdb ( i.e. ppid_t )
 */
typedef struct dshm_pfn_dir {
	ulong_t	nent; /* number of pages */
	ppid_t **pdb; /* L1 ptr	*/
} dshm_pfn_dir_t;

/*
 * The VPAGE directory
 *
 * contains buffer related constants computed at creation,
 * a pointer to the vpage locks and the vpages.
 * vpc is the number of vpages
 * vpp points to an array of pointers to vpages
 */
typedef struct dshm_vp_dir {
	lock_t 		*vlockp;	/* vpage locks 		*/
	ulong_t	   	vpc;		/* number of vpages 	*/
	segumap_vpage_t **vpp;		/* pointer to vpage L1 	*/
	ulong_t		vpagesize;	/* vpage constants...   */
	ulong_t		vpagemask;
	ulong_t		vpageshift;
	ulong_t		vpageoffset;
	uint_t		bufsize;	/* size of app's buffer */
	uchar_t		bufsperpage;	/* # buffers per PAGE */
	uchar_t		bufshift;	/* # bits to shift to convert */
                                        /* buffers to pages */
	uchar_t		pgsperslot;	/* # pages per mapslot */
} dshm_vp_dir_t;

#define DSHM_MINBUFSZ	32

/*
 * Structure whose pointer is passed to the segumap_create routine.
 */
struct segumap_crargs {
	uchar_t prot;		/* initial protection */
	uchar_t maxprot;	/* maximum protection */
	int	dshmid;		/* DSHM identifier; speeds dshmid validation*/
	void	*dshm_hatp;	/* pointer to dshm HAT private data */
	void 	*dshm_vpp; 	/* object's vpages */
  	void	*dshm_pfn;	/* object's idf pages */
};

struct hatshpt; /* required for segumap_data below */

/*
 * (Semi) private data maintained by the seg_umap driver per segment mapping.
 */
struct segumap_data {
	uchar_t prot;		/* current segment prot 		*/
	uchar_t maxprot;	/* maximum segment protections 		*/
	int	dshmid;		/* DSHM identifier, speeds validation	*/
	struct hatshpt *hatp;	/* pointer to dshm HAT private data	*/
	dshm_vp_dir_t *vpage;	/* per-page softlock counts 		*/
	dshm_pfn_dir_t *pfnd;	/* pfn directory 			*/
};
#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern int segumap_create(struct seg *, void *);
extern int segumap_load_map(const int, const vaddr_t, const ulong_t);
extern int segumap_unload_map(const int, vaddr_t);
struct kdshmid_ds;
struct dshmid_ds;
extern int segumap_create_obj(struct kdshmid_ds *);
extern void segumap_delete_obj(struct kdshmid_ds *);
extern int segumap_alloc_mem(struct kdshmid_ds *, struct dshmid_ds *,
			     unsigned int);
extern int segumap_ctl_memloc(struct kdshmid_ds *,
			      ulong_t, ulong_t, cgid_t *);
#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_SEG_UMAP_H */

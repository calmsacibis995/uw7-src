#ident	"@(#)kern-i386at:svc/sysdump.c	1.21.10.2"
#ident	"$Header$"

#include <fs/buf.h>
#include <fs/file.h>
#include <fs/specfs/snode.h> 	/* needed for the UNKNOWN_SIZE define */
#include <io/conf.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <proc/bind.h>
#include <proc/mman.h> 
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/kcore.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/locktest.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/mod_intr.h>
#include <util/mod/moddrv.h>
#include <util/plocal.h>
#include <io/autoconf/resmgr/resmgr.h>

#define	_DDI_C

#include <io/ddi.h>


extern int sleep();
extern void wakeup();

extern void modify_code(vaddr_t, vaddr_t);

extern void intnull();

extern void intr_undefer();

STATIC volatile buf_t *sysdump_bp;
STATIC ulong_t granularity;
STATIC ulong_t sysdump_buf_size;
STATIC vaddr_t sysdump_v;
STATIC vaddr_t zaddr;

STATIC uchar_t *sysdump_extmap;
STATIC ulong_t sysdump_extmap_size;
STATIC ppid_t sysdump_extmap_base;
STATIC ppid_t sysdump_extmap_end;

STATIC	struct _Compat_intr_idata	*dumpidatap;
extern	void *dumpcookie;
volatile	pl_t	pl_dump;
chan_handle_t dumpdev_handle;

#ifdef DEBUG
boolean_t sys_dump = B_FALSE;
#endif
extern	int sysdump_selective;
extern	int sysdump_poll;
extern  size_t  totalmem;
extern  paddr64_t  cr_kl1pt;


STATIC volatile boolean_t code_modified = B_FALSE;
STATIC boolean_t sysdump_init_done = B_FALSE;
STATIC volatile boolean_t sysdump_done = B_FALSE;

typedef struct {
	vaddr_t oldfunc;
	vaddr_t newfunc;
} mod_func_t;

STATIC pl_t sysdump_lock(lock_t *, pl_t);
STATIC void sysdump_unlock(lock_t *, pl_t);
STATIC void sysdump_void_return(void);
STATIC void sysdump_void_intr_run_return(void);
STATIC boolean_t sysdump_true_return(void);
 
/*
 * The following array consists of all the ksynch functions specified
 * in ddi that the drivers could call during disk i/o.
 */
static mod_func_t mod_funcs[] = {
#ifndef UNIPROC
#if defined DEBUG || defined SPINDEBUG
	{(vaddr_t)lock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)trylock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)unlock_dbg, (vaddr_t)sysdump_unlock},
	{(vaddr_t)rw_rdlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_wrlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_tryrdlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_trywrlock_dbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_unlock_dbg, (vaddr_t)sysdump_unlock},
	{(vaddr_t)lock_owned_dbg, (vaddr_t)sysdump_true_return},
	{(vaddr_t)rw_owned_dbg, (vaddr_t)sysdump_true_return},
#else
	{(vaddr_t)lock_nodbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)trylock_nodbg, (vaddr_t)sysdump_lock},
	{(vaddr_t)unlock_nodbg, (vaddr_t)sysdump_unlock},
	{(vaddr_t)rw_rdlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_wrlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_tryrdlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_trywrlock, (vaddr_t)sysdump_lock},
	{(vaddr_t)rw_unlock, (vaddr_t)sysdump_unlock},
#endif  /* DEBUG  */
#endif  /* UNIPROC */
	{(vaddr_t)sleep_lock, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sleep_trylock, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sleep_unlock, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sv_broadcast, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sv_signal, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sv_wait, (vaddr_t)sysdump_void_intr_run_return},
	{(vaddr_t)hier_lockcount, (vaddr_t)sysdump_true_return},
	{(vaddr_t)kbind, (vaddr_t)sysdump_void_return},
	{(vaddr_t)kunbind, (vaddr_t)sysdump_void_return},
	{(vaddr_t)sleep, (vaddr_t)sysdump_void_intr_run_return},
	{(vaddr_t)wakeup, (vaddr_t)sysdump_void_return},
	{(vaddr_t)intr_undefer, (vaddr_t)sysdump_void_return},
	{0, 0}
};


/*ARGSUSED*/
STATIC pl_t
sysdump_lock(lock_t *lck, pl_t pl)
{
	if(pl > pl_dump)
		spl(pl);
	return pl_dump;
}

/*ARGSUSED*/
STATIC void
sysdump_unlock(lock_t *lck, pl_t pl)
{
	spl(pl);
}

/*ARGSUSED*/
STATIC void
sysdump_void_return(void)
{
	return;
}

/*ARGSUSED*/
STATIC void
sysdump_void_intr_run_return(void)
{
	if (dumpcookie)
		cm_intr_call(dumpcookie);
}

/*ARGSUSED*/
STATIC boolean_t
sysdump_true_return(void)
{
	return B_TRUE;
}

#define DUMP_IO_SIZE	 8192	/* I/O size in bytes */
#define MZERO_GRANULARITY 512

#define EXTMAP_SIZE	 8192	/* size of the extent bit map */


/*
 * void sysdump_init(void)
 *	Open the dump device and pre-allocate a buffer.
 *
 * Calling/Exit State:
 *	Called from main. This function uses the sys_cred structure
 *	and thus this struct. should have been initialized by now.
 */
void
sysdump_init(void)
{
	extern dev_t dumpdev;
	extern cred_t  *sys_cred;
	bcb_t *bcbp;
	int err = 0;
	off64_t dumpsize;
	devsize_t dumpsize_b;
	chan_handle_t handle;
	channel_t chan;
	extern rm_key_t find_rmkey_on_dev();
	extern boolean_t find_basedev_on_rmkey();
	rm_key_t res_key;

	if (dumpdev == NODEV) {
		cmn_err(CE_WARN,
			"sysdump_init: no dump devices configured, no dump will be generated\n");
		return;
	}

	res_key = find_rmkey_on_dev(dumpdev);
	if (res_key != RM_NULL_KEY) {
		ulong_t basedev;
		find_basedev_on_rmkey(res_key,&basedev);
		chan = dumpdev - basedev;
		err = drv_open(res_key,&chan,FWRITE,sys_cred,&dumpdev_handle);
	}

	if (res_key == RM_NULL_KEY || err) {
		cmn_err(CE_WARN,
				"sysdump_init: cannot open dump device, no dump will be generated\n");
		return;
	}

	err = do_devinfo(dumpdev_handle,DI_WBCBP,&bcbp);
	if (err) {
		cmn_err(CE_WARN,
				"sysdump_init: cannot get dump devinfo, no dump will be generated\n");
		return;
	}
	if (bcbp == NULL){
		cmn_err(CE_WARN,
				"sysdump_init: bad dump devinfo, no dump will be generated\n");
		return;
	}

	sysdump_bp = getrbuf(KM_NOSLEEP);
	ASSERT(sysdump_bp);

	err = do_devinfo(dumpdev_handle, DI_SIZE, &dumpsize_b);
	dumpsize = (err == 0 ? dtob((off64_t) dumpsize_b.blkno) + dumpsize_b.blkoff :
					UNKNOWN_SIZE);

#if defined (DEBUG) || defined(DEBUG_SYSDUMP)
	cmn_err(CE_CONT,"dump dev size = %Ld bytes\n",dumpsize);
#endif /* DEBUG || DEBUG_SYSDUMP */

	if (dumpsize == UNKNOWN_SIZE)
		cmn_err(CE_NOTE,
			"sysdump_init: dump device size is unknown\n");
	else if(totalmem > dumpsize) {
		cmn_err(CE_NOTE,
			"sysdump_init: Memory size is larger than the dump device\n"
			"\tpanic dump will be selective\n");
		sysdump_selective = B_TRUE;
	}

	granularity = bcbp->bcb_granularity;
	sysdump_buf_size = min(DUMP_IO_SIZE, bcbp->bcb_max_xfer);
	sysdump_buf_size -= sysdump_buf_size % granularity;

	sysdump_bp->b_un.b_addr = kmem_alloc_physreq(sysdump_buf_size,
					bcbp->bcb_physreqp, KM_NOSLEEP);
	ASSERT(sysdump_bp->b_un.b_addr);

	sysdump_v = kpg_vm_alloc(btopr(sysdump_buf_size), NOSLEEP);
	ASSERT(sysdump_v);

	zaddr = (vaddr_t)kmem_zalloc(MZERO_GRANULARITY, KM_NOSLEEP);
	ASSERT(zaddr);

	sysdump_extmap_size = EXTMAP_SIZE;
	sysdump_extmap =  (uchar_t *)kmem_zalloc(sysdump_extmap_size, KM_NOSLEEP);
	ASSERT(sysdump_extmap);

	sysdump_init_done = B_TRUE;
}

/*
 * void 
 * set_extbit(ppid_t ppid)
 * 	set the bit corresponding to ppid in sysdump_extmap
 */
void 
set_extbit(ppid_t ppid)
{
	sysdump_extmap[ ((ppid)- sysdump_extmap_base) >> 3 ] |=
		1<<(((ppid)- sysdump_extmap_base)&0x7);
}
	

/*
 * uchar_t 
 * get_extbit(ppid_t ppid)
 * 	return the bit corresponding to ppid in sysdump_extmap
 */
uchar_t 
get_extbit(ppid_t ppid)
{
	return sysdump_extmap[ ((ppid)- sysdump_extmap_base) >> 3 ] &
		1<<(((ppid)- sysdump_extmap_base)&0x7);
}
	

/*
 * check if ppid is in the range covered by sysdump_extmap
 */
#define IN_EXTMAPRANGE(ppid) \
	(((ppid) >= sysdump_extmap_base)&&((ppid) <= sysdump_extmap_end))


/*
 * void sysdump_sync(void)
 *	When doing system dump, we make all other processors just idle.
 * 	This is because, we are modifying code that cannot be executed
 *	by other processors.
 *
 * Calling/Exit State:
 *	None
 */
/* ARGSUSED */
void
sysdump_sync(void *dummy)
{
	extern void *saveregs(kcontext_t *);
	int ena;

	DISABLE_PRMPT();
	saveregs(&u.u_kcontext);
	while (!code_modified)
		;
#ifdef DEBUG
	if (l.intr_depth) {
		int lvl = l.intr_depth;
		l.intr_depth = 0;
		spl0();
		l.intr_depth = lvl;
	}
#else
	splhi();
	/*
	 * discard deferred interrupts if we are using polled mode
	 */
	ena = intr_disable();
	if (dumpcookie) {
		dfrq_t          *dqp;
		dqp = l.deferqueue;
		while (dqp != NULL) {
			l.deferqueue = dqp->dq_next;
			dqp->dq_next = l.dqfree;
			l.dqfree = dqp;
			dqp = l.deferqueue;
			l.picipl = 0;
		}
	}
	intr_restore(ena);
	/* 
	 * pl_dump should have been set up by the panicing cpu 
	 * before calling modify_code
	 */
	spl(pl_dump);

#endif
	/*
	 * ZZZ KLUDGE - Non-panicking cpus may need to print state. ALSO, there
	 * is an open issue with sending EOIs to ASICS for unprocessed interrupts.
	 * If ANY cpu has received an interrupt from the dump device and has not 
	 * yet called ms_intr_complete, the dump will hang.
	 * The PSM currently sends EOIs in ms_show_state but this needs a 
	 * better resolution.
	 */
	ms_show_state();
	while (sysdump_done == B_FALSE)
		;
	spl0();
	/* NOTREACHED */
}

/*
 * int sysdump_write_buf(void *addr, off64_t *off, size_t sz)
 *	Writes the buffer to dumpdev in the given offset.
 *
 * Calling/Exit State:
 *	Returns -1 , if error.
 *	Returns 0, if success.
 */
STATIC int
sysdump_write_buf(void *addr, off64_t *off, size_t sz)
{
	uint_t	maj = getmajor(dumpdev);

#ifdef SYSDUMP_DEBUG2
	cmn_err(CE_CONT,"sysdump_write_buf: enter\n");
#endif

	while (sz) {
		if (CONSOLE_GETC() != -1) {
			/* Aborted by user */
			return -1;
		}
		sysdump_bp->b_flags = B_ASYNC;

		bcopy(addr, sysdump_bp->b_un.b_addr, min(sysdump_buf_size, sz));
		sysdump_bp->b_blkno = (daddr_t)btodt(*off);
		bioreset((buf_t *)sysdump_bp);

		sysdump_bp->b_bcount = sysdump_buf_size;
		if (sz < sysdump_buf_size) {
			if (sz % granularity != 0)
				sysdump_bp->b_bcount = (sz + granularity) & 
							~(granularity - 1);
			else
				sysdump_bp->b_bcount = sz;
		}

		*off += sysdump_bp->b_bcount;

#ifdef SYSDUMP_DEBUG2
	cmn_err(CE_CONT,"sysdump_write_buf: calling strategy\n");
#endif
		do_biostart(dumpdev_handle,(buf_t *)sysdump_bp);
#ifdef SYSDUMP_DEBUG2
	cmn_err(CE_CONT,"sysdump_write_buf: spinning\n");
#endif

		/*
		 * if we know the dumpcookie call the handler explicitly
		 * since the interrupt will have been turned off
		 * otherwise the interrupt will happen normally
		 */
		while ((sysdump_bp->b_flags & B_DONE) == 0) {
			if (dumpcookie) {
				pl_t	pl;

#ifdef NOTNOW
				pl = spldisk(); 
#endif
				(void)cm_intr_call(dumpcookie);
#ifdef NOTNOW
				splx(pl);
#endif
			}
		}

		if (geterror((buf_t *)sysdump_bp)) {
			cmn_err(CE_CONT, "^\n");
			cmn_err(CE_NOTE, "^I/O error during system dump.");
			return -1;
		}

		if (sz < sysdump_buf_size)
			break; 
		sz -= sysdump_buf_size;
	}
#ifdef SYSDUMP_DEBUG2
	cmn_err(CE_CONT,"sysdump_write_buf: exit\n");
#endif
	return 0;
}

STATIC int dot_print_count;
STATIC int hash_print_count;

/*
 * int sysdump_new_chunk(mreg_chunk_t *mchunk, off64_t *chunk_off,
 *		vaddr_t *base_addr)
 *	Writes the chunk header to dumpdev in offset chunk_off
 *	and writes all the image data belonging to the chunk.
 *
 * Calling/Exit State:
 *	Argument base_addr contains the physical address of all 
 *	image regions.
 *	The offset for the next chunk is set in outarg chunk_off.
 *	The mchunk is zeroed after it is written out.
 */
STATIC int
sysdump_new_chunk(mreg_chunk_t *mchunk, off64_t *chunk_off, vaddr_t *base_addr)
{
	int err = 0, num_mregs = 0;
	size_t sz, len;
	vaddr_t base, blkoff;
	int npages;
	extern void _watchdog_hook(void);

	err = sysdump_write_buf(mchunk, chunk_off, sizeof(mreg_chunk_t));
	if (err)
		return -1;

	while (num_mregs < NMREG &&
		(sz = MREG_LENGTH(mchunk->mc_mreg[num_mregs])) != 0) {

		if (MREG_TYPE(mchunk->mc_mreg[num_mregs]) != MREG_IMAGE) {
			num_mregs++;
			continue;
		}
		base = base_addr[num_mregs];
		while (sz) {
			npages = min(btopr(sysdump_buf_size), btopr(sz));
			hat_statpt_devload((vaddr_t)sysdump_v, npages,
				phystoppid(base), PROT_READ | PROT_WRITE);

			blkoff = base & (PAGESIZE - 1);
			len = min((npages * PAGESIZE) - blkoff, sz);

			err = sysdump_write_buf((void *)(sysdump_v + blkoff),
						chunk_off, len);
			if (err)
				return -1;

			sz -= len;

			hat_statpt_unload(sysdump_v, npages);
			TLBSflushtlb();
			if (err)
				return -1;

			base += len;

			if ((++dot_print_count & 0xf) == 0)
				cmn_err(CE_CONT, "^.");
			/*
			 * Indicate that the system is alive. Call all the
			 * registered handlers to reset the watchdog timer.
			 */
			_watchdog_hook();
		}
		num_mregs++;
	}

	bzero((caddr_t)mchunk, sizeof(mreg_chunk_t));
	mchunk->mc_magic = MRMAGIC;
	return 0;
}

#define      PFN(ptep)       ((ptep)->pgm.pg_pfn)
#define      PFN64(ptep)     ((ptep)->pgm.pg_pfn)


/*
 * pte_t *
 * kvtol2ptep(vaddr_t addr, engnum)
 *      Return pointer to level 2 pte for the given kernel virtual address.
 *
 * Description:
 *      If addr is in the range mapped by the per-engine level 2 page table,
 *      KVENG_L2PT, return a pointer into it.  Otherwise, return a pointer
 *      into the global set of level 2 pages.
 */
pte_t *
dkvtol2ptep(vaddr_t addr, int engnum)
{


        return (&((pte_t *)(ENGINE_PLOCAL_PTR((engnum))->
		kvpte[PDPTNDX((addr))]))[pfnum((vaddr_t)(addr))]);
}

pte64_t *
dkvtol2ptep64(vaddr_t addr, int engnum)
{
        return(&((pte64_t *)(ENGINE_PLOCAL_PTR((engnum))->
		kvpte64[PDPTNDX64((addr))]))[pae_pfndx((vaddr_t)(addr))]);
}


/*
 * pte_t *
 * dkvtol1ptep(vaddr_t addr,int engnum)
 *      Return pointer to level 1 pte for the given kernel virtual address.
 *      on engine
 *
 * Calling/Exit State:
 *      None.
 */

pte_t *
dkvtol1ptep(vaddr_t addr,int engnum)
{
        return (dkvtol2ptep((ENGINE_PLOCAL_PTR((engnum))->
		kvpte[PDPTNDX((addr))])
		+ (ptnum((addr)) << PNUMSHFT),engnum));
}


pte64_t *
dkvtol1ptep64(vaddr_t addr,int engnum)
{
        return (dkvtol2ptep64((ENGINE_PLOCAL_PTR((engnum))->
		kvpte64[PDPTNDX64((addr))]) + 
		(pae_ptnum((addr)) << PNUMSHFT),engnum));
}

/*
 * ppid_t
 * dkvtoppid32(caddr_t vaddr, int engnum)
 *      Return the physical page ID corresponding to the virtual
 *      address vaddr on engine. 
 */
ppid_t
dkvtoppid32(caddr_t vaddr, int engnum)
{
        pte_t *ptep;

        ptep = dkvtol2ptep((vaddr_t)vaddr, engnum);

        ptep = dkvtol1ptep((vaddr_t)vaddr, engnum);
        /*
         * Handle most frequent case first: VALID but not PSE.
         */
        if ((ptep->pg_pte & (PG_V|PG_PS)) == PG_V) {
                if (PG_ISVALID(ptep = dkvtol2ptep((vaddr_t)vaddr, engnum)))
                        return (ppid_t)PFN(ptep);
                else
                        return NOPAGE;
        } else if (PG_ISVALID(ptep))
                return (ppid_t)PFN(ptep) + pnum(vaddr);
        else
                return NOPAGE;
}


/*
 * ppid_t
 * pae_dkvtoppid(caddr_t vaddr, int engnum)
 *      Return the physical page ID corresponding to the virtual
 *      address vaddr on engine. 
 *
 */
ppid_t
pae_dkvtoppid(caddr_t vaddr, int engnum)
{
        pte64_t *ptep;

        ptep = dkvtol1ptep64((vaddr_t)vaddr, engnum);
        /*
         * Handle most frequent case first: VALID but not PSE.
         */
        if ((ptep->pg_pte & (PG_V|PG_PS)) == PG_V) {
                if (PG_ISVALID(ptep = dkvtol2ptep64((vaddr_t)vaddr, engnum)))
                        return (ppid_t)PFN64(ptep);
                else
                        return NOPAGE;
        } else if (PG_ISVALID(ptep))
                return (ppid_t)PFN64(ptep) + pae_pnum(vaddr);
        else
                return NOPAGE;
}


/*
 * ppid_t dkvtoppid(caddr_t vaddr, int engine)
 * 	directed kernel virtual to physical page id
 * 	return the ppid of the page corresponding to vaddr on engine
 *
 */
ppid_t
dkvtoppid(caddr_t vaddr, int engine)
{

        if (PAE_ENABLED())
                return pae_dkvtoppid(vaddr,engine);
        else
                return dkvtoppid32(vaddr,engine);


}


/*
 * void sysdump_genextbitmap()
 * 	create a bit map (sysdump_extmap) for the physical address range 
 * 	(ppid)sysdump_extmap_base to (ppid)sysdump_extmap_end
 * 	If a page is mapped in the kernel space of any engine then the
 * 	appropriate bit is set, else it is left clear.
 * 
 */
void
sysdump_genextbitmap()
{
vaddr_t kvaddr;
int     i;
ppid_t	ppid;


#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
	cmn_err(CE_CONT,"sysdump_genextbitmap: base = %x, end = %x\n",
		sysdump_extmap_base,sysdump_extmap_end);
#endif

	/* zero the extmap */
	bzero((caddr_t)sysdump_extmap,sysdump_extmap_size);

        /* need to do this for each engines page table */
        for (i = 0; i < Nengine; i++) {
                for (kvaddr = kvbase; kvaddr >= kvbase; kvaddr+= NBPP ) {
                        if ((ppid = dkvtoppid((caddr_t)kvaddr, i)) != NOPAGE) {
				/*
				 * we have a mapping so set bit in extent
				 * bitmap if in range
				 */
				if(IN_EXTMAPRANGE(ppid))
					set_extbit(ppid);
			}
			
                }
        }
}


struct cm_intr_cookie {
        struct intr_info ic_intr_info;
        int              (*ic_handler)();
};

extern  int	_Compat_intr_handler();

/*
 * void sysdump(void)
 *	Take a system dump.
 *
 * Calling/Exit State:
 *	The function is called when all other cpus have been quiesced
 *	and at this point the execution is single-threaded.
 */
void
sysdump(void)
{
	int ena, i, err, num_mregs, oldcpu;
	off64_t chunk_off = 0;
	size_t len, vlen, blkoff;
	paddr64_t base, baddr;
	ullong_t sz;
	emask_t responders;
	kcore_t header;
	mreg_chunk_t mchunk;
	vaddr_t base_addr[NMREG];
	int type;
	int dumpminor;
	ppid_t ppid;
	ulong_t npages;
	ulong_t page;
	struct intr_vect_t *ivp;
	boolean_t kerneladdr;
	boolean_t topo_has_memory_info;
	long	mapped = 0;
	long	unmapped = 0;
	long	zero = 0;
	long	nresource = 0;
	int hbanum,iov;
	struct hba_idata_v5		*hba_idatap;
	struct cm_intr_cookie *cookie;
	

	pl_dump = PLDISK - 1;

	if (!sysdump_init_done)
		return;

#ifdef DEBUG
	sys_dump = B_TRUE;
#endif
	DISABLE_PRMPT();

	CONSOLE_SUSPEND();

	/* Tunable - if sysdump_poll is 0, then don't use polling */
	if (sysdump_poll == 0)
		dumpcookie = NULL;

	/*
	 * Turn off all interrupts except xcall and clock
	 * try to turn off the disk interrupt if we know the
	 * dumpdev handler
	 */

	for (ivp=intr_vect,i = 0; i <= os_islot_max; ivp++,i++) {
#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
		cmn_err(CE_CONT,"sysdump: i = %d, ivect = %x, pri = %x ...",
			i,ivp->iv_ivect,ivp->iv_intpri);
#endif
		if (ivp->iv_ivect == (int (*)())intnull || ivp->iv_intpri >= PLHI) {
#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
			cmn_err(CE_CONT,"not disabling\n");
#endif
			continue;
		}

		if (!dumpcookie && ivp->iv_intpri == PLDISK) {
#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
			cmn_err(CE_CONT,"not disabling\n");
#endif
			/*
			 * we do not know the dumpdev interrupt
			 * guess that the dumpdev will have intpri == PLDISK
			 * and refrain from disabling this interrupt
			 */
			cmn_err(CE_WARN,
				"sysdump: reverting to interrupt mode dump\n");
			cmn_err(CE_CONT,
				"\tnot disabling PLDISK interrupt no %d\n",i);
			cmn_err(CE_CONT,
				"\tdump may be incomplete\n");
			continue;
		}
		
#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
		cmn_err(CE_CONT,"about to disable\n");
#endif

		ena = intr_disable();
		ms_intr_detach(&ivp->iv_idt);
		intr_restore(ena);
	}

#ifdef FLUSH_INTR
	/*
	 * now we have disabled interrupts, call the psm to clear
	 * all pending interuupts - this is done in ms_show_state
	 */
	ms_show_state();
#endif

	/*
	 * halt all other processors
	 */
	sysdump_done = B_FALSE;
	xcall_all(&responders, B_TRUE, sysdump_sync, NULL);

	for (i = 0; mod_funcs[i].oldfunc != 0; i++) {
		if(!dumpcookie  &&
			(mod_funcs[i].oldfunc == (vaddr_t)intr_undefer))
			continue;
		modify_code(mod_funcs[i].oldfunc, mod_funcs[i].newfunc);
	}

	/*
	 * discard deferred interrupts if we are using polled mode
	 */
	ena = intr_disable();
	if (dumpcookie) {
		dfrq_t          *dqp;
		dqp = l.deferqueue;
		while (dqp != NULL){
#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
			cmn_err(CE_NOTE,"sysdump: deferqueue not empty\n");
#endif
			l.deferqueue = dqp->dq_next;
			dqp->dq_next = l.dqfree;
			l.dqfree = dqp;
			dqp = l.deferqueue;
			l.picipl = 0;
		}
#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
		cmn_err(CE_NOTE,"sysdump: deferqueue empty\n");
#endif
	}
	intr_restore(ena);

	if (!dumpcookie)
		pl_dump = PLDISK - 1;
	else
		pl_dump = PLSTR;

	spl(pl_dump);

	code_modified = B_TRUE;

#ifdef DEBUG
	if (l.intr_depth)
		l.intr_depth = 0;
#endif

	/*
	 * Small delay to let other CPUs get to the sysdump idle loop & reset
	 * their (possible) APIC state.
	 */
	ms_time_spin(1000000);

	if(sysdump_selective)
		cmn_err(CE_CONT, "^\nGenerating selective dump\n");
	cmn_err(CE_CONT, "^\nSaving system memory image for crash analysis...\n"
		"   # = scanning memory, . = dumping to disk\n"
		"   (Press any key to abort dump.)\n");

	sysdump_bp->b_edev = dumpdev;

	i = 0;

	header.k_align 		= granularity;
	header.k_magic 		= KCORMAG;
	header.k_version 	= KCORVER;
	header.k_size 		= 0;	/* size is unknown */

	header.k_pae_enabled 	= PAE_ENABLED();
	header.k_kl1pt 		= cr_kl1pt;
	header.k_symtab_off 	= (off64_t)0; /* no symbol table */

	u.u_flags |= U_CRITICAL_MEM;

	/*
	 * write header out.
	 */
	err = sysdump_write_buf(&header, &chunk_off, sizeof(kcore_t));
	if (err) {
		cmn_err(CE_WARN,"\nCannot write dump header err = %d\n",err);
		goto abort;
	}

	baddr = 0;
	num_mregs = 0;

	bzero((caddr_t)&mchunk, sizeof(mreg_chunk_t));
	mchunk.mc_magic = MRMAGIC;


	/*
	 * scan topology to check we have memory information
	 */
	if((os_topology_p != NULL) &&
		((nresource = os_topology_p->mst_nresource) != 0)) {
		for (i = 0; i < nresource; i++){
			if(os_topology_p->mst_resources[i].msr_type ==
				MSR_MEMORY) {
				topo_has_memory_info = B_TRUE;
				break;
			}
		}
		
	}

	if(!topo_has_memory_info) {
		cmn_err(CE_WARN,"\nNo memory information in topology\n");
		goto abort;
	}
	

	/*
	 * initatialise ext bit map start to be 0
	 * mapend is the size of the extmap
	 * the initial bit map will be generated now and
	 * will be updated if we reach its limits later
	 */
	sysdump_extmap_base = phystoppid(0);
	sysdump_extmap_end = EXTMAP_SIZE<<3;
	sysdump_genextbitmap();

	for (i = 0; i < nresource; i++) {


		if(os_topology_p->mst_resources[i].msr_type !=
			MSR_MEMORY)
			continue;
		base = (paddr64_t)os_topology_p->mst_resources[i].
			msri.msr_memory.msr_address;
		sz = (ullong_t)os_topology_p->mst_resources[i].
			msri.msr_memory.msr_size;

		ASSERT((sz & PAGEOFFSET) == 0);

		if (baddr != base) {
			if (MREG_LENGTH(mchunk.mc_mreg[num_mregs]) &&
				 ++num_mregs == NMREG) {

				err = sysdump_new_chunk(&mchunk, &chunk_off,
					base_addr);
				if (err) {
					cmn_err(CE_WARN,
						"\nError (%d) writing chunk\n",
						err);
					goto abort;
				}
				num_mregs = 0;
			}
			ASSERT(base > baddr);
			vlen = base - baddr;
			do {
				mchunk.mc_mreg[num_mregs] = MREG_HOLE;
				len = min(vlen, MREG_MAX_LENGTH);
				mchunk.mc_mreg[num_mregs] += len << 
							    MREG_LENGTH_SHIFT;
				if (++num_mregs == NMREG) {
					err = sysdump_new_chunk(&mchunk, 
								&chunk_off,
								base_addr);
					if (err) {
						cmn_err(CE_WARN,
							"\nError (%d) writing chunk\n",
							err);
						goto abort;
					}
					num_mregs = 0;
				}
				vlen -= len;
			} while (vlen != 0);
		}

		baddr = base + sz;

#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
		ASSERT((sz & PAGEOFFSET) == 0);
		cmn_err(CE_CONT, "^base = %Lx extent = %Lx sz = %Lx\n", 
			os_topology_p->mst_resources[i].
				msri.msr_memory.msr_address,
			os_topology_p->mst_resources[i].
				msri.msr_memory.msr_size,sz);

#endif


		while (!geterror((buf_t *)sysdump_bp) && sz) {
			npages = min(btopr(sysdump_buf_size), btopr(sz));
			ppid = phystoppid(base);

			if (CONSOLE_GETC() != -1) {
				/* Aborted by user */
				goto abort;
			}

			if ((++hash_print_count & 0xff) == 0)
				cmn_err(CE_CONT, "^#");

			vlen = min(sz, ptob(npages));

			/* 
			 * build the mapping bitmap for physical -> kvirtual
			 * *before* we create a temp mapping
			 * for it.
			 */

			if(sysdump_selective) {
				/*
				 * check if we are going past the end of the 
				 * extent bit map
				 */
				if(!IN_EXTMAPRANGE(phystoppid(base)) ||
					!IN_EXTMAPRANGE(phystoppid(base+vlen))){
					paddr64_t b;
					ullong_t s;
					/*
					 * generate new extent bit map
					 */
					sysdump_extmap_base = phystoppid(base);

					b = (paddr64_t)os_topology_p->
						mst_resources[i].
						msri.msr_memory.msr_address;
					s = (ullong_t)os_topology_p->
						mst_resources[i].
						msri.msr_memory.msr_size;

				ASSERT((sz & PAGEOFFSET) == 0);
					sysdump_extmap_end = min(
						phystoppid(b + s),
						sysdump_extmap_base + 
						(sysdump_extmap_size << 3 )
					);

					ASSERT(sysdump_extmap_end >= sysdump_extmap_base);

					sysdump_genextbitmap();
				}

			}

			hat_statpt_devload((vaddr_t)sysdump_v, npages, ppid,
					   PROT_READ | PROT_WRITE);

			sz -= vlen;
			blkoff = 0;
			while (vlen) {
				if ((len = vlen) > MZERO_GRANULARITY)
					len = MZERO_GRANULARITY;

				/*
				 * The code assumes the following assertion
				 * to be true.
				 */
				ASSERT(MZERO_GRANULARITY < MREG_MAX_LENGTH);
				if (!bcmp((caddr_t)zaddr, 
					  (caddr_t)sysdump_v + blkoff,
					  len)) {
					type = MREG_ZERO;
					zero++;
				}
				else
				{
					if(sysdump_selective && 
						(get_extbit(phystoppid(base))
							== (uchar_t)0)){
						type = MREG_USER;
						unmapped ++;
					} else {
						type = MREG_IMAGE;
						mapped++;
					}
				}

				if (MREG_LENGTH(mchunk.mc_mreg[num_mregs]) != 
						0) {
					if ((MREG_TYPE(mchunk.mc_mreg[num_mregs]) != type) || 
					    ((MREG_LENGTH(mchunk.mc_mreg[num_mregs]) + len) > MREG_MAX_LENGTH)) {
						if (++num_mregs == NMREG) {
							hat_statpt_unload(sysdump_v, 
								  npages);
							TLBSflushtlb();
							err = 
							    sysdump_new_chunk(&mchunk,
								&chunk_off,
								base_addr);
							if (err) {
								cmn_err(CE_WARN,
									"\nError (%d) writing chunk\n",
									err);
								goto abort;
							}
							hat_statpt_devload(sysdump_v,
								   npages,
								   ppid, 
								   PROT_READ |
								   PROT_WRITE);
							num_mregs = 0;
						}
						base_addr[num_mregs] = base;
					}
				} else {
					base_addr[num_mregs] = base;
				}
				mchunk.mc_mreg[num_mregs] |= type;
				mchunk.mc_mreg[num_mregs] += len << 
							     MREG_LENGTH_SHIFT;
				vlen -= len;
				base += len;
				blkoff += len;
			}
			hat_statpt_unload(sysdump_v, npages);

			TLBSflushtlb();
		}
	}
	/*
	 * write the last chunk for the image.
	 */
	if (num_mregs || MREG_LENGTH(mchunk.mc_mreg[0]) != 0) {
		err = sysdump_new_chunk(&mchunk, &chunk_off, base_addr);
		if (err == -1)
			goto abort;
	}
	/*
	 * write final marker chunk out. 
	 */
	err = sysdump_new_chunk(&mchunk, &chunk_off, base_addr);
	if (err == -1) {
abort:
		cmn_err(CE_CONT, "^\n\nSystem memory dump aborted.\n");
		sysdump_done = B_TRUE;
		spl0();
		return;
	}

#if defined(DEBUG) || defined (SYSDUMP_DEBUG)
	cmn_err(CE_CONT,"^\nmapped chunks = %d, unmapped chunks = %d zero chunks = %d\n",
		mapped,unmapped,zero);

	ms_time_spin(10000000);
#endif


	/*
	 * give some time for disk buffers to get written out since
	 * the system will be rebooted.
	 */
	ms_time_spin(1000000);

	sysdump_done = B_TRUE;
	cmn_err(CE_CONT, "^\n\nSystem memory dump complete.\n");
	spl0();

	shutdown_drv();
}


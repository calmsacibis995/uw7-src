#ident	"@(#)kern-i386:mem/vm_rzbm.c	1.4.3.2"
#ident	"$Header$"

/*
 * VM - Restricted/Raw/Real Zoned Bit Map (RZBM) allocation routines
 *
 * RZBM allocates space from a bitmap. The bitmap is divided up into
 * a number of equal sized zones called "cells". Consequently,
 * there is a linear relationship between (i) cell address,
 * (ii) bitmap address, and (iii) physical allocation address.
 *
 * Each cell carries an allocation cursor, plus a free count.
 *
 */

#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/immu64.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/pmem.h>
#include <mem/page.h>
#include <mem/rzbm.h>
#include <proc/cg.h>
#include <proc/cguser.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/types.h>

#define	RZBM_LOCK()		LOCK(&rzbm_list.zbm_lock, VM_ZBM_IPL);
#define	RZBM_UNLOCK(opl)	UNLOCK(&rzbm_list.zbm_lock, opl);

/*
 * miscellaneous constants
 */

#define RZBM_MINCELLS		16		/* min # of cells required */
#define RZBM_CELLSIZE		(32 * NBITPW)	/* recommended cellsize (4M) */

#ifdef DEBUG

/*
 * macros to increment/decrement stats counters
 */
#define	RZBM_INC_STAT(field)	{ ++rzbm_list.zbm_stats.field; }
#define	RZBM_DEC_STAT(field)	{ --rzbm_list.zbm_stats.field; }
#define	RZBM_ADD_STAT(field, n)	{ rzbm_list.zbm_stats.field += (n); }
#define	RZBM_AUDIT(zbmp)	rzbm_audit(zbmp)

#else	/* !DEBUG */

#define	RZBM_INC_STAT(field)
#define	RZBM_ADD_STAT(field, value)
#define	RZBM_DEC_STAT(field)
#define	RZBM_AUDIT(zbmp)

#endif	/* !DEBUG */

/*
 *+ Per-zbm structure spin lock protecting all Zoned Bit Map data.
 */
STATIC LKINFO_DECL(rzbm_lkinfo, "mem: rzbm_lock: rzbm structure lock", 0);

/*
 * External routines.
 */

/*
 * Declarations: internal routines
 */

STATIC void rzbm_add_cell(struct rzbm_cell **, struct rzbm_cell *);
STATIC void rzbm_del_cell(struct rzbm_cell **, struct rzbm_cell *);
STATIC int rzbm_init_chunk_actual(ppid_t, uint_t, cgnum_t);

#ifdef DEBUG
static int rzbm_nbits(uint_t *, int);
static void rzbm_print_num(int, const char *);
static void rzbm_print_decimal(ulong_t, ulong_t, const char *);
#endif /* DEBUG */

/*
 * macros for manipulating bit numbers
 *
 *	RZBM_WDOFF	Bit number in map to bit number in a word
 *	RZBM_NWORDS	Number of bits to number of words (truncating)
 *	RZBM_NBITS	Number of words to number of bits
 */
#define	RZBM_WDOFF(bitno)	((bitno) % NBITPW)
#define	RZBM_NWORDS(bitno)	((bitno) / NBITPW)
#define	RZBM_NBITS(words)	((words) * NBITPW)

/*
 *  macros for converting to/from virtual address
 *
 *	RZBM_CELLTOPA(i, cr, bn)	cell to physical address
 *		i	= cell index
 *		cr	= cell cursor
 *		bn	= bit number in word
 *	RZBM_MAPTOPA(mi, wbn)	map to physical address
 *		mi	= map index
 *		wbn	= bit number in word
 *	RZBM_PATOBN	physical address to bitmap bit number
 */
#define	RZBM_CELLTOPA(i, cr, bn)	(zbmp->zbm_base + (RZBM_NBITS(cr + \
				((i) << zbmp->zbm_cwshft)) + (bn)))
#define	RZBM_MAPTOPA(mi, wbn)	(zbmp->zbm_base + (RZBM_NBITS(mi) + (wbn)))
#define	RZBM_PATOBN(pa)		(((pa) - zbmp->zbm_base))

/*
 * macros for converting among cell pointer, cell indicies, bitmap
 * indicies, and bitmap pointer
 *
 *	RZBM_CPTOCI		cell pointer to cell index
 * 	RZBM_MITOMP		map index to map pointer
 * 	RZBM_MITOCI		map index to cell index
 *	RZBM_CITOCP		cell index to cell pointer
 *	RZBM_MITOCP		map index to cell pointer
 *	RZBM_CITOMP		cell index to map pointer
 *	RZBM_MITOCURSOR		map index to cell cursor
 */
#define	RZBM_CPTOCI(cp)		((cp) - zbmp->zbm_cell)
#define	RZBM_MITOMP(mi)		(zbmp->zbm_bitmap + (mi))
#define	RZBM_MITOCI(mi)		((mi) >> zbmp->zbm_cwshft)
#define	RZBM_CITOCP(ci)		(zbmp->zbm_cell + (ci))
#define	RZBM_MITOCP(mi)		RZBM_CITOCP(RZBM_MITOCI(mi))
#define	RZBM_CITOMP(ci)		RZBM_MITOMP((ci) << zbmp->zbm_cwshft)
#define	RZBM_MITOCURSOR(mi)	((mi) & zbmp->zbm_cellmask)

rzbm_list_t rzbm_list;

/*
 * void
 * rzbm_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
rzbm_init(void)
{
	/*
	 * initialize zbmp->zbm_lock
	 */
	LOCK_INIT(&rzbm_list.zbm_lock, VM_ZBM_HIER, VM_ZBM_IPL, &rzbm_lkinfo,
		  KM_NOSLEEP);
	SV_INIT(&rzbm_list.zbm_sv);
}

/*
 * AUDIT style DEBUG code
 */

/*
 * int
 * rzbm_nbits(uint_t *mp, int n)
 *	Debug routine to count the number of bits set in a bitmap.
 *
 * Calling/Exit State:
 *	Returns number of set bits.
 */

static int
rzbm_nbits(uint_t *mp, int n)
{
	int count = 0;
	int i;

	while (n > 0) {
		for (i = 0; i < NBITPW; ++i) {
			if (BITMASK1_TEST1(mp, i)) {
				++count;
			}
		}
		++mp;
		--n;
	}

	return (count);
}

/*
 * int
 * rzbm_setnbits(uint_t *mp, int n)
 *
 * Calling/Exit State:
 *	None.
 */
static void
rzbm_setnbits(uint_t *mp, int n)
{
	int i;

	while (n > 0) {
		for (i = 0; i < NBITPW; ++i) {
			ASSERT(BITMASK1_TEST1(mp, i) == 0);
			BITMASK1_SET1(mp, i);
		}
		++mp;
		--n;
	}
}

/*
 * int
 * rzbm_clrnbits(uint_t *mp, int n)
 *
 * Calling/Exit State:
 *	None.
 */
static void
rzbm_clrnbits(uint_t *mp, int n)
{
	int i;

	while (n > 0) {
		for (i = 0; i < NBITPW; ++i) {
			ASSERT(BITMASK1_TEST1(mp, i) != 0);
			BITMASK1_CLR1(mp, i);
		}
		++mp;
		--n;
	}
}

/*
 * void
 * rzbm_audit(zbm_t *zbmp)
 *	This heavy duty routine should be run in a debug mode in which
 *	performance is of no consequence.
 *
 * Calling/Exit State:
 *	- called with zbmp->zbm_lock locked
 *	- returns with zbmp->zbm_lock locked
 *
 * Description:
 *	The following assertions are checked:
 *
 * 		1. The number of the set bits == zbmp->zbm_mapsize - the sum of
 *		   zbc_nfree.
 * 		2. For each cell, the number of set bits in the map is
 * 		   equal to zbmp->zbm_cellsize - zbc_nfree.
 * 		3. The count of all cells in the free pool + the count of
 * 		   all cells not in the free pool == zbmp->zbm_ncell.
 *		4. Each cell cursor is constrained to the cell.
 *		5. The bitmap cursor is constrained to the bitmap.
 *		6. The free pool cursor is constrained to the free pool
 */

void
rzbm_audit(rzbm_t *zbmp)
{
	int totalcount, cellcount, i;
	struct rzbm_cell *cellp, *lastp;
	int rvcount, nrvcount;
	int freecount, nfreecount;

	/*
	 * 1. The number of the set bits == zbmp->zbm_mapsize - the sum of
	 *    zbc_nfree.
	 */
	totalcount = rzbm_nbits(zbmp->zbm_bitmap, zbmp->zbm_mapwidth) - 
			zbmp->zbm_unavail;
	cellcount = 0;
	cellp = zbmp->zbm_cell;
	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		cellcount += cellp->zbc_nfree;
		++cellp;
	}
	ASSERT(zbmp->zbm_mapsize - cellcount == totalcount);

	cellp = zbmp->zbm_pool;
	freecount = rvcount = 0;
	if (cellp) {
		lastp = zbmp->zbm_pool->zbc_blink;
		while (cellp != zbmp->zbm_pool->zbc_blink) {
			++rvcount;
			++freecount;
			ASSERT(cellp->zbc_nfree);
			ASSERT(rvcount <= zbmp->zbm_ncell);
			ASSERT(cellp->zbc_blink == lastp);
			lastp = cellp;
			cellp = cellp->zbc_flink;
		}
		freecount++;
	}

	cellp = zbmp->zbm_psepool;
	if (cellp) {
		lastp = zbmp->zbm_psepool->zbc_blink;
		while (cellp != zbmp->zbm_psepool->zbc_blink) {
			++rvcount;
			++freecount;
			ASSERT(cellp->zbc_nfree);
			ASSERT(rvcount <= zbmp->zbm_ncell);
			ASSERT(cellp->zbc_blink == lastp);
			lastp = cellp;
			cellp = cellp->zbc_flink;
		}
		freecount++;
	}

	/*
	 * 2. For each cell, the number of set bits in the map is
	 *    equal to zbmp->zbm_cellsize - zbc_nfree.
	 */
	cellp = zbmp->zbm_cell;
	nfreecount = 0;
	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		cellcount = rzbm_nbits(RZBM_CITOMP(i), zbmp->zbm_cellwidth);
		ASSERT(cellcount == zbmp->zbm_cellsize - cellp->zbc_nfree);
		ASSERT((int)cellp->zbc_cursor <= zbmp->zbm_cellwidth);
		if (cellp->zbc_nfree == 0)
			++nfreecount;	
		++cellp;
	}

	/*
	 * 3. The count of all cells in the free pool + the count of
	 *    all cells not in the free pool == zbmp->zbm_ncell.
	 */
	ASSERT(freecount + nfreecount == zbmp->zbm_ncell);
}

/*
 * STATIC void
 * rzbm_add_chunk(rzbm_t *zbmp)
 *
 * Calling/Exit State;
 *	None.
 */
STATIC void
rzbm_add_chunk(rzbm_t *zbmp)
{
	rzbm_t *rzbmp;
	rzbm_t *last;

	rzbmp = rzbm_list.zbm_chunk;
        if (rzbmp == (rzbm_t *)NULL) {
                zbmp->zbm_flink = zbmp->zbm_blink = zbmp;
		rzbm_list.zbm_chunk = zbmp;
        } else {
		last = rzbmp->zbm_blink;
                zbmp->zbm_blink = last;
                zbmp->zbm_flink = last->zbm_flink;
                last->zbm_flink->zbm_blink = zbmp;
                last->zbm_flink = zbmp;
        }
	rzbm_list.zbm_nchunk++;
}

/*
 * STATIC void
 * rzbm_del_chunk(rzbm_t *zbmp)
 *
 * Calling/Exit State;
 *	None.
 */
STATIC void
rzbm_del_chunk(rzbm_t *zbmp)
{
	rzbm_t *next, *last;

	if (zbmp->zbm_flink == zbmp) {
		rzbm_list.zbm_chunk = NULL;
	} else {
		/*
		 * delete the chunk from the list
		 */
		next = zbmp->zbm_flink;
		last = zbmp->zbm_blink;
		next->zbm_blink = last;
		last->zbm_flink = next;

		if (rzbm_list.zbm_chunk == zbmp)
			rzbm_list.zbm_chunk = zbmp->zbm_flink;
	}
	return;
}

/*
 * int
 * rzbm_init_chunk(ppid_t base, uint_t npages, cgnum_t cgnum)
 *	Externally visible final rzbm initialisation routine
 *
 *	Really just a wrapper for rzbm_init_chunk_actual
 *      if the range passed in is not PSE aligned
 *	carve the non PSE aligned portions from the start & end
 *	of the address range [ leaving a PSE aligned range in the
 *	middle ] pass each range separately to the real
 *	rzbm initialisation routine.
 *	We do this in order to have a pool of PSE aligned chunks
 *	for segpse.
 *
 * Calling/Exit State:
 *	- called without any locks held.
 *	- returns 0 on success and non-zero on failure.
 */

int
rzbm_init_chunk(ppid_t base, uint_t npages, cgnum_t cgnum)
{
	ppid_t	prebase, postbase;
	uint_t	presz = 0,
		postsz = 0;
	int	error;

	if ( npages == 0 )
		return 0;

	if ( npages < PSE_NPAGES )
		return rzbm_init_chunk_actual(base, npages, cgnum);
	
	/*
	 * if the base isn't PSE aligned
	 * calculate the size of the first chunk
	 */
	if( base & PSE_NPGOFFSET ){
		prebase = base;
		presz = psetop(btopser64(ptob64(base))) - base;
		if(presz > npages)
			presz = npages;
		base += presz;
		npages -= presz;
	}
	
	/*
	 * Any overlap beyond PSE boundary ?
	 */
	if(npages > PSE_NPAGES){
		postsz = (base + npages) & PSE_NPGOFFSET;
		npages -= postsz;
		postbase = base + npages;
	}
	
	if(presz)
		if( error =  rzbm_init_chunk_actual(prebase, presz, cgnum))
			return error;
				
	if(postsz)
		if( error =  rzbm_init_chunk_actual(postbase, postsz, cgnum))
			return error;


	if(npages){
		return rzbm_init_chunk_actual(base, npages, cgnum);
	}
	
	return 0;
}

/*
 * int
 * rzbm_init_chunk_actual(ppid_t base, uint_t npages, cgnum_t cgnum)
 *	Final initialization for the zbm_t.
 *	Initializes all fields not already initialized.
 *
 * Calling/Exit State:
 *	- called without any locks held.
 *	- returns 0 on success and non-zero on failure.
 */

STATIC int
rzbm_init_chunk_actual(ppid_t base, uint_t npages, cgnum_t cgnum)
{
	struct rzbm_cell *cellp, *lastcellp;
	int i, ncells;
	rzbm_t *zbmp;
	int cellsize;
	uint_t rem;
	uint_t *mp;
	uint_t nwords;
	extern int upyet;

	if (npages == 0)
		return 0;

	cellsize = rzbm_cell_size(npages);
	ncells = npages / (ulong_t)cellsize;
	nwords = RZBM_NWORDS(npages);
	if ((rem = npages % (ulong_t)cellsize) != 0) {
		ncells++;
		nwords++;
	}

	/*
	 * Allocate the zbmp->zbm_cell(s) and the bitmap.
	 */
	if (upyet) {
		zbmp = kmem_zalloc(sizeof(rzbm_t), KM_SLEEP);
		zbmp->zbm_cell = kmem_zalloc(ncells * sizeof(struct rzbm_cell), KM_SLEEP);
		zbmp->zbm_bitmap = kmem_zalloc(nwords * sizeof(uint_t), KM_SLEEP);
		rzbm_add_chunk(zbmp);
	} else {
		zbmp = calloc(sizeof(rzbm_t));
		/*
		 * allocate the cells
		 *	N.B. Calculation assumes that size has been
		 *	     trimmed back to a multiple of zbmp->zbm_cellsize.
		 */
		zbmp->zbm_cell = calloc(ncells * sizeof(struct rzbm_cell));
		/*
		 * allocate the bit maps
		 *	N.B. Calculation assumes that zbmp->zbm_cellsize size
		 *	     has been trimmed back to a multiple of NBITPW.
		 */
		zbmp->zbm_bitmap = calloc(nwords * sizeof(uint_t));
		rzbm_add_chunk(zbmp);
	}	

	/*
	 * Record segment base and size.
	 * These values are now permanently fixed.
	 */
	zbmp->zbm_base = base;
	zbmp->zbm_size = npages;

	/*
	 * Initialize the cell size.
	 */
	zbmp->zbm_cellsize = cellsize;

	/*
	 * Initialize static variables.
	 */
	/* Note: cellsize should be less than 4G */
	zbmp->zbm_cellwidth = RZBM_NWORDS(zbmp->zbm_cellsize);
	zbmp->zbm_mapsize = (int) npages;
	zbmp->zbm_ncell = ncells;
	ASSERT((zbmp->zbm_ncell == zbmp->zbm_mapsize / zbmp->zbm_cellsize) ||
	       (zbmp->zbm_ncell == zbmp->zbm_mapsize / zbmp->zbm_cellsize + 1));
	zbmp->zbm_mapwidth = nwords;
	ASSERT((zbmp->zbm_mapwidth == RZBM_NWORDS(zbmp->zbm_mapsize)) ||
	       (zbmp->zbm_mapwidth == RZBM_NWORDS(zbmp->zbm_mapsize) + 1));
	zbmp->zbm_cellmask = zbmp->zbm_cellwidth - 1;
	zbmp->zbm_home = cgnum;

	/*
	 * zbmp->zbm_cwshft = log2(zbmp->zbm_cellwidth)
	 */
	i = 1;
	while (i < zbmp->zbm_cellwidth) {
		++zbmp->zbm_cwshft;
		i <<= 1;
	}

	/*
	 * Place all cells on the free pool list.
	 */
	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		cellp = &zbmp->zbm_cell[i];
		cellp->zbc_nfree = (ushort_t)zbmp->zbm_cellsize;
		cellp->zbc_flink = cellp->zbc_blink = cellp;
		rzbm_add_cell(&zbmp->zbm_pool, cellp);
	}

	/*
	 * Initialize the bitmap to all clear (all free).
	 */
	BITMASKN_CLRALL(zbmp->zbm_bitmap, zbmp->zbm_mapwidth);
	/*
	 * Zero the bits for a partially allocated cell. Do not
	 * assume that the size is going to be a multiple of
	 * cellsize.
	 */
	if (rem) {
		lastcellp = zbmp->zbm_pool->zbc_blink;
		mp = RZBM_CITOMP(RZBM_CPTOCI(lastcellp));
		lastcellp->zbc_nfree = rem;
		for (i = rem; i < NBITPW; i++) {
			BITMASK1_SET1(mp, i);
		}
		zbmp->zbm_unavail = NBITPW - rem;
	}

	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		cellp = &zbmp->zbm_cell[i];
		/*
		 * if cell has PSE_NPAGES free remove it
		 * from the 4K freepool & add it to
		 * the pse freepool.
		 * Done here instead of above because
		 * partially allocate cells aren't
		 * cleaned up until after adding to the 4k pool
		 */
		if(cellp->zbc_nfree == PSE_NPAGES){
			rzbm_del_cell(&zbmp->zbm_pool, cellp);
			rzbm_add_cell(&zbmp->zbm_psepool, cellp);
		}
	}

	rzbm_list.zbm_maxfree += zbmp->zbm_mapsize;

	return 0;
}

/*
 * int
 * rzbm_deinit_chunk(paddr64_t base, uint_t npages, cgnum_t cg)
 *	release rzbm chunk
 *
 * Calling/Exit State:
 *	- called without any locks held.
 *	- returns 0 on success and non-zero on failure.
 */

int
rzbm_deinit_chunk(paddr64_t base, uint_t npages, cgnum_t cg)
{
	struct rzbm_cell *cellp, *lastcellp;
	pl_t opl;
	int cellsize, rem;
	rzbm_t *zbmp;
	int i, ncells, maxfree;
	boolean_t found = B_FALSE;

	opl = RZBM_LOCK();

	/*
	 * Find the chunk that the paddr belongs to.
	 */
	for (i = rzbm_list.zbm_nchunk, zbmp = rzbm_list.zbm_chunk; i--;
				zbmp = zbmp->zbm_flink) {
		if ((base == zbmp->zbm_base) && (npages == zbmp->zbm_base)) {
			found = B_TRUE;
			break;
		}
	}

	if (found == B_FALSE) {
		RZBM_UNLOCK(opl);
		return 1;
	}
		
	cellsize = zbmp->zbm_cellsize;
	ncells = npages / (ulong_t)cellsize;
	rem = npages % (ulong_t)cellsize;

	for (i = 0; i < ncells; ++i) {
		cellp = &zbmp->zbm_cell[i];
		if (cellp->zbc_nfree != (ushort_t)zbmp->zbm_cellsize) {
			RZBM_UNLOCK(opl);
			return 1;
		}
		maxfree += cellp->zbc_nfree;
	}
	if (rem) {
		lastcellp = zbmp->zbm_pool->zbc_blink;
		if (lastcellp->zbc_nfree != rem) {
			RZBM_UNLOCK(opl);
			return 1;
		}
		maxfree += lastcellp->zbc_nfree;
	}

	/*
	 * All cells in the chunk are free. Subtract the zbm_maxfree
	 * count, deallocate the bitmap, unlink the chunk and release
	 * the memory. We cannot release the memory, because we cannot
	 * free the calloc space. So for now, just subtrace the maxfree
	 * count.
	 */
	rzbm_list.zbm_maxfree -= maxfree;
	RZBM_UNLOCK(opl);

	return 0;
}

/*
 * void
 * rzbm_add_cell(struct rzbm_cell **pool, struct rzbm_cell *cellp)
 *	Add a cell to the reserve list (either to the head or the tail
 *	depending upon state).  Set the new state of the cell.
 *
 * Calling/Exit State:
 *	called with zbmp->zbm_lock held
 *	returns with zbmp->zbm_lock held
 *	never blocks
 */

STATIC void
rzbm_add_cell(struct rzbm_cell **pool, struct rzbm_cell *cellp)
{
	struct rzbm_cell *next, *last;

        if (*pool == NULL) {
                *pool = cellp;
		cellp->zbc_flink = cellp->zbc_blink = cellp;
	} else {
		/*
		 * Link it to the end of the list.
		 */
		next = *pool;
		last = (*pool)->zbc_blink;
		next->zbc_blink = cellp;
		last->zbc_flink = cellp;
		cellp->zbc_flink = next;
		cellp->zbc_blink = last;
	}

	return;
}

/*
 * void
 * rzbm_del_cell(struct rzbm_cell **pool, struct zbm_cell * cellp)
 *	Remove a cell from the reserve list.
 *
 * Calling/Exit State:
 *	called with zbmp->zbm_lock held
 *	returns with zbmp->zbm_lock held
 *	never blocks
 */

STATIC void
rzbm_del_cell(struct rzbm_cell **pool, struct rzbm_cell *cellp)
{
	struct rzbm_cell *next, *last;

	if (cellp->zbc_flink == cellp) {
		*pool = NULL;
	} else {
		/*
		 * Delete the cell from the list.
		 */
		next = cellp->zbc_flink;
		last = cellp->zbc_blink;
		next->zbc_blink = last;
		last->zbc_flink = next;

		if (*pool == cellp)
			*pool = cellp->zbc_flink;
	}

	/*
	 * Wrap back the pointers (not for courtesy - this is actually
	 * required for zbm_add_cell to work)
	 */
	cellp->zbc_flink = cellp;
	cellp->zbc_blink = cellp;

	return;
}

/*
 * int
 * rzbm_resv(uint_t npages, uint_t flag)
 *	Reserve locked identitity less memory. This function is
 *	modeled after the existing mem_resv interface.
 *
 * Calling/Exit State:
 *	Returns non-zero on success.
 *
 * Remarks:
 *	This function does NOT block which means there is no way to wait
 *	for a memory reservation at this level. This is by design to spare
 *	us the overhead of looking to see if anyone is blocked when we
 *	do unresv etc. Since most of our callers can tolerate failure
 *	this works out to everybodies advantage.
 */
int
rzbm_resv(uint_t npages, uint_t flag)
{
	pl_t opl;

	opl = RZBM_LOCK();
retry:
	if (rzbm_list.zbm_resv + npages > rzbm_list.zbm_maxfree) {
		if (flag & SLEEP) {
                	SV_WAIT(&rzbm_list.zbm_sv, PRIMEM, &rzbm_list.zbm_lock);
	                (void) RZBM_LOCK();
			goto retry;
		}
		RZBM_INC_STAT(zbs_nfail);
		RZBM_UNLOCK(opl);
		return 0;
	}
	rzbm_list.zbm_resv += npages;
	RZBM_UNLOCK(opl);
	return 1;
}

/*
 * void
 * rzbm_unresv(uint_t npages)
 *      Give back an identityless memory reservation.
 *
 * Calling/Exit State:
 *      None.
 */
void
rzbm_unresv(uint_t npages)
{
	pl_t opl;

	opl = RZBM_LOCK();
	rzbm_list.zbm_resv -= npages;
	ASSERT(rzbm_list.zbm_resv <= rzbm_list.zbm_maxfree);
	RZBM_UNLOCK(opl);
	/*
	 * wake up anybody waiting for space
	 */
	if (SV_BLKD(&rzbm_list.zbm_sv)) {
		SV_BROADCAST(&rzbm_list.zbm_sv, 0);
	}
}

/*
 * ppid_t
 * rzbm_psealloc(cgnum_t cgnum)
 * 	This routine allocates contiguous physical space of 4M pagesize
 *	bytes.
 *
 * Calling/Exit State:
 * 	- called and returns with rzbm_list.zbm_lock unlocked
 *	- callers prepare to block if !NOSLEEP
 */

ppid_t
rzbm_psealloc(cgnum_t cgnum)
{
	pl_t opl;
	ppid_t pf;
	struct rzbm_cell *cellp;
	int i, j, n, remaining, limit;
	int mi;			/* global map index */
	int cursor;		/* cell word number */
	int wbn;		/* word bit number */
	int nbits;		/* pages freed in the current cell */
	rzbm_t *zbmp;
	int celli;
	uint_t *mp;
	int ncells, nchunks;

	opl = RZBM_LOCK();

retry:
	zbmp = rzbm_list.zbm_chunk;
	nchunks = rzbm_list.zbm_nchunk;
	for (; nchunks--; zbmp = zbmp->zbm_flink) {
		RZBM_INC_STAT(zbs_nchunks);
		if (zbmp->zbm_home == cgnum || cgnum == CG_NONE) {
			if (zbmp->zbm_psepool != NULL) 
				break;
		}
	}
	if (nchunks < 0 && cgnum != CG_NONE) {
		cgnum = CG_NONE;
		goto retry;
	} else if (nchunks < 0) {
		/*
		 * Only reach here if we could not allocate a page. This
		 * should never be reached since we reserve the pages
		 * before we start the allocation. However, it could fail
		 * if 4M pages are not available inspite of reservation.
		 */
		RZBM_INC_STAT(zbs_npse_fail);
		RZBM_AUDIT(zbmp);
		RZBM_UNLOCK(opl);
		return 0;
	}

	ASSERT(zbmp->zbm_psepool);
	RZBM_INC_STAT(zbs_npg_allocs);
	cellp = zbmp->zbm_psepool;
	ASSERT(cellp->zbc_nfree == PSE_NPAGES);
	celli = RZBM_CPTOCI(cellp);
	cursor = cellp->zbc_cursor;
	ASSERT(cursor == 0);
	mp = RZBM_CITOMP(celli) + cursor;
	n = BITMASKN_FFC(mp, zbmp->zbm_cellwidth - cursor);
	rzbm_setnbits(mp, zbmp->zbm_cellwidth);
	ASSERT(n != -1);
#if DEBUG
	cellp->zbc_nfree -= PSE_NPAGES;
#else	
	cellp->zbc_nfree = 0;
#endif
	cellp->zbc_cursor = cursor + RZBM_NWORDS(n + PSE_NPAGES);
	/*
	 * Compute physical address.
	 */
	pf = RZBM_CELLTOPA(celli, cursor, n);
	ASSERT(pf >= zbmp->zbm_base);
	ASSERT(pf - zbmp->zbm_base + 1 <= zbmp->zbm_size);
	/*
	 * Adjust the free pool chain.
	 */
	ASSERT((int)cellp->zbc_nfree == 0);
	rzbm_del_cell(&zbmp->zbm_psepool, cellp);
	RZBM_ADD_STAT(zbs_npse_allocated, 1);
	RZBM_AUDIT(zbmp);
	RZBM_UNLOCK(opl);
	return (pf);
}

/*
 * ppid_t
 * rzbm_alloc(cgnum_t cgnum, uint_t pagesz)
 * 	This routine allocates contiguous physical space of pagesize
 *	bytes.
 *
 * Calling/Exit State:
 * 	- called and returns with rzbm_list.zbm_lock unlocked
 *	- callers prepare to block if !NOSLEEP
 */

ppid_t
rzbm_alloc(cgnum_t cgnum, uint_t pagesz)
{
	pl_t opl;
	ppid_t pf;
	struct rzbm_cell *cellp;
	int i, j, n, remaining, limit;
	int mi;			/* global map index */
	int cursor;		/* cell word number */
	int wbn;		/* word bit number */
	int nbits;		/* pages freed in the current cell */
	rzbm_t *zbmp;
	int celli;
	uint_t *mp;
	int ncells, nchunks;

	ASSERT(pagesz == PAGESIZE || pagesz == PSE_PAGESIZE);

	if (pagesz == PSE_PAGESIZE)
		return rzbm_psealloc(cgnum);

	ASSERT(pagesz == PAGESIZE);

	opl = RZBM_LOCK();

retry:
	zbmp = rzbm_list.zbm_chunk;
	nchunks = rzbm_list.zbm_nchunk;
	for (; nchunks--; zbmp = zbmp->zbm_flink) {
		RZBM_INC_STAT(zbs_nchunks);
		if (zbmp->zbm_home == cgnum || cgnum == CG_NONE) {
			if (zbmp->zbm_pool != NULL) 
				break;
			else if (zbmp->zbm_psepool != NULL) {
				ASSERT(zbmp->zbm_pool == NULL);
				cellp = zbmp->zbm_psepool;
				rzbm_del_cell(&zbmp->zbm_psepool, cellp);
				rzbm_add_cell(&zbmp->zbm_pool, cellp);
				break;
			}
		}
	}
	if (nchunks < 0 && cgnum != CG_NONE) {
		cgnum = CG_NONE;
		goto retry;
	} else if (nchunks < 0) {
		/*
		 *+ Only reach here if we could not allocate a page. This
		 *+ should never be reached since we reserve the pages
		 *+ before we start the allocation. However, it could fail
		 *+ if 4M pages are not available inspite of reservation.
		 */
		cmn_err(CE_PANIC,
			"rzbm_alloc: %x pagesize allocation failure", pagesz);
	}

	ASSERT(zbmp->zbm_pool);
	RZBM_INC_STAT(zbs_npg_allocs);
	cellp = zbmp->zbm_pool;
	celli = RZBM_CPTOCI(cellp);
	cursor = cellp->zbc_cursor;
	if (cursor == zbmp->zbm_cellwidth)
		/* reset the cursor */
		cellp->zbc_cursor = cursor = 0;
	mp = RZBM_CITOMP(celli) + cursor;
	n = BITMASKN_FFCSET(mp, zbmp->zbm_cellwidth - cursor);
	ASSERT(n != -1);
	ASSERT(cellp->zbc_nfree >= 1);
	--cellp->zbc_nfree;
	cellp->zbc_cursor = cursor + RZBM_NWORDS(n + 1);
	/*
	 * Compute physical address.
	 */
	pf = RZBM_CELLTOPA(celli, cursor, n);
	ASSERT(pf >= zbmp->zbm_base);
	ASSERT(pf - zbmp->zbm_base + 1 <= zbmp->zbm_size);
	/*
	 * Adjust the free pool chain.
	 */
	if ((int)cellp->zbc_nfree == 0) {
		rzbm_del_cell(&zbmp->zbm_pool, cellp);
	}
	pzero(pf);
	RZBM_ADD_STAT(zbs_nallocated, 1);
	RZBM_AUDIT(zbmp);
	RZBM_UNLOCK(opl);
	return (pf);
}

/*
 * void
 * rzbm_free(ppid_t pf, uint_t pagesz)
 * 	This function frees a page size worth of physical memory.
 *
 * Calling/Exit State:
 *	- called and returns with rzbm_list.zbm_lock unlocked
 *
 * Description:
 *	clear the bits in the bitmap
 *	adjust cell state for the affected cells
 */

void
rzbm_free(ppid_t pf, uint_t pagesz)
{
	pl_t opl;
	struct rzbm_cell *cellp;
	int mi;			/* global map index */
	int mbn;		/* global map bit number */
	int cursor;		/* cell cursor following last free word */
	int wbn;		/* word bit number */
	int nbits;		/* pages freed in the current cell */
	rzbm_t *zbmp;
	int i;
	int celli;

	ASSERT(pagesz == PAGESIZE || pagesz == PSE_PAGESIZE);

	opl = RZBM_LOCK();

	/*
	 * Find the chunk that the page frame belongs to.
	 */
	for (i = rzbm_list.zbm_nchunk, zbmp = rzbm_list.zbm_chunk; i--;
				zbmp = zbmp->zbm_flink) {
		if (pf >= zbmp->zbm_base && (pf < (zbmp->zbm_base +
						zbmp->zbm_size)))
			break;
	}

	RZBM_INC_STAT(zbs_npg_free);

	ASSERT(zbmp);
	ASSERT((pf >= zbmp->zbm_base) && 
		(pf < zbmp->zbm_base + zbmp->zbm_size));
	mbn = RZBM_PATOBN(pf);
	mi = RZBM_NWORDS(mbn);
	wbn = RZBM_WDOFF(mbn);
	cellp = RZBM_MITOCP(mi);
	cursor = RZBM_MITOCURSOR(mi);

	/*
	 * clear the bits in the bitmap
	 */
	switch (pagesz) {
	case PAGESIZE:
		ASSERT(pf + 1 <= zbmp->zbm_base + zbmp->zbm_size);
		BITMASK1_CLR1(RZBM_MITOMP(mi), wbn);
		nbits = 1;
#ifdef NO_MARKCHANGES
/*
 * cursor should already be at the word which contains the free
 * bit - no need to advance it any further.
 */
		cursor += RZBM_NWORDS(wbn + 1);
#endif
		RZBM_ADD_STAT(zbs_nfreed, 1);

		/*
		 * adjust cell free count
		 */
		cellp->zbc_nfree += (ushort_t)nbits;

		/*
		 * adjust cell cursor to the first available cell.
		 */
		if (cursor < (int)cellp->zbc_cursor)
			cellp->zbc_cursor = (uchar_t)cursor;

		/*
		 * Add the cell to free pool.
		 */
		if ((int)cellp->zbc_nfree == 1)
			rzbm_add_cell(&zbmp->zbm_pool, cellp);

		/*
		 * Add the cell to PSE free pool, but first remove it
		 * from the 4K pagesize pool.
		 */
		if ((int)cellp->zbc_nfree == PSE_NPAGES) {
			ASSERT(zbmp->zbm_cellsize == PSE_NPAGES);
			rzbm_del_cell(&zbmp->zbm_pool, cellp);
			rzbm_add_cell(&zbmp->zbm_psepool, cellp);
		}

		break;

	case PSE_PAGESIZE:
		ASSERT(pf + PSE_NPAGES <= zbmp->zbm_base + zbmp->zbm_size);
		rzbm_clrnbits(RZBM_MITOMP(mi), zbmp->zbm_cellwidth);
		cellp->zbc_cursor = 0;
		ASSERT(zbmp->zbm_cellsize == PSE_NPAGES);
		cellp->zbc_nfree = zbmp->zbm_cellsize;
		RZBM_ADD_STAT(zbs_npse_freed, 1);
		/*
		 * Add the cell to PSE free pool.
		 */
		ASSERT((int)cellp->zbc_nfree == PSE_NPAGES);
		rzbm_add_cell(&zbmp->zbm_psepool, cellp);
		break;

	default:
		/*
		 *+ Only 4K and 4M identityless page pool are supported.
		 */
		cmn_err(CE_PANIC,
			"rzbm_free: Invalid page size %x", pagesz);
	};

	RZBM_AUDIT(zbmp);

	RZBM_UNLOCK(opl);

	return;
}

/*
 * int
 * rzbm_cell_size(uint_t segsize)
 *	Compute the size of a zbm cell given the size of the physical
 *	allocation space.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 * 	The priorities for the cell size are:
 *
 *	1. must be a multiple (by a power of 2) of NPITPW
 *	2. should have at least RZBM_MINCELLS allocated
 *	3. should have cells of RZBM_CELLSIZE pages
 */
int
rzbm_cell_size(uint_t segsize)
{
	int cellsize, i;

	/*
	 * This is specially for PSE 4M pages.
	 */
	if (segsize >= PSE_NPAGES)
		/* cellsize is 4M */
		return RZBM_CELLSIZE;

	cellsize = RZBM_CELLSIZE;
	if (segsize / cellsize < RZBM_MINCELLS) {
		cellsize = segsize / RZBM_MINCELLS;
	}
	i = NBITPW;
	while (i < cellsize) {
		i *= 2;
	}
	cellsize = (i > cellsize && i > NBITPW) ? i / 2 : i;

	return (cellsize);
}

/*
 * int
 * rzbm_memsize(void)
 *
 * Calling/Exit State:
 *	None.
 */
int
rzbm_memsize(void)
{
	return rzbm_list.zbm_maxfree;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_rzbm_cell(const rzbm_t *zbmp, const struct rzbm_cell *cellp)
 *	Debug routine to print out the contents of a cell
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_rzbm_cell(const rzbm_t *zbmp, const struct rzbm_cell *cellp)
{
	uint_t *mp;
	int i;

	debug_printf("%4d %5d%4d", RZBM_CPTOCI(cellp),
		     cellp->zbc_nfree, cellp->zbc_cursor);
	mp = RZBM_CITOMP(RZBM_CPTOCI(cellp));
	for (i = 0; i < zbmp->zbm_cellwidth; ++i) {
		if (i > 0 && i % 5 == 0) {
			debug_printf("\n                ");
		}
		debug_printf(" %08x", *mp);
		if (debug_output_aborted())
			return;
		++mp;
	}
	debug_printf("\n");
}

/*
 * void
 * print_rzbm(void)
 * 	This routine prints out internal zbm data structures.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_rzbm(void)
{
	const struct rzbm_cell *cellp;
	int i, j;
	struct rzbm *zbmp = rzbm_list.zbm_chunk;
	void print_zbm_cell(const rzbm_t *zbmp, const struct rzbm_cell *cellp);

	debug_printf("zbm_home=%x\n", zbmp->zbm_home);
	debug_printf("cell 4k free 4M free cur map\n");
	debug_printf("---- ------- ------- ------------------------------\n");

	for (j = 0; j < rzbm_list.zbm_nchunk; 
			j++, zbmp = zbmp->zbm_flink) {
		/*
		 *	1. Print the free pool list.
		 */
		cellp = zbmp->zbm_pool;
		if(cellp)
			debug_printf("\n4k free cells\n");
		else
			debug_printf("\nNo 4k free cells\n");
		
		while (cellp && cellp != zbmp->zbm_pool->zbc_blink) {
			print_rzbm_cell(zbmp, cellp);
			if (debug_output_aborted())
				return;
			cellp = cellp->zbc_flink;
		}
		/* print the last cell */
		if(cellp)
			print_rzbm_cell(zbmp, cellp);

		cellp = zbmp->zbm_psepool;
		if(cellp)
			debug_printf("\n4M free cells\n");
		else
			debug_printf("\nNo 4M free cells\n");
		
		while (cellp && cellp != zbmp->zbm_psepool->zbc_blink) {
			print_rzbm_cell(zbmp, cellp);
			if (debug_output_aborted())
				return;
			cellp = cellp->zbc_flink;
		}
		/* print the last cell */
		if(cellp)
			print_rzbm_cell(zbmp, cellp);
		/*
		 *	2. Print all cells.
		 */
		debug_printf("\nAll cells\n");
		cellp = zbmp->zbm_cell;
		for (i = 0; i < zbmp->zbm_ncell; ++i) {
			print_rzbm_cell(zbmp, cellp);
			if (debug_output_aborted())
				return;
			++cellp;
		}
	}
}

#endif /* DEBUG || DEBUG_TOOLS */

#ifdef DEBUG

/*
 * performance measurement and statistics code
 */

/*
 * void
 * rzbm_print_num(int num, const char *title)
 *	Statistics audit routine to print a number (in decimal) together
 *	with a description.
 *
 * Calling/Exit State:
 *	none
 */

static void
rzbm_print_num(int num, const char *title)
{
	debug_printf("%8d      %s\n", num, title);
}

/*
 * void
 * rzbm_print_decimal(ulong_t a, ulong_t b, const char *title)
 *	Statistics audit routine to print a fraction (to 4 decimal digits)
 *	together with a description.
 *
 * Calling/Exit State:
 *	none
 */

static void
rzbm_print_decimal(ulong_t a, ulong_t b, const char *title)
{
	ulong_t c;
	char str[5];

	if (b == 0) {
		debug_printf("    ----      %s\n", title);
		return;
	}

	c = ((a % b) * 10000) / b;
	str[4] = '\0';
	str[3] = (c % 10) + '0'; c /= 10;
	str[2] = (c % 10) + '0'; c /= 10;
	str[1] = (c % 10) + '0'; c /= 10;
	str[0] = (c % 10) + '0';

	debug_printf("%8d.%s %s\n", a / b, str, title);
}

/*
 * void
 * print_rzbm_stats(void)
 *	Statistic audit routine to print out a summary of the statistics.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_rzbm_stats(void)
{
	int j;
	
	rzbm_print_num(rzbm_list.zbm_stats.zbs_npg_free, "physical free requests");
	rzbm_print_num(rzbm_list.zbm_stats.zbs_nfail, "allocation failures");
	rzbm_print_num(rzbm_list.zbm_stats.zbs_nallocated,
			"pages have been allocated");
	rzbm_print_num(rzbm_list.zbm_stats.zbs_nfreed, "pages have been freed");
	rzbm_print_num(rzbm_list.zbm_stats.zbs_nallocated -
			rzbm_list.zbm_stats.zbs_nfreed, "pages are allocated");
	rzbm_print_num(rzbm_list.zbm_maxfree, "pages in the segment");
	rzbm_print_decimal(rzbm_list.zbm_stats.zbs_nallocated -
			rzbm_list.zbm_stats.zbs_nfreed, rzbm_list.zbm_maxfree,
			"of the space is allocated");
	rzbm_print_num(rzbm_list.zbm_stats.zbs_npse_fail, "pse allocation failures");
	rzbm_print_num(rzbm_list.zbm_stats.zbs_npse_allocated,
			"pages have been allocated");
	rzbm_print_num(rzbm_list.zbm_stats.zbs_npse_freed, "pse pages have been freed");
	debug_printf("\n");
}

#endif /* DEBUG */


#ident	"@(#)kern-i386:mem/pse_drv.c	1.5.8.1"
#ident	"$Header$"

#include <fs/file.h>
#include <fs/specfs/specfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pse.h>
#include <mem/rzbm.h>
#include <mem/seg_kmem.h>
#include <mem/seg_dev.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <proc/cg.h>
#include <svc/cpu.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ghier.h>
#include <util/ksynch.h>
#include <util/mod/ksym.h>
#include <util/mod/mod_obj.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/ipl.h>

/*
 * Pseudo driver supporting PSE mappings
 *
 * The driver consists of two parts:
 *
 *	(1) Allocator for device numbers (dev_t) and vnodes
 *		corresponding to the pse pseudo-device.  Minor
 *		device numbers are allocated dynamically, and
 *		physical pages are associated with them.
 *	        the physical pages are allocated from the
 *		idf pool.
 *
 *	(2) The device driver proper, including pse_mmap,
 *		required to implement PSE mappings of
 *		the PSE_PAGESIZE physical pages.
 */

/*
 * STATIC void
 * pse_pagezero(ppid_t ppid)
 *	Zero out a PSE_PAGESIZE chunk of physical memory whose first
 *	physical page has the specified ppid.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The routine zeros out the memory by calling pzero for
 *	each physical page in the PSE_PAGESIZE chunk.
 */
STATIC void
pse_pagezero(ppid_t ppid)
{
	uint_t i;

	ASSERT(ppid != NOPAGE);
	ASSERT(ppid != 0);
	ASSERT((ppid & PSE_NPGOFFSET) == 0);
	for (i = 0 ; i < PSE_NPAGES ; ++i)
		pzero(ppid + i);
}

/*
 * Declarations for device and vnode allocator:
 *
 *	psedevinfo	table of per minor device information:
 *			number of pse pages and list of ppid's for
 *			each minor device
 *
 *	pse_nminors	number of usable minor device numbers
 *
 *	pse_major	major device number (from space.c)
 *
 *	pse_drv_lock	lock guarding the driver.  It in effect
 *			also guards the memory allocator, since
 *			the driver is the only client of the
 *			memory allocator at present.
 */
STATIC struct psedevinfo {
	uint_t psedev_npse;
	ppid_t *psedev_ppidp;
} *psedevinfo;

STATIC uint_t pse_nminors;

extern int pse_major;

STATIC lock_t pse_drv_lock;
LKINFO_DECL(pse_lkinfo, "VM:PSE:pse device driver mutex", 0);

#define	PSE_LOCK()	LOCK_PLMIN(&pse_drv_lock)
#define	PSE_UNLOCK(pl)	UNLOCK_PLMIN(&pse_drv_lock, pl)

/*
 * STATIC void
 * pse_initdev(void)
 *	Initialize device-related information for allocating minor
 *	devices.
 *
 * Calling/Exit State:
 *	Called during startup, by pse_init (Driver init routine).
 */
STATIC void
pse_initdev(void)
{

	/*
	 * does this platform support PSE.
	 */
	if(!PSE_SUPPORTED())
		return;
	/*
	 * is there > PSE_PAGESIZE bytes of idf_memory
	 */
	if( (pse_nminors = (ppid_t)idf_memsize() >> PSE_NPGSHIFT ) == 0)
		return;

	psedevinfo = (struct psedevinfo *)kmem_zalloc(pse_nminors *
                        sizeof(struct psedevinfo), KM_NOSLEEP);
	
	/*
         * If space allocation fails, then act as if we have no devices
         */
        if (psedevinfo == NULL) {
                pse_nminors = 0;
                return;
        }

	LOCK_INIT(&pse_drv_lock, KERNEL_HIER_BASE, PLMIN, &pse_lkinfo,
		KM_NOSLEEP);
}

/*
 * Forward reference
 */
STATIC void pse_freedev(dev_t);

/*
 * STATIC dev_t
 * pse_allocdev(size_t nbytes)
 *	Allocate a new minor device, and associate nbytes worth
 *	of PSE pages with it.
 *
 * Calling/Exit State:
 *	Return value is either ENODEV or a device number which
 *	can be used to map in nbytes worth of PSE pages.
 */
STATIC dev_t
pse_allocdev(size_t nbytes)
{
	int minor;
	uint_t i;
	uint_t npse;
	ppid_t *ppid;
	struct psedevinfo *psep;
	pl_t opl;
	cgnum_t cgnum;
	
	/*
	 * Make a quick test; if not enough free pages, no need to try
	 *	to allocate a device
	 */
	if (pse_nminors == 0)
		return NODEV;
	
	npse = btopser(nbytes);
	if( idf_resv(npse << PSE_NPGSHIFT, NOSLEEP) == 0)
		return NODEV;

	/*
	 * Allocate an array of ppid's to store allocated pages.
	 */
	ppid = (ppid_t *)kmem_alloc(npse * sizeof(ppid_t), KM_SLEEP);

	for (i = 0, cgnum = 0; i < npse; i++) {	
		if ((ppid[i] = idf_page_get(cgnum, PSE_PAGESIZE)) == 0){
			/*
			 * idf fragmented; not enough PSE_PAGES to
			 * satisfy request
			 */
			while ((int)i > 0)
				idf_page_free(ppid[--i], PSE_PAGESIZE );
			kmem_free((char *)ppid, npse * sizeof(ppid_t));
			idf_unresv(npse << PSE_NPGSHIFT );
			return NODEV;
		}
		cgnum++;
		cgnum %= Ncg;
	}
		
	/*
	 * zero the memory
	 */
	for(i=0;i<npse;i++)	
		pse_pagezero(ppid[i]);
	
	/*
	 * Now, find a minor device.  We're guaranteed to find one
	 * since the number of minor devices is equal to the total
	 * number of PSE pages
	 */
	minor = 0;
	opl = PSE_LOCK();
	psep = psedevinfo;
	while (psep->psedev_npse != 0) {
		++minor;
		++psep;
		ASSERT(minor < pse_nminors);
	}
	/*
	 * Fill in the data for the minor device, and release the lock
	 */
	psep->psedev_ppidp = ppid;
	psep->psedev_npse = npse;
	PSE_UNLOCK(opl);

	return makedevice(pse_major, minor);
}

/*
 * STATIC dev_t
 * pse_freedev(size_t nbytes)
 *	Free a previously allocated minor device, and release the pages
 *	associated with it.
 *
 * Calling/Exit State:
 *	pse_lock is held neither on entry nor on exit.
 *
 *	On exit, the device may not be used again until it is
 *	returned by another call to pse_allocdev.  The pages
 *	formerly associated with the device are returned to
 *	the idf allocator.
 */
STATIC void
pse_freedev(dev_t dev)
{
	int minor;
	uint_t i, npse;
	struct psedevinfo *psep;
	pl_t opl;
	ppid_t *ppid;

	ASSERT(dev != ENODEV);
	ASSERT(getemajor(dev) == pse_major);
	minor = geteminor(dev);
	ASSERT((0 <= minor) && (minor < pse_nminors));
	psep = &psedevinfo[minor];
	ASSERT((psep->psedev_npse > 0) && (psep->psedev_ppidp != NULL));
	ppid = psep->psedev_ppidp;
	npse = psep->psedev_npse;
	opl = PSE_LOCK();
	psep->psedev_ppidp = NULL;
	psep->psedev_npse = 0;
	PSE_UNLOCK(opl);
	for (i = 0 ; i < npse ; ++i) {
		ASSERT(ppid[i] != 0);
		ASSERT(ppid[i] != NOPAGE);
		idf_page_free(ppid[i], PSE_PAGESIZE);
	}
	idf_unresv(npse << PSE_NPGSHIFT);
	kmem_free((char *)ppid, npse * sizeof(ppid_t));
}

/*
 * vnode_t *
 * pse_makevp(size_t nbytes, size_t *actual)
 *	Allocate a device and a corresponding vnode, with nbytes worth of
 *	PSE pages, if nbytes is greater than or equal to PSE_PAGESIZE.
 *
 * Calling/Exit State:
 *	Return value is either NULL or a pointer to a vnode which
 *	can be used to map in nbytes worth of PSE pages.
 */
vnode_t *
pse_makevp(size_t nbytes, size_t *actual)
{
	dev_t dev;
	vnode_t *vp;

	if (!PSE_SUPPORTED() || (nbytes < (PSE_PAGESIZE / 4)))
		return (vnode_t *)NULL;
	dev = pse_allocdev(nbytes);
	if (dev == NODEV)
		return (vnode_t *)NULL;
	vp = makespecvp(dev, VCHR);
	if (vp == NULL)
		pse_freedev(dev);
	else
		*actual = roundup(nbytes, PSE_PAGESIZE);
	return vp;
}

/*
 * vnode_t *
 * pse_freevp(vnode_t *vp)
 *	free the PSE pages allocated for the device referenced by vp
 *	and release the vnode.
 *
 * Calling/Exit State:
 *		Called at PLBASE
 *		No locks held.
 */
void
pse_freevp(vnode_t *vp)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	pse_freedev(vp->v_rdev);
	VN_RELE(vp);
}

/*
 * Declarations for the device driver:
 *
 *	pse_devflags		device flags for driver
 *
 *	pse_major		major number (from space.c)
 */
extern int pse_major;	/* major number */

int pse_devflag = D_MP;

extern int segpse_create(struct seg *, void *);
extern int segdev_create(struct seg *, void *);

/*
 * void
 * pse_init(void)
 *	Init routine for PSE pseudo device
 *
 * Calling/Exit State:
 *	None. Called during system startup.
 *
 * Description:
 *	Initializes the device allocator
 */
void
pse_init(void)
{

	pse_initdev();
}

/*
 * int pse_open(dev_t *, int, int, cred_t *)
 *	Open routine for PSE pseudo device.
 *
 * Calling/Exit State: 
 *	Return value is 0 if this is an existing device, ENODEV
 *	if not.
 *
 * Remarks:
 *	The expectation is that we get here through VOP_OPEN of
 *	a vnode created by pse_makevp.
 */
/*ARGSUSED*/
int
pse_open(dev_t *devp, int flag, int type, cred_t *cr)
{
	int minor;
	pl_t opl;

	ASSERT(getemajor(*devp) == pse_major);
	minor = geteminor(*devp);
	opl = PSE_LOCK();
	if ((0 <= minor) && (minor < pse_nminors) &&
			(psedevinfo[minor].psedev_npse != 0)) {
		ASSERT(psedevinfo[minor].psedev_ppidp != NULL);
		PSE_UNLOCK(opl);
		return 0;
	}
	PSE_UNLOCK(opl);
	return ENODEV;
}

/*
 * int
 * pse_close(dev_t, int, cred_t *)
 *      Close routine for PSE pseudo-device
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Calls freedev to free the device, and its associated pages.
 */
/*ARGSUSED1*/
int
pse_close(dev_t dev, int flag, cred_t *cr)
{
	pse_freedev(dev);
	return 0;
}

/*
 * int
 * psemmap(dev_t, off64_t, uint_t)
 *      mmap routine for PSE psuedo-device
 *
 * Calling/Exit State:
 *	Returns ppid for mapping the requested offset for the
 *	device, or NOPAGE if no such ppid.
 *
 * Description:
 *	Split the byte offset into two pieces: btopse64(off)
 *	gives the index into an array of 4MB-aligned ppid's
 *	associated with the device, and pnum(off) is added
 *	to the ppid to get the ppid for the specific page.
 */
/*ARGSUSED2*/
int
pse_mmap(dev_t dev, off64_t off, uint_t prot)
{
	struct psedevinfo *psep;
	int minor = geteminor(dev);

	ASSERT((0 <= minor) && (minor < pse_nminors));
	psep = &psedevinfo[minor];
	if ((0 <= off) && (off < psetob(psep->psedev_npse))) {
		ASSERT(psep->psedev_ppidp != NULL);
		ASSERT(psep->psedev_ppidp[btopse64(off)] != NOPAGE);
		return psep->psedev_ppidp[btopse64(off)] + pnum(off);
	}
	return (NOPAGE);
}

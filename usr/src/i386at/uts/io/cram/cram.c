#ident	"@(#)kern-i386at:io/cram/cram.c	1.10.1.1"
#ident	"$Header$"

/*
 * Driver for PC AT CMOS battery backed up RAM.
 *
 * PC-DOS compatibility requirements:
 *      Nearly all locations have defined values; see
 *      the PC AT Hardware reference manual for details.
 *
 */

#include <io/conf.h> 
#include <util/param.h>
#include <util/types.h>
#include <proc/signal.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <io/cram/cram.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <util/inline.h> 
#include <util/debug.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <io/ddi.h>


#define	CMOSHIER	1


int	cmosinit(void);
int	cmosopen(dev_t *, int, int, cred_t *);
int	cmosclose(dev_t, int, int, cred_t *);
int	cmosread(dev_t, struct uio *, cred_t *);
int	cmoswrite(dev_t, struct uio *, cred_t *);
int	cmosioctl(dev_t, int, void *, int, cred_t *, int *);

int cmosdevflag = D_MP;

STATIC int cmosinitflag = 0;
STATIC lock_t *cmos_mutex;

STATIC LKINFO_DECL(cmos_mutex_lkinfo, "IO:cram:cmos ram mutex lock", 0);

STATIC void CMOSwrite(uchar_t, uchar_t);


/*
 * int 
 * cmosinit(void)
 *
 * Calling/Exit State:
 *	None.
 */
int
cmosinit(void)
{
	if (cmosinitflag)
		return 0;

	if ( !(cmos_mutex = LOCK_ALLOC(CMOSHIER, plhi, 
					&cmos_mutex_lkinfo, KM_NOSLEEP)) ) {
                /*
                 *+ Failed to allocate lock at system init.
                 *+ This is happening at a time where there should
                 *+ be a great deal of free memory on the system.
                 *+ Corrective action:  Check the kernel configuration
                 *+ for excessive static data space allocation or
                 *+ increase the amount of memory on the system.
                 */
                cmn_err(CE_PANIC, "cmosinit: LOCK_ALLOC failed");
        }

	cmosinitflag = 1;

	return 0;
}


/*
 * int
 * cmosopen(dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	Return 0 on success.
 */
/* ARGSUSED */
int
cmosopen(dev_t *devp, int flag, int otyp, cred_t *cred_p)
{
	int error;

	if ((flag & FWRITE) && (error = drv_priv(cred_p)))
		return error;

	return 0;
}


/*
 * int
 * cmosread(dev_t, struct uio *, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
cmosread(dev_t dev, struct uio *uio_p, cred_t *cred_p)
{
	return 0;
}


/*
 * int
 * cmoswrite(dev_t, struct uio *, cred_t *)
 * 
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
cmoswrite(dev_t dev, struct uio *uio_p, cred_t *cred_p)
{
	return 0;
}


/*
 * int
 * cmosioctl(dev_t, int, void *, int, cred_t *, int *)
 *
 * Calling/Exit State:
 *	Return 0 on success, otherwise return error code.
 */
/* ARGSUSED */
int
cmosioctl(dev_t dev, int cmd, void *addr, int mode, 
		cred_t *cred_p, int *rval_p)
{
	uchar_t uch[2];

	switch (cmd) {
	case CMOSREAD:
		if (copyin(addr, &uch[0], 1))
			return EFAULT;

		if (uch[0] < 0 || uch[0] > 0x3f)
			return ENXIO;

		uch[1] = CMOSread(uch[0]);

		if (copyout(&uch[1], (uchar_t *)addr + 1, 1))
			return EFAULT;

		break;

	case CMOSWRITE:
		if (!(mode & FWRITE))
			return EACCES;

		if (copyin(addr, uch, 2))
			return EFAULT;

		if (uch[0] < DSB || uch[0] > 0x3f)
			return ENXIO;

		CMOSwrite(uch[0], uch[1]);

		break;

	default:
		return EINVAL;
	}

	return 0;
}


/*
 * int
 * cmosclose(dev_t, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
cmosclose(dev_t dev, int flags, int otyp, cred_t *cred_p)
{
	return 0;
}


/*
 * uchar_t
 * CMOSread(uchar_t)
 *	routine to read contents of a location in the PC AT CMOS RAM.
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 *	- The value read from CMOS RAM is returned.
 */
uchar_t
CMOSread(uchar_t addr)
{
	uchar_t uch;
	pl_t pl;

	if (cmosinitflag)
		pl = LOCK(cmos_mutex, plhi);

	outb(CMOS_ADDR, addr); /* address to read from in CMOS RAM */
	uch = inb(CMOS_DATA);

	if (cmosinitflag)
		UNLOCK(cmos_mutex, pl);

	return uch;		/* return the value from the RAM */
}


/*
 * STATIC void
 * CMOSwrite(uchar_t, uchar_t)
 *	routine to write the contents of a location in the PC AT CMOS RAM.
 *
 * Calling/Exit State:
 *	- No locks are held on entry/exit.
 */
STATIC void
CMOSwrite(uchar_t addr, uchar_t val)
{
	pl_t pl;

	pl = LOCK(cmos_mutex, plhi);
	outb(CMOS_ADDR, addr);
	outb(CMOS_DATA, val);
	UNLOCK(cmos_mutex, pl);
}

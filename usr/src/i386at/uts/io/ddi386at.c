#ident	"@(#)kern-i386at:io/ddi386at.c	1.5.7.1"
#ident	"$Header$"

/*
 *	i386at-specific Device Driver Interface functions          
 *
 * This file contains support for i386at extensions to the DDI.
 */ 

#define _DDI_C

#include <io/conf.h>
#include <svc/bootinfo.h>
#include <svc/cpu.h>
#include <svc/memory.h>
#include <svc/systm.h>
#include <svc/v86bios.h>
#include <util/inline.h>
#include <util/plocal.h>
#include <util/types.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

/*
 * EISA hardware-related defines used by EISA_BRDID subfunction:
 */
#define MAX_EISA_SLOT		16
#define EISA_SLOT_IO_BASE	0x1000
#define EISA_BRDID_ADDRESS	0xC80
#define EISA_BRD_BUSY		0x70
#define EISA_BRD_RESERVED	0x80

char *hdparmstr[2];
struct hdparms hdparms[2];

/*
 * void
 * ddi_init_p(void)
 *	Platform-specific initialization.
 */
void
ddi_init_p(void)
{
	intregs_t regs;
	struct hdparms *hdp;
	int unit;
	char *p;
	long val;

	extern char *bs_lexnum(const char *, long *);

	/*
	 * Fetch drive parameters obtained by the bootstrap, for HD_PARMS.
	 */
	for (unit = 0; unit <= 1; unit++) {
		hdp = &hdparms[unit];
		hdp->hp_unit = unit;
		/* Note: hp_precomp and hp_lz not used, so don't fill in */

		p = hdparmstr[unit];
		p = bs_lexnum(p, &val) + 1;
		hdp->hp_ncyls = val;
		p = bs_lexnum(p, &val) + 1;
		hdp->hp_nheads = val;
		(void)bs_lexnum(p, &val);
		hdp->hp_nsects = val;
	}
}

int
eisa_brdid(uint_t slot, uint_t *idp)
{
	long slotaddr;
	uchar_t pval;

	if (!(bootinfo.machflags & EISA_IO_BUS))
		return -1;

	if (slot > MAX_EISA_SLOT)
		return -1;

	slotaddr = EISA_SLOT_IO_BASE * slot + EISA_BRDID_ADDRESS;

	/* PreCharge the ID port. */
	outb(slotaddr, 0xFF);

	/*
	 * Check if the board has a readable ID:
	 *	(pval == 0xFF): port non-existent
	 *	(pval & EISA_BRD_RESERVED): reserved bit, must be 0
	 *	(pval & EISA_BRD_BUSY) == EISA_BRD_BUSY:
	 *	    if all bits set, board is not ready
	 *	Other bits in pval are don't care.
	 */
	pval = inb(slotaddr);
	if (pval == 0xFF || (pval & EISA_BRD_RESERVED) ||
	    (pval & EISA_BRD_BUSY) == EISA_BRD_BUSY)
		return -1;

	*idp = inl(slotaddr);

	return 0;
}

int
eisa_sysbrdid(uint_t *idp)
{
	return eisa_brdid(0, idp);
}

/*
 * int
 * _Compat_drv_gethardware(ulong_t parm, void *valuep)
 *	Get i386at hardware-specific information for drivers.
 *
 * Calling/Exit State:
 *	The information accessed here is all read-only after sysinit,
 *	so no locking is necessary.
 */
int
_Compat_drv_gethardware(ulong_t parm, void *valuep)
{
	switch (parm) {

	case PROC_INFO: { /* Get processor-type info. */

		struct cpuparms *cpup = valuep;

		struct_zero(cpup, sizeof (struct cpuparms));

		/*
		 * Take advantage of the fact that cpu_id is
		 * set to 3 for 386, 4 for 486, 5 for 586...
		 */
		cpup->cpu_id = l.cpu_id - 2;
		cpup->cpu_step = l.cpu_stepping;
		break;
	}

	case IOBUS_TYPE: { /* Are we ISA (std AT), EISA or MCA? */

		if (bootinfo.machflags & MC_BUS)
			*(ulong_t *)valuep = BUS_MCA;
		else if (bootinfo.machflags & EISA_IO_BUS)
			*(ulong_t *)valuep = BUS_EISA;
		else
			*(ulong_t *)valuep = BUS_ISA;
		break;
	}

	case TOTAL_MEM: { /* Return the number of bytes of physical memory. */

		*(ulong_t *)valuep = totalmem;
		break;
	}

	case DMA_SIZE: { /* How many bits of DMA addressing have we? */

		/* 24-bit is standard for MCA, EISA and ISA */
		*(ulong_t *)valuep = 24;
		break;
	}


	case BOOT_DEV: { /* What device did we boot from? */

		struct bootdev *bdevp = valuep;

		/*
		 * The struct_zero will set bdevp->bdv_unit to 0.
		 * Since on i386at machines, one always boots
		 * off the first hard disk or first flop,
		 * the unit number is always 0.
		 */
		struct_zero(bdevp, sizeof(struct bootdev));
		bdevp->bdv_type = BOOT_DISK;
		if (bootinfo.bootflags & BF_FLOPPY)
			bdevp->bdv_type = BOOT_FLOPPY;
		break;
	}

	case HD_PARMS: { /* Obtain parameters on 2 primary disks.
			  * Note that this function is not extensible
			  * to additional disks, as information for them
			  * is not available from bootinfo.
			  */

		ulong_t drive_num;
		struct hdparms *hdp = valuep;

		/* only "unit" values of 0 or 1 are OK */
		if ((drive_num = hdp->hp_unit) > 1)
			return -1;
		*hdp = hdparms[drive_num];
		break;
	}

	case EISA_BRDID: { /* Return EISA Board ID.
			    * The desired slot number is passed in via valuep.
			    * Only returns the lower 2 bytes.
			    */

		uint_t id;
		int retval;

		retval = eisa_brdid(*(long *)valuep, &id);
		*(long *)valuep = id & 0xFFFF;
		return retval;
	}

	default: /* bad parm value */
		return -1;
	}

	return 0; /* success */
}

/*
 * int
 * _Compat_drv_gethardware_7_2(ulong_t parm, void *valuep)
 *	DDI 7.2 version of drv_gethardware().
 *
 * Calling/Exit State:
 *	The information accessed here is all read-only after sysinit,
 *	so no locking is necessary.
 */
int
_Compat_drv_gethardware_7_2(ulong_t parm, void *valuep)
{
	switch (parm) {
	case TOTAL_MEM:
	case DMA_SIZE:
		return -1;
	}
	return _Compat_drv_gethardware(parm, valuep);
}

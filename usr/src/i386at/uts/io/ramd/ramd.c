/*		copyright	"%c%" 	*/

#ident	"@(#)kern-i386at:io/ramd/ramd.c	1.3.1.1"
#ident	"$Header$"

static char ramd_copyright[] = "Copyright 1987, 1988, 1989 Intel Corp. 462679";

/*
 * 	TITLE:	RAM Disk Driver
 *
 * 	This driver manages "memory". Memory is defined by a
 *	info structure which gives the size of the memory to be managed.
 *	Note that for a given system, there may be restrictions on the 
 *	configuration of the address and size of the memory.
 *
 * general structure of driver:
 *	ramdinit - initialize driver. verify (fix up if necessary) the
 *			device configuration and report errors.
 *	ramdopen - 'open' the RAM disk, allocate memory if required,
 *			load from secondary device if specified.
 *	ramdclose- closes the disk.
 *	ramdstrat- do the actual work. copy in/out of ram disk memory
 *	ramdioctl- handle all of the funnies such as building roving RAM disks
 */

#include <fs/buf.h>
#include <io/conf.h>
#include <io/open.h> 
#include <io/ramd/ramd.h>
#include <io/target/st01/tape.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>

extern int	ramd_tape_loc;
extern int	ramd_load_type;
extern int	SwapRamDiskSize;
extern int	RootRamDiskSize;
extern char	RootRamDiskBuffer[];


STATIC int	ramd_load();
STATIC void	ramd_boot_fail();
STATIC int	ramd_issue_read();


int ramddevflag = D_NOBRKUP;

int
ramdinit()
{
	minor_t	unit;

	cmn_err(CE_CONT, "RAM Disk Driver, Copyright (c) 1986, 1987, 1988, 1989 Intel Corp.\n");

	for (unit = 0; unit < ramd_num; unit++) {	/* for each RAM disk */
		ramd_info[unit].ramd_state = RAMD_ALIVE;
		if (ramd_info[unit].ramd_flag & RAMD_RUNTIME)
		     cmn_err(CE_CONT, "RAM Disk %d runtime definable\n", unit);
	}

	return 0;
}

int
ramdsize(dev)
	dev_t dev;
{
	minor_t unit = getminor(dev);	

	if (unit >= ramd_num) {
		ramdprint(dev, "invalid ramdsize argument");
		return -1;
	}
	return (ramd_info[unit].ramd_size >> RAMD_DIV_BY_512);
}

STATIC int
ramd_alloc(unit, size)
	minor_t	unit;
	ulong	size;
{
	_VOID	*addr;

	if (ramd_info[unit].ramd_state & RAMD_ALLOC)
		return EINVAL;

	if (addr = kmem_zalloc(size, KM_NOSLEEP)) {
		ramd_info[unit].ramd_state |= RAMD_ALLOC;
		ramd_info[unit].ramd_addr = (caddr_t)addr;
		ramd_info[unit].ramd_size = size;
		printf("RAM disk %d allocated, size = %dKb\n",
			unit, ramd_info[unit].ramd_size >> RAMD_DIV_BY_1024);
		/* DEBUG */
		{int i;
			for (i=0;i<100000000;i++);
		}
		return 0;
	}

	printf("RAM disk %d NOT allocated, size = %dKb\n",
		unit, ramd_info[unit].ramd_size >> RAMD_DIV_BY_1024);
	/* DEBUG */
	{int i;
		for (i=0;i<100000000;i++);
	}
	/* couldn't get the required memory for the RAM disk */
	return ENOMEM;
}

STATIC int
ramd_free(unit)
	minor_t	unit;
{
	register struct ramd_info *dp = &ramd_info[unit];

	if( unit == getminor(rootdev))
		return(0);

	if (!(dp->ramd_state & RAMD_ALLOC)) 
		return EINVAL;

	if (dp->ramd_size && dp->ramd_addr)
		kmem_free(dp->ramd_addr, dp->ramd_size);

	dp->ramd_size = 0;
	dp->ramd_addr = NULL;
	dp->ramd_state &= ~RAMD_ALLOC;
	return 0;
}

/*
 * open checks to see if the disk is alive,
 * possibly allocates memory for the RAM disk,
 * and loads the memory from secondary storage if specified.
 */

int
ramdopen(devp, flag, otyp, cred_p)
	dev_t *devp;		/* Device Number */
	int flag;		/* flag with which the file was opened */
	int otyp;		/* type of open (OTYP_BLK|OTYP_CHAR| ..) */
	struct cred *cred_p;	/* pointer to the user credential structure */
{
	minor_t unit = getminor(*devp);
	struct ramd_info *dp;			
	dev_t dev = *devp;	
	
	if (unit >= ramd_num)
		return ENXIO;

	dp = &ramd_info[unit];

	if ((dp->ramd_state & RAMD_ALIVE) == 0)
		return ENXIO;

	if ((dp->ramd_state & RAMD_OPEN) == 0) {
		if (dev == rootdev) {
			dp->ramd_state |= RAMD_ALLOC;
			dp->ramd_addr = RootRamDiskBuffer;
			dp->ramd_size = RootRamDiskSize;
		}

		/*
		 * If partition size has been specified and memory not yet
		 * allocated, try to allocate it now.  (This will be the case
		 * for first open of ramd root and swap partitions.)
		 */

		if (dp->ramd_size > 0 && (dp->ramd_state & RAMD_ALLOC) == 0) {
			if (ramd_alloc(unit, dp->ramd_size) != 0)
				return ENOMEM;

			if (dp->ramd_flag & RAMD_LOAD) {
				dev_t d	= makedevice(dp->ramd_maj, dp->ramd_min);

				if (ramd_load(unit, dp->ramd_size, d) != 0) {
					if (dev == rootdev) {
						ramd_boot_fail();
						/* NOTREACHED */
					}
					return ENOMEM;
				}
			}
		}
	}

	if (otyp == OTYP_LYR)
		dp->ramd_ocnt++;
	else
		dp->ramd_otyp |= (1 << otyp);

	dp->ramd_state |= RAMD_OPEN;
#ifdef RAMD_DEBUG
	printf("RAM disk %d opened, ocnt = %d, otyp = 0x%x\n",
		unit, dp->ramd_ocnt, dp->ramd_otyp);
#endif
	return 0;
}

int
ramdclose(dev, flag, otyp, cred_p)
	dev_t dev;				
	int flag;			
	int otyp;		
	struct cred *cred_p;
{
	register struct ramd_info *dp = &ramd_info[getminor(dev)];

	ASSERT(otyp < OTYPCNT);
	ASSERT( !(otyp == OTYP_LYR && dp->ramd_ocnt < 1) );

	if (otyp == OTYP_LYR)
		--dp->ramd_ocnt;
	else
		dp->ramd_otyp &= ~(1 << otyp);

	if (dp->ramd_ocnt == 0 && dp->ramd_otyp == 0)
		dp->ramd_state &= ~RAMD_OPEN;

#ifdef RAMD_DEBUG
	printf("RAM disk %d closed, ocnt = %d, otyp = 0x%x %s\n",
		getminor(dev), dp->ramd_ocnt, dp->ramd_otyp,
		(dp->ramd_state & RAMD_OPEN)? "(still open)" : "");
#endif
	return 0;
}

void
ramdstrategy(bp)
	struct buf *bp;
{
	int	count;			/* count of bytes to transfer */
	off32_t	offset;			/* offset of transfer */
	minor_t	unit;			/* RAM disk unit */
	register struct ramd_info *dp;	/* ptr to control structure */
	
	if ((unit = getminor(bp->b_edev)) >= ramd_num ||
	    (dp = &ramd_info[unit])->ramd_state != (RAMD_ALIVE|RAMD_OPEN|RAMD_ALLOC)) {
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
		
	/* 
	 * 	request within bounds of RAM disk? 
	 *	if it is set b_resid to 0 else set it to b_count
	 * 	and leave.
	 */
	offset = bp->b_blkno*NBPSCTR;
	if (offset >= dp->ramd_size) {
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		return;
	} else
		bp->b_resid = 0;

	/* 
	 * compute transfer size
	 * if larger than size of ramdisk just write 
	 * what we can, and set b_resid to the rest.
 	 */
	count = bp->b_bcount;
	if (offset+bp->b_bcount > dp->ramd_size) {
		count = dp->ramd_size - (ulong)offset;
		bp->b_resid = bp->b_bcount - count;
	}

	/* perform the copy */

	if (bp->b_flags & B_PAGEIO)
		bp_mapin(bp);

	if (bp->b_flags & B_READ)
		bcopy(dp->ramd_addr+offset, bp->b_un.b_addr, count);
	else
		bcopy(bp->b_un.b_addr, dp->ramd_addr+offset, count);

	biodone(bp);
	return;
}

/* 
 * raw read, use physio.
 */

int
ramdread(dev, uio_p, cred_p)
	dev_t	dev;
	struct uio *uio_p;	
	struct cred *cred_p;
{
	return physiock(ramdstrategy, NULL, dev, B_READ, ramdsize(dev), uio_p);
}

/* 
 * raw write, use physio.
 */

int
ramdwrite(dev, uio_p, cred_p)
	dev_t	dev;
	struct uio *uio_p;	
	struct cred *cred_p;
{
	return physiock(ramdstrategy, NULL, dev, B_WRITE, ramdsize(dev),uio_p);
}

/* 
 * Ioctl routine required for RAM disk.
 */

/*ARGSUSED*/
int
ramdioctl(dev, cmd, cmdarg, mode, cred_p, rval_p)
	dev_t	dev;		/* major, minor numbers */
	int	cmd;		/* command code */
	caddr_t cmdarg;		/* user structure with parameters */
	int	mode;		/* mode when device was opened (not used) */
	struct cred *cred_p;	/* pointer to the user credential structure */
	int	*rval_p;	/* pointer to return value for caller */
{
	minor_t	unit = getminor(dev);	
	ulong	ramd_size;		
	int	ramd_error = 0;	
	dev_t	d;
	register struct ramd_info *dp;
	struct ramd_info ramd_stats; 

	if (unit >= ramd_num)
		return ENXIO;

	dp = &ramd_info[unit];

	switch (cmd) {
	case RAMD_IOC_GET_INFO:
		if (copyout((caddr_t)dp, cmdarg, sizeof(struct ramd_info)))
			ramd_error = EFAULT;
		break;

	case RAMD_IOC_R_ALLOC:
		if (copyin(cmdarg, (caddr_t)&ramd_size, sizeof(ramd_size))) {
			ramd_error = EFAULT;	/* can't access */
			break;
		}
		ramd_error = ramd_alloc(unit, ramd_size);
		break;

	case RAMD_IOC_R_FREE:
		/*
		 * Don't free memory if RAM disk is busy via a layered
		 * open, or is open as a block device, mounted, etc.
		 * (Someone else may still be using the raw device,
		 * though; we can't protect against that.)
		 */
		if (dp->ramd_ocnt || (dp->ramd_otyp & ~(1 << OTYP_CHR)))
			ramd_error = EBUSY;
		else
			ramd_error = ramd_free(unit);
		break;

	case RAMD_IOC_LOAD:
		if (copyin(cmdarg, (caddr_t)&ramd_size, sizeof(ramd_size))) {
			ramd_error = EFAULT;	/* can't access */
			break;
		}
		if ((dp->ramd_state & RAMD_ALLOC) == 0
			|| dp->ramd_size < ramd_size) {
			ramd_error = ENXIO;
			break;
		}
		d = makedevice(dp->ramd_maj, dp->ramd_min);
		(void) ramd_load(unit, ramd_size, d);
		break;

	default:
		ramd_error = EINVAL;	/* bad command */
		break;
	}

	return ramd_error;
}

/*
 * Print error message; called from kernel via bdevsw.
 */

int
ramdprint(dev, str)
	dev_t	dev;
	char	*str;
{
	cmn_err(CE_NOTE, "%s on RAM disk partition %d\n", str, getminor(dev));
	return 0;
}

/*
 * Fatal error - unable to load root partition
 * from secondary storage device.
 * Called from ramdopen().
 */

STATIC void
ramd_boot_fail()
{
	if (ramd_load_type == RAMD_TAPE_LOAD)
		cmn_err(CE_NOTE, "System cannot load boot tape, contact your service representative.");
	else
		cmn_err(CE_NOTE, "System cannot load boot floppy, contact your service representative.");
	cmn_err(CE_PANIC, "System installation aborted.");
	/* NOTREACHED */
}

STATIC int
ramd_load(unit, size, dev)
	minor_t	unit;
	ulong size;
	dev_t dev;
{
	int error, i, sect, remcount;
	ulong remainder;
	struct cred cred;
	major_t maj = getmajor(dev);

	/* open the auto load driver */

	error = (*bdevsw[maj].d_open)(&dev, 0, OTYP_CHR , &cred);
	if (error) {
		(void)(*bdevsw[maj].d_close)(dev, 0, OTYP_CHR, &cred);
		return error;
	}

	if (ramd_load_type == RAMD_TAPE_LOAD){
		error = (*cdevsw[maj].d_ioctl)(dev, T_RWD, 0, 0, 0, 0);
		error = (*cdevsw[maj].d_ioctl)(dev, T_SFF, ramd_tape_loc,0,0,0);
		if (error){
			ramdprint(dev, "Failed seek to file mark");
			return ENXIO;
		}
	}

	if (ramd_load_type == RAMD_TAPE_LOAD){
		error = ramd_issue_read(size, ramd_info[unit].ramd_addr,
			0L, dev, unit, maj);
		error = (*cdevsw[maj].d_ioctl)(dev, T_SFF, 0, 0, 0, 0);

	} else { 	/* Floppy */
           if ((remainder = size % RAMD_GRAN) != 0)
		size = (size / RAMD_GRAN) * RAMD_GRAN;
	   for (i=0,sect=0; (((i*RAMD_GRAN+1) < size) && !error); i++,sect+=8) {
		error = ramd_issue_read(RAMD_GRAN, ramd_info[unit].ramd_addr + 
			(i * RAMD_GRAN), sect, dev, unit, maj);
		remcount = i;
	   }
           if ((remainder > 0) && !error)
		error = ramd_issue_read(remainder, ramd_info[unit].ramd_addr +
			(++remcount * RAMD_GRAN), sect, dev, unit, maj);
	}

	/* close the driver */

	(void)(*bdevsw[maj].d_close)(dev, 0, OTYP_CHR, &cred);
	return error;
}

STATIC int
ramd_issue_read(rdsize, addr, sect, dev, unit, maj)
	ulong rdsize;
	caddr_t addr;
	int sect;
	dev_t dev;
	minor_t unit;
	major_t maj;
{
	extern struct buf ramd_buf[];
	struct buf *bp = &ramd_buf[unit];
	int error;

	/* Setup the request buffer.  */
	bp->b_flags = B_READ|B_BUSY;
	bp->b_error = 0;
	bp->b_blkno = sect;
	bp->b_sector = sect;
	bp->b_edev = dev;
	bp->b_proc = 0;
	bp->b_bcount = rdsize;
	bp->b_resid = 0;
	bp->b_un.b_addr = addr;

	/* perform the read request */
	(*bdevsw[maj].d_strategy)(bp);

	biowait(bp);
	return geterror(bp);
}

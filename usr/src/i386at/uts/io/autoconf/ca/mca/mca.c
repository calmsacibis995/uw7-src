#ident	"@(#)kern-i386at:io/autoconf/ca/mca/mca.c	1.12.3.1"
#ident	"$Header$"

/*
 * Autoconfig -- CA/MCA Interface routines.
 *
 * For a better understanding of all bit manipulations in this source file
 * please refer to your PS/2 technical reference manual.
 */

#ifdef _KERNEL_HEADERS

#include <util/cmn_err.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <io/conf.h>
#include <io/ddi.h>
#include <io/autoconf/ca/mca/mca.h>
#include <io/autoconf/ca/ca.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <svc/bootinfo.h>

#else

#include <sys/cmn_err.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <sys/stream.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/sysmacros.h>
#include <sys/ddi.h>
#include <sys/conf.h>
#include <sys/mca.h>
#include <sys/bootinfo.h>

#endif /* _KERNEL_HEADERS */


STATIC LKINFO_DECL(mca_lockinfo, "AUTO-CONF:mca:mca_lock", 0);
lock_t *mcalock;

int mcadevflag = D_MP;

unsigned char *ppos, *pt;
ushort mcainited  = 0;
ulong mca = 0;

int
mca_init(void)
{
	int i,j;

	if (mcainited)
		return 0;

	if (!(mcalock = LOCK_ALLOC(MCA_HIER, plhi, &mca_lockinfo,
					KM_NOSLEEP)))
		return -1;
	/*
	 * Allocate memory for all the eight slots, selectively enable
	 * the adapter in each slot, read in the values and initialize
	 * the data structure.
	 */

	ppos = kmem_zalloc((NUM_SLOTS * POS_REGS_PER_SLOT), KM_NOSLEEP);
	if ((pt = ppos) == NULL)
		return -1;

	for (i = 0; i < NUM_SLOTS; i++) {
		/* Enable adapter in slot i for setup */
		outb(ADAP_ENAB, ((unsigned char)(i|NUM_SLOTS)));
		for (j = 0; j < POS_REGS_PER_SLOT; j++)
			*pt++ = inb(POS_0  + j);

		outb(ADAP_ENAB, 0);
	}
	mcainited = 1;
	return 0;
}

/*
 * int
 * mca_verify(void)
 *
 * Calling/Exit State:
 *	Return 0 on success.
 */
int
mca_verify(void)
{
	int	i, j;

	if (!(bootinfo.machflags & MC_BUS))
		return -1;

	if (mca_init() == -1) {
		cmn_err(CE_WARN, "Unable to initialize the MCA sub-system/driver\n");
		return -1;
	}
	return 0;
}

/*
 * ushort_t
 * mca_give_id(ushort_t slot)
 *	Given a slot return the ID 
 *
 * Calling/Exit State:
 *	Return the board id for <slot>.
 */
ushort_t
mca_give_id(ushort_t slot)
{
	unsigned char *pt;

	if (slot > NUM_SLOTS)
		return 0xffff;
	
	pt = ppos + (slot * POS_REGS_PER_SLOT);

	return ((*(pt + 1) << 8) | *pt);
}


/*
 * int
 * ca_mca_init(void)
 *	Register board id for each device in the slot. 
 *
 * Calling/Exit State:
 *	Return 0 on success.
 */
int
ca_mca_init(void)
{
	int     slot;
	ushort_t id;
	struct config_info *cip;

	for (slot = 0; slot < MCA_MAX_SLOTS; slot++) {
		if ((id = mca_give_id(slot)) == 0xffff)
			continue;
		CONFIG_INFO_KMEM_ZALLOC(cip);
		cip->ci_busid |= CM_BUS_MCA;
		cip->ci_mca_slotnumber = slot;
		cip->ci_mcabrdid = id;
	}

	return (0);
}

/*
 * size_t
 * ca_mca_devconfig_size(ushort slot)
 *
 * Calling/Exit State:
 *	Return the maxmimum size of configuration space for <slot>.
 */
size_t
ca_mca_devconfig_size(ushort_t slot)
{
	ushort_t id;

	if (!mcainited)
		return 0;

	if ((id = mca_give_id(slot)) == 0xffff)
		return 0;

	return (POS_REGS_PER_SLOT);
}

/*
 * void
 * mca_read(ushort_t slot, void *buf, size_t offset, size_t nbyte)
 *
 * Calling/Exit State:
 *	Return number of bytes read.
 */
int
mca_read(ushort_t slot, void *buf, size_t offset, size_t nbyte)
{
	uchar_t *mcap;
	ushort_t id;
	int i = 0;
	pl_t opri;
	uchar_t pos_reg = (uchar_t)offset;

	if (!mcainited)
		return -1;

	if ((id = mca_give_id(slot)) == 0xffff)
		return -1;

	if ((offset > (POS_REGS_PER_SLOT -1)) || 
				((offset + nbyte) > POS_REGS_PER_SLOT))
		return -1;

	mcap = (uchar_t *)buf;

	/*
	 * We want only kernel context enabling and reading 
	 * from the shared I/O space.
	 */
		
	opri = LOCK(mcalock, plhi);	/* Lock access to I/O space */
	outb(ADAP_ENAB, ((unsigned char)(slot|NUM_SLOTS)));

	for (i = 0; i < nbyte; i++, pos_reg++)
		*mcap++ = inb(POS_0 + pos_reg);

	outb(ADAP_ENAB, 0);

	UNLOCK(mcalock, opri); 	/* Release the lock */
	return i;
}

/*
 * void
 * mca_write(ushort slot, void *buf, size_t offset, size_t nbytes)
 *
 * Calling/Exit State:
 *	Return number of bytes written.
 */
int
mca_write(ushort_t slot, void *buf, size_t offset, size_t nbyte)
{
	uchar_t *mcap;
	uchar_t pos_reg;
	uchar_t	tpos;
	ushort_t id;
	int i;
	pl_t opri;

	if (!mcainited)
		return -1;

	if ((id = mca_give_id(slot)) == 0xffff)
		return -1;

	if ((offset > (POS_REGS_PER_SLOT -1)) || 
				((offset + nbyte) > (POS_REGS_PER_SLOT -1)))
		return -1;

	pos_reg = offset;
	mcap = (uchar_t *)buf;

	/*
	 * We want only kernel context enabling and reading from the 
	 * shared I/O space.
	 */

	opri = LOCK(mcalock, plhi);	/* Get the lock */

	/* Select adapter to be placed in setup */
	outb(ADAP_ENAB, ((unsigned char)(slot | NUM_SLOTS)));

	tpos = inb(POS_2);      /* Disable the adapter and place it in setup */ 
	tpos  &= ~0x1;
        outb(POS_2, tpos);

	for (i = 0; i < nbyte; i++, pos_reg++,mcap++)
		outb((POS_0 + pos_reg), *mcap);

        tpos = inb(POS_2);
        tpos  |=  0x1;
        outb(POS_2, tpos);	/* Enable the adapter and disable setup */
	outb(ADAP_ENAB,0);      /* Deselect adapter */

	UNLOCK(mcalock, opri);	/* Drop the lock */

	return i;	/* Return the no of bytes written */
}

/*
** int
** mca_read_devconfig8( ushort_t slot, uchar_t *buf, size_t offset )
**
** Calling/Exit State:
**	Return 0 for success, else EINVAL
*/

int
mca_read_devconfig8( ushort_t slot, uchar_t *buf, size_t offset )
{
	pl_t		opri;

#ifdef CM_TEST
	printf( "In mca_read_devconfig8()\n" );
#endif /* CM_TEST */

	if ( mcainited == 0  ||  offset > ( POS_REGS_PER_SLOT - 1 )  ||
					mca_give_id( slot ) == 0xffff )
		return EINVAL;

	/*
	** We want only kernel context enabling and reading 
	** from the shared I/O space.
	*/
		
	opri = LOCK( mcalock, plhi );	/* Lock access to I/O space */
	outb( ADAP_ENAB, (unsigned char)( slot | NUM_SLOTS ));

	*buf = inb( POS_0 + (uchar_t)offset );

	outb( ADAP_ENAB, 0 );

	UNLOCK( mcalock, opri ); 	/* Release the lock */
	return 0;
}

/*
** int
** mca_write_devconfig8( ushort slot, uchar_t *buf, size_t offset )
**
** Calling/Exit State:
**	Return 0 for success, else EINVAL.
*/

int
mca_write_devconfig8( ushort_t slot, uchar_t *buf, size_t offset )
{
	uchar_t	tpos;
	pl_t	opri;

#ifdef CM_TEST
	printf( "In mca_write_devconfig8()\n" );
#endif /* CM_TEST */

	if ( mcainited == 0  ||  offset > ( POS_REGS_PER_SLOT - 1 )  ||
					mca_give_id( slot ) == 0xffff )
		return EINVAL;

	/*
	** We want only kernel context enabling and reading from the 
	** shared I/O space.
	*/

	opri = LOCK( mcalock, plhi );	/* Get the lock */

	/* Select adapter to be placed in setup */

	outb( ADAP_ENAB, (uchar_t)( slot | NUM_SLOTS ));

	tpos = inb( POS_2 );      /* Disable the adapter and put it in setup */ 
	tpos &= ~0x1;
	outb( POS_2, tpos );

	outb(POS_0 + (uchar_t)offset, *buf );

	tpos = inb( POS_2 );
	tpos |= 0x1;
	outb( POS_2, tpos );	/* Enable the adapter and disable setup */
	outb( ADAP_ENAB,0 );	/* Deselect adapter */

	UNLOCK( mcalock, opri );	/* Drop the lock */
	return 0;
}

pl_t
mca_lock( void )
{
	return LOCK( mcalock, plhi );  /* Get the lock */
}

void
mca_unlock( pl_t opri )
{
	 UNLOCK( mcalock, opri );   /* Drop the lock */
}

#ifdef NOTYET 

int
mcaioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cred_p,int *rval_p)
{
	mca_arg_t mca_arg;
	mca_resource_t *mcap;
	uchar_t buf[8];
	size_t offset;
	int i;

	switch(cmd) {
	case MCA_SET_SLOT:
		if (copyin((caddr_t)arg, (caddr_t)&slot, sizeof(ulong_t)))
			return EINVAL;
		return 0;
	case MCA_SET_OFFSET:
		if (copyin((caddr_t)arg, (caddr_t)&offset, sizeof(size_t)))
			return EINVAL;
		return 0;
	case MCA_READ_POS:
		if (ca_mca_read_nvm(slot, buf) == 0xff)
			return EINVAL;
		if (copyout((caddr_t)buf, (caddr_t)arg, 8))
                	return EFAULT;
		return 0;
	case MCA_READ:
		for (i = 0; i < 8 ; i++)
			buf[i] = 0x00;
		(void)mca_read(slot, buf, offset, 1);
		if (copyout((caddr_t)buf, (caddr_t)arg, 8))
                	return EFAULT;
		return 0;
	}
	return 0;
}

mcaopen(dev_p,flags,otype,cred_p)
dev_t *dev_p;
int flags,otype;
struct cred *cred_p;
{
	return 0;
}
mcaclose(dev_p,flags,otype,cred_p)
dev_t *dev_p;
int flags,otype;
struct cred *cred_p;
{
	return 0;
}


/* Retrieve all the necessary information about the parallel port */

mca_resource_t *
mca_parallel_param(void)
{
	unsigned char pval;
	unsigned short off;
	pl_t opri;
	mca_resource_t *mcap = NULL;
	extern mca_resource_t mca_parallel[];

	opri = LOCK(mcalock, plhi); /* Lock access to shared memory */

	outb(ADAP_ENAB, 0);	/* The system setup and adapter setup should
				 * not be enabled at the same time.
				 */
	outb(SYS_ENAB, SYS_ENABLE_MASK);
	pval = inb(POS_2);
	outb(SYS_ENAB, 0xff);  	/* We have the input value so restore the
				 * SYS ENABLE port to a sane state.
				 */

	if (pval & 0xf0) {	/* If parallel port is enabled */
		pval &= 0x70;
		pval >>= 4;
		off = (pval & 0x4) ? 2 : ((pval & 0x2) ? 1 : 0);
		mcap = &mca_parallel[off];
	}
	UNLOCK(mcalock, opri);
	return mcap;
}

/* Retrieve all the necessary information about the serial port */
mca_resource_t *
mca_serial_param(void)
{
	unsigned char pval;
	pl_t opri;
	mca_resource_t *mcap = NULL;
	extern mca_resource_t mca_serial[];

	opri = LOCK(mcalock, plhi); /* Lock access to shared memory */

	outb(ADAP_ENAB,0);	/* The system setup and adapter setup should
				 * not be enabled at the same time.
				 */
	outb(SYS_ENAB, SYS_ENABLE_MASK);
	pval = inb(POS_2);
	outb(SYS_ENAB, 0xff);  	/* We have the input value so restore the
				 * SYS ENABLE port to a sane state.
				 */
	if (pval & 0x04) {	/* If built-in serial port is enabled */
		pval &= 0x08;
		pval >>= 3;
		mcap = &mca_serial[pval];
	}

	UNLOCK(mcalock, opri);
	return mcap;
}

#endif /* NOTYET */

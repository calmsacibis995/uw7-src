#ident	"@(#)kern-pdi:io/hba/mitsumi/mitsumi.c	1.5.1.18"
#ident	"$Header$"

/******************************************************************************
 ******************************************************************************
 *
 *	MITSUMI.C
 *
 *	MITSUMI CRMC
 *
 ******************************************************************************
 *****************************************************************************/

/******************************************************************************
 *
 *	INCLUDES
 *
 *****************************************************************************/


#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <mem/immu.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#if (PDI_VERSION >= 4)
#include <io/autoconf/resmgr/resmgr.h>
#endif /* (PDI_VERSION >= 4) */

#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#endif /* !(PDI_VERSION <= 1) */

#include <io/hba/mitsumi/mitsumi.h>
#include <io/hba/hba.h>
#include <util/mod/moddefs.h>
#include <io/dma.h>

#if PDI_VERSION >= PDI_SVR42MP
#include <util/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#else /* _KERNEL_HEADERS */
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/immu.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>

#include <sys/dynstructs.h>
#include <sys/mitsumi.h>
#if (PDI_VERSION >= 4)
#include <sys/resmgr.h>
#endif /* (PDI_VERSION >= 4) */

#include <sys/moddefs.h>
#include <sys/dma.h>
#include <sys/hba.h>

#if PDI_VERSION >= PDI_SVR42MP
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#endif /* _KERNEL_HEADERS */

#if PDI_VERSION < PDI_SVR42MP
#define KMEM_ZALLOC kmem_zalloc
#else /* PDI_VERSION >= PDI_SVR42MP */
/*
 * The following line is commented out and replaced
 * with a define to kmem_zalloc until DMA can be made
 * to function correctly with the Mitsumi controller.
 *
#define KMEM_ZALLOC mitsumi_kmem_zalloc_physreq
 */
#define KMEM_ZALLOC kmem_zalloc	/* Delete when DMA functions correctly */
#define MITSUMI_MEMALIGN	2
#define MITSUMI_BOUNDARY	0
#define MITSUMI_DMASIZE		24
#endif /* PDI_VERSION >= PDI_SVR42MP */

/******************************************************************************
 *
 *	DEFINES / MACROS
 *
 *****************************************************************************/

#define MITSUMI_TIMEOUT	TRUE

/******************************************************************************
 *
 *	GLOBALS
 *
 *****************************************************************************/

static int	sleepflag;			/* KM_SLEEP/KM_NOSLEEP */
int	mitsumi_dynamic = 0;		/* 1 if was a dynamic load */
static int	mitsumi_ctrl_size;	/* size of mitsumi_ctrl alloc */

scsi_ha_t	*mitsumi_sc_ha;			/* host adapter structs */
int		mitsumi_sc_ha_size;			/* size of mitsumi_sc_ha alloc */
ctrl_info_t	*mitsumi_ctrl;				/* local controller structs */

char		mitsumi_vendor[ 8 ] = "MITSUMI ";
char		mitsumi_diskprod[ 16 ];
struct ver_no	mitsumi_sdiver;		/* SDI version struct */
static int	mitsumi_drive_ver;

extern struct hba_info *mitsumihbainfo;

/******************************************************************************
 *
 *	EXTERNALS
 *
 *****************************************************************************/

extern	int	mitsumi_max_idle_time; 
extern	ushort_t mitsumi_max_tries; 
extern	int	mitsumi_chktime;
extern	int	mitsumi_cmd_timeout;
extern	int 	mitsumi_rdy_timeout;

extern 	struct head	sm_poolhead;		/* 28 byte structs */

extern int		mitsumi_cntls;		/* number of controllers */

#if PDI_VERSION >= PDI_UNIXWARE20
/*
 * The name of this pointer is the same as that of the
 * original idata array. Once it it is assigned the
 * address of the new array, it can be referenced as
 * before and the code need not change.
 */
extern HBA_IDATA_STRUCT	_mitsumiidata[];	/* hardware info */
HBA_IDATA_STRUCT	*mitsumiidata;
int			mitsumiverify();
static	int		mitsumidevflag;
#else  /* PDI_VERSION < PDI_UNIXWARE20 */
extern HBA_IDATA_STRUCT	mitsumiidata[];		/* hardware info */
#endif /* PDI_VERSION < PDI_UNIXWARE20 */

extern int		mitsumi_lowat;		/* low water mark */
extern int		mitsumi_hiwat;		/* high water mark */

extern int	mitsumi_gtol[];		/* global to local */
extern int	mitsumi_ltog[];		/* local to global */
extern void     mitsumi_rdcap(struct sb * sb, int flag);


/******************************************************************************
 *
 *	PROTOTYPES
 *
 *****************************************************************************/

/*
 *	mitsumi support
 */

int 			mitsumi_configure(scsi_ha_t *ha);
int 			mitsumi_reconfigure(scsi_ha_t *ha, struct sb *sb);
void 			mitsumi_error();
int			mitsumi_getanswer();
int			mitsumi_sendcmd();
int 			mitsumibdinit( int c, HBA_IDATA_STRUCT *idata );
int 			mitsumi_issue_rdcmd();
#ifndef	BIN2BCD
unchar 			BIN2BCD(unchar b);
unchar			BCD2BIN(unchar b);
#endif

#ifdef MITSUMI_TIMEOUT
void			mitsumi_timer(scsi_ha_t  *ha);
#endif
/*
 *	cdrom block translation support
 */

int 		msf2sector();
void 		sector2msf();

/*****************************************************************************
 *****************************************************************************
 *
 *	PDI/KERNEL INTERFACE ROUTINES
 *
 *****************************************************************************
 ****************************************************************************/

#if PDI_VERSION >= PDI_SVR42MP

/****************************************************************************** 
 *
 *	void * mitsumi_kmem_zalloc_physreq( size_t size, int sleepflag)
 *
 *      Function to be used in place of kmem_zalloc which allocates 
 *      contiguous memory, using kmem_alloc_physreq, and zero's 
 *      the memory.
 *
 *	Entry :
 *		size		size of the request
 *		sleepflags	sleep flag
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *		- all that happens is that this sends a "SCSI_TRESET" to
 *		  the associated target
 * 
 ******************************************************************************/

void    *
mitsumi_kmem_zalloc_physreq(size_t size, int sleepflag)
{
	void           *mem;

	physreq_t      *preq;

	preq = physreq_alloc(sleepflag);
	if (preq == NULL)
		return NULL;
	preq->phys_align = MITSUMI_MEMALIGN;
	preq->phys_boundary = MITSUMI_BOUNDARY;
	preq->phys_dmasize = MITSUMI_DMASIZE;
	preq->phys_flags |= PREQ_PHYSCONTIG;
	if (!physreq_prep(preq, sleepflag)) {
		physreq_free(preq);
		return NULL;
	}
	mem = kmem_alloc_physreq(size, preq, sleepflag);
	if (mem)
		bzero(mem, size);

	return mem;
}
#endif /* PDI_VERSION >= PDI_SVR42MP */

/*****************************************************************************
 *
 *	mitsumiinit( void )
 *
 *	Hardware init 
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		0		cool
 *		-1		no hardware found
 *
 ****************************************************************************/

int 
mitsumiinit( void )
{
	int 		i;
	scsi_ha_t		*ha;		/* random ha ptr */


#if PDI_VERSION >= PDI_UNIXWARE20
 	/*
 	 * Allocate and populate a new idata array based on the
 	 * current hardware configuration for this driver.
 	 */

 	mitsumiidata = sdi_hba_autoconf("mitsumi", _mitsumiidata, &mitsumi_cntls);
 	if(mitsumiidata == NULL)    {
 		return (-1);
	}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	for ( i = 0; i < MAX_HAS; i++ )		/* init hbano translation */
		mitsumi_gtol[ i ] = mitsumi_ltog[ i ] = -1;

	mitsumi_sdiver.sv_release = 1;
	mitsumi_sdiver.sv_machine = SDI_386_AT;
	mitsumi_sdiver.sv_modes = SDI_BASIC1;

	sleepflag = mitsumi_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/*
	 *	allocate space for mitsumi_sc_ha stuff -- must be contiguous
	 *      if it is going to be used with dma.
	 */

	mitsumi_sc_ha_size = mitsumi_cntls *( sizeof( struct scsi_ha ) );
	for ( i = 2; i < mitsumi_sc_ha_size; i *=2 ) ;	/* align */
	mitsumi_sc_ha_size = i;
	if ((mitsumi_sc_ha = (struct scsi_ha *) KMEM_ZALLOC( mitsumi_sc_ha_size, sleepflag ) ) == NULL ) {
		cmn_err( CE_WARN, "mitsumi: Unable to allocate MITSUMI_SC_HA( %x )\n", mitsumi_sc_ha_size );
		return( -1 );
	}

	/*
	 *	allocate space for mitsumi_ctrl stuff -- must be contiguous
	 */

	mitsumi_ctrl_size = mitsumi_cntls *( sizeof( struct ctrl_info ) );
	for ( i = 2; i < mitsumi_ctrl_size; i *=2 ) ;	/* align */
	mitsumi_ctrl_size = i;
	if (( mitsumi_ctrl = (ctrl_info_t *) KMEM_ZALLOC( mitsumi_ctrl_size, sleepflag ) ) == NULL ) {
		cmn_err( CE_WARN, "mitsumi: Unable to allocate CTRL( %x )\n", mitsumi_ctrl_size );
		kmem_free( mitsumi_sc_ha, mitsumi_sc_ha_size );
		return( -1 );
	}


	/*
	 *	loop through adapter list and check for them
	 */

	for ( i = 0; i < mitsumi_cntls; i++ ) {

		/*
		 *	do a little pre-initialization of the ha table
		 */

		ha = &mitsumi_sc_ha[ i ];
		ha->ha_state = HA_UNINIT;
		ha->ha_iobase = mitsumiidata[ i ].ioaddr1;
		ha->ha_id = mitsumiidata[ i ].ha_id;
		ha->ha_vect = mitsumiidata[ i ].iov;

		if ( mitsumibdinit( i, &mitsumiidata[ i ] ) == -1 ) {
			/*
		         *	didn't find the board
		         */
			ha->ha_vect = 0;		/* no interrupt */
			continue;
		}

		ha->ha_tid = 0;
		ha->ha_state = HA_INIT;

	}

	return(0);
}

/******************************************************************************
 *
 *	int mitsumistart( void )
 *
 *	Turn on interrupts and such for the mitsumi hardware
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		0 Success
 *		Otherwise, failure.
 *
 *****************************************************************************/

int 
mitsumistart( void )
{
	int			i;
	int			found = 0;	/* adapters found */
	int			cntlr;		/* controller number */
	int			queue_size;	/* size of queue alloc */
	scsi_ha_t		*ha;		/* random ha ptr */

	/*
	     *	figure out size of LU queues -- must be contiguous
	     */

	queue_size = MAX_EQ *( sizeof( struct scsi_lu ) );
	for ( i = 2; i < queue_size; i *=2 );
	queue_size = i;

	HBA_IDATA(mitsumi_cntls);

	for ( i = 0; i < mitsumi_cntls; i++ ) {
		ha = &mitsumi_sc_ha[ i ];

		if (ha->ha_state == HA_UNINIT)
			continue;

		/*
		 *	board found -- allocate queues and things
		 */
		if (( ha->ha_queue = (scsi_lu_t *) KMEM_ZALLOC( queue_size, sleepflag ) ) == NULL ) {
			cmn_err( CE_WARN, "mitsumi: Unable to allocate queues( %x )\n", queue_size );
			continue;
		}

		/*
		 *	register with sdi and do more setup
		 */

		if (( cntlr = sdi_gethbano( mitsumiidata[i].cntlr ) ) < 0 ) {
			cmn_err( CE_WARN, "mitsumi:(init) HA %d unable to allocate HBA with SDI\n",i);
			kmem_free( ha->ha_queue, queue_size );
			continue;
		}

		mitsumiidata[ i ].cntlr = cntlr;
		mitsumi_gtol[ cntlr ] = i;
		mitsumi_ltog[ i ] = cntlr;

		if ((cntlr = sdi_register( mitsumihbainfo, &mitsumiidata[i] ) ) < 0 ) {
			cmn_err( CE_WARN, "!mitsumi:(init) HA %d unable to register with SDI\n",i);
			kmem_free( ha->ha_queue, queue_size );
			continue;
		}
		mitsumiidata[ i ].active = 1;
		found ++;
		cmn_err(CE_NOTE, "!MITSUMI %s CDROM version %d was found", 
			mitsumi_diskprod, mitsumi_drive_ver);
	}

	if ( !found ) {
#ifdef MITSUMI_DEBUG
		cmn_err( CE_CONT, "No MITSUMI adapter(s) found\n");
#endif
		if(mitsumi_sc_ha)
			kmem_free( mitsumi_sc_ha, mitsumi_sc_ha_size );
		if(mitsumi_ctrl)
			kmem_free( mitsumi_ctrl, mitsumi_ctrl_size);
		return( -1 );
	} 

#if PDI_VERSION >= PDI_UNIXWARE20
	/*
	 * Attach interrupts for each "active" device. The driver
	 * must set the active filed of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here, in init(), as opposed
	 * to load(), because this is required for static autoconfig
	 * drivers as well as loadable.
	 */
	sdi_intr_attach(mitsumiidata, mitsumi_cntls, mitsumiintr, mitsumidevflag);
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	return( 0 );
}

/*****************************************************************************
 *****************************************************************************
 *
 *	MITSUMI HARDWARE SUPPORT
 *
 *		mitsumiintr		interrupt routine
 *		mitsumibdinit		initialize board
 *
 *****************************************************************************
 ****************************************************************************/
/******************************************************************************
 *
 *	mitsumiintr( unsigned int vec )
 *
 *	Interrupt handler for the board
 *
 *	Entry :
 *		vec		interrupt vector
 *
 *	Exit :
 *		Nothing
 *
 *
 *****************************************************************************/
void
mitsumiintr( unsigned int vec )
{
	int			c;		/* controller */
	struct scsi_ha		*ha;
	void 	mitsumi_chkread();

#ifdef MITSUMI_DEBUG
	cmn_err( CE_CONT, "mitsumi:(intr) vec = %x\n", vec );
#endif
	/*
	 *	figure out where the interrupt came from
	 */
	for ( c = 0; c < mitsumi_cntls; c++ ) {
		ha = &mitsumi_sc_ha[ c ];
		if ( ha->ha_vect == vec ) {
			break;
		}
	}
	if ( c >= mitsumi_cntls) {
#ifdef MITSUMI_DEBUG
		cmn_err( CE_CONT, "mitsumi:(intr) vec %x is out of range", vec );
#endif
		return;
	}
	if(ha->ha_cmd_sp == NULL) {
		if((inb(ha->ha_iobase+ MITSUMI_STATUS_REG)&MITSUMI_STATUS_BIT) == 0)
			ha->ha_status = inb(ha->ha_iobase)&0xFF;
		else    ha->ha_status = mitsumi_sendcmd(ha->ha_iobase, MITSUMI_GET_DRIVE_STATUS, 50, NULL, 0, 1);
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "mitsumiintr: ha_status is %x",
			ha->ha_status);
#endif
		return;
	}
	mitsumi_chkread(ha->ha_cmd_sp);
}

/******************************************************************************
 *
 *	mitsumibdinit( int c, HBA_IDATA_STRUCT *idata )
 *
 *	Initialize the MITSUMI controller 
 *
 *	Entry :
 *		c		controller number( local )
 *		*idata		ptr to hardware info
 *
 *	Exit :
 *		-1		no controller
 *		0		no disks found
 *		x		number of disks found
 *
 *	Notes :
 *
 *****************************************************************************/

int
mitsumibdinit( int c, HBA_IDATA_STRUCT *idata )
{
	register ushort_t	iobase = idata->ioaddr1;
	int 		i;
	drv_info_t		*drv;
	int status;
	uint_t ndrives = 1;
	int mitsumi_sendcmd();
	unchar	buf[3];

	/* send a reset */
	mitsumi_ctrl[ c ].flags = 0;
	status = mitsumi_sendcmd(iobase, MITSUMI_RESET_DRIVE_CMD, 250, NULL, 0, 1);
	if(status < 0) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI: Cannot find MITSUMI CDROM drives");
#endif
		return (-1);
	}
	status = mitsumi_sendcmd(iobase, MITSUMI_GET_VERSION_CMD, 250, NULL, 0, 1);

	if(status < 0) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI: Cannot read MITSUMI version");
#endif
		return (-1);
	}
	if (mitsumi_getanswer(iobase, buf, 2) != 2) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI: Cannot read MITSUMI version");
#endif
		return (-1);
	}

	/*
	 * This driver supports the following drives
	 *	MITSUMI MODEL	 ASCII CODE  
	 *	CRMC-LU005S		M
	 *	CRMC-FX001D		D
	 *	CRMC-FX001		F
	 */
	switch(buf[0]) {
		case 'M':
			bcopy((caddr_t)"CRMC-LU005S", (caddr_t)mitsumi_diskprod, 11);
			mitsumi_diskprod[12] = '\0';
			break;
		case 'D':
			bcopy((caddr_t)"CRMC-FX001D", (caddr_t)mitsumi_diskprod, 11);
			mitsumi_diskprod[12] = '\0';
			break;
		case 'F':
			bcopy((caddr_t)"CRMC-FX001", (caddr_t)mitsumi_diskprod, 10);
			mitsumi_diskprod[11] = '\0';
			break;
		default:
#ifdef MITSUMI_DEBUG
			cmn_err(CE_NOTE, "MITSUMI: Cannot read MITSUMI version");
#endif
			return (-1);
	}
	mitsumi_drive_ver = BCD2BIN(buf[1]);

	/*
	 * The driver does not work correctly with a Version 17 drive.
	 * If this drive version is detected, fail the load and let
	 * user know why.
	 */
	if(buf[0] == 'M' && mitsumi_drive_ver == 17) {
		cmn_err(CE_WARN, "A Version 17 Mitsumi CRMC-LU005S drive has been detected.  Due to\nproblems exhibited by this version of the drive, this instance is not being\nconfigured into the system.  Please refer to the technical bulletin for\nmore information.\n"); 
		return (-1);
	}

	mitsumi_ctrl[ c ].num_drives = ndrives;
	mitsumi_ctrl[ c ].iobase = iobase;

	/*
	     *	run through and allocate drives/dig up info on them
	     */

	for ( i = 0; i < ndrives; i++ ) {
		if (( drv = ( drv_info_t * ) kmem_zalloc( sizeof( struct drv_info ), sleepflag ) ) == NULL ) {
			cmn_err( CE_WARN, "mitsumi:(_bdinit) Unable to allocate drv structure\n");
			return( -1 );
		}

		mitsumi_ctrl[ c ].drive[ i ] = drv;
		cmn_err( CE_CONT, "!mitsumi:(_bdinit) Configured HA %d TC %d\n",c,i );
	}

	return( ndrives);
}

static int 
mitsumi_waitrdy(int iobase, unchar busybit)
{
	int i;

	/* wait until xfer port senses data ready */
	for (i=0; i< mitsumi_rdy_timeout; i++) {
		if ((inb(iobase+ MITSUMI_STATUS_REG) & busybit)==0)
			return 0;
		drv_usecwait(10);
	}
	return -1;
}

static int 
mitsumi_getreply(int iobase)
{
	/* wait data to become ready */
	if (mitsumi_waitrdy(iobase, MITSUMI_STATUS_BIT)<0) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "mitsumi_getreply: timeout getreply\n");
#endif
		return -1;
	}

	/* get the data */
	return inb(iobase+MITSUMI_DATA_REG) & 0xFF;
}


int 
mitsumi_sendcmd(int iobase, int	cmd, int spintime, 
unchar cmdbuf[], int bufsize, int wait_for_answer)
{
	int	status, tries, i;

	for (tries=0; tries < MITSUMI_MAX_TRIES; tries++) {
		outb(iobase+MITSUMI_COMMAND_REG, cmd);
		for (i=0; i < bufsize; i++)
			outb(iobase+MITSUMI_COMMAND_REG, cmdbuf[i]);
		if (!wait_for_answer)
			return 0;
		if (spintime)
			drv_usecwait(spintime);
		if ((status=mitsumi_getreply(iobase)) != -1)
			break;
	}
	return status;
}

int 
mitsumi_getanswer(int iobase, unchar buf[], int nbytes)
{
	int i, data;

	for (i=0; i<nbytes; i++) {
		/* wait for data */
		if ((data = mitsumi_getreply(iobase)) < 0) {
#ifdef MITSUMI_DEBUG
			cmn_err(CE_NOTE, "mitsumi_getanswer: got %d byte(s)\n",
			    i);
#endif
			return i;
		}
		buf[i] = (unchar) data;
	}
	return i;
}

void 
sector2msf(int sector, unchar *msf)
{
	msf[2] = BIN2BCD(sector % 75); sector /= 75;
	msf[1] = BIN2BCD(sector % 60);
	msf[0] = BIN2BCD(sector / 60);
}

int
msf2sector(int m, int s, int f)

{
	m = BCD2BIN(m);
	s = BCD2BIN(s);
	f = BCD2BIN(f);
	return (f + 75 * (s + 60 * m));
}
/******************************************************************************
 *
 *	mitsumi_rw (sblk_t *sp )
 *
 *	Convert a SCSI r/w request into an MITSUMI request
 *
 *	Entry :
 *		*sp		ptr to scsi job
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 *****************************************************************************/

void
mitsumi_rw(sblk_t * sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	uint_t          sector;
	ushort_t        count;
	int             c, t, l;
	scsi_lu_t	*q;		/* queue ptr */
	int		blk_num;
	int		direction;
	scsi_ha_t      *ha;
	drv_info_t     *drv;
	pl_t		ospl;

	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	ha = &mitsumi_sc_ha[c];

	drv = mitsumi_ctrl[c].drive[t];

	/*
	 * check for various command things
	 */

	if (sb->SCB.sc_cmdsz == SCS_SZ) {
		/*
		 * 6 byte command
		 */
		struct scs     *scs;

		scs = (struct scs *)(void *) sb->SCB.sc_cmdpt;
		direction = scs->ss_op == SS_READ ? B_READ : B_WRITE;
		sector = sdi_swap16(scs->ss_addr) + ((scs->ss_addr1 << 16) & 0x1f0000);
		count = scs->ss_len;
	} else if (sb->SCB.sc_cmdsz == SCM_SZ) {
		/*
		 * 10 byte extended command
		 */
		struct scm     *scm;

		scm = (struct scm *)(void *) (SCM_RAD(sb->SCB.sc_cmdpt));
		direction = scm->sm_op == SM_READ ? B_READ : B_WRITE;
		sector = sdi_swap32(scm->sm_addr);
		count = sdi_swap16(scm->sm_len);
	} else {
		/*
		 * random command here
		 */
#ifdef MITSUMI_DEBUG
		cmn_err(CE_WARN, "MITSUMI: (mitsumi_rw) Unknown SC_CMDSZ -- %x\n",
			sb->SCB.sc_cmdsz);
#endif
		mitsumi_error(ha, sp);
		return;
	}
	if (direction == B_WRITE) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_WARN, "MITSUMI: (mitsumi_rw) write cmd received");
#endif
		mitsumi_error(ha, sp);
		return;
	}

	blk_num = sector + drv->startsect;
	sp->s_tries = 0;
	sp->s_size  = count;
	sp->s_blkno = blk_num;
#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "mitsumi_rw: sector %d, offset %d, cnt %d, addr %x",
				sector, drv->startsect, count, 
				(caddr_t) (void *)  sp->sbp->sb.SCB.sc_datapt);
#endif
	if((ha->ha_status&MITSUMI_ERROR) ||
	   mitsumi_issue_rdcmd(ha, sp) < 0) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "mitsumi_rw: ha_status is %x",
				ha->ha_status);
#endif
		mitsumi_error(ha, sp);
		return;
	}
	ha->ha_cmd_sp = sp;
	ospl = spldisk();
	/*
	 *	job is submitted -- update queues
	 */

	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	q = &LU_Q( c, t, l );
	q->q_count++;
	if ( q->q_count >= mitsumi_hiwat )
		q->q_flag |= QBUSY;
	splx( ospl );
}

int
mitsumi_chkmedia(c)
int	c;
{
	scsi_ha_t *ha = &mitsumi_sc_ha[c];
	int 	iobase = ha->ha_iobase;
	int	status;
#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "mitsumi_chkmedia: iobase %x", iobase);
#endif
	ha->ha_status = 0;
	status = mitsumi_sendcmd(iobase, MITSUMI_GET_DRIVE_STATUS, 250, NULL, 0, 1);
	if(status < 0 || (status&MITSUMI_DISK_SET) == 0)
		return(-1);
	return(mitsumi_configure(ha));
}


void
mitsumi_chkread(sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	scsi_lu_t	*q;		/* queue ptr */
	int		iobase, status, hstatus;
	scsi_ha_t      *ha;

	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	q = &LU_Q( c, t, l );
	ha = &mitsumi_sc_ha[c];
	iobase = ha->ha_iobase;

#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "mitsumi_chkread: iobase %x, sb %x", iobase, sb);
#endif
	status = 0;
	hstatus = inb(iobase+ MITSUMI_STATUS_REG);
	if((hstatus&MITSUMI_STATUS_BIT) == 0) {
		status = inb(iobase+MITSUMI_DATA_REG) & 0xFF;
	}
	if((status&MITSUMI_ERROR) || 
           ((hstatus&MITSUMI_DATA_BIT) && mitsumi_waitrdy(iobase, MITSUMI_DATA_BIT) < 0)) 
	{
		drv_info_t *drv;
		int	err = 0;
		drv = mitsumi_ctrl[c].drive[t];
		if(sp->s_tries < mitsumi_max_tries){
			if(sp->s_tries > 1)
				err = mitsumi_reconfigure(ha, sb);
			if(err < 0 || mitsumi_issue_rdcmd(ha, sp) < 0) {
				cmn_err(CE_WARN, "mitsumi_chkread: cannot read block %x, max %d status %x",
				sp->s_blkno, drv->drvsize, status);
			} else {
				sp->s_tries++;
				return;
			}
		}
/*
#ifdef MITSUMI_DEBUG
*/
		cmn_err(CE_NOTE, "mitsumi_chkread: cannot read block %x, max %d status %x",
				sp->s_blkno, drv->drvsize, status);
/*
#endif
*/
		mitsumi_error(ha, sp);
	} else {
		uchar_t *addr = (uchar_t *) (void *)  sp->sbp->sb.SCB.sc_datapt;
		int cnt = sp->s_size*MITSUMI_BLKSIZE;
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI_SEEK_N_READ_CMD return SUCCESS %d", status);
#endif
		/* allow the data port to receive data */
		outb(iobase+MITSUMI_HCON_REG,
			MITSUMI_USE_STATUS_REG_FOR_DATA);

		/* read data from the data port */

		repinsb(iobase+MITSUMI_DATA_REG, addr, cnt);

		/* allow the data port to receive status data */
		outb(iobase+MITSUMI_HCON_REG,
			MITSUMI_USE_STATUS_REG_FOR_STATUS);

		ha->ha_cmd_sp = NULL;
		sb->SCB.sc_comp_code = SDI_ASW;
		sdi_callback(sb);
	}
	q->q_flag &= ~QBUSY;
	q->q_count--;
	mitsumi_next(q);
}


int
mitsumi_issue_rdcmd(scsi_ha_t *ha, sblk_t *sp)
{
	int	blkno = sp->s_blkno;
	int	blkcnt = sp->s_size;
	unchar cmdbuf[6];

	sector2msf(blkcnt, &cmdbuf[3]);
	sector2msf(blkno, cmdbuf);

#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "mitsumi_issue_rdcmd: BLK no %d, cnt %d ", 
			blkno, blkcnt);
#endif
	(void) mitsumi_sendcmd(ha->ha_iobase, MITSUMI_SEEK_N_READ_CMD, 0, cmdbuf, 6, 0);
#ifdef MITSUMI_TIMEOUT
	drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
	if(!ha->ha_tid) { 
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "mitsumi_timer is set");
#endif
		ha->ha_tid = ITIMEOUT(mitsumi_timer, (void *) ha, mitsumi_chktime, (pl_t) pldisk);
	}
#endif
	return(0);
}

void
mitsumi_error(scsi_ha_t *ha, sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	ha->ha_cmd_sp = NULL;
	sb->SCB.sc_comp_code = (ulong_t) SDI_HAERR;
	
#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "sdi_callback(sb=%x)", sb);
#endif
	sdi_callback(sb);
}

#ifdef MITSUMI_TIMEOUT
void
mitsumi_timer(scsi_ha_t  *ha)
{
	clock_t now;
	ulong_t elapsed;
	sblk_t *sp;

	ha->ha_tid = 0;
	drv_getparm(LBOLT, (void *) &now);
	elapsed = now - ha->ha_cmd_time;

#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "mitsumi_timer entered");
#endif
	if((sp = ha->ha_cmd_sp) != NULL) {
		struct sb      *sb = (struct sb *) & sp->sbp->sb;
		struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
		int             c, t, l;
		scsi_lu_t	*q;		/* queue ptr */

		c = mitsumi_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q( c, t, l );

		if(elapsed > mitsumi_cmd_timeout) {
			int err = 0;
			if(sp->s_tries < mitsumi_max_tries){
				if(sp->s_tries > 1) {
#ifdef MITSUMI_DEBUG
					cmn_err(CE_NOTE, "mitsumi_timer:reconfiguring device");
#endif
					err = mitsumi_reconfigure(ha, sb);
				}
				sp->s_tries++;
				if( err >= 0 && mitsumi_issue_rdcmd(ha, sp) >= 0) {
#ifdef MITSUMI_DEBUG
					cmn_err(CE_NOTE, "mitsumi_timer:command timeout, trying again");
#endif
					return;
				}
#ifdef MITSUMI_DEBUG
				else cmn_err(CE_NOTE, "mitsumi_timer:error while retrying");
#endif
			}
			cmn_err(CE_WARN, "mitsumi_timer abort command");
			mitsumi_error(ha, sp);
			ha->ha_status = MITSUMI_ERROR;
			mitsumi_flushq( q, SDI_HAERR);
			return;
		}
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "mitsumi_timer did not abort command");
#endif
	} else if(elapsed > mitsumi_max_idle_time) {
		/* 
		 * there is not that much activity. Stop the the timer.
		 * It will be enabled again the next command.
		 */
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "mitsumi_timer is canceled");
#endif
		return;
	}
#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "mitsumi_timer is set again");
#endif
	ha->ha_tid = ITIMEOUT(mitsumi_timer, (void *) ha, mitsumi_chktime, (pl_t) pldisk);
}
#endif

int
mitsumi_configure(scsi_ha_t *ha)
{
	int	status;
	unchar  buf[3];
	int	iobase = ha->ha_iobase;

	if ( ha->ha_state == HA_UNINIT ) {
		/*
		*	ha is dead
		*/
		return(-1);
	}

	/*
	*	got a valid HA -- initialize things
	*/

	buf[0] = MITSUMI_SELECT_INTERRUPT_TYPE; /* select interrupt type */
	buf[1] = 0;    /* no interrupts  */

	status = mitsumi_sendcmd(iobase, MITSUMI_CONFIGURE_DRIVE, 250, buf, 2, 1);
#ifdef MITSUMI_DEBUG
	if(status < 0 || (status&MITSUMI_ERROR)) {
		cmn_err(CE_WARN, "MITSUMI: Cannot disable interrupts, status %x",
			status);
	}
#endif

	buf[0] = MITSUMI_SELECT_IO_TYPE;
	buf[1] = MITSUMI_SELECT_PIO;

	status = mitsumi_sendcmd(iobase, MITSUMI_CONFIGURE_DRIVE, 250, buf, 2, 1);
	if(status < 0 || (status&MITSUMI_ERROR)) {
		cmn_err(CE_WARN, "MITSUMI: Cannot select non-DMA mode, status %x",
			status);
		return(-1);
	}
	buf[0] = MITSUMI_SELECT_INTERRUPT_TYPE; /* select interrupt type */
   	/* PRE IRQ enable + ERROR */
	buf[1] = (MITSUMI_INTERRUPT_ONERROR|MITSUMI_PRE_INTERRUPT);

	status = mitsumi_sendcmd(iobase, MITSUMI_CONFIGURE_DRIVE, 250, buf, 2, 1);
	if(status < 0 || (status&MITSUMI_ERROR)) {
		cmn_err(CE_WARN, "MITSUMI: Cannot enable interrupts, status %x",
			status);
		return(-1);
	}

	return(1);
}

int
mitsumi_reconfigure(scsi_ha_t *ha, struct sb *sb)
{
	int	status;
	int	n;

	status = mitsumi_sendcmd(ha->ha_iobase, MITSUMI_STOP_DISK_CMD, 250, NULL, 0, 1);
	if(status < 0) {
		cmn_err(CE_NOTE, "mitsumi_reconfigure: MITSUMI_RESET_DRIVE_CMD failed");
	}
	status = mitsumi_sendcmd(ha->ha_iobase, MITSUMI_RESET_DRIVE_CMD, 500, NULL, 0, 1);
	if(status < 0) {
		cmn_err(CE_NOTE, "mitsumi_reconfigure: MITSUMI_RESET_DRIVE_CMD failed");
	}
	n = 0;
	do {
		status = mitsumi_configure(ha);
		n++;
	} while (n < 5 && status != 1);
	if(status != 1)
		return (status);
	mitsumi_rdcap(sb, 0);
	return (1);
}

#ifndef BCD2BIN
unchar
BCD2BIN(unchar b)
{
	return ((b >> 4) * 10 + (b&0xf));
}

unchar 
BIN2BCD(unchar b)
{
	return (((b / 10) << 4) | (b % 10));
}
#endif

#if PDI_VERSION >= PDI_UNIXWARE20
/*
 * int
 * mitsumi_verify(rm_key_t key)
 *
 * Calling/Exit State:
 *	None
 *
 * Description:
 * 	Verify the board instance.
 */
int
mitsumiverify(rm_key_t key)
{
	HBA_IDATA_STRUCT	vfy_idata;
	register ushort_t	iobase;
	int status;
	int mitsumi_sendcmd();
	unchar	buf[3];
	int	rv;
        /*
         * Read the hardware parameters associated with the
         * given Resource Manager key, and assign them to
         * the idata structure.
         */
        rv = sdi_hba_getconf(key, &vfy_idata);
        if(rv != 0)     {
                return(rv);
        }

        /*
         * Verify hardware parameters in vfy_idata,
         * return 0 on success, ENODEV otherwise.
         */

	iobase = vfy_idata.ioaddr1;
	/* send a reset */
	status = mitsumi_sendcmd(iobase, MITSUMI_RESET_DRIVE_CMD, 250, NULL, 0, 1);
	if(status < 0) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI: Cannot find MITSUMI CDROM drives");
#endif
		return (ENODEV);
	}
	status = mitsumi_sendcmd(iobase, MITSUMI_GET_VERSION_CMD, 250, NULL, 0, 1);

	if(status < 0) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI: Cannot read MITSUMI version");
#endif
		return (ENODEV);
	}
	if (mitsumi_getanswer(iobase, buf, 2) != 2) {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI: Cannot read MITSUMI version");
#endif
		return (ENODEV);
	}
	if (buf[0] != 'M') {
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "MITSUMI: Cannot read MITSUMI version");
#endif
		return (ENODEV);
	}

	return (0);
}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

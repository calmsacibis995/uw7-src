#ident	"@(#)kern-pdi:io/hba/lmsi/lmsi.c	1.13.1.1"
#ident	"$Header$"

/******************************************************************************
 ******************************************************************************
 *
 *	LMSI.C
 *
 *	LMSI CM205/250 Version 0.00
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

#include <io/hba/lmsi/lmsi.h>
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

#include <sys/moddefs.h>
#include <sys/dma.h>
#include <sys/hba.h>
#include <sys/lmsi.h>
#if (PDI_VERSION >= 4)
#include <sys/resmgr.h>
#endif

#if PDI_VERSION >= PDI_SVR42MP
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#endif /* _KERNEL_HEADERS */
/******************************************************************************
 *
 *	DEFINES / MACROS
 *
 *****************************************************************************/

#define	LMSI_XMIT_BYTE (int (*)()) lmsi_xmit_byte


/******************************************************************************
 *
 *	GLOBALS
 *
 *****************************************************************************/

static int	sleepflag;			/* KM_SLEEP/KM_NOSLEEP */
int	lmsi_dynamic = 0;		/* 1 if was a dynamic load */
static int	lmsi_ctrl_size;	/* size of lmsi_ctrl alloc */

scsi_ha_t	*lmsi_sc_ha;			/* host adapter structs */
int		lmsi_sc_ha_size;			/* size of lmsi_sc_ha alloc */
ctrl_info_t	*lmsi_ctrl;				/* local controller structs */

char		lmsi_vendor[ 8 ] = "PHILIPS ";
char		lmsi_diskprod[ 16 ] = "LMSI CM205/250 ";
struct ver_no	lmsi_sdiver;		/* SDI version struct */


/******************************************************************************
 *
 *	EXTERNALS
 *
 *****************************************************************************/

extern	int	lmsi_max_idle_time; 
extern	ushort_t lmsi_max_tries; 
extern	int	lmsi_chktime;
extern	int	lmsi_cmd_timeout;
extern	int 	lmsi_rdy_timeout;

extern 	struct head	sm_poolhead;		/* 28 byte structs */

extern int		lmsi_cntls;		/* number of controllers */

#if PDI_VERSION >= PDI_UNIXWARE20

/*
 * The name of this pointer is the same as that of the
 * original idata array.  Once it is assigned the address
 * of the new array it can be referenced as before and
 * the code need not change.
 */
extern HBA_IDATA_STRUCT	_lmsiidata[];		/* hardware info */
HBA_IDATA_STRUCT	*lmsiidata;
int			lmsiverify();
extern int		lmsidevflag;
#else  /* PDI_VERSION < PDI_UNIXWARE20 */
extern HBA_IDATA_STRUCT	lmsiidata[];		/* hardware info */
#endif /* PDI_VERSION < PDI_UNIXWARE20 */

extern int		lmsi_lowat;		/* low water mark */
extern int		lmsi_hiwat;		/* high water mark */

extern int	lmsi_gtol[];		/* global to local */
extern int	lmsi_ltog[];		/* local to global */
extern int	sdi_started;


/******************************************************************************
 *
 *	PROTOTYPES
 *
 *****************************************************************************/

/*
 *	lmsi support
 */

int			lmsi_examine_error(scsi_ha_t *ha);
int			lmsi_xmit_byte(scsi_ha_t *ha, unchar byte);
int			lmsi_read_echo(scsi_ha_t *ha);
int 			lmsi_read_req(scsi_ha_t *ha);
#ifdef TOC_STREAM
int			read_toc_data(scsi_ha_t * ha);
#endif
int 			lmsi_configure(scsi_ha_t *ha);
int			lmsi_chkread(scsi_ha_t *ha);

void 			lmsi_chkerror(scsi_ha_t *ha);
void 			lmsi_execute_action(scsi_ha_t *ha);
void 			lmsi_sendcmd(scsi_ha_t *ha);
void 			lmsi_callback();

unchar 			lmsi_waitrdy(uint_t iobase);
int 			lmsibdinit( int c, HBA_IDATA_STRUCT *idata );
int			lmsi_issue_rdcmd(sblk_t *sp, scsi_ha_t *ha);

void			lmsi_timer(scsi_ha_t  *ha);

/*
 *	cdrom block translation support
 */

int 		msf2sector();
void 		sector2msf();
/*******************************************************************************
 *
 *	GLOBAL VARIABLES
 *
 ******************************************************************************/
/*******************************************************************************
 *	Commands structures
 ******************************************************************************/

/* read status */
static  Action lmsi_read_status_actions[] = {
  { 0x01, 0x3a, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* read status cmd */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 1  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 2  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 3  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 4  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 5  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 6  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 7  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 8  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 9  */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 10 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 11 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 12 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 13 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 14 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 15 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 16 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 17 */
  { 0x01, 0x9c, LMSI_XMIT_BYTE, NULL, lmsi_read_req },  /* request byte 18 */
};
static  SubCmd lmsi_read_status_cmd = {
  "READ STATUS COMMAND",
  lmsi_read_status_actions,
  19
};

/* clear error */
static  Action lmsi_clear_error_actions[] = {
  { 0x01, 0x4e, LMSI_XMIT_BYTE, lmsi_read_echo, NULL},  /* clear error cmd */
};
static  SubCmd lmsi_clear_error_cmd = {
  "CLEAR ERROR COMMAND",
  lmsi_clear_error_actions,
  1
};

/* spin up the disk */
static  Action lmsi_spin_up_actions[] = {
  { 0x01, 0x63, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* spin up cmd */
};
static  SubCmd lmsi_spin_up_cmd = {
  "SPIN UP COMMAND",
  lmsi_spin_up_actions,
  1
}; 

#ifdef LMSI_SPIN_DOWN
static  Action lmsi_spin_down_actions[] = {
  { 0x01, 0x74, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* spin down cmd */
};
static  SubCmd lmsi_spin_down_cmd = {
  "SPIN DOWN COMMAND",
  lmsi_spin_down_actions,
  1
};
#endif

/* read blocks */
static  Action lmsi_read_blocks_actions[] = {
{ 0x01, 0xa6, LMSI_XMIT_BYTE, lmsi_read_echo, NULL }, /* read blocks cmd       */
{ 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* start frame byte      */
{ 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* start second byte     */
{ 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* start minute byte     */
{ 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* number of sectors LSB */
{ 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* number of sectors     */
{ 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* number of sectors MSB */
};
static  SubCmd lmsi_read_blocks_cmd = {
  "READ BLOCKS COMMAND",
  lmsi_read_blocks_actions,
  7
};

#ifdef LMSI_SEEK
/* seek */
static  Action lmsi_seek_actions[] = {
  { 0x01, 0x59, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* seek cmd      */
  { 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* frame byte      */
  { 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* second byte     */
  { 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* minute byte     */
};
static  SubCmd lmsi_seek_cmd = {
  "SEEK COMMAND",
  lmsi_seek_actions,
  4
};
#endif

#ifdef TOC_STREAM
/* stream table of contents */
static  Action lmsi_read_toc_actions[] = {
  { 0x01, 0xe5, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* read toc cmd      */
  { 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* start track byte      */
  { 0x01, 0x0, LMSI_XMIT_BYTE, lmsi_read_echo, NULL },  /* end track byte     */
};
static  SubCmd lmsi_read_toc_cmd = {
  "READ TOC COMMAND",
  lmsi_read_toc_actions,
  3
};
#endif

extern struct hba_info *lmsihbainfo;

/*****************************************************************************
 *
 *	PDI/KERNEL INTERFACE ROUTINES
 *
 *****************************************************************************
 ****************************************************************************/

/*****************************************************************************
 *
 *	lmsiinit( void )
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
lmsiinit( void )
{
	int 		i;
	scsi_ha_t	*ha;
	uint_t		iobase; 

#if PDI_VERSION >= PDI_UNIXWARE20
	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */

	lmsiidata = sdi_hba_autoconf("lmsi", _lmsiidata, &lmsi_cntls);
	if(lmsiidata == NULL) {
		return(-1);
	}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	for ( i = 0; i < MAX_HAS; i++ )		/* init hbano translation */
		lmsi_gtol[ i ] = lmsi_ltog[ i ] = -1;

	sdi_started = TRUE;
	lmsi_sdiver.sv_release = 1;
	lmsi_sdiver.sv_machine = SDI_386_AT;
	lmsi_sdiver.sv_modes = SDI_BASIC1;

	sleepflag = lmsi_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/*
	 *	allocate space for lmsi_sc_ha stuff -- must be contiguous
	 *      if it is going to be used with dma.
	 */

	lmsi_sc_ha_size = lmsi_cntls *( sizeof( struct scsi_ha ) );
	for ( i = 2; i < lmsi_sc_ha_size; i *=2 ) ;	/* align */
	lmsi_sc_ha_size = i;
	if ((lmsi_sc_ha = (struct scsi_ha *) kmem_zalloc( lmsi_sc_ha_size, sleepflag ) ) == NULL ) {
		cmn_err( CE_WARN, "lmsi: Unable to allocate LMSI_SC_HA( %x )\n", lmsi_sc_ha_size );
		return( -1 );
	}

	/*
	 *	allocate space for lmsi_ctrl stuff -- must be contiguous
	 */

	lmsi_ctrl_size = lmsi_cntls *( sizeof( struct ctrl_info ) );
	for ( i = 2; i < lmsi_ctrl_size; i *=2 ) ;	/* align */
	lmsi_ctrl_size = i;
	if (( lmsi_ctrl = (ctrl_info_t *) kmem_zalloc( lmsi_ctrl_size, sleepflag ) ) == NULL ) {
		cmn_err( CE_WARN, "lmsi: Unable to allocate CTRL( %x )\n", lmsi_ctrl_size );
		kmem_free( lmsi_sc_ha, lmsi_sc_ha_size );
		return( -1 );
	}


	/*
	 *	loop through adapter list and check for them
	 */

	for ( i = 0; i < lmsi_cntls; i++ ) {

		/*
		 *	do a little pre-initialization of the ha table
		 */

		ha = &lmsi_sc_ha[ i ];
		ha->ha_state = HA_UNINIT;
		iobase = ha->ha_iobase = lmsiidata[ i ].ioaddr1;
		ha->ha_id = lmsiidata[ i ].ha_id;
		ha->ha_vect = lmsiidata[ i ].iov;

		if ( lmsibdinit( i, &lmsiidata[ i ] ) == -1 ) {
			/*
		         *	didn't find the board
		         */
			ha->ha_vect = 0;		/* no interrupt */
			continue;
		}

		outb(DataControlRegister(iobase),(DRVRESET | INIT | MSKTXDR | 1));
		drv_usecwait(10000);    /* wait 10 ms */
		outb(DataControlRegister(iobase),(MSKTXDR | 1));
		drv_usecwait(10000);    /* wait 10 ms */

		/*
	 	 *	clear out receive
	 	 */

		inb(ReceiveRegister(iobase));
		inb(ReceiveRegister(iobase));

		ha->ha_tid = 0;
		ha->ha_state = HA_INIT;

	}
	return(0);
}

/******************************************************************************
 *
 *	int lmsistart( void )
 *
 *	Turn on interrupts and such for the lmsi hardware
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
lmsistart( void )
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

	HBA_IDATA(lmsi_cntls);

	for ( i = 0; i < lmsi_cntls; i++ ) {
		ha = &lmsi_sc_ha[ i ];

		if (ha->ha_state == HA_UNINIT)
			continue;

		/*
		 *	board found -- allocate queues and things
		 */
		if (( ha->ha_queue = (scsi_lu_t *) kmem_zalloc( queue_size, sleepflag ) ) == NULL ) {
			cmn_err( CE_WARN, "lmsi: Unable to allocate queues( %x )\n", queue_size );
			continue;
		}

		/*
		 *	register with sdi and do more setup
		 */

		if (( cntlr = sdi_gethbano( lmsiidata[i].cntlr ) ) < 0 ) {
			cmn_err( CE_WARN, "lmsi:(init) HA %d unable to allocate HBA with SDI\n",i);
			kmem_free( ha->ha_queue, queue_size );
			continue;
		}

		lmsiidata[ i ].cntlr = cntlr;
		lmsi_gtol[ cntlr ] = i;
		lmsi_ltog[ i ] = cntlr;

		if ((cntlr = sdi_register( lmsihbainfo, &lmsiidata[i] ) ) < 0 ) {
			cmn_err( CE_WARN, "!lmsi:(init) HA %d unable to register with SDI\n",i);
			kmem_free( ha->ha_queue, queue_size );
			continue;
		}
		lmsiidata[ i ].active = 1;
		found ++;
		cmn_err(CE_NOTE, "!LMSI CM205/250 CDROM");
	}

	if ( !found ) {
#ifdef LMSI_DEBUG
		cmn_err( CE_CONT, "No LMSI adapter(s) found\n");
#endif
		if(lmsi_sc_ha)
			kmem_free( lmsi_sc_ha, lmsi_sc_ha_size );
		if(lmsi_ctrl)
			kmem_free( lmsi_ctrl, lmsi_ctrl_size);
		return( -1 );
	} 

#if PDI_VERSION >= PDI_UNIXWARE20
	/*
	 * Attach interrupts for each "active" device.  The driver
	 * must set the active field of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here, in init(), as opposed
	 * to load(), because this is required for static autoconfig
	 * drivers as well as loadable.
	 */
	sdi_intr_attach(lmsiidata, lmsi_cntls, lmsiintr, lmsidevflag);
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	return( 0 );
}

/*****************************************************************************
 *****************************************************************************
 *
 *	LMSI HARDWARE SUPPORT
 *
 *		lmsiintr		interrupt routine
 *		lmsibdinit		initialize board
 *
 *****************************************************************************
 ****************************************************************************/
/******************************************************************************
 *
 *	lmsiintr( unsigned int vec )
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
lmsiintr( unsigned int vec )
{
	int			c;		/* controller */
	struct scsi_ha		*ha;

#ifdef LMSI_DEBUG
	cmn_err( CE_CONT, "lmsiintr vec = %x\n", vec );
#endif
	/*
	 *	figure out where the interrupt came from
	 */
	for ( c = 0; c < lmsi_cntls; c++ ) {
		ha = &lmsi_sc_ha[ c ];
		if ( ha->ha_vect == vec ) {
			break;
		}
	}
	if ( c >= lmsi_cntls) {
		return;
	}

	/* Check that drive is ready */
	if(ha->ha_status_data.drvstatus & DRVNOTRDY) {
		lmsi_callback(ha, NULL, ha->ha_cmd_sp, SDI_HAERR);
		ha->ha_status_data.drvstatus = 0;
		return;
	}

	if(ha->ha_cmd_sp)
		lmsi_execute_action(ha);
}

static Action lmsi_chk_read_actions[] = {
{ 0x00, 0, lmsi_chkread, NULL, NULL }, 
};
static SubCmd lmsi_chk_read_cmd = {
	"CHECK READ COMMAND",
	lmsi_chk_read_actions,
	1
};

int
lmsi_issue_rdcmd(sblk_t *sp, scsi_ha_t *ha)
{
	unchar msf[3], drv;
	unchar	subcmdno = 0;
	
	drv = inb(DriveStatRegister(ha->ha_iobase));
#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "lmsi_issue_rdcmd: drv %x ", drv);
#endif
	if (drv & ATTN) {
		lmsi_chkerror(ha);
		return(0);
	}
	sector2msf(sp->s_blkno, msf);
	
	lmsi_read_blocks_actions[1].arg = msf[FRAME]; 
	lmsi_read_blocks_actions[2].arg = msf[SECOND];  
	lmsi_read_blocks_actions[3].arg = msf[MINUTE];  

	lmsi_read_blocks_actions[4].arg = BIN2BCD(sp->s_size);
	lmsi_read_blocks_actions[5].arg = 0;
	lmsi_read_blocks_actions[6].arg = 0;
	lmsi_read_blocks_actions[6].iflags = 3;

/* subcmd 0 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_read_blocks_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_read_blocks_actions);
	subcmdno++;

/* subcmd 1 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_chk_read_cmd;
	ha->ha_cmd.nactions[subcmdno] =  NUMBER_OF_ACTIONS(lmsi_chk_read_actions);
	subcmdno++;

	ha->ha_cmd.nsubcmds = subcmdno;
	ha->ha_cmd.cmdname = "READ BLOCKS COMMAND";
	ha->ha_cmd_sp = sp;

	drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
	if(!ha->ha_tid)
		ha->ha_tid = ITIMEOUT(lmsi_timer, (void *) ha, lmsi_chktime, pldisk);
	lmsi_sendcmd(ha);
	return (0);
}

#ifdef LMSI_SEEK
/*******************************************************************************
 *
 *      lmsi_seek ( int unit, int m, int s, int f )
 *
 *      Do a seek to a particular sector
 *
 *      Entry :
 *                  unit        unit #
 *                  m           absolute minute
 *                  s           absolute second
 *                  f           absolute frame
 *
 *      Exit :
 *              0               OK
 *                              If OK:
 *              -1              error
 *                              If error:
 *                                  drive error condition cleared
 *
 *      Notes :
 *
 ******************************************************************************/

int 
lmsi_seek(int unit, int m, int s, int f)

{
}
#endif

/******************************************************************************
 *
 *	lmsibdinit( int c, HBA_IDATA_STRUCT *idata )
 *
 *	Initialize the LMSI controller 
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
lmsibdinit( int c, HBA_IDATA_STRUCT *idata )
{
	register uint_t	iobase = idata->ioaddr1;
	int 		i;
	drv_info_t		*drv;
	uint_t ndrives = 1;
	int	oldpl, limit;
	unchar	echo;

	/*
	 *	check to see if there is a lmsi here -- send 
	 *	'clear errors' and see if it echoes in 20ms or less
	 */

	oldpl = spldisk(); /* do test sans interrupts */

	inb(ReceiveRegister(iobase));  /* remove any stale character */
	outb(XmitRegister(iobase),0x4e);    /* clear errors */
	limit = 1000;

	while (!(inb(DriveStatRegister(iobase)) & RXRDY) && --limit) {
		drv_usecwait(20);
	}

	echo = inb(ReceiveRegister(iobase));

	splx(oldpl);

	if (!limit || echo != 0x4e) {
		/*
		 *	drive didn't respond
		 */
		return ( -1 );
	}

	lmsi_ctrl[ c ].flags = 0;
	lmsi_ctrl[ c ].num_drives = ndrives;
	lmsi_ctrl[ c ].iobase = iobase;

	/*
	*	run through and allocate drives/dig up info on them
	*/

	for ( i = 0; i < ndrives; i++ ) {
		if (( drv = ( drv_info_t * ) kmem_zalloc( sizeof( struct drv_info ), sleepflag ) ) == NULL ) {
			cmn_err( CE_WARN, "!lmsi_bdinit: Unable to allocate drv structure\n");
			return( -1 );
		}

		lmsi_ctrl[ c ].drive[ i ] = drv;
		cmn_err( CE_CONT, "!lmsi:_bdinit Configured HA %d TC %d\n",c,i );
	}

	return( ndrives);
}

unchar 
lmsi_waitrdy(uint_t iobase)
{
	int i = 0;

	i = 0;
	while((inb((DriveStatRegister(iobase))) & RXRDY) == 0  &&
		i++ < 500)
		drv_usecwait(20);
	if(i >= 500) {
#ifdef LMSI_DEBUG
		cmn_err(CE_NOTE, "lmsi_waitrdy: could not read echo/request");
#endif
		return(0);
	}
	return inb(ReceiveRegister(iobase));
}

void
lmsi_execute_action(scsi_ha_t *ha)
{
	CM205Command 	*cmd = &ha->ha_cmd;
	int		wait_for_intr;
	int		(*func)();
	int		val;
	
	if(cmd->subcmd_no > cmd->nsubcmds) {
#ifdef LMSI_DEBUG
		cmn_err(CE_NOTE, "lmsi_execute_action: Nothing to do");
#endif
		return;
	}
	do {
		SubCmd	     	*scmd = cmd->subcmd[cmd->subcmd_no];
		Action 		*a = &scmd->action[cmd->action_no];

		switch(cmd->phase_no) {
		case 0:
			if((func = a->preaction) == NULL)
				cmd->phase_no++;
			else break;
			/* FALLTHRU  */
		case 1:
			if((func = a->getanswer) == NULL)
				cmd->phase_no++;
			else break;
			/* FALLTHRU  */
		case 2:
			if((func = a->postaction) == NULL)
				cmd->phase_no++;
			else break;
			/* FALLTHRU  */
		case 3:
			cmd->phase_no = 0;
			if(++cmd->action_no >= cmd->nactions[cmd->subcmd_no]){
				if(++cmd->subcmd_no >= cmd->nsubcmds) {
#ifdef LMSI_DEBUG
					cmn_err(CE_NOTE, 
					"lmsi_execute_action: finish with %s",
						cmd->cmdname);
#endif
					return;
				}
				cmd->action_no = 0;
			}
			lmsi_execute_action(ha);
			return;
		}
		wait_for_intr = a->iflags &(1<<cmd->phase_no);
		cmd->phase_no++;
		val = (*func)(ha, a->arg);
	} while (val > 0 && wait_for_intr == 0);
}
				

int
lmsi_xmit_byte(scsi_ha_t *ha, unchar byte)
{
	int	i = 0;
	uint_t	iobase = ha->ha_iobase;
	unchar	sts;

	sts = inb(DriveStatRegister(iobase));
	if(sts&RXRDY) {
		(void) inb(ReceiveRegister(iobase));
	}
	while((sts&TXRDY) == 0 && i++ < 5000) {
		sts = inb(DriveStatRegister(iobase));
		if(sts&RXRDY) 
			(void)  inb(ReceiveRegister(iobase));
	}
	outb(XmitRegister(iobase), byte);
	return(1);
}

void 
lmsi_sendcmd(scsi_ha_t *ha)
{
	ha->ha_cmd.subcmd_no = ha->ha_cmd.action_no = ha->ha_cmd.phase_no = 0;
	lmsi_execute_action(ha);
}

void 
sector2msf(int sector, unchar *msf)
{
	msf[FRAME] = BIN2BCD(sector % 75); sector /= 75;
	msf[SECOND] = BIN2BCD(sector % 60);
	msf[MINUTE] = BIN2BCD(sector / 60);
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
 *	lmsi_rw (sblk_t *sp )
 *
 *	Convert a SCSI r/w request into an LMSI request
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
lmsi_rw(sblk_t * sp)
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
	pl_t		ospl;

	c = lmsi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	ha = &lmsi_sc_ha[c];

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
		cmn_err(CE_WARN, "LMSI: lmsi_rw() Unknown SC_CMDSZ -- %x\n",
			sb->SCB.sc_cmdsz);
		lmsi_callback(ha, NULL, sp, SDI_HAERR);
		return;
	}
	if (direction == B_WRITE) {
#ifdef LMSI_DEBUG
		cmn_err(CE_WARN, "LMSI: (lmsi_rw) write cmd received");
#endif
		lmsi_callback(ha, NULL, sp, SDI_HAERR);
		return;
	}

	blk_num = sector + ha->ha_startsec;

	if(count > LMSI_MAXBUFFERS) {
		cmn_err(CE_WARN, "!LMSI: lmsi_rw trying to read %d blocks starting at %d", count, blk_num);
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		sdi_callback(sb);
		return;
	}
	ha->ha_interrupted = 0;
	outb(DataControlRegister(ha->ha_iobase),MSKTXDR | count);
	drv_usecwait(10);
	sp->s_size  = count;
	sp->s_blkno = blk_num;
#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "lmsi_rw: sector %d, offset %d, cnt %d, addr %x",
				sector, ha->ha_startsec, count, 
				(caddr_t) (void *)  sp->sbp->sb.SCB.sc_datapt);
#endif
	ha->ha_cmd_sp = sp;
	ha->ha_repeat = (void (*)()) lmsi_issue_rdcmd;

	ospl = spldisk();
	/*
	 *	job is submitted -- update queues
	 */

	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	q = &LU_Q( c, t, l );
	q->q_count++;
	if ( q->q_count >= lmsi_hiwat )
		q->q_flag |= QBUSY;
	splx( ospl );
	
	lmsi_issue_rdcmd(sp, ha);
}

static Action lmsi_test_actions[] = {
{ 0x00, 0, lmsi_configure, NULL },  /* configure drive */
};
static SubCmd lmsi_test_cmd = {
	"TEST LMSI 205 COMMAND",
	lmsi_test_actions,
	1
};

/*******************************************************************************
 *
 *	lmsi_test ( struct sb *sb )
 *
 *	Handle SS_TEST scsi request
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
lmsi_test(sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	scsi_ha_t *ha;
	unchar	  subcmdno = 0;

#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "lmsi_test: entered");
#endif

	c = lmsi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &lmsi_sc_ha[c];

	if ((l != 0) || ((t != ha->ha_id) && (t >= lmsi_ctrl[c].num_drives))) {
		/*
		 * Not a valid lun or not a valid target
		 */
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		sdi_callback(sb);
		return;
	}
	ha->ha_status = 0;
	ha->ha_cmd_sp = sp;
	ha->ha_repeat = lmsi_test;

/* subcmd 0 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_clear_error_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_clear_error_actions);
	subcmdno++;

/* subcmd 1 */
	ha->ha_status_byte_no = 0;
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_read_status_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_read_status_actions);
	subcmdno++;

/* subcmd 2 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_test_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_test_actions);
	subcmdno++;

/* subcmd 3 */
	/* spin up the disk */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_spin_up_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_spin_up_actions);
	subcmdno++;

	ha->ha_cmd.nsubcmds = subcmdno;
	ha->ha_cmd.cmdname = "LMSI TEST COMMAND";
	
	drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
	if(!ha->ha_tid)
		ha->ha_tid = ITIMEOUT(lmsi_timer, (void *) ha, lmsi_cmd_timeout, pldisk);

	lmsi_sendcmd(ha);
}

/*******************************************************************************
 *
 *	lmsi_rdcap ( struct sb *sb )
 *
 *	Tell the pdi about the capacity of the drive
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

int	lmsi_read_cap();

static Action lmsi_end_read_cap_actions[]  = {
	{ 0x00, 0x0, lmsi_read_cap, NULL, NULL},
};
static SubCmd lmsi_end_read_cap_cmd = {
"READ TOC COMMAND",
lmsi_end_read_cap_actions,
1
};

#ifdef TOC_STREAM
int	lmsi_set_tocinfo();
static Action lmsi_set_toc_actions[]  = {
	{ 0x00, 0x0, lmsi_set_tocinfo, NULL, NULL},
};
static SubCmd lmsi_set_toc_cmd = {
"READ TOC COMMAND",
lmsi_set_toc_actions,
1
};
#endif

void
lmsi_rdcap(sblk_t *sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	drv_info_t     *drv;
	int             c, t, l;
	scsi_ha_t 	*ha;
	unchar		subcmdno = 0;

	c = lmsi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &lmsi_sc_ha[c];
	drv = lmsi_ctrl[c].drive[t];

	if ((l != 0) || (t >= lmsi_ctrl[c].num_drives)) {
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}
	drv->sense.sd_valid = 0;  /* and remove any old sense data */

	ha->ha_cmd_sp    = sp;
	ha->ha_repeat = lmsi_rdcap;
	ha->ha_status = 0;

#ifdef LMSI_DEBUG
	{
	unchar		drvsts, dat;
	drvsts = inb(DriveStatRegister(ha->ha_iobase));
	dat = inb(DataStatRegister(ha->ha_iobase));
	cmn_err(CE_NOTE, "lmsi_rdcap: drv %x, dat %x", drvsts, dat);
	}
#endif

	/* clear any old errors */
/* subcmd 0 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_clear_error_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_clear_error_actions);
	subcmdno++;

/* subcmd 1 */
	/* read an illegal block */
	lmsi_read_blocks_actions[1].arg = BIN2BCD(74);  /* frame */
	lmsi_read_blocks_actions[2].arg = BIN2BCD(59);  /* seconds */
	lmsi_read_blocks_actions[3].arg = BIN2BCD(99);  /* minutes */
	lmsi_read_blocks_actions[4].arg = 0x01;  /* 1 sector */
	lmsi_read_blocks_actions[5].arg = 0x00;
	lmsi_read_blocks_actions[6].arg = 0x00;
	lmsi_read_blocks_actions[6].iflags = 1;

	ha->ha_cmd.subcmd[subcmdno] = &lmsi_read_blocks_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_read_blocks_actions);
	subcmdno++;

/* subcmd 2 */
	ha->ha_status_byte_no = 0;
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_read_status_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_read_status_actions);
	subcmdno++;

/* subcmd 3 */
	/* clear any old errors */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_clear_error_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_clear_error_actions);
	subcmdno++;

#ifdef TOC_STREAM
/* subcmd 4 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_set_toc_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_set_toc_actions);
	subcmdno++;
/* subcmd 5 */

	ha->ha_toc_byte_no = 0;
	ha->ha_toc_data[3] = 0xFF;
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_read_toc_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_read_toc_actions);
	subcmdno++;
#endif

/* subcmd 6 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_end_read_cap_cmd;
	ha->ha_cmd.nactions[subcmdno] = NUMBER_OF_ACTIONS(lmsi_end_read_cap_actions);
	subcmdno++;

	ha->ha_cmd.nsubcmds = subcmdno;
	ha->ha_cmd.cmdname = "LMSI TEST COMMAND";
	ha->ha_cmd_sp = sp;
	
	drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
	if(!ha->ha_tid)
		ha->ha_tid = ITIMEOUT(lmsi_timer, (void *) ha, lmsi_chktime, pldisk);
	lmsi_sendcmd(ha);
}

int
lmsi_read_cap(scsi_ha_t	*ha)
{
	capacity_t	cap;
	sblk_t		*sp = ha->ha_cmd_sp;
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t;
	drv_info_t 	*drv;
#ifdef TOC_STREAM
	unchar		*tocbuf = ha->ha_toc_data;
#endif
	c = lmsi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	drv = lmsi_ctrl[c].drive[t];

#ifdef LMSI_DEBUG
	uint_t		iobase = ha->ha_iobase;
	unchar		drvsts, dat;

	drvsts = inb(DriveStatRegister(iobase));
	dat = inb(DataStatRegister(iobase));
	
	cmn_err(CE_NOTE, "lmsi_read_cap: drv %x, dat %x",
		drvsts, dat);
#endif

#ifdef TOC_STREAM
	drv_usecwait(10000);
	read_toc_data(ha);

	outb(DataControlRegister(iobase),(MSKTXDR | 1));
	drv_usecwait(10000);    
	outb(TestRegister(iobase), 0);
	drv_usecwait(100);
#endif

	ha->ha_endsec  = msf2sector(
		ha->ha_status_data.lminute,
		ha->ha_status_data.lsecond,
		ha->ha_status_data.lframe);

#ifdef TOC_STREAM
	if(ha->ha_toc_byte_no >= 4)
		ha->ha_startsec = (tocbuf[1]+75*(tocbuf[2]+60 * tocbuf[3]));
	else	ha->ha_startsec = msf2sector(0, 2, 0);
	tocbuf = &ha->ha_toc_data[ha->ha_toc_byte_no-5];
	cmn_err(CE_NOTE, "End track %x Endsect from status %d, from toc %d",
		tocbuf[0],
		ha->ha_endsec, 
		tocbuf[1]+75*(tocbuf[2]+60 * tocbuf[3]));
#else
	ha->ha_startsec = msf2sector(0, 2, 0);
#endif

	drv->bufcnt = 0;

	drv->drvsize = ha->ha_endsec - ha->ha_startsec;

	cap.cd_addr = sdi_swap32 ( ha->ha_endsec - ha->ha_startsec);
	cap.cd_len = sdi_swap32 (LMSI_BLKSIZE);
	bcopy ( (char *)&cap, sb->SCB.sc_datapt, RDCAPSZ );
	sb->SCB.sc_comp_code = SDI_ASW;
#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "lmsi_rdcap: start block %x, last block %x", 
			ha->ha_startsec, ha->ha_endsec);
#endif
	ha->ha_cmd_sp = NULL;
	sdi_callback(sb);
	return(0);
}

/*******************************************************************************
 *
 *	lmsi_sense ( struct sb *sb )
 *
 *	Figure out sense information on a given device
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

int
lmsi_chkread(scsi_ha_t *ha)
{
	sblk_t 		*sp = ha->ha_cmd_sp;
	struct sb      *sb = (struct sb *) &sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	scsi_lu_t	*q;		/* queue ptr */
	unchar		drv, dat;
	int		iobase;
	uchar_t		*addr = (uchar_t *) sp->sbp->sb.SCB.sc_datapt;

	c = lmsi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	q = &LU_Q( c, t, l );
	iobase = ha->ha_iobase;

	drv = inb(DriveStatRegister(iobase));
	dat = inb(DataStatRegister(iobase));

	if (drv & RXRDY) 
		(void) inb(ReceiveRegister(iobase));

	/*
	 *	check if drive wants attention
	 */

	if ((drv & (ATTN|PERROR|FIFOMTY|OVERRUN)) || 
	    (dat&(DATERR|CRCERR|FIFOV))) {
		/*
		 *	handle the condition
		 */
		lmsi_chkerror(ha);
		return(0);
	}
	if (dat & DATRDY) {
		int cnt, sectors;
		
		cnt = dat & SECCNT;

		if(cnt != 0) {
#ifdef LMSI_DEBUG
			cmn_err(CE_WARN, "%d blocks read left: try again", 
					cnt);
#endif
			lmsi_chkerror(ha);
			return(0);
		} 
		sectors = sp->s_size;
		do {
		/*
		 *	there is a 16 byte preamble: 12 sync + 4 hdr to discard
		 */
		for(cnt=0; cnt<16; cnt++)
			if((inb(DriveStatRegister(iobase))&FIFOMTY) == 0)
				(void) inb(DataFifoRegister(iobase));
			else {
				lmsi_chkerror(ha);
				return(0);
			}
		/*
		 *	2048 bytes of real data
		 */
		for(cnt=0; cnt<2048; cnt++)
			if((inb(DriveStatRegister(iobase))&FIFOMTY) == 0)
				*addr++ = inb(DataFifoRegister(iobase));
			else {
				lmsi_chkerror(ha);
				return(0);
			}
		/*
		 *	EDC/ECC -- discard
		 */
		for(cnt=0; cnt<288; cnt++)
			if((inb(DriveStatRegister(iobase))&FIFOMTY) == 0) {
				(void) inb(DataFifoRegister(iobase));
			} else {
				lmsi_chkerror(ha);
				return(0);
			}
			--sectors;
		} while (sectors > 0);
		lmsi_callback(ha, q, sp, SDI_ASW);
	} else	{
#ifdef LMSI_DEBUG
		cmn_err(CE_NOTE, "lmsi_chkread: DATARY nor ATT are set drv %x, dat %x",
			drv, dat);
#endif
		lmsi_chkerror(ha);
	}
	return(0);
}

/* check for error cmd */
static  Action lmsi_check_error_actions[] = {
  { 0x00, 0x00, lmsi_examine_error, NULL, NULL},
};
static  SubCmd lmsi_check_error_cmd = {
  "EXAMINE ERROR COMMAND",
  lmsi_check_error_actions,
  1
};

void
lmsi_chkerror(scsi_ha_t *ha)
{
	unchar	subcmdno = 0;
/* subcmd 0 */
	ha->ha_status_byte_no = 0;
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_read_status_cmd;
	ha->ha_cmd.nactions[subcmdno] = 19;
	subcmdno++;

/* subcmd 1 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_clear_error_cmd;
	ha->ha_cmd.nactions[subcmdno] = 1;
	subcmdno++;

/* subcmd 2 */
	/* spin up the disk */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_spin_up_cmd;
	ha->ha_cmd.nactions[subcmdno] = 1;
	subcmdno++;

/* subcmd 3 */
	ha->ha_cmd.subcmd[subcmdno] = &lmsi_check_error_cmd; 
	ha->ha_cmd.nactions[subcmdno] = 1;
	subcmdno++;

	ha->ha_cmd.nsubcmds = subcmdno;
	ha->ha_cmd.cmdname = "CHECKING FOR ERROR COMMAND";

	drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
	if(!ha->ha_tid)
		ha->ha_tid = ITIMEOUT(lmsi_timer, (void *) ha, lmsi_chktime, pldisk);
	lmsi_sendcmd(ha);
}
int
lmsi_examine_error(scsi_ha_t *ha)
{
	sblk_t 		*sp = ha->ha_cmd_sp;
	
	drv_usecwait(100); /* wait for the disk to spin again */
#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "lmsi_examine_error:");
	cmn_err(CE_NOTE, "   drvstatus %x, drvcode %x errocode %x",
				ha->ha_status_data.drvstatus,
				ha->ha_status_data.drvcode,
				ha->ha_status_data.errcode);
#endif
	if(sp) {
		struct sb      *sb = (struct sb *) &sp->sbp->sb;
		struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
		int             c, t, l;
		scsi_lu_t	*q;		/* queue ptr */

		c = lmsi_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q( c, t, l );
		if(ha->ha_status_data.drvstatus&02){
#ifdef LMSI_DEBUG
			cmn_err(CE_NOTE, "lmsi_examine: drvsts %x drvcod %x, condition code %x",
			ha->ha_status_data.drvstatus,
			ha->ha_status_data.drvcode,
			ha->ha_status_data.errcode);
#endif
		switch(ha->ha_status_data.drvcode) {
			case 0x02:
			case 0x06:
			case 0x19:
			case 0x1A:
			case 0x1B:
			case 0x1C:
			case 0x1E:
			case 0x1F:
			if( ha->ha_repeat == (void (*)()) lmsi_issue_rdcmd)
				lmsi_callback(ha, q, ha->ha_cmd_sp, SDI_HAERR);
			else	lmsi_callback(ha, NULL, ha->ha_cmd_sp, SDI_HAERR);
			return(0);
			}
		}
		if( sp->s_tries < lmsi_max_tries){
			sp->s_tries++;
			(*ha->ha_repeat)(sp, ha);
#ifdef LMSI_DEBUG
			cmn_err(CE_NOTE, "lmsi_examine_error: retrying command");
#endif
			return(0);
		}
		if( ha->ha_repeat == (void (*)()) lmsi_issue_rdcmd)
			lmsi_callback(ha, q, ha->ha_cmd_sp, SDI_HAERR);
		else	lmsi_callback(ha, NULL, ha->ha_cmd_sp, SDI_HAERR);
	}
	return(0);
}

void
lmsi_callback(scsi_ha_t *ha, scsi_lu_t *q, sblk_t *sp, ulong_t code)
{
	struct sb      *sb;
	ha->ha_cmd_sp = NULL;
	ha->ha_repeat = NULL;
	
	if(sp == NULL) 
		return;
	sb = (struct sb *) & sp->sbp->sb;
	sb->SCB.sc_comp_code = code;
	
#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "sdi_callback(sb=%x, code=%x)", sb, code);
#endif
	sdi_callback(sb);
	if(q) {
		int	ospl = spldisk();
		q->q_flag &= ~QBUSY;
		q->q_count--;
		splx(ospl);
		lmsi_next( q );
	}
}

void
lmsi_timer(scsi_ha_t  *ha)
{
	clock_t now;
	ulong_t elapsed;
	sblk_t *sp;
#ifdef LMSI_DEBUG
	unchar	drv, dat;

	drv = inb(DriveStatRegister(ha->ha_iobase));
	dat = inb(DataStatRegister(ha->ha_iobase));
	cmn_err(CE_NOTE, "lmsi_timer entered, drv %x, dat %x", drv, dat);
#else 
	(void) inb(DriveStatRegister(ha->ha_iobase));
	(void) inb(DataStatRegister(ha->ha_iobase));
#endif 

	ha->ha_tid = 0;
	drv_getparm(LBOLT, (void *) &now);
	elapsed = now - ha->ha_cmd_time;

	if((sp = ha->ha_cmd_sp) != NULL && ha->ha_interrupted == 0) {
		struct sb      *sb = (struct sb *) & sp->sbp->sb;
		struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
		int             c, t, l;
		scsi_lu_t	*q;		/* queue ptr */

		c = lmsi_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q( c, t, l );

		if(elapsed > lmsi_cmd_timeout) {
			if(sp->s_tries < lmsi_max_tries){
				sp->s_tries++;
				(*ha->ha_repeat)(sp, ha);
#ifdef LMSI_DEBUG
				cmn_err(CE_NOTE, "lmsi_timer retrying command");
#endif
				return;
			}
#ifdef LMSI_DEBUG
			cmn_err(CE_NOTE, "lmsi_timer abort command");
#endif
			lmsi_callback(ha, q, sp, SDI_HAERR);
			return;
		}
#ifdef LMSI_DEBUG
		cmn_err(CE_NOTE, "lmsi_timer did not abort command");
#endif
	} else if(elapsed > lmsi_max_idle_time) {
		/* 
		 * there is not that much activity. Stop the the timer.
		 * It will be enabled again the next command.
		 */
#ifdef LMSI_DEBUG
		cmn_err(CE_NOTE, "lmsi_timer is canceled");
#endif
		return;
	}
#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "lmsi_timer is set again");
#endif
		
	drv_getparm(LBOLT, (void *) &ha->ha_cmd_time);
	if(!ha->ha_tid)
		ha->ha_tid = ITIMEOUT(lmsi_timer, (void *) ha, lmsi_chktime, pldisk);
}

int
lmsi_configure(scsi_ha_t *ha)
{
	if ( ha->ha_state == HA_UNINIT ) {
		/*
		*	ha is dead
		*/
		return(0);
	}
	if(ha->ha_status_data.drvstatus&02){
#ifdef LMSI_DEBUG
		cmn_err(CE_NOTE, "lmsi_configure: drvsts %xdrvcod %x, condition code %x",
		ha->ha_status_data.drvstatus,
		ha->ha_status_data.drvcode,
		ha->ha_status_data.errcode);
#endif
		lmsi_callback(ha, NULL, ha->ha_cmd_sp, SDI_HAERR);
	} else  lmsi_callback(ha, NULL, ha->ha_cmd_sp, SDI_ASW);
	return (1);
}

int
lmsi_read_echo(scsi_ha_t * ha)
{
#ifdef LMSI_DEBUG
	unchar	c = 0;
	c = lmsi_waitrdy(ha->ha_iobase);
	cmn_err(CE_NOTE, "READ_ECHO: read %x char", c);
#else
	(void) lmsi_waitrdy(ha->ha_iobase);
#endif
	return(1);
}
#ifdef TOC_STREAM
int
read_toc_data(scsi_ha_t * ha)
{
	uint_t iobase = ha->ha_iobase;
	unchar	c = 0;
	unchar	*buf = ha->ha_toc_data;
	unchar	drv, dat;

	drv = inb(DriveStatRegister(iobase));
	dat = inb(DataStatRegister(iobase));
	
	cmn_err(CE_NOTE, "lmsi_read_cap: drv %x, dat %x",
		drv, dat);
	ha->ha_toc_byte_no = 0;
	if(drv& TOCRDY) {
		cmn_err(CE_NOTE, "TOCRDY is set");
		while ((drv&FIFOMTY) == 0 && 
		    ha->ha_toc_byte_no < MAX_TOC_SIZE) {
			buf[ha->ha_toc_byte_no] = DataFifoRegister(iobase);
			cmn_err(CE_CONT, "%x ", buf[ha->ha_toc_byte_no]);
			if(buf[ha->ha_toc_byte_no] == 0xFF)
				break;
			ha->ha_toc_byte_no++;
			drv = inb(DriveStatRegister(iobase));
		}
	}
	cmn_err(CE_NOTE, "%d tocbytes were received",
		ha->ha_toc_byte_no);
	return(1);
}
#endif


int
lmsi_read_req(scsi_ha_t * ha)
{
	uint_t iobase = ha->ha_iobase;
	unchar	c = 0;
	unchar	*buf = (unchar *) &ha->ha_status_data;

	c = lmsi_waitrdy(iobase);
 
#ifdef LMSI_DEBUG
	cmn_err(CE_NOTE, "Status byte %d is %x", ha->ha_status_byte_no, c);
#endif
	buf[ha->ha_status_byte_no++] = c;
	return(1);
}

#ifdef LMSI_DEBUG
void
lmsiwakeup()
{
	int			c;		/* controller */
	struct scsi_ha		*ha;

	cmn_err( CE_CONT, "lmsiwakeup entered");
	/*
	 *	figure out where the interrupt came from
	 */
	for ( c = 0; c < lmsi_cntls; c++ ) {
		int             t, l;
		scsi_lu_t	*q;		/* queue ptr */

		ha = &lmsi_sc_ha[ c ];

		if(ha->ha_cmd_sp) {
			struct sb      *sb = (struct sb *) & ha->ha_cmd_sp->sbp->sb;
			struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
			t = SDI_TCN(sa);
			l = SDI_LUN(sa);
			q = &LU_Q( c, t, l );

			lmsi_callback(ha, q, ha->ha_cmd_sp, SDI_ASW);
		}
	}
}
#endif

#ifdef TOC_STREAM
int
lmsi_set_tocinfo(scsi_ha_t *ha)
{
	unchar	drv, dat;

	drv = inb(DriveStatRegister(ha->ha_iobase));
	dat = inb(DataStatRegister(ha->ha_iobase));
	
	outb(DataControlRegister(ha->ha_iobase),(INIT | MSKTXDR | 1));
	drv_usecwait(10000);    /* wait 10 ms */
	outb(TestRegister(ha->ha_iobase), TOCENAB);
	drv_usecwait(100);

	cmn_err(CE_NOTE, "lmsi_set_tocinfo :drv %x, dat %x, 1st track  %d, last track %d",
		drv, dat,
		ha->ha_status_data.firsttrk, 
		ha->ha_status_data.firsttrk + ha->ha_status_data.ntracks); 
	read_toc_actions[1].arg = ha->ha_status_data.firsttrk;
	read_toc_actions[2].arg =  ha->ha_status_data.firsttrk +
				   ha->ha_status_data.ntracks; 
}
#endif

#if PDI_VERSION >= PDI_UNIXWARE20
/*
 * int
 * lmsi_verify(rm_key_t key)
 *
 * Calling/Exit State:
 *	None
 *
 * Description:
 *	Verify the board instance.
 */
int
lmsiverify(rm_key_t key)
{
	HBA_IDATA_STRUCT	vfy_idata;
	register uint_t 	iobase;
	int status;
	int	rv;
	int	oldpl, limit;
	unchar	echo;

	/*
	 * Read the hardware parameters associated with the
	 * given Resource Manager key, and assign them to
	 * the idata structure.
	 */
	rv = sdi_hba_getconf(key, &vfy_idata);
	if(rv != 0) {
		return(rv);
	}

	/*
	 * Verify hardware parameters in vfy_idata,
	 * return 0 on success, ENODEV otherwise.
	 */

	iobase = vfy_idata.ioaddr1;

	/*
	 * Check to see if there is a lmsi here -- send
	 * 'clear errors' and see if it echoes in 20ms or less
	 */

	oldpl = spldisk();			/* do test sans interrupts */

	inb(ReceiveRegister(iobase));		/* remove any stale character */
	outb(XmitRegister(iobase),0x4e);	/* clear errors */
	limit = 1000;

	while (!(inb(DriveStatRegister(iobase)) & RXRDY) && --limit) {
		drv_usecwait(20);
	}

	echo = inb(ReceiveRegister(iobase));

	splx(oldpl);

	if (!limit || echo != 0x4e) {
		/*
		 *	drive didn't respond
		 */
		return (ENODEV);
	}

	return(0);
}
#endif /* PDI_VERSION >= PDI_UNIXWARWE20 */

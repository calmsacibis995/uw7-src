#ident	"@(#)kern-pdi:io/hba/sony/hba.c	1.17.3.1"
#ident	"@(#)kern-pdi:io/hba/sony/hba.c	1.17.3.1"

/*******************************************************************************
 *******************************************************************************
 *
 *	HBA.C
 *
 *	Generic part of an HBA
 *
 *	Notes :
 *		- contains all the upper level HBA routines
 *		
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	INCLUDES
 *
 ******************************************************************************/


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
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#include <io/hba/sony/sony.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#include <io/hba/sony/sony.h>
#endif /* !(PDI_VERSION <= 1) */

#include <util/mod/moddefs.h>
#include <io/dma.h>

#include <io/hba/hba.h>
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
#include <sys/sony.h>

#include <sys/moddefs.h>
#include <sys/dma.h>

#include <sys/hba.h>
#if PDI_VERSION >= PDI_SVR42MP
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif
#endif /* _KERNEL_HEADERS */

#if PDI_VERSION < PDI_SVR42MP
#define KMEM_ZALLOC kmem_zalloc
#else /* PDI_VERSION > PDI_SVR42MP */
#define KMEM_ZALLOC sony_kmem_zalloc_physreq
#define SONY_MEMALIGN	2
#define SONY_BOUNDARY	0
#define SONY_DMASIZE	24
#endif /* PDI_VERSION > PDI_SVR42MP */

/*******************************************************************************
 *
 *	DEFINES AND THINGS
 *
 ******************************************************************************/

/*******************************************************************************
 *
 *	GLOBAL VARIABLES
 *
 ******************************************************************************/

int		sonydevflag = 0;

/* the drive is having problems with requests larger than 1 block */
HBA_INFO( sony, &sonydevflag, 4096); /* hba driver info. One page max contiguous */

struct ver_no 	sony_sdiver;     	/* SDI version struct */
int     	sony_sleepflag; 	/* KM_SLEEP/KM_NOSLEEP */
#if PDI_VERSION < PDI_UNIXWARE20
int            	sony_cntls;     	/* number of controllers */
#endif
scsi_ha_t       *sony_sc_ha;            /* host adapter structs */
int             sony_sc_ha_size;        /* size of sony_sc_ha alloc */
ctrl_info_t     *sony_ctrl;             /* local controller structs */
int             sony_ctrl_size; 	/* size of sony_ctrl alloc */

STATIC int      sony_mod_dynamic = 0;   /* 1 if was a dynamic load */

/*******************************************************************************
 *
 *	EXTERNALS
 *
 ******************************************************************************/

extern long		sdi_started;
extern struct head	sm_poolhead;		/* 28 byte structs */
extern int		sony_gtol [];		/* global to local */
extern int		sony_ltog [];		/* local to global */
#if PDI_VERSION >= PDI_UNIXWARE20
extern HBA_IDATA_STRUCT	_sonyidata[];		/* hardware info */
extern int		sonyverify();
extern int		sony_cntls;		/* number of controllers */
#else  /* PDI_VERSION < PDI_UNIXWARE20 */
extern HBA_IDATA_STRUCT	sonyidata [];		/* hardware info */
#endif /* PDI_VERSION < PDI_UNIXWARE20 */
extern void 		mod_drvattach();
extern void 		mod_drvdetach();
extern struct sonyvector *sonyvectorlist[];
extern void sony_func(sblk_t * sp);
extern void sony_cmd(sblk_t * sp);

#if PDI_VERSION >= PDI_UNIXWARE20
/*
 * The name of this pointer is the same as that of the
 * original idata array.  Once it is assigned the address
 * of the new array it can be reference as before and
 * the code need not change.
 */
HBA_IDATA_STRUCT	*sonyidata;		/* hardware info */
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */



/*******************************************************************************
 *
 *	PROTOTYPES
 *
 ******************************************************************************/

/*
 *	Loadable Driver Routines
 */

int			sony_load( void );
int			sony_unload( void );

/*
 *	queue support
 */

void 			sony_putq( scsi_lu_t *q, sblk_t *sp );
void 			sony_next( scsi_lu_t *q );
void 			sony_flushq( scsi_lu_t *q, int cc );
int 			sony_illegal( ushort_t c, uchar_t t, uchar_t l, int m );

/*
 *	scsi pass-thru
 */

void 	sony_pass_thru( struct buf *bp );
void 	sony_int( struct sb *sp );

/*****************************************************************************
 *****************************************************************************
 *
 *	PDI/KERNEL INTERFACE ROUTINES
 *
 *****************************************************************************
 ****************************************************************************/

#if PDI_VERSION >= PDI_UNIXWARE20

/****************************************************************************** 
 *
 *	void * sony_kmem_zalloc_physreq( size_t size, int sleepflag)
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
sony_kmem_zalloc_physreq(size_t size, int sleepflag)
{
	void           *mem;

	physreq_t      *preq;

	preq = physreq_alloc(sleepflag);
	if (preq == NULL)
		return NULL;
	preq->phys_align = SONY_MEMALIGN;
	preq->phys_boundary = SONY_BOUNDARY;
	preq->phys_dmasize = SONY_DMASIZE;
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
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

/*******************************************************************************
 *
 *	Wrapper 
 *
 ******************************************************************************/

#if PDI_VERSION >= PDI_UNIXWARE20
MOD_ACHDRV_WRAPPER( sony, sony_load, sony_unload, NULL, sonyverify, DRVNAME );
#else
MOD_HDRV_WRAPPER( sony, sony_load, sony_unload, NULL, DRVNAME );
#endif


/*****************************************************************************
 *****************************************************************************
 *
 *	SONY HARDWARE SUPPORT
 *
 *		sonyintr		interrupt routine
 *
 *****************************************************************************
 ****************************************************************************/
/******************************************************************************
 *
 *	sonyintr( unsigned int vec )
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
sonyintr( unsigned int vec )

{
	int			c;		/* controller */
	struct scsi_ha		*ha;

#ifdef SONY_DEBUG
	cmn_err( CE_CONT, "sony:(intr) vec = %x", vec );
#endif
	/*
	 *	figure out where the interrupt came from
	 */
	for ( c = 0; c < sony_cntls; c++ ) {
		ha = &sony_sc_ha[ c ];
		if ( ha->ha_vect == vec ) {
			break;
		}
	}

	if (c >= sony_cntls) {
#ifdef SONY_ERRS
		cmn_err( CE_CONT, "sony:(intr) vec %x is out of range", vec );
#endif
		return;
	}

	if ( ha->ha_devicevect)
		ha->ha_devicevect->sonyintr(ha);
}


/*****************************************************************************
 *
 *	sonyinit( void )
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
sonyinit( void )
{
	int 		i, j;
	scsi_ha_t		*ha;		/* random ha ptr */

#if PDI_VERSION >= PDI_UNIXWARE20
	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	sonyidata = sdi_hba_autoconf("sony", _sonyidata, &sony_cntls);
	if(sonyidata == NULL) {
		return (-1);
	}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	for ( i = 0; i < MAX_HAS; i++ )		/* init hbano translation */
		sony_gtol[ i ] = sony_ltog[ i ] = -1;

	sdi_started = TRUE;
	sony_sdiver.sv_release = 1;
	sony_sdiver.sv_machine = SDI_386_AT;
	sony_sdiver.sv_modes = SDI_BASIC1;

	sony_sleepflag = sony_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/*
	 *	allocate space for sony_sc_ha stuff -- must be contiguous
	 *      if it is going to be used with dma.
	 */

	sony_sc_ha_size = sony_cntls *( sizeof( struct scsi_ha ) );
	if ((sony_sc_ha = (struct scsi_ha *) KMEM_ZALLOC( sony_sc_ha_size, sony_sleepflag ) ) == NULL ) {
		cmn_err( CE_WARN, "sony: Unable to allocate SONY31A_SC_HA( %x )\n", sony_sc_ha_size );
		return( -1 );
	}

	/*
	 *	allocate space for sony_ctrl stuff -- must be contiguous
	 */

	sony_ctrl_size = sony_cntls *( sizeof( struct ctrl_info ) );
	if (( sony_ctrl = (ctrl_info_t *) KMEM_ZALLOC( sony_ctrl_size, sony_sleepflag ) ) == NULL ) {
		cmn_err( CE_WARN, "sony: Unable to allocate CTRL( %x )\n", sony_ctrl_size );
		kmem_free( sony_sc_ha, sony_sc_ha_size );
		return( -1 );
	}


	/*
	 *	loop through adapter list and check for them
	 */

	for ( i = 0; i < sony_cntls; i++ ) {

		/*
		 *	do a little pre-initialization of the ha table
		 */

		ha = &sony_sc_ha[ i ];
		ha->ha_state = HA_UNINIT;
		ha->ha_iobase = sonyidata[ i ].ioaddr1;
		ha->ha_id = sonyidata[ i ].ha_id;
		ha->ha_vect = sonyidata[ i ].iov; 
		ha->ha_statsize = 1; /* status size for 535 */
		ha->ha_devicevect = NULL;
		/* may want to mke this per drive in future */
        	if (( ha->ha_resbuf = (uchar_t *) kmem_zalloc( SONY31A_RESSIZE, sony_sleepflag ) ) == NULL ) {
                    cmn_err( CE_WARN, "sony: Unable to allocate resbuf( 36 )\n");
                    kmem_free( ha->ha_resbuf, SONY31A_RESSIZE );
                    return( -1 );
        	}

		for (j=0; sonyvectorlist[ j ]; j++) {

		    if ( (*sonyvectorlist[ j ]->sonybdinit)( i, &sonyidata[ i ] ) != -1 ) {
			ha->ha_devicevect = sonyvectorlist[ j ];
			break;
		    }
		}
			
		/*
	         *	didn't find the board
	         */
		if (ha->ha_devicevect == NULL) {
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
 *	int sonystart( void )
 *
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
sonystart( void )
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

	HBA_IDATA(sony_cntls);

	for ( i = 0; i < sony_cntls; i++ ) {
		ha = &sony_sc_ha[ i ];

		if (ha->ha_state == HA_UNINIT)
			continue;

		/*
		 *	board found -- allocate queues and things
		 */
		if (( ha->ha_queue = (scsi_lu_t *) KMEM_ZALLOC( queue_size, sony_sleepflag ) ) == NULL ) {
			cmn_err( CE_WARN, "sony: Unable to allocate queues( %x )\n", queue_size );
			continue;
		}

		/*
		 *	register with sdi and do more setup
		 */

		if (( cntlr = sdi_gethbano( sonyidata[i].cntlr ) ) < 0 ) {
			cmn_err( CE_WARN, "sony:(init) HA %d unable to allocate HBA with SDI\n",i);
			kmem_free( ha->ha_queue, queue_size );
			continue;
		}

		sonyidata[ i ].cntlr = cntlr;
		sony_gtol[ cntlr ] = i;
		sony_ltog[ i ] = cntlr;

		if ((cntlr = sdi_register( &sonyhba_info, &sonyidata[i] ) ) < 0 ) {
			cmn_err( CE_WARN, "!sony:(init) HA %d unable to register with SDI\n",i);
			kmem_free( ha->ha_queue, queue_size );
			continue;
		}

		ha->ha_dmachan = sonyidata[i].dmachan1;
		if (ha->ha_dmachan <= 0)
			sonyhba_info.max_xfer = SONY_BLKSIZE;

#if PDI_VERSION >= PDI_SVR42MP
		if (ha->ha_dmachan > 0) {
                    if ((ha->ha_cb = dma_get_cb(sony_sleepflag)) == NULL) {
                        cmn_err(CE_CONT,"SONY: dma_get_cb() failed\n");
                        return(-1);
                    }
                    if ((ha->ha_cb->targbufs = dma_get_buf(sony_sleepflag)) == NULL) {
                        cmn_err(CE_CONT,"SONY: dma_get_buf() failed\n");
                        dma_free_cb(ha->ha_cb);
                        return(-1);
                    }
                    ha->ha_cb->targ_step = DMA_STEP_HOLD;
                    ha->ha_cb->targ_path = DMA_PATH_8;
                    ha->ha_cb->trans_type = DMA_TRANS_SNGL;
                    ha->ha_cb->targ_type = DMA_TYPE_IO;
                    ha->ha_cb->bufprocess = DMA_BUF_SNGL;
		}
#endif

		sonyidata[ i ].active = 1;
		found ++;
		cmn_err(CE_NOTE, "!SONY CDROM type %s was found", 
			ha->ha_sony_diskprod);
		cmn_err(CE_NOTE, "!SONY CDROM version %s was found", 
			ha->ha_sony_drive_ver);
#ifdef SONY_DEBUG
		drv_usecwait(1000000);
#endif
	}

	if ( !found ) {
		cmn_err( CE_CONT, "!No SONY adapter(s) found\n");
		if(sony_sc_ha)
			kmem_free( sony_sc_ha, sony_sc_ha_size );
		if(sony_ctrl)
			kmem_free( sony_ctrl, sony_ctrl_size);
		return( -1 );
	} 

#if PDI_VERSION >= PDI_UNIXWARE20
	/*
	 * Attach interrupts for each "active" device. The driver
	 * must set the active field of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here, in init(), as opposed
	 * to load(), because this is required for static autoconfig
	 * drivers as well as loadable.
	 */
	sdi_intr_attach(sonyidata, sony_cntls, sonyintr, sonydevflag);
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
	return( 0 );
}


/******************************************************************************
*******************************************************************************
 *
 *	Loadable Driver Stuff
 *
 *	Routines:
 *		sony_load			loader load 
 *		sony_unload			loader unload 
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	sony_load() 
 *
 *	Load and register the driver on demand
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		0		cool
 *		x		error
 * 
 ******************************************************************************/


int
sony_load( void )
{
        sony_mod_dynamic = 1;
	if (sonyinit()) {
#if PDI_VERSION >= PDI_UNIXWARE20
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
		sdi_acfree(sonyidata, sony_cntls);
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
		/*
		 *	unable to locate drivers
 		 */
		cmn_err( CE_CONT, "!sony: Unable to locate controllers\n");
		return( ENODEV );
	}
#if PDI_VERSION < PDI_UNIXWARE20
	mod_drvattach( &sony_attach_info );
#endif /* PDI_VERSION < PDI_UNIXWARE20 */
	if (sonystart()) {
#if PDI_VERSION >= PDI_UNIXWARE20
		/*
		 * At this point we have allocated a new idata array,
		 * free it before failing.
		 */
		sdi_acfree(sonyidata, sony_cntls);
#else  /* PDI_VERSION < PDI_UNIXWARE20 */
		mod_drvdetach (&sony_attach_info);
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */
		return(ENODEV);
	}
	return( 0 );
}

/*******************************************************************************
 *
 *	sony_unload( )
 *
 *	Unload and unregister the driver
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		0		cool
 *		x		error
 *
 ******************************************************************************/

int
sony_unload( void )
{
	return( EBUSY );
}

/*******************************************************************************
 *******************************************************************************
 *
 *	HBA SCSI Block Routines
 *
 *	Routines:
 *		sonygetblk			get a scsi block
 *		sonyfreeblk			free a scsi block
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	sonygetblk( void )
 *
 *	Allocate a SCSI block(hbadata) for the caller
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		*		ptr to the hbadata
 *
 *	Notes :
 *		- this routine CAN sleep
 *
 ******************************************************************************/

/*ARGSUSED*/
struct hbadata *
HBAGETBLK( int sleepflag )
{
	sblk_t		*sp;


	sp =(sblk_t *) SDI_GET(&sm_poolhead, sleepflag);

	return((struct hbadata *) sp );
}

/*******************************************************************************
 *
 *	sonyfreeblk( struct hbadata *hbap )
 *
 *	Free a SCSI data block 
 *
 *	Entry :
 *		*hbap		ptr to block to free
 *
 *	Exit :
 *		SDI_RET_OK	cool
 *
 ******************************************************************************/

long
HBAFREEBLK( struct hbadata *hbap )
{
	sblk_t			*sp =(sblk_t *) hbap;

	/*
	 *	free any personal stuff here if necessary
	 */
	sdi_free(&sm_poolhead, (jpool_t *)sp);

	return( SDI_RET_OK );
}


/******************************************************************************
 *
 *	sonyxlat( struct hbadata *hbap, int flag, struct proc *procp )
 *
 *	Crunch the SCB's data pointers from virtual to physical
 *
 *	Entry :
 *		*hbap		ptr to job request
 *		flags		flags for type
 *		*procp		ptr to user's process
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 *****************************************************************************/
/*ARGSUSED*/
HBAXLAT_DECL
HBAXLAT( struct hbadata *hbap, int flag, struct proc *procp, int sleepflag )
{
	sblk_t			*sp = (sblk_t *) hbap;

	if (!sp->sbp->sb.SCB.sc_datapt ) {
		/*
		 *	no data -- make sure datasz is 0
		 */
		sp->sbp->sb.SCB.sc_datasz = 0;
	} else {
		sp->s_paddr = vtop(sp->sbp->sb.SCB.sc_datapt, procp);
	}
	HBAXLAT_RETURN(0);
}

/*******************************************************************************
 *******************************************************************************
 *
 *	Command Send Routines(down from the PDI)
 *
 *	Routines:
 *		sonysend				send a cmd to ctrlr
 *		sonyicmd				send an IMMEDIATE cmd to ctrlr
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	sonysend( struct hbadata *hbap )
 *	
 *	Send a SCSI command to the controller
 *
 *	Entry :
 *		*hbap		ptr to scsi command
 *
 *	Exit :
 *		SDI_RET_OK	cool
 *		SDI_RET_RETRY	queue full -- try again later
 *		SDI_RET_ERR	something went wrong
 *
 *	Notes :
 *
 *************************************************************************/

/*ARGSUSED*/
long
HBASEND( struct hbadata *hbap, int sleepflag )
{
	struct scsi_ad		*sa;
	scsi_lu_t		*q;
	sblk_t			*sp =(sblk_t *) hbap;
	int			c;
	int			t;
	int			l;
	pl_t			oip;

#ifdef SONY31A_DEBUG
	cmn_err(CE_NOTE, "sonysend entered sb %x",
		&sp->sbp->sb);
#endif

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = sony_gtol [ SDI_HAN(sa) ];
	t = SDI_TCN( sa );
	l = SDI_LUN( sa );

	if ( sp->sbp->sb.sb_type != SCB_TYPE ) {
		/*
		 *	we only handle scsi command blocks
		 */
		cmn_err(CE_NOTE, "sonysend sb_type != SCB_TYPE");
		return( SDI_RET_ERR );
	}

	q = &SONY_LU_Q( c, t, l );

	oip = spldisk();
	if ( q->q_flag & QPTHRU ) {
		splx( oip );
#ifdef SONY31A_DEBUG
		cmn_err( CE_CONT, "queue busy for pass thru\n");
#endif
		return( SDI_RET_RETRY );
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;
	sony_putq( q, sp );
	sony_next( q );
	splx( oip );
	return( SDI_RET_OK  );
}

/*******************************************************************************
 *
 *	sonyicmd( struct hbadata *hbap )
 *
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 *
 *	Entry :
 *		*hbap		ptr to the command
 *
 *	Exit :
 *		SDI_RET_OK	cool
 *		SDI_RET_ERR	something went wrong
 *
 *	Notes :
 *
 ******************************************************************************/

/*ARGSUSED*/
long
HBAICMD( struct hbadata *hbap, int sleepflag )
{
	int			c;		/* controller */
	int			t;		/* target */
	int			l;		/* lun */
	sblk_t			*sp =(sblk_t *) hbap;
	struct scsi_ad		*sa;		/* scsi address */
	struct scs		*inq_cdb;
	scsi_lu_t		*q;
	pl_t			oip;

#ifdef NOTDEF  /* ??? fix */
	cmn_err(CE_NOTE, "sonyicmd entered sb %x",
		&sp->sbp->sb);
#endif

	oip  = spldisk();

	switch( sp -> sbp -> sb.sb_type ) {
	case SFB_TYPE :
		/*
		 *	special function type 
		 */
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = sony_gtol [ SDI_HAN( sa ) ];
		t = SDI_TCN( sa );
		l = SDI_LUN( sa );
		q = &SONY_LU_Q(c,t,l);
		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch( sp->sbp->sb.SFB.sf_func ) {
		case SFB_RESUME :
			/*
			 *	start this guy back up again
			 */
			q->q_flag &=~QSUSP;
			sony_next( q );
			break;
		case SFB_SUSPEND :
			/*
			 *	suspend this guy
			 */
			q->q_flag |= QSUSP;
			break;
		case SFB_RESETM :
			/*FALLTHRU*/
		case SFB_ABORTM :
			/*
			 *	abort a job 
			 */
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			sony_putq( q, sp);
			sony_next( q );
			splx( oip );
			return( SDI_RET_OK );
		case SFB_FLUSHR :
			/*
			 *	flush a queue
			 */
			sony_flushq( q, SDI_QFLUSH );
			break;
		case SFB_NOPF :
			/*
			 *	a nothingness instruction
			 */
			break;
		default :
			/*
			 *	don't know this one
			 */
			sp->sbp->sb.SFB.sf_comp_code =(ulong_t) SDI_SFBERR;
		} /* switch( sp->sbp->sb.SFB.sf_func ) */

		sdi_callback( (struct sb *)sp->sbp );
		splx( oip );
		return( SDI_RET_OK  );

	case ISCB_TYPE :
		/*
		 *	scsi command block type
		 */
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = sony_gtol [ SDI_HAN( sa ) ];
		t = SDI_TCN( sa );
		l = SDI_LUN( sa );
		q = &SONY_LU_Q(c, t, l );

		inq_cdb =( struct scs *)(void *) sp->sbp->sb.SCB.sc_cmdpt;
		if (( t == sony_sc_ha[c].ha_id ) &&(l == 0) &&(inq_cdb->ss_op == SS_INQUIR) ) {
			struct ident inq;       /* tmp ident buf        */
                        struct ident *inq_data; /* To buf               */
                        int inq_len;            /* To buf length        */


			/*	
			 *	controller id is being requested 
			 */
			inq_data =( struct ident * )(void *) sp->sbp->sb.SCB.sc_datapt;
			inq_len = sp->sbp->sb.SCB.sc_datasz;

			bzero(&inq, sizeof(struct ident));
                        inq.id_type = ID_PROCESOR;
                        strncpy(inq.id_vendor, sonyidata[c].name,
                        	VID_LEN+PID_LEN+REV_LEN);
                        bcopy((char *)&inq, (char *)inq_data, inq_len);

			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			splx( oip );
			sdi_callback(&sp->sbp->sb);
			return( SDI_RET_OK );
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		sony_putq( q, sp );
		sony_next( q );
		splx( oip );
		return( SDI_RET_OK  );

	default :
		/*
		 *	some random type -- you're outta here
		 */
		splx( oip );
		sdi_callback(&sp->sbp->sb);
		return( SDI_RET_ERR  );
	} /* switch( sp->sbp->sb.sb_type ) */
}


/*******************************************************************************
 *******************************************************************************
 *
 *	QUEUE Management
 *
 *	Routines: 
 *		sony_putq			put an entry on the queue
 *		sony_next			send next entry to the ctrlr
 *		sony_flushq			flush a queue
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	sony_putq( scsi_lu_t *q, sblk_t *sp )
 *
 *	Put a job on a logical queue 
 *
 *	Entry :
 *		*q		ptr to lun queue
 *		*sp		ptr to scsi job block
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *		- jobs are prioritized
 *
 *************/

void
sony_putq( scsi_lu_t *q, sblk_t *sp )
{
	int			cls = SONY_QUECLASS( sp );
	sblk_t			*nsp;
	pl_t			ospl;

	ospl = spldisk();

	/*
	 *	insert job on the queue
	 */

	if ( !q->q_first ||(cls <= SONY_QUECLASS( q->q_last ) ) ) {
		/*
		 *	if the queue is empty OR the queue class of the new
		 *	job is LESS than that of the last on on the queue then
		 *	insert onto the END of the queue
		 */
		if ( q->q_first ) {	/* something on the queue */
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		}
		else {			/* nothing on the queue */
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;
	}
	else {
		/*
		 *	sort it into the list of jobs on the queue and insert
		 */
		nsp = q->q_first;
		while ( SONY_QUECLASS( nsp ) >= cls )
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if ( nsp->s_prev )
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}
	q->q_depth ++;		/* its getting deeper */
	splx( ospl );
}

/*******************************************************************************
 *
 *	sony_next( scsi_lu_t *q ) 
 *
 *	Send the next job on the logical queue
 *
 *	Entry :
 *		*q		ptr to queue
 *
 *	Exit :
 *		Nothing 
 *
 *	Notes :
 *		- if QBUSY then don't send any jobs
 *
 ******************************************************************************/

void
sony_next( scsi_lu_t *q )
{
	sblk_t			*sp;
	pl_t			ospl;

	ospl = spldisk();


	if ( q->q_flag & QBUSY ) {
		/*	
		 *	queue is "busy" -- don't execute more jobs
		 */
		splx( ospl );
		return;
	}

	if (( sp = q->q_first ) == NULL ) {
		/*
		 *	queue is empty -- reset flags and leave
		 */
		q->q_depth = 0;
		splx( ospl );
		return;
	}

	if ( q->q_flag & QSUSP && sp->sbp->sb.sb_type == SCB_TYPE ) {
		/*
		 *	queue is suspended
		 */
		splx( ospl );
		return ;
	}


	/*
	 *	now check a whole bunch of things for special job
	 *	processing and such
	 */


	if ( sp->sbp->sb.sb_type == SFB_TYPE ) {
		/*
		 *	special function type
		 */
		if ( !(q->q_first = sp->s_next ) )
			q->q_last = NULL;
		/* there is a bug her. Another else ? ask blitz */

		q->q_depth --;
		splx( ospl );
		sony_func( sp );
	}
	else {
		/*	
		 *	regular function type 	
		 */

		if ( !(q->q_first = sp->s_next ) )
			q->q_last = NULL;

		q->q_depth --;
		splx( ospl );
		sony_cmd( sp );
	}
}

/*******************************************************************************
 *
 *	sony_flushq( scsi_lu_t *q, int cc )
 *
 *	Empty a logical unit queue
 *
 *	Entry :
 *		*q		ptr to logical queue
 *		cc		completion code
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *		- *cc* is the job completion code for the flushed job
 *
 ******************************************************************************/

void
sony_flushq( scsi_lu_t *q, int cc )
{
	sblk_t			*sp;
	sblk_t			*nsp;
	pl_t			ospl;

	ASSERT( q == NULL );

	ospl = spldisk();

	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;
	q->q_depth = 0;
	q->q_flag = 0;

	while ( sp ) {
		nsp = sp->s_next;
		sp->sbp->sb.SCB.sc_comp_code =(ulong_t) cc;
		sdi_callback( (struct sb *)sp->sbp );
		sp = nsp;
	}

	splx( ospl );
}

/*******************************************************************************
 *******************************************************************************
 *
 *	HBA Information Requests
 *
 *	Routines:
 *		sonygetinfo			return info about adapter
 *		sony_illegal			check for illegal requests
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	sony_illegal( ushort_t c, uchar_t t, uchar_t l, int m )
 *
 *	Check to see if there is an EDT entry for this scsi device
 *
 *	Entry :
 *		c		controller number
 *		t		target number
 *		l		logical unit number
 *		m		major number( not used )
 *
 *	Exit :
 *		0		cool
 *		1		no entry
 *
 ******************************************************************************/

/*ARGSUSED*/
int
sony_illegal( ushort_t c, uchar_t t, uchar_t l, int m )
{
	if ( sdi_redt( c, t, l ) ) {
		/*
	 	 *	entry exists
		 */
		return( 0 );
	}

	/*
	 *	no entry existed
	 */

	return( 1 );
}
/*******************************************************************************
 *
 *	sonygetinfo( struct scsi_ad *sa, struct hbagetinfo *getinfo )
 *
 *	Fill in the structures with information about this SCSI adapter
 *
 *	Entry :
 *		*sa		ptr to scsi adapter address struct
 *		*getinfo	structure to fill in
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
HBAGETINFO( struct scsi_ad *sa, struct hbagetinfo *getinfo )
{
	char			*s1, *s2;
	static char		temp [] = "HA X TC X";

	s1 = temp;
	s2 = getinfo -> name;
	temp [ 3 ] = SDI_HAN( sa ) + '0';
	temp [ 8 ] = SDI_TCN( sa ) + '0';

	while (( *s2++ = *s1++ ) != '\0' ) ;

	getinfo->iotype = F_DMA_24;

#if PDI_VERSION >= PDI_SVR42MP
	if (getinfo->bcbp) {

		getinfo->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfo->bcbp->bcb_max_xfer = sonyhba_info.max_xfer;
		getinfo->bcbp->bcb_flags = BCB_PHYSCONTIG;
		getinfo->bcbp->bcb_physreqp->phys_align = 2;
		/* ?? should probably be zero */
		getinfo->bcbp->bcb_physreqp->phys_boundary = 128 * 1024;
		getinfo->bcbp->bcb_physreqp->phys_dmasize = 24;
	}
#endif
}

/*******************************************************************************
 *******************************************************************************
 *
 *	SCSI Pass-Thru Support
 *
 *	Routines:
 *		sonyopen				open the device for passthru
 *		sonyclose			close the passthru device
 *		sonyioctl			special ioctls for passthru
 *
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *
 *	sonyopen( dev_t *devp, int flags, int otype, cred_t *crp )
 *
 *	Pass thru open 
 *
 *	Entry : 
 *		*devp		ptr to device numbers
 *		flags		flags for opening
 *		otype		type of open
 *		*crp 		ptr to user's credentials
 *
 *	Exit :
 *		0		cool
 *		EBUSY		device busy
 *		EPERM		not allowed to do this
*/

/*ARGSUSED*/
int
HBAOPEN( dev_t *devp, int flags, int otype, cred_t *crp )
{
	dev_t			dev = *devp;
	int			c = sony_gtol [ SC_HAN(dev) ];
	int			t = SC_TCN( dev );
	int			l = SC_LUN( dev );
	scsi_lu_t		*q;
	int			oip;

	if ( drv_priv( crp ) ) {
		/*
		 *	not allowed to do this
		 */
		return( EPERM );
	}

	if (sony_illegal(SC_HAN(dev), t, l, 0)) {
		return(ENXIO);
	}
	if ( t == sony_sc_ha[c] . ha_id ) {
		/*
		 *	opening the controller
		 */
		return( 0 );
	}

	/*
	 *	open for pass-thru
	 */

	q = &SONY_LU_Q( c, t, l );

	oip = spldisk();
	if (( q->q_count > 0 ) ||( q->q_flag &(QBUSY | QSUSP | QPTHRU ) ) ) {
		/*
		 *	sorry -- in use
		 */
		splx( oip );
		return( EBUSY );
	}

	/*
	 *	give it to him
	 */

	q->q_flag |= QPTHRU;
	splx( oip );
	return( 0 );
}

/*******************************************************************************
 *
 *	sonyclose( dev_t dev, int flags, int otype, cred_t *crp )
 *
 *	Close connection to board/lu
 *
 *	Entry :
 *		dev		device numbers
 *		flags		flags from open
 *		otype		type of open
 *		*crp		ptr to user's credentials
 *
 *	Exit :
 *		0		cool
 *		ENXIO 		not on this guy you don't
 *
 *	Notes :
 *
 ******************************************************************************/

/*ARGSUSED*/
int
HBACLOSE( dev_t dev, int flags, int otype, cred_t *crp )
{
	int			c = sony_gtol [ SC_HAN(dev) ];
	int			t = SC_TCN( dev );
	int			l = SC_LUN( dev );
	scsi_lu_t		*q;
	pl_t			oip;

	if ( t == sony_sc_ha[c].ha_id ) {
		/*
		 *	feel free to close the scsi controller
	 	 */
		return( 0 );
	}

	/*
 	 *	deal with a pass-thru
	 */

	q = &SONY_LU_Q( c, t, l );

	oip = spldisk();
	q->q_flag &= ~QPTHRU;

	if ( q->q_func != NULL ) {
		/*	
		 *	we need to tell the guy about it
		 */
		(*q->q_func)( q->q_param, SDI_FLT_PTHRU );
	}

	sony_next( q );
	splx( oip );
	return( 0 );
}

/*******************************************************************************
 *
 *	sonyioctl( dev_t devp, int cmd, int arg, int mode, cred_t *crp,int *rval_p )
 *
 *	IOCTLs for pass-thru
 *	
 *	Entry :
 *		devp		device numbers
 *		cmd		command to do
 *		arg		passes params
 *		mode		values from open
 *		*crp		ptr to user's credentials
 *		*rval_p		ptr to return value
 *
 *	Exit :
 *		0		cool
 *		EINVAL		invalid command
 *
 *	Notes :
 *
 ******************************************************************************/

/*ARGSUSED*/
int
HBAIOCTL( dev_t devp, int cmd, caddr_t arg, int mode, cred_t *crp, int *rval_p )
{
	register int	c = sony_gtol[SC_HAN(devp)];
	register int	t = SC_TCN(devp);
	register struct sb *sp;
	int  uerror = 0;
	static	char	sc_cmd [ MAX_CMDSZ ];	/* table for passthru */

	switch(cmd) {
	case SDI_SEND: 
		{
			register buf_t *bp;
			struct sb  karg;
			int  rw;
			char *save_priv;

			if (t == sony_sc_ha[c].ha_id) { 	/* illegal ID */
				return(ENXIO);
			}
			if (copyin(arg,(caddr_t)&karg, sizeof(struct sb))) {
				return(EFAULT);
			}
			if ((karg.sb_type != ISCB_TYPE) ||
			    (karg.SCB.sc_cmdsz <= 0 )   ||
			    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) {
				return(EINVAL);
			}

			sp = SDI_GETBLK( KM_SLEEP );
			save_priv = sp->SCB.sc_priv;
			bcopy((caddr_t)&karg,(caddr_t)sp, sizeof(struct sb));

			bp = getrbuf(KM_SLEEP);
			bp->b_iodone = NULL;
			sp->SCB.sc_priv = save_priv;
			sp->SCB.sc_cmdpt = sc_cmd;

			if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
			    sp->SCB.sc_cmdsz)) {
				uerror = EFAULT;
				goto done;
			}

			rw =(sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
			bp->b_private =(char *)sp;

			/*
			 * If the job involves a data transfer then the
			 * request is done thru physiock() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then sony_pass_thru()
			 * is called directly.
			 */
			if (sp->SCB.sc_datasz > 0) {
				struct iovec  ha_iov;
				struct uio    ha_uio;

				ha_iov.iov_base = sp->SCB.sc_datapt;
				ha_iov.iov_len = sp->SCB.sc_datasz;
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				ha_uio.uio_offset = 0;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;

				if (uerror = physiock(sony_pass_thru, bp, devp, rw, 
				    HBA_MAX_PHYSIOCK, &ha_uio)) {
					goto done;
				}
			} else {
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_edev = devp;
				bp->b_flags |= rw;

				sony_pass_thru(bp);  /* fake physio call */
				biowait(bp);
			}

			/* update user SCB fields */

			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;

			if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
				uerror = EFAULT;

done:
			freerbuf(bp);
			sdi_freeblk(sp);
			break;
		}

	case B_GETTYPE:
		if (copyout("scsi",((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}
		if (copyout("scsi",((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if (copyout((caddr_t)&sony_sdiver, arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	default:
		return(EINVAL);
	}
	return(uerror);
}



void
sony_pass_thru( struct buf *bp )
{
	int	c = sony_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	register struct scsi_lu	*q;
	register struct sb *sp;
	int  oip;

	sp =(struct sb *) (void *) bp->b_private;

	sp->SCB.sc_dev.sa_lun = (unsigned char)l;
	sp->SCB.sc_dev.sa_fill =(sony_ltog[c] << 3) | t;
	sp->SCB.sc_wd =(long)bp;
	sp->SCB.sc_datapt =(caddr_t) paddr(bp);
	sp->SCB.sc_int = sony_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP );

	q = &SONY_LU_Q(c, t, l);

	sdi_icmd(sp, KM_SLEEP);
}




/*******************************************************************************
 *
 *	sony_int( struct sb *sp )
 *
 *	Interrupt handler for pass-thru jobs -- it just comes
 *	around and wakes up the sleeping process
 *
 *	Entry :
 *		*sp		ptr to scsi block
 *	
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 ******************************************************************************/

void
sony_int( struct sb *sp )
{
	struct buf		*bp;

	bp =(struct buf *) sp->SCB.sc_wd;
	biodone( bp );

}
#if PDI_VERSION >= PDI_UNIXWARE20
/*
 * int
 * sony_verify(rm_key_t key)
 *
 * Description:
 *	Verify the board instance.
 */
int
sonyverify(rm_key_t key)
{
	HBA_IDATA_STRUCT	vfy_idata;
	int rv;
	int iobase;
	int j;
	scsi_ha_t	*ha;	/* random ha ptr */

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
	 * Go through sony31averify() and sony535verify()
	 * to determine if a board is present.
	 */
	for (j=0; sonyvectorlist[j]; j++) {
		if ((*sonyvectorlist[j]->sonyverify)(iobase) != -1) {
			/*
			 * A board was found.
			 */
			return(0);
		}
	}

	/*
	 * Didn't find a board.
	 */
	return(ENODEV);
}
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

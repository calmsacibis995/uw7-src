#ident	"@(#)kern-pdi:io/hba/ictha/ictha.c	1.39.2.1"
#ident	"$Header$"

#ifdef _KERNEL_HEADERS

#include <util/sysmacros.h>
#include <fs/buf.h>
#include <util/cmn_err.h>
#include <io/vtoc.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/scsi.h>
#include <io/i8237A.h>
#if (PDI_VERSION <= 1)
#include <io/hba/ictha.h>
#include <io/target/dynstructs.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/hba/ictha/ictha.h>
#include <io/target/sdi/dynstructs.h>
#endif /* !(PDI_VERSION <= 1) */
#include <io/hba/hba.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <io/dma.h>
#ifdef PDI_SVR42
#include <svc/sysenvmt.h>
#endif

#include <util/mod/moddefs.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#else /* !_KERNEL_HEADERS */

#include <sys/sysmacros.h>
#include <sys/buf.h>
#include <sys/cmn_err.h>
#include <sys/vtoc.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/i8237A.h>
#if (PDI_VERSION <= 1)
#include <sys/ictha.h>
#include <sys/dynstructs.h>
#else /* !(PDI_VERSION <= 1) */
#include <sys/ictha.h>
#include <sys/dynstructs.h>
#endif /* !(PDI_VERSION <= 1) */
#include <sys/hba.h>
#include <sys/kmem.h>
#include <sys/errno.h>
#include <sys/dma.h>
#define STATIC	
#ifdef PDI_SVR42
#include <sys/sysenvmt.h>
#endif

#include <sys/moddefs.h>

/* These must come last: */
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif /* !_KERNEL_HEADERS */

extern int  timeout();
void 	    ictha_timeout();
static int  ictha_init_time;

int 	ictha_rdstatus(), ictha_chk_excpt(),
	ictha_docmd(), ictha_rdstat_rdy(), ictha_setdma(), 
	ictha_excpt_clr();

int	wait_til_ready(), wait_til_not_ready();

void 	ictha_status_upd(), ictha_clrstatus(), ictha_clrflags(), 
	ictha_gen_intr();

static void ictha_nicreset();

#if (PDI_VERSION >= PDI_UNIXWARE20)
int	ictha_mca_conf(), ictha_verify();
#endif

/**
** NOTE:
**
** Entry 7 in the ict_ex array used to be:
**
** {ICT_BBX, 0x84, 0x00, 0xef, 0xff, 13, "Read Error, Bad Block Xfer"},
**
** it was changed to:
**
** {ICT_EOM, 0x84, 0x00, 0xef, 0xff, 13, "End of Media"},
**
** via MR ul94-25502. The change was required because some Wangtek
** drive/controller combinations report EOM detection with status
** byte 0 set to 0x84 rather than the specified 0x88. In particular
** Controller ASSY 30850 Firmware F29G and Drive ASSY 30337-102 REV.B
** report status byte 0 as 0x84 for EOM detection.
**
** A result of this change is that User Level applications will think
** they've encountered EOM if the drive tries to report a bad block
** on read. We're no worse off since in either case, we can't get past
** the bad block.
***/

struct ictha_ex {
	int	type; 	/* type of exception */
	int	byte0;  /* byte0, byte1 are two of the six bytes status */
	int	byte1;
	int	mask0;
	int	mask1;
	int	drverror;
	char	*errtext;
} ictha_ex[] = {
	{ICTHA_NCT, 0xc0, 0x00, 0xef, 0x00, 10, "No Cartridge"},
	{ICTHA_EOM, 0x88, 0x00, 0xff, 0x00, 26, "End of Media"},
	{ICTHA_EOF, 0x81, 0x00, 0xe7, 0x00, 26, "Read a Filemark"},
	{ICTHA_WRP, 0x90, 0x00, 0xff, 0x77,  9, "Write Protected"},
	{ICTHA_DFF, 0x20, 0x00, 0xff, 0xff, 13, "Device Fault Flag"},
	{ICTHA_RWA, 0x84, 0x88, 0xef, 0xff, 13, "Read or Write Abort"},
	{ICTHA_EOM, 0x84, 0x00, 0xef, 0xff, 13, "End of Media"},
	{ICTHA_FBX, 0x86, 0x00, 0xef, 0xff, 13, "Read Error, Filler Block Xfer"},
	{ICTHA_NDT, 0x86, 0xa0, 0xef, 0xff, 13, "Read Error, No Data"},
	{ICTHA_NDE, 0x8e, 0xa0, 0xef, 0xff, 13, "Read Error, No Data & EOM"},
	{ICTHA_ILC, 0x00, 0xc0, 0x0f, 0xf7, 16, "Illegal Command"},
	{ICTHA_PRR, 0x00, 0x81, 0x0f, 0xf7,  0, "Power On/Reset"},
	{ICTHA_MBD, 0x81, 0x10, 0xef, 0xff, 13, "Marginal Block Detected"},
	{ICTHA_UND, 0x00, 0x00, 0x00, 0x00, 25, "Undetermined Error"}
};

#define NEXCPTS	(sizeof(ictha_ex) / sizeof(struct ictha_ex))

STATIC	int	ictha_hba2ctrl[MAX_HAS];
STATIC	int	ictha_hacnt;
STATIC	ulong	ictha_pagesize;
STATIC	ulong	ictha_pagemask;

extern	int	ictha_cntls;
struct  ictha_ctrl      	*ictha_ctrl;

#if (PDI_VERSION >= PDI_UNIXWARE20)
extern	HBA_IDATA_STRUCT       _icthaidata[];
/*
 * The name of this pointer is the same as that of the
 * original idata array. Once it it is assigned the
 * address of the new array, it can be referenced as
 * before and the code need not change.
 */
HBA_IDATA_STRUCT       *icthaidata;
#else
extern HBA_IDATA_STRUCT       icthaidata[];
#endif

extern 	ulong	ictha_rdwr_duration;
extern 	ulong	ictha_cmds_duration;
extern 	ulong	ictha_retention_duration;
extern 	ulong	ictha_rewind_duration;
extern 	ulong	ictha_erase_duration;
extern 	ulong	ictha_space_duration;
extern 	ulong	ictha_wr_maxout;
extern	ulong	ictha_init_wait_limit;
extern	ulong	ictha_wait_poll_limit;
extern  ulong 	ictha_waitcnt;

extern 	long	ictha_blocksize;

int	ictha_devflag = 0;
char	*ictha_ctrl_name;

#ifdef ICTHA_DEBUG
int	ictha_debug_flag = 0;
#endif
ulong	ictha_wait_limit;
int	ictha_sleepflag = KM_NOSLEEP;

struct  dma_cb  *ictha_cb;

void	ictha_cmds_watchdog();
void	ictha_rdwr_watchdog();

#define	DRVNAME "ICTHA TAPE"

#if (PDI_VERSION >= PDI_UNIXWARE20)
MOD_ACHDRV_WRAPPER(ictha, ictha_load, ictha_unload, NULL,  ictha_verify, DRVNAME);
#else
MOD_HDRV_WRAPPER(ictha, ictha_load, ictha_unload, NULL, DRVNAME);
#endif
/*
HBA_INFO(ictha, &ictha_devflag, ICTHA_MAX_XFER);
*/

STATIC  void		ictha_chk_endjob();
STATIC  void		ictha_rdwr_cmd();
STATIC  void		ictha_scb_cmd();
STATIC  void		ictha_read_ahead();
STATIC	void 		ictha_inquir();
STATIC	int		ictha_wait_int();

STATIC	int		In_load = 0;

#define icthaicmd icthasend

HBA_INFO(ictha, &ictha_devflag, ICTHA_MAX_XFER);

/*
 * STATIC int
 * ictha_load(void) 
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
ictha_load(void)
{
	ictha_sleepflag = KM_SLEEP;
#if (PDI_VERSION < PDI_UNIXWARE20)
	mod_drvattach( &ictha_attach_info );
#endif
	if( icthastart()) {
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
#if (PDI_VERSION >= PDI_UNIXWARE20)
		sdi_acfree(icthaidata, ictha_cntls);
#else
		mod_drvdetach( &ictha_attach_info );
#endif

		return( ENODEV );
	}
	return(0);
}

/*
 * STATIC int
 * ictha_unload(void)
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
ictha_unload(void)
{
	return(EBUSY);
}

#define ICTHA_CALLBACK(sb, status)\
	{\
		sb->SCB.sc_comp_code = (unsigned long) status; \
		sdi_callback(sb);\
	}

#define ICTHA_SDI_RETURN(driveptr, status, sleepflag) { \
		if(driveptr->drv_flags&CFLG_CMD) {\
			struct	sb *tmpsb = driveptr->drv_req.req_sb; \
			driveptr->drv_req.req_sb = NULL; \
			driveptr->drv_flags &= ~CFLG_CMD; \
		   	tmpsb->SCB.sc_comp_code = (unsigned long) status; \
			sdi_callback(tmpsb); \
		} \
		driveptr->drv_busy = B_FALSE; \
		if(driveptr->drv_headque) { \
			struct ictha_xsb *hbap = driveptr->drv_headque;\
			driveptr->drv_headque = driveptr->drv_headque->next;\
			HBASEND((struct hbadata *)hbap,sleepflag); \
		} \
}

#define	ICTHA_ASSERT(Y, X )	{  \
	Y->ictha_cntrl_mask |= ( X ); \
	outb( Y->ictha_control, Y->ictha_cntrl_mask ); \
}

#define	ICTHA_DEASSERT(Y, X )	{ \
	Y->ictha_cntrl_mask &= (unchar)~(X); \
	outb(Y->ictha_control, Y->ictha_cntrl_mask ); \
}
#define	ICTHA_CHK_ASSERT(Y, X )	{  \
	if((Y->ictha_cntrl_mask&(X)) == 0){ \
		Y->ictha_cntrl_mask |= ( X ); \
		outb( Y->ictha_control, Y->ictha_cntrl_mask ); \
	}\
}

#define	ICTHA_CHK_DEASSERT(Y, X )	{ \
	if((Y->ictha_cntrl_mask&(X)) != 0){ \
		Y->ictha_cntrl_mask &= (unchar)~(X); \
		outb(Y->ictha_control, Y->ictha_cntrl_mask ); \
	}\
}

/*
 * icthastart()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Board/drive(s) initialization.
 */

int
icthastart()
{
	register CtrlPtr ctrlptr;
	register DrivePtr driveptr;
	register int	c, l;
	int	cntl_num;

#if (PDI_VERSION >= PDI_UNIXWARE20)
	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	icthaidata = sdi_hba_autoconf("ictha", _icthaidata, &ictha_cntls);
	if(icthaidata == NULL)	{
		return(-1);
	}
	sdi_mca_conf(icthaidata, ictha_cntls, ictha_mca_conf);
#endif

	HBA_IDATA(ictha_cntls);

	ictha_ctrl = (struct ictha_ctrl *)
		     kmem_zalloc(ictha_cntls * sizeof(struct ictha_ctrl),
				 ictha_sleepflag);
	if(ictha_ctrl == NULL)	{
		cmn_err(CE_WARN, "icthastart(): could not allocate ictha_ctrl");
		return(-1);
	}

	ictha_hacnt = 0;
	ictha_init_time = 1;
	ictha_wait_limit = ictha_init_wait_limit;
	ictha_pagesize = (ulong) ptob(1);
	ictha_pagemask = ictha_pagesize - 1;

	for( c=0; c < ictha_cntls; c++) {
		ctrlptr = &ictha_ctrl[c];

		if(ictha_bdinit(ctrlptr, &icthaidata[c]) == 0) {
			continue;
		}

		driveptr = ctrlptr->ictha_drives;
		for( l=0; l < MAX_ICTHA_DRIVES; l++, driveptr++){
			ictha_drvinit(ctrlptr, driveptr);
		}
		icthaidata[c].active = 1;

		/* Get an HBA number from SDI and Register HBA with SDI */
		if( (cntl_num = sdi_gethbano( icthaidata[c].cntlr )) <= -1) {
			/*
			 *+ No HBA number available from SDI
			 */
			cmn_err (CE_CONT,"%s: No HBA number available.\n", 
			    icthaidata[c].name);
			icthaidata[c].active = 0;
			continue;
		}

		icthaidata[c].cntlr = cntl_num;
		ictha_hba2ctrl[cntl_num] = c;

		cntl_num = sdi_register(&icthahba_info, &icthaidata[c]);
		if(cntl_num < 0) {
			/*
			*+ SDI registry failure.
			*/
			cmn_err (CE_CONT,"!%s:HA %d, SDI registry failure %d.\n",
			    icthaidata[c].name, c, cntl_num);
			icthaidata[c].active = 0;
			continue;
		}
		ctrlptr->ictha_dmachan = icthaidata[c].dmachan1;
		ictha_hacnt++;
	}
	if(ictha_hacnt == 0) {
#ifdef ICTHA_DEBUG
		cmn_err(CE_CONT,"ICTHA: No tape devices found.\n");
#endif
		return (1);
	}
	else {
		ictha_init_time = 0;
		ictha_wait_limit = ictha_wait_poll_limit;

		if ((ictha_cb = dma_get_cb(ictha_sleepflag)) == NULL) {
#ifdef ICTHA_DEBUG
			cmn_err(CE_CONT,"ICTHA: dma_get_cb() failed\n");
#endif
			return(1);
		}
		if ((ictha_cb->targbufs = dma_get_buf(ictha_sleepflag)) == NULL) {
#ifdef ICTHA_DEBUG
			cmn_err(CE_CONT,"ICTHA: dma_get_buf() failed\n");
#endif
			dma_free_cb(ictha_cb);
			return(1);
		}
		ictha_cb->targ_step = DMA_STEP_HOLD;
		ictha_cb->targ_path = DMA_PATH_8;
		ictha_cb->trans_type = DMA_TRANS_SNGL;
		ictha_cb->targ_type = DMA_TYPE_IO;
		ictha_cb->bufprocess = DMA_BUF_SNGL;

#if (PDI_VERSION >= PDI_UNIXWARE20)
		sdi_intr_attach(icthaidata, ictha_cntls, icthaintr, ictha_devflag);
#endif
		return(0);
	}
}

/*
 * ictha_select(CtrlPtr, unchar)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Try to also select drive 0 to make sure this not an ethernet
 * 	controller.
 */

int
ictha_select(ctrlptr, selcmd)
CtrlPtr ctrlptr;
unchar selcmd;
{
	int	l;

	if( wait_til_ready(ctrlptr, ictha_wait_limit, 9 ) == ICTHA_FAILURE ) {
#ifdef ICTHA_DEBUG
		cmn_err(CE_CONT, "1st ready fail");
#endif 
		return( ICTHA_FAILURE );
	}
	if( ctrlptr->ictha_type == ICTHA_WANGTEK ) {
		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_online );
	}
	outb( ctrlptr->ictha_command, selcmd);

	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_request );
	if( wait_til_ready(ctrlptr, ictha_wait_limit, 10 ) == ICTHA_FAILURE ) {
#ifdef ICTHA_DEBUG
		cmn_err(CE_CONT, "2nd ready fail");
#endif 
		return( ICTHA_FAILURE );
	}
	ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );
	if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
#ifdef ICTHA_DEBUG
		cmn_err(CE_CONT, "not ready fail");
#endif 
		return( ICTHA_FAILURE );
	}
	for(l=0; l< ictha_wait_limit; l++) {
		ctrlptr->ictha_cntr_status = inb( ctrlptr->ictha_status );
		if((ctrlptr->ictha_cntr_status & ctrlptr->ictha_exception) == 0 &&
		   ictha_chk_excpt(ctrlptr, &ctrlptr->ictha_drives[0], 1) == ICTHA_EXCEPTION) {
#ifdef ICTHA_DEBUG
			cmn_err(CE_CONT, "exception !!");
#endif 

			return( ICTHA_FAILURE );
		}
		if((ctrlptr->ictha_cntr_status & ctrlptr->ictha_ready) == 0)
			return( ICTHA_SUCCESS );
		drv_usecwait( 10 );
	}
#ifdef ICTHA_DEBUG
	cmn_err(CE_CONT, "timeout !!");
#endif 
	return( ICTHA_FAILURE );
}

int
ictha_chkstatus(ctrlptr) 
CtrlPtr	ctrlptr;
{
	int	tries = 0;
	unchar	*statbuf = ctrlptr->ictha_status_buf;
	int	i;

#define	BNL	(1<<1)
#define	UDA	(1<<2)
#define	DFF	(1<<5)

#define	MBD	(1<<4)
#define	NDT	(1<<5)
#define	ILL	(1<<6)

	while (ictha_rdstatus(ctrlptr) == ICTHA_SUCCESS && tries < 2)
	{
#ifdef ICTHA_DEBUG
		for(i=0; i< 6; i++)
			cmn_err(CE_NOTE, "ictha_status[%d] = %x", 
						i, statbuf[i]);
#endif
		if((statbuf[1]&(MBD|NDT|ILL|ICTHA_POR|ICTHA_SBYTE1)) ==
		    	(ICTHA_POR|ICTHA_SBYTE1) &&
		   (statbuf[0]&(BNL|UDA|DFF)) == 0 &&
		   statbuf[2] == 0 && statbuf[3] == 0 &&
		   statbuf[4] == 0 && statbuf[5] == 0)
				return(ICTHA_SUCCESS);
		tries++;		  
	}
#ifdef ICTHA_DEBUG
	cmn_err(CE_NOTE, "ictha_chkstatus return failure");
#endif
	return(ICTHA_FAILURE);
}

/*
 * ictha_nicreset()
 *
 * Calling/Exit State:
 *
 * Description:
 */
static void
ictha_nicreset()
{
	int i;
	unchar hold;
	static int on_bflop = -1;

	if (!In_load)
		return;
	if (on_bflop < 0) {
		struct bootdev bdev;

		drv_gethardware(BOOT_DEV, &bdev);
		on_bflop = (bdev.bdv_type == BOOT_FLOPPY);
	}
	if (!on_bflop)
		return;
	for (i = 0; i < 10; i++)
		hold = inb(0x31F);
	outb(0x31F, hold);
}

/*
 * ictha_bdinit()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Board initialization.
 */
/* ARGSUSED */
ictha_bdinit(ctrlptr, idataptr)
CtrlPtr	ctrlptr;
HBA_IDATA_STRUCT *idataptr;
{
	int	contr_type;

	In_load = (idataptr->ioaddr1 == 0x300);

	/*
	 * Initialize the non-controller specific of ctrlptr
	 * structure fields.
	 */

	ctrlptr->ictha_cntrl_mask = 0;

	/*
	 * Common Control Port Bit Masks 
	 */

	ctrlptr->ictha_online	= ICTHA_ONLINE;
	ctrlptr->ictha_dma1_2	= ICTHA_DMA1_2;

	/*
	** The following entries are specific to the
	** supported controllers, and will be given
	** appropriate values once the type of controller
	** has been determined.
	**/

	ctrlptr->ictha_type = ICTHA_WANGTEK; 
	for( contr_type = 0; contr_type < 2; ++contr_type ) {
		if( contr_type == ICTHA_WANGTEK ) {
			uint	bus_p;
			/*
			** Port Addresses 
			*/
			ctrlptr->ictha_status = idataptr->ioaddr1;
			ctrlptr->ictha_control = idataptr->ioaddr1;
			ctrlptr->ictha_command = idataptr->ioaddr1 + 1;
			ctrlptr->ictha_data = idataptr->ioaddr1 + 1;

			ctrlptr->ictha_reset		= WANGTEK_RESET;
			ctrlptr->ictha_request		= WANGTEK_REQUEST;
			ctrlptr->ictha_intr_enable 	= WANGTEK_INTENAB;
			ctrlptr->ictha_ready		= WANGTEK_READY;
			ctrlptr->ictha_exception 	= WANGTEK_EXCEPTION;

			ctrlptr->ictha_dma_go	= 0;
			ctrlptr->ictha_reset_dma	= 0;

			/*
			** Power On Reset Delay 
			*/
			ctrlptr->ictha_por_delay	= 1000000;

			ictha_ctrl_name = "Wangtek QIC";

			if (!drv_gethardware(IOBUS_TYPE, &bus_p) && 
			    (bus_p == BUS_MCA)){
				ictha_ctrl_name = "MCA Archive QIC";
				ctrlptr->ictha_intr_enable = ARCHIVE_MCA_INTENAB;
				ctrlptr->ictha_dma_enable = ARCHIVE_MCA_DMAENAB;
				contr_type = ICTHA_MCA_ARCHIVE;
			}
		}
		else if( contr_type == ICTHA_ARCHIVE ) {
			ictha_ctrl_name = "Archive QIC";
			/*
			** Port Addresses 
			*/
			ctrlptr->ictha_status = idataptr->ioaddr1 + 1;
			ctrlptr->ictha_control = idataptr->ioaddr1 + 1;
			ctrlptr->ictha_command = idataptr->ioaddr1;
			ctrlptr->ictha_data = idataptr->ioaddr1;

			ctrlptr->ictha_reset		= ARCHIVE_RESET;
			ctrlptr->ictha_request 		= ARCHIVE_REQUEST;
			ctrlptr->ictha_intr_enable 	= ARCHIVE_INTENAB;
			ctrlptr->ictha_ready		= ARCHIVE_READY;
			ctrlptr->ictha_exception	= ARCHIVE_EXCEPTION;

			ctrlptr->ictha_dma_go		= idataptr->ioaddr1 + 2;
			ctrlptr->ictha_reset_dma	= idataptr->ioaddr1 + 3;

			/*
			** Power On Reset Delay 
			*/
			ctrlptr->ictha_por_delay	= 5000000;
		}
#ifdef ICTHA_DEBUG
		cmn_err (CE_CONT, "ICTHA: Searching for %s controller ...\n",
		    ictha_ctrl_name);
#endif

		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_intr_enable );

		if( ctrlptr->ictha_type == ICTHA_WANGTEK ) {
			ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_online );
		}

		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_reset );

		drv_usecwait( ictha_waitcnt );
		ictha_nicreset();

		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_reset );

		drv_usecwait( ctrlptr->ictha_por_delay );

		if (ictha_chkstatus(ctrlptr) == ICTHA_SUCCESS) {
			if(ictha_select(ctrlptr, ICTHA_SELECT0) == ICTHA_FAILURE) {
				ictha_nicreset();
				continue;
			}
			cmn_err (CE_CONT, "!ICTHA: %s controller is found\n",
			    ictha_ctrl_name);
			ctrlptr->ictha_type = contr_type;

			if( contr_type == ICTHA_ARCHIVE ) {
				outb( ctrlptr->ictha_reset_dma, 0x01 );
			}
			In_load = 0;
			return( 1 );
		}
#ifdef ICTHA_DEBUG
		else {
			cmn_err ( CE_CONT, "Not Found\n");
		}
#endif
	}


	In_load = 0;
	return( 0 );
}

/*
 * ictha_free_cache()
 *
 * Calling/Exit State:
 *
 * Description:
 */
void
ictha_free_cache(drvptr)
register DrivePtr drvptr;
{
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err(CE_CONT, "ictha_free_cache(ctrlptr, driveptr)");
#endif

	if(drvptr->drv_dmavbuf == NULL)
		return;
	kmem_free(drvptr->drv_dmavbuf, ictha_blocksize);
	drvptr->drv_dmavbuf = NULL;
}

/*
 * ictha_alloc_cache()
 *
 * Calling/Exit State:
 *
 * Description:
 *	Allocate ictha_blocksize bytes for blocks that cross non-contiguous
 * 	pages.
 */
int
ictha_alloc_cache(drvptr)
register DrivePtr drvptr;
{
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err(CE_CONT, "ictha_alloc_cache(ctrlptr, driveptr)");
#endif
	if(drvptr->drv_dmavbuf != NULL)
		return(ICTHA_SUCCESS);
	drvptr->drv_dmavbuf = (caddr_t) kmem_alloc(ictha_blocksize, KM_SLEEP);
	if(drvptr->drv_dmavbuf == NULL)
		return(ICTHA_FAILURE);
	drvptr->drv_dmapbuf = vtop(drvptr->drv_dmavbuf, NULL);
	return(ICTHA_SUCCESS);
}

/*
 * ictha_drvinit()
 *
 * Calling/Exit State:
 *
 * Description:
 *	init the drive.
 */
/* ARGSUSED */
ictha_drvinit(ctrlptr, drvptr)
register CtrlPtr ctrlptr;
register DrivePtr drvptr;
{
	int nblks = (ulong )(ICTHA_MAX_XFER+ictha_blocksize-1)/ictha_blocksize;

	drvptr->drv_max_xfer_blks = nblks;
	
	drvptr->drv_req.req_block = (struct ictha_block *)
		kmem_alloc(nblks * sizeof(struct ictha_block), KM_SLEEP);

	if(drvptr->drv_req.req_block == NULL)
		return(ICTHA_FAILURE);

	drvptr->drv_ctrlptr = ctrlptr;
	drvptr->drv_at_bot = 1;
	drvptr->drv_dmavbuf = NULL;

	drvptr->drv_busy	= B_FALSE;
	drvptr->drv_headque	= NULL;
	drvptr->drv_tailque	= NULL;

	drvptr->drv_inqdata.id_type = ID_TAPE;
	drvptr->drv_inqdata.id_pqual = ID_QOK;
	drvptr->drv_inqdata.id_ver = 0x1;
	drvptr->drv_inqdata.id_len = 31;

	strncpy(drvptr->drv_inqdata.id_vendor, ictha_ctrl_name,VID_LEN+PID_LEN);
	strncpy(drvptr->drv_inqdata.id_revnum, "1.00", REV_LEN);

	return ( ICTHA_SUCCESS ) ;
}

/*
 * ictha_clear_timeout()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	untimeout any timeout routine previously registered.
 *	Also, wait for the interrupt if a read ahead request was
 *	initiated but not finished yet.
 */
void
ictha_clear_timeouts(driveptr)
register DrivePtr driveptr;
{
	if(driveptr->drv_timeid) {
		untimeout(driveptr->drv_timeid);
		driveptr->drv_timeid = 0;
	}
}

/*
 * ictha_send(hbap)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	This routine is be called by sdi_send passing "hbap" as the address
 *	of a "struct ictha_xsb" that sdi_send allocated by calling ictha_xlat. 	
 */
/* ARGSUSED */
STATIC long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	struct ictha_xsb *blk =  (struct ictha_xsb *) hbap;
	int	c, t, l;
	DrivePtr driveptr;
	CtrlPtr   ctrlptr;
	struct blklen *blklenptr;
	struct scsi_ad *sa;
	register struct sb *sb = blk->sb;
	struct mode *mp;
	pl_t opl;	

#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err ( CE_CONT, "icthasend(%x)\n", blk);
#endif

	sa = &sb->SCB.sc_dev;

	c = ictha_hba2ctrl[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err(CE_CONT, "c %d, t %d l %d\n", c, t, l);
#endif
	if(l != 0 || (t != 0 && t != ICTHA_HBA_TARGET)){
		sb->SCB.sc_comp_code = (unsigned long) SDI_SCBERR;
		sdi_callback(sb);
		return(SDI_RET_OK);
	}
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err(CE_CONT, "c %d, t %d l %d\n", c, t, l);
#endif
	ctrlptr  = &ictha_ctrl[c];
	driveptr = &ctrlptr->ictha_drives[0];

	opl = spldisk();	
	if (driveptr->drv_busy) {
		blk->next = NULL;
		if (driveptr->drv_headque) {
			driveptr->drv_tailque->next = blk;
		} else {
			driveptr->drv_headque = blk;
		}
		driveptr->drv_tailque = blk;
		splx(opl);
		return(SDI_RET_OK);
	}
	splx(opl);

	ctrlptr->ictha_statrdy = 0;
	driveptr->drv_req.req_count = 0;
	driveptr->drv_procp = blk->procp;
	switch (sb->sb_type)
	{
	case SFB_TYPE:
		ictha_clear_timeouts(driveptr);
		switch(sb->SFB.sf_func){
		case SFB_NOPF:
			sb->SFB.sf_comp_code = SDI_ASW;
			break;
		case SFB_ABORTM:
			/* FALLTHRU */
		case SFB_RESETM:
			{
				if(ictha_cmd(ICTHA_RESETM_CMD, ctrlptr, driveptr) == ICTHA_SUCCESS)
					sb->SFB.sf_comp_code = SDI_ASW;
				else sb->SFB.sf_comp_code = (unsigned long) SDI_SFBERR;
			}
			break;
		case SFB_FLUSHR:
			sb->SFB.sf_comp_code = SDI_ASW;
			break;
		case SFB_SUSPEND:
			sb->SFB.sf_comp_code = SDI_ASW;
			break;
		case SFB_RESUME:
			sb->SFB.sf_comp_code = SDI_ASW;
			break;
		default:
			cmn_err(CE_CONT, "SFB DEFAULTS HIT");
			sb->SFB.sf_comp_code = (unsigned long) SDI_SFBERR;
		}
		sdi_callback(sb);
		break;
	case ISCB_TYPE:
	case SCB_TYPE:
		sb->SCB.sc_status = S_GOOD;
		/* figure out if its a 6 or 10 byte SCSI command */
		if(sb->SCB.sc_cmdsz == SCS_SZ)
		{
			struct scs *scsp;
			scsp = (struct scs *) (void *) sb->SCB.sc_cmdpt;

			if(scsp->ss_op == SS_READ)
				ictha_rdwr_cmd(ICTHA_READ_CMD, sb, ctrlptr, driveptr);
			else if (scsp->ss_op == SS_WRITE)
				ictha_rdwr_cmd(ICTHA_WRITE_CMD, sb, ctrlptr, driveptr);
			else {
				ictha_clear_timeouts(driveptr);
				switch(scsp->ss_op){
				case SS_FLMRK:
					driveptr->drv_req.req_count = (uint)scsp->ss_len;
					if( (int)scsp->ss_len <= 0 ) {
						ICTHA_CALLBACK(sb, SDI_TCERR);
						break;
					}
					ictha_scb_cmd(ctrlptr, driveptr, ICTHA_FLMRK_CMD, sb);
					break;
				case SS_ERASE:
					ictha_scb_cmd(ctrlptr, driveptr, ICTHA_ERASE_CMD, sb);
					break;
				case SS_REWIND:
					ictha_scb_cmd(ctrlptr, driveptr, ICTHA_REWIND_CMD, sb);
					break;
				case SS_SPACE:
					driveptr->drv_req.req_count = (uint)scsp->ss_len;
					if( scsp->ss_addr1 == ICTHA_FILEMARKS ){
						if( (uint)scsp->ss_len == 0 ) {
							ICTHA_CALLBACK(sb, SDI_ASW);
						}
						else if( (uint)scsp->ss_len > 125 ) {
							ICTHA_CALLBACK(sb, SDI_TCERR);
						}
						else ictha_scb_cmd(ctrlptr, driveptr,ICTHA_SPACE_CMD, sb);
					} else ICTHA_CALLBACK(sb, SDI_ERROR);
					break;
				case SS_LOAD:
					switch (scsp->ss_len) {
					case ICTHA_LOAD:
						ictha_scb_cmd(ctrlptr, driveptr, ICTHA_LOAD_CMD, sb);
						break;
					case ICTHA_UNLOAD:
						ICTHA_CALLBACK(sb, SDI_ASW);
						break;
					case ICTHA_RETENSN:
						ictha_scb_cmd(ctrlptr, driveptr, ICTHA_RETENSION_CMD, sb);
						break;
					}
					break;
				case SS_LOCK:
					ICTHA_CALLBACK(sb, SDI_ERROR);
					break;
				case SS_RDBLKLEN:
					blklenptr = (struct blklen *) (void *)(sb->SCB.sc_datapt);
					blklenptr->res1 =0;
					blklenptr->max_blen = sdi_swap24(ictha_blocksize);
					blklenptr->min_blen = sdi_swap16(ictha_blocksize);
					ICTHA_CALLBACK(sb, SDI_ASW);
					break;
				case SS_TEST:	    /* Test unit ready       */
					ICTHA_CALLBACK(sb, SDI_ASW);
					break;
				case SS_REQSEN:	    /* Request sense         */
					ictha_scb_cmd(ctrlptr, driveptr, ICTHA_REQSEN_CMD, sb);
					break;
				case SS_INQUIR:	    /* Inquire               */
					ictha_inquir(driveptr, c, t, sb);
					ICTHA_CALLBACK(sb, SDI_ASW);
					break;
				case SS_MSELECT:    /* Mode select           */
					mp = (struct mode *)sb->SCB.sc_datapt;
					if(mp->md_bsize == ictha_blocksize &&
						mp->md_dens == 0){
						ICTHA_CALLBACK(sb, SDI_ASW);
					} else {
						ICTHA_CALLBACK(sb, SDI_ERROR);
					}
					break;
				case SS_RESERV:	    /* Reserve unit          */
					if(ictha_alloc_cache(driveptr) == ICTHA_FAILURE) {
						ICTHA_CALLBACK(sb, SDI_ERROR);
					} else {
						ICTHA_CALLBACK(sb, SDI_ASW);
					}
					break;
				case SS_RELES:	    /* Release unit          */
					ictha_free_cache(driveptr);
					ictha_scb_cmd(ctrlptr, driveptr, ICTHA_RELEASE_CMD, sb);
					if(driveptr->drv_timeid) {
						untimeout(driveptr->drv_timeid);
						driveptr->drv_timeid = 0;
					}
					break;
				case SS_MSENSE:	    /* Mode Sense            */
					ictha_scb_cmd(ctrlptr, driveptr, ICTHA_MSENSE_CMD, sb);
					break;
				case SS_SDDGN:	    /* Send diagnostic       */
					ICTHA_CALLBACK(sb, SDI_ASW);
					break;
				default:
					cmn_err(CE_CONT, "ICTHA: SCB SCS DEFAULTS HIT");
					ICTHA_CALLBACK(sb, SDI_ERROR);
					break;
				}
			}
		}
		else if (sb->SCB.sc_cmdsz == SCM_SZ){
			struct scm *scm;
			scm = (struct scm *) (void *)(SCM_RAD(sb->SCB.sc_cmdpt));
			if(scm->sm_op== SM_READ)
				ictha_rdwr_cmd(ICTHA_READ_CMD, sb, ctrlptr, driveptr);
			else if (scm->sm_op== SM_WRITE)
				ictha_rdwr_cmd(ICTHA_WRITE_CMD, sb, ctrlptr, driveptr);
			else {
				ictha_clear_timeouts(driveptr);
				switch(scm->sm_op){
				case  SM_SEEK:		/* Seek extended      */
					ICTHA_CALLBACK(sb, SDI_ERROR);
					break;
				default:
					cmn_err(CE_CONT, "ICTHA: SCB SCM DEFAULTS HIT");
					ICTHA_CALLBACK(sb, SDI_TCERR);
					break;
				}
			}
		}
		else{
			cmn_err(CE_CONT, "ICTHA: UNKOWN size DEFAULTS HIT");
			ICTHA_CALLBACK(sb, SDI_TCERR);
			/* error */
		}
		break;
	default:
		cmn_err(CE_CONT, "ICTHA: UNKOWN type DEFAULTS HIT");
		ICTHA_CALLBACK(sb, SDI_ERROR);
		break;
	}
	return(SDI_RET_OK);
}

/*
 * ictha_scb_cmd()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	This routine handle all non-read/write commands.
 */

void
ictha_scb_cmd(ctrlptr, driveptr, cmd, sb)
CtrlPtr ctrlptr;
DrivePtr driveptr;
int	cmd;
struct sb *sb;
{
	int	r = 0;

#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err ( CE_CONT, "ictha_scb_cmd(cmd = %x)\n", cmd);
#endif

	driveptr->drv_req.req_addr = (caddr_t)sb->SCB.sc_datapt;

	driveptr->drv_flags = CFLG_CMD;
	driveptr->drv_req.req_sb = sb;

	if((r = ictha_cmd(cmd,ctrlptr,driveptr)) != ICTHA_FAILURE) {
		sb->SCB.sc_comp_code = SDI_ASW;
	}
	else {
		sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
	}
	if(r != ICTHA_INPROGRESS) {
		driveptr->drv_flags = 0;
		driveptr->drv_req.req_sb = NULL;
		driveptr->drv_savecmd = 0;
		sdi_callback(sb);
		if(r == ICTHA_FAILURE)
			ictha_gen_intr(ctrlptr, driveptr);
	}
	driveptr->drv_savecmd = 0;
}

/*
 * ictha_rdwr_cmd()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	This routine handle read/write commands. It breaks up non-aligned
 *	memory blocks (blocks that span across two physically non-contiguous 
 *	pages).  
 */

void
ictha_rdwr_cmd(cmd, sb, ctrlptr, driveptr)
CtrlPtr ctrlptr;
DrivePtr driveptr;
int	cmd;
struct sb *sb;
{
	long	count = 0;
	struct	ictha_block	*blk;
	caddr_t ictha_vpart2;
	paddr_t ictha_part2;
	caddr_t newkvadr;
	ulong	ictha_offset = 0, ictha_leftover = 0;
	paddr_t ictha_bpaddr;
	caddr_t ictha_vadr;
	caddr_t ictha_kvadr;

	ictha_offset = 0;
	ictha_leftover = 0;
	ictha_vadr = (caddr_t)sb->SCB.sc_datapt;

	driveptr->drv_req.req_count = sb->SCB.sc_datasz / ictha_blocksize;
	driveptr->drv_req.req_resid = driveptr->drv_req.req_count;
	driveptr->drv_req.req_addr = NULL;

#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err ( CE_CONT, "ictha_rdwr_cmd(count %d, 0x%x)",
		    driveptr->drv_req.req_count, ictha_vadr);
#endif

	if(ictha_vadr == NULL || driveptr->drv_req.req_count <= 0 ||
	    driveptr->drv_req.req_count >= driveptr->drv_max_xfer_blks) {
		cmn_err ( CE_CONT, "ictha_rdwr_cmd: Illegal request");
		ICTHA_CALLBACK(sb, SDI_ERROR);
		return;
	}

	blk = &driveptr->drv_req.req_block[0];

	for(count = 0; count < driveptr->drv_req.req_count; count++, blk++)
	{
		ictha_offset 	= ((ulong ) ictha_vadr) & ictha_pagemask;
		if(ictha_offset == 0 || count == 0) {
			ictha_bpaddr = vtop(ictha_vadr, driveptr->drv_procp);
			ictha_kvadr = ictha_vadr;
		}
		ictha_leftover 	  = ictha_pagesize - ictha_offset;
		blk->blk_copyflag = 0;
		blk->blk_paddr    = ictha_bpaddr;
		blk->blk_vaddr1   = ictha_kvadr;
		/* Is this block span across two pages */
		if(ictha_leftover < ictha_blocksize) {
			newkvadr = ictha_kvadr + ictha_blocksize;
			ictha_vpart2 = ictha_vadr + ictha_leftover;

			ictha_part2 = vtop(ictha_vpart2, driveptr->drv_procp);
			/* are these two pages physically non-contiguous */
			if(ictha_part2 != (ictha_bpaddr + ictha_leftover)) {
				blk->blk_paddr = driveptr->drv_dmapbuf;
				blk->blk_copyflag = 1;
			}
			ictha_kvadr = newkvadr;
			ictha_bpaddr = ictha_part2 + (ictha_blocksize - ictha_leftover);
			ictha_vadr += ictha_blocksize;
		} else {
			ictha_vadr += ictha_blocksize;
			ictha_kvadr += ictha_blocksize;
			ictha_bpaddr += ictha_blocksize;
		}
	}

	driveptr->drv_req.req_blkptr = &driveptr->drv_req.req_block[0];

	driveptr->drv_flags = CFLG_CMD;
	driveptr->drv_req.req_sb = sb;

	driveptr->drv_busy = B_TRUE;
	if( ictha_cmd(cmd,ctrlptr,driveptr) == ICTHA_FAILURE) {
		if(driveptr->drv_at_eom)	{
			driveptr->drv_req.req_sb->SCB.sc_status = S_CKCON;
			ICTHA_CALLBACK(sb, SDI_CKSTAT);
		}
		else	{
			driveptr->drv_busy = B_FALSE;
			driveptr->drv_flags = 0;
			driveptr->drv_req.req_sb = NULL;
			ICTHA_CALLBACK(sb, SDI_ERROR);
			ictha_gen_intr(ctrlptr, driveptr);
		}
	}
}

/*
 * ictha_cmd()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	exec cmd.
 */
int
ictha_cmd(cmd, ctrlptr, driveptr)
int	cmd;
register CtrlPtr ctrlptr;
register DrivePtr driveptr;
{
	int i;
	struct mode *mp;
	struct sense *sp;

#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err ( CE_CONT, "ictha_cmd(cmd = %x)\n", cmd);
#endif

	if( driveptr->drv_nocartridge ) {
		if( ctrlptr->ictha_type == ICTHA_WANGTEK ) {
			ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_online );
		}

		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_reset );

		drv_usecwait( ictha_waitcnt );

		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_reset );

		drv_usecwait( ctrlptr->ictha_por_delay );

		driveptr->drv_at_bot = 1;
		ctrlptr->ictha_reles = 1;
		driveptr->drv_nocartridge = 0;
		driveptr->drv_drverror = 0;
#ifdef ICTHASTAT
		ctrlptr->ictha_savecmd = 0;
#endif
		driveptr->drv_savecmd = 0;
	}

	if( cmd != ICTHA_LOAD_CMD )
		ictha_status_upd( ctrlptr, driveptr );

	if( driveptr->drv_nocartridge &&
	    (cmd != ICTHA_READ_CMD && cmd != ICTHA_WRITE_CMD && cmd != ICTHA_RELEASE_CMD))
		return (ICTHA_FAILURE);
	driveptr->drv_timeout = ictha_cmds_watchdog;
	driveptr->drv_duration = ictha_cmds_duration;

	switch (cmd) {
	case ICTHA_READ_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: SS_READ\n");
		}
#endif
		driveptr->drv_drvcmd = ICTHA_READ;
#ifdef ICTHASTAT
		ctrlptr->ictha_drvcmd = ICTHA_READ;
#endif
		driveptr->drv_state = DSTA_NORMIO;
		if( driveptr->drv_at_filemark || driveptr->drv_nocartridge ) {
			return(ICTHA_FAILURE);
		}
		break;

	case ICTHA_WRITE_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_WRITE_CMD\n");
		}
#endif
		if( driveptr->drv_at_eom || driveptr->drv_nocartridge ) {
			return(ICTHA_FAILURE);
		}
		driveptr->drv_drvcmd = ICTHA_WRITE;
#ifdef ICTHASTAT
		ctrlptr->ictha_drvcmd = ICTHA_WRITE;
#endif
		driveptr->drv_state = DSTA_NORMIO;
		break;

	case ICTHA_ERASE_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_ERASE_CMD\n");
		}
#endif
		driveptr->drv_drvcmd = ICTHA_ERASE;
#ifdef ICTHASTAT
		ctrlptr->ictha_drvcmd = ICTHA_ERASE;
#endif
		driveptr->drv_duration = ictha_erase_duration;
		driveptr->drv_state = DSTA_RECAL;
		driveptr->drv_req.req_addr = (paddr_t)0;
		break;

	case ICTHA_RETENSION_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: RETENSION\n");
		}
#endif
		driveptr->drv_drvcmd = ICTHA_RETENSION;
#ifdef ICTHASTAT
		ctrlptr->ictha_drvcmd = ICTHA_RETENSION;
#endif
		driveptr->drv_state = DSTA_RECAL;
		driveptr->drv_req.req_addr = (paddr_t)0;
		driveptr->drv_duration = ictha_retention_duration;
		break;

	case ICTHA_FLMRK_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: DCDM_WFM\n");
		}
#endif

		driveptr->drv_drvcmd = ICTHA_WRFILEM;
#ifdef ICTHASTAT
		ctrlptr->ictha_drvcmd = ICTHA_WRFILEM;
#endif
		driveptr->drv_state = DSTA_RECAL;
		driveptr->drv_req.req_addr = (paddr_t)0;
		driveptr->drv_duration = ictha_cmds_duration *
		    driveptr->drv_req.req_count;
		break;

	case ICTHA_REWIND_CMD:
		driveptr->drv_duration = ictha_rewind_duration;
		driveptr->drv_exception = 0;
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_REWIND_CMD\n");
		}
#endif
		driveptr->drv_req.req_addr = (paddr_t)0;
		if( driveptr->drv_savecmd == ICTHA_READ &&
		    ctrlptr->ictha_type == ICTHA_WANGTEK ) {

			/*
			** WorkAround for a WANGETK Conrtoller Problem 
			***                                           
			** Some Wangtek controllers will not execute 
			** a REWIND command if the last command was 
			** a READ. This logic forces the REWIND by 
			** dropping the ONLINE bit.               
			*/
			ctrlptr->ictha_datardy = 0;

			ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_online );
			ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
			driveptr->drv_savecmd = driveptr->drv_drvcmd = ICTHA_REWIND;
#ifdef ICTHASTAT
			ctrlptr->ictha_savecmd = ICTHA_REWIND;
#endif
			driveptr->drv_state = DSTA_RECAL;
			drv_usecwait( 10 );
			ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
			driveptr->drv_at_bot = 1;
			driveptr->drv_flags |= CFLG_INT;
			return(ICTHA_INPROGRESS);
		}
		else {
			driveptr->drv_drvcmd = ICTHA_REWIND;
#ifdef ICTHASTAT
			ctrlptr->ictha_savecmd = ICTHA_REWIND;
#endif
			driveptr->drv_state = DSTA_RECAL;
		}

		break;

	case ICTHA_SPACE_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_SPACE_CMD\n");
		}
#endif
		driveptr->drv_drvcmd = ICTHA_RDFILEM;
#ifdef ICTHASTAT
		ctrlptr->ictha_savecmd = ICTHA_RDFILEM;
#endif
		if(driveptr->drv_exception && driveptr->drv_at_filemark) {
			driveptr->drv_exception  = 0;
			driveptr->drv_at_filemark = 0;
			driveptr->drv_state = DSTA_IDLE;
			return(ICTHA_SUCCESS);
		}
		driveptr->drv_exception = 0;
		driveptr->drv_state = DSTA_RECAL;
		driveptr->drv_req.req_addr = (paddr_t)0;
		driveptr->drv_duration = ictha_space_duration;
		break;

	case ICTHA_LOAD_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_LOAD_CMD\n");
		}
#endif
		driveptr->drv_req.req_addr = (paddr_t)0;
		if( ictha_rdstatus(ctrlptr) == ICTHA_FAILURE ) {
			driveptr->drv_nocartridge = 1;
			return(ICTHA_FAILURE);
		}

		if( ctrlptr->ictha_status_buf[ 0 ] & 0x40 ) {
			/*
			**  Cartidge Not In Place 
			***                      
			**      Try it again    
			** for Archive 2150L drive
			*/
			if( ictha_rdstatus(ctrlptr) == ICTHA_FAILURE ) {
				driveptr->drv_nocartridge = 1;
				return(ICTHA_FAILURE);
			}
		}

		if( ctrlptr->ictha_status_buf[ 0 ] & 0x40 ) {
			/*
			** Cartidge Not In Place
			*/
			driveptr->drv_nocartridge = 1;
			return(ICTHA_FAILURE);
		}

		ctrlptr->ictha_reles = 0;
		return(ICTHA_SUCCESS);

	case ICTHA_REQSEN_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_REQSEN\n");
		}
#endif
		/*
		** Currently a RDSTATUS is not being issued        
		** since the only time we __SHOULD__ be getting    
		** a REQSENSE command is after we report an        
		** exception back up the chain, and we will have   
		** done the RDSTATUS at the time of the exception. 
		** If we need to handle REQSENSE commands in cases 
		** other than exceptions, a RDSTATUS will have to  
		** be issued here.                                 
		*/
		sp = (struct sense *)(void *)((char *) driveptr->drv_req.req_addr-1);
		sp->sd_errc = 0x70;
		sp->sd_valid = 1;
		sp->sd_ili = 0;
		sp->sd_key = SD_NOSENSE;
		sp->sd_eom = driveptr->drv_at_eom;
		sp->sd_fm = driveptr->drv_at_filemark;
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1) {
			cmn_err ( CE_CONT, "ICTHA_CMD: req_resid %d\n",
			    driveptr->drv_req.req_resid);
		}
#endif
		sp->sd_ba = sdi_swap32(driveptr->drv_req.req_resid);
		sp->sd_len = 0x0e;
		sp->sd_sencode = SC_IDERR;

		if( ctrlptr->ictha_newstatus ) {
			for( i = 0; i < ICTHA_STATBUF_SZ; ++i ) {
				ctrlptr->ictha_status_buf[i] = ctrlptr->ictha_status_new[i];
			}
			ctrlptr->ictha_newstatus = 0;
		}
		ictha_status_upd( ctrlptr, driveptr );
		return(ICTHA_SUCCESS);

	case ICTHA_MSENSE_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_MSENSE_CMD\n");
		}
#endif
		mp = ((struct mode *)(void *)driveptr->drv_req.req_addr);
		mp->md_len = 0x0;
		mp->md_media = 0x0;
		mp->md_speed = 0x0;
		mp->md_bm = 0x0;
		mp->md_bdl = 0x0;
		mp->md_dens = 0x0;
		mp->md_nblks = 0x0;
		mp->md_res = 0x0;
		mp->md_bsize = (unsigned) ictha_blocksize;

		if( ictha_rdstatus(ctrlptr) == ICTHA_FAILURE ) {
			return(ICTHA_FAILURE);
		}

		if( ctrlptr->ictha_status_buf[ 0 ] & 0x10 ) {
			mp->md_wp = 1;
		}
		else {
			mp->md_wp = 0;
		}

		return(ICTHA_SUCCESS);

	case ICTHA_RELEASE_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_RELES_CMD\n");
		}
#endif
		driveptr->drv_savecmd = driveptr->drv_drvcmd = 0;
#ifdef ICTHASTAT
		ctrlptr->ictha_prev_nblocks = ctrlptr->ictha_nblocks;
		ctrlptr->ictha_nblocks = 0;
		ctrlptr->ictha_savecmd = 0;
#endif
		ctrlptr->ictha_reles = 1;

		if( !driveptr->drv_at_eom )
			return(ICTHA_SUCCESS);
		/* else do a reset */
		/* FALLTHRU */

	case ICTHA_RESETM_CMD:
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1  &&  cmd == ICTHA_RESETM_CMD ) {
			cmn_err ( CE_CONT, "ICTHA_CMD: ICTHA_RESETM\n");
		}
#endif

		ctrlptr->ictha_datardy = 0;
		driveptr->drv_req.req_addr = (paddr_t)0;
		if( ctrlptr->ictha_type == ICTHA_WANGTEK ) {
			ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_online );
		}

		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_reset );

		drv_usecwait( ictha_waitcnt );

		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_reset );

		drv_usecwait( ctrlptr->ictha_por_delay );

		if( ictha_rdstatus(ctrlptr) != ICTHA_SUCCESS )
			return(ICTHA_FAILURE);
		if ((ctrlptr->ictha_status_buf[1]&( ICTHA_POR|ICTHA_SBYTE1)) ==
		     (ICTHA_POR|ICTHA_SBYTE1)) {
			driveptr->drv_at_bot = 1;
			return(ICTHA_SUCCESS);
		}
		return(ICTHA_FAILURE);
	default:
		driveptr->drv_req.req_addr = (paddr_t)0;
		cmn_err(CE_CONT,"ictha_cmd: Unknown command [%d]\n",cmd);
		return(ICTHA_FAILURE);
	}

	if( ictha_docmd( ctrlptr, driveptr ) == ICTHA_INPROGRESS) {
		driveptr->drv_savecmd = driveptr->drv_drvcmd;
#ifdef ICTHASTAT
		ctrlptr->ictha_drvcmd = driveptr->drv_drvcmd;
#endif
		return(ICTHA_INPROGRESS);
	}
	else {
#ifdef ICTHASTAT
		ctrlptr->ictha_savecmd = 0;
#endif
		driveptr->drv_savecmd = 0;
		return(ICTHA_FAILURE);
	}
}

/*
 * icthaintr()
 *
 * Calling/Exit State:
 *
 * Description:
 */
/* ARGSUSED */

void
icthaintr(intidx)
unsigned int	intidx;
{
	DrivePtr driveptr;
	CtrlPtr ctrlptr;
	/*
	May be that how it should be done.
	ctrlptr  = &ictha_ctrl[int2ctrl[intidx]];
	*/
	ctrlptr  = &ictha_ctrl[0];
	driveptr = &ctrlptr->ictha_drives[0];

#ifdef ICTHA_DEBUG
	if( ictha_debug_flag == 1 ) {
		cmn_err ( CE_CONT, "ICTHA_INT %d\n", intidx);
	}
#endif

	/*
	** Deassert Interrupt Enable on the controller 
	*/
	ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_intr_enable );

	if((driveptr->drv_flags & CFLG_INT) == 0 ) {
		/*
		** Stray interrupt since we're not doing anything. 
		*/
#ifdef ICTHA_DEBUG
		cmn_err ( CE_CONT, "ICTHA_INT: Stray\n");
#endif
		return;
	}

	driveptr->drv_flags &= ~CFLG_INT;

	if(ictha_chk_excpt(ctrlptr, driveptr, 0) == ICTHA_EXCEPTION &&
	   driveptr->drv_drvcmd != ICTHA_RDFILEM ) {
		ictha_chk_endjob(ctrlptr, driveptr);
		return;
	}

	if( ctrlptr->ictha_statrdy ) {
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_INT: ictha_rdstat_rdy DINT_ERRABORT\n");
		}
#endif
		ctrlptr->ictha_datardy = 0;
		ctrlptr->ictha_statrdy = 0;
		ictha_rdstat_rdy(ctrlptr);
		return;
	}

	if( ctrlptr->ictha_datardy ) {
		/*
		** Special case, first read/write interrupt	
		** indicating device is ready for data transfer.	
		*/
#ifdef ICTHA_DEBUG
		if(ictha_debug_flag)
			cmn_err ( CE_CONT, "ictha_int special case\n");
#endif
		if( ctrlptr->ictha_datardy == ICTHA_DMA_WRITE ) {
			ctrlptr->ictha_datardy = 0;
			ictha_timeout(driveptr);
			return;
		}
		ictha_setdma(ctrlptr, driveptr, ctrlptr->ictha_datardy );
		ctrlptr->ictha_datardy = 0;
		return;
	}

	/*
	** If a DMA job was in progress, mask out DMA channel 1
	*/
	if( ctrlptr->ictha_type != ICTHA_MCA_ARCHIVE &&
	    ( driveptr->drv_drvcmd == ICTHA_WRITE ||
	    driveptr->drv_drvcmd == ICTHA_READ )) {

		dma_disable(ctrlptr->ictha_dmachan);
	}

	switch ( (int)driveptr->drv_state ) {
	case DSTA_RECAL:

#ifdef ICTHA_DEBUG
		if(ictha_debug_flag)
			cmn_err ( CE_CONT, "icthaintr: DSTA_RECAL\n");
#endif
		driveptr->drv_at_bot = (driveptr->drv_drvcmd == ICTHA_REWIND ||
		    driveptr->drv_drvcmd == ICTHA_ERASE) ? 1:0;

		if( driveptr->drv_drvcmd != ICTHA_WRFILEM &&
		    driveptr->drv_drvcmd != ICTHA_RDFILEM ) {
			if(driveptr->drv_timeid !=  0) {
				untimeout(driveptr->drv_timeid);
				driveptr->drv_timeid =  0;
			}
			ICTHA_SDI_RETURN(driveptr, SDI_ASW, KM_NOSLEEP);
			return;
		}

		if( driveptr->drv_drvcmd == ICTHA_RDFILEM ) {
			ictha_clrstatus( ctrlptr->ictha_status_buf );
			driveptr->drv_at_filemark = 0;
		}
		--driveptr->drv_req.req_count;
		if(driveptr->drv_req.req_count <= 0 ) {
			if(driveptr->drv_timeid !=  0) {
				untimeout(driveptr->drv_timeid);
				driveptr->drv_timeid =  0;
			}
			ICTHA_SDI_RETURN(driveptr, SDI_ASW, KM_NOSLEEP);
			return;
		}
		if( ictha_docmd( ctrlptr, driveptr ) == ICTHA_FAILURE) {
			if(driveptr->drv_timeid !=  0) {
				untimeout(driveptr->drv_timeid);
				driveptr->drv_timeid =  0;
			}
			ICTHA_SDI_RETURN(driveptr, SDI_ERROR, KM_NOSLEEP);
			ictha_gen_intr(driveptr->drv_ctrlptr, driveptr);
			/*
			 *+ Unable to read/write filemark.
			 */
			if( driveptr->drv_drvcmd == ICTHA_WRFILEM )
				cmn_err(CE_WARN,"ICTHA_INT: Unable to write filemark.");
			else	cmn_err(CE_WARN,"ICTHA_INT: Unable to read filemark.");
		}
		return;

	case DSTA_NORMIO:
#ifdef ICTHA_DEBUG
		if(ictha_debug_flag)
			cmn_err ( CE_CONT, "icthaintr DSTA_NORMIO");
#endif
#ifdef ICTHASTAT
		ctrlptr->ictha_nblocks++;
#endif
		driveptr->drv_at_bot = 0;
		driveptr->drv_cmds_ack++;

		if( driveptr->drv_drverror != 0 ) {
#ifdef ICTHA_DEBUG
			if( ictha_debug_flag == 1 ) {
				cmn_err ( CE_CONT, "ICTHA_INT: NORMIO DINT_ERRABORT\n");
			}
#endif
#ifdef ICTHASTAT
			++ctrlptr->ictha_errabort;
#endif
			ICTHA_SDI_RETURN(driveptr, SDI_ERROR, KM_NOSLEEP);
			ictha_gen_intr(driveptr->drv_ctrlptr, driveptr);
			return;
		}
		if(driveptr->drv_drvcmd == ICTHA_READ && 
		    driveptr->drv_req.req_resid > 0 &&
		    driveptr->drv_req.req_blkptr->blk_copyflag) {
			bcopy (
		 		driveptr->drv_dmavbuf, 
				driveptr->drv_req.req_blkptr->blk_vaddr1,
		 		ictha_blocksize);
		}
		driveptr->drv_req.req_blkptr++;
		driveptr->drv_req.req_resid--;

		if(driveptr->drv_req.req_resid > 0) {
			if( driveptr->drv_drvcmd == ICTHA_WRITE ) {
#ifdef ICTHASTAT
				++ctrlptr->ictha_writecnt;
#endif
				ictha_setdma(ctrlptr, driveptr, ICTHA_DMA_WRITE );
			}
			else {
#ifdef ICTHASTAT
				++ctrlptr->ictha_readcnt;
#endif
				ictha_setdma(ctrlptr, driveptr, ICTHA_DMA_READ );
			}
		} else {
			ICTHA_SDI_RETURN(driveptr, SDI_ASW, KM_NOSLEEP);
		}
		return;
	default:
#ifdef ICTHASTAT
		++ctrlptr->ictha_errabort;
#endif
		/*
		 *+ ictha_int: Invalid State.
		 */
		cmn_err(CE_WARN,"ictha_int: Invalid State - 0x%x",driveptr->drv_state);
		ICTHA_SDI_RETURN(driveptr, SDI_ERROR, KM_NOSLEEP);
	}
}

/*
 * ictha_docmd(ctrlptr, driveptr)
 * Calling/Exit State:
 *
 * Description:
 */
/* ARGSUSED */
int
ictha_docmd(ctrlptr, driveptr)
CtrlPtr ctrlptr;
DrivePtr driveptr;
{
	int ret = 0;

#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err ( CE_CONT, "ictha_docmd\n");
#endif
	driveptr->drv_flags &= ~CFLG_INT;

	ctrlptr->ictha_datardy = 0;
	if( driveptr->drv_drvcmd == ICTHA_WRITE &&
	    driveptr->drv_savecmd == ICTHA_WRITE ) {
#ifdef ICTHASTAT
		++ctrlptr->ictha_writecnt;
#endif
		ictha_setdma(ctrlptr, driveptr, ICTHA_DMA_WRITE );
		return(ICTHA_INPROGRESS);
	}

	if( driveptr->drv_drvcmd == ICTHA_READ &&
	    driveptr->drv_savecmd == ICTHA_READ &&
	    driveptr->drv_at_filemark == 0 &&
	    driveptr->drv_at_eom == 0 ) {
#ifdef ICTHASTAT
		++ctrlptr->ictha_readcnt;
#endif
		ictha_setdma(ctrlptr, driveptr, ICTHA_DMA_READ );
		return(ICTHA_INPROGRESS);
	}


	if( wait_til_ready(ctrlptr, ictha_wait_limit, 11 ) == ICTHA_FAILURE ) {
		driveptr->drv_drverror = ICTHA_NOT_READY;
		return(ICTHA_FAILURE);
	}
	if( (ctrlptr->ictha_type == ICTHA_WANGTEK || 
	    ctrlptr->ictha_type == ICTHA_MCA_ARCHIVE) &&
	    ( driveptr->drv_drvcmd == ICTHA_READ ||
	    driveptr->drv_drvcmd == ICTHA_WRITE || 
	    driveptr->drv_drvcmd == ICTHA_RDFILEM ||
	    driveptr->drv_drvcmd == ICTHA_WRFILEM ) ) {

		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_online );
	}

	outb( ctrlptr->ictha_command, driveptr->drv_drvcmd );

	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_request );

	if( wait_til_ready(ctrlptr, ictha_wait_limit, 1 ) == ICTHA_FAILURE ) {
		driveptr->drv_drverror = ICTHA_NOT_READY;
		return(ICTHA_FAILURE);
	}

	if( driveptr->drv_drvcmd == ICTHA_WRITE ) {

#ifdef ICTHASTAT
		++ctrlptr->ictha_writecnt;
#endif
		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );

		if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
			driveptr->drv_drverror = ICTHA_NOT_READY;
			return(ICTHA_FAILURE);
		}

		if( wait_til_ready(ctrlptr, ictha_wait_limit, 2 ) == ICTHA_FAILURE ) {
			if( driveptr->drv_at_bot ) {
				ctrlptr->ictha_datardy = ICTHA_DMA_WRITE;
				ret = ictha_wait_int(driveptr, ictha_wr_maxout);
				if( ctrlptr->ictha_datardy || ret ) {
					driveptr->drv_drverror = ICTHA_NOT_READY;
					ctrlptr->ictha_datardy = 0;
					return(ICTHA_FAILURE);
				}
			}
		}
		driveptr->drv_cmds_sent = 0;
		driveptr->drv_cmds_ack  = 0;
		driveptr->drv_timeout = ictha_rdwr_watchdog;
		driveptr->drv_duration = ictha_rdwr_duration;
		ictha_clear_timeouts(driveptr);
		driveptr->drv_timeid = timeout(driveptr->drv_timeout,
		    (caddr_t) driveptr, driveptr->drv_duration * HZ);
		ictha_setdma(ctrlptr, driveptr, ICTHA_DMA_WRITE );
	}
	else if( driveptr->drv_drvcmd == ICTHA_READ ) {

		driveptr->drv_cmds_sent = 0;
		driveptr->drv_cmds_ack  = 0;
#ifdef ICTHASTAT
		++ctrlptr->ictha_readcnt;
#endif
		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );

		if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
			driveptr->drv_drverror = ICTHA_NOT_READY;
			return(ICTHA_FAILURE);
		}

		driveptr->drv_flags |= CFLG_INT;
		ctrlptr->ictha_datardy = ICTHA_DMA_READ;
		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
		driveptr->drv_timeout = ictha_rdwr_watchdog;
		driveptr->drv_duration = ictha_rdwr_duration;
		ictha_clear_timeouts(driveptr);
		driveptr->drv_timeid = timeout(driveptr->drv_timeout,
		    (caddr_t) driveptr, driveptr->drv_duration * HZ);
	}
	else {
		drv_getparm(LBOLT, (void *)&driveptr->drv_starttime);
		driveptr->drv_flags |= CFLG_INT;
		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );
		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
		ictha_clear_timeouts(driveptr);
		driveptr->drv_timeid = timeout(driveptr->drv_timeout,
		    (caddr_t) driveptr, driveptr->drv_duration * HZ);
	}
	driveptr->drv_drverror = 0;

	return(ICTHA_INPROGRESS);
}

/*
 * ictha_timeout()
 *
 * Calling/Exit State:
 * 	None
 */
/* ARGSUSED */

void
ictha_timeout(driveptr)
DrivePtr driveptr;
{
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err ( CE_CONT, "ictha_timeout\n");
#endif
	driveptr->drv_sleepflag = 0;
	wakeup( (caddr_t)&driveptr->drv_sleepflag );
}

/*
 * ictha_rdstatus()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Read the six bytes status.
 */
ictha_rdstatus(ctrlptr)
CtrlPtr ctrlptr;
{
	register int i;
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err ( CE_CONT, "ictha_rdstatus\n");
#endif

	ICTHA_CHK_DEASSERT(ctrlptr, ctrlptr->ictha_intr_enable );

	ictha_clrstatus( ctrlptr->ictha_status_buf);

	outb( ctrlptr->ictha_command, ICTHA_RD_STATUS );

	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_request );

	ictha_nicreset();
	if( ictha_excpt_clr(ctrlptr) == ICTHA_FAILURE ) {
		return( ICTHA_FAILURE );
	}

	if( wait_til_ready(ctrlptr, ictha_wait_limit, 3 ) == ICTHA_FAILURE ) {
		return( ICTHA_FAILURE );
	}

	ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );

	if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
		return( ICTHA_FAILURE );
	}

	for( i = 0; i < 6; ++i ) {

		if( wait_til_ready(ctrlptr, ictha_wait_limit, 4 ) == ICTHA_FAILURE ) {
			return( ICTHA_FAILURE );
		}

		ctrlptr->ictha_status_buf[ i ] = inb( ctrlptr->ictha_data );

		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_request );
		ictha_nicreset();

		if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
			ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );
			return( ICTHA_FAILURE );
		}

		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );
	}

	return( ICTHA_SUCCESS );
}

/*
 * wait_til_ready()
 *
 * Calling/Exit State:
 *
 * Description:
 *	Wait until the controller is ready
 */
/* ARGSUSED */
int
wait_til_ready(ctrlptr, wait_limit, debugflag)
CtrlPtr ctrlptr;
ulong	wait_limit;
int	debugflag;
{
	register int	i;
	unchar	 status;

	for( i = 0; i < wait_limit; ++i ) {
		status = inb( ctrlptr->ictha_status );
		if((status & ctrlptr->ictha_ready) == 0)
			return(ICTHA_SUCCESS);
		drv_usecwait( 10 );
	}
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err(CE_CONT, "wait_til_ready FAILED");
#endif
	return( ICTHA_FAILURE );
}

/*
 * wait_til_not_ready()
 *
 * Calling/Exit State:
 *
 * Description:
 *	Wait until the controller is not ready
 */
int
wait_til_not_ready(ctrlptr, wait_limit)
CtrlPtr ctrlptr;
ulong	wait_limit;
{
	register int	i;
	unchar	 status;

	for( i = 0; i < wait_limit; ++i ) {
		status = inb( ctrlptr->ictha_status );
		if((status & ctrlptr->ictha_ready) != 0)
			return(ICTHA_SUCCESS);
		drv_usecwait( 10 );
	}
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err(CE_CONT, "wait_til_not_ready FAILED");
#endif
	return( ICTHA_FAILURE );
}

/*
 * ictha_excpt_clr()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Wait until the exception is cleared (until bit is on).
 */
int
ictha_excpt_clr(ctrlptr)
CtrlPtr ctrlptr;
{
	register int	i;
	unchar	 status;

	for( i = 0; i < ictha_wait_limit; ++i ) {
		status = inb( ctrlptr->ictha_status );
		if((status&ctrlptr->ictha_exception) != 0)
			return( ICTHA_SUCCESS );
		drv_usecwait(10);
	}
#ifdef ICTHA_DEBUG
	if(ictha_debug_flag)
		cmn_err(CE_CONT, "ictha_excpt_clr FAILED");
#endif
	return( ICTHA_FAILURE );
}

/*
 * ictha_setdma()
 *
 * Calling/Exit State:
 *
 * Description:
 *	Setup the DMA for the read/write operation.
 */
/* ARGSUSED */
ictha_setdma(ctrlptr, driveptr, rd_wr_flag )
CtrlPtr ctrlptr;
register DrivePtr driveptr;
char	rd_wr_flag;
{
	struct ictha_block *blk = driveptr->drv_req.req_blkptr;


	driveptr->drv_flags |= CFLG_INT;

	if(blk->blk_copyflag && rd_wr_flag == ICTHA_WRITE) {
			bcopy (
				driveptr->drv_req.req_blkptr->blk_vaddr1,
		 		driveptr->drv_dmavbuf, 
		 		ictha_blocksize);
	}

	dma_disable(ctrlptr->ictha_dmachan);

	ictha_cb->targbufs->address = blk->blk_paddr;
	ictha_cb->targbufs->count = (ushort_t)ictha_blocksize;
	if (rd_wr_flag == ICTHA_DMA_READ)       {
		ictha_cb->command = DMA_CMD_READ;
	}
	else    {
		ictha_cb->command = DMA_CMD_WRITE;
	}

	if (dma_prog(ictha_cb, ctrlptr->ictha_dmachan, DMA_NOSLEEP) == FALSE) {
#ifdef DEBUG
		cmn_err(CE_NOTE, "ictha: dma_prog() failed!");
#endif  /* DEBUG */
		return (1);
	}

	drv_getparm(LBOLT, (void *)&driveptr->drv_starttime);

	switch( ctrlptr->ictha_type ) {
	case ICTHA_WANGTEK:
		ICTHA_CHK_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
		dma_enable(ctrlptr->ictha_dmachan);
		break;
	case ICTHA_ARCHIVE:
		ICTHA_CHK_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
		outb( ctrlptr->ictha_dma_go, 0x01 );
		dma_enable(ctrlptr->ictha_dmachan);
		break;
	case ICTHA_MCA_ARCHIVE:
		dma_enable(ctrlptr->ictha_dmachan);
		ctrlptr->ictha_cntrl_mask |= ctrlptr->ictha_intr_enable;
		outb( ctrlptr->ictha_control, ctrlptr->ictha_dma_enable);
		break;
	}
	driveptr->drv_cmds_sent++;

	return( 0 );
}

/*
 * ictha_chk_excpt()
 *
 * Calling/Exit State:
 *
 * Description:
 *	Check if the controller is showing an exception condition.
 */
int 
ictha_chk_excpt(ctrlptr, driveptr, excptflag)
CtrlPtr ctrlptr;
register DrivePtr driveptr;
int	excptflag;
{
	int	i, exception;

	if(!excptflag) {
		/*
		** Read the controller status port 
		*/
		ctrlptr->ictha_cntr_status = inb( ctrlptr->ictha_status );
#ifdef ICTHA_DEBUG
		if(ictha_debug_flag)
			cmn_err ( CE_CONT, 
			    "ICTHA_CHK_EXCPT: status is 0x%x, control 0x%x\n",
			    (int) ctrlptr->ictha_cntr_status,
			    (int) ctrlptr->ictha_cntrl_mask);
#endif
		/*
		** Is the controller showing EXCEPTION? 
		*/
		if((ctrlptr->ictha_cntr_status & ctrlptr->ictha_exception) != 0)
			return(ICTHA_ERROR);
#ifdef ICTHA_DEBUG
		if(ictha_debug_flag) {
			cmn_err ( CE_CONT, 
			    "ICTHA_CHK_EXCPT: Execption! status is 0x%x, control 0x%x\n",
			    (int) ctrlptr->ictha_cntr_status,
			    (int) ctrlptr->ictha_cntrl_mask);
		}
#endif
	}
	/*
	** Read the controller's status register file 
	*/
	ictha_rdstatus(ctrlptr);

	if( ictha_init_time == 1 ) {
#ifdef ICTHA_DEBUG
		if( ictha_debug_flag == 1 ) {
			cmn_err ( CE_CONT, "ICTHA_EXCPT: Init time exception!\n");
		}
#endif
		return(~ICTHA_EXCEPTION);
	}


	for(i=0; i<NEXCPTS; i++) {
		if((ctrlptr->ictha_status_buf[0] & ictha_ex[i].mask0) == ictha_ex[i].byte0  &&
		    (ctrlptr->ictha_status_buf[1] & ictha_ex[i].mask1) == ictha_ex[i].byte1) {
			exception = ictha_ex[i].type;
			driveptr->drv_drverror = ictha_ex[i].drverror;
			break;
		}
	}

#ifdef ICTHA_DEBUG
	if(ictha_debug_flag) {
		if(i < NEXCPTS)
			cmn_err ( CE_CONT, 
			    "ICTHA_EXCPT: %s\n", ictha_ex[i].errtext);
		else	cmn_err ( CE_CONT, "ICTHA_EXCPT: i = %d\n", i);
	}
#endif
	switch (exception) {
	case ICTHA_EOM:
		if( driveptr->drv_at_eom == 0 ) {
			driveptr->drv_at_eom = 1;
			return(ICTHA_EXCEPTION);
		}
		break;
	case ICTHA_EOF:
		if( driveptr->drv_at_filemark == 0 ) {
			driveptr->drv_at_filemark = 1;
			return(ICTHA_EXCEPTION);
		}
		break;
	case ICTHA_NCT:
		driveptr->drv_nocartridge = 1;
		break;
	}

	if( ctrlptr->ictha_datardy ) {
		driveptr->drv_nocartridge = 1;
	}

	return( ICTHA_EXCEPTION );
}

/*
 * ictha_rdstat_setup()
 *
 * Calling/Exit State:
 *
 * Description:
 */
ictha_rdstat_setup(ctrlptr)
CtrlPtr ctrlptr;
{
	ICTHA_CHK_DEASSERT(ctrlptr, ctrlptr->ictha_intr_enable );

	ictha_clrstatus( ctrlptr->ictha_status_new );

	outb( ctrlptr->ictha_command, ICTHA_RD_STATUS );

	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_request );

	if( ictha_excpt_clr(ctrlptr) == ICTHA_FAILURE ) {
		return( ICTHA_FAILURE );
	}

	if( wait_til_ready(ctrlptr, ictha_wait_limit, 5 ) == ICTHA_FAILURE ) {
		return( ICTHA_FAILURE );
	}

	ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );

	if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
		return( ICTHA_FAILURE );
	}
	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
	return ( ICTHA_SUCCESS );
}

/*
 * ictha_rdstat_rdy()
 *
 * Calling/Exit State:
 *
 * Description:
 */
ictha_rdstat_rdy(ctrlptr)
CtrlPtr ctrlptr;
{
	register int i;

	ICTHA_CHK_DEASSERT(ctrlptr, ctrlptr->ictha_intr_enable );

	for( i = 0; i < 6; ++i ) {

		if( wait_til_ready(ctrlptr, ictha_wait_limit, 6 ) == ICTHA_FAILURE ) {
			return( ICTHA_FAILURE );
		}

		ctrlptr->ictha_status_new[ i ] = inb( ctrlptr->ictha_data );

		ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_request );

		if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
			ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );
			return( ICTHA_FAILURE );
		}

		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );
	}

	ctrlptr->ictha_newstatus = 1;
	return( ICTHA_SUCCESS );
}

/*
 * ictha_status_upd()
 *
 * Calling/Exit State:
 *
 * Description:
 */
void
ictha_status_upd(ctrlptr, driveptr )
CtrlPtr ctrlptr;
DrivePtr driveptr;
{
	struct ictha_ex *exp;

	if( ctrlptr->ictha_reles) {
		ctrlptr->ictha_reles = 0;
		ictha_rdstatus( ctrlptr);
	}
	exp = &ictha_ex[0];
	if( ((ctrlptr->ictha_status_buf[ 0 ] & exp->mask0 ) == exp->byte0)  &&
	    ((ctrlptr->ictha_status_buf[ 1 ] & exp->mask1 ) == exp->byte1) ) {
		/* NO Cartridge */
		driveptr->drv_nocartridge = 1;
		driveptr->drv_drverror = 10;
	}

	exp++;
	if( ((ctrlptr->ictha_status_buf[ 0 ] & exp->mask0 ) == exp->byte0)  &&
	    ((ctrlptr->ictha_status_buf[ 1 ] & exp->mask1 ) == exp->byte1) ) {
		driveptr->drv_at_eom = 1;
	}
	else {
		driveptr->drv_at_eom = 0;
	}

	exp++;
	if( ((ctrlptr->ictha_status_buf[ 0 ] & exp->mask0 ) == exp->byte0)  &&
	    ((ctrlptr->ictha_status_buf[ 1 ] & exp->mask1 ) == exp->byte1) ) {
		driveptr->drv_at_filemark = 1;
	}
	else {
		driveptr->drv_at_filemark = 0;
	}
}

/*
 * ictha_clrstatus()
 *
 * Calling/Exit State:
 *
 * Description:
 */
void
ictha_clrstatus (stat_buf)
unsigned char *stat_buf;
{
	int i;
	/*
	** Clear the status buffer.
	***/
	for( i = 0; i < ICTHA_STATBUF_SZ; ++i ) {
		*stat_buf++ = '\0';
	}
}

/*
 * ictha_clrflags()
 *
 * Calling/Exit State:
 *
 * Description:
 */
void
ictha_clrflags (ctrlptr, driveptr)
CtrlPtr ctrlptr;
DrivePtr driveptr;
{
	driveptr->drv_at_filemark = 0;
	driveptr->drv_at_eom = 0;
	ctrlptr->ictha_statrdy = 0;
}

/*
 * ictha_gen_intr()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	generate an interrupt from the command side 
 *	to complete (unsuccessfully the request).
 */
void
ictha_gen_intr(ctrlptr, driveptr)
CtrlPtr ctrlptr;
DrivePtr driveptr;
{
#ifdef ICTHA_DEBUG
	cmn_err ( CE_CONT, "ictha_gen_intr: cmd 0x%x\n", driveptr->drv_drvcmd);
#endif
	ctrlptr->ictha_datardy = 0;
#ifdef ICTHASTAT
	ctrlptr->ictha_savecmd = 0;
#endif
	driveptr->drv_savecmd = 0;
	ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_intr_enable );

	if( ctrlptr->ictha_type == ICTHA_WANGTEK ) {
		ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_online );
	}

	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_reset );

	drv_usecwait( ictha_waitcnt );

	ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_reset );

	drv_usecwait( ctrlptr->ictha_por_delay );
	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
	drv_usecwait( ictha_waitcnt );

	/* Initiate a read status to generate an interrupt */
	ictha_rdstat_setup(ctrlptr);
	ctrlptr->ictha_statrdy = 1;
	driveptr->drv_flags |= CFLG_INT;
}
#ifdef PDI_SVR42
/*
 * ictha_selfmt - select format for read/writing.  Choices include:
 *	QIC-24 format for reading
 *	QIC-120 format for writing(15 track)
 *	QIC-150 format for writing(18 track)
 * (currently not called, so commented out - future ioctl may need it)
 

ictha_selfmt(ctrlptr, cmd)
CtrlPtr ctrlptr;
int cmd;
{
#ifdef ICTHA_DEBUG
	cmn_err ( CE_CONT, "ictha_selfmt: cmd 0x%x\n", cmd);
#endif
	outb( ctrlptr->ictha_command, cmd );

	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_request );

	if( wait_til_ready(ctrlptr, ictha_wait_limit, 7 ) == ICTHA_FAILURE ) {
		return( 1 );
	}

        ICTHA_DEASSERT(ctrlptr, ctrlptr->ictha_request );

        if( wait_til_not_ready(ctrlptr, ictha_wait_limit ) == ICTHA_FAILURE ) {
                return( 1 );
        }
	if( wait_til_ready(ctrlptr, ictha_wait_limit, 8 ) == ICTHA_FAILURE ) {
		return( 1 );
	}

	return( 0 );
}
end of commented code */
#endif


#ifdef ICTHA_DEBUG
/*
** DEBUG ROUTINES 
*/
/*
 * ictha_disp_cntrl()
 *
 * Calling/Exit State:
 *
 * Description:
 */
ictha_disp_cntrl(ctrlptr)
CtrlPtr ctrlptr;
{
	cmn_err ( CE_CONT, "ictha_disp_cntrl:\n");
	cmn_err ( CE_CONT, "ctrlptr->ictha_cntrl_mask: 0x%x\n",
	    (int)ctrlptr->ictha_cntrl_mask);
	cmn_err ( CE_CONT, "register: 0x%x\n",
	    (int)inb(ctrlptr->ictha_control));

	return( 0 );
}
#endif

/*
 * ictha_inquir()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	inquiry command routine.
 */
/* ARGSUSED */
void
ictha_inquir(driveptr, c, t, sb)
DrivePtr driveptr;
int	c, t;
struct sb *sb;
{
	struct scs *scb = (struct scs *) (void *)sb->SCB.sc_cmdpt;
	int	i;

	/* The only lun that we use is 0 all others are invalid */
	if (t != ICTHA_HBA_TARGET) {
		bcopy((char *)&driveptr->drv_inqdata, sb->SCB.sc_datapt , scb->ss_len);
		sb->SCB.sc_comp_code = SDI_ASW;
	}
	else {
		struct ident inq;
		struct ident *inq_data;
		int inq_len;

		bzero(&inq, sizeof(struct ident));
		(void)strncpy(inq.id_vendor, icthaidata[c].name, 
			VID_LEN+PID_LEN+REV_LEN);
		inq.id_type = ID_PROCESOR;

		inq_data = (struct ident *)(void *)sb->SCB.sc_datapt;
		inq_len = sb->SCB.sc_datasz;
		bcopy((char *)&inq, (char *)inq_data, inq_len);
		sb->SCB.sc_comp_code = SDI_ASW;
	}
}

/*
 * ictha_freeblk()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Free block allocated with ictha_getblk(). Called by SDI
 */
/* ARGSUSED */
STATIC long 		
HBAFREEBLK(struct hbadata *blk)
{
	kmem_free(blk, sizeof(struct ictha_xsb));
	return(0);
}
/*
 * ictha_getblk()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	allocate a block. Called by SDI
 */
STATIC struct hbadata	* 
HBAGETBLK(int sleepflag)
{
	struct ictha_xsb *blk;
#if (PDI_VERSION <= PDI_UNIXWARE11)
	int sleepflag = KM_SLEEP;
#endif /* (PDI_VERSION <= PDI_UNIXWARE11) */

	blk  = (struct ictha_xsb *) kmem_alloc(sizeof(struct ictha_xsb),
	    sleepflag);
	return((struct hbadata	*) blk);
}
/*
 * ictha_getinfo()
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC void
HBAGETINFO(sa, getinfo)
struct scsi_ad *sa;
struct hbagetinfo *getinfo;
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";

	s1 = temp;
	s2 = getinfo->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;
	getinfo->iotype |= F_DMA_24;
#if (PDI_VERSION >= PDI_SVR42MP)
	if (getinfo->bcbp) {
		getinfo->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfo->bcbp->bcb_flags = BCB_PHYSCONTIG;
		getinfo->bcbp->bcb_max_xfer = icthahba_info.max_xfer;
		getinfo->bcbp->bcb_physreqp->phys_align = 2;
		getinfo->bcbp->bcb_physreqp->phys_boundary = 128 * 1024;
		getinfo->bcbp->bcb_physreqp->phys_dmasize = 24;
	}
#endif /* PDI_VERSION >= PDI_SVR42MP */
}

/*
 * ictha_xlat()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Called by SDI.
 */
/* ARGSUSED */
STATIC HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	struct ictha_xsb *blk =  (struct ictha_xsb *) hbap;
	blk->procp = procp;
	HBAXLAT_RETURN (SDI_RET_OK);
}
/*
 * ictha_open()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	A stub routine.
 */
/* ARGSUSED */
STATIC	int		
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	return(0);
}
/*
 * ictha_close()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	A stub routine.
 */
/* ARGSUSED */
STATIC	int		
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	return(0);
}
/*
 * ictha_ioctl()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	A stub routine.
 */
/* ARGSUSED */
STATIC	int		
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	return(0);
}

#ifdef ICTHA_DEBUG

/*
 * ictha_wakeup()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	A debugging function for waking up a hung controller.
 */

void
ictha_wakeup()
{
	DrivePtr driveptr;
	CtrlPtr ctrlptr;

	ctrlptr  = &ictha_ctrl[0];
	driveptr = &ctrlptr->ictha_drives[0];

	ctrlptr->ictha_cntr_status = inb( ctrlptr->ictha_status );
	cmn_err ( CE_CONT, "ICTHA_CHK_EXCPT: status is 0x%x, control 0x%x\n",
	    (int) ctrlptr->ictha_cntr_status,
	    (int) ctrlptr->ictha_cntrl_mask);

	ICTHA_SDI_RETURN(driveptr, SDI_ERROR, KM_NOSLEEP);
	ictha_gen_intr(driveptr->drv_ctrlptr, driveptr);
	driveptr->drv_flags = 0;
	driveptr->drv_exception  = 0;
	driveptr->drv_state = DSTA_IDLE;
}
#endif

/*
 * ictha_chk_endjob()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Check different exceptions conditions.
 */
void
ictha_chk_endjob(ctrlptr, driveptr)
CtrlPtr ctrlptr;
DrivePtr driveptr;
{
#ifdef ICTHA_DEBUG
	cmn_err(CE_CONT, "ictha_chk_endjob");
#endif
	if(driveptr->drv_timeid) {
		untimeout(driveptr->drv_timeid);
		driveptr->drv_timeid = 0;
	}
	ctrlptr->ictha_datardy = 0;
	if( driveptr->drv_req.req_sb == NULL) {
#ifdef ICTHA_DEBUG
		if(ictha_debug_flag)
			cmn_err ( CE_CONT, "ICTHA_ENDJOB: sb is NULL");
#endif
		return;
	}
	if(driveptr->drv_at_filemark) {
		if(driveptr->drv_drvcmd == ICTHA_READ) {
#ifdef ICTHA_DEBUG
			cmn_err ( CE_CONT, "ICTHA_ENDJOB: reached a filemark on read");
#endif
			if(driveptr->drv_drvcmd == ICTHA_READ && 
		    		driveptr->drv_req.req_resid > 0 &&
		    		driveptr->drv_req.req_blkptr->blk_copyflag) {
				bcopy (
		 			driveptr->drv_dmavbuf, 
					driveptr->drv_req.req_blkptr->blk_vaddr1,
		 			ictha_blocksize);
				}
			driveptr->drv_req.req_resid--;
			driveptr->drv_cmds_ack++;
			driveptr->drv_req.req_sb->SCB.sc_status = S_CKCON;
			ICTHA_SDI_RETURN(driveptr, SDI_CKSTAT, KM_NOSLEEP);
		} else if(driveptr->drv_drvcmd == ICTHA_RDFILEM) {
#ifdef ICTHA_DEBUG
			cmn_err ( CE_CONT, "ICTHA_ENDJOB: reached a filemark on RDFILEM");
#endif
			ICTHA_SDI_RETURN(driveptr, SDI_ASW, KM_NOSLEEP);
		}
#ifdef ICTHA_DEBUG
		else cmn_err(CE_CONT, "ICTHA_ENDJOB: reached a filemark on %d",
		    driveptr->drv_drvcmd);
#endif
	}
	else if (driveptr->drv_at_eom &&
		 driveptr->drv_drvcmd == ICTHA_WRITE)	{
		driveptr->drv_req.req_resid--;
		driveptr->drv_cmds_ack++;
		driveptr->drv_req.req_sb->SCB.sc_status = S_CKCON;
		ICTHA_SDI_RETURN(driveptr, SDI_CKSTAT, KM_NOSLEEP);
	}
	else if(driveptr->drv_sleepflag) {
		ictha_timeout(driveptr);
	} else {

#ifdef ICTHA_DEBUG
		if(ictha_debug_flag)
			cmn_err ( CE_CONT, "ICTHA_ENDJOB: reached ERROR");
#endif
		ICTHA_SDI_RETURN(driveptr, SDI_ERROR, KM_NOSLEEP);
		ictha_gen_intr(driveptr->drv_ctrlptr, driveptr);
	}
}

/*
 * ictha_cmds_watchdog()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Timeout routine for non-read/write commands.
 */

void	ictha_cmds_watchdog(driveptr)
DrivePtr driveptr;
{
	pl_t	opri;
	clock_t timenow;
	ulong	elapsedtime;
	
	opri = spldisk();
	drv_getparm(LBOLT, (void *)&timenow);
	elapsedtime = timenow - driveptr->drv_starttime;
	if(driveptr->drv_flags&CFLG_CMD){
	   if(elapsedtime > (driveptr->drv_duration * HZ)){
		ICTHA_SDI_RETURN(driveptr, SDI_ERROR, KM_NOSLEEP);
		ictha_gen_intr(driveptr->drv_ctrlptr, driveptr);
	   } else driveptr->drv_timeid = timeout(driveptr->drv_timeout,
	    		(caddr_t) driveptr, driveptr->drv_duration * HZ);
	} else  driveptr->drv_timeid = 0;
	splx(opri);
}

/*
 * ictha_cmds_watchdog()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Timeout routine for read/write commands.
 */

void	
ictha_rdwr_watchdog(driveptr)
DrivePtr driveptr;
{
	pl_t	opri;
	clock_t timenow;
	ulong	elapsedtime;
	
	opri = spldisk();
	drv_getparm(LBOLT, (void *)&timenow);
	elapsedtime = timenow - driveptr->drv_starttime;
	if(elapsedtime > (driveptr->drv_duration * HZ) &&
	   driveptr->drv_cmds_sent != driveptr->drv_cmds_ack){
		ICTHA_SDI_RETURN(driveptr, SDI_ERROR, KM_NOSLEEP);
		ictha_gen_intr(driveptr->drv_ctrlptr, driveptr);
		driveptr->drv_timeid = 0;
		driveptr->drv_savecmd = 0;
		splx(opri);
		return;
	}
	driveptr->drv_timeid = timeout(driveptr->drv_timeout,
	    (caddr_t) driveptr, driveptr->drv_duration * HZ);
	splx(opri);
}

/*
 * ictha_wait_int()
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Wait until the last read ahead request is done.
 */

int
ictha_wait_int(driveptr, duration)
DrivePtr driveptr;
ulong	duration;
{
	int ret = 0, time_id;
	CtrlPtr  ctrlptr = driveptr->drv_ctrlptr;
	pl_t opri;

	if((inb(ctrlptr->ictha_status)&ctrlptr->ictha_ready) == 0) {
		return(ret);
	}
	opri = spldisk();
	driveptr->drv_flags |= CFLG_INT;
	driveptr->drv_sleepflag = 1;
	ICTHA_ASSERT(ctrlptr, ctrlptr->ictha_intr_enable );
	time_id = timeout(ictha_timeout, (caddr_t) driveptr, duration*HZ);
	while( driveptr->drv_sleepflag ) {
		if(ret = sleep((caddr_t)&driveptr->drv_sleepflag,
			    PCATCH|(PZERO+1) ))
			driveptr->drv_sleepflag = 0;
		}
	untimeout( time_id );
	splx(opri);
	return(ret);
}


#if (PDI_VERSION >= PDI_UNIXWARE20)
/*
 * ictha_verify(rm_key_t key)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Verify the board instance.
 */
int
ictha_verify(rm_key_t key)
{
	HBA_IDATA_STRUCT	idata;
	struct	ictha_ctrl	ctrl;
	int	rv;

	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	rv = sdi_hba_getconf(key, &idata);
	if (rv != 0)	{
		return (rv);
	}

	ictha_hacnt = 0;
	ictha_init_time = 1;
	ictha_wait_limit = ictha_init_wait_limit;
	ictha_pagesize = (ulong) ptob(1);
	ictha_pagemask = ictha_pagesize - 1;
       
	/*
	 * Verify hardware parameters in vfy_idata,
	 * return 0 on success, ENODEV otherwise.
	 */
	if (ictha_bdinit(&ctrl, &idata) == 0)	{
		return (ENODEV);
	}

	return (0);
}

/*
 * ictha_mca_conf(HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Read the MCA POS registers to get the hardware configuration.
 */
int
ictha_mca_conf(HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen)
{
	unsigned	char	pos[3];

	if (cm_read_devconfig(idp->idata_rmkey, 2, pos, 3) != 3) {
		cmn_err(CE_CONT,
			"!%s could not read POS registers for MCA device",
			idp->name);

		return (1);
	}

	/*
	 * Interpret POS data and populate idp accordingly.
	 */

	idp->ioaddr1	= (ulong)((pos[1] << 8) | (pos[0] & 0xfe));
	idp->iov	= (int)(pos[2] >> 4);
	idp->dmachan1	= (int)(pos[2] & 0x0f);

	idp->idata_memaddr = 0;

	*ioalen = 2;
	*memalen = 0;

	return (0);
}
#endif

#ident	"@(#)kern-pdi:io/hba/efp2/efp2.c	1.21.4.2"
/*	Copyright (C) Ing. C. Olivetti & C. S.p.a., 1991, 1992.*/
/*	All Rights Reserved	*/

/*
 *	SCSI Host Adapter Driver for Olivetti EFP2/ESC-2  controller
 */
#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <acc/priv/privilege.h>
#include <io/mkdev.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <svc/bootinfo.h>
#include <util/debug.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/dynstructs.h>
#include <util/mod/moddefs.h>

#if	PDI_VERSION >= PDI_SVR42MP
#include <util/ksynch.h>
#endif	/* PDI_SVR42 */

#ifdef DDICHECK
#include  <io/ddicheck.h>
#endif

#ifdef CALLDEBUG
#include <util/kdb/xdebug.h>
#endif

#include <io/hba/efp2/efp2.h>

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#else /* _KERNEL_HEADERS */
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/privilege.h>
#include <sys/mkdev.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/bootinfo.h>
#include <sys/cm_confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/debug.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/dynstructs.h>
#include <sys/moddefs.h>

#include <sys/ksynch.h>

#ifdef DDICHECK
#include  <sys/ddicheck.h>
#endif

#ifdef CALLDEBUG
#include <sys/xdebug.h>
#endif /* CALLDEBUG */

#include "./efp2.h"

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif	 /* _KERNEL_HEADERS */


#define         DRVNAME "esc2-efp2 - Olivetti SCSI HBA driver"

#define EFP2_BLKSHFT	  9	/* PLEASE NOTE:  Currently pass-thru	    */
#define EFP2_BLKSIZE	512	/* SS_READ/SS_WRITE, SM_READ/SM_WRITE	    */
				/* supports only 512 byte blocksize devices */
int efp2_unload(void), efp2_load(void);
STATIC void efp2_putq(int , int , sblk_t *),efp2_int(struct sb *); 
int efp2_ha_present(int );
long	efp2_next(int, int, pl_t);
int efp2_ha_intr (int );
STATIC void efp2_pass_thru(buf_t *bp);


#if PDI_VERSION < PDI_SVR42MP
typedef ulong_t vaddr_t;
#define KMEM_ZALLOC kmem_zalloc
#define KMEM_FREE kmem_free
#define efp2_pass_thru0 efp2_pass_thru

#else /* PDI_VERSION < PDI_SVR42MP */
STATIC void efp2_pass_thru0(struct buf *bp);
STATIC void *efp2_kmem_zalloc_physreq (size_t size, int flags);

#define KMEM_ZALLOC efp2_kmem_zalloc_physreq
#define KMEM_FREE kmem_free

#define EFP_MEMALIGN	16
#define EFP_BOUNDARY	0
#endif /* PDI_VERSION >= PDI_SVR42MP */

extern struct ver_no    efp2_sdi_ver;            /* SDI version structure    */

/* The following are allocated in the space file */
extern int			efp2_cnt;	/* Total # of controllers*/
extern HBA_IDATA_STRUCT _efp2idata[];
HBA_IDATA_STRUCT *saveidata;
HBA_IDATA_STRUCT	*efp2idata;

/* The sm_poolhead struct is used for the dynamic struct allocation routines */
/* as pool to extarct from and return structs to. THis pool provides 28 byte */
/* structs (the size of sblk_t structs roughly).                             */

extern struct head      sm_poolhead;

int efp2_present = 0;	/* number of boards found */
int efp2_hacnt = 0;	/* number of boards registered */

STATIC lock_t *efp2_dmalist_lock;
STATIC lock_t *scsi_lock[MAX_HAS];
STATIC LKINFO_DECL(lkinfo_efp2dmalist, "IO:efp2:efp2_dmalist_lock", 0);
#define EFP2_DMALIST_LOCK(p) 	(p) = LOCK(efp2_dmalist_lock, pldisk)
#define EFP2_DMALIST_UNLOCK(p) 	UNLOCK(efp2_dmalist_lock, (p))

#if	PDI_VERSION > PDI_SVR42MP
int    efp2devflag = HBA_MP;
#else	/* VERSION < PDI_SVR42MP */
int	efp2devflag;
#endif /* PDI_SVR42 */


MOD_HDRV_WRAPPER(efp2, efp2_load, efp2_unload, NULL, DRVNAME);
HBA_INFO(efp2, &efp2devflag, 0x8000);

/* This struct is compiled at initialization time, during the scansion of
 * all the board present on the ESIA bus.
 */
typedef struct board {
	char          board_name[12];	/* ASCII identification string */
	char	      boot_flag;	/* Boolean ON bootable OFF not-bootable*/
	EFP2_DATA     *board_datap[2];	/* Pointer to controller data area */
	int	      lcntrl;		/* logical cntrl Number */
	int	      board_slot;	/* position on EISA bus */
	int 	      n_channel;	/* Logical Identification of Channel */
	int	      base_io_addr;	/* I/O address */
} board ;

struct board	Efpboard[MAX_HAS];
int  Board_cnt = 0;

#define FIRST_CHANNEL 	0
#define SECOND_CHANNEL 	1

#ifdef MIRROR
/*MIRROR*/
#define MIRRTABENTRY  ((4*MAX_TCS*1) /2)  /* the correct value is :  */
					  /* MAX_HAS * MAX_TCS * MAX_LUS */
	struct  {
		char    c;
		char    t;
		char    l;
		char    type;   /* 0 = null entry */
				/* 1 = source disk busy */
				/* 2 = mirrored disk busy */
				/* 0x8x = error already logged */

	} mirr_tab[MIRRTABENTRY];

void efp2_mirtime();

int     efp2_mirrtimeout = 0;
#endif MIRROR
/*MIRROR END*/


int  efp2_gtol[MAX_HAS]; 	/* xlate global hba# to loc */
boolean_t efp2init_time;	/* Poll flag in initialization time */
int	efp2_asym = EFP2_FALSE;

static dma_t            *efp2_dfreelist;    /* List of free DMA lists   */
STATIC sv_t   		*efp2_dmalist_sv;   /* sleep argument for UP        */

/* Boot from every controller */
int	efp2_scsi_boot_slot =-1;
int	efp2_scsi_boot_id =-1;
int	efp2_scsi_boot_chan =-1;

LKINFO_DECL(lkinfo_scsi, "IO:efp2:efp2cq_lock", 0);
LKINFO_DECL(lkinfo_efp2ha, "IO:efp2:efp2ha_lock", 0);
LKINFO_DECL(lkinfo_efp2mbx, "IO:efp2:efp2mbx_lock", 0);
LKINFO_DECL(lkinfo_efp2cq, "IO:efp2:efp2cq_lock", 0);
LKINFO_DECL(lkinfo_efp2_scsi, "IO:efp2:efp2scsi_lock", 0);
LKINFO_DECL(lkinfo_replyq_lock, "IO:efp2:replyq_lock", 0);

int		efp2_setmode(EFP2_DATA *, int),
		efp2_put_cmd(),
		efp2_get_reply();
queue_entry_t		*efp2_get_qe();
configuration_entry_t	*efp2_get_ce();

/* external and exported symbols */
#ifdef OLI_SVR40
extern void	(* onoff_func)();
void		efp2_onoff();
#endif

/* Error messages */
char	*efp2_enomem	= "Cannot allocate memory.";
char	*efp2_enocont	= "Cannot allocate contiguous memory.";
char	*efp2_eput_cmd	= "Error in efp2_put_cmd()";
char	*efp2_epoll	= "Error in efp2_ha_poll()";
char	*efp2_eset	= "Error in efp_set()";
char	*efp2_estart	= "Error in efp_start()";
char	*efp2_ewarning	= "Error in efp_warning()"; 

int efp2ccount = 0;

int tracecmd= 0;
int printlock= 1;

#ifdef  EFPDEBUG
void printBoard(void), printIdata(HBA_IDATA_STRUCT *);
int     efp2_print_full = 1;
#define PRINTF(parms)  if (efp2_print_full) printf parms
#ifdef  TRACE_IDX
#define MAX_IDS 256
int     efp2_id[MAX_IDS];
#endif  /* TRACE_IDX */
#else   /* EFPDEBUG */
#define PRINTF(parms)
#endif  /* EFPDEBUG */

#ifdef DEBUG_TRACE
int deb_print = 1;
#define TRACE(parms)   if (deb_print) printf parms
#else   /* DEBUG_TRACE */
#define TRACE(parms)
#endif  /* DEBUG_TRACE */

#ifdef MTDEBUG
#define MTDEB_BUFSZ     0x800
#define EFPDEB(c) efp_trace(c)
#else
#define EFPDEB(c)
#endif /* MTDEBUG */


#ifdef	CALLDEBUG
int	efp2_calldeb = 0;
#define CALLDEB()	calldebug()
#else
#define CALLDEB()	
#endif


#define SC(x,b)	Efpboard[x].board_datap[b]
/* It was...
***	EFP2_DATA 	*efp2data[SDI_MAX_HBAS*2];
***	#define SC(x)	efp2data[x]
***/
static  int     efp2_dynamic = 0;


/*
**	efp2_load : entry point for the loading of the driver 
*/
STATIC int
efp2_load(void)
{
        efp2_dynamic = 1;

	if(efp2init()) {	/* Was .. checking minus 1 */
		/*
		** Free the previous allocated areas (inside efp2init)
		**
		*/
		 sdi_acfree(efp2idata, efp2_cnt); 
		return  ENODEV;
	}

	if (efp2start() ) {
		sdi_acfree(efp2idata, efp2_cnt); 
		return  ENODEV;
	}	
        return (0);
}

/*
**	efp2_unload : entry point for the unloading of the driver 
*/

STATIC int
efp2_unload(void)
{
        return(EBUSY);
}




/*============================================================================
** Function name: efp2init()
** Description:
**	This function resets the HA board and
**	initializes the communication queues.
**
** SMP: Initialize ha spin-lock and mailbox command queue spin-lock
*/

int efp2init() 
{
	int sleepflag, found;
	int bus_found;
	int		c,i, t, l;
	int datacnt;
	uint 		bus_p;
	EFP2_DATA	*had;
	register dma_t  *dp;
	void efp2intr(uint );
	dma_t           *dmalist;      /* pointer to DMA list pool */

	sleepflag = efp2_dynamic ? KM_SLEEP : KM_NOSLEEP;

	datacnt = found = 0;
	had = (EFP2_DATA *)NULL;
	dmalist = (dma_t *)NULL;

	 /* if not running in an EISA machine, skip initialization */
        if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p != BUS_EISA))
                return(-1);

  	/*
        ** Allocate and populate a new idata array based on the
        ** current hardware configuration for this driver.
        */

	efp2idata = sdi_hba_autoconf("efp2", _efp2idata, &efp2_cnt);
	if(efp2idata == NULL) {
                cmn_err(CE_WARN,"!efp2_init sdi_hba_autoconf failed (%x,%d)",
                                _efp2idata,efp2_cnt);
                return(-1);
	}
	
	HBA_IDATA(efp2_cnt);

	efp2init_time = EFP2_TRUE;

        for(i = 0; i < MAX_HAS; i++)
                efp2_gtol[i] = -1;

        efp2_sdi_ver.sv_release = 1;
	efp2_sdi_ver.sv_machine = SDI_386_EISA;
        efp2_sdi_ver.sv_modes   = SDI_BASIC1;

        datacnt = efp2_cnt * ( NDMA * (sizeof(dma_t)));

        if( (dmalist = (dma_t *)KMEM_ZALLOC(datacnt, sleepflag) ) == NULL) {
                efp2_err(CE_WARN, "dmalist",8,8,8);
                return (-1);
        }

        /* Build list of free DMA lists */
        efp2_dfreelist = NULL;
        for (i = 0; i < NDMA; i++) {
                dp = &dmalist[i];
                dp->d_next = efp2_dfreelist;
                efp2_dfreelist = dp;
        }

	efp2_present = efp2_ha_present(sleepflag);

#ifdef EFPDEBUG
        printBoard();
#endif

        if ( efp2_present == 0 ) {
                cmn_err(CE_WARN,"!No efp2 board found on EISA bus");
                KMEM_FREE(dmalist,(NDMA * (sizeof(dma_t))));
                return (-1);

        }

	efp2_dmalist_sv = SV_ALLOC (sleepflag);
	efp2_dmalist_lock = LOCK_ALLOC (EFP2_HIER, 	
				        pldisk, 
					&lkinfo_efp2dmalist, 
					sleepflag);


	sdi_intr_attach(efp2idata, efp2_cnt, efp2intr, efp2devflag);
	return(0);  
}

int efp2start( void ) 
{
	int		c,i, t, l;
	pl_t		oip;
	int cntl_num;
	int datacnt;
	int retcode = -1;	


	oip = spldisk();

	for (c = 0; c < efp2_cnt; c++) {

	    if(efp2idata[c].active == 0)
		continue;

	   /* Get an HBA number from SDI and Register HBA with SDI */
	   if( (cntl_num = sdi_gethbano(efp2idata[c].cntlr)) <= -1) {
		efp2_err(CE_WARN,"No HBA number available c=%d.", c);
	   	efp2idata[c].active = 0;
		continue;
	    }

	    TRACE( ("sdi_gethbano ret(%d) for logical cntlr %d count %d",
			SC(c,0)->ha_lhba, efp2idata[c].cntlr, c) );

	    efp2idata[c].cntlr = cntl_num;
	    efp2_gtol[cntl_num] = c;

#ifdef EFPDEBUG
	    (void)printIdata(&efp2idata[c]);
#endif
	    if( sdi_register(&efp2hba_info, &efp2idata[c]) < 0) {
		cmn_err(CE_NOTE,
		"!SDI register failed on controller %d(%x) using entrypoint #%d",
			c,&efp2idata[c], efp2idata[c].cntlr);

	   	efp2idata[c].active = 0;
		continue;	
	     } 
	    efp2_ha_start(c);
	    efp2_hacnt++;
	    retcode = 0;
	}

	if (efp2_hacnt == 0) {
                cmn_err(CE_WARN,"!No efp2 boards registered");
		retcode = -1;
	}

	efp2init_time = EFP2_FALSE;
	splx(oip);
	return retcode ;
}

void fake (HBA_IDATA_STRUCT *Idata)
{ 
	return;
}

/*
** This function perform hardware initialization on efp2/esc2 channel
** it is called by efp2_ha_present for each channel found on both type
** board.
*/
int efp2_ha_init(register int c)
{
	queue_entry_t 	*qe;

	mbox_command_t	mbox_cmd;
	int 		i;
	int	ch; 
	int	tmp;

	register EFP2_DATA *had = (EFP2_DATA*)SC(c,0);
	EFP2_DATA *prev_had;

	int sleepflag = efp2_dynamic ? KM_SLEEP : KM_NOSLEEP;

	int x = 0;

#ifdef MIRROR
/*MIRROR*/
	mirr_tab[0].type = 0;
#endif MIRROR
/*MIRROR END*/

	TRACE(( " Working on had(%d) at address 0x%x chan %d", c, had, had->ha_scsi_channel));
	
	had->ha_state &= ~C_OPERATIONAL;
	had->ha_startlist = had->ha_endlist = 0; 
	had->ha_lenlist = 0;

	for (i=0; i<HA_CTRLS; i++)
		had->ha_ctrl[i] = SCSI_NO_CTRL;

        had->ha_lock = LOCK_ALLOC(EFP2_HIER+1, pldisk, &lkinfo_efp2ha, sleepflag);
	scsi_lock[c] = LOCK_ALLOC(EFP2_HIER, pldisk, &lkinfo_efp2_scsi, sleepflag);
	/* Allocate controller specific structures */

	had->ha_qd = (queue_descr_t *)KMEM_ZALLOC( 
			sizeof(queue_header_t) + (sizeof(queue_entry_t) * MAX_QUEUES),
			sleepflag);

	if (had->ha_qd == NULL) {
		efp2_err(CE_WARN, efp2_enocont,c,8,8);
		return FAIL;
	}

	had->ha_mbox = (command_queue_t *)KMEM_ZALLOC(
		sizeof(command_system_header_t) + 
		sizeof(command_queue_header_t) + 
		(sizeof(command_queue_entry_t) * MBOX_ENTRIES),
		sleepflag);

	if (had->ha_mbox == NULL) {
		efp2_err(CE_WARN, efp2_enocont,c,8,8);
		return FAIL;
	}

	had->ha_rq = (reply_queue_t *)KMEM_ZALLOC(
		sizeof(reply_system_header_t) + 
		(sizeof(reply_queue_t) * RQ_ENTRIES) ,
		sleepflag);

	if (had->ha_rq == NULL) {
		efp2_err(CE_WARN, efp2_enocont,c,8,8);
		return FAIL;
	}

	had->ha_qd->qd_header.qh_n_cmd = 1;	/* Only mbox queue */
	had->ha_qd->qd_header.qh_maintenance = NO;	/* User environment */
	had->ha_qd->qd_header.qh_type_reply = INTR_EACH_REPLY;
	had->ha_qd->qd_header.qh_rep_queue = 

	/* was kvtophys */
	vtop((caddr_t)&had->ha_rq->rq_entry, NULL);

	had->ha_qd->qd_header.qh_n_reply = RQ_ENTRIES;
	qe = had->ha_qd->qd_entry + MBOX_QUEUE;
	qe->qe_scsi_level = MBOX_PROT;
	qe->qe_channel = MBOX_CHAN;
	qe->qe_id = MBOX_ID;
	qe->qe_lun = MBOX_LUN;
	qe->qe_n_cmd = MBOX_ENTRIES;
	qe->qe_notfull_intr = YES;
	qe->qe_no_ars = YES;
	qe->qe_rfu1 = 0;

	/* WAS ..
	** qe->qe_cmd_queue = kvtophys((caddr_t)&had->ha_mbox->cq_header);
	*/

	qe->qe_cmd_queue = vtop((caddr_t)&had->ha_mbox->cq_header, NULL);

	had->ha_mbox->cq_sys_header.sh_lock = 
		LOCK_ALLOC(EFP2_HIER, pldisk, &lkinfo_efp2mbx, sleepflag);

	had->ha_mbox->cq_sys_header.sh_flag = 0;
	had->ha_mbox->cq_sys_header.sh_size = MBOX_ENTRIES;
	had->ha_mbox->cq_sys_header.sh_iobase = EFP2_BASE(c);
	had->ha_mbox->cq_sys_header.sh_maxq = 0;

	had->ha_mbox->cq_header.cq_get = had->ha_mbox->cq_header.cq_put = 0;

	had->ha_rq->rq_sys_header.sh_full = EFP2_FALSE;
	had->ha_rq->rq_sys_header.sh_size = RQ_ENTRIES;
	had->ha_rq->rq_sys_header.sh_get = 0;
	had->ha_rq->rq_sys_header.sh_maxr = 0;
	had->ha_rq->rq_sys_header.sh_iobase = EFP2_BASE(c);

	/* Set controller in EFP2 mode and     */
	/* start communication with controller */

	if (efp2_setmode(had, c)==FAIL) {
		cmn_err(CE_WARN,"efp_ha_init : efp2_setmod failed on (%d)", c);
		return FAIL;
	}

	/* Get controller specific informations */

	mbox_cmd.mc_header.ch_userid = 0x12340000;	/* Easy to find */
	mbox_cmd.mc_header.ch_sort = NO;
	mbox_cmd.mc_header.ch_prior = HIGHEST_PRIOR;
	mbox_cmd.mc_header.ch_mod = NORMAL_MOD;
	mbox_cmd.mc_header.ch_cmd_type = MBX_GET_INFO;

	mbox_cmd.mc_length = 0;
	mbox_cmd.mc_user_data[0] = 0;
	mbox_cmd.mc_user_data[1] = 0;
	mbox_cmd.mc_user_data[2] = 0;
	mbox_cmd.mc_user_data[3] = 0;
	mbox_cmd.mc_address = NULL;

	had->ha_mbox_reply.mr_status = -1;
	had->ha_mbox_reply.mr_flag = 0;

	PRINTF(("EFP: efp2_put_cmd(MBX_GET_INFO);\n"));

	if (efp2_put_cmd(had, had->ha_mbox, &mbox_cmd, 1) == EFP2_FALSE) {
		efp2_err(CE_WARN, efp2_eput_cmd, c,8,8);
		return FAIL;
	}

	if (efp2_ha_poll(c, 2*ONE_MILL_U_SEC,EFP2_TRUE)!=PASS) { /* wait 2 seconds */
		efp2_err(CE_WARN, efp2_epoll, c,8,8);
		return FAIL;
	}

	if (had->ha_mbox_reply.mr_status != CORRECT_END) {
		efp2_err(CE_WARN, "MBX_GET_INFO returned %x", c,8,8, 
					had->ha_mbox_reply.mr_status);
		return FAIL;
	}

	had->ha_info = *((get_information_t *)
			&had->ha_mbox_reply.mr_appl_field);

#ifdef EFPDEBUG
	{
	char *s, fw[4];

	fw[3] = 0;
	fw[2] = had->ha_info.gi_fw_rel[0];
	fw[1] = had->ha_info.gi_fw_rel[1];
	fw[0] = had->ha_info.gi_fw_rel[2];

	PRINTF(("Controller specific informations : "));
	PRINTF(("\tFirmware release... : %x\n", *((long*)fw)));
	PRINTF(("\tSCSI level ........ : %d\n", had->ha_info.gi_scsi_level));
	PRINTF(("\tEnvironment ....... : %d\n", had->ha_info.gi_environment));
#ifdef MIRROR
/*MIRROR*/
	switch (had->ha_info.gi_environment)
	{
	case MIRRSINGLE0 :
		PRINTF(("\tSingle bus mirror on channel 0\n"));
	case MIRRSINGLE1 :
		PRINTF(("\tSingle bus mirror on channel 1\n"));
	case MIRRDUAL :
		PRINTF(("\tDual bus  mirror on channel 0-1\n"));
	}
#endif MIRROR
/*MIRROR END*/
	PRINTF(("\tDual long ......... : "));

	switch(had->ha_info.gi_dual_long)
	{
	case ALL_EXTENDED: 
		s = "support dual and 2 entries commands";
		break;
	case NO_DUAL_COMMAND:
		s = "does not support dual commands";
		break;
	case NO_DOUBLE_ENTRIES:
		s = "does not support 2 entries commands";
		break;
	case NO_EXTENDED:
		s = "does not support dual and 2 entries commands";
		break;
	}
	PRINTF(("%s\n", s));
	PRINTF(("\tMax cmd entries ... : %d\n", had->ha_info.gi_max_cmd_ent));
	PRINTF(("\tContr.id on 1st bus : %d\n", had->ha_info.gi_id[0]));
	PRINTF(("\tContr.id on 2nd bus : %d\n", had->ha_info.gi_id[1]));
	PRINTF(("\tContr.id on 3rd bus : %d\n", had->ha_info.gi_id[2]));
	PRINTF(("\tContr.id on 4th bus : %d\n", had->ha_info.gi_id[3]));
	}
#endif /* EFPDABUG */

	had->ha_flag = 0;

	/* use had->ha_info.gi_environment for mirroring */

	switch(had->ha_info.gi_dual_long)
	{
	case ALL_EXTENDED: 
		had->ha_flag |= HA_F_DUAL_COMMAND|HA_F_DOUBLE_ENTRY;
		break;
	case NO_DUAL_COMMAND:
		had->ha_flag |= HA_F_DOUBLE_ENTRY;
		break;
	case NO_DOUBLE_ENTRIES:
		had->ha_flag |= HA_F_DUAL_COMMAND;
		break;
	case NO_EXTENDED:
		/* do nothing */
		break;
	}

	had->ha_l = 0;

	if (efp2_scsi_boot_slot >= 0)	/* Valid boot controller */
		i = efp2_scsi_boot_chan;
	else
		for (i=0; i<HA_CTRLS; i++)
			if (had->ha_info.gi_id[i] != SCSI_NO_ID) break;

	if (i >= 0 && i < HA_CTRLS && had->ha_info.gi_id[i] != SCSI_NO_ID) {  
			/* Valid channel */
			had->ha_ctrl[i] = c;
			had->ha_id = had->ha_info.gi_id[i];
	} else {
		efp2_err(CE_WARN, "No valid channels\n",c,8,8);
		return FAIL;
	}

	had->ha_flag |= HA_F_INIT;			/* Init done */


	/* I've moved those statements here because of the fact 
	** that also this field must be copied to the structures
	** associated to the 2nd channel 
	*/
	had->ha_conf = (configuration_table_t *)
			KMEM_ZALLOC(sizeof(configuration_table_t),
			sleepflag);

	if (had->ha_conf == NULL) {
		efp2_err(CE_WARN, efp2_enocont,c,8,8);
		return FAIL;
	}

#ifdef NOTDEF
	/*
	 * Dual Host
	 */
	if ( (had->ha_nameid == OLI_EFP2) || (had->ha_nameid == OLI_ESC2)) {
		if ( (had->ha_start_addr/0x1000 == efp2_scsi_boot_slot) &&
		     (efp2_get_chan(c, channel) == efp2_scsi_boot_chan) ) {
			if (had->ha_id != MASTER_ID)
				efp2_asym = EFP2_TRUE;
		}
	}
#endif
	had->ha_state    |= C_OPERATIONAL;

	/*
	 * - Initializing other SCSI bus, we get needed information
	 *   from previous controller.
	 */

	/* Marco:
	** Adding manage of ESC2 board.
	** It has NO bi-channel
	*/
	if(had->ha_nameid != OLI_ESC2)  {
		prev_had = had;		/* Save previuos Channel Data Pointer */
		had++;			/* Set to next channel */

		TRACE(("Setting had at %x",had));

                /*
                 * Check if board has uninitialized channels
                 */

                for (i=0; i<HA_CTRLS; i++) {
                        if ((prev_had->ha_info.gi_id[i] != SCSI_NO_ID)
                        && (prev_had->ha_ctrl[i] == SCSI_NO_CTRL)) {
                                /*
                                 * Valid channel, Not initialized
                                 */
                                prev_had->ha_ctrl[i] = c;
                                had->ha_id = prev_had->ha_info.gi_id[i];
                                break;
                        }
                }

                if (i>=HA_CTRLS) {
                        efp2_err(CE_WARN, "Too many HA on this board.",c,8,8);
                        return FAIL;
                }

                /* mark this HA operational */
                *SC(c, 1) = *SC(c, 0);         /* Copy structure 4 2nd Channel */

	}
	efp2idata[c].active = 1;
	return PASS;
}


/*============================================================================
** Function name: efp2_ha_post_init()
** Description:
**	This function performs post initialization actions.
*/
int efp2_ha_post_init(EFP2_DATA *had, register int c)
{
	configuration_entry_t	*ce;
	queue_entry_t 		*qe;
	command_queue_t 	*cq, **pcq;
	mbox_command_t 		mbox_cmd;
	int i, t, l;
	caddr_t addr;
	int size;

	had->ha_state &= ~C_OPERATIONAL;

	mbox_cmd.mc_header.ch_userid = 0x12340001;	/* Easy to find */
	mbox_cmd.mc_header.ch_sort = NO;
	mbox_cmd.mc_header.ch_prior = HIGHEST_PRIOR;
	mbox_cmd.mc_header.ch_mod = NORMAL_MOD;
	mbox_cmd.mc_header.ch_cmd_type = MBX_GET_CONFIG;

	mbox_cmd.mc_length = sizeof(configuration_table_t);
	mbox_cmd.mc_user_data[0] = 0;
	mbox_cmd.mc_user_data[1] = 0;
	mbox_cmd.mc_user_data[2] = 0;
	mbox_cmd.mc_user_data[3] = 0;
	mbox_cmd.mc_address = 

	vtop((caddr_t)had->ha_conf, NULL);

	had->ha_mbox_reply.mr_status = -1;
	had->ha_mbox_reply.mr_flag = 0;

	PRINTF(("EFP: efp2_put_cmd(MBX_GET_CONFIG);\n"));

	if (efp2_put_cmd(had, had->ha_mbox, &mbox_cmd, 1) == EFP2_FALSE) {
		efp2_err(CE_WARN, efp2_eput_cmd, c,8,8);
		return FAIL;
	}

	if (efp2_ha_poll(c, 120*ONE_MILL_U_SEC,EFP2_TRUE)!=PASS) 
							/* wait 2 minutes */
	{
		efp2_err(CE_WARN, efp2_epoll, c,8,8);
		return FAIL;
	}
	
	if (had->ha_mbox_reply.mr_status != CORRECT_END) {
		efp2_err(CE_WARN, "MBX_GET_CONFIG returned %x\n", 
				c,8,8, had->ha_mbox_reply.mr_status);
		return FAIL;
	}

	had->ha_l = l = 
		had->ha_mbox_reply.mr_length/sizeof(configuration_entry_t);

#ifdef EFPDEBUG
	{
	PRINTF(("Devices configuration :\n\n"));
	PRINTF(("Dev	Dev	SCSI\n"));
	PRINTF(("type   qual    lev     Env     Chan    Id      Lun\n"));
	PRINTF(("====   ====    ====    ===     ====    ==      ===\n"));

	if(had->ha_conf == NULL)  {
		EFPDEB('a');
		cmn_err(CE_PANIC,"efp2_ha_post_init");
	} else
		cmn_err(CE_WARN,"ha_conf <%x>", had->ha_conf);

	for (ce = had->ha_conf[i=0]; i<l; i++, ce++)
		PRINTF(("%d	%d	%d	%d%d	%d	%d	%d\n", ce->ce_dev_type, ce->ce_dev_qual, ce->ce_scsi_lev, ce->ce_env>>4, ce->ce_env&0x0F, ce->ce_channel, ce->ce_id, ce->ce_lun));
	}
#endif

	for (ce = had->ha_conf[i=0]; i<l; i++, ce++) {
		if (efp2_alloc_queue(c,ce->ce_id,ce->ce_lun,had,
			ce->ce_channel,ce->ce_scsi_lev,ce->ce_dev_type)==FAIL) {
			return FAIL;
		}
	}

	/* Set again controller in EFP2 mode */

	if (efp2_setmode(had, c)==FAIL) {
		cmn_err(CE_WARN,"efp_ha_post_init : efp2_setmod failed on %d", c);
		return FAIL;
	}

	/*
	 * Set queues in NO ARS mode, then target drivers 
	 * (via HA_SETCONF ioctl) can switch in ARS mode.
	 *
	 * efp2_ha_start will set new configurations.
	 */

	l = had->ha_qd->qd_header.qh_n_cmd;

	for (i=CMD_QUEUE, qe = had->ha_qd->qd_entry+i; i<l; i++, qe++) {
		qe->qe_no_ars = YES;
	}

	had->ha_flag |= HA_F_POSTINIT;		/* Postinit done */

	had->ha_state |= C_OPERATIONAL;
	return PASS;
}

/*============================================================================
** Function name: efp2_alloc_queue()
** Description:
**	This function allocate and initialize a command queue
**
** SMP: Initialize command queue spin-lock
*/
int
efp2_alloc_queue(c,t,l,had,ch,scsi_lev,typ)
int		c,t,l;
EFP2_DATA	 *had;
int   ch,scsi_lev,typ;
{
	int	entries, cc;
	command_queue_t *cq;
	queue_entry_t 	*qe;
	int		  q;
	int	found = 0;
	int	n;
	queue_entry_t 	*nqe;
	int bus;

	int sleepflag = efp2_dynamic ? KM_SLEEP : KM_NOSLEEP;

	cc = c;
	if(ch > 1) bus = ch-1; else bus = 0;

	switch(typ)
	{
	case PROCESSOR_DEV :	/* not allocated */
		return PASS;
	case DIRECT_ACCESS :	/* allocate MANY_ENTRIES */
		entries = min(MANY_ENTRIES,had->ha_info.gi_max_cmd_ent);
		break;
	default :		/* allocate FEW_ENTRIES */
		entries = min(FEW_ENTRIES,had->ha_info.gi_max_cmd_ent);
		break;
	}

	if ((qe = efp2_get_qe(SC(cc,bus), cc,t,l)) != NULL) {
		found = 1;
		/* Mi serve il numero della coda */
		for (q=0, 
		     nqe=had->ha_qd->qd_entry, 
		     n=had->ha_qd->qd_header.qh_n_cmd; 
		   q<n; 
		   q++, nqe++) { 
			if (qe == nqe)
				break;
		}

		cq = CMD_Q(cc, bus, t,l);
		KMEM_FREE((caddr_t)cq, 
				sizeof(command_system_header_t) + 
				sizeof(command_queue_header_t) + 
				(sizeof(command_queue_entry_t) * qe->qe_n_cmd));
	} else {

		if ((q=had->ha_qd->qd_header.qh_n_cmd) >= MAX_QUEUES) {
			return FAIL;
		}

		qe = had->ha_qd->qd_entry + q;
	}
	
	cq = (command_queue_t *)KMEM_ZALLOC(
			sizeof(command_system_header_t) + 
			sizeof(command_queue_header_t) + 
			(sizeof(command_queue_entry_t) * entries),
			sleepflag);

	if (cq == NULL) {
		efp2_err(CE_WARN, efp2_enocont,c,8,8);
		return FAIL;
	}

	qe->qe_scsi_level = scsi_lev;
	qe->qe_channel = ch;
	qe->qe_id = t;
	qe->qe_lun = l;
	qe->qe_n_cmd = entries;
	qe->qe_notfull_intr = YES;
	qe->qe_rfu1 = 0;
	qe->qe_no_ars = NO;
	qe->qe_cmd_queue = vtop((caddr_t)&cq->cq_header, NULL);

	cq->cq_sys_header.sh_lock =
		LOCK_ALLOC(EFP2_HIER+1, pldisk, &lkinfo_efp2cq, sleepflag);

	cq->cq_sys_header.sh_flag = 0;
	if (had->ha_flag & HA_F_DUAL_COMMAND)
		cq->cq_sys_header.sh_flag |= QUEUE_DUAL_COMMAND;
	if (had->ha_flag & HA_F_DOUBLE_ENTRY)
		cq->cq_sys_header.sh_flag |= QUEUE_DOUBLE_ENTRY;
	cq->cq_sys_header.sh_size = entries;
	cq->cq_sys_header.sh_queue = q;
	cq->cq_sys_header.sh_iobase = EFP2_BASE(c);
	cq->cq_sys_header.sh_maxq = 0;

	cq->cq_header.cq_get = cq->cq_header.cq_put = 0;

	CMD_Q(cc, bus, t,l) = cq;

#ifdef EFPDEBUG
	cmn_err(CE_WARN,
		"Allocq  %d,%d,%d(ch=%d) CMD_Q(%x)", 
			cc,t,l,ch,CMD_Q(cc,bus, t,l) );
#endif

	if (found == 0)
		had->ha_qd->qd_header.qh_n_cmd++;

	return PASS;
}


/*============================================================================
** Function Name: efp2_ha_poll()
** Description:
**	This routine is used to wait for a completion from the host 
**	adapter. This routine polls the status register on the HA every
**	milli-second. After the interrupt is seen, the HA's interrupt 
**	service routine is manually called. This routine is used primarily
**	at init time before the sleeping is legal.
** NOTE:	
**	This routine allows for no concurrency and as such, should 
**	be used selectivly.
*/
int
efp2_ha_poll(c, usecs_to_wait, where)
int	c, where;
int	usecs_to_wait;	/* microseconds to wait for completion */
{
	register EFP2_DATA *had = (EFP2_DATA *)SC(c, 0);

	while (usecs_to_wait > 0) {
		 if (efp2_ha_intr(c)!=NO_INT_PNDG) { 
			if (where) {
				if (had->ha_mbox_reply.mr_flag == 1) {
					return (PASS);
				}
			} else
				if (had->ha_msg != -1) {
					return (PASS);
				}
		}
		drv_usecwait(ONE_THOU_U_SEC);	/* wait 1 milli-seconds */
		usecs_to_wait -= ONE_THOU_U_SEC;
	}
	return(FAIL);	
}


/*
*       This work around is needed because
*       the xxxintr functions is declared by
*       sdi_hier while compiling whit multi processor
*       flag;
*/
void efp2intr(unsigned int vect)
{
	if(efp2init_time)
		/* This can be happen between the time interrupts are
		 * enabled and efp2start turns off efp2init_time.
		 */
		 return ;

	(void)efp2_ha_intr(vect);
}

int efp2_ha_intr(int vect)
{
	register EFP2_DATA *efp;
	int	i, reply; 
	int	retcode;
	register sblk_t *otherc = (sblk_t *)0;
	int bus;	/* autoconf */

	pl_t pri;
	pl_t oip;

#ifdef MIRROR
	char *contr;
#endif MIRROR

	register int c;

	EFPDEB('I');

	if(efp2init_time) {
		c = vect;		/* Poll During Init Time */
		if ((reply = efp_read_msg(EFP2_BASE(c))) == 0) 
			return NO_INT_PNDG;
	} else {	
		/* 
		** Get the right host bus adapter 
		*/
		for (c=0; c < efp2_cnt; c++)			
			if(efp2idata[c].iov == vect) {

				if(efp2idata[c].active != 1) {
					TRACE(("EFP2 : Inactive HBA %d\n",c));
					continue;
				}

				efp2_checkshrintr(c);

				if ((reply = efp_read_msg(EFP2_BASE(c))) == 0) {
					continue ;	/* Try next controller */
				} else {
					break;		/* Work on Interrupt */
				}
		}
	}

	if (c == efp2_cnt)
		return INT_NO_JOB;	/* interrupts for inactive board */
	
	efp = (EFP2_DATA *)SC(c, 0);

	EFPDEB('m');

	if (reply & EFP2_MSG) {
		register int msg;
		if ((msg=GET_MSG(reply)) == REPLY_QUEUE_FULL) {
			efp->ha_rq->rq_sys_header.sh_full = YES;
			TRACE(("REPLY QUEUE FULL on ctrl(%d)\n", c));
			reply |= EFP2_REPLY;
		} else
			if (msg & COMMAND_QUEUE_NOT_FULL) {
				if (GET_QUEUE(msg) == MBOX_QUEUE) {
					efp2_err(CE_WARN, 
					      "Mailbox was full ???", c, 8, 8, 8);
				} else {
					register queue_entry_t *qe;
					register command_queue_t *q;
					register int c;
					qe = efp->ha_qd->qd_entry + GET_QUEUE(msg);
					bus = qe->qe_channel -1;

					c = efp->ha_ctrl[bus];

#ifdef EFPDEBUG
			printf("c == %x  channe %x\n", c, qe->qe_channel);
			printf("EFP: HA%dQ%d - queue no more FULL\n",
				c,GET_QUEUE(msg));
#endif
					q =CMD_Q(c, bus, qe->qe_id, qe->qe_lun);
					oip = LOCK(scsi_lock[c], pldisk);
					q->cq_sys_header.sh_flag &= ~QUEUE_FULL;

					while (efp2_next(c, bus,  oip)!=EFP2_FALSE);
					UNLOCK(scsi_lock[c], oip);
				}
			} else {
				efp->ha_msg = msg;	/* Only in init phase */
			}
	}
	if (reply & EFP2_REPLY)  {
	  reply_queue_entry_t re;
	  int 		     env;


	  EFPDEB('r');
	  while (env=efp2_get_reply(c, efp, &re))
	  {
		register sblk_t *psb;
		int	b, t,l;

		if (re.re_cmd_que == MBOX_QUEUE) {	/* <========= */
			register mbox_reply_t *mre = (mbox_reply_t *) &re;
			efp->ha_mbox_reply = *mre;
			continue;
		}

#if	defined(DEBUG) && defined(TRACE_IDX)

		psb = (sblk_t *)(efp2_id[re.re_user_id % MAX_IDS] & ~1);
		efp2_id[re.re_user_id % MAX_IDS] = (long) psb; /* Mark free */
#else
		psb = (sblk_t *)re.re_user_id;
#endif

		if(!psb)  {
			cmn_err(CE_PANIC, "psb null in intr %x", psb);
			CALLDEB();
		}
			
		if (re.re_status == TRANSFER_ERROR) {
			efp2_err(CE_WARN,"EFP: Transfer error.", c, 8, 8);
			efp2_err(CE_WARN, c,-1,-1, E_TRANSFER, 0, &re);
			continue;
		}

		if (re.re_status == OVER_UNDER_RUN) {
			efp2_err(CE_WARN,
                                  "EFP:Over-Under runnig during transfer",
					c,8,8);

			continue;
		}

		/* 
		 * Because sdi sees each EFP2 SCSI BUS as different controller
		 * and interrupts are received only on the first of them,
		 * here we have to find the right controller.
		 */

		if (c != psb->s_c) {
			otherc = psb;
		}

		c = psb->s_c;
		b = psb->s_b;
		t = psb->s_t;
		l = psb->s_l;

		if(tracecmd && b > 0)
			cmn_err(CE_WARN,
			"Get reply for (%d,%d,%d,%d) at Q(%x)SB(%x) ",
				c,b,t,l,CMD_Q(c,b,t,l),psb->sbp);

		pri = LOCK(scsi_lock[c], pldisk);
		 
		/*
		 * Determine the job completion status.
		 */
		EFPDEB(re.re_status);
		switch(re.re_status)
		{
		case CORRECT_END:
			EFPDEB('c');
#ifdef MIRROR
/*MIRROR*/
			if ((re.re_ex_stat == NOTHING_TO_SIGNAL) ||
			    (re.re_ex_stat == NOT_YET_MIRRORING ) )  
#else
			if (re.re_ex_stat == NOTHING_TO_SIGNAL) 
#endif MIRROR
/*MIRROR END*/
			{
				psb->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				/* set residual cout */
				psb->sbp->sb.SCB.sc_resid = 0;
			} else {
				psb->sbp->sb.SCB.sc_comp_code = SDI_CKSTAT;
				bcopy(re.re_u.nre_u.nre_scsi_sense,
					(caddr_t)(sdi_sense_ptr(&psb->sbp->sb))+1,
					SCSI_SENSE_LEN);
				switch (re.re_ex_stat)
				{
				case COND_MET_GOOD      :
					psb->sbp->sb.SCB.sc_status = S_METGD;
					break;
				case INTERMED_GOOD      :
					psb->sbp->sb.SCB.sc_status = S_INGD;
					break;
				case INTERM_COND_MET_GOOD:
					psb->sbp->sb.SCB.sc_status = S_INMET;
					break;
				}
			}
			break;

		case RECOVER_OK :
			EFPDEB('R');
			/* Autonomous recovery procedure was OK */
			psb->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			psb->sbp->sb.SCB.sc_resid = 0;
#ifdef MIRROR
/*MIRROR*/
			if (re.re_flag & MIRROR_RESPONSE) {
			/* MIRRORING - return SENSE data */
				efp2_err(CE_WARN, c,t,l, E_MIRROR,
					(re.re_ex_stat==STILL_MIRRORING)
					? EM_STILLMIRROR : EM_NOMIRROR,
					&re);

				if (re.re_ex_stat == NOT_YET_MIRRORING)
					efp2_mirractiv(c,t,l,
					               (re.MRE_ATTR ==
							       SOURCE_OUT) ?
							           SOURCE_DISK:
								   MIRROR_DISK);
				break;
			}

#endif MIRROR
/*MIRROR END*/
			efp2_err(CE_WARN, c,t,l, E_RECOVER_OK, 0, &re);
			break;

		case RECOVER_NOK :
			EFPDEB('n');
			/* Autonomous recovery procedure was NOK */
#ifdef MIRROR
/*MIRROR*/
			switch(SC(c,0)->ha_nameid)
			{
			case    OLI_ESC2:
				contr = "ESC-2";
				break;
			case    OLI_EFP2:
				contr = "EFP2";
				break;
			}

cmn_err(CE_WARN," %s: HA %d TC %d LU %d - Error in automatic recovery procedure"
		, contr, c, t, l, re.re_status, re.re_ex_stat);

			if (!re.re_flag & MIRROR_RESPONSE)
#endif MIRROR
/*MIRROR END*/
				efp2_err(CE_WARN, c,t,l, E_RECOVER_NOK, 0, &re);

		case DISK_ERROR:

			EFPDEB('e');
			psb->sbp->sb.SCB.sc_comp_code = SDI_CKSTAT;

#ifdef MIRROR
/*MIRROR*/
			if (re.re_flag & MIRROR_RESPONSE) {
				/* MIRRORING - return SENSE data */

				if (psb->sbp->sb.SCB.sc_datasz != re.re_scsi_len) {

				/* Signal ERROR to driver    */

					switch(re.MRE[0].mre_valid)
					{
					case SAQL_FROM_RECOVERY:
					case SAQ_FROM_DEV:
					case SAQL_FROM_DEV:
					  psb->sbp->sb.SCB.sc_status = S_CKARS;
	
					  (void) bzero(
						(caddr_t) &psb->sbp->sb.sb_ars,
						sizeof(psb->sbp->sb.sb_ars));

					  psb->sbp->sb.ARS_SENSE[0] =
							re.MRE[0].mre_valid;

					  *(long *)(psb->sbp->sb.ARS_SENSE+3) =
					      sdi_swap32(re.MRE[0].mre_log_blk);

					  psb->sbp->sb.ARS_SENSE[2] =
							re.MRE[0].mre_sense;

					  psb->sbp->sb.ARS_SENSE[12] =
							re.MRE[0].mre_addit;

					  /*
					   * psb->sbp->sb.ARS_SENSE[6] = len ??
					   *
					   * psb->sbp->sb.ARS_SENSE[?] =
					   *    re.MRE[0].mre_qualif;
					   */
					  break;
					case COND_MET_GOOD:
					  psb->sbp->sb.SCB.sc_status = S_METGD;
					  break;
					case TARGET_BUSY:
					  psb->sbp->sb.SCB.sc_status = S_BUSY;
					  break;
					case INTERMED_GOOD:
					  psb->sbp->sb.SCB.sc_status = S_INGD;
					  break;
					case INTERMED_COND_MET_GOOD:
					  psb->sbp->sb.SCB.sc_status = S_INMET;
					  break;
					case RESERV_CONFLICT:
					  psb->sbp->sb.SCB.sc_status = S_RESER;
					  break;
					}
				} else {
					/* Signal OK to driver    */

					psb->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				}
				efp2_err(CE_WARN, c,t,l, E_MIRROR,
					(re.re_ex_stat==STILL_MIRRORING)
					? EM_STILLMIRROR : EM_NOMIRROR,
					&re);

				if (re.re_ex_stat == NOT_YET_MIRRORING)
					efp2_mirractiv(c,t,l,
						       (re.MRE_ATTR ==
							       SOURCE_OUT) ?
								   SOURCE_DISK:
								   MIRROR_DISK);
				break;
			}
#endif MIRROR
/*MIRROR END*/

			switch (re.re_ex_stat)
			{
			case NOTHING_TO_SIGNAL  :
				EFPDEB('h');
#ifdef ARS
				/* ARS - return SENSE data */
				{
				register union re_u *reu =
					(union re_u *) &psb->sbp->sb.sb_ars;
				*reu = re.re_u;
				}
				psb->sbp->sb.SCB.sc_status = S_CKARS;
				break;
#endif ARS
			case CHECK_CONDITION	:
				EFPDEB('k');
				psb->sbp->sb.SCB.sc_status = S_CKCON;
				break;
			case TARGET_BUSY	:
				EFPDEB('b');
				psb->sbp->sb.SCB.sc_status = S_BUSY;
				break;
			case RESERV_CONFLICT	:
				EFPDEB('C');
				psb->sbp->sb.SCB.sc_status = S_RESER;
				break;
			case ABORT_COMMAND	:
				EFPDEB('a');
				/*
				 * This could appear a bit strange
				 * but EFP2 after a CKCON refuse
				 * every command until it does not
				 * receive a REQUEST SENSE, so
				 * we try to tell to the target
				 * driver that there are no error
				 * and the command is just to be
				 * retried later
				 */
				psb->sbp->sb.SCB.sc_comp_code = SDI_RETRY;
				psb->sbp->sb.SCB.sc_status = S_GOOD;

				/*
				 * Otherwise I can try to just
				 * re-enqueue the command, so
				 * it can automatically be 
				 * sent again
				 *
				 * sub_job(psb, c,t,l);	<=========
				 * add_job(psb, c,t,l);	<=========
				 * continue;
				 */

				break;
			}
			break;

		
		default:			/* <====== Other ERRORs ??? */
#ifdef MIRROR
/*MIRROR*/
			switch(SE(c).ha_board_type)
			{
			case    OLI_ESC2:
				contr = "ESC-2";
				break;
			case    OLI_EFP2:
				contr = "EFP2";
				break;

			}
cmn_err(CE_WARN," %s: HA %d TC %d LU %d - re_status=0x%x re_ex_status=0x%x",
		contr, c, t, l, re.re_status, re.re_ex_stat);
#endif MIRROR
/*MIRROR END*/
			EFPDEB('E');
			psb->sbp->sb.SCB.sc_comp_code = SDI_HAERR;

			PRINTF(("EFP: c%dt%dl%d - re_status=%x\n",c,t,l,re.re_status));

			/*
			 * Should I start a reset sequence ??? <======
			 * (see dpt.c)
			 */
		}

		UNLOCK(scsi_lock[c], pri);

		/* call target driver interrupt handler */
		if(psb->sbp->sb.SCB.sc_int) {
			EFPDEB(0xc0);
			sdi_callback(&psb->sbp->sb);
		} 

		EFPDEB(0x04);
		oip = LOCK(scsi_lock[c], pldisk);
		while (efp2_next(c, b , oip)!=EFP2_FALSE)
			EFPDEB(0x05);
		UNLOCK(scsi_lock[c], oip);

	  }

	  retcode = INT_JOB_CMPLT;
	} else {
	  retcode = INT_NO_JOB;
	}

	/* 
	 * It need because EFP2 has 2 channel
	 * but only one interrupt 
	 */

	if (otherc != (sblk_t *)0) {
		c = otherc->s_c;
		bus = otherc->s_b;
		oip = LOCK(scsi_lock[c], pldisk);
		efp2_next(c, bus, oip);
		UNLOCK(scsi_lock[c], oip);

	}

	return retcode;
} 


/*
** Function name: efp2icmd()
** Description:
**      Send an immediate command.  If the logical unit is busy, the job
**      will be queued until the unit is free.  SFB operations will take
**      priority over SCB operations.
*/

long HBAICMD(struct hbadata *hbap, int sleepflag)
{
        register struct scsi_ad *sa;
        register sblk_t *sp = (sblk_t *) hbap;
        register int    c, t, l;
	int b;		/* Bus */
        pl_t   oip;
        struct ident         *inq_data;
        struct scs           *inq_cdb;
        struct sdi_edt  *edtp;

	register command_queue_t *q;


        switch (sp->sbp->sb.sb_type) {
        case SFB_TYPE:
                sa = &sp->sbp->sb.SFB.sf_dev;
                c = efp2_gtol[SDI_EXHAN(sa)];
		b = SDI_BUS(sa);
                t = SDI_EXTCN(sa);
                l = SDI_EXLUN(sa);
		q = CMD_Q(c, b, t, l);

		/* Refuse any l > 0 requests:  return TIMEOUT */
		if(l != 0)  {
                	sp->sbp->sb.SFB.sf_comp_code = SDI_TIME;
			if(sp->sbp->sb.SFB.sf_int) {
				EFPDEB(0xc1);
				sdi_callback(&sp->sbp->sb);
			} 
                        else TRACE( ("(icmd1)sdi_callback\n")) ;
                        return (SDI_RET_OK);
		}

                sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

                switch (sp->sbp->sb.SFB.sf_func) {
                case SFB_RESUME:
			oip = LOCK(scsi_lock[c], pldisk);
                        efp2_next(c, b,  oip);
			UNLOCK(scsi_lock[c], oip);
                        break;
                case SFB_SUSPEND:
                        break;
                case SFB_RESETM:
                        if((edtp = sdi_redt(c,t,l)) && edtp->pdtype == ID_TAPE)
 {
                                /*  this is a NOP for tape devices */
                                sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;
                                break;
                        }
                        /* else fall thru */

                case SFB_ABORTM:
                        sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			oip = LOCK(scsi_lock[c], pldisk);
                        efp2_putq(c, b, sp);
                        efp2_next(c, b,  oip);
			UNLOCK(scsi_lock[c], oip);
                        return (SDI_RET_OK);

                case SFB_FLUSHR:
#ifdef NOTDEFYET
			cmn_err(CE_PANIC,"Not implemented by MarcoT\n");
                        efp2_flushq(c, SDI_QFLUSH, 0);
#endif
                        break;
                case SFB_NOPF:
                        break;
                default:
                        sp->sbp->sb.SFB.sf_comp_code =
                            (unsigned long)SDI_SFBERR;
                }

		if(sp->sbp->sb.SFB.sf_int) {
			EFPDEB(0xc3);
			sdi_callback(&sp->sbp->sb);
		} 
                else TRACE(("(icmd3)sdi_callback\n"));

                return (SDI_RET_OK);

        case ISCB_TYPE:
                sa = &sp->sbp->sb.SCB.sc_dev;
                c = efp2_gtol[SDI_EXHAN(sa)];
		b = SDI_BUS(sa);
                t = SDI_EXTCN(sa);
                l = SDI_EXLUN(sa);
		q = CMD_Q(c, b, t, l);

                inq_cdb = (struct scs *)sp->sbp->sb.SCB.sc_cmdpt;
                if ((t == SC(c,b)->ha_id) && (l == 0) && 
					     (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, efp2idata[c].name, 
				VID_LEN+PID_LEN+REV_LEN);
			inq.id_type = ID_PROCESOR;

			inq_data = (struct ident *)(void *)sp->sbp->sb.SCB.sc_datapt;
			inq_len = sp->sbp->sb.SCB.sc_datasz;
			bcopy((char *)&inq, (char *)inq_data, inq_len);
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
                }
		/* Refuse any request about lun */
		if( l != 0 || (CMD_Q(c, b, t,l) == NULL)) {
                        sp->sbp->sb.SCB.sc_comp_code = SDI_TIME;
			if(sp->sbp->sb.SCB.sc_int) {
				EFPDEB(0xc4);
				sdi_callback(&sp->sbp->sb);
			} 
                        else TRACE(("(icmd4)sdi_callback\n"));
                        return (SDI_RET_OK);
		}
                if(efp2_illegal(SDI_EXHAN(sa), b,  t, l) &&
                    !efp2init_time) {
                        sp->sbp->sb.SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
			if(sp->sbp->sb.SCB.sc_int) {
				EFPDEB(0xc5);
				sdi_callback(&sp->sbp->sb);
			} 
                        else TRACE(("(icmd5)sdi_callback\n"));
                        return (SDI_RET_OK);
                }
			
                sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
                sp->sbp->sb.SCB.sc_status = 0;
/*ASS*/
		sp->s_t = t;
		sp->s_l = l;

		oip = LOCK(scsi_lock[c], pldisk);
                efp2_putq(c, b,  sp);
                efp2_next(c, b,  oip);
		UNLOCK(scsi_lock[c], oip);
                return (SDI_RET_OK);

        default:
		sdi_callback(&sp->sbp->sb);
                return (SDI_RET_ERR);
        }
}


/*
** Function name: efp2getblk()
** Description:
**      Allocate a SB structure for the caller.  The function will
**      sleep if there are no SCSI blocks available.
*/

struct hbadata *
HBAGETBLK(int sleepflag)
{
        register sblk_t *sp;

        sp = (sblk_t *)SDI_GET(&sm_poolhead, sleepflag);
        return((struct hbadata *)sp);
}


/*
** Function name: efp2freeblk()
** Description:
**      Release previously allocated SB structure. If a scatter/gather
**      list is associated with the SB, it is freed via dma_freelist().
**      A nonzero return indicates an error in pointer or type field.
*/

long
HBAFREEBLK(struct hbadata *hbap)
{
        extern void     efp2dma_freelist();
        register sblk_t *sp = (sblk_t *) hbap;

        if(sp->s_dmap) {
                efp2dma_freelist(sp->s_dmap);
                sp->s_dmap = NULL;
        }
        sdi_free(&sm_poolhead, (jpool_t *)sp);
        return (SDI_RET_OK);
}

efp2_illegal(hba, bus, scsi_id, lun)
short hba;
int bus;
unsigned char scsi_id, lun;
{
        if(sdi_rxedt(hba, bus, scsi_id, lun)) {
                return(0);
        } else {
                return(1);
        }
}

int traceputq = 0;

void efp2_putq(int c, int bus, sblk_t *sp)
{
	EFP2_DATA * had = SC(c,bus);
	sblk_t	*b = had->ha_startlist;
	sblk_t	*e = had->ha_endlist;
	int class = sp_prio(sp);

#ifdef SMARTQ	

	int incr = 0;

	/* New encoding policy to manage commands
	** with different type/priority
	*/
	if(traceputq)
		cmn_err(CE_NOTE, "%d in efp2_putq %x S(%x) E(%x) \n", sp_prio(sp), sp ,b,e);

	if (b == 0) {
		/* No Jobs are in queue */
		sp->s_prev = sp->s_next = NULL;
		b = e = sp;
	} else {

	class = sp_prio(sp);
	
	if(traceputq)
		cmn_err(CE_NOTE,"Start while");

	while(e && sp_prio(e) < class){		/* Scan the queue searching 
						** for low priority sp */
		e = e->s_prev;
		incr++;
	}

	if( !e ) { 
		/* Became First in list */
		if(traceputq)
			cmn_err(CE_NOTE,"(1)-- %d  jobs ctrl(%d)",
					had->ha_lenlist,c);
		sp->s_next = b;
		sp->s_prev = NULL;
		b = b->s_prev = sp;
	
	} else if( !e->s_next) {	

		/* Became last in queue */
		sp->s_next = NULL;
		sp->s_prev = e;
		e->s_next = sp;

		} else {
			if(traceputq)
			cmn_err(CE_NOTE,"(3)-- %d  jobs ctrl(%d)",
					had->ha_lenlist,c);
			sp->s_next = e->s_next;
			sp->s_prev = e;
			e->s_next = e->s_next->s_prev = sp;
		}
	}

#else	/* SMARTQ */

	if (b == 0) {
		b = sp;
		e = sp;
	} else {
		e->s_next = sp;
		e = sp;
	} 

#endif	/* SMARTQ */

	had->ha_startlist = b;
	had->ha_endlist = e;
	had->ha_lenlist++;
}

/*
** Function name: efp2dma_freelist()
** Description:
**      Release a previously allocated scatter/gather DMA list.
*/

void
efp2dma_freelist(dmap)
dma_t *dmap;
{
        register int  oip;

	EFP2_DMALIST_LOCK(oip);
        dmap->d_next = efp2_dfreelist;
        efp2_dfreelist = dmap;

        if (dmap->d_next == NULL) {
		EFP2_DMALIST_UNLOCK(oip);
                SV_BROADCAST(efp2_dmalist_sv, 0);
	} else
		EFP2_DMALIST_UNLOCK(oip);
}



HBAXLAT_DECL HBAXLAT(register struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{

        extern int efp2_dmalist();
        extern void     efp2dma_freelist();
        register sblk_t *sp = (sblk_t *) hbap;
	
	EFPDEB('X');
        if (sp->s_dmap) {
                efp2dma_freelist(sp->s_dmap);
                sp->s_dmap = NULL;
        }
        if (sp->sbp->sb.SCB.sc_link != NULL) {
                efp2_err(CE_WARN,"Linked commands NOT available.", 8, 8, 8);
                sp->sbp->sb.SCB.sc_link = NULL;

        }
        if (sp->sbp->sb.SCB.sc_datapt != NULL) {
                /* Don't use scatter/gather if the job fits on a single page. */

                sp->s_addr = vtop(sp->sbp->sb.SCB.sc_datapt, procp);
                if (sp->sbp->sb.SCB.sc_datasz > pgbnd(sp->s_addr)) {
                        efp2_dmalist(sp, procp, sleepflag);
		}

        } else  {
                sp->sbp->sb.SCB.sc_datasz = NULL;
	}
        
	HBAXLAT_RETURN(SDI_RET_OK);
}

/*
** Function name: efp2open()
** Description:
**      Driver open() entry point. It checks permissions, and in the
**      case of a pass-thru open, suspends the particular LU queue.
*/

HBAOPEN(devp, flags, otype, cred_p)
dev_t   *devp;
cred_t  *cred_p;
int     flags;
int     otype;
{
        dev_t   dev = *devp;
	minor_t         minor = geteminor(dev);
        register int    c = efp2_gtol[SC_EXHAN(minor)];
        register int    b = SC_BUS(minor);
        register int    t = SC_EXTCN(minor);
        register int    l = SC_EXLUN(minor);
	EFP2_DATA *had = SC(c,b);

        if (drv_priv(cred_p)) {
                return(EPERM);
        }
        if (t == SC(c,b)->ha_id) {
                return(0);
	}

        return(0);
}

/*
** Function name: efp2close()
** Description:
**      Driver close() entry point.  In the case of a pass-thru close
**      it resumes the queue and calls the target driver event handler
**      if one is present.
*/

HBACLOSE(dev, flags, otype, cred_p)
cred_t  *cred_p;
dev_t   dev;
int     flags;
int     otype;
{
	minor_t         minor = geteminor(dev);
        register int    c = efp2_gtol[SC_EXHAN(minor)];
        register int    b = SC_BUS(minor);
        register int    t = SC_EXTCN(minor);
        register int    l = SC_EXLUN(minor);
	EFP2_DATA *had = SC(c,b);
	pl_t		oip;

        if (t == had->ha_id)
                return(0);

	oip = LOCK(scsi_lock[c], pldisk);
        efp2_next(c, b,  oip);
	UNLOCK(scsi_lock[c], oip);
        return(0);
}

/*
** Function name: efp2_dmalist()
** Description:
**      Build the physical address(es) for DMA to/from the data buffer.
**      If the data buffer is contiguous in physical memory, sp->s_addr
**      is already set for a regular SB.  If not, a scatter/gather list
**      is built, and the SB will point to that list instead.
*/

int efp2_dmalist(sblk_t *sp, struct proc *procp, int sleepflag)
{
	struct dma_vect tmp_list[SG_SEGMENTS];
        register struct dma_vect  *pp;
        register dma_t  *dmap;
        register long   count, fraglen, thispage;
        caddr_t         vaddr;
        paddr_t         addr, base;
        int             i, oip;

	EFPDEB('D');
        vaddr = sp->sbp->sb.SCB.sc_datapt;
        count = sp->sbp->sb.SCB.sc_datasz;
        pp = &tmp_list[0];

        /* First build a scatter/gather list of physical addresses and sizes */

        for (i = 0; (i < SG_SEGMENTS) && count; ++i, ++pp) {
		EFPDEB(0xe1);
                base = vtop(vaddr, procp);      /* Phys address of segment */
                fraglen = 0;                    /* Zero bytes so far */
                do {
			EFPDEB(0xe2);
                        thispage = min(count, pgbnd(vaddr));
                        fraglen += thispage;    /* This many more contiguous */
                        vaddr += thispage;      /* Bump virtual address */
                        count -= thispage;      /* Recompute amount left */
                        if (!count)
                                break;          /* End of request */
                        addr = vtop(vaddr, procp); /* Get next page's address */
                } while (base + fraglen == addr);


                /* Now set up dma list element */
		pp->d_addr = base;
                pp->d_cnt = fraglen;
        }
        if (count != 0)
                efp2_err(CE_PANIC, "Job too big for DMA list",8,8,8);

        /*
         * If the data buffer was contiguous in physical memory,
         * there was no need for a scatter/gather list; We're done.
         */
        if (i > 1)
        {
		EFPDEB(0xe3);
                /*
                 * We need a scatter/gather list.
                 * Allocate one and copy the list we built into it.
                 */
		EFP2_DMALIST_LOCK(oip);

		if(!efp2_dfreelist && (sleepflag == KM_NOSLEEP)) {
			/* DEADLOCK!!!!! 
			** EFP2_DMALIST_LOCK(oip);
			*/
			if(printlock)
				cmn_err(CE_WARN,"Dealloc DMALIST_LOCK\n");
			EFP2_DMALIST_UNLOCK(oip);
			return(1);
		}
                while ( !(dmap = efp2_dfreelist)) {
                        SV_WAIT(efp2_dmalist_sv, PRIBIO, efp2_dmalist_lock);
			EFP2_DMALIST_LOCK(oip);
			EFPDEB(0xe4);
		}
		EFPDEB(0xe5);
                efp2_dfreelist = dmap->d_next;
		EFP2_DMALIST_UNLOCK(oip);

                sp->s_dmap = dmap;;
                sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
                dmap->d_size = i * sizeof(struct dma_vect);
                bcopy((caddr_t) &tmp_list[0],
                        (caddr_t) dmap->d_list, dmap->d_size);
        }
	EFPDEB(0xef);
        return;
}

/*============================================================================
** Function name: efp2send()
** Description:
**	This function fills in the request queue entry and interrupts the
**	HA board. Non SCB jobs generate express ints. while SCB jobs
**	generate normal interrupts.
** Return Code: TRUE if a job was immediately sent, FALSE otherwise.
**
*/
long
HBASEND(register struct hbadata *hbap, int sleepflag)
{
	int	c, b, t, l;
	sblk_t	*sp = (sblk_t *)hbap;
        register struct scsi_ad *sa;

	int		retcode = EFP2_FALSE;
	int  		dsize, n;
	register command_queue_t *q;
	register queue_entry_t  *qe;
	pl_t		oip;

	union {
		normal_command_t	n;
		struct {
			sg_command_t	sg;
			linked_entry_t	le;
		} s;
		extended_sg_t		e;
	} cmd;


        sa = &sp->sbp->sb.SCB.sc_dev;
        c = efp2_gtol[SDI_EXHAN(sa)];
	b = SDI_BUS(sa);
        t = SDI_EXTCN(sa);
        l = SDI_EXLUN(sa);
	q = CMD_Q(c, b, t, l);

	if(tracecmd && b > 0 )
	cmn_err(CE_WARN,"Send cmd to (%d,%d,%d,%d) Q(%x) SB(%x)", c,b,t,l,q,sp->sbp);
	
	if(sp->sbp->sb.sb_type != SCB_TYPE) {
		return(SDI_RET_ERR);
	}

	if (q == NULL) {
		/*
		 * This is a trick to avoid scsi_cmd() waiting for an 
		 * interrupt we cannot supply.
		 */
		if (efp2init_time)
			sp->sbp->sb.SCB.sc_comp_code = SDI_TIME;
		else
			efp2_err(CE_WARN, 
				"(1)command sent to unequipped channel.",c,t,l);

		if(sp->sbp->sb.SCB.sc_int) {
			EFPDEB(0xc6);
			sdi_callback(&sp->sbp->sb);
		} 
		else TRACE(("(send1)sdi_callback\n"));
                return (SDI_RET_OK);
	}

	/* marco 
	** Refuse any request about lun
	**/
	if(l != 0) {
               	sp->sbp->sb.SCB.sc_comp_code = SDI_TIME;
		if(sp->sbp->sb.SCB.sc_int) {
			EFPDEB(0xc7);
			sdi_callback(&sp->sbp->sb);
		}  
		else 
			TRACE(("(send2)sdi_callback\n"));
                return (SDI_RET_OK);
	}

	oip = LOCK(scsi_lock[c], pldisk);

	sp->s_c = c;
	sp->s_b = b;
	sp->s_t = t;
	sp->s_l = l;


        sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
        sp->sbp->sb.SCB.sc_status = 0;


        efp2_putq(c, b, sp);

        efp2_next(c, b,  oip);
	UNLOCK(scsi_lock[c], oip);
        return (SDI_RET_OK);

}

/*
**	This routine is called by:	efp2intr
**					efp2_pass_thru
**					efp2icmd
**					efp2send
**					efp2close
**	
*/

long efp2_next (int c, int b,  pl_t ipl_locked)
{
	int	t, l;
	sblk_t	*sp;
	register struct scsi_ad *sa;

	EFP2_DATA *had = SC(c,b);
	int		retcode = EFP2_FALSE;
	int  		dsize, n;
	register command_queue_t *q;
	register queue_entry_t  *qe;
	int oip;
	int mpoip;

	union {
		normal_command_t	n;
		struct {
			sg_command_t	sg;
			linked_entry_t	le;
		} s;
		extended_sg_t		e;
	} cmd;

	if (had->ha_lenlist == 0)
		return(EFP2_FALSE);

	oip = spldisk();

	if((sp = had->ha_startlist) == NULL) {
		splx(oip);
		return(EFP2_FALSE);
	}

	t = sp->s_t;
	l = sp->s_l;
	q = CMD_Q(c, b, t,l);

	if(q == NULL) {
		had->ha_startlist = sp->s_next;
		if (sp->s_next == 0) 
			had->ha_endlist = 0;
		had->ha_lenlist--;
		splx(oip);
		return EFP2_FALSE;
	}

	cmd.n.nc_header.ch_userid = (long) sp;
	cmd.n.nc_header.ch_sort = NO;
	cmd.n.nc_header.ch_prior = 0;
	cmd.n.nc_header.ch_mod = NORMAL_MOD;

	n=1;	/* Single entry command */

	if ((sp->sbp->sb.SCB.sc_datapt == NULL) || 
			(sp->sbp->sb.SCB.sc_datasz == 0)) {
		cmd.n.nc_header.ch_cmd_type = NO_DATA_TRANSFER; 
		dsize = 0;
	} else {
		cmd.n.nc_header.ch_cmd_type = 
			(sp->sbp->sb.SCB.sc_mode & SCB_READ) ? 
					SCSI_TO_SYSTEM : SYSTEM_TO_SCSI;
	}

	if (sp->s_dmap == NULL) {	/* Normal command */
		EFPDEB(0x04);
		cmd.n.nc_cdb_l = sp->sbp->sb.SCB.sc_cmdsz;
		cmd.n.nc_rfu1[0] = cmd.n.nc_rfu1[1] = cmd.n.nc_rfu1[2] = 0;
		cmd.n.nc_length = sp->sbp->sb.SCB.sc_datasz;
		cmd.n.nc_address = sp->s_addr;

		/* copy SCB cdb to controller CB cdb */
		(void) bcopy((caddr_t)sp->sbp->sb.SCB.sc_cmdpt, 
			(caddr_t)cmd.n.nc_cdb_scsi, CDB_LEN);
	} else {
		dsize = sp->s_dmap->d_size / sizeof(struct dma_vect);

		if ((dsize <= 3 
		|| (dsize <= 8 && (q->cq_sys_header.sh_flag & QUEUE_DOUBLE_ENTRY))) 
		&& ( *(sp->sbp->sb.SCB.sc_cmdpt) == SM_READ 
		  || *(sp->sbp->sb.SCB.sc_cmdpt) == SM_WRITE) ) {
				/* Could be a scatter gather command */

			/* Prepare for SHORT scatter gather command */
		
			/* Each entry must be <= SHORT_SG_MAX_LEN <===== */
			register struct dma_vect *p = &sp->s_dmap->d_list[0];
			register unsigned short *l = &cmd.s.sg.sg_l1;
			register paddr_t 	*a = &cmd.s.sg.sg_a1;
			int			 i;

			EFPDEB(0x06);
			cmd.s.sg.sg_header.ch_cmd_type = 
				(sp->sbp->sb.SCB.sc_mode & SCB_READ) ? 
							SHORT_SG_READ : SHORT_SG_WRITE;
		
			if (sp->sbp->sb.SCB.sc_cmdsz == SCS_SZ) {
				register struct scs *sc = 
					(struct scs *)sp->sbp->sb.SCB.sc_cmdpt;
				cmd.s.sg.sg_log_blk = sdi_swap16(sc->ss_addr);
			} else {
				register struct scm *sc = (struct scm *)
					SCM_ADJ(sp->sbp->sb.SCB.sc_cmdpt);
				cmd.s.sg.sg_log_blk = sdi_swap32(sc->sm_addr);
			}

			cmd.s.sg.sg_rfu1 = 0;			/* Ordine? <= */
			cmd.s.sg.sg_size_blk = STD_BLK_SIZ;	/* Ordine? <= */

			for (i=0; i<3; i++, l++, a++, p++) {
				if (i<dsize) {
					*l = (short) p->d_cnt;/*Check size<== */
					*a = p->d_addr;
				} else {
					*l = NO_LEN;
					*a = NO_ADDR;
				}
			}

			if (dsize > 3 || sp->sbp->sb.SCB.sc_datasz >= SHORT_SG_MAX_LEN) {
							/* Long scatter gather */

				cmd.s.sg.sg_header.ch_cmd_type = 
					 (sp->sbp->sb.SCB.sc_mode & SCB_READ) ?
						LONG_SG_READ : LONG_SG_WRITE;

				cmd.s.le.le_link = CMD_LINK;
				l = &cmd.s.le.le_l4;
				a = &cmd.s.le.le_a4;

				for (i=3; i<8; i++, l++, a++, p++) {
					if (i<dsize) {
						*l = (short)(p->d_cnt);/*Check size<== */
						*a = p->d_addr;
					} else {
						*l = NO_LEN;
						*a = NO_ADDR;
					}
				}

				n = 2;
			}
		} else {				/* Extended scatter gather */
			register long l, *p;

			EFPDEB(0x07);
			cmd.e.esg_header.ch_cmd_type = 
				(sp->sbp->sb.SCB.sc_mode & SCB_READ) ?
						EXT_SG_READ : EXT_SG_WRITE;

			cmd.e.esg_cdb_l = sp->sbp->sb.SCB.sc_cmdsz;
			cmd.e.esg_rfu1[0] = cmd.e.esg_rfu1[1] = cmd.e.esg_rfu1[2] = 0;
			cmd.e.esg_lb = l = sp->s_dmap->d_size;
			cmd.e.esg_rfu2 = 0;
			cmd.e.esg_ab = (paddr_t)sp->s_addr;

			/* copy SCB cdb to controller CB cdb */
			(void) bcopy((caddr_t)sp->sbp->sb.SCB.sc_cmdpt, 
				(caddr_t)cmd.e.esg_cdb_scsi, CDB_LEN);

		}
	}

	/* Try to enqueue cmd into the ha command queue */

	if (efp2_put_cmd(had, q, &cmd, n) == EFP2_TRUE) {
		if(efp2init_time == EFP2_FALSE)
			efp2ccount ++;
		had->ha_startlist = sp->s_next;
		if (sp->s_next == 0) 
			had->ha_endlist = 0;
		EFPDEB(0x09);
		EFPDEB(had->ha_lenlist);
		had->ha_lenlist--;
		if(efp2init_time) {	 /* poll : wait 30 seconds */

			UNLOCK(scsi_lock[c], ipl_locked);
			if (efp2_ha_poll(c, 30*ONE_MILL_U_SEC,EFP2_FALSE)!=PASS) {
				efp2_err(CE_WARN, efp2_epoll, c,8,8);
				sp->sbp->sb.SCB.sc_comp_code = (u_long) SDI_TIME;
				EFPDEB(0xc9);
				sdi_callback(&sp->sbp->sb);
			}	
			(void)LOCK(scsi_lock[c], pldisk);

		}
		EFPDEB(0x0a);
		retcode = EFP2_TRUE;
	} else {
                TRACE(("efp2_next put_cmd failed(%d)(0x%x) \n", efp2ccount,sp));
		EFPDEB(0x0b);
		retcode = EFP2_FALSE;
	}
	splx(oip);
	return (retcode);
}

/*
** Function name: efp2getinfo()
** Description:
**      Return the name, etc. of the given device.  The name is copied into
**      a string pointed to by the first field of the getinfo structure.
*/

void 
HBAGETINFO(struct scsi_ad *sa, struct hbagetinfo *getinfo)
{
register char  *s1;
register char  *s2;
static char temp[] = "HA X TC X";

        s1 = temp;
        s2 = getinfo->name;
        temp[3] = SDI_HAN(sa) + '0';
        temp[8] = SDI_TCN(sa) + '0';

        while ((*s2++ = *s1++) != '\0') ;
        getinfo->iotype = F_DMA | F_SCGTH

#if PDI_VERSION >= PDI_UNIXWARE20
	/* getinfo->iotype |= F_RESID */
#endif
		;
#if	PDI_VERSION >= PDI_SVR42MP
	if(getinfo->bcbp) {
		getinfo->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfo->bcbp->bcb_flags     = 0;
		getinfo->bcbp->bcb_max_xfer  = efp2hba_info.max_xfer;
		getinfo->bcbp->bcb_physreqp->phys_align  = 512;
		getinfo->bcbp->bcb_physreqp->phys_boundary  = 0;
		getinfo->bcbp->bcb_physreqp->phys_dmasize   = 32;
	}
#endif
}


/*
** Function name: efp2_pass_thru()
** Description:
**      Send a pass-thru job to the HA board.
*/

#if PDI_VERSION >= PDI_SVR42MP
STATIC void
efp2_pass_thru0(buf_t *bp)
{
        int     c = efp2_gtol[SC_EXHAN(bp->b_edev)];
	minor_t	 minor = getminor(bp->b_edev);
	int     b = SC_BUS(minor);
        int     t = SC_EXTCN(minor);
        int     l = SC_EXLUN(minor);
	EFP2_DATA *had = SC(c,b); 
	
	if (!had->ha_bcbp ) {
		struct sb *sp;
		struct scsi_ad *sa;
	
		sp = bp->b_priv.un_ptr;
		sa = &sp->SCB.sc_dev;
		if (( had->ha_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		had->ha_bcbp->bcb_addrtypes = BA_KVIRT;
		had->ha_bcbp->bcb_flags     = BCB_SYNCHRONOUS;
		had->ha_bcbp->bcb_max_xfer  = efp2hba_info.max_xfer - ptob(1);
		had->ha_bcbp->bcb_physreqp->phys_align  = 512;
		had->ha_bcbp->bcb_physreqp->phys_boundary  = 0;
		had->ha_bcbp->bcb_physreqp->phys_dmasize   = 32;
		had->ha_bcbp->bcb_granularity = 1;
	}
	buf_breakup(efp2_pass_thru, bp, had->ha_bcbp);
}
#endif

void
efp2_pass_thru(bp)
struct buf  *bp;
{
        int     c = efp2_gtol[SC_EXHAN(bp->b_edev)];
	minor_t	 minor = getminor(bp->b_edev);
	int     b = SC_BUS(minor);
        int     t = SC_EXTCN(minor);
        int     l = SC_EXLUN(minor);
	EFP2_DATA *had = SC(c,b); 
        register struct sb *sp;
        struct proc *procp;
        int  oip;
	register command_queue_t *q;
	caddr_t *addr;
	char op;
	daddr_t blkno_adjust;

#if PDI_VERSION < PDI_SVR42MP
	sp = (struct sb *)bp->b_private;
#else
	sp = bp->b_priv.un_ptr;
#endif

	sp-> SCB.sc_wd = (long) bp;
        sp->SCB.sc_datapt =  bp->b_un.b_addr;
	sp->SCB.sc_datasz = bp->b_bcount;
        sp->SCB.sc_int = efp2_int;

        SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP); 

	q = CMD_Q(c,b,t,l);

#if (PDI_VERSION >= PDI_UNIXWARE20)
	/*
	 * (This is a workaround for the 2G limitation for physiock offset.)
	 * Set new block number in the SCSI command if breakup occurred
	 * by adjusting the starting block number with the adjustment 
	 * from b_blkno.  Starting block number was saved in b_priv2,
	 * and having told physiock that the offset was 0, we add
	 * the adjustment now.
	 */
	blkno_adjust = bp->b_blkno;
	op = had->ha_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)(void *)had->ha_sc_cmd;
		daddr_t blkno;

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
			scs->ss_addr  = sdi_swap16(blkno);
		}
		scs->ss_len   = (char)(bp->b_bcount >> EFP2_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		daddr_t blkno;
		struct scm *scm = (struct scm *)(void *)((char *)had->ha_sc_cmd - 2);

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scm->sm_addr = sdi_swap32(blkno);
		}
		scm->sm_len  = sdi_swap16(bp->b_bcount >> EFP2_BLKSHFT);
	}
#else /* (PDI_VERSION < PDI_UNIXWARE20) */
	/*
	 * Please NOTE:
	 * UnixWare 1.1 has 2Gig passthru limit
	 * due to physiock offset limitation.
	 */
	op = had->ha_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)(void *)had->ha_sc_cmd;
		daddr_t blkno = bp->b_blkno;
		scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
		scs->ss_addr  = sdi_swap16(blkno);
		scs->ss_len   = (char)(bp->b_bcount >> EFP2_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		struct scm *scm = (struct scm *)(void *)((char *)had->ha_sc_cmd - 2);
		scm->sm_addr = sdi_swap32(bp->b_blkno);
		scm->sm_len  = sdi_swap16(bp->b_bcount >> EFP2_BLKSHFT);
	}
#endif /* (PDI_VERSION < PDI_UNIXWARE20) */

	sdi_icmd(sp, KM_SLEEP);
}

/*
** Function name: efp2_int()
** Description:
**      This is the interrupt handler for pass-thru jobs.  It just
**      wakes up the sleeping process.
*/

void
efp2_int(sp)
struct sb *sp;
{
        struct buf  *bp;

        bp = (struct buf *) sp->SCB.sc_wd;
        biodone(bp);
}




/*============================================================================
** Function name: efp2_put_cmd()
** Description:
**	This function put a command into specified command queue
** Return Code: TRUE if command was accepted, FALSE otherwise (queue is full).
**
** Side effect:
**	if command queue was empty (EMPTY/NOTEMPTY transition) signal it
**	to controller
**
** SMP:  Acquire command-queue lock to enqueue the command and
**       had->ha_lock to protect access to hw I/O ports [ efp_cmd() ]
*/
int
efp2_put_cmd(had , q, data, n)
register EFP2_DATA *had; 
register command_queue_t *q;
command_queue_entry_t *data;
int n;
{
	register int put, get, lput, lget;

        pl_t lock_pri0; /* return value from lock is previous priority */
        pl_t lock_pri;  /* return value from lock is previous priority */


	lock_pri0 = LOCK(q->cq_sys_header.sh_lock, pldisk);
	
	EFPDEB('P');
	if (q->cq_sys_header.sh_flag & QUEUE_FULL) {
		EFPDEB(0x01);
		if (q->cq_header.cq_put == q->cq_header.cq_get) {
			PRINTF(("EFP: efp2_put_cmd(%d) - queue is now EMPTY\n", q->cq_sys_header.sh_queue));
			EFPDEB(0x02);
			q->cq_sys_header.sh_flag &= ~QUEUE_FULL;
		} else {
			PRINTF(("EFP: efp2_put_cmd(%d) - queue still FULL\n", q->cq_sys_header.sh_queue));
                        UNLOCK(q->cq_sys_header.sh_lock, lock_pri0);          
			EFPDEB(0x03);
			EFPDEB(0xff);
			return EFP2_FALSE;
		}
	}

	EFPDEB(0x04);
	put = lput = q->cq_header.cq_put;
	lget = q->cq_header.cq_get;

	/*
	 * Check if queue is full :
	 *				<--------- qsize ------->
	 *	-------------------------........................
	 *	|     |     |     |     |     !     !     !     !
	 *	-------------------------........................
	 *	               ^     ^     ^           ^
	 *		      Get   Put	 Put+n	     Get+qsize
	 *
	 * Queue is full when Put + n.of elements to insert
	 * became higher than get.
	 *
	 * Note:
	 *   To check this condition 'get' is managed in order to have
	 *   it alwais greather than 'put'
	 */

	if (lget <= lput) lget += q->cq_sys_header.sh_size;

	if (lget-lput-1 <= n) {
		q->cq_sys_header.sh_flag = QUEUE_FULL;
		PRINTF(("EFP: efp2_put_cmd(%d) - queue is FULL\n", q->cq_sys_header.sh_queue));
		UNLOCK(q->cq_sys_header.sh_lock, lock_pri0);          
		EFPDEB(0x05);
		return EFP2_FALSE;
					/* <==== Insert cmd in wait queue */
	}

	while (n--) {
		q->cq_entry[lput] = *data++;
		lput = (lput+1) % q->cq_sys_header.sh_size;
	}

	q->cq_header.cq_put = lput;

	get = q->cq_header.cq_get;

	UNLOCK(q->cq_sys_header.sh_lock, lock_pri0);          

	if (put == get)	{	/* queue was previously empty */

                lock_pri = LOCK(had->ha_lock, pldisk);
		EFPDEB(0x06);

		for (n=0; n<EFP_RETRIES; n++) {
			EFPDEB(0x07);
			if (efp_cmd(q->cq_sys_header.sh_iobase, 
				q->cq_sys_header.sh_queue)==0) break;
			PRINTF(("EFP: efp_cmd(%x,%d) returned error.\n", q->cq_sys_header.sh_iobase, q->cq_sys_header.sh_queue));
		}		

		UNLOCK(had->ha_lock, lock_pri);
	}

#ifdef EFPDEBUG
	{
	int nq = q->cq_sys_header.sh_size - (lget-put-1) - n;

	if (nq > q->cq_sys_header.sh_maxq) {
		q->cq_sys_header.sh_maxq = nq;
		PRINTF(("EFP: slot = %d, queue = %d, max entries = %d\n", q->cq_sys_header.sh_iobase/0x1000, q->cq_sys_header.sh_queue, nq));
		}
	}
#endif

	EFPDEB(0x08);
	EFPDEB(0x0f);
	return EFP2_TRUE;
}


/*============================================================================
** Function name: efp2_get_reply()
** Description:
**	This function gets a reply from reply queue
** Return Code: YES if a reply was available, NO if no more replies
**		are available.
**
** Side effect:
**	if reply queue is full (signalled from controller) reset the
**	full condition (signal it to controllor)
**
** SMP:  Acquires efp->ha_lock to protect access to hw I/O ports
**       [ efp_reply_no_full() ]
*/
efp2_get_reply(c, efp, data)
int c;
register EFP2_DATA *efp;
reply_queue_entry_t *data;
{
	register reply_queue_t *q = efp->ha_rq;
	register reply_queue_entry_t *entry;
	register int reply;

        pl_t prev_pri;          /* Previous  Lock  Priority*/
	pl_t sh_getlock;	/* Sync sh_get field */


	prev_pri = LOCK(efp->ha_lock, pldisk);

	entry = q->rq_entry + q->rq_sys_header.sh_get;

	if (reply=entry->re_flag) {
		*data = *entry;
		entry->re_flag = NO;
		q->rq_sys_header.sh_get = 
			(q->rq_sys_header.sh_get+1) % q->rq_sys_header.sh_size;

		if (q->rq_sys_header.sh_full) {
			int n;

			q->rq_sys_header.sh_full = NO;

			for (n=0; n<EFP_RETRIES; n++)
				if (efp_reply_no_full(
					q->rq_sys_header.sh_iobase)==0) break;

			PRINTF(("EFP: efp_reply_no_full(%d);\n", entry->re_cmd_que));
		}
	}  
	else 
		TRACE(("efp2_get_reply null %x reply %x\n", entry, reply));

	UNLOCK(efp->ha_lock, prev_pri);
	return reply;
}

/*============================================================================
** Function name: efp2ioctl()
** Description:
**	HA specific ioctl () Entry Point. Used to implement the following 
**	special functions:
**
**    HA_GETINFO	- Read HA and target configuration 
**			  ( only used in init phase )
**		
**    HA_SETINFO	- Update HA and target configuration
**			  ( only used in init phase )
**
**    HA_ADD_DEV	- Add needed structures for the specified
**			  device (not found in init phase, 
**			  so not yet configured)
**
**    HA_LOAD           - download data from host to efp2 controller
**
**    HA_UNLOAD         - upload data from efp2 controller to host
*/
HBAIOCTL (dev_t dev, int cmd, caddr_t argp, int mode, cred_t *cred_p, int *rval_p)
{

	register queue_entry_t 	*qe;
	int i;
	minor_t	 minor = getminor(dev);
        register int    gc = SC_EXHAN(minor);
        register int    c = efp2_gtol[gc];
	register int    t = SC_EXTCN(minor);
	register int    l = SC_EXLUN(minor);
	register int    b = SC_BUS(minor);
	register EFP2_DATA *had = (EFP2_DATA *)SC(c,b);
	int uerror = 0;
	register struct sb *sp;

        if (efp2_illegal(gc, b, t, l)) {
                return(ENXIO);
        }

	TRACE(("ioctl cmd (%d,%d,%d,%d)(%d)\n",c, b, t,l,cmd));

	switch(cmd) {
	 case SDI_SEND: {
                register buf_t *bp;
                struct sb  karg;
                int  rw;
                char *save_priv;

                if (t == had->ha_id) {      /* illegal ID */
                        return(ENXIO);
                }
                if (copyin(argp, (caddr_t)&karg, sizeof(struct sb))) {
                        return(EFAULT);
                }
                if ((karg.sb_type != ISCB_TYPE) ||
                    (karg.SCB.sc_cmdsz <= 0 )   ||
                    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) {
                        return(EINVAL);
                }
		
                sp = SDI_GETBLK(KM_SLEEP);
                save_priv = sp->SCB.sc_priv;
                bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

		bp = getrbuf(KM_SLEEP);
                bp->b_iodone = NULL;
                sp->SCB.sc_priv = save_priv;
                sp->SCB.sc_wd = (long)bp;
                sp->SCB.sc_cmdpt = (caddr_t) had->ha_sc_cmd;

                if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
                    sp->SCB.sc_cmdsz)) {
                        uerror = EFAULT;
                        goto done;
                }
		sp->SCB.sc_dev.sa_lun = (uchar_t)l;
		sp->SCB.sc_dev.sa_bus = (uchar_t)b;
		sp->SCB.sc_dev.sa_exta = (uchar_t)t;
		sp->SCB.sc_dev.sa_ct  = SDI_SA_CT(gc,t);
                rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;

#if PDI_VERSION < PDI_SVR42MP
		bp->b_private = (char *)sp;
#else
		bp->b_priv.un_ptr = sp;
#endif

		/*
                 * If the job involves a data transfer then the
                 * request is done thru physiock() so that the user
                 * data area is locked in memory. If the job doesn't
                 * involve any data transfer then efp2_pass_thru()
                 * is called directly.
                 */
		
		if (sp->SCB.sc_datasz > 0) {
                        struct iovec  ha_iov;
                        struct uio    ha_uio;
			char op;

                        ha_iov.iov_base = sp->SCB.sc_datapt;
                        ha_iov.iov_len = sp->SCB.sc_datasz;
                        ha_uio.uio_iov = &ha_iov;
                        ha_uio.uio_iovcnt = 1;
			ha_uio.uio_offset = 0;
                        ha_uio.uio_segflg = UIO_USERSPACE;
                        ha_uio.uio_fmode = 0;
                        ha_uio.uio_resid = sp->SCB.sc_datasz;
			op = had->ha_sc_cmd[0];
#if (PDI_VERSION >= PDI_UNIXWARE20)
			/*
			 * Save the starting block number, if r/w.
			 */
			if (op == SS_READ || op == SS_WRITE) {
				struct scs *scs = 
				   (struct scs *)(void *)had->ha_sc_cmd;
				bp->b_priv2.un_int =
				   ((sdi_swap16(scs->ss_addr) & 0xffff)
				   + (scs->ss_addr1 << 16));
			}
			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm = 
				(struct scm *)(void *)((char *)had->ha_sc_cmd - 2);
				bp->b_priv2.un_int =
				   sdi_swap32(scm->sm_addr);
			}
#else /* (PDI_VERSION < PDI_UNIXWARE20) */
			/*
			 * Please NOTE: 
			 * UnixWare 1.1 has 2Gig passthru limit
			 * due to physiock offset limitation.
			 */
			if (op == SS_READ || op == SS_WRITE) {
				struct scs *scs =
				   (struct scs *)(void *)had->ha_sc_cmd;
				ha_uio.uio_offset = 
				   ((sdi_swap16(scs->ss_addr) & 0xffff)+
				   (scs->ss_addr1 << 16)) * EFP2_BLKSIZE;
			}
			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm =
				(struct scm *)(void *)((char *)had->ha_sc_cmd - 2);
				ha_uio.uio_offset = 
				   sdi_swap32(scm->sm_addr) * EFP2_BLKSIZE;
			}
#endif /* (PDI_VERSION < PDI_UNIXWARE20) */

                        if (uerror = physiock(efp2_pass_thru0, bp, dev, rw,
                            HBA_MAX_PHYSIOCK, &ha_uio)) {
                                goto done;
                        }
                } else {
                        bp->b_un.b_addr = NULL;
                        bp->b_bcount = 0 ; 
                        bp->b_blkno = NULL;
                        bp->b_edev = dev;
                        bp->b_flags |= rw;
			
		 	efp2_pass_thru(bp);  /* fake physio call */
                        biowait(bp);
                }

                /* update user SCB fields */

                karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
                karg.SCB.sc_status = sp->SCB.sc_status;
                karg.SCB.sc_time = sp->SCB.sc_time;
		if (copyout((caddr_t)&karg, argp, sizeof(struct sb)))
                        uerror = EFAULT;

		done:
			freerbuf(bp);
			sdi_freeblk(sp);
			break;
		}

	 case B_GETTYPE:
                if(copyout("scsi", ((struct bus_type *)argp)->bus_name, 5)) {
                        return(EFAULT);
                }
                if(copyout("scsi", ((struct bus_type *)argp)->drv_name, 5)) {
                        return(EFAULT);
                }
                break;

        case    HA_VER:
                if (copyout((caddr_t)&efp2_sdi_ver,argp, sizeof(struct ver_no)))
                        return(EFAULT);
                break;

#ifdef MRESET
        case SDI_BRESET: {

		oip = spl5();
                if (had->ha_npend > 0) {     /* jobs are outstanding */
                        return(EBUSY);
		} else {
                }
                splx(oip);
                break;
                }
#endif /* MRESET*/

#ifdef NOTDEF

#ifdef GETINFO
	case HA_GETINFO:
		if (efp2init_time || !(SC(c).ha_state & C_OPERATIONAL)) {
			struct ha_info *info = (struct ha_info *) argp;
			configuration_entry_t *ce;

			info->hi_fw_rel[0]	= had->ha_info.gi_fw_rel[0];
			info->hi_fw_rel[1]	= had->ha_info.gi_fw_rel[1];
			info->hi_fw_rel[2]	= had->ha_info.gi_fw_rel[2];
			info->hi_environment	= had->ha_info.gi_environment;
			info->hi_qlen		= had->ha_info.gi_max_cmd_ent;
			info->hi_board_type	= SE(c).ha_board_type;

			info->hi_host_id = SC(c).ha_id;

			/*
			 * Dual Host
			 */
			if (SE(c).ha_board_type == OLI_EFP2)
				if ( efp2_asym == EFP2_TRUE)
					info->hi_host_mode = HI_DUAL_MASTER;
				else
				info->hi_host_mode = (SC(c).ha_id == MASTER_ID)
						? HI_DUAL_MASTER:HI_DUAL_SLAVE;
			else
				info->hi_host_mode = HI_NORMAL;

			/* 
			 * search qe related to
			 * c,info->hi_addr.ha_tc,info->hi_addr.ha_lu
			 */

			qe = efp2_get_qe(had, 
				c,info->hi_addr.ha_tc,info->hi_addr.ha_lu);

			if (qe==NULL)
				return EINVAL;

			info->hi_scsi_level	= qe->qe_scsi_level;
			info->hi_supported	= HI_SORT | HI_ARS;

			info->hi_used = (qe->qe_no_ars != YES) ? HI_ARS : 0;
			/*
			 * search ce related to
			 * c,info->hi_addr.ha_tc,info->hi_addr.ha_lu
			 */

			ce = efp2_get_ce(had,
				c,info->hi_addr.ha_tc,info->hi_addr.ha_lu);

			if (ce!=NULL)
				info->hi_env = ce->ce_env;
			else
				/*
				 * error !!!
				 */
				info->hi_env = 0;

			return 0;
		}
		break;

	case HA_SETINFO:
		if (efp2init_time || !(SC(c).ha_state & C_OPERATIONAL)) {
			struct ha_info *info = (struct ha_info *) argp;

			/* 
			 * search qe related to
			 * c,info->hi_addr.ha_tc,info->hi_addr.ha_lu
			 */

			qe = efp2_get_qe(had,
				c,info->hi_addr.ha_tc,info->hi_addr.ha_lu);

			if (qe==NULL)
				return EINVAL;

			qe->qe_no_ars = (info->hi_used & HI_ARS) ? NO : YES;
			qe->qe_rfu1 = info->hi_timeout;
			qe->qe_rfu2 = info->hi_used;
			return 0;
		}
		break;

	case HA_ADD_DEV:
		if (efp2init_time || !(SC(c).ha_state & C_OPERATIONAL)) {
			struct ha_dev *addr = (struct ha_dev *) argp;
			int ch;
			queue_entry_t   *qe;

			for (ch=0; ch<HA_CTRLS; ch++)
				if (had->ha_ctrl[ch] == c) break;

			PRINTF(("EFP: Add new device - c=%d, t=%d, l=%d, ch=%d, typ=%d\n",c,addr->ha_tc,addr->ha_lu,ch));

			if (ch>=HA_CTRLS)
				return EINVAL;
			if (efp2_alloc_queue(c,addr->ha_tc,addr->ha_lu,
				had,ch+1,SCSI_1_PROT,addr->ha_type) != PASS)
					return ENOMEM;
			/* Set NO ARS for the driver configured */
			/* in /stand/boot			*/ 

			qe = efp2_get_qe(had, c, addr->ha_tc, addr->ha_lu);
			qe->qe_no_ars = YES;
			qe->qe_rfu1 = 0;

			return 0;
		}
		break;
#endif CONFIG
#endif NOTDEF
		default:
			uerror = EINVAL;
			break;
	}
	
	return (uerror);;
}
/*============================================================================
** Function name: efp2_get_chan()
** Description:
**	This function return the EFP2 channel associated to the specified
**	controller
**
*/
#ifdef NOTDEF
int
efp2_get_chan(int c, int channel)
{
	return (SC(c, channel)->ha_scsi_channel);
}
#endif

/*============================================================================
** Function name: efp2_get_qe()
** Description:
** 	Search the queue entry related to specified c,t,l
** Return: 
**	Queue entry or NULL if not found.
*/
queue_entry_t *
efp2_get_qe(efp, c,t,l)
register EFP2_DATA *efp;
int c,t,l;
{
	int i,n,ch;
	register queue_entry_t *qe;

	if((ch = efp->ha_scsi_channel) == SCSI_NO_CHAN)
		return NULL;

	for (i=0, qe=efp->ha_qd->qd_entry, n=efp->ha_qd->qd_header.qh_n_cmd; 
	     i<n; i++, qe++) 
		if (qe->qe_channel==ch && qe->qe_id==t && qe->qe_lun==l)
			return qe;
	
	return NULL;
}

/*============================================================================
** Function name: efp2_get_ce()
** Description:
**	Search the configuration entry related to specified c,t,l
** Return:
**	Configuration entry or NULL if not found.
*/
configuration_entry_t *
efp2_get_ce(efp, c,t,l)
register EFP2_DATA *efp;
int c,t,l;
{
	int i,n,ch;
	register configuration_entry_t *ce;

	if((ch = efp->ha_scsi_channel) == SCSI_NO_CHAN)
		return NULL;

	for (ce = efp->ha_conf[i=0]; i<efp->ha_l; i++, ce++)
		if (ce->ce_channel==ch && ce->ce_id==t && ce->ce_lun==l)
				return ce;

	return NULL;
}


/*============================================================================
** Function name: efp2_setmode()
** Description:
** 	Set controller in EFP2 mode and start communication.
** Return Code: 
**	PASS if succsfully terminated, FAIL elsewhere
*/
int efp2_setmode(EFP2_DATA *had, int c)
{

	int vect = had->ha_iov;
	int base = EFP2_BASE(c);
	int hard_vect;


	int i,n;
	queue_entry_t *qe;
	command_queue_header_t *cq;

#ifdef EFPDEBUG
	hard_vect = efp2_get_irq(base);
	PRINTF(("EFP: efp_set get_irq (%x, %d)\n", base, hard_vect));

	if(hard_vect != vect ) {
		efp2_err(CE_WARN,
			"Interrupt conflict (hardware <%d>, software <%d>)",
			hard_vect, vect);
	}
#endif /* EFPDEBUG */

	PRINTF(("EFP: efp_set(%x, %d)\n", base, vect));

	had->ha_msg = -1;

	for (n=0; n<EFP_RETRIES; n++)
		if (efp_set(base, vect)==0) break;
	
	if (n==EFP_RETRIES) {
		efp2_err(CE_WARN, efp2_eset,c,8,8);
		return FAIL;
	}

	/* wait 1 second */
	if (efp2_ha_poll(c,ONE_MILL_U_SEC,EFP2_FALSE)!=PASS) { 
		efp2_err(CE_WARN, efp2_epoll, c,8,8);
		return FAIL;
	}

	if (had->ha_msg != INIT_DIAG_OK) {
		efp2_err(CE_WARN, "efp_set() returned %x", had->ha_msg);
		return FAIL;
	}

	EFPDEB('k');
	had->ha_rq->rq_sys_header.sh_full = EFP2_FALSE;
	EFPDEB(0x1);
	had->ha_rq->rq_sys_header.sh_get = 0;
	EFPDEB(0x2);

	/*
	 * Reset queues
	 */
	n = had->ha_qd->qd_header.qh_n_cmd;
	EFPDEB(0x3);
	qe = had->ha_qd->qd_entry;
	EFPDEB(0x4);

	for (i=0; i<n; i++, qe++)
	{
		EFPDEB(0x5);
		cq = (command_queue_header_t *)physmap(qe->qe_cmd_queue,
						       sizeof(command_queue_header_t), 
						       KM_SLEEP);
		EFPDEB(0x6);
		cq->cq_put = cq->cq_get = 0;
	}
		EFPDEB(0x7);

	/* Start communication with controller */

	PRINTF(("EFP: efp_start(%x, %x)\n", base, had->ha_qd));

	had->ha_msg = -1;

	for (n=0; n<EFP_RETRIES; n++)
		if (efp_start(base, vtop((caddr_t)had->ha_qd, NULL)) == 0) break;

	if (n==EFP_RETRIES) {
		efp2_err(CE_WARN, efp2_estart, c,8,8);
		return FAIL;
	}

	/* wait 1 second */
	if (efp2_ha_poll(c, ONE_MILL_U_SEC,EFP2_FALSE)!=PASS) { 
		efp2_err(CE_WARN, efp2_epoll, c,8,8);
		return FAIL;
	}

	if (had->ha_msg != INIT_DIAG_OK) {
		efp2_err(CE_WARN, "efp_start() returned %x", had->ha_msg);
		return FAIL;
	}
	return PASS;
}

void efp2_ha_start(int c)
{

	register EFP2_DATA	*had = (EFP2_DATA *)Efpboard[c].board_datap[0];

	if ((had->ha_state & C_OPERATIONAL) && !(had->ha_flag & HA_F_START))
	{
		int n = had->ha_qd->qd_header.qh_n_cmd;

		had->ha_state &= ~C_OPERATIONAL;


		/*
		 * Queue was setted to work in NO ARS mode and then
		 * target drivers could have changed the configuration, so
		 * set controller in EFP2 mode for the 3rd (and last) time 
		 */

		if (efp2_setmode(had, c) == PASS) {
			had->ha_flag |= HA_F_START;	/* Start done */
			had->ha_state |= C_OPERATIONAL;
#ifdef MIRROR
			efp2_mirrok(c);
#endif MIRROR
		} else cmn_err(CE_WARN,"efp_ha_start : efp2_setmod failed on %d",c);
	}
	return;
}

/*============================================================================
** Function name: efp2_ha_inquiry ()
**
*/
int
efp2_ha_inquiry(c, id_p, inq_cdb_p)
int c;
struct ident *id_p;
caddr_t	inq_cdb_p;		/* pointer to cdb 	*/
{
	int i;
	static char efp_id_vendor[] = "OLIVETTI";
	char *efp_id_prod;

	switch (SC(c,0)->ha_nameid)
	{
	case OLI_ESC2:
		efp_id_prod = "ESC-2           ";
		break;
	case OLI_EFP2:
		efp_id_prod = "EFP2            ";
		break;
	default:
		efp_id_prod = "UNKNOWN HA BOARD";
		break;
	}
	for (i = 0; i < VID_LEN; i++) 
		id_p->id_vendor[i] = efp_id_vendor[i];

	for (i = 0; i < PID_LEN; i++)
		id_p->id_prod[i] = efp_id_prod[i];

	return(SDI_ASW);
}

#ifdef SVR40_DEBUG

#define	OUTPORT	0x378
#define	OUTVAL	0x0

/*
 * Just to put a break on
 */
efp2_softbreak(msg,parm)
char *msg;
long parm;
{
	outb(OUTPORT, OUTVAL);
	printf("SOFTBREAK: %s %x\n",msg, parm);
#ifdef CALLDEBUG
	if (efp2_calldeb)
		calldebug();

	if (efp2_calldeb==0x12345678) {
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
		efp2_softbreak(msg,parm++);
	}
#endif
}
#endif /* SVR40_DEBUG */

#ifdef OLI_SVR40
/*============================================================================
** Function name: efp2_onoff()
** Description:
**	This function informs every EFP2 board
**	of PEM/HOST state transition.
**
** SMP:  Acquires SC(c).efp.ha_lock to protect access to hw I/O ports
**	[ efp_warning() ]
*/
void
efp2_onoff()
{
	int c,n;
	int base = -1;

        pl_t prev_pri;          /* Previous Priority */

	for (c=0; c<efp2_cnt; c++) {
		if (EFP2_BASE(c) == base)       /* Skip additional channels */
			continue;

		base = EFP2_BASE(c);

		if (SC(c).ha_nameid == OLI_EFP2) {
                        prev_pri = LOCK((SC(c).ha_lock), pldisk);
 
			for (n=0; n<EFP_RETRIES; n++)
				if (efp_warning(base)==0) break;

                        UNLOCK((SC(c).ha_lock), prev_pri);
			if (n==EFP_RETRIES)
				efp2_err(CE_WARN, efp2_ewarning, c,8,8);
		}
	}
}
#endif /* OLI_SVR40 */

#ifdef MIRROR
/*MIRROR*/
efp2_mirractiv(c,t,l,type)
int     c,t,l,type;

{
int     j;
configuration_entry_t   *ce;

        for (j=0; j < MIRRTABENTRY; j++) {
                if ( ((mirr_tab[j].type & 0xf) == type) &&
                     (mirr_tab[j].c == c) &&
                     (mirr_tab[j].t == t) &&
                     (mirr_tab[j].l == l)
                   )
                        return;

                if (mirr_tab[j].type == 0)
                        break;
        }

        for (j=0; j < MIRRTABENTRY; j++) {
                if (mirr_tab[j].type == 0) {
                        mirr_tab[j].c = c;
                        mirr_tab[j].t = t;
                        mirr_tab[j].l = l;
                        mirr_tab[j].type = type;

			
                        ce = efp2_get_ce(SC(c), c, t ,l);
                        ce->ce_env &= ~(type);

                        if (efp2_mirrtimeout == 0) {
                                timeout (efp2_mirtime, NULL, ONE_TICK);
                                efp2_mirrtimeout = 1;
                        }
                        return;
                }
        }
}

efp2_mirrok(c)
int     c;

{
        register EFP2_DATA      *had = (EFP2_DATA *)SC(c);
        configuration_entry_t   *ce;
        int                     i,j;
        reply_queue_entry_t     re;

	for (ce = had->ha_conf[i=0]; i<had->ha_l; i++, ce++) {

		if ( (ce->ce_env & 0xf0) &&
		     ((ce->ce_env & 0xf) != (MIRROR_DISK | SOURCE_DISK)) ) {

			(void)bzero( (caddr_t)&re, sizeof(reply_queue_entry_t));
			re.re_status = DISK_ERROR;
			re.re_ex_stat = NOT_YET_MIRRORING;

			if ((ce->ce_env & 0xf) != MIRROR_DISK) {
				re.MRE[1].mre_valid = SAQ_FROM_DEV;
				re.MRE[1].mre_sense = SD_HARDERR;
				re.MRE_ATTR = MIRROR_OUT;
				efp2_mirractiv(c,ce->ce_id,ce->ce_lun,
					MIRROR_DISK);
			}
			if ((ce->ce_env & 0xf) != SOURCE_DISK) {
				re.MRE[0].mre_valid = SAQ_FROM_DEV;
				re.MRE[0].mre_sense = SD_HARDERR;
				re.MRE_ATTR = SOURCE_OUT;
				efp2_mirractiv(c,ce->ce_id,ce->ce_lun,
					SOURCE_DISK);
			}
			efp2_err(CE_WARN, c, ce->ce_id, ce->ce_lun, E_MIRROR,
				 EM_NOMIRROR, &re);
		}
	}
}

void
efp2_mirtime()

{
int     c,t,l;
char    *contr;
int     flg = 0;
int     i;

        for (i=0; i< MIRRTABENTRY; i++) {

                if (mirr_tab[i].type != 0) {
                        c = mirr_tab[i].c;
                        t = mirr_tab[i].t;
                        l = mirr_tab[i].l;

                        switch(SE(c).ha_board_type)
                        {
                        case    OLI_ESC2:
                                contr = "ESC-2";
                                break;
                        case    OLI_EFP2:
                                contr = "EFP2";
                                break;
                        }

                        if ((mirr_tab[i].type & 0xf) == SOURCE_DISK )

cmn_err(CE_WARN," %s: HA %d TC %d LU %d -SRC DISK OF MIRROR PAIR IS OFF LINE",
                 contr, c, t, l);
                        else
cmn_err(CE_WARN," %s: HA %d TC %d LU %d -MIR DISK OF MIRROR PAIR IS OFF LINE",
                 contr, c, t, l);

                        flg = 1;
                }
                else
                        break;
        }

        if (flg)
                timeout (efp2_mirtime, NULL, ONE_MINUTE*10);
}
/*MIRROR END*/
#endif MIRROR


#if PDI_VERSION >= PDI_SVR42MP
/*
 * STATIC void *
 * efp2_kmem_zalloc_physreq (size_t size, int flags)
 *		
 * function to be used in place of kma_zalloc
 * which allocates contiguous memory, using kmem_alloc_physreq,
 * and zero's the memory.
 *
 * Entry/Exit Locks: None.
 */
STATIC void *
efp2_kmem_zalloc_physreq (size_t size, int flags)
{
	void *mem;
	physreq_t *preq;

	preq = physreq_alloc(flags);
	if (preq == NULL)
		return NULL;
	preq->phys_align = EFP_MEMALIGN;
	preq->phys_boundary = EFP_BOUNDARY;
	preq->phys_dmasize = 32;
	preq->phys_flags |= PREQ_PHYSCONTIG;
	if (!physreq_prep(preq, flags)) {
		physreq_free(preq);
		return NULL;
	}
	mem = kmem_alloc_physreq(size, preq, flags);
	physreq_free(preq);
	if (mem)
		bzero(mem, size);

	return (void *)mem;
}
#endif /* PDI_VERSION >= PDI_SVR42MP */

/*============================================================================
** Function name: efp2_err()
** Description:
**      This function prints an error message through cmn_err. It prepends
**      the driver name, and the HA, TC, and LU numbers if they apply.
**      On EISA systems it will give an indication of the slot number of
**      the host adapter as well as the logical HA number.
** Message Format:
**      WARNING: SCSI: SLOT n HA n TC n LU n message less than 40 chars
**      WARNING: SCSI: message less than 60 chars
** Usage:
**      for brevity call with c, t, or l == 8 if not used in message.
**
** SMP:  No locks required.
*/

#define ARGS    a0,a1,a2,a3,a4,a5
efp2_err(level, format, c, t, l, ARGS)
int level;
char *format;
int c, t, l;
int ARGS;
{
        char message[128];
        register int m = 0;
        register int slot;
	int scsi_cnt = 1;

        message[m++] = 'S';
        message[m++] = 'C';
        message[m++] = 'S';
        message[m++] = 'I';
        message[m++] = ':';
        message[m++] = ' ';
        if (c < scsi_cnt) {
		if (Efpboard[c].base_io_addr >= 0x1000) {
                        slot = Efpboard[c].base_io_addr/0x1000;
                        message[m++] = 'S';
                        message[m++] = 'L';
                        message[m++] = 'O';
                        message[m++] = 'T';
                        message[m++] = ' ';
                        if (slot >= 10) {
                                message[m++] = '1';
                                message[m++] = '0' + (slot - 10);
                      } else {
                                message[m++] = '0' + slot;
                        }
                        message[m++] = ' ';
		}
                message[m++] = 'H';
                message[m++] = 'A';
                message[m++] = ' ';
                message[m++] = '0' + c;
                if (t < MAX_TCS) {
                        message[m++] = ' ';
                        message[m++] = 'T';
                        message[m++] = 'C';
                        message[m++] = ' ';
                        message[m++] = '0' + t;
                        if (l < MAX_LUS) {
                                message[m++] = ' ';
                                message[m++] = 'L';
                                message[m++] = 'U';
                                message[m++] = ' ';
                                message[m++] = '0' + l;
                        }
                }
        message[m++] = ' ';
        }

        (void)strcpy (&message[m], format);
        cmn_err (level, message, ARGS);
        return;
}

#ifdef EFPDEBUG
void printIdata(HBA_IDATA_STRUCT *Idata)
{
	printf("%s , version_num %d, ", Idata->name, Idata->version_num);
	printf("%x id  , ioaddr1 %x\n ", Idata->ha_id, Idata->ioaddr1);
	printf("%x dmachan  , iov %x, ", Idata->dmachan1, Idata->iov);
	printf("%d cntrl  , active %d\n ", Idata->cntlr, Idata->active);
	printf("%d idata_memaddr,ctl order %d",Idata->idata_memaddr, Idata->idata_ctlorder);
	printf("%d n_bus,ntarget %d\n",Idata->idata_nbus,Idata->idata_ntargets);
	printf("%d n_nluns,rmkey %d,",Idata->idata_nluns, Idata->idata_rmkey);
	printf("%x intrcookie\n ", Idata->idata_intrcookie);
}

void printBoard()
{
	int i ,x = 0;
	for (i = 0; i < efp2_cnt; i++) {
		printf(" Board [%d] name %s SLOT[%d]	\n",
			i,Efpboard[i].board_name,Efpboard[i].board_slot);
		for(x=0;x < 2;x++) 
			printf(" Pointer  Channel[%d][%x] \n",
				x, Efpboard[i].board_datap[x]);
		printf(" Channel [%d] boot_flag [%d]\n", 
				Efpboard[i].n_channel, Efpboard[i].boot_flag);
	}
}
#endif

efp2_checkshrintr(int c)
{
#ifdef EFPDEBUG
        if(EFP2_BASE(c) == NULL) {
                cmn_err(CE_WARN,
                   "EFP2 BASE NOT INITIALIZED %d",c);
                CALLDEB();
        }

        if(SC(c,0)->ha_iov == 0) {
                cmn_err(CE_WARN,
                    "INTERRUPT TROUBLES %d", c);
                CALLDEB();
        }
#endif /* EFPDEBUG */
}

/**************************************************************************
*
* - procedure   :   efp2_get_irq
*
* - input       :   base address
*
* - output      :   irq number
*
* - function    :   get irq number
*
* - side effect :   none
*
**************************************************************************/
int
efp2_get_irq(base)
int          base;

{
	char                irq;

	  irq = esc_getreg(base,0x02);
	  irq = (irq & 0x03);
	  switch (irq) {
	      case 0: irq=11;
		       break;
	      case 1: irq=10;
		       break;
	      case 2:  irq=5;
		       break;
	      case 3:  irq=15;
		       break;
	      default: irq=11;
  	}
	return(irq);
}


int efp2_ha_present(int sleepflag)
{
	static char *nn = "EFP-2";
	static char *n1 = "ESC-2";
	EFP2_DATA   *had;
	EFP2_DATA   *Host_p;
	int idata_cnt, datacnt;
	int ha_init , n_scsi_bus;
	int i;

	int slot = 0;
	int efp2_bus_index = 0;

	for (Board_cnt = 0; Board_cnt < efp2_cnt; Board_cnt++)
		Efpboard[Board_cnt].board_datap[0] =
		   Efpboard[Board_cnt].board_datap[1] = (EFP2_DATA*)-1;

	efp2_bus_index = 0;
	Board_cnt = 0;

	/*
	**  Loop Through idata's
	*/
	for (idata_cnt = 0; idata_cnt < efp2_cnt; idata_cnt++) {

                unsigned char   boardid[4];
                int bid;
                int base;

		efp2idata[idata_cnt].ioaddr1 &= (0xfffff000);
		base = efp2idata[idata_cnt].ioaddr1;
		slot = base >> 12;

#ifdef EFP_DEBUG
		cmn_err(CE_NOTE, "examining slot %d", slot);
#endif

		 bid = inl(base + SYSID);
                *(unsigned int *)boardid = bid;

                if (boardid[0] == OLI1 &&
                    boardid[1] == OLI2 &&
                    boardid[2] == SCSIBOARD) {

                        if (boardid[3] == BID_EFP2) {
#ifdef EFP_DEBUG
				cmn_err(CE_NOTE,"Found EFP2 board in slot %d idatcnt %d",slot, idata_cnt);
			fake(&efp2idata[idata_cnt]);
#endif
				efp2idata[idata_cnt].idata_nbus = 2;
				efp2idata[idata_cnt].idata_nluns = 2;
				n_scsi_bus &= 2;

				/* 
				 * Initialize Board channel 1 
				 */
				strcpy(Efpboard[idata_cnt].board_name, nn);

				Efpboard[idata_cnt].lcntrl       = idata_cnt;
				Efpboard[idata_cnt].n_channel    = 2;
				Efpboard[idata_cnt].board_slot   = slot;
				Efpboard[idata_cnt].boot_flag    = had->ha_boot ;
				Efpboard[idata_cnt].base_io_addr = base;

				/* Allocate structure for both EFP2 bus */
				datacnt =  ( sizeof (EFP2_DATA) * 2);
				if ((had = (EFP2_DATA *)KMEM_ZALLOC(datacnt , sleepflag))
					== NULL) {
					cmn_err(CE_WARN, efp2_enomem);
					return (-1);
				}


				/*
				 *  Initialize HA Data Struct First EFP2 Channel
				 */
				
			 	strcpy(had->ha_name, nn);

				had->ha_nameid  = OLI_EFP2;
                                had->ha_start_addr = base;
                                had->ha_iov = efp2idata[idata_cnt].iov;
                                had->ha_scsi_channel = 1;	/** CHANNEL **/
			 	strcpy(had->ha_name, nn);


				SC(idata_cnt,FIRST_CHANNEL) = (EFP2_DATA *)had;
#ifdef EFPDEBUG
				cmn_err(CE_WARN, "Setting address %x on crtl %d chan%d",had,idata_cnt,0);
#endif

				/* Next */
				efp2_bus_index++;
				had++;

			        /* 
				 * Initialize HA Data Struct Second EFP2 Channel
				 */

			 	strcpy(had->ha_name, nn);

				had->ha_nameid  = OLI_EFP2;
                                had->ha_start_addr = base;
                                had->ha_iov = efp2idata[idata_cnt].iov;

				SC(idata_cnt,SECOND_CHANNEL) = (EFP2_DATA *)had;
#ifdef EFPDEBUG
				cmn_err(CE_WARN, 
					"Setting address %x on crtl %d chan%d",had,idata_cnt,1);
#endif
				if ((efp2_scsi_boot_slot == -1) &&
                                    (scsi_get_boot(base, OLI_EFP2,
                                       &efp2_scsi_boot_id, &efp2_scsi_boot_chan) == 1)) {
                                        had->ha_boot = 1;
                                        efp2_scsi_boot_slot = slot;
					efp2idata[idata_cnt].idata_ctlorder = 1;
					efp2idata[idata_cnt].iov = 
						efp2_get_irq(base);
					cm_AT_putconf(
					efp2idata[idata_cnt].idata_rmkey, 
					efp2idata[idata_cnt].iov,CM_ITYPE_LEVEL,
					0,0,0,0,0, CM_SET_IRQ|CM_SET_ITYPE,1);
                                } else {
					/*
					 * Remove prior claim
					 */
					cm_args_t	cma;
					cma.cm_key = 
					      efp2idata[idata_cnt].idata_rmkey;
					cma.cm_n = 0;
					cma.cm_param = CM_CLAIM;
					cma.cm_vallen = sizeof(cm_num_t);

					(void)cm_delval(&cma);
				}

				if(efp2_ha_init(idata_cnt) == FAIL) {
	   				TRACE(("Failed Hard Initit on Ctrl(%d found in slot %d",
					idata_cnt, slot));
					continue;
				} else TRACE(("%s C(%d)initialized!!!!\n",
						efp2idata[idata_cnt].name,idata_cnt));
                                had->ha_scsi_channel = 2;	/** CHANNEL **/

				/*
				** Post Initialization is needed only for Channel 0 
				*/
                        	if (efp2_ha_post_init(SC(idata_cnt,0),idata_cnt) != PASS ) {
                                	cmn_err(CE_WARN, "SCSI post init failed on crtl #%d",
						idata_cnt);
					continue;
				} else cmn_err(CE_NOTE, "!%s Controller (%d) is now operational",
                                        efp2idata[idata_cnt].name, idata_cnt);


			} else if ((boardid[3] == BID_ESC2mb)||
					(boardid[3] == BID_ESC2pi) ||
				     	(boardid[3] == BID_ESC2fd) ) {
				
#ifdef EFPDEBUG
				cmn_err(CE_NOTE,"!Found ESC2 board in slot %d",slot);
#endif
				efp2idata[idata_cnt].idata_nbus = 1;
				n_scsi_bus &= 1;

				/* 
			         * Allocate data struct for ESC2 Mono Bus 
				 */

				datacnt = (sizeof (EFP2_DATA));
				had = (EFP2_DATA *)KMEM_ZALLOC(datacnt , sleepflag);
				if( had == (EFP2_DATA *)0) {
					cmn_err(CE_WARN, efp2_enomem);
					return (-1);
				}

			        /* 
				 * Initialize HA Data Struct ESC2 Channel 
				 */
				strcpy(had->ha_name, n1);
				had->ha_nameid     = OLI_ESC2;
				had->ha_start_addr = base;
				had->ha_iov = efp2idata[idata_cnt].iov;
				had->ha_scsi_channel  = 1;

				if ((efp2_scsi_boot_slot == -1)  &&
                                    (scsi_get_boot(base, OLI_ESC2,
                                       &efp2_scsi_boot_id, &efp2_scsi_boot_chan) == 1)) {
                                        had->ha_boot = 1;
                                        efp2_scsi_boot_slot = slot;
					efp2idata[idata_cnt].idata_ctlorder = 1;
					efp2idata[idata_cnt].iov = 
						efp2_get_irq(base);
					cm_AT_putconf(
					efp2idata[idata_cnt].idata_rmkey, 
					efp2idata[idata_cnt].iov,CM_ITYPE_LEVEL,
					0,0,0,0,0, CM_SET_IRQ|CM_SET_ITYPE,1);
                                } else {
					/*
					 * Remove prior claim
					 */
					cm_args_t	cma;
					cma.cm_key = 
					      efp2idata[idata_cnt].idata_rmkey;
					cma.cm_n = 0;
					cma.cm_param = CM_CLAIM;
					cma.cm_vallen = sizeof(cm_num_t);

					(void)cm_delval(&cma);
				}


				/* Initialize Board channel  */
                                strcpy(Efpboard[idata_cnt].board_name, n1);
				Efpboard[idata_cnt].lcntrl       = idata_cnt;
                                Efpboard[idata_cnt].n_channel    = 1;
                                Efpboard[idata_cnt].board_slot   = slot;
                                Efpboard[idata_cnt].boot_flag    = had->ha_boot ;
                                Efpboard[idata_cnt].base_io_addr = base;
				Efpboard[idata_cnt].board_datap[0] = (EFP2_DATA *)had;
				Efpboard[idata_cnt].board_datap[1] = (EFP2_DATA *)0;

                		ha_init = efp2_ha_init(idata_cnt);
				if( ha_init == FAIL) {
	   				TRACE(("Failed Hard Initit on Ctrl %d, Bus %d found in slot %d",
		 				idata_cnt, efp2_bus_index, slot));
					SC(idata_cnt,0)->ha_state    &= ~C_OPERATIONAL;
					continue;
				} else 
				       TRACE(("%s %d initialized!!!!\n",
					efp2idata[idata_cnt].name,idata_cnt));

                        	if (efp2_ha_post_init(had, idata_cnt) != PASS ) {
					cmn_err(CE_WARN, "SCSI post init failed on crtl #%d",
								idata_cnt);
					continue;
				} else cmn_err(CE_NOTE, 
						"!%s Controller (%d) is now operational",
                                        		efp2idata[idata_cnt].name, idata_cnt);


				efp2_bus_index++;
			}

		} 
#ifdef EFPDEBUG
	else  cmn_err(CE_NOTE," Not an Olivetti Board in slot %d\n", slot);
#endif
	}
	return (efp2_bus_index) ;
}

#ifdef MTDEBUG
char efp_debug[MTDEB_BUFSZ];
int pos_deb = 0;
efp_trace(char c)
{
	/*
	if(efp2init_time)
		return; 
	*/

	efp_debug[pos_deb++] = c;
	pos_deb &= (MTDEB_BUFSZ -1);
}

efp_dump(int count)
{
	int i;
	int pos = (pos_deb-count);
	char p  = efp_debug[pos];

	for (i=0; i <= count; i++) {
		p = efp_debug[pos+i];
		printf("[%x]", (char)p);
	}
}
#endif	/* MTDEBUG */




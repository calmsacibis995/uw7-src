#ident	"@(#)kern-pdi:io/hba/adss/adss.c	1.24.7.2"

/***************************************************************************
 *	Copyright (c) 1993  Adaptec Corporation				   *
 *	All Rights Reserved						   *
 *									   *
 *      ADAPTEC CORPORATION PROPRIETARY INFORMATION			   *
 *									   *
 *	This software is supplied under the terms of a license agreement   *
 *	or nondisclosure agreement with Adaptec Corporation and may not be *
 *	copied or disclosed except in accordance with the terms of that    *
 *	agreement.							   *
 ***************************************************************************/

/***************************************************************************
 *									   *
 *	SCSI Host Adapter Driver for Adaptec AIC-6X60 based adapters	   *
 *									   *
 ***************************************************************************
 *	V0.1 03/22/93							   *
 *	- currently limited to PIO only for the Alpha release		   *
 *	- released to the factory on 04/02/93				   *
 *	V0.2 04/05/93							   *
 *	- fixed a bug where the SCB was being released before the command  *
 *	  was done for scatter/gather requests				   *
 *	- released to the factory on 04/09/93				   *
 *	V0.3 04/14/93							   *
 *	- increased the number of SCB's allocated to keep the system from  *
 *	  running out.  There was a chance of this if more than 4 devices  *
 *	  were used.							   *
 *	- Driver sent to Unisys 04/14/93				   *
 *	V1.0 BETA 05/18/93						   *
 *	- cleaned up unused variables in the code			   *
 *	- corrected a cmn_err message which left out the host adapter name *
 *	  as an argument						   *
 *	- combined HACB and struct scsi_ha into one structure		   *
 *	- incorporated new HIM code					   *
 *	- added more debug output when AHA_DEBUG_ is defined		   *
 *	- moved the release of the HACB if the adapter is found.  It was   *
 *	  being freed before all the attached pointers in the structure    *
 *	  were released causing a PANIC if the adapter fails init	   *
 *	- added adss_disconnect as a control variable in the space.c file  *
 *	  allows the user to control disconnects			   *
 *	- removed unused DMA code from the HIM module to reduce the size of*
 *	  the driver and reduce checking of unused paths for DMA usage to  *
 *	  help performance.						   *
 *	- used the kernel bcmp function instead of the local adss_cmp for  *
 *	  the HIM code as it is a faster routine.			   *
 *	- added a call to HIM6X60AbortSCB when a timeout occurs on a SCSI  *
 *	  command, during poll time.					   *
 *	- added code to do a reset of the SCSI bus, during poll time, if   *
 *	  a previously issued Abort failed.				   *
 *	- moved the display of the found message immediately after the init*
 *	  6x60 function is called so the user won't think the machine is   *
 *	  dead and added a message if anything goes wrong during the real  *
 *	  initialization.						   *
 *	- moved the setting of the scb length from the send code into the  *
 *	  init code as it doesn't need to be done but once.		   *
 *	- added code in the scatter gather function to return the next seg *
 *	  to the HIM code if the offset matches the offset stored in the   *
 *	  s/g structure.  Better performance as well as reliability	   *
 *	- added code to call complete function if the Queue function	   *
 *	  returns a non-SCB_PENDING value.				   *
 *	- delivered to the factory on 5/20/93				   *
 *	V1.1 BETA 06/7/93						   *
 *	- altered the drive init routine to look only for the adapter that *
 *	  is actually supposed to be configured in the system.  I was	   *
 *	  looking for both adapters before.  This was an error.		   *
 *	- sent to the factory 06/10/93					   *
 *	V1.2 BETA 07/07/93						   *
 *	- corrected the above fix to search for both adapters as the kernel*
 *	  does not search both ports even if there are entries in the	   *
 *	  sdevice file.							   *
 *	- sent to the factory 07/08/93					   *
 *	V1.3 11/11/93							   *
 *	- incorporated new HIM code 1004.				   *
 *	- sent to the factory 11/11/93					   *
 *	- Group 567221-00						   *
 *	- Source 567222-00						   *
 *	- for AIC-6x60 V1.0 Release					   *
 *	V1.4 11/23/93							   *
 *	- fixed loop which searched for host adapters, it was stopping	   *
 *	  prematurely.							   *
 *	- fixed pass-thru code with updates from USL			   *
 *	- changed init code to detect whether we have been here before	   *
 *	- for AIC-6x60 V1.0 Release					   *
 *	V1.5 11/29/93							   *
 *	- corrected pass_thru code where one change was not made causing   *
 *	  the loss of the device number (which also contains the target ID)*
 *	V1.6 12/20/93							   *
 *	- added code to allow the driver to timeout on SCSI commands	   *
 *	- sent to the factory and USL on 12/21/93			   *
 ***************************************************************************/

#ifdef	_KERNEL_HEADERS

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/dma.h>
#include <util/mod/moddefs.h>
#include <util/ksynch.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_hier.h>
#include <io/target/sdi/dynstructs.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/hba/adss/him_code/him_scsi.h>
#include <io/hba/adss/him_code/scb6x60.h>
#include <io/hba/adss/adss.h>

/* These must come last: */
#include <io/hba/hba.h>
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#ifdef DDICHECK
#include <io/ddicheck.h>
#endif

#elif defined(_KERNEL)

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/dma.h>
#include <sys/moddefs.h>
#include <sys/ksynch.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/sdi_hier.h>
#include <sys/dynstructs.h>
#include <sys/resmgr.h>
#include "him_code/him_scsi.h"
#include "him_code/scb6x60.h"
#include "adss.h"

/* These must come last: */
#include <sys/hba.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif			/* _KERNEL_HEADERS */

#define ADSS_BLKSHFT	  9	/* PLEASE NOTE:  Currently pass-thru	    */
#define ADSS_BLKSIZE	512	/* SS_READ/SS_WRITE, SM_READ/SM_WRITE	    */
				/* supports only 512 byte blocksize devices */

#define ADSS_MEMALIGN	512
#define ADSS_BOUNDARY	0
#define ADSS_DMASIZE	32
#define PHYSIOCK_MAGIC	0

#define KMEM_ZALLOC kmem_zalloc
#define KMEM_FREE kmem_free

STATIC void	adss_pass_thru0(buf_t *);

STATIC long	adss_tol(char []);

STATIC int	adss_hainit(HACB *),
			adss_alloc_sg(HACB *, int),
			adss_illegal(short, uchar_t, uchar_t),
			adss_wait(int, int);

STATIC SGARRAY	*adss_get_sglist(int);

STATIC ADSS_SCB	*adss_getscb(int);

STATIC void adss_free_sg(HACB *, int),
#ifdef NOT_YET
			adss_timeout(ADSS_SCB *),
#endif
			adss_init_scbs(HACB *, struct req_sense_def *, int),
			adss_flushq(struct scsi_lu *, int, int),
			adss_pass_thru0(buf_t *),
			adss_pass_thru(buf_t *),
			adss_int(struct sb *),
			adss_lockinit(int),
			adss_lockclean(int),
			adss_putq(struct scsi_lu *, sblk_t *),
			adss_next(struct scsi_lu *),
			adss_func(sblk_t *),
			adss_send(int, ADSS_SCB *),
			adss_cmd(SGARRAY *, sblk_t *),
			adss_done(int, int, ADSS_SCB *, int, struct sb *),
			adss_ha_done(int, ADSS_SCB *, int),
			adss_sgdone(int, ADSS_SCB *, int),
			adss_sctgth(struct scsi_lu *, int),
			adss_mem_release(HACB **, int);

STATIC char	adss_init_flags = 0;
STATIC int	adssdevflag = D_NEW;	/* SVR4 style driver	*/
STATIC int	adss_num_scbs = 0;
STATIC int	adss_req_structsize,
			adss_hacb_structsize,
			adss_scb_structsize,
			adss_luq_structsize,
			adss_lucb_structsize;
STATIC volatile boolean_t	adss_waitflag = TRUE;	/* Init time wait flag, TRUE	*/
					/* until interrupt serviced	*/

/* Allocated in space.c */
extern HBA_IDATA_STRUCT	_adssidata[];
extern struct ver_no	adss_sdi_ver;	    /* SDI version structure	*/
extern char		adss_sg_enable;		/* s/g control byte */
extern char		adss_disconnect;	/* disconnect control	*/
#ifdef NOT_YET
extern char		adss_timeout_enable;
extern int		adss_timeout_count;
#endif
extern int		adss_cntls;
extern int		adss_io_per_tar;
extern int		adss_gtol[]; /* xlate global hba# to loc */
extern int		adss_ltog[]; /* local hba# to global     */
extern long		sdi_started;    /* SDI initialization flag	*/

static char		adssinit_time;	    /* Init time (poll) flag	*/
static int		adss_mod_dynamic = 0;
HACB			*adss_sc_ha[MAX_6X60];

STATIC int	adss_devflag = HBA_MP;
STATIC LKINFO_DECL(adss_lkinfo_sgarray, "IO:adss:ha->ha_sg_lock", 0);
STATIC LKINFO_DECL(adss_lkinfo_scb, "IO:adss:ha->ha_scb_lock", 0);
STATIC LKINFO_DECL(adss_lkinfo_hba, "IO:adss:ha->ha_hba_lock", 0);
STATIC LKINFO_DECL(adss_lkinfo_q, "IO:adss:q->q_lock", 0);

/*
 * This pointer will be used to reference the new idata structure
 * returned by sdi_hba_autoconf.  In the case of releases that do
 * not support autoconf, it will simply point to the idata structure
 * from the Space.c.
 */
HBA_IDATA_STRUCT	*adssidata;

/* The sm_poolhead struct is used for the dynamic struct allocation routines */
/* as pool to extract from and return structs to. This pool provides 28 byte */
/* structs (the size of sblk_t structs roughly).			     */
extern struct head	sm_poolhead;

#ifdef AHA_DEBUG_
int			adssdebug = 0;
#endif
#ifdef DEBUG
STATIC	int	adss_panic_assert = 0;
#endif

#ifdef AHA_DEBUG_TRACE
#define AHA_DTRACE cmn_err(CE_CONT,"%d-%s:",__LINE__,__FILE__);
#else
#define AHA_DTRACE
#endif

#define		DRVNAME	"Adaptec AIC-6X60 SCSI"

MOD_HDRV_WRAPPER(adss, adss_load, adss_unload, NULL, DRVNAME);
HBA_INFO(adss, &adss_devflag, 0x10000);

int
adss_load(void)
{
	AHA_DTRACE;

	adss_mod_dynamic = 1;

	if( adssinit()) {
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
		sdi_acfree(adssidata, adss_cntls);
		return( ENODEV );
	}
	if ( adssstart()) {
		/*
		 * At this point we have allocated a new idata array,
		 * free it before failing.
		 */
		sdi_acfree(adssidata, adss_cntls);
		return(ENODEV);
	}
	return(0);
}

int
adss_unload(void)
{
	AHA_DTRACE;

	return(EBUSY);
}

#ifdef NOT_YET
STATIC void
adss_timeout(ADSS_SCB *scb_to_abort)
{
	ADSS_SCB	*scb;
	HACB		*ha;

	AHA_DTRACE;

	ha = adss_sc_ha[scb_to_abort->adapter];
	scb = adss_getscb(scb_to_abort->adapter);

	scb->function = SCB_ABORT_REQUESTED;
	HIM6X60TerminateSCB(ha, scb, scb_to_abort);
	FREEBLK(scb);
}
#endif

/*
 * adss_tol()
 *	This routine flips a 4 byte Intel ordered long to a 4 byte SCSI
 *	style long.
 */
STATIC long
adss_tol(char adr[])
{
	union {
		char  tmp[4];
		long lval;
	}  x;

	x.tmp[0] = adr[3];
	x.tmp[1] = adr[2];
	x.tmp[2] = adr[1];
	x.tmp[3] = adr[0];
	return(x.lval);
}

/**************************************************************************
** Function name: adssxlat()						 **
** Description:								 **
**	Perform the virtual to physical translation on the SCB		 **
**	data pointer. 							 **
**************************************************************************/

/*ARGSUSED*/
HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	sblk_t *sp = (sblk_t *) hbap;

	AHA_DTRACE;

	if(sp->sbp->sb.SCB.sc_link) {
		cmn_err(CE_WARN, "Adaptec: Linked commands NOT available");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if(sp->sbp->sb.SCB.sc_datapt) {
		sp->s_addr = sp->sbp->sb.SCB.sc_datapt;
	} else
		sp->sbp->sb.SCB.sc_datasz = 0;

	HBAXLAT_RETURN (SDI_RET_OK);
}

/***************************************************************************
** Function name: adssinit()						  **
** Description:								  **
**	This is the initialization routine for the SCSI HA driver.	  **
**	All data structures are initialized, the boards are initialized	  **
**	and the EDT is built for every HA configured in the system.	  **
***************************************************************************/
int
adssinit(void)
{
	HACB		*hacb;
	int			found, i, port, sleepflag, mem_release, adss_hacnt;
	uint_t		bus_type;
	struct req_sense_def	*req_sense;

	AHA_DTRACE;

	if(adss_init_flags) {
		cmn_err(CE_WARN, "Adaptec AIC-6X60: Already initialized.");
		return(-1);
	}

	/* determine I/O bus configuration of host processor */

	if (drv_gethardware(IOBUS_TYPE, &bus_type) == -1) {
		/*
		 *+ adss: cannot determine bus type at init time
		 */
		cmn_err(CE_WARN,
		"%s: drv_gethardware(IOBUS_TYPE) fails", adssidata[0].name);
		return(-1);
	}

	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	adssidata = sdi_hba_autoconf("adss", _adssidata, &adss_cntls);
	if(adssidata == NULL)    {
		return (-1);
	}

	HBA_IDATA(adss_cntls);
	
	adssinit_time = TRUE;

	for(i = 0; i < SDI_MAX_HBAS; i++)
		adss_gtol[i] = adss_ltog[i] = -1;

	sdi_started = TRUE;

	sleepflag = adss_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	adss_sdi_ver.sv_release = 1;
	if (bus_type == BUS_EISA)
		adss_sdi_ver.sv_machine = SDI_386_EISA;
	else if (bus_type == BUS_MCA)
		adss_sdi_ver.sv_machine = SDI_386_MCA;
	else
		adss_sdi_ver.sv_machine = SDI_386_AT;
	adss_sdi_ver.sv_modes   = SDI_BASIC1;

	/*
	 * To keep the user from over-flowing memory allocation, let's force
	 * adss_io_per_tar to a more resonable number if the user gets crazy
	 */
	if(adss_io_per_tar > 2 || adss_io_per_tar <= 0)
		adss_io_per_tar = 2;
	adss_num_scbs = (adss_io_per_tar * 8) + 8;

#ifdef NOT_YET
	adss_timeout_count *= 100;
#endif

	adss_hacb_structsize = sizeof(HACB);
	adss_lucb_structsize = 64 * sizeof(LUCB);
	adss_scb_structsize = adss_num_scbs * (sizeof(ADSS_SCB));
	adss_req_structsize = adss_num_scbs * (sizeof(struct req_sense_def));
	adss_luq_structsize = MAX_EQ * (sizeof(struct scsi_lu));

#ifdef AHA_DEBUG_
	if(adss_hacb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: hacb's exceed pagesize (may cross physical page boundary)", adssidata[0].name);
	if(adss_luq_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: LUQs exceed pagesize (may cross physical page boundary)", adssidata[0].name);
	if(adss_req_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: Request Sense Blocks exceed pagesize (may cross physical page boundary)", adssidata[0].name);
	if(adss_scb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: CCBs exceed pagesize (may cross physical page boundary)", adssidata[0].name);
	if(adss_lucb_structsize > PAGESIZE)
		cmn_err(CE_WARN, "%s: him_data_blocks exceed pagesize (may cross physical page boundary)", adssidata[0].name);
#endif

	adss_init_flags = 1;
	found = 0;

	for(adss_hacnt = 0; adss_hacnt < adss_cntls; adss_hacnt++) {
		port = adssidata[adss_hacnt].ioaddr1;

		if(HIM6X60FindAdapter((AIC6X60_REG *)port) == FALSE) {
			cmn_err(CE_NOTE,"!%s: No HBA's found at port 0x%x.",
				adssidata[0].name, port);
			continue;
		}

		cmn_err(CE_NOTE, "!V1.6 %s: found at base port 0x%x",
		    	adssidata[adss_hacnt].name, port);

		mem_release = 0;

		hacb = (HACB *)KMEM_ZALLOC(adss_hacb_structsize, sleepflag);
		if(hacb == NULL) {
			cmn_err(CE_WARN, "!%s: Cannot allocate HACB blocks",
				adssidata[adss_hacnt].name);
			continue;
		}
		mem_release |= HA_HACB_REL;
		hacb->baseAddress = (AIC6X60_REG *)port;
		adss_sc_ha[adss_hacnt] = hacb;
		hacb->ha_name = adssidata[adss_hacnt].name;

		req_sense = \
		   (struct req_sense_def *)KMEM_ZALLOC(adss_req_structsize, sleepflag);
		if(req_sense == NULL) {
			cmn_err(CE_WARN, "!%s: Cannot allocate request sense blocks",
			    adssidata[adss_hacnt].name);
			adss_mem_release(&adss_sc_ha[adss_hacnt], mem_release);
			continue;
		}
		mem_release |= HA_REQS_REL;

		hacb->ha_scb = (ADSS_SCB *)KMEM_ZALLOC(adss_scb_structsize, sleepflag);
		if (hacb->ha_scb == NULL) {
			cmn_err(CE_WARN, "!%s: Cannot allocate command blocks",
				adssidata[adss_hacnt].name);
			adss_mem_release(&adss_sc_ha[adss_hacnt], mem_release);
			continue;
		}
		mem_release |= HA_SCB_REL;
		adss_init_scbs(hacb, req_sense, adss_num_scbs);

		hacb->ha_dev = (struct scsi_lu *)KMEM_ZALLOC(adss_luq_structsize, \
		                   sleepflag);
		if (hacb->ha_dev == NULL) {
			cmn_err(CE_WARN, "!%s: Cannot allocate lu queues",
				adssidata[adss_hacnt].name);
			adss_mem_release(&adss_sc_ha[adss_hacnt], mem_release);
			continue;
		}
		mem_release |= HA_DEV_REL;
		hacb->ha_id = adssidata[adss_hacnt].ha_id;
		hacb->ha_vect = adssidata[adss_hacnt].iov;

		hacb->ha_lucb = (LUCB *)KMEM_ZALLOC(adss_lucb_structsize, sleepflag);
		if (hacb->ha_lucb == NULL) {
			cmn_err(CE_WARN, "!%s: Cannot allocate LUCB command blocks",
				adssidata[adss_hacnt].name);
			adss_mem_release(&adss_sc_ha[adss_hacnt], mem_release);
			continue;
		}
		mem_release |= HA_LUCB_REL;

		if(adss_alloc_sg(hacb, adss_num_scbs) != 0) {
			adss_mem_release(&adss_sc_ha[adss_hacnt], mem_release);
			continue;
		}
		mem_release |= HA_SG_REL;

		/* init HA communication */
		if(!adss_hainit(hacb)) {
			adss_mem_release(&adss_sc_ha[adss_hacnt], mem_release);
			continue;
		}

		adssidata[adss_hacnt].active = 1;

		adss_lockinit(adss_hacnt);

		found++;
	}

	if(!found) {
		cmn_err(CE_WARN, "!%s: No HBA's found.", adssidata[0].name);
		return(-1);
	}
	/*
	 * Attach interrupts for each "active" device. The driver
	 * must set the active field of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here in init() as opposed
	 * to load() because this call is required for both static
	 * and loadable autoconfig drivers.
 	 */
	sdi_intr_attach(adssidata, adss_cntls, adssintr, adss_devflag);
	return(0);
}

/****************************************************************************
** Function name: adssstart()						   **
** Description:								   **
**	Called by kernel to perform driver initialization		   **
**	after the kernel data area has been initialized.		   **
****************************************************************************/
int
adssstart(void)
{
	int			found, cntl_num, adss_hacnt;

	AHA_DTRACE;

	for(found = 0, adss_hacnt = 0; adss_hacnt < adss_cntls; adss_hacnt++) {

		if (!adssidata[adss_hacnt].active)
			continue;

		if((cntl_num = sdi_gethbano(adssidata[adss_hacnt].cntlr)) < 0) {
     			cmn_err(CE_WARN, "!%s: No HBA number available. %d",
				adssidata[adss_hacnt].name, cntl_num);
			adssidata[adss_hacnt].active = 0;
			continue;
		}

		adssidata[adss_hacnt].cntlr = cntl_num;
		adss_gtol[cntl_num] = adss_hacnt;
		adss_ltog[adss_hacnt] = cntl_num;

		if((cntl_num = sdi_register(&adsshba_info, &adssidata[adss_hacnt])) < 0) {
			cmn_err(CE_WARN, "!%s: SDI registry failure %d.",
				adssidata[adss_hacnt].name, cntl_num);
			adssidata[adss_hacnt].active = 0;
			continue;
		}
		found++;
	}

	for(cntl_num = 0; cntl_num < adss_cntls; cntl_num++) {
		adss_lockclean(cntl_num);
		if (!adssidata[cntl_num].active)
			adss_mem_release(&adss_sc_ha[cntl_num], HA_ALL_REL);
	}

	if(!found) {
		if (adssidata == NULL)
			adssidata = _adssidata;
		cmn_err(CE_WARN, "!%s: No attached devices found.", adssidata[0].name);
		return(-1);
	}

	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally.
	 */
	adssinit_time = FALSE;
	return(0);
}

/***************************************************************************
** adss_hainit()							  **
**	This is the guy that does all the hardware initialization of the  **
**	adapter after it has been determined to be in the system.	  **
***************************************************************************/
STATIC int
adss_hainit(HACB *hacb)
{

	AHA_DTRACE;

	hacb->length = sizeof(HACB);
	if(HIM6X60GetConfiguration(hacb) == FALSE) {
		cmn_err(CE_WARN, "%s: Unable to get configuration information\n",
			hacb->ha_name);
		return(0);
	}

	if(hacb->DefaultConfiguration == FALSE) {
		if(hacb->ha_vect != hacb->IRQ) {
			cmn_err(CE_WARN, "%s: Interrupt should be %d, currently set to %d",
			  hacb->ha_name, hacb->ha_vect, hacb->IRQ);
			return(0);
		}
	} else {
		hacb->IRQ = hacb->ha_vect;
		hacb->CheckParity = TRUE;
		hacb->InitiateSDTR = TRUE;
	}

	hacb->FastSCSI = TRUE;
	hacb->UseDma = FALSE;
	if(adss_disconnect)
		hacb->NoDisconnect = FALSE;
	else
		hacb->NoDisconnect = TRUE;

	if(HIM6X60Initialize(hacb) == FALSE) {
		return(0);
	}
	return(1);
}

/***************************************************************************
** Function name: adss_init_scbs()					  **
** Description:								  **
**	Initialize the adapter SCB free list.				  **
***************************************************************************/
STATIC void
adss_init_scbs(HACB *ha, struct req_sense_def *req_sense, int cnt)
{
	ADSS_SCB	*cp;
	int		i;

	AHA_DTRACE;

	for(i = 0; i < cnt; i++) {
		cp = &ha->ha_scb[i];

		cp->length = sizeof(ADSS_SCB);
		cp->c_next = &ha->ha_scb[i+1];
		cp->cdb = &cp->scsi_cmd[0];
		cp->senseData = (uchar_t *)&req_sense[i];
		cp->senseDataLength = sizeof(struct req_sense_def);
	}
	cp->c_next = &ha->ha_scb[0];
	ha->ha_scb_next = cp;
}

/**************************************************************************
** Function name: adssopen()						 **
** Description:								 **
** 	Driver open() entry point. It checks permissions, and in the	 **
**	case of a pass-thru open, suspends the particular LU queue.	 **
**************************************************************************/

/*ARGSUSED*/
int
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;
	int	c = adss_gtol[SC_HAN(dev)];
	int	t = SC_TCN(dev);
	int	l = SC_LUN(dev);
	struct scsi_lu *q;
	HACB		*ha = adss_sc_ha[c];

	AHA_DTRACE;

	if (adss_illegal(SC_HAN(dev), t, l)) {
		return(ENXIO);
	}

	if (t == ha->ha_id)
		return(0);

	/* This is the pass-thru section */

	q = &LU_Q(c, t, l);

	ADSS_SCSILU_LOCK(q);
 	if ((q->q_count > 0)  || (q->q_flag & (QBUSY | QSUSP | QPTHRU))) {
		ADSS_SCSILU_UNLOCK(q);
		return(EBUSY);
	}

	q->q_flag |= QPTHRU;
	ADSS_SCSILU_UNLOCK(q);
	return(0);
}

/***************************************************************************
** Function name: adssclose()						  **
** Description:								  **
** 	Driver close() entry point.  In the case of a pass-thru close	  **
**	it resumes the queue and calls the target driver event handler	  **
**	if one is present.						  **
***************************************************************************/

/*ARGSUSED*/
int
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	int	c = adss_gtol[SC_HAN(dev)];
	int	t = SC_TCN(dev);
	int	l = SC_LUN(dev);
	struct scsi_lu *q;
	HACB		*ha = adss_sc_ha[c];

	AHA_DTRACE;

	if (t == ha->ha_id)
		return(0);

	q = &LU_Q(c, t, l);

	ADSS_CLEAR_QFLAG(q,QPTHRU);

#ifdef AHA_DEBUG_
	sdi_aen(SDI_FLT_PTHRU, c, t, l);
#else
	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
#endif

	ADSS_SCSILU_LOCK(q);
	adss_next(q);
	return(0);
}

/**************************************************************************
** Function name: adssintr()						 **
** Description:								 **
**	Driver interrupt handler entry point.  Called by kernel when	 **
**	a host adapter interrupt occurs.				 **
**************************************************************************/
void
adssintr(uint vect)
{
	HACB		*hacb;
	int		adapter;
        HACB    *ha;

	AHA_DTRACE;

	for(adapter = 0; adapter < adss_cntls; adapter++) {
		if(!(hacb = adss_sc_ha[adapter]))
			continue;
		if(hacb->ha_vect != vect)
			continue;
		ha = hacb;
                ADSS_HBA_LOCK(ha);
#ifdef AHA_DEBUG_
		if(HIM6X60ISR(hacb) == TRUE)
			printf("(I)");
#else
		HIM6X60ISR(hacb);
#endif
		ADSS_HBA_UNLOCK(ha);
	}
}

void
HIM6X60CompleteSCB(HACB *ha, ADSS_SCB *scb_ptr)
{
	struct scsi_lu *q;
	int			adapter;
	int			int_status;
	int			lun, target;
	uchar_t	function;

	AHA_DTRACE;

#ifdef NOT_YET
	if(adss_timeout_enable)
		untimeout(scb_ptr->tm_id);
#endif

	ADSS_HBA_UNLOCK(ha);
	adapter = scb_ptr->adapter;

	if(!adssidata[adapter].active)
		return;
	target = scb_ptr->targetID;
	int_status = scb_ptr->scbStatus;
	function = scb_ptr->function;
	switch(SCB_STATUS(int_status)) {
		case SCB_PENDING:
		case SCB_COMPLETED_OK:
		case SCB_ABORTED:
		case SCB_ABORT_FAILURE:
		case SCB_ERROR:
		case SCB_BUSY:
		case SCB_INVALID_SCSI_BUS:
		case SCB_TIMEOUT:
		case SCB_SELECTION_TIMEOUT:
		case SCB_MESSAGE_REJECTED:
		case SCB_SCSI_BUS_RESET:
		case SCB_PARITY_ERROR:
		case SCB_REQUEST_SENSE_FAILURE:
		case SCB_DATA_OVERRUN:
		case SCB_BUS_FREE:
		case SCB_PROTOCOL_ERROR:
		case SCB_INVALID_LENGTH:
		case SCB_INVALID_LUN:
		case SCB_INVALID_TARGET_ID:
		case SCB_INVALID_FUNCTION:
		case SCB_ERROR_RECOVERY:
		case SCB_TERMINATED:
		case SCB_TERMINATE_IO_FAILURE:
			if(function == SCB_EXECUTE) {
				ADSS_HBA_LOCK(ha);
				ha->ha_npend--;
				ADSS_HBA_UNLOCK(ha);
				lun = scb_ptr->lun;
				q = &LU_Q(adapter, target, lun);
				ADSS_SCSILU_LOCK(q);
				q->q_count--;
				if(q->q_count < adss_io_per_tar) {
					q->q_flag &= ~QBUSY;
				}
				ADSS_SCSILU_UNLOCK(q);
				adss_sgdone(adapter, scb_ptr, int_status);
			} else if(function == SCB_BUS_DEVICE_RESET) {
				adss_ha_done(adapter, scb_ptr, int_status);
			}
			break;
		default:
			cmn_err(CE_WARN, "!%s: Invalid interrupt status - 0x%x",
				 ha->ha_name, int_status);
			break;
	}
	if(function == SCB_EXECUTE || function == SCB_BUS_DEVICE_RESET) {
		adss_waitflag = FALSE;
	}
	ADSS_HBA_LOCK(ha);

}

/**************************************************************************
** Function: adss_sgdone()						 **
** Description:								 **
**	This routine is used as a front end to calling adss_done.	 **
**	This is the cleanest way I found to cleanup a local s/g command. **
**	One should note, however, with this scheme, if an error occurs	 **
**	during the local s/g command, the system will think an error	 **
**	will have occured on all the commands.				 **
**************************************************************************/
STATIC void
adss_sgdone(int c, ADSS_SCB *cp, int status)
{
	sblk_t		*sb;
	struct sb	*sp;
	int		i;
	long			x;

	AHA_DTRACE;

	if(cp->c_sg_p != NULL) {
		x = cp->c_sg_p->sg_len;
		x--;
		for(i = 0; i < x; i++) {
			sb = cp->c_sg_p->spcomms[i];
			adss_done(SG_NOSTART, c, cp, status, &sb->sbp->sb);
		}
		sb = cp->c_sg_p->spcomms[i];
		adss_done(SG_START, c, cp, status, &sb->sbp->sb);
		cp->c_sg_p->sg_flags = SG_FREE;
		cp->c_sg_p = NULL;

		FREEBLK(cp);
	} else { /* call target driver interrupt handler */
		sp = cp->c_bind;
		adss_done(SG_START, c, cp, status, sp);

		FREEBLK(cp);
	}
}

/**************************************************************************
** Function name: adss_ha_done()					 **
** Description:								 **
**	This is the interrupt handler routine for SCSI jobs which have	 **
**	a controller CB and a SFB structure defining the job.		 **
**************************************************************************/
STATIC void
adss_ha_done(int c, ADSS_SCB *cp, int status)
{
	struct scsi_lu	*q;
	struct sb	*sp;
	char			t,l;

	AHA_DTRACE;

	t = cp->targetID;
	l = cp->lun;
	q = &LU_Q(c, t, l);

	sp = cp->c_bind;

	/* Determine completion status of the job */

	switch(SCB_STATUS(status)) {
		case SCB_INVALID_LENGTH:
			sp->SFB.sf_comp_code = (ulong_t)SDI_SCBERR;
			break;
		case SCB_INVALID_SCSI_BUS:
		case SCB_INVALID_TARGET_ID:
		case SCB_INVALID_LUN:
			sp->SFB.sf_comp_code = (ulong_t)SDI_NOTEQ;
			break;
		case SCB_COMPLETED_OK:
			sp->SFB.sf_comp_code = SDI_ASW;
			adss_flushq(q, SDI_CRESET, 0);
			break;
		case SCB_TERMINATED:
		case SCB_TERMINATE_IO_FAILURE:
		case SCB_ABORTED:
		case SCB_ABORT_FAILURE:
			sp->SFB.sf_comp_code = (ulong_t)SDI_ABORT;
			break;
		case SCB_SCSI_BUS_RESET:
			sp->SFB.sf_comp_code = (ulong_t)SDI_RESET;
			break;
		case SCB_TIMEOUT:
			sp->SFB.sf_comp_code = (ulong_t)SDI_TIME;
			break;
		case SCB_SELECTION_TIMEOUT:
			sp->SFB.sf_comp_code = (ulong_t)SDI_NOSELE;
			break;
		case SCB_PENDING:
			sp->SFB.sf_comp_code = (ulong_t)SDI_PROGRES;
			break;
		case SCB_PROTOCOL_ERROR:
			sp->SFB.sf_comp_code = (ulong_t)SDI_TCERR;
			break;
		case SCB_ERROR:
		case SCB_BUSY:
		case SCB_MESSAGE_REJECTED:
		case SCB_PARITY_ERROR:
		case SCB_REQUEST_SENSE_FAILURE:
		case SCB_DATA_OVERRUN:
		case SCB_BUS_FREE:
		case SCB_ERROR_RECOVERY:
			sp->SFB.sf_comp_code = (ulong_t)SDI_ERROR;
			break;
		default:
			ASSERT(adss_panic_assert);
			sp->SFB.sf_comp_code = (ulong_t)SDI_ERROR;
			break;
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	FREEBLK(cp);

	ADSS_SCSILU_LOCK(q);
	adss_next(q);
} 

/**************************************************************************
** Function name: adss_done()						 **
** Description:								 **
**	This is the interrupt handler routine for SCSI jobs which have	 **
**	a controller CB and a SCB structure defining the job.		 **
**************************************************************************/
STATIC void
adss_done(int flag, int c, ADSS_SCB *cp, int status, struct sb *sp)
{
	struct scsi_lu	*q;
	int		i;
	char			target, lun;

	AHA_DTRACE;

	target = cp->targetID;
	lun = cp->lun;
	q = &LU_Q(c, target, lun);

	ADSS_CLEAR_QFLAG(q,QSENSE);	/* Old sense data now invalid */

	/* Determine completion status of the job */
	switch(SCB_STATUS(status)) {
		case SCB_INVALID_LENGTH:
			sp->SCB.sc_comp_code = (ulong_t)SDI_SCBERR;
			break;
		case SCB_INVALID_SCSI_BUS:
		case SCB_INVALID_TARGET_ID:
		case SCB_INVALID_LUN:
			sp->SCB.sc_comp_code = (ulong_t)SDI_NOTEQ;
			break;
		case SCB_COMPLETED_OK:
			sp->SCB.sc_comp_code = SDI_ASW;
			break;
		case SCB_ABORTED:
		case SCB_ABORT_FAILURE:
			sp->SCB.sc_comp_code = (ulong_t)SDI_ABORT;
			break;
		case SCB_SCSI_BUS_RESET:
			sp->SCB.sc_comp_code = (ulong_t)SDI_RESET;
			break;
		case SCB_TIMEOUT:
			sp->SCB.sc_comp_code = (ulong_t)SDI_TIME;
			break;
		case SCB_SELECTION_TIMEOUT:
			sp->SCB.sc_comp_code = (ulong_t)SDI_NOSELE;
			break;
		case SCB_PENDING:
			sp->SCB.sc_comp_code = (ulong_t)SDI_PROGRES;
			break;
		case SCB_PROTOCOL_ERROR:
			sp->SCB.sc_comp_code = (ulong_t)SDI_TCERR;
			break;
		case SCB_ERROR:
			sp->SCB.sc_status = cp->targetStatus;
			/* Cache request sense info (note padding) */
			if(cp->scbStatus & SCB_SENSE_DATA_VALID) {
				sp->SCB.sc_comp_code = (ulong_t)SDI_CKSTAT;
				if(cp->senseDataLength < sizeof(q->q_sense))
					i = cp->senseDataLength;
				else
					i = sizeof(q->q_sense);
				bcopy((caddr_t)(cp->senseData), (caddr_t)(sdi_sense_ptr(sp))+1, i);
	
				bcopy((caddr_t)(cp->senseData), (caddr_t)(&q->q_sense)+1, i);
			} else {
				sp->SCB.sc_comp_code = (ulong_t)SDI_ERROR;
			}
			ADSS_SET_QFLAG(q,QSENSE);
			break;

		case SCB_BUSY:
		case SCB_MESSAGE_REJECTED:
		case SCB_PARITY_ERROR:
		case SCB_REQUEST_SENSE_FAILURE:
		case SCB_DATA_OVERRUN:
		case SCB_BUS_FREE:
		case SCB_ERROR_RECOVERY:
		default:
			ASSERT(adss_panic_assert);
			sp->SCB.sc_comp_code = (ulong_t)SDI_ERROR;
			break;
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	if(flag == SG_START) {
		ADSS_SCSILU_LOCK(q);
		adss_next(q);
	}
} 

/**************************************************************************
** Function name: adss_int()						 **
** Description:								 **
**	This is the interrupt handler for pass-thru jobs.  It just	 **
**	wakes up the sleeping process.					 **
**************************************************************************/
STATIC void
adss_int(struct sb *sp)
{
	buf_t  *bp;

	AHA_DTRACE;

	bp = (buf_t *)sp->SCB.sc_wd;
	biodone(bp);
}

/**************************************************************************
** Function: adss_illegal()						 **
** Description: Used to verify commands					 **
**************************************************************************/
STATIC int
adss_illegal(short hba, uchar_t scsi_id, uchar_t lun)
{
	AHA_DTRACE;

	if(sdi_redt(hba, scsi_id, lun))
		return(0);

	return(1);
}

/**************************************************************************
** Function name: adss_func()						 **
** Description:								 **
**	Create and send an SFB associated command. 			 **
**************************************************************************/
STATIC void
adss_func(sblk_t *sp)
{
	struct scsi_ad	*sa;
	HACB		*ha;
	ADSS_SCB	*cp;
	int			c, target, lun;

	AHA_DTRACE;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = adss_gtol[SDI_HAN(sa)];
 	ha = adss_sc_ha[c];
	target = SDI_TCN(sa);
	lun = sa->sa_lun;

	cp = adss_getscb(c);
	cp->function = SCB_BUS_DEVICE_RESET;
	cp->targetID = (uchar_t)target;
	cp->lun = (uchar_t)lun;
	cp->dataLength = 0;
	cmn_err(CE_WARN, "%s: TC%d LUN%d is being reset",
		ha->ha_name, target, lun);
	cp->adapter = c;

	ADSS_HBA_LOCK(ha);
	if (HIM6X60QueueSCB(ha, cp) != SCB_PENDING) {
		ADSS_HBA_UNLOCK(ha);
		HIM6X60CompleteSCB(ha, cp);
	} else
		ADSS_HBA_UNLOCK(ha);
}

/*
 * STATIC void
 * adss_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
adss_pass_thru0(buf_t *bp)
{
	int	c = adss_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	struct scsi_lu	*q = &LU_Q(c, t, l);

	AHA_DTRACE;

	if (!q->q_bcbp) {
		struct sb *sp;
		struct scsi_ad *sa;

		sp = bp->b_priv.un_ptr;
		sa = &sp->SCB.sc_dev;
		if ((q->q_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		q->q_bcbp->bcb_addrtypes = BA_KVIRT;
		q->q_bcbp->bcb_flags = BCB_SYNCHRONOUS;
		q->q_bcbp->bcb_max_xfer = adsshba_info.max_xfer;
		q->q_bcbp->bcb_physreqp->phys_align = ADSS_MEMALIGN;
		q->q_bcbp->bcb_physreqp->phys_boundary = ADSS_BOUNDARY;
		q->q_bcbp->bcb_physreqp->phys_dmasize = ADSS_DMASIZE;
		q->q_bcbp->bcb_granularity = 1;
	}

	buf_breakup(adss_pass_thru, bp, q->q_bcbp);
}

/**************************************************************************
** Function name: adss_pass_thru()					 **
** Description:								 **
**	Send a pass-thru job to the HA board.				 **
**************************************************************************/
STATIC void
adss_pass_thru(buf_t *bp)
{
	int	c = adss_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	struct scsi_lu	*q;
	struct sb *sp;
	caddr_t *addr;
	char op;
	daddr_t blkno_adjust;

	AHA_DTRACE;

	sp = (struct sb *)bp->b_private;

	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = adss_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	q = &LU_Q(c, t, l);

	/*
	 * (This is a workaround for the 2G limitation for physiock offset.)
	 * Set new block number in the SCSI command if breakup occurred
	 * by adjusting the starting block number with the adjustment 
	 * from b_blkno.  Starting block number was saved in b_priv2,
	 * and having told physiock that the offset was 0, we add
	 * the adjustment now.
	 */
	blkno_adjust = bp->b_blkno;
	op = q->q_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)(void *)q->q_sc_cmd;
		daddr_t blkno;

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
			scs->ss_addr  = sdi_swap16(blkno);
		}
		scs->ss_len   = (char)(bp->b_bcount >> ADSS_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		daddr_t blkno;
		struct scm *scm = (struct scm *)(void *)((char *)q->q_sc_cmd - 2);

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scm->sm_addr = sdi_swap32(blkno);
		}
		scm->sm_len  = sdi_swap16(bp->b_bcount >> ADSS_BLKSHFT);
	}

	sdi_icmd(sp, KM_SLEEP);
}

/*
 * Function name: adss_next()
 * Description:
 *	Attempt to send the next job on the logical unit queue.
 *	Only SFB jobs are sent if the Q is busy.
 */
STATIC void
adss_next(struct scsi_lu *q)
{
	sblk_t			*sp;
	struct scsi_ad		*sa;
	int			adapter;

	AHA_DTRACE;
	ASSERT(q);

	if (q->q_flag & QBUSY) {
		ADSS_SCSILU_UNLOCK(q);
		return;
	}

	if((sp = q->q_first) == NULL) {	 /*  queue empty  */
		q->q_depth = 0;
		ADSS_SCSILU_UNLOCK(q);
		return;
	}

	if (q->q_flag & QSUSP && sp->sbp->sb.sb_type == SCB_TYPE) {
		ADSS_SCSILU_UNLOCK(q);
		return;
	}

/*
 * The test conditions:
 * 1) If the command type is a function, instead of a normal SCSI command
 *    can't do s/g.  So do [1a].
 * 2) There is more than 1 command in the queue.
 * 3) If the current command is already a s/g command from the page I/O stuff
 *    just do it in a normal manner.
 * 4) All the test conditions were right, so we have a chance to do a s/g
 *    command.
 * 5) Shucks!  One of the test conditions (2, or 3) failed and we have to
 *    do the command normally.
 */
	if(sp->sbp->sb.sb_type == SFB_TYPE) {		/* [1]  */
		if ( !(q->q_first = sp->s_next))
			q->q_last = NULL;
		else
			q->q_first->s_prev = NULL;
		q->q_depth--;
		ADSS_SCSILU_UNLOCK(q);
		adss_func(sp);				/* [1a] */
	} else {
		if(q->q_depth > 1 && adss_sg_enable == 1) { /* [2]  */
			sa = &sp->sbp->sb.SCB.sc_dev;
			adss_sctgth(q, adss_gtol[SDI_HAN(sa)]);
		} else {
			if(!(q->q_first = sp->s_next))
				q->q_last = NULL;
			else
				q->q_first->s_prev = NULL;
			q->q_depth--;
			ADSS_SCSILU_UNLOCK(q);
			adss_cmd(NULL , sp);	/* [5]  */
		}
	}

	if (adssinit_time) {		/* need to poll */
		if(sp->sbp->sb.sb_type == SFB_TYPE)
			adapter = adss_gtol[SDI_HAN(&sp->sbp->sb.SFB.sf_dev)];
		else
			adapter = adss_gtol[SDI_HAN(&sp->sbp->sb.SCB.sc_dev)];
		if(adss_wait(adapter, 30000) == LOC_FAILURE)  {
			sp->sbp->sb.SCB.sc_comp_code = (ulong_t)SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		}
	}
}

/**************************************************************************
** Function name: adss_putq()						 **
** Description:								 **
**	Put a job on a logical unit queue.  Jobs are enqueued		 **
**	on a priority basis.						 **
**************************************************************************/
STATIC void
adss_putq(struct scsi_lu *q, sblk_t *sp)
{
	int cls = ADSS_QUECLASS(sp);

	AHA_DTRACE;

	/* 
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if(!q->q_first || (cls <= ADSS_QUECLASS(q->q_last))) {
		if(q->q_first) {
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		} else {
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;

	} else {
		sblk_t *nsp = q->q_first;

		while(ADSS_QUECLASS(nsp) >= cls)
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if(nsp->s_prev)
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}
	q->q_depth++;
}

/**************************************************************************
** Function name: adss_flushq()						 **
** Description:								 **
**	Empty a logical unit queue.  If flag is set, remove all jobs.	 **
**	Otherwise, remove only non-control jobs.			 **
**************************************************************************/
STATIC void
adss_flushq(struct scsi_lu *q, int cc, int flag)
{
	sblk_t  *sp, *nsp;

	AHA_DTRACE;
	ASSERT(q);

	ADSS_SCSILU_LOCK(q);
	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_depth = 0;
	ADSS_SCSILU_UNLOCK(q);

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (ADSS_QUECLASS(sp) > HBA_QNORM)) {
			ADSS_SCSILU_LOCK(q);
			adss_putq(q, sp);
			ADSS_SCSILU_UNLOCK(q);
		} else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}
		sp = nsp;
	}
}

/**************************************************************************
** Function name: adss_send()						 **
** Description:								 **
**	Send a command to the host adapter board.			 **
**************************************************************************/
STATIC void
adss_send(int c, ADSS_SCB *cp)
{
	HACB		*ha = adss_sc_ha[c];
	struct scsi_lu	*q;
	UCHAR	result;

	AHA_DTRACE;

	q = &LU_Q(c, cp->targetID, cp->lun);
	ADSS_SCSILU_LOCK(q);
	q->q_count++;
	if(q->q_count >= adss_io_per_tar) {
		q->q_flag |= QBUSY;
	}
	ADSS_SCSILU_UNLOCK(q);

	ADSS_HBA_LOCK(ha);

	ha->ha_npend++;

#ifdef NOT_YET
	if(adss_timeout_enable)
		cp->tm_id = timeout(adss_timeout, (caddr_t)cp, adss_timeout_count);
#endif

	result = HIM6X60QueueSCB(ha, cp);
	ADSS_HBA_UNLOCK(ha);

	if (result != SCB_PENDING)
		HIM6X60CompleteSCB(ha, cp);
}

/**************************************************************************
** Function name: adss_getscb()						 **
** Description:								 **
**	Allocate a controller command block structure.			 **
**************************************************************************/
STATIC ADSS_SCB *
adss_getscb(int c)
{
	HACB		*ha = adss_sc_ha[c];
	ADSS_SCB	*cp;
	int		cnt;
	pl_t		oip;

	AHA_DTRACE;

	cnt = 0;

	ADSS_SCB_LOCK(oip);
	cp = ha->ha_scb_next;
	while(cp->c_active == TRUE) {
		if(++cnt >= adss_num_scbs) {
			cmn_err(CE_PANIC, "%s: Out of command blocks", ha->ha_name);
		}
		cp = cp->c_next;
	}
	ha->ha_scb_next = cp->c_next;
	cp->c_active = TRUE;
	ADSS_SCB_UNLOCK(oip);
	ha->ha_poll = cp;
	return(cp);
}

/**************************************************************************
** Function name: adss_cmd()						 **
** Description:								 **
**	Create and send an SCB associated command, using a scatter	 **
**	gather list created by adss_sctgth(). 				 **
**************************************************************************/
STATIC void
adss_cmd(SGARRAY *sg_p, sblk_t *sp)
{
	struct scsi_ad		*sa;
	ADSS_SCB		*cp;
	struct scsi_lu		*q;
	int			i;
	struct req_sense_def		*reqs;
	char				*p;
	long				bcnt, cnt;
	int				c, target;

	AHA_DTRACE;

	if(sg_p != NULL)
		sp = sg_p->spcomms[0];

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adss_gtol[SDI_HAN(sa)];
	target = SDI_TCN(sa);

	cp = adss_getscb(c);

	/* fill in the controller command block */

	cp->chain = NULL;
	cp->linkedScb = NULL;
	cp->function = SCB_EXECUTE;
	cp->scbStatus = 0;
	cp->flags = 0;
	cp->targetStatus = 0;
	cp->scsiBus = 0;
	cp->targetID = (uchar_t)target;
	cp->lun = sa->sa_lun;
	cp->cdbLength = sp->sbp->sb.SCB.sc_cmdsz;
	cp->adapter = c;
	cp->c_bind = &sp->sbp->sb;
	cp->dataOffset = 0;
	cp->segmentAddress = 0;
	cp->segmentLength = 0;

	if((sp->sbp->sb.SCB.sc_mode & SCB_READ) == 1) {
		cp->flags |= SCB_DATA_IN;
	} else {
		cp->flags |= SCB_DATA_OUT;
	}

	reqs = (struct req_sense_def *)cp->senseData;
	if(reqs->sense_key)
		bzero((char *)cp->senseData, cp->senseDataLength);

	if(sg_p != NULL) {
		cp->flags |= SCB_VIRTUAL_SCATTER_GATHER;
		cp->dataPointer = NULL;
		cnt = sg_p->dlen;
		cp->c_sg_p = sg_p;
		cp->dataLength = (int)cnt;
	} else {					/* [3] */
		cp->flags &= ~SCB_VIRTUAL_SCATTER_GATHER;
		cp->dataPointer = (UCHAR *)(sp->s_addr);
		if(sp->sbp->sb.SCB.sc_datasz) {
			cp->dataLength = sp->sbp->sb.SCB.sc_datasz;
		} else {
			cp->dataLength = 0;
		}
	}

	p = sp->sbp->sb.SCB.sc_cmdpt;
	for(i = 0; i < (int)cp->cdbLength; i++)
		cp->scsi_cmd[i] = *p++;

	if(sg_p != NULL) {
		bcnt = sg_p->dlen >> 9;
		switch(cp->scsi_cmd[0]) {			/* [3] */
			case SS_READ:
				cp->scsi_cmd[4] = (uchar_t)bcnt;
				break;
			case SM_READ:
				cp->scsi_cmd[7] = (uchar_t)((bcnt >> 8) & 0xff);
				cp->scsi_cmd[8] = (uchar_t)(bcnt & 0xff);
				break;
			case SS_WRITE:
				cp->scsi_cmd[4] = (uchar_t)bcnt;
				break;
			case SM_WRITE:
				cp->scsi_cmd[7] = (uchar_t)((bcnt >> 8) & 0xff);
				cp->scsi_cmd[8] = (uchar_t)(bcnt & 0xff);
				break;
			default:				/* [3a] */
				cmn_err(CE_PANIC, "%s: Unknown SCSI command for s/g",
					adss_sc_ha[c]->ha_name);
				break;
		}
	}

	q = &LU_Q(c, target, sa->sa_lun);
	ADSS_SCSILU_LOCK(q);
	if ((cp->scsi_cmd[0] == SS_REQSEN) && (q->q_flag & QSENSE)) {
		bcopy((caddr_t)(&q->q_sense)+1, sp->s_addr, cp->dataLength);
		q->q_flag &= ~QSENSE;
		ADSS_SCSILU_UNLOCK(q);

		adss_done(SG_START, c, cp, SCB_COMPLETED_OK, cp->c_bind);
		FREEBLK(cp);
		adss_waitflag = FALSE;
	} else {
		ADSS_SCSILU_UNLOCK(q);
		adss_send(c, cp);   /* send cmd */
	}
}

/**************************************************************************
** Function: adss_sctgth()						 **
** Description:								 **
**	This routine is responsible for building local s/g commands.  It **
**	depends on the driver being able to queue commands for any given **
**	target, as well as the adapter being able to support s/g types   **
**	of commands.							 **
**************************************************************************/
STATIC void
adss_sctgth(struct scsi_lu *q, int c)
{
	SGARRAY	*sg_p;
	SG		*sg;
	sblk_t		*sp, *start_sp;
	char			unlocked, current_command, command;
	int			segments;
	int			length;
	long			start_block, current_block;
	long			total_count;
	caddr_t			p;

	AHA_DTRACE;
/*
 * This comment section is rather long.  I hope you don't mind how I like to
 * comment code.  If you do, tough.
 *
 * [1]	Right off the bat, see if we can get a s/g list.  If not, do the
 *	command in a normal manner.
 * [2]	Get the command to do.
 * [3]	Not real sure why this is done, but that is the way it was done in the
 *	original code.
 * [4]	Is the command a NULL?  If not, then doit, else return.
 * [5]	What type of command is it?
 * [6]	Just your normal SCSI command.
 * [7]	Must have gotten a s/g list.
 * [8]	Set the segment counter to 0.  segment is used to count the number of
 *	s/g segments we have done, as well as getting the pointer to the next
 *	s/g segment in the list.
 * [9]	While the command is not a NULL, try to build a s/g command.
 * [10]	Darn, the command is a function type of command.  Can't get a s/g
 *	list for that.
 * [11]	Get a pointer to the current segment in the s/g list of segments.
 * [12]	If segments == 0, then this is the first segment of the s/g list,
 *	so I need to setup the initial variables.  You may wonder why I don't
 *	do this outside of the while() loop.  The next step in the evolution
 *	of the code is to make it recursively look through the queued command
 *	list for more s/g stuff.
 * [13]	Have to get the SCSI command for a later compare.
 * [14]	Now we have to get the physical block address of the command.  I hope
 *	that number will be stored by sd01 in some element in the future
 *	FIXIT?
 * [15]	Got to get the total count also.  13, 14, and 15 become clear in a
 *	second.
 * [16]	Got to save the pointer to this command in two places.  One in
 *	start_sp, in case we don't get a hit and in spcomms, in case we do get
 *	a hit.
 * [17]	Shove the count and data address for this command into the first s/g
 *	segment.
 * [18]	Increment the segment variable so we can get the next s/g segment.
 * [19]	Set the length to the length of a s/g segment.
 * [20]	Point to the next command in the queue.
 * [21]	Start it again, at ([9]).
 * [22]	Okay, we have setup the first s/g segment, now the fun begins.  Get
 *	the block number and command for this command.
 * [23]	Now let's see if we have a potential s/g command.  If the command
 *	is the same as the first command, the block number + the total
 *	count = the first command block number, and the number of segments
 *	used does not exceed the s/g list >>>>we have a s/g hit!
 * [24]	Increment the total count of data by what is set in the current
 *	command.
 * [25]	Increment the length of the s/g list.
 *	NOTE: The stuff between 25 and 26 should be self explanatory.
 * [26] Darn, we did not have a s/g hit, or have hit the end of the s/g list.
 * [27]	We only get here if we have s/g hits and still have segments to use.
 *	So we need to make sure we are pointing to the next command.
 *
 * There are three conditions that will get us to this point (28).  1) We
 * have a s/g command to do.  2) There are no more commands in the queue to
 * look at.  3) We do not have a sequential access, so no s/g to do.
 *
 * [28]	Darn, no s/g hit, we'll have to do the command as a normal I/O.
 * [29]	Is it a real command?  Or am I just being paranoid?
 * [30] Oh BOY!! A s/g command to do.
 * [31]	Cleanup time.  There will always be at least one more command to do
 *	at this point unless, we are leaving due to running out of commands
 *	in the queue, so this check is not a paranoid check.
 * [32]	Make sure we are pointing to the next command before leaving, or you
 *	will be sorry.
 */


	if((sg_p = adss_get_sglist(c)) == NULL) {		/* [1] */
		sp = q->q_first;				/* [2] */
		if(!(q->q_first = sp->s_next))			/* [3] */
			q->q_last = NULL;			/*  ^  */
		else
			q->q_first->s_prev = NULL;
		if(sp != NULL) {				/* [4] */
			if (sp->sbp->sb.sb_type == SFB_TYPE) {	/* [5] */
				q->q_depth--;
				ADSS_SCSILU_UNLOCK(q);
				adss_func(sp);
			} else {				/* [6] */
				q->q_depth--;
				ADSS_SCSILU_UNLOCK(q);
				adss_cmd(NULL, sp);
			}
		} else
			ADSS_SCSILU_UNLOCK(q);
		return;
	}							/* [7] */
	segments = 0;						/* [8] */
	start_sp = NULL;
	while((sp = q->q_first) != NULL) {			/* [9] */
		if (!(q->q_first = sp->s_next))		/* [20]*/
			q->q_last = NULL;
		else
			q->q_first->s_prev = NULL;
		if(sp->sbp->sb.sb_type == SFB_TYPE) {		/* [10]*/
			q->q_depth--;
			ADSS_SCSILU_UNLOCK(q);
			adss_func(sp);
			if(segments == 0) {
				sg_p->sg_flags = SG_FREE;
				return;
			}
			ADSS_SCSILU_LOCK(q);
			continue;
		}
 		p = sp->sbp->sb.SCB.sc_cmdpt;			/* [13]*/
		if(sp->sbp->sb.SCB.sc_datasz > 65535 ||
		   (p[0] != SS_READ && p[0] != SM_READ &&
		   p[0] != SS_WRITE && p[0] != SM_WRITE)) {
			break;
		}
		sg = &sg_p->sg_seg[segments];			/* [11]*/
		if(segments == 0) {				/* [12]*/
			command = p[0];
/*
 * This is how we get the disk block number from the stuff above me.
 */
			if(command == SM_READ || command == SM_WRITE)
				start_block = adss_tol(&p[2]);
			else {
				start_block = (p[1] & 0x1f);
				start_block <<= 8;
				start_block |= p[2];
				start_block <<= 8;
				start_block |= p[3];
			}
			/* [15] */
			total_count = sp->sbp->sb.SCB.sc_datasz;
			start_sp = sp;				/* [16]*/
			sg_p->spcomms[segments] = sp;
			sg->sg_size = total_count;
			sg->sg_addr = sp->s_addr;
			sg_p->next_length = total_count;
			sg_p->next_addr = sg->sg_addr;
			segments++;				/* [18] */
			length = SG_LENGTH;			/* [19] */
			continue;				/* [21]*/
		}
		current_command = p[0];				/*  ^  */
		if(current_command == SM_READ || current_command == SM_WRITE) {
			current_block = adss_tol(&p[2]);
		} else {
			current_block = (p[1] & 0x1f);
			current_block <<= 8;
			current_block |= p[2];
			current_block <<= 8;
			current_block |= p[3];
		}
								/* [23] */
		if(((start_block + (total_count >> 9)) == current_block) &&
		  command == current_command &&
		  segments < SG_SEGMENTS) {
			total_count += sp->sbp->sb.SCB.sc_datasz;   /* [24]*/
			length += SG_LENGTH;			/* [25] */
			sg_p->spcomms[segments] = sp;
			sg->sg_size = sp->sbp->sb.SCB.sc_datasz;
			sg->sg_addr = sp->s_addr;
			segments++;
		} else {
			break;					/* [26] */
		}
	}
	unlocked = 0;
	if(segments <= 1) {					/* [28] */
		sg_p->sg_flags = SG_FREE;
		if(start_sp != NULL)  {				/* [29] */
			q->q_depth--;
			ADSS_SCSILU_UNLOCK(q);
			unlocked = 1;
			adss_cmd(NULL, start_sp);
		}
	} else {						/* [30] */
		sg_p->sg_len = length / SG_LENGTH;
		sg_p->dlen = total_count;
		q->q_depth -= segments;
		ADSS_SCSILU_UNLOCK(q);
		unlocked = 1;
		adss_cmd(sg_p, ((sblk_t *)0));
	}
	if(sp != NULL) {					/* [31] */
		if ( unlocked )
			ADSS_SCSILU_LOCK(q);
		q->q_depth--;
		ADSS_SCSILU_UNLOCK(q);
		adss_cmd(NULL, sp);
	}
}

/**************************************************************************
** Function: adss_get_sglist()						 **
** Description:								 **
**	Here is the routine that returns a pointer to the next free	 **
**	s/g list.  Each list is 17 segments long.  I have only allocated **
**	adss_num_scb of these structures, instead of doing it on a per	 **
**	adapter basis.  Doesn't hurt if it returns a NULL as it just	 **
**	means the calling routine will just have to do it the		 **
**	old-fashion way.  This routine runs just like adss_getscb, which **
**	is commented very well.  So see those comments if you need to.	 **
**************************************************************************/
STATIC SGARRAY *
adss_get_sglist(int c)
{
	HACB	*ha = adss_sc_ha[c];
	SGARRAY	*sg;
	int		cnt;
	pl_t		oip;

	AHA_DTRACE;

	ADSS_SGARRAY_LOCK(oip);
	sg = ha->ha_sgnext;
	cnt = 0;

	while(sg->sg_flags == SG_BUSY) {
		cnt++;
		if(cnt >= adss_num_scbs) {
			ADSS_SGARRAY_UNLOCK(oip);
			return(NULL);
		}
		sg = sg->sg_next;
	}
	sg->sg_flags = SG_BUSY;
	ha->ha_sgnext = sg->sg_next;
	ADSS_SGARRAY_UNLOCK(oip);
	sg->next_seg = 0;
	sg->next_length = 0;
	sg->next_offset = 0;
	return(sg);
}

/**************************************************************************
** Function Name: adss_wait()						 **
** Description:								 **
**	Poll for a completion from the host adapter.  If an interrupt	 **
**	is seen, the HA's interrupt service routine is manually called.  **
**  NOTE:								 **
**	This routine allows for no concurrency and as such, should 	 **
**	be used selectively.						 **
**************************************************************************/
STATIC int
adss_wait(int c, int time)
{
	HACB		*ha = adss_sc_ha[c];
	ADSS_SCB		*cp, *cp1;
	int			time1;

	AHA_DTRACE;

	cp = ha->ha_poll;
	time1 = time;
	while(time > 0) {
		if (adss_waitflag == FALSE) {
			adss_waitflag = TRUE;
			return(LOC_SUCCESS);
		}
		drv_usecwait(1000);  /* wait 1 msec */
		time--;
	}

	cp1 = adss_getscb(c);
	cp1->function = SCB_ABORT_REQUESTED;
	cp1->adapter = c;
	cp1->targetID = cp->targetID;
	cp1->lun = cp->lun;
	if (HIM6X60AbortSCB(ha, cp1, cp) == SCB_PENDING) {
		time = time1;
		while(time > 0) {
			if (adss_waitflag == FALSE) {
				adss_waitflag = TRUE;
				FREEBLK(cp1);
 				return(LOC_FAILURE);	
			}
			drv_usecwait(1000);  /* wait 1 msec */
			time--;
		}

		HIM6X60ResetBus(ha, c);
		time = time1;
		while(time > 0) {
			if (adss_waitflag == FALSE) {
				adss_waitflag = TRUE;
				FREEBLK(cp1);
 				return(LOC_FAILURE);	
			}
			drv_usecwait(1000);  /* wait 1 msec */
			time--;
		}
		FREEBLK(cp);
	}
	adss_waitflag = TRUE;
	FREEBLK(cp1);
 	return(LOC_FAILURE);	
}

/*
 * Function name: adssfreeblk()
 * Description:	
 *	Release previously allocated SB structure.
 *	A nonzero return indicates an error in pointer or type field.
 */
long
HBAFREEBLK(struct hbadata *hbap)
{
	sblk_t *sp = (sblk_t *) hbap;

	AHA_DTRACE;

	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return (SDI_RET_OK);
}

/*
 * Function name: adssgetblk()
 * Description:
 *	Allocate a SB structure for the caller.  The function will
 *	sleep if there are no SCSI blocks available.
 */
struct hbadata *
HBAGETBLK(int sleepflag)
{
	sblk_t	*sp;

	AHA_DTRACE;

	sp = (sblk_t *)SDI_GET(&sm_poolhead, sleepflag);
	return((struct hbadata *)sp);
}

/**************************************************************************
** Function name: adssgetinfo()						 **
** Description:								 **
**	Return the name, etc. of the given device.  The name is copied	 **
**	into a string pointed to by the first field of the getinfo	 **
**	structure.							 **
**************************************************************************/
void
HBAGETINFO(struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	char  *s1, *s2;
	static char temp[] = "HA X TC X";

	AHA_DTRACE;

	s1 = temp;
	s2 = getinfop->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;

	getinfop->iotype = F_PIO | F_SCGTH;

	if (getinfop->bcbp) {
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = adsshba_info.max_xfer;
		getinfop->bcbp->bcb_physreqp->phys_align = ADSS_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = ADSS_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = ADSS_DMASIZE;
	}
}

/**************************************************************************
** Function name: adssicmd()						 **
** Description:								 **
**	Send an immediate command.  If the logical unit is busy, the job **
**	will be queued until the unit is free.  SFB operations will take **
**	priority over SCB operations.					 **
**************************************************************************/

/*ARGSUSED*/
long
HBAICMD(struct  hbadata *hbap, int sleepflag)
{
	struct scsi_ad *sa;
	struct scsi_lu *q;
	sblk_t *sp = (sblk_t *) hbap;
	int	c, t, l;
	struct scs	*inq_cdb;
	struct sdi_edt	*edtp;

	AHA_DTRACE;

	switch (sp->sbp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = adss_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, l);

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {
		case SFB_RESUME:
			ADSS_SCSILU_LOCK(q);
			q->q_flag &= ~QSUSP;
			adss_next(q);
			break;
		case SFB_SUSPEND:
			ADSS_SET_QFLAG(q,QSUSP);
			break;
		case SFB_RESETM:
			edtp = sdi_redt(SDI_HAN(sa),t,l);
			if(edtp != NULL && edtp->pdtype == ID_TAPE) {
				/*  this is a NOP for tape devices */
				sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;
				break;
			}
			/*FALLTHRU*/
		case SFB_ABORTM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			ADSS_SCSILU_LOCK(q);
			adss_putq(q, sp);
			adss_next(q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			adss_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code = (ulong_t)SDI_SFBERR;
		}

		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = adss_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);
		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;

		if((t == adss_sc_ha[c]->ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, adssidata[c].name, 
				VID_LEN+PID_LEN+REV_LEN);
			inq.id_type = ID_PROCESOR;

			inq_data = (struct ident *)(void *)sp->sbp->sb.SCB.sc_datapt;
			inq_len = sp->sbp->sb.SCB.sc_datasz;
			bcopy((char *)&inq, (char *)inq_data, inq_len);

			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		ADSS_SCSILU_LOCK(q);
		adss_putq(q, sp);
		adss_next(q);
		return (SDI_RET_OK);

	default:
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_ERR);
	}
}

/**************************************************************************
** Function name: adssioctl()						 **
** Description:								 **
**	Driver ioctl() entry point.  Used to implement the following 	 **
**	special functions:						 **
**									 **
**	SDI_SEND     -	Send a user supplied SCSI Control Block to	 **
**			the specified device.				 **
**	B_GETTYPE    -  Get bus type and driver name			 **
**	B_HA_CNT     -	Get number of HA boards configured		 **
**	REDT	     -	Read the extended EDT from RAM memory		 **
**	SDI_BRESET   -	Reset the specified SCSI bus 			 **
**									 **
**************************************************************************/

/*ARGSUSED*/
int
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	int	c = adss_gtol[SC_HAN(dev)];
	int	t = SC_TCN(dev);
	int	l = SC_LUN(dev);
	struct sb *sp;
	int  uerror = 0;

	AHA_DTRACE;

	switch(cmd) {
	case SDI_SEND: {
		buf_t *bp;
		struct sb  karg;
		int  rw;
		char *save_priv;
		struct scsi_lu *q;

		if (t == adss_sc_ha[c]->ha_id) { 	/* illegal ID */
			return(ENXIO);
		}
		if (copyin(arg, (caddr_t)&karg, sizeof(struct sb))) {
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

		q = &LU_Q(c, t, l);
		if (q->q_sc_cmd == NULL) {
			/*
			 * Allocate space for the SCSI command and add 
			 * sizeof(int) to account for the scm structure
			 * padding, since this pointer may be adjusted -2 bytes
			 * and cast to struct scm.  After the adjustment we
			 * still want to be pointing within our allocated space!
			 */
			q->q_sc_cmd = (char *)KMEM_ZALLOC(MAX_CMDSZ + 
					sizeof(int), KM_SLEEP) + sizeof(int);
		}
		sp->SCB.sc_cmdpt = q->q_sc_cmd;

		if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
		    sp->SCB.sc_cmdsz)) {
			uerror = EFAULT;
			goto done;
		}
		sp->SCB.sc_dev.sa_lun = (uchar_t)l;
		sp->SCB.sc_dev.sa_fill = (adss_ltog[c] << 3) | t;

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
		bp->b_private = (char *)sp;

		/*
		 * If the job involves a data transfer then the
		 * request is done thru physiock() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then adss_pass_thru()
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
			op = q->q_sc_cmd[0];
			/*
			 * Save the starting block number, if r/w.
			 */
			if (op == SS_READ || op == SS_WRITE) {
				struct scs *scs = 
				   (struct scs *)(void *)q->q_sc_cmd;
				bp->b_priv2.un_int =
				   ((sdi_swap16(scs->ss_addr) & 0xffff)
				   + (scs->ss_addr1 << 16));
			}
			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm = 
				(struct scm *)(void *)((char *)q->q_sc_cmd - 2);
				bp->b_priv2.un_int =
				   sdi_swap32(scm->sm_addr);
			}

			if (uerror = physiock(adss_pass_thru0, bp, dev, rw, 
			    PHYSIOCK_MAGIC, &ha_uio)) {
				goto done;
			}
		} else {
			bp->b_un.b_addr = NULL;
			bp->b_bcount = 0;
			bp->b_blkno = 0;
			bp->b_edev = dev;
			bp->b_flags |= rw;

			adss_pass_thru(bp);  /* Bypass physiock call */
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
		if(copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}
		if(copyout("scsi", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if (copyout((caddr_t)&adss_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET: {
		HACB *ha = adss_sc_ha[c];

		ADSS_HBA_LOCK(ha);
		if (ha->ha_npend > 0) {     /* jobs are outstanding */
			ADSS_HBA_UNLOCK(ha);
			return(EBUSY);
		} else {
			cmn_err(CE_WARN, "%s: - Bus is being reset", ha->ha_name);
			(void)HIM6X60ResetBus(ha, 0);
		}
		ADSS_HBA_UNLOCK(ha);
		break;
		}

	default:
		return(EINVAL);
	}
	return(uerror);
}

/**************************************************************************
** Function name: adsssend()						 **
** Description:								 **
** 	Send a SCSI command to a controller.  Commands sent via this	 **
**	function are executed in the order they are received.		 **
**************************************************************************/

/*ARGSUSED*/
long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	struct scsi_ad *sa;
	struct scsi_lu *q;
	sblk_t *sp = (sblk_t *) hbap;
	int	c, t, l;

	AHA_DTRACE;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adss_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	q = &LU_Q(c, t, l);

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	ADSS_SCSILU_LOCK(q);

	adss_putq(q, sp);

	if (!(q->q_flag & QPTHRU))
		adss_next(q);
	else
		ADSS_SCSILU_UNLOCK(q);

	return (SDI_RET_OK);
}

STATIC int
adss_alloc_sg(HACB *ha, int cnt)
{
	int		i;
	struct sg_array *sg_p, *sg_p_tmp;
	int			sg_listsize;
	int			sleepflag;
	SGARRAY			*adss_sg_next;
	SGARRAY			*adss_scatter;

	AHA_DTRACE;

	sleepflag = adss_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

/****************************************************************************
 * Now add the initialization for the scatter/gather lists and the pointers *
 * for the sblk_t sp arrays to keep track of.  I have opted for circular    *
 * queue (linked lists) of each of these lists.  In the routine that returns*
 * a free list I will keep a forward moving global that will keep pointing  *
 * to the next array.  This will almost always insure the free pointer will *
 * be found in the first check, keeping overhead to a minimum. (i.e. atomic)*
 ****************************************************************************/

	sg_listsize = sizeof(SGARRAY);
#ifdef AHA_DEBUG_
	if(sg_listsize > PAGESIZE)
		cmn_err(CE_WARN,
			 "%s: scatter array exceeds pagesize (may cross physical page boundary)",
			 ha->ha_name);
#endif

	adss_sg_next = NULL;
	for(i = 0; i < cnt; i++) {
		if((adss_scatter =
		  (SGARRAY *)KMEM_ZALLOC(sg_listsize, sleepflag)) == NULL) {
			cmn_err(CE_WARN, "%s: Cannot allocate s/g array",
				ha->ha_name);
			return(-1);
		}
		if(i == 0)
			sg_p_tmp = adss_scatter;
		sg_p = adss_scatter;
		sg_p->next_addr = NULL;
		sg_p->sg_flags = SG_FREE;
		sg_p->sg_next = adss_sg_next;
		adss_sg_next = adss_scatter;
	}
	sg_p_tmp->sg_next = adss_scatter;
	sg_p_tmp->sg_flags = SG_FREE;
	sg_p_tmp->next_addr = NULL;
	ha->ha_sglist = adss_scatter;
	ha->ha_sgnext = adss_scatter;
	return(0);
}

/**************************************************************************
** Function name: adss_free_sg()					 **
** Description:								 **
**	Free previously allocated s/g lists				 **
**************************************************************************/
STATIC void
adss_free_sg(HACB *ha, int cnt)
{
	int		i;
	SGARRAY	*sg_p, *adss_sg_next;
	int			sg_listsize;

	AHA_DTRACE;

	sg_listsize = sizeof(SGARRAY);

	sg_p = ha->ha_sglist;
	for(i = 0; i < cnt; i++) {
		if(sg_p != NULL) {
			adss_sg_next = sg_p->sg_next;
			KMEM_FREE(sg_p, sg_listsize);
			sg_p = adss_sg_next;
		}
	}
}

/*ARGSUSED*/
VOID *
HIM6X60GetVirtualAddress(HACB *hacb, ADSS_SCB *scb, VOID *addr, ULONG offset, ULONG *length)
{
	int			i, len;
	SG			*sg;
	SGARRAY				*sg_p;
	char				*data_p;
	long				l_length;

	AHA_DTRACE;
	ASSERT(scb->c_sg_p);

	l_length = 0;
	sg_p = scb->c_sg_p;
	len = sg_p->sg_len;
	if(offset == sg_p->next_offset) {
		*length = sg_p->next_length;
		data_p = sg_p->next_addr;
		sg_p->next_seg++;
		sg = &sg_p->sg_seg[sg_p->next_seg];
		sg_p->next_offset += sg->sg_size;
		sg_p->next_addr = sg->sg_addr;
		sg_p->next_length = sg->sg_size;
		return(data_p);
	}
	for(i = 0; i < len; i++) {
		sg = &sg_p->sg_seg[i];
		data_p = sg->sg_addr;
		if(l_length == offset) {
			*length = sg->sg_size;
			break;
		}
		if(l_length > offset) {
			data_p += (sg->sg_size - (l_length - offset));
			*length = l_length - offset;
			break;
		}
		l_length += sg->sg_size;
	}
	return(data_p);
}

void
adss_set(char *dest, int value, int size)
{
	int i;

	for(i = 0; i < size; i++)
		dest[i] = (char)value;
}

void
HIM6X60Event(HACB *hacb, UCHAR event)
{
	AHA_DTRACE;

	switch(event) {
		case EVENT_SCSI_BUS_RESET:
			cmn_err(CE_WARN, "!%s: SCSI Bus Reset", hacb->ha_name);
#ifdef AHA_DEBUG1
			for(;;) {
			}
#endif
			break;
		default:
			cmn_err(CE_WARN, "%s: Unknown Event", hacb->ha_name);
			break;
	}
}

/*ARGSUSED*/
LUCB *
HIM6X60GetLUCB(HACB *hacb, UCHAR adapter, UCHAR target, UCHAR lun)
{
	int	i;
	LUCB	*lucb;

	AHA_DTRACE;

	i = target;
	i <<= 3;
	i |= lun;
	lucb = &hacb->ha_lucb[i];
	return(lucb);
}

void
adss_mem_release(HACB **hap, int mem_release)
{
	HACB *hacb = *hap;
	struct req_sense_def *req_sense = (struct req_sense_def *)(hacb->ha_scb[0].senseData);

	if(mem_release & HA_LUCB_REL) {
		KMEM_FREE(hacb->ha_lucb, adss_lucb_structsize);
		hacb->ha_lucb = NULL;
	}
	if(mem_release & HA_DEV_REL) {
		KMEM_FREE(hacb->ha_dev, adss_luq_structsize);
		hacb->ha_dev = NULL;
	}
	if(mem_release & HA_SG_REL) {
		adss_free_sg(hacb, adss_num_scbs);
	}
	if(mem_release & HA_SCB_REL) {
		KMEM_FREE(hacb->ha_scb, adss_scb_structsize);
		hacb->ha_scb = NULL;
	}
	if(mem_release & HA_HACB_REL) {
		KMEM_FREE(hacb, adss_hacb_structsize);
		hacb = NULL;
	}
	if(mem_release & HA_REQS_REL) {
		KMEM_FREE(req_sense, adss_req_structsize);
	}
}

/*
 * STATIC void
 * adss_lockinit(int c)
 *	Initialize adss locks:
 *		1) device queue locks (one per queue),
 *		2) scb lock (one per controller),
 *		3) scatter/gather lists lock (one per controller),
 *		4) and controller lock (one per controller).
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adss_lockinit(int c)
{
	HACB  *ha;
	struct scsi_lu	 *q;
	int	t,l;
	int		sleepflag;

	AHA_DTRACE;

	sleepflag = adss_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	ha = adss_sc_ha[c];
	ha->ha_hba_lock = LOCK_ALLOC(ADSS_HIER, pldisk, &adss_lkinfo_hba, sleepflag);
	ha->ha_sg_lock = LOCK_ALLOC(ADSS_HIER+1, pldisk, &adss_lkinfo_sgarray, sleepflag);
	ha->ha_scb_lock = LOCK_ALLOC(ADSS_HIER, pldisk, &adss_lkinfo_scb, sleepflag);

	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			q = &LU_Q(c,t,l);
			q->q_lock = LOCK_ALLOC(ADSS_HIER, pldisk, &adss_lkinfo_q, sleepflag);
		}
	}
}

/*
 * STATIC void
 * adss_lockclean(int c)
 *	Removes unneeded locks.  Controllers that are not active will
 *	have all locks removed.  Active controllers will have locks for
 *	all non-existant devices removed.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adss_lockclean(int c)
{
	HACB  *ha = adss_sc_ha[c];
	struct scsi_lu	 *q;
	int	t,l;

	AHA_DTRACE;

	if (!ha || !ha->ha_hba_lock)
		return;

	if (!adssidata[c].active) {
		LOCK_DEALLOC(ha->ha_sg_lock);
		LOCK_DEALLOC(ha->ha_scb_lock);
		LOCK_DEALLOC(ha->ha_hba_lock);
	}

	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			if (!adssidata[c].active || adss_illegal(adss_ltog[c], t, l)) {
				q = &LU_Q(c,t,l);
				LOCK_DEALLOC(q->q_lock);
			}
		}
	}
}

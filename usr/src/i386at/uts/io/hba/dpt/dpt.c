#ident	"@(#)kern-pdi:io/hba/dpt/dpt.c	1.66.16.2"
#ident	"$Header$"

/*      Copyright (c) 1988, 1989, 1990, 1991  Intel Corporation            */
/* All Rights Reserved                                                     */

/*      INTEL CORPORATION PROPRIETARY INFORMATION                          */
/*                                                                         */
/* This software is supplied under the terms of a license agreement        */
/* or nondisclosure agreement with Intel Corporation and may not be        */
/* copied or disclosed except in accordance with the terms of that         */
/* agreement.                                                              */
/*                                                                         */
/*      Copyright (c) 1990 Distributed Processing Technology               */
/*      All Rights Reserved                                                */

#ifdef _KERNEL_HEADERS

#include <svc/errno.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>	
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_hier.h>

#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>

#include <io/hba/dpt/dpt.h>

#include <io/conf.h>
#include <io/dma.h>
#include <io/ddi.h>

#else /* _KERNEL_HEADERS */

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/sdi.h>
#include <sys/sdi_hier.h>

#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>

#include "dpt.h"

#include <sys/conf.h>
#include <sys/dma.h>
#include <sys/ddi.h>

#endif /* _KERNEL_HEADERS */

/*
 * Function Prototypes
 */

STATIC scsi_ha_t *dpt_init(HBA_IDATA_STRUCT *);
STATIC int	dpt_reinit(scsi_ha_t *ha);
STATIC int	dptintr(scsi_ha_t *);
STATIC int	dpt_verify(rm_key_t rmkey);
STATIC void	dpt_suspend(scsi_ha_t *ha);
STATIC void	dpt_resume(scsi_ha_t *ha);
STATIC void	dpt_halt(scsi_ha_t *);
STATIC void 	dpt_PauseBus(scsi_ha_t *, int b);
STATIC void 	dpt_ResumeBus(scsi_ha_t *, int b);
STATIC void	dpt_stop(scsi_ha_t *ha);

STATIC ccb_t *	dpt_getccb(scsi_ha_t *ha);
STATIC void	dpt_freeccb(scsi_ha_t *ha, ccb_t *cp);

STATIC void 	dpt_hafree(scsi_ha_t *ha);
STATIC int	dpt_ha_init(scsi_ha_t *ha);
STATIC int	dpt_physreq_init(scsi_ha_t *ha, int sleepflag);
STATIC void 	dpt_pause_func(dpt_pause_func_arg_t *);
STATIC void	dpt_cache_detect(scsi_ha_t *ha, char *p, paddr_t pa, int sz);
STATIC void	dpt_mode_sense(scsi_ha_t *ha, char *p, paddr_t pa, int sz);
STATIC void	dpt_Inquiry(scsi_ha_t *ha, char *p, paddr_t pa, int sz);
STATIC void 	dpt_pass_thru(struct buf *bp);
STATIC void 	dpt_pass_thru_init(scsi_ha_t *ha, struct scsi_ad *sa);
STATIC void 	dpt_putq(register scsi_lu_t *q, register sblk_t *sp);
STATIC void 	dpt_lockinit(scsi_ha_t *ha);
STATIC void 	dpt_lockclean(scsi_ha_t *ha);
STATIC scsi_lu_t * dpt_qalloc(void);
STATIC void 	dpt_qfree(scsi_lu_t *q);

STATIC void	dpt_done(scsi_ha_t *ha, ccb_t *cp, int status, int resid);
STATIC void	dpt_int(struct sb *sp);
STATIC void	dpt_flushq(register scsi_lu_t *q, int cc, int flag);
STATIC void	dpt_waitq(scsi_ha_t *ha, scsi_lu_t *q);
STATIC void	dpt_next(scsi_ha_t *ha, register scsi_lu_t *q);
STATIC void	dpt_cmd(scsi_ha_t *ha, sblk_t *sp, register ccb_t *cp);
STATIC void	dpt_func(scsi_ha_t *ha, sblk_t *sp, register ccb_t *cp);
STATIC void	dptintr_blocking (scsi_ha_t *ha, ccb_t *cp);
STATIC void	dptintr_done (scsi_ha_t *ha, ccb_t *cp);
STATIC int	_dpt_send(scsi_ha_t *ha, ccb_t *cp, ccb_callback callback);

STATIC int	dpt_init_cps(scsi_ha_t *ha);
STATIC void	dpt_board_reset(int);
STATIC int	dpt_wait(scsi_ha_t *ha, int time, int intr);
STATIC int	EATA_ReadConfig(int port, struct RdConfig *eata_cfg);
STATIC int	dpt_illegal(int hba, int bus, int scsi_id, int lun, int m);
STATIC void 	dpt_SendEATACmd(scsi_ha_t *ha, int bus, int cmd);
STATIC int	dptr_BlinkLED(register int port);
STATIC void	dpt_send_cmd(scsi_ha_t *ha, paddr_t addr, int cmd);

#ifdef DPT_TARGET_MODE
STATIC void 	dpt_targSet(scsi_ha_t *ha, int, ccb_t *cp);
STATIC void	dptintr_target (scsi_ha_t *ha, ccb_t *cp);
#endif /* DPT_TARGET_MODE */

#ifdef DPT_TIMEOUT_RESET_SUPPORT
STATIC sblk_t * dpt_timeout_remove(scsi_ha_t *, struct sb *);
STATIC void 	dpt_timeout_insert(scsi_ha_t *, sblk_t *, ccb_t *);
STATIC void	dpt_abort_cmd(scsi_ha_t *ha, paddr_t addr);
STATIC void	dpt_reset_device(scsi_ha_t *ha, int b, int t, int l);
STATIC void	dpt_reset_bus(scsi_ha_t *ha, int bus);
STATIC void	dpt_watchdog( scsi_ha_t *ha );
#ifdef DPT_TIMEOUT_RESET_DEBUG
int	dpt_force_time = 0;		/* For debugging: force a timeout */
int	dpt_trace_list = 0;
void	dpt_active_jobs(scsi_ha_t *ha);
#endif /* DPT_TIMEOUT_RESET_DEBUG */
#endif /* DPT_TIMEOUT_RESET_SUPPORT */

/*
 * dpt multi-processor locking variables
 */
STATIC LKINFO_DECL(dpt_lkinfo_eata, "IO:dpt:ha->ha_eata_lock", 0);
STATIC LKINFO_DECL(dpt_lkinfo_ccb, "IO:dpt:ha->ha_ccb_lock", 0);
STATIC LKINFO_DECL(dpt_lkinfo_StPkt, "IO:dpt:ha->ha_StPkt_lock", 0);
STATIC LKINFO_DECL(dpt_lkinfo_q, "IO:dpt:q->q_lock", 0);
STATIC LKINFO_DECL(dpt_lkinfo_QWait, "IO:dpt:ha->ha_QWait_lock", 0);

#ifdef DPT_TIMEOUT_RESET_SUPPORT
STATIC LKINFO_DECL(dpt_lkinfo_watchdog, "IO:dpt:ha->ha_watchdog_lock", 0);
#endif /** DPT_TIMEOUT_RESET_SUPPORT **/

#ifdef DPT_TIMEOUT_RESET_SUPPORT
STATIC int dpt_devflag = HBA_MP | HBA_HOT | HBA_TIMEOUT_RESET | HBA_CALLBACK;
#else
STATIC int dpt_devflag = HBA_MP | HBA_HOT | HBA_CALLBACK;
#endif /* DPT_TIMEOUT_RESET_SUPPORT */

HBA_INFO(dpt, &dpt_devflag, 0x20000);

/* variables allocated in space.c */
extern char dpt_modname[];		/* initialized in space.c */
extern HBA_IDATA_STRUCT _dptidata[]; 	/* initialized in space.c */
extern scsi_ha_t *dpt_sc_ha[];		/* SCSI HA structures */
extern struct head sm_poolhead; 	/* pool for dynamic struct allocation */
					/* for sblk_t */
extern	int dpt_max_jobs;		/* Maximum number of jobs allowed on */
					/* the HBA at one time */
extern int dpt_targetmode;		/* Is targetmode enabled */
extern int dpt_reserve_release;		/* Flag to disable reserve/release */

STATIC dpt_sig_t dpt_sig = {
	'd', 'P', 't', 'S', 'i', 'G', SIG_VERSION, PROC_INTEL,
	PROC_386 | PROC_486 | PROC_PENTIUM | PROC_P6, FT_HBADRVR, 0,
	OEM_DPT, OS_UNIXWARE,
#if (defined(DPT_RAID_0))
		CAP_RAID0 | CAP_PASS | CAP_OVERLAP,
#else
		CAP_PASS | CAP_OVERLAP,
#endif /* DPT_RAID_0 */
	DEV_ALL, ADF_ALL_MASTER, 0, 0, DPT_VERSION, DPT_REVISION,
	DPT_SUBREVISION, DPT_MONTH, DPT_DAY, DPT_YEAR,
	"DPT UnixWare Driver"
};

STATIC int	dpt_config(cfg_func_t, void *, rm_key_t);
	
STATIC drvops_t	dpt_drvops = { dpt_config };

/* TODO - set the flags for each CFG_* instances supported */
STATIC drvinfo_t	dpt_drvinfo = {
	&dpt_drvops, dpt_modname, (D_MP | D_HOT), NULL, 0
};

/* TODO - need pragma here */
/* asm ulong
 * dpt_swap32(ulong)
 *
 * Description: Fast byte-order swap
 *
 * Calling/Exit State:
 * None
 */
asm ulong
dpt_swap32(ulong i)
{
%mem i;

	movl i,%eax
	bswap %eax
}

/*
 * int
 * _load(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the module is loaded.
 *
 * Note: ...
 *
 */

int
_load()
{
	return (drv_attach(&dpt_drvinfo));
}

/*
 * int
 * _unload(void)
 *
 * Calling/Exit State:
 *    ...
 *
 * Description:
 *    Called by DLM when the module is unloading.
 *
 * Note: ...
 *
 */

int
_unload()
{
	int i;

	for (i = 0; i < SDI_MAX_HBAS; i++)
		if (dpt_sc_ha[i] != NULL) {
			cmn_err(CE_WARN, "%s: instance is busy",
						dpt_sc_ha[i]->ha_name);
			return (EBUSY);
		}

	drv_detach(&dpt_drvinfo);

	return (0);
}

/*
 * int
 * dpt_config(cfg_func_t func, void *idata, rm_key_t key)
 *
 * Description:
 *
 * Calling/Exit State:
 *	none.
 */

int
dpt_config(cfg_func_t func, void *idata, rm_key_t rmkey)
{
	scsi_ha_t *ha;
	HBA_IDATA_STRUCT *idp;
	int	b;

	switch(func) {

	case CFG_ADD:  {

		void **idatap = idata;

		idp = sdi_idata_alloc(rmkey, _dptidata);
		if (idp == NULL) {
			return (ENODEV);
		}

		if ((ha = dpt_init(idp)) == NULL) {
			sdi_idata_free(idp);
			return (ENODEV);	
		}

		*idatap = ha;

		sdi_enable_instance(idp);

		} /* case CFG_ADD */
		break;

	case CFG_SUSPEND:

		ha = idata;
		idp = ha->ha_idata;
		
		dpt_suspend(ha);

		/*
		 * Detach interrupts for this device. 
		 * Also make sure that cookie is set to NULL to avoid
		 *  multiple cm_intr_detaches which could cause panics.
		 */
		cm_intr_detach(idp->idata_intrcookie);
		idp->idata_intrcookie = NULL;

		break;

	case CFG_RESUME:

		ha = idata;

		/* re-initialize */
		if (dpt_reinit(ha)) {
			return (ENXIO);	
		}

		dpt_resume(ha);

		break;

	case CFG_REMOVE:

		ha = idata;
		idp = ha->ha_idata;

		/* fail all the jobs */
		dpt_stop(ha);

		sdi_deregister(idp);
		sdi_idata_free(idp);

		dpt_hafree(ha);

		break;

	case CFG_MODIFY:

		ha = idata;
		idp = ha->ha_idata;

		sdi_idata_modify(idp, rmkey);

		break;

	case CFG_VERIFY:
		
		if (dpt_verify(rmkey))
			return ENODEV;
		break;

	default:
		return EOPNOTSUPP;
	}
	return 0;
}

STATIC scsi_ha_t *
dpt_init(HBA_IDATA_STRUCT *idp)
{
	scsi_ha_t	*ha;
	cm_args_t	cma;
	int	i;
	int	bus_type;
	int	maxdevs;

	/*
	 * allocate space for ha
	 */

	ha = (scsi_ha_t *)kmem_zalloc(sizeof(scsi_ha_t), KM_SLEEP);

	ha->ha_idata = idp;
	ha->ha_base = idp->ioaddr1;
	ha->ha_vect = idp->iov;
	ha->ha_rmkey = idp->idata_rmkey;
	ha->ha_cntlr = idp->cntlr;	/* SDI assigned controller number */
	ha->ha_name = idp->name;	/* SDI assigned name */

	/*
	 * Allocate and initalize the HBA SMP locks
	 */
	dpt_lockinit(ha);

	/*
	 * find the bus type
	 */
	cma.cm_key = ha->ha_rmkey;
	cma.cm_n = 0;
	cma.cm_param = CM_BRDBUSTYPE;
	cma.cm_val = &bus_type;
	cma.cm_vallen = sizeof(cm_num_t);
	cm_begin_trans(cma.cm_key, RM_READ);
	cm_getval(&cma);
	cm_end_trans(cma.cm_key);

	ha->ha_bus_type = bus_type;
	if (bus_type == CM_BUS_EISA) {
		ha->ha_base = (ha->ha_base & (~DPT_IOADDR_MASK)) + 
							DPT_EISA_BASE + 8;
	} else if ( bus_type == CM_BUS_PCI ) {
		ha->ha_base += DPT_PCI_OFFSET;
	}
	
	if (dpt_physreq_init(ha, KM_SLEEP)) {
		cmn_err(CE_WARN,"%s: physreq initialization failed", idp->name);
		dpt_hafree(ha);
		return NULL;
	}

	if (dpt_init_cps(ha)) {
		/* Command Packets initialization failed */
		cmn_err(CE_WARN, "%s: command packets initialization failed",
					idp->name);
		dpt_hafree(ha);
		return NULL;
	}

	/*
	 * Attach interrupts for this device.
	 */
	if (!cm_intr_attach(ha->ha_rmkey, dptintr, ha, &dpt_drvinfo, 
						&idp->idata_intrcookie)) {
		idp->idata_intrcookie = NULL;
		cmn_err(CE_WARN, "dpt_config: cm_intr_attach failed");
		dpt_hafree(ha);
		return NULL;	
	}

	if (dpt_start(idp, ha)) {
		dpt_hafree(ha);
		return NULL;
	}

	return ha;
}

/*
 * int
 * dpt_start(HBA_IDATA_STRUCT *idp, scsi_ha_t *ha)
 *
 * Description:
 *
 * Calling/Exit State:
 * None.
 */

int
dpt_start(HBA_IDATA_STRUCT *idp, scsi_ha_t *ha)
{
	int	i;
	int	c, b, t, l;
	int	maxdevs;

	if (dpt_ha_init( ha )) {
		/* board initialization failed */
#ifdef DPT_DEBUG
		cmn_err(CE_WARN, "%s: board initialization failed", idp->name);
#endif /* DPT_DEBUG */
		return -1;
	}

	/*
	 * Calculate the maximum number of devices
	 * supported by this HBA and allocate the device
	 * queue list of pointers for it.
	 */
	maxdevs = ha->ha_ntargets * ha->ha_nluns * ha->ha_nbus;

	ha->ha_dev = (scsi_lu_t * *)
			kmem_zalloc(maxdevs * sizeof(scsi_lu_t *), KM_SLEEP);

	/* Set up the number of buses, targets and LUNs supported
	 * before we register this HBA.
	 */
	idp->idata_ntargets = ha->ha_ntargets;
	idp->idata_nluns = ha->ha_nluns;
	idp->idata_nbus = ha->ha_nbus;

	/*
	 * SetUp the HBA's Target ID based on the EATA ReadConfig data.
	 */
	for(b = 0; b < ha->ha_nbus; b++)
		idp->ha_chan_id[b] = ha->ha_id[b];
	idp->ha_id = ha->ha_id[0];	/* don't forget to set this */

	/*
	 * Initally we will allocate a single structure and make
	 * all of the device entries point to it so the bus scan
	 * can be done and the EDT generated.
	 */
	ha->ha_dev[0] = dpt_qalloc();

	maxdevs = ha->ha_ntargets * ha->ha_nluns * ha->ha_nbus;

	for (i = 1; i < maxdevs; ++i) {
		ha->ha_dev[i] = ha->ha_dev[0];
	}

	c = ha->ha_cntlr;
	dpt_sc_ha[c] = ha;
	idp->active = 1;

	/* Register the HBA devices with SDI */
	if ((i = sdi_register(&dpthba_info, idp)) < 0) {
		cmn_err(CE_WARN,
			"%s: sdi_register() failed, status=%d", idp->name, i);
		dpt_sc_ha[c] = NULL;
		return -1;
	}

	/* Clear out all of the device pointers so they can be set
	 * up later for the devices that were found by the system
	 * scan
	 */
	dpt_qfree(ha->ha_dev[0]);

	for (i = 0; i < maxdevs; ++i) {
		ha->ha_dev[i] = 0;
	}

	/*
	 * Find and alloc a device queue structure for all
	 * devices in the EDT
	 */
	for (b = 0; b < ha->ha_nbus; b++)  {
		for (t = 0; t < ha->ha_ntargets; t++)  {
			for (l = 0; l < ha->ha_nluns; l++)  {

				struct sdi_edt *edtp;

				edtp = sdi_rxedt(c, b, t, l);
				if (!edtp)
					continue;

				LU_Q(ha,b,t,l) = dpt_qalloc();

				if (LU_Q(ha,b,t,l) == NULL) {
					cmn_err(CE_WARN, 
			"%s: cannot allocate logical unit queues", idp->name);
					dpt_sc_ha[c] = NULL;
					return(-1);
				}
			}
		}
	}

	/*
	 * Set all unallocated Lu Queue structure pointers to point
	 * to the HBA Lu Queue structure to give access to an
	 * unallocated device.
	 */

	for (b = 0; b < ha->ha_nbus; b++)  {
		scsi_lu_t * q = LU_Q(ha,b,ha->ha_id[b],0);
		q->q_flag |= DPT_QHBA;
		for (t = 0; t < ha->ha_ntargets; t++) 
			for (l = 0; l < ha->ha_nluns; l++) 
				if (LU_Q(ha,b,t,l) == NULL)
					LU_Q(ha,b,t,l) = q;
	}

#ifdef DPT_TARGET_MODE
	if (dpt_targetmode) {
		for (b = 0; b < ha->ha_nbus; b++) {
			ha->ha_tgCcb[b] = dpt_getccb(ha);
			dpt_targSet(ha, b, ha->ha_tgCcb[b]);
		}
	}
#endif

	return(0);
}

STATIC void
dpt_suspend(scsi_ha_t *ha)
{
	int c, b, t, l;
	scsi_lu_t * q;

	ha->ha_state |= C_SUSPENDED;

	/*
	 * Loop through all devices on this HBA and wait until all jobs
	 * currently sent to the device have completed
	 */
	c = ha->ha_cntlr;
	for (b = 0; b < ha->ha_nbus; b++) 
		for ( t = 0; t < ha->ha_ntargets; t++) 
			for ( l = 0; l <= ha->ha_nluns; l++) {
				q = LU_Q(ha, b, t, l);
				ASSERT(q);
				if (q->q_flag & DPT_QHBA)
					continue;

				dpt_waitq(ha, q);
			}

	dpt_halt(ha);	/* stop the controller */

	for (b=0; b < ha->ha_nbus; b++)
		dpt_SendEATACmd(ha, b, BUS_QUIET);
}

STATIC void
dpt_resume(scsi_ha_t *ha)
{
	int c, b, t, l;
	scsi_lu_t * q;

	for (b=0; b < ha->ha_nbus; b++) 
		dpt_SendEATACmd(ha, b, BUS_UNQUIET);

	ha->ha_state &= ~C_SUSPENDED;

	c = ha->ha_cntlr;
	for (b=0; b < ha->ha_nbus; b++) 
		for (t = 0; t < ha->ha_ntargets; t++) 
			for (l = 0; l <= ha->ha_nluns; l++) {
				q = LU_Q(ha,b,t,l);
				ASSERT(q);
				if (q->q_flag & DPT_QHBA)
					continue;
				DPT_SCSILU_LOCK(q);
				dpt_next(ha, q );  /* start all the queues */
			}
}

STATIC int
dpt_reinit(scsi_ha_t *ha)
{
	cm_args_t	cma;
	int	b;
	int	bus_type;
	HBA_IDATA_STRUCT *idp = ha->ha_idata;

	ha->ha_base = idp->ioaddr1;
	ha->ha_vect = idp->iov;
	ha->ha_rmkey = idp->idata_rmkey;

	/*
	 * Find the bus type
	 */
	cma.cm_key = ha->ha_rmkey;
	cma.cm_n = 0;
	cma.cm_param = CM_BRDBUSTYPE;
	cma.cm_val = &bus_type;
	cma.cm_vallen = sizeof(cm_num_t);
	cm_begin_trans(cma.cm_key, RM_READ);
	cm_getval(&cma);
	cm_end_trans(cma.cm_key);

	if (bus_type == CM_BUS_EISA) {
		ha->ha_base = (ha->ha_base & (~DPT_IOADDR_MASK)) + 
							DPT_EISA_BASE + 8;
	} else if ( bus_type == CM_BUS_PCI ) {
		ha->ha_base += DPT_PCI_OFFSET;
	}
	
	if (bus_type != ha->ha_bus_type) {
		cmn_err(CE_WARN, "%s: bus type mismatch", ha->ha_name);
		return -1;
	}

	/*
	 * Attach interrupts for this device.
	 */
	if (!cm_intr_attach(ha->ha_rmkey, dptintr, ha, &dpt_drvinfo, 
						&idp->idata_intrcookie)) {
		idp->idata_intrcookie = NULL;
		cmn_err(CE_WARN, "dpt_config: cm_intr_attach failed");
		return -1;	
	}

	if (dpt_ha_init( ha )) {
		cmn_err(CE_WARN, "%s: board initialization failed", idp->name);
		cm_intr_detach(idp->idata_intrcookie); /* detach interrupts */
		idp->idata_intrcookie = NULL;
		return -1;
	}

	/* make sure the bord scsi id hasn't changed */
	for(b = 0; b < ha->ha_nbus; b++) {
		if (idp->ha_chan_id[b] != ha->ha_id[b]) {
			cmn_err(CE_WARN, "%s: board id mismatch", ha->ha_name);
			cm_intr_detach(idp->idata_intrcookie); 
			idp->idata_intrcookie = NULL;
			return -1;
		}
	}

	/* make sure the number of devices supported hasn't changed */
	if ( (idp->idata_nbus != ha->ha_nbus) ||
	     (idp->idata_ntargets != ha->ha_ntargets) ||
	     (idp->idata_nluns != ha->ha_nluns) ) {
		cmn_err(CE_WARN, "%s: device support mismatch", ha->ha_name);
		cm_intr_detach(idp->idata_intrcookie); /* detach interrupts */
		idp->idata_intrcookie = NULL;
		return -1;
	}
	   
#ifdef DPT_TARGET_MODE
	if (dpt_targetmode) {
		for (b = 0; b < ha->ha_nbus; b++) {
			dpt_targSet(ha, b, ha->ha_tgCcb[b]);
		}
	}
#endif

	return(0);
}

STATIC void
dpt_stop(scsi_ha_t *ha)
{
	int	b,t,l;
	scsi_lu_t * q;

	/*
	 * Loop through all devices on this HBA bus and fail all 
	 * the incoming jobs as well as all the pending jobs
	 */
	for (b = 0; b < ha->ha_nbus; b++)
		for (t = 0; t < ha->ha_ntargets; t++)
			for (l = 0; l < ha->ha_nluns; l++) {
				q = LU_Q(ha, b, t, l);
				ASSERT(q);
				if (q->q_flag & DPT_QHBA)
					continue;

				/* fail all the pending jobs */
				dpt_flushq(q, SDI_NOTEQ, 1);
			}
}
	
void
dpt_hafree(scsi_ha_t *ha)
{
	scsi_lu_t *q;
	int c, b,i;
	int devs;
	ccb_t *cp, *cp1;

	if (!ha) 
		return;		/* nothing to do */

	/* free the queues */
	if (ha->ha_dev) {
		for (b = 0; b < ha->ha_nbus; b++) {
			q = LU_Q(ha,b,ha->ha_id[b],0);
			devs = ha->ha_ntargets * ha->ha_nluns;
			for (i = 0; i < devs; i++)  {
				if ((ha->ha_dev[i] == NULL) || 
				    (ha->ha_dev[i] == q))
					continue;

				dpt_qfree(ha->ha_dev[i]);
			}
			dpt_qfree(q);
		}

		/* free the array of pointers */
		devs = ha->ha_ntargets * ha->ha_nluns * ha->ha_nbus;
		kmem_free(ha->ha_dev, devs * sizeof(scsi_lu_t *));
	}

	if (ha->sc_hastat) {
		kmem_free(ha->sc_hastat, sizeof(scsi_stat_t));
	}

	cp = ha->ha_cblist;
	while (cp) {
		cp1 = cp->c_next;
		kmem_free(cp, sizeof(ccb_t));
		cp = cp1;
	}
	ha->ha_cblist = NULL;

	if (ha->ha_physreq)
		physreq_free(ha->ha_physreq);

	kmem_free(ha, sizeof(scsi_ha_t));
}

/*
 * void
 * dpt_halt(scsi_ha_t *ha)
 *
 * Description:
 * Called by kernel at shutdown, the cache on all DPT controllers
 * is flushed
 *
 * Calling/Exit State:
 * None.
 */

void
dpt_halt(scsi_ha_t *ha)
{
	int c = ha->ha_cntlr;
	int b, t, l;
	ccb_t *cp;
	scsi_lu_t *q;
	extern dpt_flush_time_out;

	/*
	 * Flush and invalidate cache for all SCSI ID/LUNs by issuing a 
	 * SCSI ALLOW MEDIA REMOVAL command.
	 * This will cause the HBA to flush all requests to the device and
	 * then invalidate its cache for that device.  This code operates
	 * as a single thread, only one device is flushed at a time. It
	 * would probably be better to send all of them at once.
	 */

	cp = dpt_getccb(ha);
	cp->c_time = 0;

	/*
	 * Build the EATA Command Packet structure for
	 * a SCSI Prevent/Allow Cmd
	 */
	cp->CPop.byte  = 0;
	cp->CPcdb[0]  = SS_LOCK;
	*(ulong * )&cp->CPcdb[1] = 0;
	cp->CPcdb[5]  = 0;
	cp->CPdataDMA   = 0L;

	/*
	*+ DPT issuing command to flush cache.
	*/
#ifdef DPT_DEBUG
	cmn_err(CE_NOTE, "%s: Flushing cache, if present.", ha->ha_name);
#endif /* DPT_DEBUG */
	for (b = 0; b < ha->ha_nbus; b++) {
		for (t = 0;t < ha->ha_ntargets; t++) {
			for (l = 0;l < ha->ha_nluns; l++) {
				q = LU_Q(ha,b,t,l);
				if (q->q_flag & DPT_QHBA)
					continue;
				cp->CPbus = b;
				cp->CPID  = t;
				cp->CPmsg0 = (HA_IDENTIFY_MSG|HA_DISCO_RECO) + l;
				cp->c_bind = NULL;

				if ( dpt_send_wait(ha, cp, dpt_flush_time_out) ==
								FAILURE ) {
					/*
					*+ DPT flush timed out before completed
					*+ for given target,lun.
					*/
					cmn_err(CE_NOTE, 
					     "%s(%d,%d,%d): Incomplete Flush ",
						ha->ha_name,b,t,l);
				}
			}
		}
	}

	/*
	*+ DPT flush completed successfully.
	*/
#ifdef DPT_DEBUG
	cmn_err(CE_NOTE, "%s: Flushing complete.", ha->ha_name);
#endif /* DPT_DEBUG */
	dpt_freeccb(ha, cp); /* Release SCSI command block. */
}

/*
 * STATIC int
 * dpt_verify(rm_key_t key)
 *
 * Calling/Exit State:
 * none
 *
 * Description:
 *  Verify the board instance.
 */
int
dpt_verify(rm_key_t key)
{
	HBA_IDATA_STRUCT vfy_idata;
	int	rv;
	int	bus_type;
	ulong_t ha_base;
	struct	RdConfig *eata_cfg;	/* Pointer to EATA Read Config struct */
	cm_args_t	cma;

	/*
	 * find the bus type
	 */
	cma.cm_key = key;
	cma.cm_n = 0;
	cma.cm_param = CM_BRDBUSTYPE;
	cma.cm_val = &bus_type;
	cma.cm_vallen = sizeof(cm_num_t);
	cm_begin_trans(cma.cm_key, RM_READ);
	cm_getval(&cma);
	cm_end_trans(cma.cm_key);

	if (bus_type == CM_BUS_EISA || bus_type == CM_BUS_PCI)
		return 0;

	/* only for ISA controller */
	/*
	 * Read the hardware parameters associated with the
	 * given Resource Manager key, and assign them to
	 * the idata structure.
	 */
	rv = sdi_hba_getconf(key, &vfy_idata);

	if (rv != 0) {
#ifdef DPT_DEBUG
		cmn_err(CE_WARN, "dpt_verify: sdi_hba_getconf failed (rv=%d)\n", rv);
#endif /* DPT_DEBUG */
		return ENODEV;
	}

	/*
	 * Verify hardware parameters in vfy_idata,
	 * return 0 on success, ENODEV otherwise.
	 */

	ha_base = vfy_idata.ioaddr1;

	eata_cfg = (struct RdConfig *)
			kmem_zalloc(sizeof(struct RdConfig ), KM_SLEEP);

	if ( eata_cfg == NULL) {
		cmn_err(CE_WARN, "dpt: failed allocating eata_cfg");
		return -1;
	}

	if (!(rv = EATA_ReadConfig(ha_base, eata_cfg))) {
		/*
		 * Controller may not be in a sane state, try resetting
		 */
		dpt_board_reset(ha_base);
		rv = EATA_ReadConfig(ha_base, eata_cfg);
	}

	kmem_free(eata_cfg, sizeof(struct RdConfig ));

	if (!rv)
		return ENODEV;

	return 0;
}

/*
 * int
 * dptopen(dev_t *devp, int flags, int otype, cred_t *cred_p)
 *
 * Description:
 *  Driver open() entry point. It checks permissions, and in the
 * case of a pass-thru open, suspends the particular LU queue.
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC int
dptopen(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;

	int	c = SC_EXHAN(dev);
	int	b = SC_BUS(dev);
	int	t = SC_EXTCN(dev);
	int	l = SC_EXLUN(dev);

	scsi_ha_t	*ha = dpt_sc_ha[c];
	scsi_lu_t	*q;

	if (dpt_illegal(c, b, t, l, 0)) {
		return(ENXIO);
	}

	if (t == ha->ha_id[b]) {
		return(0);
	}

	q = LU_Q(ha,b,t,l);

	DPT_SCSILU_LOCK(q);

	if ((q->q_count > 0) ||
	    (q->q_active == ha->ha_max_jobs) ||
	    (q->q_flag & (DPT_QSUSP | DPT_QPTHRU)) ||
	    (ha->ha_state & (C_SUSPENDED | C_REMOVED))) {

		DPT_SCSILU_UNLOCK(q);

		return(EBUSY);
	}

	q->q_flag |= DPT_QPTHRU;

	DPT_SCSILU_UNLOCK(q);

	return(0);
}

/*
 * int
 * dptclose(dev_t dev, int flags, int otype, cred_t *cred_p)
 *
 * Description:
 *  Driver close() entry point.  In the case of a pass-thru close
 * it resumes the queue and calls the target driver event handler
 * if one is present.
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC int
dptclose(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	int	c = SC_EXHAN(dev);
	int	b = SC_BUS(dev);
	int	t = SC_EXTCN(dev);
	int	l = SC_EXLUN(dev);

	scsi_ha_t *ha = dpt_sc_ha[c];
	scsi_lu_t *q;

	if (t == ha->ha_id[b]) {
		return(0);
	}

	q = LU_Q(ha,b,t,l);
	DPT_SCSILU_LOCK(q);
	q->q_flag &= ~DPT_QPTHRU;
	if (q->q_func != NULL) {
		DPT_SCSILU_UNLOCK(q);
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
		DPT_SCSILU_LOCK(q);
	}

	dpt_next(ha, q);

	return(0);
}

/*
 * int
 * dptioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
 *
 * Description:
 * Driver ioctl() entry point.  Used to implement the following
 * special functions:
 *
 * SDI_SEND - Send a user supplied SCSI Control Block to the specified device.
 * B_GETTYPE -  Get bus type and driver name
 * B_HA_CNT  - Get number of HA boards configured
 * REDT      - Read the extended EDT from RAM memory
 * SDI_BRESET - Reset the specified SCSI bus
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC int
dptioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	int	c = SC_EXHAN(dev);
	int	b = SC_BUS(dev);
	int	t = SC_EXTCN(dev);
	int	l = SC_EXLUN(dev);

	pl_t	opri;
	int	i;
	scsi_ha_t *ha = dpt_sc_ha[c];
	struct sb *sp;

	switch (cmd) {
	case SDI_SEND:
	{
		buf_t *bp;
		struct sb karg;
		int errnum = 0, rw;
		char	*save_priv;
		scsi_lu_t * q;
		EataPassThru_t EataPassThru;
		paddr_t paddr;

		/*
		 * If the target ID is the host adapters target ID check to
		 * see if this is an EATA pass-through command
		 */
		if (t == ha->ha_id[b]) {

			if (copyin(arg, (caddr_t) & EataPassThru, 4)) {
				return(EFAULT);
			}

			/*
			 * An EATA pass-through command will have
			 * the EATA signature
			 */
			if ((EataPassThru.EataID[0] != 'E') ||
			    (EataPassThru.EataID[1] != 'A') ||
			    (EataPassThru.EataID[2] != 'T') ||
			    (EataPassThru.EataID[3] != 'A'))   {
				return(ENXIO);
			}

			/*
			 * We have a winner so copy in the rest
			 * of the structure
			 */
			if (copyin(arg, (caddr_t) & EataPassThru, sizeof(EataPassThru_t))) {
				return(EFAULT);
			}

			/* Handle the specific commands */
			switch (EataPassThru.EataCmd) {
			case DPT_SIGNATURE :
				if (copyout((caddr_t) & dpt_sig, EataPassThru.CmdBuffer, sizeof(dpt_sig_t))) {
					return(EFAULT);
				}

				break;

			case DPT_CTRLINFO :
				if (copyout((caddr_t)ha, EataPassThru.CmdBuffer, 56)) {
					return(EFAULT);
				}

				break;

			case DPT_BLINKLED:
				DPT_STPKT_LOCK(ha, opri);
				i = dptr_BlinkLED((int)(ha->ha_base));
				DPT_STPKT_UNLOCK(ha, opri);
				if (copyout((caddr_t) &i, EataPassThru.CmdBuffer, 4)) {
					return(EFAULT);
				}
				break;

			default:
				return(EINVAL);
			}

			return(0);

		}

		/*
		 * Handle all non EATA pass-through commands
		 */
		if (copyin(arg, (caddr_t) & karg, sizeof(struct sb ))) {
			return(EFAULT);
		}

		if ((karg.sb_type != ISCB_TYPE) ||
		    (karg.SCB.sc_cmdsz <= 0 )   ||
		    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) {
			return(EINVAL);
		}

		sp = sdi_xgetblk(HBA_EXT_ADDRESS, KM_SLEEP);

		/*
		 * Save address format and extended address pointer
		 */
		karg.SCB.sc_dev.pdi_adr_version = HBA_EXT_ADDRESS;
		karg.SCB.sc_dev.extended_address =
			sp->SCB.sc_dev.extended_address;

		save_priv = sp->SCB.sc_priv;

		bcopy((caddr_t) & karg, (caddr_t)sp, sizeof(struct sb ));

		sp->SCB.sc_priv = save_priv;

		q = LU_Q(ha, b, t, l);

		if (q->q_sc_cmd == NULL) {
			/*
			 * Allocate space for the SCSI command and add
			 * sizeof(int) to account for the scm structure
			 * padding, since this pointer may be adjusted
			 * -2 bytes and cast to struct scm. After the
			 * adjustment we still want to be pointing within
			 * our allocated space!
			 */
			q->q_sc_cmd = (char *)kmem_zalloc(MAX_CMDSZ + sizeof(int), KM_SLEEP) + sizeof(int);
		}

		sp->SCB.sc_cmdpt = q->q_sc_cmd;

		if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt, sp->SCB.sc_cmdsz)) {
			errnum = EFAULT;

			goto done;
		}

		SDI_HAN_32(&sp->SCB.sc_dev) = c;
		SDI_BUS_32(&sp->SCB.sc_dev) = b;
		SDI_TCN_32(&sp->SCB.sc_dev) = t;
		SDI_LUN_32(&sp->SCB.sc_dev) = l;

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;

		bp = getrbuf(KM_SLEEP);

		bp->b_flags |= rw;
		bp->b_priv.un_ptr = sp;

		/*
		 * If the job involves a data transfer then the
		 * request is done thru uiobuf/buf_breakup so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then dpt_pass_thru()
		 * is called directly.
		 */
		if (sp->SCB.sc_datasz > 0) {

			struct iovec ha_iov;
			struct uio ha_uio;
			char	op;
			bcb_t	*bcbp;

			ha_iov.iov_base = sp->SCB.sc_datapt;
			ha_iov.iov_len = sp->SCB.sc_datasz;
			ha_uio.uio_iov = &ha_iov;
			ha_uio.uio_iovcnt = 1;
			ha_uio.uio_segflg = UIO_USERSPACE;
			ha_uio.uio_resid = sp->SCB.sc_datasz;

			op = q->q_sc_cmd[0];

			if (op == SS_READ || op == SS_WRITE) {
				struct scs *scs = (struct scs *)(q->q_sc_cmd);
				bp->b_blkno = ((sdi_swap16(scs->ss_addr) & 0xffff) + (scs->ss_addr1 << 16));
				bp->b_blkoff = 0;
			}

			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm = (struct scm *)((char *)q->q_sc_cmd - 2);
				bp->b_blkno = sdi_swap32(scm->sm_addr);
				bp->b_blkoff = 0;
			}

			bcbp = sdi_xgetbcb(HBA_EXT_ADDRESS, &sp->SCB.sc_dev, 
						KM_SLEEP);
			
			bcbp->bcb_flags = BCB_SYNCHRONOUS|BCB_ONE_PIECE;
			bcbp->bcb_granularity = 1;

			(void) uiobuf(bp, &ha_uio);
			buf_breakup(dpt_pass_thru, bp, bcbp);

			biowait(bp);

			sdi_freebcb(bcbp);
		}
		else {
			bp->b_un.b_addr = NULL;
			bp->b_bcount = 0;
			bp->b_blkno = 0;
			bp->b_blkoff = 0;

			dpt_pass_thru(bp);  

			biowait(bp);
		}


		/* update user SCB fields */
		karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
		karg.SCB.sc_status = sp->SCB.sc_status;
		karg.SCB.sc_time = sp->SCB.sc_time;

		if (copyout((caddr_t) & karg, arg, sizeof(struct sb ))) {
			errnum = EFAULT;
		}
done:
		freerbuf(bp);
		sdi_xfreeblk(HBA_EXT_ADDRESS, sp);
		return(errnum);
	}

	case B_GETTYPE:

		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}

		if (copyout("scsi", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}

		break;

	case SDI_HBANAME:

		if (copyout("dpt", arg, 3)) {
			return(EFAULT);
		}

		break;

	case SDI_BRESET:

		DPT_STPKT_LOCK(ha, opri);
		if (ha->ha_npend > 0) {
			DPT_STPKT_UNLOCK(ha, opri);
			return(EBUSY); /* there are jobs outstanding */
		}
		DPT_STPKT_UNLOCK(ha, opri);

		/* DPT Host Adapter SCSI bus reset command issued.  */
		cmn_err(CE_WARN, "!dpt: HA %d - Bus reset\n", c);

		outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(1000);

		break;

	default:

		return(EINVAL);
	}

	return(0);
}

/* Callback handler for blocking EATA commands */
STATIC void
dptintr_blocking (scsi_ha_t *ha, ccb_t *cp)
{
	/*
	 * The command was issued internally, and dpt_wait was
	 * called to poll for completion.  Make sure the parameters
	 * indicate this, since otherwise it is spurious.
	 */
	if ((ha->ha_waitflag == TRUE) && 
	    (ha->ha_cmd_in_progress == cp->CPcdb[0])) {
		ha->ha_waitflag = FALSE;
		return;
	}

	/*
	 *+ DPT unexpected interrupt received. No associated SCSI
	 *+ block set.
	 */
	cmn_err(CE_NOTE, "!%s: Spurious Interrupt; blocking task not set.",
	    ha->ha_name);

	return;
}

#ifdef DPT_TARGET_MODE
STATIC void
dptintr_target (scsi_ha_t *ha, ccb_t *cp)
{
	/*
	 * A Write buffer has occurred, the target driver must be notified.
	 */
	if (dpt_targetmode) {
		int c = ha->ha_cntlr;
		int b = (int) cp->CPbus;
		int t = (int) cp->CPID;
		int l = (int) cp->CPmsg0 & 0x07;
		int offset = dpt_swap32(*((long *)cp->CP_status.SP_Buf_off));

		sdi_xaen(SDI_TARMOD_WRCOMP, c,t,l,b,offset);
		dpt_targSet(ha, b, cp);
		return;
	}
	cmn_err(CE_NOTE, "!%s: Spurious Interrupt; target mode not set.",
	    						ha->ha_name);
}
#endif  /* #ifdef DPT_TARGET_MODE */

STATIC void
dptintr_done(scsi_ha_t *ha, ccb_t *cp)
{
	if (cp->c_bind == NULL) {
		/*
		 *+ DPT unexpected interrupt received. No associated SCSI
		 *+ block set.
		 */
		cmn_err(CE_NOTE, "!%s: Spurious Interrupt; c_bind not set.",
		   ha->ha_name);

		return;
	}

	dpt_done(ha, cp, cp->CP_ha_status, 
		dpt_swap32(*((long *)cp->CP_status.SP_Inv_Residue)));

	return;
}

/*
 * int
 * dptintr(scsi_ha_t *ha)
 *
 * Description:
 *	Driver interrupt handler entry point.
 *	Called by kernel when a host adapter interrupt occurs.
 *
 * Calling/Exit State:
 * None.
 */

int
dptintr(scsi_ha_t *ha)
{
	pl_t  opri;
	ccb_t *cp;
	scsi_stat_t *sp;

	ASSERT(ha);

	if ( !(inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_INTR) ) {
#ifdef DPT_INTR_DEBUG
		cmn_err(CE_WARN, 
			"dptintr: unexpected/shared interrupt received");
#endif /* DPT_INTR_DEBUG */
		/* ignore it */
		return ISTAT_NONE;
	}

	/* get Status Packet pointer */
	sp = ha->sc_hastat;

	/* Get Command Packet Vpointer.*/
	if ( (cp = sp->CPaddr.vp) == 0 ) {

		/*
		 *+ DPT unexpected interrupt received. Command control
		 *+ block not set.
		 */
		cmn_err(CE_NOTE,"!%s: Spurious Interrupt; CCB pointer not set.",
				 ha->ha_name);

		(void)inb(ha->ha_base + HA_STATUS);

		return ISTAT_ASSUMED;
	}
	/*
	 * Clear the sp->CPaddr.vp so that a subsequent command completion 
	 * will not get a false command completion status code.
	 */
	sp->CPaddr.vp = 0; 

	cp->c_active = FALSE;  /* Mark it not active.	 */

	bcopy(sp, &cp->CP_status, sizeof(scsi_stat_t));
	cp->CP_status.SP_Controller_Status &= HA_STATUS_MASK;

	/* Read the status register to clear the interrupt on the adapter */
	cp->CP_ha_status = inb(ha->ha_base + HA_STATUS);

	DPT_STPKT_LOCK(ha, opri);
	ha->ha_npend--;
	DPT_STPKT_UNLOCK(ha, opri);

	(*cp->c_callback) (ha, (ccb_t *)(cp->c_vp));
	return ISTAT_ASSERTED;
}

/*
 * void
 * dpt_done(scsi_ha_t *ha, ccb_t *cp, int status, int resid)
 *
 * Description:
 * This is the interrupt handler routine for SCSI jobs which have
 * a controller CB and a SCB structure defining the job.
 *
 * Calling/Exit State:
 * None.
 */

void
dpt_done(scsi_ha_t *ha, ccb_t *cp, int status, int resid)
{
	register scsi_lu_t *q, *q1;
	struct sb *sp;
	int	b, t, l;
#ifdef DPT_DEBUG
	int	c = ha->ha_cntlr;
#endif /* DPT_DEBUG */	

	b = (int)cp->CPbus;
	t = (int)cp->CPID;
	l = (int)cp->CPmsg0 & 0x07;

	q = LU_Q(ha,b,t,l);

	ASSERT(q);

	sp = cp->c_bind;

	ASSERT(sp);

	if (sp->sb_type == SFB_TYPE) {

#ifdef DPT_TIMEOUT_RESET_SUPPORT
		if( sp->SFB.sf_func == SFB_RESET_DEVICE || 
		    sp->SFB.sf_func == SFB_RESET_BUS ) {

			if ( (status & HA_ST_ERROR) && 
			     cp->CP_status.SP_Controller_Status != S_GOOD ) {
				sp->SFB.sf_comp_code = SDI_HAERR;
			}
			else {
				sp->SFB.sf_comp_code = SDI_ASW;
			}
		}
		else {
			if ( (status & HA_ST_ERROR) && 
			     cp->CP_status.SP_Controller_Status != S_GOOD ) {
				sp->SFB.sf_comp_code = SDI_RETRY;
			}
			else {
				sp->SFB.sf_comp_code = SDI_ASW;
			}
		}
#else
		if ( (status & HA_ST_ERROR) && 
		     cp->CP_status.SP_Controller_Status != S_GOOD ) {
			sp->SFB.sf_comp_code = SDI_RETRY;
		}
		else {
			sp->SFB.sf_comp_code = SDI_ASW;
		}
#endif /* DPT_TIMEOUT_RESET_SUPPORT */

	} /* SFB type */
	else {	/* SCB type */
		sp->SCB.sc_resid = resid;

		DPT_SCSILU_LOCK(q);

		/*
		 * Old sense data is now invalid
		 */
		q->q_flag &= ~DPT_QSENSE;

		DPT_SCSILU_UNLOCK(q);

		if ( !(status & HA_ST_ERROR) && 
		     cp->CP_status.SP_Controller_Status == 0 ) {

			sp->SCB.sc_comp_code = SDI_ASW;
		}
		else if (cp->CP_status.SP_Controller_Status) {

			sp->SCB.sc_status = cp->CP_status.SP_SCSI_Status;

			switch (cp->CP_status.SP_Controller_Status) {
			case HA_ERR_SELTO:

				sp->SCB.sc_comp_code = SDI_NOSELE;
	/*			sp->SCB.sc_comp_code = SDI_HAERR; */

#ifdef DPT_DEBUG
				cmn_err(CE_NOTE,"dpt_done[c%db%dt%dd%d]: Command[0x%x]; Selection TimeOut", c, b, t, l, ((struct scs *)sp->SCB.sc_cmdpt)->ss_op);
#endif
				break;

			case HA_ERR_CMDTO:

				sp->SCB.sc_comp_code = SDI_TIME;

#ifdef DPT_DEBUG
				cmn_err(CE_NOTE,"dpt_done[c%db%dt%dd%d]: Command[0x%x]; Command TimeOut", c, b, t, l, ((struct scs *)sp->SCB.sc_cmdpt)->ss_op);
#endif
				break;

			case HA_ERR_SHUNG:

				sp->SCB.sc_comp_code = SDI_HAERR;

#ifdef DPT_DEBUG
				cmn_err(CE_NOTE,"dpt_done[c%db%dt%dd%d]: Command[0x%x]; SCSI Bus Hung", c, b, t, l, ((struct scs *)sp->SCB.sc_cmdpt)->ss_op);
#endif
				break;

			case HA_ERR_RESET:

				sp->SCB.sc_comp_code = SDI_RESET;

#ifdef DPT_DEBUG
				cmn_err(CE_NOTE,"dpt_done[c%db%dt%dd%d]: Command[0x%x]; SCSI Bus Reset Detected", c, b, t, l, ((struct scs *)sp->SCB.sc_cmdpt)->ss_op);
#endif
				break;

			case HA_ERR_INITPWR:

				sp->SCB.sc_comp_code = SDI_ASW;

				break;

#ifdef DPT_TIMEOUT_RESET_SUPPORT
			case HA_ERR_CPABRTNA:	/* CP aborted - NOT on Bus */
			case HA_ERR_CPABORTED:	/* CP aborted - WAS on Bus */
				sp->SCB.sc_comp_code = SDI_ABORT;
				break;
#endif /** DPT_TIMEOUT_RESET_SUPPORT **/

			default:

				sp->SCB.sc_comp_code = SDI_RETRY;

#ifdef DPT_DEBUG
				cmn_err(CE_NOTE,"dpt_done[c%db%dt%dd%d]: Command[0x%x]; Controller Status[0x%x]", c, b, t, l, ((struct scs *)sp->SCB.sc_cmdpt)->ss_op, cp->CP_status.SP_Controller_Status);
#endif
				break;

			}
		}
		else if (cp->CP_status.SP_SCSI_Status) {
#ifdef DPT_DEBUG
			cmn_err(CE_NOTE,"!dpt_done[c%db%dt%dd%d]: Command[0x%x]; SCSI Status[0x%x]", c, b, t, l, ((struct scs *)sp->SCB.sc_cmdpt)->ss_op, cp->CP_status.SP_SCSI_Status);
#endif
			sp->SCB.sc_status = cp->CP_status.SP_SCSI_Status;

			sp->SCB.sc_comp_code = SDI_CKSTAT;

			bcopy((caddr_t)&cp->CP_sense, 
			    (caddr_t)(sdi_sense_ptr(sp)), sizeof(struct sense));
			/*
			 * Cache request sense info into QueSense
			 */
			DPT_SCSILU_LOCK(q);

			bcopy((caddr_t)&cp->CP_sense, 
				(caddr_t)(&q->q_sense), sizeof(struct sense));

			q->q_flag |= DPT_QSENSE;

			DPT_SCSILU_UNLOCK(q);
		}
		else {
			sp->SCB.sc_comp_code = SDI_ASW;
		}

#ifdef DPT_TIMEOUT_RESET_SUPPORT
		dpt_timeout_remove(ha, sp);
#endif
	} /* SCB type */

	/*
	 * call the target driver interrupt handler
	 */
	sdi_callback(sp);

	/*
	 * Release SCSI command block.
	 */
	dpt_freeccb(ha, cp);

	/*
	 * If there is a queue on the waiting list for this HBA, service
	 * it first before we service any possible commands on this queue.
	 */
	if (ha->ha_LuQWaiting != NULL) {

		DPT_QWAIT_LOCK(ha, ha->ha_QWait_opri);

		if (ha->ha_LuQWaiting != NULL) {
			q1 = q;
			q = ha->ha_LuQWaiting;
			ha->ha_LuQWaiting = q->q_WaitingNext;
			DPT_QWAIT_UNLOCK(ha, ha->ha_QWait_opri);
			DPT_SCSILU_LOCK(q);
			q->q_flag &= ~DPT_QWAIT;
			dpt_next(ha, q);
			q = q1;
		}
		else {
			DPT_QWAIT_UNLOCK(ha, ha->ha_QWait_opri);
		}
	}

	DPT_SCSILU_LOCK(q);

	q->q_active--;  /* for this queue in particular */

	dpt_next(ha, q);  /* Send Next Job on the Queue. */
}

/*===========================================================================*/
/* SCSI Driver Interface (SDI-386) Functions                                 */
/*===========================================================================*/

/*
 * long
 * dptsend(struct hbadata *hbap)
 *
 * Description:
 *  Send a SCSI command to a controller.  Commands sent via this
 * function are executed in the order they are received.
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC long
dptsend(struct hbadata *hbap, int sleepflag)
{
	scsi_ha_t *ha;
	scsi_lu_t *q;
	sblk_t *sp = (sblk_t *) hbap;
	struct scsi_ad *sa;
	int	c, b, t, l;

	sa = &sp->sbp->sb.SCB.sc_dev;

	ASSERT(sa);

	c = SDI_HAN_32(sa);
	b = SDI_BUS_32(sa);
	t = SDI_TCN_32(sa);
	l = SDI_LUN_32(sa);

	ha = dpt_sc_ha[c];

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	if (ha->ha_state & C_REMOVED) {	/* fail all the jobs */
		sp->sbp->sb.SCB.sc_comp_code = SDI_NOTEQ;
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);
	}

	q = LU_Q(ha, b, t, l);
	ASSERT(q);

	DPT_SCSILU_LOCK(q);

	if (q->q_flag & DPT_QPTHRU) {
		DPT_SCSILU_UNLOCK(q);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

#ifdef DPT_TARGET_MODE
	if (t == ha->ha_id[b]) {
		struct scs *cdb = (struct scs *)sp->sbp->sb.SCB.sc_cmdpt;
		if (cdb->ss_op == SS_TEST) {
			DPT_SCSILU_UNLOCK(q);
			if (l == 0) {
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			} else {
				sp->sbp->sb.SCB.sc_comp_code = (ulong_t)SDI_HAERR;
			}
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}
	}
#endif /* DPT_TARGET_MODE */

	dpt_putq(q, sp);
	dpt_next(ha, q);

	return (SDI_RET_OK);
}

/*
 * long
 * dpticmd(struct hbadata *hbap, int sleepflag)
 *
 * Description:
 * Send an immediate command.  If the logical unit is busy, the job
 * will be queued until the unit is free.  SFB operations will take
 * priority over SCB operations.
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC long
dpticmd(struct hbadata *hbap, int sleepflag)
{
	int	c, b, t, l;
	struct scsi_ad *sa;
	scsi_lu_t * q;
	struct scs *inq_cdb;
	scsi_ha_t *ha;
	pl_t opri;
	sblk_t  *sp = (sblk_t *)hbap;

	switch (sp->sbp->sb.sb_type) {

	case SFB_TYPE:

		sa = &sp->sbp->sb.SFB.sf_dev;

		ASSERT(sa);

		c = SDI_HAN_32(sa);
		b = SDI_BUS_32(sa);
		t = SDI_TCN_32(sa);
		l = SDI_LUN_32(sa);

		ha = dpt_sc_ha[c];

		if (ha->ha_state & C_REMOVED) {	/* fail all the jobs */
			sp->sbp->sb.SFB.sf_comp_code = SDI_NOTEQ;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		q = LU_Q(ha, b, t, l);
		ASSERT(q);

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {

		case SFB_PAUSE:

			dpt_PauseBus(ha, b);

			dpt_SendEATACmd(ha, b, BUS_QUIET);

			break;

		case SFB_CONTINUE:

			dpt_SendEATACmd(ha, b, BUS_UNQUIET);

			dpt_ResumeBus(ha, b);

			break;

		case SFB_ADD_DEV:

			/* Allocate a device structure for this device */
			q = dpt_qalloc();
			LU_Q(ha,b,t,l) = q;

			break;

		case SFB_RM_DEV:

			DPT_SCSILU_LOCK(q);

			/* Make sure that there is no IO for this device and
			 * it is not the HBA
			 */
			if ((q->q_first == NULL) && (q->q_count == 0) && 
						(!(q->q_flag & DPT_QHBA))) {
				DPT_SCSILU_UNLOCK(q);
				dpt_qfree(q);
				q = LU_Q(ha, b, ha->ha_id[b], 0);
				LU_Q(ha, b, t, l) = q;
			}
			else {
				sp->sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
				DPT_SCSILU_UNLOCK(q);
			}

			break;

#ifdef DPT_TIMEOUT_RESET_SUPPORT
		case SFB_TIMEOUT_ON:

			if( ha->ha_watchdog_id == -1 ) {
				ha->ha_flags |= DPT_TIMEOUT_ON;
				ha->ha_actv_jobs = NULL;
				ha->ha_actv_last = NULL;
				ha->ha_watchdog_id = itimeout(dpt_watchdog, ha,
						 (500 | TO_PERIODIC), pldisk );
			}

			break;

		case SFB_TIMEOUT_OFF:

			if( ha->ha_watchdog_id != -1 ) {
				untimeout( ha->ha_watchdog_id );
				ha->ha_watchdog_id = -1;
				ha->ha_flags &= ~DPT_TIMEOUT_ON;
				ha->ha_actv_jobs = NULL;
				ha->ha_actv_last = NULL;
			}
			break;

		case SFB_RESET_BUS:
		case SFB_RESET_DEVICE:

			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			DPT_SCSILU_LOCK(q);
			dpt_putq(q, sp);
			dpt_next(ha, q);
			/* Do not callback until job is complete */
			return SDI_RET_OK;

#endif   /* ifdef DPT_TIMEOUT_RESET_SUPPORT */

		case SFB_RESUME:
			/*
			 *+ DPT device queue being resumed.
			 */
			DPT_SCSILU_LOCK(q);
			q->q_flag &= ~DPT_QSUSP;
			dpt_next(ha, q);
			break;

		case SFB_SUSPEND:
			/*
			 *+ DPT device queue being suspended.
			 */
			DPT_SCSILU_LOCK(q);
			q->q_flag |= DPT_QSUSP;
			DPT_SCSILU_UNLOCK(q);
			break;

		case SFB_ABORTM:
		case SFB_RESETM:

			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			DPT_SCSILU_LOCK(q);
			dpt_putq(q, sp);
			dpt_next(ha, q);
			return (SDI_RET_OK);
			break;

		case SFB_FLUSHR:

			dpt_flushq(q, SDI_QFLUSH, 0);
			break;

		case SFB_NOPF:

			break;

		default:

			sp->sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
		}

		/*
		 * All SFB types have been handled and return codes set
		 * so return back
		 */
		sdi_callback(&sp->sbp->sb);

		return (SDI_RET_OK);

		break;

	case ISCB_TYPE:

		sa = &sp->sbp->sb.SCB.sc_dev;

		ASSERT(sa);

		c = SDI_HAN_32(sa);
		b = SDI_BUS_32(sa);
		t = SDI_TCN_32(sa);
		l = SDI_LUN_32(sa);

		ha = dpt_sc_ha[c];

		if (ha->ha_state & C_REMOVED) {	/* fail all the jobs */
			sp->sbp->sb.SCB.sc_comp_code = SDI_NOTEQ;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		q = LU_Q(ha, b, t, l);
		ASSERT(q);

		inq_cdb = (struct scs *)sp->sbp->sb.SCB.sc_cmdpt;

		if ((t == ha->ha_id[b]) && (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int	inq_len;

			bzero(&inq, sizeof(struct ident ));
			(void)strncpy(inq.id_vendor, ha->ha_name, 
						VID_LEN + PID_LEN + REV_LEN);
			inq.id_type = ID_PROCESOR;
			inq_data = (struct ident *)sp->sbp->sb.SCB.sc_datapt;
			inq_len = sp->sbp->sb.SCB.sc_datasz;
			bcopy((char *) & inq, (char *)inq_data, inq_len);

			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}
#ifdef DPT_TARGET_MODE
		if (t == ha->ha_id[b] && inq_cdb->ss_op == SS_TEST) {
			if (l == 0) {
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			} else {
				sp->sbp->sb.SCB.sc_comp_code = SDI_HAERR;
			}
			sdi_callback(&sp->sbp->sb);
			return(SDI_RET_OK);
		}
#endif
		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		DPT_SCSILU_LOCK(q);

		dpt_putq(q, sp);
		dpt_next(ha, q);

		return (SDI_RET_OK);
		break;

	default:

		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_ERR);
	}
}

/*
 * int
 * dptxlat(struct hbadata *hbap, int flag, proc_t *procp, int sleepflag)
 *
 * Description:
 * Perform the virtual to physical translation on the SCB
 * data pointer.
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC int
dptxlat(struct hbadata *hbap, int flag, proc_t *procp, int sleepflag)
{
	sblk_t *sp = (sblk_t *) hbap;
	buf_t *bp;
	scgth_el32_t *sg;
	int data_type, i;
	caddr_t vaddr;
	
	if (sp->sbp->sb.SCB.sc_link) {
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if (sp->sbp->sb.SCB.sc_datasz) {
		data_type = sdi_data_type((struct sb *)sp->sbp);
		ASSERT(data_type == SDI_PHYS_ADDR || 
			data_type == SDI_IO_BUFFER);

		if (data_type == SDI_IO_BUFFER) {
			bp = sdi_buf_ptr((struct sb *)sp->sbp);
			sg = bp->b_scgth.sg_elem.el32;
			for (i = 0; i < bp->b_scgth.sg_nelem; i++, sg++) {
				sg->sg_base = dpt_swap32(sg->sg_base);
				sg->sg_size = dpt_swap32(sg->sg_size);
			}
			sp->s_scgth = &bp->b_scgth;
			sp->s_len = i * sizeof(scgth_el32_t);
			sp->s_addr = (paddr_t)
					bp->b_scgth.sg_el_addr.ea32.sg_base;
		}
		else {
			/* Get the physical address of the data buffer */
			sp->s_scgth = NULL;
			sp->s_len = sp->sbp->sb.SCB.sc_datasz;
			vaddr = (caddr_t) sdi_datapt_ptr((struct sb *)sp->sbp);
			sp->s_addr = *((paddr_t *)vaddr);
		}
	}
	else {
		sp->s_addr = 0;
		sp->s_len = 0;
	}

	return (SDI_RET_OK);
}

/*
 * struct hbadata *
 * dptgetblk(int sleepflag)
 *
 * Description:
 * Allocate a SB structure for the caller.  The function will
 * sleep if there are no SCSI blocks available.
 *
 * Calling/Exit State:
 * None.
 */

STATIC struct hbadata *
dptgetblk(int sleepflag)
{
	sblk_t  * sp;

#ifdef DPT_TIMEOUT_RESET_SUPPORT
	sp = (sblk_t *)kmem_zalloc( sizeof( sblk_t ), sleepflag );
#else
	sp = (sblk_t * )SDI_GET(&sm_poolhead, sleepflag);
#endif

	return ((struct hbadata *)sp);
}

/*
 * long
 * dptfreeblk(struct hbadata *hbap, int hba_no)
 *
 * Description:
 * Release previously allocated SB structure. 
 * A nonzero return indicates an error in pointer or type field.
 *
 * Calling/Exit State:
 * None.
 */

STATIC long
dptfreeblk(struct hbadata *hbap)
{
	sblk_t *sp = (sblk_t *) hbap;
	struct scsi_ad *sa = &sp->sbp->sb.SCB.sc_dev;
	int	c = SDI_HAN_32(sa);
	scsi_ha_t *ha = dpt_sc_ha[c];

#ifdef DPT_TIMEOUT_RESET_SUPPORT
	if (sp->s_flags & SB_CALLED_BACK) {
#ifdef DPT_TIMEOUT_RESET_DEBUG
		if (dpt_timeout_remove(ha, &sp->sbp->sb)) {
			cmn_err(CE_NOTE, "Freed job removed from timeout list");
		}
		else {
			cmn_err(CE_NOTE, "Freed job not on timeout list");
		}
#else
		dpt_timeout_remove(ha, &sp->sbp->sb);
#endif
	}

	kmem_free(sp, sizeof(sblk_t));
#else
	sdi_free(&sm_poolhead, (jpool_t * )sp);
#endif

	return ((long) SDI_RET_OK);
}

/*
 * void
 * dptgetinfo(struct scsi_ad *sa, struct hbagetinfo *getinfop)
 *
 * Description:
 * Return the name and iotype of the given device.  The name is copied
 * into a string pointed to by the first field of the getinfo structure.
 *
 * Calling/Exit State:
 * None.
 */

STATIC void
dptgetinfo(struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	scsi_ha_t * ha;

	if (getinfop->name) {
		int c, t, i, d[16];
		char *s2 = getinfop->name;

		c = SDI_HAN_32(sa);
		t = SDI_TCN_32(sa);

		strcpy(s2, "HA ");
		s2 += 3;			/* strlen(s2); */
		for (i = 0; c > 0; c /= 10, i++)
			d[i] = c % 10;
		if (i == 0)
			*s2++ = '0';
		else
			while (i-- > 0)
				*s2++ = d[i] + '0';
		
		strcpy(s2, " TC ");
		s2 += 4;			/* strlen(s2); */
		for (i = 0; t > 0; t /= 10, i++)
			d[i] = t % 10;
		if (i == 0)
			*s2++ = '0';
		else
			while (i-- > 0)
				*s2++ = d[i] + '0';

		*s2 = '\0';	
	}
#ifdef DPT_DEBUG
	cmn_err(CE_NOTE, "cntl=%d name=%s", SDI_HAN_32(sa), getinfop->name);
#endif
	ha = dpt_sc_ha[SDI_HAN_32(sa)];

	getinfop->iotype = F_SCGTH | F_RESID;
#ifdef DPT_TARGET_MODE
	if (ha->ha_bus_type & (CM_BUS_EISA|CM_BUS_PCI)) {
		getinfop->iotype |= F_EXIOTYPE;
		getinfop->ex_iotype = F_TARGETMODE;
	}
#endif /* DPT_TARGET_MODE */

	if (getinfop->bcbp) {
		bcb_t	*bcbp = getinfop->bcbp;
		physreq_t *physreqp = bcbp->bcb_physreqp;

		/* initialize the bcb */
		bcbp->bcb_addrtypes = BA_SCGTH;
		bcbp->bcb_flags = 0;
		bcbp->bcb_max_xfer = dpthba_info.max_xfer;
		
		/* initialize the physreq */
		physreqp->phys_align = ha->ha_physreq->phys_align;
		physreqp->phys_boundary = ha->ha_physreq->phys_boundary;
		physreqp->phys_dmasize = ha->ha_physreq->phys_dmasize;
		physreqp->phys_max_scgth = ha->ha_physreq->phys_max_scgth;
	}
}

/*===========================================================================*/
/* SCSI Host Adapter Driver Utilities                                        */
/*===========================================================================*/

/*
 * STATIC void
 * dpt_pass_thru(buf_t *bp)
 * Send a pass-thru job to the HA board.
 *
 * Calling/Exit State:
 * No locks held on entry or exit.
 */
STATIC void
dpt_pass_thru(buf_t *bp)
{
	struct sb *sp = bp->b_priv.un_ptr;
	struct scsi_ad *sa = &sp->SCB.sc_dev;
	int	c = SDI_HAN_32(sa);
	int	b = SDI_BUS_32(sa);
	int	t = SDI_TCN_32(sa);
	int	l = SDI_LUN_32(sa);
	scsi_ha_t *ha = dpt_sc_ha[c];
	scsi_lu_t *q = LU_Q(ha, b, t, l);

	daddr_t blkno_adjust;
	char	op;
	struct xsb  *xsb;

	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = dpt_int;

	xsb = (struct xsb *)sp;
	xsb->extra.sb_bufp = bp;
	xsb->extra.sb_data_type = SDI_IO_BUFFER;

	sdi_xtranslate(HBA_EXT_ADDRESS, sp, bp->b_flags, NULL, KM_SLEEP);
	sp->SCB.sc_mode = bp->b_flags & B_READ ? SCB_READ : SCB_WRITE;

	op = q->q_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)q->q_sc_cmd;
		scs->ss_addr = sdi_swap16(bp->b_blkno);
		scs->ss_addr1 = (bp->b_blkno & 0x1F0000) >> 16;
		scs->ss_len   = (char)(bp->b_bcount >> DPT_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		struct scm *scm = (struct scm *)((char *)q->q_sc_cmd - 2);
		scm->sm_addr = sdi_swap32(bp->b_blkno);
		scm->sm_len  = sdi_swap16(bp->b_bcount >> DPT_BLKSHFT);
	}

	sdi_xicmd(HBA_EXT_ADDRESS, sp, KM_SLEEP);
}

/*
 * void
 * dpt_int(struct sb *sp)
 *
 * Description:
 * This is the interrupt handler for pass-thru jobs.  It just
 * wakes up the sleeping process.
 *
 * Calling/Exit State:
 * None.
 */

void
dpt_int(struct sb *sp)
{
	struct buf *bp;

	bp = (struct buf *) sp->SCB.sc_wd;

	biodone(bp);
}

/*
 * void
 * dpt_flushq(scsi_lu_t *q, int cc, int flag)
 *
 * Description:
 * Empty a logical unit queue.  If flag is set, remove all jobs.
 * Otherwise, remove only non-control jobs.
 *
 * Calling/Exit State:
 * None.
 */

void
dpt_flushq(scsi_lu_t *q, int cc, int flag)
{
	sblk_t  *sp, *nsp;

	ASSERT(q);

	DPT_SCSILU_LOCK(q);

	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;

	DPT_SCSILU_UNLOCK(q);

	while (sp) {

		nsp = sp->s_next;

		if (!flag && (QCLASS(sp) > DPT_QNORM)) {
			DPT_SCSILU_LOCK(q);
			dpt_putq(q, sp);
			DPT_SCSILU_UNLOCK(q);
		}
		else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}

		sp = nsp;
	}
}

/*
 * STATIC void
 * dpt_PutWaitingQue(scsi_ha_t *ha,scsi_lu_t *q);
 * Put a logical LU Queue onto the waiting list in FIFO order.
 *
 * Calling/Exit State:
 * DPT_SCSILU_LOCK(q) for q
 */
STATIC void
dpt_PutWaitingQue(scsi_ha_t *ha, scsi_lu_t *q)
{
	scsi_lu_t * WaitQ_p;

	q->q_WaitingNext = NULL;

	if (ha->ha_LuQWaiting == NULL) {
		/*
		 * List is empty so add it as the first element
		 */
		ha->ha_LuQWaiting = q;
	}
	else {
		/*
		 * List is not empty so add it to the end of the list
		 */
		WaitQ_p = ha->ha_LuQWaiting;

		while (WaitQ_p->q_WaitingNext != NULL) {
			WaitQ_p = WaitQ_p->q_WaitingNext;
		}

		WaitQ_p->q_WaitingNext = q;
	}
}

/*
 * STATIC void
 * dpt_putq(scsi_lu_t *q, sblk_t *sp)
 * Put a job on a logical unit queue in FIFO order.
 *
 * Calling/Exit State:
 * DPT_SCSILU_LOCK(q) for q
 */
STATIC void
dpt_putq(scsi_lu_t *q, sblk_t *sp)
{
	int	cls = QCLASS(sp);
	sblk_t	*nsp;

	ASSERT(q);
	ASSERT(sp);

	if (!q->q_first || (cls <= QCLASS(q->q_last))) {
		/*
		 * if the queue is empty OR the queue class of the new job is
		 * LESS than that of the last on on the queue then insert
		 * onto the END of the queue
		 */
		if (q->q_first) {	/* something on the queue */
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		} else {		/* nothing on the queue */
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;
	} else {
		/*
		 * sort it into the list of jobs on the queue and insert
		 */
		nsp = q->q_first;
		while (QCLASS(nsp) >= cls)
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if (nsp->s_prev)
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}

	q->q_count++;
}

/*
 * void
 * dpt_next(scsi_ha_t *ha, scsi_lu_t *q)
 *
 * Description:
 * Attempt to send the next job on the logical unit queue.
 * All jobs are not sent if the Q is busy.
 *
 * Calling/Exit State: DPT_SCSILU_LOCK(q) for q on entry.
 *  None on exit.
 */

void
dpt_next(scsi_ha_t *ha, scsi_lu_t *q)
{
	ccb_t *cp;
	sblk_t *sp;
	pl_t opri;

	ASSERT(q);

	if ( ha->ha_state & (C_SUSPENDED|C_REMOVED)) {
		DPT_SCSILU_UNLOCK(q);
		return;
	}

	if ((sp = q->q_first) == NULL) {
		DPT_SCSILU_UNLOCK(q);
		return;
	}

	if (sp->sbp->sb.sb_type == SCB_TYPE) {
		if (q->q_flag & DPT_QSUSP) {
			DPT_SCSILU_UNLOCK(q);
			return;
		}
	}

	/*
	 * If we could not allocate a CCB structure and this Queue
	 * currently has no outstanding jobs or is not on the waiting list,
	 * add the Queue to the waiting list.
	 */
	if ((cp = dpt_getccb(ha)) == (ccb_t * )NULL) {

		if ((!q->q_active) && (!(q->q_flag & DPT_QWAIT))) {
			q->q_flag |= DPT_QWAIT;
			DPT_QWAIT_LOCK(ha, ha->ha_QWait_opri);
			dpt_PutWaitingQue(ha, q);
			DPT_QWAIT_UNLOCK(ha, ha->ha_QWait_opri);
		}

		DPT_SCSILU_UNLOCK(q);

		return;
	}

	if ( !(q->q_first = sp->s_next)) {
		q->q_last = NULL;
	}

	q->q_count--;
	q->q_active++;

	DPT_SCSILU_UNLOCK(q);

	if (sp->sbp->sb.sb_type == SFB_TYPE) {
		dpt_func(ha, sp, cp);
	}
	else {
		dpt_cmd(ha, sp, cp);
	}
}

/*
 * void
 * dpt_cmd(scsi_ha_t *ha, sblk_t *sp, register ccb_t *cp)
 *
 * Description:
 * Create and send an SCB associated command.
 *
 * Calling/Exit State:
 * None.
 */

void
dpt_cmd(scsi_ha_t *ha, sblk_t *sp, register ccb_t *cp)
{
	struct scsi_ad *sa;
	scsi_lu_t *q;
	int	i;
	int	b, t, l;
	char	*p;
	int	req_sense_cmd = FALSE;

	sa = &sp->sbp->sb.SCB.sc_dev;

	b = SDI_BUS_32(sa);
	t = SDI_TCN_32(sa);
	l = SDI_LUN_32(sa);

	cp->c_bind = &sp->sbp->sb;
	cp->c_time = (sp->sbp->sb.SCB.sc_time * HZ) / 1000;

	/*
	 * Build the EATA Command Packet structure
	 */
	cp->CP_OpCode	= CP_DMA_CMD;
	cp->CPop.byte	= HA_AUTO_REQ_SEN;
	cp->CPID	= (BYTE)(t);
	cp->CPbus	= (BYTE)(b);
	cp->CPmsg0	= (HA_IDENTIFY_MSG | HA_DISCO_RECO) + l;

	if (sp->s_CPopCtrl) {
		cp->CPop.byte |= sp->s_CPopCtrl;
		sp->s_CPopCtrl = 0;
	}

	if (sp->s_len) {
		if (sp->s_scgth)	/* scatter/gather is used */
			cp->CPop.bit.Scatter = 1; 
		cp->CPdataDMA = dpt_swap32(sp->s_addr);
		cp->CPdataLen = dpt_swap32(sp->s_len);
		if (sp->sbp->sb.SCB.sc_mode & SCB_READ) 
			cp->CPop.bit.DataIn  = 1;
		else 
			cp->CPop.bit.DataOut = 1;
	}
	else {
		cp->CPdataLen = 0;
	}

	q = LU_Q(ha, b, t, l);

	/*
	 * If a Request Sense command and ReqSen Data cached then copy to
	 *   data buffer and return.
	 */

	p = sp->sbp->sb.SCB.sc_cmdpt;

	DPT_SCSILU_LOCK(q);
	if (q->q_flag & DPT_QSENSE) {
		req_sense_cmd = TRUE;
		q->q_flag &= ~DPT_QSENSE;
	}
	DPT_SCSILU_UNLOCK(q);
			
	if ( req_sense_cmd && (*p == SS_REQSEN)) {

		/*
		 * copy request sense data 
		 */
		bcopy(SENSE_AD(&q->q_sense), sp->sbp->sb.SCB.sc_datapt, SENSE_SZ);
		cp->CP_status.SP_Controller_Status = S_GOOD;

		dpt_done(ha, cp, HA_ST_SEEK_COMP | HA_ST_READY, 0);

		return;
	}

	for (i = 0; i < sp->sbp->sb.SCB.sc_cmdsz; i++) {
		cp->CPcdb[i] = *p++;	/* Copy SCB cdb to CP cdb. */
	}

#ifdef DPT_TIMEOUT_RESET_SUPPORT
	/*
	 * Are Job TimeOuts Enabled?
	 */
	if( ha->ha_flags & DPT_TIMEOUT_ON ) {
#ifdef DPT_TIMEOUT_RESET_DEBUG
		if(sp->sbp->sb.SCB.sc_time || dpt_force_time )
#else
		if(sp->sbp->sb.SCB.sc_time)
#endif
			dpt_timeout_insert(ha, sp, cp);
	}
#endif /** DPT_TIMEOUT_RESET_SUPPORT **/

	dpt_send(ha, cp, dptintr_done);
}

/*
 * void
 * dpt_func(scsi_ha_t *ha, sblk_t *sp, register ccb_t *cp)
 *
 * Description:
 * Create and send an SFB associated command.
 *
 * Calling/Exit State:
 * None.
 */

void
dpt_func(scsi_ha_t *ha, sblk_t *sp, register ccb_t *cp)
{
	int	c, b, t, l;
	struct scsi_ad *sa;
	struct sdi_edt *edtp;
	pl_t opri;

	/*
	 * Only SFB_ABORTM and SFB_RESETM messages get here.
	 */

	sa = &sp->sbp->sb.SFB.sf_dev;

	c = SDI_HAN_32(sa);
	b = SDI_BUS_32(sa);
	t = SDI_TCN_32(sa);
	l = SDI_LUN_32(sa);

	cp->c_bind = &sp->sbp->sb;
	cp->CPID   = (BYTE)t;
	cp->CPbus  = (BYTE)b;
	cp->CPmsg0 = l;
	cp->c_time = 0;

#ifdef DPT_TIMEOUT_RESET_SUPPORT
	switch ( sp->sbp->sb.SFB.sf_func ) {

	case SFB_RESET_BUS:

		cmn_err(CE_NOTE,"DPT: Restting SCSI Busi %d", b);
		dpt_reset_bus(ha, b);
		break;

	case SFB_RESET_DEVICE:

		cmn_err(CE_NOTE,"DPT: Resetting Bus/Target/Lun %d/%d/%d",b,t,l);
		dpt_reset_device(ha, b, t, l );
		break;

	default:

		cmn_err(CE_NOTE, "!DPT: Unexpected Function - dpt_func");
		break;
	}
#else
	if ( (edtp = sdi_rxedt(c, cp->CPbus, cp->CPID, cp->CPmsg0)) != 
			(struct sdi_edt *)0 && edtp->pdtype != ID_TAPE ) {
		/*
		 *+ DPT SFB_ABORTM/SFB_RESETM command received,
		 *+ SCSI bus reset command being issued.
		 */
		cmn_err(CE_WARN, "!%s: Bus is being reset", ha->ha_name);
		DPT_STPKT_LOCK(ha, opri);
		outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(1000);
		DPT_STPKT_UNLOCK(ha, opri);
	}
#endif

	cp->CP_status.SP_Controller_Status = 0;
	cp->CP_status.SP_SCSI_Status       = 0;

	dpt_done(ha, cp, 0, 0);
}

STATIC int
dpt_send_wait(scsi_ha_t *ha, ccb_t *cp, int timeout)
{

	ha->ha_waitflag = TRUE;
	ha->ha_cmd_in_progress = cp->CPcdb[0];

	(void)dpt_send(ha, cp, dptintr_blocking);

	if (dpt_wait(ha, timeout, DPT_INTR_ON) == FAILURE) {
		return FAILURE;
	}

	return SUCCESS;
}

/*
 * STATIC int
 * _dpt_send(scsi_ha_t *ha, ccb_t *cp, ccb_callback callback)
 *
 * Description:
 *	Send a command to the host adapter board.
 *
 * Calling/Exit State:
 * None.
 */

STATIC int
_dpt_send(scsi_ha_t *ha , ccb_t *cp, ccb_callback callback)
{
	pl_t opri;
	int	i;

	/*
	 * If the Reserve/Release flag is set, check for a reserve or
	 * release command. If found change it to a test unit ready
	 */
	if(dpt_reserve_release){

		if((cp->CPcdb[0] == SS_RESERV)||(cp->CPcdb[0] == SS_RELES)) {
			for(i = 0; i < 6; ++i) {
				cp->CPcdb[0] = 0;
			}
		}
	}

	cp->c_active = TRUE; /* for debugging only */

	/*
	 * Here we enter to the Hardware Layer from the Software Layer
	 * and must save the Software Layer VCP and replace it with the
	 * Hardware Layer VCP. The interrupt routine will restore the
	 * Software Layer VCP before making any callbacks.
	 */
	cp->c_vp = cp->CPaddr.vp;	/* Save double virtual pointer   */
	cp->CPaddr.vp = cp;		/* Make sure we point to the CCB */
	cp->c_callback = callback;

	DPT_STPKT_LOCK(ha, opri);
	ha->ha_npend++;  /* Increment number pending on ctlr */

	dpt_send_cmd(ha, cp->c_addr, CP_DMA_CMD);

	DPT_STPKT_UNLOCK(ha, opri);

	return (COMMAND_PENDING);
}

/*
 * ccb_t *
 * dpt_getccb(scsi_ha_t *ha)
 *
 * Description:
 *	Allocate a controller command block structure.
 *
 * Calling/Exit State:
 * None.
 */

ccb_t *
dpt_getccb(scsi_ha_t *ha)
{
	register ccb_t *cp;
	pl_t opri;

	DPT_CCB_LOCK(ha, opri);

	if (ha->ha_active_jobs >= ha->ha_max_jobs )  {
		DPT_CCB_UNLOCK(ha, opri);
		return((ccb_t * )NULL);
	}

	cp = ha->ha_cblist;

	ha->ha_cblist = cp->c_next;

	++ha->ha_active_jobs;

	DPT_CCB_UNLOCK(ha, opri);

	return (cp);
}

/*
 * void
 * dpt_freeccb(scsi_ha_t *ha, ccb_t *cp)
 *
 * Description:
 *	Release a previously allocated command block.
 *
 * Calling/Exit State:
 * None.
 */

void
dpt_freeccb(scsi_ha_t *ha, ccb_t *cp)
{
	pl_t opri;

	/*
	 * cp not on free list yet, so do this before lock
	 */
	cp->c_bind = NULL;

	DPT_CCB_LOCK(ha, opri);

	cp->c_next = ha->ha_cblist;
	ha->ha_cblist = cp;
	--ha->ha_active_jobs;

	/*
	 * if there were no CCBs left, there might be someone in the EATA
	 * pass-through waiting for a CCB so issue a wake up call
	 */
	if (cp->c_next == NULL) {
		DPT_CCB_UNLOCK(ha, opri);
		DPT_EATA_LOCK(ha, opri);
		SV_BROADCAST(ha->ha_eata_sv, 0);
		DPT_EATA_UNLOCK(ha, opri);
	}
	else  {
		DPT_CCB_UNLOCK(ha, opri);
	}
}

/*
 * int
 * dpt_wait(scsi_ha_t * ha, int time, intr)
 *
 * Description:
 * Poll for a completion from the host adapter.  If an interrupt
 * is seen, the HA's interrupt service routine is manually called.
 * The intr arguments specifies whether interrupts are inabled
 * or disabled when this routine is called:
 *  intr == 0 Interrupts disabled
 *   1 Interrupts enabled
 *  NOTE:
 * This routine allows for no concurrency and as such, should
 * be used selectively.
 *
 * Calling/Exit State:
 * None.
 */
int
dpt_wait(scsi_ha_t *ha, int time, int intr)
{
	int	ret = FAILURE;

	while (time > 0) {

		if (intr == DPT_INTR_OFF) {

			if (ha->ha_waitflag == FALSE) {
				/*
				 * Controller has generated an interrupt to
				 * acknowledge completion of command, and
				 * interrupt service routine has serviced it.
				 */
				ret = SUCCESS;
				break;
			}

			if (inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_INTR) {
				dptintr(ha);
				/* waitflag should be checked again here */
				ret = SUCCESS;
				break;
			}
		}
		else {
			if (ha->ha_waitflag == FALSE) {
				ret = SUCCESS;
				break;
			}
		}

		drv_usecwait(1000);
		time--;
	}

	return (ret);
}

/*
 * STATIC int
 * dpt_init_cps(scsi_ha_t *ha)
 *
 * Description:
 *	Initialize the controller CP free list.
 *
 * Calling/Exit State:
 * None.
 */
STATIC int
dpt_init_cps(scsi_ha_t *ha)
{
	ccb_t *ccb_p, *ccb_pa_p;
	paddr_t ccb_pa, sc_pa;
	int	i;

	/*
	 * allocate space for status packet
	 * note: kmem_alloc_phys can never fail (it uses KM_SLEEP)
	 */
	ha->sc_hastat = (scsi_stat_t *)
		kmem_alloc_phys(sizeof(scsi_stat_t), ha->ha_physreq, &sc_pa, 0);
	bzero(ha->sc_hastat, sizeof(scsi_stat_t));

	/*
	 * allocate and initialize ccb's
	 */

	ha->ha_max_jobs = dpt_max_jobs;
	ha->ha_cblist = NULL;

	for (i = 0; i < dpt_max_jobs; i++) {
		ccb_p = (ccb_t *)kmem_alloc_phys(sizeof(ccb_t), ha->ha_physreq,
								&ccb_pa, 0);
		bzero(ccb_p, sizeof(ccb_t));
		ccb_pa_p = (ccb_t *)ccb_pa; /* physical address */

		/* Save phys addresses into CP in  68000 format */
		ccb_p->c_addr    = ccb_pa;
		ccb_p->CPstatDMA = dpt_swap32(sc_pa); /* status packet */
		ccb_p->CP_ReqDMA = dpt_swap32((long)
					SENSE_AD(&ccb_pa_p->CP_sense));
		ccb_p->ReqLen    = SENSE_SZ;
		ccb_p->CPaddr.vp = ccb_p; /* Save CP virtual address pointer */

		ccb_p->c_next = ha->ha_cblist;
		ha->ha_cblist = ccb_p;
	}

	return 0;
}

/*
 * int
 * dpt_illegal(int hba, int bus, int scsi_id, int lun, int m)
 *
 * Description:
 *
 * Calling/Exit State:
 *  None.
 */
/*ARGSUSED*/
int
dpt_illegal(int hba, int bus, int scsi_id, int lun, int m)
{
	if (sdi_rxedt(hba, bus, scsi_id, lun)) {
		return 0;
	}
	else {
		return 1;
	}
}

/*
 * STATIC void
 * dpt_lockinit(scsi_ha_t *ha)
 *
 * Description:  Initialize dpt locks:
 *  1) device queue locks (one per queue),
 *  2) ccb lock (one only),
 *  3) Command Status Packet lock (one per controller).
 *
 * Calling/Exit State:
 * None.
 */
STATIC void
dpt_lockinit( scsi_ha_t *ha )
{

	int sleepflag = KM_SLEEP;

	ha->ha_pause_sv = SV_ALLOC(sleepflag);
	ha->ha_eata_sv = SV_ALLOC(sleepflag);
	ha->ha_eata_lock = LOCK_ALLOC(DPT_HIER, pldisk, &dpt_lkinfo_eata, sleepflag);
	ha->ha_StPkt_lock = LOCK_ALLOC(DPT_HIER, pldisk, &dpt_lkinfo_StPkt, sleepflag);
	ha->ha_QWait_lock = LOCK_ALLOC(DPT_HIER + 1, pldisk, &dpt_lkinfo_QWait, sleepflag);
	ha->ha_ccb_lock = LOCK_ALLOC(DPT_HIER + 1, pldisk, &dpt_lkinfo_ccb, sleepflag);
#ifdef DPT_TIMEOUT_RESET_SUPPORT
	ha->ha_watchdog_lock = LOCK_ALLOC (DPT_HIER, pldisk, &dpt_lkinfo_watchdog, sleepflag);
#endif /* DPT_TIMEOUT_RESET_SUPPORT */

}

/*
 * STATIC void
 * dpt_lockclean(scsi_ha_t *ha)
 *
 * Removes unneeded locks.  Controllers that are not active will
 * have all locks removed.  Active controllers will have locks for
 * all non-existant devices removed.
 *
 * Calling/Exit State:
 * None.
 */

STATIC void
dpt_lockclean(scsi_ha_t *ha)
{
	if (ha->ha_pause_sv)
		SV_DEALLOC(ha->ha_pause_sv);
	if (ha->ha_eata_sv)
		SV_DEALLOC (ha->ha_eata_sv);
	if (ha->ha_eata_lock)
		LOCK_DEALLOC (ha->ha_eata_lock);
	if (ha->ha_StPkt_lock)
		LOCK_DEALLOC (ha->ha_StPkt_lock);
	if (ha->ha_QWait_lock)
		LOCK_DEALLOC (ha->ha_QWait_lock);
	if (ha->ha_ccb_lock)
		LOCK_DEALLOC (ha->ha_ccb_lock);
}

/*
 * STATIC dpt_scsi_lu *
 * dpt_qalloc(void)
 *
 * Description:  Allocates a logical device queue structure and lock
 *
 * Calling/Exit State:
 * None.
 */
STATIC scsi_lu_t *
dpt_qalloc(void)
{
	scsi_lu_t *q;

	q = (scsi_lu_t * )kmem_zalloc(sizeof(scsi_lu_t), KM_SLEEP);
	q->q_lock = LOCK_ALLOC(DPT_HIER, pldisk, &dpt_lkinfo_q, KM_SLEEP);

	return(q);
}

/*
 * STATIC void
 * dpt_qfree(register scsi_lu_t *q)
 *
 * Description:  frees a logical device queue structure and lock
 *
 * Calling/Exit State:
 * None.
 */
STATIC void
dpt_qfree(register scsi_lu_t *q)
{
	LOCK_DEALLOC (q->q_lock);
	kmem_free(q, sizeof(scsi_lu_t));
}

STATIC void
dpt_pause_func(dpt_pause_func_arg_t *arg)
{
	scsi_ha_t *ha = arg->ha;
	scsi_lu_t *q = arg->q;

	DPT_SCSILU_LOCK(q);

	SV_BROADCAST(ha->ha_pause_sv, 0);

	DPT_SCSILU_UNLOCK(q);
}



/*
 * STATIC void
 * dpt_PauseBus(scsi_ha_t *ha, int b)
 *
 * Description: Clear Out All IO For The Specified Bus
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
void
dpt_PauseBus(scsi_ha_t *ha, int b)
{
	int	c,t,l;
	scsi_lu_t * q;
					

	/*
	 * Loop through all devices on this HBA bus and wait until all jobs
	 * currently sent to the device have completed
	 */
	c = ha->ha_cntlr;
	for ( t = 0; t < ha->ha_ntargets; t++) 
		for ( l = 0; l <= ha->ha_nluns; l++) {
			q = LU_Q(ha, b, t, l);
			if (q->q_flag & DPT_QHBA)
				continue;
			ASSERT(q);
			DPT_SCSILU_LOCK(q);
			q->q_flag |= DPT_QSUSP;
			DPT_SCSILU_UNLOCK(q);

			dpt_waitq(ha, q);
		}
}

STATIC void
dpt_waitq(scsi_ha_t *ha, scsi_lu_t *q)
{
	dpt_pause_func_arg_t arg;

	/*
	 * loop until all outstanding commands
	 * have completed
	 */

	arg.ha = ha;
	arg.q = q;
	/*CONSTANTCONDITION*/
	while (1) {

		DPT_SCSILU_LOCK(q);
		if (q->q_active <= 0) {
			DPT_SCSILU_UNLOCK(q);
			break;
		}

		/*
		 * sleep for .1 seconds to allow the 
		 * jobs to complete
		 */
		itimeout(dpt_pause_func, &arg, drv_usectohz(100000), pldisk);
		SV_WAIT(ha->ha_pause_sv, pridisk, q->q_lock);
	}
}

/*
 * STATIC void
 * dpt_ResumeBus(scsi_ha_t *ha, int b)
 *
 * Description: Resume All Waiting IO For The Specified Bus
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC void
dpt_ResumeBus(scsi_ha_t *ha, int b)
{
	int	c,t,l;
	scsi_lu_t * q;


	c = ha->ha_cntlr;
	for (t = 0; t < ha->ha_ntargets; t++) {
		for (l = 0; l <= ha->ha_nluns; l++) {
			q = LU_Q(ha,b,t,l);
			if (q->q_flag & DPT_QHBA)
				continue;
			DPT_SCSILU_LOCK(q);
			q->q_flag &= ~DPT_QSUSP;
			dpt_next(ha, q );
		}
	}
}

#ifdef DPT_TIMEOUT_RESET_SUPPORT
STATIC void
dpt_timeout_insert(scsi_ha_t *ha, sblk_t *srb_ptr, ccb_t *cp)
{
	int 	quantum;
	int 	total_quantum;
	pl_t	oip;
	sblk_t	*job_ptr;

	/*
	 * The quantum must be rounded up + 1.  As an example, consider a
	 * 1 second timeout. If the quantum is 1, it will expire in the
	 * next watchdog call, which could be in 1 microsecond.
	 */
#ifdef DPT_TIMEOUT_RESET_DEBUG
	if (dpt_force_time) {
		quantum = (dpt_force_time + DPT_TIME_QUANTUM - 1) / DPT_TIME_QUANTUM;
	}
	else
#endif /* DPT_TIMEOUT_RESET_DEBUG */
		quantum = (srb_ptr->sbp->sb.SCB.sc_time + DPT_TIME_QUANTUM - 1) / DPT_TIME_QUANTUM;

	quantum++;

	if (srb_ptr->s_flags & SB_CALLED_BACK) {

		/*
		 * This is a retry.  The assumption is that the target driver
		 * is retrying a job that had been aborted, but that DPT never
		 * saw a timeout for the abort.
		 *
		 * The problem here is whether to free the scb or not, since
		 * the cp may still be active. For example, this can be a retry
		 * due to a device reset, whereas the cp may come back on a bus
		 * reset.  The safe thing to do is to not free the scb, but that
		 * will lead to a hang when we eventually run out of cp's.
		 */
		dpt_timeout_remove(ha, &srb_ptr->sbp->sb);
	}

	srb_ptr->s_flags = 0;

	DPT_WATCHDOG_LOCK(ha, oip);

	srb_ptr->s_cp = cp;

	/*
	 * Add the job to software timeout watchdog list.
	 */
	if( ha->ha_actv_jobs == (sblk_t *)NULL ) {
		/*
		 * First job being watched, just add it.
		 */
		ha->ha_actv_last = ha->ha_actv_jobs = srb_ptr;
		srb_ptr->s_job_prev = srb_ptr->s_job_next = NULL;
		srb_ptr->s_time_quantum = quantum;
		ha->ha_quantum = quantum;
		DPT_WATCHDOG_UNLOCK(ha, oip);
		return;
	}

	total_quantum = ha->ha_quantum;

	if (quantum >= total_quantum) {
		/*
		 * This job expires after all jobs on the queue.
		 */
		srb_ptr->s_time_quantum = quantum - total_quantum;
		ha->ha_quantum = quantum;
		ha->ha_actv_last->s_job_next = srb_ptr;
		srb_ptr->s_job_prev = ha->ha_actv_last;
		srb_ptr->s_job_next = NULL;
		ha->ha_actv_last = srb_ptr;
		DPT_WATCHDOG_UNLOCK(ha, oip);
		return;
	}

	quantum -= total_quantum;
	job_ptr = ha->ha_actv_last;

	while( job_ptr && quantum < 0) {
		quantum += job_ptr->s_time_quantum;
		job_ptr = job_ptr->s_job_prev;
	}

	srb_ptr->s_time_quantum = quantum;

	/*
	 * Insert after job_ptr
	 */
	if (!job_ptr) {
		ha->ha_actv_jobs->s_time_quantum -= quantum;
		ha->ha_actv_jobs->s_job_prev = srb_ptr;
		srb_ptr->s_job_next = ha->ha_actv_jobs;
		srb_ptr->s_job_prev = NULL;
		ha->ha_actv_jobs = srb_ptr;
	}
	else {
		job_ptr->s_job_next->s_time_quantum -= quantum;
		srb_ptr->s_job_next = job_ptr->s_job_next;
		srb_ptr->s_job_prev = job_ptr;
		job_ptr->s_job_next->s_job_prev = srb_ptr;
		job_ptr->s_job_next = srb_ptr;
	}

#ifdef DPT_TIMEOUT_RESET_DEBUG
	if( dpt_trace_list ) {
		cmn_err(CE_CONT,"DPT_TIMEOUT_INSERT COMPLETE\n");
		dpt_active_jobs(ha);
	}
#endif /* DPT_TIMEOUT_RESET_DEBUG */

	DPT_WATCHDOG_UNLOCK(ha, oip);
}

STATIC sblk_t *
dpt_timeout_remove(scsi_ha_t *ha, struct sb *sp)
{
	sblk_t		*srb_ptr;
	pl_t			oip;

	DPT_WATCHDOG_LOCK(ha, oip);

	for (srb_ptr=ha->ha_actv_jobs; srb_ptr; srb_ptr=srb_ptr->s_job_next)
		if (&srb_ptr->sbp->sb == sp)
			break;

	if (!srb_ptr) {
		DPT_WATCHDOG_UNLOCK(ha, oip);
		return NULL;
	}

	if (srb_ptr->s_job_prev && srb_ptr->s_job_next) {
		srb_ptr->s_job_next->s_time_quantum += srb_ptr->s_time_quantum;
		srb_ptr->s_job_next->s_job_prev = srb_ptr->s_job_prev;
		srb_ptr->s_job_prev->s_job_next = srb_ptr->s_job_next;
	}
	else {
		if (!srb_ptr->s_job_prev) {
			ha->ha_actv_jobs = srb_ptr->s_job_next;
			if (ha->ha_actv_jobs) {
			ha->ha_actv_jobs->s_job_prev = NULL;
			ha->ha_actv_jobs->s_time_quantum += srb_ptr->s_time_quantum;
		}
	}

	if (!srb_ptr->s_job_next) {
		ha->ha_actv_last = srb_ptr->s_job_prev;
		if (ha->ha_actv_last)
			ha->ha_actv_last->s_job_next = NULL;
			ha->ha_quantum -= srb_ptr->s_time_quantum;
		}
	}

	srb_ptr->s_job_next = srb_ptr->s_job_prev = NULL;

#ifdef DPT_TIMEOUT_RESET_DEBUG
	if( dpt_trace_list ) {
		dpt_active_jobs(ha);
	}
#endif /* DPT_TIMEOUT_RESET_DEBUG */

	DPT_WATCHDOG_UNLOCK(ha, oip);

	return srb_ptr;
}

/*ARGSUSED*/
void
dpt_watchdog( scsi_ha_t *ha )
{
	int		bus;
	pl_t		oip;
	sblk_t	*srb_ptr;
	sblk_t	*sb;
	struct sb	*sspp;

	if (ha == CNFNULL) {
		return;
	}

	/*
	 * Are software timeouts enabled?
	 */
	if (!(ha->ha_flags & DPT_TIMEOUT_ON)) {
		return;
	}

	DPT_WATCHDOG_LOCK(ha, oip);

	srb_ptr = ha->ha_actv_jobs;

	if( !srb_ptr ) {
		DPT_WATCHDOG_UNLOCK(ha, oip);
		return;
	}

	/* Mark all jobs that must be processed.
	 * Jobs can conceivably come into the list
	 * while processing is taking place when
	 * we drop the watchdog lock.  These new
	 * jobs should not be processed until the
	 * next watchdog invocation.
	 */
	while (srb_ptr) {
		srb_ptr->s_flags |= SB_PROCESS;
		srb_ptr = srb_ptr->s_job_next;
	}
	srb_ptr = ha->ha_actv_jobs;

	while (srb_ptr && srb_ptr->s_time_quantum == 0) {

		srb_ptr->s_flags &= ~SB_PROCESS;

		if( !(srb_ptr->s_flags & SB_BEING_ABORTED) )  {
#ifdef DPT_TIMEOUT_RESET_DEBUG
			/*
			 * First time processed.
			 */
			cmn_err(CE_CONT, "DPT_WATCHDOG: (sb:0x%x) SB_BEING_ABORTED Not Set\n", &srb_ptr->sbp->sb);
#else
			/* NULL BODY */
#endif /* DPT_TIMEOUT_RESET_DEBUG */
		}
		else if (srb_ptr->s_flags & SB_CALLED_BACK) {
			cmn_err(CE_CONT, "DPT_WATCHDOG: (sb:0x%x) SB_CALLED_BACKED Set\n", &srb_ptr->sbp->sb);
			cmn_err(CE_CONT, "              0x%x, 0x%x\n", srb_ptr->s_cp->CP_status.SP_Controller_Status,srb_ptr->s_cp->CP_status.SP_SCSI_Status);
		}
		else {

#ifdef DPT_TIMEOUT_RESET_DEBUG
			cmn_err(CE_CONT, "DPT_WATCHDOG: (sb:0x%x) Abort Request Timed Out\n", &srb_ptr->sbp->sb);
#endif /* DPT_TIMEOUT_RESET_DEBUG */
			/*
			 * This job timed-out previously, and a Job Abort
			 * was issued. Apparently no interrupt arrived
			 * for job completion, so let's call the target
			 * driver interrupt handler now.
			 */

			srb_ptr->s_flags |= SB_CALLED_BACK;

			DPT_WATCHDOG_UNLOCK(ha, oip);

			srb_ptr->sbp->sb.SCB.sc_comp_code = SDI_TIME_NOABORT;
			sdi_callback( &srb_ptr->sbp->sb );

			DPT_WATCHDOG_LOCK(ha, oip);

			/* We must start our search again because
			 * the lock was dropped, possibly resulting
			 * in changes to the list.
			 */
			srb_ptr = ha->ha_actv_jobs;
		}

		/*
		 * Search again for job to process.  We do not need to start
		 * from the head unless we dropped the lock above. Find a job
		 * that needs to be processed or that has a non-zero timeout.
		 */
		while (srb_ptr) {
			if (srb_ptr->s_flags & SB_PROCESS)
				break;
			if (srb_ptr->s_time_quantum)
				break;
			srb_ptr = srb_ptr->s_job_next;
		}
	}

	/*
	 * ha->ha_quantum and srb->s_time_quantum must
	 * be decremented in tandem in order for the queue
	 * to remain consistent.
	 */

	if (ha->ha_quantum) {
		ha->ha_quantum--;
	}

	/*
	 * srb_ptr is pointing at the first job,
	 * with or without SB_PROCESS, with a
	 * non-zero s_time_quantum.
	 */

	if (srb_ptr) {
		srb_ptr->s_time_quantum--;
		if (!(srb_ptr->s_flags & SB_PROCESS)) {
			DPT_WATCHDOG_UNLOCK(ha, oip);
			return;
		}
	}

	while (srb_ptr && srb_ptr->s_time_quantum == 0) {

		struct sb *sbp = &srb_ptr->sbp->sb;
		int hastatus = srb_ptr->s_cp->CP_status.SP_Controller_Status;

		srb_ptr->s_flags &= ~SB_PROCESS;

		/*
		 * The associated job has exceeded maximum execution
		 * time. Attempt to abort the job.
		 */
		srb_ptr->s_flags |= SB_BEING_ABORTED;

		/*
		 * It is possible that a previous job on the list
		 * initiated a device reset, which would result
		 * in other jobs being aborted already.
		 */
		cmn_err(CE_CONT, "DPT_WATCHDOG: (sb:0x%x) Status:\n", &srb_ptr->sbp->sb);
		cmn_err(CE_CONT, "              0x%x, 0x%x\n", srb_ptr->s_cp->CP_status.SP_Controller_Status,srb_ptr->s_cp->CP_status.SP_SCSI_Status);

		if (hastatus != HA_ERR_CPABRTNA || 
		    hastatus != HA_ERR_CPABORTED) {

#ifdef DPT_TIMEOUT_RESET_DEBUG
			cmn_err( CE_WARN, "DPT_WATCHDOG: Aborting Job (sb:0x%x)\n", sbp);
			dpt_breakpoint();
#endif /* DPT_TIMEOUT_RESET_DEBUG */

			DPT_WATCHDOG_UNLOCK(ha, oip);

			dpt_abort_cmd(ha, srb_ptr->s_cp->c_addr);

			DPT_WATCHDOG_LOCK(ha, oip);

		}
#ifdef DPT_TIMEOUT_RESET_DEBUG
		else {
			cmn_err(CE_CONT, "DPT_WATCHDOG: (sb:0x%x) Job not being Aborted\n", &srb_ptr->sbp->sb);
		}
#endif /* DPT_TIMEOUT_RESET_DEBUG */

		/*
		 * Search again for job to process. We do not need to start
		 * from the head unless we dropped the lock above.  Find a job
		 * that needs to be processed or that has a non-zero timeout.
		 */
		while (srb_ptr) {
			if (srb_ptr->s_flags & SB_PROCESS)
				break;
			if (srb_ptr->s_time_quantum)
				break;
			srb_ptr = srb_ptr->s_job_next;
		}
	}
	DPT_WATCHDOG_UNLOCK(ha, oip);
}

#ifdef DPT_TIMEOUT_RESET_DEBUG
void
dpt_active_jobs(scsi_ha_t *ha)
{
	sblk_t	*srb_ptr;
	sblk_t	*prev = NULL;
	int total;
	int so_far = 0;
	int panic = 0;
	int print = 1;
	int line = 1;
	int c = ha->ha_cntlr;

	srb_ptr = ha->ha_actv_jobs;
	total = ha->ha_quantum;


	if (ha->ha_actv_jobs && ha->ha_actv_jobs->s_job_prev) {
		cmn_err(CE_WARN, "DPT(%d): List previous not null.", c);
	}
	if (ha->ha_actv_last && ha->ha_actv_last->s_job_next) {
		cmn_err(CE_WARN, "DPT(%d): Last next not null.", c);
	}

	if (srb_ptr) {
		cmn_err(CE_NOTE, "DPT(%d): Total Quantum %d\n", c, ha->ha_quantum);
	}

	if (!srb_ptr && ha->ha_actv_last) {
		cmn_err(CE_WARN, "DPT(%d): First Null, Last not.", c);
	}

	while (srb_ptr) {

		ccb_t *cp = srb_ptr->s_cp;

		cmn_err(CE_CONT, "****\n");
		cmn_err(CE_CONT, "C_TIME = 0x%x\n",cp->c_time);
		cmn_err(CE_CONT, "CP_status.SP_Controller_Status = 0x%x\n",
			cp->CP_status.SP_Controller_Status);
		cmn_err(CE_CONT, "CP_status.SP_SCSI_Status = 0x%x\n",
			cp->CP_status.SP_SCSI_Status);
		cmn_err(CE_CONT, "S_FLAGS = 0x%x\n",srb_ptr->s_flags);
		cmn_err(CE_CONT, "S_TIME_QUANTUM = 0x%x\n",
			srb_ptr->s_time_quantum);

		if (srb_ptr->s_flags & SB_MARK) {
			cmn_err(CE_CONT, "Circular List! STOP");
			break;
		}

		if (srb_ptr->s_job_prev != prev) {
			cmn_err(CE_CONT, "Incorrect prev pointer.\n");
		}

		if (srb_ptr->s_time_quantum > 0x7fff) {
			cmn_err(CE_CONT, "Negative Quantum on list!\n");
		}

		so_far += srb_ptr->s_time_quantum;
		if (so_far > total) {
			cmn_err(CE_CONT, "List Time Values Inconsistent\n");
		}

		srb_ptr->s_flags |= SB_MARK;

		prev = srb_ptr;

		if (!srb_ptr->s_job_next && srb_ptr != ha->ha_actv_last) {
			cmn_err(CE_CONT, "ha_actv_last wrong!\n");
		}

		srb_ptr = srb_ptr->s_job_next;
	}

	if (so_far != total) {
		cmn_err(CE_CONT, "List Total Time Value Invalid\n");
	}

	srb_ptr = ha->ha_actv_jobs;
	while (srb_ptr ) {
		srb_ptr->s_flags &= ~SB_MARK;
		srb_ptr = srb_ptr->s_job_next;
	}
}

dpt_dump_ha(scsi_ha_t *ha)
{
	cmn_err(CE_CONT,"BASE:0x%x, IRQ:0x%x\n",
		ha->ha_base, ha->ha_vect);
	cmn_err(CE_CONT,"NPEND:0x%x, ACTIVE_JOBS:0x%x\n",
		ha->ha_npend, ha->ha_active_jobs);
	cmn_err(CE_CONT,"HA_CBLIST:0x%x\n",
		ha->ha_cblist);
	cmn_err(CE_CONT,"HA_FLAGS:0x%x, HA_QUANTUM:0x%x\n",
		ha->ha_flags, ha->ha_quantum);
	cmn_err(CE_CONT,"HA_ACTV_JOBS:0x%x, HA_ACTV_LAST:0x%x\n",
		ha->ha_actv_jobs, ha->ha_actv_last);
}

dpt_dump_srb( srb_ptr )
	sblk_t	*srb_ptr;
{
	cmn_err(CE_CONT,"SBP:0x%x, S_TIME_QUANTUM:0x%x\n",
		srb_ptr->sbp, srb_ptr->s_time_quantum);
	cmn_err(CE_CONT,"S_NEXT:0x%x, S_PREV:0x%x\n",
		srb_ptr->s_next, srb_ptr->s_prev);
	cmn_err(CE_CONT,"S_JOB_NEXT:0x%x, S_JOB_PREV:0x%x\n",
		srb_ptr->s_job_next, srb_ptr->s_job_prev);
}

dpt_breakpoint()
{
	return;
}
#endif /* DPT_TIMEOUT_RESET_DEBUG */

#endif /* DPT_TIMEOUT_RESET_SUPPORT */

/************* hardware specific modules *********************/

/*
 * void
 * dpt_board_reset(int base)
 *
 * Description:
 *	Reset the HA board.
 *
 * Calling/Exit State:
 * None.
 */
void
dpt_board_reset(int base)
{
	if (inb(base + HA_STATUS) != (HA_ST_SEEK_COMP | HA_ST_READY)) {
		outb((base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(4000000);  /* 4 full second wait */
	}
}
/*
 * STATIC int
 * EATA_ReadConfig(int port, struct RdConfig *eata_cfg)
 *
 * Issue an EATA Read Config Command, Process PIO.
 *
 * Calling/Exit State:
 * None.
 */
STATIC int
EATA_ReadConfig(int port, struct RdConfig *eata_cfg)
{
	ulong  loop = 50000L;

	/*
	 * Wait for controller not busy
	 */
	while ((inb(port + HA_STATUS) & HA_ST_BUSY) && loop--) {
		drv_usecwait(1);
	}
	if ( loop == 0 )
		return(0);

	/*
	 * Send the Read Config EATA PIO Command
	 */
	outb(port + HA_COMMAND, CP_READ_CFG_PIO);

	/*
	 * Wait for DRQ Interrupt
	 */
	loop   = 50000L;
	while ( (inb(port + HA_STATUS) != ( HA_ST_DRQ + HA_ST_SEEK_COMP + HA_ST_READY)) && loop--) {
		drv_usecwait(1);
	}

	if ( loop == 0 ) {
		/*
		 * Timed Out Waiting For DRQ
		 */
		return(0);
	}

	/*
	 * Take the Config Data
	 */
	repinsw(port + HA_DATA, (ushort_t * )eata_cfg, 512 / 2 );

	if ( inb(port + HA_STATUS) & HA_ST_ERROR ) 
		return(0);

	/*
	 * Verify that it is an EATA Controller
	 */
	if (strncmp((char *)eata_cfg->EATAsignature, "EATA", 4))	
		return(0);

	return(1);
}

/*
 * void
 * dpt_SendEATACmd(scsi_ha_t *ha, int bus, int cmd)
 *
 * Description:
 *	For EATA Pass-Through, Send Off The Command And Wait
 *	For It To Complete
 *
 * Calling/Exit State:
 * None.
 */

/*ARGSUSED*/
STATIC void
dpt_SendEATACmd(scsi_ha_t *ha, int bus, int cmd)
{
	pl_t opri;
	ccb_t *cp;

	/*
	 * If The Firmware Supports The Bus Quiet Command
	 * Correctly Send It Down
	 */
	if ((ha->ha_fw_version[0] < '0') && 
	    (ha->ha_fw_version[1] < '6')) {
		return;
	}

	/* Allocate A CCB From The HA's Freelist  */
	DPT_EATA_LOCK(ha, opri);

	while ((cp = dpt_getccb(ha)) == (ccb_t * )NULL) {
		SV_WAIT(ha->ha_eata_sv, pridisk, ha->ha_eata_lock);
		DPT_EATA_LOCK(ha, opri);
	}

	DPT_EATA_UNLOCK(ha, opri);

	cp->CPop.byte = HA_AUTO_REQ_SEN;
	cp->CPop.bit.Interpret = 1;
	cp->CPID = 0;
	cp->CPbus = (BYTE)(bus);
	cp->CPmsg0 = (HA_IDENTIFY_MSG | HA_DISCO_RECO) + 0;
	cp->CPdataLen = 0;
	cp->CPcdb[0] = MULTIFUNCTION_CMD;
	cp->CPcdb[1] = 0;
	cp->CPcdb[2] = cmd;
	cp->CPcdb[3] = 0;
	cp->CPcdb[4] = 0;
	cp->CPcdb[5] = 0;

	cp->CP_OpCode = 0;
	cp->c_bind = NULL;

	(void)dpt_send_wait(ha, cp, 10000);

	dpt_freeccb(ha, cp);
}

STATIC void
dpt_init_cp(ccb_t *cp)
{
	cp->CP_OpCode	= CP_DMA_CMD;
	cp->c_time	= 0;
	cp->CPop.byte	= 0;
	cp->CPop.bit.DataIn = 1;
	cp->CPop.bit.Interpret = 1;
	cp->CPID  = 0;
	cp->CPbus = 0;
	cp->CPmsg0  = HA_IDENTIFY_MSG | HA_DISCO_RECO;
	cp->c_bind  = NULL;
}

/*
 * STATIC void
 * dpt_cache_detect(scsi_ha_t *ha, char *buf_p, paddr_t buf_pa)
 *
 * Description: Detect parameters of any contoller cache.
 *
 * Entry/Exit Locks: None.
 */
STATIC void
dpt_cache_detect(scsi_ha_t *ha, char *buf_p, paddr_t buf_pa, int buf_size)
{
	int	bytes;
	char	*parm;
	ccb_t * cp = dpt_getccb(ha);

	/*
	 * In case of error, don't hurt best performing system
	 */
	ha->ha_cache = DPT_CACHE_WRITEBACK;

	dpt_init_cp(cp);
	cp->CPdataDMA   = (paddr_t) dpt_swap32(buf_pa);
	cp->CPdataLen = dpt_swap32(buf_size);

	bzero(buf_p, buf_size); /* Enable sanity check below */

	/*
	 * Build the EATA Command Packet structure
	 * for a Log Sense Command.
	 */
	cp->CPcdb[0]  = 0x4d;
	cp->CPcdb[1]  = 0x0;
	cp->CPcdb[2]  = 0x40;
	cp->CPcdb[2]  |= 0x33;
	cp->CPcdb[5]  = 0;
	cp->CPcdb[6]  = 0;
	*(ushort *)&cp->CPcdb[7] = sdi_swap16(buf_size);

	if (dpt_send_wait(ha, cp, 10000) == FAILURE) {
		dpt_freeccb(ha, cp);
		return;
	}

	dpt_freeccb(ha, cp);

	/*
	 * Sanity check
	 */
	if (buf_p[0] != 0x33) {
		return;
	}

	bytes = DPT_HCP_LENGTH(buf_p);

	parm  = DPT_HCP_FIRST(buf_p);

	if (DPT_HCP_CODE(parm) != 1) {
		/*
		 *+ DPT Log Page layout error
		 */
		cmn_err(CE_NOTE, "!%s: Log Page (1) layout error", ha->ha_name);
		return;
	}

	if (!(parm[4] & 0x4)) {
		ha->ha_cache = DPT_NO_CACHE;
		return;
	}

	while (DPT_HCP_CODE(parm) != 4)  {

		parm = DPT_HCP_NEXT(parm);

		if (parm < buf_p || parm >= &buf_p[bytes]) {
			dpt_mode_sense(ha, buf_p, buf_pa, buf_size);
			return;
		}
	}

	ha->ha_cachesize = dpt_swap32(*(ulong *)&parm[4]);

	parm  = DPT_HCP_FIRST(buf_p);
	while (DPT_HCP_CODE(parm) != 6)  {

		parm = DPT_HCP_NEXT(parm);

		if (parm < buf_p || parm >= &buf_p[bytes]) {
			dpt_mode_sense(ha, buf_p, buf_pa, buf_size);
			return;
		}
	}

	if (parm[4] & 0x2) {
		ha->ha_cache = DPT_NO_CACHE; /* Cache disabled */
		return;
	}

	if (parm[4] & 0x4) {
		ha->ha_cache = DPT_CACHE_WRITETHROUGH;
		return;
	}

	return;
}

/*
 * STATIC void
 * dpt_mode_sense(scsi_ha_t *ha, char *buf_p, paddr_t buf_pa, int buf_size)
 *
 * Description: Send Mode Sense to HBA to determine cache type.  This routine
 * is only called on firmware that does not return enough information
 * from Log Sense.
 *
 * This routine should only be called if cache has already been detected
 * on the controller.
 *
 * Entry/Exit Locks: None.
*/
STATIC void
dpt_mode_sense(scsi_ha_t *ha, char *buf_p, paddr_t buf_pa, int buf_size)
{
	char	*page;
	ccb_t * cp = dpt_getccb(ha);

	dpt_init_cp(cp);
	cp->CPdataDMA   = (paddr_t) dpt_swap32(buf_pa);
	cp->CPdataLen = dpt_swap32(buf_size);

	bzero(buf_p, buf_size); /* Enable sanity check below */

	/*
	 * Build the EATA Command Packet structure
	 * for a Mode Sense Command.
	 */
	cp->CPcdb[0]  = SS_MSENSE; /* Mode Sense */
	cp->CPcdb[1]  = 0;
	cp->CPcdb[2]  = 0x00;  /* PC: Current Values */
	cp->CPcdb[2]  |= 0x8;  /* Caching Page */
	cp->CPcdb[3]  = 0;
	cp->CPcdb[4]   = buf_size;

	if (dpt_send_wait(ha, cp, 10000) == FAILURE) {
		dpt_freeccb(ha, cp);
		return;
	}

	dpt_freeccb(ha, cp);

	page =  &buf_p[4] + buf_p[3];

	if ((page[0] & 0x3F) != 0x8 || page[1] != 0xA) {
		/*
		 *+ DPT Caching Page layout error
		 */
		cmn_err(CE_NOTE, "!dpt(%d): Caching Page layout error", ha->ha_base);
		return;

	}

	if (!(page[2] & 0x4)) {
		ha->ha_cache = DPT_CACHE_WRITETHROUGH;
		return;
	}

	return;
}
/*
 * STATIC void
 * dpt_Inquiry(scsi_ha_t *ha, char *buf_p, paddr_t buf_pa, int buf_size)
 *
 * Description: Fill Out And Send Off An Inquiry Command
 *
 *
 * Entry/Exit Locks: None.
*/

STATIC void
dpt_Inquiry(scsi_ha_t *ha, char *buf_p, paddr_t buf_pa, int buf_size)
{
	ccb_t * cp = dpt_getccb(ha);

	dpt_init_cp(cp);
	cp->CPID = ha->ha_id[0];
	cp->CPdataDMA = (paddr_t) dpt_swap32(buf_pa);
	cp->CPdataLen = dpt_swap32(buf_size);

	bzero(buf_p, buf_size);

	/*
	* Build the EATA Command Packet structure
	* for an Adapter Inquiry.
	*/
	cp->CPcdb[0] = SS_INQUIR;
	cp->CPcdb[1] = 0;
	cp->CPcdb[2] = 0;
	cp->CPcdb[3] = 0;
	cp->CPcdb[4] = buf_size;

	if (dpt_send_wait(ha, cp, 10000) == FAILURE) {
		/* DPT Inquiry timeout */
		cmn_err(CE_WARN, "%s: Inquiry command timed out", ha->ha_name);
		dpt_freeccb(ha, cp);
		return;
	}

	dpt_freeccb(ha, cp);

	return;
}

#ifdef DPT_TARGET_MODE
/*
 * STATIC void
 * dpt_targSet(scsi_ha_t *ha, int b, ccb_t *cp)
 *
 * Description: Fill Out a read buffer command with the 'sync' bit on.
 *		This will make the command wait until a corresponding
 *		write buffer is received by the adapter.
 */

STATIC void
dpt_targSet(scsi_ha_t *ha, int b, ccb_t *cp)
{
	dpt_init_cp(cp);
	cp->CPop.byte = HA_AUTO_REQ_SEN;
	cp->CPID = (BYTE) ha->ha_id[b];
	cp->CPbus = (BYTE)(b);
	cp->CPop.bit.Interpret = 0;

	cp->CPdataDMA   = 0;
	cp->CPdataLen   = 0;

	/*
	 * Build the EATA Command Packet structure
	 * for a Read Buffer Command.
	 */
	cp->CPcdb[0]  = SM_RDDB;	/* Read Buffer */
	cp->CPcdb[2]  = 0x00;		/* Buffer Id */
	cp->CPcdb[3]  = cp->CPcdb[4]  = cp->CPcdb[5]  =  0; /* Buffer offset */
	cp->CPcdb[6]  = 0;
	cp->CPcdb[7]  = 0;
	cp->CPcdb[8]  = 0;
	cp->CPcdb[9]  = 0x80;	/* 'sync' bit */
	cp->CPcdb[1]  = RW_BUF_DATA;

	(void)dpt_send(ha, ha->ha_tgCcb[b], dptintr_target);
}
#endif /* DPT_TARGET_MODE */

/*
 * STATIC int
 * dptr_BlinkLED(register int port)
 *
 * Description: Get the blink LED state for the HBA at the passed in
 *	      IO address.
 *
 *
 * Entry/Exit Locks:  DPT_STPKT_LOCK
*/

STATIC int
dptr_BlinkLED(register int port)
{
	int  Rtnval;
	int  Attempts;
	long BlinkIndicator;
	long State;
	long OldState;

	Rtnval = 0;
	Attempts = 10;
	BlinkIndicator = 0x42445054;
	State = 0x12345678;
	OldState = 0;
	while((Attempts--)&&(State != OldState)) {
		OldState = State;
		State = inl(port + 1);
	}
	if((State == OldState)&&(State == BlinkIndicator))
		Rtnval = (int)(inb(port + 5) & 0x0ff);
	return(Rtnval);
}

/*
 * STATIC int
 * dpt_ha_init(scsi_ha_t *ha)
 *
 * Description:
 * This is the Host Adapter initialization routine
 *
 * Calling/Exit State:
 * None.
 */
STATIC int
dpt_ha_init(scsi_ha_t *ha)
{
	struct	RdConfig *eata_cfg;	/* Pointer to EATA Read Config struct */
	char 	*dpt_buf;
	paddr_t	dpt_buf_pa;

	eata_cfg = (struct RdConfig *)
			kmem_zalloc(sizeof(struct RdConfig ), KM_SLEEP);

	if ( eata_cfg == NULL) {
		cmn_err(CE_WARN, "dpt: failed allocating eata_cfg");
		return -1;
	}

	if ( EATA_ReadConfig(ha->ha_base, eata_cfg) != 1 ) {
		/*
		 * Controller may not be in a sane state, try resetting
		 */
		dpt_board_reset(ha->ha_base);
		if( EATA_ReadConfig(ha->ha_base, eata_cfg) != 1 ) {
			/*
			 * Cannot communicate with controller
			 */
			kmem_free(eata_cfg, sizeof(struct RdConfig ));
			return -1;
		}
	}

	if ( eata_cfg->DMAChannelValid ) {

		if (dma_cascade(8 - eata_cfg->DMA_Channel, DMA_ENABLE | DMA_NOSLEEP) == B_FALSE) {
			/*
			 *+ The DMA channel in question is already
			 *+ in use  and is not in cascade mode.
			 */
			cmn_err(CE_WARN, "dpt: DMA channel %d is not available",
						8 - eata_cfg->DMA_Channel);
			kmem_free(eata_cfg, sizeof(struct RdConfig ));
			return -1;
		}
	}

	if ((inb(ha->ha_base + HA_STATUS) == (HA_ST_SEEK_COMP | HA_ST_READY)) &&
	   ((ha->ha_bus_type & (CM_BUS_EISA|CM_BUS_PCI)) ||
	    (ha->ha_vect == eata_cfg->IRQ_Number))) {
		/*
		*+ DPT Host Adaptor found at given address.
		*/
		cmn_err(CE_NOTE, "!dpt: Adapter found at address 0x%X\n", 
						(int)ha->ha_base);
		ha->ha_state |= C_SANITY; /* Mark HA operational */
	}
	else {
	   	if (!(ha->ha_bus_type & (CM_BUS_EISA|CM_BUS_PCI)) &&
		   (ha->ha_vect != eata_cfg->IRQ_Number)) {
			cmn_err(CE_CONT, "!dpt: Adapter NOT configured at correct IRQ, board setting is %d, configuration is %d\n", eata_cfg->IRQ_Number, ha->ha_vect);
		}
		kmem_free(eata_cfg, sizeof(struct RdConfig ));
		return -1;
	}

	/*
	 * Set Up The HBA Target ID, Number Of Busses,
	 * Targets, And LUNS supported By this HBA
	 */

	ha->ha_id[0] = eata_cfg->Chan_0_ID;
	ha->ha_id[1] = eata_cfg->Chan_1_ID;
	ha->ha_id[2] = eata_cfg->Chan_2_ID;
	ha->ha_id[3] = 0;

#ifdef DPT_TIMEOUT_RESET_SUPPORT
	ha->ha_flags = 0;
	ha->ha_actv_jobs = (sblk_t *)NULL;
	ha->ha_watchdog_id = -1;
#endif /* DPT_TIMEOUT_RESET_SUPPORT */

	/*
	 * Check the read config length to see if
	 * it is the new version
	 */

	*(ulong * )eata_cfg->ConfigLength =
				sdi_swap32(*(ulong * )eata_cfg->ConfigLength);

	if (*(ulong * )eata_cfg->ConfigLength >= HBA_BUS_TYPE_LENGTH) {
		/* 
		 * This is the new version of the read config, so set up 
		 * the maximum number of busses, targets, and luns.
		 */
		ha->ha_ntargets = eata_cfg->MaxScsiID + 1;
		ha->ha_nluns = eata_cfg->MaxLUN + 1;
		ha->ha_nbus = eata_cfg->MaxChannel + 1;
	}
	else {
		/*
		 * This is the old version so go with the defaults.
		 */
		ha->ha_ntargets = 8;
		ha->ha_nluns = 8;
		ha->ha_nbus = 1;
	}

	/*
	 * Set up the target and bus shift values for
	 * indexing into the device queue list.
	 */
	if (ha->ha_nluns == 8) 
		ha->ha_tshift = 3;
	else if (ha->ha_nluns == 16) 
		ha->ha_tshift = 4;
	else 
		ha->ha_tshift = 5;

	if (ha->ha_ntargets == 8) 
		ha->ha_bshift = 3;
	else if (ha->ha_ntargets == 16)
		ha->ha_bshift = 4;
	else 
		ha->ha_bshift = 5;

	/*
	kmem_free(eata_cfg, sizeof(struct RdConfig ));

	/*
	 * Check for cache and get the adapters firmware revision
	 */

	dpt_buf = kmem_alloc_phys(DPT_BUFSIZE, ha->ha_physreq, &dpt_buf_pa, 0);
	bzero(dpt_buf, DPT_BUFSIZE);

	dpt_cache_detect(ha, dpt_buf, dpt_buf_pa, DPT_BUFSIZE);

	dpt_Inquiry(ha, dpt_buf, dpt_buf_pa, 38);
	ha->ha_fw_version[0] = dpt_buf[32];
	ha->ha_fw_version[1] = dpt_buf[33];
	ha->ha_fw_version[2] = dpt_buf[34];
	ha->ha_fw_version[3] = dpt_buf[35];

	/*
	 * free up the temporary buffer
	 */
	kmem_free((void *)dpt_buf, DPT_BUFSIZE);

	return 0;
}

STATIC int
dpt_physreq_init(scsi_ha_t *ha, int sleepflag)
{
	physreq_t *physreq;

	physreq = physreq_alloc(sleepflag);

	if (physreq == NULL) {
		return -1;
	}

	physreq->phys_align = DPT_MEMALIGN;
	physreq->phys_boundary = DPT_BOUNDARY;
	physreq->phys_dmasize = 
			(ha->ha_bus_type & (CM_BUS_EISA|CM_BUS_PCI)) ? 32 : 24;
	physreq->phys_max_scgth = MAX_DMASZ;

	if (!physreq_prep(physreq, sleepflag)) {
		physreq_free(physreq);
		return -1;
	}
	ha->ha_physreq = physreq;

	return 0;
}


STATIC void
dpt_send_cmd(scsi_ha_t *ha, paddr_t addr, int cmd)
{
	ulong base = ha->ha_base;
	ulong  loop = 50000L;

	/*
	 * Wait for controller not busy
	 */
	while ( (inb(base + HA_AUX_STATUS) & HA_AUX_BUSY) && loop--) {
		drv_usecwait(1);
	}

	if ( loop == 0 ) {
		cmn_err(CE_WARN,"dpt_send_cmd: completed unsuccessfully");
		return;
	}

	outl(base + HA_DMA_BASE, addr);

	outb(base + HA_COMMAND, cmd);
}

#ifdef DPT_TIMEOUT_RESET_SUPPORT

STATIC void
dpt_reset_device(scsi_ha_t *ha, int b, int t, int l)
{
	ulong base = ha->ha_base;
	ulong  loop = 50000L;

	/*
	 * Wait for controller not busy
	 */
	while ( (inb(base + HA_AUX_STATUS) & HA_AUX_BUSY) && loop--) {
		drv_usecwait(1);
	}

	if ( loop == 0 ) {
		cmn_err(CE_WARN,"dpt_reset_device: completed unsuccessfully");
		return;
	}

	outb(base + 4, l);
	outb(base + 5, (b << 5) + t);

	/* send FA,01 */
	outb(base + 6, 0x01);
	outb(base + HA_COMMAND, CP_IMMEDIATE);
}

/*
 * Eata Immediate command FA,02 resets all the buses on the controller,
 * the alternate command FA,09,<BUS MASK> is not supported on older
 * controllers. The later could be sent, but I will await confirmation of
 * the sequence to perform it safely before implementation.
 */

STATIC void
dpt_reset_bus(scsi_ha_t *ha, int bus)
{
	ulong base = ha->ha_base;
	ulong  loop = 50000L;

	/*
	 * Wait for controller not busy
	 */
	while ( (inb(base + HA_AUX_STATUS) & HA_AUX_BUSY) && loop--) {
		drv_usecwait(1);
	}

	if ( loop == 0 ) {
		cmn_err(CE_WARN,"dpt_reset_bus: completed unsuccessfully");
		return;
	}
	
	/* send FA,02 */
	outb(base + 6, 0x02);
	outb(base + HA_COMMAND, CP_IMMEDIATE);
}

STATIC void
dpt_abort_cmd(scsi_ha_t *ha, paddr_t addr)
{
	ulong base = ha->ha_base;
	ulong  loop = 50000L;

	/*
	 * Wait for controller not busy
	 */
	while ( (inb(base + HA_AUX_STATUS) & HA_AUX_BUSY) && loop--) {
		drv_usecwait(1);
	}

	if ( loop == 0 ) {
		cmn_err(CE_WARN,"dpt_abort_cmd: completed unsuccessfully");
		return;
	}

	outl(base + HA_DMA_BASE, addr);

	/* send FA,03 */	
	outb(base + 6, 0x03); 
	outb(base + HA_COMMAND, CP_IMMEDIATE);
}

#endif /* DPT_TIMEOUT_RESET_SUPPORT */


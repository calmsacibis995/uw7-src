#ident	"@(#)kern-pdi:io/hba/adsc/adsc.c	1.127.6.2"

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
**	SCSI Host Adapter Driver for Adaptec AHA-1542A
*/

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/dma.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/dynstructs.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/hba/adsc/adsc.h>
#include <io/hba/hba.h>
#include <util/mod/moddefs.h>
#ifndef PDI_SVR42
#include <util/ksynch.h>
#endif /* PDI_SVR42 */

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#define		DRVNAME	"adsc - Adaptec SCSI HBA driver"

#ifdef PDI_SVR42
typedef ulong_t vaddr_t;
#define KMEM_ZALLOC kmem_zalloc
#define KMEM_FREE kmem_free
#define adsc_pass_thru0 adsc_pass_thru

#else /* PDI_SVR42 */

#define KMEM_ZALLOC adsc_kmem_zalloc_physreq
#define KMEM_FREE kmem_free

#define ADSC_MEMALIGN	512
#define ADSC_BOUNDARY	0
#define ADSC_DMASIZE	24
#endif /* !PDI_SVR42 */

extern	int	adsc_gtol[];	/* global-to-local hba# mapping	*/
extern	int	adsc_ltog[];	/* local-to-global hba# mapping	*/

/* space.c identification variables */
extern	struct ver_no	adsc_sdi_ver;	/* SDI version structure	*/

/* space.c controller variables */
extern	int	adsc_hiwat;		/* LU Q low water mark		*/
extern	int	adsc_sg_enable;		/* s/g control bytes		*/
extern	int	adsc_buson;		/* bus on time			*/
extern	int	adsc_busoff;		/* bus off time			*/
extern	int	adsc_dma_rate;		/* DMA transfer rate		*/

/* space.c configuration variables */
extern	int	adsc_cntls;		/* Number of adsc controllers	*/
extern	HBA_IDATA_STRUCT _adscidata[];	/* I/O vector, IRQ, etc per controller*/

/* adsc debugging variables */
#ifdef AHA_DEBUG
#define	STATIC
STATIC	int	adscdebug = 0;
#endif
#ifdef DEBUG
STATIC	int	adsc_panic_assert = 1;
#endif

/* Misc adsc variables */
STATIC	struct	scsi_ha	*adsc_ha;	/* SCSI Host Adapter structures	*/
STATIC	boolean_t adscinit_time;	/* Init time (poll) flag	*/
STATIC	volatile boolean_t adsc_waitflag = TRUE;
STATIC	int	adsc_hacnt;		/* # of adsc HBAs found		*/
STATIC	int	adsc_num_ccbs;		/* # of allocated ctrl cmd blks */
STATIC	int	adsc_num_dma;		/* # of dmalist/sglist per HBA  */
STATIC	int	adsc_dynamic = 0;	/* Sleep/no sleep indicator for start */
STATIC	int	adsc_op = ADSC_OP_RESID;/* CCB opcode non-sg		*/
STATIC	int	adsc_op_dma = ADSC_OP_DMA_RESID;/* CCB opcode sg	*/
STATIC	dma_t	*dmalist;		/* pointer to DMA list pool	*/
STATIC	dma_t	*dfreelist;		/* List of free DMA lists	*/
STATIC	SGARRAY	*scatter;		/* pointer to scatter/gather pool*/
STATIC	SGARRAY	*sg_next;		/* pointer to next scatter/gather*/
STATIC	uchar_t	adsc_ccb_offset[MAX_CTLS]; /* optimization for ccb alloc */
STATIC	sv_t   *adsc_dmalist_sv;	/* sleep argument for UP	*/
STATIC	int	adsc_ha_structsize, adsc_mb_structsize, adsc_ccb_structsize;
STATIC	int	adsc_luq_structsize, adsc_dma_listsize, adsc_sg_listsize;


/* adsc multi-processor locking variables */
#ifndef PDI_SVR42
STATIC	int	adsc_devflag = HBA_MP;
STATIC lock_t *adsc_dmalist_lock;
STATIC lock_t *adsc_sgarray_lock;
STATIC lock_t *adsc_ccb_lock;
STATIC LKINFO_DECL(lkinfo_dmalist, "IO:adsc:adsc_dmalist_lock", 0);
STATIC LKINFO_DECL(lkinfo_sgarray, "IO:adsc:adsc_sgarray_lock", 0);
STATIC LKINFO_DECL(lkinfo_ccb, "IO:adsc:adsc_ccb_lock", 0);
STATIC LKINFO_DECL(lkinfo_mbx, "IO:adsc:ha->ha_mbx_lock", 0);
STATIC LKINFO_DECL(lkinfo_q, "IO:adsc:q->q_lock", 0);
#else
STATIC	int	adsc_devflag = 0;
#endif /* !PDI_SVR42 */

#ifdef AHA_DEBUGSG1
STATIC void adsc_merge_debug(SGARRAY *, int);
#endif

/*
 * This pointer will be used to reference the new idata structure
 * returned by sdi_hba_autoconf.  In the case of releases that do
 * not support autoconf, it will simply point to the idata structure
 * from the Space.c.
 */
HBA_IDATA_STRUCT	*adscidata;

/* The sm_poolhead struct is used for the dynamic struct allocation routines */
/* as pool to extarct from and return structs to. This pool provides 28 byte */
/* structs (the size of sblk_t structs roughly).			     */
extern struct head	sm_poolhead;

struct adsc_dmac {
    unsigned long amode;
    unsigned char dmode;
    unsigned long amask;
    unsigned char dmask;
    unsigned long mca_amask;
    unsigned char mca_dmask;
   
};

/*
 * DMA channels 1, 2, 3, and 4 are not used by the 1540.  If an attempt to
 * set the software to any of these DMA channel is done, it would be best to
 * panic than to program them.
 */

struct adsc_dmac adsc_dmac[] = {
    {	/* DMA channel 0 */
	0x0b,			/* DMA mode address */
	0xc0,			/* mode data */
	0x0a,			/* DMA mask address*/
	0x00,			/* mask data */
	0x0a,			/* MCA DMA mask address*/
	0x04,			/* MCA mask data */
    },
/*
 * These DMA channels are not used by the 1540.  If an attempt to set the
 * software to any of these DMA channel is done, it would be best to panic
 * than to program them.
 * 
 */
    {   /* DMA channel 1 */
        0x0b,                   /* DMA mode address */
        0xc1,                   /* mode data */
        0x0a,                   /* DMA mask address*/
        0x01,                   /* mask data */
        0x0a,                   /* MCA DMA mask address*/
        0x05,                   /* MCA mask data */
    },
    {   /* DMA channel 2 */
        0x0b,                   /* DMA mode address */
        0xc2,                   /* mode data */
        0x0a,                   /* DMA mask address*/
        0x02,                   /* mask data */
        0x0a,                   /* MCA DMA mask address*/
        0x06,                   /* MCA mask data */
    },
    {   /* DMA channel 3 */
        0x0b,                   /* DMA mode address */
        0xc3,                   /* mode data */
        0x0a,                   /* DMA mask address*/
        0x03,                   /* mask data */
        0x0a,                   /* MCA DMA mask address*/
        0x07,                   /* MCA mask data */
    },
    {   /* DMA channel 4 */
        0xd6,                   /* DMA mode address */
        0xc0,                   /* mode data */
        0xd4,                   /* DMA mask address*/
        0x00,                   /* mask data */
        0xd4,                   /* MCA DMA mask address*/
        0x04,                   /* MCA mask data */
    },
    {	/* DMA channel 5 */
	0xd6,			/* DMA mode address */
	0xc1,			/* mode data */
	0xd4,			/* DMA mask address*/
	0x01,			/* mask data */
	0xd4,			/* MCA DMA mask address*/
	0x05,			/* MCA mask data */
    },
    {	/* DMA channel 6 */
	0xd6,			/* DMA mode address */
	0xc2,			/* mode data */
	0xd4,			/* DMA mask address*/
	0x02,			/* mask data */
	0xd4,			/* MCA DMA mask address*/
	0x06,			/* MCA mask data */
    },
    {	/* DMA channel 7 */
	0xd6,			/* DMA mode address */
	0xc3,			/* mode data */
	0xd4,			/* DMA mask address*/
	0x03,			/* mask data */
	0xd4,			/* MCA DMA mask address*/
	0x07,			/* MCA mask data */
    }
};

/* adsc function prototypes */
STATIC SGARRAY * adsc_get_sglist(void);

STATIC int adsc_dma_init(int),
          adsc_dmalist(sblk_t *, struct proc *, int),
          adsc_hainit(int),
          adsc_hapresent(ulong),
          adsc_illegal(short, uchar_t, uchar_t),
          adsc_wait(int);


STATIC long adsc_tol(char adr[]);

STATIC struct ccb * adsc_getccb(int);

STATIC void adsc_ccb_init(int),
            adsc_cmd(SGARRAY *, sblk_t *, struct scsi_lu *),
            adsc_dma_freelist(dma_t *),
            adsc_done(int, struct scsi_lu *, struct ccb *, int, struct sb *, 
			long),
            adsc_flushq(struct scsi_lu *, int, int),
            adsc_func(sblk_t *),
            adsc_int(struct sb *),
            adsc_lockclean(int),
            adsc_lockinit(int),
            adsc_mkadr3(uchar_t str[], uchar_t adr[]),
            adsc_next(struct scsi_lu *),
            adsc_next_nl(struct scsi_lu *),
            adsc_pass_thru(buf_t *),
            adsc_putq(struct scsi_lu *, sblk_t *),
            adsc_merge(struct scsi_lu *, sblk_t *),
            adsc_send(int, int, struct ccb *),
	    adsc_set_bsize(struct scsi_lu *q, struct sb *sp, int status),
            adsc_sgdone(struct scsi_lu *, struct ccb *, int);


STATIC int adsc_getadr(sblk_t *);
STATIC sblk_t *adsc_schedule(struct scsi_lu *);
STATIC sblk_t *adsc_find_merge(struct scsi_lu *, sblk_t *, sblk_t *);

#ifdef AHA_DEBUG

STATIC int adsc_schedule_debug(sblk_t *, int current_head_position);
void adsc_queck(struct scsi_lu *);
void adsc_brk(void);

#endif /* AHA_DEBUG */

int adscinit(void);
int adscstart(void);

long adscfreeblk(struct hbadata *);

void adsc_hadone(struct scsi_lu *, struct ccb *, int),
     adscintr(uint);
#ifndef PDI_SVR42
STATIC	void *		adsc_kmem_zalloc_physreq(size_t, int);
STATIC	void		adsc_pass_thru0(buf_t *);
#endif /* !PDI_SVR42 */

#define ADSC_BSIZE(sp, q) (sp->sbp->sb.SCB.sc_datasz >> q->q_shift)
#define ADSC_IS_READ(c)		(c == SS_READ || c == SM_READ)
#define ADSC_IS_WRITE(c)	(c == SS_WRITE || c == SM_WRITE)
#define ADSC_IS_RW(c) (ADSC_IS_READ(c) || ADSC_IS_WRITE(c))
#define ADSC_CMD(sp) (*((char *)sp->sbp->sb.SCB.sc_cmdpt))

extern	void mod_drvattach(),  mod_drvdetach();
extern	unsigned char inb();

int adsc_verify(rm_key_t);
int adsc_mca_conf(HBA_IDATA_STRUCT *, int *, int *);
MOD_ACHDRV_WRAPPER(adsc, adsc_load, adsc_unload, NULL, adsc_verify, DRVNAME);
HBA_INFO(adsc, &adsc_devflag, 0x10000);

/*
 * int
 * adsc_verify(rm_key_t key) 
 *
 * Calling/Exit State:
 *	None.
 */
int
adsc_verify(rm_key_t key)
{
	HBA_IDATA_STRUCT        vfy_idata;
	cm_args_t	cma;
	cm_num_t	num;
	int 			rv;

	/*
	 * First, check to see if this is a 1640
	 */
	cma.cm_key = key;
	cma.cm_n   = 0;
	cma.cm_val = &num;
	cma.cm_vallen = sizeof(cm_num_t);
	cma.cm_param = CM_BRDBUSTYPE;

	if (cm_getval(&cma) == 0) {
		if (num == CM_BUS_MCA) {
			return 0;
		}
	}

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
	if(adsc_hapresent(vfy_idata.ioaddr1))
		return 0;
	else
		return ENODEV;
}

/*
 * STATIC int
 * adsc_load(void) 
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
adsc_load(void)
{
	adsc_dynamic = 1;

	if (adscinit()) {
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
		sdi_acfree(adscidata, adsc_cntls);
		return( ENODEV );
	}
	if (adscstart()) {
		/*
		 * At this point we have allocated a new idata array,
		 * free it before failing.
		 */
		sdi_acfree(adscidata, adsc_cntls);
		return(ENODEV);
	}
	return(0);
}

/*
 * STATIC int
 * adsc_unload(void)
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_unload(void)
{
	return(EBUSY);
}

/*
 * int
 * adscinit(void)
 *	Called by kernel to perform driver initialization.
 *	All data structures are initialized, and the board is initialized
 *	for every HA configured in the system.
 *
 * Calling/Exit State:
 *	None
 */
int
adscinit(void)
{
	register struct scsi_ha *ha;
	register dma_t  *dp;
	int  c, i;
	int sleepflag;
	struct sg_array	*sg_p;
	struct sg_array	*prev_sg_p = NULL;
	uint	bus_p;

	adscinit_time = TRUE;

	for(i = 0; i < MAX_HAS; i++)
		adsc_gtol[i] = adsc_ltog[i] = -1;

	if (drv_gethardware(IOBUS_TYPE, &bus_p) == -1) {
		/*
		 *+ adsc: cannot determine bus type at init time
		 */
		cmn_err(CE_WARN,
		"%s: drv_gethardware(IOBUS_TYPE) fails", adscidata[0].name);
		return(-1);
	}

	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	adscidata = sdi_hba_autoconf("adsc", _adscidata, &adsc_cntls);
	if(adscidata == NULL)    {
		return (-1);
	}
	sdi_mca_conf(adscidata, adsc_cntls, adsc_mca_conf);

	HBA_IDATA(adsc_cntls);

	sleepflag = adsc_dynamic ? KM_SLEEP : KM_NOSLEEP;
	adsc_sdi_ver.sv_release = 1;
	if (bus_p == BUS_EISA)
		adsc_sdi_ver.sv_machine = SDI_386_EISA;
	else if (bus_p == BUS_MCA)
		adsc_sdi_ver.sv_machine = SDI_386_MCA;
	else
		adsc_sdi_ver.sv_machine = SDI_386_AT;
	adsc_sdi_ver.sv_modes   = SDI_BASIC1;

	/* need to allocate array of adsc_ha's, one for each controller */
	adsc_ha_structsize = adsc_cntls*(sizeof (struct scsi_ha));
	for (i = 2; i < adsc_ha_structsize; i *= 2);
	adsc_ha_structsize = i;

	adsc_ha = (struct scsi_ha *)kmem_zalloc(adsc_ha_structsize, sleepflag);
	if (adsc_ha == NULL) {
		/*
		 *+ adsc: cannot allocate Host Adapter structures, adsc_ha.
		 */
		cmn_err(CE_WARN, "%s: %s Host Adapter structures", adscidata[0].name, AD_IERR_ALLOC);
		return(-1);
	}

	/*
 	* Need to allocate space for mailboxes, ccb's, dma_lists and
 	* scatter/gather lists, which must be physically contiguous.
 	* Need also to allocate space for the device queues (luq), 
	* which need not be physically contiguous since sense data
	* may cross page boundary
	*
	* ccbs are allocated in such a way as to not ever run out
	* since it is based on maximum number of concurrent jobs
	* per target.  dmalists and scatter/gather lists are also
	* sized to never run out since they are based on the maximum
	* number of concurrent jobs.
 	*/
	adsc_mb_structsize = 2*NMBX*(sizeof (struct mbx));
	adsc_num_ccbs = adsc_hiwat * MAX_TCS;
	adsc_ccb_structsize = adsc_num_ccbs*(sizeof (struct ccb));
	adsc_luq_structsize = MAX_EQ*(sizeof (struct scsi_lu));
	adsc_num_dma = adsc_num_ccbs;
	adsc_dma_listsize = adsc_num_dma * (sizeof (dma_t));
	adsc_sg_listsize = adsc_num_dma * (sizeof (SGARRAY));

#ifdef PDI_SVR42
	for (i = 2; i < adsc_mb_structsize; i *= 2);
	adsc_mb_structsize = i;

	for (i = 2; i < adsc_ccb_structsize; i *= 2);
	adsc_ccb_structsize = i;

	for (i = 2; i < adsc_luq_structsize; i *= 2);
	adsc_luq_structsize = i;

	for (i = 2; i < adsc_dma_listsize; i *= 2);
	adsc_dma_listsize = i;

	for (i = 2; i < adsc_sg_listsize; i *= 2);
	adsc_sg_listsize = i;

#ifdef	AHA_DEBUG
	if(adsc_mb_structsize > ptob(1))
		/*
		 *+ adsc: Mailbox alloc exceeds page, may cross phys boundary.
		 */
		cmn_err(CE_WARN, "%s: mb_structs exceeds pagesize (may cross physical page boundary)\n", adscidata[0].name);
	if(adsc_ccb_structsize > ptob(1))
		/*
		 *+ adsc: ccb alloc exceeds page, may cross phys boundary.
		 */
		cmn_err(CE_WARN, "%s: CCBs exceed pagesize (may cross physical page boundary)\n", adscidata[0].name);
	if(adsc_dma_listsize > ptob(1))
		/*
		 *+ adsc: dmalist alloc exceeds page, may cross phys boundary.
		 */
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)\n", adscidata[0].name);
	if(adsc_sg_listsize > ptob(1))
		/*
		 *+ adsc: sgarray alloc exceeds page, may cross phys boundary.
		 */
		cmn_err(CE_WARN, "%s: scatter array exceeds pagesize (may cross physical page boundary)\n", adscidata[0].name);
#endif /* AHA_DEBUG */
#endif /* PDI_SVR42 */

	/* Get adsc_num_dma number of lists for each controller, but
	 * link them together on one freelist
	 */
	for (c = 0; c < adsc_cntls; c++) {
		dmalist = (dma_t *)KMEM_ZALLOC(adsc_dma_listsize, sleepflag); 
		if(dmalist == NULL) {
			/*
			 *+ adsc: dmalist allocation failure.
			 */
			cmn_err(CE_WARN, "%s: %s dmalist\n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		if (c == 0) {
			dfreelist = NULL;
		}
		/* Build list of free DMA lists */
		for (i = 0; i < adsc_num_dma; i++) {
			dp = &dmalist[i];
			dp->d_next = dfreelist;
			dfreelist = dp;
		}
	}

	/* Get adsc_num_dma number of lists for each controller, but
	 * link them together on one freelist
	 */
	for (c = 0; c < adsc_cntls; c++) {
		scatter = (SGARRAY *)KMEM_ZALLOC(adsc_sg_listsize, sleepflag);
		if(scatter == NULL) {
			/*
			 *+ adsc: sgarray allocation failure.
			 */
			cmn_err(CE_WARN, "%s: %s scatter array\n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}
		if (c == 0) {
			sg_next = scatter;
		}
		for(i = 0; i < adsc_num_dma; i++) {
			sg_p = &scatter[i];
			if (prev_sg_p)
				prev_sg_p->sg_next = sg_p;
			sg_p->sg_flags = SG_FREE;
			prev_sg_p = sg_p;
		}
	}
	sg_p->sg_next = sg_next;

	for (c = 0; c < adsc_cntls; c++) {
		/* see if an adapter is there */
		if(!adsc_hapresent(adscidata[c].ioaddr1))
			continue;


		ha = &adsc_ha[c];

		ha->ha_mbo = (struct mbx *)KMEM_ZALLOC(adsc_mb_structsize, sleepflag);
		if (ha->ha_mbo == NULL) {
			/*
			 *+ adsc: Mailbox allocation failure.
			 */
			cmn_err(CE_WARN, "%s: %s mailboxes\n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}
		ha->ha_mbi = &ha->ha_mbo[NMBX];

		ha->ha_ccb = (struct ccb *)KMEM_ZALLOC(adsc_ccb_structsize, sleepflag);
		if (ha->ha_ccb == NULL) {
			/*
			 *+ adsc: ccb allocation failure.
			 */
			cmn_err(CE_WARN, "%s: %s command blocks\n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		/*
		 * Get the physical address of the ccb's
		 */
		ha->ha_pccb = vtop((caddr_t)ha->ha_ccb, NULL) & ADSC_CCB_MASK;

		ha->ha_dev = (struct scsi_lu *)kmem_zalloc(adsc_luq_structsize, sleepflag);
		if (ha->ha_dev == NULL) {
			/*
			 *+ adsc: Device queue allocation failure.
			 */
			cmn_err(CE_WARN, "%s: %s lu queues \n", adscidata[c].name, AD_IERR_ALLOC);
			continue;
		}

		ha->ha_id     = ( unsigned short)adscidata[c].ha_id;
		ha->ha_give   = &ha->ha_mbo[0];
		ha->ha_take   = &ha->ha_mbi[0];

		ha->ha_base = adscidata[c].ioaddr1;
		ha->ha_vect = adscidata[c].iov;

		if (adsc_dma_init(c)) {
			continue;
		}
#ifdef AHA_DEBUG
		if(adscdebug > 3)
			cmn_err ( CE_CONT, "adscinit calling adsc_ccb_init\n");
#endif
		adsc_ccb_init(c);

		if(!adsc_hainit(c)) {	/* initialize HA communication */
			continue;
		}
		adscidata[c].active = 1;
		adsc_lockinit(c);
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
	sdi_intr_attach(adscidata, adsc_cntls, adscintr, adsc_devflag);
	return(0);
}

/* ARGSUSED */
/*
 * STATIC int
 * adscopen(dev_t *devp, int flags, int otype, cred_t *cred_p)
 * 	Driver open() entry point. It checks permissions, and in the
 *	case of a pass-thru open, suspends the particular LU queue.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC int
adscopen(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;
	register int	c = adsc_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct scsi_lu *q;

	if (t == adsc_ha[c].ha_id)
		return(0);

	if (adsc_illegal(SC_HAN(dev), t, l)) {
		return(ENXIO);
	}

	/* This is the pass-thru section */

	q = &LU_Q(c, t, l);

	ADSC_SCSILU_LOCK(q->q_opri);
 	if ((q->q_outcnt > 0)  ||
	    (q->q_flag & (ADSC_QBUSY | ADSC_QSUSP | ADSC_QPTHRU)))
	{
		ADSC_SCSILU_UNLOCK(q->q_opri);
		return(EBUSY);
	}

	q->q_flag |= ADSC_QPTHRU;
	ADSC_SCSILU_UNLOCK(q->q_opri);
	return(0);
}

/* ARGSUSED */
/*
 * STATIC int
 * adscclose(dev_t dev, int flags, int otype, cred_t *cred_p)
 * 	Driver close() entry point.  In the case of a pass-thru close
 *	it resumes the queue and calls the target driver event handler
 *	if one is present.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC int
adscclose(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	register int	c = adsc_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct scsi_lu *q;

	if (t == adsc_ha[c].ha_id)
		return(0);

	q = &LU_Q(c, t, l);

	ADSC_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~ADSC_QPTHRU;
	ADSC_SCSILU_UNLOCK(q->q_opri);

#ifdef AHA_DEBUG
	sdi_aen(SDI_FLT_PTHRU, c, t, l);
#else
	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
#endif

	adsc_next_nl(q);
	return(0);
}

/* ARGSUSED */
/*
 * STATIC int
 * adscioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
 *	Driver ioctl() entry point.  Used to implement the following 
 *	special functions:
 *
 *	SDI_SEND     -	Send a user supplied SCSI Control Block to
 *			the specified device.
 *	B_GETTYPE    -  Get bus type and driver name
 *	HA_VER       -	Get version
 *	SDI_BRESET   -	Reset the specified SCSI bus 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_MBX_LOCK(opri).
 */
STATIC int
adscioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	register int	c = adsc_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct sb *sp;
	pl_t  opri;
	int  uerror = 0;

	switch(cmd) {
	case SDI_SEND: {
		register buf_t *bp;
		struct sb  karg;
		int  rw;
		char *save_priv;
		struct scsi_lu	*q;

		if (t == adsc_ha[c].ha_id) { 	/* illegal ID */
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

		sp = sdi_getblk(KM_SLEEP);
		save_priv = sp->SCB.sc_priv;
		bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

		bp = getrbuf(KM_SLEEP);
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
		sp->SCB.sc_dev.sa_lun = ( unsigned char )l;
		sp->SCB.sc_dev.sa_fill = (adsc_ltog[c] << 3) | t;

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
#ifdef PDI_SVR42
		bp->b_private = (char *)sp;
#else
		bp->b_priv.un_ptr = sp;
#endif
		/*
		 * If the job involves a data transfer then the
		 * request is done thru physiock() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then adsc_pass_thru()
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

			if (uerror = physiock(adsc_pass_thru0, bp, dev, rw, 
			    HBA_MAX_PHYSIOCK, &ha_uio)) {
				goto done;
			}
		} else {
			bp->b_un.b_addr = NULL;
			bp->b_bcount = 0;
			bp->b_blkno = 0;
 			bp->b_edev = dev;
			bp->b_flags |= rw;

			adsc_pass_thru(bp);  /* Bypass physiock call */
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
		if (copyout((caddr_t)&adsc_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET: {
		register struct scsi_ha *ha = &adsc_ha[c];

		ADSC_MBX_LOCK(opri);
		if (ha->ha_npend > 0) {     /* jobs are outstanding */
			ADSC_MBX_UNLOCK(opri);
			return(EBUSY);
		}
		else {
			/*
			 *+ Adaptec SCSI Bus is being reset.
			 */
			cmn_err(CE_WARN,
				"!%s: HA %d - Bus is being reset\n",
				adscidata[c].name, adsc_ltog[c]);
			outb(ha->ha_base + HA_CNTL, HA_SBR);
		}
		ADSC_MBX_UNLOCK(opri);
		break;
		}

	default:
		return(EINVAL);
	}
	return(uerror);
}

/*
 * void
 * adscintr(uint  vect)
 *	Driver interrupt handler entry point.  Called by kernel when
 *	a host adapter interrupt occurs.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_MBX_LOCK(opri).
 */
void
adscintr(uint  vect)
{
	register struct scsi_ha	*ha;
	register struct mbx	*mp;
	register struct ccb	*cp, *ccbchain;
	struct ccb		*ccbhead = NULL;
	register int		c,n;
	unsigned long		addr;
	pl_t			opri;
	struct scsi_lu		*q;
	char			t,l;
	char			*err_str;


#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adscintr(%d)\n", vect);
#endif
	/*
	 * Determine which host adapter interrupted from
	 * the interrupt vector.
	 */
	for (c = 0; c < adsc_cntls; c++) {
		ha = &adsc_ha[c];
		if (ha->ha_vect == vect)
			break;
	}

	if( !adscidata[c].active) {
#ifdef AHA_DEBUG
			/*
			 *+ Adaptec interrupt received for inactive controller.
			 */
			cmn_err (CE_WARN,"%s: Inactive HBA received an interrupt.\n",
adscidata[c].name);
#endif
                return;
	}
 
	/*
	 * Search for an incoming mailbox that is not empty.
	 * A round-robin search algorithm is used.
	 */
	n  = NMBX;
	ADSC_MBX_LOCK(opri);
	mp = ha->ha_take;
	do {
		if (mp->m_stat != EMPTY)
			break;
		if (++mp == &ha->ha_mbi[NMBX])
			mp = ha->ha_mbi;
	} while (--n);

	/*
	 * Repeat until no outstanding incoming messages
	 * are left before exiting the interrupt routine.
	 */
	while (mp->m_stat != EMPTY) {
		switch (mp->m_stat) {
		case FAILURE:
		case ABORTED:
		case SUCCESS:
			addr = sdi_swap24((int)mp->m_addr);
			cp = ADSC_CCB_PHYSTOKV(ha, (struct ccb *)addr);
			/*
			 * The adapter status has been virtually ignored 
			 * all along.  If an adapter (c_hstat) error occurs, 
			 * the user needs to know about it, as there will
			 * be no sense key/error code to rely upon for the 
			 * actual error that may have occured.
			 */
			if(cp->c_hstat > HS_ST) {
				switch(cp->c_hstat) {
					case HS_DORUR:
						err_str =
						   "Data Over/Under Run\n";
						break;
					case HS_UBF:
						err_str =
						   "Unexpected Bus Free\n";
						break;
					case HS_TBPSF:
						err_str =
						   "Target Bus Phase Sequence Failure\n";
						break;
					case HS_ICOC:
						/*
						 * We may be using opcodes that
						 * are not yet support on this
						 * board, so try old ones.
						 */
						if(adsc_op == ADSC_OP_RESID) {
						   adsc_op = ADSC_OP;
						   adsc_op_dma = ADSC_OP_DMA;
						}
						err_str =
						   "Invalid CCB Operation Code\n";
						break;
					case HS_LCDNHTSL:
						err_str =
					   	   "Linked CCB does not have the same LUN\n";
						break;
					case HS_ITDRFH:
						err_str =
						   "Invalid Target Direction Received from Host\n";
						break;
					case HS_DCRITM:
						err_str =
						   "Duplicate CCB Received in Target Mode\n";
						break;
					case HS_ICOSLP:
						err_str =
						   "Invalid CCB or Segment List Pointer\n";
						break;
					default:
						err_str =
						   "Unknown Adapter Status\n";
						break;
				}
				/*
			 	 *+ Adaptec Host Adaptor error status received.
			 	 */
				cmn_err(CE_WARN, "%s: %d: %s",
				   adscidata[c].name, adsc_ltog[c], err_str);
				if(cp->c_hstat > HS_ICOSLP)
					/*
			 		 *+ 
			 		 */
					cmn_err(CE_CONT, " 0x%x", cp->c_hstat);
			}
			ha->ha_npend--;

			/*
			 * Chain completed ccb's for done loop.
			 */
			if (ccbhead == NULL) {
				ccbhead = ccbchain = cp;
			}
			else {
				ccbchain->c_donenext = cp;
				ccbchain = cp;
			}
			cp->c_donenext = NULL;
			cp->c_mstat = mp->m_stat;

			break;

		case NOT_FND:	/* CB presented before timeout abort */
			break;

		default:

#ifdef AHA_DEBUG
			/*
			 *+ Adaptec illegal input mailbox status received.
			 */
			cmn_err(CE_WARN,
				"%s: HA %d - Illegal mailbox status 0x%x\n",
				adscidata[c].name, adsc_ltog[c], mp->m_stat);
#endif
			break;
		}
		/*
		 * Mark mail box as empty and advance 
		 * to the next incoming mail box.
		 */
		mp->m_stat = EMPTY;
		if (++mp == &ha->ha_mbi[NMBX])
			mp = ha->ha_mbi;
	}
	/*
	 * Acknowledge the interrupt, once finished with all the current MBI's.
	 */
	if((inb(ha->ha_base + HA_ISR)) & HA_INTR)
		outb((ha->ha_base + HA_CNTL), HA_IACK); /* acknowledge */

	ha->ha_take = mp;   /* save pointer value */
	ADSC_MBX_UNLOCK(opri);
	/*
	 * Now do the done processing for each completed ccb.
	 */
	cp = ccbhead;
	while (cp) {
		t = (cp->c_dev >> 5);
		l = (cp->c_dev & 0x07);
		q = &LU_Q(c, t, l);

		if ((cp->c_opcode == adsc_op) || 
		    (cp->c_opcode == adsc_op_dma)) {
			adsc_sgdone(q, cp, cp->c_mstat);
		} else 
			adsc_hadone(q, cp, cp->c_mstat);

		cp = cp->c_donenext;
	}
	adsc_waitflag = FALSE;
}

/*
 * STATIC void
 * adsc_done(struct scsi_lu *q, int status, struct ccb *cp, int flag,
 *	struct sb *sp, long resid)
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SCB structure defining the job.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri) and ADSC_CCB_LOCK(opri).
 */
STATIC void
adsc_done(int flag, struct scsi_lu *q, struct ccb *cp, int status, 
	struct sb *sp, long resid)
{
	pl_t opri;
	char *cmd;
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_done(%x, %x, %x)\n", q, cp, status);
#endif

	ASSERT(sp);

	ADSC_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~ADSC_QSENSE;	/* Old sense data now invalid */
	ADSC_SCSILU_UNLOCK(q->q_opri);


	/* Determine completion status of the job */
	switch(status) {
	case SUCCESS:
#ifdef AHA_DEBUGSGK
		if(cp->c_opcode == adsc_op_dma)
			cmn_err ( CE_CONT, "adsc_done SUCCESS\n");
#endif
		sp->SCB.sc_comp_code = SDI_ASW;
		break;

	case FAILURE:
#ifdef AHA_DEBUGSG
		cmn_err ( CE_CONT, "adsc_done FAILURE %x\n", cp->c_hstat);
#endif
		if(cp->c_hstat == NO_ERROR) {	/* good HA status */
#ifdef AHA_DEBUG
			if(adscdebug > 3)
				cmn_err ( CE_CONT, "adsc_done FAILURE - NO_ERROR\n");
#endif
			if (cp->c_tstat == S_GOOD) {
#ifdef AHA_DEBUG
			if(adscdebug > 3)
				cmn_err ( CE_CONT, "adsc_done FAILURE - NO_ERROR - GOOD\n");
#endif
				sp->SCB.sc_comp_code = SDI_ASW;
			} else {
				sp->SCB.sc_comp_code = (unsigned long)SDI_CKSTAT;
				sp->SCB.sc_status = cp->c_tstat;
				bcopy((caddr_t)(cp->c_cdb+cp->c_cmdsz),
					(caddr_t)(sdi_sense_ptr(sp))+1, 
					sizeof(q->q_sense)-1);
				/* Cache request sense info (note padding) */
				ADSC_SCSILU_LOCK(q->q_opri);
				bcopy((caddr_t)(cp->c_cdb+cp->c_cmdsz),
					(caddr_t)(&q->q_sense)+1,
					sizeof(q->q_sense)-1);
#ifdef AHA_DEBUGSG
				cmn_err ( CE_CONT, "Set ADSC_QSENSE>\n");
#endif
				q->q_flag |= ADSC_QSENSE;
				ADSC_SCSILU_UNLOCK(q->q_opri);

			}
		}
		else if (cp->c_hstat == NO_SELECT)
			sp->SCB.sc_comp_code = (unsigned long)SDI_NOSELE;
		else if (cp->c_hstat == TC_PROTO)
			sp->SCB.sc_comp_code = (unsigned long)SDI_TCERR; 
		else
			sp->SCB.sc_comp_code = (unsigned long)SDI_HAERR;
		break;
	case RETRY:
#ifdef AHA_DEBUGSG
		cmn_err ( CE_CONT, "adsc_done RETRY\n");
#endif
		sp->SCB.sc_comp_code = (unsigned long)SDI_RETRY;

		break;

	case ABORTED:
#ifdef AHA_DEBUGSG
		cmn_err ( CE_CONT, "adsc_done ABORTED\n");
#endif
		sp->SCB.sc_comp_code = (unsigned long)SDI_ABORT;

		break;

	default:
#ifdef AHA_DEBUGSG
		{
		int c;
		struct scsi_ad *sa;

		sa = &sp->sbp->sb.SCB.sc_dev;
		c = adsc_gtol[SDI_HAN(sa)];
		/*
		 *+ Adaptec illegal Host Adapter status.
		 */
		cmn_err(CE_WARN, "%s: Illegal status 0x%x!\n",adscidata[c].name, status);
		}
#endif
		ASSERT(adsc_panic_assert);
		sp->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
		break;
	}

	cmd = (char *)sp->SCB.sc_cmdpt;

	if (*cmd == SM_RDCAP || *cmd == SS_MSELECT || *cmd == SS_MSENSE) 
		adsc_set_bsize(q, sp, status);

	/* call target driver interrupt handler */
	sp->SCB.sc_resid = resid;
	sdi_callback(sp);

	if(flag == SG_START) {
		ADSC_CCB_LOCK(opri);
		FREECCB(cp);
		ADSC_CCB_UNLOCK(opri);

		ADSC_SCSILU_LOCK(q->q_opri);
		q->q_outcnt--;
		if(q->q_outcnt < adsc_hiwat)
			q->q_flag &= ~ADSC_QBUSY;

		adsc_next(q);
	}
} 

/*
 * void
 * adsc_hadone(struct scsi_lu *q, struct ccb *cp, int status)
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SFB structure defining the job.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri) and ADSC_CCB_LOCK(opri).
 */
void
adsc_hadone(struct scsi_lu *q, struct ccb *cp, int status)
{
	register struct sb *sp;
	pl_t opri;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_hadone(%x, %x, %x)\n", q, cp, status);
#endif

	sp = cp->c_bind;

	ASSERT(sp);

	/* Determine completion status of the job */

	switch (status) {
	case SUCCESS:
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		cmn_err ( CE_CONT, "adsc_hadone SUCCESS\n");
#endif
		sp->SFB.sf_comp_code = SDI_ASW;
		adsc_flushq(q, SDI_CRESET, 0);
		break;
	case FAILURE:
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		cmn_err ( CE_CONT, "adsc_hadone FAILURE\n");
#endif
		sp->SFB.sf_comp_code = (unsigned long)SDI_HAERR;
		ADSC_SCSILU_LOCK(q->q_opri);
		q->q_flag |= ADSC_QSUSP;
		ADSC_SCSILU_UNLOCK(q->q_opri);
		break;
	default:
		ASSERT(adsc_panic_assert);
#ifdef AHA_DEBUG
		/*
		 *+ Adaptec illegal Host Adapter status.
		 */
		cmn_err(CE_WARN, "%s: Illegal status 0x%x!\n",adscidata[0].name, status);
#endif
		sp->SFB.sf_comp_code = (unsigned long)SDI_ERROR;
		break;
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	ADSC_CCB_LOCK(opri);
	FREECCB(cp);
	ADSC_CCB_UNLOCK(opri);

	ADSC_SCSILU_LOCK(q->q_opri);
	q->q_outcnt--;
	if(q->q_outcnt < adsc_hiwat)
		q->q_flag &= ~ADSC_QBUSY;

	adsc_next(q);
}

/*
 * void
 * adsc_set_bsize(struct scsi_lu *q, struct sb *sp, int status)
 *	Record the sector size for device.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
adsc_set_bsize(struct scsi_lu *q, struct sb *sp, int status)
{
	register int size, shift;
	char *cmd;
	struct sdi_edt *edtp;
	struct scsi_ad *sa;
	int c, t, l, b;

	if (status != SUCCESS) {
		/*
		 *+ Adaptec Cannot Update Sector Size
		 */
		cmn_err (CE_WARN, "%s: Cannot update sector size\n", adscidata[0].name);
		return;
	}

	cmd  = (char *) sp->SCB.sc_cmdpt;
	if (*cmd == SM_RDCAP) {
		/*
		 * Read Capacity returns two 32-bit ints.  We are
		 * interested in the second.
		 */
		struct rdcap { int address; int length; } *rdcap;
		rdcap = (struct rdcap *) sp->SCB.sc_datapt;
		size = sdi_swap32(rdcap->length);
	} else {
		/*
		 * Doing SS_MSELECT or SS_MSENSE 
		 * Since changer devices do not support the optional Mode
		 * parameter block descriptors, do not change q->shift for
		 * these devices.
		 */
		sa = &sp->SCB.sc_dev;
		c = adsc_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		edtp = sdi_redt(adsc_ltog[c], t, l);
		if(edtp != NULL && edtp->pdtype == ID_CHANGER) {
			return;
		} else {
			struct mode {
				unsigned len   :8;
				unsigned media :8;
				unsigned speed :4;
				unsigned bm    :3;
				unsigned wp    :1;
				unsigned bdl   :8;
				unsigned dens  :8;
				unsigned nblks :24;
				unsigned res   :8;
				unsigned bsize :24;
			} *mode = (struct mode *) sp->SCB.sc_datapt;

			size = sdi_swap24(mode->bsize);
		}
	}

	/* Assume sector size is power of 2.  Is this ok? */
	for (shift = 0; shift < 32 && 1 << shift != size; shift++) 
		;
	if (shift == 32)
		/*
		 *+ Adaptec Illegal Sector Size
		 */
		cmn_err (CE_WARN, "!%s: Illegal Sector Size (%x)\n", adscidata[0].name, size);
	else {
		/*
		 *+ Adaptec Setting sector size for device
		 */
		q->q_shift = shift;
	}
}

/*===========================================================================*/
/* SCSI Driver Interface (SDI-386) Functions
/*===========================================================================*/

/*
 * int
 * adscstart(void)
 *	This is the start routine for the SCSI HA driver.
 *	This phase of the initialization registers the controller(s)
 *	along with the devices with SDI.
 *	We make use of that fact that interrupts are enabled,
 *	in the adsc_wait loop, but since SDI inquiries expect
 *	data on the return path, we have to poll wait for completion.	
 *
 * Calling/Exit State:
 *	None
 */
int
adscstart(void)
{
	register struct scsi_ha *ha;
	int  c, i, j;
	int cntl_num;

	for (c = 0; c < adsc_cntls; c++) {
		if (!adscidata[c].active)
			continue;

		/* Get an HBA number from SDI and Register HBA with SDI */
		if( (cntl_num = sdi_gethbano( adscidata[c].cntlr )) <= -1) {
				/*
			 	 *+ adsc: No HBA number available from SDI.
			 	 */
	     		cmn_err (CE_WARN,"%s: No HBA number available.\n", adscidata[c].name);
			adscidata[c].active = 0;
			continue;
		}

		adscidata[c].cntlr = cntl_num;
		adsc_gtol[cntl_num] = c;
		adsc_ltog[c] = cntl_num;

		if( (cntl_num = sdi_register(&adschba_info, &adscidata[c])) < 0) {
			/*
			 *+ adsc: SDI registry failed.
			 */
	     		cmn_err (CE_WARN,"!%s: HA %d, SDI registry failure %d.\n",
				adscidata[c].name, c, cntl_num);
			adscidata[c].active = 0;
			continue;
		}
		adsc_ckop(c, adsc_dynamic ? KM_SLEEP : KM_NOSLEEP);
		adsc_hacnt++;
	}

	for (c = 0; c < adsc_cntls; c++) {
		adsc_lockclean(c);
		if( !adscidata[c].active) {
			ha = &adsc_ha[c];
			if( ha->ha_mbo != NULL) 
				KMEM_FREE(ha->ha_mbo, adsc_mb_structsize);
			if( ha->ha_ccb != NULL) 
				KMEM_FREE(ha->ha_ccb, adsc_ccb_structsize);
			if( ha->ha_dev != NULL) 
				kmem_free(ha->ha_dev, adsc_luq_structsize);
			ha->ha_mbo = NULL;
			ha->ha_ccb = NULL;
			ha->ha_dev = NULL;
		}
	}
	if (adsc_hacnt == 0) {
		/*
		 *+ Adaptec - No Host Adaptors found.
		 */
		if(adscidata == NULL)
			adscidata = _adscidata;
		cmn_err(CE_NOTE,"!%s: No HAs found.\n",adscidata[0].name);
		if (dmalist != NULL) 
			KMEM_FREE(dmalist, adsc_dma_listsize);
		if (scatter != NULL) 
			KMEM_FREE(scatter, adsc_sg_listsize);
		if (adsc_ha != NULL) 
			kmem_free((void *)adsc_ha, adsc_ha_structsize);
		return(-1);
	}

	/* Initialize q structures			       	*/
	/* Find Random Access devices that should be scheduled 	*/
	/* Assume default sector size of 512			*/

        for (c=0; c < adsc_cntls; c++) {
                if (!adscidata[c].active)
                        continue;
              for (i=0; i<=7; i++) {
                        for (j=0; j<=7; j++) {
                                struct sdi_edt *edtp;
				struct scsi_lu *q = &LU_Q(c, i, j);

				q->q_shift = 9;

                                edtp = sdi_redt(adsc_ltog[c], i, j);
                                if (edtp && edtp->pdtype == ID_RANDOM) {
                                        /* Other device types can potentially
                                         * be scheduled, but the scheduling
                                         * algorithm has been tuned only for
                                         * disks.
					 */
                                        struct scsi_lu *q = &LU_Q(c, i, j);
                                        q->q_flag |= ADSC_QSCHED;
                                }
                        }
                }
        }

	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally.
	 */
	adscinit_time = FALSE;

	return(0);
}

/*
 * STATIC long
 * HBASEND(struct hbadata *hbap, int sleepflag)
 * 	Send a SCSI command to a controller.  Commands sent via this
 *	function are executed in the order they are received.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct scsi_lu *q;
	register sblk_t *sp = (sblk_t *) hbap;
	register int	c, t;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adscsend(%x)\n", hbap);
#endif
	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adsc_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	q = &LU_Q(c, t, sa->sa_lun);

	ADSC_SCSILU_LOCK(q->q_opri);
	if (q->q_flag & ADSC_QPTHRU) {
		ADSC_SCSILU_UNLOCK(q->q_opri);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	adsc_putq(q, sp);
	adsc_next(q);
	return (SDI_RET_OK);
}

/*
 * STATIC long
 * HBAICMD(struct hbadata *hbap, int sleepflag)
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC long
HBAICMD(struct hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct scsi_lu *q;
	register sblk_t *sp = (sblk_t *) hbap;
	register int	c, t, l;
	struct ident	     *inq_data;
	struct scs	     *inq_cdb;
	struct sdi_edt *edtp;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adscicmd(%x)\n", hbap);
#endif

	switch (sp->sbp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = adsc_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		cmn_err ( CE_CONT, "sdi_icmd: SFB c=%d t=%d l=%d \n", c, t, sa->sa_lun);
#endif

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {
		case SFB_RESUME:
			ADSC_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~ADSC_QSUSP;
			adsc_next(q);
			break;
		case SFB_SUSPEND:
			ADSC_SCSILU_LOCK(q->q_opri);
			q->q_flag |= ADSC_QSUSP;
			ADSC_SCSILU_UNLOCK(q->q_opri);
			break;
		case SFB_RESETM:
			edtp = sdi_redt(adsc_ltog[c],t,l);
			if(edtp != NULL && edtp->pdtype == ID_TAPE) {
				/*  this is a NOP for tape devices */
				sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;
				break;
			}
			/*FALLTHRU*/
		case SFB_ABORTM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			ADSC_SCSILU_LOCK(q->q_opri);
			adsc_putq(q, sp);
			adsc_next(q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			adsc_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code =
			    (unsigned long)SDI_SFBERR;
		}

		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = adsc_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);
#ifdef AHA_DEBUG
	if(adscdebug > 3)
		cmn_err ( CE_CONT, "sdi_icmd: SCB c=%d t=%d l=%d \n", c, t, sa->sa_lun);
#endif
		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;
		if ((t == adsc_ha[c].ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, adscidata[c].name, 
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

		ADSC_SCSILU_LOCK(q->q_opri);
		adsc_putq(q, sp);
		adsc_next(q);
		return (SDI_RET_OK);

	default:
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_ERR);
	}
}


/*
 * STATIC int
 * adscxlat(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
 *	Perform the virtual to physical translation on the SCB
 *	data pointer. 
 *	This routine can fail if a dmalist is not available, and
 *	the sleepflag is KM_NOSLEEP.
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adscxlat(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	register sblk_t *sp = (sblk_t *) hbap;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adscxlat(%x, %x, %x)\n", hbap, flag, procp);
#endif

	if(sp->s_dmap) {
#ifdef AHA_DEBUGSGK
		cmn_err ( CE_CONT, "s_dmap = %x, sc_link = %x\n",
		    sp->s_dmap,sp->sbp->sb.SCB.sc_link);
#endif
#ifdef AHA_DEBUG
		if(adscdebug > 0)
			cmn_err ( CE_CONT, "adscxlat: calling adsc_dma_freelist(%x)\n",
			        sp->s_dmap);
#endif
		adsc_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}

	if(sp->sbp->sb.SCB.sc_link) {
		/*
		 *+ Adaptec linked commands currently not available.
		 */
		cmn_err(CE_WARN,
		    "Adaptec: Linked commands NOT available\n");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if(sp->sbp->sb.SCB.sc_datapt) {
		/*
		 * Do scatter/gather if data spans multiple pages
		 */
		if(!(flag & B_PHYS)) {
			procp = (struct proc *)NULL;
		}
		sp->s_addr = vtop(sp->sbp->sb.SCB.sc_datapt, procp);
		if (sp->sbp->sb.SCB.sc_datasz > pgbnd(sp->s_addr)) {
			if (adsc_dmalist(sp, procp, sleepflag))
				return (SDI_RET_RETRY);
		}
	} else
		sp->sbp->sb.SCB.sc_datasz = 0;

#ifdef AHA_DEBUG
	if(adscdebug > 3)
		cmn_err ( CE_CONT, "adscxlat: returning\n");
#endif
	return (SDI_RET_OK);
}

/*
 * STATIC struct hbadata *
 * adscgetblk(int sleepflag)
 *	Allocate a SB structure for the caller.  The function will
 *	sleep if there are no SCSI blocks available.
 *
 * Calling/Exit State:
 *	None
 */
STATIC struct hbadata *
adscgetblk(int sleepflag)
{
	register sblk_t	*sp;

	sp = (sblk_t *)sdi_get(&sm_poolhead, sleepflag);
	return((struct hbadata *)sp);
}

/*
 * long
 * adscfreeblk(struct hbadata *hbap)
 *	Release previously allocated SB structure. If a scatter/gather
 *	list is associated with the SB, it is freed via adsc_dma_freelist().
 *	A nonzero return indicates an error in pointer or type field.
 *
 * Calling/Exit State:
 *	None
 */
long
adscfreeblk(struct hbadata *hbap)
{
	register sblk_t *sp = (sblk_t *) hbap;

	if(sp->s_dmap) {
		adsc_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return (SDI_RET_OK);
}

/*
 * STATIC void
 * adscgetinfo(struct scsi_ad *sa, struct hbagetinfo *getinfo)
 *	Return the name, etc. of the given device.  The name is copied into
 *	a string pointed to by the first field of the getinfo structure.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adscgetinfo(struct scsi_ad *sa, struct hbagetinfo *getinfo)
{
	register char  *s1, *s2;
	STATIC char temp[] = "HA X TC X";
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adscgetinfo(%x, %s)\n", sa, getinfo->name);
#endif

	s1 = temp;
	s2 = getinfo->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;

	getinfo->iotype = F_DMA_24 | F_SCGTH;

	if (adsc_op == ADSC_OP_RESID)
		getinfo->iotype |= F_RESID;
	
#ifndef PDI_SVR42
	if (getinfo->bcbp) {
		getinfo->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfo->bcbp->bcb_flags = 0;
		getinfo->bcbp->bcb_max_xfer = adschba_info.max_xfer;
		getinfo->bcbp->bcb_physreqp->phys_align = ADSC_MEMALIGN;
		getinfo->bcbp->bcb_physreqp->phys_boundary = ADSC_BOUNDARY;
		getinfo->bcbp->bcb_physreqp->phys_dmasize = ADSC_DMASIZE;
	}
#endif
}

/*===========================================================================*/
/* SCSI Host Adapter Driver Utilities
/*===========================================================================*/

#ifndef PDI_SVR42
/*
 * STATIC void
 * adsc_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
adsc_pass_thru0(buf_t *bp)
{
	int	c = adsc_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	struct scsi_lu	*q = &LU_Q(c, t, l);

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
		q->q_bcbp->bcb_max_xfer = adschba_info.max_xfer - ptob(1);
		q->q_bcbp->bcb_physreqp->phys_align = ADSC_MEMALIGN;
		q->q_bcbp->bcb_physreqp->phys_boundary = ADSC_BOUNDARY;
		q->q_bcbp->bcb_physreqp->phys_dmasize = ADSC_DMASIZE;
		q->q_bcbp->bcb_granularity = 1;
	}

	buf_breakup(adsc_pass_thru, bp, q->q_bcbp);
}
#endif /* !PDI_SVR42 */

/*
 * STATIC void
 * adsc_pass_thru(buf_t *bp)
 *	Send a pass-thru job to the HA board.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC void
adsc_pass_thru(buf_t *bp)
{
	int	c = adsc_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	register struct scsi_lu	*q;
	register struct sb *sp;
	caddr_t *addr;
	char op;
	daddr_t sblkno_adjust;

#ifdef PDI_SVR42
	sp = (struct sb *)bp->b_private;
#else
	sp = bp->b_priv.un_ptr;
#endif
	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = bp->b_un.b_addr;
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = adsc_int;

	sdi_translate(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	q = &LU_Q(c, t, l);

	/*
	 * (This is a workaround for the 2G limitation for physiock offset.)
	 * Set new SCSI block address in the SCSI command if breakup occurred
	 * by adjusting the starting SCSI block number with the kernel block
	 * number (b_blkno) adjusted to bytes per SCSI block.  Starting SCSI
	 * block number was saved in b_priv2, and having told physiock that the
	 * offset was 0, we add the adjustment now.
	 */
	if (q->q_shift >= SCTRSHFT)
		sblkno_adjust = bp->b_blkno >> (q->q_shift - SCTRSHFT);
	 else
		sblkno_adjust = bp->b_blkno << (SCTRSHFT - q->q_shift);
	op = q->q_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		daddr_t sblkno;
		struct scs *scs = (struct scs *)(void *)q->q_sc_cmd;

		if (sblkno_adjust) {
			sblkno = bp->b_priv2.un_int + sblkno_adjust;
			scs->ss_addr1 = (sblkno & 0x1F0000) >> 16;
			scs->ss_addr  = sdi_swap16(sblkno);
		}
		scs->ss_len   = (char)(bp->b_bcount >> q->q_shift);
	}
	if (op == SM_READ || op == SM_WRITE) {
		daddr_t sblkno;
		struct scm *scm = (struct scm *)(void *)((char *)q->q_sc_cmd - 2);

		if (sblkno_adjust) {
			sblkno = bp->b_priv2.un_int + sblkno_adjust;
			scm->sm_addr = sdi_swap32(sblkno);
		}
		scm->sm_len  = sdi_swap16(bp->b_bcount >> q->q_shift);
	}

	sdi_icmd(sp, KM_SLEEP);
}

/*
 * STATIC void
 * adsc_int(struct sb *sp)
 *	This is the interrupt handler for pass-thru jobs.  It just
 *	wakes up the sleeping process.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_int(struct sb *sp)
{
	struct buf  *bp;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_int: sp=%x \n",sp);
#endif
	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}

/*
 * STATIC void
 * adsc_flushq(struct scsi_lu *q, int  cc, int flag)
 *	Empty a logical unit queue.  If flag is set, remove all jobs.
 *	Otherwise, remove only non-control jobs.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC void
adsc_flushq(struct scsi_lu *q, int  cc, int flag)
{
	register sblk_t  *sp, *nsp;

	ASSERT(q);

	ADSC_SCSILU_LOCK(q->q_opri);
	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	/* XXX Clearing this may cause q_outcnt to go negative! */
	/* XXX q->q_outcnt = 0; */
	q->q_depth = 0;
	ADSC_SCSILU_UNLOCK(q->q_opri);

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (ADSC_QCLASS(sp) > ADSC_QNORM)) {
			ADSC_SCSILU_LOCK(q->q_opri);
			adsc_putq(q, sp);
			ADSC_SCSILU_UNLOCK(q->q_opri);
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
 * adsc_putq(struct scsi_lu *q, sblk_t *sp)
 *	Put a job on a logical unit queue in FIFO order.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK(q->q_opri) for q
 */
STATIC void
adsc_putq(struct scsi_lu *q, sblk_t *sp)
{
	int cls;
	sblk_t *ip = q->q_last;		/* insert job after ip */

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_putq(%x, %x)\n", q, sp);
#endif
	q->q_depth++;
	cls = ADSC_QCLASS(sp);
	if (cls != SFB_TYPE && ADSC_IS_RW(ADSC_CMD(sp))) {
		sp->s_block = adsc_getadr(sp);
	}
	if (!ip) {
		sp->s_prev = sp->s_next = NULL;
		q->q_first = q->q_last = sp;
		return;
	}
	cls = ADSC_QCLASS(sp);
	while(ip && ADSC_QCLASS(ip) < cls)
		ip = ip->s_prev;

	if (!ip) {
		sp->s_next = q->q_first;
		sp->s_prev = NULL;
		q->q_first = q->q_first->s_prev = sp;
	} else if (!ip->s_next) {
		sp->s_next = NULL;
		sp->s_prev = q->q_last;
		q->q_last = ip->s_next = sp;
	} else {
		sp->s_next = ip->s_next;
		sp->s_prev = ip;
		ip->s_next = ip->s_next->s_prev = sp;
	}
}

/*
 * STATIC void
 * adsc_getq(struct scsi_lu *q, sblk_t *sp)
 *	remove a job from a logical unit queue.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK(q->q_opri) for q
 */
STATIC void
adsc_getq(struct scsi_lu *q, sblk_t *sp)
{
	ASSERT(q);
	ASSERT(sp);
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_getq(%x, %x)\n", q, sp);
#endif
	if (sp->s_prev && sp->s_next) {
		sp->s_next->s_prev = sp->s_prev;
		sp->s_prev->s_next = sp->s_next;
	} else {
		if (!sp->s_prev) {
			q->q_first = sp->s_next;
			if (q->q_first)
				q->q_first->s_prev = NULL;
		} 
		if (!sp->s_next) {
			q->q_last = sp->s_prev;
			if (q->q_last)
				q->q_last->s_next = NULL;
		} 
	}
	q->q_depth--;
}

/*
 * STATIC int
 * adsc_getadr(sblk_t *sp)
 *	Return the logical address of the disk request pointed to by
 *	by sp.  If there is no associated disk address associated with
 *	sp, return -1.
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_getadr(sblk_t *sp)
{
	char *p;
	ASSERT(sp->sbp->sb.sb_type != SFB_TYPE);
	ASSERT(ADSC_IS_RW(ADSC_CMD(sp)));
        p = (char *)sp->sbp->sb.SCB.sc_cmdpt;
        switch(p[0]) {
        case SM_READ:
        case SM_WRITE:
                return adsc_tol(&p[2]);
        case SS_READ:
        case SS_WRITE:
                return ((p[1]&0x1f) << 16) | (p[2] << 8) | p[3];
        }
	/* NOTREACHED */
}

/* STATIC int
 * adsc_serial(struct scsi_lu *q, sblk_t *last)
 *	Return non-zero if the last job in the chain from
 *	first to last can be processed ahead of all the 
 *	other jobs on the chain.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK(q->q_opri) for q held on entry
 *	ADSC_SCSILU_LOCK(q->q_opri) for q held on exit
 */
STATIC int
adsc_serial(struct scsi_lu *q, sblk_t *last)
{
	sblk_t *first = q->q_first;
	while (first != last) {
		unsigned int sb1 = last->s_block;
		unsigned int eb1 = sb1 + ADSC_BSIZE(last, q) - 1;
		if (ADSC_IS_WRITE(ADSC_CMD(first)) || 
		    ADSC_IS_WRITE(ADSC_CMD(last))) {
			unsigned int sb2 = first->s_block;
			unsigned int eb2 = sb2 + ADSC_BSIZE(first, q) - 1;
			if (sb1 <= sb2 && eb1 >= sb2)
				return 0;
			if (sb2 <= sb1 && eb2 >= sb1)
				return 0;
		}
		first = first->s_next;
	}
	return 1;
}
			
#define ADSC_ABS_DIFF(x,y)	(x < y ? y - x : x - y)

/*
 * STATIC sblk_t *
 * adsc_schedule(struct scsi_lu *)
 *	Select the next job to process.  This routine assumes jobs
 *	are placed on the queue in sorted order.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK(q->q_opri) for q held on entry
 *	ADSC_SCSILU_LOCK(q->q_opri) for q held on exit
 */
STATIC sblk_t *
adsc_schedule(struct scsi_lu *q)
{
        sblk_t *sp = q->q_first;
        sblk_t  *best = NULL;
        unsigned int	best_position, best_distance, distance, position;
        int class;
#ifdef AHA_DEBUG
        int count;
#endif
	if (!sp || ADSC_QCLASS(sp) == SFB_TYPE || !(q->q_flag&ADSC_QSCHED))
		return sp;
	class = ADSC_QCLASS(sp);
	best_distance = (unsigned int)~0;
	
#if AHA_DEBUG
	if (adscdebug > 4)
		count = adsc_schedule_debug(sp, q->q_addr);
#endif

        /* Implement shortest seek first */

        while (sp && ADSC_QCLASS(sp) == class && ADSC_IS_RW(ADSC_CMD(sp))) {
		position = sp->s_block;
                distance = ADSC_ABS_DIFF(q->q_addr, position);
                if (distance < best_distance && adsc_serial(q, sp)) {
                        best_position = position;
                        best_distance = distance;
                        best = sp;
                }
		sp = sp->s_next;
        }
#ifdef AHA_DEBUG
        if (adscdebug > 2 && count && best) 
		cmn_err(CE_CONT,"Selected position %d\n", best_position);
#endif
	if (best) {
		q->q_addr = best_position + ADSC_BSIZE(best, q);
		return best;
	} 
	return q->q_first;
}
	
#ifdef AHA_DEBUG

/* Only print scheduler choices when there are more than ADSC_QDBG jobs */
#define ADSC_QDBG 2

/* 
 * STATIC int
 * adsc_schedule_debug(sblk_t *sp, int head)
 *	Debug the disk scheduler.  Return non-zero if number of jobs being
 *	considered is at least ADSC_QDBG.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK(q->q_opri) for q held on entry
 *	ADSC_SCSILU_LOCK(q->q_opri) for q held on exit
 */
STATIC int
adsc_schedule_debug(sblk_t *sp, int head)
{
        sblk_t *tsp = sp;
        int count=0;
        while (tsp && ADSC_QCLASS(tsp) == ADSC_QCLASS(sp)) {
                count++;
                tsp = tsp->s_next;
        }
        if (count < ADSC_QDBG)
                return 0;
        cmn_err(CE_CONT, "\n\nSchedule:\n");
        cmn_err(CE_CONT, "\thead position: %d\n", head);
        cmn_err(CE_CONT, "\tChoosing among %d choices\n",count);
        cmn_err(CE_CONT, "\tAddresses:");
        tsp = sp;
        while (tsp && ADSC_QCLASS(tsp) == ADSC_QCLASS(sp)) {
		if (tsp->sbp->sb.sb_type==SFB_TYPE ||!ADSC_IS_RW(ADSC_CMD(tsp)))
			cmn_err(CE_CONT, " COM");
		else
			cmn_err(CE_CONT, " %d", tsp->s_block);
                tsp = tsp->s_next;
        }
        cmn_err(CE_CONT, "\n");
        return count;
}
#endif /* AHA_DEBUG */

/*
 * STATIC void
 * adsc_next_nl(struct scsi_lu *q)
 *	Aquire the queue lock, then call adsc_next(), which
 *	attempts to send the next job on the logical unit queue.
 *	All jobs are not sent if the Q is busy.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC void
adsc_next_nl(struct scsi_lu *q)
{
#ifdef AHA_DEBUG
	if(adscdebug > 2)
		cmn_err ( CE_CONT, "adsc_next_nl(%x)\n", q);
#endif
	ADSC_SCSILU_LOCK(q->q_opri);
	adsc_next(q);	/* releases SCSILU lock */
}

/*
 * STATIC void
 * adsc_next(struct scsi_lu *q)
 *	Attempt to send the next job on the logical unit queue.
 *	All jobs are not sent if the Q is busy.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK(q->q_opri) for q held on entry
 *	no locks held on exit.
 */
STATIC void
adsc_next(struct scsi_lu *q)
{
	register sblk_t	*sp;

	ASSERT(q);
#ifdef AHA_DEBUG
	adsc_queck(q);
	if(adscdebug > 2)
		cmn_err ( CE_CONT, "adsc_next(%x)\n", q);
#endif
	/*
	 * ADSC_QBUSY indicates the queue of commands has reached the
	 * high water mark for building I/O's for a given target.
	 * If the number of commands falls below the adsc_hiwat variable, 
	 * then more I/O's are allowed to start.
	 * Once the number of I/O's is above the adsc_hiwat variable,
	 * the commands are shoved in the queue for later work.  
	 * This is where s/g (local) becomes possible, as long as the 
	 * commands have been sorted in an ascending order.
	 */
	if (q->q_flag & ADSC_QBUSY) {
		ADSC_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	sp = adsc_schedule(q);

	if(sp == NULL) {	 /*  queue empty  */
#ifdef AHA_DEBUG
		if(adscdebug > 2)
			cmn_err ( CE_CONT, "adsc_next: queue is empty\n");
#endif
		ASSERT(q->q_depth == 0);
		ADSC_SCSILU_UNLOCK(q->q_opri);
		return;			/*       or	  */
	}

	if (q->q_flag & ADSC_QSUSP && sp->sbp->sb.sb_type == SCB_TYPE) {
#ifdef AHA_DEBUG
	if(adscdebug > 2)
		cmn_err ( CE_CONT, "adsc_next: device is suspended\n");
#endif
		ADSC_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	adsc_getq(q, sp);
	if(sp->sbp->sb.sb_type == SFB_TYPE) {
		/* Cannot do job merging */
		q->q_outcnt++;
		if(q->q_outcnt >= adsc_hiwat)
			q->q_flag |= ADSC_QBUSY;
		ADSC_SCSILU_UNLOCK(q->q_opri);
		adsc_func(sp);
	} else if (adsc_sg_enable && !sp->s_dmap) {
		adsc_merge(q, sp);
	} else {
		q->q_outcnt++;
		if(q->q_outcnt >= adsc_hiwat)
			q->q_flag |= ADSC_QBUSY;
		ADSC_SCSILU_UNLOCK(q->q_opri);
		adsc_cmd(SGNULL, sp, q);
	}

	if(adscinit_time == TRUE) {		/* need to poll */
		if(adsc_wait(30000) == FAILURE) {
			cmn_err(CE_NOTE, "ADSC Initialization Problem, timeout with no interrupt\n");
			sp->sbp->sb.SCB.sc_comp_code = (unsigned long)SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		     }
	}
#ifdef AHA_DEBUG
	adsc_queck(q);
#endif
}

/*
 * STATIC void
 * adsc_func(sblk_t *sp)
 *	Create and send an SFB associated command. 
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_func(sblk_t *sp)
{
	register struct scsi_ad *sa;
	register struct ccb *cp;
	int  c, t;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = adsc_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = adsc_getccb(c);
	cp->c_bind = &sp->sbp->sb;

	/* fill in the controller command block */

	cp->c_opcode = SCSI_TRESET;
	cp->c_dev = (char)((t << 5) | sa->sa_lun);
	cp->c_hstat = 0;
	cp->c_tstat = 0;

	/*
	 *+ Adaptec Host Adaptor being reset.
	 */
	cmn_err(CE_WARN,
	    "!%s: HA %d TC %d is being reset\n", adscidata[c].name, c, t);
	adsc_send(c, START, cp);   /* send cmd */
}

/*
 * STATIC void
 * adsc_send(int c, int cmd, struct ccb cp)
 *	Send a command to the host adapter board.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_MBX_LOCK(opri).
 */
STATIC void
adsc_send(int c, int cmd, struct ccb *cp)
{
	register struct scsi_ha *ha = &adsc_ha[c];
	register struct mbx	*mp;
	register int 		n;
	int			opri;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_send( %x, %x, %x)\n", c, cmd , cp);
#endif
	ADSC_MBX_LOCK(opri);
	if (cmd == START) {
		ha->ha_npend++;
	}

	/*
	 * Search for an empty outgoing mail box.
	 * A round-robin search algorithm is used.
	 */
	n = NMBX;
	mp = ha->ha_give;
	do {
		if (mp->m_stat == EMPTY)
			break;
		if (++mp == &ha->ha_mbo[NMBX])
			mp = ha->ha_mbo;
	} while (--n);

	ASSERT (mp->m_stat == EMPTY);

	/*
	 * Fill in the mailbox and inform host adapter
	 * of the new request.
         */
	mp->m_addr = sdi_swap24((int)cp->c_addr);
	mp->m_stat = cmd;
	outb((ha->ha_base + HA_CMD), HA_CKMAIL);	

	/*
	 * Advance to the next outgoing mail box and
	 * save pointer value.
	 */
	if (++mp == &ha->ha_mbo[NMBX])
		mp = ha->ha_mbo;
	ha->ha_give = mp;
	ADSC_MBX_UNLOCK(opri);
}

/*
 * STATIC struct ccb *
 * adsc_getccb(int c)
 *	Allocate a controller command block structure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_CCB_LOCK(opri).
 */
STATIC struct ccb *
adsc_getccb(int c)
{
	register struct scsi_ha	*ha = &adsc_ha[c];
	register struct ccb	*cp;
	register int		cnt;
	register int		x;
	int			opri;

	ADSC_CCB_LOCK(opri);
	x = adsc_ccb_offset[c];
	cp = &ha->ha_ccb[x];
	cnt = 0;

	/*
	 * [1]	If the current ccb is not being used then (1a), advance the
	 *	counter, check to see if it needs to wrap (probably could use
	 *	x %= adsc_num_ccbs but what the heck) and break out of the loop.
	 * [2]	The value of x shoud be at least one more than it was before.
	 *	Mark the ccb active(TRUE) and return the pointer.
	 * [3]	The current ccb is active, so let's increment a counter.  If
	 *	the counter is greater than than the number of mailboxes
	 *	(adsc_num_ccbs), then we have searched through the entire list
	 *	of ccb's and didn't find one we could use (too bad).  Now we
	 *	have to PANIC the system.  This should really never occur unless
	 *	something has gone awry somewhere else.  But if we still have
	 *	some ccb's to look at, we advance to the next ccb (cp->c_next).
	 * [3a]	We would get here if we found a busy(TRUE) ccb, so we need to
	 *	move the x counter up one.
	 */
	for(;;) {
		if(cp->c_active == TRUE) {		/* [1] */
			cnt++;				/* [3] */
			ASSERT (cnt < adsc_num_ccbs);
			cp = cp->c_next;
			x++;				/* [3a] */
			if(x >= adsc_num_ccbs)
				x = 0;
		} else {				/* [1a] */
			x++;
			if(x >= adsc_num_ccbs)
				x = 0;
			break;
		}
	}
	adsc_ccb_offset[c] = ( unsigned char )x;		/* [2] */
	cp->c_active = TRUE;
	ADSC_CCB_UNLOCK(opri);
	return(cp);
}

/*
 * STATIC int
 * adsc_dmalist(sblk_t *sp, struct proc *procp, int sleepflag)
 *	Build the physical address(es) for DMA to/from the data buffer.
 *	If the data buffer is contiguous in physical memory, sp->s_addr
 *	is already set for a regular SB.  If not, a scatter/gather list
 *	is built, and the SB will point to that list instead.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_DMALIST_LOCK(opri).
 */
STATIC int
adsc_dmalist(sblk_t *sp, struct proc *procp, int sleepflag)
{
	struct dma_vect	tmp_list[SG_SEGMENTS];
	register struct dma_vect  *pp;
	register dma_t  *dmap;
	register long   count, fraglen, thispage;
	caddr_t		vaddr;
	paddr_t		addr, base;
	int		i;
	pl_t		opri;

	vaddr = sp->sbp->sb.SCB.sc_datapt;
	count = sp->sbp->sb.SCB.sc_datasz;
	pp = &tmp_list[0];

	/* First build a scatter/gather list of physical addresses and sizes */
#ifdef AHA_DEBUGSGK
	cmn_err ( CE_CONT, "Kernel - build s/g command>\n");
#endif

	for (i = 0; (i < SG_SEGMENTS) && count; ++i, ++pp) {
		base = vtop(vaddr, procp);	/* Phys address of segment */
		fraglen = 0;			/* Zero bytes so far */
		do {
			thispage = min(count, pgbnd(vaddr));
			fraglen += thispage;	/* This many more contiguous */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */
			if (!count)
				break;		/* End of request */
			addr = vtop(vaddr, procp); /* Get next page's address */
		} while (base + fraglen == addr);

		/* Now set up dma list element */
		pp->d_addr[0] = MSBYTE(base);
		pp->d_addr[1] = MDBYTE(base);
		pp->d_addr[2] = LSBYTE(base);
		pp->d_cnt[0] = MSBYTE(fraglen);
		pp->d_cnt[1] = MDBYTE(fraglen);
		pp->d_cnt[2] = LSBYTE(fraglen);
	}
	ASSERT (count == 0);

	/*
	 * If the data buffer was contiguous in physical memory,
	 * there was no need for a scatter/gather list; We're done.
	 */
	if (i > 1)
	{
		/*
		 * We need a scatter/gather list.
		 * Allocate one and copy the list we built into it.
		 */
		ADSC_DMALIST_LOCK(opri);
		if (!dfreelist && (sleepflag == KM_NOSLEEP)) {
			ADSC_DMALIST_UNLOCK(opri);
			return (1);
		}
		while (!(dmap = dfreelist)) {
			SV_WAIT(adsc_dmalist_sv, PRIBIO, adsc_dmalist_lock);
			ADSC_DMALIST_LOCK(opri);
		}
		dfreelist = dmap->d_next;
		ADSC_DMALIST_UNLOCK(opri);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
		dmap->d_size = i * sizeof(struct dma_vect);
#ifdef AHA_DEBUGSGK
		cmn_err ( CE_CONT, "S/G len %d DL%ld>\n", i, datal);
#endif
		bcopy((caddr_t) &tmp_list[0],
			(caddr_t) dmap->d_list, dmap->d_size);
	}
	return (0);
}

/*
 * STATIC void
 * adsc_dma_freelist(dma_t *dmap)
 *	Release a previously allocated scatter/gather DMA list.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_DMALIST_LOCK(opri).
 */
STATIC void
adsc_dma_freelist(dma_t *dmap)
{
	pl_t opri;

	ASSERT(dmap);
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_dma_freelist(%x)\n", dmap);
#endif

	ADSC_DMALIST_LOCK(opri);
	dmap->d_next = dfreelist;
	dfreelist = dmap;
	if (dmap->d_next == NULL) {
		ADSC_DMALIST_UNLOCK(opri);
		SV_BROADCAST(adsc_dmalist_sv, 0);
	} else
		ADSC_DMALIST_UNLOCK(opri);
}

/*
 * STATIC int
 * adsc_wait(int c, int time)
 *	Poll for a completion from the host adapter.  If an interrupt
 *	is seen, the HA's interrupt service routine is manually called. 
 *  NOTE:	
 *	This routine allows for no concurrency and as such, should 
 *	be used selectively.
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_wait(int time)
{
	
	while (time > 0) {
		if (adsc_waitflag == FALSE) {
			adsc_waitflag = TRUE;
			return (SUCCESS);
		}
		drv_usecwait(1000);
		time--;
	}
	return (FAILURE);	
}

/*
 * STATIC void
 * adsc_ccb_init(int c)
 *	Initialize the controller CB free list.
 *	This routine assumes the control blocks have been zero alloc'ed.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_ccb_init(int c)
{
	register struct scsi_ha		*ha = &adsc_ha[c];
	register struct ccb		*cp;
	register int			i;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_ccb_init( %x)\n", c);
#endif

	adsc_ccb_offset[c] = 0;
	for(i = 0; i < (adsc_num_ccbs-1); i++) {
		cp = &ha->ha_ccb[i];
		cp->c_addr = ADSC_CCB_KVTOPHYS(ha, cp);
		cp->c_active = FALSE;
		cp->c_next = &ha->ha_ccb[i+1];
		cp->c_sg_p = SGNULL;
	}
	cp = &ha->ha_ccb[i];
	cp->c_addr = ADSC_CCB_KVTOPHYS(ha, cp);
	cp->c_active = FALSE;
	cp->c_next = &ha->ha_ccb[0];
	cp->c_sg_p = SGNULL;
}

/*
 * STATIC int
 * adsc_hapresent(ulong port)
 *	Reset the HA board and return good status if it looks like a 1540
 *	else return bad status.
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_hapresent(ulong port)
{
	register int	i;
	/*
	 * If the control port is a 0xff, the adapter is not there.
	 * but, if it isn't a 0xff then,............
	 * reset the adapter and look at the status ports to determine if it
	 * looks like a adsc type of adapter.  If it does, then return good
	 * and keep on truckin.
	 * Regardless of the state of the 1540, there is no way the adapter
	 * status port will contain a 0xff, unless the adapter is really brain
	 * dead, in which case, it ain't gonna' work anyway.
	 */
	i = inb(port + HA_CNTL);
	if(i == 0xff)
		return(0);
	if (adsc_blc_get_model(port))
		return 0;
	outb(port + HA_CNTL, HA_HRST);

	drv_usecwait(4000000) ;

	i = inb(port + HA_STAT);

	if(!(i & HA_DIAGF) && (i & HA_IREQD)) {
		if (adsc_blc_get_model(port))
			return 0;
		outb(port+HA_CNTL, HA_IACK);
		return 1;
	}
	return(0);
}

/*
 * STATIC int
 * adsc_hainit(int c)
 *	This is the routine that does all the hardware initialization of the
 *	adapter after it has been determined to be in the system.  Must be
 *	called immediately after adsc_hapresent to insure the status of the 
 *	adaptor ports are not changed.
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_hainit(int c)
{
	register struct scsi_ha  *ha = &adsc_ha[c];
	register int	i;
	register char 	*ch;
	unsigned char	status, inquiry;
	unsigned long	addr;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_hainit(%x)\n", c);
#endif

	status = inb(ha->ha_base + HA_STAT);
	if(!(status & HA_DIAGF) && (status & HA_IREQD)) {
		/*
		 *      Check for a host adapter with extended bios enabled.
		 *      We do this by issuing the RXBIOS command which is only
		 *      valid on the 154xC or a 154xB with upgraded BIOS ROM
		 *       ( at least for now ).  We then check
		 *      the status byte to make sure the HA accepted the
		 *      command before proceeding.
		 *
		 *      If this command works, do the extra steps to
		 *      unlock the mailbox init command if required.
		 */
		outb((ha->ha_base + HA_CMD), HA_RXBIOS);
		drv_usecwait(1000);
		status = inb(ha->ha_base + HA_STAT);
		if(!(status & HA_INVDCMD)) {  /* if the RXBIOS command worked */
			status = inb(ha->ha_base + HA_DATA);
			drv_usecwait(1000);
			inquiry = inb(ha->ha_base + HA_DATA);
			drv_usecwait(1000);

			outb((ha->ha_base + HA_CNTL), HA_IACK);

			if ( inquiry ) {                /* a lock-code is set */
				outb((ha->ha_base + HA_CMD), HA_MBXENB);
				drv_usecwait(1000);
				outb((ha->ha_base + HA_CMD), 0x00);
				drv_usecwait(1000);
				outb((ha->ha_base + HA_CMD), inquiry);
				drv_usecwait(1000);

				outb((ha->ha_base + HA_CNTL), HA_IACK);
			}
		} else {
			outb((ha->ha_base + HA_CNTL), HA_IACK);
		}

		/* initialize the communication area */

		for (i = 0; i < NMBX; i++) {
			ha->ha_mbo[i].m_stat = EMPTY;
			ha->ha_mbi[i].m_stat = EMPTY;
		}

		addr = vtop((caddr_t)ha->ha_mbo, NULL);
		ch = (char *)&addr;

		outb((ha->ha_base + HA_CMD), HA_INIT);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CMD), NMBX);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CMD), ch[2]);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CMD), ch[1]);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CMD), ch[0]);
		drv_usecwait(1000);

		outb((ha->ha_base + HA_CNTL), HA_IACK);
		/*
		 * The bus-on time should be set to 7 micro-seconds.  This
		 * is the safest value to use without starving floppy DMA.
		 */
		outb((ha->ha_base + HA_CMD), HA_BONT);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CMD), adsc_buson);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CNTL), HA_IACK);

		/*
		 * By default the adapter is usually set to 4 micro-seconds
		 * bus-off time, but there have been various versions of the 
		 * adapter that have used different times.  
		 * This is just to insure the adapter is set correctly.
		 */
		outb((ha->ha_base + HA_CMD), HA_BOFFT);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CMD), adsc_busoff);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CNTL), HA_IACK);
		/*
		 * The default for the DMA rate is set via the jumpers 
		 * on the 1542, but can be overriden by software.  
		 * By default the DMA rate is set to 5.0MB/sec
		 * from the factory, but can be jumpered only up to 
		 * 8.0MB/sec.  Software can set the speed higher and 
		 * quite a few speeds inbetween.  See space.c
		 * for further explanations.
		 */
		outb((ha->ha_base + HA_CMD), HA_XFERS);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CMD), adsc_dma_rate);
		drv_usecwait(1000);
		outb((ha->ha_base + HA_CNTL), HA_IACK);

		status = inb(ha->ha_base + HA_STAT);
		if(!(status & HA_IREQD) && (status & HA_READY)) {
#ifdef AHA_DEBUG
	if(adscdebug > 3) { 
		cmn_err ( CE_CONT, "adsc_hainit: this HA is operational\n");
	}
#endif
			/*
			 *+ Adaptec Host Adaptor found at given address.
			 */
			cmn_err(CE_NOTE,
			    "!%s found at address 0x%X\n",
			     adscidata[c].name, ha->ha_base);
			/* mark this HA operational */
			ha->ha_state |= C_SANITY;
			return(1);
		}
	}
	/*
	 *+ Adaptec Host Adapter not found at given address.
	 */
	cmn_err(CE_CONT, "!%s %d NOT found at address 0x%X\n",
	    adscidata[c].name, adsc_ltog[c],ha->ha_base);

	return(0);
}

/*
 * STATIC int
 * adsc_dma_init(int c)
 *	Init the mother board dma channel for first party DMA.
 *	c is the location controller number
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_dma_init(int c)
{

	/* get the dma channel number from the configuration info */
	register ulong chan = (adscidata[c].dmachan1 & 0x7);

	/* if running in a micro-channel machine, do different initialization */
	if (adsc_sdi_ver.sv_machine == SDI_386_MCA) {
		/* set mask on of the needed channel */
		outb(adsc_dmac[chan].mca_amask,adsc_dmac[chan].mca_dmask);

	}
	else
	{
	/*
 	* If the DMA chan gets set to something other than 0, 5, 6, or 7, 
 	* something will go wrong.  I put in this as insurance, but you 
 	* may want to deal with it in another manner.  FIXIT?
 	*/

	if(chan > (unsigned int )0 && chan < 5) {
		/*
		 *+ DMA channels 1-4 not valid for Adaptec.
		 */
		cmn_err(CE_WARN,
		    "%s: DMA channel %d is not valid for the 154x",
		    adscidata[c].name, chan);
		return (-1);
	}

#ifndef PDI_SVR42
	if (dma_cascade(chan, DMA_ENABLE | DMA_NOSLEEP) == B_FALSE) {
		/*
		 *+  The DMA channel in question is already in use
		 *+ and is not in cascade mode.
		 */
		cmn_err(CE_WARN,
		    "%s: DMA channel %d is not available",
		    adscidata[c].name, chan);
		return (-1);
	}
#else   /* PDI_SVR42 */
	/* set mode of the needed channel */
	outb(adsc_dmac[chan].amode,adsc_dmac[chan].dmode);

	/* set mask off of the needed channel */
	outb(adsc_dmac[chan].amask,adsc_dmac[chan].dmask);
#endif  /* PDI_SVR42 */
	}
	return (0);
}
STATIC void
adsc_ckop_int(struct sb *sp)
{
	if(adsc_op == ADSC_OP)
		cmn_err ( CE_CONT, "!adsc_ckop_int: board does not support residual count");
	return;
}
/*
 * STATIC int
 * adsc_ckop(int c)
 * This routine sends a SS_TEST command to target ha_id, lun 0 at start time
 * just to see if this adapter supports the Command Control Block
 * operations: 0x03, 0x04 (return residual data length where appropriate).
 * 1542A and early 1542Bs did not support these.
 * If the interrupt completion gets invalid CCB opcode, the opcodes are
 * set back to 0x00, 0x02.
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_ckop(int c, int sleepflag)
{
	struct scsi_ha  *ha = &adsc_ha[c];
	struct sb *sp;
	struct scs *scsp;
	int t = ha->ha_id;
	int l = 0;

	sp = sdi_getblk(sleepflag);

	scsp = KMEM_ZALLOC(MAX_CMDSZ, sleepflag);
	/*
	 * Send SS_TEST command to adapter
	 */
	scsp->ss_op = SS_TEST;
	sp->SCB.sc_cmdpt = (caddr_t)scsp;
	sp->SCB.sc_cmdsz = SCS_SZ;

	sp->SCB.sc_dev.sa_lun = ( unsigned char )l;
	sp->SCB.sc_dev.sa_fill = (adsc_ltog[c] << 3) | t;

	sp->SCB.sc_int = adsc_ckop_int;
	sp->sb_type = SCB_TYPE;

	sdi_translate(sp, 0, NULL, sleepflag);
	sdi_icmd(sp, KM_SLEEP);
	/*
	adsc_cmd(SGNULL, (sblk_t *)((struct xsb *)sp)->hbadata_p, q);
	*/
	adsc_wait(500);
}

/*
 * STATIC int
 * adsc_illegal(short hba, uchar_t scsi_id, uchar_t lun)
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
adsc_illegal(short hba, uchar_t scsi_id, uchar_t lun)
{
	if(sdi_redt(hba, scsi_id, lun)) {
		return(0);
	} else {
#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_illegal(%d,%d,%d)\n",hba, scsi_id, lun);
#endif
		return(1);
	}
}


/*
 * STATIC sblk_t *
 * adsc_find_merge(struct scsi_lu * q, sblk_t *first, sblk_t *last)
 *	This routine is responsible for finding continguous disk requests.
 *	It assumes that jobs are stored in sorted order on the queue within
 *	a class.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK for q held on entry/exit
 */
STATIC sblk_t *
adsc_find_merge(struct scsi_lu *q, sblk_t *first, sblk_t *last)
{
	sblk_t *sp = q->q_first;
	unsigned int sblock = first->s_block;
	unsigned int eblock = last->s_block + ADSC_BSIZE(last, q) - 1;
	int cmd = ADSC_CMD(first);
	int cls = ADSC_QCLASS(first);

        while(sp && ADSC_QCLASS(sp) == cls && ADSC_IS_RW(ADSC_CMD(sp))) {
		if (!sp->s_dmap && cmd == ADSC_CMD(sp)) {

			unsigned int block = sp->s_block;
			unsigned int size = ADSC_BSIZE(sp, q);
			if (block == eblock + 1 || block + size == sblock)
				if (adsc_serial(q, sp))
					return sp;
		}
		sp = sp->s_next;
	} 
	return NULL;
}

/*
 * STATIC void
 * adsc_merge(struct scsi_lu *q, sblk_t *start_sp)
 *	This routine is responsible for merging continguous disk requests.
 *	It depends on the driver being able to queue commands for any given 
 *	target, as well as the adapter being able to support s/g type commands.
 *
 * Calling/Exit State:
 *	ADSC_SCSILU_LOCK for q held on entry
 *	no locks held on exit
 */
STATIC void
adsc_merge(struct scsi_lu *q, sblk_t *start_sp)
{
	SG *sg;
	sblk_t *sp = start_sp;
	char cmnd;
	long start_block, end_block, total_count;
	SGARRAY	*sg_p = NULL;
	int segments = 1;
	sblk_t *first, *last;


	ASSERT(start_sp);
	ASSERT(start_sp->sbp->sb.sb_type != SFB_TYPE);
	ASSERT(start_sp->s_dmap == NULL);

	total_count = start_sp->sbp->sb.SCB.sc_datasz;
	cmnd = *((char *)start_sp->sbp->sb.SCB.sc_cmdpt);
	if(!ADSC_IS_RW(cmnd))
		goto send_job;
	if ((sg_p = adsc_get_sglist()) == NULL) 
		goto send_job;

	first = last = start_sp;
	start_block = start_sp->s_block;
	end_block = start_block + (total_count>>q->q_shift) - 1;

	while(segments < SG_SEGMENTS) {
	      	if ((sp = adsc_find_merge(q, first, last)) == NULL)
			break;

		/* Have a job to merge */

		if (segments == 1) {
			/* Save information for first segment */
			sg_p->spcomms[0] = start_sp;
			sg = &sg_p->sg_seg[0];
			adsc_mkadr3(&sg->sg_size_msb, (uchar_t *)&total_count);
			adsc_mkadr3(&sg->sg_addr_msb, (uchar_t *)&start_sp->s_addr);
		}
		adsc_getq(q, sp);
		total_count += sp->sbp->sb.SCB.sc_datasz;
		if (sp->s_block > end_block) {
			/* Merge at end of S/G list */

			sg_p->spcomms[segments] = sp;
			sg = &sg_p->sg_seg[segments];
			last = sp;
			end_block = start_block + (total_count>>q->q_shift) - 1;
		} else {
			/* Merge before existing S/G list */
			int i;
			for (i = segments; i>0; i--) {
				sg_p->spcomms[i] = sg_p->spcomms[i-1];
				sg_p->sg_seg[i] = sg_p->sg_seg[i-1];
			}
			sg_p->spcomms[0] = sp;
			sg = &sg_p->sg_seg[0];
			start_block = sp->s_block;
			first = sp;
		}
		adsc_mkadr3(&sg->sg_size_msb, (uchar_t *)&sp->sbp->sb.SCB.sc_datasz);
		adsc_mkadr3(&sg->sg_addr_msb, (uchar_t *)&sp->s_addr);
		segments++;
	}
	/* Prepare to send single job or S/G list */
	if(segments == 1) {		
		/* No merges done; Send the job normally */
		sg_p->sg_flags = SG_FREE;
		sg_p = SGNULL;
		sp = start_sp;
	} else {
		sg_p->sg_len = SG_LENGTH * segments;
		sg_p->dlen = total_count;
		sg_p->addr = vtop((caddr_t)&sg_p->sg_seg[0].sg_size_msb, NULL);
		sp = NULL;
	}
	q->q_addr = end_block;

send_job:
#ifdef AHA_DEBUGSG1
	if (ADSC_IS_RW(cmnd)) adsc_merge_debug(sg_p, segments);
#endif
	q->q_outcnt++;
	if(q->q_outcnt >= adsc_hiwat)
		q->q_flag |= ADSC_QBUSY;
	ADSC_SCSILU_UNLOCK(q->q_opri);
	adsc_cmd(sg_p, sp, q);
}

#ifdef AHA_DEBUGSG1
/*
 * STATIC void
 * adsc_merge_debug(SGARRAY *sg, int segs)
 *	Print out statistics and debugging information for merged jobs.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_merge_debug(SGARRAY *sg, int segs)
{
	int i = 0;
	static int trials;
	static int segments;
	int dec;
	trials++;
	segments += segs;
	if (trials % 100 == 0)  {
		dec = (segments % trials) * 100/trials;
		if (dec >= 10)
			cmn_err(CE_CONT, "jobs per I/O: %d.%d\n", segments/trials, dec);
		else
			cmn_err(CE_CONT, "jobs per I/O: %d.0%d\n", segments/trials, dec);
	}
	if (segs == 1 || !sg)
		return;
		
	cmn_err(CE_CONT, "Merged %d jobs\n", segs);
	while (i < segs) {
		cmn_err(CE_CONT, "\tblock: %d\tlength: %d\n",
			adsc_getadr(sg->spcomms[i]),
			sg->spcomms[i]->sbp->sb.SCB.sc_datasz);
		i++;
	}
}
#endif

/*
 * STATIC SGARRAY *
 * adsc_get_sglist(void)
 *	Here is the routine that returns a pointer to the next free
 *	s/g list.  Each list is 16 segments long.  I have only allocated
 *	NMBX number of these structures, instead of doing it on a per
 *	adapter basis.  Doesn't hurt if it returns a NULL as it just means
 *	the calling routine will just have to do it the old-fashion way.
 *	This routine runs just like adsc_getccb, which is commented very
 *	well.  So see those comments if you need to.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SGARRAY_LOCK.
 */
STATIC SGARRAY *
adsc_get_sglist(void)
{
	register SGARRAY	*sg;
	register int		cnt;
	int			opri;

	ADSC_SGARRAY_LOCK(opri);
	sg = sg_next;
	cnt = 0;

	for(;;) {
		if(sg->sg_flags == SG_BUSY) {
			cnt++;
			if(cnt >= NMBX) {
#ifdef AHA_DEBUGSG
				cmn_err ( CE_CONT, "NO S/G Segments>\n");
#endif
				return(SGNULL);
			}
			sg = sg->sg_next;
		} else {
			break;
		}
	}
	sg->sg_flags = SG_BUSY;
	sg_next = sg->sg_next;
	ADSC_SGARRAY_UNLOCK(opri);
	return(sg);
}

/*
 * STATIC void
 * adsc_cmd(SGARRAY *sg_p, sblk_t *sp, struct scsi_lu *q)
 *	Create and send an SCB associated command, using a scatter gather
 *	list created by adsc_merge(). 
 *	I have re-written it to take sufficient arguments to do a local
 *	s/g command.  The code favors this type of command as there is more
 *	overhead to build a s/g command anyway, so it should balance out.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires ADSC_SCSILU_LOCK(q->q_opri).
 */
STATIC void
adsc_cmd(SGARRAY *sg_p, sblk_t *sp, struct scsi_lu *q)
{
	register struct scsi_ad *sa;
	register struct ccb	*cp;
	register int		i;
	char			*p;
	long			bcnt, cnt ;
	int			c, t;
	paddr_t			addr;

#ifdef AHA_DEBUG
	if(adscdebug > 2)
		cmn_err ( CE_CONT, "adsc_cmd(%x)\n", sp);
#endif

	if(sg_p != SGNULL)
		sp = sg_p->spcomms[0];

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = adsc_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = adsc_getccb(c);
	cp->c_bind = &sp->sbp->sb;

	/* fill in the controller command block */

	cp->c_dev = (char)((t << 5) | sa->sa_lun);
	cp->c_hstat = 0;
	cp->c_tstat = 0;
	cp->c_cmdsz = sp->sbp->sb.SCB.sc_cmdsz;
	cp->c_sensz = sizeof(cp->c_sense);

	/*
	 * If the sg_p pointer is not NULL or the sp->s_dmap contains a valid
	 * address, we have a s/g command to do.
	 * [1] Set the ccb opcode to a s/g type.
	 * [2] If s_dmap is valid, then set the length and addr.  If it is
	 * 	not valid, the length and addr will have been passed in the
	 *	call to adsc_cmd, to indicate the driver built a local s/g
	 *	command. Setting the c_sg_p to a NULL will allow the interrupt
	 *	to recognize this command as a normal I/O or a page I/O s/g
	 *	command.
	 * [3] Must be a normal command. (shucks)
	 * [4] Set up the ccb for a normal command, set the length and addr.
	 */
	if(sp->s_dmap || sg_p != SGNULL) {
#ifdef AHA_DEBUGSGK
		cmn_err ( CE_CONT, "S/G command ");
#endif
		cp->c_opcode = adsc_op_dma;		/* [1] */
		if(sp->s_dmap) {			/* [2] */
#ifdef AHA_DEBUGSGK
			cmn_err ( CE_CONT, "KERNEL BUILT>\n");
#endif
			cnt = sp->s_dmap->d_size;
			addr = sp->s_addr;
			cp->c_sg_p = SGNULL;
		} else {
			cnt = sg_p->sg_len;
			addr = sg_p->addr;
#ifdef AHA_DEBUGSGK
			cmn_err ( CE_CONT, "HOMEMADE>\n");
#endif
		}
	} else {					/* [3] */
		cp->c_opcode = adsc_op;		/* [4] */
		cnt = sp->sbp->sb.SCB.sc_datasz;
		addr = sp->s_addr;
		cp->c_sg_p = SGNULL;
	}

	adsc_mkadr3(&cp->c_datasz[0], (uchar_t *)&cnt);
	adsc_mkadr3(&cp->c_datapt[0], (uchar_t *)&addr);

	/* copy SCB cdb to controller CB cdb */
 	p = sp->sbp->sb.SCB.sc_cmdpt;
	for (i = 0; i < sp->sbp->sb.SCB.sc_cmdsz; i++)
		cp->c_cdb[i] = *p++;

	/*
	 * [1]  Have to save the pointer to the s/g list, if it was built
	 *	locally so it can be released at interrupt time.
	 * [2]  Go ahead and divide by sector size to get the actual block 
	 *	count for the SCSI command.
	 * [3]  If the command was a locally built s/g command, we have to munge
	 *	the actual number of blocks requested in the SCSI command.
	 * [3a] I don't know if I should PANIC here or not.  If this code can be
	 *	reached and *not* be a READ or WRITE, then in adsc_merge we
	 *	should explicitly look for READ or WRITE as part of the
	 *	determining factor whether a s/g command can be done or not.
	 */
	if(sg_p != SGNULL) {
		cp->c_sg_p = sg_p;				/* [1] */
		bcnt = sg_p->dlen >> q->q_shift;
		switch(cp->c_cdb[0]) {				/* [3] */
			case SS_READ:
				cp->c_cdb[4] = (unsigned char)bcnt;
				cp->c_dev |= HA_RD_DIR;
				break;
			case SM_READ:
				cp->c_cdb[7] =
				    (unsigned char)((bcnt >> 8) & 0xff);
				cp->c_cdb[8] =
				    (unsigned char)(bcnt & 0xff);
				cp->c_dev |= HA_RD_DIR;
				break;
			case SS_WRITE:
				cp->c_cdb[4] = (unsigned char)bcnt;
				cp->c_dev |= HA_WRT_DIR;
				break;
			case SM_WRITE:
				cp->c_cdb[7] =
				    (unsigned char)((bcnt >> 8) & 0xff);
				cp->c_cdb[8] =
				    (unsigned char)(bcnt & 0xff);
				cp->c_dev |= HA_WRT_DIR;
				break;
			default:				/* [3a] */
				/*
			 	 *+ Adaptec unknown SCSI command for scatter/gather.
			 	 */
				cmn_err(CE_PANIC,
				    "%s: Unknown SCSI command for s/g\n",
				    adscidata[c].name);
				break;
		}
	}

	ADSC_SCSILU_LOCK(q->q_opri);
	if ((q->q_flag & ADSC_QSENSE) && (cp->c_cdb[0] == SS_REQSEN)) {
		int	copyfail ;
		caddr_t	toaddr ;
#ifdef AHA_DEBUGSG
		/*
		 *+ 
		 */
		cmn_err ( CE_CONT, "REQUEST SENSE COMMAND>\n");
#endif
		copyfail = 0 ;

		q->q_flag &= ~ADSC_QSENSE;
		ADSC_SCSILU_UNLOCK(q->q_opri);

		/*
		 * If the REQ SENSE data is going to a buffer within kernel
		 * space, then the data is just copied.  Otherwise, the
		 * data is going to a user-space buffer (Pass-thru) so the
		 * data must be copied to the individual segments of
		 * contiguous physical memory that make up the user's buffer.
		 * Note - The buffer address sent by the user
		 * (sp->sbp->sb.SCB.sc_datapt) can not be used directly 
		 * since this routine may be executing on the interrupt thread.
		 */
 		if (cp->c_opcode != adsc_op_dma) {
#ifdef AHA_DEBUGSG
			/*
		 	*+ 
		 	*/
			cmn_err ( CE_CONT, "Non-S/G type>\n");
#endif
			if (toaddr = physmap(sp->s_addr, cnt, KM_NOSLEEP))
			{
 				bcopy((caddr_t)(&q->q_sense)+1, toaddr, cnt);
#ifdef AHA_DEBUGSG
			/*
		 	*+ 
		 	*/
			cmn_err ( CE_CONT, "BCOPY DONE>\n");
#endif
				physmap_free(toaddr, cnt, 0);
			} else
				copyfail++ ;
 		} else {
			struct dma_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src;
			caddr_t	optr, Dest;
			int	Count;
		 
#ifdef AHA_DEBUGSG
			/*
		 	*+ 
		 	*/
			cmn_err ( CE_CONT, "S/G type>\n");
#endif
			if (VectPtr = (struct dma_vect *) physmap( sp->s_addr, 
				cnt, KM_NOSLEEP))
			{
				optr = (caddr_t)VectPtr;
 				VectCnt = cnt / sizeof(struct dma_vect);
 				Src = (caddr_t)(&q->q_sense) + 1;
 				for (i=0; i < VectCnt; i++) {
 					Dest = (caddr_t)(VectPtr->d_addr[0] << 16 |
 						VectPtr->d_addr[1] << 8 |
 						VectPtr->d_addr[2]);
					Count = (int)(VectPtr->d_cnt[0] << 16 |
 				 		VectPtr->d_cnt[1] << 8 |
 				 		VectPtr->d_cnt[2]);
 
					if ( Dest = (caddr_t)physmap((unsigned long) Dest, 
						Count, KM_NOSLEEP))
					{
 						bcopy (Src, Dest, Count);
 						Src += Count;
 						VectPtr++;
						physmap_free(Dest, Count, 0);
					} else {
						copyfail++ ;
						break ;
					}
 				}
				physmap_free(optr, cnt, 0);
 			} else
				copyfail++ ;
		}
#ifdef AHA_DEBUGSG
		cmn_err ( CE_CONT, "Calling adsc_done>\n");
#endif
	adsc_done(SG_START, q, cp, copyfail?RETRY:SUCCESS, cp->c_bind, cnt);
		adsc_waitflag = FALSE;
		return;
	} else
		ADSC_SCSILU_UNLOCK(q->q_opri);

	adsc_send(c, START, cp);   /* send cmd */
}

/*
 * STATIC long
 * adsc_tol(char adr[])
 *	This routine flips a 4 byte Intel ordered long to a 4 byte SCSI/
 *	Adaptec style long.
 *
 * Calling/Exit State:
 *	None
 */
STATIC long
adsc_tol(char adr[])
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

/*
 * STATIC void
 * adsc_mkadr3(uchar_t str[], uchar_t adr[])
 *	This is a handy dandy routine to convert the lower three bytes of
 *	a long to the backwards 3 byte structure the 1542 uses.  Yes I know it's
 *	dumb that it should have to be done anyway.  But it would either have to
 *	be done on the adapter or by the host CPU, and the host CPU has quite a
 *	bit more power than our poor little 8085.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_mkadr3(uchar_t str[], uchar_t adr[])
{
	str[0] = adr[2];
	str[1] = adr[1];
	str[2] = adr[0];
}

/*
 * STATIC void
 * adsc_sgdone(struct scsi_lu *q, struct ccb *cp, int status)
 *	This routine is used as a front end to calling adsc_done.
 *	This is the cleanest way I found to cleanup a local s/g command.
 *	One should note, however, with this scheme, if an error occurs
 *	during the local s/g command, the system will think an error will
 *	have occured on all the commands.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_sgdone(struct scsi_lu *q, struct ccb *cp, int status)
{
	register sblk_t		*sb;
	register struct sb	*sp;
	register int		i;
	int			r;
	int			nsg;
	SGARRAY			*sg_p;
	long			total_resid, bcnt, total_trans;
	long			resid = 0;

	if(cp->c_opcode == adsc_op_dma && cp->c_sg_p != SGNULL) {
#ifdef AHA_DEBUGSG
		long	x;
		x = cp->c_datasz[0];
		x <<= 8;
		x |= cp->c_datasz[1];
		x <<= 8;
		x |= cp->c_datasz[2];
		x /= 6;
		cmn_err ( CE_CONT, "T%d -", x);
#endif
		/* Disengage sg_p from ccb, since ccb is freed in done */
		sg_p = cp->c_sg_p;
		cp->c_sg_p = SGNULL;

		/* get number of sg entries and residual count.
		 * Calculate the total bytes transfered.
		 */
		nsg = sg_p->sg_len/(sizeof(SG));
		total_resid = 0;
		adsc_mkadr3((uchar_t *)&total_resid, cp->c_datasz);
		total_trans = sg_p->dlen - total_resid;

		r = SG_NOSTART;
		for(i = 0; i < nsg; i++) {
			sb = sg_p->spcomms[i];
#ifdef AHA_DEBUGSG
			ASSERT(sb);
#endif
			if((i + 1) == nsg)
				r = SG_START;
			if (total_resid) {
				/*
				 * There was a residual count...calculate
				 * for this part of the job
				 */
				bcnt = 0;
				adsc_mkadr3((uchar_t *)&bcnt, 
					&sg_p->sg_seg[i].sg_size_msb);
				if (total_trans >= bcnt)
					total_trans -= bcnt;
				else {
					resid = bcnt - total_trans;
					total_trans = 0;
				}
			}
			adsc_done(r, q, cp, status, &sb->sbp->sb, resid);
#ifdef AHA_DEBUGSG
			sg_p->spcomms[i] = ((sblk_t *)0);
			cmn_err ( CE_CONT, " %d", i);
#endif
		}
#ifdef AHA_DEBUGSG
		cmn_err ( CE_CONT, "n");
#endif
		sg_p->sg_flags = SG_FREE;
	} else { /* call target driver interrupt handler */
#ifdef AHA_DEBUGSGK
		if(cp->c_opcode == adsc_op_dma)
			cmn_err ( CE_CONT, "Kernel S/G\n");
#endif
		sp = cp->c_bind;
		adsc_mkadr3((uchar_t *)&resid, cp->c_datasz);
		adsc_done(SG_START, q, cp, status, sp, resid);
	}
}


#ifndef PDI_SVR42
/*
 * STATIC void
 * adsc_lockinit(int c)
 *	Initialize adsc locks:
 *		1) device queue locks (one per queue),
 *		2) dmalists lock (one only),
 *		3) ccb lock (one only),
 *		4) scatter/gather lists lock (one only),
 *		5) and controller mailbox queue lock (one per controller).
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_lockinit(int c)
{
	register struct scsi_ha  *ha;
	register struct scsi_lu	 *q;
	register int	t,l;
	int		sleepflag;
	static		firsttime = 1;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_lockinit()\n");
#endif
	sleepflag = adsc_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if (firsttime) {

		adsc_dmalist_sv = SV_ALLOC (sleepflag);
		adsc_dmalist_lock = LOCK_ALLOC (ADSC_HIER, pldisk, &lkinfo_dmalist, sleepflag);

		adsc_sgarray_lock = LOCK_ALLOC (ADSC_HIER+1, pldisk, &lkinfo_sgarray, sleepflag);

		adsc_ccb_lock = LOCK_ALLOC (ADSC_HIER, pldisk, &lkinfo_ccb, sleepflag);
		firsttime = 0;
	}

	ha = &adsc_ha[c];
	ha->ha_mbx_lock = LOCK_ALLOC (ADSC_HIER, pldisk, &lkinfo_mbx, sleepflag);
	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			q = &LU_Q (c,t,l);
			q->q_lock = LOCK_ALLOC (ADSC_HIER, pldisk, &lkinfo_q, sleepflag);
		}
	}
}

/*
 * STATIC void
 * adsc_lockclean(int c)
 *	Removes unneeded locks.  Controllers that are not active will
 *	have all locks removed.  Active controllers will have locks for
 *	all non-existant devices removed.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_lockclean(int c)
{
	register struct scsi_ha  *ha;
	register struct scsi_lu	 *q;
	register int	t,l;
	static		firsttime = 1;

#ifdef AHA_DEBUG
	if(adscdebug > 0)
		cmn_err ( CE_CONT, "adsc_lockclean(%d)\n", c);
#endif
	if (firsttime && !adsc_hacnt) {

		if (adsc_dmalist_sv == NULL)
			return;
		SV_DEALLOC (adsc_dmalist_sv);
		LOCK_DEALLOC (adsc_dmalist_lock);
		LOCK_DEALLOC (adsc_sgarray_lock);
		LOCK_DEALLOC (adsc_ccb_lock);
		firsttime = 0;
	}

	if( !adscidata[c].active) {
		ha = &adsc_ha[c];
		if (ha->ha_mbx_lock == NULL)
			return;
		LOCK_DEALLOC (ha->ha_mbx_lock);
	}

	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			if (!adscidata[c].active || adsc_illegal(adsc_ltog[c],
			    t, l)) {
				q = &LU_Q (c,t,l);
				LOCK_DEALLOC (q->q_lock);
			}
		}
	}
}
#else /* !PDI_SVR42 */

/*
 * STATIC void
 * adsc_lockinit(int c)
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_lockinit(int c)
{
}

/*
 * STATIC void
 * adsc_lockclean(int c)
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
adsc_lockclean(int c)
{
}
#endif /* PDI_SVR42 */

#ifndef PDI_SVR42
/*
 * STATIC void *
 * adsc_kmem_zalloc_physreq(size_t size, int flags)
 *	function to be used in place of kma_zalloc
 *	which allocates contiguous memory, using kmem_alloc_physreq,
 *	and zero's the memory.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void *
adsc_kmem_zalloc_physreq(size_t size, int flags)
{
	void *mem;
	physreq_t *preq;

	preq = physreq_alloc(flags);
	if (preq == NULL)
		return NULL;
	preq->phys_align = ADSC_MEMALIGN;
	preq->phys_boundary = ADSC_BOUNDARY;
	preq->phys_dmasize = ADSC_DMASIZE;
	preq->phys_flags |= PREQ_PHYSCONTIG;
	if (!physreq_prep(preq, flags)) {
		physreq_free(preq);
		return NULL;
	}
	mem = kmem_alloc_physreq(size, preq, flags);
	physreq_free(preq);
	if (mem)
		bzero(mem, size);

	return mem;
}
#endif /* !PDI_SVR42 */

#ifdef AHA_DEBUG
/*
 * void
 * adsc_queck(struct scsi_lu *q)
 *	debugging function to check validity of device queue.
 *
 * Calling/Exit State:
 *	None
 */
void
adsc_queck(struct scsi_lu *q)
{
	sblk_t *sp;

	if(sp = q->q_first) {
		if (q->q_last == NULL) {
			/*
			 *+ adsc_queck: error found in a device queue.
			 *+ There is a first job and no last job!
			 */
			cmn_err(CE_NOTE,"adsc: q_first %x, q_last NULL",sp);
			adsc_brk();
		}
		if (sp->s_prev) {
			/*
			 *+ adsc_queck: error found in a device queue.
			 *+ First job on queue pointing to a non-existent 
			 *+ previous job.
			 */
			cmn_err(CE_NOTE,"adsc: s_prev on q_first, sp %x",sp);
			adsc_brk();
		}
		while (sp) {
			if (sp->s_next) {
				if (sp->s_next->s_prev != sp) {
					/*
					 *+ adsc_queck: error found in a device queue.
					 *+ Broken chain.
					 */
					cmn_err(CE_NOTE,"adsc: sp != sp->s_next->s_prev, sp %x\n", sp);
					adsc_brk();
				}
			}
			sp = sp->s_next;
		}
	}
}

/*
 * void
 * adsc_brk()
 *	debugging function called by adsc_queck() for kdb handle.
 *
 * Calling/Exit State:
 *	None
 */
void
adsc_brk(void)
{
	/* For AHA_DEBUG:
	 *	Handle for kdb breakpoint when queue check, adsc_queck,
	 *	fails.
	 */
}
#endif /* AHA_DEBUG */

#define POS_SIZE	6
/*
 * adsc_mca_conf(HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Read the MCA POS registers to get the hardware configuration.
 */
int
adsc_mca_conf(HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen)
{
	unsigned	char	pos[POS_SIZE];

	if (cm_read_devconfig(idp->idata_rmkey, 0, pos, POS_SIZE) != POS_SIZE) {
		cmn_err(CE_CONT,
			"!%s could not read POS registers for MCA device",
			idp->name);
		return (1);
	}

	/*
	 * Interpret POS data and populate idp accordingly.
	 */

	idp->ioaddr1	= (ulong)((((pos[3] & 0x03) << 8) | \
						0x30 | ((pos[3] & 0x0c) >> 4))) ;

	idp->iov		= (int)((pos[4] & 0x07) | 0x08);

	idp->dmachan1	= (int)(pos[5] & 0x0f);

	idp->idata_memaddr = (int)(((pos[3] & 0x38) << 11) | 0xC0000);

	idp->ha_id = (int)((pos[4] & 0xe0) >> 5);

	*ioalen = 4;
	*memalen = 2;

	return (0);
}

/* The following two routines check for a BusLogic adapter instead of an
 * Adaptec.
 */
STATIC int 
adsc_blc_wait(ushort port, ushort mask, ushort onbits, ushort offbits )
{
	register int    i, maskval;

	drv_usecwait(1000);
	for( i = 200000; i; i-- )
	{
		maskval = inb(port) & mask;
		if((maskval & onbits) == onbits && !(maskval & offbits))
			return(0);

		drv_usecwait(10);
	}
	return(1);
}

#define BLC_Adapter_Ready     0x10
#define BLC_Cmd_Parm_Busy     0x08
#define BLC_Cmd_Invalid       0x01
#define BLC_DataIn_Ready      0x04
#define BLC_Cmd_Complete      0x04

#define BLC_Get_Model	      0x8b

/* Return non-zero if blc responds */

STATIC int 
adsc_blc_get_model(int port)
{
	int i;
	char buf[4];
	int opl = spldisk();
	/*
	**  Make sure the host adapter is ready.
	*/
	if( adsc_blc_wait(port+HA_STAT, (BLC_Adapter_Ready | BLC_Cmd_Parm_Busy),
		BLC_Adapter_Ready, BLC_Cmd_Parm_Busy) )
	{
		cmn_err(CE_CONT, "!adsc: adapter busy or not ready (%x)",
		(int) inl(port+HA_STAT));
		splx(opl);
		return(0);
	}
	outb(port+HA_CMD, BLC_Get_Model);
	if( adsc_blc_wait(port+HA_STAT, (BLC_Cmd_Parm_Busy | BLC_Cmd_Invalid), 0,
		(BLC_Cmd_Parm_Busy | BLC_Cmd_Invalid)) )
	{
		cmn_err(CE_CONT, "!adsc: adapter busy or invalid cmd");
		splx(opl);
		return(0);
	}
	outb(port+HA_CMD, 4);
	if( adsc_blc_wait(port+HA_STAT, BLC_Cmd_Invalid, 0, BLC_Cmd_Invalid) )
	{
		cmn_err(CE_CONT, "!adsc: invalid cmd");
		splx(opl);
		return(0);
	}
	for (i=0; i<4; i++) {
		if( adsc_blc_wait(port+HA_STAT, BLC_DataIn_Ready,
			BLC_DataIn_Ready, 0) )
		{
			cmn_err(CE_CONT, "!adsc: no data");
			splx(opl);
			return(0);
		}
		buf[i] = inb(port+HA_DATA);
	} 

	if( adsc_blc_wait(port+HA_ISR, BLC_Cmd_Complete, BLC_Cmd_Complete, 0) )
	{
		cmn_err(CE_CONT, "!adsc: no cmd complete");
		splx(opl);
		return(0);
	}

	/*
	**  Reset interrupt.
	*/
	outb(port+HA_CNTL, HA_IACK);
	splx(opl);
	return(1);
}

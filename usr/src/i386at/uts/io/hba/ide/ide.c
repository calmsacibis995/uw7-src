#ident	"@(#)kern-pdi:io/hba/ide/ide.c	1.14.4.2"

#ifdef	_KERNEL_HEADERS

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <acc/priv/privilege.h>
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
#include <io/hba/ide/ide.h>
#include <io/hba/ide/ide_ha.h>

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
#include <sys/privilege.h>
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
#include "ide.h"
#include "ide_ha.h"

/* These must come last: */
#include <sys/hba.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>

#endif			/* _KERNEL_HEADERS */

#if PDI_VERSION != PDI_UNIXWARE11 && PDI_VERSION < PDI_SVR42MP
#error Incorrect PDI version number detected, build cannot continue.
#endif

#define KMEM_ZALLOC ide_kmem_zalloc_physreq
#define KMEM_FREE kmem_free

STATIC void *ide_kmem_zalloc_physreq(size_t , physreq_t *, int);
STATIC void	ide_pass_thru0(buf_t *);

STATIC struct control_area *ide_getblk(int);

STATIC int	ide_dmalist(sblk_t *, struct proc *, int),
		ide_ha_init(struct ata_ha *, int, int),
		ide_illegal(short , int, uchar_t , uchar_t),
		ide_wait(int),
		ide_find_ha(int, int *, int *),
		ide_mca_conf(HBA_IDATA_STRUCT *, int *, int *);

STATIC void		ide_flushq(struct scsi_lu *, int, int),
		ide_pass_thru(struct buf *),
		ide_init_scbs(struct ata_ha *, struct req_sense_def *),
		ide_int(struct sb *),
		ide_putq(struct scsi_lu *, sblk_t *),
		ide_next(struct scsi_lu *),
		ide_restart(struct scsi_lu *),
		ide_func(sblk_t *),
		ide_lockclean(int),
		ide_lockinit(int),
		ide_send(int, struct control_area *),
		ide_cmd(sblk_t *),
		ide_done(struct scsi_lu *, struct control_area *, int),
		ide_sfb_done(struct scsi_lu *, struct control_area *, int),
		ide_mem_release(struct ata_ha **, int);

STATIC int ide_serial(struct scsi_lu *, sblk_t *, sblk_t *);
STATIC sblk_t * ide_schedule(struct scsi_lu *);
STATIC void ide_getq(struct scsi_lu *, sblk_t *);

boolean_t	ideinit_time;	    /* Init time (poll) flag	*/
STATIC boolean_t	ide_waitflag = TRUE;	/* Init time wait flag, TRUE	*/
					/* until interrupt serviced	*/
STATIC int  		ide_num_scbs,
			ide_habuscnt,
			ide_dma_listsize,
			ide_req_structsize,
			ide_ha_size,
			ide_luq_structsize,
			ide_scb_structsize;
STATIC int  		ide_mod_dynamic = 0;
struct ata_ha	*ide_sc_ha[SDI_MAX_HBAS * IDE_MAX_BPC]; /* SCSI bus structs */
STATIC int		ide_ctob[SDI_MAX_HBAS];  /* Controller number to */
						  /* bus number */

STATIC sv_t   *ide_dmalist_sv;	/* sleep argument for UP	*/
STATIC int	ide_devflag = 0;
STATIC lock_t *ide_dmalist_lock;
STATIC LKINFO_DECL(ide_lkinfo_dmalist, "IO:ide:ide_dmalist_lock", 0);
STATIC LKINFO_DECL(ide_lkinfo_HIM, "IO:ide:ha->ha_HIM_lock", 0);
STATIC LKINFO_DECL(ide_lkinfo_scb, "IO:ide:ha->ha_scb_lock", 0);
STATIC LKINFO_DECL(ide_lkinfo_npend, "IO:ide:ha->ha_npend_lock", 0);
STATIC LKINFO_DECL(ide_lkinfo_q, "IO:ide:q->q_lock", 0);

/*
 *  ide_job_done is a routine defined in the ide.c file that is used by
 *	the lower layer to complete a SCSI job in the PDI layer.  For now,
 *	the lower layer depends on the name being ide_job_done.
 */

void	ide_job_done(struct control_area *);

/* Allocated in space.c */
extern struct ver_no	ide_sdi_ver;	    /* SDI version structure	*/

extern int ide_table_entries;
extern struct ide_cfg_entry ide_table[];

extern int		ide_cntls;
extern HBA_IDATA_STRUCT	_ideidata[];
extern int		ide_gtol[]; /* xlate global hba# to loc */
extern int		ide_ltog[]; /* local hba# to global     */

/*
 * The name of this pointer is the same as that of the
 * original idata array. Once it it is assigned the
 * address of the new array, it can be referenced as
 * before and the code need not change.
 */
HBA_IDATA_STRUCT	*ideidata;

/* The sm_poolhead struct is used for the dynamic struct allocation routines */
/* as pool to extract from and return structs to. This pool provides 28 byte */
/* structs (the size of sblk_t structs roughly).			     */
extern struct head	sm_poolhead;

#ifdef ATA_DEBUG1
int			idedebug = 0;
#endif

#ifdef DEBUG
STATIC	int	ide_panic_assert = 0;
#endif

#ifdef ATA_DEBUG_TRACE
#define ATA_DTRACE cmn_err(CE_NOTE,"%d-%s",__LINE__,__FILE__);
#else
#define ATA_DTRACE
#endif

#define		DRVNAME	"IDE/ATAPI/ESDI HBA driver"
#define		IDE_IO_PER_CONTROLLER	1

/*
#define IDE_BLKCNT(sp, q) (sp->sbp->sb.SCB.sc_datasz>>q->q_shift)
*/
#define IDE_BLKCNT(sp, q) (sp->sbp->sb.SCB.sc_datasz>>9)
#define IDE_IS_READ(c)         (c == SM_READ)
#define IDE_IS_WRITE(c)        (c == SM_WRITE)
#define IDE_IS_RW(c) (IDE_IS_READ(c) || IDE_IS_WRITE(c))
#define IDE_CMDPT(sp) (*((char *)sp->sbp->sb.SCB.sc_cmdpt))
#define IDE_IS_DISK(q) (q->q_ha->ha_config->cfg_drive[q->q_ha->ha_config->cfg_curdriv].dpb_devtype == DTYP_DISK)

MOD_ACHDRV_WRAPPER(ide, ide_load, ide_unload, idehalt, NULL, DRVNAME);
HBA_INFO(ide, &ide_devflag, 0x10000);

int
ide_load(void)
{
	ATA_DTRACE;

	ide_mod_dynamic = 1;

	if (ideinit()) {
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
		sdi_acfree(ideidata, ide_cntls);
		return( ENODEV );
	}
	if (idestart()) {
		/*
		 * At this point we have allocated a new idata array,
		 * free it before failing.
		 */
		sdi_acfree(ideidata, ide_cntls);
		return(ENODEV);
	}
	return(0);
}

int
ide_unload(void)
{
	ATA_DTRACE;

	return(EBUSY);
}

/*
 * Function name: ide_dma_freelist()
 * Description:
 *	Release a previously allocated scatter/gather DMA list.
 */
STATIC void
ide_dma_freelist(sblk_t *sp)
{
	pl_t		oip;
	dma_t	       *dmap = sp->s_dmap;
	struct scsi_ad *sa = &sp->sbp->sb.SCB.sc_dev;
	int		c = ide_gtol[SDI_EXHAN(sa)];
	int		b = SDI_BUS(sa);
	int		gb = ide_ctob[c] + b;
	struct ata_ha  *ha = ide_sc_ha[gb];

	ATA_DTRACE;
	ASSERT(dmap);

	IDE_DMALIST_LOCK(oip);
	dmap->d_next = ha->ha_dfreelistp;
	ha->ha_dfreelistp = dmap;
	if (dmap->d_next == NULL) {
		IDE_DMALIST_UNLOCK(oip);
		SV_BROADCAST(ide_dmalist_sv, 0);
	} else
		IDE_DMALIST_UNLOCK(oip);
}

/*
 * Function name: ide_dmalist()
 * Description:
 *	Build the physical address(es) for DMA to/from the data buffer.
 *	If the data buffer is contiguous in physical memory, sp->s_addr
 *	is already set for a regular SB.  If not, a scatter/gather list
 *	is built, and the SB will point to that list instead.
 */
STATIC int
ide_dmalist(sblk_t *sp, struct proc *procp, int sleepflag)
{
	struct dma_vect			tmp_list[SG_SEGMENTS];
	struct dma_vect  	*pp;
	dma_t			*dmap;
	long			count, fraglen, thispage;
	caddr_t				vaddr;
	paddr_t				addr, base;
	pl_t				oip;
	int				i;
#ifdef ATA_DEBUGSG_
	long				datal = 0;
#endif

	ATA_DTRACE;

	vaddr = sp->sbp->sb.SCB.sc_datapt;
	count = sp->sbp->sb.SCB.sc_datasz;
	pp = &tmp_list[0];

	/* First build a scatter/gather list of physical addresses and sizes */

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
		pp->d_addr = base;
		pp->d_cnt = fraglen;
#ifdef ATA_DEBUGSG_
		datal += fraglen;
#endif
	}
	if (count != 0)
		cmn_err(CE_PANIC, "Job too big for DMA list");

	/*
	 * If the data buffer was contiguous in physical memory,
	 * there was no need for a scatter/gather list; We're done.
	 */
	if (i > 1) {
		struct scsi_ad *sa = &sp->sbp->sb.SCB.sc_dev;
		int		c = ide_gtol[SDI_EXHAN(sa)];
		int		b = SDI_BUS(sa);
		int		gb = ide_ctob[c] + b;
		struct ata_ha  *ha = ide_sc_ha[gb];
		/*
		 * We need a scatter/gather list.
		 * Allocate one and copy the list we built into it.
		 */
		IDE_DMALIST_LOCK(oip);
		if (!ha->ha_dfreelistp && (sleepflag == KM_NOSLEEP)) {
			IDE_DMALIST_UNLOCK(oip);
			return (1);
		}
		while ( !(dmap = ha->ha_dfreelistp)) {
			SV_WAIT(ide_dmalist_sv, PRIBIO, ide_dmalist_lock);
			IDE_DMALIST_LOCK(oip);
		}
		ha->ha_dfreelistp = dmap->d_next;
		IDE_DMALIST_UNLOCK(oip);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
		dmap->d_size = i * sizeof(struct dma_vect);
#ifdef ATA_DEBUGSG_
		cmn_err(CE_CONT,"<S/G len %d DL%ld>\n", i, datal);
#endif
		bcopy((caddr_t) &tmp_list[0],
			(caddr_t) dmap->d_list, dmap->d_size);
	}
	return (0);
}

/*
 * Function name: idexlat()
 * Description:
 *	Perform the virtual to physical translation on the SCB
 *	data pointer.
 */

/*ARGSUSED*/
HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	sblk_t *sp = (sblk_t *) hbap;

	ATA_DTRACE;

	if (sp->s_dmap) {
		ide_dma_freelist(sp);
		sp->s_dmap = NULL;
	}

	if (sp->sbp->sb.SCB.sc_link) {
		cmn_err(CE_WARN, "Linked commands NOT available");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if (sp->sbp->sb.SCB.sc_datapt && sp->sbp->sb.SCB.sc_datasz) {
		/*
		 * Do scatter/gather if data spans multiple pages
		 */
		sp->s_addr = vtop(sp->sbp->sb.SCB.sc_datapt, procp);
		if (sp->sbp->sb.SCB.sc_datasz > pgbnd(sp->s_addr))
			if (ide_dmalist(sp, procp, sleepflag))
				HBAXLAT_RETURN (SDI_RET_RETRY);
	} else
		sp->sbp->sb.SCB.sc_datasz = 0;

	HBAXLAT_RETURN (SDI_RET_OK);
}

/*
 * Function name: ideinit()
 * Description:
 *	This is the initialization routine for the IDE HA driver.
 *	All data structures are initialized and the boards are initialized.
 */
int
ideinit(void)
{
	struct ata_ha	*ha;
	dma_t		*dp_page, *dp;
	int			iobus, cntls, i, j, gb;
	uint_t			port;
	int			sleepflag;
	int			mem_release;
	int			num_drives, num_io_channels;
	int			idata_index;
	int			ide_table_offset;
	uint_t bus_type;
	struct req_sense_def	*req_sense;
	struct hbagetinfo	getinfo;

	ATA_DTRACE;

	/* if gethardware fails, skip initialization */
	if (drv_gethardware(IOBUS_TYPE, &bus_type) == -1)
		return(-1);

	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	ideidata = sdi_hba_autoconf("ide", _ideidata, &ide_cntls);

	if (ideidata == NULL)    {
		return (-1);
	}
	cmn_err(CE_NOTE, "!%s: checking %d controllers", ideidata[0].name, ide_cntls);
	sdi_mca_conf(ideidata, ide_cntls, ide_mca_conf);

	HBA_IDATA(ide_cntls);

	ideinit_time = TRUE;

	for(i = 0; i < SDI_MAX_HBAS; i++)
		ide_gtol[i] = ide_ltog[i] = -1;

	sleepflag = ide_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	ide_sdi_ver.sv_release = 1;
	ide_sdi_ver.sv_machine = SDI_386_EISA;
	ide_sdi_ver.sv_modes   = SDI_BASIC1;

	ide_num_scbs = (IDE_IO_PER_CONTROLLER * 16) + 1;

	ide_scb_structsize = ide_num_scbs * (sizeof(struct control_area));
	ide_req_structsize = ide_num_scbs * (sizeof(struct req_sense_def));
	ide_luq_structsize = MAX_EQ * (sizeof (struct scsi_lu));
	ide_ha_size = sizeof(struct ata_ha);

	/*
	 * Now, loop through all of the idata structs and identify the
	 * controller that is there.
	 *
	 * Once we find an HA, ideidata.idata_nbus is set to the number 
	 * of SCSI buses on the HA.
	 *
	 * First, null out the ha array.
	 */
	for(gb = 0; gb < (SDI_MAX_HBAS * IDE_MAX_BPC); gb++)
		ide_sc_ha[gb] = CNFNULL;

	ide_habuscnt = 0;
	for(idata_index = 0; idata_index < ide_cntls; idata_index++) {
		/*
		 * Have we max'ed out the operating systems limit of
		 * adapters?
		 */
		if (idata_index > SDI_MAX_HBAS) {
			break;
		}
		num_io_channels = ide_find_ha(idata_index, &ide_table_offset, &num_drives);

		/*
		 * No controller at this port or unable to initialize the
		 * controller at this port.
		 */
		if (num_io_channels <= 0)
			continue;

		port = ideidata[idata_index].ioaddr1;
		ideidata[idata_index].idata_nbus = num_io_channels;

		cmn_err(CE_NOTE,"!Found %d ATA channel(s) at port 0x%x", num_io_channels, port);

		mem_release = 0;

		ide_ctob[idata_index] = ide_habuscnt;

		for(iobus = 0; iobus < num_io_channels; iobus++) {

			ha = (struct ata_ha *)kmem_zalloc(ide_ha_size, sleepflag);
			if (ha == NULL) {
				cmn_err(CE_WARN,
					"%s: port 0x%x CH%d Cannot allocate adapter blocks",
					ideidata[idata_index].name, port, iobus);
				goto ide_release;
			}
			mem_release |= HA_REL;
			ide_sc_ha[ide_habuscnt] = ha;

			ha->ha_config = &ide_table[ide_table_offset];
			ha->ha_devices = num_drives;

			ha->ha_name = ideidata[idata_index].name;
			cmn_err(CE_NOTE, "!V0.1a %s: Channel %d, port 0x%x",
			    ha->ha_name, iobus, port);
			ha->ha_ctl = (uchar_t)idata_index;
			ha->ha_num_chan = (uchar_t)num_io_channels;
			ha->ha_chan_num = (char)iobus;

			if ((ha->ha_bcbp = bcb_alloc(sleepflag)) == NULL) {
				cmn_err(CE_WARN,
				"%s: port 0x%x CH%d Cannot allocate bcb block",
				ideidata[idata_index].name, port, iobus);
				goto ide_release;
			}
			if ((ha->ha_bcbp->bcb_physreqp = physreq_alloc(sleepflag)) == NULL) {
				cmn_err(CE_WARN,
				"%s: port 0x%x CH%d Cannot allocate physreq",
				ideidata[idata_index].name, port, iobus);
				goto ide_release;
			}
			getinfo.bcbp = ha->ha_bcbp;
			ha->ha_config->cfg_GETINFO(&getinfo);
			req_sense = (struct req_sense_def *)KMEM_ZALLOC(ide_req_structsize, ha->ha_bcbp->bcb_physreqp, sleepflag);
			if (req_sense == NULL) {
				cmn_err(CE_WARN,
					"%s: port 0x%x CH%d Cannot allocate request sense blocks",
					ideidata[idata_index].name, port, iobus);
				goto ide_release;
			}
			mem_release |= HA_REQS_REL;

			ha->ha_dev = (struct scsi_lu *)KMEM_ZALLOC(ide_luq_structsize, ha->ha_bcbp->bcb_physreqp, sleepflag);
			if (ha->ha_dev == NULL) {
				cmn_err(CE_WARN,
					"%s: port 0x%x CH%d Cannot allocate lu queues",
					ideidata[idata_index].name, port, iobus);
				goto ide_release;
			}
			for (i = 0; i < MAX_EQ; i++) {
				ha->ha_dev[i].q_ha = ha;
			}
			mem_release |= HA_DEV_REL;

			ha->ha_id = ideidata[idata_index].ha_id;
			ha->ha_vect = ideidata[idata_index].iov;

			ha->ha_scb = (struct control_area *)KMEM_ZALLOC(ide_scb_structsize, ha->ha_bcbp->bcb_physreqp, sleepflag);
			if (ha->ha_scb == NULL) {
				cmn_err(CE_WARN,
					"%s: port 0x%x CH%d Cannot allocate command blocks",
					ideidata[idata_index].name, port, iobus);
				goto ide_release;
			}
			mem_release |= HA_SCB_REL;

			ide_init_scbs(ha, req_sense);

			/* init HA communication */
				if (ide_alloc_dma(ha)) {
					ide_habuscnt++;
					ideidata[idata_index].active = 1;
					ide_lockinit(idata_index);
					continue;
				}
				if (ha->ha_dfreelistp)
					mem_release |= HA_DMA_REL;

ide_release:
			if (num_io_channels == 1) {
				mem_release |= HA_STRUCT_REL;
			} else if (iobus == 0) {
				mem_release |= HA_CH0_REL;
			} else {
				mem_release |= HA_CH1_REL;
			}
			ide_mem_release(&ide_sc_ha[ide_habuscnt], mem_release);
			break;
		}
	}
	if (ide_habuscnt == 0) {
		cmn_err(CE_NOTE,"!%s: No HA's found.",ideidata[0].name);
		return(-1);
	}

	/*
	 * Attach interrupts for each "active" device. The driver
	 * must set the active field of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here, in init(), as opposed
	 * to load(), because this is required for static autoconfig
	 * drivers as well as loadable.
 	 */
	sdi_intr_attach(ideidata, ide_cntls, ideintr, ide_devflag);

	return(0);
}

/*
 * Function name: idestart()
 * Description:	
 *	Called by kernel to perform driver initialization
 *	after the kernel data area has been initialized.
 *	The EDT is built for every HA configured in the system.
 */
int
idestart(void)
{
	int		any_has, c, cntl_num, mem_release, iobus;
	int		sleepflag;
	struct ata_ha	*ha;
	int			gb;

	ATA_DTRACE;

	sleepflag = ide_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/*
	 * For all configured controller channels, register them with SDI
	 * one at a time if they are currently marked active.
	 *
	 * The active flag is cleared so we can identify the ones that
	 * fail to sdi_register correctly.
	 */
	for(c = 0; c < ide_cntls; c++) {
		if (!ideidata[c].active)
			continue;

		ha = ide_sc_ha[ide_ctob[c] + 0];

		for(iobus = 0; iobus < ha->ha_num_chan; iobus++) {
			(void)ide_ha_init(ha, iobus, sleepflag);
		}

		/* Get an HBA number from SDI and register HBA with SDI */
		if ((cntl_num = sdi_gethbano(ideidata[c].cntlr)) <= -1) {
			cmn_err(CE_WARN, "%s: C%d No HBA number available. %d", \
				ideidata[c].name, c, cntl_num);
			ideidata[c].active = 0;
			continue;
		}

		ideidata[c].cntlr = cntl_num;
		ide_gtol[cntl_num] = c;
		ide_ltog[c] = cntl_num;

		if (ha->ha_config->cfg_capab & CCAP_BOOTABLE)
			ideidata[c].idata_ctlorder = 1;	/* Set boot controller */

		if ((cntl_num = sdi_register(&idehba_info, &ideidata[c])) < 0) {
			cmn_err(CE_WARN, "!%s: C%d, SDI registry failure %d.", \
			   ideidata[c].name, c, cntl_num);
			ideidata[c].active = 0;
			continue;
		}
	}

	/*
	 * For all configured controller channels, count the ones that
	 * are currently marked active.
	 *
	 * If one is not active, this means that either sdi_gethbano failed
	 * or sdi_register failed or it previously failed in ideinit.
	 * In any case, if there is still memory associated with it, free it.
	 */
	for(mem_release = 0, any_has = 0, c = 0; c < ide_cntls; c++) {
		ide_lockclean(c);
		if (ideidata[c].active) {
			any_has++;
			continue;
		}
		for (gb = 0; gb < ide_habuscnt; gb++) {

			ha = ide_sc_ha[gb];
			if (ha == CNFNULL || ha->ha_ctl != c)
				continue;
			mem_release |= (HA_REQS_REL|HA_DEV_REL|HA_SCB_REL|HA_CONFIG_REL|HA_REL);
			if (ha->ha_num_chan == 1) {
				mem_release |= HA_STRUCT_REL;
			} else if (ha->ha_chan_num == 0) {
				mem_release |= HA_CH0_REL;
			} else {
				mem_release |= HA_CH1_REL;
			}
			ide_mem_release(&ide_sc_ha[gb], mem_release);
		}
	}

	/*
	 * If there are no active HA channels, fail the start
	 */
	if (!any_has )
		return(-1);

	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally and return SUCCESS.
	 */
	ideinit_time = FALSE;
	return(0);
}

/*
 * ide_ha_init()
 *	This is the guy that used to do all the hardware initialization of the
 *	adapter after it has been determined to be in the system.
 *
 *	At this point, all of the init has been done in the find_ha funcs.
 *	So, this just enables the interrupts on the device now.
 */

/*ARGSUSED*/
STATIC int
ide_ha_init(struct ata_ha *ha, int flag, int sleepflag)
{
	struct ide_cfg_entry *conf;

	ATA_DTRACE;

	conf = ha->ha_config;

	if (conf->cfg_START)
		(conf->cfg_START)(conf);
	return(1);
}

/*
 * Function name: ide_init_scbs()
 * Description:	
 *	Initialize the adapter scb free list.
 */
STATIC void
ide_init_scbs(struct ata_ha *ha, struct req_sense_def *req_sense)
{
	struct control_area	*cp;
	int			i, cnt;

	ATA_DTRACE;

	ha->ha_num_scb = cnt = ide_num_scbs;

	cnt--;
	for(i = 0; i < cnt; i++) {
		cp = &ha->ha_scb[i];
		cp->c_next = &ha->ha_scb[i+1];
		cp->virt_SensePtr = &req_sense[i];
		cp->SCB_SenseLen = sizeof(struct req_sense_def);
	}
	cp = &ha->ha_scb[i];
	cp->c_next = &ha->ha_scb[0];
	cp->virt_SensePtr = &req_sense[i];
	cp->SCB_SenseLen = sizeof(struct req_sense_def);

	ha->ha_scb_next = cp;
}

/*
 * Function name: ideopen()
 * Description:
 * 	Driver open() entry point. It checks permissions, and in the
 *	case of a pass-thru open, suspends the particular LU queue.
 */

/*ARGSUSED*/
int
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;
	minor_t		minor = geteminor(dev);
	int	c = ide_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	struct scsi_lu *q;
	int		gb = ide_ctob[c] + b;
	struct ata_ha  *ha = ide_sc_ha[gb];

	ATA_DTRACE;

	if (t == ha->ha_id)
		return(0);

	if (ide_illegal(SC_EXHAN(minor), b, t, l)) {
		return(ENXIO);
	}

	/* This is the pass-thru section */

	q = &LU_Q(c, b, t, l);

	IDE_SCSILU_LOCK(q->q_opri);
 	if ((q->q_outcnt > 0)  || (q->q_flag & (QSUSP | QPTHRU))) {
		IDE_SCSILU_UNLOCK(q->q_opri);
		return(EBUSY);
	}

	q->q_flag |= QPTHRU;
	IDE_SCSILU_UNLOCK(q->q_opri);
	return(0);
}

/*
 * Function name: ideclose()
 * Description:
 * 	Driver close() entry point.  In the case of a pass-thru close
 *	it resumes the queue and calls the target driver event handler
 *	if one is present.
 */

/*ARGSUSED*/
int
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	minor_t		minor = geteminor(dev);
	int	c = ide_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	struct scsi_lu *q;
	int		gb = ide_ctob[c] + b;
	struct ata_ha  *ha = ide_sc_ha[gb];

	ATA_DTRACE;

	if (t == ha->ha_id)
		return(0);

	q = &LU_Q(c, b, t, l);

	IDE_CLEAR_QFLAG(q,QPTHRU);

	if (q->q_func != NULL)
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);

	ide_restart(q);
	return(0);
}

/*
 * Function name: ideintr()
 * Description:	
 *	Driver interrupt handler entry point.  Called by kernel when
 *	a host adapter interrupt occurs.
 */
void
ideintr(uint vect)
{
	struct ata_ha	*ha;
	struct ide_cfg_entry *conf;
	struct ata_parm_entry *parms;
	int		bus;

	ATA_DTRACE;

	for(bus = 0; bus < ide_habuscnt; bus++) {
		ha = ide_sc_ha[bus];
		if (ha == CNFNULL)
			continue;
		if (ha->ha_vect != vect)
			continue;
		conf = ha->ha_config;

		IDE_HIM_LOCK;
		if ((parms = conf->cfg_INTR(conf))) {
			if ( parms->dpb_intret != DINT_CONTINUE ) {
				conf->cfg_CPL(conf, parms->dpb_control);
			}
		}
		IDE_HIM_UNLOCK;
	}
}

/*
 * void
 * ide_job_done(struct control_area *scb_ptr)
 *
 *	Interface back into the PDI layer from the lower layer.  Calls ide_done
 *	or ide_sfb_done as appropriate which then call sdi_callback.
 *
 * Calling/Exit State:
 *	IDE_HIM_LOCK held on entry and reacquired before exit.
 *	IDE_HIM_LOCK released during execution.
 *	Acquires IDE_NPEND_LOCK.
 *	Acquires IDE_SCSILU_LOCK.
 */
void
ide_job_done(struct control_area *scb_ptr)
{
	struct ata_ha	*ha;
	struct scsi_lu *q;
	int			gbus;
	int			int_status;
	pl_t		oip;

	ATA_DTRACE;

	gbus = scb_ptr->adapter;

	ha = ide_sc_ha[gbus];
	if (!ideidata[ha->ha_ctl].active) {
		return;
	}

	IDE_HIM_UNLOCK;

	int_status = scb_ptr->SCB_Stat;
#ifdef ATA_DEBUG0
	cmn_err(CE_CONT,"target %x, int_status %x\n", scb_ptr->target, int_status);
#endif
#ifdef DEBUG
	switch(int_status) {
		case DINT_CONTINUE:
		case DINT_COMPLETE:
		case DINT_GENERROR:
		case DINT_ERRABORT:
#endif
			IDE_NPEND_LOCK(oip);
			ha->ha_npend--;
			IDE_NPEND_UNLOCK(oip);

			q = &LU_Q_GB(gbus, scb_ptr->target, scb_ptr->SCB_Lun);
			IDE_SCSILU_LOCK(q->q_opri);
			q->q_outcnt--;
			IDE_SCSILU_UNLOCK(q->q_opri);
		
			if (scb_ptr->SCB_Cmd == EXEC_SCB) 
				ide_done(q, scb_ptr, int_status);
			else
				ide_sfb_done(q, scb_ptr, int_status);
#ifdef DEBUG
			break;
		default:
			cmn_err(CE_WARN, "%s: CH%d Invalid interrupt status - 0x%x",
				 ha->ha_name, ha->ha_chan_num, int_status);
			break;
	}
#endif

	IDE_HIM_LOCK;

	ide_waitflag = FALSE;
}

/*
 * Function name: ide_sfb_done()
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SFB structure defining the job.
 */
STATIC void
ide_sfb_done(struct scsi_lu *q, struct control_area *cp, int status)
{
	struct sb	*sp;

	ATA_DTRACE;

#ifdef ATA_DEBUG1
	cmn_err(CE_CONT,"entering ide_sfb_done(%x, %x, %x)\n", q, cp, status);
#endif

	sp = cp->c_bind;

	/* Determine completion status of the job */

	switch (status) {
		case DINT_COMPLETE:
			sp->SFB.sf_comp_code = SDI_ASW;
			ide_flushq(q, SDI_CRESET, 0);
			break;
		case DINT_GENERROR:
		case DINT_CONTINUE:
			sp->SFB.sf_comp_code = (ulong_t)SDI_HAERR;
			break;
		case DINT_ERRABORT:
			if ( cp->SCB_TargStat == DERR_BADCMD )
				sp->SFB.sf_comp_code = (ulong_t)SDI_SFBERR;
			else if ( cp->SCB_TargStat == DERR_NODISK )
				sp->SFB.sf_comp_code = SDI_NOSELE;
			else
				sp->SFB.sf_comp_code = (ulong_t)SDI_HAERR;
			break;
		default:
			ASSERT(ide_panic_assert);
			sp->SFB.sf_comp_code = (ulong_t)SDI_MISMAT;
			break;
	}

	/* call target driver interrupt handler */
	sdi_callback(sp);

	FREESCB(cp);

	ide_restart(q);
}

/*
 * Function name: ide_done()
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SCB structure defining the job.
 */
STATIC void
ide_done(struct scsi_lu *q, struct control_area *cp, int status)
{
	int		i;
	struct sb *sp = cp->c_bind;

	ATA_DTRACE;

	IDE_CLEAR_QFLAG(q,QSENSE); /* Old sense data now invalid */

	/* Determine completion status of the job */
	switch(status) {
	case DINT_COMPLETE:
		sp->SCB.sc_comp_code = SDI_ASW;
		sp->SCB.sc_status = S_GOOD;
		break;

	case DINT_RETRY:
		sp->SCB.sc_comp_code = SDI_RETRY;
		break;

	case DINT_ERRABORT:
	case DINT_GENERROR:
		switch(cp->SCB_TargStat) {
		case DERR_NOERR:
			sp->SCB.sc_comp_code = SDI_ASW;
			sp->SCB.sc_status = S_GOOD;
			break;
		case DERR_TIMEOUT:
			sp->SCB.sc_comp_code = SDI_TIME;
			break;
		case DERR_BADCMD:
			sp->SCB.sc_comp_code = SDI_SCBERR;
			break;
		case DERR_NODISK:
			sp->SCB.sc_comp_code = SDI_NOSELE;
			break;
		case DERR_ABORT:
			sp->SCB.sc_comp_code = SDI_ABORT;
			break;
		case DERR_DNOTRDY:
			sp->SCB.sc_comp_code = SDI_CKSTAT;
			sp->SCB.sc_status = SD_UNATTEN;
			break;
		case DERR_OVERRUN:
			cmn_err(CE_WARN, "%s: CH%d Data over/under run error",
				 ide_sc_ha[cp->adapter]->ha_name,
				 ide_sc_ha[cp->adapter]->ha_chan_num);
			sp->SCB.sc_comp_code = (ulong_t)SDI_HAERR;
			break;
		default:
			sp->SCB.sc_comp_code = SDI_CKSTAT;
			sp->SCB.sc_status = S_CKCON;

			/* Cache request sense info (note padding) */
			if (cp->SCB_SenseLen < (sizeof(q->q_sense) - 1))
				i = cp->SCB_SenseLen;
			else
				i = sizeof(q->q_sense) - 1;
			bcopy((caddr_t)cp->virt_SensePtr, (caddr_t)(sdi_sense_ptr(sp))+1, i);
			IDE_SCSILU_LOCK(q->q_opri);
			bcopy((caddr_t)cp->virt_SensePtr, (caddr_t)(&q->q_sense)+1, i);
			q->q_flag |= QSENSE;	/* sense data now valid */
			IDE_SCSILU_UNLOCK(q->q_opri);
			bzero((caddr_t)cp->virt_SensePtr, cp->SCB_SenseLen);
			break;
		}
		break;
	default:
		cmn_err(CE_WARN, "!%s: CH%d Illegal status 0x%x!",
			ide_sc_ha[cp->adapter]->ha_name,
			ide_sc_ha[cp->adapter]->ha_chan_num, status);
		ASSERT(ide_panic_assert);
		sp->SCB.sc_comp_code = SDI_ERROR;
		break;
	}
	cp->SCB_TargStat = DERR_NOERR;

	sdi_callback(sp); /* call target driver interrupt handler */

	FREESCB(cp);

	ide_restart(q);
} 

/*
 * Function name: ide_restart()
 * Description:
 *	This function restarts all lu Q's associated with the ha
 *	that the q passed as an arguement is associated with.
 *
 *	It always restarts the q passed to it first.
 *
 * NOTE: the implementation currently assumes that only the first
 *       LU of the first 2 targets is present.
 */
STATIC void
ide_restart(struct scsi_lu *q)
{
	struct scsi_lu 			*next;
	struct ata_ha 			*ha = q->q_ha;

	ATA_DTRACE;

	if ( ha->ha_devices > 1 ) {
		next = &ha->ha_dev[SUBDEV(1,0)];
		if ( q == next ) {
			next = &ha->ha_dev[SUBDEV(0,0)];
		}
		IDE_SCSILU_LOCK_A(next,next->q_opri);
		ide_next(next);
	}

	IDE_SCSILU_LOCK(q->q_opri);
	ide_next(q);
} 

/*
 * Function name: ide_int()
 * Description:
 *	This is the interrupt handler for pass-thru jobs.  It just
 *	wakes up the sleeping process.
 */
STATIC void
ide_int(struct sb *sp)
{
	struct buf  *bp;

	ATA_DTRACE;

	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}

/*
 * Function: ide_illegal()
 * Description: Used to verify commands
 */
STATIC int
ide_illegal(short hba, int bus, uchar_t scsi_id, uchar_t lun)
{
	if (sdi_rxedt(hba, bus, scsi_id, lun))
		return(0);
	else
		return(1);
}

/*
 * Function name: ide_func()
 * Description:
 *	Create and send an SFB associated command.
 */
STATIC void
ide_func(sblk_t *sp)
{
	struct scsi_ad	*sa;
	struct ata_ha *ha;
	struct control_area	*cp;
	struct ide_cfg_entry	*conf;
	int			c, b, gb, target, lun;

	ATA_DTRACE;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = ide_gtol[SDI_EXHAN(sa)];
	b = SDI_BUS(sa);
	target = SDI_EXTCN(sa);
	lun = SDI_EXLUN(sa);
	gb = ide_ctob[c] + b;
 	ha = ide_sc_ha[gb];

	cp = ide_getblk(gb);
	conf = ha->ha_config;
	cp->SCB_Cmd = HARD_RST_DEV;
	cp->SCB_Stat = 0;
	cp->SCB_Lun = lun;
	cp->SCB_SegCnt = 0;
	cp->SCB_SegPtr = 0;
	cp->SCB_TargStat = 0;
	cp->data_len = 0;
	cp->target = target;
	cp->adapter = gb;
	cp->c_bind = &sp->sbp->sb;
	
	if (!gdev_valid_drive(cp)) {
		struct scsi_lu *q = &LU_Q_GB(gb, cp->target, cp->SCB_Lun);
		cp->SCB_TargStat = DERR_NODISK;
		ide_sfb_done(q, cp, DINT_ERRABORT);
		ide_waitflag = FALSE;
		return;
	}

	cmn_err(CE_WARN, "%s: CH%d TARGET%d LUN%d is being reset",
		ha->ha_name, ha->ha_chan_num, target, lun);

	IDE_HIM_LOCK;
	(conf->cfg_SEND)(conf, cp);
	IDE_HIM_UNLOCK;
}

/*
 * STATIC void
 * ide_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
ide_pass_thru0(buf_t *bp)
{
	minor_t	minor = geteminor(bp->b_edev);
	int	c = ide_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	struct scsi_lu	*q = &LU_Q(c, b, t, l);
	int		gb = ide_ctob[c] + b;
	struct ata_ha  *ha = ide_sc_ha[gb];

	ATA_DTRACE;

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
		q->q_bcbp->bcb_flags = BCB_SYNCHRONOUS;
		q->q_bcbp->bcb_granularity = 1;
	}

	buf_breakup(ide_pass_thru, bp, q->q_bcbp);
}

/*
 * STATIC void
 * ide_pass_thru(buf_t *bp)
 *	Send a pass-thru job to the HA board.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires IDE_SCSILU_LOCK(q->q_opri).
 */
STATIC void
ide_pass_thru(buf_t *bp)
{
	minor_t	minor = geteminor(bp->b_edev);
	int	c = ide_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	struct scsi_lu	*q;
	struct sb *sp;
	caddr_t *addr;
	char op;
	daddr_t blkno_adjust;
	int	gb = ide_ctob[c] + b;
	struct ata_ha *ha = ide_sc_ha[gb];
	struct ide_cfg_entry *config = ha->ha_config;
	struct ata_parm_entry *parms = &config->cfg_drive[t];

	ATA_DTRACE;

	sp = (struct sb *)bp->b_private;

	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = ide_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	q = &LU_Q(c, b, t, l);

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
		scs->ss_len   = (char)(bp->b_bcount / parms->dpb_secsiz);
	}
	if (op == SM_READ || op == SM_WRITE) {
		daddr_t blkno;
		struct scm *scm = (struct scm *)(void *)((char *)q->q_sc_cmd - 2);

		if (blkno_adjust) {
			blkno = bp->b_priv2.un_int + blkno_adjust;
			scm->sm_addr = sdi_swap32(blkno);
		}
		scm->sm_len  = sdi_swap16(bp->b_bcount / parms->dpb_secsiz);
	}

	sdi_icmd(sp, KM_SLEEP);
}

/*
 * Function name: ide_next()
 * Description:
 *	Attempt to send the next job on the logical unit queue.
 *	No jobs of any type are sent if the controller is busy.
 */
STATIC void
ide_next(struct scsi_lu *q)
{
	pl_t		oip;
	sblk_t			*sp, *sched;
	struct scsi_ad		*sa;
	int			c, gb;
	struct ata_ha		*ha;

	ATA_DTRACE;
	ASSERT(q);

	ha = q->q_ha;
	IDE_NPEND_LOCK(oip);
	if (ha->ha_npend >= IDE_IO_PER_CONTROLLER) {
		IDE_NPEND_UNLOCK(oip);
		IDE_SCSILU_UNLOCK(q->q_opri);
		return;
	}
	IDE_NPEND_UNLOCK(oip);

	if ((sp = q->q_first) == NULL) {	 /*  queue empty  */
		q->q_depth = 0;
		IDE_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	if ((q->q_flag & QSUSP) && sp->sbp->sb.sb_type == SCB_TYPE) {
		IDE_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	ide_getq(q, sp);
	IDE_SCSILU_UNLOCK(q->q_opri);
	if (sp->sbp->sb.sb_type == SFB_TYPE) 
		ide_func(sp);
	else
		ide_cmd(sp);

	if (q->q_first) {
		IDE_SCSILU_LOCK(q->q_opri);
		sched = ide_schedule(q);
		if (sched != q->q_first) {
			ide_getq(q, sched);
			sched->s_next = q->q_first;
			sched->s_prev = NULL;
			q->q_first->s_prev = sched;
			q->q_first = sched;
			q->q_depth++;
		}
		IDE_SCSILU_UNLOCK(q->q_opri);
	}


	if (ideinit_time == TRUE) {		/* need to poll */
		if (ide_wait(30000) == LOC_FAILURE)  {
			cmn_err(CE_NOTE,
				"IDE Initialization Problem, timeout with no interrupt\n");
			sp->sbp->sb.SCB.sc_comp_code = (ulong_t)SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		}
	}
}

/*
 * Function name: ide_putq()
 *	Put a job on a logical unit queue.  Jobs are enqueued
 *	on a priority basis.
 */
STATIC void
ide_putq(struct scsi_lu *q, sblk_t *sp)
{
	int cls = IDE_QUECLASS(sp);

	ATA_DTRACE;

	/* 
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if (!q->q_first || (cls <= IDE_QUECLASS(q->q_last))) {
		if (q->q_first) {
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

		while(IDE_QUECLASS(nsp) >= cls)
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if (nsp->s_prev)
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}
	q->q_depth++;
}

/*
 * Function name: ide_flushq()
 *	Empty a logical unit queue.  If flag is set, remove all jobs.
 *	Otherwise, remove only non-control jobs.
 */
STATIC void
ide_flushq(struct scsi_lu *q, int cc, int flag)
{
	sblk_t  *sp, *nsp;

	ATA_DTRACE;
	ASSERT(q);

	IDE_SCSILU_LOCK(q->q_opri);
	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_depth = 0;
	IDE_SCSILU_UNLOCK(q->q_opri);

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (IDE_QUECLASS(sp) > HBA_QNORM)) {
			IDE_SCSILU_LOCK(q->q_opri);
			ide_putq(q, sp);
			IDE_SCSILU_UNLOCK(q->q_opri);
		} else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}
		sp = nsp;
	}
}

/*
 * Function name: ide_send()
 * Description:
 *	Send a command to the host adapter board.
 */
STATIC void
ide_send(int gb, struct control_area *cp)
{
	struct ata_ha		*ha = ide_sc_ha[gb];
	struct scsi_lu		*q;
	struct ide_cfg_entry *conf;
	pl_t				oip;

	ATA_DTRACE;

	q = &LU_Q_GB(gb, cp->target, cp->SCB_Lun);
	IDE_SCSILU_LOCK(q->q_opri);
	q->q_outcnt++;
	IDE_SCSILU_UNLOCK(q->q_opri);

	IDE_NPEND_LOCK(oip);
	ha->ha_npend++;
	IDE_NPEND_UNLOCK(oip);

	conf = ha->ha_config;
	IDE_HIM_LOCK;
	(conf->cfg_SEND)(conf, cp);		/* send the command via HIM */
	IDE_HIM_UNLOCK;
}

/*
 * Function name: ide_getblk()
 * Description:	
 *	Allocate a controller command block structure.
 */
STATIC struct control_area *
ide_getblk(int gb)
{
	struct ata_ha	*ha = ide_sc_ha[gb];
	struct control_area	*cp;
#ifdef DEBUG
	int		cnt;
#endif
	pl_t	oip;

	ATA_DTRACE;

	IDE_SCB_LOCK(oip);
	cp = ha->ha_scb_next;
#ifdef DEBUG
	cnt = 0;
#endif

	while(cp->c_active) {
#ifdef DEBUG
		cnt++;
#endif
		ASSERT(cnt < ha->ha_num_scb);
		cp = cp->c_next;
	}
	ha->ha_scb_next = cp->c_next;
	cp->c_active = TRUE;
	IDE_SCB_UNLOCK(oip);
	ha->ha_poll = cp;
	return(cp);
}

/*
 * Function name: ide_cmd()
 * Description:	
 *	Create and send an SCB associated command, using a scatter
 *	gather list created by ide_sctgth().
 */
STATIC void
ide_cmd(sblk_t *sp)
{
	struct scsi_ad		*sa;
	struct control_area	*cp;
	struct scsi_lu		*q;
	int			i;
	char				*p;
	long				bcnt, cnt;
	int				c, b, target, lun, gb;
	struct dma_vect 		*addr;

	ATA_DTRACE;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = ide_gtol[SDI_EXHAN(sa)];
	b = SDI_BUS(sa);
	gb = ide_ctob[c] + b;
	target = SDI_EXTCN(sa);
	lun = SDI_EXLUN(sa);

	cp = ide_getblk(gb);


	/* fill in the controller command block */

	cp->SCB_Cmd = EXEC_SCB;
	cp->SCB_Stat = 0;
	cp->SCB_Lun = lun;

	cp->target = target;
	cp->adapter = gb;
	cp->c_bind = &sp->sbp->sb;

	if (sp->s_dmap) {
		cnt = sp->s_dmap->d_size;
		addr = sp->s_dmap->d_list;
		cp->data_len = sp->sbp->sb.SCB.sc_datasz;
		cp->SCB_SegPtr = addr;
		cp->SCB_SegCnt = cnt / sizeof(struct dma_vect);
	} else {
		if (sp->sbp->sb.SCB.sc_datasz) {
			cp->data_len = sp->sbp->sb.SCB.sc_datasz;
			cp->c_dma_vect.d_addr = sp->s_addr;
			cp->c_dma_vect.d_cnt = cp->data_len;
			cp->SCB_SegPtr = &cp->c_dma_vect;
			cp->SCB_SegCnt = 1;
		} else {
			cp->data_len = 0;
			cp->SCB_SegCnt = 0;
			cp->SCB_SegPtr = NULL;
		}
	}

 	p = sp->sbp->sb.SCB.sc_cmdpt;

	q = &LU_Q(c, b, target, lun);

	IDE_SCSILU_LOCK(q->q_opri);

	if (!gdev_valid_drive(cp)) {
		cp->SCB_TargStat = DERR_NODISK;
		IDE_SCSILU_UNLOCK(q->q_opri);
		ide_done(q, cp, DINT_ERRABORT);
		ide_waitflag = FALSE;
	} else if ((q->q_flag & QSENSE) && (*p == SS_REQSEN)) {
		int copyfail;
		caddr_t toaddr;
		copyfail = 0;

		q->q_flag &= ~QSENSE;

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
 		if (cp->SCB_SegCnt <= 1) {
			if (toaddr = physmap(sp->s_addr, cp->data_len, KM_NOSLEEP)) {
 				bcopy((caddr_t)(&q->q_sense)+1, toaddr, cp->data_len);
				physmap_free(toaddr, cp->data_len, 0);
			} else {
				copyfail++ ;
			}
 		} else {
			struct dma_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src;
			caddr_t	optr, Dest;
			ulong_t	Count;
		 
 			if (VectPtr = (struct dma_vect *)(void *)physmap(sp->s_addr, cnt, KM_NOSLEEP)) {
				optr = (caddr_t)VectPtr;
 				VectCnt = cnt / sizeof(struct dma_vect);
 				Src = (caddr_t)(&q->q_sense) + 1;
 				for (i = 0; i < VectCnt; i++) {
 					Dest = (caddr_t)(VectPtr->d_addr);
 					Count = (int)(VectPtr->d_cnt);

 					if (Dest = physmap((paddr_t)Dest, Count, KM_NOSLEEP)) {
 						bcopy (Src, Dest, Count);
 						Src += Count;
 						VectPtr++;
						physmap_free(Dest, Count, 0);
					} else {
						copyfail++;
						break;
					}
 				}
				physmap_free(optr, cnt, 0);
 			} else {
				copyfail++;
 			}
 		}
		IDE_SCSILU_UNLOCK(q->q_opri);
		ide_done(q, cp, copyfail?DINT_RETRY:DINT_COMPLETE);
		ide_waitflag = FALSE;
	} else {
		IDE_SCSILU_UNLOCK(q->q_opri);
		ide_send(gb, cp);
	}

	return;
}

/*
 * Function Name: ide_wait()
 * Description:
 *	Poll for a completion from the host adapter.  If an interrupt
 *	is seen, the HA's interrupt service routine is manually called.
 *  NOTE:
 *	This routine allows for no concurrency and as such, should
 *	be used selectively.
 */
STATIC int
ide_wait(int time)
{

	ATA_DTRACE;

	while (time > 0) {
		if (ide_waitflag == FALSE) {
			ide_waitflag = TRUE;
			return (LOC_SUCCESS);
		}
#ifdef ATA_DEBUG2
if (!(time%8)) cmn_err(CE_CONT,".");
#endif
		drv_usecwait(1000);
		time--;
	}

#ifdef ATA_DEBUG2
cmn_err(CE_NOTE,"Bad news, timeout with no interrupt\n");
#endif
 	return (LOC_FAILURE);	
}

/*
 * Function name: idefreeblk()
 * Description:
 *	Release previously allocated SB structure. If a scatter/gather
 *	list is associated with the SB, it is freed via
 *	ide_dma_freelist().
 *	A nonzero return indicates an error in pointer or type field.
 */
long
HBAFREEBLK(struct hbadata *hbap)
{
	sblk_t *sp = (sblk_t *) hbap;

	ATA_DTRACE;

	if (sp->s_dmap) {
		ide_dma_freelist(sp);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return (SDI_RET_OK);
}

/*
 * Function name: idegetblk()
 * Description:
 *	Allocate a SB structure for the caller.  The function will
 *	sleep if there are no SCSI blocks available.
 */
struct hbadata *
HBAGETBLK(int sleepflag)
{
	sblk_t	*sp;

	ATA_DTRACE;

	sp = (sblk_t *)SDI_GET(&sm_poolhead, sleepflag);
	return((struct hbadata *)sp);
}

/*
 * Function name: idegetinfo()
 * Description:
 *	Return the name, etc. of the given device.  The name is copied
 *	into a string pointed to by the first field of the getinfo
 *	structure.
 */
void
HBAGETINFO( struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	char  *s1, *s2;
	static char temp[] = "HA X TC X";
	int		c = ide_gtol[SDI_EXHAN(sa)];
	int		gb = ide_ctob[c] + 0;
	struct ata_ha  *ha = ide_sc_ha[gb];

	ATA_DTRACE;

	s1 = temp;
	s2 = getinfop->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;
#ifdef ATA_DEBUG1
	cmn_err(CE_CONT,"idegetinfo(%x, %s)\n", sa, getinfop->name);
#endif

	ha->ha_config->cfg_GETINFO(getinfop);

}

/*
 * Function name: ideicmd()
 * Description:
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 */

/*ARGSUSED*/
long
HBAICMD(struct  hbadata *hbap, int sleepflag)
{
	struct scsi_ad *sa;
	struct scsi_lu *q;
	sblk_t *sp = (sblk_t *) hbap;
	int	c, t, l, b, gb;
	struct scs	*inq_cdb;
	struct sdi_edt	*edtp;

	ATA_DTRACE;

#ifdef ATA_DEBUG0
	cmn_err(CE_CONT,"sb address=0x%x\n",&(sp->sbp->sb));
#endif

	switch (sp->sbp->sb.sb_type) {
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = ide_gtol[SDI_EXHAN(sa)];
		t = SDI_EXTCN(sa);
		l = SDI_EXLUN(sa);
		b = SDI_BUS(sa);
		q = &LU_Q(c, b, t, l);
#ifdef ATA_DEBUG2
		cmn_err(CE_CONT,"ide_icmd: SFB c=%d b=%d t=%d l=%d \n", c, b, t, l);
#endif

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func) {
		case SFB_RESUME:
			IDE_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~QSUSP;
			ide_next(q);
			break;
		case SFB_SUSPEND:
			IDE_SET_QFLAG(q,QSUSP);
			break;
		case SFB_RESETM:
			edtp = sdi_rxedt(ide_ltog[c],b,t,l);
			if (edtp != NULL && edtp->pdtype == ID_TAPE) {
				/*  this is a NOP for tape devices */
				sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;
				break;
			}
			/*FALLTHRU*/
		case SFB_ABORTM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			IDE_SCSILU_LOCK(q->q_opri);
			ide_putq(q, sp);
			ide_next(q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			ide_flushq(q, SDI_QFLUSH, 0);
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
		c = ide_gtol[SDI_EXHAN(sa)];
		t = SDI_EXTCN(sa);
		l = SDI_EXLUN(sa);
		b = SDI_BUS(sa);
		gb = ide_ctob[c] + b;
		q = &LU_Q(c, b, t, l);
#ifdef ATA_DEBUG2
		cmn_err(CE_CONT,"ide_icmd: SCB c=%d b=%d t=%d l=%d \n", c, b, t, l);
#endif
		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;

		if ((t == ide_sc_ha[gb]->ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, ide_sc_ha[gb]->ha_name, 
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

		IDE_SCSILU_LOCK(q->q_opri);
		ide_putq(q, sp);
		ide_next(q);
		return (SDI_RET_OK);

	default:
#ifdef ATA_DEBUG2
		cmn_err(CE_CONT,"ide_icmd: SDI_RET_ERR\n");
#endif
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_ERR);
	}
}

#ifndef SM_VERIFY
#define SM_VERIFY 0x2f
#endif

/*
 * Function name: ideioctl()
 * Description:	
 *	Driver ioctl() entry point.  Used to implement the following
 *	special functions:
 *
 *	SDI_SEND     -	Send a user supplied SCSI Control Block to
 *			the specified device.
 *	B_GETTYPE    -  Get bus type and driver name
 *	B_HA_CNT     -	Get number of HA boards configured
 *	REDT	     -	Read the extended EDT from RAM memory
 *	SDI_BRESET   -	Reset the specified SCSI bus
 *
 */

/*ARGSUSED*/
int
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	minor_t	minor = geteminor(dev);
	int	c = ide_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	int	gb = ide_ctob[c] + b;
	struct sb *sp;
	pl_t  oip;
	int  uerror = 0;

	ATA_DTRACE;

	switch(cmd) {
	case SDI_SEND: {
		buf_t *bp;
		struct sb  karg;
		struct scm  *scmp;
		int  rw;
		char *save_priv;
		struct scsi_lu *q;
		struct ata_ha *ha = ide_sc_ha[gb];

		if (t == ha->ha_id) { 	/* illegal ID */
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

		scmp = (struct scm *)(void *)SCM_RAD(karg.SCB.sc_cmdpt);
		if(karg.SCB.sc_cmdsz == SCM_SZ && scmp->sm_op == SM_VERIFY) {
			return(ide_doverify(dev, arg, &karg));
		}

		sp = SDI_GETBLK(KM_SLEEP);
		save_priv = sp->SCB.sc_priv;
		bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

		bp = getrbuf(KM_SLEEP);
		bp->b_iodone = NULL;
		sp->SCB.sc_priv = save_priv;

		q = &LU_Q(c, b, t, l);
		if (q->q_sc_cmd == NULL) {
			/*
			 * Allocate space for the SCSI command and add 
			 * sizeof(int) to account for the scm structure
			 * padding, since this pointer may be adjusted -2 bytes
			 * and cast to struct scm.  After the adjustment we
			 * still want to be pointing within our allocated space!
			 */
			q->q_sc_cmd = (char *)KMEM_ZALLOC(MAX_CMDSZ + 
				sizeof(int), ha->ha_bcbp->bcb_physreqp, KM_SLEEP) + sizeof(int);
		}
		sp->SCB.sc_cmdpt = q->q_sc_cmd;

		if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
		    sp->SCB.sc_cmdsz)) {
			uerror = EFAULT;
			goto done;
		}
		sp->SCB.sc_dev.sa_lun = (uchar_t)l;
		sp->SCB.sc_dev.sa_bus = (uchar_t)b;
		sp->SCB.sc_dev.sa_exta = (uchar_t)t;
		sp->SCB.sc_dev.sa_ct = SDI_SA_CT(ide_ltog[c],t);

		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
		bp->b_private = (char *)sp;

		/*
		 * If the job involves a data transfer then the
		 * request is done thru physiock() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then ide_pass_thru()
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
				struct scs *scs = (struct scs *)(void *)q->q_sc_cmd;
				bp->b_priv2.un_int =
				   ((sdi_swap16(scs->ss_addr) & 0xffff)
				   + (scs->ss_addr1 << 16));
			}
			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm = 
				(struct scm *)(void *)((char *)q->q_sc_cmd - 2);
				bp->b_priv2.un_int = sdi_swap32(scm->sm_addr);
			}

			if (uerror = physiock(ide_pass_thru0, bp, dev, rw, 
			    HBA_MAX_PHYSIOCK, &ha_uio)) {
				goto done;
			}
		} else {
			bp->b_un.b_addr = NULL;
			bp->b_bcount = 0;
			bp->b_blkno = 0;
			bp->b_edev = dev;
			bp->b_flags |= rw;

			ide_pass_thru(bp);  /* Bypass physiock call */
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
		if (copyout("dcd", ((struct bus_type *)arg)->bus_name, 4)) {
			return(EFAULT);
		}
		if (copyout("dcd", ((struct bus_type *)arg)->drv_name, 4)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if (copyout((caddr_t)&ide_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET: {
		struct ata_ha *ha = ide_sc_ha[gb];

		IDE_NPEND_LOCK(oip);
		if (ha->ha_npend > 0) {     /* jobs are outstanding */
			IDE_NPEND_UNLOCK(oip);
			return(EBUSY);
		} else {
			cmn_err(CE_WARN, "%s: CH%d - Bus is being reset",
				ha->ha_name, ha->ha_chan_num);
		}
		IDE_NPEND_UNLOCK(oip);
		break;
		}

	default:
		return(EINVAL);
	}
	return(uerror);
}

/*
 * Function name: idesend()
 * Description:
 * 	Send a SCSI command to a controller.  Commands sent via this
 *	function are executed in the order they are received.
 */

/*ARGSUSED*/
long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	struct scsi_ad *sa;
	struct scsi_lu *q;
	sblk_t *sp = (sblk_t *) hbap;
	int	c, b, t, l;

	ATA_DTRACE;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = ide_gtol[SDI_EXHAN(sa)];
	b = SDI_BUS(sa);
	t = SDI_EXTCN(sa);
	l = SDI_EXLUN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE) {
		return (SDI_RET_ERR);
	}

	q = &LU_Q(c, b, t, l);

	IDE_SCSILU_LOCK(q->q_opri);
	if (q->q_flag & QPTHRU) {
		IDE_SCSILU_UNLOCK(q->q_opri);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	ide_putq(q, sp);
	ide_next(q);

	return (SDI_RET_OK);
}

STATIC int
ide_alloc_dma(struct ata_ha *ha)
{
	int		i;
	int		sleepflag;
	int		ndma_list;
	dma_t		*dmalistp, *dp;

	ATA_DTRACE;

	sleepflag = ide_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/*
	 * Allocate the space for the dma_list structures.
	 */

	ndma_list =  ide_num_scbs;
	ide_dma_listsize = sizeof(dma_t) *ndma_list;

	dmalistp = (dma_t *)KMEM_ZALLOC(ide_dma_listsize, ha->ha_bcbp->bcb_physreqp, sleepflag);
	if (dmalistp == NULL) {
		cmn_err(CE_WARN, "%s: Cannot allocate dmalistp",
			 ideidata[0].name);
		return(0);
	}

	ha->ha_dfreelistp = NULL;
	for (i = 0; i < ndma_list; i++) {
		dp = &dmalistp[i];
		dp->d_next = ha->ha_dfreelistp;
		ha->ha_dfreelistp = dp;
	}

	return (1);
}

/*
 * ide_mem_release()
 *	This routine releases allocated memory back to the kernel based
 *	on the flag variable.
 */
STATIC void
ide_mem_release(struct ata_ha **hap, int flag)
{
	struct ata_ha *ha = *hap;
	struct req_sense_def *req_sense = ha->ha_scb[0].virt_SensePtr;

	ATA_DTRACE;

	if (flag & HA_DEV_REL) {
		KMEM_FREE(ha->ha_dev, ide_luq_structsize);
		ha->ha_dev = NULL;
		flag &= ~HA_DEV_REL;
	}
	if (flag & HA_DMA_REL) {
		KMEM_FREE(ha->ha_dfreelistp, ide_dma_listsize);
		flag &= ~HA_DMA_REL;
	}
	if (flag & HA_SCB_REL) {
		KMEM_FREE(ha->ha_scb, ide_scb_structsize);
		ha->ha_scb = NULL;
		flag &= ~HA_SCB_REL;
	}
	if (flag & HA_REQS_REL) {
		KMEM_FREE(req_sense, ide_req_structsize);
		flag &= ~HA_REQS_REL;
	}
	if (flag & HA_REL) {
		KMEM_FREE(ha, ide_ha_size);
		*hap = CNFNULL;
		flag &= ~HA_REL;
	}
}

/*
 * STATIC void
 * ide_lockinit(int c)
 *	Initialize ide locks:
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
ide_lockinit(int c)
{
	struct ata_ha  *ha, *other_ha;
	struct scsi_lu	 *q;
	int	gb,t,l;
	int		sleepflag;
	static		firsttime = 1;

	ATA_DTRACE;

	sleepflag = ide_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if (firsttime) {

		ide_dmalist_sv = SV_ALLOC (sleepflag);
		ide_dmalist_lock = LOCK_ALLOC (IDE_HIER, pldisk, &ide_lkinfo_dmalist, sleepflag);
		firsttime = 0;
	}

	other_ha = CNFNULL;
	for (gb = 0; gb < ide_habuscnt; gb++) {
		if (((ha = ide_sc_ha[gb]) == CNFNULL) || (ha->ha_ctl != c))
			continue;
		ha->ha_npend_lock = LOCK_ALLOC (IDE_HIER+1, pldisk, &ide_lkinfo_npend, sleepflag);
		ha->ha_scb_lock = LOCK_ALLOC (IDE_HIER, pldisk, &ide_lkinfo_scb, sleepflag);
		if (other_ha ) {
			ha->ha_HIM_lock = other_ha->ha_HIM_lock;
		} else {
			ha->ha_HIM_lock =
				(ide_lock_t *)kmem_zalloc(sizeof(ide_lock_t), sleepflag);
			ha->ha_HIM_lock->al_HIM_lock =
				LOCK_ALLOC (IDE_HIER, pldisk, &ide_lkinfo_HIM, sleepflag);
			other_ha = ha;
		}
		for (t = 0; t < MAX_TCS; t++) {
		    for (l = 0; l < MAX_LUS; l++) {
			q = &LU_Q_GB (gb,t,l);
			q->q_lock = LOCK_ALLOC (IDE_HIER, pldisk, &ide_lkinfo_q, sleepflag);
		    }
		}
	}
}

/*
 * STATIC void
 * ide_lockclean(int c)
 *	Removes unneeded locks.  Controllers that are not active will
 *	have all locks removed.  Active controllers will have locks for
 *	all non-existant devices removed.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
ide_lockclean(int c)
{
	struct ata_ha  *ha;
	struct scsi_lu	 *q;
	int	b,gb,t,l;
	static		firsttime = 1;

	ATA_DTRACE;

	if (firsttime && !ide_habuscnt) {

		if (ide_dmalist_sv == NULL)
			return;
		SV_DEALLOC (ide_dmalist_sv);
		LOCK_DEALLOC (ide_dmalist_lock);
		firsttime = 0;
	}

	for (gb = 0, b = 0; gb < ide_habuscnt; gb++) {
		if ( !(ha = ide_sc_ha[gb]) || (ha->ha_ctl != c))
			continue;
		if (!ideidata[c].active) {

			if (ha->ha_npend_lock) {
				LOCK_DEALLOC (ha->ha_npend_lock);
				LOCK_DEALLOC (ha->ha_scb_lock);
#ifdef NOT_YET
				LOCK_DEALLOC (ha->ha_HIM_lock);
#endif
			}
		}

		for (t = 0; t < MAX_TCS; t++) {
		    for (l = 0; l < MAX_LUS; l++) {
				if (!ideidata[c].active || ide_illegal(ide_ltog[c], b, t, l)) {
					q = &LU_Q_GB (gb,t,l);
					LOCK_DEALLOC (q->q_lock);
				}
		    }
		}
		b++;
	}
}

/*
 * STATIC void *
 * ide_kmem_zalloc_physreq(size_t size, int flags)
 *	function to be used in place of kma_zalloc
 *	which allocates contiguous memory, using kmem_alloc_physreq,
 *	and zero's the memory.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void *
ide_kmem_zalloc_physreq(size_t size, physreq_t *physreqp, int flags)
{
	void *mem;
	physreq_t preq;

	ATA_DTRACE;
	preq = *physreqp;
	preq.phys_flags |= PREQ_PHYSCONTIG;

	if (!physreq_prep(&preq, flags)) {
		return NULL;
	}
	mem = kmem_alloc_physreq(size, &preq, flags);
	if (mem)
		bzero(mem, size);

	return mem;
}

/*
 * STATIC int
 * ide_find_ha(int idata_index)
 *	function that searches thru all configured controllers
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
ide_find_ha(int idata_index, int *ide_table_offset, int *numdrv)
{
	int j, table_offset, ide_drivecnt, sleepflag;

	sleepflag = ide_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/* 
	 * loop through the entries in ide_table,
	 * calling all the find_ha functions. 
	 */
	for (table_offset = 0; table_offset < ide_table_entries; table_offset++) {
		struct ide_cfg_entry *config = &ide_table[table_offset];

		if ( config->cfg_flags & CFLG_INITDONE )
			continue;

		config->cfg_flags = CFLG_INIT;

		*numdrv = (config->cfg_FHA)(config, &ideidata[idata_index]);

		for (j = 0; j < *numdrv; j++) {

			struct ata_parm_entry *parms = &config->cfg_drive[j];

			(void)(config->cfg_DINIT)(j, config);

			if (parms->dpb_flags & DFLG_OFFLINE)
				continue;

			/* re-program controller with (possibly new) values */
			(void)(config->cfg_CMD)(DCMD_SETPARMS, j, config);
			ide_drivecnt++;
		}

		if (*numdrv) {
			config->cfg_flags = CFLG_INITDONE;
			*ide_table_offset = table_offset;
			return 1;
		}
	}
	return 0;
}

int
ide_doverify(
	dev_t dev,
	caddr_t argp,
	struct sb *karg)
{
	minor_t	minor = geteminor(dev);
	int	c = ide_gtol[SC_EXHAN(minor)];
	int	b = SC_BUS(minor);
	int	t = SC_EXTCN(minor);
	int	lun = SC_EXLUN(minor);
	int	gb = ide_ctob[c] + b;
	struct ata_ha *ha = ide_sc_ha[gb];
	struct ide_cfg_entry *config = ha->ha_config;
	struct ata_parm_entry *parms = &config->cfg_drive[t];
	
	char *datapt;
	struct scm *cmdpt;
	dev_t dcddev;
	int start, end;
	int  i, size;

	/* The only lun that we use is 0 all others are invalid */
	if ((t >= config->cfg_ata_drives + config->cfg_atapi_drives) || lun != 0)
		return (ENXIO);

	size = parms->dpb_sectors * parms->dpb_secsiz;

	datapt = (char *)KMEM_ZALLOC(size, ha->ha_bcbp->bcb_physreqp, KM_SLEEP);

	cmdpt = (struct scm *)(void *)SCM_RAD(karg->SCB.sc_cmdpt);

	start = sdi_swap32(cmdpt->sm_addr);
	end = sdi_swap16(cmdpt->sm_len) + start;

	for (i = start; i < end; i += parms->dpb_sectors) {
		int len;
		int startsav = i;

		parms->dpb_flags &= ~(DFLG_RETRY | DFLG_ERRCORR);
		len = ((end-i)>(int)parms->dpb_sectors) ? parms->dpb_sectors : (end-i);

		if (ide_do_readver(dev, karg, i, len, datapt, parms, c, b, t)) {
			/* gotta read each sector in track */
			int j;
			int addr;

			for (j = 0, addr = startsav; j < len; j++, addr++) {
				if (ide_do_readver(dev, karg, addr, 1, datapt, parms, c,b,t)) {
					karg->SCB.sc_comp_code = SDI_CKSTAT;
					karg->SCB.sc_status = S_CKCON;  
					parms->dpb_reqdata.sd_key = SD_MEDIUM;
					parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
					parms->dpb_reqdata.sd_len = 0xe;
					parms->dpb_reqdata.sd_ba = sdi_swap32(addr);
					parms->dpb_reqdata.sd_valid = 1;
					parms->dpb_reqdata.sd_sencode = SC_IDERR;
					parms->dpb_flags |= (DFLG_RETRY | DFLG_ERRCORR);
					if (copyout((caddr_t)karg, argp, sizeof(struct sb))) {
						KMEM_FREE(datapt, size);
						return (EFAULT);
					}
					KMEM_FREE(datapt, size);
					return (0);
				}
			}
		}

		parms->dpb_flags |= (DFLG_RETRY | DFLG_ERRCORR);
	}
	karg->SCB.sc_comp_code = SDI_ASW;	/* temp */
	karg->SCB.sc_status = S_GOOD;  	/* temp */
	if (copyout((caddr_t)karg, argp, sizeof(struct sb))) {
		KMEM_FREE(datapt, size);
		return (EFAULT);
	}
	KMEM_FREE(datapt, size);
	return (0);
}

/* return 0 if everything OK, 1 if found badblock */

int
ide_do_readver(
	dev_t dev,
	struct sb	*karg,
	int	addr,
	int	len,
	char	*datapt,
	struct ata_parm_entry *parms,
	int	c,
	int	b,
	int	t)
{
	buf_t *bp;
	struct scm *scm;
	struct sb *sp;
	int uerror = 0;
	char *save_priv;
	int	size;
	struct scsi_lu *q;

	size = len * parms->dpb_secsiz;

	sp = SDI_GETBLK(KM_SLEEP);

	save_priv = sp->SCB.sc_priv;

	bcopy((caddr_t)karg, (caddr_t)sp, sizeof(struct sb));

	bp = getrbuf(KM_SLEEP);
	bp->b_iodone = NULL;

	sp->SCB.sc_priv = save_priv;
	sp->SCB.sc_datapt = (caddr_t)datapt;
	sp->SCB.sc_cmdsz = SCM_SZ;
	sp->SCB.sc_dev.sa_lun = 0;
	sp->SCB.sc_dev.sa_bus = (uchar_t)b;
	sp->SCB.sc_dev.sa_exta = (uchar_t)t;
	sp->SCB.sc_dev.sa_ct = SDI_SA_CT(ide_ltog[c],t);

	q = &LU_Q(c, b, t, 0);
	if (q->q_sc_cmd == NULL) {
		int		gb = ide_ctob[c] + b;
		struct ata_ha  *ha = ide_sc_ha[gb];
		/*
		 * Allocate space for the SCSI command and add 
		 * sizeof(int) to account for the scm structure
		 * padding, since this pointer may be adjusted -2 bytes
		 * and cast to struct scm.  After the adjustment we
		 * still want to be pointing within our allocated space!
		 */
		q->q_sc_cmd = (char *)KMEM_ZALLOC(MAX_CMDSZ + sizeof(int), 
			ha->ha_bcbp->bcb_physreqp, KM_SLEEP) + sizeof(int);
	}
	sp->SCB.sc_cmdpt = q->q_sc_cmd;

	/* setup scsi command */
	scm = (struct scm *)(char *)SCM_RAD(sp->SCB.sc_cmdpt);
	scm->sm_op = SM_READ;
	scm->sm_lun = 0;
	scm->sm_addr = sdi_swap32(addr);
	scm->sm_len = sdi_swap16(len);

	bp->b_private = sp;

	bp->b_un.b_addr = sp->SCB.sc_datapt;
	bp->b_bcount = sp->SCB.sc_datasz = size;
	bp->b_blkno = addr;
	bp->b_edev = dev;
	bp->b_flags |= B_READ;
	ide_pass_thru(bp);  /* bypass physiock call */
	biowait(bp);

	/* update user SCB fields */
	karg->SCB.sc_time += sp->SCB.sc_time;
	if (sp->SCB.sc_comp_code != SDI_ASW)
		uerror = 1;

	freerbuf(bp);
	sdi_freeblk(sp);
	return (uerror);
}

extern int ide_halt_delay;

/*
 * void
 * idehalt(void)
 *
 * Description:
 * Called by kernel at shutdown, used to reset any devices that require it
 *
 * Calling/Exit State:
 * None.
 */

void
idehalt(void)
{

	int	c;
	struct ata_ha *ha;
	struct ide_cfg_entry *conf;
	static uchar_t ide_halt_done = 0;

	ATA_DTRACE;

	if (ide_halt_done) {
		return;
	}

	/*
	 * For each ATA device in the system, call it's halt routine
	 */
	for (c = 0; c < ide_cntls; c++) {

		/*
		 * If the controller is not active, move on
		 */
		if (!ideidata[c].active) {
			    continue;
		}

		ha = ide_sc_ha[c];
		conf = ha->ha_config;

		if (conf->cfg_HALT) {
			(conf->cfg_HALT)(conf);
		}

	}

	if( ide_halt_delay )
		drv_usecwait( ide_halt_delay * 1000000 );

	ide_halt_done = 1;
}
/*
 * ide_mca_conf(HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Read the MCA POS registers to get the hardware configuration.
 */
#define POS_SIZE 4
int
ide_mca_conf(HBA_IDATA_STRUCT *idp, int *ioalen, int *memalen)
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

	idp->ioaddr1	= (int)((pos[2] & 0x2) ? 0x3518 : 0x3510);
	*ioalen = 8;

	idp->iov	= (int)((pos[2] & 0x2) ? 15 : 14);

	idp->dmachan1	= (int)(pos[2] >> 2) & 0xf;

	if (pos[3] & 0x8) {
		idp->idata_memaddr = 0x0;
		*memalen = 0x0;
	} else {
		idp->idata_memaddr = (int)(0xC0000 + ((pos[3] & 0x7) * 0x4000));
		*memalen = 0x3FFF;
	}

	return (0);
}

/*
 * STATIC void
 * ide_getq(struct scsi_lu *q, sblk_t *sp)
 *	remove a job from a logical unit queue.
 *
 */
STATIC void
ide_getq(struct scsi_lu *q, sblk_t *sp)
{
	ASSERT(q);
	ASSERT(sp);

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
 * ide_getadr(sblk_t *sp)
 *	Return the logical address of the disk request pointed to by
 *	by sp.
 */
STATIC int
ide_getadr(sblk_t *sp)
{
	char *p;
	struct scm *scmp;
	ASSERT(sp->sbp->sb.sb_type != SFB_TYPE);
	ASSERT(IDE_IS_RW(IDE_CMDPT(sp)));
	p = (char *)sp->sbp->sb.SCB.sc_cmdpt;
	switch(p[0]) {
	case SM_READ:
	case SM_WRITE:
		scmp = (struct scm *)(p - 2);
		return sdi_swap32(scmp->sm_addr);
	case SS_READ:
	case SS_WRITE:
		return ((p[1]&0x1f) << 16) | (p[2] << 8) | p[3];
	}
	/* NOTREACHED */
}

/* STATIC int
 * ide_serial(struct ide_lu *q,sblk_t *first, sblk_t *last)
 *	Return non-zero if the last job in the chain from
 *	first to last can be processed ahead of all the 
 *	other jobs on the chain.
 */
STATIC int
ide_serial(struct scsi_lu *q, sblk_t *first, sblk_t *last)
{
	while (first != last) {
		unsigned int sb1 = ide_getadr(last);
		unsigned int eb1 = sb1 + IDE_BLKCNT(last,q) - 1;
		if (IDE_IS_WRITE(IDE_CMDPT(first)) || 
		    IDE_IS_WRITE(IDE_CMDPT(last))) {
			unsigned int sb2 = ide_getadr(first);
			unsigned int eb2 = sb2 + IDE_BLKCNT(first, q) - 1;
			if (sb1 <= sb2 && eb1 >= sb2)
				return 0;
			if (sb2 <= sb1 && eb2 >= sb1)
				return 0;
		}
		first = first->s_next;
	}
	return 1;
}
			
#define IDE_ABS_DIFF(x,y)	(x < y ? y - x : x - y)

/*
 * STATIC struct sblk_t *
 * ide_schedule(struct scsi_lu *, int)
 *	Select the next job to process.  This routine assumes jobs
 *	are placed on the queue in sorted order.
 */
STATIC sblk_t *
ide_schedule(struct scsi_lu *q)
{
	sblk_t *sp = q->q_first;
	sblk_t  *best = NULL;
	unsigned int	best_position, best_distance, distance, position;

	ASSERT(sp);
	if (IDE_QUECLASS(sp) != SCB_TYPE || !(IDE_IS_DISK(q))
				|| !IDE_IS_RW(IDE_CMDPT(sp)))
		return sp;

	best_distance = (unsigned int)~0;
	
	/* Implement shortest seek first */
	do {
		position = ide_getadr(sp);
		distance = IDE_ABS_DIFF(q->q_addr, position);
		if (distance < best_distance) {
			best_position = position;
			best_distance = distance;
			best = sp;
		}
		sp = sp->s_next;
	}
	while (sp && IDE_QUECLASS(sp) == SCB_TYPE && IDE_IS_RW(IDE_CMDPT(sp)));
	ASSERT(best);

	if (ide_serial(q, q->q_first, best)) {
		q->q_addr = best_position + IDE_BLKCNT(best, q);
		return best;
	} 

	/* Should rarely if ever get here */
	q->q_addr = ide_getadr(q->q_first) + IDE_BLKCNT(q->q_first, q);
	return q->q_first;
}

#ident	"@(#)kern-pdi:io/hba/ide/gendev.c	1.1"

#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <fs/buf.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <io/iobuf.h>
#include <util/cmn_err.h>
#include <io/target/alttbl.h>
#include <io/vtoc.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_hier.h>
#include <io/target/sdi/sdi.h>
#include <io/hba/ide/ide.h>
#include <io/hba/ide/ide_ha.h>
#include <io/hba/ide/gdev.h>
#include <io/hba/ide/ata/ata.h>
#include <io/hba/ide/ata/ata_ha.h>
#include <util/mod/moddefs.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#ifdef ATA_DEBUG_TRACE
#define ATA_DTRACE cmn_err(CE_NOTE,"%d-%s",__LINE__,__FILE__);
#else
#define ATA_DTRACE
#endif

extern struct ata_ha *ide_sc_ha[]; /* SCSI bus structs */

void
gdev_test(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) {
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
	} else if (parms->dpb_flags & DFLG_OFFLINE) {
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_DNOTRDY;
	} else {
		control->SCB_Stat = DINT_COMPLETE;
		control->SCB_TargStat = DERR_NOERR;
	}
}

/*
 *  Is the device specified in the control_area
 *  a valid drive?
 */
int
gdev_valid_drive(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct scsi_ad *sa;
	struct sb *sb;
	int drives;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	drives = config->cfg_ata_drives + config->cfg_atapi_drives;

	sb = control->c_bind;
	sa = &sb->SCB.sc_dev;

#ifdef ATA_DEBUG0
	cmn_err(CE_CONT,"cfg_ata_drives,cfg_atapi_drives=%d,%d\n",config->cfg_ata_drives,config->cfg_atapi_drives);
	cmn_err(CE_CONT,"adapter,drives,target,lun=%d,%d,%d,%d\n",control->adapter,drives,control->target,SDI_EXLUN(sa));
	cmn_err(CE_CONT,"sb address=0x%x\n",sb);
#endif

	/* The only lun that we use is 0 all others are invalid */
	if (control->target >= drives || SDI_EXLUN(sa) != 0) { 
		return 0;
	} else {
		return 1;
	}
}

void
gdev_inquir(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;

	/* The only lun that we use is 0; all others are invalid */
	if (gdev_valid_drive(control)) { 
		parms = &config->cfg_drive[control->target];
		bcopy((char *)&parms->dpb_inqdata, sb->SCB.sc_datapt, sb->SCB.sc_datasz);
		control->SCB_Stat = DINT_COMPLETE;
		control->SCB_TargStat = DERR_NOERR;
	} else {
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
	}
}

void
gdev_msense(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	struct scs *cmd = (struct scs *)(void *)sb->SCB.sc_cmdpt;
	struct mdata mdata;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) { 
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
		return;
	}

	mdata.plh.plh_type = 0;
	mdata.plh.plh_wp = parms->dpb_flags & DFLG_REMOVE ? 1 : 0;
	mdata.plh.plh_bdl = 8;

	/* check to see which page code we have */

	switch (cmd->ss_addr & 0x3f) {
	case 0x00: /* page 0 (vendor specific, in our case pdinfo ) */
		mdata.pdata.pg3.pg_pc = 0x0;
		mdata.pdata.pg3.pg_res1	= 0;
		mdata.pdata.pg3.pg_len = sizeof(struct pdinfo);
		mdata.pdata.pg0.cyls = parms->dpb_cyls;
		mdata.pdata.pg0.sectors = parms->dpb_sectors;
		mdata.pdata.pg0.bytes = parms->dpb_secsiz;
		mdata.pdata.pg0.secovhd = parms->dpb_secovhd;
		mdata.pdata.pg0.pcyls = parms->dpb_pcyls;
		mdata.pdata.pg0.psectors = parms->dpb_psectors;
		mdata.pdata.pg0.pbytes = parms->dpb_pbytes;
		mdata.pdata.pg0.interleave = parms->dpb_interleave;
		mdata.pdata.pg0.skew = parms->dpb_skew;
		break;
	case 0x03: /* page 3 */
		mdata.pdata.pg3.pg_pc = 0x3;
		mdata.pdata.pg3.pg_res1	= 0;
		mdata.pdata.pg3.pg_len = sizeof(DADF_T);
		mdata.plh.plh_len = 3 + mdata.plh.plh_bdl + mdata.pdata.pg3.pg_len;
		mdata.pdata.pg3.pg_trk_z = sdi_swap16(0);
		mdata.pdata.pg3.pg_asec_z = sdi_swap16(0);
		mdata.pdata.pg3.pg_atrk_z = sdi_swap16(0);
		mdata.pdata.pg3.pg_atrk_v = sdi_swap16(0);
		mdata.pdata.pg3.pg_sec_t = sdi_swap16(parms->dpb_sectors);
		mdata.pdata.pg3.pg_bytes_s = sdi_swap16(parms->dpb_secsiz);
		mdata.pdata.pg3.pg_intl	= sdi_swap16(parms->dpb_interleave);
		mdata.pdata.pg3.pg_trkskew = sdi_swap16(parms->dpb_skew);
		mdata.pdata.pg3.pg_cylskew = sdi_swap16(0);
		mdata.pdata.pg3.pg_res2	= 0;
		mdata.pdata.pg3.pg_ins	=0;
		mdata.pdata.pg3.pg_surf	= 0;
		mdata.pdata.pg3.pg_rmb	= parms->dpb_flags & DFLG_REMOVE ? 1 : 0;
		mdata.pdata.pg3.pg_hsec	= 0x0;
		mdata.pdata.pg3.pg_ssec	= 0x1;
		break;
	case 0x04: /* page 4 */
		mdata.pdata.pg4.pg_pc  = 0x4;
		mdata.pdata.pg4.pg_res1	= 0;
		mdata.pdata.pg4.pg_len = sizeof(RDDG_T);
		mdata.plh. plh_len = 3 + mdata.plh. plh_bdl + mdata.pdata.pg3.pg_len;
		mdata.pdata.pg4.pg_cylu	= (parms->dpb_cyls>>8) & 0xFFFF;
		mdata.pdata.pg4.pg_cyll = parms->dpb_cyls & 0xFF;
		mdata.pdata.pg4.pg_head = parms->dpb_heads;
		mdata.pdata.pg4.pg_wrpcompu = (parms->dpb_wpcyl>>8) & 0xFFFF;
		mdata.pdata.pg4.pg_wrpcompl = parms->dpb_wpcyl & 0xFF;
		mdata.pdata.pg4.pg_redwrcur = sdi_swap16(parms->dpb_parkcyl); /* ? */
		mdata.pdata.pg4.pg_drstep = sdi_swap16(1); /* ???? */
		mdata.pdata.pg4.pg_landu = (parms->dpb_parkcyl>>8) & 0xFFFF;
		mdata.pdata.pg4.pg_landl = parms->dpb_parkcyl & 0xFF;
		mdata.pdata.pg4.pg_res2	= 0;
		break;
	default:
		parms->dpb_reqdata.sd_key = SD_ILLREQ;
		parms->dpb_reqdata.sd_valid = 0;
		parms->dpb_reqdata.sd_sencode = SC_INVOPCODE;
		parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
		parms->dpb_reqdata.sd_len = 0xe;
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_UNKNOWN;
		return;
	}

	bcopy((char *)&mdata, sb->SCB.sc_datapt, cmd->ss_len);

	control->SCB_Stat = DINT_COMPLETE;
	control->SCB_TargStat = DERR_NOERR;
}

void
gdev_reqsen(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) { 
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
		return;
	}

	bcopy(SENSE_AD(&parms->dpb_reqdata), sb->SCB.sc_datapt, sb->SCB.sc_datasz);

	parms->dpb_reqdata.sd_key = SD_NOSENSE;
	parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
	parms->dpb_reqdata.sd_len = 0xe;
	parms->dpb_reqdata.sd_valid = 0;
	parms->dpb_reqdata.sd_sencode = SC_NOSENSE;

	control->SCB_Stat = DINT_COMPLETE;
	control->SCB_TargStat = DERR_NOERR;
}

void
gdev_mselect(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	struct scs *cmd = (struct scs *)(void *)sb->SCB.sc_cmdpt;
	char *page;
	struct  {
		ulong   cyls;
		ushort  heads;
		ushort  sectors;
		ushort  secsiz;
		ushort  flags;
	} sav;
	SENSE_PLH_T *plh; 
	struct pdinfo *pdinfo;
	DADF_T *pg3;
	RDDG_T *pg4;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) { 
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
		return;
	}

	/*
	 * Save the existing drive
	 * parameters in case reconfig at bottom fails.
	 */
	sav.cyls = parms->dpb_cyls;
	sav.heads = parms->dpb_heads;
	sav.sectors = parms->dpb_sectors;
	sav.secsiz = parms->dpb_secsiz;
	sav.flags = parms->dpb_flags;

	plh = (SENSE_PLH_T *)(void *)(sb->SCB.sc_datapt);
	page = (char *)(sb->SCB.sc_datapt) + SENSE_PLH_SZ + plh->plh_bdl;

	/* check to see which page code we have */

	switch (cmd->ss_addr & 0x3f) {
	case 0x00: /* page 0 (vendor specific, in our case pdinfo ) */
		pdinfo = (struct pdinfo *)(void *)page;
		parms->dpb_cyls = pdinfo->cyls;
		parms->dpb_sectors = pdinfo->sectors;
		parms->dpb_secsiz = pdinfo->bytes;
		parms->dpb_secovhd = pdinfo->secovhd;
		parms->dpb_pcyls = pdinfo->pcyls;
		parms->dpb_psectors = pdinfo->psectors;
		parms->dpb_pbytes = pdinfo->pbytes;
		parms->dpb_interleave = pdinfo->interleave;
		parms->dpb_skew = pdinfo->skew;
		break;
	case 0x03: /* page 3 */
		pg3 =  (DADF_T *) (void *)page;
		parms->dpb_sectors = pg3->pg_sec_t ;
		break;
	case 0x04: /* page 4 */
		pg4 =  (RDDG_T *) (void *)page;
		parms->dpb_cyls = (pg4->pg_cylu << 8) | pg4->pg_cyll;
		parms->dpb_heads = pg4->pg_head;
	default:
		parms->dpb_reqdata.sd_key = SD_ILLREQ;
		parms->dpb_reqdata.sd_valid = 0;
		parms->dpb_reqdata.sd_sencode = SC_INVOPCODE;
		parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
		parms->dpb_reqdata.sd_len = 0xe;
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_UNKNOWN;
		return;
	}

	/*
 	 * Reconfigure the controller.
 	 * If all ok, return.  If not, restore old params and config again.
 	 */
#ifdef OLD_STUFF
	oldpri = SPLDISK;
#endif
	/* save time when command was started */
	drv_getparm(LBOLT, &config->cfg_laststart);
	(void)(config->cfg_CMD)(DCMD_SETPARMS, control->target, config);
	if (sb->SCB.sc_comp_code != SDI_ASW) {       
		/* command failed.  Put it back... */
		parms->dpb_cyls = sav.cyls;
		parms->dpb_heads = sav.heads;
		parms->dpb_sectors = sav.sectors;
		parms->dpb_secsiz = sav.secsiz;
		parms->dpb_flags = sav.flags;
		/* save time when command was started */
		drv_getparm(LBOLT, &config->cfg_laststart);
		(void)(config->cfg_CMD)(DCMD_SETPARMS, control->target, config);
	}
#ifdef OLD_STUFF
	splx(oldpri);
#endif
}

void
gdev_rdcap(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	CAPACITY_T cap;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) { 
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
		return;
	}

	cap.cd_addr = sdi_swap32(parms->dpb_heads * parms->dpb_cyls * parms->dpb_sectors);
	cap.cd_len = sdi_swap32(parms->dpb_secsiz);

	bcopy((char *)&cap, sb->SCB.sc_datapt, RDCAPSZ);

	control->SCB_Stat = DINT_COMPLETE;
	control->SCB_TargStat = DERR_NOERR;
}

void
gdev_format(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	struct scs *cmd = (struct scs *)(void *)sb->SCB.sc_cmdpt;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) { 
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
		return;
	}

	parms->dpb_reqdata.sd_key = SD_ILLREQ;
	parms->dpb_reqdata.sd_valid = 0;
	parms->dpb_reqdata.sd_sencode = SC_NOSENSE;
	parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
	parms->dpb_reqdata.sd_len = 0xe;

	control->SCB_Stat = DINT_ERRABORT;
	control->SCB_TargStat = DERR_UNKNOWN;
}

void
gdev_reasgn(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	int smx_addr;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) { 
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
		return;
	}

	smx_addr = ((int *)(void *)(sb->SCB.sc_datapt))[1];

	parms->dpb_reqdata.sd_ba = smx_addr;
	parms->dpb_reqdata.sd_key = SD_MEDIUM;
	parms->dpb_reqdata.sd_valid = 1;
	parms->dpb_reqdata.sd_sencode = SC_NODFCT;
	parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
	parms->dpb_reqdata.sd_len = 0xe;

	control->SCB_Stat = DINT_ERRABORT;
	control->SCB_TargStat = DERR_UNKNOWN;
}

void
gdev_verify(struct control_area *control)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	struct scs *cmd = (struct scs *)(void *)sb->SCB.sc_cmdpt;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

	/* The only lun that we use is 0 all others are invalid */
	if (!gdev_valid_drive(control)) { 
		control->SCB_Stat = DINT_ERRABORT;
		control->SCB_TargStat = DERR_BADCMD;
		return;
	}

	parms->dpb_reqdata.sd_key = SD_ILLREQ;
	parms->dpb_reqdata.sd_valid = 0;
	parms->dpb_reqdata.sd_sencode = SC_NOSENSE;
	parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
	parms->dpb_reqdata.sd_len = 0xe;

	control->SCB_Stat = DINT_ERRABORT;
	control->SCB_TargStat = DERR_UNKNOWN;
}

int
gdev_sb2drq(struct control_area *control, int sleepflag)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	int direction, sector, count, secsiz, command;
	ulong spc;

	ATA_DTRACE;

	ha = ide_sc_ha[control->adapter];
	config = ha->ha_config;
	parms = &config->cfg_drive[control->target];

#ifdef NOT_YET
	if (!(config->cfg_capab & CCAP_NOSEEK)) {

		spc = parms->dpb_sectors * parms->dpb_heads;
		parms->dpb_cursect = drq_daddr(drqp);

		if ((parms->dpb_cursect/(daddr_t)spc)!=parms->dpb_curcyl){

			drv_getparm(LBOLT, &config->cfg_laststart);

			parms->dpb_flags |= DFLG_BUSY;
			config->cfg_flags |= CFLG_BUSY;

			config->cfg_CMD(DCMD_SEEK, control->target, config);

#ifdef OLD_STUFF
			drqp->drq_type = DRQT_CONT;
#endif
			parms->dpb_curcyl = parms->dpb_cursect/(daddr_t)spc;
			return;
		}
	}
#endif

	if (sb->SCB.sc_cmdsz == SCS_SZ) {
		struct scs * scs;
		scs = (struct scs *)(void *)sb->SCB.sc_cmdpt;
		direction = scs->ss_op == SS_READ ? B_READ : B_WRITE;
		sector = sdi_swap16(scs->ss_addr) + ((scs->ss_addr1 << 16 ) & 0x1f0000);
		count = scs->ss_len;
	} else if (sb->SCB.sc_cmdsz == SCM_SZ) {
		struct scm * scm;
		scm = (struct scm *)(void *)(SCM_RAD(sb->SCB.sc_cmdpt));
		direction = scm->sm_op == SM_READ ? B_READ : B_WRITE;
		sector = (unsigned int)sdi_swap32(scm->sm_addr);
		count = sdi_swap16(scm->sm_len);
	} else {
		return (-1);
	}

	if (direction == B_READ) {
		parms->dpb_flags |= DFLG_READING;
		command = DCMD_READ;
	} else {
		parms->dpb_flags &= ~DFLG_READING;
		command = DCMD_WRITE;
	}

	parms->dpb_flags |= DFLG_BUSY;
	config->cfg_flags |= CFLG_BUSY;

	secsiz = parms->dpb_secsiz;

	parms->dpb_cursect = sector;
	parms->dpb_sectcount = count;

	parms->dpb_newaddr = parms->dpb_curaddr = (paddr_t)sb->SCB.sc_datapt;
	parms->dpb_newvaddr = parms->dpb_virtaddr = (paddr_t)sb->SCB.sc_datapt;
	parms->dpb_newcount = parms->drq_count = sb->SCB.sc_datasz;;

	drv_getparm(LBOLT, &config->cfg_laststart);

	config->cfg_CMD(command, control->target, config);

	return 0;
}

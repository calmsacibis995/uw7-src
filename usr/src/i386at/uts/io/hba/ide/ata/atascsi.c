#ident	"@(#)kern-pdi:io/hba/ide/ata/atascsi.c	1.2"

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

/*
 * ata_send_cmd -- translate and send a command to the controller.
 */
void
ata_send_cmd(struct ide_cfg_entry *config, struct control_area *control)
{
	int sleepflag, curdrv = control->target;
	struct ata_parm_entry *parms = &config->cfg_drive[curdrv];

	ATA_DTRACE;

	sleepflag = KM_NOSLEEP;

	parms->dpb_control = control;

	if ( parms->dpb_devtype == DTYP_DISK ) {
		(void)ata_ata_cmd(control, sleepflag);
	} else {
		(void)ata_atapi_cmd(control, sleepflag);
	}
}

#ifndef SS_FMT_UNIT
#define SS_FMT_UNIT 0x04
#endif

#ifndef SM_VERIFY
#define SM_VERIFY 0x2f
#endif

int
ata_ata_cmd(struct control_area *control, int sleepflag)
{
	struct  sb *sb = control->c_bind;

	ATA_DTRACE;

	control->SCB_Stat = DINT_COMPLETE;

	switch (sb->sb_type) {
	case SFB_TYPE:
		switch (sb->SFB.sf_func) {
		case SFB_ABORTM:
		case SFB_RESETM:
		default:
			control->SCB_Stat = DINT_ERRABORT;
			control->SCB_TargStat = DERR_BADCMD;
			break;
		}
		ide_job_done(control);
		break;
	case ISCB_TYPE:
	case SCB_TYPE:
		if(sb->SCB.sc_cmdsz == SCS_SZ) { /* 6 byte SCSI command */
			struct scs *scsp;
			scsp = (struct scs *)(void *)sb->SCB.sc_cmdpt;
			switch (scsp->ss_op) {
			case SS_TEST:
				gdev_test(control);
				ide_job_done(control);
				break;
			case SS_REQSEN:
				gdev_reqsen(control);
				ide_job_done(control);
				break;
			case SS_READ:
			case SS_WRITE:
				control->SCB_Stat = DINT_CONTINUE;
				if (gdev_sb2drq(control, sleepflag))
					return (-1);
				break;
			case SS_INQUIR:
				gdev_inquir(control);
				ide_job_done(control);
				break;
			case SS_MSELECT:
				gdev_mselect(control);
				ide_job_done(control);
				break;
			case SS_MSENSE:
				gdev_msense(control);
				ide_job_done(control);
				break;
			case SS_REZERO:
			case SS_RESERV:
			case SS_RELES:
			case SS_SDDGN:
			case SS_SEEK:
			case SS_ST_SP:
				control->SCB_Stat = DINT_COMPLETE;
				ide_job_done(control);
				break;
			case SS_REASGN:
				gdev_reasgn(control);
				ide_job_done(control);
				break;
			case SS_FMT_UNIT:
				gdev_format(control);
				ide_job_done(control);
				break;
			case SS_LOCK:
			default:
				control->SCB_Stat = DINT_ERRABORT;
				control->SCB_TargStat = DERR_BADCMD;
				ide_job_done(control);
				break;
			}
		} else if (sb->SCB.sc_cmdsz == SCM_SZ) { /* 10 byte SCSI command */
			struct scm *scm;
			scm = (struct scm *)(void *)(SCM_RAD(sb->SCB.sc_cmdpt));
			switch (scm->sm_op) {
			case SM_RDCAP:
				gdev_rdcap(control);
				ide_job_done(control);
				break;
			case SM_READ:
			case SM_WRITE:
				control->SCB_Stat = DINT_CONTINUE;
				if (gdev_sb2drq(control, sleepflag))
					return (-1);
				break;
			case SM_VERIFY:
				gdev_verify(control);
				ide_job_done(control);
				break;
			case SM_SEEK:
			case SM_RDDL:
			case SM_RDDB:
			case SM_WRDB:
				control->SCB_Stat = DINT_COMPLETE;
				ide_job_done(control);
				break;
			default:
				control->SCB_Stat = DINT_ERRABORT;
				control->SCB_TargStat = DERR_BADCMD;
				ide_job_done(control);
				break;
			}
		} else {
			control->SCB_Stat = DINT_ERRABORT;
			control->SCB_TargStat = DERR_BADCMD;
			ide_job_done(control);
		}
		break;
	default:
		break;
		
	}
	return (0);
}

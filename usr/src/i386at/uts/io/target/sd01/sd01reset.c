#ident	"@(#)kern-pdi:io/target/sd01/sd01reset.c	1.1.4.3"
#ident	"$Header$"

#define	_DDI	8

/* sd01 timeout failure recovery */

#include        <util/types.h>
#include        <io/vtoc.h>
#include        <io/target/sd01/fdisk.h>
#include        <fs/buf.h>
#include        <io/conf.h>
#include        <io/elog.h>
#include        <io/open.h>
#include        <io/target/altsctr.h>
#include        <io/target/alttbl.h>
#include        <io/target/scsi.h>
#include        <io/target/sd01/sd01.h>
#include        <io/target/sd01/sd01_ioctl.h>
#include        <io/target/sdi/dynstructs.h>
#include        <io/target/sdi/sdi.h>
#include        <io/target/sdi/sdi_edt.h>
#include        <io/target/sdi/sdi_hier.h>
#include        <io/uio.h>      /* Included for uio structure argument*/
#include        <mem/kmem.h>    /* Included for DDI kmem_alloc routines */
#include        <proc/cred.h>   /* Included for cred structure arg   */
#include        <proc/proc.h>   /* Included for proc structure arg   */
#include        <svc/errno.h>
#include        <util/cmn_err.h>
#include        <util/debug.h>
#include        <util/mod/moddefs.h>
#include        <util/param.h>
#include        <util/ipl.h>
#include        <util/ksynch.h>
#include        <io/ddi.h>      /* must come last */

void sd01int_test_unit_ready(struct sb *sbp);
void sd01int_request_sense(struct sb *sbp);
void sd01int_reset(struct sb *sbp);
void sd01int_retry(struct sb *sbp);
void sd01_next_phase(struct disk *dk);
void sd01_recovery_complete(struct sb *sbp);

/* sd01_gauntlet:
 *
 *	Called from sdi_start_gauntlet if this is the first job through
 *	the gauntlet.  Otherwise it is called from sdi_end_gauntlet.
 *
 *	If clean_abort is non-zero, we retry the job and go to device reset
 *	on failure.
 *
 *	If clean_abort is zero, we reset the device immediately.
 */
void
sd01_gauntlet(struct sb *sbp, int clean_abort)
{
	struct job *jp = (struct job *)sbp->SCB.sc_wd;
	struct disk *dk = jp->j_dk;
	struct gauntlet *gauntlet = &dk->dk_gauntlet;
	gauntlet->g_sb = sbp;
	gauntlet->g_int = sbp->SCB.sc_int;
	gauntlet->g_state = GAUNTLET_START;
	jp->j_flags |= J_IN_GAUNTLET;

	cmn_err(CE_CONT, "!sd01_gauntlet(sb:0x%x)\n", sbp);
	
	if (clean_abort) {
		sd01test_unit_ready(dk);
		return;
	}
	sd01_next_phase(dk);
}

/* sd01test_unit_ready:
 *
 *	Issue test unit ready command.
 *
 *	The is the first phase of an attempted job retry.
 */
sd01test_unit_ready(struct disk *dk)
{
	struct sb  *sbp = dk->dk_gauntlet.g_tur_sb;
	struct scs *cmd = &dk->dk_gauntlet.g_scs;

	sbp->SCB.sc_int = sd01int_test_unit_ready;
	sbp->SCB.sc_resid = 0;
	sbp->SCB.sc_time = sd01_get_timeout(SS_TEST, dk);
	cmd->ss_op = SS_TEST;
	cmd->ss_lun = sbp->SCB.sc_dev.sa_lun;
	cmd->ss_addr1 = 0;
	cmd->ss_addr = 0;
	cmd->ss_len = 0;
	cmd->ss_cont = 0;
	dk->dk_sense->sd_key = SD_NOSENSE;

	if (sdi_xicmd(dk->dk_addr.pdi_adr_version, sbp, KM_NOSLEEP) != SDI_RET_OK)
		sd01_recovery_complete(dk->dk_gauntlet.g_sb);
	return;
}

/* sd01int_test_unit_ready:
 *
 *	This is the second phase of the retry mechanism. It
 *	is called from sdi_callback upon completion of a
 *	gauntlet generated test unit ready.  The original command
 *	may be retried if the test unit ready was successful.  
 *	If a check condition was generated for a target reset, issue 
 *	a request sense before doing the retry.  Only attempt the
 *	retry if the sense data is what was expected.
 */
void
sd01int_test_unit_ready(struct sb *sbp)
{
	struct job *jp = (struct job *)sbp->SCB.sc_wd;
	struct disk *dk = jp->j_dk;
	struct sb *osbp = dk->dk_gauntlet.g_sb;
	int code = sbp->SCB.sc_comp_code;
	struct gauntlet *gauntlet = &dk->dk_gauntlet;

	/*
	 * If this test unit ready were to fail it will cause
	 * the gauntlet to execute completely and exit. Because of this we
	 * must make sure that the call back sc_int field is reset so we
	 * don't get called back inadvertently.
	 */
	sbp->SCB.sc_int = NULL;

	if (code == SDI_ASW) {
		osbp->SCB.sc_int = sd01int_retry;
		cmn_err(CE_CONT, "!call sd01retry(sb:0x%x)\n",osbp);
		sd01retry(osbp->SCB.sc_wd);
		return;
	}
	if (code == SDI_CKSTAT) {
		sd01request_sense(dk);
		return;
	}
	if (sbp->SCB.sc_comp_code == SDI_RETRY) {
		cmn_err(CE_CONT, "Retrying test unit ready\n");
		sd01test_unit_ready(dk);
		return;
	}
	sd01_next_phase(dk);
}

/* sd01request_sense:
 *
 *	Called from sd01int_test_unit_ready if a check condition
 *	was raised.  Issue the request sense.
 */
sd01request_sense(struct disk *dk)
{
	struct sb  *sbp = dk->dk_gauntlet.g_req_sb;
	struct scs *cmd = &dk->dk_gauntlet.g_scs;

	sbp->SCB.sc_int = sd01int_request_sense;
	sbp->SCB.sc_resid = 0;
	sbp->SCB.sc_time = sd01_get_timeout(SS_REQSEN, dk);
	cmd->ss_op = SS_REQSEN;
	cmd->ss_lun = sbp->SCB.sc_dev.sa_lun;
	cmd->ss_addr1 = 0;
	cmd->ss_addr = 0;
	cmd->ss_len = SENSE_SZ;
	cmd->ss_cont = 0;
	dk->dk_sense->sd_key = SD_NOSENSE;

	if (sdi_xicmd(dk->dk_addr.pdi_adr_version, sbp, KM_NOSLEEP) != SDI_RET_OK)
		sd01_recovery_complete(dk->dk_gauntlet.g_sb);
	return;
}

void
sd01int_request_sense(struct sb *sbp)
{
	struct job *jp = (struct job *)sbp->SCB.sc_wd;
	struct disk *dk = jp->j_dk;
	struct sb *osbp = dk->dk_gauntlet.g_sb;

	/* Insure we are not called recursively. */
	sbp->SCB.sc_int = NULL;

	if (sbp->SCB.sc_comp_code == SDI_ASW) {
		/* We may want to verify that the sense data
		 * is what we expected.
		 */
		osbp->SCB.sc_int = sd01int_retry;
		cmn_err(CE_CONT, "!call sd01retry(sb:0x%x)\n",osbp);
		sd01retry(osbp->SCB.sc_wd);
		return;
	}
	if (sbp->SCB.sc_comp_code == SDI_RETRY) {
		cmn_err(CE_CONT, "Retrying Request Sense\n");
		sd01request_sense(dk);
		return;
	}
	sd01_next_phase(dk);
}

/* sd01int_reset
 *
 *	We are here when the bus or device reset completed.  If the reset was
 *	successful, retry the job.  Otherwise go to the next phase of
 *	recovery.
 */
void
sd01int_reset(struct sb *sbp)
{
	struct job *jp = (struct job *)sbp->SFB.sf_wd;
	struct disk *dk = jp->j_dk;

	if (sbp->SCB.sc_comp_code == SDI_ASW) {
		sd01test_unit_ready(dk);
		return;
	}
	sd01_next_phase(dk);
}

/* sd01int_retry:
 *
 *	Interrupt handler for retried jobs.  Either end gauntlet
 *	if the job was successful or go to the next phase of recovery.
 */
void
sd01int_retry(struct sb *sbp)
{
	struct job *jp = (struct job *)sbp->SCB.sc_wd;
	struct disk *dk = jp->j_dk;

	if (sbp->SCB.sc_comp_code == SDI_ASW) {
		sd01_recovery_complete(sbp);
		return;
	}
	if (sbp->SCB.sc_comp_code == SDI_RETRY) {
		cmn_err(CE_CONT, "Retrying job retry\n");
		sd01retry(jp);
		return;
	}
	sd01_next_phase(dk);
}

/* sd01_recovery_complete:
 *
 *	Recovery is complete.  The original job's status can be
 *	used to tell if recovery was successful.  Call the original
 *	interrupt routine and then signal the gauntlet is over.
 */
void
sd01_recovery_complete(struct sb *sbp)
{
	struct job *jp = (struct job *)sbp->SCB.sc_wd;
	struct disk *dk = jp->j_dk;
	struct scsi_adr sa;

	cmn_err(CE_CONT, "sd01_recovery_complete(sb:0x%x)\n",sbp);

	if (! dk->dk_addr.pdi_adr_version ) {
		sa.scsi_ctl = SDI_EXHAN(&dk->dk_addr);
		sa.scsi_target = SDI_EXTCN(&dk->dk_addr);
		sa.scsi_lun = SDI_EXLUN(&dk->dk_addr);
		sa.scsi_bus = SDI_BUS(&dk->dk_addr);
	}
	else {
		sa.scsi_ctl = SDI_HAN_32(&dk->dk_addr);
		sa.scsi_target = SDI_TCN_32(&dk->dk_addr);
		sa.scsi_lun = SDI_LUN_32(&dk->dk_addr);
		sa.scsi_bus = SDI_BUS_32(&dk->dk_addr);
	}

	jp->j_flags &= ~J_IN_GAUNTLET;
	jp->j_flags |= J_DONE_GAUNTLET;
	(*dk->dk_gauntlet.g_int)(sbp);
	sdi_end_gauntlet(&sa);
}

/* sd01_next_phase(dk)
 *
 *	Start the next phase of recovery.
 */
void
sd01_next_phase(struct disk *dk)
{
	struct scsi_adr sa;
	struct sb *sbp;
	int *state = &dk->dk_gauntlet.g_state;

	if (! dk->dk_addr.pdi_adr_version ) {
		sa.scsi_ctl = SDI_EXHAN(&dk->dk_addr);
		sa.scsi_target = SDI_EXTCN(&dk->dk_addr);
		sa.scsi_lun = SDI_EXLUN(&dk->dk_addr);
		sa.scsi_bus = SDI_BUS(&dk->dk_addr);
	}
	else {
		sa.scsi_ctl = SDI_HAN_32(&dk->dk_addr);
		sa.scsi_target = SDI_TCN_32(&dk->dk_addr);
		sa.scsi_lun = SDI_LUN_32(&dk->dk_addr);
		sa.scsi_bus = SDI_BUS_32(&dk->dk_addr);
	}
		
	if (dk->dk_gauntlet.g_state & GAUNTLET_START) {
		*state &= ~GAUNTLET_START;
		*state |= DEVICE_RESET;
		sbp = dk->dk_gauntlet.g_dev_sb;
		sbp->SFB.sf_int = sd01int_reset;
		if (sdi_device_reset(&sa, sbp) != SDI_RET_OK) {
			sd01_next_phase(dk);
			return;
		}
	} else if (dk->dk_gauntlet.g_state & DEVICE_RESET) {
		*state &= ~DEVICE_RESET;
		*state |= BUS_RESET;
		sbp = dk->dk_gauntlet.g_bus_sb;
		sbp->SFB.sf_int = sd01int_reset;
		if (sdi_bus_reset(&sa,sbp) != SDI_RET_OK) {
			sd01_next_phase(dk);
			return;
		}
	} else if (dk->dk_gauntlet.g_state & BUS_RESET) {
		*state &= ~BUS_RESET;
		*state |= GAUNTLET_COMPLETE;
		sd01_recovery_complete(dk->dk_gauntlet.g_sb);
	}
}

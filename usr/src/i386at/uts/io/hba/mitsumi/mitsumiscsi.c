#ident	"@(#)kern-pdi:io/hba/mitsumi/mitsumiscsi.c	1.7"
#ident	"$Header$"

/*******************************************************************************
 *******************************************************************************
 *
 *	MITSUMISCSI.C
 *
 *	SCSI Emulation routines for the MITSUMI
 *
 *	MITSUMI MITSUMI Driver  Version 0.00
 *
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
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>

#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#include <io/hba/mitsumi.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#include <io/hba/mitsumi/mitsumi.h>
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
#include <sys/mitsumi.h>

#include <sys/moddefs.h>
#include <sys/dma.h>
#include <sys/hba.h>

#if PDI_VERSION >= PDI_SVR42MP
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#endif /* _KERNEL_HEADERS */
/*******************************************************************************
 *
 *	EXTERNALS
 *
 ******************************************************************************/

extern char     mitsumi_vendor[];
extern char     mitsumi_diskprod[];
extern HBA_IDATA_STRUCT	mitsumiidata[];		/* hardware info */

extern	scsi_ha_t	*mitsumi_sc_ha;	/* host adapter structs */
extern int   mitsumi_gtol[];	/* global to local */
extern int   mitsumi_ltog[];	/* local to global */
extern ctrl_info_t *mitsumi_ctrl;	/* local controller structs */

/*******************************************************************************
 *
 *	PROTOTYPES
 *
 ******************************************************************************/

#ifndef	BIN2BCD
unchar 		BIN2BCD(unchar b);
unchar		BCD2BIN(unchar b);
#endif

/*
 * Emulation Routines
 */

int             mitsumi_test(struct sb * sb);
void            mitsumi_inquir(struct sb * sb);
void            mitsumi_sense(struct sb * sb);
void            mitsumi_format(struct sb * sb);
void            mitsumi_rdcap(struct sb * sb, int flag);
void            mitsumi_msense(struct sb * sb);

/*
 * mitsumi support
 */

/*******************************************************************************
 *
 *	mitsumi_inquir ( sb )
 *
 *	Convert an SS_INQUIR command
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
mitsumi_inquir(struct sb * sb)
{
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	struct ident   *inq_data;
	int             i;
	int             c, t, l;
	scsi_ha_t *ha;

	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &mitsumi_sc_ha[c];

	if (t == ha->ha_id) {
		/*
				 * they are checking out the processor card
				 */
		if (l == 0) {
			/*
						 * lun 0 is the only valid lun on the processor card
						 */
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, mitsumiidata[c].name, 
				VID_LEN+PID_LEN+REV_LEN);
			inq.id_type = ID_PROCESOR;

			inq_data = (struct ident *)(void *)sb->SCB.sc_datapt;
			inq_len = sb->SCB.sc_datasz;
			bcopy((char *)&inq, (char *)inq_data, inq_len);
			sb->SCB.sc_comp_code = SDI_ASW;
			return;
		}
		/*
		 * invalid lun on the processor card
		 */
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}
	/*
	 * inquiring about disk drives
	 */

	if ((l != 0) || (t >= mitsumi_ctrl[c].num_drives)) {
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
#ifdef MITSUMI_DEBUG
		cmn_err(CE_NOTE, "INQUIRY, no such target");
#endif
		return;
	}
	inq_data = (struct ident *)(void *) sb->SCB.sc_datapt;
	inq_data->id_type = ID_ROM;
	inq_data->id_rmb = 1;
	inq_data->id_pqual = 0;
	for (i = 0; i < VID_LEN; i++)
		inq_data->id_vendor[i] = mitsumi_vendor[i];
	for (i = 0; i < PID_LEN; i++)
		inq_data->id_prod[i] = mitsumi_diskprod[i];
	inq_data->id_prod[i - 1] = 0x00;	/* zero terminate for grins */
	inq_data->id_len = IDENT_SZ - 5;
	sb->SCB.sc_comp_code = SDI_ASW;
}

/*******************************************************************************
 *
 *	mitsumi_test ( struct sb *sb )
 *
 *	Handle SS_TEST scsi request
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

int
mitsumi_test(struct sb * sb)
{
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	int	mitsumi_chkmedia();
	scsi_ha_t *ha;

	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &mitsumi_sc_ha[c];

	if ((l != 0) || ((t != ha->ha_id) && (t >= mitsumi_ctrl[c].num_drives))) {
		/*
				 * Not a valid lun or not a valid target
				 */
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return(-1);
	}
	ha->ha_cmd_sp    = NULL;
	return(mitsumi_chkmedia(c));
}

/*******************************************************************************
 *
 *	mitsumi_sense ( struct sb *sb )
 *
 *	Figure out sense information on a given device
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
mitsumi_sense(struct sb * sb)
{
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t;
	drv_info_t     *drv;

	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	drv = mitsumi_ctrl[c].drive[t];
	
	drv->mitsumi_sense.sd_len = 6;
	drv->mitsumi_sense.sd_errc = 0x70;
	drv->mitsumi_sense.sd_key = 0;
	drv->mitsumi_sense.sd_sencode = 0;

	if (drv->status & MITSUMI_DISK_CHANGED) {
		/*
		 *	somebody changed the disk
		 */
		drv->mitsumi_sense.sd_key = SD_UNATTEN;
		drv->mitsumi_sense.sd_sencode = SC_MEDCHNG;
		drv->mitsumi_sense.sd_valid = 1;
		drv->status &= ~MITSUMI_DISK_CHANGED;
	}
	else {
		/*
	 	 *	normal drive happenings
		 */
		switch ( drv->lasterr) {
		default:
	 		drv->mitsumi_sense.sd_key = SD_ILLREQ;
	 		drv->mitsumi_sense.sd_valid = 1;

			/*
			 *	something really went wrong
			 */
	 		drv->mitsumi_sense.sd_key = SD_HARDERR;
	 		drv->mitsumi_sense.sd_valid = 1;
		}
	}

	bcopy (SENSE_AD(&drv->mitsumi_sense),sb->SCB.sc_datapt,SENSE_SZ);
	drv->mitsumi_sense.sd_valid = 0;
}

/*******************************************************************************
 *
 *	mitsumi_format ( struct sb *sb )
 *
 *	Format the mitsumi stuff -- pretty easy since you can't format
 *	an MITSUMI drive
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
mitsumi_format(struct sb * sb)
{
	sb->SCB.sc_comp_code = (ulong_t) SDI_HAERR;
	return;
}

/*******************************************************************************
 *
 *	mitsumi_rdcap ( struct sb *sb )
 *
 *	Tell the pdi about the capacity of the drive
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
mitsumi_rdcap(struct sb * sb, int copyflag)
{
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	capacity_t      cap;
	drv_info_t     *drv;
	int             c, t, l;
	int		n, status;
	scsi_ha_t 	*ha;
	int		msf2sector();
	int		mitsumi_sendcmd(), mitsumi_getanswer();

	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &mitsumi_sc_ha[c];

	if ((l != 0) || (t >= mitsumi_ctrl[c].num_drives)) {
		cmn_err(CE_WARN, "mitsumi: Unable to read disk capacity\n");
		if(copyflag)
			sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}
	drv = mitsumi_ctrl[c].drive[t];
	status = mitsumi_sendcmd(ha->ha_iobase, MITSUMI_REQUEST_TOC_CMD, 250, NULL, 0, 1);
	if(status < 0 || (status & MITSUMI_ERROR)) {
		cmn_err(CE_WARN, "mitsumi: Unable to read disk capacity\n");
		if(copyflag)
			sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}

	if ((n = mitsumi_getanswer(ha->ha_iobase, (unsigned char *) drv, 8)) != 8) {
		cmn_err(CE_NOTE, "mitsumi_rdcap: got %d byte(s)only", n);
		if(copyflag)
			sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}
	drv->starting_track = (int) BCD2BIN(drv->trk_low);
	drv->ending_track = (int) BCD2BIN(drv->trk_high);

	drv->startsect = msf2sector(drv->start_sect_m, drv->start_sect_s,
				 drv->start_sect_f);
	drv->lastsect  = msf2sector(drv->end_sect_m, drv->end_sect_s,
				 drv->end_sect_f);
	drv->drvsize = drv->lastsect - drv->startsect;

	cap.cd_addr = sdi_swap32 ( drv->lastsect - drv->startsect);
	cap.cd_len = sdi_swap32 (MITSUMI_BLKSIZE);
	if(copyflag) {
		bcopy ( (char *)&cap, sb->SCB.sc_datapt, RDCAPSZ );
		sb->SCB.sc_comp_code = SDI_ASW;
	}
#ifdef MITSUMI_DEBUG
	cmn_err(CE_NOTE, "mitsumi_rdcap: start block %x, last block %x", 
			drv->startsect, drv->lastsect);
#endif
}

/*******************************************************************************
 *
 *	mitsumi_msense ( struct sb *sb )
 *
 *	Issue a mode sense
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
mitsumi_msense(struct sb * sb)
{
#ifdef SENSE_SUPPORTED
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	struct scs *cmd = (struct scs *)sb->SCB.sc_cmdpt;
	drv_info_t	*drv;

	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

#ifdef MITSUMI_DEBUG
	cmn_err(CE_CONT, "mitsumi: Mode Sense C=%d T=%d L=%d P=0x%x\n", 
	    c, t, l, cmd->ss_addr );
#endif

	/*
	 * Fill in the mode sense
	 */

	if ((l != 0) || (t >= mitsumi_ctrl[c].num_drives)) {
		cmn_err(CE_CONT, "mitsumi: Unable to mode sense\n");
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}
	drv = mitsumi_ctrl[c].drive[t];
#endif
	sb->SCB.sc_comp_code = SDI_ASW;
}

#ident	"@(#)kern-pdi:io/hba/lmsi/lmsiscsi.c	1.3"
#ident	"$Header$"

/*******************************************************************************
 *******************************************************************************
 *
 *	LMSISCSI.C
 *
 *	SCSI Emulation routines for the LMSI
 *
 *	LMSI 205/250 Driver  Version 0.00
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
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#endif /* !(PDI_VERSION <= 1) */

#include <io/hba/lmsi/lmsi.h>
#include <io/hba/hba.h>
#include <util/mod/moddefs.h>
#include <io/dma.h>

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
#include <sys/lmsi.h>

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

extern char     lmsi_vendor[];
extern char     lmsi_diskprod[];
extern HBA_IDATA_STRUCT	lmsiidata[];		/* hardware info */

extern	scsi_ha_t	*lmsi_sc_ha;	/* host adapter structs */
extern int   lmsi_gtol[];	/* global to local */
extern int   lmsi_ltog[];	/* local to global */
extern ctrl_info_t *lmsi_ctrl;	/* local controller structs */

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

void            lmsi_inquir(struct sb * sb);
void            sense(struct sb * sb);
void            lmsi_format(struct sb * sb);
void            lmsi_msense(struct sb * sb);

/*
 * lmsi support
 */

/*******************************************************************************
 *
 *	lmsi_inquir ( sb )
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
lmsi_inquir(struct sb * sb)
{
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             i;
	int             c, t, l;
	scsi_ha_t *ha;
	struct ident *inq_data;
	struct ident inq;
	int inq_len;

	c = lmsi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &lmsi_sc_ha[c];

	if (t == ha->ha_id) {
		/*
				 * they are checking out the processor card
				 */
		if (l == 0) {
			/*
						 * lun 0 is the only valid lun on the processor card
						 */
			bzero(&inq, sizeof(struct ident));
			(void)strncpy(inq.id_vendor, lmsiidata[c].name, 
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

	if ((l != 0) || (t >= lmsi_ctrl[c].num_drives)) {
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
#ifdef LMSI_DEBUG
		cmn_err(CE_NOTE, "INQUIRY, no such target");
#endif
		return;
	}
	bzero(&inq, sizeof(struct ident));
	inq.id_type = ID_ROM;
	inq.id_rmb = 1;
	for (i = 0; i < VID_LEN; i++)
		inq.id_vendor[i] = lmsi_vendor[i];
	for (i = 0; i < PID_LEN; i++)
		inq.id_prod[i] = lmsi_diskprod[i];
	inq.id_prod[i - 1] = 0x00;	/* zero terminate for grins */
	inq.id_len = IDENT_SZ - 5;
	inq_data = (struct ident *)(void *)sb->SCB.sc_datapt;
	inq_len = sb->SCB.sc_datasz;
	bcopy((char *)&inq, (char *)inq_data, inq_len);
	sb->SCB.sc_comp_code = SDI_ASW;
}

/*******************************************************************************
 *
 *	lmsi_format ( struct sb *sb )
 *
 *	Format the lmsi stuff -- pretty easy since you can't format
 *	an LMSI drive
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
lmsi_format(struct sb * sb)
{
	sb->SCB.sc_comp_code = (ulong_t) SDI_HAERR;
	return;
}

/*******************************************************************************
 *
 *	lmsi_msense ( struct sb *sb )
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
lmsi_msense(struct sb * sb)
{
#ifdef SENSE_SUPPORTED
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	struct scs *cmd = (struct scs *)sb->SCB.sc_cmdpt;
	drv_info_t	*drv;

	c = lmsi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

#ifdef LMSI_DEBUG
	cmn_err(CE_CONT, "lmsi: Mode Sense C=%d T=%d L=%d P=0x%x\n", 
	    c, t, l, cmd->ss_addr );
#endif

	/*
	 * Fill in the mode sense
	 */

	if ((l != 0) || (t >= lmsi_ctrl[c].num_drives)) {
		cmn_err(CE_CONT, "lmsi: Unable to mode sense\n");
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}
	drv = lmsi_ctrl[c].drive[t];
#endif
	sb->SCB.sc_comp_code = SDI_ASW;
}

#ident	"@(#)kern-pdi:io/hba/sony/scsifake.c	1.2.1.1"
#ident	"$Header$"

/*******************************************************************************
 *******************************************************************************
 *
 *	SCSIFAKE.C
 *
 *	SDI to NON-SCSI Interface
 *
 *	Notes :
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
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#include <io/hba/sony/sony.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#include <io/hba/sony/sony.h>
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
#include <sys/sony.h>

#include <sys/moddefs.h>
#include <sys/dma.h>

#include <sys/hba.h>
#if PDI_VERSION >= PDI_SVR42MP
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif
#endif /* _KERNEL_HEADERS */


/*******************************************************************************
 *
 *	EXTERNALS
 *
 ******************************************************************************/

extern int   sony_gtol[MAX_HAS];	/* global to local */
extern int   sony_ltog[MAX_HAS];	/* local to global */
extern scsi_ha_t *sony_sc_ha;
extern int  sony_hiwat;
extern ctrl_info_t *sony_ctrl;
extern struct sonyvector sony31avector;


/******************************************************************************
 *
 *	PROTOTYPES
 *
 ******************************************************************************/

/*
 * Routines to handle translation
 */

void     sony_func(sblk_t * sp);
void     sony_cmd(sblk_t * sp);

int	 sony_test(sblk_t * sp);
void	 sony_inquir(struct sb * sb);
void     sony_rw(sblk_t * sp);
void     sony_sense(struct sb * sb);
void     sony_format(struct sb * sb);
void     sony_mselect(struct sb * sb);
void     sony_msense(struct sb * sb);

/******************************************************************************
 ******************************************************************************
 *
 *	SCSI TRANSLATION UTILITIES
 *
 ******************************************************************************
 *****************************************************************************/

/*******************************************************************************
 *
 *	sony_func ( sblk_t *sp )
 *
 *	Process a FUNCTION request
 *
 *	Entry :
 *		*sp		ptr to scsi job
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 ******************************************************************************/

void
sony_func(sblk_t * sp)
{
	struct scsi_ad *sa;
	int             c;
	int             t;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = sony_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	/*
	 * Build a SCSI_TRESET job and send it on its way
	 */

	cmn_err(CE_WARN, "sony: (_func) HA %d TC %d wants to be reset\n", c, t);
	sdi_callback((struct sb *)sp->sbp);

}

/*******************************************************************************
 *
 *	sony_cmd( sblk_t *sp )
 *
 *	Build and send an SCB associated command to the hardware
 *
 *	Entry :
 *		*sp		ptr to scsi job ptr
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *		- this routine ASSUMES that *sp* is SCB or ISCB -- no SFB
 *
 ******************************************************************************/

void 
sony_cmd(sblk_t * sp)
{
	struct scsi_ad *sa;
	int             c;
	int             t;
	int		l;
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	scsi_ha_t      *ha;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = sony_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &sony_sc_ha[c];

	/*
	 * get and fillout a command block
	 */

	if (sb->SCB.sc_cmdsz == SCS_SZ) {
		/*
		 * 6 byte SCSI command
		 */
		struct scs     *scsp;	/* gross but true, folks */

		scsp = (struct scs *)(void *) sb->SCB.sc_cmdpt;

		switch (scsp->ss_op) {

		case SS_REWIND:
		case SS_ERASE:
		case SS_FLMRK:
		case SS_SPACE:
		case SS_LOAD:
		case SS_LOCK:
			cmn_err(CE_NOTE, "cmd:%d ctl:%d,%d,%d.", scsp->ss_op, 
			    c, t, l);
			sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
			sdi_callback(sb);
			break;

		case SS_TEST:
			sony_test(sp);
			break;

		case SS_REQSEN:
			/*
			 * request sense
			 */
			sony_sense(sb);
			sdi_callback(sb);
			break;

		case SS_READ:
		case SS_WRITE:
			/*
			 * issue read/write
			 */
			sony_rw(sp);
			break;

		case SS_INQUIR:
			/*
			 * issue a device inquiry
			 */
			sony_inquir(sb);
			sdi_callback(sb);
			break;

		case SS_RESERV:
		case SS_RELES:
			/*
			 * reserve/release unit
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case SS_MSENSE:
			/*
			 * mode sense
			 */
			sony_msense(sb);
			sdi_callback(sb);
			break;

		case SS_SDDGN:
			/*
			 * send diagnostic
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case SS_REASGN:
			/*
			 * reassign blocks
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case 0x04:
			/*
			 * format unit
			 */
			sony_format(sb);
			sdi_callback(sb);
			break;

		default:
			cmn_err(CE_WARN, "sony: (_cmd) Unknown SCSI 6 BYTE command : %x\n", scsp->ss_op);
			sb->SCB.sc_comp_code = (unsigned long)SDI_ERROR;
			sdi_callback(sb);
			break;
		}
	}
	/* 6 byte command */
	else if (sb->SCB.sc_cmdsz == SCM_SZ) {

		/*
		 * 10 byte SCSI command
		 */
		struct scm     *scm;
		scm = (struct scm *)(void *) (SCM_RAD(sb->SCB.sc_cmdpt));

		switch (scm->sm_op) {

		case SM_RDCAP:
			/*
			 * read drive capacity
			 */
			ha->ha_devicevect->sony_rdcap(sp);
			break;

		case SM_READ:
		case SM_WRITE:
			/*
			 * read/write extended
			 */
			sony_rw(sp);
			break;

		case SM_SEEK:
			/*
			 * seek extended
			 */
			sb->SCB.sc_comp_code = (unsigned long) SDI_ASW;
			sdi_callback(sb);
			break;

		case 0x2fL:
			/*
			 * verify command
			 */
			sb->SCB.sc_comp_code = SDI_ASW;
			sdi_callback(sb);
			break;

		case SM_RDDL:
		case SM_RDDB:
		case SM_WRDB:
		case SD_ERRHEAD:
		case SD_ERRHEAD1:
			/*
			 * defect list, data buffer, error code junk
			 */
			sb->SCB.sc_comp_code = SDI_ASW;
			sdi_callback(sb);
			break;

		default:
			/*
			 * yet another unknown scsi command
			 */
			cmn_err(CE_WARN, "sony: (_cmd) Unknown 10 Byte Command - %x\n", scm->sm_op);
			sb->SCB.sc_comp_code = (unsigned long)SDI_TCERR;
			sdi_callback(sb);
			break;
		}		/* 10 byte command */
	} else {
		/*
		 * some really bogus thing
		 */
		sb->SCB.sc_comp_code = (unsigned long) SDI_TCERR;
		sdi_callback(sb);
	}
}


/*******************************************************************************
 *
 *	sony_inquir ( sb )
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
sony_inquir(struct sb * sb)
{
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	struct ident   *inq_data;
	int             i;
	int             c, t, l;
	scsi_ha_t *ha;

	c = sony_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &sony_sc_ha[c];

	if (t == ha->ha_id) {
		/*
		 * they are checking out the processor card
		 */
		if (l == 0) {
			/*
			 * lun 0 is the only valid lun on the processor card
			 * Note: we should not pass back more than asked for
			 * by sc_size field.
			 */
			inq_data = (struct ident *)(void *) sb->SCB.sc_datapt;
			inq_data->id_type = ID_PROCESOR;
			for (i = 0; i < VID_LEN; i++)
				inq_data->id_vendor[i] = ha->ha_sony_vendor[i];
			for (i = 0; i < PID_LEN; i++)
				inq_data->id_prod[i] = ha->ha_sony_prod[i];
			inq_data->id_prod[i - 1] = 0x00;
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

	if ((l != 0) || (t >= sony_ctrl[c].num_drives)) {
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
#ifdef SONY31A_DEBUG
		cmn_err(CE_NOTE, "INQUIRY, no such target");
#endif
		return;
	}
	inq_data = (struct ident *)(void *) sb->SCB.sc_datapt;
	inq_data->id_type = ID_ROM;
	inq_data->id_rmb = 1;
	inq_data->id_pqual = 0;
	for (i = 0; i < VID_LEN; i++)
		inq_data->id_vendor[i] = ha->ha_sony_vendor[i];
	for (i = 0; i < PID_LEN; i++)
		inq_data->id_prod[i] = ha->ha_sony_diskprod[i];
	inq_data->id_prod[i - 1] = 0x00;	/* zero terminate for grins */
	inq_data->id_len = IDENT_SZ - 5;
	sb->SCB.sc_comp_code = SDI_ASW;
#ifdef SONY31A_DEBUG
		cmn_err(CE_NOTE, "INQUIRY, success with c=%d t=%d l=%d",c,t,l);
#endif
}

/*******************************************************************************
 *
 *	sony_test ( sblk_t *sp )
 *
 *	Handle SS_TEST scsi request
 *
 *	Entry :
 *	*sp             ptr to scsi job ptr
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

int
sony_test(sblk_t * sp)
{
        struct sb               *sb = (struct sb *)&sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	scsi_ha_t *ha;

	c = sony_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	ha = &sony_sc_ha[c];

	if ((l != 0) || ((t != ha->ha_id) && (t >= sony_ctrl[c].num_drives))) {
		/*
				 * Not a valid lun or not a valid target
				 */
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return(-1);
	}
	ha->ha_cmd_sp = sp;
	return (ha->ha_devicevect->sony_chkmedia(ha));
}

/*******************************************************************************
 *
 *	sony_sense ( struct sb *sb )
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
sony_sense(struct sb * sb)
{
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t;
	drv_info_t     *drv;
	struct scsi_ha *ha;

	c = sony_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	drv = sony_ctrl[c].drive[t];
	ha = &sony_sc_ha[ c ];

	
	drv->sony_sense.sd_len = 6;
	drv->sony_sense.sd_errc = 0x70;
	drv->sony_sense.sd_key = 0;
	drv->sony_sense.sd_sencode = 0;

	/* what other drv status should we set/check ?? */

	/*fix up when you expect to have more than one drive per adapter ??*/
	if (ha->ha_driveStatus & (SONY31A_CHANGE|SONY31A_NODISK)) {
		/*
		 *	somebody changed the disk
		 */
		drv->sony_sense.sd_key = SD_UNATTEN;
		drv->sony_sense.sd_sencode = SC_MEDCHNG;
		drv->sony_sense.sd_valid = 1;
		ha->ha_driveStatus &= ~(SONY31A_CHANGE|SONY31A_NODISK);
#ifdef SONY31A_ERRS
		cmn_err(CE_WARN, "SONY31A: sense CHANGE");
#endif
	}
	else {
		/*
	 	 *	normal drive happenings
                 *      note you are not copying this out of the ha yet ?
		 */
		/* for now ?? into drv struct later */
		switch ( ha->ha_lasterr) {

                        case    ERR_CMD :
                        case    ERR_PARAM :
                                /*
                                 *      invalid command to drive
                                 */
				drv->sony_sense.sd_key = SD_ILLREQ;
                        	drv->sony_sense.sd_valid = 1;
#ifdef SONY31A_ERRS
		cmn_err(CE_WARN, "SONY31A: sense ERRS");
#endif
                                break;

                        case    ERR_NO_ERROR:
                                /*
                                 *      everything is cool
                                 */
                                break;

			default:
				/*
			 	 *	something really went wrong
			 	*/
	 			drv->sony_sense.sd_key = SD_HARDERR;
	 			drv->sony_sense.sd_valid = 1;
#ifdef SONY31A_ERRS
		cmn_err(CE_WARN, "SONY31A: sense Bad default");
#endif
		}
	}

	bcopy (SENSE_AD(&drv->sony_sense),sb->SCB.sc_datapt,SENSE_SZ);
	drv->sony_sense.sd_valid = 0;
}

/*******************************************************************************
 *
 *	sony_format ( struct sb *sb )
 *
 *	Format the sony stuff -- pretty easy since you can't format
 *	an SONY31A drive
 *
 *	Entry :
 *		*sb		ptr to scsi block
 *
 *	Exit :
 *		Nothing
 *
 ******************************************************************************/

void
sony_format(struct sb * sb)
{
	sb->SCB.sc_comp_code = (ulong_t) SDI_HAERR;
	return;
}


/*******************************************************************************
 *
 *	sony_msense ( struct sb *sb )
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
sony_msense(struct sb * sb)
{
#ifdef SENSE_SUPPORTED
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	int             c, t, l;
	struct scs *cmd = (struct scs *)sb->SCB.sc_cmdpt;
	drv_info_t	*drv;

	c = sony_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

#ifdef SONY31A_DEBUG
	cmn_err(CE_CONT, "sony: Mode Sense C=%d T=%d L=%d P=0x%x\n", 
	    c, t, l, cmd->ss_addr );
#endif

	/*
	 * Fill in the mode sense
	 */

	if ((l != 0) || (t >= sony_ctrl[c].num_drives)) {
		cmn_err(CE_CONT, "sony: Unable to mode sense\n");
		sb->SCB.sc_comp_code = (unsigned long)SDI_SCBERR;
		return;
	}
	drv = sony_ctrl[c].drive[t];
#endif
	sb->SCB.sc_comp_code = SDI_ASW;
}


/******************************************************************************
 *
 *	sony_rw (sblk_t *sp )
 *
 *	Convert a SCSI r/w request into an SONY31A request
 *
 *	Entry :
 *		*sp		ptr to scsi job
 *
 *	Exit :
 *		Nothing
 *
 *	Notes :
 *
 *****************************************************************************/

void
sony_rw(sblk_t * sp)
{
	struct sb      *sb = (struct sb *) & sp->sbp->sb;
	struct scsi_ad *sa = (struct scsi_ad *) & sb->SCB.sc_dev;
	uint_t          sector;
	ushort_t        count;
	int             c, t, l;
	scsi_lu_t	*q;		/* queue ptr */
	int		blk_num;
	int		direction;
	scsi_ha_t      *ha;
	drv_info_t     *drv;
	pl_t		ospl;

	c = sony_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	ha = &sony_sc_ha[c];

	drv = sony_ctrl[c].drive[t];

	/*
	 * check for various command things
	 */

	if (sb->SCB.sc_cmdsz == SCS_SZ) {
		/*
		 * 6 byte command
		 */
		struct scs     *scs;

		scs = (struct scs *)(void *) sb->SCB.sc_cmdpt;
		direction = scs->ss_op == SS_READ ? B_READ : B_WRITE;
		sector = sdi_swap16(scs->ss_addr) + ((scs->ss_addr1 << 16) & 0x1f0000);
		count = scs->ss_len;
	} else if (sb->SCB.sc_cmdsz == SCM_SZ) {
		/*
		 * 10 byte extended command
		 */
		struct scm     *scm;

		scm = (struct scm *)(void *) (SCM_RAD(sb->SCB.sc_cmdpt));
		direction = scm->sm_op == SM_READ ? B_READ : B_WRITE;
		sector = sdi_swap32(scm->sm_addr);
		count = sdi_swap16(scm->sm_len);
	} else {
		/*
		 * random command here
		 */
#ifdef SONY31A_DEBUG
		cmn_err(CE_WARN, "SONY31A: (sony_rw) Unknown SC_CMDSZ -- %x\n",
			sb->SCB.sc_cmdsz);
#endif
		ha->ha_devicevect->sony_error(ha, sp);
		return;
	}
	if (direction == B_WRITE) {
#ifdef SONY31A_DEBUG
		cmn_err(CE_WARN, "SONY31A: (sony_rw) write cmd received");
#endif
		ha->ha_devicevect->sony_error(ha, sp);
		return;
	}

	blk_num = sector + drv->startsect;
	sp->s_tries = 0;
	sp->s_size  = count;
	sp->s_blkno = blk_num;
#ifdef SONY31A_DEBUG
	cmn_err(CE_NOTE, "sony_rw: sector %d, offset %d, cnt %d, addr %x",
				sector, drv->startsect, count, 
				(caddr_t) (void *)  sp->sbp->sb.SCB.sc_datapt);
#endif
	if((ha->ha_status&SONY31A_ERROR) ||
		ha->ha_devicevect->sony_issue_rdcmd(ha, sp) < 0) {
#ifdef SONY31A_DEBUG
		cmn_err(CE_NOTE, "sony_rw: ha_status is %x",
				ha->ha_status);
#endif
		ha->ha_devicevect->sony_error(ha, sp);
		return;
	}
	ha->ha_cmd_sp = sp;
	ospl = spldisk();
	/*
	 *	job is submitted -- update queues
	 */

	t = SDI_TCN(sa);
	l = SDI_LUN(sa);
	q = &SONY_LU_Q( c, t, l );
	q->q_count++;
	if ( q->q_count >= sony_hiwat )
		q->q_flag |= QBUSY;
	splx( ospl );
}

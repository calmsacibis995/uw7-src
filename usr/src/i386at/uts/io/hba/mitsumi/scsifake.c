#ident	"@(#)kern-pdi:io/hba/mitsumi/scsifake.c	1.6"
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

extern int   mitsumi_gtol[MAX_HAS];	/* global to local */
extern int   mitsumi_ltog[MAX_HAS];	/* local to global */
extern ctrl_info_t *ctrl;	/* local controller structs */

/*******************************************************************************
 *
 *	PROTOTYPES
 *
 ******************************************************************************/

/*
 * Routines to handle translation
 */

void            mitsumi_func(sblk_t * sp);
void            mitsumi_cmd(sblk_t * sp);

extern int      mitsumi_test(struct sb * sb);
extern void     mitsumi_rw(sblk_t * sp);
extern void     mitsumi_inquir(struct sb * sb);
extern void     mitsumi_sense(struct sb * sb);
extern void     mitsumi_format(struct sb * sb);
extern void     mitsumi_rdcap(struct sb * sb, int flag);
extern void     mitsumi_mselect(struct sb * sb);
extern void     mitsumi_msense(struct sb * sb);

/******************************************************************************
 ******************************************************************************
 *
 *	SCSI TRANSLATION UTILITIES
 *
 ******************************************************************************
 *****************************************************************************/

/*******************************************************************************
 *
 *	mitsumi_func ( sblk_t *sp )
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
mitsumi_func(sblk_t * sp)
{
	struct scsi_ad *sa;
	int             c;
	int             t;

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	/*
	 * Build a SCSI_TRESET job and send it on its way
	 */

	cmn_err(CE_WARN, "mitsumi: (_func) HA %d TC %d wants to be reset\n", c, t);
	sdi_callback((struct sb *)sp->sbp);

}

/*******************************************************************************
 *
 *	mitsumi_cmd( sblk_t *sp )
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
mitsumi_cmd(sblk_t * sp)
{
	struct scsi_ad *sa;
	int             c;
	int             t;
	int		l;
	struct sb      *sb = (struct sb *) & sp->sbp->sb;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = mitsumi_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	l = SDI_LUN(sa);

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
			if(mitsumi_test(sb) > 0)
				sb->SCB.sc_comp_code = SDI_ASW;
			else sb->SCB.sc_comp_code = (unsigned long) SDI_ERROR;
			sdi_callback(sb);
			break;
		case SS_REQSEN:
			/*
			 * request sense
			 */
			mitsumi_sense(sb);
			sdi_callback(sb);
			break;

		case SS_READ:
		case SS_WRITE:
			/*
			 * issue read/write
			 */
			mitsumi_rw(sp);
			break;

		case SS_INQUIR:
			/*
			 * issue a device inquiry
			 */
			mitsumi_inquir(sb);
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
			mitsumi_msense(sb);
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
			mitsumi_format(sb);
			sdi_callback(sb);
			break;

		default:
			cmn_err(CE_WARN, "mitsumi: (_cmd) Unknown SCSI 6 BYTE command : %x\n", scsp->ss_op);
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
			mitsumi_rdcap(sb, 1);
			sdi_callback(sb);
			break;

		case SM_READ:
		case SM_WRITE:
			/*
			 * read/write extended
			 */
			mitsumi_rw(sp);
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
			cmn_err(CE_WARN, "mitsumi: (_cmd) Unknown 10 Byte Command - %x\n", scm->sm_op);
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

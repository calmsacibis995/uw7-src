#ident	"@(#)kern-pdi:io/hba/ide/ata/atapi.c	1.5.5.3"

#include <util/cmn_err.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi.h>
#include <io/hba/ide/ide.h>
#include <io/hba/ide/ide_ha.h>
#include <io/hba/ide/ata/ata.h>
#include <io/hba/ide/ata/atapi.h>
#include <io/hba/ide/ata/atapi_ha.h>
#include <io/hba/ide/ata/ata_ha.h>

/* These must come last: */
#include <sys/debug.h>
#include <io/ddi.h>

#define ATAPI_DTRACE
#ifdef ATAPI_DEBUG_TRACE
#define ATAPI_DTRACE cmn_err(CE_NOTE,"%d-%s",__LINE__,__FILE__);
#else
#define ATAPI_DTRACE
#endif

extern struct ata_ha *ide_sc_ha[]; /* SCSI bus structs */

/*
 * Forward declarations.
 */
int atp_busywait(struct ide_cfg_entry *, int, ushort_t *, ushort_t, ushort_t *);
STATIC void atp_seterr(struct ata_parm_entry *);

int
atp_softrst(int drv, struct ide_cfg_entry *cfgp)
{
#ifdef ATAPI_DEBUG
	cmn_err(CE_NOTE,"atapi_softrst %d", drv);
#endif

	/* select the proper drive */
	outb(cfgp->cfg_ioaddr1+ATP_DRVSEL,ata_intToAtasel(drv));
	cfgp->cfg_lastdriv = drv;

	outb(cfgp->cfg_ioaddr1+AT_CMD,ATP_SOFTRST);
	if (ata_wait(cfgp->cfg_ioaddr1, ATS_BUSY, 0, ATS_BUSY)) {
		return 1;
	}
	return 0;
}

struct ata_parm_entry *
atapi_protocol (struct ide_cfg_entry *cfgp)
{
	uchar_t laststat;
	int drv = cfgp->cfg_curdriv;
	struct ata_parm_entry *parms = &cfgp->cfg_drive[drv];
	int error = 0;
	int toofast = 1;

restart:
#ifdef ATAPI_DEBUG
	cmn_err(CE_NOTE, "atapi_protocol %d (%d)", drv, parms->dpb_state);
	cmn_err(CE_NOTE, "newvaddr=0x%x newcount=0x%x",parms->dpb_newvaddr, parms->dpb_newcount);
#endif

	switch (parms->dpb_state) {
	case ATP_INITIAL: {
		ushort_t wantTransfer;

		if (ata_wait(cfgp->cfg_ioaddr1,
			(ATS_BUSY|ATS_DATARQ), 0, (ATS_DATARQ|ATS_BUSY))) {
			cmn_err(CE_WARN, "atapi_protocol: ATS_BUSY does not go off");
			error = 1;
			break;
		}

		wantTransfer = parms->dpb_newcount > ATP_MAXPIO ?
				ATP_MAXPIO : parms->dpb_newcount;

		outb(cfgp->cfg_ioaddr1+ATP_LSB, wantTransfer & 0xFF);
		outb(cfgp->cfg_ioaddr1+ATP_MSB, wantTransfer >> 8);
		outb(cfgp->cfg_ioaddr1+ATP_DRVSEL,(drv==0 ? ATDH_DRIVE0 : ATDH_DRIVE1));
		cfgp->cfg_lastdriv = drv;
		outb(cfgp->cfg_ioaddr1+ATP_CMD, ATP_PKTCMD);

		parms->dpb_state = ATP_SNDPKT;

		if (parms->dpb_atpdrq != ATP_INTDRQ) {
			/*
			 * This device does not generate an interrupt to indicate that
			 * it is ready for the Packet Command.
			 */
			if (ata_wait(cfgp->cfg_ioaddr1, ATS_BUSY|ATS_DATARQ,
						ATS_DATARQ,ATS_BUSY)){
				cmn_err(CE_WARN,"!atapi_protocol: Sent Packet Command, ATS_BUSY doesn't go off");
				error = 1;
				break;
			}
			goto restart;
		}
	} break;
	case ATP_SNDPKT:
		if ((parms->dpb_atpdrq==ATP_INTDRQ) || parms->dpb_atpbusywait){
			if (ata_wait(cfgp->cfg_ioaddr1, ATS_BUSY|ATS_DATARQ,
						ATS_DATARQ,ATS_BUSY)){
				cmn_err(CE_WARN,"atapi_protocol: Sent Packet Command, ATS_BUSY doesn't go off");
				error = 1;
				break;
			}
		}

		laststat = inb(cfgp->cfg_ioaddr1+ATP_INTR);
		if (!(laststat & ATP_COD)) {
			cmn_err(CE_WARN, " atapi_protocol: CoD is not set alone");
			error = 1;
			break;
		}
		/* Write Atapi CDB  (6 words) */
		repoutsw(cfgp->cfg_ioaddr1, (ushort_t *) parms->dpb_atppkt, 6);
		parms->dpb_state = ATP_GETDATA;
	break;
	case ATP_GETDATA: {
		uchar_t atpState;
		ushort_t didTransfer;
		ushort_t wantTransfer;
		register ushort_t extra;

		if (parms->dpb_atpbusywait) {
			if (ata_wait(cfgp->cfg_ioaddr1, ATS_BUSY, 0,ATS_BUSY)){
				cmn_err(CE_WARN,"atapi_protocol: Sent Packet Command, ATS_BUSY doesn't go off");
				error = 1;
				break;
			}
			drv_usecwait(atapi_delay);
		}

		atpState = inb(cfgp->cfg_ioaddr1 + ATP_INTR);
		laststat = inb(cfgp->cfg_ioaddr1 + ATP_STATUS);

		if ((atpState & (ATP_IO|ATP_COD)) == (ATP_IO|ATP_COD)) {
			parms->dpb_state = ATP_GETSTATUS;
			goto restart;
		}

		if (!(laststat & ATS_DATARQ) || (atpState & ATP_COD)) {
			/*
			 * Sometimes we hit invalid states because to host
			 * is too fast.
			 */
			if (toofast) {
				toofast--;
				/*cmn_err(CE_NOTE, "TOOFAST: laststat=0x%x atpState=0x%x", laststat, atpState);*/
				drv_usecwait( 1500000 );
				goto restart;
			}
			cmn_err(CE_WARN, "atapi_protocol: Invalid state in ATP_STATE laststat=0x%x atpState=0x%x", laststat, atpState);
			error = 1;
			break;
		}

		didTransfer = inb(cfgp->cfg_ioaddr1+ATP_LSB) |
					  inb(cfgp->cfg_ioaddr1+ATP_MSB) << 8;

		wantTransfer = parms->dpb_newcount > ATP_MAXPIO ?
						ATP_MAXPIO : parms->dpb_newcount;

		if (didTransfer <= wantTransfer) {
			parms->dpb_newcount -= didTransfer;
			extra = 0;
		} else {
			/*
			 * It could happen that the device wants to transfer more data
			 * than what we asked for.
			 */
			extra = (didTransfer - wantTransfer) / 2;
			didTransfer = wantTransfer;
			if (atpState & ATP_IO) {
				cmn_err(CE_WARN, "!atapi_protocol: extra bytes in %d\n", extra);
			} else {
				cmn_err(CE_WARN, "!atapi_protocol: extra bytes out %d\n", extra);
			}
		}

		ASSERT(parms->dpb_newvaddr);
		if (atpState & ATP_IO) {
			repinsw(cfgp->cfg_ioaddr1, (ushort *) parms->dpb_newvaddr,
					didTransfer/2);
		} else {
			repoutsw(cfgp->cfg_ioaddr1, (ushort *) parms->dpb_newvaddr,
					didTransfer/2);
		}
		parms->dpb_newvaddr += didTransfer;

		while(extra--) {
			if (atpState & ATP_IO) {
				inw(cfgp->cfg_ioaddr1);
			} else {
				outw(cfgp->cfg_ioaddr1, 0);
			}
		}
		/* Stay in the same state */
	} break;
	case ATP_GETSTATUS: {
		struct control_area *control = parms->dpb_control;
		uchar_t error;
		
		laststat = inb(cfgp->cfg_ioaddr1 + ATP_STATUS);
		error = inb(cfgp->cfg_ioaddr1 + ATP_ERROR);
#ifdef ATAPI_DEBUG
		cmn_err(CE_NOTE,"Done stat=0x%x error=%x!", laststat, error);
#endif

		if (!(laststat & ATP_ERROR)) {
			if (parms->dpb_drverror == DERR_ATAPI) {
				/* This is a sense command that generated by this routine */
				parms->dpb_intret = DINT_ERRABORT;
			} else {
				/* Everything is fine, we're done! */
				parms->dpb_intret = DINT_COMPLETE;
			}

			/*
			 * PATCH:
			 * Mitsumi has a fw bug that freaks SC01. In the Read Capacity
			 * the block size returned is 0x930. It should be 0x800.
			 */
			if(*parms->dpb_atppkt == SM_RDCAP) {
				ulong_t *pt = ((ulong *) parms->dpb_newvaddr) - 1;

				if (*pt == 0x30090000) {
					*pt = 0x80000;
				}
			}
			return parms;
		} else {
			/*
			 * There is an error so send a Request Sense
			 */
			uchar_t *atpcdb = parms->dpb_atppkt;

			if (parms->dpb_retrycnt++ > 1) {
				/* Too many errors */
				parms->dpb_intret = DINT_ERRABORT;
				break;
			}

			/* Build the packet */
			bzero(atpcdb, ATP_PKTSZ);
			atpcdb[0] = SS_REQSEN;
			atpcdb[4] = control->SCB_SenseLen;

			/* Set the data transfer */
			parms->dpb_newvaddr = (paddr_t) control->virt_SensePtr;
			parms->dpb_newcount = control->SCB_SenseLen;

			/* Flag IDE that there is no need for sense data faking */
			control->SCB_TargStat = parms->dpb_drverror = DERR_ATAPI;

			/* Set the next state */
			parms->dpb_state = ATP_INITIAL;
			goto restart;
		}
	} break;
	default:
		cmn_err(CE_WARN, "atapi_protocol: Invalid state");
		break;
	}

	/*
	 * Error condition in the protocol. Reset and bailout.
	 */
	if (error) {
		atp_softrst(drv, cfgp);
		atp_seterr(parms);
		return parms;
	}

	/*
	 * We are in busywait mode.
	 */
	if (parms->dpb_atpbusywait) {
		goto restart;
	}

	/*
	 * We are interrupt driven, but let's take a timer incase interrupts
	 * are lost.
	 */
	if (drv_getparm(LBOLT, &cfgp->cfg_laststart) != -1) {
		parms->dpb_atpbusywait = 0;
		return parms;
	} else {
		/* No timer, hence busywait */
		parms->dpb_atpbusywait = 1;
		goto restart;
	}
}

int
atapi_drvinit (int drv, struct ide_cfg_entry *cfgp)
{
	uchar_t laststat;
	struct ident inqdata;
	struct atp_ident ident;
	struct ata_parm_entry *parms = &cfgp->cfg_drive[drv];
	uchar_t *packet = parms->dpb_atppkt;

	/* set up some defaults in the dpb... */
	parms->dpb_flags = (DFLG_RETRY | DFLG_ERRCORR | DFLG_VTOCSUP | DFLG_FIXREC);
	parms->dpb_drvflags = (ATF_FMTRST | ATF_RESTORE);
	parms->dpb_secsiz = cfgp->cfg_defsecsiz;

	/*
	 * In an ATAPI CD-ROM these fields do not directly apply. They'll be
	 * filled with information that will allow for 'discovery'.
	 */
	parms->dpb_pcyls = parms->dpb_cyls = 1024;
	parms->dpb_pheads = parms->dpb_heads = 16;
	parms->dpb_psectors = parms->dpb_sectors = 255;

	ata_drvident(drv, cfgp, &ident);
	parms->dpb_atpdrq = ident.atp_drq;

	/* Debug stuff */
	bcopy("CDROM:", parms->dpb_tag,6);
	parms->dpb_tag[6] = drv | 0x30;


	/* Setup inquiry packet */
	bzero((char *) packet, ATP_PKTSZ);
	packet[0] = SS_INQUIR;
	packet[4] = sizeof(struct ident);
	parms->dpb_newvaddr = (paddr_t) &parms->dpb_inqdata;
	parms->dpb_newcount = packet[4];

	cfgp->cfg_curdriv = drv;
	cfgp->cfg_flags |= CFLG_BUSY;
	parms->dpb_state = ATP_INITIAL;
	parms->dpb_retrycnt = 0;
	parms->dpb_intret = DINT_CONTINUE;
	parms->dpb_drverror = DERR_NOERR;
	parms->dpb_atptimecnt = 0;
	parms->dpb_atpbusywait = 1;
	parms->dpb_control = NULL;

	atapi_protocol(cfgp);

	return 0;
}

int
ata_atapi_cmd(struct control_area *control, int sleepflag)
{
	struct  sb *sb = control->c_bind;
	struct ata_ha *ha;
	struct ide_cfg_entry *cfgp;
	struct ata_parm_entry *parms;

	ha = ide_sc_ha[control->adapter];
	cfgp = ha->ha_config;
	parms = &cfgp->cfg_drive[control->target];

	ATAPI_DTRACE;

	control->SCB_Stat = DINT_COMPLETE;

	switch (sb->sb_type) {
	case SFB_TYPE:
		switch (sb->SFB.sf_func) {
		case SFB_ABORTM:
		case SFB_RESETM:
			atp_softrst(control->target, cfgp);
			break;
		default:
			control->SCB_Stat = DINT_ERRABORT;
			control->SCB_TargStat = DERR_BADCMD;
			break;
		}
		ide_job_done(control);
		break;
	case ISCB_TYPE:
	case SCB_TYPE: {
		uchar_t *scsicdb = (uchar_t *) sb->SCB.sc_cmdpt;
		register uchar_t *atapicdb = parms->dpb_atppkt;

		if (!gdev_valid_drive(control)) { 
			control->SCB_Stat = DINT_ERRABORT;
			control->SCB_TargStat = DERR_BADCMD;
			ide_job_done(control);
			return;
		}


#ifdef ATAPI_DEBUG
		cmn_err(CE_NOTE,"cmd=0x%x buf=0x%x sz=0x%x drv=%d ctrl=0x%x parms=0x%x", scsicdb[0], sb->SCB.sc_datapt, sb->SCB.sc_datasz, control->target, parms->dpb_control,parms);
#endif
		switch (scsicdb[0]) {
			case SS_INQUIR:
			case SS_REQSEN:
			case SM_RDCAP:
				bcopy(scsicdb, atapicdb, SCS_SZ);
				bzero(&atapicdb[SCS_SZ], ATP_PKTSZ - SCS_SZ);

				parms->dpb_newvaddr = (paddr_t)sb->SCB.sc_datapt;
				parms->dpb_newcount = sb->SCB.sc_datasz;
				break;
			case SS_LOCK:
			case SS_ST_SP:
			case SM_SEEK:
			case SS_TEST:
				bcopy(scsicdb, atapicdb, SCS_SZ);
				bzero(&atapicdb[SCS_SZ], ATP_PKTSZ - SCS_SZ);

				parms->dpb_newvaddr = NULL;
				parms->dpb_newcount = 0;
				break;
			case SM_READ:
				atapicdb[0] = scsicdb[0];
				atapicdb[1] = 0;
				bcopy(&scsicdb[2], &atapicdb[2], SCM_SZ);
				bzero(&atapicdb[SCM_SZ], ATP_PKTSZ - SCM_SZ);

				parms->dpb_newvaddr = (paddr_t)sb->SCB.sc_datapt;
				parms->dpb_newcount = sb->SCB.sc_datasz;
				break;
			default:
				control->SCB_Stat = DINT_ERRABORT;
				control->SCB_TargStat = DERR_BADCMD;
				ide_job_done(control);
				return;
		}

		/*
		 * If the timeout code was executed we know the device looses
		 * interrupts. Hence busywaiting is required during operation.
		 * This will only happen after init time.
		 */
		if (!ideinit_time && parms->dpb_atptimecnt == 0)
			parms->dpb_atpbusywait = 0;

		cfgp->cfg_curdriv = control->target;
		cfgp->cfg_flags |= CFLG_BUSY;
		parms->dpb_state = ATP_INITIAL;
		parms->dpb_retrycnt = 0;
		parms->dpb_intret = DINT_CONTINUE;
		parms->dpb_drverror = DERR_NOERR;

		/* Go in peace and do ATAPI */
		atapi_protocol(cfgp);

		if (parms->dpb_atpbusywait) {
			parms->dpb_control->SCB_Stat = parms->dpb_intret;
			ata_cplerr(cfgp, control);
		}
		} break;
	default:
		break;
	}
	return (0);
}

/*
 * atp_timeout - Recovery routine for a stuck atapi command.
 */
void
atp_timeout(struct ide_cfg_entry *cfgp)
{
	int drv = cfgp->cfg_curdriv;
	struct ata_parm_entry *parms = &cfgp->cfg_drive[drv];
	struct ata_ha *ha= ide_sc_ha[parms->dpb_control->adapter];

	/* Needed for IDE_HIM_LOCK */
	ha = ide_sc_ha[parms->dpb_control->adapter];

	IDE_HIM_LOCK;
	if (!parms->dpb_atpbusywait) {
		if (parms->dpb_atptimecnt++ > ATP_MAXTIMEOUT) {
			atp_softrst(drv, cfgp);
			atp_seterr(parms);
		} else {
			cmn_err(CE_WARN,"Atapi device going into polling mode");
			parms->dpb_atpbusywait = 1;
			parms->dpb_control->SCB_Stat = DINT_ERRABORT;
			atapi_protocol(cfgp);
		}
		ata_cplerr(cfgp, parms->dpb_control);
	}
	IDE_HIM_UNLOCK;
}

/*
 * atp_seterr
 * Set the appropiate fields in the structures that ide (the upper layer)
 * uses to abort a command.
 */
STATIC void
atp_seterr(struct ata_parm_entry *parms)
{
	parms->dpb_intret = DINT_ERRABORT;
	if (parms->dpb_control)
		parms->dpb_control->SCB_TargStat = DERR_ATAPI;
	parms->dpb_drverror = DERR_ATAPI;
	parms->dpb_state = ATP_INVALID;
#ifdef FIXIT
	untimeout(parms->dpb_atptimeout);
#endif
}


/*
 * Some devices on reboot do not get reset, this may cause a problem
 * when looking for ATAPI signatures.
 */
void
ata_halt(struct ide_cfg_entry *cfgp)
{
	if (cfgp->cfg_atapi_drives == 2) {
		atp_softrst(0, cfgp);
		atp_softrst(1, cfgp);
	} else if (cfgp->cfg_atapi_drives == 1) {
		if (cfgp->cfg_ata_drives == 0) {
			atp_softrst(0, cfgp);
		} else {
			atp_softrst(1, cfgp);
		}
	}
}

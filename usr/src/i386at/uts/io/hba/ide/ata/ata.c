#ident	"@(#)kern-pdi:io/hba/ide/ata/ata.c	1.7.3.1"

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
#include <io/hba/ide/ata/ata.h>
#include <io/hba/ide/ata/ata_ha.h>
#include <io/hba/ide/ata/atapi.h>
#include <io/hba/ide/ata/atapi_ha.h>
#include <util/mod/moddefs.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#ifdef ATA_DEBUG_TRACE
#define ATA_DTRACE cmn_err(CE_NOTE,"%d-%s",__LINE__,__FILE__);
#else
#define ATA_DTRACE
#endif

#define ATA_MAXXFER	64*1024
#define ATA_MEMALIGN	4
#define ATA_BOUNDARY	0
#define ATA_DMASIZE	0

#ifdef TIMEOUT_DEBUG
/* These two variables are used for creating timeouts for testing purposes. */
/* Look in ata_cmd for futher details on their use.			    */
int ata_readcnt = 0;
int ata_writecnt = 0;
#endif

extern struct ata_ha *ide_sc_ha[];
extern int ide_waitsecs;

int ata_wait(ushort, uint, uint, uint);
void ata_xferok(struct ata_parm_entry *, int);
ushort ata_err(unchar);
void atatimeout(struct ide_cfg_entry *);
void ataintdisable(struct ide_cfg_entry *);

void ata_detect (int, struct ide_cfg_entry *, struct atp_ident *, int *, int *);

int
ata_find_ha(gdev_cfgp config, HBA_IDATA_STRUCT *idata)
{
	int ata_drives = 0 , atapi_drives = 0;
	struct atp_ident ident;

	/* Reset */
	outb(config->cfg_ioaddr2+AT_DEV_CNTL,AT_CTLRESET);

	/* Disable interrupts */
	ataintdisable(config);

	/*
	 * The outb to the address "or"ed with 0x80 below is a HACK!!!
	 * On Adaptec controllers set up for the secondary position,
	 * once you reset them, they forget that they're supposed
	 * to respond to secondary addresses until you un-do the
	 * reset!!! Thus, you have to set the reset bit in port
	 * 0x376 and clear it in port 0x3F6!!! Gross, but this
	 * shouldn't hurt, as long as we're still in initialization.
	 */
	outb(((config->cfg_ioaddr2+AT_DEV_CNTL) | 0x80),AT_INTRDISAB);


	/* Conservative 25 us + 400 ns before BUSY is asserted */
	drv_usecwait( 30 );

	/* Wait for busy to clear */
	if (ata_wait(config->cfg_ioaddr1, ATS_BUSY, 0, ATS_BUSY)) {
		return (0);
	}

	/* Detect the Master device. */
	ata_detect(0, config, &ident, &ata_drives, &atapi_drives);

	/* Detect the Slave device. */
	ata_detect(1, config, &ident, &ata_drives, &atapi_drives);

	/*
 	 * Set global structures.
	 */
	config->cfg_ata_drives = ata_drives;
	config->cfg_atapi_drives = atapi_drives;
	if (ata_drives == 1) {
		/***
		** According to the configuration information
		** we are only using 1 drive on this controller.
		** Mark CCAP_NOSEEK since we do not perform
		** explicite seeks on single drive configuration.
		**/
		config->cfg_capab |= CCAP_NOSEEK;
	}
	if( config->cfg_ioaddr1 == ATA_PRIMEADDR  && ata_drives) {
		config->cfg_capab |= CCAP_BOOTABLE;
	}

	/* Start timer */
	if (ata_drives + atapi_drives) {
		ITIMEOUT(atatimeout, (caddr_t)config, ((30*HZ) | TO_PERIODIC), pldisk);
	}

	cmn_err(CE_NOTE,"!ata_find_ha: found %d ata drives and %d atapi drives at 0x%x",ata_drives, atapi_drives, config->cfg_ioaddr1);

	return ata_drives + atapi_drives;
}

/*
 * void ata_detect (int,struct ide_cfg_entry *,struct atp_ident *, int *, int *)
 *
 * ata_detect
 * Detects devices configured off and IDE interface. This is VERY TRICKY.
 */
void
ata_detect (int drv, struct ide_cfg_entry *cfgp, struct atp_ident *ident, int *ata_drives, int *atapi_drives)
{
	uchar_t laststat;

	outb(cfgp->cfg_ioaddr1+ATP_DRVSEL,ata_intToAtasel(drv));
	drv_usecwait( 20 );
	laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);

	/***
	** If laststat is 0xff, there isn't actually an IDE/ESDI
	** drive/controller. MCESDI systems populate the disk params
	** in bootinfo but use a different I/O port range
	***/
	if (laststat == 0xff)
		return;

	/*
	 * If there is something there it must reflect the selection.
	 */
	if (inb(cfgp->cfg_ioaddr1+ATP_DRVSEL) != ata_intToAtasel(drv)) {
		return;
	}

	/*
	 * See if the signature is already present.
	 */
	if (atp_isAtapi(cfgp)) {
		goto isAtapi;
	}

	/* Try an IDENTIFY DEVICE command to extract signature */
	ata_drvident(drv, cfgp, ident);
	if (atp_isAtapi(cfgp)) {
		goto isAtapi;
	}

	/* Now try an Software Reset command to extract signature */
	atp_softrst(drv, cfgp);

	/*
	 * After many tested devices this wait seems to be just right.
	 */
	drv_usecwait(10000);
	laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);
	if (atp_isAtapi(cfgp)) {
		goto isAtapi;
	} 

	if (laststat & ATS_ERROR) {
		outb(cfgp->cfg_ioaddr1+AT_DRVHD, ata_intToAtasel(drv));
		outb(cfgp->cfg_ioaddr1+AT_CMD, ATC_RESTORE);
		drv_usecwait( 100 );
		laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);
		if (laststat & ATS_BUSY) {
			drv_usecwait( 100 );
			laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);
		}
		if ((laststat & (ATS_READY|ATS_SEEKDONE)) &&
		   ((laststat & (ATS_ERROR|ATS_BUSY)) == 0)) {
			++*ata_drives;
			return;
		}
	}

	if (!ata_wait(cfgp->cfg_ioaddr1,
	    (ATS_BUSY|ATS_READY|ATS_WRFAULT|ATS_SEEKDONE|ATS_ERROR),
	    (ATS_READY|ATS_SEEKDONE), (ATS_BUSY|ATS_WRFAULT|ATS_ERROR)) ) {
			++*ata_drives;
	}

	return;

isAtapi:
	cfgp->cfg_drive[drv].dpb_devtype = DTYP_ATAPI;
	++*atapi_drives;
}

/*
 *
 * Assumption: It leaves drive select set to whatever drv is.
 */
int
ata_drvident(int drv, struct ide_cfg_entry *cfgp, struct atp_ident *ident)
{
	uchar_t laststat;

#ifdef ATAPI_DEBUG
	cmn_err(CE_NOTE,"atapi_drvident %d", drv);
#endif

	/* select the proper drive */
	outb(cfgp->cfg_ioaddr1+ATP_DRVSEL,ata_intToAtasel(drv));
	cfgp->cfg_lastdriv = drv;

	outb(cfgp->cfg_ioaddr1+AT_CMD,ATP_IDENT);
	ata_wait(cfgp->cfg_ioaddr1, ATS_BUSY,0,ATS_BUSY);
	laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);
	if (!(laststat & ATS_DATARQ)) {
		/*atp_softrst(drv, cfgp);*/
		return 1;
	}

	repinsw(cfgp->cfg_ioaddr1 + AT_DATA, (ushort *) ident, ATP_IDENTSZ);

	laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);
#ifdef ATAPI_DEBUG
	cmn_err(CE_NOTE,"laststat = 0x%x 0x%x ", laststat, ident->atp_drq);
#endif

	/*drv_usecwait(100000);*/
	return 0;
}


/*
 * We must use special commands to check for drives on a DPT type
 * controller. This must be done since the code in ata_bdinit()
 * will detect two drives on a DPT even with none connected. The
 * cmds in this routine will only work on a DPT controllers.
 * If this is a DPT type controller then return # of drives else -1.
 */

int
ata_DPTinit(struct ide_cfg_entry *cfgp)
{
	int i, DPTerror=0;
	unchar  laststat,tmpbuf[32];

	ATA_DTRACE;

	/* First we will use a DPT INQUIRY cmnd to make sure this is a DPT*/
	drv_usecwait(cfgp->cfg_delay*100);

	outb(cfgp->cfg_ioaddr1+AT_DRVHD, 0x00);   /* indicate 1st drive */
	outb(cfgp->cfg_ioaddr1+AT_HCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_LCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_SECT, 0x10); /* amount of data we expect */
	outb(cfgp->cfg_ioaddr1+AT_COUNT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_CMD, 0xd2); /* isu a inquiry cmd */

	drv_usecwait(cfgp->cfg_delay*100);

	/* Wait for it to complete */
	/* Wait for DRQ or error status */
	for (i=0; i<100; i++) {
		laststat = inb(cfgp->cfg_ioaddr1+AT_STATUS);
		if ( (laststat & (ATS_DATARQ|ATS_ERROR)) != 0)
			break;
		drv_usecwait(10);
	}

	/* We took too long or got an error so this must not be DPT*/
	if (i==100 || laststat & ATS_ERROR) {
		DPTerror++;
		goto dptdone;
	}
	repinsw(cfgp->cfg_ioaddr1+AT_DATA,(ushort *)(void *)&tmpbuf, 16);

	/* Now check that this is really a DPT vender="DPT" */
	if (tmpbuf[8] != 'D' || tmpbuf[9] != 'P' || tmpbuf[10] != 'T') {
		DPTerror++;
		goto dptdone;
	}

	/* Yes this is really a DPT so now on with drive tests */
	/* Try DPT type Test Unit ready for second drive */
	drv_usecwait(10);
	outb(cfgp->cfg_ioaddr1+AT_DRVHD, 0x20);   /* indicate second drive */
	outb(cfgp->cfg_ioaddr1+AT_HCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_LCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_SECT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_COUNT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_CMD, 0xc0); /* isu a test unit ready cmd */
	drv_usecwait(cfgp->cfg_delay*10);

	if (!ata_wait(cfgp->cfg_ioaddr1,
	    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
	    (ATS_READY|ATS_SEEKDONE),
	    (ATS_BUSY|ATS_ERROR)))
	{       /* Have second drive with good status */
		return (2);
	}

	/* look for first drive DPT way  since no second drive*/
	drv_usecwait(10);
	outb(cfgp->cfg_ioaddr1+AT_DRVHD, 0x0);   /* indicate first drive */
	outb(cfgp->cfg_ioaddr1+AT_HCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_LCYL, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_SECT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_COUNT, 0x0);
	outb(cfgp->cfg_ioaddr1+AT_CMD, 0xc0); /* isu a test unit ready cmd */
	drv_usecwait(cfgp->cfg_delay*10);

	if (!ata_wait(cfgp->cfg_ioaddr1,
	    (ATS_BUSY|ATS_READY|ATS_SEEKDONE|ATS_ERROR),
	    (ATS_READY|ATS_SEEKDONE),
	    (ATS_BUSY|ATS_ERROR)))
	{       /* Have first drive with good status */
		cfgp->cfg_capab |= CCAP_NOSEEK; /* don't issue seeks on 1-drive board */
		return (1);
	}

	/* this must  be a DPT with no drive reset it for normal use */
dptdone:
	outb(cfgp->cfg_ioaddr2+AT_DEV_CNTL,AT_CTLRESET);    /* Reset controller */
	drv_usecwait(20); /* wait atleast mandatory 10 microsecs */
	outb(cfgp->cfg_ioaddr2+AT_DEV_CNTL,AT_INTRDISAB);   /* Turn off interrupts */
	/*
	 * The outb to the address "or"ed with 0x80 below is dirty. On
	 * Adaptec controllers set up for the secondary position, once you reset
	 * them, they forget that they're supposed to respond to secondary 
	 * addresses until you un-do the reset. Thus, you have to set the reset
	 * bit in port 0x376 and clear it in port 0x3F6. But this shouldn't
	 * hurt, as long as we're still in initialization.
	 */
	outb(((cfgp->cfg_ioaddr2+AT_DEV_CNTL) | 0x80),AT_INTRDISAB);   /* Turn off interrupts */

	if (DPTerror)
		return (-1);	/* tell the world this is not DPT */
	else
		return (0);     /* tell the world we have no drives */
}

/*
 * ata_wait -- wait for the status register of a controller to achieve a
 *              specific state.  Arguments are a mask of bits we care about,
 *              and two sub-masks.  To return normally, all the bits in the
 *              first sub-mask must be ON, all the bits in the second sub-
 *              mask must be OFF.  If 10 seconds pass without the controller
 *              achieving the desired bit configuration, we return 1, else
 *              0.
 */
int
ata_wait(ushort baseport, uint mask, uint onbits, uint offbits)
{
	int i;
	ushort portno = baseport+AT_STATUS;
	uint maskval = 0;

	ATA_DTRACE;

	for (i=(ide_waitsecs*100000); i; i--)
	{
		maskval = inb(portno) & mask;
		if (((maskval & onbits) == onbits) &&
		    ((maskval & offbits) == 0))
			return(0);
		drv_usecwait(10);
	}
	return(1);
}

/*
 * ata_drvinit -- Initialize drive on controller.
 *                 Sets values in dpb for drive (based on info from controller
 *                 or ROM BIOS or defaults).
 */

ata_drvinit(int curdrv, gdev_cfgp config)
{
	struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	struct admsbuf ab;
	struct wdrpbuf wb;
	unchar laststat;
	int i;

	if (parms->dpb_devtype == DTYP_ATAPI) {
		atapi_drvinit(curdrv, config);
		return 0;
	}

	ATA_DTRACE;

	/* set up some defaults in the dpb... */
	parms->dpb_flags = (DFLG_RETRY | DFLG_ERRCORR | DFLG_VTOCSUP | DFLG_FIXREC);
	parms->dpb_drvflags = (ATF_FMTRST | ATF_RESTORE);
	parms->dpb_devtype = DTYP_DISK;
	parms->dpb_secsiz = config->cfg_defsecsiz;

	/* select the proper drive */
	outb(config->cfg_ioaddr1+AT_DRVHD,
	    (curdrv == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1));
	config->cfg_lastdriv = curdrv;

	/* wait for BUSY to clear after drive select */
	(void) ata_wait(config->cfg_ioaddr1,ATS_BUSY,0,ATS_BUSY);

	/* Establish defaults for disk geometry */
	parms->dpb_secovhd = 81;
	parms->dpb_ressecs = 0;
	parms->dpb_pbytes = 512;
	parms->dpb_drvflags |= (DPCF_CHGCYLS | DPCF_CHGHEADS | DPCF_CHGSECTS);
	bcopy("IDRIVE:",parms->dpb_tag,7);
	parms->dpb_tag[7] = curdrv | 0x30;
	parms->dpb_inqdata.id_type = 0;
	parms->dpb_inqdata.id_qualif = 0;
	parms->dpb_inqdata.id_rmb = 0;
	parms->dpb_inqdata.id_ver = 0x1;		/* scsi 1 */
	parms->dpb_inqdata.id_len = 31;
	strncpy(parms->dpb_inqdata.id_vendor, "Generic ", 8);
	strncpy(parms->dpb_inqdata.id_prod, "IDE/ESDI        ", 16);
	strncpy(parms->dpb_inqdata.id_revnum, "1.00", 4);

	if (config->cfg_ioaddr1 == ATA_PRIMEADDR)
	{       /* Get default BIOS parameters for primary drive */
		struct hdparms hd;
		hd.hp_unit = (curdrv == 0 ? 0 : 1);
		if ( drv_gethardware(HD_PARMS, &hd) == -1 )
		{
			/*
			 *+ could not get hard disk paramaters
			 */
			cmn_err(CE_WARN,
			"HD_PARMS failed: cannot get hard disk parameters\n");
		}

		parms->dpb_cyls = hd.hp_ncyls;
		parms->dpb_heads = (ushort)hd.hp_nheads;
		parms->dpb_sectors = (ushort)hd.hp_nsects;
	}
	else {       /* Pick something that will let everything be discovered */
		parms->dpb_cyls = 1024;
		parms->dpb_heads = 16;   /* updated at open time, if possible */
		parms->dpb_sectors = 255; /* updated at open time, if possible */
	}

	outb(config->cfg_ioaddr2+AT_DEV_CNTL,((parms->dpb_heads > 8 ? AT_EXTRAHDS
	    : AT_NOEXTRAHDS) |
	    AT_INTRDISAB)); /* no interrupt on this */

	/*  include fix for dtc controller here !  */
	outb(config->cfg_ioaddr1+AT_DRVHD, curdrv == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1);
	outb(config->cfg_ioaddr1+AT_HCYL, 0x0);
	outb(config->cfg_ioaddr1+AD_SPCLCMD,ADSC_MODESENSE);

	outb(config->cfg_ioaddr1+AT_CMD,ADC_SPECIAL);  /* try modesense command */
	drv_usecwait( 10 );

	(void) ata_wait(config->cfg_ioaddr1,ATS_BUSY,0,ATS_BUSY);

	/* Wait for DRQ or error status */

	for (i=0; i<10000; i++) {
		if (((laststat=inb(config->cfg_ioaddr1+AT_STATUS)) &
		    (ATS_DATARQ|ATS_ERROR)) != 0) {
			break;
		}
		if ( ((laststat & (ATS_READY|ATS_SEEKDONE)) &&
		      (~laststat & ATS_BUSY))  != 0) {
			break;
		}
		drv_usecwait( 10 );
	}

	if (!(laststat & ATS_ERROR) && (laststat & (ATS_DATARQ|ATS_READY))) {
		cmn_err(CE_CONT,"!ATA_DRVINIT: got (ATS_DATARQ|ATS_READY) after ModeSense\n");
        	/* Get modesense data */
		repinsw(config->cfg_ioaddr1+AT_DATA, (ushort *)(void *)&ab, 6);

	        /* mode sense command appears to have succeeded, for valid */
		/* Adaptec EDSI/RLL must have ADMS_VALID set and have	   */
		/* (ADMSF_VAC|ADMSF_VMS) set 				   */
		if ((ab.adms_valid == ADMS_VALID) &&
		    (ab.adms_flags & (ADMSF_VAC | ADMSF_VMS))) {
			parms->dpb_secovhd = ab.adms_secovhd;
			parms->dpb_drvflags &= ~(DPCF_CHGHEADS | DPCF_CHGSECTS);
			/* set to adaptec controllor */
			parms->dpb_drvflags |= (ATF_ADAPTEC | DPCF_CHGCYLS);  
			if ((ab.adms_flags & ADMSF_CIDMSK) == ADCID_ESDI) {
		        /* ESDI controller - 2 reserved cyls and BADMAP avail */
				parms->dpb_rescyls = 2;
				parms->dpb_drvflags |= DPCF_BADMAP;
			}
			else { /* RLL controller - 1 res cyl and no BADMAP */
				parms->dpb_rescyls = 1;
				parms->dpb_drvflags |= (DPCF_CHGHEADS | DPCF_CHGCYLS);
				strncpy(parms->dpb_inqdata.id_prod, "MFM/RLL         ", 16);
			}

			/* modesense data, use it */
			parms->dpb_pcyls = ((ushort)ab.adms_cylh << 8) | ab.adms_cyll;
			parms->dpb_pheads = ab.adms_nhds;
			parms->dpb_psectors = ab.adms_nsect;
			parms->dpb_secovhd = ab.adms_secovhd;
			parms->dpb_pdsect = parms->dpb_sectors;

			/* If we aren't on the primary controller, there aren't 
			 * BIOS values to use as operating values, so use the 
		 	 * physical ones from the controller.
	 	 	*/
			if (config->cfg_ioaddr1 != ATA_PRIMEADDR) {
				parms->dpb_cyls = parms->dpb_pcyls;
				parms->dpb_heads = parms->dpb_pheads;
				parms->dpb_sectors = parms->dpb_psectors;
			} else 
			/* Otherwise, be sure to report the true number of 
	 	 	 * cylinders. The values obtained from the BIOS may be 
		 	 * wrong when we are using the true geometry, and the 
	 		 * number of cylinders reported by the BIOS is less 
		 	 * than the number of physical ones.  We determine that
		 	 * we are in true geometry by making sure that the heads
		 	 * match and the sectors are close (to allow for 
			 * slipped sectoring).
	 	 	*/
				if (parms->dpb_heads == parms->dpb_pheads &&
		    	   	( !(i = parms->dpb_sectors - parms->dpb_psectors) ||
		    	   	i == -1 || i == -2) &&
		    	   	(parms->dpb_pcyls > 1023) &&
		    	   	(parms->dpb_pcyls > parms->dpb_cyls)) {
					parms->dpb_cyls = parms->dpb_pcyls;
				}
			return(0);
		}
		cmn_err(CE_CONT,"!ATA_DRVINIT: Invalid ModeSense data\n");
	}

	/* laststat was error, never got DRQ, or mode sense data returned  */
	/* is invalid some IDE drives have successful return to mode sense */
        /* but data is random garbage. Try the Read Parameters command     */

	drv_usecwait( 20 );
	outb(config->cfg_ioaddr1+AT_CMD,AWC_READPARMS);
	(void) ata_wait(config->cfg_ioaddr1,ATS_BUSY,0,ATS_BUSY);
	/* Wait for DRQ or error status */
	for (i=0; i<10000; i++) {
		if (((laststat=inb(config->cfg_ioaddr1+AT_STATUS)) &
		    (ATS_DATARQ|ATS_ERROR)) != 0)
			break;
		drv_usecwait( 10 );
	}

	if(laststat & ATS_ERROR) {
		cmn_err(CE_CONT,"!ATA_DRVINIT: ATS_ERROR present after ReadParms\n");
		parms->dpb_pcyls = parms->dpb_cyls;
		parms->dpb_pheads = parms->dpb_heads;
		parms->dpb_psectors = parms->dpb_sectors;
		goto check_parms;
	}
	if( (laststat & ATS_DATARQ) == 0 ) {
		cmn_err(CE_CONT,"!ATA_DRVINIT: ATS_DATARQ not present after ReadParms\n");
		parms->dpb_pcyls = parms->dpb_cyls;
		parms->dpb_pheads = parms->dpb_heads;
		parms->dpb_psectors = parms->dpb_sectors;
		goto check_parms;
	}

	/* Read Parameters worked.  Get data */
	repinsw(config->cfg_ioaddr1+AT_DATA,(ushort *)&wb,WDRPBUF_LEN);
	parms->dpb_pcyls = wb.wdrp_fixcyls;
	parms->dpb_pheads = wb.wdrp_heads;
	parms->dpb_psectors = wb.wdrp_sectors;
	parms->dpb_secovhd = wb.wdrp_secsiz - parms->dpb_secsiz;

	/* If we aren't on the primary controller, there are no BIOS 
	 * values to use as operating values, so use the physical ones 
	 * from the controller.
	 */
	if (config->cfg_ioaddr1 != ATA_PRIMEADDR) {
		parms->dpb_cyls = wb.wdrp_fixcyls;
		parms->dpb_heads = wb.wdrp_heads;
		parms->dpb_sectors = wb.wdrp_sectors;
		parms->dpb_rescyls = 2;
	} 

check_parms:
	if ((parms->dpb_heads > 16) && (parms->dpb_pheads > 16)) {
		if (config->cfg_ioaddr1 == ATA_PRIMEADDR) 
			cmn_err(CE_CONT,
"Disk Driver: Disk %d (primary controller) has invalid parameters. The hard\n",
				curdrv);
		else
			cmn_err(CE_CONT,
"Disk Driver: Disk %d (secondary controller) has invalid parameters. The hard\n",
				curdrv);
		cmn_err(CE_CONT,
"disk is defined with %d heads. The maximum valid number is 16. You must \n",
			parms->dpb_pheads);
		cmn_err(CE_CONT,
"re-run your setup utilities to chose a valid disk type parameter. Please\n");
		cmn_err(CE_CONT,
"refer to the installation guide for more information on this problem.\n\n");
		/*
		 *+ Hard disk parameters invalid too many disk heads.  
		 *+ Re-run setup utilities to correct.
		 *+ Set flags to offline so inquiry fails and drive isn't
		 *+ added to the EDT.
		 */
		parms->dpb_flags |= DFLG_OFFLINE;
	}
	/* Otherwise, be sure to report the true number of cylinders.
	 * The values obtained from the BIOS may be wrong when we are
	 * using the true geometry, and the number of cylinders
	 * reported by the BIOS is less than the number of physical
	 * ones.  We determine that we are in true geometry by
	 * making sure that the heads match and the sectors are close
	 * (to allow for slipped sectoring).
	 */
	if (parms->dpb_heads == parms->dpb_pheads &&
	   (!(i = parms->dpb_sectors - parms->dpb_psectors) ||
	   i == -1 || i == -2) &&
	   (parms->dpb_pcyls > 1023) && (parms->dpb_pcyls > parms->dpb_cyls)) {
		parms->dpb_cyls = parms->dpb_pcyls;
		parms->dpb_rescyls = 2;
	}

	/* if BIOS heads > 16 and phys heads <= 16, do translation of */
	/* requests when issuing commands			      */
	if ((parms->dpb_heads > 16) && (parms->dpb_pheads <= 16)) {
		parms->dpb_flags |= DFLG_TRANSLATE;
		cmn_err(CE_NOTE,"!ATA TRANS: Cyls %d Hds %d sects %d Pcyls %d Phds %d Psects %d\n",
		parms->dpb_cyls, parms->dpb_heads, parms->dpb_sectors,
		parms->dpb_pcyls, parms->dpb_pheads, parms->dpb_psectors);
	}
	return(0);
}

/*
 * ata_cmd -- perform command on AT-class hard disk controller.
 */

int
ata_cmd(int cmd, int curdrv, gdev_cfgp config)
{
	struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	int error = 0;

	ATA_DTRACE;

	/* Trying to execute an ATA command on an ATAPI device */
	if (parms->dpb_devtype == DTYP_ATAPI) {
		return 1;
	}

	if (cmd == DCMD_RETRY) {
		cmd = parms->dpb_command;        /* just try again */
		if ((parms->dpb_retrycnt == 3) || (parms->dpb_retrycnt == 4)) 
			if (parms->dpb_drvflags & ATF_RECALDONE) {
				parms->dpb_drvflags &= ~ATF_RECALDONE; 
			}
			else {
				parms->dpb_drvflags |= (ATF_RESTORE|ATF_RECALDONE);
				cmn_err(CE_NOTE,"!ATA_CMD Recal retry %d, cmd 0x%x\n",
					parms->dpb_retrycnt, cmd);
			}
	}
	else {       /* prime for new command */
		parms->dpb_command = (ushort) cmd;   /* save cmd for retry */
		config->cfg_curdriv = curdrv;
	}

	if ((parms->dpb_drvflags & ATF_RESTORE) &&
	    ((cmd == DCMD_READ) || (cmd == DCMD_WRITE)))
	{       /* we have to recalibrate heads before actual I/O command */
		cmd = DCMD_RECAL;
	}

	switch (cmd)
	{
	case DCMD_RESET:
		ata_reset(config, NO_ERR);
		return(0);
	case DCMD_READ:
		parms->dpb_drvcmd = ATC_RDSEC;
		/* enable controller ECC & RETRY only if we're supposed to. */
		if (parms->dpb_flags & (DFLG_RETRY | DFLG_ERRCORR))
			parms->dpb_drvcmd &= ~ATCM_ECCRETRY; /* inverse logic */
		parms->dpb_state = DSTA_NORMIO;
#ifdef TIMEOUT_DEBUG
		/* Use KDB to put a value in ata_readcnt. This causes that */
		/* number of reads to NOT be submitted simulating timeouts  */
		/* (or a different error if the mechanism is enhanced). */
		if (ata_readcnt) {
			ata_readcnt--;
			return(0);
		}
#endif
		break;
	case DCMD_WRITE:
		parms->dpb_drvcmd = ATC_WRSEC;
		/* enable controller ECC & RETRY only if we're supposed to. */
		if (parms->dpb_flags & (DFLG_RETRY | DFLG_ERRCORR))
			parms->dpb_drvcmd &= ~ATCM_ECCRETRY; /* inverse logic */
		parms->dpb_state = DSTA_NORMIO;
#ifdef TIMEOUT_DEBUG
		/* Use KDB to put a value in ata_writecnt. This causes that */
		/* number of writes to NOT be submitted simulating timeouts  */
		/* (or a different error if the mechanism is enhanced). */
		if (ata_writecnt) {
			ata_writecnt--;
			return(0);
		}
#endif
		break;
	case DCMD_SETPARMS:
		{
		struct admsbuf ab;      /* for mode select */
		ata_setskew(parms);     /* set skew factor appropriately */
		
		if (curdrv != config->cfg_lastdriv) {
			/* select the proper drive */
			outb(config->cfg_ioaddr1+AT_DRVHD, (curdrv == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1));
			config->cfg_lastdriv = curdrv;
		}
		
		if (parms->dpb_drvflags & ATF_ADAPTEC && parms->dpb_rescyls < 2) {
			/* Adaptec 237x (RLL or MFM) device.  Must use Adaptec
			 * Mode Select command to program the controller.  Do
			 * Adaptec 232x (ESDI) as a normal controller,
			 * programing it with a Set Parameters command.  The
			 * Mode Select command messes up any translate mode
			 * that an Adaptec ESDI drive might be using.
			 */
			ushort temp;

			ab.adms_valid = ADMS_VALID;
			temp = parms->dpb_cyls + parms->dpb_rescyls;
			ab.adms_cylh = temp >> 8;
			ab.adms_cyll = temp & 0xff;
			ab.adms_nhds = parms->dpb_heads;
			temp = parms->dpb_sectors + parms->dpb_ressecs;
			ab.adms_nsect = (unsigned char) temp;
			outb(config->cfg_ioaddr1+AD_SPCLCMD,ADSC_MODESEL);
			outb(config->cfg_ioaddr1+AT_CMD,ADC_SPECIAL);
			ata_wait(config->cfg_ioaddr1,ATS_DATARQ,ATS_DATARQ,0);
			repoutsw(config->cfg_ioaddr1+AT_DATA, 
					(ushort *)(void *)&ab, 6);
		} else {       /* Not Adaptec RLL-- use WD approach */
			if (parms->dpb_flags & DFLG_TRANSLATE)
				outb(config->cfg_ioaddr1+AT_DRVHD,
			    	(curdrv == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1)
			    	| (parms->dpb_pheads-1));
			else
				outb(config->cfg_ioaddr1+AT_DRVHD,
			    	(curdrv == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1)
			    	| (parms->dpb_heads-1));
			outb(config->cfg_ioaddr1+AT_COUNT,
			    parms->dpb_sectors+parms->dpb_ressecs);
			outb(config->cfg_ioaddr1+AT_LCYL, parms->dpb_cyls & 0xff);
			outb(config->cfg_ioaddr1+AT_HCYL, parms->dpb_cyls >> 8);
			outb(config->cfg_ioaddr1+AT_CMD,ATC_SETPARAM);
			/* set write precomp cylinder as follows:
			 * If number of heads is odd, use number of cyls.
			 * If even, use half the number of cyls.
			 */
			parms->dpb_wpcyl = ((parms->dpb_heads & 1) ? parms->dpb_cyls : parms->dpb_cyls / 2);
		}
		ata_wait(config->cfg_ioaddr1,ATS_BUSY,0,ATS_BUSY);
		drv_usecwait(1000); 
		return(0);
		}
	case DCMD_RECAL:
		parms->dpb_drvcmd = ATC_RESTORE;
		parms->dpb_state = DSTA_RECAL;
		break;
	case DCMD_SEEK:
		parms->dpb_drvcmd = ATC_SEEK;
		parms->dpb_state = DSTA_SEEKING;
		break;
	default:
#ifdef DEBUG
		cmn_err(CE_WARN,"ata_cmd unimplemented command %d!",cmd);
#endif
		break;
	}
	ata_docmd(curdrv, config);
	return (error);
}

/*
 * ata_docmd -- output a command to the controller.
 *               Generate all the bytes for the task file & send them.
 */

void
ata_docmd(int curdrv, gdev_cfgp config)
{
	struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	ulong resid;
	uint  headval, cmd;
	uint head, sector;

	ATA_DTRACE;

retry:

	if (ata_wait(config->cfg_ioaddr1, (ATS_BUSY|ATS_READY), ATS_READY, ATS_BUSY)) {
		cmn_err(CE_WARN,"!ATA_DOCMD: Controller not ready, or busy");
		cmd = parms->dpb_command;   /* save to restore after reset */
		ata_reset(config, NO_ERR);
		parms->dpb_command = (ushort) cmd; /* restore to previous value*/
	}

	outb(config->cfg_ioaddr1+AT_PRECOMP, parms->dpb_wpcyl >> 2);

	sector = parms->dpb_sectors;

	outb(config->cfg_ioaddr1+AT_COUNT, parms->dpb_sectcount);
	outb(config->cfg_ioaddr1+AT_SECT, (parms->dpb_cursect % sector) + 1);

	if (parms->dpb_flags & DFLG_TRANSLATE)
		head = parms->dpb_pheads;
	else
		head = parms->dpb_heads;

	outb(config->cfg_ioaddr2+AT_DEV_CNTL, (head > 8 ? AT_EXTRAHDS : AT_NOEXTRAHDS));

	resid = parms->dpb_cursect / sector;
	headval = resid % head;
	headval |= (curdrv == 0 ? ATDH_DRIVE0 : ATDH_DRIVE1);

	outb(config->cfg_ioaddr1+AT_DRVHD, headval);

	resid = resid / head;
	parms->dpb_curcyl = (ushort) resid;  /* save current cylinder number */

	outb(config->cfg_ioaddr1+AT_LCYL, resid & 0xff);
	outb(config->cfg_ioaddr1+AT_HCYL, resid >> 8);
	outb(config->cfg_ioaddr1+AT_CMD, parms->dpb_drvcmd);

	parms->dpb_drverror = 0;

	/* If we're doing some output, fill the controller's buffer */

	if (((parms->dpb_drvcmd & ~(ATCM_ECCRETRY|ATCM_LONGMODE)) ==
	    (ATC_WRSEC & ~(ATCM_ECCRETRY|ATCM_LONGMODE)))) {
		if (ata_senddata(config, parms)) {
			/* save cmd to restore after reset */
			cmd = parms->dpb_command;   
			ata_reset(config, NO_ERR);
			parms->dpb_command = (ushort) cmd; /* restore for retry */
			goto retry;
		}
	}
}

/* reset_count is used to monitor the number of consecutive resets */
STATIC int	reset_count = 0;

/* General reset mechanism to reset controller and reset driver params */
void
ata_reset(gdev_cfgp config, int set_error)
{
	int curdrive, drive;

	ATA_DTRACE;

	cmn_err(CE_NOTE,"!ATARESET: reset attempt %d\n",reset_count);
	reset_count++;

	/* Save the current drive to restore state later */
	curdrive = config->cfg_curdriv;

	/* Reset controller, then set params for drives */
	outb(config->cfg_ioaddr2+AT_DEV_CNTL, AT_CTLRESET);
	drv_usecwait(20);
	outb(config->cfg_ioaddr2+AT_DEV_CNTL, AT_NOEXTRAHDS);
	drv_usecwait(50);

	if (ata_wait(config->cfg_ioaddr1,
				(ATS_BUSY|ATS_READY), ATS_READY, ATS_BUSY))
		cmn_err(CE_WARN,"Disk Driver: Reset failure please check your disk controller");

	for (drive=0; drive < config->cfg_ata_drives; drive++) {

		ata_cmd(DCMD_SETPARMS, drive, config);

		config->cfg_drive[drive].dpb_drvflags |= ATF_RESTORE;

		if (ata_wait(config->cfg_ioaddr1,
					(ATS_BUSY|ATS_READY), ATS_READY, ATS_BUSY))
			cmn_err(CE_WARN,"!Disk Driver: reset disk params failed\n");
	}

	config->cfg_curdriv = (ushort) curdrive;

	if (set_error == SET_ERR) {

		/* set error value to represent a timeout so job is retried. */
		config->cfg_drive[curdrive].dpb_drverror = DERR_TIMEOUT;
		config->cfg_drive[curdrive].dpb_intret = DINT_ERRABORT;
		config->cfg_drive[curdrive].dpb_retrycnt = 11; 
	}
}

/*
 * ata_setskew -- set the dpb_skew value based on interleave and sectors
 *                 per track.  If interleave isn't 1, set to 0 (which means
 *                 it's either irrelevant or unknown).  Otherwise, if we
 *                 are on an RLL controller, we use a skew of 1.  Else,
 *                 use skew of 2 (ESDI -- ST506 isn't interleave 1).
 */
void
ata_setskew(struct ata_parm_entry *parms)
{
	ATA_DTRACE;

	parms->dpb_skew = ((parms->dpb_sectors == RLL_NSECS) ? 1 : 2);
}

/*
 * ata_senddata -- send a sector's worth of data to the controller.
 */

int
ata_senddata(gdev_cfgp config, struct ata_parm_entry *parms)
{
	int bcnt = parms->dpb_secsiz;   /* Number of bytes to send */

	ATA_DTRACE;

	/* before writing data, BUSY must be clear and DATARQ set. If */
	/* status isn't set properly fail request */

	if (ata_wait(config->cfg_ioaddr1,
				(ATS_BUSY|ATS_DATARQ), ATS_DATARQ, ATS_BUSY)) {
		cmn_err(CE_CONT, "!ATA_SENDDATA: Controller not ready!");
		return(1);
	}

	repoutsw(config->cfg_ioaddr1+AT_DATA,
			(ushort *)(void *)parms->dpb_newvaddr, bcnt/2);

	parms->dpb_newvaddr += bcnt;   /* update buf ptr */
	parms->dpb_newcount -= bcnt;   /* and decr count */

	return(0);
}

/*
 * ata_recvdata -- like ata_senddata, but for getting data out of the
 *                  controller.
 */

int
ata_recvdata(gdev_cfgp config,struct ata_parm_entry *parms)
{
	int bcnt = parms->dpb_secsiz;   /* Number of bytes to send */

	ATA_DTRACE;

	/* before writing data, BUSY must be clear and DATARQ set. If */
	/* status isn't set properly fail request */

	if (ata_wait(config->cfg_ioaddr1,
				(ATS_BUSY|ATS_DATARQ), ATS_DATARQ, ATS_BUSY)) {
		cmn_err(CE_CONT, "!ATA_RECVDATA: Controller not ready!");
		return(1);
	}

	repinsw(config->cfg_ioaddr1+AT_DATA,
			(ushort *)(void *)parms->dpb_newvaddr, bcnt/2);

	parms->dpb_newvaddr += bcnt;   /* update buf ptr */
	parms->dpb_newcount -= bcnt;   /* and decr count */

	return(0);
}

/*
 * ata_int -- process AT-class controller interrupt.
 */
struct ata_parm_entry *
ata_int(struct ide_cfg_entry *config)
{
	struct ata_parm_entry *parms;
	unchar cmdstat;
	unchar drverr;
	int i;

	parms = &config->cfg_drive[config->cfg_curdriv];

	/* It's atapi */
	if ( parms->dpb_devtype != DTYP_DISK ) {
		if ((config->cfg_flags & CFLG_BUSY) == 0) {
			/* Not for us, we're doing nothing */
			return(NULL);
		}

		/*
		 * This device has missed interrupts, therefore we are
		 * polling mode and we dispose of all interrupts. 
		 */
		if (parms->dpb_atpbusywait) {
			/*cmn_err(CE_NOTE,"Unneeded interrupt");*/
			return NULL;
		}
		atapi_protocol(config);
		goto done;
	}

	/*
	 * If we don't have an Adaptec,
	 * get the contents of the status (and error, if necessary) register(s).
	 * This will guarantee that the controllers on Compaqs drop their interrupt
	 * line.  We do this even if the controller isn't supposed to be busy so
	 * we don't get out of sync on leftover interrupts...
	 */

	if ((parms->dpb_drvflags & ATF_ADAPTEC) == 0)
		cmdstat = inb(config->cfg_ioaddr1+AT_STATUS);

	if ((config->cfg_flags & CFLG_BUSY) == 0)
		return(NULL);   /* Not for us, we aren't doing anything */

	switch ((int)parms->dpb_state)
	{
	case DSTA_RECAL:
		if (parms->dpb_drvflags & ATF_RESTORE)
		{       /* restored, just do normal op as a retry */
			parms->dpb_drvflags &= ~ATF_RESTORE;
			ata_cmd(DCMD_RETRY, config->cfg_curdriv, config);
			parms->dpb_intret = DINT_CONTINUE;
			break;
		}
		/* FALLTHRU */
	case DSTA_SEEKING:
		if (parms->dpb_drvflags & ATF_ADAPTEC)
			cmdstat = inb(config->cfg_ioaddr1+AT_STATUS);
		if (cmdstat & ATS_ERROR)
		{       /* Error on seek/restore... tell guys above */
			/* wait for non-busy status */
			for (i = 0; i < 1000; i++) {
				if (((cmdstat = inb(config->cfg_ioaddr1 + AT_STATUS)) & ATS_BUSY) == 0)
					break;
				drv_usecwait(10);
			}
			if (i == 1000)
				cmn_err(CE_NOTE,"!ata_int never got non-busy, got 0x%x\n",cmdstat);
			drverr = inb(config->cfg_ioaddr1+AT_ERROR);
			parms->dpb_drverror = ata_err(drverr);
			parms->dpb_intret = DINT_ERRABORT;
		}
		else {       /* seek/restore completed normally. */
			parms->dpb_intret = DINT_COMPLETE;
			/* successful I/O, if following a reset we need to */
			/* set reset_count, so zero rather than test and set */
			reset_count = 0;
		}
		break;
	case DSTA_NORMIO:
		if (parms->dpb_flags & DFLG_READING) {
			if (parms->dpb_drvflags & ATF_ADAPTEC) {
				ata_recvdata(config, parms);     /* Get data */
				cmdstat = inb(config->cfg_ioaddr1+AT_STATUS);
			}
			else
				/* don't read data if either error condition */
				if (!(cmdstat & (ATS_WRFAULT|ATS_ERROR))) {
					/* if unable to read, reset & fail job*/
					if (ata_recvdata(config, parms)) {
						ata_reset(config, SET_ERR);
						break;
					}
				}
		} else if (parms->dpb_drvflags & ATF_ADAPTEC) {
			cmdstat = inb(config->cfg_ioaddr1+AT_STATUS);
		}
		/* For a Write Fault, reset controller, and issue retries */
		if (cmdstat & ATS_WRFAULT) {
			cmn_err(CE_WARN,"Disk Driver: controller error, resetting controller");
			ata_reset(config, SET_ERR);
			break;
		}
		/* check for ECC correction when ECC is off */
		if ((cmdstat & ATS_ECC) && !(parms->dpb_flags & DFLG_ERRCORR)) {
			parms->dpb_drverror = DERR_DATABAD;
			parms->dpb_intret = DINT_ERRABORT;
			break;
			/* check for real error */
		} else 
			if (cmdstat & ATS_ERROR) {
		        /* Error on I/O... tell guys above */
			/* wait for non-busy status */
				for (i = 0; i < 1000; i++) {
					if (((cmdstat = inb(config->cfg_ioaddr1 + AT_STATUS)) & ATS_BUSY) == 0)
						break;
					drv_usecwait(10);
				}
				if (i == 1000)
					cmn_err(CE_NOTE,"!ata_int never got non-busy, got 0x%x\n",cmdstat);
				drverr = inb(config->cfg_ioaddr1+AT_ERROR);
				reset_count = 0;
				cmn_err(CE_NOTE,"!ATAINT: stat 0x%x, errval 0x%x \n",cmdstat, drverr);
				parms->dpb_drverror = ata_err(drverr);
				parms->dpb_intret = DINT_ERRABORT;
				break;
			}
		/* Read/wrote data successfully.  Update dpb. */

		/* successful I/O, if following a reset we need to */
		/* set reset_count, so just zero rather than test and set */
		reset_count = 0;

		ata_xferok(parms,1);

		if (parms->dpb_intret == DINT_CONTINUE)
		{       /* see if we actually have to do anything */
			if (!(parms->dpb_flags & DFLG_READING))
				if (ata_senddata(config, parms)) {
					/* can't write data so reset & fail */
					ata_reset(config, SET_ERR);
					break;
				}
		}
		break;

	default:
#ifdef DEBUG
		cmn_err(CE_PANIC,"ata_int: invalid state 0x%x!",parms->dpb_state);
#endif
		break;
	}

done:
	parms->dpb_control->SCB_Stat = parms->dpb_intret;
	return (parms);  /* let caller know we did something... */
}

/*
 * ata_xferok -- used by drivers that can detect partial transfer success
 *                to indicate that some number of sectors have been moved
 *                successfully between memory and the drive.
 *
 *                This function also sets dpb_intret depending on
 *                on dpb_sectcount.
 */
void
ata_xferok(struct ata_parm_entry *parms, int sectors_transferred)
{

	parms->dpb_cursect += sectors_transferred;
#ifdef NOT_YET
	parms->dpb_totcount += (sectors_transferred * parms->dpb_secsiz);
#endif

	parms->drq_count = parms->dpb_newcount;
	parms->dpb_curaddr = parms->dpb_newaddr;
	parms->dpb_virtaddr = parms->dpb_newvaddr;

	/* set dpb_intret depending on what's left... */

	if ((parms->dpb_sectcount -= sectors_transferred) != 0) {
		parms->dpb_intret = DINT_CONTINUE;
	} else {
		parms->dpb_intret = DINT_COMPLETE;
	}
}

/*
 * The following table is used to translate the bits in the controller's
 * 'error' register (AT_ERROR) to Generic error codes.  It is indexed by
 * the bit number of the first bit to be found ON in the register (or 8
 * if NO bits are found to be on).
 */

static ushort xlaterr [9] =
{
	DERR_DAMNF,
	DERR_TRK00,
	DERR_ABORT,
	DERR_UNKNOWN,
	DERR_SECNOTFND,
	DERR_UNKNOWN,
	DERR_DATABAD,
	DERR_BADMARK,
	DERR_UNKNOWN
};

/*
 * athd_err -- Translate the AT_ERROR register into a Generic error #
 */
ushort
ata_err(unchar errcod)
{
	int i;

	for (i = 0; i<8; i++)
	{
		if (errcod & 1) break;
		errcod >>= 1;
	}
	return (xlaterr[i]);
}

/*
 * ata_cplerr -- process device completions and errors
 */

void
ata_cplerr(struct ide_cfg_entry *config, struct control_area *control)
{
	struct ata_parm_entry *parms;

	parms = &config->cfg_drive[config->cfg_curdriv];

	switch ((int)parms->dpb_intret) {

	case DINT_CONTINUE:
		break;

	case DINT_COMPLETE:
	case DINT_NEWSECT:
		parms->dpb_flags &= ~DFLG_BUSY;
		parms->dpb_state = DSTA_IDLE;
		config->cfg_flags &= ~CFLG_BUSY;
 		ide_job_done(control);
		break;

	case DINT_ERRABORT:
		/* Something blew up.  Handle error properly */
		if (ata_error(config, parms)) {
			parms->dpb_flags &= ~DFLG_BUSY;
			parms->dpb_state = DSTA_IDLE;
			config->cfg_flags &= ~CFLG_BUSY;
			ide_job_done(control);
		}
		break;

	default:
		break;
	}

}

/*
 * gdev_error -- handle error reported by controller-level code...
 */

int
ata_error(struct ide_cfg_entry *config, struct ata_parm_entry *parms)
{
	ushort save_cmd;

	switch (parms->dpb_drverror) {

	case DERR_ATAPI:
		return (1);
		break;

	case DERR_DNOTRDY:
	case DERR_WRTPROT:
	case DERR_NODISK:
	case DERR_MEDCHANGE:
	case DERR_PASTEND:
	case DERR_EOF:
		parms->dpb_reqdata.sd_ba = sdi_swap32(parms->dpb_cursect);
		parms->dpb_reqdata.sd_key = SD_MEDIUM;
		parms->dpb_reqdata.sd_sencode = SC_IDERR;
		parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
		parms->dpb_reqdata.sd_valid = 1;

		return (1);
		break;

	default:
		if ((parms->dpb_flags & DFLG_RETRY) ||
		    ((parms->dpb_retrycnt < 1) && (parms->dpb_devtype == DTYP_DISK))) {
			if (++parms->dpb_retrycnt <= 10) {

				parms->dpb_newaddr = parms->dpb_curaddr;
				parms->dpb_newvaddr = parms->dpb_virtaddr;
				parms->dpb_newcount = parms->drq_count;

				drv_getparm(LBOLT, &config->cfg_laststart);

				if (parms->dpb_retrycnt == 5) {
					save_cmd = parms->dpb_command;
					config->cfg_CMD(DCMD_RESET, config->cfg_curdriv, config);
					parms->dpb_command = save_cmd;
				}
				parms->dpb_flags |= DFLG_BUSY;
				config->cfg_flags |= CFLG_BUSY;
				config->cfg_CMD(DCMD_RETRY, config->cfg_curdriv, config);
				return(0);
			}
		}
		parms->dpb_reqdata.sd_ba = sdi_swap32(parms->dpb_cursect);
		parms->dpb_reqdata.sd_key = SD_MEDIUM;
		parms->dpb_reqdata.sd_sencode = SC_IDERR;
		parms->dpb_reqdata.sd_errc = SD_ERRHEAD;
		parms->dpb_reqdata.sd_valid = 1;

		return (1);
	}
}

/*
 * ata_getinfo -- get controller specific info...
 */

int
ata_getinfo(struct hbagetinfo *getinfop)
{
	getinfop->iotype = F_PIO;

	if (getinfop->bcbp) {
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = ATA_MAXXFER;
		getinfop->bcbp->bcb_physreqp->phys_align = ATA_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = ATA_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = ATA_DMASIZE;
	}
}

/*
 * ata_timeout - Recovery routine for a stuck ata command.
 */
void
ata_timeout(struct ide_cfg_entry *config)
{
	struct ata_parm_entry *parms = &config->cfg_drive[config->cfg_curdriv];
	struct ata_ha *ha = ide_sc_ha[parms->dpb_control->adapter];

	cmn_err(CE_WARN,"Disk Driver Request Timed Out, Resetting Controller");

	ata_reset(config, SET_ERR);

	/* now call gdev_cplerr to signal failed completion of job */
        IDE_HIM_LOCK;
        parms->dpb_intret = parms->dpb_control->SCB_Stat = DINT_ERRABORT;
        parms->dpb_control->SCB_TargStat = DERR_TIMEOUT;
        ata_cplerr(config, parms->dpb_control);
        IDE_HIM_UNLOCK;
}

/*
 * atatimeout - Called by the periodic timer to see if any commands are
 *		stuck in the controller.
 */
void
atatimeout(struct ide_cfg_entry *config)
{
	long lbolt_val;
	struct ata_parm_entry *parms = &config->cfg_drive[config->cfg_curdriv];
	struct ata_ha *ha;

	if (drv_getparm(LBOLT,&lbolt_val) == -1) {
		return;
	}

	/* Nothing on the controller */
	if ((config->cfg_flags & CFLG_BUSY) == 0) {
		return;
	}

	if ((parms->dpb_devtype == DTYP_ATAPI) &&
		(!parms->dpb_atpbusywait) &&
		(lbolt_val - config->cfg_laststart > atapi_timeout)) {
		atp_timeout(config);
	} else if (lbolt_val - config->cfg_laststart > 30 * HZ) {
		ata_timeout(config);
	}
}

/*
 * ata_int_enable -- enable AT-class controller interrupts.
 */
void
ata_int_enable(struct ide_cfg_entry *config)
{
	outb(config->cfg_ioaddr2+AT_DEV_CNTL, AT_INTRENABL);
}

/*
 * ataintdisable -- disable AT-class controller interrupts.
 */
void
ataintdisable(struct ide_cfg_entry *config)
{
	outb(config->cfg_ioaddr2+AT_DEV_CNTL, AT_INTRDISAB);
}

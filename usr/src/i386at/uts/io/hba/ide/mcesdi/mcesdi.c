#ident	"@(#)kern-pdi:io/hba/ide/mcesdi/mcesdi.c	1.2.1.2"
#ident	"$Header$"

/*
 * IBM Micro Channel Hard Disk Low-Level Controller Interface Routines.
 * For use with Generic Disk Driver.  This file supports the stock 
 * ESDI controllers as either primary or secondary controller.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

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
#include <io/dma.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi_hier.h>
#include <io/target/sdi/sdi.h>
#include <io/hba/ide/ide.h>
#include <io/hba/ide/ide_ha.h>
#include <io/hba/ide/gdev.h>
#include <io/hba/ide/mcesdi/mc_esdi.h>
#include <util/mod/moddefs.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#ifdef MCESDI_DEBUG_TRACE
#define MCESDI_DTRACE cmn_err(CE_NOTE,"%d-%s",__LINE__,__FILE__);
#else
#define MCESDI_DTRACE
#endif

#define MCESDI_MAXXFER	64*1024
#define MCESDI_MEMALIGN	512
#define MCESDI_BOUNDARY	0
#define MCESDI_DMASIZE	32

#ifndef SS_FMT_UNIT
#define SS_FMT_UNIT 0x04
#endif

#ifndef SM_VERIFY
#define SM_VERIFY 0x2f
#endif

extern int ide_waitsecs;
extern struct ata_ha *ide_sc_ha[]; /* SCSI bus structs */



void	mcesdi_xferok();
void	ESDIwait();
void	ESDIstin();
int 	ESDIdma();
ushort	esdierrmsg(int, struct ide_cfg_entry *, unchar);
ushort	esdi2errmsg(ushort);
unchar	ESDIstatus();


int	mcfind_drive(), mcesdi_docmd(), esdi_chk_isr();

ushort ESDI_SB[ESDI_SB_SIZE];
struct	dma_cb	*mcesdi_cb;


/*
 *	The following inb, outb, inw, outw functions were created for the 
 *	MCA ESDI controller must be different to workaround a TLB bug 
 *	in the 386 (B stepping) chip involving I/O addresses above 1000h.
 *	
 *	The following functions were replaced with the asm inline equivalents
 * 	to speed them up and avoid the use of non-DDI compliant routines.
 *
 *esdiinb(reg)
 *unsigned int reg;
 *{
 *	unsigned int val;
 *
 *	intr_disable();
 *	flushtlb();
 *	val = inb(reg);
 *	intr_restore();
 *	return(val);
 *}
 *
 *esdiinw(reg)
 *unsigned int reg;
 *{
 *	unsigned int val;
 *
 *	intr_disable();
 *	flushtlb();
 *	val = inw(reg);
 *	intr_restore();
 *	return(val);
 *}
 *
 *esdioutb(reg,val)
 *unsigned int reg, val;
 *{
 *	intr_disable();
 *	flushtlb();
 *	outb(reg,val);
 *	intr_restore();
 *	return;
 *}
 *
 *esdioutw(reg,val)
 *unsigned int reg, val;
 *{
 *	intr_disable();
 *	flushtlb();
 *	outw(reg,val);
 *	intr_restore();
 *	return;
 *}
 */

asm unsigned char
esdiinb(addr)
{
%mem addr; lab not_b1;
	movl	addr, %edx
	pushfl
	cli
#if defined(PDI_SVR42) || defined(BUG386B1)
	cmpl	$0,do386b1
	je	not_b1
	movl    %cr3, %eax
	movl    %eax, %cr3
not_b1:
#endif
	xorl	%eax, %eax
	inb	(%dx)
	sti
	popfl
}

asm unsigned short
esdiinw(addr)
{
%mem addr; lab not_b1;
	movl	addr, %edx
	pushfl
	cli
#if defined(PDI_SVR42) || defined(BUG386B1)
	cmpl	$0,do386b1
	je	not_b1
	movl    %cr3, %eax
	movl    %eax, %cr3
not_b1:
#endif
	xorl	%eax, %eax
	inw	(%dx)
	sti
	popfl
}

asm void
esdioutb(addr, val)
{
%mem addr, val; lab not_b1;
	pushfl
	cli
#if defined(PDI_SVR42) || defined(BUG386B1)
	cmpl	$0,do386b1
	je	not_b1
	movl    %cr3, %eax
	movl    %eax, %cr3
not_b1:
#endif
	movl	addr, %edx
	movl	val, %eax
	outb	(%dx)
	sti
	popfl
}

asm void
esdioutw(addr, val)
{
%mem addr, val; lab not_b1;
	pushfl
	cli
#if defined(PDI_SVR42) || defined(BUG386B1)
	cmpl	$0,do386b1
	je	not_b1
	movl    %cr3, %eax
	movl    %eax, %cr3
not_b1:
#endif
	movl	addr, %edx
	movl	val, %eax
	outw	(%dx)
	sti
	popfl
}

struct cb ESDI_CB;

#ifdef DEBUG
int esdiwarning = 0;	/* So we can turn on/off warning messages */

/*
 * error messages 
 */
static char *ESDIerrmsg[] = {
	"Reserved",
	"Command Completed with Success",
	"Reserved",
	"Command Completed with ECC Applied",
	"Reserved",
	"Command Completed with Retries",
	"Format Command Partially Completed",
	"Command Completed with ECC and Retries",
	"Command Completed with Warning",
	"Abort Complete",
	"Reset Complete",
	"Data Transfer Ready",
	"Command Completed with Failure",
	"DMA Error",
	"Command Block Error",
	"Attention Error",
	"Data Transfer Interrupt Expected",
	"Data Transfer Interrupt NOT Expected",
	"Format Interrupt Expected",
	"Format Interrupt NOT Expected"
};
#endif /* DEBUG */


/* Error codes if error 0xC (failure) use error codes below */
static ushort xlaterr[]= {
	DERR_UNKNOWN,		/* Reservered */
	DERR_NOERR,		/* Success    */
	DERR_UNKNOWN,		/* Reservered */
	DERR_ERRCOR,		/* Success w/ ECC */
	DERR_UNKNOWN,		/* Reservered */
	DERR_ERRCOR,		/* Success w/ retries */
	DERR_FMTERR,		/* Partial Format */
	DERR_ERRCOR,		/* Success w/ ECC retries */
	DERR_ERRCOR,		/* Success w/ warning */
	DERR_NOERR,		/* Abort complete */
	DERR_NOERR,		/* Reset complete */
	DERR_NOERR,		/* Data Xfer ready */
	DERR_DATABAD,		/* Failure */
	DERR_OVERRUN,		/* DMA error */
	DERR_ABORT,		/* Command Block error */
	DERR_CTLERR,		/* Attention Error */
	DERR_DATABAD,		/* Int expected */
	DERR_CTLERR,		/* Int  NOT expected */
	DERR_FMTERR,		/* Format int expected */
	DERR_FMTERR		/* Format int NOT expected */
};

/* These device codes map form 0x00 - 0x18 */
static short xlat2err[] = {
	DERR_NOERR,		/* NO Error */
	DERR_SEEKERR,		/* Seek Fault */
	DERR_CTLERR,		/* Interface Fault */
	DERR_BADSECID,		/* Block Not found (couldn't locate ID )*/
	DERR_BADSECID,		/* AM Not found (Not Formatted ) */
	DERR_DATABAD,		/* Data ECC error */
	DERR_BADSECID,		/* ID ECC Error */
	DERR_PASTEND,		/* RBA Out of Range */
	DERR_TIMEOUT,		/* Time Out Error */
	DERR_DATABAD,		/* Defective Block */
	DERR_MEDCHANGE,		/* Disk Changed */
	DERR_ABORT,		/* Selection Error */
	DERR_WRTPROT,		/* Media Write Protected */
	DERR_WFAULT,		/* Write Fault */
	DERR_DATABAD,		/* Read Fault */
	DERR_SECNOTFND,		/* Bad Format */
	DERR_PASTEND,		/* Volume Overflow */
	DERR_DAMNF,		/* No Data AM found */
	DERR_BADSECID,		/* No ID AM and ID ECC */
	DERR_DRVCONFIG,		/* No Device Configuration Info */
	DERR_BADCMD,		/* Missing first and last RBA flags */
	DERR_SECNOTFND,		/* No ID's found on track */
};

/* These device codes map to 0x80 - 0x86 */
static short xlat3err[] = {
	DERR_UNKNOWN,		/* Reserved */
	DERR_NOSEEKC,		/* time out wiating for SERDES to Stop */
	DERR_TIMEOUT,		/* Time OUT waiting for Data Transfer to end */
	DERR_TIMEOUT,		/* Time Out waiting for FIFO request */
	DERR_ABORT,		/* SERDES has stopped */
	DERR_ABORT,		/* Time Out waiting for Head Switch */
	DERR_TIMEOUT,		/* Timout waiting for DMA complete */
};

/*
 * mcesdi_find_ha -- Find the controller
 *                We return the number of drives which exist on that
 *                controller (or 0 if controller doesn't respond)...
 */
/* ARGSUSED */
int
mcesdi_find_ha(struct ide_cfg_entry *config, HBA_IDATA_STRUCT *idata)
{
	int drives;
	uint	bus_p;

	MCESDI_DTRACE;

	/* if not running in a micro-channel machine, skip initialization */
	if (!drv_gethardware(IOBUS_TYPE, &bus_p) && (bus_p != BUS_MCA))
		return(0);

        config->cfg_ioaddr1 = idata->ioaddr1;
        config->cfg_ioaddr2 = 0x0;
        config->cfg_dmachan1 = idata->dmachan1;

	if (drives = mcesdi_bdinit(config)) {
                config->cfg_ata_drives = drives;
                cmn_err(CE_NOTE,"mcesdi_find_ha: found %d ATA drives at 0x%x",
                                drives,config->cfg_ioaddr1);
	}

	return drives;
}

/*
 * mcesdi_bdinit -- Initialize AT-class hard disk controller.
 *                We return the number of drives which exist on that
 *                controller (or 0 if controller doesn't respond)...
 */
/* ARGSUSED */
int
mcesdi_bdinit(struct ide_cfg_entry *config)
{
	int i;
	unsigned char status;

	/* Reset the adpater */
	esdioutb(config->cfg_ioaddr1 + ESDI_BCR, 0);
	for (i = HDTIMOUT; i > 0; i--) {
		if (!(esdiinb(config->cfg_ioaddr1 + ESDI_BSR) & (BUSY | INTPND)))
			break ;
		drv_usecwait(10);
	}
	if (i <= 0)
		return (0);
	esdioutb(config->cfg_ioaddr1 + ESDI_ATTN, ADAPTER_ID | AR_RESET);
	status = ESDIstatus(config->cfg_ioaddr1);
	if (status == (ADAPTER_ID | ID_RESET))
		ESDIstin(config->cfg_ioaddr1);
	esdioutb(config->cfg_ioaddr1 + ESDI_ATTN, ADAPTER_ID | AR_EOI);

	if (!mcfind_drive(config->cfg_ioaddr1, 0)){
		return (0);
	}

	if ((mcesdi_cb = dma_get_cb(DMA_NOSLEEP)) == NULL) {
#ifdef DEBUG
		cmn_err(CE_CONT, "mcesdi: dma_get_cb() failed");
#endif /* DEBUG */
		return (0);
	}
	if ((mcesdi_cb->targbufs = dma_get_buf(DMA_NOSLEEP)) == NULL) {
#ifdef DEBUG
		cmn_err(CE_CONT, "mcesdi: dma_get_buf() failed");
#endif /* DEBUG */
		dma_free_cb(mcesdi_cb);
		return (0);
	}
	mcesdi_cb->targ_step = DMA_STEP_HOLD;
	mcesdi_cb->targ_path = DMA_PATH_16;
	mcesdi_cb->trans_type = DMA_TRANS_SNGL;
	mcesdi_cb->targ_type = DMA_TYPE_IO;
	mcesdi_cb->bufprocess = DMA_BUF_SNGL;
	config->cfg_capab |= CCAP_BOOTABLE;

	if (config->cfg_ata_drives == 1) {
		/* don't bother to look for second drive */
		return (1);
	}

	if (mcfind_drive(config->cfg_ioaddr1, 1)) {
		return (2);	/* got second drive */
	} else  {
		/*
	         * Only first drive responded on controller 
		 * don't issue seeks on 1-drive board 
		 */
		config->cfg_capab |= CCAP_NOSEEK;
		return (1);
	}
}


/*
 * mcesdi_drvinit -- Initialize drive on controller.
 *                 Sets values in dpb for drive (based on info from controller
 *                 or ROM BIOS or defaults).
 */

int
mcesdi_drvinit(int curdrv, struct ide_cfg_entry *config)
{
        struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	int i;
	unchar status;
	unchar idrive, id;
	ulong drivesize;
	void ESDIcmdout();

	MCESDI_DTRACE;

	bcopy("IDRIVE:",parms->dpb_tag,7);
	parms->dpb_tag[7] = curdrv | 0x30;

	parms->dpb_inqdata.id_type = 0;
	parms->dpb_inqdata.id_qualif = 0;
	parms->dpb_inqdata.id_rmb = 0;
	parms->dpb_inqdata.id_ver = 0x1;		/* scsi 1 */
	parms->dpb_inqdata.id_len = 31;
	strncpy(parms->dpb_inqdata.id_vendor, "IBM     ", 8);
	strncpy(parms->dpb_inqdata.id_prod, "MCA ESDI Disk   ", 16);
	strncpy(parms->dpb_inqdata.id_revnum, "1.00", 4);

	esdioutb(config->cfg_ioaddr1 + ESDI_BCR, 0);	/* turn off interupt */

	/* set up some defaults in the dpb... */
	parms->dpb_flags=(DFLG_RETRY | DFLG_ERRCORR | DFLG_VTOCSUP|DFLG_FIXREC);
	parms->dpb_drvflags = (DPCF_NOTRKFMT | DPCF_BADMAP);
	parms->dpb_devtype = DTYP_DISK;

	/* Establish defaults for disk geometry */
	ESDI_CB.command = GET_DEV_CFG | (curdrv << DEVICE_SHIFT) | 
	    RESERVED6 | TYPE_2;
	ESDI_CB.blocks = 0;
	ESDI_CB.RBA = 0;

	ESDIwait(config->cfg_ioaddr1); /* wait for controller not busy */
	esdioutb(config->cfg_ioaddr1 + ESDI_ATTN, AR_COMMAND | (curdrv << DEVICE_SHIFT));
	/*
	 * make sure that we don't have an interrupt here already 
	 * if we have an interrupt this means that the drive doesn't exist
	 * and we shouldn't send it the rest of the command
	 */
	for (i = HDTIMOUT/500; i > 0; i--) {
		if (esdiinb(config->cfg_ioaddr1 + ESDI_BSR) & IRPT) {
			if ((status = esdiinb(config->cfg_ioaddr1 + ESDI_ISR)) & 0xf)
				return(-1);
			else
				cmn_err(CE_CONT,
				"ESDI: unexpected interrupt while initializing drive\n");
		}
		drv_usecwait(10);
	}

	ESDIcmdout(config->cfg_ioaddr1);
	status = ESDIstatus(config->cfg_ioaddr1);
	idrive = status & DEVICE_SELECT;
	id = status & INTID;
	if (((idrive >> DEVICE_SHIFT) == curdrv) &&
	    (id == ID_OK) ||
	    (id == ID_ECC) ||
	    (id == ID_RETRY) ||
	    (id == ID_ECC_RETRY) ||
	    (id == ID_WARNING)) {
		ESDIstin(config->cfg_ioaddr1);
		drivesize = ((ulong)(ESDI_SB[3]) << 16) + ESDI_SB[2];
		/*
		 * drivesize now contains the actual size of the drive in
		 * sectors.
		 * To present a standard interface at the BIOS level,
		 * IBM has decided that ESDI drives should have virtual
		 * cylinders of 2048 sectors (1 MB).  This lets them access
		 * 1-GIG drives by having only 1024 cylinders.  We will do
		 * the same in reporting drive geometry.  Note that this may
		 * leave some part of the drive (<1MB at the end) inaccessable.
		 */
		parms->dpb_cyls = drivesize / 2048;
		parms->dpb_heads = 64;
		parms->dpb_sectors = 32;
		parms->dpb_secsiz = SECTOR_SIZE;
		parms->dpb_drvflags |= (DPCF_CHGCYLS);
	}
	ESDIwait(config->cfg_ioaddr1); /* wait for controller not busy */
	esdioutb(config->cfg_ioaddr1 + ESDI_ATTN, idrive | AR_EOI);

	esdioutb(config->cfg_ioaddr1 + ESDI_BCR, INTEN);
	return(0);
}


/*
 * mcesdi_cmd -- perform command on AT-class hard disk controller.
 */

int
mcesdi_cmd(int cmd, int curdrv, struct ide_cfg_entry *config)
{
        struct ata_parm_entry *parms = &config->cfg_drive[curdrv];

	int error = 0;

	MCESDI_DTRACE;

	if (cmd == DCMD_RETRY)
		cmd = parms->dpb_command;        /* just try again */
	else /* prime for new command */
		/* save command code for later retry */
		parms->dpb_command = (ushort) cmd;

	switch (cmd) {
	case DCMD_READ:
		parms->dpb_drvcmd = READ_DATA;
		parms->dpb_state = DSTA_NORMIO;
		break;
	case DCMD_WRITE:
		parms->dpb_drvcmd = WRITE_DATA;
		parms->dpb_state = DSTA_NORMIO;
		break;
	default:
		return (SDI_TCERR);
	}
	if (mcesdi_docmd(curdrv, config) == FAILURE) {
		config->cfg_INTR(config);
	}
	return (error);
}


/*
 * mcesdi_int -- process AT-class controller interrupt.
 */

/* ARGSUSED */
struct ata_parm_entry *
mcesdi_int(struct ide_cfg_entry *config)
{
        struct ata_parm_entry *parms;
	unchar drive, id;
	unchar sys_ctrl_A;
	int i, curdrv;
	struct control_area *control;


	curdrv = config->cfg_curdriv;
	parms = &config->cfg_drive[curdrv];
	control = parms->dpb_control;

	if ((config->cfg_flags & CFLG_BUSY) == 0) {

		cmn_err(CE_WARN, "ESDI: unexpected interrupt\n");
		return(NULL);  
	}

	if (!(esdiinb(config->cfg_ioaddr1 + ESDI_BSR) & IRPT)) {
		cmn_err(CE_WARN, "ESDI: no pending interrupts\n");
		return (NULL);
	}

	esdi_chk_isr(curdrv, config, &id);

	/* get current drive */
	drive = (unchar) curdrv;
	/* turn the disk light off */
	sys_ctrl_A = inb(DISKLIGHT_REG);
	outb(DISKLIGHT_REG, sys_ctrl_A & (~(0x80 >> drive)));
        for (i = HDTIMOUT; i > 0; i--) {
                if (!(esdiinb(config->cfg_ioaddr1 + ESDI_BSR) & (BUSY)))
                        break;
                drv_usecwait(10);
        }
	esdioutb(config->cfg_ioaddr1+ESDI_ATTN, (drive << DEVICE_SHIFT) | AR_EOI);

	switch ((int)parms->dpb_state) {
	case DSTA_RECAL:
		parms->dpb_intret = DINT_COMPLETE;
		break;
	case DSTA_NORMIO:
		switch(id) {
		case ID_ECC:
			if (!(parms->dpb_flags & DFLG_ERRCORR)) {
				parms->dpb_drverror = DERR_DATABAD;
				parms->dpb_intret = DINT_ERRABORT;
				control->SCB_Stat = parms->dpb_intret;
				return (parms);
			}
			/* FALLTHRU */
		case ID_RETRY:
			if (!(parms->dpb_flags & DFLG_RETRY)) {
				parms->dpb_drverror = DERR_DATABAD;
				parms->dpb_intret = DINT_ERRABORT;
				control->SCB_Stat = parms->dpb_intret;
				return (parms);
			}
			/* FALLTHRU */
		case ID_ECC_RETRY:
			if (!(parms->dpb_flags & (DFLG_ERRCORR|DFLG_RETRY))) {
				parms->dpb_drverror = DERR_DATABAD;
				parms->dpb_intret = DINT_ERRABORT;
				control->SCB_Stat = parms->dpb_intret;
				return (parms);
			}
		}
		mcesdi_xferok(parms, parms->drq_count);
		if (parms->dpb_intret == DINT_CONTINUE)
			if (mcesdi_docmd(curdrv, config) == FAILURE) {
				config->cfg_INTR(config);
			}
		break;
	case DSTA_FORMAT:
		if (id == ID_FORMAT) {
			parms->dpb_drverror = DERR_FMTERR;
			parms->dpb_intret = DINT_ERRABORT;
		} else {
			parms->dpb_drverror = DERR_NOERR;
			parms->dpb_intret = DINT_COMPLETE;
		}
		break;
	default:
		break;
	}
	control->SCB_Stat = parms->dpb_intret;
	return (parms);  /* let caller know we did something... */
}

/*
 * mcesdi_docmd -- output a command to the controller.
 *               Generate all the bytes for the task file & send them.
 */

int
mcesdi_docmd(int curdrv, struct ide_cfg_entry *config)
{
        struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	unchar sys_ctrl_A;
	void ESDIcmdout();
	unchar	int_id;
	int ret_status = SUCCESS;

	parms->dpb_state |= DSTA_NORMIO;
	ESDI_CB.command = parms->dpb_drvcmd | (curdrv << DEVICE_SHIFT)
	    | RESERVED2 | TYPE_4;
	ESDI_CB.blocks = parms->dpb_sectcount;
	ESDI_CB.RBA = parms->dpb_cursect;

	/* Turn the disk light on */
	sys_ctrl_A = inb(DISKLIGHT_REG);
	outb(DISKLIGHT_REG, sys_ctrl_A | (0x80 >> curdrv));
	ESDIwait(config->cfg_ioaddr1); /* wait for controller not busy */
	esdioutb(config->cfg_ioaddr1 + ESDI_ATTN, (curdrv << DEVICE_SHIFT) | AR_COMMAND);
	if ((parms->dpb_drvcmd == READ_DATA) ||
	    (parms->dpb_drvcmd == WRITE_DATA)) {
		/*
		 * Disable controller interrupts and wait for the 
		 * esdi command to complete to set up the DMA.
		 */
		esdioutb(config->cfg_ioaddr1 + ESDI_BCR, 0);
		ESDIcmdout(config->cfg_ioaddr1);
		while (ret_status = esdi_chk_isr(curdrv, config, &int_id)) {
			if (int_id != ID_XFER)
				continue;
			else {
				/* setup DMA for the data transfer */
				if (ESDIdma(curdrv, config) != 0) {
					ret_status = FAILURE;
				}
				break;
			}
		}
	} else {
		ESDIcmdout(config->cfg_ioaddr1);
	}
	return ret_status;
}

/*
 * Routine to set up the DMA controller to do the requested
 * transfer from/to the disk controller.
 */

int
ESDIdma(int curdrv, struct ide_cfg_entry *config)
{
        struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	int 	channel = config->cfg_dmachan1;
	int 	rw = parms->dpb_flags & DFLG_READING;
	paddr_t	addr = parms->dpb_curaddr;
	size_t len = parms->drq_count;

	mcesdi_cb->targbufs->address = addr;
	mcesdi_cb->targbufs->count = (ushort_t)len;
	mcesdi_cb->targbufs->count_hi = (ushort_t)(len >> 16);

	dma_disable(channel);

	if (rw)	{
		mcesdi_cb->command = DMA_CMD_WRITE;
	} else {
		mcesdi_cb->command = DMA_CMD_READ;
	}

	if (dma_prog(mcesdi_cb, channel, DMA_NOSLEEP) == FALSE) {
#ifdef DEBUG
		cmn_err(CE_CONT, "mcesdi: dma_prog() failed!");
#endif /* DEBUG */
		return (1);
	}

	dma_enable(channel);

	esdioutb(config->cfg_ioaddr1 + ESDI_BCR, INTEN | DMAEN);
	return (0);
}


/*
 * Wait for controller CMDIN to be cleared.  Will wait
 * approx. 1/4 second if controller CMDIN still not cleared,
 * then will panic.
 * If cleared then output command word.
 */
void
ESDIcmdout(addr)
ushort addr;
{
	int	i, j, len;
	ushort *word;

	word = (ushort *) &ESDI_CB;
	len = (((uint)(ESDI_CB.command & TYPE) >> TYPE_SHIFT) + 1) * 2;
	for (j = 0; j < len; j++) {
		for (i = HDTIMOUT; i > 0; i--) {
			if (!(esdiinb(addr + ESDI_BSR) & CMDIN))
				break;
			drv_usecwait(10);
		}
		esdioutw((unsigned int)(addr+ESDI_CIR), (unsigned int)word[j]);
	}
}

int
mcfind_drive(ioaddr, drive)
ushort ioaddr;
int drive;
{
	int i;
	unchar status;
	unchar idrive, id;
	int mystatus = 0;

	ESDI_CB.command = GET_DEV_CFG | (drive << DEVICE_SHIFT) | 
	    RESERVED6 | TYPE_2;
	ESDI_CB.blocks = 0;
	ESDI_CB.RBA = 0;

	ESDIwait(ioaddr); /* wait for controller not busy */
	esdioutb(ioaddr + ESDI_ATTN, AR_COMMAND | (drive << DEVICE_SHIFT));

	/* make sure that we don't have an interrupt here already 
	 * if we have an interrupt this means that the drive doesn't exist
	 * and we shouldn't send it the rest of the command
	 */
	for (i = HDTIMOUT/500; i > 0; i--) {
		if (esdiinb(ioaddr + ESDI_BSR) & IRPT) {
			if( (status=esdiinb(ioaddr + ESDI_ISR)) & 0xf) {
				return (mystatus);
			} else
				cmn_err(CE_CONT,
				"ESDI unexpected interrupt while initializing drive\n");
		}
		drv_usecwait(10);
	}
	ESDIcmdout(ioaddr);
	status = ESDIstatus(ioaddr);
	idrive = status & DEVICE_SELECT;
	id = status & INTID;
	if (((idrive >> DEVICE_SHIFT) == drive) &&
	    (id == ID_OK) ||
	    (id == ID_ECC) ||
	    (id == ID_RETRY) ||
	    (id == ID_ECC_RETRY) ||
	    (id == ID_WARNING)) {
		ESDIstin(ioaddr);
		mystatus = 1;
	}
	ESDIwait(ioaddr);
	esdioutb(ioaddr + ESDI_ATTN, (drive << DEVICE_SHIFT) | AR_EOI);
	return (mystatus);
}

/*
 * Wait for controller IRPT to be set.  Will wait
 * approx. 1/4 second if controller IRPT still not set,
 * then will panic.
 * Return the value of the controller interrupt status register.
 */

unchar
ESDIstatus(addr)
ushort addr;
{
	register int		i;
	unchar	 retval;

	for (i = HDTIMOUT; i > 0; i--) {
		if ((retval = esdiinb(addr + ESDI_BSR)) & IRPT)
			return (esdiinb(addr + ESDI_ISR));
		drv_usecwait(10);
	}
	return(retval);
}


/*
 * Wait for controller BUSY and INTPND to be cleared.  Will wait
 * approx. 1/4 second if controller BUSY and INTPND still not cleared,
 * then will panic.
 */
void
ESDIwait(addr)
ushort addr;
{
	register int	i;

	for (i = HDTIMOUT; i > 0; i--) {
		if (!(esdiinb(addr + ESDI_BSR) & (BUSY | INTPND)))
			return;
		drv_usecwait(10);
	}
	cmn_err(CE_CONT, "ESDIwait timed out, BSR %x\n",
		esdiinb(addr + ESDI_BSR));
}

/*
 * Wait for controller STOUT to be set.  Will wait
 * approx. 1/4 second if controller STOUT still not set,
 * then will panic.
 * If set then input status word.
 */
void 
ESDIstin(addr)
ushort addr;
{
	int	i, j, len;
	ushort *word;

	word = ESDI_SB;
	len = 1;
	for (j = 0; j < len; j++) {
		for (i = HDTIMOUT; i > 0; i--) {
			if (esdiinb(addr + ESDI_BSR) & STOUT)
				break;
			drv_usecwait(10);
		}
		word[j] = esdiinw(addr + ESDI_SIR);
		if (j == 0) {
			len = (uint)(ESDI_SB[0] & WORD_COUNT) >> WORD_SHIFT;
		}
	}
}


/* ARGSUSED */
ushort
esdierrmsg(int curdrv, struct ide_cfg_entry *config, unchar status)
{
        struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	int id, oid;
#ifdef DEBUG
	int i;
#endif

	id = status & INTID;
	oid = id;
	if ((id != ID_XFER) && (parms->dpb_state ==  DSTA_NORMIO))
		id = 16;
	else if ((id == ID_XFER) && (parms->dpb_state != DSTA_NORMIO))
		id = 17;
	else if ((id != ID_FORMAT) && (parms->dpb_state == DSTA_FORMAT))
		id = 18;
	else if ((id == ID_FORMAT) && (parms->dpb_state != DSTA_FORMAT))
		id = 19;
#ifdef DEBUG
	if (esdiwarning)
		cmn_err(CE_CONT,
		"ESDI error: %s\n          drive %d, RBA %uld    status 0x%x\n",
		    ESDIerrmsg[id], curdrv, ESDI_CB.RBA, status);

	ESDIstin(config->cfg_ioaddr1);
	if (esdiwarning) {
		cmn_err(CE_CONT, "            status block ");
		for (i = 0; i < ((uint)(ESDI_SB[0] & WORD_COUNT)>>WORD_SHIFT); i++)
			cmn_err(CE_CONT, " 0x%x", ESDI_SB[i]);
		cmn_err(CE_CONT, "\n");
	}
#endif
	if ((oid & 0x0F) == 0xC) {	/* Command Complete with Failure */
		return (ushort)esdi2errmsg(ESDI_SB[2]);
	} else
		return xlaterr[id];
}

ushort
esdi2errmsg(ushort status)
{
	unchar id;
	id  = status & 0xff;

	if(id & 0x80)
		return xlat3err[id & 0x0f];
	else if ((id & 0xf0) == 0  || (id & 0x10) == 1)
		return xlat2err[id];
	else	/* codes 0x19 - 80  and 0x87 - 0xff are reserved */
		return DERR_UNKNOWN;
}


int
esdi_chk_isr(int curdrv, struct ide_cfg_entry *config, unchar *int_id)
{
        struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	unchar drive, id, status;
	unchar sys_ctrl_A;

	status = ESDIstatus(config->cfg_ioaddr1);
	drive = (uint)(status & DEVICE_SELECT) >> DEVICE_SHIFT;
	id = status & INTID;
	*int_id = id;

	if (((config->cfg_flags & CFLG_BUSY) == 0) || 
	    (drive != curdrv)) {
		if ((id != ID_XFER) && (id != ID_ATTN_ERROR)) {
			/* wait for controller not busy	*/
			ESDIwait(config->cfg_ioaddr1);
			esdioutb(config->cfg_ioaddr1 + ESDI_ATTN, drive | AR_EOI);
		}
	}

	if (!((id == ID_OK) ||
	    (id == ID_ECC) ||
	    (id == ID_RETRY) ||
	    (id == ID_ECC_RETRY) ||
	    (id == ID_WARNING) ||
	    ((id == ID_XFER)  && (parms->dpb_state == DSTA_NORMIO ))||
	    ((id == ID_FORMAT) && (parms->dpb_state == DSTA_FORMAT)))) {
		/* turn the disk light off */
		sys_ctrl_A = inb(DISKLIGHT_REG);
		outb(DISKLIGHT_REG, sys_ctrl_A & (~(0x80 >> drive)));
		parms->dpb_drverror = (ushort)esdierrmsg(curdrv, config, status);
		if ((id != ID_XFER) && (id != ID_ATTN_ERROR)) {
			/* wait for controller not busy	*/
			ESDIwait(config->cfg_ioaddr1);
			esdioutb(config->cfg_ioaddr1 + ESDI_ATTN, drive | AR_EOI);
		}
		parms->dpb_intret = DINT_ERRABORT;
		return (FAILURE);
	}
	return (SUCCESS);
}

/*
 * mcesdi_xferok -- used by drivers that can detect partial transfer success
 *                to indicate that some number of sectors have been moved
 *                successfully between memory and the drive.
 *
 *                This function also sets dpb_intret depending on
 *                on dpb_sectcount.
 */
void
mcesdi_xferok(struct ata_parm_entry *parms, int bytes_xfer)
{

	struct control_area *control = parms->dpb_control;
	int sec_xfer, count;

#ifdef NOT_YET
	parms->dpb_totcount += bytes_xfer;
#endif

	sec_xfer = (bytes_xfer / parms->dpb_secsiz);

	/* Was data sent?... */

	if (bytes_xfer == parms->drq_count) {
		/*
		 * We sent the last piece.  is there more?
		 */
		if ((--control->SCB_SegCnt) == 0) {
			/*
			 * DONE!
			 */
			parms->dpb_intret = DINT_COMPLETE;
			return;
		}
		/*
		 * Send the next piece
		 */
		control->SCB_SegPtr++;
	}
	count = control->SCB_SegPtr->d_cnt;
	parms->drq_count = count;
	parms->dpb_sectcount = count/parms->dpb_secsiz;
	parms->dpb_cursect += sec_xfer;
	parms->dpb_curaddr = control->SCB_SegPtr->d_addr;
	parms->dpb_virtaddr += bytes_xfer;
	parms->dpb_intret = DINT_CONTINUE;
}

/*
 * mcesdi_send_cmd -- translate and send a command to the controller.
 */
void
mcesdi_send_cmd(struct ide_cfg_entry *config, struct control_area *control)
{
	int sleepflag, curdrv = control->target;
	struct ata_parm_entry *parms = &config->cfg_drive[curdrv];
	struct  sb *sb;

	MCESDI_DTRACE;

	sleepflag = KM_NOSLEEP;

	parms->dpb_control = control;

	sb = control->c_bind;
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
				mcesdi_sb2drq(control, sleepflag);
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
				mcesdi_sb2drq(control, sleepflag);
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
}

void
mcesdi_cplerr(struct ide_cfg_entry *config, struct control_area *control)
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
 * mcesdi_getinfo -- get controller specific info...
 */

int
mcesdi_getinfo(struct hbagetinfo *getinfop)
{
	getinfop->iotype = F_DMA_32 | F_SCGTH;

	if (getinfop->bcbp) {
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = MCESDI_MAXXFER;
		getinfop->bcbp->bcb_physreqp->phys_align = MCESDI_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = MCESDI_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = MCESDI_DMASIZE;
	}
}

/*
 * mcesdi_int_enable -- enable AT-class controller interrupts.
 */
void
mcesdi_int_enable(struct ide_cfg_entry *config)
{
}
void
mcesdi_halt(struct ide_cfg_entry *cfgp)
{
}
int
mcesdi_sb2drq(struct control_area *control, int sleepflag)
{
	struct ata_ha *ha;
	gdev_cfgp config;
	struct ata_parm_entry *parms;
	struct sb *sb = control->c_bind;
	int direction, sector, count, secsiz, command;
	ulong spc;

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
	count = control->SCB_SegPtr->d_cnt;

	parms->dpb_newvaddr = 
	parms->dpb_virtaddr = (paddr_t)sb->SCB.sc_datapt;

	parms->dpb_newaddr = 
	parms->dpb_curaddr = (paddr_t)control->SCB_SegPtr->d_addr;

	parms->dpb_newcount = 
	parms->drq_count = count;

	parms->dpb_sectcount = count/secsiz;
	parms->dpb_cursect = sector;

	drv_getparm(LBOLT, &config->cfg_laststart);

	config->cfg_CMD(command, control->target, config);

	return 0;
}

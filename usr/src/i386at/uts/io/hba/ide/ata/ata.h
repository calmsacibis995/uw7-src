#ifndef _IO_HBA_IDE_ATA_ATA_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IDE_ATA_ATA_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/ata/ata.h	1.2.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * PC AT Hard disk controller definitions.
 * This file supports the stock IBM ST-506 controller (and clones), the
 * Adaptec RLL and ESDI controllers, and the WD 1005-WAH ESDI controller.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1986 INTERACTIVE Systems Corporation
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


#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif	/* _KERNEL_HEADERS */


/*
 * NOTE: the FDR mentioned below is the Fixed Disk Register (which is
 * actually in the FLOPPY controller's I/O address space.  This register's
 * address is contained in dcbp->ioaddr2.  The base address for the Task File
 * registers of the AT controller is in dcbp->ioaddr1 (where 'dcbp' points
 * at the proper disk_ctl_block.
 */

#define ATA_PRIMEADDR  0x1f0           /* Primary Controller Task File addr */
#define ATA_IOADDR2_OFF  0x200           /* offset to secondary I/O addr */

/*
 * Bit 3 of the FDR must be set to 1 to access heads
 * 8 - 15 of a hard disk on a stock AT controller.
 * Most others ignore the bit, so it's OK to set there, too...
 */

#define AT_EXTRAHDS     0x08    /* set into FDR to access high disk heads */
#define AT_NOEXTRAHDS   0x00    /* set into FDR if no high disk heads */
#define AT_INTRDISAB    0x02    /* set into FDR to disable interrupts */
#define AT_CTLRESET     0x04    /* set into FDR for 10 microsec to reset */
#define AT_INTRENABL    0x00    /* set into FDR to enable interrupts */

/*
 * port offsets from base address in dcbp->dcb_ioaddr1, above.
 */
#define	AT_DATA		0x00	/* data register */
#define	AT_ERROR	0x01	/* error register/write precomp */
#define	AT_PRECOMP	0x01	/* error register/write precomp */
#define AD_SPCLCMD      0x01    /* Adaptec special command code */
#define AT_COUNT        0x02    /* sector count */
#define	AT_SECT		0x03	/* sector number */
#define	AT_LCYL		0x04	/* cylinder low byte */
#define	AT_HCYL		0x05	/* cylinder high byte */
#define AT_DRVHD        0x06    /* drive/head register */
#define	AT_STATUS	0x07	/* status/command register */
#define	AT_CMD		0x07	/* status/command register */

/*
 * port offsets from base address in dcbp->dcb_ioaddr2, above.
 */
#define	AT_DEV_CNTL			0x06	/* device control register */
#define	AT_ALT_ATAPI_STAT	0x06	/* alternate status register */

/*
 * Status bits from AT_STATUS register
 */
#define ATS_BUSY        0x80    /* controller busy */
#define ATS_READY       0x40    /* drive ready */
#define ATS_WRFAULT     0x20    /* write fault */
#define ATS_SEEKDONE    0x10    /* seek operation complete */
#define ATS_DATARQ      0x08    /* data request */
#define ATS_ECC         0x04    /* ECC correction applied */
#define ATS_INDEX       0x02    /* disk revolution index */
#define ATS_ERROR       0x01    /* error flag */

/*
 * Drive selectors for AT_DRVHD register
 */
#define ATDH_DRIVE0     0xa0    /* or into AT_DRVHD to select drive 0 */
#define ATDH_DRIVE1     0xb0    /* or into AT_DRVHD to select drive 1 */

/*
 * Hard disk commands. 
 */
#define	ATC_RESTORE	0x10	/* restore cmd, bottom 4 bits set step rate */
#define ATC_SEEK        0x70    /* seek cmd, bottom 4 bits set step rate */
#define ATC_RDSEC       0x21    /* read sector cmd, bottom 2 bits set ECC and
					retry modes */
#define ATC_WRSEC       0x31    /* write sector cmd, bottom 2 bits set ECC and
					retry modes */
#define	ATC_FORMAT	0x50	/* format track command */
#define	ATC_RDVER	0x40	/* read verify cmd, bot. bit sets retry mode */
#define ATC_DIAG        0x90    /* diagnose command */
#define	ATC_SETPARAM	0x91	/* set parameters command */
#define ADC_SPECIAL     0xf0    /* Adaptec 'special command' (below) */
#define AWC_READPARMS   0xec    /* WD1005-WAH Read Parameters command */

/*
 * Low bits for Read/Write commands...
 */
#define ATCM_ECCRETRY   0x01    /* Enable ECC and RETRY by controller */
				/* enabled if bit is CLEARED!!! */
#define ATCM_LONGMODE   0x02    /* Use Long Mode (get/send data & ECC) */
				/* enabled if bit is SET!!! */

/*
 * Adaptec special commands -- these values are written to the AD_SPCLCMD
 * register before an AT_SPECIAL command is issued.
 */

#define ADSC_MODESEL    0x15    /* Mode Select */
#define ADSC_MODESENSE  0x1a    /* Mode Sense */
#define ADSC_RDBUF      0x1e    /* Read Data Buffer */
#define ADSC_WRBUF      0x1f    /* Write Data Buffer */
#define ADSC_RDSENSE    0x20    /* Read Sense Data */
#define ADSC_RDRES      0x21    /* Read Reserved Sectors */
#define ADSC_WRRES      0x22    /* Write Reserved Sectors */
#define ADSC_FMTRES     0x23    /* Format Reserved Track */
#define ADSC_SAVCMD     0x24    /* Save Command */
#define ADSC_RETCMD     0x25    /* Retrieve Command */
#define ADSC_UPDSTAT    0x26    /* Update Status Register */

#define AT_SECSIZE      512     /* default sector size */
#define AT_SECSHFT      9
#define AT_SECMASK      (SECSIZE-1)

/*
 * operational mode error bits
 */
#define	DAM_NOT_FOUND	0x01
#define	TR000_ERR	0x02
#define	ABORTED		0x04
#define	ID_NOT_FOUND	0x10
#define	ECC_ERR		0x40
#define	BAT_BLK		0x80

/*
 * Structure to hold disk geometry parameters.
 * Information is copied from PC AT ROM BIOS.
 */
struct ROMdiskparam {
	short	dp_ncyl;	/* number of cylinders */
	short	dp_nhead;	/* number of heads */
	short	dp_wprecmp;	/* write precomp cylinder */
	short	dp_lz;		/* landing zone cylinder */
	short	dp_nsect;	/* sectors per track */
};

/*
 * Mode Select/Sense Buffer
 */
struct  admsbuf {
	unsigned char   adms_valid;     /* Must be 0x02 to be valid sense */
	unsigned char   adms_cylh;      /* High byte of number of cyls */
	unsigned char   adms_cyll;      /* Low byte of number of cyls */
	unsigned char   adms_nhds ;     /* head count */
	unsigned char   adms_nsect;     /* sector count per track */
	unsigned char   adms_rsrvd[4];  /* future space */
	unsigned char   adms_flags;     /* see below */
	unsigned char   adms_jumps;     /* jumpers installed on board */
	unsigned char   adms_secovhd;   /* # bytes of overhead per sector */
	} ;

#define ADMS_VALID      0x02            /* to check/set adms_valid byte */
#define RLL_NSECS       26              /* number of sectors/track for RLL */

/* flags from adms_flags */

#define ADMSF_VMS       0x80    /* Valid Mode Select */
#define ADMSF_VAC       0x40    /* Valid Auto-Configuration */
#define ADMSF_SKEW      0x20    /* Skew first sector in tracks */
#define ADMSF_CIDMSK    0x07    /* Mask to get Controller ID (below) */

/* Adaptec controller ID's */

#define ADCID_IBM       0       /* Stock IBM AT controller (not adaptec) */
#define ADCID_ESDI      1       /* "Norton" ESDI controller */
#define ADCID_RLL       2       /* "Eddie" RLL controller */
#define ADCID_ARLL      3       /* "Casper" ARLL controller */

/*
 * structure of data returned from the Western Digital 'Read Parameters'
 * command...
 */
struct wdrpbuf
	{
	ushort  wdrp_config;    /* general configuration bits */
	ushort  wdrp_fixcyls;   /* # of fixed cylinders */
	ushort  wdrp_remcyls;   /* # of removable cylinders */
	ushort  wdrp_heads;     /* # of heads */
	ushort  wdrp_trksiz;    /* unformatted bytes/track */
	ushort  wdrp_secsiz;    /* unformatted bytes/sector */
	ushort  wdrp_sectors;   /* sectors/track */
	ushort  wdrp_other[249]; /* other stuff */
	};
#define WDRPBUF_LEN     256      /* # of words returned in Read Parameters */

/*
 * The following flags are for the 'dpb_drvflags' field of the disk_parm_block
 * (which is reserved for us).  We keep flags which will be returned in
 * dp_ctlflags for V_GETPARMS in the low end (where they are defined as
 * being in vtoc.h) and put our personal flags in the high end...
 */

#define ATF_FMTRST      0x8000  /* Need to do RESTORE before next FORMAT op */
#define ATF_ADAPTEC     0x4000  /* controller is Adaptec (use mode sense/select) */
#define ATF_RESTORE     0x2000  /* need to do RESTORE before next I/O op */
#define ATF_RECALDONE	0x1000  /* Indicate recal completed for next retry */

/* The two following defines are used in ata_reset to determine if the	 */
/* dpb should have the error values populated. When doing a reset in the */
/* action of sending a command NO_ERR should be used. When a command has */
/* failed and the failure needs to be passed back up SET_ERR is used.	 */
#define SET_ERR 1
#define NO_ERR 0

/*
 * the following structure is used by ata_senddata and ata_recvdata to
 * keep track of whether they had to cross a memory section boundary at an
 * odd byte count to the controller.  If so, they have to do construction of
 * a complete word to talk to the controller (which doesn't speak bytes).
 */

struct oddbyte
	{
	char    usedp;  /* non-zero if 'datab' has a byte in it */
	unchar  datab;  /* the actual data byte */
	};

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_IDE_ATA_ATA_H */

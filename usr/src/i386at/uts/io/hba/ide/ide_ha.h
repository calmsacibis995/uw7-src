#ifndef _IO_HBA_IDE_IDE_HA_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IDE_IDE_HA_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/ide_ha.h	1.6.1.1"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *    control area structure definition
 */

typedef struct control_area {

	unsigned short	c_active;	/* marks this SCB as busy	*/
	uchar_t SCB_Cmd;                   /* SCB command type   */
	uchar_t SCB_Stat;                  /* SCB command status */
	int		data_len;	/* length of data transfer	*/
	int		adapter;	/* adapter this scb is for	*/
	int		target;		/* target ID of SCSI device	*/
	paddr_t		c_addr;		/* SCB physical address		*/
	struct sb	*c_bind;	/* associated SCSI block	*/
	struct dma_vect *SCB_SegPtr;       /* Pointer to DMA vector */
	struct dma_vect c_dma_vect;	/* DMA vecter */
	struct control_area *c_next; /* pointer to next SCB struct */
	struct req_sense_def *virt_SensePtr; /* virt address of SCB_SensePtr */
	vaddr_t SCB_SenseLen;              /* Sense Length       */
	uchar_t SCB_Lun;                /* Target/Channel/LUN    */
	uchar_t SCB_SegCnt;                /* Number of segments */
	uchar_t SCB_TargStat;              /* Target status      */

} control_struct;

#define  EXEC_SCB       0x02
#define  SOFT_RST_DEV   0x04
#define  HARD_RST_DEV   0x08

/*
 * config area structure definition
 */

struct ata_parm_entry {
	char    dpb_tag[8];
	ulong   dpb_flags;      /* Drive flags (below) */
	ulong   dpb_newcount;   /* Bytes remaining in active memory section */
	ulong   drq_count;   /* Bytes remaining in active memory section */
	ulong   dpb_sectcount;  /* Sectors remaining in current disk section */
	ushort  dpb_retrycnt;   /* Number of retries performed on command */
	ushort  dpb_state;      /* Drive state (below) */
	ushort  dpb_intret;     /* What happened on last interrupt (below) */
	ushort  dpb_drvflags;   /* Drive-specific flags */
	struct control_area *dpb_control;
	ushort  dpb_command;    /* Current command being executed on drive */
	ushort  dpb_drvcmd;     /* Actual drive-level command being executed */
	ushort  dpb_drverror;   /* Last error code from drive */
	ushort  dpb_secsiz;     /* Sector size (in bytes) */
	paddr_t dpb_newaddr;    /* Active physical memory address for xfer */
	paddr_t dpb_newvaddr;   /* Active virtual memory address for xfer */
	paddr_t dpb_curaddr;    /* Current physical memory transfer address */
	paddr_t dpb_virtaddr;   /* Current virtual memory transfer address */
	ulong   dpb_cyls;       /* Number of available cylinders on drive */
	ulong   dpb_rescyls;    /* Number of reserved cylinders on drive */
	ulong   dpb_parkcyl;    /* Head-parking cyl (if necessary) */
	ulong   dpb_pdsect;     /* Default 'pdinfo' sector number */
	ushort  dpb_heads;      /* Number of heads per cylinder */
	ushort  dpb_sectors;    /* Number of available sectors per track */
	ushort  dpb_ressecs;    /* Number of reserved sectors per track */
	ushort  dpb_secovhd;    /* Sector overhead (in bytes) */
	ushort  dpb_devtype;    /* Device type indicator (below) */
	ushort  dpb_curcyl;     /* Current DISK cylinder */
	ulong	dpb_pcyls;	    /* physical cylinders; 0 if unknown */
	ulong	dpb_pheads;	    /* physical heads */
	ulong	dpb_psectors;	/* physical sectors/track */
	ulong	dpb_pbytes;	    /* physical bytes/sector */
	struct  sense dpb_reqdata;		/* request sense data */
	struct	ident dpb_inqdata;		/* inquiry data */
	daddr_t dpb_cursect;    /* Current sector number on drive */
	unchar  dpb_interleave; /* Hardware Interleave Factor used on drive */
	unchar  dpb_skew;       /* Skew factor (if known & intlv == 1, 0 otherwise */
	ulong   dpb_wpcyl;      /* Write-precomp cyl (if necessary) */
	uchar_t	dpb_atppkt[12];	/* Atapi packet */
	uchar_t	dpb_atpdrq;	/* DRQ assertion for packet acceptance */
	uchar_t dpb_atpbusywait;/* Should we busywait? */
	ulong	dpb_atptimecnt;	/* Number of timeouts called on the state */
};

struct ide_cfg_entry {
	char    cfg_tag[4];     /* Controller tag (text string for debugging) */
	ushort_t cfg_flags;     /* Controller Activity flags (below) */
	char    *cfg_name;      /* Controller Name (text string for err msgs) */
	ulong   cfg_capab;      /* Controller Capability flags (below) */
	paddr_t cfg_memaddr1;   /* Controller Memory Address (prime) */
	paddr_t cfg_memaddr2;   /* Controller Memory Address (secondary) */
	ushort  cfg_ioaddr1;    /* Controller I/O Space Address (prime) */
	ushort  cfg_ioaddr2;    /* Controller I/O Space Address (secondary) */
	ushort  cfg_dmachan1;   /* DMA channel used (prime) */
	ushort  cfg_maxsec;     /* Max # of sectors in single controller req */
	ushort  cfg_ata_drives; /* number of ATA drives on controller */
	ushort  cfg_atapi_drives; /* number of ATAPI drives on controller */
	ushort  cfg_delay;      /* Delay time for switching drives in 10us units */
	ushort  cfg_defsecsiz;  /* Default sector size for drives on this ctl */
	int     (*cfg_FHA)();	/* Initialize Board function */
	int     (*cfg_DINIT)();	/* Initialize Drive function */
	int     (*cfg_GETINFO)();/* Getinfo function */
	int     (*cfg_CMD)();   /* Perform Command function */
	void    (*cfg_SEND)();	/* send SCB to lower layer function */
	int     (*cfg_OPEN)();  /* Device Opening function (or NULL) */
	int     (*cfg_CLOSE)(); /* Device Closing function (or NULL) */
	struct ata_parm_entry * (*cfg_INTR)();  /* interrupt handler */
	void    (*cfg_CPL)();	/* completion handler */
	void    (*cfg_START)();	/* start interrupts */
	void    (*cfg_HALT)();	/* HALT routine */
	ulong   cfg_lastdriv;   /* number of last active drive on controller */
	ushort_t cfg_curdriv;   /* Current drive for this driver */
	clock_t	cfg_laststart;	/* lbolt time set at cmd start */
	struct  ata_parm_entry cfg_drive[2];
};

typedef struct ide_cfg_entry *gdev_cfgp;

/*
 * Values for dpb_intret.  This value is set by the controller-level
 * interrupt code to let the driver know what occurred during processing
 * of the last interrupt.  This drives a switch after the controller-level
 * interrupt handler to determine what to do next.
 */

#define DINT_RETRY      0       /* retry current command */
#define DINT_CONTINUE   1       /* Current controller command is continuing */
				/* No intervention by generic code is necessary. */
#define DINT_COMPLETE   2       /* Current request has completed normally */
#define DINT_GENERROR   3       /* Some general error occurred */
				/* (dpb_drverror is not accurate yet -- */
				/* generic code will request error code) */
#define DINT_ERRABORT   4       /* Current command terminated abnormally */
				/* (generic error code is in dpb_drverror) */
#define DINT_NEEDCMD    5       /* A command needs to be issued */
#define DINT_NEWSECT    6       /* Processing needs to be initiated on a new */
				/* section request. */

/* Controller Capability Flags: */
#define CCAP_MULTI      0x01    /* Multiple concurrent I/O requests supported */
#define CCAP_DMA        0x02    /* DMA supported */
#define CCAP_SCATGATH   0x04    /* Scatter-gather I/O supported */
#define CCAP_16BIT      0x08    /* Limit contiguous xfers to 64K (len & bound) */
#define CCAP_CYLLIM     0x10    /* I/O request limited to a single cylinder */
#define CCAP_CHAINSECT  0x20    /* Command chaining at sector boundaries supported */
#define CCAP_NOSEEK     0x40    /* No explicit SEEK commands should be used */
#define CCAP_RETRY      0x80    /* Controller can do automatic retries */
#define CCAP_ERRCOR     0x100   /* Controller can do error correction */
#define CCAP_SHARED     0x200   /* Controller can be shared between different */
				/* drivers (up to GDEV_SHAREMAX of them). */
				/* Used for disk & tape on same controller */
#define CCAP_PIO        0x400   /* Controller use programmed I/O transfers */
#define CCAP_BOOTABLE   0x800   /* This controller is the boot device */

/* Drive flags: */
#define DFLG_BUSY       0x01    /* Drive is currently busy */
#define DFLG_RETRY      0x02    /* Retries should be attempted on commands */
#define DFLG_ERRCORR    0x04    /* Error correction should be attempted */
#define DFLG_OPENING    0x08    /* Drive is currently being opened */
#define DFLG_VTOCOK     0x10    /* Drive has a valid pdinfo/vtoc/etc */
#define DFLG_CLOSING    0x20    /* Last partition on drive is being closed */
#define DFLG_VTOCSUP    0x40    /* VTOC/PDINFO/ALTS/etc supported on drive */
#define DFLG_REMOVE     0x80    /* Drive supports removable media */
#define DFLG_SPECIAL    0x100   /* A special (no queue entry) request in progress */
				/* (a 'wakeup' on gdev_param_block will be done at */
				/* completion of the request).  */
#define DFLG_READING    0x200   /* This drive is currently involved in a READ */
				/* request.  Used in deciding whether READ or */
				/* WRITE commands should be issued for I/O */
#define DFLG_OPEN       0x400   /* This drive is currently opened */
#define DFLG_OFFLINE    0x800   /* Drive is offline for some reason */
#define DFLG_FIXREC     0x1000  /* Drive supports fixed-length records only */
				/* Above is intended for tape (cart vs mag) */
#define DFLG_EXCLREQ    0x2000  /* Someone wants access to specific device   */
				/* We will grant access when device goes idle*/
#define DFLG_SUSPEND	0x4000	/* The queue has been suspended */
#define DFLG_TRANSLATE	0x8000	/* Drive parms have different physical and */
				/* BIOS/CMOS (heads) parms, use phys for I/O */


/* Drive states: */
#define DSTA_IDLE       0       /* Drive is idle */
#define DSTA_SEEKING    1       /* Drive is seeking */
#define DSTA_NORMIO     2       /* Drive is performing normal I/O (read/write) */
#define DSTA_RECAL      3       /* Drive is doing diagnostic recalibrate */
#define DSTA_GETERR     4       /* Drive is getting extended error code */
#define DSTA_RDVER	5	/* Drive is performing read/verify cmd */
#define DSTA_FORMAT	6	/* Drive is being formatted */

/* Device type codes: */
#define DTYP_UNKNOWN    0       /* Unknown device type */
#define DTYP_DISK       1       /* Disk type device */
#define DTYP_TAPE       2       /* Tape type device */
#define DTYP_CDROM      3       /* CD-ROM type device */
#define DTYP_ATAPI      4       /* Only known to be ATAPI */

/* Drive Commands: */
#define DCMD_READ       1       /* Read Sectors/Blocks */
#define DCMD_WRITE      2       /* Write Sectors/Blocks */
#define DCMD_FORMAT     3       /* Format Tracks */
#define DCMD_SETPARMS   4       /* Set Drive Parameters */
#define DCMD_RECAL      5       /* Recalibrate */
#define DCMD_GETERR     6       /* Get Generic Error Code */
#define DCMD_SEEK       7       /* Seek to Cylinder */
#define DCMD_RETRY      9       /* Restart current command for retry */
#define DCMD_FMTDRV     10      /* Format entire drive */
#define DCMD_REWIND     11      /* Rewind */
#define DCMD_SEOF       12      /* Skip to File Mark */
#define DCMD_UNLOAD     13      /* Unload removable medium */
#define DCMD_SERVWRT    14      /* Write Servo Data */
#define DCMD_ERASE      15      /* Erase Tape */
#define DCMD_RETENSION  16      /* Retension Tape */
#define DCMD_WFM        17      /* Write File Mark */
#define DCMD_GETPARMS   18      /* Get Parameters from device */
#define DCMD_RDVER	19	/* Read Verify sectors on disk */
#define DCMD_FMTBAD	20	/* Format Bad Track */
#define DCMD_LOAD	21	/* Load removable medium */
#define DCMD_LOCK	22	/* Lock removable medium */
#define DCMD_UNLOCK	23	/* Unlock removable medium */
#define DCMD_RESET	24	/* Reset tape device */
#define DCMD_MSENSE	25	/* Mode Sense command */
#define DCMD_REQSEN	26	/* Request Sense command */
#define DCMD_RELES	27	/* release command */

/* Controller Activity Flags: */
#define CFLG_BUSY       0x01    /* Controller is busy processing a request. */
				/* For single-thread controllers, this is */
				/* handled entirely by generic code.  For */
				/* multi-thread (CCAP_MULTI) controllers, */
				/* low-level code should clear this and call */
				/* the drv_START routine for the first driver */
				/* when the controller is available */
				/* to process another request */
#define CFLG_EXCLREQ    0x02    /* An exclusive controller request is pending */
				/* (a 'wakeup' on &gdev_ctl_block will be */
				/* done when controller goes idle) */
#define CFLG_EXCL       0x04    /* Someone has exclusive access to controller. */
				/* Do not re-grant exclusivity until released */
#define CFLG_INIT       0x08    /* We are initializing this controller */
#define CFLG_INITDONE   0x10    /* Controller initialization has been done */
				/* (intended for shared controllers) */

/*
 * Generic Device Error Numbers.  Each controller-level handler must convert
 * its own controller's error codes into these.  A standard array of these
 * numbers in a well-known order should be part of the driver's space.c
 * file to facilitate this translation.  It may not be possible to get all
 * error codes from all controller/drive combinations.  This list attempts
 * to be exhaustive.
 */

#define DERR_NOERR      0       /* No error found */
#define DERR_DAMNF      1       /* Data address mark not found */
#define DERR_TRK00      2       /* Unable to recalibrate to track 0 */
#define DERR_WFAULT     3       /* Write Fault on drive */
#define DERR_DNOTRDY    4       /* Drive is not ready */
#define DERR_CNOTRDY    5       /* Controller will not come ready */
#define DERR_NOSEEKC    6       /* Seek will not complete */
#define DERR_SEEKERR    7       /* Seek error (wrong cylinder found) */
#define DERR_NOIDX      8       /* No Index signal found */
#define DERR_WRTPROT    9       /* Medium is write-protected */
#define DERR_NODISK     10      /* Medium is not present in drive */
#define DERR_BADSECID   11      /* Error found in sector ID field */
#define DERR_SECNOTFND  12      /* Sector not found */
#define DERR_DATABAD    13      /* (uncorrectable) Error found in sector data */
#define DERR_BADMARK    14      /* Sector or track was marked bad */
#define DERR_FMTERR     15      /* Error during Format operation */
#define DERR_BADCMD     16      /* Illegal/erroneous command */
#define DERR_CTLERR     17      /* Controller error or failure */
#define DERR_ABORT      18      /* Command aborted with no apparent cause */
#define DERR_SEEKING    19      /* Drive is still seeking (try again later) */
#define DERR_MEDCHANGE  20      /* Medium has been changed in drive */
#define DERR_PASTEND    21      /* I/O past end of drive */
#define DERR_OVERRUN    22      /* Data overrun */
#define DERR_TIMEOUT    23      /* Command timeout */
#define DERR_DRVCONFIG  24      /* Unable to get valid drive configuration */
#define DERR_UNKNOWN    25      /* Undetermined error */
#define DERR_EOF        26      /* Found EOF on Read or EOM on Write */
#define DERR_ERRCOR     27      /* Correctable data error occurred */
#define DERR_ATAPI      28      /* ATAPI error, request sense valid */


#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_IDE_IDE_HA_H */

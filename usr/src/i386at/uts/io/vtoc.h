#ifndef _IO_VTOC_H	/* wrapper symbol for kernel use */
#define _IO_VTOC_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/vtoc.h	1.14.5.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define V_NUMPAR 	16		/* The number of partitions */
#define HDPDLOC		29		/* hard disk PDinfo sector */
#define VTOC_SEC	HDPDLOC		/* VTOC sector number on disk */


#define VTOC_SANE		0x600DDEEE	/* Indicates a sane VTOC */
#define V_VERSION_1	0x01            /* version 1 layout */
#define V_VERSION_2	0x02            /* version 2 layout */
#define V_VERSION_3	0x03            /* version 3 layout */
#define V_VERSION_4	0x04            /* version 4 layout */
#define V_VERSION_5	0x05            /* version 5 layout */
#define V_VERSION_DIVVY	0x06            /* OpenServer layout */
#define V_VERSION	V_VERSION_5     /* layout version number */

/*
 * V_VERSION HISTORY:
 *      1  --   Initial release
 *      2  --   Added V_REMAP flag to p_flag field so that alternate sector
 *              mapping could be enabled/disabled on a per-partition basis.
 *              Driver turns this bit on for all partitions (except 0) for
 *              a version 1 VTOC.
 *              (INTERACTIVE UNIX release 1.1, DLP)
 * 	3  --	Extended the PDINFO to include physical characteristics of
 *		devices, when known.  Allows us to take advantage of, for
 *		example, manufacturer's defect lists on disks.
 *		(INTERACTIVE UNIX release 2.2, DSR)
 *	4  --   version 2 & 3 changes.
 *	5  --   Added vtoc extension for supporting more than 16 slices
 */
#define XPDVERS		3		/* 1st version of extended pdinfo */
#define	XVTOCVERS	V_VERSION_5	/* 1st version of extended vtoc */


/* Partition identification tags */
#define V_UNUSED	0x00		/* Unused slice */
#define V_BOOT		0x01		/* Boot slice */
#define V_ROOT		0x02		/* Root filesystem */
#define V_SWAP		0x03		/* Swap filesystem */
#define V_USR		0x04		/* Usr filesystem */
#define V_BACKUP	0x05		/* full disk */
#define V_ALTS          0x06            /* alternate sector space */
#define V_OTHER         0x07            /* non-unix space */
#define V_ALTTRK	0x08		/* alternate track space */
#define V_ALTSOSR5	0x08		/* V_ALTTRK is not really used anywhere
					 * so we will redefine it to OSR5.
					 */
#define V_STAND		0x09		/* Stand slice */
#define V_VAR		0x0a		/* Var slice */
#define V_HOME		0x0b		/* Home slice */
#define V_DUMP		0x0c		/* dump slice */
#define V_ALTSCTR	0x0d		/* alternate sector slice */
#define V_MANAGED_1	0x0e		/* Volume management public slice */
#define V_MANAGED_2	0x0f		/* Volume management private slice */

/* Partition permission flags */
#define V_UNMNT		0x01		/* Unmountable partition */
#define V_RONLY		0x10		/* Read only */
#define V_REMAP         0x20            /* do alternate sector mapping */
#define V_OPEN          0x100           /* Partition open (for driver use) */
#define V_VALID         0x200           /* Partition is valid to use */
#define V_VOMASK        0x300           /* mask for open and valid */

/* driver ioctl() commands */
#define VIOC		('V'<<8)
#define V_CONFIG        (VIOC|1)        /* Configure Drive */
#define V_REMOUNT       (VIOC|2)        /* Remount Drive */
#define V_ADDBAD        (VIOC|3)        /* Add Bad Sector */
#define V_GETPARMS      (VIOC|4)        /* Get drive/partition parameters */
#define V_FORMAT        (VIOC|5)        /* Format track(s) */
#define	V_PDLOC		(VIOC|6)	/* Ask driver where pdinfo is on disk */
#define	V_GETERR	(VIOC|7)	/* Get last error */
#define V_EXERR		(VIOC|8)	/* Save extended errors */
#define V_NOEXERR	(VIOC|9)	/* Don't save extended errors (def) */
#define V_RDABS		(VIOC|10)	/* Read a sector at an absolute addr */
#define V_WRABS		(VIOC|11)	/* Write a sector to absolute addr */
#define V_VERIFY	(VIOC|12)	/* Read verify sector(s)           */
#define V_XFORMAT	(VIOC|13)	/* Selectively mark sectors as bad */
#define V_EXERRCTL      (VIOC|14)       /* X-Error Control (arg is ON/OFF) */
#define V_XGETPARMS     (VIOC|15)       /* NEW Get drv/partition parameters */
#define V_RETRYCTL      (VIOC|20)       /* Retry control (arg is ON/OFF) */
#define V_DEFECTS       (VIOC|21)       /* Get defect list from drive */
#define V_FMTDRV        (VIOC|22)       /* Format entire drive (NOTE: This */
                                        /* is only valid if DPCF_NOTRKFMT */
                                        /* is set in dp_drvflags of V_GETPARMS*/
                                        /* data for partition 0 of drive) */

/* SCSI driver ioctl() commands */
#define V_PREAD		(VIOC|14)	/* Physical Read */
#define V_PWRITE	(VIOC|15)	/* Physical Write */
#define V_PDREAD	(VIOC|16)	/* Read of Physical Description Area */
#define V_PDWRITE	(VIOC|17)	/* Write of Physical Description Area */

#define	V_READ_VTOC	(VIOC|23)	/* Read Extended VTOC */
#define	V_WRITE_VTOC	(VIOC|24)	/* Write Extended VTOC */

#define V_READ_PDINFO	(VIOC|25)	/* Read pdinfo */
#define V_WRITE_PDINFO	(VIOC|26)	/* Write pdinfo */

/* SCSI ioctl() error return codes */
#define V_BADREAD		0x01
#define V_BADWRITE		0x02

/* BOOT code soft-VTOC status codes for vtoc_state field */
#define VTOC_HARD		0x00
#define VTOC_SOFT		0x01

#ifdef MERGE386

/* Floppy driver ioctl() commands for MERGE386 */
#define V_SETPARMS	(VIOC|128)	/* Set floppy parameters */
#define V_GETFLOPSTAT	(VIOC|129)	/* Get floppy status */
#define V_RESET		(VIOC|130)	/* Reset floppy status */

/* Status structure returned by V_GETFLOPSTAT: */
struct flop_status {
	unsigned char	fs_lastopst;	/* Status of last operation */
	unsigned char	fs_changeline;	/* 1 if disk was "changed"
					 *	(i.e. door has been opened)
					 * since driver open or last V_RESET
					 */
};

/* Values for fs_lastopst: */
#define FD_NO_ERR	0xE0
#define BAD_CMD		0x01
#define BAD_ADDR_MARK	0x02
#define RECORD_NOT_FND	0x04
#define BAD_SECTOR	0x0A
#define DATA_CORRECTED	0x11
#define BAD_SEEK	0x40
#define NOT_RDY		0xAA
#define UNDEF_ERR	0xBB

#endif /* MERGE386 */

/* Sanity word for the physical description area */
#define VALID_PD		0xCA5E600D

struct partition {
	ushort_t p_tag;			/* ID tag of partition */
	ushort_t p_flag;		/* permision flags */
	daddr_t p_start;		/* start sector no of partition */
	long p_size;			/* # of blocks in partition */
};

/*
 * Version 3 driver internal partition structure.  The extra field is
 * determined at open time and allows us to use MS-DOS extended partitions,
 * which themselves have sub-partitions.
 */
struct xpartition {
	ushort_t p_tag;		/* ID tag of partition */
	ushort_t p_flag;	/* permision flags */
	daddr_t p_start;        /* physical start sector no of partition */
	long p_size;            /* # of physical sectors in partition */
	ulong_t p_type;		/* partition type from fdisk entry */
};

struct vtoc {
	ulong_t v_sanity;			/* to verify vtoc sanity */
	ulong_t v_version;			/* layout version */
	char v_volume[8];			/* volume name */
	ushort_t v_nparts;			/* number of partitions */
	ushort_t v_pad;				/* pad for 286 compiler */
	ulong_t v_reserved[10];			/* free space */
	struct partition v_part[V_NUMPAR];	/* partition headers */
	time_t timestamp[V_NUMPAR];		/* SCSI time stamp */
};

/*
 * vtoc_ext_hdr is located after the vtoc in VTOC_SEC (sector 29):
 *	VTOC_SEC:
 *		struct pdinfo
 *		struct vtoc
 *		struct vtoc_ext_hdr
 *
 * In order to preserve compatibility, the following:
 * 	sizeof(struct pdinfo) +
 *		sizeof(struct vtoc) +
 *		sizeof(struct vtoc_ext_hdr) <= the smallest sector size (VTOC_SMALLEST_SECTOR)
 * must always be TRUE
 */
struct vtoc_ext_hdr {
	ulong_t 	ve_sanity;	/* to verify vtoc extension sanity */
	ushort_t	ve_nslices;
	ushort_t	ve_nsects;	/* sectors in vtoc extension */
	daddr_t		ve_firstsect;	/* first sector of extension */
	uchar_t		ve_fill[12];	
};

struct vtoc_ext_trailer	{
	ushort_t	ve_firstslice;	/* first slice in this sector */
	ushort_t	ve_nslices;	/* number of slices in this sector */
	ulong_t		ve_cksum;	/* checksum of information in this
						sector */
};

#define	VTOC_SMALLEST_SECTOR	512

#define	VE_SLICES_PER_SECT	\
	((VTOC_SMALLEST_SECTOR - sizeof(struct vtoc_ext_trailer)) / sizeof(struct partition))

/*
 * The vtoc extension will be located in the 4 sectors (30-33) following the
 * VTOC_SEC sector (29) of the boot slice, each sector with the format:
 */
struct vtoc_ext_sect {
	struct partition	ve_slices[VE_SLICES_PER_SECT];
	struct vtoc_ext_trailer	ve_trailer;
};

/*
 * The maximum number of slices supported is given by the maximum number of
 * sectors available and the smallest sector size (VTOC_SMALLEST_SECTOR)
 */
#define	VE_FIRSTSECT	(VTOC_SEC + 1)	/* start with sector 30 of boot slice */
#define	VE_MAXSECTS	4		/* use sectors 30-33 */
#define	VE_MAXSLICES	(VE_MAXSECTS * VE_SLICES_PER_SECT)
#define	V_NUMSLICES	(V_NUMPAR + VE_MAXSLICES)

/*
 * VE_NSECTS gives the number of sectors needed to hold extnslices
 */
#define	VE_NSECTS(extnslices)	\
	(((extnslices) + VE_SLICES_PER_SECT - 1) / VE_SLICES_PER_SECT)


/*
 * This structure is used to pass extended VTOC information between PDI
 * commands and sd01 driver for V_EXT_VTOC_WRITE and V_EXT_VTOC_READ ioctls
 */
struct vtoc_ioctl {
	int v_nslices;				/* number of slices */
	struct partition v_slices[V_NUMSLICES];	/* slice headers */
};


/*
 * This structure will hold the merged in-core copy of the VTOC and vtoc
 * extension
 */
struct vtoc_incore {
	ulong_t v_sanity;			/* to verify vtoc sanity */
	ulong_t v_version;			/* layout version */
	ushort_t v_nparts;			/* number of partitions */
	struct partition *v_part;		/* partition headers */
	time_t *timestamp;			/* SCSI time stamp */
};


struct pdinfo	{
	unsigned long driveid;		/*identifies the device type*/
	unsigned long sanity;		/*verifies device sanity*/
	unsigned long version;		/*version number*/
	char serial[12];		/*serial number of the device*/
	unsigned long cyls;		/*number of cylinders per drive*/
	unsigned long tracks;		/*number tracks per cylinder*/
	unsigned long sectors;		/*number sectors per track*/
	unsigned long bytes;		/*number of bytes per sector*/
	unsigned long logicalst;	/*sector address of logical sector 0*/
	unsigned long errlogst;		/*sector address of error log area*/
	unsigned long errlogsz;		/*size in bytes of error log area*/
	unsigned long mfgst;		/*sector address of mfg. defect info*/
	unsigned long mfgsz;		/*size in bytes of mfg. defect info*/
	unsigned long defectst;		/*sector address of the defect map*/
	unsigned long defectsz;		/*size in bytes of defect map*/
	unsigned long relno;		/*number of relocation areas*/
	unsigned long relst;		/*sector address of relocation area*/
	unsigned long relsz;		/*size in sectors of relocation area*/
	unsigned long relnext;		/*address of next avail reloc sector*/
/* the previous items are left intact from AT&T's 3b2 pdinfo.  Following
   are added for the 80386 port */
	unsigned long vtoc_ptr;         /*byte offset of vtoc block*/
	unsigned short vtoc_len;        /*byte length of vtoc block*/
	unsigned short vtoc_pad;        /* pad for 16-bit machine alignment */
	unsigned long alt_ptr;          /*byte offset of alternates table*/
	unsigned short alt_len;         /*byte length of alternates table*/
		/* new in version 3 */
	unsigned long pcyls;		/*physical cylinders per drive*/
	unsigned long ptracks;		/*physical tracks per cylinder*/
	unsigned long psectors;		/*physical sectors per track*/
	unsigned long pbytes;		/*physical bytes per sector*/
	unsigned long secovhd;		/*sector overhead bytes per sector*/
	unsigned short interleave;	/*interleave factor*/
	unsigned short skew;		/*skew factor*/
	unsigned long pad[8];		/*space for more stuff*/
};

/*
 * Convenient macros to allow backward compatibility.  These require only
 * that the pdinfo structure's version field be set correctly.
 */
#define PDINFO_SIZE(pdp) (sizeof(struct pdinfo) -		\
			        ((pdp)->version<XPDVERS?sizeof(long)*14:0))
#define getpcyls(pdp)	     ((pdp)->version<XPDVERS?0:(pdp)->pcyls)
#define getptracks(pdp)	     ((pdp)->version<XPDVERS?0:(pdp)->ptracks)
#define getpsectors(pdp)     ((pdp)->version<XPDVERS?0:(pdp)->psectors)
#define getpbytes(pdp)	     ((pdp)->version<XPDVERS?0:(pdp)->pbytes)
#define getsecovhd(pdp)	     ((pdp)->version<XPDVERS?0:(pdp)->secovhd)
#define getinterleave(pdp)   ((pdp)->version<XPDVERS?(pdp)->relnext:(pdp)->interleave)
#define getskew(pdp)	     ((pdp)->version<XPDVERS?0:(pdp)->skew)

#define setpcyls(pdp,x)	     ((pdp)->version<XPDVERS?(x):((pdp)->pcyls=(x)))
#define setptracks(pdp,x)    ((pdp)->version<XPDVERS?(x):((pdp)->ptracks=(x)))
#define setpsectors(pdp,x)   ((pdp)->version<XPDVERS?(x):((pdp)->psectors=(x)))
#define setpbytes(pdp,x)     ((pdp)->version<XPDVERS?(x):((pdp)->pbytes=(x)))
#define setsecovhd(pdp,x)    ((pdp)->version<XPDVERS?(x):((pdp)->secovhd=(x)))
#define setinterleave(pdp,x) ((pdp)->version<XPDVERS?((pdp)->relnext=(x)):((pdp)->interleave=(x)))
#define setskew(pdp,x)	     ((pdp)->version<XPDVERS?(x):((pdp)->skew=(x)))

union   io_arg {
	struct  {
		ushort_t ncyl;		/* number of cylinders on drive */
		uchar_t nhead;		/* number of heads/cyl */
		uchar_t nsec;		/* number of sectors/track */
		ushort_t secsiz;        /* number of bytes/sector */
		} ia_cd;                /* used for Configure Drive cmd */
	struct  {
		ushort_t flags;		/* flags (see below) */
		daddr_t bad_sector;     /* absolute sector number */
		daddr_t new_sector;     /* RETURNED alternate sect assigned */
		} ia_abs;               /* used for Add Bad Sector cmd */
	struct  {
		ushort_t start_trk;	/* first track # */
		ushort_t num_trks;	/* number of tracks to format */
		ushort_t intlv;		/* interleave factor */
		} ia_fmt;               /* used for Format Tracks cmd */
	struct	{
		ushort_t start_trk;	/* first track	*/
		char    *intlv_tbl;	/* interleave table */
		} ia_xfmt;		/* used for the V_XFORMAT ioctl */
        ushort  ia_fmtdrv_intlv;        /* used for Format Drive command */
        struct  {
                ushort_t head;		/* Head number */
                ushort_t ndef;		/* RETURNED number of defects on head */
                struct v_defect *defs;  /* pointer at defects buffer */
                } ia_def;               /* used for Get Defect List cmd */
};

/*
 * Data structure for the V_VERIFY ioctl
 */
union	vfy_io	{
	struct	{
		daddr_t abs_sec;	/* absolute sector number        */
		ushort_t num_sec;	/* number of sectors to verify   */
		ushort_t time_flg;	/* flag to indicate time the operation */
		}vfy_in;
	struct	{
		clock_t deltatime;	/* duration of operation */
		ushort_t err_code;	/* reason for failure    */
		}vfy_out;
};

/* Flags for Add Bad Sector command */
#define V_ABS_NEAR      1       /* Assign closest alternate available */

/* int-style arg for V_RETRYCTL */
#define V_RETRY_ON      1L      /* Turn ECC and Retries ON (default) */
#define V_RETRY_OFF     2L      /* Turn ECC and Retries OFF */


/* data structure returned by the Get Parameters ioctl: */

struct  disk_parms {
	char    dp_type;                /* Disk type (see below) */
	unchar  dp_heads;               /* Number of heads */
	ulong   dp_cyls;                /* Number of cylinders */
	unchar  dp_sectors;             /* Number of sectors/track */
	ushort  dp_secsiz;              /* Number of bytes/sector */
					/* for this partition: */
	ushort  dp_ptag;                /* Partition tag */
	ushort  dp_pflag;               /* Partition flag */
        union   {                       /* value depends on partition # */
                struct  {               /* returned for partition 0: */
                uint dp0_secovhd   : 8; /* Controller's per-sector overhead */
                uint dp0_rsrvdcyls : 3; /* # of reserved cylinders from total */
                uint dp0_intlv     : 4; /* Interleave factor (0 if unknown) */
                uint dp0_skew      : 3; /* Sector skew factor (per head) */
                                        /* Meaningful only if dp0_intlv == 1 */
                                        /* If intlv == 1 & skew == 0, unknown */
                uint dp0_ctlflags  : 14; /* Controller-specific flags */
                        } dp0_ctl;
                daddr_t dp1_pstartsec;  /* returned for any other partition */
                } dp_psense;
	daddr_t dp_pnumsec;             /* Number of sectors */
	};

/* to make getting at things in the union (above) make some kind of sense */
#define dp_pstartsec    dp_psense.dp1_pstartsec
#define dp_rsrvdcyls    dp_psense.dp0_ctl.dp0_rsrvdcyls
#define dp_ctlflags     dp_psense.dp0_ctl.dp0_ctlflags
#define dp_secovhd      dp_psense.dp0_ctl.dp0_secovhd
#define dp_interleave   dp_psense.dp0_ctl.dp0_intlv
#define dp_skew         dp_psense.dp0_ctl.dp0_skew

/* data structure returned by the NEW Get Parameters ioctl: */

struct  disk_xparms {
	char	dpx_type;		/* Disk type (see below) */
	ulong	dpx_heads;		/* Number of heads */
	ulong	dpx_cyls;		/* Number of cylinders */
	ulong	dpx_sectors;		/* Number of sectors/track */
	ulong	dpx_secsiz;		/* Number of bytes/sector */
					/* for this partition: */
	ushort	dpx_ptag;		/* Partition tag */
	ushort	dpx_pflag;		/* Partition flag */
	union   {                       /* value depends on partition # */
		struct  {               /* returned for partition 0: */
		ulong dp0_secovhd;	/* Controller's per-sector overhead */
		ulong dp0_rsrvdcyls;	/* # of reserved cylinders from total */
		ulong dp0_intlv;	/* Interleave factor (0 if unknown) */
		ulong dp0_skew;		/* Sector skew factor (per head) */
					/* Meaningful only if dp0_intlv == 1 */
					/* If intlv == 1 & skew == 0, unknown */
		ulong dp0_ctlflags;	/* Controller-specific flags */
		ulong dp0_pheads;	/* Physical heads */
		ulong dp0_pcyls;	/* Physical cylinders */
		ulong dp0_psectors;	/* Physical sectors/track */
		ulong dp0_psecsiz;	/* Physical bytes/sector */
			} dp0_ctl;
		daddr_t dp1_pstartsec;	/* returned for any other partition */
		} dpx_psense;
	daddr_t dpx_pnumsec;		/* Number of sectors */
};

/* to make getting at things in the union (above) make some kind of sense */

#define dpx_pstartsec	dpx_psense.dp1_pstartsec
#define dpx_rsrvdcyls	dpx_psense.dp0_ctl.dp0_rsrvdcyls
#define dpx_ctlflags	dpx_psense.dp0_ctl.dp0_ctlflags
#define dpx_secovhd	dpx_psense.dp0_ctl.dp0_secovhd
#define dpx_interleave	dpx_psense.dp0_ctl.dp0_intlv
#define dpx_skew	dpx_psense.dp0_ctl.dp0_skew
#define dpx_pcyls	dpx_psense.dp0_ctl.dp0_pcyls
#define dpx_pheads	dpx_psense.dp0_ctl.dp0_pheads
#define dpx_psectors	dpx_psense.dp0_ctl.dp0_psectors
#define dpx_psecsiz	dpx_psense.dp0_ctl.dp0_psecsiz


/* Disk types for disk_parms.dp_type: */
#define DPT_WINI        1               /* Winchester disk */
#define DPT_FLOPPY      2               /* Floppy */
#define DPT_OTHER       3               /* Other type of disk */
#define DPT_NOTDISK     0               /* Not a disk device */
#define DPT_SCSI_HD	4               /* SCSI hard disk device */
#define DPT_SCSI_OD	5               /* SCSI optical disk device */
#define DPT_SCSI_FD	6               /* SCSI floppy drive device */
#define DPT_ESDI_HD	0x11            /* ESDI hard disk device */

/* For partition 0 (which always starts at physical sector 0) various
 * other information is encoded in the dp0_ctl structure which is used
 * instead of dpx_pstartsec.  The value of dp0_rsrvdcyls should be
 * SUBTRACTED from dp_cyls before any calculations are done.
 * The flags bits are genericized below. */

/* Values for dp_ctlflags: */
#define DPCF_NOTRKFMT   0x02    /* Controller cannot format individual tracks
				   Only the entire drive can be formatted */
#define DPCF_BADMAP     0x04    /* Bad sector map is available from drive */
#define DPCF_CHGHEADS   0x08    /* Legal to change the number of heads */
#define DPCF_CHGSECTS   0x10    /* Legal to change the number of sectors/track */
#define DPCF_CHGCYLS    0x20    /* Legal to change the number of cylinders */
#define DPCF_CHGSECSIZ  0x40    /* Legal to change the sector size */
#define DPCF_NOALTS     0x80    /* No software alternates support */

/* Data structure for V_RDABS/V_WRABS ioctl's */
struct absio {
	daddr_t	abs_sec;		/* Absolute sector number (from 0) */
	char	*abs_buf;		/* Sector buffer */
};

/* Data structure for SCSI physical read/write ioctl's */
struct phyio {
	int retval;			/* Return value			*/
	unsigned long sectst;		/* Sector address		*/
	unsigned long memaddr;		/* Buffer address		*/
	unsigned long datasz;		/* Transfer size in bytes	*/
};

#define EDVTOC_LOCK	"/etc/scsi/.pdimkdev_hint"

#ifdef IOCTL_ERROR
/* Errors which may be retrieved with an ioctl when IOCTL_ERROR is used.  */

/* Error message types */

#define	FD_NOARGS	0		/* No arguments are applicable */
#define	FD_TRKERR	1		/* Track number is applicable */
#define	FD_BLKERR	2		/* Block number is applicable */

#define	FD_ENOERROR	0		/* No error */
#define	FD_ECMDTIMEOUT	1		/* command timeout */
#define	FD_ESTATIMEOUT	2		/* status timeout */
#define	FD_EBUSY	3		/* busy */
#define	FD_EMISSDADDR	4		/* Missing data address mark */
#define	FD_EBADCYL	5		/* Cylinder marked bad */
#define	FD_EWRONGCYL	6		/* Seek error (wrong cylinder) */
#define	FD_ECANTREAD	7		/* Uncorrectable data read */
#define	FD_EBADSECTOR	8		/* Sector marked bad */
#define	FD_EMISSHADDR	9		/* Missing header address mark */
#define	FD_EWRITEPROT	10		/* Write protected */
#define	FD_ESECNOTFND	11		/* Sector not found */
#define	FD_EDATAOVRUN	12		/* Data overrun */
#define	FD_EHCANTREAD	13		/* Header read error */
#define	FD_ILLSECT	14		/* Illegal sector */
#define	FD_EDOOROPEN	15		/* Door open */

typedef	struct lasterr_t {
	char	number;
	char	type;
	union	{
		int	trk;
		int	blk;
	} arg1;
} lasterr_t;
#endif /* IOCTL_ERROR */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_VTOC_H */

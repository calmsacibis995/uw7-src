#ifndef _IO_TARGET_SD01_SD01_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SD01_SD01_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sd01/sd01.h	1.46.13.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/buf.h>			/* REQUIRED */
#include <io/elog.h>			/* REQUIRED */
#include <io/target/altsctr.h>		/* REQUIRED */
#include <io/target/scsi.h>		/* REQUIRED */
#include <io/target/sdi/sdi.h>		/* REQUIRED */
#include <io/target/sdi/sdi_hier.h>	/* REQUIRED */
#include <io/target/sdi/sdi_layer.h>	/* REQUIRED */
#include <io/vtoc.h>			/* REQUIRED */

#include <io/metdisk.h>			/* REQUIRED */
#include <util/ksynch.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/buf.h>			/* REQUIRED */
#include <sys/elog.h>			/* REQUIRED */
#include <sys/altsctr.h>		/* REQUIRED */
#include <sys/scsi.h>			/* REQUIRED */
#include <sys/sdi.h>			/* REQUIRED */
#include <sys/sdi_hier.h>		/* REQUIRED */
#include <sys/sdi_layer.h>		/* REQUIRED */
#include <sys/vtoc.h>			/* REQUIRED */

#include <sys/metdisk.h>		/* REQUIRED */
#include <sys/ksynch.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* 
 * Job data structure for each active job.
 */

struct job {
	struct job	*j_next;	/* Next job in the work queue 	*/
	struct disk	*j_dk;		/* Physical device to be accessed*/
	struct job	*j_priv;	/* private pointer for dynamic */
					/* alloc routines DON'T modify it */
	struct sb	*j_cont;	/* SCB for this job 		*/
	void		(*j_done)();	/* Function to call when done 	*/
	buf_t		*j_bp;		/* Pointer to buffer header	*/
	union sc {			/* SCSI command block 		*/
		struct scs cs;
		struct scm cm;
	} j_cmd;		
	struct job	*j_fwmate;	/* next subjob			*/
	struct job	*j_bkmate;	/* previous subjob		*/
	daddr_t		j_daddr;	/* disk sector #		*/
	caddr_t		j_memaddr;	/* memory address		*/
	uint_t		j_seccnt;	/* sector count			*/
	int		j_flags;	/* job flags			*/
	long		j_error;	/* Error for this job 		*/
};

/* job flags definition */
#define J_BADBLK        0x0001  /* job has had ECC problems previously   */
#define J_ROF           0x0002  /* remap on failure to hardware reassign */
#define J_RETRY         0x0004  /* retry job after remap/reassign        */
#define J_DATINVAL      0x0008  /* data is invalid for this job          */
#define J_OREM          0x0010  /* job is old remanant                   */
#define J_FIFO          0x0020	/* job is to be queued on fifo queue	 */
#define J_HDEECC        0x0040	/* job faulted with unrecoverable ECC	 */
#define J_HDEREC        0x0080	/* job faulted with marginal ECC	 */
#define J_HDEBADBLK     0x00C0	/* Mask for HDE errors			 */
#define J_NOERR		0x0100	/* don't log errors for this job */
#define J_TIMEOUT	0x0200	/* Job was timed out - do not free 	*/
#define J_FREEONLY	0x0400	/* biodone() already done		*/
#define J_IN_GAUNTLET	0x0800  /* Job is currently in reset gauntlet	*/
#define J_DONE_GAUNTLET 0x1000  /* Job has completed reset gauntlet	*/


#define sjq_memaddr(X)  (caddr_t)((X)->j_memaddr)
#define set_sjq_memaddr(X,VAL) (X)->j_memaddr = (caddr_t)(VAL)
#define sjq_daddr(X)    (daddr_t)((X)->j_daddr)
#define set_sjq_daddr(X,VAL) (X)->j_daddr = (daddr_t)(VAL)

/*
 * Define for Reassign Blocks defect list size.
 */

#define RABLKSSZ	8	/* Defect list in bytes		*/

/*
 * Define for Read Capacity data size.
 */

#define RDCAPSZ 	8	/* Length of data area		*/

/*
 * Defines for Mode sense data command.
 */

#define FPGSZ 		0x1C	/* Length of page 3 data area	*/
#define RPGSZ 		0x18	/* Length of page 4 data area	*/
#define	SENSE_PLH_SZ	4	/* Length of page header	*/

/*  
 * Define the Read Capacity Data Header format.
 */

typedef struct capacity {
	int cd_addr;		/* Logical Block Address	*/
	int cd_len;		/* Block Length			*/
} CAPACITY_T;

/*
 *  Define the Mode Sense Parameter List Header format.
 */

typedef struct sense_plh {
	uchar_t 	plh_len;	/* Data Length			*/
	uchar_t 	plh_type;	/* Medium Type			*/
	uint_t  	plh_res : 7;	/* Reserved			*/
	uint_t  	plh_wp : 1;	/* Write Protect		*/
	uchar_t 	plh_bdl;	/* Block Descriptor Length	*/
} SENSE_PLH_T;

/*  
 * Define the Direct Access Device Format Parameter Page format.
 */

typedef struct dadf {
	int pg_pc	: 6;	/* Page Code			*/
	int pg_res1	: 2;	/* Reserved			*/
	uchar_t pg_len;		/* Page Length			*/
	int pg_trk_z	: 16;	/* Tracks per Zone		*/
	int pg_asec_z	: 16;	/* Alternate Sectors per Zone	*/
	int pg_atrk_z	: 16;	/* Alternate Tracks per Zone	*/
	int pg_atrk_v	: 16;	/* Alternate Tracks per Volume	*/
	int pg_sec_t	: 16;	/* Sectors per Track		*/
	int pg_bytes_s	: 16;	/* Bytes per Physical Sector	*/
	int pg_intl	: 16;	/* Interleave Field		*/
	int pg_trkskew	: 16;	/* Track Skew Factor		*/
	int pg_cylskew	: 16;	/* Cylinder Skew Factor		*/
	int pg_res2	: 27;	/* Reserved			*/
	int pg_ins	: 1;	/* Inhibit Save			*/
	int pg_surf	: 1;	/* Allocate Surface Sectors	*/
	int pg_rmb	: 1;	/* Removable			*/
	int pg_hsec	: 1;	/* Hard Sector Formatting	*/
	int pg_ssec	: 1;	/* Soft Sector Formatting	*/
} DADF_T;

/*  
 * Define the Rigid Disk Drive Geometry Parameter Page format.
 */

typedef struct rddg {
	int pg_pc	: 6;	/* Page Code			 */
	int pg_res1	: 2;	/* Reserved			 */
	uchar_t pg_len;		/* Page Length			 */
	int pg_cylu	: 16;	/* Number of Cylinders (Upper)	 */
	uchar_t pg_cyll;		/* Number of Cylinders (Lower)	 */
	uchar_t pg_head;		/* Number of Heads		 */
	int pg_wrpcompu	: 16;	/* Write Precompensation (Upper) */
	uchar_t pg_wrpcompl;	/* Write Precompensation (Lower) */
	int pg_redwrcur	: 24;	/* Reduced Write Current	 */
	int pg_drstep	: 16;	/* Drive Step Rate		 */
	int pg_landu	: 16;	/* Landing Zone Cylinder (Upper) */
	uchar_t pg_landl;	/* Landing Zone Cylinder (Lower) */
	int pg_res2	: 24;	/* Reserved			 */
} RDDG_T;

/*
 * Logical Sector conversion parameters
 */
struct sect_shift {
	uint_t		sect_byt_shft;	/* Byte <-> Sector shift count	*/
	uint_t		sect_blk_shft;	/* Block <-> Sector shift count	*/
	uint_t		sect_blk_mask;	/* Block to Sector boundry mask	*/
};

/* Minor number macros. */

#define	DMSIZE(dm)	(dm->dm_parms.dp_cyls * \
			dm->dm_parms.dp_heads * \
			dm->dm_parms.dp_sectors)

#define	DKSIZE(dk)	(dk->dk_dm->dm_parms.dp_cyls * \
			dk->dk_dm->dm_parms.dp_heads * \
			dk->dk_dm->dm_parms.dp_sectors)

#define	DK_MAX_PATHS        512

/* The gauntlet structure holds variables per disk for use by
 * the reset gauntlet only.
 */
struct gauntlet {
	/* The dk and jp for the job in the gauntlet can be obtained
	 * via g_sb
	 */
	struct sb    *g_sb;
	int           g_state;
	void        (*g_int)();     /* original interrupt handler */
	struct sb    *g_req_sb;	    /* Request Sense */
	struct sb    *g_tur_sb;	    /* Test Unit Ready */
	struct sb    *g_dev_sb;     /* Bus Device Reset */
	struct sb    *g_bus_sb;     /* Bus Reset */
	struct scs    g_scs;
	struct job   *g_job;
};

/* defines for g_state */

#define GAUNTLET_START	01
#define DEVICE_RESET	02
#define BUS_RESET	04
#define GAUNTLET_COMPLETE	010

/*
 * The disk_media structure holds information for a disk media (physical disk).
 * Each disk_media structure may be referenced by multiple disk paths.
 */
struct disk_media {
	long dm_refcount;		/* Reference count		*/
	long dm_state;			/* State of this disk media	*/
	char dm_iotype;			/* Drive capability (DMA/PIO)	*/
	struct disk *dm_fo_dp;		/* Disk path used on first-open	*/
	struct disk *dm_res_dp;		/* Disk path used on reserve	*/
	struct sect_shift dm_sect;	/* Blk <-> Sec conversion values*/
	struct disk_parms dm_parms;	/* Current drive configuration	*/
	struct ident dm_ident;	        /* Inquiry Data structure	*/
	struct pdinfo dm_pdinfo;	/* Physical descriptor information */
	struct pd_stamp dm_stamp;	/* Disk stamp */
	lock_t *dm_lock;		/* Disk_media read/write spin lock */
	pl_t dm_pl;			/* Ipl for dm_lock              */
	sv_t *dm_sv;			/* Sync var for dm		*/

/* Fields for bad block handling: begin */
	struct sb *dm_bbhmblk;		/* SCB for reassigning blocks	*/
	struct sb *dm_bbhblk;		/* SCB for read/write bbh icmd	*/
	char *dm_dl_data;		/* Defect List data 		*/
	char *dm_datap;			/* Bad block data area		*/
	int  *dm_altcount;     		/* # of alts for partition 	*/
	struct alt_ent **dm_firstalt; 	/* 1st alt for partition */
	struct alts_parttbl dm_alts_parttbl;/* alternate partition table*/
	daddr_t	dm_hdesec;		/* Bad sector #			*/
	buf_t  *dm_bbhbuf;		/* Buffer header for bbh icmd	*/
	struct job *dm_bbhjob;		/* Job struct for bbh icmds	*/
	struct alts_mempart *dm_amp;	/* Incore alts partition info	*/
	struct alts_mempart *dm_wkamp;	/* Working cpy alts part info	*/
	struct alt_info *dm_alttbl;	/* AT&T alternate sector table	*/
	struct altsectbl *dm_ast;	/* Ptr to working cpy of alttbl	*/
	struct alts_ent *dm_gbadsec_p;	/* Growing bad sector entry ptr	*/
	int dm_gbadsec_cnt;		/* # of growing bad sector	*/
	struct alts_ent dm_gbadsec;	/* Growing bad sector entry	*/
	int (*dm_remapalts)();		/* Function for software remap 	*/
	struct disk *dm_waitp;		/* List of waiting devices	*/
        struct job *dm_jp_orig;		/* Original job which entered bbhndlr */
        struct sd01_bbhque *dm_bbhque;	/* Bad block jobs queued on dk  */
/* Fields for bad block handling: end */

/* Fields to be moved to VTOC driver: begin */
	long dm_hdestate;		/* State of hard disk errors	*/
	/*
	 * dm_part_flag is mutex'd by sd01_dp_mutex and the dm_lock for write.
	 * for read, either is sufficient.
	 */
	long dm_openCount;		/* Number of opens on the disk */
	int dm_nslices;	/* Number of slices */
	int	dm_v_version;
	struct partition  *dm_partition;
	sdi_signature_t dm_signature;	/* Disk signature */
/* Fields to be moved to VTOC driver: end */
};
#define DM_FIND_BIG	1		/* search for the largest entry	*/

/* State flags for disk media */
#define	DMRESERVE   0X0400		/* Disk media is currently reserved */
#define	DMRESDEVICE 0X0800		/* Disk media has been reserved */
#define DMCONFLICT  0X4000		/* Reservation Confict on Open 	*/
#define DMPARMS     0X10000		/* Drive parameters set & valid	*/
#define DMFDISK     0X20000		/* Fdisk table read and valid	*/
#define DMMAPBAD    0X80000		/* Dynamic bad block mapping 	*/
#define DM_FO_LC    0X2000000		/* First open/last close	*/

/* State values for each slice  */
#define DMFREE 0			/* The slice is not in use 	*/
#define DMONLY 1			/* Slice is open for exclusive 	*/
#define DMGEN  2			/* Slice is open for general use */
#define DMMNT  0X100			/* Slice opened for Mounted FS 	*/
#define DMSWP  0X200			/* Slice opened for Swapping Device*/
#define DMBLK  0X400			/* Slice opened Buffered I/O 	*/
#define DMCHR  0X800			/* Slice opened for Char I/O 	*/
#define DMLYR  0X10000			/* Inc/dec the Driver open count*/
/* The upper 16 bits are reserved for Driver opens */
/* See matching set of define's in mirror.h        */

/*
 * The disk structure holds information for a disk path.
 */
struct disk {
	int marked;		        /* TRUE/FALSE for disclaiming disks */
	struct disk_media *dk_dm;	/* Disk media for the disk path	*/
	struct job **dk_timeout_list;   /* Array of buckets.  Jobs in any
					 * one bucket all timeout at the
					 * same time.
					 */
	struct job **dk_timeout_snapshot; /* Snapshot of above timeout list.
					* Simplifies timeout processing/locking.
					*/
	struct job *dk_oldjobs;		/* Very old jobs not yet timed-out */
	int dk_last_asw;		/* LBOLT time of last SDI_ASW return */
	int dk_time;		        /* Current time mod SD01_TIMEOUT */
	int dk_timeout_id;		/* Timeout id returned by sdi_timeout */
	struct job *dk_queue;		/* 1st entry on work queue 	*/
	struct job *dk_lastlow;		/* Last low-priority entry on queue */
	struct job *dk_lasthi;		/* Last hi-priority entry on queue  */
	long dk_state;			/* State of this disk 		*/
	long dk_count;			/* Number of jobs on work que 	*/
	long dk_outcnt;			/* Jobs in HAD for this disk 	*/
	long dk_error;			/* Number of errors detected 	*/
	long dk_sendid;			/* Timeout id for sd01send 	*/
	bcb_t *dk_bcbp;			/* Drive breakup control block	*/
	bcb_t *dk_bcbp_max;		/* Drive breakup control block  */
	time_t dk_start;		/* When the disk became active 	*/
	struct scsi_ad dk_addr;		/* Major/Minor number of device */
	struct dev_spec *dk_spec;	/* Device helper structure	*/
	struct sb *dk_fltreq;		/* SCSI block for request sense */
	struct sb *dk_fltres;		/* SCSI block for reserve job 	*/
	struct sb *dk_fltsus;		/* SCSI block for suspend job 	*/
	struct sb *dk_fltfqblk;		/* SFB for flushing disk req Q	*/
	struct disk *dk_fltnext;	/* Next disk in RESUME list 	*/
	struct gauntlet dk_gauntlet;	/* For reset gauntlet use only  */
	long dk_rescnt;			/* Number of RESUME Bus Resets 	*/
	long dk_spcount;		/* Retry count for special jobs */
	struct scs dk_fltcmd;		/* Request Sense/Reserve command*/
	struct sense *dk_sense;		/* Request Sense data 		*/
	char *dk_rc_data;               /* Read Capacity data           */
        char *dk_ms_data;               /* Mode Sense or Select data    */
 	met_disk_stats_t *dk_stat;	/* Performance data for SAR/ESMP*/
	struct job *dk_fltque;		/* Fault jobs queued		*/
	lock_t	*dk_lock;		/* Disk struct read/write spin lock */
	pl_t	dk_pl;			/* Ipl for dk_lock              */
	ushort_t *dk_timeout_table;	/* List of timeout values */
	struct sdi_device *dk_sdi_device;
};

/* State flags for disk path */
#define DKSUSP	    0X0001		/* The HAD susupended the que	*/
#define DKDRAIN	    0X0002		/* The work que has filled up	*/
#define DKSEND	    0X0004		/* sd01send has requested timeout*/
#define DKINIT	    0X0010		/* The disk has been initialized.*/
#define DKDIR	    0X0020		/* Direction flag for elevator 	*/
#define DKEL_OFF    0X0040		/* Elevator off flag 		*/
#define DKTSMD      0X0100		/* Updating TS for mirrored part*/
#define	DKFLT       0X0200		/* Disk is recovering from a fault*/
#define	DKONRESQ    0X1000		/* Disk on the Resume Queue 	*/
#define	DKPENDRES   0X2000		/* Disk has a pending Resume 	*/
#define DKSUSP_WAIT 0X40000		/* waiting for hba Q suspend	*/
#define DKFLUSHQ    0X100000		/* flush disk pending queue	*/
#define DKWANTQ     0X400000		/* want to access job queue	*/
#define DKPENDQ     0X800000		/* waiting on the pending queue */
#define DKTIMEOUT   0x1000000	        /* disk timeouts are enabled */
#define DK_RDWR_ERR 0X4000000		/* error in read/write command	*/
#define DKINSANE    0x8000000	        /* disk insane timeouts enabled	*/

/* State flags for bad block handling */
#define HDERECERR   0X100000		/* Hit marginal bad block	*/
#define HDEECCERR   0X200000		/* Hit actual bad block		*/
#define HDEBADBLK   0XF00000		/* Bad block type mask	 	*/
#define HDEENOSPR   0X010000		/* Error no spare sectors 	*/
#define HDESMASK    0X00FFFF		/* Bad block command state mask	*/
#define HDESINIT    0X000000		/* Initial state		*/
#define HDESI       0X000001		/* Reassign command failed	*/
#define HDESII      0X000002		/* Reassign command passed	*/
#define HDESIII     0X000003		/* Write command failed		*/
#define HDESIV      0X000004		/* Write command passed		*/
#define HDEHWMASK   0XF000000		/* HW reassignment mask		*/
#define HDEHWREA    0X1000000		/* HW reassignment in progress	*/
#define HDEREAERR   0X2000000		/* HW reassignment error	*/
#define HDESWMAP    0X4000000		/* SW remap in progress		*/

#define HDEREMAP_0    0X000000		/* 1st write command completed	*/
#define HDEREMAP_1    0X000001		/* 2nd write command completed	*/
#define HDEREMAP_2    0X000002		/* 3rd write command completed	*/
#define HDEREMAP_DONE 0X000003		/* Remap complete		*/
#define HDEREMAP_RSTR 0X000004          /* Restore data to alternate    */
                                        /*   good sector (optional)     */
/*	Control flags for bad block handling- BBH			*/
/*	- Sd01bbh_flg defined in space.c				*/
#define BBH_DYNOFF	0x0001		/* disable dynamic BBH remap 	*/
#define BBH_RDERR	0x0002		/* ret err on rd req after BBH 	*/

/* Define indices for bad block messages */
#define HDEECCMSG  0			/* ECC corection needed		*/
#define HDESACRED  1			/* Marginal in sacred area	*/
#define HDENOSPAR  2			/* No spares for marginal block */
#define HDEBADMAP  3			/* Reassign failed on marginal 	*/
#define HDEMAPBLK  4			/* Alternate for marginal 	*/
#define HDEBADWRT  5			/* Write of data failed		*/
#define HDEBSACRD  6			/* Bad block in sacred area	*/
#define HDEREASGN  7			/* Alternate for bad block 	*/
#define HDEBADRED  8			/* Read of marginal failed	*/
#define HDEBNOSPR  9			/* No spares for bad block	*/
#define HDEBNOMAP  10			/* Reassign failed on bad block	*/
#define HDENOINIT  11			/* Write of zeros failed 	*/
#define HDESWALTS  12			/* software remap success	*/
#define HDEHWREAS  13			/* reassign success		*/

struct resume {
	struct disk *res_head;		/* Next disk to use the RESUME SB*/
	struct disk *res_tail;		/* Last disk to use the RESUME SB*/
};

struct free_jobs {			/* List of free job structures	*/
	int fj_state;			/* -1 if waiting for jobs 	*/
	int fj_count;			/* Number free jobs 		*/
	struct job *fj_ptr;		/* Pointer to free list 	*/
};

/* fj_state values. */
#define FJEMPTY -1			/* The free list was empty 	*/
#define FJOK 0				/* The free list is ok 		*/
#define FJHIGH 8			/* The high water mark 		*/

/* Error codes for disk log records. */
#define SFB_ERR 1			/* Function request error 	*/
#define DR_ERR  2			/* Internal driver error 	*/
#define HA_ERR  3			/* Host Adapter error 		*/
#define IO_ERR  4			/* Read/Write error 		*/

/* DR_ERR extended error codes. */
#define TYPE_ERR 1			/* Bad type field was detected 	*/
#define JOB_ERR  2			/* Bad job pointer was detected */

/* Space file declarations. */
extern struct dev_spec *sd01_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg SD01_dev_cfg[];	/* configurable devices struct	*/
extern int SD01_dev_cfg_size;		/* number of dev_cfg entries	*/
extern major_t Sd01_Identity;

/*
 * indexed by instance number.  mutexed by sdi_rinit sleep lock
 * and sd01_dp_mutex.  must get both.
 */
extern struct disk *sd01_dp[];		/* Array of disk structure ptrs	*/
extern short Sd01diskinfo[];		/* Flag to set disk parameters	*/
extern int Sd01_boot_time;		/* flag for boot kernel	or not */
extern int Sd01log_marg;		/* Flag to log marginal blocks	*/
extern char Sd01_debug[];		/* Array of debug levels	*/
extern unsigned int Sd01bbh_flg;	/* control flag for BBH		*/

/* mutexed by sdi_rinit sleep lock */
extern int sd01_diskcnt;		/* Number of disks defined 	*/
extern int sd01_jobcnt;			/* Number of allocated jobs 	*/

extern int sd01_sync;
extern int sd01_timeout;
extern int sd01_insane;

/*
 * Logical Sector conversion macros
 */
#define BLKSZ(dk)	((int)(dk)->dk_dm->dm_parms.dp_secsiz)
#define BLKSHF(dk) 	((int)(dk)->dk_dm->dm_sect.sect_byt_shft)
#define BLKSEC(dk)	((int)(dk)->dk_dm->dm_sect.sect_blk_shft)
#define BLKMSK(dk)	((int)(dk)->dk_dm->dm_sect.sect_blk_mask)

#define KBLKSZ		512	/* Conversion values for a Kernel block */
#define KBLKSHF		9
#define KBLKSEC		0

#define PDBLKNO		29		/* PD sector address 		*/
#define FDBLKNO		0		/* fdisk table block address 	*/
#define VTBLKNO		PDBLKNO		/* VTOC sector address 		*/
#define JTIME		10000		/* Ten seconds for a job 	*/
#define LATER		20000		/* How much later when retrying */

#define SD01_RETRYCNT	100		/* Retry count			*/
#define SD01_RST_ERR (SD01_RETRYCNT-1)  /* Reset error retry count 	*/
#define SD01_MAXSIZE	0xF000		/* Maximum job size 		*/
#define SD01_OUTSZ_LO	3		/* Min number of job maintained in
					 * the HBA for this device 	*/
#define SD01_OUTSZ_BRK	2		/* breakoff point if sync job
					 * has been completed		*/
#define SD01_OUTSZ_HI	8		/* Max number of job maintained in
					 * the HBA for this device 	*/

#define SD01_KERNEL	0		/* The buffer is in kerenl space*/
#define SD01_USER	1		/* The buffer is in user space 	*/

#define SD01_DEBUGFLG	73		 /* Turn debugs on/off 		*/

/*
 * Flag definitions for sd01gen_sjq()
 */
#define SD01_NORMAL	0
#define SD01_IMMED	1
#define SD01_FIFO	2

/*
 * Bad Block queue definition
 */
struct sd01_bbhque {
        struct sd01_bbhque *bbh_next;   /* Next bad block on queue      */
                                        /* (dm->dm_bbhque is head of queue)*/
        struct job *bbh_jp;		/* Bad block job                */
	char	*bbh_priv;		/* private ptr for dynamic alloc*/
        uint_t	bbh_cond;		/* Condition of bad block job   */
        char    *bbh_datap;             /* Data ptr if restore is needed*/
	daddr_t	bbh_hdesec;		/* Block that is bad		*/
};

/* Definitions for sd01_bbhndlr() conditions */
#define SD01_ECCRDWR    1       /* Unrecoverable error on read/write - no */
                                /*   previous history of bad block        */
#define SD01_ECCDWR     2       /* Unrecoverable error occurred previously*/
                                /*   so now process the reassign/remap    */
#define SD01_ECCDRD     3       /* Unrecoveralbe error occurred previously*/
                                /*   but this read succeeded, so reassign */
                                /*   or remap and recover the data.       */
#define SD01_REENTER    4       /* Reentry after bad block processing     */

/* Definitions for sd01_bbremap() conditions */
#define SD01_ADDINTRM   1       /* add interim alternate entry  */
#define SD01_RMINTRM    2       /* remove interim alternate entry       */
#define SD01_REMAP      3       /* do software remap                    */
#define SD01_ADDBAD     4       /* add bad entries from sd01addbad      */
#define SD01_REMAPEND   5       /* end of processing entry              */

#define	SD01_DM_LOCK(dm)	dm->dm_pl = LOCK(dm->dm_lock, pldisk)
#define	SD01_DM_UNLOCK(dm)	UNLOCK(dm->dm_lock, dm->dm_pl)
#define	SD01_DK_LOCK(dk)	dk->dk_pl = LOCK(dk->dk_lock, pldisk)
#define	SD01_DK_UNLOCK(dk)	UNLOCK(dk->dk_lock, dk->dk_pl)

#ifdef SD01_DEBUG
#define HWREA   0x00001000
#define DXALT   0x00002000
#define DCLR    0x00004000
#define DKADDR	0x00008000
#define DKD	0x00010000
#define DBM	0x00020000
#define DSAR	0x00040000
#define STFIN   0x80000000
#endif /* SD01_DEBUG */

#ifdef DEBUG
#define DPR(l)          if (Sd01_debug[l]) printf
#endif

/*
 * Prototypes
 */

int 	sd01alt_badsec(struct disk *, struct job *, ushort_t, int),
    	sd01getalts(struct disk *),
	sd01qresume(struct disk *);
void sd01_bbhdwrea(struct job *),
     sd01_bbhndlr(struct disk *, struct job *, int, char *, daddr_t),
     sd01flts(struct disk *, struct job *),
     sd01logerr(struct sb *, struct disk *, int),
     sd01setalts_idx(struct disk *),
     sd01watchdog(int),
     sd01_gauntlet(struct sb *, int);

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SD01_SD01_H */

#ifndef _IO_TARGET_SP01_SP01_H	/* wrapper symbol for kernel use */
#define _IO_TARGET_SP01_SP01_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/target/sp01/sp01.h	1.3.1.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/target/scsi.h>	/* REQUIRED */
#include <io/target/sdi/sdi.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>		/* REQUIRED */
#include <sys/sdi.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


/*
 * the SCSI ID_PROCESSOR minor device number is interpreted as follows:
 *
 *     bits:
 *	 15         4 0
 * 	+-----------+-+
 * 	|   node    | |
 * 	+-----------+-+
 *
 */

#define SP01_MAX_NODES	4		/* MAX number of nodes supported*/

#define SP_MINORS_PER	16		/* Minors per device		*/
#define NODE(X)		(X)>>4		/* Node number from minor num.  */
#define SP01_DINFO	0x01		/* Directory info valid		*/
#define SP01_RW_MAGIC	0xABCDABCD	/* Magic number in directory	*/
#define SP01_MAX_RWB	4		/* Max number of local directories */
					/* (one per channel)		*/
#define SP01_RWBUF_SLOT_SIZE 1024	/* Default slot size		*/

/*
 * Structure defining the Node directory entry info in the read buffer.
 */
struct sp01_rwbuf_dir_entry {
	unsigned int rwd_magic;		/* Valid entry indicator	*/
	unsigned int rwd_flags;		/* flags: ASSIGNED, etc.	*/
	unsigned int rwd_id;		/* id of slot assignment	*/
	unsigned int rwd_offset;	/* Slot offset			*/
	unsigned int rwd_size;		/* Slot size			*/
};

/*
 * rwd_flags
 */
#define	RWD_ASSIGNED	0x0001		/* Directory entry assigned	*/
#define	RWD_WRCOMP	0x0002		/* Write since last read	*/

/*
 * Structure defining read/write buf directories
 */
struct sp01_rwbuf_dir {
	struct sp01_rwbuf_dir_entry	sp01_rwdir[SP01_MAX_NODES];
};

/*
 * Structure defining the local node read/write buf directories
 * for internal bookkeeping.
 */
struct sp01_rwd_def {
	int	rwdd_ctl;		/* ctl number of directory	*/
	int	rwdd_chan;		/* channel number of directory	*/
	int	rwdd_flags;		/* flags			*/
	struct scsi_ad rwdd_addr;	/* SCSI addr of local node	*/
	struct sp01_rwbuf_dir *rwdd_dirp; /* pointer to the directory	*/
};

/*
 * rwdd_flags
 */
#define	RWDD_UPDATED	0x0001		/* Directory updated and not written */

/*
 * Misc defines 
 */
#define SM_READ_BUF	SM_RDDB		/* SCSI READ BUFFER command	*/
#define SM_WRITE_BUF	SM_WRDB		/* SCSI WRITE BUFFER command	*/

#define BLKSIZE  512			/* default block size */
#define BLKMASK  (BLKSIZE-1)

#define JTIME	10000			/* ten sec for a job */
#define LATER   20000

#define MAXPEND   2
#define MAXRETRY  2

#define INQ_SZ		98		/* inquiry data size		*/
#define	RDSTATUS_SZ	10		/* playback status &		*/
					/*  subcode Q address data size	*/

/* Values of p_state */
#define	SP_INIT		0x01		/* Device has been initialized  */
#define	SP_INIT2	0x02		/* Device has been initialized  */
#define	SP_WOPEN	0x04		/* Waiting for 1st open	 */
#define	SP_DIRECTION	0x08		/* Elevator direction flag	 */
#define	SP_SUSP		0x10		/* Device Q suspended by HA	 */
#define	SP_SEND		0x20		/* Send requested timeout	 */
#define	SP_PARMS	0x40		/* Device parms set and valid	 */
#define	SP_LOCALNODE	0x80		/* Device is local node	 */

/*
 * Job structure
 */
struct job {
	struct job       *j_next;	   /* Next job on queue		 */
	struct job       *j_prev;	   /* Previous job on queue	 */
	struct job	 *j_priv;	   /* private pointer for dynamic  */
					   /* alloc routines DON'T USE IT  */
	struct sb	 *j_cont;	   /* Next job block of request  */
	struct sb        *j_sb;		   /* SCSI block for this job	 */
	struct buf       *j_bp;		   /* Pointer to buffer header	 */
	struct sp01_proc *j_pcp;	   /* Device to be accessed	 */
	unsigned	  j_errcnt;	   /* Error count (for recovery) */ 
	daddr_t		  j_addr;	   /* Physical block address	 */
	union sc {			   /* SCSI command block	 */
		struct scs  ss;		   /*	group 0 (6 bytes)	*/
		struct scm  scm;	   /*	group 1 (10 bytes)	*/
		struct scv  sv;		   /*	group 6 (10 bytes)	*/
		struct rwb_scm  sm;	   /*	group 1 (10 bytes)	*/
	} j_cmd;
};

/*
 * processor information structure
 */
struct sp01_proc {
	struct job       *p_queue;	    /* 1st entry on work queue	 */
	struct job       *p_lastlow;	    /* Last low-priority entry on queue */
	struct job       *p_lasthi;	    /* Last hi-priority entry on queue	*/
	struct scsi_ad	  p_addr;	    /* SCSI address		 */
	struct scsi_ad	  p_mbi_addr;	    /* SCSI address (mbi)	 */
	struct scsi_adr	  p_saddr;	    /* SCSI address (new form)	 */
	unsigned int	  p_ha_chan_id;	    /* SCSI ID of channel	 */	
	unsigned long	  p_sendid;	    /* Timeout id for send	 */
	unsigned  	  p_state;	    /* Operational state	 */ 
	unsigned  	  p_count;	    /* Number of jobs on Q	 */ 
	unsigned 	  p_npend;	    /* Number of jobs sent 	 */ 
	unsigned	  p_fltcnt;	    /* Retry cnt for fault jobs	 */
	struct job       *p_fltjob;	    /* Job associated with fault */
	struct sb        *p_fltreq;	    /* SB for request sense	 */
	struct sb        *p_fltres;	    /* SB for resume job	 */
	struct scs	  p_fltcmd;	    /* Request Sense command	 */
	struct sense	  *p_sense;	    /* Request Sense data	 */
	struct dev_spec  *p_spec;
	uint_t		  p_tot_rwbufsiz;   /* Total Read/Write BUFFER size*/
	uint_t		  p_mbo_rwbufsize;  /* OUT Read/Write BUFFER node size*/
	uint_t		  p_mbo_rwblksize;  /* OUT Read/Write BUFFER node size*/
	uint_t		  p_mbo_rwoffset;   /* OUT Read/Write BUFFER node offst*/
	uint_t		  p_mbi_rwbufsize;  /* IN Read/Write BUFFER node size*/
	uint_t		  p_mbi_rwblksize;  /* IN Read/Write BUFFER node size*/
	uint_t		  p_mbi_rwoffset;   /* IN Read/Write BUFFER node offset*/
	struct sp01_rwbuf_dir_entry *p_mbi_dp; /* IN directory entry	 */
	bcb_t		 *p_bcbp;	    /* Breakup control block     */
	char		  p_iotype;	    /* io device capability	 */
	unsigned int	  p_ex_iotype;	    /* extended io device capability */
	lock_t           *p_lock;
	pl_t             p_pl;
	sv_t             *p_sv;
};

extern struct dev_spec *sp01_dev_spec[];/* pointers to helper structs	*/
extern struct dev_cfg SP01_dev_cfg[];	/* configurable devices struct	*/
extern int SP01_dev_cfg_size;		/* number of dev_cfg entries	*/

#define PIOC		('P' << 8)
/* Group 0 commands */
#define	P_TESTUNIT	(PIOC | 0x01)	/* test unit ready	      (0x00) */
#define	P_INQUIR	(PIOC | 0x02)	/* inquiry		      (0x12) */
#define	P_TOT_RWBUFSIZ	(PIOC | 0x03)	/* Total RW buffer size	      	     */
#define	P_IN_RWBUFSIZ	(PIOC | 0x04)	/* Input RW buffer slot size	     */
#define	P_OUT_RWBUFSIZ	(PIOC | 0x05)	/* Output RW buffer slot size	     */

/* Group 1 commands */
#define	P_RWBUFCAP	(PIOC | 0x09)	/* read capacity	      (0x3B) */

/* arg of the P_INQUIRY ioctl */
struct sp01_proc_inq {
	unsigned short length;	/* The length of the required sense data*/
	char		*addr;	/* First address of the space where the	*/
				/* inquiry data is stored */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TARGET_SP01_SP01_H */

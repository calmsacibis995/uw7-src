#ifndef _IO_HBA_AD1542_H	/* wrapper symbol for kernel use */
#define _IO_HBA_AD1542_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/adsc/adsc.h	1.30.2.1"
#ident	"$Header$"

#if defined(__cplusplus)
extern "C" {
#endif

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

#ifdef	_KERNEL_HEADERS

#include <io/target/scsi.h>		/* REQUIRED */
#include <io/target/sdi/sdi_hier.h>	/* REQUIRED */
#ifndef PDI_SVR42
#include <util/ksynch.h>		/* REQUIRED */
#endif
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/scsi.h>			/* REQUIRED */
#include <sys/sdi_hier.h>		/* REQUIRED */
#ifndef PDI_SVR42
#include <sys/ksynch.h>			/* REQUIRED */
#endif
#include <sys/types.h>			/* REQUIRED */

#endif	/* _KERNEL_HEADERS */

#define HBA_PREFIX	adsc

/*  parameters of this implementation  */

#define MAX_CTLS	4		/* Max controllers		*/
#define MAX_EQ	 (MAX_TCS * MAX_LUS)	/* Max equipage per controller	*/
#define NDMA		20		/* Number of DMA lists		*/
/*
 * The NMBX variable was set to 50 before.  This is nuts.  No way could
 * you fill that many CCB's with the kernel command latency that is currently
 * present.  16 allows a more reasonable approach to the real number we would
 * like to have.  (i.e. 2 commands per target ID)
 */
#define NMBX		16		/* Number of mailbox pairs	*/
#define ADSC_SCSI_ID	7		/* Default ID of controllers	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	ONE_MIN		60000

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

struct scsi_cfg {
	unsigned int	ha_id;		/* SCSI identifier 		*/
	unsigned int	ivect;		/* Interrupt vector number	*/
	unsigned long	io_base;	/* I/O base address		*/
};

/* offsets from base */
#define HA_CNTL		0x00		/* Control register		*/
#define HA_CMD		0x01		/* Command /data (out) register	*/
#define HA_ISR		0x02		/* Interrupt status register	*/
#define HA_STAT		0x00		/* Hardware status register	*/
#define HA_DATA		0x01		/* Data (in) register		*/

/* ctl reg bits */
#define HA_SBR		0x10		/* Reset the SCSI bus		*/
#define	HA_IACK		0x20		/* Acknowledge interrupt	*/
#define HA_RST		0x40		/* Host adapter (soft) reset	*/
#define HA_HRST		0x80		/* Host adapter (hard) reset	*/

/* cmd reg codes */
#define	HA_INIT		0x01		/* Mail box initialization	*/
#define	HA_CKMAIL	0x02		/* Check outgoing mbx		*/
#define	HA_INQ		0x04		/* Perform Adapter Inquiry	*/
#define HA_BONT         0x07            /* Set Bus-On Time              */
#define HA_BOFFT        0x08            /* Set Bus-Off Time             */
#define HA_XFERS        0x09            /* Set Transfer Speed           */
#define HA_RXBIOS       0x28            /* Return Extended BIOS info    */
#define HA_MBXENB       0x29            /* Set Mailbox Interface Enable */

/* intr reg bits */
#define HA_INTR		0x80		/* Interrupt pending service   	*/

/* stat reg bits */
#define HA_INVDCMD	0x01		/* Invalid Host Adapter command */
#define HA_DIPFULL	0x04		/* Data in port full		*/
#define HA_CMDFULL	0x08		/* Command port full		*/
#define HA_READY	0x10		/* Command port ready		*/
#define HA_IREQD	0x20		/* Initialization required 	*/
#define HA_DIAGF	0x40		/* Diagnostic failure		*/

/* dir bits for ccb */
#define	HA_RD_DIR	0x08
#define	HA_WRT_DIR	0x10

/*
 * Mail Box structure
 */
struct mbx {
	unsigned int	m_stat : 8;	/* Mail box status or command	*/
	unsigned int	m_addr : 24;	/* CB physical address		*/
};

#define	EMPTY		0x00		/* Mail box is free		*/
#define	SUCCESS		0x01		/* CB successfully completed 	*/
#define	ABORTED		0x02		/* CB aborted by host   	*/
#define	NOT_FND		0x03		/* Aborted CB not found 	*/
#define	FAILURE		0x04		/* CB completed with error    	*/
#define RETRY		0x05		/* Recoverable error, retry	*/
#define	START		0x01		/* Start the CB	command		*/
#define	ABORT		0x02		/* Abort the CB	command 	*/


/*
 * Controller Command Block 
 */
struct ccb {
	unsigned char	c_opcode;
	unsigned char	c_dev;		/* TC LU command destination	*/
	unsigned char	c_cmdsz;	/* Size of command		*/
	unsigned char	c_sensz;	/* Request Sense alloc size	*/
	unsigned char	c_datasz[3];	/* Size of data transfer	*/
	unsigned char	c_datapt[3];	/* Physical src or dst		*/
	unsigned char	c_linkpt[3];	/* link pointer (not supported) */
	unsigned char	c_linkid;	/* Command link ID		*/
	unsigned char	c_hstat; 	/* Host adapter error status	*/
	unsigned char	c_tstat;	/* Target completion status	*/
	unsigned char	c_res[2];	/* reserved for later use	*/
	unsigned char	c_cdb[12];	/* Command Descriptor Block	*/
#ifndef HP_REQ_LEN
	unsigned char	c_sense[sizeof(struct sense)];
#else
	unsigned char	c_sense[75];
#endif
					/* Sense data			*/

     /* from here down does not get sent to the controller */

	paddr_t		c_addr;		/* CB physical address		*/
	struct sb	*c_bind;	/* Associated SCSI block	*/
	struct ccb	*c_next;	/* Pointer to next CCB		*/
	struct sg_array	*c_sg_p;	/* Pointers to the s/g list	*/
	unsigned short 	c_active;	/* Command sent to controller	*/
	unsigned char	c_mstat;	/* Save area for mailbox status */
	struct ccb	*c_donenext;	/* Chain of completed CCBs	*/
};

/* errors that would be stored in c_hstat */

#define	HS_ST		0x11	/* SCSI bus selection timeout		*/
#define	HS_DORUR	0x12	/* Data over/under run error		*/
#define	HS_UBF		0x13	/* Unexpected Bus Free			*/
#define	HS_TBPSF	0x14	/* Target Bus Phase Sequence Failure	*/
#define	HS_ICOC		0x16	/* Invalid CCB Operation Code		*/
#define	HS_LCDNHTSL	0x17	/* Linked CCB does not have the same	*/
				/* LUN */
#define	HS_ITDRFH	0x18	/* Invaild target direction received	*/
				/* from host				*/
#define	HS_DCRITM	0x19	/* Duplicate CCB received in target mode*/
#define	HS_ICOSLP	0x1A	/* Ivalid CCB or segment list parameter	*/

#define MAX_CMDSZ	12

#define	NO_ERROR	0x00		/* No adapter detected error	*/
#define	NO_SELECT	0x11		/* Selection time out		*/
#define	TC_PROTO	0x14		/* TC protocol error		*/

#define ADSC_OP			0x00
#define ADSC_OP_DMA		0x02
#define ADSC_OP_RESID		0x03
#define ADSC_OP_DMA_RESID	0x04
#define SCSI_TRESET		0x81

#define MSBYTE(x)	(((x) >> 16) & 0xff);
#define MDBYTE(x)	(((x) >>  8) & 0xff);
#define LSBYTE(x)	((x) & 0xff);

/*
 * CCB Macros to convert kernel virtual to physical and vice versa.
 */
#define ADSC_CCB_MASK	0xffffff
#define ADSC_CCB_KVTOPHYS(H, C) (paddr_t)((struct ccb *)(H->ha_pccb) + (int)(C - H->ha_ccb))
#define ADSC_CCB_PHYSTOKV(H, PC) (H->ha_ccb  + (int)(PC - (struct ccb *)(H->ha_pccb)))

/*
 * DMA vector structure
 */
struct dma_vect {
	unsigned char	d_cnt[3];	/* Size of data transfer	*/
	unsigned char	d_addr[3];	/* Physical src or dst		*/
};

/*
 * DMA list structure
 */

#define	SG_SEGMENTS	16		/* Hard limit in the controller */
#define pgbnd(a)	(ptob(1) - ((ptob(1) - 1) & (vaddr_t)(a)))

struct dma_list {
	unsigned int	d_size;	/* List size (in bytes)   	*/
	struct dma_list *d_next;	/* Points to next free list	*/
	struct dma_vect d_list[SG_SEGMENTS];	/* DMA scatter/gather list */
};

typedef struct dma_list dma_t;


/*
 * SCSI Request Block structure
 *
 *	THIS STRUCTURE MUST NOT BE LARGER THAN 28 BYTES BECAUSE
 *	OF THE SIZE OF THE POOL FROM WHICH WE ALLOCATE STRUCTURES.
 *
 *	This limit comes from SM_POOLSIZE in sdi_comm.h.
 *
 */
struct srb {
	struct xsb	*sbp;		/* Target drv definition of SB	*/
	struct srb    	*s_next;	/* Next block on LU queue	*/
	struct srb    	*s_priv;	/* Private ptr for dynamic alloc*/
					/* routines DON'T USE OR MODIFY */
	struct srb    	*s_prev;	/* Previous block on LU queue	*/
	dma_t	 	*s_dmap;	/* DMA scatter/gather list	*/
	paddr_t		s_addr;		/* Physical data pointer	*/
	unsigned long	s_block;	/* logical address of r/w command */
};

typedef struct srb sblk_t;


/*
 * Logical Unit Queue structure
 */
struct scsi_lu {
	struct srb     *q_first;	/* First job on LU queue	*/
	struct srb     *q_last;		/* Last job on LU queue	*/
	int		q_flag;		/* LU queue state flags		*/
	struct sense	q_sense;	/* Sense data			*/
	int     	q_outcnt;	/* pending jobs to controller	*/ 
	int		q_depth;	/* jobs in the queue		*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long		q_param;	/* Target driver event param	*/
	pl_t		q_opri;		/* Saved priority level		*/
	unsigned int	q_addr;		/* Last read/write logical address */
	bcb_t		*q_bcbp;	/* Device breakup control block */
	int		q_shift;	/* bits to shift to get to/from
					 * sector size			*/
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru	*/
#ifndef PDI_SVR42
	lock_t		*q_lock;	/* Device queue lock		*/
#endif
};

/*
 * q_flag defines
 */
#define	ADSC_QBUSY		0x01
#define	ADSC_QSUSP		0x04
#define	ADSC_QSENSE		0x08		/* Sense data cache valid */
#define	ADSC_QPTHRU		0x10
#define ADSC_QSCHED		0x20		/* Schedule this queue	  */

#define ADSC_QCLASS(x)	((x)->sbp->sb.sb_type)
#define	ADSC_QNORM		SCB_TYPE


/*
 * Host Adapter structure
 */
struct scsi_ha {
	unsigned short	ha_state;	  /* Operational state 		*/
	unsigned short	ha_id;		  /* Host adapter SCSI id	*/
	int		ha_vect;	  /* Interrupt vector number	*/
	unsigned long   ha_base;	  /* Base I/O address		*/
	int		ha_npend;	  /* # of jobs sent to HA	*/
	struct mbx     *ha_give;	  /* Points to next mbo		*/
	struct mbx     *ha_take;	  /* Points to next mbi		*/
	struct mbx	*ha_mbo;	  /* Outgoing mail box area	*/
	struct mbx	*ha_mbi;	  /* Incoming mail box area	*/
	struct ccb	*ha_ccb;	  /* Controller command blocks	*/
	paddr_t		ha_pccb;	  /* Controller command blocks  */
					  /*  (physical address)	*/
	struct scsi_lu	*ha_dev;	  /* Logical unit queues	*/
#ifndef PDI_SVR42
	lock_t		*ha_mbx_lock;	  /* Lock for controller mailboxes */
#endif
};

#define C_SANITY	0x8000


/*	
**	Macros to help code, maintain, etc.
*/

#define SUBDEV(t,l)	((t << 3) | l)
#define LU_Q(c,t,l)	adsc_ha[c].ha_dev[SUBDEV(t,l)]
#define	FREECCB(x) x->c_active = FALSE

/*
 * Here is the stuff pertaining to the scatter gather code that is done
 * locally in the driver.
 */

#define	SG_SEGMENTS	16		/* number of scatter/gather segments */
#define SG_FREE		0		/* set the s/g list free */
#define	SG_BUSY		1		/* keep track of s/g segment usage */
#define	SG_LENGTH	6		/* each s/g segment is 6 bytes long */

struct sg {
	unsigned char	sg_size_msb;
	unsigned char	sg_size_mid;
	unsigned char	sg_size_lsb;
	unsigned char	sg_addr_msb;
	unsigned char	sg_addr_mid;
	unsigned char	sg_addr_lsb;
};
typedef struct sg SG;

struct sg_array {
	char		sg_flags;	/* use to mark busy/free */
	int		sg_len;		/* length of s/g list */
	long		dlen;		/* total lenth of data */
	paddr_t		addr;		/* address pointer  */
	struct sg_array	*sg_next;	/* pointer to next sg_array list */
	SG		sg_seg[SG_SEGMENTS];
	sblk_t	 	*spcomms[SG_SEGMENTS];
};
typedef struct sg_array SGARRAY;

#define SGNULL	((SGARRAY *)0)

/* These two defines are used for controlling whether or not to kick
   start another command when completing s/g commands.  Typically, I don't
   want another command started until I have serviced all the I/O's in the
   s/g list.  Then on the last one, go ahead and start another command.
*/
#define	SG_NOSTART	0
#define	SG_START	1

#define AD_IERR_ALLOC	"Initialization error - cannot allocate"


/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primatives for multi-processor
 * or spl/splx for uniprocessor.
 */

#define ADSC_DMALIST_LOCK(p) p = LOCK(adsc_dmalist_lock, pldisk)
#define ADSC_SGARRAY_LOCK(p) p = LOCK(adsc_sgarray_lock, pldisk)
#define ADSC_CCB_LOCK(p) p = LOCK(adsc_ccb_lock, pldisk)
#define ADSC_MBX_LOCK(p) p = LOCK(ha->ha_mbx_lock, pldisk)
#define ADSC_SCSILU_LOCK(p) p = LOCK(q->q_lock, pldisk)

#define ADSC_DMALIST_UNLOCK(p) UNLOCK(adsc_dmalist_lock, p)
#define ADSC_SGARRAY_UNLOCK(p) UNLOCK(adsc_sgarray_lock, p)
#define ADSC_CCB_UNLOCK(p) UNLOCK(adsc_ccb_lock, p)
#define ADSC_MBX_UNLOCK(p) UNLOCK(ha->ha_mbx_lock, p)
#define ADSC_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)

#ifndef PDI_SVR42
/*
 * Locking Hierarchy Definition
 */
#define ADSC_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#endif /* !PDI_SVR42 */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_AD1542_H */

#ifndef _IO_HBA_ADSS_ADSS_H	/* wrapper symbol for kernel use */
#define _IO_HBA_ADSS_ADSS_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/adss/adss.h	1.5.4.1"

/*****************************************************************************
 *      ADAPTEC CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license agreement
 *	or nondisclosure agreement with Adaptec Corporation and may not be
 *	copied or disclosed except in accordance with the terms of that
 *	agreement.
 *****************************************************************************/

#define HBA_PREFIX	 adss	/* driver function prefix for hba.h */
#define ADSS_SCSI_ID	7		/* Default ID of controllers	*/

/*****************************************************************************
 * Various defines for control operations
 *****************************************************************************/
#define	PRIMARY_PORT	0x1
#define	SECONDARY_PORT	0x2

#define	MAX_6X60	2	/* 2 of these are all there can be in the system */
#define MAX_EQ	 MAX_TCS * MAX_LUS	/* Max equipage per controller	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	ONE_MIN		60000

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#define	LOC_SUCCESS	0
#define	LOC_FAILURE	1

#define	MAX_CMDSZ	12		/* maximum SCSI command length	*/

/*
 * structure used to obtain a pointer that allows me to index thru
 * alloc'd memory 1 page at a time
 */
struct adss_dma_page {
	uchar_t	d_page[PAGESIZE];
};

typedef struct adss_dma_page adss_page_t;

/*****************************************************************************
 * DMA vector structure
 * First the scatter/gather element defination
 *****************************************************************************/
#define	SG_SEGMENTS	16		/* Hard limit in the controller */
#define pgbnd(a)	(ptob(1) - ((ptob(1) - 1) & (vaddr_t)(a)))

struct dma_vect {
	char	*d_addr;	/* Virtual src or dst		*/
	long	d_cnt;		/* Size of data transfer	*/
};

/**************************************************************************
 * SCSI Request Block structure
 **************************************************************************/
struct srb {
	struct xsb	*sbp;		/* Target drv definition of SB	*/
	struct srb    	*s_next;	/* Next block on LU queue	*/
	struct srb    	*s_priv;	/* Private ptr for dynamic alloc*/
					/* routines DON'T USE OR MODIFY */
	struct srb    	*s_prev;	/* Previous block on LU queue	*/
	caddr_t		s_addr;		/* virtual data pointer	*/
};
typedef struct srb sblk_t;

/*****************************************************************************
 * Here is the stuff pertaining to the scatter gather code that is done
 * locally in the driver.
 *****************************************************************************/
#define SG_FREE		0		/* set the s/g list free */
#define	SG_BUSY		1		/* keep track of s/g segment usage */
#define	SG_LENGTH	8		/* each s/g segment is 8 bytes long */
#define	SG_NOSTART	0
#define	SG_START	1

struct sg {
	char	*sg_addr;
	long	sg_size;
};
typedef struct sg SG;

struct sg_array {
	char		sg_flags;	/* use to mark busy/free */
	int		sg_len;		/* length of s/g list RAN 4/2/92 */
	long		dlen;		/* total lenth of data RAN 4/2/92 */
	struct sg_array	*sg_next;	/* pointer to next sg_array list */
	int		next_seg;
	long		next_offset;
	long		next_length;
	char		*next_addr;
	SG		sg_seg[SG_SEGMENTS];
	sblk_t	 	*spcomms[SG_SEGMENTS];
};
typedef struct sg_array SGARRAY;

#define SGNULL	((SGARRAY *)0)

/*****************************************************************************
 * Logical Unit Queue structure
 *****************************************************************************/
#define	QBUSY		0x01
#define	QSUSP		0x04
#define	QSENSE		0x08		/* Sense data cache valid */
#define	QPTHRU		0x10
#define ADSS_QUECLASS(x)	((x)->sbp->sb.sb_type)
#define	HBA_QNORM		SCB_TYPE

struct scsi_lu {
	struct srb     *q_first;	/* First block on LU queue	*/
	struct srb     *q_last;		/* Last block on LU queue	*/
	int		q_flag;		/* LU queue state flags		*/
	struct sense	q_sense;	/* Sense data			*/
	int     	q_count;	/* jobs running on this SCSI LU	*/
	int		q_depth;	/* jobs in the queue RAN C0	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long		q_param;	/* Target driver event param	*/
	pl_t		q_opri;		/* Saved priority level		*/
	char	       *q_sc_cmd;	/* SCSI cmd for pass-thru	*/
	bcb_t		*q_bcbp;	/* Device breakup control block */
	lock_t		*q_lock;	/* Device queue lock		*/
};

struct req_sense_def {
	unsigned char	error_code :7;
	unsigned char	valid :1;

	unsigned char	segment_number;

	unsigned char	sense_key :4;
	unsigned char	res :1;
	unsigned char	ili :1;
	unsigned char	eom :1;
	unsigned char	file_mark :1;

	unsigned char	information[4];
	unsigned char	additional_sense_length;
	unsigned char	command_specific[4];
	unsigned char	add_sense_code;
	unsigned char	add_sense_qualifier;
	unsigned char	fru_code;
	unsigned char	sksv[3];
};

/*****************************************************************************
 * These defines are used for controlling when memory is de-allocated during *
 * the init routine.							     *
 *****************************************************************************/
#define HA_HACB_REL	0x1
#define	HA_SCB_REL	0x2
#define	HA_REQS_REL	0x4
#define	HA_DEV_REL	0x8
#define	HA_SG_REL	0x10
#define	HA_LUCB_REL	0x20
#define	HA_ALL_REL	(HA_HACB_REL|HA_SCB_REL|HA_REQS_REL|HA_DEV_REL|HA_SG_REL|HA_LUCB_REL)

/*****************************************************************************
 *	Macros to help code, maintain, etc.
 *****************************************************************************/
#define SUBDEV(t,l)	((t << 3) | l)
#define LU_Q(c,t,l)	adss_sc_ha[c]->ha_dev[SUBDEV(t,l)]
#define	FREEBLK(x)	x->c_active = FALSE

#define hmsbyte(x)	(((x) >> 32) & 0xff);
#define msbyte(x)	(((x) >> 16) & 0xff);
#define mdbyte(x)	(((x) >>  8) & 0xff);
#define lsbyte(x)	((x) & 0xff);

/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primatives for multi-processor
 * or spl/splx for uniprocessor.
 */

#define ADSS_DMALIST_LOCK(p) p = LOCK(adss_dmalist_lock, pldisk)
#define ADSS_SGARRAY_LOCK(p) p = LOCK(ha->ha_sg_lock, pldisk)
#define ADSS_SCB_LOCK(p) p = LOCK(ha->ha_scb_lock, pldisk)
#define ADSS_HBA_LOCK(ha) ha->ha_opri = LOCK(ha->ha_hba_lock, pldisk)
#define ADSS_SCSILU_LOCK(q) q->q_opri = LOCK(q->q_lock, pldisk)
#ifdef DEBUG
#define ADSS_SCSILU_IS_LOCKED(q) ((q->q_lock)->sp_lock)
#endif

#define ADSS_DMALIST_UNLOCK(p) UNLOCK(adss_dmalist_lock, p)
#define ADSS_SGARRAY_UNLOCK(p) UNLOCK(ha->ha_sg_lock, p)
#define ADSS_SCB_UNLOCK(p) UNLOCK(ha->ha_scb_lock, p)
#define ADSS_HBA_UNLOCK(ha) UNLOCK(ha->ha_hba_lock, ha->ha_opri)
#define ADSS_SCSILU_UNLOCK(q) UNLOCK(q->q_lock, q->q_opri)

#define ADSS_SET_QFLAG(q,flag) { \
					ADSS_SCSILU_LOCK(q); \
					q->q_flag |= flag; \
					ADSS_SCSILU_UNLOCK(q); \
				}
#define ADSS_CLEAR_QFLAG(q,flag) { \
					ADSS_SCSILU_LOCK(q); \
					q->q_flag &= ~flag; \
					ADSS_SCSILU_UNLOCK(q); \
				}

#ifndef PDI_SVR42
/*
 * Locking Hierarchy Definition
 */
#define ADSS_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#endif /* !PDI_SVR42 */

#endif /* _IO_HBA_ADSS_ADSS_H */

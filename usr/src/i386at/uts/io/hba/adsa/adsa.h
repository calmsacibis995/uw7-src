#ifndef _IO_HBA_ADSA_ADSA_H	/* wrapper symbol for kernel use */
#define _IO_HBA_ADSA_ADSA_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/adsa/adsa.h	1.13.2.1"

#if defined(__cplusplus)
extern "C" {
#endif

/*****************************************************************************
 *      ADAPTEC CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license agreement
 *	or nondisclosure agreement with Adaptec Corporation and may not be
 *	copied or disclosed except in accordance with the terms of that
 *	agreement.
 *****************************************************************************/

/*****************************************************************************
 * Various defines for control operations
 *****************************************************************************/
#define HBA_PREFIX	 adsa	/* driver function prefix for hba.h */
#define MAX_EQ	 MAX_TCS * MAX_LUS	/* Max equipage per controller	*/
#define NDMA		16		/* Number of DMA lists	per controller	*/

#define ADSA_SCSI_ID	7		/* Default ID of controllers	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	ONE_MIN		60000

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#define	ADSA_MAX_SLOTS	15		/* Maximum EISA slots searched for */
					/* the 7770 */
#define	ADSA_MAX_BPC	2		/* Maximum SCSI buses per controller */
#define	LOC_SUCCESS	0
#define	LOC_FAILURE	1
#define	SCB_RETRY	-1

#define	MAX_CMDSZ	12		/* maximum SCSI command length	*/
/*****************************************************************************
 * These defines are set for the synchronous negotiation values used by the
 * driver.
 *****************************************************************************/
#define	SYNC_36		0x70
#define	SYNC_40		0x60
#define	SYNC_44		0x50
#define	SYNC_50		0x40
#define	SYNC_57		0x30
#define	SYNC_67		0x20
#define	SYNC_80		0x10
#define	SYNC_100	0x00
#define	SYNC_MASK	0x8f
#define	SYNC_RATE_36	36
#define	SYNC_RATE_40	40
#define	SYNC_RATE_44	44
#define	SYNC_RATE_50	50
#define	SYNC_RATE_57	57
#define	SYNC_RATE_67	67
#define	SYNC_RATE_80	80
#define	SYNC_RATE_100	100


/*****************************************************************************
 * EISA family control ports.  These define the base ports which
 * need to be or'd with the slot address to get the actual port address
 * for the adapter.
 *****************************************************************************/
#define	SLOT_SHIFT	0xc
#define	SLOT_MASK	0x0f

#define	HOSTID_PORT	0xc80	/* base port for 7770 search via findha */
#define	BASE_PORT	0xc00	/* base port to pass during init time	*/

#define	ADSA_UNDEFINED	0xfff	/* offset base addr undefined in idata */
#define	ADSA_IOADDR_MASK	0xfff	/* offset base addr mask */


/*
 * structure used to obtain a pointer that allows me to index thru
 * alloc'd memory 1 page at a time
 */
struct adsa_dma_page {
	uchar_t	d_page[PAGESIZE];
};

typedef struct adsa_dma_page adsa_page_t;

/*****************************************************************************
 * DMA vector structure
 * First the scatter/gather element defination
 *****************************************************************************/
#define	SG_SEGMENTS	16		/* Hard limit in the controller */
#define pgbnd(a)	(ptob(1) - ((ptob(1) - 1) & (vaddr_t)(a)))

struct dma_vect {
	paddr_t	d_addr;		/* Physical src or dst		*/
	long	d_cnt;		/* Size of data transfer	*/
};

/*****************************************************************************
 * DMA scatter/gather linked list structure
 *****************************************************************************/
struct dma_list {
	unsigned int	d_size;		/* List size (in bytes)   	*/
	struct dma_list *d_next;	/* Points to next free list	*/
	struct dma_vect d_list[SG_SEGMENTS];	/* DMA scatter/gather list */
};

typedef struct dma_list dma_t;

/**************************************************************************
 * SCSI Request Block structure
 **************************************************************************/
struct srb {
	struct xsb	*sbp;		/* Target drv definition of SB	*/
	struct srb    	*s_next;	/* Next block on LU queue	*/
	struct srb    	*s_priv;	/* Private ptr for dynamic alloc*/
					/* routines DON'T USE OR MODIFY */
	struct srb    	*s_prev;	/* Previous block on LU queue	*/
	dma_t	 	*s_dmap;	/* DMA scatter/gather list	*/
	paddr_t		s_addr;		/* Physical data pointer	*/
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
	paddr_t	sg_addr;
	long	sg_size;
};
typedef struct sg SG;

struct sg_array {
	char		sg_flags;	/* use to mark busy/free */
	int		sg_len;		/* length of s/g list RAN 4/2/92 */
	long		dlen;		/* total lenth of data RAN 4/2/92 */
	paddr_t		sg_paddr;	/* address pointer RAN 4/2/92 */
	struct sg_array	*sg_next;	/* pointer to next sg_array list */
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
#define ADSA_QUECLASS(x)	((x)->sbp->sb.sb_type)
#define	HBA_QNORM		SCB_TYPE

struct scsi_lu {
	struct srb     *q_first;	/* First block on LU queue	*/
	struct srb     *q_last;		/* Last block on LU queue	*/
	int		q_flag;		/* LU queue state flags		*/
	struct sense	q_sense;	/* Sense data			*/
	int     	q_outcnt;	/* jobs running on this SCSI LU	*/
	int		q_depth;	/* jobs in the queue RAN C0	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long		q_param;	/* Target driver event param	*/
	pl_t		q_opri;		/* Saved priority level		*/
	bcb_t		*q_bcbp;	/* Device breakup control block */
	char		*q_sc_cmd;	/* SCSI command pointer for pass-thru*/
	lock_t		*q_lock;	/* Device queue lock		*/
};


/*****************************************************************************
 * Host Adapter structure
 *****************************************************************************/
#define C_SANITY	0x8000

struct adsa_lock {
	lock_t		*al_HIM_lock;	  /* Lock for HIM layer */
	pl_t		al_HIM_opri;	  /* spl for HIM layer lock */
};
typedef struct adsa_lock	adsa_lock_t;

struct scsi_ha {
	unsigned char		ha_id;		/* Host adapter SCSI id      */
	unsigned char		ha_chan_num;	/* channel number	     */
	unsigned char		ha_num_chan;	/* number of channels on this HA */
	unsigned char		ha_ctl;		/* controller(internal) number*/
	int			ha_num_scb;	/* number of scb's	     */
#ifdef AHA_DEBUG1
	int			ha_slot;	/* Slot address		     */
#endif
	int			ha_vect;	/* Interrupt vector number   */
	int			ha_npend;	/* # of jobs sent to HA      */
	char			*ha_name;	/* name of driver	     */
	struct him_config_block	*ha_config;	/* config info for adapter   */
	struct scsi_lu		*ha_dev;	/* Logical unit queues	     */
	SGARRAY		*ha_sgnext;	/* next free s/g structure   */
	SGARRAY		*ha_sglist;	/* list of s/g structures    */
	int			ha_sglist_size;	/* number of SG's	     */
	struct sequencer_ctrl_block *ha_scb_next;/* next command block	     */
	struct sequencer_ctrl_block *ha_scb;	/* Controller command blocks */
	struct sequencer_ctrl_block *ha_poll;	/* Polled command block      */
	adsa_lock_t		*ha_HIM_lock;	  /* Lock and spl for HIM layer */
	lock_t		*ha_npend_lock;	  /* Lock for controller pending job count */
	lock_t		*ha_sg_lock;	  /* Lock for controller SG lists */
	lock_t		*ha_scb_lock;	  /* Lock for controller scb lists */
};

#define	CNFNULL ((struct scsi_ha *) 0)

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
 * the init routine.  Added for BETA 1.6				     *
 *****************************************************************************/
#define HA_CH0_REL	0x1
#define HA_CH1_REL	0x2
#define	HA_STRUCT_REL	0x3	/* first 2 bits used to count the channels   */
#define	HA_REL		0x4
#define	HA_CONFIG_REL	0x8
#define	HA_SCB_REL	0x10
#define	HA_REQS_REL	0x20
#define	HA_DEV_REL	0x40
#define	HA_SG_REL	0x80

/*****************************************************************************
 *	Macros to help code, maintain, etc.
 *****************************************************************************/
#define SUBDEV(t,l)	((t << 3) | l)
#define LU_Q(c,b,t,l)	adsa_sc_ha[adsa_ctob[(c)] + (b)]->ha_dev[SUBDEV(t,l)]
#define LU_Q_GB(gb,t,l)	adsa_sc_ha[(gb)]->ha_dev[SUBDEV(t,l)]
#define	FREESCB(x)	x->c_active = FALSE

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

#define ADSA_HIM_LOCK ha->ha_HIM_lock->al_HIM_opri = LOCK(ha->ha_HIM_lock->al_HIM_lock, pldisk)
#define ADSA_DMALIST_LOCK(p) p = LOCK(adsa_dmalist_lock, pldisk)
#define ADSA_SGARRAY_LOCK(p) p = LOCK(ha->ha_sg_lock, pldisk)
#define ADSA_SCB_LOCK(p) p = LOCK(ha->ha_scb_lock, pldisk)
#define ADSA_NPEND_LOCK(p) p = LOCK(ha->ha_npend_lock, pldisk)
#define ADSA_SCSILU_LOCK(p) p = LOCK(q->q_lock, pldisk)

#define ADSA_HIM_UNLOCK UNLOCK(ha->ha_HIM_lock->al_HIM_lock, ha->ha_HIM_lock->al_HIM_opri)
#define ADSA_DMALIST_UNLOCK(p) UNLOCK(adsa_dmalist_lock, p)
#define ADSA_SGARRAY_UNLOCK(p) UNLOCK(ha->ha_sg_lock, p)
#define ADSA_SCB_UNLOCK(p) UNLOCK(ha->ha_scb_lock, p)
#define ADSA_NPEND_UNLOCK(p) UNLOCK(ha->ha_npend_lock, p)
#define ADSA_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)
#define ADSA_SET_QFLAG(q,flag) { \
					ADSA_SCSILU_LOCK(q->q_opri); \
					q->q_flag |= flag; \
					ADSA_SCSILU_UNLOCK(q->q_opri); \
				}
#define ADSA_CLEAR_QFLAG(q,flag) { \
					ADSA_SCSILU_LOCK(q->q_opri); \
					q->q_flag &= ~flag; \
					ADSA_SCSILU_UNLOCK(q->q_opri); \
				}

/*
 * Locking Hierarchy Definition
 */
#define ADSA_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_ADSA_ADSA_H */

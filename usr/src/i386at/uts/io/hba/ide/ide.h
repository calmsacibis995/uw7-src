#ifndef _IO_HBA_IDE_IDE_H	/* wrapper symbol for kernel use */
#define _IO_HBA_IDE_IDE_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/ide/ide.h	1.3.2.1"

#if defined(__cplusplus)
extern "C" {
#endif

/*****************************************************************************
 * Various defines for control operations
 *****************************************************************************/
#define HBA_PREFIX	 ide	/* driver function prefix for hba.h */
#define MAX_EQ	 MAX_TCS * MAX_LUS	/* Max equipage per controller	*/
#define NDMA		16		/* Number of DMA lists	per controller	*/

#define IDE_SCSI_ID	7		/* Default ID of controllers	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	ONE_MIN		60000

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#define	IDE_MAX_BPC	2		/* Maximum ATA buses per controller */
#define	LOC_SUCCESS	0
#define	LOC_FAILURE	1

#define	MAX_CMDSZ	12		/* maximum SCSI command length	*/

#define	LUN	0x07

#define SCM_RAD(x) ((char *)x - 2)

/*****************************************************************************
 * DMA vector structure
 * First the scatter/gather element defination
 *****************************************************************************/
#define	SG_SEGMENTS	17		/* Hard limit in the controller */
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
 * Logical Unit Queue structure
 *****************************************************************************/
#define	QSCHED		0x02
#define	QSUSP		0x04
#define	QSENSE		0x08		/* Sense data cache valid */
#define	QPTHRU		0x10
#define IDE_QUECLASS(x)	((x)->sbp->sb.sb_type)
#define	HBA_QNORM		SCB_TYPE

struct scsi_lu {
	struct srb     *q_first;	/* First block on LU queue	*/
	struct srb     *q_last;		/* Last block on LU queue	*/
	int		q_flag;		/* LU queue state flags		*/
	int     	q_outcnt;	/* jobs running on this SCSI LU	*/
	int		q_depth;	/* jobs in the queue RAN C0	*/
	void	      (*q_func)();	/* Target driver event handler	*/
	long		q_param;	/* Target driver event param	*/
	pl_t		q_opri;		/* Saved priority level		*/
	bcb_t		*q_bcbp;	/* Device breakup control block */
	char		*q_sc_cmd;	/* SCSI command pointer for pass-thru*/
	lock_t		*q_lock;	/* Device queue lock		*/
	struct ata_ha	*q_ha;	/* HA for this LU			*/
	int	q_shift;	/* byte to block shift for this device */
	unsigned int	q_addr;
	struct sense	q_sense;	/* Sense data			*/
};

/*****************************************************************************
 * Host Adapter structure
 *****************************************************************************/
struct ide_lock {
	lock_t		*al_HIM_lock;	  /* Lock for HIM layer */
	pl_t		al_HIM_opri;	  /* spl for HIM layer lock */
};
typedef struct ide_lock	ide_lock_t;

struct ata_ha {
	unsigned char		ha_id;		/* Host adapter SCSI id      */
	unsigned char		ha_chan_num;	/* channel number	     */
	unsigned char		ha_num_chan;	/* number of channels on this HA */
	unsigned char		ha_ctl;		/* controller(internal) number*/
	int			ha_num_scb;	/* number of scb's	     */
	int			ha_vect;	/* Interrupt vector number   */
	int			ha_npend;	/* # of jobs sent to HA      */
	int			ha_devices;	/* # of devices on HA      */
	char			*ha_name;	/* name of driver	     */
	struct ide_cfg_entry	*ha_config;	/* config info for adapter   */
	struct scsi_lu		*ha_dev;	/* Logical unit queues	     */
	dma_t		*ha_dfreelistp;	/* free list of dma structures    */
	bcb_t		*ha_bcbp;	/* Device breakup control block */
	struct control_area *ha_scb_next;/* next command block	     */
	struct control_area *ha_scb;	/* Controller command blocks */
	struct control_area *ha_poll;	/* Polled command block      */
	ide_lock_t		*ha_HIM_lock;	  /* Lock and spl for HIM layer */
	lock_t		*ha_npend_lock;	  /* Lock for controller pending job count */
	lock_t		*ha_scb_lock;	  /* Lock for controller scb lists */
};

#define	CNFNULL ((struct ata_ha *) 0)

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
#define	HA_DMA_REL	0x100

/*****************************************************************************
 *	Macros to help code, maintain, etc.
 *****************************************************************************/
#define SUBDEV(t,l)	((t << 3) | l)
#define LU_Q(c,b,t,l)	ide_sc_ha[ide_ctob[(c)] + (b)]->ha_dev[SUBDEV(t,l)]
#define LU_Q_GB(gb,t,l)	ide_sc_ha[(gb)]->ha_dev[SUBDEV(t,l)]
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

#define IDE_HIM_LOCK ha->ha_HIM_lock->al_HIM_opri = LOCK(ha->ha_HIM_lock->al_HIM_lock, pldisk)
#define IDE_DMALIST_LOCK(p) p = LOCK(ide_dmalist_lock, pldisk)
#define IDE_SCB_LOCK(p) p = LOCK(ha->ha_scb_lock, pldisk)
#define IDE_NPEND_LOCK(p) p = LOCK(ha->ha_npend_lock, pldisk)
#define IDE_SCSILU_LOCK(p) IDE_SCSILU_LOCK_A(q,p)
#define IDE_SCSILU_LOCK_A(q,p) p = LOCK(q->q_lock, pldisk)

#define IDE_HIM_UNLOCK UNLOCK(ha->ha_HIM_lock->al_HIM_lock, ha->ha_HIM_lock->al_HIM_opri)
#define IDE_DMALIST_UNLOCK(p) UNLOCK(ide_dmalist_lock, p)
#define IDE_SCB_UNLOCK(p) UNLOCK(ha->ha_scb_lock, p)
#define IDE_NPEND_UNLOCK(p) UNLOCK(ha->ha_npend_lock, p)
#define IDE_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)
#define IDE_SET_QFLAG(q,flag) { \
					IDE_SCSILU_LOCK(q->q_opri); \
					q->q_flag |= flag; \
					IDE_SCSILU_UNLOCK(q->q_opri); \
				}
#define IDE_CLEAR_QFLAG(q,flag) { \
					IDE_SCSILU_LOCK(q->q_opri); \
					q->q_flag &= ~flag; \
					IDE_SCSILU_UNLOCK(q->q_opri); \
				}

/*
 * Locking Hierarchy Definition
 */
#define IDE_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_IDE_IDE_H */

#ifndef _IO_HBA_ADSL_ADSL_H	/* wrapper symbol for kernel use */
#define _IO_HBA_ADSL_ADSL_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/adsl/adsl.h	1.1.6.1"

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
#define HBA_PREFIX	 adsl	/* driver function prefix for hba.h */

#define ADSL_MAX_EXTCS 16		/* MAX_EXTCS/2 (HW & CHIM limitation) */
#define ADSL_MAX_EXLUS 16		/* MAX_EXLUS/2 */
#define MAX_EQ	 (ADSL_MAX_EXTCS * ADSL_MAX_EXLUS)	/* Max equipage per controller	*/

#define ADSL_SCSI_ID	7		/* Default ID of controllers	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	ONE_MIN		60000

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

					/* the 7870 */
#define	ADSL_MAX_BPC	1		/* Maximum SCSI buses per controller */
#define	LOC_SUCCESS	0
#define	LOC_FAILURE	1
#define	SCB_RETRY	-1

#define	MAX_CMDSZ	12		/* maximum SCSI command length	*/


/*
 * structure used to obtain a pointer that allows me to index thru
 * alloc'd memory 1 page at a time
 */
struct adsl_dma_page {
	uchar_t	d_page[PAGESIZE];
};

typedef struct adsl_dma_page adsl_page_t;

/*****************************************************************************
 * DMA vector structure
 * First the scatter/gather element defination
 *****************************************************************************/
#define	SG_SEGMENTS	17		
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
	unsigned long   s_block;        /* logical address of r/w command */
#if PDI_VERSION >= PDI_SVR42MP
/** TimeOut & Reset Code: START **/
	struct srb	*s_job_next;    /* WatchDog Active Job List     */
	struct srb	*s_job_prev;	/* WatchDog Active Job List     */
	ushort		s_time_quantum;	/* This is the maximum number   */
					/* of ADSL_TIME_QUANTUM units   */
					/* this command can take before */
					/* we software time it out.     */
	ushort		s_flags;	/* Misc job related flags, see  */
					/* below for description        */
	struct HIM_IOB_		*s_cp;		/* Pointer to the associated    */
					/* HIM representation of this   */
					/* job. This is used when we    */
					/* need to abort the command    */
/** TimeOut & Reset Code: END **/
#endif
};
typedef struct srb sblk_t;

#if PDI_VERSION >= PDI_SVR42MP
/** TimeOut & Reset Code: START **/
#define SB_BEING_ABORTED	0x0001  /* This job is in the process   */
                                        /* of being aborted.            */
#define SB_CALLED_BACK		0x0002  /* Callback was done for this   */
                                        /* job.                         */
#define SB_PROCESS		0x0004	/* This job was processed in current
					 * watchdog call.
					 */
#define SB_MARK			0x0008	/* For debugging purposes only	*/
#define HDL_GOOD		0x0010	/* This job is succesfully */
#define HDL_ABORT_NOT_FUND	0x0020	/* This job maybe finished already, or 
						maybe some error ocured */
#define HDL_ABORT_STARTED	0x0040	/* Abort is processing */
#define HDL_ABORT_ALREADY_DONE  0x0080	/* The job was just done */
#define HDL_OTHER_ERR		0x0100	/* Unexpected status */

#define ADSL_TIMEOUT_ON		0x0001
#define ADSL_TIME_QUANTUM	5

#define ADSL_WATCHDOG_LOCK(p) p = LOCK(ha->ha_watchdog_lock, pldisk)
#define ADSL_WATCHDOG_UNLOCK(p) UNLOCK(ha->ha_watchdog_lock, p)
/** TimeOut & Reset Code: END **/
#endif


/*****************************************************************************
 * Here is the stuff pertaining to the scatter gather code that is done
 * locally in the driver.
 *****************************************************************************/
#define SG_FREE		0		/* set the s/g list free */
#define	SG_BUSY		1		/* keep track of s/g segment usage */
#define	SG_LENGTH	8		/* each s/g segment is 8 bytes long */

#define	SG_NOSTART	0
#define	SG_START	1
#define SG_TIMEOUT	2
#define SG_TIME_NOABT	4
#define SG_DONE		8

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
#define	Q_BUSY		0x01
#define	Q_FULL		0x02
#define	Q_SUSP		0x04
#define	Q_SENSE		0x08		/* Sense data cache valid */
#define	Q_PTHRU		0x10
#define	Q_REMOVED	0x20
#define	Q_RETRY		0x40
#define Q_TAG		0x80
#define Q_SUSPR		0x100	/* Q suspended due to 3rd party reset */
#define ADSL_QUECLASS(x)	((x)->sbp->sb.sb_type)
#define	Q_NORM		SCB_TYPE

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
	int		q_limit;	/* max jobs for target		*/
#if PDI_VERSION >= PDI_SVR42MP
	bcb_t		*q_bcbp;	/* Device breakup control block */
	char		*q_sc_cmd;	/* SCSI command pointer for pass-thru*/
	lock_t		*q_lock;	/* Device queue lock		*/
	int		 q_shift;	/* Bits to shift for sector size */
#endif
};


/*****************************************************************************
 * Host Adapter structure
 *****************************************************************************/
#define C_SANITY	0x8000
#define ADSL_RWBUFSIZE	0x2000		/* Default size of Read/Write BUFFER */

struct adsl_lock {
	lock_t		*al_HIM_lock;	  /* Lock for HIM layer */
	pl_t		al_HIM_opri;	  /* spl for HIM layer lock */
};
typedef struct adsl_lock	adsl_lock_t;

struct scsi_ha {
	unsigned char		ha_id;		/* Host adapter SCSI id      */
	unsigned char		ha_chan_num;	/* channel number	     */
	unsigned char		ha_num_chan;	/* number of channels on this HA */
	unsigned char		ha_ctl;		/* controller(internal) number*/
	int			ha_num_iob;	/* number of iob's	     */
#ifdef EISA
	int			ha_slot;	/* Slot address		     */
#else	/* PCI */
	int			reserved;
	unsigned char	ha_pci_architecture;
	unsigned char	ha_pci_bus;
	unsigned char	ha_pci_dev;
	unsigned char	ha_pci_func;
	unsigned char	ha_bios;
	unsigned char	ha_himType;
	unsigned char	ha_buscnt;
	unsigned char	ha_numTarget;
	ulong_t			ha_productID;
	ulong_t			ha_baseAddr;
	void			*ha_adapterTSH;
	struct HIM_FUNC_PTRS_	*ha_himFuncPtr;
	struct HIM_ADAPTER_PROFILE_	*ha_profile;
	void			*ha_targetTSH[MAX_EQ];
	struct HIM_TARGET_PROFILE_	**ha_targetProfile;
	struct HIM_CONFIGURATION_ 	*ha_himConfig;	/* config info for adapter   */
	void			*ha_cioConfig;
#endif
	int			ha_vect;	/* Interrupt vector number   */
	char			*ha_name;	/* name of driver	     */
	struct scsi_lu		**ha_dev;	/* Logical unit queues	     */
	SGARRAY		*ha_sgnext;	/* next free s/g structure   */
	SGARRAY		*ha_sglist;	/* list of s/g structures    */
	int			ha_sglist_size;	/* number of SG's	     */
	struct HIM_IOB_ 	*ha_iob_next; /* next command block	     */
	struct HIM_IOB_ 	*ha_iob;	/* Controller command blocks */
	struct HIM_IOB_ 	*ha_poll;	/* Polled command block      */
#if PDI_VERSION >= PDI_SVR42MP
	adsl_lock_t		*ha_HIM_lock;	  /* Lock and spl for HIM layer */
	lock_t		*ha_sg_lock;	  /* Lock for controller SG lists */
	lock_t		*ha_iob_lock;	  /* Lock for controller iob lists */
/** TimeOut & Reset Code: START **/
	lock_t          *ha_watchdog_lock;/* Lock for timeout srb lists */
	int             ha_flags;
	struct srb      *ha_actv_jobs;    /* Active jobs being monitored   */
                                          /* the watchdog timer.           */
	struct srb	*ha_actv_last;	  /* Last job on active last	   */
	ushort		ha_quantum;	  /* Quantum until last job times out */
/** TimeOut & Reset Code: END **/
#endif
#ifdef _TARGET_MODE
	char 		*ha_rwbufp;	/* Read/Write BUFFER pointer	*/
	struct ident	*ha_inqp;	/* Inquiry data pointer		*/
	struct sense	*ha_sensep;	/* Request sense data pointer	*/
	struct read_buf_desc ha_rbdesc;	/* Read/Write BUFFER desc.	*/
	struct sg	ha_sg;		/* Scatter/Gather list		*/
#endif
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


struct adsl_productInfo	{
	ulong_t	id;
	ulong_t	mask;
	char	index;
	char	himtype;
};

/*****************************************************************************
 * These defines are used for controlling when memory is de-allocated during *
 * the init routine.  Added for BETA 1.6				     *
 *****************************************************************************/
#define HA_CH0_REL	0x1
#define HA_CH1_REL	0x2
#define	HA_STRUCT_REL	0x3	/* first 2 bits used to count the channels   */
#define	HA_REL		0x4
#define	HA_ADDR_REL		0x5
#define	HA_CONF_REL		0x6
#define	HA_PROFILE_REL	0x8
#define	HA_IOB_REL	0x10
#define	HA_REQS_REL	0x20
#define	HA_OPTION_REL	0x30
#define	HA_DEV_REL	0x40
#define	HA_CFP_OPTION_REL	0x50
#define	HA_SG_REL	0x80
#define	HA_LUQ_REL	0x100
#define	HA_IDENT_REL	0x200
#define	HA_IOBRESV_REL	0x300
#define	HA_SENSE_REL	0x400

/*****************************************************************************
 *	Macros to help code, maintain, etc.
 *****************************************************************************/
#define SUBDEV(t,l)	((t << 4) | l)
#define LU_Q(c,b,t,l)	adsl_sc_ha[adsl_ctob[(c)] + (b)]->ha_dev[SUBDEV(t,l)]
#define LU_Q_GB(gb,t,l)	adsl_sc_ha[(gb)]->ha_dev[SUBDEV(t,l)]
#define	FREEIOB(x)	((OSMsp *)x->osRequestBlock)->c_active = FALSE

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

#define ADSL_HIM_LOCK ha->ha_HIM_lock->al_HIM_opri = LOCK(ha->ha_HIM_lock->al_HIM_lock, pldisk)
#define ADSL_DMALIST_LOCK(p) p = LOCK(adsl_dmalist_lock, pldisk)
#define ADSL_SGARRAY_LOCK(p) p = LOCK(ha->ha_sg_lock, pldisk)
#define ADSL_IOB_LOCK(p) p = LOCK(ha->ha_iob_lock, pldisk)
#define ADSL_SCSILU_LOCK(p) p = LOCK(q->q_lock, pldisk)

#define ADSL_HIM_UNLOCK UNLOCK(ha->ha_HIM_lock->al_HIM_lock, ha->ha_HIM_lock->al_HIM_opri)
#define ADSL_DMALIST_UNLOCK(p) UNLOCK(adsl_dmalist_lock, p)
#define ADSL_SGARRAY_UNLOCK(p) UNLOCK(ha->ha_sg_lock, p)
#define ADSL_IOB_UNLOCK(p) UNLOCK(ha->ha_iob_lock, p)
#define ADSL_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)
#define ADSL_SET_QFLAG(q,flag) { \
					ADSL_SCSILU_LOCK(q->q_opri); \
					q->q_flag |= flag; \
					ADSL_SCSILU_UNLOCK(q->q_opri); \
				}
#define ADSL_CLEAR_QFLAG(q,flag) { \
					ADSL_SCSILU_LOCK(q->q_opri); \
					q->q_flag &= ~flag; \
					ADSL_SCSILU_UNLOCK(q->q_opri); \
				}

#if PDI_VERSION >= PDI_SVR42MP
/*
 * Locking Hierarchy Definition
 */
#define ADSL_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#endif /* PDI_VERSION >= PDI_SVR42MP */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_ADSL_ADSL_H */





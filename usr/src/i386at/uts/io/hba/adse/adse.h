#ifndef _IO_HBA_ADSE_ADSE_H	/* wrapper symbol for kernel use */
#define _IO_HBA_ADSE_ADSE_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/adse/adse.h	1.8.2.1"

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
#define HBA_PREFIX	 adse	/* driver function prefix for hba.h */
#define MAX_EQ	 MAX_TCS * MAX_LUS	/* Max equipage per controller	*/
#define	MAX_COMMANDS	32		/* maximum number of SCSI commands
					   to be issued to any given adapter */
#define	MAX_PER_TAR	4		/* maximum number of commands to be
					   allowed on any given target */
#define NDMA		20		/* Number of DMA lists per controller */

#define ADSE_SCSI_ID		7		/* Default ID of controllers	*/

#define	ONE_MSEC	1
#define	ONE_SEC		1000
#define	ONE_MIN		60000

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#define AD_IERR_ALLOC	"Initialization error - cannot allocate"

#define	ADSE_MAX_SLOTS	15		/* Maximum EISA slots searched for */
					/* the 1740 */
#define	LOC_SUCCESS	0
#define	LOC_FAILURE	1

#define	MAX_CMDSZ	12		/* maximum SCSI command length	*/

/*****************************************************************************
 * AHA-1740 family control ports.  These define the base ports which
 * need to be or'd with the slot address to get the actual port address
 * for the adapter.
 *****************************************************************************/
#define	SLOT_SHIFT	0xc
#define	SLOT_MASK	0x0f
#define	SLOT_1		0x1000
#define	SLOT_2		0x2000
#define	SLOT_3		0x3000
#define	SLOT_4		0x4000
#define	SLOT_5		0x5000
#define	SLOT_6		0x6000
#define	SLOT_7		0x7000
#define	SLOT_8		0x8000
#define	SLOT_9		0x9000

#define	HOSTID_PORT	0xc80	/*   R host ID ports 0xc80 - 0xc82	*/
#define	ADSE_IOADDR_MASK	0xfff   /* pick off I/O address information */
#define	ADSE_UNDEFINED	0xfff	/* offset base addr undefined in idata */

/**************************************************************************
 * The following registers must be or'd with the slot number the 1740 is
 * in, so the correct port address can be found.  NOTE: Not all these
 * defines are used by the driver, but are here for reference.
 **************************************************************************/
#define	EXP_BD_PORT	0xc84	/* W/R expansion board control port	*/
#define	EXP_ERRST	0x4
#define	EXP_HAERR	0x2
#define	EXP_CDEN	0x1

struct exp_port_def {			/********************************/
	unsigned char	cden :1;	/* This structure is provided	*/
	unsigned char	haerr :1;	/* for reference only.		*/
	unsigned char	errst :1;	/********************************/
	unsigned char	res :5;
};

#define	BMIC_PORT	0xc88	/* W/R base port for BMIC 0xc88 - 0xc9f	*/

/**************************************************************************
 * These are definations for the port address bits
 **************************************************************************/
#define	IO_ADDRESS_PORT	0xcc0	/* W/R I/O port address			*/
#define	IO_ENHANCED	0x80
#define	IO_CONFIGURE	0x40
#define	IO_MASK		0x7
#define	P334		0x7
#define	P330		0x6
#define	P234		0x5
#define	P230		0x4
#define	P134		0x3
#define	P130		0x2

struct io_port_def {			/********************************/
	unsigned char	adr :3;		/* This structure is provided	*/
	unsigned char	res :3;		/* for reference only and will	*/
	unsigned char	configure :1;	/* not be used by the driver	*/
	unsigned char	enhanced :1;	/********************************/
};


/**************************************************************************
 * Defines for the BIOS address bits in the bio_add_def->biosel variable
 **************************************************************************/
#define	BIOS_ADDRESS	0xcc1	/* W/R BIOS address port		*/

#define	B_WRTPRT	0x80
#define	B_BIOSEN	0x40
#define	B_RAMEN		0x20
#define	B_MASK		0x0f

#define	B_EC000		0xb
#define	B_E8000		0xa
#define	B_E4000		0x9
#define	B_E0000		0x8
#define	B_DC000		0x7
#define	B_D8000		0x6
#define	B_D4000		0x5
#define	B_D0000		0x4
#define	B_CC000		0x3
#define	B_C8000		0x2

struct bios_add_def {			/********************************/
	unsigned char	biosel :4;	/* This structure is simply	*/
	unsigned char	res :1;		/* provided to make it easier	*/
	unsigned char	ramen :1;	/* understand the above defines	*/
	unsigned char	biosen :1;	/* It will not be used in the	*/
	unsigned char	wrtprt :1;	/* driver at all		*/
};					/********************************/

/**************************************************************************
 * Defines for the interrupt port structure - int_port_def->intsel
 **************************************************************************/
#define	INT_DEF_PORT	0xcc2	/* W/R Interrupt defination port	*/

#define	I_INTEN		0x10
#define	I_INTHIGH	0x8
#define	I_MASK		0x7
#define	I_BASE		0x9
#define	I_15		0x6
#define	I_14		0x5
#define	I_12		0x3
#define	I_11		0x2
#define	I_10		0x1
#define	I_9		0x0
					/********************************/
struct int_port_def {			/* This structure will not be	*/
	unsigned char	intsel :3;	/* used, but is simply here to	*/
	unsigned char	inthigh :1;	/* document the structure of 	*/
	unsigned char	inten :1;	/* this particular register	*/
	unsigned char	res :3;		/********************************/
};


/**************************************************************************
 * Defines for the scsi_port_def->hscsiid variable
 **************************************************************************/
#define	SCSI_DEF_PORT	0xcc3	/* W/R SCSI defination port		*/

#define	ID_RSTPWR	0x10
#define	ID_MASK		0x0f
#define ID_7		0x7
#define	ID_6		0x6
#define	ID_5		0x5
#define	ID_4		0x4
#define	ID_3		0x3
#define	ID_2		0x2
#define	ID_1		0x1
#define	ID_0		0x0

struct scsi_port_def {			/********************************/
	unsigned char	hscsiid :4;	/* This structure is provided	*/
	unsigned char	rstpwr :1;	/* for reference and will not be*/
	unsigned char	res :3;		/* used in the driver.		*/
};					/********************************/

/**************************************************************************
 * Defines for the bus_port_def->buson variable
 * Defines for the bus_port_def->dma_channel variable
 **************************************************************************/
#define	BUS_DEF_PORT	0xcc4	/* W/R Bus defination port		*/

#define	ADSE_BO_8		0x2
#define	ADSE_BO_4		0x1
#define	ADSE_BO_0		0x0

#define	ADSE_DMA_7		0x3
#define	ADSE_DMA_6		0x2
#define	ADSE_DMA_5		0x1
#define	ADSE_DMA_0		0x0

struct bus_port_def {
	unsigned char	buson :2;
	unsigned char	dma_channel :2;
	unsigned char	res :4;
};

#define	FLOPPY_DEF_PORT	0xcc5	/* W/R Floppy defination port		*/
#define	RES_PORT	0xcc6	/*     reserved				*/
#define	RES1_PORT	0xcc7	/*     reserved				*/
#define	MBOX_OUT_PORT	0xcd0	/* W/R 4 ports/bytes long		*/


/**************************************************************************
 * These defines tell the adapter what to do with the MBO.  First the MBO
 * must be loaded and then the ATTN register is written with these values
 * or'd with the target ID of the device the ECB is for.
 **************************************************************************/
#define	ATTN_PORT	0xcd4	/* W/R attention port			*/

#define	IMMED_COMMAND	0x10
#define	SEND_COMMAND	0x40
#define	ABORT_COMMAND	0x50
#define	ATTN_RESET	0x80
#define	ATTN_RES_DEV	0x4

struct attn_def {			/********************************/
	unsigned char	target_id :4;	/* This structure is provided	*/
	unsigned char	op_code :4;	/* for reference only		*/
};					/********************************/

struct attn_reset_def {
	unsigned char	command;
	unsigned char	res;
	short		options;
};

/**************************************************************************
 * control bits for the control (zcd5) register
 **************************************************************************/
#define	CONTROL_PORT	0xcd5	/* W/R control port			*/

#define	HARD_RESET	0x80
#define	CLEAR_INTERRUPT	0x40
#define	SET_HOST_READY	0x20

struct control_def {			/********************************/
	unsigned char	res :5;		/* This structure is provided	*/
	unsigned char	set_ready :1;	/* for reference only		*/
	unsigned char	clear_in :1;	/********************************/
	unsigned char	hard_reset :1;
};


/**************************************************************************
 * interrupt status register bits
 **************************************************************************/
#define	INT_STATUS_PORT	0xcd6	/*   R interrupt status port		*/

#define	ADSE_SDI_RETRY	0xff
#define	INT_CCB_SUCCESS	0x10
#define	INT_CCB_RETRY	0x50
#define	INT_ADAPTER_BAD	0x70
#define	INT_IMM_SUCCES	0xa0
#define	INT_CCB_ERROR	0xc0
#define	INT_AEN		0xd0
#define	INT_IMM_ERROR	0xe0
#define	INT_TAR_MASK	0x0f

struct interrupt_def {			/********************************/
	unsigned char	target :4;	/* This structure is provided	*/
	unsigned char	int_stat :4;	/* for reference only		*/
};					/********************************/

/**************************************************************************
 * status register bits
 **************************************************************************/
#define	STATUS1_PORT	0xcd7	/*   R status 1 port			*/

#define	ST1_MBO_EMPTY	0x4
#define	ST1_INT_PENDING	0x2
#define	ST1_BUSY_SET	0x1

struct status_def {			/********************************/
	unsigned char	busy :1;	/* This structure is provided	*/
	unsigned char	int_pending :1;	/* for reference only		*/
	unsigned char	mbo_empty :1;	/********************************/
	unsigned char	res :5;
};

#define	MBOX_IN_PORT	0xcd8	/*   R 4 ports/bytes long		*/


/**************************************************************************
 * status 2 register bits
 **************************************************************************/
#define	STATUS2_PORT	0xcdc	/*   R status 2 port			*/

#define	HOST_READY	0x1

struct stat2_def {
	unsigned char	host_ready :1;
	unsigned char	res :7;
};

/*****************************************************************************
 * Defines for host adapter ecb flag words
 * Flag word 1 defines
 *****************************************************************************/
#define	FLAG1_CNE	0x1
#define	FLAG1_DI	0x80
#define	FLAG1_SES	0x400
#define	FLAG1_SG	0x1000
#define	FLAG1_DSB	0x4000
#define	FLAG1_ARS	0x8000
/*****************************************************************************
 * Flag word 2 defines
 *****************************************************************************/
#define	FLAG2_LUN_MASK	0x7
#define	FLAG2_TAG	0x8
#define	FLAG2_TT	0x30
#define	FLAG2_ND	0x40
#define	FLAG2_DAT	0x100
#define	FLAG2_DIR	0x200
#define	FLAG2_ST	0x400
#define	FLAG2_CHK	0x800
#define	FLAG2_REC	0x4000
#define	FLAG2_NRB	0x8000

/**************************************************************************
 * Error codes that can be returned from an adapter reset
 **************************************************************************/
#define	NO_ERROR	0x0
#define	ROM_TEST_FAIL	0x1
#define	RAM_TEST_FAIL	0x2
#define	POWER_ERROR	0x3
#define	CPU_FAIL	0x4
#define	BUFFER_FAIL	0x5
#define	HARDWARE_FAIL	0x7
#define	SCSI_FAIL	0x8


/*****************************************************************************
 * Defines for status word in the status block (status_word)
 *****************************************************************************/
#define	STAT_DON	0x1	/*   command done - no error		*/
#define	STAT_DU		0x2	/*   data underrun			*/
#define	STAT_QF		0x8	/*   host adapter queue full		*/
#define STAT_SC		0x10	/*   specification check		*/
#define STAT_DO		0x20	/*   data overrun			*/
#define STAT_CH		0x40	/*   chaining halted			*/
#define STAT_INT	0x80	/*   interrupt issued for SCB		*/
#define STAT_ASA	0x100	/*   additional status available	*/
#define STAT_SNS	0x200	/*   sense information stored		*/
#define STAT_INI	0x800	/*   initialization required		*/
#define STAT_ME		0x1000	/*   major error or exception occurred	*/
#define STAT_ECA	0x4000	/*   extended contingent allegiance	*/

/*****************************************************************************
 * Defines for target status in the status block (target_stat)
 *****************************************************************************/
#define	TAR_GOOD	0x0	/* good status */
#define	TAR_CHECK	0x2	/* check condition */
#define	TAR_MET		0x4	/* condition met */
#define	TAR_BUSY	0x8	/* target busy */
#define	TAR_INTER	0x10	/* intermmediate */
#define	TAR_INTER_MET	0x14	/* intermmediate condition met */
#define	TAR_CONFLICT	0x18	/* reservation conflict */

/*****************************************************************************
 * Defines for host adapter errors that would be stored in ha_stat
 *****************************************************************************/
#define	AD_NHASA	0x0	/* no host adapter status available	*/
#define	AD_CABH		0x4	/* command aborted by host		*/
#define	AD_CABHA	0x5	/* command aborted by host adapter	*/
#define	AD_FND		0x8	/* firmware not downloaded		*/
#define	AD_TNATSS	0xa	/* target not asssigned to SCSI subsystems */
#define	AD_ST		0x11	/* selection timeout 			*/
#define	AD_DOOUO	0x12	/* data overrun or underrun occurred	*/
#define	AD_UBFO		0x13	/* unexpected bus free occurred		*/
#define	AD_IBPD		0x14	/* Invalid bus phase detected		*/
#define	AD_IOC		0x16	/* Invalid operation code		*/
#define	AD_ISLO		0x17	/* Invalid SCSI linking operation	*/
#define	AD_ICBP		0x18	/* Invalid control block parameter	*/
#define	AD_DTCBR	0x19	/* duplicate target control block received */
#define	AD_ISGL		0x1a	/* invalid scatter/gather list		*/
#define	AD_RSCF		0x1b	/* request sense command failed		*/
#define	AD_TQMRBT	0x1c	/* tagged queueing message rejected by
				   target */
#define	AD_HAHE		0x20	/* host adapter hardware error		*/
#define	AD_TDNRTA	0x21	/* target did not respond to attention	*/
#define	AD_SBRBHA	0x22	/* SCSI bus reset by host adapter	*/
#define	AD_SBRBOD	0x23	/* SCSI bus reset by other device	*/
#define	AD_PCF		0x80	/* program checksum failure		*/


/*****************************************************************************
 * the status block structure for the ecb
 *****************************************************************************/
struct stat_def {
	short	status_word;		/* as defined in the above bits	*/
	char	ha_stat;		/* host adapter status		*/
	char	target_stat;		/* the target status - see above*/
	long	residual_count;		/* # of data bytes transfered	*/
	paddr_t	residual_ptr;		/* pointer to the buffer	*/
	short	add_stat_len;		/* additional status length	*/
	char	sense_len;		/* actual sense data stored	*/
	char	res;
	long	res1;
	long	res2;
	char	target_cdb[6];
};


/*
 * The defines for the command opcodes for the ecb structure
 */
#define	NO_OPERATION	0x0
#define	INIT_SCSI_COMM	0x1
#define	RUN_DIAGS	0x5
#define	INIT_SCSI_SYS	0x6
#define	READ_SENSE	0x8
#define	DOWNLOAD_FIRM	0x9
#define	READ_ADAP_INQ	0xa
#define	TAR_SCSI_COMM	0x10

#define	ECB_NULL	((struct ecb *)0)

/*
 * Controller Command Block 
 */
struct ecb {
	short		command_word;	/* command opcode */
	short		flag1;
	short		flag2;
	short		reserv;
	paddr_t		data_ptr;	/* data/scatter/gather pointer	*/
	long		data_length;	/* data/scatter/gather length	*/
	paddr_t		status_blk_ptr;	/* struct stat_def		*/
	paddr_t		chain_ptr;	/* chain pointer		*/
	long		reserv1;
	paddr_t		sense_ptr;	/* struct req_sense_def		*/
	uchar_t		sense_length;	/* request sense data length	*/
	uchar_t		ccb_length;	/* scsi command length		*/
	short		data_checksum;	/* data checksum - not used	*/
	char		scsi_command[12]; /* scsi command area		*/

     /* from here down does not get sent to the controller */

	int		c_target;		/* target ID of SCSI device	*/
	paddr_t		c_addr;		/* ECB physical address		*/
	struct req_sense_def *c_s_ptr;	/* Sense Pointer		*/
	struct stat_def	*c_stat_ptr;	/* Status block pointer		*/
	unsigned short 	c_active;	/* Command sent to controller	*/
	struct sb	*c_bind;	/* Associated SCSI block	*/
	struct ecb	*c_next;	/* Pointer to next ECB on ECB list	*/
	struct sg_array	*c_sg_p;	/* Pointers to the s/g list	*/
	int		c_ctlr;		/* adapter this command is on   */
#ifdef NOT_YET
	int		c_tm_id;		/* timeout id for untimeout	*/
#endif
	int	c_status;	/* Save area for ecb status */
	struct ecb	*c_donenext;	/* Chain of completed ECBs	*/
};

/*
 * macro for getting vaddr of an ecb from the paddr
 */
#define ADSE_ECB_PHYSTOKV(HA,PECB) \
	((HA)->ha_ecb + (int)((struct ecb *)(PECB) - (struct ecb *)(HA)->ha_pecb))

/*
 * Local flags for the ecb->flags variable
 */
#define	MY_TIMEOUT	0x1
#define	MY_SG_CMD	0x2


/*
 * structure used to obtain a pointer that allows me to index thru
 * alloc'd memory 1 page at a time
 */
struct adse_dma_page {
	uchar_t	d_page[PAGESIZE];
};

typedef struct adse_dma_page adse_page_t;

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
 * Here is the stuff pertaining to the scatter gather code that is done
 * locally in the driver.
 *****************************************************************************/
#define SG_FREE		0		/* set the s/g list free */
#define	SG_BUSY		1		/* keep track of s/g segment usage */
#define	SG_LENGTH	8		/* each s/g segment is 6 bytes long */
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
	paddr_t		addr;		/* address pointer RAN 4/2/92 */
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
#define ADSE_QUECLASS(x)	((x)->sbp->sb.sb_type)
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
	struct ecb		*q_func_cp;	/* sp that caused immediate functio to occur */
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru	*/
	pl_t		q_opri;		/* Saved priority level		*/
	bcb_t		*q_bcbp;	/* Device breakup control block */
	lock_t		*q_lock;	/* Device queue lock		*/
};


/*****************************************************************************
 * Host Adapter structure
 *****************************************************************************/
#define C_SANITY	0x8000

struct scsi_ha {
	unsigned short	ha_state;	  /* Operational state 		  */
	unsigned short	ha_id;		  /* Host adapter SCSI id	  */
	int		ha_vect;	  /* Interrupt vector number	  */
	int		ha_slot;	  /* Slot address		  */
	int		ha_npend;	  /* # of jobs sent to HA	  */
	struct ecb	*ha_free;	  /* pointer to the next free ecb */
	struct ecb	*ha_ecb;	  /* Controller command blocks	  */
	struct scsi_lu	*ha_dev;	  /* Logical unit queues	  */
	struct ecb	*ha_poll;	  /* pointer to the polled ecb    */
	paddr_t	ha_pecb;	/* physaddr of first ecb */
#ifndef PDI_SVR42
	lock_t		*ha_hba_lock;	  /* Lock for controller */
#endif
};

/*****************************************************************************
 * structures for the various command opcodes available
 *
 * The structure which defines the data to initialize the host adapter with
 * so the SCSI bus can be set as the user expects it.  This is supposed to
 * be done when the EISA config utility is used, but the Phoenix utility
 * does not support this information (free form data).  So this allows the
 * user to set the modes up for the SCSI bus via the driver.  This will
 * eventually be moved to the space.c file for user tuning. 6/26/92
 *****************************************************************************/
#define	SYNC_10		0x0
#define	SYNC_667	0x1
#define	SYNC_5		0x2
#define	SYNC_4		0x3
#define	SYNC_333	0x4

struct setting_def {
	unsigned short	reserv3 :2;
	unsigned short	dis_en :1;
	unsigned short	sync_en :1;
	unsigned short	reserv2 :1;
	unsigned short	parity :1;
	unsigned short	reserv1 :2;
	unsigned short	sync_rate :3;
	unsigned short	reserv :5;
};

/*****************************************************************************
 * Defination for the local adapter INITIALIZE SCSI DEFINATION command
 * FLAG word 1 bits - DI (disable interrupt), DSB (disable status block)
 *****************************************************************************/
struct init_scsi_def {
	short	opcode;			/* = INIT_SCSI_SYS;		*/
	short	flag1;
	long	res;			/* = 0;				*/
	struct setting_def init_data[16];
	long	config_len;
	paddr_t	status_ptr;		/* struct stat_def		*/
	char	resv[28];
};

/*****************************************************************************
 * Defination for the local adapter NO OPERATION command
 * FLAG word 1 bits - CNE (chain no error), DI (disable interrupt), DSB
 *	(disable status block)
 * FLAG word 2 bits - LUN (logical unit number)
 *****************************************************************************/
struct no_op_def {
	short	opcode;			/* = NO_OPERATION;		*/
	short	flag1;
	short	flag2;
	short	res;
	long	res1;
	long	res2;
	paddr_t	status_ptr;		/* struct stat_def		*/
	paddr_t	chain_ptr;
	char	res3[24];
};


/*****************************************************************************
 * Defination for the local adapter inquiry data
 * FLAG word 1 bits - DI (disable interrupt), DSB (disable status block)
 *	SES (suppress error on data underrun), ARS (automatic request sense
 *	disable)
 *****************************************************************************/
struct adapter_inq_def {
	short	opcode;			/* = READ_ADAP_INQ;		*/
	short	flag1;
	long	res;
	paddr_t	inq_ptr;		/* struct ad_inqdata_def	*/
	long	inq_len;
	paddr_t	status_ptr;
	char	res1[28];
};

struct ad_inqdata_def {
	unsigned short	dev_type :5;
	unsigned short	tmd :1;
	unsigned short	tms :1;
	unsigned short	res :9;	/* this defines the scsi dev type field */

	unsigned short	res2 :1;
	unsigned short	scsi2 :1;
	unsigned short	res1 :13;
	unsigned short	aen :1;	/* this defines the support level field */

	char	num_luns;
	char	additional_lng;

	unsigned char	res4 :2;
	unsigned char	dif :1;
	unsigned char	lnk :1;
	unsigned char	syn :1;
	unsigned char	wid :1;
	unsigned char	res3 :2;	/* this defines the flags field */

	char	num_cbs;
	char	vendor_ver[8];
	char	product_id[8];
	char	firmware_type[8];
	char	firmware_rev[4];
	char	release_date[8];
	char	release_time[8];
	short	firmware_chksum;
	short	res5;
	char	scsi_periphs[30];
	char	res6[160];
};

/**************************************************************************
 * Defines the local read sense information available from the host
 * adapter.
 * Valid bits for flag1 word 1 are SES (suppress error on underrun), and
 * DSB (disable status block).
 * Valid bits for flag word 2 are LUN (logical unit number), ND (no
 * disconnect), 
 **************************************************************************/
struct read_sense_def {
	short	opcode;			/* = READ_SENSE;		 */
	short	flag1;
	short	flag2;
	short	res;
	long	res1;
	long	res2;
	paddr_t status_ptr;
	long	res3;
	long	res4;
	paddr_t	sense_ptr;
	long	sense_len;
	char	res5[12];
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
 *	Macros to help code, maintain, etc.
 *****************************************************************************/
#define ADSE_SUBDEV(t,l)	((t << 3) | l)
#define ADSE_LU_Q(c,t,l)	adse_sc_ha[c].ha_dev[ADSE_SUBDEV(t,l)]
#define	FREEECB(x)	x->c_active = FALSE

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

#define ADSE_DMALIST_LOCK(p) p = LOCK(adse_dmalist_lock, pldisk)
#define ADSE_SGARRAY_LOCK(p) p = LOCK(adse_sgarray_lock, pldisk)
#define ADSE_ECB_LOCK(p) p = LOCK(adse_ecb_lock, pldisk)
#define ADSE_HBA_LOCK(p) p = LOCK(ha->ha_hba_lock, pldisk)
#define ADSE_SCSILU_LOCK(p) p = LOCK(q->q_lock, pldisk)
#ifdef DEBUG
#define ADSE_SCSILU_IS_LOCKED ((q->q_lock)->sp_lock)
#endif

#define ADSE_DMALIST_UNLOCK(p) UNLOCK(adse_dmalist_lock, p)
#define ADSE_SGARRAY_UNLOCK(p) UNLOCK(adse_sgarray_lock, p)
#define ADSE_ECB_UNLOCK(p) UNLOCK(adse_ecb_lock, p)
#define ADSE_HBA_UNLOCK(p) UNLOCK(ha->ha_hba_lock, p)
#define ADSE_SCSILU_UNLOCK(p) UNLOCK(q->q_lock, p)

#define ADSE_SET_QFLAG(q,flag) { \
					ADSE_SCSILU_LOCK(q->q_opri); \
					q->q_flag |= flag; \
					ADSE_SCSILU_UNLOCK(q->q_opri); \
				}
#define ADSE_CLEAR_QFLAG(q,flag) { \
					ADSE_SCSILU_LOCK(q->q_opri); \
					q->q_flag &= ~flag; \
					ADSE_SCSILU_UNLOCK(q->q_opri); \
				}

#ifndef PDI_SVR42
/*
 * Locking Hierarchy Definition
 */
#define ADSE_HIER	HBA_HIER_BASE	/* Locking hierarchy base for hba */

#endif /* !PDI_SVR42 */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_HBA_ADSE_ADSE_H */

#ifndef _IO_HBA_DPT_H	/* wrapper symbol for kernel use */
#define _IO_HBA_DPT_H	/* subject to change without notice */

#ident	"@(#)kern-pdi:io/hba/dpt/dpt.h	1.25.3.5"
#ident  "$Header$"

#if defined(__cplusplus)
extern "C" {
#endif
	
/*	Copyright (c) 1988, 1989  Intel Corporation		*/ 
/*	All Rights Reserved					*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION		*/

/* This software is supplied under the terms of a license agreement */ 
/* or nondisclosure agreement with Intel Corporation and may not be */ 
/* copied or disclosed except in accordance with the terms of that  */ 
/* agreement.							*/

/* Copyright (c) 1990 Distributed Processing Technology		*/ 
/*	All Rights Reserved					*/
	
#define HBA_PREFIX dpt
	
/************************************************************************* 
**									** 
**  The host adapter major/minor device numbers are defined as		** 
**									** 
**	MAJOR	MINOR							** 
**	| MMMMMMMM | nnndddss |						** 
**									** 
**	M ==> Assigned by idinstall (major device number).		** 
**	n ==> Relative Host Adapter number.		(0-7) 		** 
**	d ==> target device ID, Drive or Bridge controller ID.(0-7) 	** 
**	s ==> sub device LUN, for bridge controllers.	(0-3)		** 
*************************************************************************/

#define MINOR_DEV(dev)  geteminor(dev)
	
/************************************************************************* 
**		General Implementation Definitions			** 
*************************************************************************/ 
#define MAX_EQ  (MAX_BUS * MAX_EXTCS * MAX_EXLUS) /* Max equipage per HBA */ 
#define NDMA	12		/* Number of DMA lists	*/
#define NCPS	64		/* Number of command packets    */ 
#define SCSI_ID	7		/* Default ID of controllers    */
	
#define ONE_MSEC	1
#define ONE_SEC	1000
#define ONE_MIN	60000
	
#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif
#define BYTE   unsigned char
	
#define DMA0_3MD	0x000B	/* 8237A DMA Mode Reg (0-3)	*/ 
#define DMA4_7MD	0x00D6	/* 8237A DMA Mode Reg (4-7)	*/ 
#define CASCADE_DMA	0x00C0	/* Puts DMA in Cascade Mode	*/ 
#define DMA0_3MK	0x000A	/* 8237A DMA mask register	*/ 
#define DMA4_7MK	0x00D4	/* 8237A DMA mask register	*/
	
#define DPT_PRIMARY	0x1f0	/* DPT primary base address	*/ 
#define DPT_SECONDARY   0x170	/* DPT secondary base address   */ 
#define DPT_UNDEFINED   0xfff	/* DPT offset base address undef*/ 
#define DPT_IOADDR_MASK 0xfff	/* DPT offset mask		*/

#define DPT_MEMALIGN 4
#define DPT_BOUNDARY 0

/************************************************************************* 
**		Controller IO Register Offset Definitions		** 
*************************************************************************/ 
#define HA_COMMAND	0x07	/* Command register		*/ 
#define HA_STATUS	0x07	/* Status register		*/ 
#define HA_DMA_BASE	0x02	/* LSB for DMA Physical Address */ 
#define HA_ERROR	0x01	/* Error register		*/ 
#define HA_DATA		0x00	/* Data In/Out register	*/ 
#define HA_AUX_STATUS   0x08	/* Auxiliary Status register    */
	
#define HA_AUX_BUSY	0x01	/* Aux Reg Busy bit.	*/ 
#define HA_AUX_INTR	0x02	/* Aux Reg Interrupt Pending.   */
	
#define HA_ST_ERROR	0x01	/* HA_STATUS register bit defs  */ 
#define HA_ST_INDEX	0x02
#define HA_ST_CORRCTD   0x04
#define HA_ST_DRQ	0x08
#define HA_ST_SEEK_COMP 0x10
#define HA_ST_WRT_FLT   0x20
#define HA_ST_READY	0x40
#define HA_ST_BUSY	0x80
	
#define HA_ER_DAM	0x01	/* HA_ERROR register bit defs   */ 
#define HA_ER_TRK_0	0x02
#define HA_ER_ABORT	0x04
#define HA_ER_ID	0x10
#define HA_ER_DATA_ECC  0x40
#define HA_ER_BAD_BLOCK 0x80
	
/************************************************************************* 
**		Controller Commands Definitions				** 
*************************************************************************/ 
#define CP_READ_CFG_PIO 0xF0	/* Read Configuration Data, PIO */
#define CP_PIO_CMD	0xF2	/* Execute Command, PIO	*/
#define CP_TRUCATE_CMD  0xF4	/* Trunc Transfer Command, PIO  */
#define CP_EATA_RESET   0xF9	/* Reset Controller And SCSI Bus*/
#define CP_IMMEDIATE    0xFA	/* EATA Immediate command	*/
#define CP_READ_CFG_DMA 0xFD	/* Read Configuration Data, DMA */ 
#define CP_DMA_CMD	0xFF	/* Execute Command, DMA	*/
#define ECS_EMULATE_SEN 0xD4	/* Get Emmulation Sense Data    */
	
#define CP_ABORT_MSG    0x06
	
/************************************************************************* 
**		EATA Command/Status Packet Definitions			** 
*************************************************************************/ 
#define HA_DATA_IN	0x80
#define HA_DATA_OUT	0x40
#define HA_SCATTER	0x08
#define HA_AUTO_REQ_SEN  0x04
#define HA_HBA_INIT	0x02
#define HA_SCSI_RESET    0x01
	
#define HA_ERR_SELTO	0x01	/* Device Selection Time Out    */
#define HA_ERR_CMDTO	0x02	/* Device Command Time Out	*/
#define HA_ERR_RESET	0x03	/* SCSI Bus was RESET !	*/
#define HA_ERR_INITPWR   0x04	/* Initial Controller Power Up  */
#define HA_ERR_UBUSPHASE 0x05	/* Unexpected BUS Phase	*/
#define HA_ERR_UBUSFREE  0x06	/* Unexpected BUS Free	*/
#define HA_ERR_BUSPARITY 0x07	/* SCSI Bus Parity Error	*/
#define HA_ERR_SHUNG	0x08	/* SCSI Bus Hung		*/
#define HA_ERR_UMSGRJCT  0x09	/* Unexpected Message Reject    */
#define HA_ERR_RSTSTUCK  0x0A	/* SCSI Bus Reset Stuck	*/
#define HA_ERR_RSENSFAIL 0x0B	/* Auto-Request Sense Failed    */
#define HA_ERR_PARITY    0x0C	/* HBA Memory Parity error	*/
#define HA_ERR_CPABRTNA  0x0D	/* CP aborted - NOT on Bus	*/
#define HA_ERR_CPABORTED 0x0E	/* CP aborted - WAS on Bus	*/
#define HA_ERR_CPRST_NA  0x0F	/* CP was reset - NOT on Bus    */
#define HA_ERR_CPRESET   0x10	/* CP was reset - WAS on Bus    */
	
	
#define HA_STATUS_MASK  0x7F
#define HA_IDENTIFY_MSG 0x80
#define HA_DISCO_RECO   0x40	/* Disconnect/Reconnect	*/
	
#define SUCCESS	0x01	/* Successfully completed	*/ 
#define FAILURE	0x02	/* Completed with error	*/ 
#define START	0x01	/* Start the CP command	*/ 
#define ABORT	0x02	/* Abort the CP command	*/
	
#define DPT_INTR_OFF    0x00	/* Interrupts disabled	*/ 
#define DPT_INTR_ON	0x01	/* Interrupts enabled	*/
	
#define COMMAND_PENDING		1
#define COMMAND_COMPLETE	0
#define COMMAND_RETRY		-1
#define COMMAND_TIMEOUT		-2
#define COMMAND_ERROR		-3


/*
 * Host Adapter structure
 */
struct dpt_scsi_ha {
	uint	ha_state;		/* Operational state	*/
	BYTE	ha_id[4];		/* Host adapter SCSI ids	*/
	int	ha_vect;		/* Interrupt vector number	*/
	ulong_t ha_base;		/* Base I/O address		*/
	int	ha_max_jobs;		/* Max number of Active Jobs    */
	int	ha_cache:2;		/* Cache parameters		*/
	int	ha_cachesize:30;	/* In meg, only if cache present*/
	int	ha_nbus;		/* Number Of Busses on HBA	*/
	int	ha_ntargets;		/* Number Of Targets Supported  */
	int	ha_nluns;		/* Number Of LUNs Supported	*/
	int	ha_tshift;		/* Shift value for target	*/
	int	ha_bshift;		/* Shift value for bus	*/
	int	ha_npend;		/* # of jobs sent to HA	*/
	int	ha_active_jobs;		/* Number Of Active Jobs	*/
	char	ha_fw_version[4];	/* Firmware Revision Level	*/
	struct dpt_ccb * ha_cblist;	/* Command block free list	*/
	lock_t * ha_ccb_lock;		/* CCB Lock			*/
	struct dpt_scsi_lu * *ha_dev;	/* Logical unit queues	*/
	lock_t * ha_StPkt_lock;		/* Status Packet Lock	*/
	struct dpt_scsi_lu * ha_LuQWaiting;	/* Lu Queue Waiting List */
	lock_t	*ha_QWait_lock;		/* Device Que Waiting Lock	*/
	pl_t	ha_QWait_opri;		/* Saved Priority Level	*/
#ifdef DPT_TIMEOUT_RESET_SUPPORT
	lock_t * ha_watchdog_lock;	/* Lock for timeout srb lists */
	int	ha_flags;
	int	ha_watchdog_id;
	struct dpt_srb * ha_actv_jobs;	/* Active jobs being monitored   */
					/* the watchdog timer.	*/
	struct dpt_srb * ha_actv_last;	/* Last job on active last	*/
	ushort	ha_quantum;		/* Quantum until last job times out */
#endif /** DPT_TIMEOUT_RESET_SUPPORT **/

#ifdef DPT_TARGET_MODE
	struct dpt_ccb *ha_tgCcb[4];	/* Command block waiting writebuf */
#endif /* DPT_TARGET_MODE */

	physreq_t 	*ha_physreq;	/* physreq */
	struct Status_Packet *sc_hastat; /* Ptr To SCSI HA Status Packets */
	HBA_IDATA_STRUCT *ha_idata;	/* Ptr to IDATA struct */
	int		ha_cntlr;
	char		*ha_name;
	rm_key_t	ha_rmkey;	/* autoconfig resource manager key */
	boolean_t	ha_waitflag;	/* Using dpt_wait */
	int		ha_cmd_in_progress;
	int		ha_bus_type;

	lock_t 	*ha_eata_lock;
	sv_t	*ha_pause_sv;
	sv_t	*ha_eata_sv;
};

typedef struct dpt_scsi_ha scsi_ha_t;

#define  CNFNULL ((struct dpt_scsi_ha *) 0)
/*******************************************************************************
** ReadConfig data structure - this structure contains the EATA Configuration **
*******************************************************************************/
#define HBA_BUS_TYPE_LENGTH    32	/* Length For Bus Type Field    */ 

struct RdConfig {
	BYTE ConfigLength[4];	/* Len in bytes after this field.	*/
	BYTE EATAsignature[4];	/* EATA Signature Field		*/
	BYTE EATAversion;	/* EATA Version Number		*/
	BYTE OverLapCmds:1;	/* TRUE if overlapped cmds supported  */
	BYTE TargetMode:1;	/* TRUE if target mode supported	*/
	BYTE TrunNotNec:1;
	BYTE MoreSupported:1;
	BYTE DMAsupported:1;	/* TRUE if DMA Mode Supported	*/
	BYTE DMAChannelValid:1;	/* TRUE if DMA Channel Valid.	*/
	BYTE ATAdevice:1;
	BYTE HBAvalid:1;	/* TRUE if HBA field is valid	*/
	BYTE PadLength[2];
	BYTE Reserved0;		/* Reserved field			*/
	BYTE Chan_2_ID;		/* Channel 2 Host Adapter ID	*/
	BYTE Chan_1_ID;		/* Channel 1 Host Adapter ID	*/
	BYTE Chan_0_ID;		/* Channel 0 Host Adapter ID	*/
	BYTE CPlength[4];	/* Command Packet Length		*/
	BYTE SPlength[4];	/* Status Packet Length		*/
	BYTE QueueSize[2];	/* Controller Que depth		*/
	BYTE Reserved1[2];	/* Reserved field			*/
	BYTE SG_Size[2];	/* Number Of S. G. Elements Supported */
	BYTE IRQ_Number:4;	/* IRQ Ctlr is on ... ie 14,15,12	*/
	BYTE IRQ_Trigger:1;	/* 0 =Edge Trigger, 1 =Level Trigger  */
	BYTE Secondary:1;	/* TRUE if ctlr not parked on 0x1F0   */
	BYTE DMA_Channel:2;	/* DMA Channel used if PM2011	*/
	BYTE Reserved2;		/* Reserved Field			*/
	BYTE Disable:1;		/* Secondary I/O Address Disabled	*/
	BYTE ForceAddr:1;	/* PCI Forced To EISA/ISA Address	*/
	BYTE Reserved3:6;	/* Reserved Field			*/
	BYTE MaxScsiID:5;	/* Maximun SCSI Target ID Supported   */
	BYTE MaxChannel:3;	/* Maximum Channel Number Supported   */
	BYTE MaxLUN;		/* Maximum LUN Supported		*/
	BYTE Reserved4:6;	/* Reserved Field			*/
	BYTE PCIbus:1;		/* PCI Adapter Flag		*/
	BYTE EISAbus:1;		/* EISA Adapter Flag		*/
	BYTE RaidNum;		/* Raid Host Adapter Number	*/
	BYTE Reserved5;		/* Reserved Field			*/
	BYTE pad[512 - 38];
};

struct cp_bits {
	BYTE SReset:1;
	BYTE HBAInit:1;
	BYTE ReqSen:1;
	BYTE Scatter:1;
	BYTE Resrvd:1;
	BYTE Interpret:1;
	BYTE DataOut:1;
	BYTE DataIn:1;
};

typedef struct Status_Packet {

	BYTE    SP_Controller_Status;
	BYTE    SP_SCSI_Status;
	BYTE	SP_Buf_len[2];		/* length of incoming write buffer*/
	BYTE    SP_Inv_Residue[4];

	union {
		struct dpt_ccb *vp;	/* Command Packet Vir Address.    */
		BYTE	va[4];		/* Command Packet Other Info.   */
	} CPaddr;

	BYTE    SP_ID_Message;
	BYTE    SP_Que_Message;
	BYTE    SP_Tag_Message;
	BYTE    SP_Messages[5];
	BYTE	SP_Buf_off[4];		/* offset of incoming write buffer*/
} scsi_stat_t;

typedef void (*ccb_callback)(struct dpt_scsi_ha *, struct dpt_ccb *);

/*
 * Controller Command Block
 */
struct dpt_ccb {

	union {				/*** EATA Packet sent to ctlr ***/
		struct cp_bits bit;	/*				*/
		unsigned char	byte;	/*  Operation Control bits.	*/
	} CPop;

	BYTE ReqLen;		/*  Request Sense Length.	*/
	BYTE Unused[3];		/*  Reserved		*/
	BYTE CPfwnest:1;	/*  Firmwawe Nested Bit	*/
	BYTE CPresv1:7;		/*  Reserved		*/
	BYTE CPphsunit:1;	/*  Physical Bit		*/
	BYTE CPiat:1;		/*  Inhibit Address translation */
	BYTE CPhbaci:1;		/*  Inhibit Caching		*/
	BYTE CPresv2:5;		/*  Reserved		*/
	BYTE CPID:5;		/*  Target SCSI ID		*/
	BYTE CPbus:3;		/*  Channel Number		*/
	BYTE CPmsg0;		/*  Identify/DiscoReco... Msg   */
	BYTE CPmsg1;		/*				*/
	BYTE CPmsg2;		/*				*/
	BYTE CPmsg3;		/*				*/
	BYTE CPcdb[12];		/*  Embedded SCSI CDB.	*/

	ulong CPdataLen;	/*  Transfer Length.	*/

	union {
		struct dpt_ccb *vp;	/*  Command Packet Vir Address. */
		BYTE	va[4];	/*  Command Packet Other Info.  */
	} CPaddr;

	paddr_t CPdataDMA;	/*  Data Physical Address.	*/
	paddr_t CPstatDMA;	/*  Status Packet Phy Address.  */
	paddr_t CP_ReqDMA;	/*  ReqSense Data Phy Address.  */

	BYTE CP_OpCode;
	BYTE CP_ha_status;	/* command status */
	scsi_stat_t CP_status;	/* command status packet */
	struct sense CP_sense;	/* Sense data		*/
	ccb_callback c_callback; /* Callback Routine	*/
	struct	dpt_ccb *c_vp;	/* Callback VCP		*/
	paddr_t c_addr;		/* CB physical address	*/
	int	c_active;	/* Command sent to controller   */
	time_t  c_time;		/* Timeout count (msecs)	*/
	struct	sb *c_bind;	/* Associated SCSI block	*/
#if (defined(DPT_RAID_0))
	void * s_dmap;		/* DMA scatter/gather list	*/
	caddr_t SG;		/* Virtual SG pointer (unallocated) */
#endif
	struct dpt_ccb *c_next;	/* Pointer to next free CB	*/
};
	
typedef struct dpt_ccb ccb_t;

struct EATACommandPacket {

	union {
		struct cp_bits bit;
		unsigned char	byte;
	} CPop;

	BYTE    ReqLen;
	BYTE    Unused[3];		/*  Reserved		*/
	BYTE    CPfwnest:1;		/*  Firmwawe Nested Bit	*/
	BYTE    CPresv1:7;		/*  Reserved		*/
	BYTE    CPphsunit:1;		/*  Physical Bit		*/
	BYTE    CPiat:1;		/*  Inhibit Address translation */
	BYTE    CPhbaci:1;		/*  Inhibit Caching		*/
	BYTE    CPresv2:5;		/*  Reserved		*/
	BYTE    CPID:5;    
	BYTE    CPbus:3; 
	BYTE    CPmsg0;
	BYTE    CPmsg1;
	BYTE    CPmsg2;
	BYTE    CPmsg3;
	BYTE    CPcdb[12];

	ulong   CPdataLen;   

	union {
		ccb_t *vp;		/* Command Packet Vir Address.  */
		BYTE	va[4];		/* Command Packet Other Info.   */
	} CPaddr;

	paddr_t CPdataDMA;
	paddr_t CPstatDMA;
	paddr_t CP_ReqDMA;
};

typedef struct EATACommandPacket EataCP_t;

#define MAX_CMDSZ	12

#define NO_ERROR	0x00	/* No adapter detected error    */ 
#define NO_SELECT	0x11	/* Selection time out	*/ 
#define TC_PROTO	0x14	/* TC protocol error	*/
	
#define MAX_DMASZ	32
#define pgbnd(a)	(ptob(1) - ((ptob(1) - 1) & (int)(a)))

/*
 * SCSI Request Block structure
 */
struct dpt_srb {
	struct xsb *sbp;		/* Target drv definition of SB  */
	struct dpt_srb *s_next;		/* Next block on LU queue	*/
	struct dpt_srb *s_priv;		/* Private ptr for dynamic alloc*/
	struct dpt_srb *s_prev;		/* Previous block on LU queue   */
	scgth_t *s_scgth;		/* DMA scatter/gather list	*/
	paddr_t	s_addr;			/* Physical data pointer	*/
	int	s_len;			/* data length or no of sg elems */
	BYTE	s_CPopCtrl;		/* Additional Control info	*/
#ifdef DPT_TIMEOUT_RESET_SUPPORT
	struct dpt_srb	*s_job_next;	/* Watchdog active job list 	*/
	struct dpt_srb	*s_job_prev;	/* Watchdog active job list 	*/
	ushort	s_time_quantum;		/* This is the maximum number 	*/
					/* of DPT_TIME_QUANTUM units	*/
					/* this command can take before	*/
					/* we software time it out.	*/
	ushort	s_flags;		/* Misc job related flags, see	*/
					/* below for description.	*/
	ccb_t	*s_cp;			/* Point to CCB			*/
#endif /** DPT_TIMEOUT_RESET_SUPPORT **/
};

typedef struct dpt_srb sblk_t;
	
#ifdef DPT_TIMEOUT_RESET_SUPPORT
#define SB_BEING_ABORTED	0x0001	/* This job is in the process   */
					/* of being aborted.	*/
#define SB_CALLED_BACK		0x0002	/* Callback was done for this   */
					/* job.			*/
#define SB_PROCESS		0x0004	/* This job was processed in current */
					/* watchdog call.		*/
#define SB_MARK			0x0008	/* For debugging purposes only  */

#define DPT_TIMEOUT_ON	0x0001
#define DPT_TIME_QUANTUM   5

#define DPT_WATCHDOG_LOCK(ha,p)	p = LOCK(ha->ha_watchdog_lock, pldisk)
#define DPT_WATCHDOG_UNLOCK(ha,p)	UNLOCK(ha->ha_watchdog_lock, p)
#endif /** DPT_TIMEOUT_RESET_SUPPORT **/

/*
 * Logical Unit Queue structure
 */
struct dpt_scsi_lu {
	struct dpt_srb	*q_first;	/* First block on LU queue	*/
	struct dpt_srb	*q_last;	/* Last block on LU queue	*/
	int		q_flag;		/* LU queue state flags	*/
	struct sense	q_sense;	/* Sense data		*/
	int		q_count;	/* Outstanding job counter	*/
	void		(*q_func)();    /* Target driver event handler  */
	long		q_param;	/* Target driver event param    */
	ushort		q_active;	/* Number of concurrent jobs    */
	pl_t		q_opri;		/* Saved Priority Level	*/
	unsigned int	q_addr;		/* Last read/write logical addr */
	char		*q_sc_cmd;	/* SCSI cmd for pass-thru	*/
	lock_t		*q_lock;	/* Device Que Lock		*/
	struct dpt_scsi_lu *q_WaitingNext; /* Queue Waiting Pointer */
};

typedef struct dpt_scsi_lu scsi_lu_t;

#define DPT_QBUSY	0x01
#define DPT_QWAIT	0x02
#define DPT_QSUSP	0x04
#define DPT_QSENSE	0x08	/* Sense data cache valid */ 
#define DPT_QPTHRU	0x10
#define DPT_QHBA	0x20
	
#define QCLASS(x)   ((x)->sbp->sb.sb_type) 

#define DPT_QNORM	SCB_TYPE

typedef struct dpt_pause_func_arg {
                scsi_ha_t *ha;
                scsi_lu_t *q;
} dpt_pause_func_arg_t;

/*
 * Valid values for ha_cache
 */
#define DPT_NO_CACHE	0
#define DPT_CACHE_WRITETHROUGH 1
#define DPT_CACHE_WRITEBACK    2

#define C_ISA		0x0001	/* DPT card is ISA	*/ 
#define C_EISA_PCI	0x0002	/* DPT card is EISA	*/ 
#define C_SUSPENDED	0x0010	/* controller is suspended */
#define C_REMOVED	0x0020	/* controller is removed */
#define C_SANITY	0x8000

#ifndef SDI_386_EISA
#define SDI_386_EISA    0x08
#endif

/*
 * Macros to help code, maintain, etc. 
 */
#define TGTVAL(ha,t)		((t) << ha->ha_tshift)
#define BUSVAL(ha,b)		((b) << ha->ha_bshift)
#define DEVOFF(ha,b,t,l)	(BUSVAL(ha,b) | TGTVAL(ha,t) | (l))
#define LU_Q(ha,b,t,l)		ha->ha_dev[DEVOFF(ha,b,t,l)]
/*
 * DPT EISA board addressing
 */
#define DPT_EISA_BASE	0xC80
#define SLOT_ID_ADDR(s)	((s) * 0x1000 + DPT_EISA_BASE) 
#define SLOT_BASE_IO_ADDR(s) (SLOT_ID_ADDR(s) + 8)

/*
 * DPT Specific PCI I/O Address Adjustment 
 */
#define DPT_PCI_OFFSET  0x10

/*
 * DPT EISA board id defined as follows: 
 * e.g. 0x1214= 0 00100 10000 10100
 *		D	P	T
 */
#define DPT_EISA_ID1 0x12
#define DPT_EISA_ID2 0x14

/*
 * Other varieties of DPT board ids.
 */
#define DPT_EISA_ATT_ID1 0x06
#define DPT_EISA_ATT_ID2 0x94
#define DPT_EISA_ATT_ID3 0x0

#define DPT_EISA_NEC_ID1 0x38
#define DPT_EISA_NEC_ID2 0xA3
#define DPT_EISA_NEC_ID3 0x82

#define MAX_EISA_SLOTS   255
#define DPT_NVARIETIES   3

struct dpt_cfg {
	unsigned char	cf_irq;		/* Interrupt vector		*/
	unsigned char	cf_itype;	/* Edge/Level interrupt type    */
	unsigned short	cf_daddr;	/* Board I/O base address	*/
	unsigned short	cf_eaddr;	/* EISA Board I/O base address  */
	unsigned short	cf_bdtype;	/* Board type (should be C_EISA)*/
	short	cf_idata;		/* idata struct matching entry  */
};

/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primatives for multi-processor 
 * or spl/splx for uniprocessor.
 */
#define DPT_CCB_LOCK(ha,p)	p = LOCK(ha->ha_ccb_lock, pldisk)
#define DPT_STPKT_LOCK(ha,p)	p = LOCK(ha->ha_StPkt_lock, pldisk) 
#define DPT_QWAIT_LOCK(ha,p)	p = LOCK(ha->ha_QWait_lock, pldisk) 
#define DPT_EATA_LOCK(ha,p)	p = LOCK(ha->ha_eata_lock, pldisk) 
#define DPT_SCSILU_LOCK(q)	q->q_opri = LOCK(q->q_lock, pldisk)

#define DPT_CCB_UNLOCK(ha,p)	UNLOCK(ha->ha_ccb_lock, p)
#define DPT_STPKT_UNLOCK(ha,p)	UNLOCK(ha->ha_StPkt_lock, p) 
#define DPT_QWAIT_UNLOCK(ha,p)	UNLOCK(ha->ha_QWait_lock, p) 
#define DPT_EATA_UNLOCK(ha,p)	UNLOCK(ha->ha_eata_lock, p) 
#define DPT_SCSILU_UNLOCK(q)	UNLOCK(q->q_lock, q->q_opri)

/*
 * Locking Hierarchy Definition
 */
#define DPT_HIER HBA_HIER_BASE	/* Locking hierarchy base for hba */

struct EATA_PassThrough {
	char	EataID[4];
	uint	EataCmd;
	BYTE	* CmdBuffer;
	EataCP_t  EataCP; 
	ulong	TimeOut ;
	BYTE	HostStatus;
	BYTE	TargetStatus;
	BYTE	Retries; 
};

typedef struct EATA_PassThrough EataPassThru_t;

/*
 * EATA Driver IOCTL interface command Definitions
 */
#define EATAUSRCMD	(('D'<<8)|65)	/* EATA PassThrough Command   */
#define DPT_SIGNATURE   (('D'<<8)|67)	/* Get Signature Structure    */
#define DPT_NUMCTRLS    (('D'<<8)|68)	/* Get Number Of DPT Adapters */
#define DPT_CTRLINFO    (('D'<<8)|69)	/* Get Adapter Info Structure */ 
#define DPT_SYSINFO	(('D'<<8)|72)	/* Get System Info Structure  */
#define DPT_BLINKLED	(('D'<<8)|75)	/* Get The BlinkLED Status    */

#define SI_CMOS_Valid		0x0001
#define SI_NumDrivesValid	0x0002
#define SI_ProcessorValid	0x0004
#define SI_MemorySizeValid	0x0008
#define SI_DriveParamsValid	0x0010
#define SI_SmartROMverValid	0x0020
#define SI_OSversionValid	0x0040
#define SI_OSspecificValid	0x0080	
#define SI_BusTypeValid	0x0100

#define SI_ALL_VALID	0x0FFF
#define SI_NO_SmartROM	0x8000

#define SI_ISA_BUS		0x00
#define SI_MCA_BUS		0x01
#define SI_EISA_BUS		0x02
#define SI_PCI_BUS		0x04

#define HBA_BUS_ISA		0x00
#define HBA_BUS_EISA		0x01
#define HBA_BUS_PCI		0x02

struct driveParam_S {
	ushort   	cylinders;	/* Upto 1024		*/
	unsigned char	heads;		/* Upto 255		*/
	unsigned char	sectors;	/* Upto 63		*/
};

typedef struct driveParam_S driveParam_T;

struct sysInfo_S {
	unsigned char	drive0CMOS;		/* CMOS Drive 0 Type	*/
	unsigned char	drive1CMOS;		/* CMOS Drive 1 Type	*/
	unsigned char	numDrives;		/* 0040:0075 contents	*/
	unsigned char	processorFamily;	/* Same as DPTSIG definition  */
	unsigned char	processorType;		/* Same as DPTSIG definition  */
	unsigned char	smartROMMajorVersion;	/*			*/
	unsigned char	smartROMMinorVersion;	/* SmartROM version	*/
	unsigned char	smartROMRevision;	/*			*/
	ushort	flags;				/* See bit definitions above  */
	ushort	conventionalMemSize;		/* in KB		*/
	ulong	extendedMemSize;		/* in KB		*/
	ulong	osType;				/* Same as DPTSIG definition  */
	unsigned char	osMajorVersion;		/*			*/
	unsigned char	osMinorVersion;		/* The OS version	*/
	unsigned char	osRevision;		/*			*/
	unsigned char	osSubRevision;		/*			*/
	unsigned char	busType;		/* See defininitions above    */
	unsigned char	pad[3];			/* For alignment	*/
	driveParam_T    drives[16];		/* SmartROM Logical Drives    */
};

typedef struct sysInfo_S sysInfo_T;

#define MULTIFUNCTION_CMD	0x0e	/* SCSI Multi Function Cmd    */
#define BUS_QUIET		0x04	/* Quite Scsi Bus Code	*/
#define BUS_UNQUIET		0x05	/* Un Quiet Scsi Bus Code */

/* DPT SIGNATURE SPEC AND HEADER FILE			*/
/* Signature Version 1 (sorry no 'A')			*/

typedef unsigned char	sigBYTE;
typedef unsigned short	sigWORD;
typedef unsigned long	sigLONG;

/* must make sure the structure is not word or double-word aligned	*/
/* ---------------------------------------------------------------	*/
/* Borland will ignore the following pragma:				*/
/* Word alignment is OFF by default.  If in the, IDE make		*/
/* sure that Options | Compiler | Code Generation | Word Alignment	*/
/* is not checked.  If using BCC, do not use the -a option.		*/

#pragma PACK_ON

/* Current Signature Version - sigBYTE dsSigVersion; */
/* ------------------------------------------------------------------ */
#define SIG_VERSION 1

/* Processor Family - sigBYTE dsProcessorFamily;  DISTINCT VALUES */
/* ------------------------------------------------------------------ */
/* What type of processor the file is meant to run on. */
/* This will let us know whether to read sigWORDs as high/low or low/high. */
#define PROC_INTEL	0x00    /* Intel 80x86 */
#define PROC_MOTOROLA   0x01    /* Motorola 68K */
#define PROC_MIPS4000   0x02    /* MIPS RISC 4000 */
#define PROC_ALPHA	0x03    /* DEC Alpha */

/* Specific Minimim Processor - sigBYTE dsProcessor;    FLAG BITS */
/* ------------------------------------------------------------------ */
/* Different bit definitions dependent on processor_family */

/* PROC_INTEL: */
#define PROC_8086	0x01    /* Intel 8086 */
#define PROC_286	0x02    /* Intel 80286 */
#define PROC_386	0x04    /* Intel 80386 */
#define PROC_486	0x08    /* Intel 80486 */
#define PROC_PENTIUM    0x10    /* Intel 586 aka P5 aka Pentium */
#define PROC_P6		0x20	/* Intel 686 aka P6 */

/* PROC_MOTOROLA: */
#define PROC_68000	0x01    /* Motorola 68000 */
#define PROC_68020	0x02    /* Motorola 68020 */
#define PROC_68030	0x04    /* Motorola 68030 */
#define PROC_68040	0x08    /* Motorola 68040 */

/* Filetype - sigBYTE dsFiletype;	DISTINCT VALUES */
/* ------------------------------------------------------------------ */
#define FT_EXECUTABLE   0	/* Executable Program */
#define FT_SCRIPT	1	/* Script/Batch File??? */
#define FT_HBADRVR	2	/* HBA Driver */
#define FT_OTHERDRVR    3	/* Other Driver */
#define FT_IFS		4	/* Installable Filesystem Driver */
#define FT_ENGINE	5	/* DPT Engine */
#define FT_COMPDRVR	6	/* Compressed Driver Disk */
#define FT_LANGUAGE	7	/* Foreign Language file */
#define FT_FIRMWARE	8	/* Downloadable or actual Firmware */
#define FT_COMMMODL	9	/* Communications Module */
#define FT_INT13	10	/* INT 13 style HBA Driver */
#define FT_HELPFILE	11	/* Help file */
#define FT_LOGGER	12	/* Event Logger */
#define FT_INSTALL	13	/* An Install Program */
#define FT_LIBRARY	14	/* Storage Manager Real-Mode Calls */
#define FT_RESOURCE	15 /* Storage Manager Resource File */
#define FT_MODEM_DB	16  /* Storage Manager Modem Database */

/* Filetype flags - sigBYTE dsFiletypeFlags;    FLAG BITS */
/* ------------------------------------------------------------------ */
#define FTF_DLL		0x01    /* Dynamic Link Library */
#define FTF_NLM		0x02    /* Netware Loadable Module */
#define FTF_OVERLAYS    0x04    /* Uses overlays */
#define FTF_DEBUG	0x08    /* Debug version */
#define FTF_TSR		0x10    /* TSR */
#define FTF_SYS		0x20    /* DOS Lodable driver */
#define FTF_PROTECTED   0x40    /* Runs in protected mode */
#define FTF_APP_SPEC    0x80    /* Application Specific */

/* OEM - sigBYTE dsOEM;	DISTINCT VALUES */
/* ------------------------------------------------------------------ */
#define OEM_DPT		0	/* DPT */
#define OEM_ATT		1	/* ATT */
#define OEM_NEC		2	/* NEC */
#define OEM_ALPHA	3	/* Alphatronix */
#define OEM_AST		4	/* AST */
#define OEM_OLIVETTI    5	/* Olivetti */
#define OEM_SNI		6	/* Siemens/Nixdorf */

/* Operating System  - sigLONG dsOS;    FLAG BITS */
/* ------------------------------------------------------------------ */
#define OS_DOS		0x00000001	/* PC/MS-DOS */
#define OS_WINDOWS	0x00000002	/* Microsoft Windows 3.x */
#define OS_WINDOWS_NT   0x00000004	/* Microsoft Windows NT */
#define OS_OS2M		0x00000008	/* OS/2 1.2.x,MS 1.3.0,IBM 1.3.x - Monolithic */
#define OS_OS2L		0x00000010	/* Microsoft OS/2 1.301 - LADDR */
#define OS_OS22x	0x00000020	/* IBM OS/2 2.x */
#define OS_NW286	0x00000040	/* Novell NetWare 286 */
#define OS_NW386	0x00000080	/* Novell NetWare 386 */
#define OS_GEN_UNIX	0x00000100	/* Generic Unix */
#define OS_SCO_UNIX	0x00000200	/* SCO Unix */
#define OS_ATT_UNIX	0x00000400	/* ATT Unix */
#define OS_UNIXWARE	0x00000800	/* UnixWare Unix */
#define OS_INT_UNIX	0x00001000	/* Interactive Unix */
#define OS_SOLARIS	0x00002000	/* SunSoft Solaris */
#define OS_QNX		0x00004000	/* QNX for Tom Moch */
#define OS_NEXTSTEP	0x00008000	/* NeXTSTEP */
#define OS_BANYAN	0x00010000	/* Banyan Vines */
#define OS_OLIVETTI_UNIX 0x00020000	/* Olivetti Unix */
#define OS_OTHER	0x80000000	/* Other */

/* Capabilities - sigWORD dsCapabilities;	FLAG BITS */
/* ------------------------------------------------------------------ */
#define CAP_RAID0	0x0001  /* RAID-0 */
#define CAP_RAID1	0x0002  /* RAID-1 */
#define CAP_RAID3	0x0004  /* RAID-3 */
#define CAP_RAID5	0x0008  /* RAID-5 */
#define CAP_SPAN	0x0010  /* Spanning */
#define CAP_PASS	0x0020  /* Provides passthrough */
#define CAP_OVERLAP	0x0040  /* Passthrough supports overlapped commands */
#define CAP_ASPI	0x0080  /* Supports ASPI Command Requests */
#define CAP_ABOVE16MB   0x0100  /* ISA Driver supports greater than 16MB */
#define CAP_EXTEND	0x8000  /* Extended info appears after description */

/* Devices Supported - sigWORD dsDeviceSupp;    FLAG BITS */
/* ------------------------------------------------------------------ */
#define DEV_DASD	0x0001  /* DASD (hard drives) */
#define DEV_TAPE	0x0002  /* Tape drives */
#define DEV_PRINTER	0x0004  /* Printers */
#define DEV_PROC	0x0008  /* Processors */
#define DEV_WORM	0x0010  /* WORM drives */
#define DEV_CDROM	0x0020  /* CD-ROM drives */
#define DEV_SCANNER	0x0040  /* Scanners */
#define DEV_OPTICAL	0x0080  /* Optical Drives */
#define DEV_JUKEBOX	0x0100  /* Jukebox */
#define DEV_COMM	0x0200  /* Communications Devices */
#define DEV_OTHER	0x0400  /* Other Devices */
#define DEV_ALL		0xFFFF  /* All SCSI Devices */

/* Adapters Families Supported - sigWORD dsAdapterSupp; FLAG BITS */
/* ------------------------------------------------------------------ */
#define ADF_2001	0x0001  /* PM2001	*/
#define ADF_2012A	0x0002  /* PM2012A	*/
#define ADF_PLUS_ISA    0x0004  /* PM2011,PM2021    */
#define ADF_PLUS_EISA   0x0008  /* PM2012B,PM2022   */
#define ADF_SC3_ISA	0x0010  /* PM2021	*/
#define ADF_SC3_EISA	0x0020  /* PM2022,PM2122, etc */
#define ADF_SC3_PCI	0x0040  /* SmartCache III PCI */
#define ADF_SC4_ISA	0x0080  /* SmartCache IV ISA */
#define ADF_SC4_EISA	0x0100  /* SmartCache IV EISA */
#define ADF_SC4_PCI	0x0200	/* SmartCache IV PCI */
#define ADF_ALL_MASTER	0xFFFE  /* All bus mastering */
#define ADF_ALL_CACHE	0xFFFC  /* All caching */
#define ADF_ALL		0xFFFF  /* ALL DPT adapters */

/* Application - sigWORD dsApplication;	FLAG BITS */
/* ------------------------------------------------------------------ */
#define APP_DPTMGR	0x0001  /* DPT Storage Manager */
#define APP_ENGINE	0x0002  /* DPT Engine */
#define APP_SYTOS	0x0004  /* Sytron Sytos Plus */
#define APP_CHEYENNE    0x0008  /* Cheyenne ARCServe + ARCSolo */
#define APP_MSCDEX	0x0010  /* Microsoft CD-ROM extensions */
#define APP_NOVABACK    0x0020  /* NovaStor Novaback */
#define APP_AIM		0x0040  /* Archive Information Manager */

/* Requirements - sigBYTE dsRequirements;	FLAG BITS		*/
/* ------------------------------------------------------------------   */
#define REQ_SMARTROM    0x01    /* Requires SmartROM to be present	*/
#define REQ_DPTDDL	0x02    /* Requires DPTDDL.SYS to be loaded	*/
#define REQ_HBA_DRIVER  0x04    /* Requires an HBA driver to be loaded  */
#define REQ_ASPI_TRAN   0x08    /* Requires an ASPI Transport Modules   */
#define REQ_ENGINE	0x10    /* Requires a DPT Engine to be loaded   */
#define REQ_COMM_ENG    0x20    /* Requires a DPT Communications Engine */

typedef struct dpt_sig {
	char	dsSignature[6];	/* ALWAYS "dPtSiG" */
	sigBYTE dsSigVersion;	/* signature version (currently 1) */
	sigBYTE dsProcessorFamily;   /* what type of processor */
	sigBYTE dsProcessor;	/* precise processor */
	sigBYTE dsFiletype;	/* type of file */
	sigBYTE dsFiletypeFlags;	/* flags to specify load type, etc. */
	sigBYTE dsOEM;		/* OEM file was created for */
	sigLONG dsOS;		/* which Operating systems */
	sigWORD dsCapabilities;	/* RAID levels, etc. */
	sigWORD dsDeviceSupp;	/* Types of SCSI devices supported */
	sigWORD dsAdapterSupp;	/* DPT adapter families supported */
	sigWORD dsApplication;	/* applications file is for */
	sigBYTE dsRequirements;	/* Other driver dependencies */
	sigBYTE dsVersion;	/* 1 */
	sigBYTE dsRevision;	/* 'J' */
	sigBYTE dsSubRevision;	/* '9'   ' ' if N/A */
	sigBYTE dsMonth;	/* creation month */
	sigBYTE dsDay;		/* creation day */
	sigBYTE dsYear;		/* creation year since 1980 (1993=13) */
	char	dsDescription[50];   /* description (NULL terminated) */
} dpt_sig_t;

/* 32 bytes minimum - with no description.  Put NULL at description[0] */
/* 81 bytes maximum - with 49 character description plus NULL. */

/* This line added at Roycroft's request */
/* Microsoft's NT compiler gets confused if you do a pack and don't */
/* restore it. */
#pragma PACK_OFF

#define DPT_BUFSIZE 100

/* The following macros should be used to navigate the
 * Hardware Configuration Page
 */

/* Length of page data: include first 4 bytes */
#define DPT_HCP_LENGTH(page) (sdi_swap16(*(short *)(void *)(&page[2]))+4)

/* Address of first log parameter */
#define DPT_HCP_FIRST(page) (&page[4])

/* Address of next log parameter */
#define DPT_HCP_NEXT(parm) (&parm[3 + parm[3] + 1])

/* parameter code */
#define DPT_HCP_CODE(parm) sdi_swap16(*(short *)(void *)parm)

#if (defined(DPT_RAID_0))

extern void DPTR_Scan_LSUs();
extern void DPTR_eataCP_startCp(void *, void *, void (*)(void *, void *,void *));
# define dpt_send(controller,packet,callback)				\
        DPTR_eataCP_startCp ((void *)(controller), (void *)(packet),	\
                (void (*)(void *, void *, void *))(callback))

#else /* ! DPT_RAID_0 */

# define dpt_send		_dpt_send

#endif

#define DPT_VERSION		1
#define DPT_REVISION		'B'
#define DPT_SUBREVISION		'2'
#define DPT_MONTH		3
#define DPT_DAY			5
#define DPT_YEAR		1997 - 1980

			/************************************************/
#define	DPT_BLKSHFT   9 /* PLEASE NOTE:  Currently pass-thru		*/
#define DPT_BLKSIZE 512 /* SS_READ/SS_WRITE, SM_READ/SM_WRITE		*/
			/* supports only 512 byte blocksize devices	*/
			/************************************************/
#if defined(__cplusplus)
}
#endif

#endif /* _IO_HBA_DPT_H */

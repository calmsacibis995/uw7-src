#ident	"@(#)kern-pdi:io/hba/adsa/him_code/him_scb.h	1.5"

/* $Header$ */
/****************************************************************************

  Name:  HIM_SCB.H

  Description:  This file is comprised of three sections:

                1. O/S specific definitions that can be customized to allow
                   for proper compiling and linking of the Hardware Interface
                   Module.

                2. Structure definitions:
                     SCB_STRUCT - Sequencer Control Block Structure
                                  (may be customized)
                     HIM_CONFIG - Host Adapter Configuration Structure
                                  (may be customized)
                     HIM_STRUCT - Hardware Interface Module Data Structure
                                  (cannot be customized)

                3. Function prototypes for the Hardware Interface Module.

                Refer to the Hardware Interface Module specification for
                more information.

  History:
      
****************************************************************************/

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/
/*                                                                         */
/*                CUSTOMIZED OPERATING SPECIFIC DEFINITIONS                */
/*                                                                         */
/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/

#ifdef SVR40
#include <util/types.h>
#define	RestoreState(int)
#define	SaveAndDisable()	0
#endif

/****************************************************************************
                             %%% Type Sizes %%%
****************************************************************************/
#define UBYTE     unsigned char        /* Must be 8 bits                   */
#define UWORD     unsigned short       /* Must be 16 bits                  */
#define DWORD     unsigned long        /* Must be 32 bits                  */


typedef void ((*FCNPTR)(DWORD, void *, void *));

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/
/*                                                                         */
/*        STRUCTURE DEFINITIONS: SCB_STRUCT, HIM_CONFIG, HIM_STRUCT        */
/*                                                                         */
/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/

/****************************************************************************
                    %%% Sequencer Control Block (SCB) %%%
****************************************************************************/
#define MAX_CDB_LEN      12          /* Maximum length of SCSI CDB         */

typedef struct sequencer_ctrl_block {

/************************************/
/*    Hardware Interface Module     */
/*        Specific members          */
/************************************/

   UBYTE SCB_Cmd;                   /* SCB command type                    */
   UBYTE SCB_Stat;                  /* SCB command status                  */
   UBYTE SCB_Flags;                 /* Option flags                        */
   UBYTE SCB_Rsvd1;                 /* Reserved = 0                        */
   UBYTE SCB_MgrStat;               /* Intermediate status of SCB          */

   /* Arrow SCB follows */

   UBYTE SCB_Cntrl;                 /* Control register                    */
   UBYTE SCB_Tarlun;                /* Target/Channel/LUN                  */
   UBYTE SCB_SegCnt;                /* Number of Scatter/Gather segments   */
   DWORD SCB_SegPtr;                /* Pointer to Scatter/Gather list      */
   DWORD SCB_CDBPtr;                /* Pointer to Command Descriptor Block */
   UBYTE SCB_CDBLen;                /* Length of Command Descriptor Block  */
   UBYTE SCB_Rsvd2;                 /* Reserved = 0                        */
   UBYTE SCB_HaStat;                /* Host Adapter status                 */
   UBYTE SCB_TargStat;              /* Target status                       */
   DWORD SCB_ResCnt;                /* Residual byte count                 */

   UBYTE SCB_RsvdX[16];             /* EXTERNAL SCB SCHEME needs all 32 bytes */
                                    /* RsvdX[8] = chain control,           */
                                    /*                                     */
                                    /*  bits 7-4: 2's complement of progress */
                                    /*            count                    */
                                    /*  bit 3:    aborted flag             */
                                    /*  bit 2:    abort_SCB flag           */
                                    /*  bit 1:    concurrent flag          */
                                    /*  bit 0:    holdoff_SCB flag         */
                                    /*                                     */
                                    /* RsvdX[10] = aborted HaStat          */

   UBYTE SCB_CDB[MAX_CDB_LEN];      /* SCSI Command Descriptor Block       */

   DWORD SCB_SensePtr;              /* Pointer to Sense Area               */
   DWORD SCB_SenseLen;              /* Sense Length                        */

   struct sequencer_ctrl_block *SCB_Next;  /* next SCB pointer on queue    */
   UBYTE SCB_MgrWork[8];            /* Work area for extended messages     */
   DWORD SCB_OSspecific;            /* Pointer to custom data structures   */

/************************************/
/*    Operating System Specific     */
/*      members may follow          */
/************************************/
   	int		c_segcnt;	/* copy Number of S/G segments	*/
	int		data_len;	/* lenght of data transfer	*/
	int		adapter;	/* adapter this scb is for	*/
	int		target;		/* target ID of SCSI device	*/
	paddr_t		c_addr;		/* SCB physical address		*/
	unsigned short	c_active;	/* marks this SCB as busy	*/
	struct sb	*c_bind;	/* associated SCSI block	*/
	struct sg_array	*c_sg_p;	/* pointer to s/g list		*/
	struct {			 /* a single sg for normal commands */
		paddr_t	sg_addr;
		long	sg_size;
	} single_sg;
	struct sequencer_ctrl_block *c_next; /* pointer to next SCB struct */
	struct req_sense_def *virt_SensePtr; /* virt address of SCB_SensePtr */

} scb_struct;

#define  SCB_CHAIN_INDEX   0x08     /* Index of chain control byte         */
#define  SCB_CHAIN_ABORT   0x08     /* Aborted bit in chain control byte   */

/****************************************************************************
                        %%% SCB_Cmd values %%%
****************************************************************************/
/* #define  EXEC_SCB       0x02 */        /* Standard SCSI command               */
/* #define  RESET_DEV      0x04 */        /* Bus device reset given Targ/LUN     */

#define  EXEC_SCB       0x02        /* Standard SCSI command               */
#define  READ_SENSE     0x01        /*                */
#define  SOFT_RST_DEV   0x03        /*                */
#define  HARD_RST_DEV   0x04        /*                */
#define  NO_OPERATION   0x00        /*                */
#define  BOOKMARK       0x80        /* Used for hard host adapter reset */

/****************************************************************************
                        %%% scb_special opcodes %%%
****************************************************************************/

#define  ABORT_SCB      0x00        /*                */
#define  SOFT_HA_RESET  0x01        /*                */
#define  HARD_HA_RESET  0x02        /*                */
#define  FORCE_RENEGOTIATE 0x03     /*                */
#define  REALIGN_DATA   0x04        /* Re-link config structures with him struct */
#define  BIOS_ON        0x05        /*                */
#define  BIOS_OFF       0x06        /*                */
#define  H_BIOS_OFF     0x10        /*                */

/****************************************************************************
                       %%% SCB_Stat values %%%
****************************************************************************/
#define  SCB_PENDING    0x00        /* SCSI request in progress            */
#define  SCB_COMP       0x01        /* SCSI request completed no error     */
#define  SCB_ABORTED    0x02        /* SCSI request aborted                */
#define  SCB_ERR        0x04        /* SCSI request completed with error   */
#define  INV_SCB_CMD    0x80        /* Invalid SCSI request                */

/****************************************************************************
                       %%% SCB_Flags values %%%
****************************************************************************/
#define  NEGO_IN_PROG   0x02        /* Negotiation in progress             */
#define  SCB_HEAD       0x04        /* SCB is at head of chain             */
#define  SCB_CHAINED    0x08        /* SCB_Next valid                      */
#define  SUPPRESS_NEGO  0x20        /* Suppress negotiation                */
#define  NO_UNDERRUN    0x40        /* 1 = Suppress underrun errors        */
#define  AUTO_SENSE     0x80        /* 1 = Automatic Request Sense enabled */

/****************************************************************************
                       %%% SCB_MgrStat values %%%
****************************************************************************/
#define  SCB_PROCESS    0x00        /* SCB needs to be processed           */
#define  SCB_DONE       SCB_COMP    /* SCB finished without error          */
#define  SCB_DONE_ABT   SCB_ABORTED /* SCB finished due to abort from host */
#define  SCB_DONE_ERR   SCB_ERR     /* SCB finished with error             */
#define  SCB_READY      0x10        /* SCB ready to be loaded into Arrow   */
#define  SCB_WAITING    0x20        /* SCB waiting for another to finish   */
#define  SCB_ACTIVE     0x40        /* SCB loaded into Arrow               */
#define  SCB_DONE_ILL   INV_SCB_CMD /* SCB finished due to illegal command */

#define  SCB_AUTOSENSE  0x08        /* SCB w/autosense in progress */

#define  SCB_DONE_MASK  SCB_DONE+SCB_DONE_ABT+SCB_DONE_ERR+SCB_DONE_ILL

/****************************************************************************
                       %%% SCB_Cntrl values %%%
****************************************************************************/
#define  REJECT_MDP     0x80        /* Non-contiguous data                 */
#define  DIS_ENABLE     0x40        /* Disconnection enabled               */
#define  TAG_ENABLE     0x20        /* Tagged Queuing enabled              */
#define  SWAIT          0x08        /* Sequencer trying to select target   */
#define  SDISCON        0x04        /* Traget currently disconnected       */
#define  TAG_TYPE       0x03        /* Tagged Queuing type                 */

/****************************************************************************
                       %%% SCB_Tarlun values %%%
****************************************************************************/
#define  TARGET_ID      0xf0        /* SCSI target ID (4 bits)             */
#define  CHANNEL        0x08        /* SCSI Bus selector: 0=chan A,1=chan B*/
#define  LUN            0x07        /* SCSI target's logical unit number   */

/***************************************************************************
                      %%% SCB_HaStat values %%%
****************************************************************************/
#define  HOST_NO_STATUS 0x00        /* No adapter status available         */
#define  HOST_ABT_HOST  0x04        /* Command aborted by host             */
#define  HOST_ABT_HA    0x05        /* Command aborted by host adapter     */
#define  HOST_SEL_TO    0x11        /* Selection timeout                   */
#define  HOST_DU_DO     0x12        /* Data overrun/underrun error         */
#define  HOST_BUS_FREE  0x13        /* Unexpected bus free                 */
#define  HOST_PHASE_ERR 0x14        /* Target bus phase sequence error     */
#define  HOST_INV_LINK  0x17        /* Invalid SCSI linking operation      */
#define  HOST_SNS_FAIL  0x1b        /* Auto-request sense failed           */
#define  HOST_TAG_REJ   0x1c        /* Tagged Queuing rejected by target   */
#define  HOST_HW_ERROR  0x20        /* Host adpater hardware error         */
#define  HOST_ABT_FAIL  0x21        /* Target did'nt respond to ATN (RESET)*/
#define  HOST_RST_HA    0x22        /* SCSI bus reset by host adapter      */
#define  HOST_RST_OTHER 0x23        /* SCSI bus reset by other device      */

/****************************************************************************
                     %%% SCB_TargStat values %%%
****************************************************************************/
#define  UNIT_GOOD      0x00     /* Good status or none available       */
#define  UNIT_CHECK     0x02     /* Check Condition                     */
#define  UNIT_MET       0x04     /* Condition met                       */
#define  UNIT_BUSY      0x08     /* Target busy                         */
#define  UNIT_INTERMED  0x10     /* Intermediate command good           */
#define  UNIT_INTMED_GD 0x14     /* Intermediate condition met          */
#define  UNIT_RESERV    0x18     /* Reservation conflict                */

#define  SCB_CDB_OFFSET 35       /* To calculate the physical address of the SCB
                                    to pass to the sequencer in External mode,
                                    subtract this number from the CDB physical
                                    address. */

#define  NEXT_SCB_NULL  0x7F     /* Null entry value for next scb array */

#define  PHYS_SCB_NULL  0xFFFFFFFF  /* Null entry for scb_array */

#define  INDEX_ARRAY_NULL 0xFF   /* Null entry for qin, qout, busy ptr arrays */

typedef struct scbp
{
   UWORD Prm_AccessMode;         /* SCB handling mode to use:
                                    0 = Use default
                                    1 = Internal method (4 SCBs max.)
                                    2 = Optima method (255 SCBs max.) */

   UBYTE Prm_MaxNTScbs;          /* Max. nontagged SCBs per channel/target,
                                    valid settings are 1 or 2 */

   UBYTE Prm_MaxTScbs;           /* Max. tagged SCBs per channel/target,
                                    valid settings are 1 - 32 */

   UWORD Prm_NumberScbs;         /* Number SCBs allowed in sequencer queue,
                                    valid settings are 1 - 255 */

   DWORD Prm_HimDataPhysaddr;    /* Physical address of HIM data structure */
   UWORD Prm_HimDataSize;        /* Working size of HIM data structure in
                                    bytes, valid after scb_getconfig */

} ScbParam;

/****************************************************************************
               %%% Host Adapter Configuration Structure %%%
****************************************************************************/

typedef struct him_config_block {
                                    /*    HOST ADAPTER IDENTIFICATION      */
   UWORD Cfe_AdapterID;             /* Host Adapter ID (ie. 0x7770)        */
   UBYTE Cfe_ReleaseLevel;          /* HW Interface Module release level   */
   UBYTE Cfe_RevisionLevel;         /* HW Interface Module revision level  */
                                    /*      INITIALIZATION PARAMETERS      */
   UWORD Cfe_PortAddress;           /* Base port address (ISA or EISA)     */
   struct him_data_block *Cfe_HaDataPtr; /* Pointer to HIM data stucture     */
   UBYTE Cfe_SCSIChannel;           /* SCSI channel designator             */
                                    /*          HOST CONFIGURATION         */
   UBYTE Cfe_IrqChannel;            /* Interrupt channel #                 */
   UBYTE Cfe_DmaChannel;            /* DMA channel # (ISA only)            */
   UBYTE Cfe_BusRelease;            /* DMA bus release timing (EISA)       */
   UBYTE Cfe_BusOn;                 /* DMA bus-on timing (ISA)             */
   UBYTE Cfe_BusOff;                /* DMA bus-off timing (ISA)            */
   UWORD Cfe_StrobeOn;              /* DMA bus timing (ISA)                */
   UWORD Cfe_StrobeOff;             /* DMA bus timing (ISA)                */
   UBYTE Cfe_Threshold;             /* Data FIFO threshold                 */
   UBYTE Cfe_Reserved;
                                    /*          SCSI CONFIGURATION         */
   UWORD Cfe_ConfigFlags;           /* Flags                               */
   UBYTE Cfe_ScsiId;                /* Host Adapter's SCSI ID              */
   UBYTE Cfe_MaxTargets;            /* Maximum number of targets on bus    */
   UBYTE Cfe_ScsiOption[16];        /* SCSI config options (1 byte/target) */
   UWORD Cfe_AllowDscnt;            /* Bit map to allow disconnection      */

   /* Parameters added to intialize and control Optima mode
      NOTE - A channel parameters apply to both channels, B channel
      parameters are ignored by the HIM during initialization.       */

   ScbParam Cfe_ScbParam;        /* SCB parameter block */

   DWORD Cfe_OSspecific;         /* Pointer to custom data structures
                                    required by O/S specific interface. */

   /* The Following parameters are available in extended mode only. */

    FCNPTR Cfe_CallBack[8];      /* OSM Callbacks */

   DWORD Cfe_ExtdFlags;          /* Extended flag word */
} him_config;
/****************************************************************************
                     %%% Cfe_ConfigFlags values %%%
****************************************************************************/
#define  DRIVER_IDLE    0x8000      /* HIM Driver and sequencer idle (Runtime Only) */
#define  TWO_CHNL       0x8000      /* Used by HIM at initialization only  */

#define  EXTD_CFG       0x4000      /* Extended parameters in him_config used */

#define  SAVE_SEQ       0x0800      /* Save BIOS sequencer on initialization */
#define  BIOS_AVAILABLE 0x0400      /* BIOS available for swapping */
#define  DIFF_SCSI      0x0200      /* BIOS sequencer currently swapped in */
#define  RESVD_9        0x0100      /* Reserved for PCI */
#define  INIT_NEEDED    0x0080      /* Initialization required             */
#define  RESET_BUS      0x0040      /* Reset SCSI bus at 1st initialization*/
#define  SCSI_PARITY    0x0020      /* Parity checking enabled if equal 1  */
#define  STIMESEL       0x0018      /* Selection timeout (2 bits)          */
#define  INTHIGH        0x0004      /* Interrupts high edge true           */
/*@BIOSDETECT*/
#define  BIOS_ACTIVE    0x0002      /* SCSI BIOS presently active          */
/*@BIOSDETECT*/
#define  PRI_CH_ID      0x0001      /* 0 = A channel is primary            *
                                     * 1 = B channel is primary            */
#define  SC_PRI_CH_ID   0x08        /* Mask for PRI_CH_ID at BIOS_CNTRL    *
                                     * in scratch RAM                      */

/****************************************************************************
                      %%% scsi_channel values %%%
****************************************************************************/
#define  A_CHANNEL    0x00          /* A SCSI channel                      */
#define  B_CHANNEL    0x08          /* B SCSI channel                      */

/****************************************************************************
                      %%% scsi_options values %%%
****************************************************************************/
#define  WIDE_MODE      0x80        /* Allow wide negotiation if equal 1   */
#define  SYNC_RATE      0x70        /* 3-bit mask for maximum transfer rate*/
#define  SYNC_MODE      0x01        /* Allow synchronous negotiation if = 1*/

/****************************************************************************
                      %%% Asynchronous Event Callback Parameters %%%
****************************************************************************/

#define  AE_3PTY_RST    0           /* 3rd party SCSI bus reset         */
#define  AE_HA_RST      1           /* Host Adapter SCSI bus reset      */
#define  AE_TGT_SEL     2           /* Host Adapter selected as target  */

#define  CALLBACK_ASYNC 0  /* Array Index of Asynchronous Event Callback */


typedef struct sequencer_ctrl_block * (*scblink)();

/****************************************************************************
                  %%% Host Adapter Data Structure %%%
****************************************************************************/

typedef struct him_data_block {

   /* Arrays for EXTERNAL SCB implementation */

   DWORD scb_array[256];

   UBYTE qin_array[256];            /* x400 */
   UBYTE qout_array[256];           /* x500 */
   UBYTE busy_array[256];           /* x600 */
   UBYTE free_ptr_list[256];        /* x700 SCB ptr free list */
   UBYTE actstat[256];              /* x800 Status of SCB array (active or free)*/

   UBYTE act_chtar[16];             /* x900 Array used to track untagged commands */

   UBYTE free_hi;                   /* x910 */
   UBYTE free_lo;                   /* x911 */

   /* External SCB parameters */

   UBYTE cur_scb_ptr;               /* x912 scb array index of current scb */

   UBYTE qin_index;                 /* x913 ptr to next completed SCB */
   UBYTE qout_index;                /* x914 ptr to next completed SCB */

   UWORD active_scb;                /* x915 Active SCB register */
   UWORD qin_cnt;                   /* x917 QIN FIFO Count register */

   DWORD *scb_ptr_array;            /* Pointers to external arrays */
   UBYTE *qin_ptr_array;
   UBYTE *qout_ptr_array;
   UBYTE *busy_ptr_array;

   struct sequencer_ctrl_block *actptr[256]; /* Array of SCB pointers        */

   /* END OF EXTERNAL SCB STUFF */

   struct him_config_block *AConfigPtr; /* Ptrs to Config structures   */
   struct him_config_block *BConfigPtr;

   UWORD scsiseq;                   /* SCSI Sequence Control register      */
   UWORD sxfrctl0;                  /* SCSI Transfer Control register 0    */
   UWORD sxfrctl1;                  /* SCSI Transfer Control register 1    */
   UWORD scsisig;                   /* SCSI Control Signals register       */
   UWORD scsirate;                  /* SCSI Rate Control register          */
   UWORD scsiid;                    /* SCSI ID                             */
   UWORD scsidatl;                  /* SCSI Latched Data register (low)    */
   UWORD clrsint0;                  /* Clear SCSI Interrupts register 0    */
   UWORD clrsint1;                  /* Clear SCSI Interrupts register 1    */
   UWORD sstat0;                    /* SCSI Status register 1              */
   UWORD sstat1;                    /* SCSI Status register 1              */
   UWORD simode1;                   /* SCSI Interrupt Mask register 1      */
   UWORD scsibusl;                  /* SCSI Data Bus register (low)        */
   UWORD sblkctl;                   /* SCSI Block Control register         */
   UWORD xfer_option;               /* SCSI Data Transfer Options array    */
   UWORD pass_to_driver;            /* Sequencer Interrupt Info register   */
   UWORD seqctl;                    /* Sequencer Address register (low)    */
   UWORD seqaddr0;                  /* Sequencer Address register (low)    */
   UWORD hcntrl;                    /* Host Control register               */
   UWORD scbptr;                    /* SCB Pointer register                */
   UWORD intstat;                   /* Interrupt Status register           */
   UWORD clrint;                    /* Clear Interrupt Status register     */
   UWORD scbcnt;                    /* SCB Count register                  */
   UWORD qinfifo;                   /* Queue In FIFO register              */
   UWORD qincnt;                    /* Queue In Count register             */
   UWORD qoutfifo;                  /* Queue Out FIFO register             */
   UWORD qoutcnt;                   /* Queue Out Count register            */
   UWORD scb00;                     /* Start of SCB array                  */
   UWORD scb02;                     /* Scatter/Gather list count           */
   UWORD scb03;                     /* Scatter/Gather pointer (LSB)        */
   UWORD scb11;                     /* SCSI Command descriptor block length*/
   UWORD scb14;                     /* Target Status                       */

   struct sequencer_ctrl_block *Head_Of_Queue; /* First SCB ptr on Queue   */
   struct sequencer_ctrl_block *End_Of_Queue;  /* Last SCB ptr on Queue    */
   /* struct sequencer_ctrl_block *actptr[5]; */ /* Array of SCB pointers        */
   UBYTE HaFlags;                   /* Flags                               */
   UWORD Hse_ConfigFlags;
   UWORD Hse_AccessMode;
   UBYTE max_nontag_cmd;            /* Max # of non-tagged cmds per target */
   UBYTE max_tag_cmd;               /* Max # of tagged cmds per target     */
   UBYTE free_scb;                  /* Max # of SCBs that can be loaded    */
   UBYTE done_cmd;

   UWORD sel_cmp_brkpt;             /* Sequencer address for breakpoint on
                                       target selection complete */
   UWORD idle_brkpt;                /* Sequencer address for breakpoint on
                                       sequencer idle loop */

   int (*EnqueScb)();               /* Pointers to him modules unique */
   void (*PreemptScb)();            /* to Optima or Non-Optima modes */
   void (*ChekCond)();
   scblink DequeScb;

   DWORD Hse_HimDataPhysaddr;       /* Physical address of HIM data structure */
   UWORD Hse_HimDataSize;           /* Working size of HIM data structure in */

   scb_struct scb_mark;

   UWORD g_state;

   UBYTE BIOSScratchRAM[64];        /* Used to save Scratch RAM state */
   UBYTE DrvrScratchRAM[64];

   union
   {
      UBYTE InstrBytes[1792];
      DWORD InstrWords[448];
   } Hse_BiosSeq;
} him_struct;

#define  SEQNOOP        0x00000000
#define  SEQMAX         1792

/****************************************************************************
                        %%% Miscellaneous %%%
****************************************************************************/
#define  SCB_LENGTH     0x13        /* # of SCB bytes to download to Arrow */
#define  ERR            0xff        /* Error return value                  */
#define  NOERR          0x00        /* No error return value               */
#define  NONEGO         0x00        /* Responding to target's negotiation  */ 
#define  NEEDNEGO       0x8f        /* Need negotiation response from targ */
#define  SCB18          0x12        /* Address of Residual Count in SCB    */
#define  MBCTL_DMAER_ON 0x01        /* MSBRSTCTL bit ON or DMA error       */
#define  MSBRSTCTL_OFF  0x00        /* MSBRSTCTL bit OFF                   */

/****************************************************************************
                       %%% HaFlags values %%%
****************************************************************************/
#define  HAFL_SWAP_ON      0x01     /* Optima mode active         */
#define  HAFL_BIOS_RAM     0x04     /* BIOS Scratch RAM loaded    */
#define  HAFL_MBRCTL_ON    0x08     /* MSBRSTCTL bit ON, reg BUSTIME  ja 3/17    */
#define  HAFL_BIOS_CACHED  0x20     /* BIOS sequencer currently swapped in       */

#define  DMA_PASS          0x00
#define  DMA_FAIL          0xFF

/****************************************************************************
                      %%% targ_option values %%%
****************************************************************************/
#define  ALLOW_DISCON   0x04        /* Allow targets to disconnect       */
#define  SYNCNEG        0x08        /* Init for synchronous negotiation  */
#define  PARITY_ENAB    0x20        /* Enable SCSI parity checking       */
#define  WIDENEG        0x80        /* Init for Wide negotiation         */
#define  MAX_SYNC_RATE  0x07        /* Maximum synchronous transfer rate */

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/
/*                                                                         */
/*              CONSTANTS FOR FAULT TOLERANT CODE                          */
/*                                                                         */
/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/

/* Return Codes for scb_deque */

#define  DQ_GOOD        0x00        /* SCB successfully dequeued */
#define  DQ_FAIL        0xFF        /* SCB is ACTIVE, cannot dequeue */

/* Values for actstat array in ha_struct */

#define  AS_SCBFREE        0x00
#define  AS_SCB_ACTIVE     0x01
#define  AS_SCB_ABORT      0x02
#define  AS_SCB_ABORT_CMP  0x03
#define  AS_SCB_BIOS       0x40
#define  AS_SCB_RST        0x10
                        
#define  ABORT_DONE     0x00
#define  ABORT_INPROG   0x01
#define  ABORT_FAILED   0xFF

/* "NULL ptr-like" value (Don't want to include libraries ) */

#define  NOT_DEFINED    0x00

/* Values for scsi_state in send_trm_msg */

#define  NOT_CONNECTED     0x0C
#define  CONNECTED         0x00
#define  WAIT_4_SEL        0x88
#define  OTHER_CONNECTED   0x40

/* Return code mask when int_handler detects BIOS interrupt */

#define  BIOS_INT          0x80

#define  NOT_OUR_INT       0x80
#define  OUR_CC_INT        0x40
#define  OUR_OTHER_INT     0x20
#define  OUR_INT           0x60
#define  OUR_SW_INT        0x10

/****************************************************************************
               %%% BIOS Information Structure %%%
****************************************************************************/

#define  BI_BIOS_ACTIVE    0x01
#define  BI_GIGABYTE       0x02        /* Gigabyte support enabled */
#define  BI_DOS5           0x04        /* DOS 5 support enabled */

#define  BI_MAX_DRIVES     0x08        /* DOS 5 maximum, 2 for DOS 3,4 */

#define  BIOS_BASE         0x0A
#define  BIOS_DRIVES       BIOS_BASE + 4
#define  BIOS_GLOBAL       BIOS_BASE + 12
#define  BIOS_FIRST_LAST   BIOS_BASE + 14

#define  BIOS_GLOBAL_DUAL  0x08
#define  BIOS_GLOBAL_WIDE  0x04
#define  BIOS_GLOBAL_GIG   0x01

#define  BIOS_GLOBAL_DOS5  0x40

typedef struct bios_info_block {

   UBYTE bi_global;
   UBYTE bi_first_drive;
   UBYTE bi_last_drive;
   UBYTE bi_drive_info[BI_MAX_DRIVES];
   UBYTE bi_bios_control;
   UBYTE bi_reserved[4];

} bios_info;

/* scb_bios_special opcodes */

#define  SET_BIOS_VECT     1
#define  CLEAR_BIOS_VECT   2
#define  CHECK_BIOS_VECT   3

/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/
/*                                                                         */
/*              HARDWARE INTERFACE MODULE FUNCTION PROTOTYPES              */
/*                                                                         */
/*/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/*/

/****************************************************************************
            %%% Hardware Interface Module Function Prototypes %%%
****************************************************************************/
/* UBYTE scb_findha(unsigned int port_addr); */
/* void  scb_getconfig(him_config *config_ptr); */
/* UBYTE scb_initha(him_config *config_ptr); */
/* void  scb_send(him_config *config_ptr,scb_struct *scb_pointer); */
/* UBYTE scb_abort(him_config *config_ptr,scb_struct *scb_pointer); */
/* UBYTE int_handler(him_config *config_ptr); */
/* void  scb_force_int(him_config *config_ptr); */
/* void  scb_enable_int(him_config *config_ptr); */
/* void  scb_disable_int(him_config *config_ptr); */
/* UBYTE scb_poll_int(him_config *config_ptr); */
/****************************************************************************/

#define INTR_WINDOW  INTR_ON;                      \
                     INBYTE(ha_ptr->hcntrl);       \
                     INTR_OFF

#define GET_SCB_INDEX(index)                       \
           if (ha_ptr->HaFlags & HAFL_SWAP_ON)     \
           {                                       \
              index = INBYTE(ha_ptr->active_scb);  \
           }                                       \
           else                                    \
           {                                       \
              index = INBYTE(ha_ptr->scbptr);      \
           }

/* External SCB stuff */

#define  USE_DEFAULT    0x00

#define  SMODE_NOSWAP   0x01
#define  SMODE_SWAP     0x02

#define  HARD_QDEPTH    0x4      /* Maximum number of active SCB's */
#define  SOFT_QDEPTH    0xFF     /* Maximum number of active SCB's */

#define  NULL_INDEX     0xFF     /* Null SCB index value */

#define  MAX_NONTAG     0x02     /* Max. active nontag SCB's per channel/id */
#define  MAX_EXT_TAG    0x20     /* Max. active tagged SCB's per channel/id */
#define  MAX_INT_TAG    0x02     /* Max. active tagged SCB's per channel/id */

#define  SCB_CHAIN_CTRL 0x09     /* Chain control byte in SCB_RsvdX */
#define  SCB_CDB_ABORTED 0x04    /* Aborted flag, set to for sequencer to abort */

#define  INTERNAL_SCBS  0x01     /* Values for call to scb_LoadEntryTbl */
#define  SWAPPING_SCBS  0x02

#define  HSE_PAD        0x100    /* Padding for Optima him_struct */

#define  CHNLCFG_SINGLE       1
#define  CHNLCFG_TWIN         2
#define  CHNLCFG_WIDE         3
#define  CHNLCFG_DIFFWIDE     4

/* #define SCRATCH   EISA_SCRATCH1 */

/* #define  WAITING_SCB_CH0   SCRATCH+27     1 byte index         C3B */
/* #define  ACTIVE_SCB        SCRATCH+28     1 byte index         C3C */
/* #define  WAITING_SCB_CH1   SCRATCH+29     1 byte index         C3D */
/* #define  SCB_PTR_ARRAY     SCRATCH+30     4 byte ptr           C3E */
/* #define  QIN_CNT           SCRATCH+34     1 byte count         C42 */
/* #define  QIN_PTR_ARRAY     SCRATCH+35     4 byte ptr to array  C43 */
/* #define  NEXT_SCB_ARRAY    SCRATCH+39     16 byte index array  C47 */
/* #define  QOUT_PTR_ARRAY    SCRATCH+55     4 byte ptr to array  C57 */
/* #define  BUSY_PTR_ARRAY    SCRATCH+59     4 byte ptr to array  C5B */

/* Seqint Driver interrupt codes identify action to be taken by the Driver.*/

/* #define  SYNC_NEGO_NEEDED  0x00     initiate synchronous negotiation    */
/* #define  CDB_XFER_PROBLEM  0x10     possible parity error in cdb:  retry*/
/* #define  HANDLE_MSG_OUT    0x20     handle Message Out phase            */
/* #define  DATA_OVERRUN      0x30     data overrun detected               */
/* #define  UNKNOWN_MSG       0x40     handle the message in from target   */
/* #define  CHECK_CONDX       0x50     Check Condition from target         */
/* #define  PHASE_ERROR       0x60     unexpected scsi bus phase           */
/* #define  EXTENDED_MSG      0x70     handle Extended Message from target */
/* #define  ABORT_TARGET      0x80     abort connected target              */
/* #define  NO_ID_MSG         0x90     reselection with no id message      */

/****************************************************************************
                       %%% Run-Time Library Functions %%%
****************************************************************************/
#ifdef SVR40
#define INBYTE    inb			/* read byte from port              */
#define INWORD    inw			/* read word from port              */
#define OUTBYTE   outb			/* write byte to port               */
#define OUTWORD   outw			/* write word to port               */
#define INTR_OFF			/* disable system interrupts        */
#define INTR_ON				/* enable system interrupts         */

#define  PAUSE_SEQ(hcntrl)                            \
         outb(hcntrl, (UBYTE)(inb(hcntrl) | PAUSE));  \
         while(!(inb(hcntrl) & PAUSE))

#define  UNPAUSE_SEQ(hcntrl,intstat)                  \
         if ((inb(hcntrl) & PAUSE))                   \
         {                                            \
            if (!(inb(intstat) & ANYPAUSE))           \
               OUTBYTE(hcntrl, (UBYTE)(INBYTE(hcntrl) & ~PAUSE)); \
            inb(hcntrl);                              \
         }                                            

#define  UPDATE_HCNTRL(hcntrl, value)                 \
         outb(hcntrl, (UBYTE)(inb(hcntrl) | value))   

#define  WRITE_HCNTRL(hcntrl, value)                  \
         outb(hcntrl, value)

#define  READ_HINTSTAT(hcntrl, intstat, intval)  if ((inb(hcntrl) & PAUSE)) \
         {                                                        \
            intval = inb(intstat);                                \
         }                                                        \
         else                                                     \
         {                                                        \
            outb(hcntrl, (UBYTE)(inb(hcntrl) | PAUSE));           \
            while(!(inb(hcntrl) & PAUSE));                        \
            intval = inb(intstat);                                \
            OUTBYTE(hcntrl, (UBYTE)(INBYTE(hcntrl) & ~PAUSE));    \
            inb(hcntrl);                                          \
         }

#else
#define INBYTE    inp                  /* read byte from port              */
#define INWORD    inpw                 /* read word from port              */
#define OUTBYTE   outp                 /* write byte to port               */
#define OUTWORD   outpw                /* write word to port               */
#define INTR_OFF  _disable()           /* disable system interrupts        */
#define INTR_ON   _enable()            /* enable system interrupts         */

#define  PAUSE_SEQ(hcntrl)                            \
         outp(hcntrl, (UBYTE)(inp(hcntrl) | PAUSE));  \
         while(!(inp(hcntrl) & PAUSE))

#define  UNPAUSE_SEQ(hcntrl,intstat)                  \
         if ((inp(hcntrl) & PAUSE))                   \
         {                                            \
            if (!(inp(intstat) & ANYPAUSE))           \
               OUTBYTE(hcntrl, (UBYTE)(INBYTE(hcntrl) & ~PAUSE)); \
            inp(hcntrl);                              \
         }                                            

#define  UPDATE_HCNTRL(hcntrl, value)                 \
         outp(hcntrl, (UBYTE)(inp(hcntrl) | value))   

#define  WRITE_HCNTRL(hcntrl, value)                  \
         outp(hcntrl, value)

#define  READ_HINTSTAT(hcntrl, intstat, intval)  if ((inp(hcntrl) & PAUSE)) \
         {                                                        \
            intval = inp(intstat);                                \
         }                                                        \
         else                                                     \
         {                                                        \
            outp(hcntrl, (UBYTE)(inp(hcntrl) | PAUSE));           \
            while(!(inp(hcntrl) & PAUSE));                        \
            intval = inp(intstat);                                \
            OUTBYTE(hcntrl, (UBYTE)(INBYTE(hcntrl) & ~PAUSE));    \
            inp(hcntrl);                                          \
         }

#endif

#define  WATCH_ON    0x6666
#define  WATCH_OFF   0x5555

#define  SET_STATE(config_ptr, value)                 \
         config_ptr->Cfe_HaDataPtr->g_state = (UWORD) value
/*
#ifdef _CUSTOM_ 
#include "custom.h"
#endif
*/

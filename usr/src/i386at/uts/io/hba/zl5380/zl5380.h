/*      Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.      */
/*      Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.        */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.      */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */


/*
 * File Name: zl5380.h
 * This file has the constant and structure definitions for T-160 driver
 */
#ident	"@(#)kern-pdi:io/hba/zl5380/zl5380.h	1.1.2.1"

#ifndef ZL5380_IO_HBA_INCLUDED
#define ZL5380_IO_HBA_INCLUDED

#if defined (__cplusplus) 
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/sdi_hier.h>
#include <fs/buf.h>
#include <util/ksynch.h>
#include <io/ddi.h>

#elif defined (_KERNEL)

#include <sys/types.h>
#include <sys/scsi.h>
#include <sys/sdi.h>
#include <sys/sdi_hier.h>
#include <sys/buf.h>
#include <sys/ksynch.h>
#include <sys/ddi.h>

#endif /* _KERNEL_HEADERS */

extern unsigned long zl5380_wait_count;
extern unsigned int  zl5380_retry_count;
extern unsigned int  zl5380_arb_retry_count;
extern unsigned long zl5380_reset_wait_count;
extern unsigned long zl5380_reset_delay_count1;
extern unsigned long zl5380_reset_delay_count2;


#define HBA_PREFIX		zl5380  /* Driver Prefix */
#define ZL5380_SCSI_ID		7       /* Controller SCSI ID */

#define MAX_CNTLS		4
#define MAX_EQ			(MAX_TCS * MAX_LUS)

#define MAX_CMDSZ		12

#ifndef TRUE
#define TRUE			1
#define FALSE			0
#endif


/*
 * LU Queue status flags
 */
#define	QBUSY		0x01
#define	QSUSP		0x04
#define	QPTHRU		0x08
#define QSENSE		0x10

#define ZL_QUECLASS(x)	((x)->sbp->sb.sb_type)
#define	HBA_QNORM		SCB_TYPE

#define FREEBLK(x)	(x->c_active = FALSE)

#define SUBDEV(t,l)	((t << 3) | l)
#define LU_Q(c,t,l)	(zl5380scha[c]->ha_dev[SUBDEV(t,l)])
#define LUCB(ha, t, l)  (&ha->ha_lucb[SUBDEV(t,l)])

/*
 * Command block structure
 */
struct zl5380scb {
	struct   zl5380scb	*c_chain;
	unsigned char		c_opcode;
	unsigned char		c_lun;
	unsigned char		c_target;
	unsigned char		c_cmdsz;
	unsigned short		c_flags;
        caddr_t			c_datapointer;
	ulong			c_datalength;
	unsigned char		c_cdb[12];
	unsigned char		c_status;
	struct sense    	c_sense;
	unsigned short		c_active;
	struct sb		*c_bind;
	struct zl5380scb	*c_next;
	int			c_adapter;
};
typedef struct zl5380scb	zl5380scb_t; 

struct zl5380lucb {
      	struct zl5380scb	*activescb;
};
typedef struct zl5380lucb zl5380lucb_t;



/*
 * Logical Unit Queue Structure
 */
struct zl5380scsi_lu {
	int		q_count;	/* Jobs running on this SCSI LU     */
        int		q_flag;	        /* LU queue state flags             */
	struct srb	*q_first;	/* First block on LU queue          */
	struct srb	*q_last;	/* Last block on LU queue           */
	struct sense	q_sense;        /* Sense data                       */
	void		(*q_func)();	/* Target driver event handler      */
	long		q_param;	/* Target driver event param        */
	caddr_t		q_sc_cmd;	/* Command pointer for Passthru     */
	pl_t		q_opri;
	bcb_t		*q_bcbp;
	lock_t		*q_lock;
};

typedef struct zl5380scsi_lu zl5380scsi_lu_t;

/*
 * ZL5380 register addresses structure
 */
struct zl5380reg_set {
	int	csdr;	/* Current SCSI Data Register 		*/
	int 	odr;	/* Output Data Register			*/
	int	icr;	/* Initiator Command Register		*/
	int	mr; 	/* Mode Register			*/
	int 	tcr;	/* Target Command Register		*/
	int	cscr;	/* Current SCSI Bus Status Register	*/
	int	ser;	/* Select Enable Register		*/
	int	bsr;	/* Bus and Status Register		*/
	int	sdsr;	/* Start DMA Send Register		*/
	int	idr;	/* Input Data Register			*/
	int	sdtr;	/* Start DMA Target Receive Register	*/
	int	rpir;	/* Reset Parity/Interrupt Register	*/
	int	sdir;	/* Start DMA Initiator Receive Register	*/
};

typedef struct zl5380reg_set zl5380reg_set_t;

/*
 * Host Adapter Structure. This stores the information about every HBA 
 * controller configured in the system.
 */
struct zl5380ha {
	char		*ha_name;	/* Name of the HBA                   */
	unsigned short	ha_id;		/* Host adapter SCSI id              */
	unsigned char 	ha_bitid;	/* 0x80 for scsi id 7		     */
	int		ha_vect;	/* Interrupt vector number           */
	unsigned long	ha_base;	/* Base I/O address                  */
	zl5380reg_set_t ha_5380_reg;	/* zl5380 register addresses.        */
	int		ha_type;	/* Adapter type = media vision,t160..*/

	zl5380scsi_lu_t	*ha_dev;	/* Logical unit queues               */
	zl5380lucb_t	*ha_lucb;       /* Array of LUCBs		     */
	zl5380scb_t	*ha_eligiblescb;/* List of SCBs to be processed      */	
	zl5380scb_t	*ha_scb;	/* Pool of command blocks 	     */
	int		ha_disconnects; /* No of disconnected jobs	     */
	int		ha_intr_mode;	/* TRUE => uses interrupts	     */

	zl5380scb_t	*ha_scb_next;	/* Next SCB in the list     	     */
	zl5380scb_t	*ha_poll;	/* SCB to be polled     	     */
	zl5380scb_t	*ha_currentscb;	/* SCB being processed		     */

	unsigned char 	ha_reselectingtarget;	/* Current target id	     */
	unsigned char	ha_msgout;	/* Byte to be sent out during msg out*/
	unsigned char	ha_targetstatus;/* Status byte of last command 	     */
	unsigned char	ha_lun;		/* LUN receiving command	     */
	caddr_t		ha_datapointer; /* Pointer to area for data xfer     */
	ulong		ha_datalength;  /* No of bytes left to be xferred    */

	void		*ha_scb_lock;	/* Lock for command blocks	     */
	void		*ha_hba_lock;	/* Lock for controller		     */
};

typedef struct zl5380ha zl5380ha_t;



/*
 * SCSI Request Block structure
 */
struct srb {
	struct xsb	*sbp;		/* Target drv definition of SB       */
	struct srb	*s_next;	/* Next block on LU queue            */
	struct srb	*s_priv;	/* Private pointer for dynamic alloc */
	                                /* Routines DON'T USE OR MODIFY      */
	struct srb	*s_prev;	/* Previous block on LU queue        */
	caddr_t		s_addr;		/* Physical data pointer             */
};
typedef struct srb	sblk_t;
	

/* Values defined for the SCB 'c_opcode' field. */

#define SCB_EXECUTE		0x00
#define SCB_BUS_DEVICE_RESET	0x01

/* Values defined for the SCB 'status' field. */

#define SCB_PENDING			0x00
#define SCB_COMPLETED_OK		0x01
#define SCB_ABORTED			0x02
#define SCB_ERROR			0x04
#define SCB_TIMEOUT			0x08
#define SCB_SELECTION_TIMEOUT		0x0A
#define SCB_INVALID_LUN			0x20
#define SCB_SENSE_DATA_VALID		0x80

#define SCB_ARBITRATION_SUCCESS		0xA0 

#define SCB_STATUS(status) ((status) & ~SCB_SENSE_DATA_VALID)

/* Values defined for the SCB 'flags' field */

#define REQUEST_SENSE_PENDING		0x0001
#define DISCONNECT			0x0002
#define SCB_DISABLE_DISCONNECT		0x0004
#define SCB_DATA_IN			0x0040
#define SCB_DATA_OUT			0x0080

#define LINKSCB_AT_HEAD(H,S) 		{ \
					S->c_chain = *H; \
   					*H = S; \
					}


/*
 * Adapter types : Media Vision PAS 16, Trantor T160
 */
#define UNKNOWN_ADAPTER		0
#define MV_PAS16_ADAPTER	1
#define	T160_ADAPTER		2

/*
 * REGISTER OFFSETS from Base address
 */

/*
 * SCSI Core registers
 */
#define CSDR	(zl5380ha->ha_5380_reg.csdr)
#define ODR	(zl5380ha->ha_5380_reg.odr)
#define ICR	(zl5380ha->ha_5380_reg.icr)
#define MR	(zl5380ha->ha_5380_reg.mr)
#define TCR	(zl5380ha->ha_5380_reg.tcr)
#define CSCR	(zl5380ha->ha_5380_reg.cscr)
#define SER	(zl5380ha->ha_5380_reg.ser)
#define BSR	(zl5380ha->ha_5380_reg.bsr)
#define SDSR	(zl5380ha->ha_5380_reg.sdsr)
#define IDR	(zl5380ha->ha_5380_reg.idr)
#define SDTR	(zl5380ha->ha_5380_reg.sdtr)
#define RPIR	(zl5380ha->ha_5380_reg.rpir)
#define SDIR	(zl5380ha->ha_5380_reg.sdir)

/*
 *	Initiator Command Register Bit Definitions
 */

#define	ZL5380_ICR_ASSERT_DATA_BUS		0x01
#define	ZL5380_ICR_ASSERT_ATN			0x02
#define	ZL5380_ICR_ASSERT_SEL			0x04
#define	ZL5380_ICR_ASSERT_BSY			0x08
#define	ZL5380_ICR_ASSERT_ACK			0x10
#define	ZL5380_ICR_LOST_ARBITRATION		0x20	/* (Read only) */
#define	ZL5380_ICR_DIFF_EN			0x20
#define	ZL5380_ICR_TRISTATE_MODE      		0x40	/* (Write only) */
#define	ZL5380_ICR_ARBITRATION_IN_PROGRESS    	0x40	/* (Read only)  */
#define	ZL5380_ICR_ASSERT_RST		      	0x80
#define ZL5380_ICR_READ_MASK_FOR_WRITING	0x9f

/*
 *	Mode Register Bit Definitions
 */

#define	ZL5380_MR_ARBITRATE			0x01
#define	ZL5380_MR_DMA_MODE			0x02
#define	ZL5380_MR_MONITOR_BUSY			0x04
#define	ZL5380_MR_ENABLE_EOP_INTERRUPT		0x08
#define	ZL5380_MR_ENABLE_PARITY_INTERRUPT       0x10
#define	ZL5380_MR_ENABLE_PARITY_CHECKING        0x20
#define	ZL5380_MR_TARGET_MODE			0x40
#define	ZL5380_MR_BLOCK_MODE_DMA	        0x80

/*
 *	Target Command Register Bit Definitions
 */

#define	ZL5380_TCR_ASSERT_IO			0x01
#define	ZL5380_TCR_ASSERT_CD			0x02
#define	ZL5380_TCR_ASSERT_MSG		        0x04
#define	ZL5380_TCR_ASSERT_REQ			0x08
#define	ZL5380_TCR_LAST_BYTE_SENT	        0x80
#define ZL5380_TCR_DATA_OUT_PHASE	        0x00
#define ZL5380_TCR_DATA_IN_PHASE	        0x01
#define ZL5380_TCR_COMMAND_PHASE		0x02
#define ZL5380_TCR_STATUS_PHASE			0x03
#define ZL5380_TCR_MESSAGE_OUT_PHASE		0x06
#define ZL5380_TCR_MESSAGE_IN_PHASE		0x07

/*
 *	Current SCSI Bus Status Register Bit Definitions
 */

#define	ZL5380_CSCR_DBP				0x01
#define	ZL5380_CSCR_SEL				0x02
#define	ZL5380_CSCR_IO				0x04
#define	ZL5380_CSCR_CD				0x08
#define	ZL5380_CSCR_MSG				0x10
#define	ZL5380_CSCR_REQ				0x20
#define	ZL5380_CSCR_BSY				0x40
#define	ZL5380_CSCR_RST				0x80
#define ZL5380_CSCR_PHASE_MASK			0x1c
#define ZL5380_CSCR_BUS_FREE		(ZL5380_CSCR_SEL | ZL5380_CSCR_BSY)

/*
 *	SCSI Information Transfer Phases 
 */

#define	ZL5380_CSCR_DATA_OUT_PHASE	0x00
#define	ZL5380_CSCR_DATA_IN_PHASE	ZL5380_CSCR_IO
#define	ZL5380_CSCR_COMMAND_PHASE	ZL5380_CSCR_CD
#define	ZL5380_CSCR_STATUS_PHASE	(ZL5380_CSCR_IO | ZL5380_CSCR_CD)
#define ZL5380_CSCR_MESSAGE_OUT_PHASE	(ZL5380_CSCR_CD | ZL5380_CSCR_MSG)
#define	ZL5380_CSCR_MESSAGE_IN_PHASE	(ZL5380_CSCR_IO | ZL5380_CSCR_CD |\
					 ZL5380_CSCR_MSG)


/*
 *	Bus and Status register Bit Definitions
 */

#define	ZL5380_BSR_ACK				0x01
#define	ZL5380_BSR_ATN				0x02
#define	ZL5380_BSR_BUSY_ERROR			0x04
#define	ZL5380_BSR_PHASE_MATCH			0x08
#define	ZL5380_BSR_INT_REQ_ACTIVE	  	0x10
#define	ZL5380_BSR_PARITY_ERROR			0x20
#define	ZL5380_BSR_DMA_REQUEST			0x40
#define	ZL5380_BSR_END_OF_DMA_TRANSFER		0x80


/*
 *	SCSI Bus Timings in Micro Seconds
 */

#define	ZL5380_ARBITRATION_DELAY	3
#define	ZL5380_BUS_CLEAR_DELAY		1
#define	ZL5380_FREE_DELAY	 	1
#define	ZL5380_BUS_SET_DELAY		2
#define	ZL5380_BUS_SETTLE_DELAY		1
#define	ZL5380_RESET_HOLD_TIME		25

#define WAIT_COUNT			zl5380_wait_count
#define RETRY_COUNT			zl5380_retry_count
#define ARB_RETRY_COUNT			zl5380_arb_retry_count 
#define ZL5380_RESET_WAIT_COUNT		zl5380_reset_wait_count
#define ZL5380_RESET_DELAY_COUNT1	zl5380_reset_delay_count1
#define ZL5380_RESET_DELAY_COUNT2	zl5380_reset_delay_count2


/*
 *	SCSI Message Codes
 */

#define COMMAND_COMPLETE_MSG		0x00
#define EXTENDED_MSG			0x01
#define SAVE_DATA_POINTER_MSG		0x02
#define RESTORE_POINTERS_MSG		0x03
#define DISCONNECT_MSG			0x04
#define INITIATOR_DETECTED_ERROR_MSG	0x05
#define ABORT_MSG			0x06
#define MESSAGE_REJECT_MSG		0x07
#define NOP_MSG				0x08
#define MESSAGE_PARITY_ERROR_MSG	0x09
#define LINKED_COMMAND_COMPLETE_MSG	0x0A
#define BUS_DEVICE_RESET_MSG		0x0C
#define IDENTIFY_MSG			0x80
#define PERMIT_DISC			0x40

/* SCSI status definitions */

#define GOOD				0x00
#define CHECK_CONDITION			0x02
#define CONDITION_MET			0x04
#define BUSY				0x08
#define INTERMEDIATE			0x10
#define INTERMEDIATE_CONDITION_MET	0x14
#define RESERVATION_CONFLICT		0x18


/*
 * Status codes for commands
 */
#define SUCCESS		0
#define FAILURE		1
#define COMPLETED	3
#define DISCONNECTED	4
#define PENDING		5
#define NOTHING		6
#define ABORTED		7

#define PHASECHANGE	1
#define TIMEDOUT	2

/*
 * Flags for cleaning up on failures during initialisation
 */
#define HA_ZL5380HA_REL		0x1
#define HA_ZL5380SCB_REL	0x2
#define HA_DEV_REL		0x4
#define HA_LUCB_REL		0x10
#define HA_LOCK_REL		0x20

#define HA_ALL_REL	(HA_ZL5380HA_REL | HA_ZL5380SCB_REL | HA_DEV_REL \
				| HA_LUCB_REL | HA_LOCK_REL)

/*
 * Locking Macro Definitions
 *
 * LOCK/UNLOCK definitions are lock/unlock primaries for multi-processor or
 * spl/splx for uniprocessor.
 */

#define ZL5380_SCB_LOCK(p)     p = LOCK(zl5380ha->ha_scb_lock, pldisk)
#define ZL5380_HBA_LOCK(p)     p = LOCK(zl5380ha->ha_hba_lock, pldisk)
#define ZL5380_SCSILU_LOCK(q)  q->q_opri = LOCK(q->q_lock, pldisk)

#define ZL5380_SCB_UNLOCK(p)     UNLOCK(zl5380ha->ha_scb_lock, p)
#define ZL5380_HBA_UNLOCK(p)     UNLOCK(zl5380ha->ha_hba_lock, p)
#define ZL5380_SCSILU_UNLOCK(q)  UNLOCK(q->q_lock, q->q_opri)


/*
 * Macros to modify queue flags
 */

#define ZL5380_SET_QFLAG(q, flag) 	{				\
					ZL5380_SCSILU_LOCK(q);		\
					q->q_flag |= flag;		\
					ZL5380_SCSILU_UNLOCK(q);	\
					}
#define ZL5380_CLEAR_QFLAG(q, flag) 	{				\
					ZL5380_SCSILU_LOCK(q);		\
					q->q_flag &= ~flag;		\
					ZL5380_SCSILU_UNLOCK(q);	\
					}

/*
 * Locking Hierachy Definition
 */
#define ZL5380_HIER	HBA_HIER_BASE


#if defined (__cplusplus)
	}
#endif

#endif  /* ZL5380_IO_HBA_INCLUDED */

/*      Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.      */
/*      Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.        */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.      */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */


/*
 * This file contains the code for the zl5380 scsi functionality
 * driver.
 */

#ident  "@(#)kern-pdi:io/hba/zl5380/zl5380.c	1.1.1.3"

#ifdef	_KERNEL_HEADERS

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include "zl5380.h"
#include <io/ddi.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include "zl5380.h"
#include <sys/ddi.h>

#endif	/* _KERNEL_HEADERS */


/*
 *	Local functions
 */

void
zl5380initialize (zl5380ha_t *);

STATIC int
zl5380send_ack (zl5380ha_t *);

STATIC void
zl5380initiate_msg_send (zl5380ha_t *);

int
zl5380pio_write (zl5380ha_t *);

int
zl5380pio_read (zl5380ha_t *);

STATIC void 
zl5380reselection (zl5380ha_t *);

void
zl5380isr (zl5380ha_t *);

STATIC int
zl5380post_phase_process (zl5380ha_t *, int );

STATIC void
zl5380phase_control (zl5380ha_t *);

STATIC int
zl5380send_message (zl5380ha_t *);

STATIC int
zl5380receive_message (zl5380ha_t *);

STATIC int
zl5380receive_status (zl5380ha_t *);

STATIC int
zl5380send_command (zl5380ha_t *, zl5380scb_t *);

STATIC int
zl5380arbitrate_and_select (zl5380ha_t *);

void
zl5380transfer_scb (zl5380ha_t *, zl5380scb_t *);

int
zl5380findadapter (int);

STATIC int
zl5380scsi_bus_free (int );

STATIC int
zl5380wait_for_bit_set (int , int, int );

STATIC int
zl5380wait_for_bit_reset (int, int , int );

void
zl5380reset (zl5380ha_t *zl5380ha);

STATIC int
zl5380unlinkscb (zl5380scb_t **,struct  zl5380scb *);

STATIC void
zl5380linkscb (zl5380scb_t **, zl5380scb_t *);

int 
zl5380check_interrupt(zl5380ha_t *);

extern void zl5380scb_done (zl5380ha_t *, zl5380scb_t *);

extern int mv_pas16adapter(int);
extern int t160adapter(int);
extern void mv_pas16initialize(zl5380ha_t *);
extern void t160initialize(zl5380ha_t *);
extern int pc9010_data_transfer(zl5380ha_t *, int);


/*
 * int
 * zl5380check_interrupt(zl5380ha_t *zl5380ha)
 *
 * Description:
 *	Checks if the specified interupt configuration is correct
 * 
 * Returns:
 *	True if interrupts are working, False otherwise
 */
int zl5380check_interrupt(zl5380ha_t *zl5380ha)
{

	if (!zl5380ha->ha_vect)
		return FALSE;

	outb (MR, inb(MR) | ZL5380_MR_MONITOR_BUSY);
	zl5380ha->ha_intr_mode = FALSE;

	/*
	 * A longer delay seems necessary on faster machines
	 */
	drv_usecwait (ZL5380_BUS_SETTLE_DELAY * 100);
	if (!zl5380ha->ha_intr_mode){
		/*
 		 *+ Interrupt configuration is not valid
		 */
		cmn_err (CE_WARN, 
			"%s: IRQ %d has not been set correctly",
			zl5380ha->ha_name,
			zl5380ha->ha_vect);
	}

	return zl5380ha->ha_intr_mode;
}

/*
 * void
 * zl5380initialize (zl5380ha_t *zl5380ha)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *      Resets the registers, FIFO and the SCSI bus
 * Remarks:
 *	None
 */
void
zl5380initialize (zl5380ha_t *zl5380ha)
{

	switch (zl5380ha->ha_type) {
	case T160_ADAPTER:
		t160initialize(zl5380ha);
		break;
	case MV_PAS16_ADAPTER:
		mv_pas16initialize(zl5380ha);
		break;
	default:
		/*
		 *+ The adapter type is not known
		 */
		cmn_err (CE_WARN, "ZL5380 : Unknown Adapter");
		break;
	}

	/* Reset the registers */
	outb(ICR, 0);
	outb(MR, 0);
	outb(TCR, 0);

	/* Reset Scsi Bus */
	outb(ICR, ZL5380_ICR_ASSERT_RST);
	drv_usecwait(1000);
	outb(ICR, 0);

	/* Clear Interrupts */
	(void) inb(RPIR);

	return;
}


/*
 * STATIC void
 * zl5380linkscb (zl5380scb_t **queuehead, zl5380scb_t *scb)
 *	Links the supplied scb at the end of the queue.
 *
 * Calling/Exit State :
 *	ZL5380_HBA_LOCK (oip) is held on entry and exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC void
zl5380linkscb (zl5380scb_t **queuehead, zl5380scb_t *scb)
{
	while (*queuehead != 0) {
		queuehead = &((*queuehead)->c_chain);
	}
	scb->c_chain = *queuehead;
	*queuehead = scb;

}

/*
 * STATIC int
 * zl5380unlinkscb (zl5380scb_t **queuehead,zl5380scb_t *scb) 
 *	Unlinks an scb from the head of the eligible queue. Returns failure
 *      if the queue is empty or the given scb is not present.
 *
 * Calling/Exit State :
 *	ZL5380_HBA_LOCK (oip) is held on entry and exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380unlinkscb (zl5380scb_t **queuehead,zl5380scb_t *scb)
{
	while (*queuehead != 0)
		if (*queuehead == scb) {
			*queuehead = scb->c_chain;
			scb->c_chain = 0;
			return 0;
		} 
		else {
			queuehead = &(*queuehead)->c_chain;
		}

	return 1;
}

/*
 * void
 * zl5380reset (zl5380ha_t *zl5380ha)
 *	Resets the SCSI bus.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
void
zl5380reset (zl5380ha_t *zl5380ha)
{
	outb (ICR, ZL5380_ICR_ASSERT_RST);
	drv_usecwait (ZL5380_RESET_DELAY_COUNT1);
	outb (ICR, 0);
	drv_usecwait (ZL5380_RESET_DELAY_COUNT2);
	(void) inb (RPIR);
	outb (MR,0);
	outb (SER,0);
	drv_usecwait (ZL5380_RESET_WAIT_COUNT);
}

/*
 * STATIC int
 * zl5380wait_for_bit_set (int reg, int bit, int wait_count)
 *	Checks for a particular bit to be set. Returns success if the bit
 *	is set.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380wait_for_bit_set (int reg, int bit, int wait_count)
{
	register int	i;
	unsigned char 	val;
	for (i=0; i < wait_count; i++) {
		val = inb (reg);
		if ( val & bit) {
			return 0;
		}
	}
	return 1;
}

/*
 * STATIC int
 * zl5380wait_for_bit_reset (int reg, int bit, int wait_count)
 *	Checks for a particulr bit to be reset. Returns success if the bit is 
 *	cleared.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380wait_for_bit_reset (int reg, int bit, int wait_count)
{
	register int	i;
	unsigned char 	val;
	for (i=0; i < wait_count; i++) {
		val = inb (reg);
		if (!(val & bit)) {
			return 0;
		}
	}
	return 1;
}

/*
 * STATIC int
 * zl5380scsi_bus_free(int reg)
 *	Checks for SCSI bus free. Both BSY and SEL should be continuously false
 *	for atleast for a bus settle delay.
 *
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380scsi_bus_free(int reg)
{
	register int	i;

	for (i=0; i < RETRY_COUNT; i++) {
		if(!(inb (reg) & (ZL5380_CSCR_BUS_FREE))) {
			drv_usecwait (ZL5380_BUS_SETTLE_DELAY);
			if(!(inb (reg) & (ZL5380_CSCR_BUS_FREE))) {
				return 0;
			}
		}
	}
	return 1;
}


/*
 * int
 * zl5380findadapter(int baseaddress)
 *	Checks whether the adapter is present at the given base address. 
 *	Returns TRUE on success.
 *
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
int
zl5380findadapter(int base)
{

	if (mv_pas16adapter(base)) {
		return MV_PAS16_ADAPTER;
	}
	else if (t160adapter(base)) {
		return T160_ADAPTER;
	}
	else {
		return UNKNOWN_ADAPTER;
	}
}


/*
 * void 
 * zl5380transfer_scb (zl5380ha_t *zl5380ha, zl5380scb_t *pscb)
 *	This function queues an scb on the eligible scb queue and starts 
 *	arbitration if the SCSI bus is free. 
 *
 * Calling/Exit State :
 *	ZL5380_HBA_LOCK (oip) is held on entry and exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
void 
zl5380transfer_scb (zl5380ha_t *zl5380ha, zl5380scb_t *pscb)
{
	zl5380lucb_t 	*lucb;
	pl_t	oip;

	lucb = LUCB (zl5380ha,pscb->c_target,pscb->c_lun);

	ZL5380_HBA_LOCK(oip);
	lucb->activescb = pscb;
	zl5380linkscb (&zl5380ha->ha_eligiblescb, pscb);
	if (zl5380ha->ha_currentscb == NULL) {
		if (!zl5380scsi_bus_free (CSCR)) {
			if (!zl5380arbitrate_and_select(zl5380ha)) {
			        /*
				 * Arbitration and Selection successful
				 */
				ZL5380_HBA_UNLOCK(oip);
				zl5380phase_control(zl5380ha);
				return;
			}
		}
	}
	ZL5380_HBA_UNLOCK(oip);
	return;
}

/*
 * STATIC int 
 * zl5380arbitrate_and_select(zl5380ha_t *zl5380ha)
 *	Starts arbitration and selects the given target. Calls zl5380scb_done
 * 	with SCB_TIMEOUT or SCB_SELECTION_TIMEOUT on failure.
 *
 * Calling/Exit State :
 *	ZL5380_HBA_LOCK (oip) is held on entry and exit.
 *	Returns 1 on error, 0 on success
 *
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380arbitrate_and_select(zl5380ha_t *zl5380ha)
{
	unsigned char		temp;
	int			status=FAILURE;
	int			i=0;
	zl5380scb_t		*pscb=zl5380ha->ha_eligiblescb;

	zl5380ha->ha_currentscb = pscb;
	zl5380ha->ha_datapointer = pscb->c_datapointer;
	zl5380ha->ha_datalength = pscb->c_datalength;

	if (pscb->c_opcode == SCB_EXECUTE) {
		zl5380ha->ha_msgout = IDENTIFY_MSG | pscb->c_lun | PERMIT_DISC;
	}
	else {
		zl5380ha->ha_msgout = BUS_DEVICE_RESET_MSG;
	}

	outb (TCR,0);
	outb (MR,(inb (MR) & ~ZL5380_MR_ARBITRATE));
	outb (ICR,0);
	while (i < ARB_RETRY_COUNT) {
		if(zl5380scsi_bus_free (CSCR)) {
			zl5380ha->ha_currentscb = 0;
			return 1;
		}
		outb (ODR, zl5380ha->ha_bitid);
		outb (MR, (inb (MR) | ZL5380_MR_ARBITRATE));
		if(zl5380wait_for_bit_set (
		    ICR,
		    ZL5380_ICR_ARBITRATION_IN_PROGRESS,
		    WAIT_COUNT)) {
			outb (MR,(inb (MR) & ~ZL5380_MR_ARBITRATE));
			pscb->c_status = SCB_TIMEOUT;
			status = FAILURE;
			break;
		} else {
			drv_usecwait (ZL5380_ARBITRATION_DELAY);
			if(!(inb (ICR) & ZL5380_ICR_LOST_ARBITRATION)) {
				if((~((int)inb (CSDR)) < (int)zl5380ha->ha_bitid))
				{
					status=SUCCESS;
					break;
				}
			}
		}
		outb (MR,(inb (MR) & ~ZL5380_MR_ARBITRATE));
		i++;
	}

	if (status == FAILURE)
	{
	        /*
		 * Arbitration failed
		 */
		outb (ICR, 0);
		outb (MR,(inb (MR) & ~ZL5380_MR_ARBITRATE));
		(void) inb (RPIR);
		zl5380unlinkscb (&zl5380ha->ha_eligiblescb, pscb);
		zl5380ha->ha_currentscb = NULL;
		pscb->c_status = SCB_TIMEOUT;
		zl5380scb_done (zl5380ha, pscb);
		return 1;
	}

	temp 	= ZL5380_ICR_ASSERT_BSY 
	    | ZL5380_ICR_ASSERT_DATA_BUS 
	    | ZL5380_ICR_ASSERT_SEL;
	/* 
	 * Disconnection only if interrupts are enabled
	 */
	if (zl5380ha->ha_intr_mode) {
		temp |= ZL5380_ICR_ASSERT_ATN;
	}

	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING) | temp));
	drv_usecwait (ZL5380_BUS_CLEAR_DELAY + ZL5380_BUS_SETTLE_DELAY);
	outb (ODR, (1 << pscb->c_target) | zl5380ha->ha_bitid);
	outb(MR,(inb (MR) & ~ZL5380_MR_ARBITRATE));
	outb (SER, 0);
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    & ~ZL5380_ICR_ASSERT_BSY));

	if (zl5380wait_for_bit_set (CSCR, ZL5380_CSCR_BSY,WAIT_COUNT)) {
	        /*
		 * Selection Failed
		 */
		outb (ICR, 0);
		outb (MR, 0);
		(void) inb (RPIR);
		zl5380unlinkscb (&zl5380ha->ha_eligiblescb, pscb);
		pscb->c_status = SCB_SELECTION_TIMEOUT;
		zl5380ha->ha_currentscb = NULL;
		zl5380scb_done (zl5380ha, pscb);
		return 1;
	}

	temp =  ((int)~(ZL5380_ICR_ASSERT_SEL 
	    | ZL5380_ICR_ASSERT_DATA_BUS));
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING) & temp));

	zl5380unlinkscb (&zl5380ha->ha_eligiblescb, pscb);

#ifdef ZL5380_DEBUG
	cmn_err (CE_CONT, 
		"!Arbitration and selection successful for c%db0t%dl0 \n",
		pscb->c_adapter, pscb->c_target);
#endif

	return 0;    /* Arbitration and Selection successful */
}

/*
 * STATIC  void
 * zl5380reselection(zl5380ha_t *zl5380ha)
 *	This function carries out reselection procedure.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *	
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC void
zl5380reselection(zl5380ha_t *zl5380ha)
{
	register unsigned char	targetid;
	register unsigned char  x,y;

	/*
	 * CSDR should have only the target id and host id bits set
	 */
	targetid = inb (CSDR) ^ zl5380ha->ha_bitid;

	/*
	 * Starting with the target id value shift left till the LSB
	 * goes high or all bits are 0
	 * The reselecting target's id is also determined in y
	 */
	for (x = targetid, y = 0; x != 0; x = x >> 1, y++) {
		if (x & 0x01) {
			/* LSB is set */
			break;
		}
	}

	if (x ^ 0x01)
	{
		/*
		 * More than one bit is set besides the ha id
		 */
		return;
	}

	outb (SER, 0);
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    | ZL5380_ICR_ASSERT_BSY));
	(void) inb (RPIR);
	if (zl5380wait_for_bit_reset (CSCR,ZL5380_CSCR_SEL,WAIT_COUNT))
	{
		return;
	}

	zl5380ha->ha_reselectingtarget = y;
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    & ~ZL5380_ICR_ASSERT_BSY));

	zl5380phase_control(zl5380ha);

	return;
}

/*
 * STATIC int
 * zl5380send_command(zl5380ha_t *zl5380ha, zl5380scb_t *pscb)
 *	This function sends a SCSI command to the LUN.
 *	Returns ABORTED on failure.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *	
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380send_command(zl5380ha_t *zl5380ha, zl5380scb_t *pscb)
{
	register unsigned char	temp;
	register int		count=0;

	outb (TCR, ZL5380_TCR_COMMAND_PHASE);
	while ((int)count < (int)pscb->c_cmdsz)
	{
		if (zl5380wait_for_bit_set (CSCR,ZL5380_CSCR_REQ,WAIT_COUNT))
		{
			return ABORTED;
		}
		temp = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
		if (!(temp & ZL5380_CSCR_COMMAND_PHASE))
		{
			return ABORTED;
		}
		outb (ODR, pscb->c_cdb[count++]);
		outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
		    | ZL5380_ICR_ASSERT_DATA_BUS
		    | ZL5380_ICR_ASSERT_ACK));

		if (zl5380wait_for_bit_reset(CSCR,ZL5380_CSCR_REQ,WAIT_COUNT)) {
			return ABORTED;
		}
		temp = ((int)~(ZL5380_ICR_ASSERT_DATA_BUS 
		    | ZL5380_ICR_ASSERT_ACK));
		outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
		    & temp));
	}

	return SUCCESS;
}

/*
 * STATIC int
 * zl5380receive_status(zl5380ha_t *zl5380ha)
 *	Receives the status of the command sent to the LUN.
 *	Returns ABORTED on failure.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380receive_status(zl5380ha_t *zl5380ha)
{
	register unsigned char	temp;

	outb (TCR, ZL5380_TCR_STATUS_PHASE);
	temp = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
	if (temp != ZL5380_CSCR_STATUS_PHASE)
	{
		return ABORTED;
	}

	zl5380ha->ha_targetstatus = inb (CSDR);
	if (zl5380send_ack(zl5380ha))
	{
		return ABORTED;
	}
	return SUCCESS;
}

/*
 * STATIC int
 * zl5380receive_message (zl5380ha_t *zl5380ha)
 *	This function receives and interprets the message.
 *	Returns ABORTED on failure.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *	
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380receive_message (zl5380ha_t *zl5380ha)
{

	register unsigned char	temp;
	register unsigned char	message;
	zl5380lucb_t	        *lucb;
	zl5380scb_t *scb = zl5380ha->ha_currentscb;

	outb (TCR, ZL5380_TCR_MESSAGE_IN_PHASE);
	temp = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
	if (temp != ZL5380_CSCR_MESSAGE_IN_PHASE)
	{
		return ABORTED;
	}
	message = inb (CSDR);

	if (message & IDENTIFY_MSG)
	{
		zl5380ha->ha_lun = message & 0x07;
		if(zl5380ha->ha_reselectingtarget == 0x07) {
			zl5380initiate_msg_send (zl5380ha);
		}
		lucb = LUCB (zl5380ha,zl5380ha->ha_reselectingtarget,
					 zl5380ha->ha_lun);
		zl5380ha->ha_disconnects --;
		zl5380ha->ha_currentscb = scb = lucb->activescb;
		if (scb) {
			zl5380ha->ha_datapointer = scb->c_datapointer;
			zl5380ha->ha_datalength = scb->c_datalength;
		} else {
			zl5380initiate_msg_send (zl5380ha);
		}
	} else {
		switch (message) {

		case COMMAND_COMPLETE_MSG:
			if (scb->c_flags != REQUEST_SENSE_PENDING ) {
				switch (zl5380ha->ha_targetstatus) {

				case CONDITION_MET:
				case GOOD:
					scb->c_status=SCB_COMPLETED_OK;
					break;

				case CHECK_CONDITION:
					scb->c_status = SCB_ERROR;
					scb->c_flags = REQUEST_SENSE_PENDING;
					break;

				default:
					scb->c_status = SCB_ERROR;
					break;
				}
			}
			else {
				if (zl5380ha->ha_targetstatus == GOOD) {
					scb->c_status = SCB_ERROR | 
					    SCB_SENSE_DATA_VALID;
					scb->c_flags=0;
				}
				else {
					scb->c_status = SCB_ERROR;
					scb->c_flags=0;
				}
			}
			if (zl5380send_ack (zl5380ha)) {
				return (ABORTED);
			}
			return COMPLETED;

		case SAVE_DATA_POINTER_MSG:
			scb->c_datapointer = zl5380ha->ha_datapointer;
			scb->c_datalength = zl5380ha->ha_datalength;
			break;

		case RESTORE_POINTERS_MSG:
			zl5380ha->ha_datapointer = scb->c_datapointer;
			zl5380ha->ha_datalength = scb->c_datalength;
			break;

		case DISCONNECT_MSG:
			if (zl5380send_ack (zl5380ha)) {
				return (ABORTED);
			}
			zl5380ha->ha_disconnects++;
			scb->c_flags = DISCONNECT;
			scb->c_status = SCB_PENDING;
			return DISCONNECTED;

		case MESSAGE_REJECT_MSG:
			if (scb->c_cdb[0] == IDENTIFY_MSG) {
				scb->c_status = SCB_INVALID_LUN;
				if (zl5380send_ack (zl5380ha)) {
					return (ABORTED);
				}
				zl5380initiate_msg_send (zl5380ha);
				return COMPLETED;
			}
			break;
		default:
			zl5380initiate_msg_send (zl5380ha);
			break;
		}
	}
	if (zl5380send_ack (zl5380ha)) {
		return (ABORTED);
	}
	return SUCCESS;
}

/*
 * STATIC int
 * zl5380send_ack (zl5380ha_t *zl5380ha)
 *	Sends an acknowledgement to the target device.
 *	Returns FALSE on failure.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *	
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380send_ack (zl5380ha_t *zl5380ha)
{
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    | ZL5380_ICR_ASSERT_ACK));

	outb (SER,0);
	(void) inb (RPIR);
	if (zl5380wait_for_bit_reset (CSCR,ZL5380_CSCR_REQ,WAIT_COUNT)) {
		outb (ICR, 0);
		return 1;
	}
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    & ~ZL5380_ICR_ASSERT_ACK));

	return 0;
}

/*
 * STATIC int
 * zl5380send_message(zl5380ha_t *zl5380ha, zl5380scb_t *pscb)
 *	Sends a message to the target.
 *	Returns FALSE on failure.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None.
 */
STATIC int
zl5380send_message(zl5380ha_t *zl5380ha)
{
	register unsigned char	temp;

	outb (TCR, ZL5380_TCR_MESSAGE_OUT_PHASE);
	temp = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
	if (temp != ZL5380_CSCR_MESSAGE_OUT_PHASE) {
		return ABORTED;
	}
	outb (ODR, zl5380ha->ha_msgout);
	outb (ICR ,((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    | ZL5380_ICR_ASSERT_DATA_BUS
	    | ZL5380_ICR_ASSERT_ACK));
	outb (ICR ,((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    & ~ZL5380_ICR_ASSERT_ATN));

	if (zl5380wait_for_bit_reset (CSCR, ZL5380_CSCR_REQ, WAIT_COUNT)) {
		outb (ICR , 0);
		return ABORTED;
	}

	temp = ((int)~(ZL5380_ICR_ASSERT_DATA_BUS | ZL5380_ICR_ASSERT_ACK));
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING) & temp));

	return SUCCESS;
}

/*
 * STATIC void
 * zl5380phase_control (zl5380ha_t *zl5380ha)
 *	This function manages the SCSI bus phases. Calls the appropriate phase
 * 	handling routine.
 *	Returns ABORTED on failure.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *	
 * Description:
 *
 * Remarks:
 *	None
 */
STATIC void
zl5380phase_control (zl5380ha_t *zl5380ha)
{
	register unsigned char	phase;
	register int		status;
	unsigned int		time_to_wait;

#ifdef ZL5380_DEBUG
	zl5380scb_t		*pscb = zl5380ha->ha_currentscb;
#endif

	time_to_wait = WAIT_COUNT;
	if (!zl5380ha->ha_intr_mode) {
		/*
		 * In case of polling mode, we have to wait for the extra
		 * duration to get the data in phase; Hence the *1000
		 * Particularly in case of jobs like retensioning of tape
		 */
		time_to_wait *= 1000;
	}

	do {
		if (zl5380wait_for_bit_set(CSCR,
					   ZL5380_CSCR_REQ,time_to_wait)) {
			status = ABORTED;
			/*
			 *+ The job timed out.  Try increasing the wait
			 *+ variable
			 */
			cmn_err (CE_WARN, "!%s : job timed out",
				zl5380ha->ha_name);
			break ;
		}

		phase = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;

		switch(phase) {

		case ZL5380_CSCR_DATA_OUT_PHASE:
			if (zl5380ha->ha_type == T160_ADAPTER) 
				status = pc9010_data_transfer (zl5380ha, 0);
			else /* MV PAS 16 */
				status = zl5380pio_write (zl5380ha);
			break;

		case ZL5380_CSCR_DATA_IN_PHASE:
			if (zl5380ha->ha_type == T160_ADAPTER) 
       				status = pc9010_data_transfer(zl5380ha, 1);
			else /* MV PAS 16 */
				status = zl5380pio_read (zl5380ha);
			break;

		case ZL5380_CSCR_COMMAND_PHASE:
			status = zl5380send_command (zl5380ha, 
						     zl5380ha->ha_currentscb);
			break;

		case ZL5380_CSCR_STATUS_PHASE:
			status = zl5380receive_status (zl5380ha);
			break;

		case ZL5380_CSCR_MESSAGE_OUT_PHASE:
			status = zl5380send_message (zl5380ha);
			break;

		case ZL5380_CSCR_MESSAGE_IN_PHASE:
			status = zl5380receive_message (zl5380ha);
			break;

		default:
			status = NOTHING;
			break;
		}
#ifdef ZL5380_DEBUG
		pscb = zl5380ha->ha_currentscb;
		if (pscb) {
			cmn_err (CE_CONT,
				"!ZL5380:c%db0t%dl0:Phase %x done;status= %x\n",
				pscb->c_adapter, pscb->c_target, phase, status);
		}
		else {
			cmn_err (CE_CONT,
				"!ZL5380: %s disconnected\n",
				zl5380ha->ha_name);
		}
#endif
		/*
		 * The loop is continued if job is not completed, disconnected
		 * or aborted.  If it is completed, disconnected or aborted,
		 * we continue, if post phase processing starts off processing 
		 * new job.  This avoids mutual recursion which appears to
		 * cause stack overflow under some circumstances.
		 */

	}  while (((status != COMPLETED) && (status != DISCONNECTED) &&
		   (status != ABORTED)) ||
		  (zl5380post_phase_process (zl5380ha, status)));

}

/*
 * STATIC int
 * zl5380post_phase_process (zl5380ha_t *zl5380ha, int status)
 *	This function does the processing required after an information 
 *	transfer phase is complete.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *	Acquires ZL5380_HBA_LOCK (oip).
 *
 * Description:
 * 	Returns 1 if a new job processing has been initiated,
 *	0 otherwise
 *
 * Remarks:
 *	None
 */
STATIC int
zl5380post_phase_process (zl5380ha_t *zl5380ha, int status)
{
	zl5380scb_t 		*pscb = zl5380ha->ha_currentscb;
	int			retval;
	pl_t			oip;

	switch (status) {
	case COMPLETED :
		if (pscb->c_flags == REQUEST_SENSE_PENDING) {
			/*
			 * Automatic Request sensing
			 */
			pscb->c_datapointer = SENSE_AD (&pscb->c_sense);
			pscb->c_datalength = SENSE_SZ;
			pscb->c_cdb[0] = SS_REQSEN;
			pscb->c_cdb[1] = pscb->c_cdb[2] = pscb->c_cdb[3] = 
						pscb->c_cdb[5] = 0;
			pscb->c_cdb[4] = SENSE_SZ;
			pscb->c_cmdsz = 6;
			ZL5380_HBA_LOCK (oip);
			LINKSCB_AT_HEAD (&zl5380ha->ha_eligiblescb, pscb)
			zl5380ha->ha_currentscb = (zl5380scb_t *)0;
			ZL5380_HBA_UNLOCK (oip);
		}
		else 
		{
			/*
			 * No pending request sense
			 */
			zl5380scb_done (zl5380ha,pscb);
			ZL5380_HBA_LOCK (oip);
			zl5380ha->ha_currentscb = (zl5380scb_t *)0;
			ZL5380_HBA_UNLOCK (oip);
		}
		outb (SER,0);
		outb (SER,zl5380ha->ha_bitid);
		break;

	case DISCONNECTED:
		/*
		 * The current scb is not being cleared because issuing
		 * another job when another is disconnected seems to 
		 * cause a job to hang occasionally with a tape drive and
		 * and hard disk working in parallel on same controller
		 * Looking into it. 
       		 * ZL5380_HBA_LOCK (oip);
		 * zl5380ha->ha_currentscb = (zl5380scb_t *)0;
		 * ZL5380_HBA_UNLOCK (oip);
		 */
		outb (SER,0);
		outb (SER,zl5380ha->ha_bitid);
		break;

	case ABORTED:
		pscb->c_status = SCB_ABORTED;
		outb (SER,0);
		outb (SER,zl5380ha->ha_bitid);
		ZL5380_HBA_LOCK (oip);
		zl5380ha->ha_currentscb = (zl5380scb_t *)0;
		ZL5380_HBA_UNLOCK (oip);
		zl5380scb_done (zl5380ha,pscb);
		break;
	default :
		break;
	}
	ZL5380_HBA_LOCK (oip);
	if (zl5380ha->ha_eligiblescb) {
		if (zl5380ha->ha_currentscb == NULL ) {
			if(!zl5380scsi_bus_free(CSCR)) {
				pscb = zl5380ha->ha_eligiblescb;
				if (!pscb) {
					ZL5380_HBA_UNLOCK (oip);
					return 0;
				}
				retval=zl5380arbitrate_and_select (zl5380ha);
				ZL5380_HBA_UNLOCK (oip);

				if (!retval) {
				        /*
					 * Arbitration and selection successful
					 */
					return 1;
				}
				/*
				 * If Arbitration and selection failed, 
				 * sdi_callback is called already
				 */
				return 0;
			}
		}
	}
	ZL5380_HBA_UNLOCK (oip);
	return 0;
}


#define zl5380reselection_in_progress()				\
	 (!(((inb(CSCR) & (ZL5380_CSCR_SEL | ZL5380_CSCR_IO)) == 	\
			(ZL5380_CSCR_SEL |ZL5380_CSCR_IO)) &&	\
	    (inb(CSDR) & zl5380ha->ha_bitid))) 		

/*
 * int
 * zl5380isr (zl5380ha_t *zl5380ha)
 *	Low level Interrupt service routine for ZL5380. 
 * 	Services the following interrupts.
 *	## Reselection
 *
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
void
zl5380isr (zl5380ha_t *zl5380ha)
{
	(void) inb (RPIR);
	if (!zl5380ha->ha_intr_mode) {
		/*
		 * The interrupt setting on the card is being tested
		 */
		outb (MR, inb(MR) & ~ZL5380_MR_MONITOR_BUSY);
		zl5380ha->ha_intr_mode = TRUE;
		return;
	}

	if (!(zl5380reselection_in_progress ())) {
		zl5380reselection (zl5380ha);
	}
}

/*
 * STATIC int
 * zl5380initiate_msg_send (zl5380ha_t *zl5380ha)
 *	Asserts ATN so that target goes into message out phase.
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */

STATIC void
zl5380initiate_msg_send (zl5380ha_t *zl5380ha)
{
	zl5380ha->ha_msgout = MESSAGE_REJECT_MSG;
	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
	    | ZL5380_ICR_ASSERT_ATN));
}

/*
 * int
 * zl5380pio_write (zl5380ha_t *zl5380ha)
 *	Writes data in programmed I/O mode.
 *
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
int
zl5380pio_write (zl5380ha_t *zl5380ha)
{
	register uchar_t	temp;
	register uchar_t	phase;

	temp = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
	outb (TCR,temp >> 2);
	while (zl5380ha->ha_datalength) {
	       	outb (ODR,*zl5380ha->ha_datapointer);
	       	if ((zl5380wait_for_bit_set(
	       	    CSCR,ZL5380_CSCR_REQ,WAIT_COUNT))) {
	       		return ABORTED;
	       	}
	       	phase = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
	       	if(phase != ZL5380_CSCR_DATA_OUT_PHASE) {
	       		return NOTHING;
	       	}
	       	outb(ICR,((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
			    | ZL5380_ICR_ASSERT_ACK
			    | ZL5380_ICR_ASSERT_DATA_BUS));
      
		if ((zl5380wait_for_bit_reset (
		    CSCR,ZL5380_CSCR_REQ,WAIT_COUNT))) {
			return NOTHING;
		}
	       	zl5380ha->ha_datapointer++;
		zl5380ha->ha_datalength--;
      
	       	temp = ((int)~(ZL5380_ICR_ASSERT_DATA_BUS 
			    | ZL5380_ICR_ASSERT_ACK));
	       	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
			    & temp));
	}
	return (NOTHING);
}


/*
 * int
 * zl5380pio_read (zl5380ha_t *zl5380ha)
 *	Reads data in programmed I/O mode.
 *
 * Calling/Exit State :
 *	No locks are held on entry or exit.
 *
 * Description:
 *
 * Remarks:
 *	None
 */
int
zl5380pio_read (zl5380ha_t *zl5380ha)
{
	register uchar_t	temp;
	register uchar_t	phase;

	temp = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
	outb (TCR,temp >> 2);
	while (zl5380ha->ha_datalength) {
	       	if ((zl5380wait_for_bit_set(
	       	    CSCR,ZL5380_CSCR_REQ,WAIT_COUNT))) {
	       		return ABORTED;
	       	}
	       	phase = inb (CSCR) & ZL5380_CSCR_PHASE_MASK;
	       	if(phase != ZL5380_CSCR_DATA_IN_PHASE) {
	       		return NOTHING;
	       	}
		*zl5380ha->ha_datapointer = inb (CSDR);
	       	outb(ICR,((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
			    | ZL5380_ICR_ASSERT_ACK));
      
		if ((zl5380wait_for_bit_reset (
		    CSCR,ZL5380_CSCR_REQ,WAIT_COUNT))) {
			return NOTHING;
		}
	       	zl5380ha->ha_datapointer++;
		zl5380ha->ha_datalength--;
      
	       	outb (ICR, ((inb (ICR) & ZL5380_ICR_READ_MASK_FOR_WRITING)
			    & ~ZL5380_ICR_ASSERT_ACK));
	}
	return (NOTHING);
}

#ifdef ZL5380_DEBUG

/*
 * Printing zl5380ha_t data structure
 */
void
zl5380ha_dump(zl5380ha_t *zl5380ha)
{
	cmn_err (CE_CONT, "ZL5380HA Pointer = %x\n", zl5380ha);
	cmn_err (CE_CONT, "Current SCB = %x\n", zl5380ha->ha_currentscb);
	cmn_err (CE_CONT, "Eligible SCB Queue Head= %x\n", zl5380ha->ha_eligiblescb);
	cmn_err (CE_CONT, "Command Block List Head= %x\n", zl5380ha->ha_scb);
	cmn_err (CE_CONT, "Data Pointer = %x\n", zl5380ha->ha_datapointer);
	cmn_err (CE_CONT, "Data Length = %x\n", zl5380ha->ha_datalength);
}

/*
 * Printing zl5380scb_t data structures
 */
void
zl5380scb_dump(zl5380scb_t *zl5380scb)
{
	cmn_err (CE_CONT, "ZL5380 SCB Command Pointer %x \n", zl5380scb);
	cmn_err (CE_CONT, "Next SCB Command Block %x \n", zl5380scb->c_next);
	if (!zl5380scb->c_active) {
		cmn_err (CE_CONT, "Command Block is not active\n");
		return;
	}
	cmn_err (CE_CONT, "Next Eligible SCB Command Block %x \n", zl5380scb->c_chain);
	cmn_err (CE_CONT, "SCB Command Data Pointer %x \n", zl5380scb->c_datapointer);
	cmn_err (CE_CONT, "SCB Command Data Length %x \n", zl5380scb->c_datalength);
}
	
#endif

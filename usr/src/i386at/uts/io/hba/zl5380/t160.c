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

#ident  "@(#)kern-pdi:io/hba/zl5380/t160.c	1.1.1.1"

#ifdef  _KERNEL_HEADERS

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include "zl5380.h"
#include "t160.h"
#include <io/ddi.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include "zl5380.h"
#include "t160.h"
#include <sys/ddi.h>

#endif  /* _KERNEL_HEADERS */

/*
 * Local Functions
 */
STATIC void pc9010_fifo_setup(zl5380ha_t *, int);
STATIC int pc9010_read_16bit(zl5380ha_t *);
STATIC int pc9010_write_16bit(zl5380ha_t *);
STATIC int pc9010_wait_fifo_ready(zl5380ha_t *, int, int);
STATIC int pc9010_wait_fifo_empty(zl5380ha_t *, int);

int pc9010_data_transfer(zl5380ha_t *, int);
int t160adapter(int base);
void t160initialize(zl5380ha_t *);

extern int zl5380pio_write(zl5380ha_t *);

/*
 * t160initialize(zl5380ha_t *zl5380ha)
 *
 * Description:
 *	Initializes the FIFO registers
 *
 * Returns:
 * 	None
 */
void t160initialize(zl5380ha_t *zl5380ha)
{	
	CSDR = zl5380ha->ha_base + 0x08;
	ODR = zl5380ha->ha_base + 0x08;
	ICR = zl5380ha->ha_base + 0x09;
	MR = zl5380ha->ha_base + 0x0a;
	TCR = zl5380ha->ha_base + 0x0b;
	CSCR = zl5380ha->ha_base + 0x0c;
	SER = zl5380ha->ha_base + 0x0c;
	BSR = zl5380ha->ha_base + 0x0d;
	SDSR = zl5380ha->ha_base + 0x0d;
	IDR = zl5380ha->ha_base + 0x0e;
	SDTR = zl5380ha->ha_base + 0x0e;
	RPIR = zl5380ha->ha_base + 0x0f;
	SDIR = zl5380ha->ha_base + 0x0f;

	/* Reset the FIFO */
	outb (CONTROL, CTR_FRST);

	/* Reset the control reg. of PC9010, keep intr. enabled */
	outb (CONTROL, CTR_IRQEN);	

	return;
}

/*
 * t160adapter(int base)
 *
 * Description:  
 *	Checks if a T160 card is present at the given address
 *
 * Returns: 1 if it is a T160 card, else 0
 */

int
t160adapter(int base)
{
	outb(base + PC9010_CONTROL,
		inb(base + PC9010_CONTROL) & (~CTR_CONFIG));

	if (inb(base + PC9010_CONTROL) & CTR_CONFIG) {
               return 0;
	}

	outb(base + PC9010_CONTROL, inb(base + PC9010_CONTROL) | CTR_CONFIG);
        if (! (inb(base + PC9010_CONTROL) & CTR_CONFIG)) {
                return 0;
	}
 
        /* Config bit is now set.  Read the configuration information */
 
        if (inb(base + PC9010_CONFIG) != PC9010_JEDEC_ID) {
                return 0;
	}

        /* this byte is no of continuation chars, must be 0 */
        if (inb(base + PC9010_CONFIG)) {
                return 0;
        }
 
	return 1;
}

/*
 * pc9010_fifo_setup(zl5380ha_t *zl5380ha, int cmd_is_read)
 *      Sets up the FIFO registers for data transfer through FIFO and
 *      enables the 5380 dma
 *
 * Calling/Exit State:
 *      returns - none
 *      locks - no locks are held on entry or exit
 *
 * Remarks:
 *      Note that FIFO is not enabled in this routine
 */

STATIC void
pc9010_fifo_setup(zl5380ha_t *zl5380ha, int cmd_is_read)
{
        /* disable FIFO, then reset it. */
        outb(CONTROL, (inb(CONTROL) & (~CTR_FEN)));
        outb(CONTROL, (inb(CONTROL) | CTR_FRST));

        outb(CONTROL, (inb(CONTROL) &
            (~(CTR_FRST | CTR_CONFIG | CTR_SWSEL))));
        if (cmd_is_read)
                outb(CONTROL, inb(CONTROL)&~CTR_FDIR);
        else
                outb(CONTROL, inb(CONTROL)| CTR_FDIR);

        if (inb(CONFIG) & TRANSFER_MODE_8BIT) {
                /*
                 *+ 8-bit data transfer mode is not supported.
                 *+ Check the card configuration.
                 */
                cmn_err(CE_WARN,
                        "%s :: Eight Bit FIFO Transfer not supported",
                        zl5380ha->ha_name);
                return;
        }

        /* Enable 16 bit data transfer */
        outb(CONTROL, (inb(CONTROL) | CTR_F16));

        /*
         * Enable dma on the 5380
         */

        /* clear the interrupts */
	(void) inb(RPIR);

	/* enable DMA */
	outb(MR, (inb(MR) | ZL5380_MR_DMA_MODE));
	if (cmd_is_read)
		outb(SDIR , 1);
	else {
		outb(SDSR , 1);
		outb(ICR, inb(ICR) | ZL5380_ICR_ASSERT_DATA_BUS);
	}
		
	return;
}

/*
 * pc9010_data_transfer(zl5380ha_t *zl5380ha, int cmd_is_read)
 *	Reads/Writes data from/to the FIFO	
 * 
 * Calling/Exit State:
 *      cmd_is_read is 1 in case of read, 0 of write
 *	No locks are held on entry or exit
 *	returns SUCCESS on complete data transfer, ABORTED on failure
 *
 */

int
pc9010_data_transfer(zl5380ha_t *zl5380ha, int cmd_is_read)
{
	int transfer_status;
	int i, ack_count = 3;

	/* Put DATAIN/OUT into the Target command register */
	outb(TCR, 
	     (cmd_is_read ? ZL5380_TCR_DATA_IN_PHASE : 
			    ZL5380_TCR_DATA_OUT_PHASE));

	/* Set up the FIFO, enable DMA read and then FIFO */
	pc9010_fifo_setup(zl5380ha, cmd_is_read);

	outb(CONTROL, (inb(CONTROL) | CTR_FEN));

#ifdef T160_DEBUG
	cmn_err (CE_CONT, "Data transfer from %x %d SCB %x %d bytes Dir %d\n", 
		zl5380ha->ha_datapointer,
		zl5380ha->ha_datalength,
		zl5380ha->ha_currentscb->c_datapointer,
		zl5380ha->ha_currentscb->c_datalength,
		cmd_is_read);
#endif
	/* Read/Write the data in 16bit mode */
	if (cmd_is_read) {
	        transfer_status = pc9010_read_16bit(zl5380ha);
        }
	else {
	        transfer_status = pc9010_write_16bit(zl5380ha);
        }

	/* Disable the FIFO and then clear the DMA mode */
	outb(CONTROL, (inb(CONTROL) & (~CTR_FEN)));
	while (inb(CONTROL) & CTR_FEN)
		;

	if (cmd_is_read) {
		outb(MR, (inb(MR) & (~ZL5380_MR_DMA_MODE)));
		return transfer_status;
	}

	/*
	 * We reach here only in case of write
	 */

	/*
	 * In case of write, while disabling DMA, ensure that the
	 * last byte in the FIFO has been sent out before disabling
	 */

	outb(ICR, inb(ICR) &~ZL5380_ICR_ASSERT_DATA_BUS);
	for (i = 0; i < 10000; i ++) {
		if (inb(CSCR) & ZL5380_CSCR_REQ) {
			ack_count = 3;
			if (!inb (BSR) & ZL5380_BSR_PHASE_MATCH)
				break;
		}
		else {
			if (inb(BSR) & ZL5380_BSR_ACK) {
				ack_count --;
				if (ack_count == 0) break;
			}
		}
	}
	outb(MR, (inb(MR) & (~ZL5380_MR_DMA_MODE)));
	
	if ((transfer_status == SUCCESS) && 
	    (zl5380ha->ha_datalength == 1)) {
	        /*
		 * Transfer last byte thru programmed io
		 */
	        transfer_status = zl5380pio_write (zl5380ha);
	}
	return transfer_status;
}

/*
 * pc9010_read_16bit(zl5380ha_t *zl5380ha)
 *	Performs the actual reading of the data in 16 bit mode
 *
 * Calling/Exit state:
 *	No locks are held on entry or exit
 *	returns SUCCESS on complete data transfer or phase change,
 *			ABORTED if data transfer is aborted in middle
 */

STATIC int
pc9010_read_16bit(zl5380ha_t *zl5380ha)
{
	int fifo_state;
	ushort_t tmp;

	while (zl5380ha->ha_datalength) {
		if (inb(FIFO_STATUS) & FSR_FFUL) {
			/* FIFO is full */
			zl5380ha->ha_datalength -= PC9010_FIFO_SIZE;

			/*
			 * LINTED pointer alignment
			 */
			repinsw (FIFO, 
				 (ushort_t *)zl5380ha->ha_datapointer, 
				 PC9010_FIFO_SIZE/2);
			zl5380ha->ha_datapointer += PC9010_FIFO_SIZE;
		}	/* end of if FIFO is FULL */

		else if (inb(FIFO_STATUS) & (FSR_FHEMP | FSR_FLEMP)) {
			/* 
			 * FIFO low lane or high lane is empty
			 * if the low lane is not empty and only
			 * one more byte is 
			 * expected, then read it
			 * otherwise, wait for the fifo to be ready
			 */
			if ((zl5380ha->ha_datalength == 1) &&
			    (! (inb(FIFO_STATUS) & FSR_FLEMP))) {
				/* Read the last byte left in the fifo */
				tmp = inw(FIFO);
				*(zl5380ha->ha_datapointer) = (uchar_t) tmp;
				zl5380ha->ha_datalength --;
				break;
			}

			fifo_state = pc9010_wait_fifo_ready(zl5380ha,
				 (FSR_FLEMP | FSR_FHEMP), WAIT_COUNT);
			switch (fifo_state) {
			case TIMEDOUT:
				return ABORTED;

			case PHASECHANGE:
				if (inb(FIFO_STATUS) & (FSR_FLEMP|FSR_FHEMP)) {
					/* 
				 	 * If low lane is empty, all bytes 
					 * have been transferred
					 * otherwise read the lone byte in 
					 * the low lane
					 */
					if (! (inb(FIFO_STATUS) & FSR_FLEMP)) {
						tmp = inw(FIFO);
						*(zl5380ha->ha_datapointer) =
								 (uchar_t)tmp;
						zl5380ha->ha_datalength --;
					}

					return SUCCESS;
				}
				/* Continue with the data transfer */
				break;

			default:
				/* Continue with the data transfer */
				break;
			}
		} /* End of FIFO not ready */

		else {
			/* FIFO has a word so read it */
			/*
			 * LINTED pointer alignment
			 */
       			*((ushort_t *)zl5380ha->ha_datapointer) = inw (FIFO);
			zl5380ha->ha_datalength -= 2;
			zl5380ha->ha_datapointer += 2;
		}
	}	/* End of while */

	return SUCCESS;
}

/*
 * pc9010_wait_fifo_ready(zl5380ha_t *zl5380ha, int mask, int wait_count)
 *	Waits for fifo to be ready for reading or writing.  The mask
 *	must be set appropriately for reading or writing.
 * 
 * Calling/Exit State:
 *	No locks are held on entry or exit
 *	Returns SUCCESS if fifo is ready, TIMEDOUT if fifo is not ready within
 *	the specified wait time, PHASECHANGE if the phase changes during the
 *	wait.
 *
 * Remarks:
 *	mask is FSR_FHEMP | FSR_FLEMP for read and
 * 		FSR_FHFUL | FSR_FLFUL for write
 *	The fifo is disabled while checking the scsi core registers for
 *	phase change and re-enabled after that.
 */

STATIC int
pc9010_wait_fifo_ready(zl5380ha_t *zl5380ha, int mask, int wait_count)
{
	register int i;

	for (i=0; i < wait_count; i++)
	{
		if (!(inb(FIFO_STATUS) & mask)) {
			return SUCCESS;
		}

		/* disable fifo */
		outb(CONTROL, (inb(CONTROL) & (~CTR_FEN)));
		while (inb(CONTROL) & CTR_FEN);

		/* check for request and phase mismatch */
		if ((inb(CSCR) & ZL5380_CSCR_REQ) &&
		    (! (inb(BSR) & ZL5380_BSR_PHASE_MATCH))) {
			return PHASECHANGE;
		}

		/* enable fifo */
		outb(CONTROL, (inb(CONTROL) | CTR_FEN));
	}

	return TIMEDOUT;
}

/*
 * pc9010_write_16bit(zl5380ha_t *zl5380ha)
 *	Performs the actual writing of the data in 16 bit mode
 *
 * Calling/Exit state:
 *	No locks are held on entry or exit
 *	returns SUCCESS on complete data transfer or phase change,
 *			ABORTED if data transfer is aborted in middle
 *
 * Remarks:
 *	In the case of write, we write in chunks of FIFO SIZE, or number
 *	of bytes left, whichever is lesser. We do this because, in case of
 * 	very large data transfers, the phase changes during data-transfer 
 *	phase itself [disconnection].  This results in some bytes being left 
 *	in the FIFO itself.  There seems to be no way to determine how
 *	many bytes are left in the FIFO.  So to work around this, we write
 * 	in chunks of FIFO SIZE bytes.  It is ASSUMED that phase changes
 * 	will occur when data xfer is at 128-byte boundary [FIFO SIZE is 128].
 *	So if the phase changes, we move back the data pointer by the number
 *	bytes last written.
 *	NOTE we dont have this problem in case of read.
 */

STATIC int
pc9010_write_16bit(zl5380ha_t *zl5380ha)
{
	short num = 0; 		/* Number of bytes last written */
	int fifo_state;
	int	x;
	void 	*y;

	while (zl5380ha->ha_datalength > 1) {
		if (inb(FIFO_STATUS) & FSR_FEMP) {
			/* FIFO is empty */
			num = zl5380ha->ha_datalength > PC9010_FIFO_SIZE ? 
				PC9010_FIFO_SIZE : zl5380ha->ha_datalength;

			zl5380ha->ha_datalength -= num;

			repoutsw(FIFO, (ushort_t *)zl5380ha->ha_datapointer, num/2);
			zl5380ha->ha_datapointer += num;
			x = zl5380ha->ha_datalength;
			y = zl5380ha->ha_datapointer;

			/*
			 * If one byte is left it shall be transferred
			 * later thru pio
			 */

		}	/* end of if FIFO is EMPTY */
		/*
		 * Wait for FIFO to be empty
		 */
		fifo_state = pc9010_wait_fifo_empty(zl5380ha, WAIT_COUNT);
		if (fifo_state == TIMEDOUT) {
			return ABORTED;
       		}
		else if (fifo_state == PHASECHANGE) {
			/*
			 * Phase has changed; set the data pointer,
			 * length back to previous value
			 */ 
			zl5380ha->ha_datalength += num;
			zl5380ha->ha_datapointer -= num;
			return SUCCESS;
		}
	}	/* End of while */

	return SUCCESS;

}

/*
 * pc9010_wait_fifo_empty(zl5380ha_t *zl5380ha, int wait_count)
 *	Waits for FIFO to be empty while checking for phase change
 *
 * Calling/Exit state:
 *	No locks are held on entry or exit
 *	returns SUCCESS if fifo is empty
 *		ABORTED if phase changes
 */
STATIC int
pc9010_wait_fifo_empty(zl5380ha_t *zl5380ha, int wait_count)
{
	register int i;

	for (i=0; i < wait_count; i++)
	{
		if ((inb(FIFO_STATUS) & FSR_FEMP)) {
			return SUCCESS;
		}

		/* disable fifo */
		outb(CONTROL, (inb(CONTROL) & (~CTR_FEN)));
		while (inb(CONTROL) & CTR_FEN);

		/* check for request and phase mismatch */
		if ((inb(CSCR) & ZL5380_CSCR_REQ) &&
		    (! (inb(BSR) & ZL5380_BSR_PHASE_MATCH))) {
			return PHASECHANGE;
		}

		/* enable fifo */
		outb(CONTROL, (inb(CONTROL) | CTR_FEN));
	}

	return TIMEDOUT;
}



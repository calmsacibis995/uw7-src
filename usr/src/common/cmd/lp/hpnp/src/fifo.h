/*		copyright	"%c%"	*/
#ident	"@(#)fifo.h	1.2"
/*
 * (c) Copyright 1991 Hewlett-Packard Company.  All Rights Reserved.
 *
 * This source is for demonstration purposes only.
 *
 * Since this source has not been debugged for all situations, it will not be
 * supported by Hewlett-Packard in any manner.
 *
 * This material is provided "as is".  Hewlett-Packard makes no warranty of
 * any kind with regard to this material.  Hewlett-Packard shall not be liable
 * for errors contained herein for incidental or consequential damages in 
 * connection with the furnishing, performance, or use of this material.
 * Hewlett-Packard assumes no responsibility for the use or reliability of 
 * this software.
 */

#define FIFOSIZE 4*1024

typedef struct		    /* FIFO structure */
{
    char    buff[FIFOSIZE];   /* buffer */
    char    *front;	    /* front pointer */
    char    *back;	    /* back pointer */
}
    Fifo;

extern Fifo	Xmit;			/* transmit FIFO (host to peripheral) */
extern Fifo	Recv;			/* receive FIFO (peripheral to host) */


#define	FifoInit(pfifo)		/* initialize a FIFO structure */ \
	{ \
	    (pfifo)->front = (pfifo)->buff; \
	    (pfifo)->back = (pfifo)->buff; \
	}
#define	FifoBytesFree(pfifo)	/* number of free bytes in FIFO */ \
	(FIFOSIZE - ((pfifo)->front - (pfifo)->buff))

#define	FifoBytesUsed(pfifo)	/* number of used bytes in FIFO */ \
	((pfifo)->front - (pfifo)->back)

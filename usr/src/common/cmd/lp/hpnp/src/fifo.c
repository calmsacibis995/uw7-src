/*		copyright	"%c%"	*/
#ident	"@(#)fifo.c	1.2"
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

#include	<stdio.h>
#include	"fifo.h"

Fifo		Xmit;			/* transmit FIFO (host to peripheral) */
Fifo		Recv;			/* receive FIFO (peripheral to host) */

/*
 * NAME
 *	FifoFill - fill a FIFO from a specified source
 *
 * SYNOPSIS
 *	int FifoFill(pfifo, fd, mapnl);
 *	Fifo	*pfifo;	    pointer to FIFO
 *	int	fd;	    file descriptor of source
 *      int     mapnl;      map newlines to carriage-return/linefeed
 *
 * DESCRIPTION
 *	This routine attempts to read as many bytes as possible from the
 *	file or socket specified by "fd" and store them in the FIFO pointed
 *	to by "pfifo".   If necessary, newlines (ASCII LF) are translated
 *      to CR/LF.  If the translation takes place, it is assumed that
 *      the fifo to fill is empty.
 *
 * RETURNS
 *	"FifoFill" returns the number of bytes actually read or -1 if an error
 *	occurs.
 */

int
FifoFill(pfifo, fd, mapnl)
Fifo	*pfifo;	    /* pointer to FIFO */
int	fd;	    /* file descriptor of source */
int     mapnl;
{
	int	    n;	/* number of bytes */

	if (!mapnl) {
		/*
		 * Read data into the buffer beginning at the current value
		 * of the front pointer for as many free bytes as are in the
		 * FIFO
		 */
		if ((n = read(fd, pfifo->front, FifoBytesFree(pfifo))) != -1) {
			/*
			 * We were successful. Adjust the front pointer.
			 */
			(pfifo->front) += n;
		}
	} else {
		/*
		 * Only read in a buffer half the size of the fifo
		 * so that we can store a CR for each NL.  This can
		 * not overflow the FIFO buffer since the worst case
		 * (all newlines) doubles the size of the buffer.
		 *
		 * It is assumed that when the translation occurs FifoFill 
		 * is only called when the original fifo is empty.
		 */
		char	tmpbuf[FIFOSIZE/2];
		char	*tmp;
		int	i;

		tmp = tmpbuf;
		if ((n = read(fd, tmp, sizeof(tmpbuf))) != -1) {
			/*
			 * Map NL to CR/LF and store in the fifo.
			 * If the input is already CR/LF, making it CR/CR/LF
			 * is not harmful.
			 */
			char	*ptr = pfifo->front;
			for (i=n; i; i--) {
				if (*tmp == '\n') {
					*ptr++ = '\r';
					n++;
				}
				*ptr++ = *tmp++;
			}
			pfifo->front = ptr;
		}
	}
	/*
	 * Return the number of bytes read (or an error indication).
	 */
	return (n);
}

/*
 * NAME
 *	FifoFlush - flush a FIFO to a specified destination
 *
 * SYNOPSIS
 *	int FifoFlush(pfifo, fd);
 *	Fifo	*pfifo;	    pointer to FIFO
 *	int	fd;	    file descriptor of destination
 *
 * DESCRIPTION
 *	This routine attempts to write as many bytes as possible from the
 *	FIFO pointed to by "pfifo" to the file or socket specified by "fd".
 *
 * RETURNS
 *	"FifoFlush" returns the number of bytes actually written or -1 if 
 *	an error occurs.
 */
int
FifoFlush(pfifo, fd)
Fifo	*pfifo;	    /* pointer to FIFO */
int	fd;	    /* file descriptor of source */
{
    int	    n;	/* number of bytes */

    /*
     * Write data from the buffer beginning at the current value of the 
     * back pointer for as many used bytes as are in the FIFO.
     */
    if ((n = write(fd, pfifo->back, FifoBytesUsed(pfifo))) != -1)
    {
        /*
         * We were successful. Adjust the back pointer.
    	 */
    	(pfifo->back) += n;
    }
    
    /*
     * Return the number of bytes written (or an error indication).
     */
    return (n);
}

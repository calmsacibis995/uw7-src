#ident	"@(#)OSRcmds:cpio/bufio.c	1.1"
#pragma comment(exestr, "@(#) bufio.c 25.8 94/12/01 ")
/***************************************************************************
 *			bufio.c
 *--------------------------------------------------------------------------
 * Functions that manipulate the cpio read and write buffers.
 *
 *--------------------------------------------------------------------------
 *
 *	Portions Copyright 1983-1994 The Santa Cruz Operation, Inc
 *			All Rights Reserved
 *
 *	Copyright (c) 1984, 1986, 1987, 1988 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 *
 *--------------------------------------------------------------------------
 * Revision History:
 *
 *	L000	01 Sep 1993		scol!ashleyb
 *	  - Module created in an attempt to clean up code.
 *	S001	07 Dec 1993		sco!evanh
 *	  - Added check in chgreel() for named pipes to be treated
 *	    the same as character special devices; i.e, dochange()
 *	    is called when the archive size limit is reached.  A
 *	    named pipe must be specified on the command line with
 *	    the -I or -O flag for this to work.
 *	L002	11 Feb 1994	scol!jamesh from scol!ashleyb
 *	  - Remove unreferenced LXXX and SXXX modification marks from file
 *	  - Flush of /dev/tty input before asking for next volume.
 *	L003	05 Jul 1994	scol!ianw
 *	  - Added #include of errno.h, necessary now the default stdio.h no
 *	    longer #includes it.
 *	L004	06 Jul 1994	scol!ashleyb
 *	  - Use errorl and psyserrorl for error messages (unmarked).
 *	  - Don't let sBlocks become negative.
 *	L005	08 Nov 1994	scol!trevorh
 *	 - message catalogued.
 *	L006	08 Nov 1994	scol!ashleyb
 *	 - added a full stop to one of the messages.
 *
 *==========================================================================
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/tape.h>
#include <sys/param.h>
#include <termio.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>						/* L003 */
#include <string.h>
#include <fcntl.h>

/* #include <errormsg.h> */						/* L005 Start */
#ifdef INTL
#  include <locale.h>
#  include "cpio_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */						/* L005 Stop */

#define _BUFIO_C

#include "../include/osr.h"
#include "cpio.h"
#include "errmsg.h"

/* Global Variables (yuck!) */

extern char	*Buf,
		*swfile,
		*eommsg,
		*ttydev;

extern short	Cflag,
		oamflag,
		Option,
		kflag;

extern int	fillbuf,
		newreel,
		Input,
		Output,
		buf_error,
		error_cnt,
		flushbuf;

extern long	Blocks,
		sBlocks;

extern buf_info buf;

extern unsigned Bufsize,
		Mediasize,
		Mediaused;

extern DevPtr	nextdev;

extern FILE	*Rtty,
		*Wtty;

/* Function Declarations: */

int bread(char *buf, int req, int filelen);
void bwrite(char *buf, int amount);
void rstbuf(void);
void bflush(void);

static void	write_recovered(int f,char *buffer,int amount,int Bufsize);
static int	chgreel(int x,int fl, int rv);
static int	do_change(int fl,int x,int tmperrno);
static int	eomchgreel(int rv);

static int reelcount = 1;

static int input_fd;
static int output_fd;

/*
 * User has given -P flag, so store the file descriptors that have been
 * opened in the parent and signal that we will use these for the EOM
 * messages.
 */
int register_io(int in_fd, int out_fd)
{
	int status;

	input_fd = in_fd;
	output_fd = out_fd;

	/*
	 * Check that the file descriptors are open and can be read or written.
	 */

	if ((status = fcntl(input_fd,F_GETFL,0)) == -1)
	{
		errorl(MSGSTR(BUFIO_MSG_NO_GET_STAT, "couldn't get status of file descriptor %d."),input_fd);
		return(0);
	}

	if (status & O_WRONLY)
	{
		errorl(MSGSTR(BUFIO_MSG_FDESC_NO_READ, "file descriptor %d not open for reading."),input_fd);
		return(0);
	}

	if ((status = fcntl(output_fd,F_GETFL,0)) == -1)
	{
		errorl(MSGSTR(BUFIO_MSG_NO_GET_STAT, "couldn't get status of file descriptor %d."),output_fd);					/* L006 */
		return(0);
	}

	if ((status & (O_WRONLY | O_RDWR)) == 0)
	{
		errorl(MSGSTR(BUFIO_MSG_FDESC_NO_WTR, "file descriptor %d not open for writing."),output_fd);
		return(0);
	}

	/*
	 * Convert file descriptors into streams.
	 */

	if ((Wtty = fdopen(output_fd,"w")) == NULL)
	{
		errorl(MSGSTR(BUFIO_MSG_NO_OPEN_ODESC, "can't open output descriptor as a file stream."));
		cleanup(1);
	}

	if ((Rtty = fdopen(input_fd,"r")) == NULL)
	{
		errorl(MSGSTR(BUFIO_MSG_NO_OPEN_IDESC, "can't open input descriptor as a file stream."));
		cleanup(1);
	}
	return(1);
}

int check_io(char *io_string,int *in_fd, int *out_fd)
{
	char *first,*second;
	int digit;

	if (*io_string == '\000')
		return(0);

	first = io_string;
	second = io_string;

	while(*second != ',')
	{
		if (*second == '\000')
			break;
		second++;
	}

	*second = '\000';

	second++;

	if (*second == '\000')
		return(0);

	if (!isdigit(*first))
	{
		errorl(MSGSTR(BUFIO_MSG_NOT_DIGIT, "%s is not a digit."),first);
		return(0);
	}
	if (!isdigit(*second))
	{
		errorl(MSGSTR(BUFIO_MSG_NOT_DIGIT, "%s is not a digit."),second);
		return(0);
	}

	*in_fd = atoi(first);
	*out_fd = atoi(second);

	return(1);
}

	/*
	 * rstbuf resets the bread buffer, moves incomplete potential headers 
	 * to the beginning of the buffer and forces bread to refill it.
	 * It returns bread's return value to synch (in case of I/O errors).
	 */

void rstbuf()
{
	register int pad;	/* use to insure reads begin a word boundary */
	char scratch[512];
	
	if (buf.b_out_p != buf.b_base_p) {
		pad = ((buf.b_count + 0x3) & ~0x3);
		buf.b_in_p = buf.b_base_p + pad;
		pad -= buf.b_count;
		memcpy(buf.b_base_p + pad, buf.b_out_p, buf.b_count);
		buf.b_out_p = buf.b_base_p + pad;
	}
	if (Option == IN) {
		fillbuf = 1;
		if (bread(scratch, 0, oamflag?512:0) == -1) {
			errorl(MSGSTR(BUFIO_MSG_IO_ERR, "I/O error during rstbuf()."));
			cleanup(1);
		}
	} else
		flushbuf = 0;
}

	/*
	 * bread reads req_cnt bytes out of filelen bytes from the I/O buffer,
	 * moving them to rd_buf_p.  When there are no bytes left in the
	 * I/O buffer, fillbuf is set and the I/O buffer is filled.  Total
	 * is used as the distance to lseek if an I/O error is encountered
	 * with the -k option set (converted to a multiple of Bufsize.
	 */

bread(char *rd_buf_p, int req_cnt, int filelen)
{
	register int rv;
	int rc = 0, i = 0;
	int count;
	static int poss_eom = 0;

	switch (Cflag){
	case BINARY :	req_cnt += (2 - (req_cnt % 2)) % 2;
			break;
	case CRC    :
	case ASCII  :	req_cnt += (4 - (req_cnt % 4)) % 4;
			break;
	default     :   break;
	}
	if (Cflag == BINARY) {	/* round req_cnt up to an even number */
		req_cnt = (req_cnt + 1) / 2;
		req_cnt *= 2;
	}
	while (req_cnt || fillbuf) {
		while (fillbuf) {
			if ( oamflag && buf.b_count >= filelen) {
				fillbuf = 0;
				break;
			}
			if ((buf.b_end_p - buf.b_in_p) < Bufsize) {
				fillbuf = 0;
				break;
			}
			errno = 0;
			if ((rv = read(Input, buf.b_in_p, Bufsize)) < 1) {
				if (rv == -1 && (errno == ENOSPC || errno == ENXIO || newreel )) {
					Input = chgreel(0, Input, rv);
					newreel = 1;
					poss_eom = 0;
					continue;
				} else if (rv == 0) {
					if (poss_eom) {
						Input = chgreel(0, Input, rv);
						newreel = 1;
						poss_eom = 0;
						continue;
					} else {
						poss_eom = 1;
						fillbuf = 0;
						continue;
					}
				} else if (kflag) {
					int dist, s;
					if (i++ > 10) {
						psyserrorl(errno,MSGSTR(BUFIO_MSG_IO_ERR2, "cannot recover from I/O error"));
						cleanup(2);
					}
					dist = !(s = filelen / Bufsize) ? Bufsize : s * Bufsize;
					lseek(Input, dist, 1);
					error_cnt++;
					buf_error++;
					rv = 0;
					if (i == 1) {
						fillbuf = 0;
						req_cnt = 0;
						continue;
					}
				} else {
					errorl(MSGSTR(BUFIO_MSG_IO_READ_ERR, "I/O error on read()."));
					cleanup(1);
				}
			}
			newreel = 0;
			buf.b_in_p += rv;
			buf.b_count += rv;
			if (rv == Bufsize)
				Blocks++;
			else
				sBlocks += rv;
			if (sBlocks >= Bufsize)			/* L004 */
			{
				Blocks += sBlocks / Bufsize;
				sBlocks %= Bufsize;
			}
			if (rv == 0) {
				fillbuf = 0;
				break;
			}
		} /* fillbuf */
		newreel = 0;
		while (req_cnt && !fillbuf) {
			if (buf.b_count == 0) {
				if (buf_error) {
					rc = -1;
					buf_error = 0;
				}
				buf.b_in_p = buf.b_base_p;
				buf.b_out_p = buf.b_base_p;
				fillbuf = 1;
				break;
			}
			count = (buf.b_count < req_cnt) ? buf.b_count : req_cnt;
			memcpy(rd_buf_p, buf.b_out_p, count);
			req_cnt -= count;
			filelen -= count;
			rd_buf_p += count;
			buf.b_out_p += count;
			buf.b_count -= count;
	 	} /* (req_cnt && !fillbuf) */
	} /* (req_cnt || fillbuf) */
	return(rc);
}

	/*
	 * bwrite moves wr_cnt bytes from data_p into the I/O buffer.  When the
	 * I/O buffer is full, flushbuf is set and the buffer is written out.
	 */

void bwrite(char *data_p, int wr_cnt)
{
	register int rv;
	long count, have;


	switch (Cflag) {
	case BINARY :	wr_cnt += (2 - (wr_cnt % 2)) % 2;
			break;
	case CRC    :
	case ASCII  :	wr_cnt += (4 - (wr_cnt % 4)) % 4;
			break;
	default	    :	break;
	}


	while (wr_cnt || flushbuf) {
		while (flushbuf) {
			if (buf.b_count < Bufsize) {
				rstbuf();
				break;
			}
			errno = 0;
			if ((Mediasize)
			&&  (Mediaused+((Bufsize+0x3ff)>>10) > Mediasize)) {
				rv = eomchgreel(0);
				Mediaused = 0;
				goto did_write;
			}
			if ((rv = write(Output, buf.b_out_p, Bufsize)) == -1)
				switch(errno) {
					case ENXIO :	rv = eomchgreel(0);
							Mediaused = 0;
							break; 
					case ENOSPC :	psyserrorl(errno,MSGSTR(BUFIO_MSG_OPT_K, "Attempt to write past end of media after writing %d KBytes\nPlease reissue the cpio command using -K with appropriate media size\n"), Mediaused);
							cleanup(1);
					default     :	psyserrorl(errno,MSGSTR(BUFIO_MSG_IO_WRT_ERR, "I/O error on write() "));
							cleanup(1);
				}
did_write:
			Mediaused += (rv+0x3ff)>>10;
					/* round up to 1k units */
			buf.b_out_p += rv;
			buf.b_count -= (long)rv;
			if (rv == Bufsize)
				Blocks++;
			else if (rv > 0)
				sBlocks += rv;
		} /* flushbuf */
		while (wr_cnt && !flushbuf) {
			if (buf.b_count == buf.b_size) {
				buf.b_out_p = buf.b_base_p;
				flushbuf = 1;
				break;
			}
			have = buf.b_end_p - buf.b_in_p;
			count = (have < (long)wr_cnt) ? have : (long)wr_cnt;
			memcpy(buf.b_in_p, data_p, count);
			wr_cnt -= (int)count;
			data_p += count;
			buf.b_in_p += count;
			buf.b_count += count;
	 	} /* (wr_cnt && !flushbuf) */
	} /* wr_cnt || flushbuf */
}

	/*
	 * blush() forces the final (potentially partial) block
	 * of the archive to be printed out.
	 */

void bflush()
{

	flushbuf = 1;
	(void)bwrite(Buf, 0);	/* write all but partial last block */
	while (buf.b_count) {		/* a partial block remains */
		buf.b_count = Bufsize; /* make it into a "full" block */
		flushbuf = 1;
		(void)bwrite(Buf, 0);	/* write partial last block */
	}
}

static int chgreel(int x,int fl, int rv)
{
	struct stat statb;
	int tmperrno;

	tmperrno = errno;
	fstat(fl, &statb);

	if (! ((statb.st_mode & S_IFCHR) ||
               (swfile && (statb.st_mode & S_IFIFO)))) 	/* S001 */
		{
		if (x && rv == -1)
			switch (tmperrno)
				{
				case EFBIG:	errorl(MSGSTR(BUFIO_MSG_ULIMIT, "ulimit reached for output file"));
						break;
				case ENOSPC:	errorl(MSGSTR(BUFIO_MSG_NO_SPACE, "no space left for output file"));
						break;
				default:	psyserrorl(errno,
						  MSGSTR(BUFIO_MSG_WRT_FAIL, "write() in bwrite() failed"));
				}
		else
			errorl(MSGSTR(BUFIO_MSG_NO_READ_EOF, 
				"can't read input:  end of file encountered prior to expected end of archive."));
		cleanup(2);
		}
	if ((rv == 0) || ((rv == -1) && (tmperrno == ENOSPC  ||  tmperrno == ENXIO))) {
		if (Wtty == NULL) {
			if ((Wtty = fopen(ttydev, "w")) == NULL)
			{
				errorl(MSGSTR(BUFIO_MSG_NO_OPEN_WRT, "can't open %s for writing."), ttydev);
				cleanup(1);
			}
		}
		fflush(stdout);
		fflush(stderr);
		fflush(Wtty);
		if (x)
			fprintf(Wtty, MSGSTR(BUFIO_MSG_END_MEDIUM_O, "\007Reached end of medium on output.\n") );
		else
			fprintf(Wtty, MSGSTR(BUFIO_MSG_END_MEDIUM_I, "\007Reached end of medium on input.\n") );
		fflush(Wtty);
	} else
		{
		if (x)
			psyserrorl(errno,MSGSTR(BUFIO_MSG_ERR_O, "\007Encountered an error on output") );
		else
			psyserrorl(errno,MSGSTR(BUFIO_MSG_ERR_I, "\007Encountered an error on input") );
		if (!newreel)
			cleanup(2);
		}

	/* Handle the end-of-media conditions. */
	return(do_change(fl,x,tmperrno));
}

	/*
	 * Change reel due to reaching end-of-media.  Keep trying for a 
	 * successful write before considering the change a success.
	 */

static int eomchgreel(int rv)
{
	for(;;){
		Output = chgreel(1, Output, rv);
		errno = 0;
		newreel = 1;
		if ((rv = write(Output, buf.b_out_p, Bufsize)) == Bufsize){
			newreel = 0;
			return(rv);
		}
		errorl(MSGSTR(BUFIO_MSG_NO_WRT_MEDIUM, "unable to write this medium.  Try again.") );
		reelcount--;
	}
	/*NOTREACHED*/
}

static void write_recovered(int f,char *buffer,int amount,int Bufsize)
{
	while (amount){
		int ret,next;

		next = (amount <= Bufsize) ? amount : Bufsize;
		if ((ret = write(f,buffer,next)) == -1){
			psyserrorl(errno,MSGSTR(BUFIO_MSG_NO_WRT_BUF, "cannot write recovered data buffer"));
			kmesg();
			cleanup(1);
		}
		buffer += ret;
		amount -= ret;
	}
}

/*******************************************************************************
 *
 * dochange() - handle change of media conditions.
 *	For cpio -i just do close() and open().
 *	For cpio -o we need to see how we got here, and
 *	what we are writeing to.
 *	If errcode != ENXIO then just do close() and open()
 *	else if device type is SCSI try to recover the data from
 *	the buffers and write them out to the new device.
 *	If something goes wrong with the ioctls, we issue a warning
 *	and are forced back into using open() and close().
 *
 *	Returns the file descriptor of the new device.
 ******************************************************************************/
static int do_change(int fl,int x,int tmperrno)
{

	int remain;		/* Amount of data outstanding on SCSI device */
	char *buffer;		/* Somewhere to put this data. */
	int method= USED_STD;
	char str[BUFSIZ];
	int type,f;

	if( Rtty == NULL )
		Rtty = zfopen( EERROR, ttydev, "r");

	/* UW doesn't suuport tape device specific ioctls, so remove */
/*	if (ioctl(fl,MT_REPORT,&type) == -1)
 */		type = 0;

/*	if ( (x == 0) || (tmperrno != ENXIO) || (type != MT_SCSI_TYPE))
 */		close(fl);
	/* The bunch of code below removed, since not supported in UW */
/*	else {
		if (ioctl(fl,MT_REMAIN,&remain) == -1){
			psyserrorl(errno,MSGSTR(BUFIO_MSG_MT_REMAIN_FAIL, "MT_REMAIN failed for this device"));
			kmesg();
			cleanup(1);
		}

		if ((buffer = zmalloc(EERROR,remain)) == NULL){
			errorl(MSGSTR(BUFIO_MSG_NO_MEM, "could not get memory for recovered data."));
			fprintf(stderr,MSGSTR(BUFIO_MSG_SMALLER_BUF, "      Use a smaller buffer size\n"));
			cleanup(1);
		}

		if (ioctl(fl,MT_RECOVER,buffer) == -1){
			psyserrorl(errno,MSGSTR(BUFIO_MSG_MT_RECOVER_FAIL, "MT_RECOVER failed for this device"));
			kmesg();
			free(buffer);
			cleanup(1);
		}

		if (close(fl) == -1){
			psyserrorl(errno,MSGSTR(BUFIO_MSG_ERR_CLOSE, "error while closing device."));
			free(buffer);
			cleanup(1);
		}

		method = USED_RECOVER;
	}
 */
	reelcount++;

again:
	if( swfile ) {

		if (nextdev->used == 0)
		{
			strcpy(str,nextdev->device);
			swfile = nextdev->device;
			nextdev->used = 1;
			nextdev = nextdev->next;
		} else {
	    	askagain:
			fflush(stdout);
			fflush(stderr);
			fflush(Wtty);
			fprintf(Wtty,MSGSTR(BUFIO_MSG_DEV, "Device %s: "),swfile);
			fprintf(Wtty, eommsg, reelcount);
			fflush(Wtty);
			/* Get the file number of the user input device
			 * and loose any input that may still be on it.
			 */
			tcflush(fileno(Rtty),TCIFLUSH);		/* L002 */
			skipln(fgets(str, sizeof str, Rtty), Rtty);
			if (feof(Rtty))
				*str = 'q';
			switch( *str ) {
			case '\0':
				strcpy( str, swfile );
				break;
			case 'q':
				cleanup(4);
			default:
				goto askagain;
			}
		}
	}
	else {
		fflush(stdout);
		fflush(stderr);
		fprintf(Wtty, MSGSTR(BUFIO_MSG_GO_ON, "If you want to go on, type device/file name when ready.\n"));
		fflush(Wtty);
		skipln(fgets(str, sizeof str, Rtty), Rtty);
		if (feof(Rtty) || *str == '\0')
			cleanup(2);
	}

	if((f = open(str, x? O_WRONLY | O_CREAT | O_TRUNC: 0,0666)) < 0) {
		psyserrorl(errno,MSGSTR(BUFIO_MSG_NO_OPEN, "That didn't work, cannot open \"%s\""), str);
		goto again;
	}
	if (method == USED_RECOVER){
		write_recovered(f,buffer,remain,Bufsize);
		free(buffer);
	}
	return f;
}

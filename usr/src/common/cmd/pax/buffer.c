/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)pax:buffer.c	1.1.1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/*
 * buffer.c - Buffer management functions
 *
 * DESCRIPTION
 *
 *	These functions handle buffer manipulations for the archiving
 *	formats.  Functions are provided to get memory for buffers, 
 *	flush buffers, read and write buffers and de-allocate buffers.  
 *	Several housekeeping functions are provided as well to get some 
 *	information about how full buffers are, etc.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms and that any documentation, advertising materials, and other 
 * materials related to such distribution and use acknowledge that the 
 * software was developed * by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Revision 1.2  89/02/12  10:04:02  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:01  mark
 * Initial revision
 * 
 */

/* Headers */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/utime.h>
#include <locale.h>
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"


/* Function Prototypes */


static int ar_write(int, char *, uint);
static void buf_pad(OFFSET);
static int indata(int, OFFSET, char *);
static void outflush(void);
static void buf_use(uint);
static int buf_in_avail(char **, uint *);
static uint buf_out_avail(char **);



/* inentry - install a single archive entry
 *
 * DESCRIPTION
 *
 *	Inentry reads an archive entry from the archive file and writes it
 *	out the the named file.  If we are in PASS mode during archive
 *	processing, the pass() function is called, otherwise we will
 *	extract from the archive file.
 *
 *	Inentry actaully calls indata to process the actual data to the
 *	file.
 *
 * PARAMETERS
 *
 *	char	*name	- name of the file to extract from the archive
 *	Stat	*asb	- stat block of the file to be extracted from the
 *			  archive.
 *
 * RETURNS
 *
 * 	Returns zero if successful, -1 otherwise. 
 */


int
inentry(char *name, Stat *asb)
{
    Link           *linkp;
    int             ifd;
    int             ofd;
    struct utimbuf  tstamp;

    if ((ofd = openout(name, asb, linkp = linkfrom(name, asb), 0)) > 0) {
	if (asb->sb_size || linkp == (Link *)NULL || linkp->l_size == 0) {
	    close(indata(ofd, asb->sb_size, name));
	} else if ((ifd = open(linkp->l_path->p_name, O_RDONLY)) < 0) {
	    warn(linkp->l_path->p_name, strerror(errno));
	} else {
	    passdata(linkp->l_path->p_name, ifd, name, ofd);
	    close(ifd);
	    close(ofd);
	}
    } else {
	return(buf_skip((OFFSET) asb->sb_size) >= 0);
    }
    /*
     * We want to restore access time, but ustar and cpio don't
     * store it. So we use whats in the stat buffer only if its
     * usefull (ie not -1).
     */
    tstamp.actime = (f_extract_access_time) ? 
			(asb->sb_atime == -1) ? time((time_t *) 0) :
			asb->sb_atime : time((time_t *) 0);
    tstamp.modtime = f_mtime ? asb->sb_mtime : time((time_t *) 0);
    utime(name, &tstamp);

    if (f_mode)
	chmod(name, asb->sb_mode & S_IPERM);

    if (f_owner) {
	if (chown(name, asb->sb_uid, asb->sb_gid) < 0) 
	    warn(name, gettxt(BUF_PRESERVE, "unable to preserve owner/group"));
	else if (f_mode)
	    chmod(name, asb->sb_mode & (S_IPERM | S_ISUID | S_ISGID));
    }
    return (0);
}


/* outdata - write archive data
 *
 * DESCRIPTION
 *
 *	Outdata transfers data from the named file to the archive buffer.
 *	It knows about the file padding which is required by tar, but no
 *	by cpio.  Outdata continues after file read errors, padding with 
 *	null characters if neccessary.   Closes the input file descriptor 
 *	when finished.
 *
 * PARAMETERS
 *
 *	int	fd	- file descriptor of file to read data from
 *	char   *name	- name of file
 *	OFFSET	size	- size of the file
 *
 */


void outdata(int fd, char *name, Stat *sb)

{
    uint            chunk;
    int             got;
    int             oops;
    uint            avail;
    int		    pad;
    char           *buf;
    struct utimbuf  tstamp;
    OFFSET          size = sb->sb_size;

    oops = got = 0;
    if (pad = (size % BLOCKSIZE)) {
	pad = (BLOCKSIZE - pad);
    }
    while (size) {
	avail = buf_out_avail(&buf);
	size -= (chunk = size < avail ? (uint) size : avail);
	if (oops == 0 && (got = read(fd, buf, (unsigned int) chunk)) < 0) {
	    oops = -1;
	    warn(name, strerror(errno));
	    got = 0;
	}
	if (got < chunk) {
	    if (oops == 0) {
		oops = -1;
	    }
	    warn(name, gettxt(BUF_EOF, "Early EOF"));
	    while (got < chunk) {
		buf[got++] = '\0';
	    }
	}
	buf_use(chunk);
    }
    close(fd);
    if (f_access_time) {	/* -t option: preserve access time of input */
	tstamp.actime  = sb->sb_atime;
	tstamp.modtime = sb->sb_mtime;
	utime(name, &tstamp);
    }
    if (ar_format == TAR) {
	buf_pad((OFFSET) pad);
    }
}


/* write_eot -  write the end of archive record(s)
 *
 * DESCRIPTION
 *
 *	Write out an End-Of-Tape record.  We actually zero at least one 
 *	record, through the end of the block.  Old tar writes garbage after 
 *	two zeroed records -- and PDtar used to.
 */


void write_eot(void)

{
    OFFSET           pad;
    char            header[M_STRLEN + H_STRLEN + 1];

    if (ar_format == TAR) {
	/* write out two zero blocks for trailer */
	pad = 2 * BLOCKSIZE;
    } else {
	if (pad = (total + M_STRLEN + H_STRLEN + TRAILZ) % BLOCKSIZE) {
	    pad = BLOCKSIZE - pad;
	}
	strcpy(header, M_ASCII);
	sprintf(header + M_STRLEN, H_PRINT, 0, 0,
		       0, 0, 0, 1, 0, (time_t) 0, TRAILZ, pad);
	outwrite(header, M_STRLEN + H_STRLEN);
	outwrite(TRAILER, TRAILZ);
    }
    buf_pad((OFFSET) pad);
    outflush();
}

 
/* outwrite -  write archive data
 *
 * DESCRIPTION
 *
 *	Writes out data in the archive buffer to the archive file.  The
 *	buffer index and the total byte count are incremented by the number
 *	of data bytes written.
 *
 * PARAMETERS
 *	
 *	char   *idx	- pointer to data to write
 *	uint	len	- length of the data to write
 */


void outwrite(char *idx, uint len)

{
    uint            have;
    uint            want;
    char           *endx;

    endx = idx + len;
    while (want = endx - idx) {
	if (bufend - bufidx < 0) {
	    fatal(gettxt(BUF_OVERFLOW, "Buffer overflow in write"));
	}
	if ((have = bufend - bufidx) == 0) {
	    outflush();
	}
	if (have > want) {
	    have = want;
	}
	memcpy(bufidx, idx, (int) have);
	bufidx += have;
	idx += have;
	total += have;
    }
}


/* passdata - copy data to one file
 *
 * DESCRIPTION
 *
 *	Copies a file from one place to another.  Doesn't believe in input 
 *	file descriptor zero (see description of kludge in openin() comments). 
 *	Closes the provided output file descriptor. 
 *
 * PARAMETERS
 *
 *	char	*from	- input file name (old file)
 *	int	ifd	- input file descriptor
 *	char	*to	- output file name (new file)
 *	int	ofd	- output file descriptor
 */


void passdata(char *from, int ifd, char *to, int ofd)

{
    int             got;
    int             sparse;
    char            block[BUFSIZ];

    if (ifd) {
	lseek(ifd, (OFFSET) 0, 0);
	sparse = 0;
	while ((got = read(ifd, block, sizeof(block))) > 0
	       && (sparse = ar_write(ofd, block, (uint) got)) >= 0) {
	    total += got;
	}
	if (got) {
	    warn(got < 0 ? from : to, strerror(errno));
	} else if (sparse > 0
		 && (lseek(ofd, (OFFSET)(-sparse), 1) < 0
		     || write(ofd, block, (uint) sparse) != sparse)) {
	    warn(to, strerror(errno));
	}
    }
    close(ofd);
}


/* buf_allocate - get space for the I/O buffer 
 *
 * DESCRIPTION
 *
 *	buf_allocate allocates an I/O buffer using malloc.  The resulting
 *	buffer is used for all data buffering throughout the program.
 *	Buf_allocate must be called prior to any use of the buffer or any
 *	of the buffering calls.
 *
 * PARAMETERS
 *
 *	int	size	- size of the I/O buffer to request
 *
 * ERRORS
 *
 *	If an invalid size is given for a buffer or if a buffer of the 
 *	required size cannot be allocated, then the function prints out an 
 *	error message and returns a non-zero exit status to the calling 
 *	process, terminating the program.
 *
 */


void buf_allocate(OFFSET size)

{
    if (size <= 0) {
	fatal(gettxt(BUF_BLKSIZE, "Invalid value for blocksize"));
    }
    if ((bufstart = malloc((unsigned) size)) == (char *)NULL) {
	fatal(gettxt(BUF_IOBUF, "Cannot allocate I/O buffer"));
    }
    bufend = bufidx = bufstart;
    bufend += size;
}


/* buf_skip - skip input archive data
 *
 * DESCRIPTION
 *
 *	Buf_skip skips past archive data.  It is used when the length of
 *	the archive data is known, and we do not wish to process the data.
 *
 * PARAMETERS
 *
 *	OFFSET	len	- number of bytes to skip
 *
 * RETURNS
 *
 * 	Returns zero under normal circumstances, -1 if unreadable data is 
 * 	encountered. 
 */


int buf_skip(OFFSET len)

{
    uint            chunk;
    int             corrupt = 0;

    while (len) {
	if (bufend - bufidx < 0) {
	    fatal(gettxt(BUF_BUFSKIP, "Buffer overflow in skip"));
	}
	while ((chunk = bufend - bufidx) == 0) {
	    corrupt |= ar_read();
	}
	if (chunk > len) {
	    chunk = len;
	}
	bufidx += chunk;
	len -= chunk;
	total += chunk;
    }
    return (corrupt);
}


/* buf_read - read a given number of characters from the input archive
 *
 * DESCRIPTION
 *
 *	Reads len number of characters from the input archive and
 *	stores them in the buffer pointed at by dst.
 *
 * PARAMETERS
 *
 *	char   *dst	- pointer to buffer to store data into
 *	uint	len	- length of data to read
 *
 * RETURNS
 *
 * 	Returns zero with valid data, -1 if unreadable portions were 
 *	replaced by null characters. 
 */


int buf_read(char *dst, uint len)

{
    int             have;
    int             want;
    int             corrupt = 0;
    char           *endx = dst + len;

    while (want = endx - dst) {
	if (bufend - bufidx < 0) {
	    fatal(gettxt(BUF_BUF_READ, "Buffer overflow in read"));
	}
	while ((have = bufend - bufidx) == 0) {
	    have = 0;
	    corrupt |= ar_read();
	}
	if (have > want) {
	    have = want;
	}
	memcpy(dst, bufidx, have);
	bufidx += have;
	dst += have;
	total += have;
    }
    return (corrupt);
}


/* indata - install data from an archive
 *
 * DESCRIPTION
 *
 *	Indata writes size bytes of data from the archive buffer to the output 
 *	file specified by fd.  The filename which is being written, pointed
 *	to by name is provided only for diagnostics.
 *
 * PARAMETERS
 *
 *	int	fd	- output file descriptor
 *	OFFSET	size	- number of bytes to write to output file
 *	char	*name	- name of file which corresponds to fd
 *
 * RETURNS
 *
 * 	Returns given file descriptor. 
 */


static int indata(int fd, OFFSET size, char *name)

{
    uint            chunk;
    char           *oops;
    int             sparse;
    int             corrupt;
    char           *buf;
    uint            avail;

    corrupt = sparse = 0;
    oops = (char *)NULL;
    while (size) {
	corrupt |= buf_in_avail(&buf, &avail);
	size -= (chunk = size < avail ? (uint) size : avail);
	if (oops == (char *)NULL && (sparse = ar_write(fd, buf, chunk)) < 0) {
	    oops = strerror(errno);
	}
	buf_use(chunk);
    }
    if (corrupt) {
	warn(name, gettxt(BUF_CORRUPT, "Corrupt archive data"));
    }
    if (oops) {
	warn(name, oops);
    } else if (sparse > 0 && (lseek(fd, (OFFSET) - 1, 1) < 0
			      || write(fd, "", 1) != 1)) {
	warn(name, strerror(errno));
    }
    return (fd);
}


/* outflush - flush the output buffer
 *
 * DESCRIPTION
 *
 *	The output buffer is written, if there is anything in it, to the
 *	archive file.
 */


static void outflush(void)

{
    char           *buf;
    int             got;

    /* if (bufidx - buf > 0) */
	for (buf = bufstart; bufidx - buf > 0;) {
	    if ((got = write(archivefd, buf, blocksize)) > 0) {
		buf += got;
	    } else if ((got < 0) && (errno == ENXIO || errno == ENOSPC)) {
		next(AR_WRITE);
	    } else {
		warn("write", strerror(errno));
		fatal(gettxt(BUF_WRITE, "Tape write error"));
	    }
	}
    bufend = (bufidx = bufstart) + blocksize;
}


/* ar_read - fill the archive buffer
 *
 * DESCRIPTION
 *
 * 	Remembers mid-buffer read failures and reports them the next time 
 *	through.  Replaces unreadable data with null characters.   Resets
 *	the buffer pointers as appropriate.
 *
 * RETURNS
 *
 *	Returns zero with valid data, -1 otherwise. 
 */


int ar_read(void)

{
    int             got;
    static int      failed;

    bufend = bufidx = bufstart;
    if (!failed) {
	if (areof) {
	    if (total == 0) {
		fatal(gettxt(BUF_INPUT, "No input"));
	    } else {
		next(AR_READ);
	    }
	}
	while (!failed && !areof && bufstart + blocksize - bufend >= blocksize) {
	    if ((got = read(archivefd, bufend, (unsigned int) blocksize)) > 0) {
		bufend += got;
	    } else if (got < 0) {
		failed = -1;
		warnarch(strerror(errno), (OFFSET) 0 - (bufend - bufidx));
	    } else {
		++areof;
	    }
	}
    }
    if (failed && bufend == bufstart) {
	failed = 0;
	for (got = 0; got < blocksize; ++got) {
	    *bufend++ = '\0';
	}
	return (-1);
    }
    return (0);
}


/* ar_write - write a filesystem block
 *
 * DESCRIPTION
 *
 * 	Writes len bytes of data data from the specified buffer to the 
 *	specified file.   Seeks past sparse blocks. 
 *
 * PARAMETERS
 *
 *	int     fd	- file to write to
 *	char   *buf	- buffer to get data from
 *	uint	len	- number of bytes to transfer
 *
 * RETURNS
 *
 *	Returns 0 if the block was written, the given length for a sparse 
 *	block or -1 if unsuccessful. 
 */


static int ar_write(int fd, char *buf, uint len)

{
    char           *bidx;
    char           *bend;

    bend = (bidx = buf) + len;
    while (bidx < bend) {
	if (*bidx++) {
	    return (write(fd, buf, len) == len ? 0 : -1);
	}
    }
    return (lseek(fd, (OFFSET) len, 1) < 0 ? -1 : len);
}


/* buf_pad - pad the archive buffer
 *
 * DESCRIPTION
 *
 *	Buf_pad writes len zero bytes to the archive buffer in order to 
 *	pad it.
 *
 * PARAMETERS
 *
 *	OFFSET	pad	- number of zero bytes to pad
 *
 */


static void buf_pad(OFFSET pad)

{
    int             idx;
    int             have;

    while (pad) {
	if ((have = bufend - bufidx) > pad) {
	    have = pad;
	}
	for (idx = 0; idx < have; ++idx) {
	    *bufidx++ = '\0';
	}
	total += have;
	pad -= have;
	if (bufend - bufidx == 0) {
	    outflush();
	}
    }
}


/* buf_use - allocate buffer space
 *
 * DESCRIPTION
 *
 *	Buf_use marks space in the buffer as being used; advancing both the
 *	buffer index (bufidx) and the total byte count (total).
 *
 * PARAMETERS
 *
 *	uint	len	- Amount of space to allocate in the buffer
 */


static void buf_use(uint len)

{
    bufidx += len;
    total += len;
}


/* buf_in_avail - index available input data within the buffer
 *
 * DESCRIPTION
 *
 *	Buf_in_avail fills the archive buffer, and points the bufp
 *	parameter at the start of the data.  The lenp parameter is
 *	modified to contain the number of bytes which were read.
 *
 * PARAMETERS
 *
 *	char   **bufp	- pointer to the buffer to read data into
 *	uint	*lenp	- pointer to the number of bytes which were read
 *			  (returned to the caller)
 *
 * RETURNS
 *
 * 	Stores a pointer to the data and its length in given locations. 
 *	Returns zero with valid data, -1 if unreadable portions were 
 *	replaced with nulls. 
 *
 * ERRORS
 *
 *	If an error occurs in ar_read, the error code is returned to the
 *	calling function.
 *
 */


static int buf_in_avail(char **bufp, uint *lenp)

{
    uint            have;
    int             corrupt = 0;

    while ((have = bufend - bufidx) == 0) {
	corrupt |= ar_read();
    }
    *bufp = bufidx;
    *lenp = have;
    return (corrupt);
}


/* buf_out_avail  - index buffer space for archive output
 *
 * DESCRIPTION
 *
 * 	Stores a buffer pointer at a given location. Returns the number 
 *	of bytes available. 
 *
 * PARAMETERS
 *
 *	char	**bufp	- pointer to the buffer which is to be stored
 *
 * RETURNS
 *
 * 	The number of bytes which are available in the buffer.
 *
 */


static uint buf_out_avail(char **bufp)

{
    int             have;

    if (bufend - bufidx < 0) {
	fatal(gettxt(BUF_AVAIL, "Buffer overflow in buf_out_avail"));
    }
    if ((have = bufend - bufidx) == 0) {
	outflush();
    }
    *bufp = bufidx;
    return (have);
}

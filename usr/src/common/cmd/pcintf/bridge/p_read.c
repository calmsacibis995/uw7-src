#ident	"@(#)pcintf:bridge/p_read.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_read.c	6.7	LCC);	/* Modified: 1/7/92 11:15:33 */

/*****************************************************************************

	Copyright (c) 1984-90 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

/*
 *	MODIFICATION HISTORY
 *	[12/16/87 JD] Jeremy Daw.	SPR# 2234
 *		Calls to err_handler were passing only one arguement. 
 *	Err_handler expects two and so was pulling a bogus value off the stack.
 *	The bug showed up when accessing an fcb that had already been closed,
 *	this is legal! What happened is if the fcb had been closed and we did
 *	a read on it dos bridge would check the return code and reopen the
 *	file. But since err_handler was called without enuff arguements the
 *	correct error code was not getting back to dos bridge and so it thought
 *	the read had failed for reasons other than an unopened fdes, so it never
 *	tried to reopen the fcb. I thought this explanation might be usefull
 *	for archaeological reasons.
 *	TBD, p_fstatus calls err_handler without the second arguement. Can't
 *	be fixed this release, but it needs to be.
 */

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <time.h>

#if defined(RLOCK)
#	include <rlock.h>
extern int	ioStart();		/* Check for locks & excludes */
extern void 	ioDone();		/* Unlocks the lock table */
#endif  /* RLOCK */

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "dossvr.h"
#include "log.h"

extern long lseek();


extern unsigned char	request;	/* Current request type */
extern unsigned char	brg_seqnum;	/* Current sequence number */
extern  int swap_how;                   /* How to swap output packets */
extern  int outputframelength;          /* Length of last output buffer */

extern  char err_class, err_action, err_locus;
extern  char *optr;                     /* Pointer to last frame sent */

extern  struct output out1;             /* Output frame buffers */

extern  struct ni2 ndata;               /* Ethernet header */

int     read_state;                     /* Current state */

unsigned short bytesread;		/* Number of bytes returned */
unsigned short brequested;		/* Number of bytes requested */

struct  output out2, out3;              /* Read ahead output buffers */
struct  output *saved_address;          /* Save address of next output buffer */


#if defined(__STDC__)
int pciran_read(int vdescriptor, long offset, int whence, u_short inode,
						register struct output *addr)
#else
int
pciran_read(vdescriptor, offset, whence, inode, addr)
    int		vdescriptor;		/* PCI virtual file descriptor */
    long	offset;			/* Offset to read from */
    int		whence;			/* Base to lseek from */
    u_short	inode;			/* Actual UNIX inode of file */
    register	struct	output	*addr;	/* Pointer to response buffer */
#endif
{
    register	int
	adescriptor;			/* Actual UNIX fiel descriptor */


    /* Swap-in UNIX descriptor */
	if ((adescriptor = swap_in(vdescriptor, inode)) < 0) {
	    addr->hdr.res = FILDES_INVALID;
	    return FALSE;
	}

	if ((lseek(adescriptor, offset, whence)) < 0) {
	    err_handler(&addr->hdr.res, request, NULL);	/* [12/16/87 JD] */
	    return FALSE;
	}
	return TRUE;
}


/*
 *	What happens is that server() makes a pciseq_read request and if
 *	there is an error condition we return without setting the packet
 *	size. In the case of RS232 it could be large. This is because large
 *	chunks of data can be requested and serviced in smaller packets.
 *	The problem was running a program on the virtual drive, it would load
 *	up some large packets, then on the first record locking violation
 *	this large packet size would be propogated up and there would be
 *	a crash in chksum() because of the packet size.
 *	I *beleive* that in the ethernet case the packet size gets limited
 *	somewhere else and thus the packet size doesn't get humungus.
 *	TBD 
 *	The TBD above applies to the error returns above that also don't
 *	reset the packet size. Should the packet size be set upon entry
 *	to this routine? I don't know, I don't have time for further
 *	investigation right now.
 */

#if defined(__STDC__)
#	if defined(JANUS)
struct output *pciseq_read(
	int vdescriptor,		/* PCI virtual file descriptor */
	unsigned short bytes,		/* Number of bytes to read */
	char *intoAddr			/* Where to put the read data */
	)
#	else /* not JANUS */
struct output *pciseq_read(int vdescriptor, unsigned short bytes)
#	endif /* not JANUS */
#else
#ifdef JANUS
struct output *
pciseq_read(vdescriptor, bytes, intoAddr)
    int			vdescriptor;	/* PCI virtual file descriptor */
    unsigned short	bytes;		/* Number of bytes to read */
    char		*intoAddr;	/* Where to put the read data */
#else	/* not JANUS */
struct	output	*
pciseq_read(vdescriptor, bytes)
    int			vdescriptor;	/* PCI virtual file descriptor */
    unsigned short	bytes;		/* Number of bytes to read */
#endif	/* not JANUS */
#endif
{
    register	int
	status,				/* Return value from system calls */
	adescriptor;			/* Actual UNIX descriptor */

    long
	offset;				/* Current pointer position in file */

    int
	bytestoread;			/* Number of bytes to read now */

    register	struct	output
	*addr;				/* Pointer to response buffer */

#ifdef RLOCK  /* record locking */
long 		iolow;			/* For ioStart lock check */

struct vFile	*vfSlot;		/* Pointer to virtual file */
#endif  /* RLOCK */
#ifdef	JANUS
	/* JANUS does not split up reads and use the "more-to-follow",
	** scheme, because the data is read directly into the DOS memory,
	** thus the limited packet text size is not a problem.
	** "intoAddr" is passed in as a parameter to this routine.
	*/
#else	/* not JANUS */
	int	might_do_mtf;		/* Might to "more-to-follow", which
					** means splitting a big read into
					** several smaller ones, because of
					** limited packet size.
					*/ 
	char	*intoAddr;		/* Read into this buffer */
#endif	/* not JANUS */
#ifdef	JANUS
	int	preset_read;		/* Bytes to read from preset file */
	static	char	*inbuf = NULL;
	static	int	inbufsize = 0;
#endif	/* JANUS */


    /* Swap-in actual UNIX file descriptor */
	addr = &out2;
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
	   				 ? FILDES_INVALID : ACCESS_DENIED);
	    addr->hdr.t_cnt = 0;		/* no text only a response */
	    return addr;
	}

#ifdef JANUS
	bytestoread = bytes;
#else	/* not JANUS */
	/* Read the requested amount or MAX_OUTPUT, whichever is less.
	** If cannot read the full amount, then set up the "more-to-follow"
	** scheme to finish the read.
	*/
	brequested = bytes;
	if (brequested > (unsigned short)MAX_OUTPUT) {
		might_do_mtf = 1;
		bytestoread = MAX_OUTPUT;
	} else {
		might_do_mtf = 0;
		bytestoread = brequested;
	}
	intoAddr = addr->text;
#endif	/* not JANUS */

#ifdef RLOCK  /* record locking */
	/* Get the handle */
	vfSlot = validFid(vdescriptor, (pid_t) getpid());
	if (vfSlot == NULL) {
		addr->hdr.res = FILDES_INVALID;
		addr->hdr.t_cnt = 0;		/* no text only a response */
		return addr;
	}

	/* Verify the file is readable */
	if ((vfSlot->flags & FF_READ) == 0) {
		addr->hdr.res = ACCESS_DENIED;
		addr->hdr.t_cnt = 0;		/* no text only a response */
		return addr;
	}

	iolow = lseek(adescriptor, 0L, 1);  /* current file pointer loc */

	/* Check for lock conflicts and lock the lock table */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* If file is lockable type... */
	if (ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow,
 	   (unsigned long)bytestoread)
	    == -1) {
		addr->hdr.res = LOCK_VIOLATION;
		addr->hdr.t_cnt = 0;		/* no text only a response */
		return addr;
	}
#endif  /* RLOCK */

#ifdef	JANUS
	/* When reading JANUS preset file (stdin), read half requested amount,
	** because NLS translation could expand each byte into two.
	*/
	preset_read = bytestoread >> 1;
	if (!preset_read)
		preset_read = 1;
	if ((vfSlot->flags & VF_PRESET) && (inbufsize < preset_read)) {
		if (inbuf) free(inbuf);
		inbuf = malloc(preset_read);
		inbufsize = preset_read;
	}
#endif	/* JANUS */
	do {
#ifdef	JANUS
	    if ((vfSlot->flags & VF_PRESET) && inbuf) {
		/* Janus preset files are stdin, stdout and stderr.
		** (reads here will normally only be from stdin)
		** Do possible unix-to-dos character translation.
		** Because translation can double the number of
		** bytes, only read half of the requested amount.
		*/
		status = read(adescriptor, inbuf, preset_read);
		if (status != -1) {
			bytesread = janus_u2d(intoAddr, bytestoread,
				inbuf, status, &(vfSlot->vf_jstate));
		}
#ifndef	JANUS
		/* Because the translation can change the number of bytes
		** up or down, the later calculations with bytesread
		** and brequested can get really screwed up.  Here we
		** assume that nothing bad will happen if we satisfy the
		** read here, and not do the MTF (more-to-follow) stuff.
		** This is safe for two reasons:
		** 1) JANUS does not do the MTF stuff.
		** 2) We are translating stdin, so more reads on the DOS
		**    side will be done again anyway.
		*/
		might_do_mtf = 0;
#endif	/* not JANUS */
	    }
	    else
#endif	/* JANUS */
	    {
		status = read(adescriptor, intoAddr, bytestoread);
	    }
	} while (status == -1 && errno == EINTR);
	if (status == -1) {
	    err_handler(&addr->hdr.res, request, NULL);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.t_cnt = 0;		/* no text only a response */
	    return addr;
	}
	bytesread = (unsigned short)status;

	offset = lseek(adescriptor, 0L, 1);
#ifndef	JANUS
	/* See if fully satisfied the read, or will need to do MTF.
	** The read is not satisfied only in the case when
	** "might_do_mtf" is set, and got the full amount requested
	** by the read command.
	*/
	if (!might_do_mtf || (int)bytesread < bytestoread)
#endif	/* not JANUS */
	{
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.res = SUCCESS;
	    addr->hdr.stat = NEW;
	    addr->hdr.fdsc = (unsigned short)vdescriptor;
	    addr->hdr.b_cnt = bytesread;
#ifdef JANUS
		/* With JANUS, we are doing a "mem copy", which means
		** that we read the data into the buffer that is pointed
		** at by "intoAddr", which points to the directly into
		** the DOS process's memory.  The data is NOT read into
		** the outgoing packet.  The packet header is only sent
		** back.  Thus we set t_cnt to zero.  "b_cnt" has the
		** count of the total number of bytes read.
		*/
	    addr->hdr.t_cnt = 0;
#else	/* not JANUS */
	    addr->hdr.t_cnt = bytesread;
#endif	/* not JANUS */
	    addr->hdr.offset = offset;

		/* NOTE: a JANUS "mem copy" read returns here. */
	    return addr;
	}
#ifndef	JANUS
    /* Send first frame of multiple-frame transaction */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW_MTF;
	addr->pre.select = BRIDGE;
	addr->hdr.req = request;
	addr->hdr.seq = brg_seqnum;
	addr->hdr.fdsc = (unsigned short)vdescriptor;
	addr->hdr.b_cnt = NULL;
	addr->hdr.t_cnt = bytesread;
	addr->hdr.offset = offset;
	optr   = (char *)addr;

	outputframelength = xmtPacket((struct output *)optr, &ndata, swap_how);

	saved_address = addr = &out3;

    /* Read until the end of request or EOF */
	bytestoread = (brequested - bytesread >= MAX_OUTPUT)
					? MAX_OUTPUT : brequested - bytesread;
	do
	    status = read(adescriptor, addr->text, bytestoread);
	while (status == -1 && errno == EINTR);
	if (status < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	    return NULL;
	}
	bytesread += (unsigned short)status;
	offset = lseek(adescriptor, 0L, 1);
	if (status < bytestoread || bytesread == brequested) {
	    addr->hdr.stat = EXT;
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	} else {
	    addr->hdr.stat = EXT_MTF;
	    addr->hdr.b_cnt = NULL;
	    read_state = READ_MTF;
	}
	addr->hdr.res = SUCCESS;
	addr->hdr.fdsc = (unsigned short)vdescriptor;
	addr->hdr.t_cnt = (unsigned short)status;
	addr->hdr.offset = offset;

#ifdef RLOCK  /* record locking */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	return NULL;
#endif	/* not JANUS */
}



struct	output *
pci_ack_read(vdescriptor) 
    int	vdescriptor;
{
#ifdef	JANUS
	register struct	output *addr;	/* Pointer to response buffer */
	addr = &out1;
	addr->hdr.res = INVALID_FUNCTION;
	addr->hdr.stat = NEW;
	return addr;
#else	/* not JANUS */
    register	int
	status,				/* Return value from system call */
	adescriptor;			/* Actual UNIX descriptor */
    
    long
	offset;				/* Current offset with file */

    int
	bytestoread;			/* Number of bytes to read now */

    register	struct	output
	*addr;				/* Pointer to response buffer */

#ifdef RLOCK  /* record locking */
long 		iolow;			/* For ioStart lock check */

struct vFile	*vfSlot;		/* Pointer to virtual file */
#endif  /* RLOCK */


    /* Is there a prior context? */
	if (read_state == NULL) {
	    addr = &out1;
	    addr->hdr.res = INVALID_FUNCTION;
	    addr->hdr.stat = NEW;
	    return addr;
	}

	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    addr = &out1;
	    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
	   				 ? FILDES_INVALID : ACCESS_DENIED);
	    return addr;
	}

    /* Send frame already built */
	addr = saved_address;
	addr->hdr.seq = brg_seqnum;
	addr->pre.select = BRIDGE;
	addr->hdr.req = request;
	optr = (char *)addr;

	outputframelength = xmtPacket((struct output *)optr, &ndata, swap_how);

    /* Build next response in parallel frame */
	if (read_state == READ_LAST) {
	    read_state = NULL;
	    return NULL;
	}

    /* Select next output buffer */
	addr = (addr == &out2) ? &out3 : &out2;
	saved_address = addr;

	bytestoread = (brequested - bytesread >= MAX_OUTPUT)
			? MAX_OUTPUT : brequested - bytesread;

#ifdef RLOCK  /* record locking */
	/* Get the handle */
	vfSlot = validFid(vdescriptor, (pid_t) getpid());
	if (vfSlot == NULL) {
		addr->hdr.res = FILDES_INVALID;
		read_state = READ_LAST;
		return NULL;
	}

	/* Verify the file is readable */
	if ((vfSlot->flags & FF_READ) == 0) {
		addr->hdr.res = ACCESS_DENIED;
		read_state = READ_LAST;
		return NULL;
	}

	iolow = lseek(adescriptor, 0L, 1);  /* current file pointer loc */

	/* Check for lock conflicts and lock the lock table */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* If file is lockable type... */
	if (ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow,
	    (unsigned long) bytestoread)
			== -1) {
		addr->hdr.res = LOCK_VIOLATION;
		read_state = READ_LAST;
		return NULL;
	}
#endif  /* RLOCK */


	do
	    status = read(adescriptor, addr->text, bytestoread);
	while (status == -1 && errno == EINTR);
	if (status < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	    return NULL;
	}
	bytesread += (unsigned short)status;
	offset = lseek(adescriptor, 0L, 1);
	if (status < bytestoread || bytesread == brequested) {
	    addr->hdr.stat = EXT;
	    addr->hdr.b_cnt = bytesread;
	    read_state = READ_LAST;
	} else {
	    addr->hdr.stat = EXT_MTF;
	    addr->hdr.b_cnt = NULL;
	    read_state = READ_MTF;
	}
	addr->hdr.res = SUCCESS;
	addr->hdr.fdsc = (unsigned short)vdescriptor;
	addr->hdr.t_cnt = (unsigned short)status;
	addr->hdr.offset = offset;

#ifdef RLOCK  /* record locking */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	return NULL;
#endif	/* not JANUS */
}

#ident	"@(#)pcintf:bridge/p_write.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_write.c	6.5	LCC);	/* Modified: 11:20:17 1/7/92 */

/*****************************************************************************

	Copyright (c) 1984-90 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#include "pci_types.h"
#include "dossvr.h"
#include "log.h"

#if defined(RLOCK)
#	include <rlock.h>		/* record locking defs */
extern int	ioStart();		/* Check for locks & excludes */
					/* others from lock table. */
extern void 	ioDone();		/* Unlocks the lock table */
#endif  /* RLOCK */


extern long lseek();

int     writecount;                     /* Total number of bytes written */

#if defined(__STDC__)
int pci_ran_write(register int vdescriptor, long offset, register u_short inode,
		register int whence, register struct output *addr, int request)
#else
int
pci_ran_write(vdescriptor, offset, inode, whence, addr, request)
    register int	vdescriptor;	/* PCI virtual file descriptor */
    long		offset;		/* Offset to read from */
    register u_short	inode;		/* Actual UNIX inode of file */
    register int	whence;		/* Base to lseek from */
    register struct output	*addr;	/* Pointer to response buffer */
    int			request;	/* DOS request number simulated */
#endif
{
    register	int
	adescriptor;			/* Actual UNIX descriptor */

    /* Swap-in actual UNIX file descriptor */
       if ((adescriptor = swap_in(vdescriptor, inode)) < 0) {
	   log("pci_ran_write: swap_in failed ; errno = %d\n", errno);
	    addr->hdr.res = FILDES_INVALID;
	   return FALSE;
       }

	if ((lseek(adescriptor, offset, whence)) < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
	    log("pci_ran_write: lseek failed\n");
	    return FALSE;
	}
	return TRUE;
}


void
pci_seq_write(vdescriptor, textbuffer, textcount, trans_status, offset,
	      addr, request)
    register int	vdescriptor;	/* PCI virtual file descriptor */
    char	*textbuffer;		/* Pointer to input text */
    int		textcount;		/* Number of bytes being written */
    int         trans_status;           /* Indicates frame sequencing */
    long	offset;
    register	struct	output	*addr;	/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    register	int
 	adescriptor;			/* Actual UNIX descriptor */

    struct	stat	
	filstat;

#ifdef RLOCK  /* record locking */
    long	 	iolow;			/* For ioStart lock check */

    long 		truncount;		/* For ioStart lock check */

    struct vFile	*vfSlot;		/* Pointer to virtual file */
#endif  /* RLOCK */
#ifdef	JANUS
	unsigned long	secret;
#endif	/* JANUS */


    /* Swap-in actual UNIX file descriptor */
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    log("pci_seq_write: swap_in 1 failed; errno = %d\n", errno);
	    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
	   				 ? FILDES_INVALID : ACCESS_DENIED);
	    return;
	}


    /* If count is zero DOS expects file to be truncated to length offset */
	if (textcount)
	{

#ifdef RLOCK  /* record locking */
	    /* Get the handle */
    	    vfSlot = validFid(vdescriptor, (pid_t) getpid());
    	    if (vfSlot == NULL) {
	        log("pci_seq_write: validFid 1 failed; errno = %d\n", errno);
		addr->hdr.res = FILDES_INVALID;
		return;
	    }

	    /* Verify the file is writeable */
	    if ((vfSlot->flags & FF_WRITE) == 0) {
	        log("pci_seq_write: not FF_WRITEable; errno = %d\n", errno);
		addr->hdr.res = ACCESS_DENIED;
		return;
	    }

	    iolow = lseek(adescriptor, 0L, 1);	/* current file ptr position */

	    /* Check for lock conflicts and lock the lock table */
	    /* No truncate here */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	    if(ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid,iolow,(long)textcount)
			== -1) {
		log("pci_seq_write: ioStart 1 failed ; rlockErr = %d\n",
				rlockErr);
		addr->hdr.res = LOCK_VIOLATION;
		return;
	    }
#endif  /* RLOCK */

#ifdef	JANUS
	    if (vfSlot->flags & VF_PRESET) {
		/* Janus preset files are stdin, stdout and stderr.
		** Do possible dos-to-unix character translation.
		** This call may modify the any of the parameters:
		** The location of the translation and the new
		** byte count is put back into "textbuffer" and
		** "textcount".  The translation might be done in
		** place, or into another place, so this calling
		** code should not depend on either case.
		** If the translation expands the number of bytes,
		** then it will use another buffer: it will not
		** modify the original buffer beyond the size given
		** in textcount.  If another buffer is used, then
		** don't assume the original buffer is unchanged,
		** assume it has been partially modified in some
		** mysterious way.
		** Note: it is VERY important that janus_2clean()
		** is called after the translated buffer has been
		** used.  Don't use the textbuffer pointer after
		** janus_2clean has been called.
		*/
		secret = janus_d2u(&textbuffer,&textcount,&(vfSlot->vf_jstate));
	    }
#endif	/* JANUS */
    	    /* Read the requested amount or 1K, whichever is less */
	    do
		writecount = write(adescriptor, textbuffer, (unsigned)textcount);
	    while (writecount == -1 && errno == EINTR);
#ifdef	JANUS
	    if (vfSlot->flags & VF_PRESET) {
		/* Clean up after previous janus_d2u call */
		janus_2clean(secret);
	    }
#endif	/* JANUS */
	    if (writecount != textcount)
	    {
		err_handler(&addr->hdr.res, request, NULL);
	        log("pci_seq_write: write failed; errno = %d\n", errno);
#ifdef RLOCK  /* record locking */
	        if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	            ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
		return;
	    }

#ifdef RLOCK  /* record locking */
	        if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	            ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */

	    file_written(vdescriptor);
	    if (fstat(adescriptor, &filstat)  < 0)
	    {
		err_handler(&addr->hdr.res, request, NULL);
	        log("pci_seq_write: fstat failed; errno = %d\n", errno);
		return;
	    }

	    offset = lseek(adescriptor, 0L, 1);
	    if((trans_status == NEW) || (trans_status == 1))
	    {
		addr->hdr.res = SUCCESS;
		addr->hdr.stat = ACK;
		addr->hdr.fdsc = (unsigned short)vdescriptor;
		addr->hdr.b_cnt = (unsigned short)writecount;
		addr->hdr.f_size = filstat.st_size;
		addr->hdr.offset = offset;
	    }
	    else
	    {
		addr->hdr.res = SUCCESS;
		addr->hdr.stat = ACK;
		addr->hdr.fdsc = (unsigned short)vdescriptor;
		addr->hdr.f_size = filstat.st_size;
		addr->hdr.offset = offset;
	    }
	}
	else  /* Was told to write zero bytes */
	{

	/* MS-DOS expects UNIX to truncate/extend the file */
	    if (fstat(adescriptor, &filstat) < 0) {
		err_handler(&addr->hdr.res, request, NULL);
	        log("pci_ran_write: fstat failed\n");
		return;
	    }

	    offset = lseek(adescriptor, 0L, 1);

	    if (filstat.st_size > offset) {

#ifdef RLOCK  /* record locking */
		/* Get the handle */
		vfSlot = validFid(vdescriptor, (pid_t) getpid());
		if (vfSlot == NULL) {
	        	log("pci_seq_write: validFid 2 failed; errno = %d\n", errno);
			addr->hdr.res = FILDES_INVALID;
			return;
		}

		/* Verify the file is writeable */
		if ((vfSlot->flags & FF_WRITE) == 0) {
	        	log("pci_seq_write: not FF_WRITEable 2; errno = %d\n", errno);
			addr->hdr.res = ACCESS_DENIED;
			return;
		}

		/* Find the size of the area being truncated */
		iolow = lseek(adescriptor, 0L, 1);	/* current position */
		truncount = lseek(adescriptor, 0L, 2); 	/* end of file */
		truncount = truncount - offset;
		lseek(adescriptor, iolow, 0);		/* return to position */

		iolow = offset + 1;		/* start of truncation */

		/* Check for lock conflicts and lock the lock table */
	        if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
		if(ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow, truncount)
				== -1) {
			log("pci_seq_write: ioStart 2 failed ; rlockErr = %d\n",
					rlockErr);
			addr->hdr.res = LOCK_VIOLATION;
			return;
		}
#endif  /* RLOCK */

 	 	/* Read the requested amount or 1K, whichever is less */
		if ((pci_truncate(offset, vdescriptor)) == FALSE) {
	            log("pci_seq_write: truncate failed; errno = %d\n", errno);
		    addr->hdr.res = ACCESS_DENIED;
#ifdef RLOCK  /* record locking */
	            if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	                ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
		    return;
		}

#ifdef RLOCK  /* record locking */
	        if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	            ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */

	    /* Get UNIX descriptor again since truncate could change */
		file_written(vdescriptor);
		if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	   	    log("pci_seq_write: swap_in 2 failed; errno = %d\n", errno);
		    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
					? FILDES_INVALID : ACCESS_DENIED);
		    return;
		}

		if (fstat(adescriptor, &filstat) < 0) {
		    err_handler(&addr->hdr.res, request, NULL);
	            log("pci_ran_write: fstat failed 3\n");
		    return;
		}

		offset = lseek(adescriptor, 0L, 1);
		addr->hdr.res = SUCCESS;
		addr->hdr.stat = ACK;
		addr->hdr.fdsc = (unsigned short)vdescriptor;
		addr->hdr.f_size = filstat.st_size;
		addr->hdr.offset = offset;
	    }
	    else if (filstat.st_size < offset) {
		lseek(adescriptor, -1L, 1);
#ifdef RLOCK  /* record locking */
	       /* Get the handle */
    	        vfSlot = validFid(vdescriptor, (pid_t) getpid());
    	        if (vfSlot == NULL) {
	       	    log("pci_seq_write: validFid 3 failed; errno = %d\n", errno);
		    addr->hdr.res = FILDES_INVALID;
		    return;
	        }

	        /* Verify the file is writeable */
	        if ((vfSlot->flags & FF_WRITE) == 0) {
	       	    log("pci_seq_write: not FF_WRITEable 3; errno = %d\n", errno);
		    addr->hdr.res = ACCESS_DENIED;
		    return;
	        }

		/* Note: I have doubts about the way Merge truncates here.
		   For one thing, no account is taken of when offset = st_size.
		   For another, It seems to know where the file pointer is,
		   and that it wants to back up one byte and truncate (?).
		   Maybe.
			 At any rate I check for the locks as it is done here
		   by the code.  
		*/

		/* Find the size of the area being truncated */
		iolow = lseek(adescriptor, 0L, 1);	/* current position */
		truncount = lseek(adescriptor, 0L, 2); 	/* end of file */
		truncount = truncount - offset;
		lseek(adescriptor, iolow, 0);		/* return to position */

		/* Check for lock conflicts and lock the lock table */
	        if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
		if(ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow, truncount)
				== -1) {
			log("pci_seq_write: ioStart 3 failed ; rlockErr = %d\n",
					rlockErr);
			addr->hdr.res = LOCK_VIOLATION;
			return;
		}
#endif  /* RLOCK */

		do
		    writecount = write(adescriptor, "", 1);
		while (writecount == -1 && errno == EINTR);
		if (writecount < 0) {
		    err_handler(&addr->hdr.res, request, NULL);
	            log("pci_seq_write: write failed 2; errno = %d\n", errno);
#ifdef RLOCK  /* record locking */
	            if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	                ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
		    return;
		}

#ifdef RLOCK  /* record locking */
	        if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	            ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */

		file_written(vdescriptor);
		fstat(adescriptor, &filstat);
		offset = lseek(adescriptor, 0L, 1);
		addr->hdr.res = SUCCESS;
		addr->hdr.stat = ACK;
		addr->hdr.fdsc = (unsigned short)vdescriptor;
		addr->hdr.f_size = filstat.st_size;
		addr->hdr.offset = offset;
	    }
	}
	debug(0, ("f:res=%d b_cnt=%d f_size=%d offset=%d\n", addr->hdr.res,
		addr->hdr.b_cnt, addr->hdr.f_size, addr->hdr.offset));
}


void
pci_mid_write(vdescriptor, textbuffer, textcount, addr, request)
    int		vdescriptor;		/* PCI virtual file descriptor */
    char	*textbuffer;		/* Pointer to text buffer */
    unsigned	textcount;		/* Number of bytes to write */
    register	struct	output	*addr;	/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    register	int
	status,				/* Return value from system call */
	adescriptor;			/* Actual UNIX descriptor */

    long
	offset;				/* Current offset within file */

    struct	stat
	filstat;

#ifdef RLOCK  /* record locking */
    long 		iolow;		/* For ioStart lock check */

    struct vFile	*vfSlot;	/* Pointer to virtual file */
#endif  /* RLOCK */


    /* Swap-in actual UNIX file descriptor */
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    log("pci_mid_write: swap_in failed; errno = %d\n", errno);
	    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
					? FILDES_INVALID : ACCESS_DENIED);
	    return;
	}


#ifdef RLOCK  /* record locking */
	    /* Get the handle */
    	    vfSlot = validFid(vdescriptor, (pid_t) getpid());
    	    if (vfSlot == NULL) {
	       	log("pci_mid_write: validFid failed; errno = %d\n", errno);
		addr->hdr.res = FILDES_INVALID;
		return;
	    }

	    /* Verify the file is writeable */
	    if ((vfSlot->flags & FF_WRITE) == 0) {
	       	log("pci_mid_write: not FF_WRITEable; errno = %d\n", errno);
		addr->hdr.res = ACCESS_DENIED;
		return;
	    }

	iolow = lseek(adescriptor, 0L, 1);  /* current file pointer loc */

	/* Check for lock conflicts and lock the lock table */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	if (ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow, (long)textcount)
			== -1) {
		log("pci_mid_write: ioStart failed ; rlockErr = %d\n", rlockErr);
		addr->hdr.res = LOCK_VIOLATION;
		return;
	}
#endif  /* RLOCK */

	addr->hdr.b_cnt = (unsigned short)writecount;   /* when fail in mid,  */
				/*      want to return amount already written */
	do
	    status = write(adescriptor, textbuffer, textcount);
	while (status == -1 && errno == EINTR);
	if (status != textcount) {
	    err_handler(&addr->hdr.res, request, NULL);
	    log("pci_mid_write: write failed; errno = %d\n", errno);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    return;
	}

	if (fstat(adescriptor, &filstat) < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
	    log("pci_mid_write: fstat failed\n");
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    return;
	}

#ifdef RLOCK  /* record locking */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */

	writecount += status;
	offset = lseek(adescriptor, 0L, 1);
	addr->hdr.res = NULL;
	addr->hdr.stat = ACK;
	addr->hdr.fdsc = (unsigned short)vdescriptor;
	addr->hdr.b_cnt = (unsigned short)writecount;
	addr->hdr.f_size = filstat.st_size;
	addr->hdr.offset = offset;
	debug(0, ("m:res=%d b_cnt=%d f_size=%d offset=%d\n", addr->hdr.res,
		addr->hdr.b_cnt, addr->hdr.f_size, addr->hdr.offset));
}




void
pci_end_write(vdescriptor, textbuffer, textcount, addr, request)
    int		vdescriptor;		/* PCI virtual file descriptor */
    char	*textbuffer;		/* Pointer to text buffer */
    unsigned	textcount;		/* Number of bytes to write */
    register	struct	output	*addr;	/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    register	int
	status,				/* Return value from system call */
	adescriptor;			/* Actual UNIX descriptor */

    long
	offset;				/* Current offset within file */

    struct	stat
	filstat;			/* Buffer for stat() */

#ifdef RLOCK  /* record locking */
    long 		iolow;		/* For ioStart lock check */

    struct vFile	*vfSlot;	/* Pointer to virtual file */
#endif  /* RLOCK */


    /* Swap-in actual UNIX file descriptor */
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    log("pci_mid_write: swap_in failed; errno = %d\n", errno);
	    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
					? FILDES_INVALID : ACCESS_DENIED);
	    return;
	}

#ifdef RLOCK  /* record locking */
	/* Get the handle */
    	vfSlot = validFid(vdescriptor, (pid_t) getpid());
    	if (vfSlot == NULL) {
		addr->hdr.res = FILDES_INVALID;
	       	log("pci_end_write: validFid failed; errno = %d\n", errno);
		return;
	}

	/* Verify the file is writeable */
	if ((vfSlot->flags & FF_WRITE) == 0) {
	       	log("pci_end_write: not FF_WRITEable; errno = %d\n", errno);
		addr->hdr.res = ACCESS_DENIED;
		return;
	}

	iolow = lseek(adescriptor, 0L, 1);  /* current file pointer loc */

	/* Check for lock conflicts and lock the lock table */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	if (ioStart((int) vfSlot->shareIndex, (long) vfSlot->dosPid, iolow, (long)textcount)
			== -1) {
		log("pci_end_write: ioStart failed ; rlockErr = %d\n", rlockErr);
		addr->hdr.res = LOCK_VIOLATION;
		return;
	}
#endif  /* RLOCK */

	addr->hdr.b_cnt = (unsigned short)writecount;   /* when fail in end,  */
				/*      want to return amount already written */
	do
	    status = write(adescriptor, textbuffer, textcount);
	while (status == -1 && errno == EINTR);
	if (status != textcount) {
	    err_handler(&addr->hdr.res, request, NULL);
	    log("pci_end_write: write failed; errno = %d\n", errno);
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    return;
	}

	offset = lseek(adescriptor, 0L, 1);
	if (fstat(adescriptor, &filstat) < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
	    log("pci_end_write: fstat failed\n");
#ifdef RLOCK  /* record locking */
	    if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */
	    return;
	}

#ifdef RLOCK  /* record locking */
	if ((vfSlot->flags & FF_NOLOCK) == 0)  /* file lockable ? */
	        ioDone();	/* Release exclusion on lock table */
#endif  /* RLOCK */

	addr->hdr.res = SUCCESS;
	addr->hdr.stat = EXT;
	addr->hdr.fdsc = (unsigned short)vdescriptor;
	addr->hdr.b_cnt = (unsigned short)writecount+status;
	addr->hdr.f_size = filstat.st_size;
	addr->hdr.offset = offset;
	debug(0, ("e:res=%d b_cnt=%d f_size=%d offset=%d\n", addr->hdr.res,
		addr->hdr.b_cnt, addr->hdr.f_size, addr->hdr.offset));
}

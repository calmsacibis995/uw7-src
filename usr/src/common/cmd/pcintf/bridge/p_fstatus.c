#ident	"@(#)pcintf:bridge/p_fstatus.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)p_fstatus.c	6.10	LCC);	/* Modified: 11:01:47 2/12/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#if defined(LOCUS)
#	include <dstat.h>
#elif defined(SYS5)
#if	defined(SYS5_4)
#	include <sys/statvfs.h>
#else
#	include <sys/statfs.h>
#endif
#endif	/* SYS5 || !LOCUS */

#include "pci_proto.h"
#include "pci_types.h"
#include "dossvr.h"
#include "log.h"

#if defined(LOCUS)
extern long	disksize	PROTO((gfs_t));
extern long	diskfree	PROTO((gfs_t));
#elif defined(SYS5)
#if !defined(SYS5_4)
extern int	statfs		PROTO((const char *, struct statfs *,
								int, int));
#endif
#else
extern long	disksize	PROTO((dev_t));
extern long	diskfree	PROTO((dev_t));
#endif	/* LOCUS */


void
pci_fsize(dos_fname, recordsize, addr, request)
    char	*dos_fname;		/* Name of file */
    int		recordsize;		/* Return size in units of records */
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    char	filename[MAX_FN_TOTAL];
    register	int
	length;				/* Length of file in units */
	
    struct	stat
	filstat;


	if (recordsize <= 0) {
	    addr->hdr.res = FAILURE;
	    return;
	}

	/* If unmapping filename returns collision return failure */
	if (cvt_fname_to_unix(MAPPED, (unsigned char *)dos_fname,
	    (unsigned char *)filename)) {
	    addr->hdr.res = FAILURE;
	    return;
	}

	if ((stat(filename, &filstat)) < 0)
	{
	    err_handler(&addr->hdr.res, request, filename);
	    return;
	}

	length = (filstat.st_size + recordsize - 1) / recordsize;

    /* Fill-in response header */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
	addr->hdr.offset = length;
}



void
pci_setstatus(dos_fname, mode, addr, request)
char
	*dos_fname;		/* Name of file */
int
	mode;			/* Mode to change file to */
struct output
	*addr;			/* Pointer to response buffer */
int
	request;		/* DOS request number simulated */
{
	char	filename[MAX_FN_TOTAL];

	/* If unmapping filename returns collision return failure */
	if (cvt_fname_to_unix(MAPPED, (unsigned char *)dos_fname,
	    (unsigned char *)filename)) {
	    addr->hdr.res = PATH_NOT_FOUND;
	    return;
	}

	if (chmod(filename, mode) < 0) {
		err_handler(&addr->hdr.res, request, filename);
		return;
	}

	/* Fill-in response header */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
}


void
pci_fstatus(addr, request)
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    unsigned long 
	freeblk,
	totlblk;

#if	defined(SYS5) && !defined(LOCUS)
#if	defined(SYS5_4)	
	struct statvfs sfs;

	if (statvfs(CurDir, &sfs) < 0) {
#else
	struct statfs sfs;

	if (statfs(CurDir, &sfs, sizeof (struct statfs), 0) < 0) {
#endif

	    err_handler(&addr->hdr.res, request, CurDir);
	    return;
	}
	if (sfs.f_frsize == 0)		/* use fragment size for block size */
		sfs.f_frsize = 512;	/* fragment size not supported */
	freeblk = sfs.f_bfree;
	freeblk = (freeblk * (sfs.f_frsize/512L)) / (FREESPACE_UNITS/512);
	totlblk = sfs.f_blocks;
	totlblk = (totlblk * (sfs.f_frsize/512L)) / (FREESPACE_UNITS/512);
#elif	defined(LOCUS)
	struct dstat ds_buf;

	if (dstat(CurDir, &ds_buf, sizeof(ds_buf), 0) < 0) {
	    log("pci_fstatus: dstat failed, errno: %d\n", errno);
	    err_handler(&addr->hdr.res, request, CurDir);
	    return;
	}
	freeblk = diskfree(ds_buf.dst_gfs) / FREESPACE_UNITS;
	totlblk = disksize(ds_buf.dst_gfs) / FREESPACE_UNITS;
#else
    struct stat filstat;

	if (stat((*CurDir == '\0') ? "/" : CurDir, &filstat) < 0) {
	    err_handler(&addr->hdr.res, request,
					(*CurDir == '\0') ? "/" : CurDir);
	    return;
	}
	freeblk = diskfree(filstat.st_dev) / FREESPACE_UNITS;
	totlblk = disksize(filstat.st_dev) / FREESPACE_UNITS;
#endif

    /* Fill-in response header */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;

	addr->hdr.b_cnt = (unsigned short)((freeblk > 0xffff)
							? 0xffff : freeblk);
	addr->hdr.mode  = (unsigned short)((totlblk > 0xffff)
							? 0xffff : totlblk);

#ifdef	ICT
	if ( addr->hdr.b_cnt > 32000 )
		addr->hdr.b_cnt = 29000;
#endif	/* ICT */
}

void
pci_devinfo(vdescriptor, addr)
    int		vdescriptor;		/* PCI virtual file descriptor */
    struct	output	*addr;		/* Pointer to response buffer */
{
    int adescriptor;                    /* Actual UNIX descriptor */
    unsigned short status;

/* Get actual UNIX file descriptor */
    if ((adescriptor = swap_in(vdescriptor, 0)) < 0)
    {
	addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
						? FILDES_INVALID : FAILURE);
	return;
    }

    if (isatty(adescriptor))
    {
	/* is Not a file */
	/* filling in DOS device info word */
	/* with CTRL=0 ISDEV=1 EOF=1 RAW=1 ISCLK=0 ISNUL=0 */
	/* "reserved" bits = 0 */
	/* decide on bits ISCOT and ISCIN ("console output","console input" */
	/* Am not sure What this really means, am just seeing if */
	/* stdin, or stdout/stderr */
	switch (adescriptor)
	{
	case 0:
	    status = 0x00e1;    /* console input */
	    break;
	case 1:
	case 2:
	    status = 0x00e2;    /* console output */
	    break;
	default:
	    status = 0x00e0;    /* not stdio */
	    break;
	}
    }
    else /* is a file */
    {
	/* only need to set two bits: ISDEV=0.  EOF=0 when */
	/* "channel has been written" */
	if (write_done(vdescriptor))
	    status = 0x0000;
	else
	    status = 0x0040;
    }
    /* Fill-in response buffer */
    addr->hdr.res = SUCCESS;
    addr->hdr.stat = NEW;
    addr->hdr.mode = status;
}

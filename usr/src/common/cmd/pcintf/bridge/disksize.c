#ident	"@(#)pcintf:bridge/disksize.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)disksize.c	6.3	LCC);	/* Modified: 6/12/91 21:00:48 */

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
#include <fcntl.h>
#include <string.h>
#include <ustat.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "log.h"

#if defined(LOCUS)
long		disksize	PROTO((gfs_t));
long		diskfree	PROTO((gfs_t));
#else	/* LOCUS */
long		disksize	PROTO((dev_t));
long		diskfree	PROTO((dev_t));
extern int	ustat		PROTO((dev_t, struct ustat *));
#endif	/* LOCUS */


#define	FAKEUSTAT (0xd33 * FREESPACE_UNITS)


/*
 *      disksize() -    Scan the disktab searching for the specified device.
 *                      Returns the number of blocks in the file system.
 */

#ifndef LOCUS

long
disksize(device)
dev_t	device;
{
    return(FAKEUSTAT);
}


/*
 *      diskfree() -    Returns the numberof free blocks in the file system.
 */

long
diskfree(device)
dev_t	device;
{
    struct ustat ustatbuf;              /* Ustat buffer */

    /* This function shouldn't be used any more.  See p_fstatus.c */

    /* NOTE: ustat() always returns the number of free blocks in units of */
    /* 512 for generic System V. Locus uses units of 1024. IX370 uses 4096. */
#define USTAT_BLKS      512

    if (ustat(device, &ustatbuf) < 0)
	return(FAKEUSTAT);
	
    return(ustatbuf.f_tfree*USTAT_BLKS);
}

/*
 * These routines are for LOCUS only !!!
 */

#else	/* LOCUS */

#include <dustat.h>
/*
 *      disksize() -    Determines the number of blocks in the file system
 *			specified by the GFS number passed.  Returns the
 *			number of BYTES in the file system.
 */

long
disksize(gfs)
gfs_t	gfs;
{
    struct dustat du_buf;              /* dustat buffer */

    if (dustat(gfs, 0, &du_buf, sizeof(du_buf)) < 0) {
	log("disksize: dustat failed, errno: %d\n", errno);
	return(FAKEUSTAT);
    }

    return(du_buf.du_fsize * du_buf.du_bsize);
}

/*
 *      diskfree() -    Determines the number of free blocks in the file
 *			system specified by the GFS number passed.  Returns
 *			the number of free BYTES !
 */

long
diskfree(gfs)
gfs_t	gfs;
{
    struct dustat du_buf;              /* dustat buffer */

    if (dustat(gfs, 0, &du_buf, sizeof(du_buf)) < 0) {
	log("diskfree: dustat failed, errno: %d\n", errno);
	return(FAKEUSTAT);
    }
	
    return(du_buf.du_tfree * du_buf.du_bsize);
}

#endif	/* LOCUS */

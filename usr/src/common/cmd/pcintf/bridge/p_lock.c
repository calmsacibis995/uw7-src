#ident	"@(#)pcintf:bridge/p_lock.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_lock.c	6.4	LCC);	/* Modified: 14:16:31 1/7/92 */

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
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>

#include "pci_types.h"
#include "dossvr.h"

#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif

extern long
	lseek(),
	time();


#if defined(__STDC__)
void pci_lock(int vdescriptor, int mode,
#	if defined(RLOCK)
					 unsigned short dospid,
#	endif  /* RLOCK */
		long offset, long length, struct output *addr, int request)
#else
void
pci_lock(vdescriptor, mode,
#	if defined(RLOCK)
			dospid, 	/* dos pid asking for lock */
#	endif  /* RLOCK */
			offset, length, addr, request)

    int		vdescriptor;		/* PCI virtual file descriptor */
    int		mode;			/* Lock or unlock */
#	if defined(RLOCK)
    unsigned short
		dospid;	 		/* dos pid asking for lock */
#	endif  /* RLOCK */
    long	offset, length;		/* Position and size of lock */
    struct	output	*addr;
    int		request;		/* DOS request number simulated */
#endif
{
#ifdef RLOCK  /* record locking */
    int		adescriptor;		/* Acutual UNIX file descriptor */

    struct	stat
		filstat;		/* Buffer contains data from stat() */

	    if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
		    /* returns "invalid file descriptor" to dos, even if
		      the problem is just a lack of available unix descriptors.
		      This doesn't seem right but the way swap_in was written
		      and is used would take too much time to change now.
		    */
		addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
						? FILDES_INVALID : FAILURE);
		return;
	    }

	    if (fstat(adescriptor, &filstat) < 0) {
		err_handler(&addr->hdr.res, request, NULL);
		return;
	    }

	    /* lock or unlock the file */
	    if (lock_file(vdescriptor,dospid,mode,offset,length) < 0) {
		addr->hdr.res = LOCK_VIOLATION;
		return;
	    }
#endif  /* RLOCK */

	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
	return;
}

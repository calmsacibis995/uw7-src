#ident	"@(#)pcintf:bridge/p_exit.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_exit.c	6.2	LCC);	/* Modified: 21:08:34 6/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"
#include "pci_types.h"
#include "dossvr.h"


void
pci_exit(pid, addr)
    int		pid;			/* Id of MS-DOS process exiting */
    struct	output	*addr;		/* Pointer to response buffer */
{

    /* Delete all search contexts and file contexts with process id */
	deldir_pid((int)pid);
	delfile_pid((int)pid);
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
}

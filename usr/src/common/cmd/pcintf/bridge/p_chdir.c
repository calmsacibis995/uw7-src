#ident	"@(#)pcintf:bridge/p_chdir.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_chdir.c	6.4	LCC);	/* Modified: 11:04:44 12/3/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "pci_types.h"
#include "dossvr.h"


void
pci_chdir(dos_dir, addr, request)
    char	*dos_dir;		/* DOS Target directory */
    struct	output	*addr;
    int		request;		/* DOS request number */
{
	register char *p;
	char directory[MAX_FN_TOTAL];	/* UNIX version of directory */


	/* Translate MS-DOS directory to UNIX */
	if (cvt_fname_to_unix(MAPPED, (unsigned char *)dos_dir,
	    (unsigned char *)directory)) {
		addr->hdr.res = PATH_NOT_FOUND;
		return;
	}

	/* this is dumb, but DOS doesn't like doing chdirs to places ending
	*  with backslashes
	*/

	if ( ((p = strrchr(directory,'/')) && (p != directory) && (*++p == 0))
		|| (*directory == 0) ) {
		addr->hdr.res = PATH_NOT_FOUND;
		return;
	}


    /* Update current working directory string */
	if (chdir(directory)) {
		err_handler(&addr->hdr.res, request, directory);
		return;
	}

    /* Construct new cwd and store in MS-DOS form */
	pci_getcwd(directory, CurDir);	

/*
 * Becuase MS-DOS fills in the DPB with the current working directory
 * we must get the current directory and send it back to the bridge.
 * The mapped mode is used, because we want it to look like a standard
 * MS-DOS entry.
 */

	pci_pwd(MAPPED, addr);
}


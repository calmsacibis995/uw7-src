#ident	"@(#)pcintf:bridge/p_rmdir.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_rmdir.c	6.3	LCC);	/* Modified: 12:01:20 12/3/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <errno.h>
#include <malloc.h>
#include <string.h>

#include "pci_types.h"
#include "dossvr.h"


void
pci_rmdir(dName, addr, request)
    char	*dName;			/* Name of directory to remove */
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    char  directory[MAX_FN_TOTAL];
    char
	*dotdir;			/* copy of CurDir to compare against */

	/* Translate MS-DOS to UNIX directory name */

	/*  Different systems set different errno values on attempt to
	**  remove root, so we check for it here.
	*/

	if (cvt_fname_to_unix(MAPPED, (unsigned char *)dName,
	    (unsigned char *)directory) ||
	    !strcmp(directory, "/")) {
		addr->hdr.res = ACCESS_DENIED;
		return;
	}

	/* Error if attempting to remove the current working directory */
	dotdir = (char *) malloc(strlen(CurDir) + 3);	/* make a copy of cwd */
	strcpy (dotdir, CurDir);		/* with '/.' appended to */
	strcat (dotdir, "/.");			/* compare against */

	if (!strcmp(directory, CurDir) || !strcmp(directory, dotdir)) {
	    free( dotdir );
	    addr->hdr.res = ATTEMPT_TO_REMOVE_DIR;
	    return;
	}
	free( dotdir );

	if (rmdir(directory) < 0) {
	    if (errno == ENOENT)	/* DOS rd command can't return F_N_F */
		addr->hdr.res = PATH_NOT_FOUND;
	    else
		err_handler(&addr->hdr.res, request, dName);
	}
	else
	    addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
}

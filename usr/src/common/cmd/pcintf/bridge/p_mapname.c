#ident	"@(#)pcintf:bridge/p_mapname.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)p_mapname.c	6.6	LCC);	/* Modified: 15:07:46 2/20/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include <string.h>

#include "sysconfig.h"
#include "pci_types.h"
#include "dossvr.h"
#include "log.h"

extern int unmap_fail;		/* Fail unmapname if file doesn't exist */


/*
 * The MAPFILE function maps a dos filename to the corresponding unix
 * filename if mode is zero and performs the inverse operation if mode   
 * is one.  There is an extended IOCTL function on the bridge that makes    
 * use of this service.
 *    
 */

void
pci_mapname(name, mode, addr)
char *name;
int mode;
struct output *addr;
{
    char nametemp[MAX_FN_TOTAL];
    register char *np;
    int flag;
    
    if (mode) { /* map unix name to dos name */
	cvt_to_unix(name, nametemp);
	/*
	 *  The bridge sends the pathname with backslashes,
	 *  so convert them to forward slashes.
	 */
	for (np = nametemp; *np; np++) {
	    if (*np == '\\')
		*np = '/';
	}
 	flag = cvt_fname_to_dos(MAPPED, (unsigned char *)CurDir,
	    (unsigned char *)nametemp, (unsigned char *)addr->text, (ino_t)0);
    }
    else { /* map dos name to unix name */
	unmap_fail = 1;
	flag = cvt_fname_to_unix(MAPPED, (unsigned char *)name,
	    (unsigned char *)nametemp);
	cvt_to_dos((unsigned char *)nametemp, (unsigned char *)addr->text);
	unmap_fail = 0;
    }
    Vlog(("  out: %s  flag: %d\n", "%s %d\n", addr->text, flag));
    if (flag == 0) {
	addr->hdr.t_cnt = (unsigned short)strlen(addr->text) + 1;
	addr->hdr.res = SUCCESS;
    }
    else {
	addr->hdr.t_cnt = 0;
	addr->hdr.res = FILE_NOT_FOUND; 
    }
}

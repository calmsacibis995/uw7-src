#ident	"@(#)pcintf:bridge/p_pwd.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)p_pwd.c	6.4	LCC);	/* Modified: 2/20/92 15:10:44 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <string.h>

#include "pci_types.h"
#include "dossvr.h"
#include "pw_gecos.h"


void
pci_pwd(mode, addr)
int mode;
    struct	output	*addr;		/* Pointer to response buffer */
{
    register	int
	datalength;			/* Length of response string */

    char
	directory[MAX_FN_TOTAL];	/* String contains directory name */

	directory[0] = '\0';
	directory[1] = '\0';
	cvt_fname_to_dos(mode, (unsigned char *)CurDir, (unsigned char *)CurDir,
	    (unsigned char *)directory, (ino_t)0);

	datalength = strlen(directory);

	/* Fill response buffer */
	/* DOS working directory name does not include leading slash */
	(void) strcpy(addr->text, &directory[1]);
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
	addr->hdr.t_cnt = datalength ? (unsigned short)datalength : 1;
}

#ifdef	LOCUS
/*
 *	parseGecos() - Routine to parset the pw_gecos field in the
 * 		       struct passwd and put the subfields in struct 
 *		       struct pw_gecos;
 *
 *	Format of the pw_gecos field is:
 *		useName/fileLimit;siteInfo;siteAccessPerm
 */
struct pw_gecos *
parseGecos(gecosPtr) 
char	 *gecosPtr;
{
static struct pw_gecos theGecos = 
	{(char *)NULL, 0L, (char *)NULL, (char *)NULL};
char	*nameFLimit;
char	*ptr;

	/* Get User Name and File Limit, siteInfo and Access Perm */
	nameFLimit = gecosPtr;
	if ((ptr = strchr(gecosPtr, ';')) != (char *)NULL)
		*ptr++ = '\0';
	theGecos.siteInfo = ptr;
	if ((ptr = strchr(ptr, ";")) != (char *)NULL)
		*ptr++ = '\0';
	theGecos.siteAccessPerm = ptr;
	theGecos.userName = strtok(nameFLimit, "/");
	if (theGecos.userName != (char *)NULL) {
	    ptr = strtok((char *)NULL, "/");
	    if (ptr) theGecos.fileLimit = atol(ptr);
	}

	/* Set the NULL strings to \0 */
	if (theGecos.userName == (char *)NULL)
	    theGecos.userName = "\0";
	if (theGecos.siteInfo == (char *)NULL)
	    theGecos.siteInfo = "\0";
	if (theGecos.siteAccessPerm == (char *)NULL)
	    theGecos.siteAccessPerm = "\0";
	return &theGecos;
};

#endif	/* LOCUS */

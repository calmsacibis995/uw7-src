#ident	"@(#)pcintf:bridge/nls_init.c	1.2"
#ifndef NO_SCCSIDS
#include <sccs.h>
SCCSID(@(#)nls_init.c	6.4	LCC);	/* Modified: 23:22:08 7/12/91 */
#endif 

/*****************************************************************************

	Copyright (c) 1989 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <errno.h>
#include <stdio.h>

#include <lmf.h>

#include "common.h"
#include "log.h"

#define ERR_BUF 80	/* size of error buffer */

static char *nls_domain = "LCC.PCI.UNIX";


/* Define default NLS settings.
*/
char unix_table_name[MAX_FN_TOTAL] = "8859";
char dos_table_name[MAX_FN_TOTAL]  = "pc850";
char *lcspath_default = "/usr/pci/lib";
static char *nls_file = "dosmsg";
static char *nls_lang = "En";
static char *nls_path = "/usr/pci/%N/%L.%C";


/* 
 * nls_init
 *	Perform nls initialization.
 *    RETURNS:
 *	Returns 0 if successful.
 *	When unsuccessful because of failure to open the message file,
 *		a serious error is logged, and 1 is returned.
 *	When unsuccessful because could not push domain, a fatal error
 *		is logged (which does an exit).
 */
int
nls_init()
{
	char err_buf[ERR_BUF];	/* for error messages */
	int lmf_handle;


	/* Open the message file */
	if ((lmf_handle = lmf_open_file(nls_file, nls_lang, nls_path)) >= 0 &&
		lmf_push_domain(nls_domain)) {	/* Push the domain */
			sprintf(err_buf,
				"nls_init:Can't push domain %s, lmf_errno %d\n",
				nls_domain, lmf_errno);
			fatal(err_buf);	
			/* NO RETURN */
	}

	/* Set up the fast domain */
	/* 
	lmf_fast_domain(nls_domain);
	*/
	return 0;
}

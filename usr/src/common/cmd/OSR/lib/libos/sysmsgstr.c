#ident	"@(#)OSRcmds:lib/libos/sysmsgstr.c	1.1"
#pragma comment(exestr, "@(#) sysmsgstr.c 26.1 95/06/01 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1992-1995.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * sysmsgstr	- Standardised system message describing an errno value
 *
 * Returns a pointer to a standard-format errno-value-describing string.
 * The errno-value to describe is <code>; if a non-NULL <ptr> is given,
 * the string is placed in the pointed-to memory  (which is assumed to
 * be large enough), else a static buffer is used.
 * 
 * N.b.	The static buffer may be overwritten on a subsequent call.
 *
 *  MODIFICATION HISTORY
 *	6 March 1992	scol!blf	creation
 *		- Slightly reworked generalization of routines used for
 *		  some time in varying forms -- to encourge the radical
 *		  concept of standardisation and modularity.
 *	20 Dec 1994	scol!donaldp	L001
 *		- Replace indexing sys_errlist with call to strerror()
 *		  to get localised error messages and removed now
 *		  redundant range check.
 *	26th May 1995	scol!ashleyb	L002
 *		- Internationalised code
 */

/* #include <unistd.h> */
#include <string.h>
/* #include "errormsg.h" */	/* use local master not unknown installed version */
#include "libos_intl.h"			       		/* L002 */

#define	MSG_ERRORL 16

char *
sysmsgstr(
	int		code,		/* errno		*/
	char		*ptr		/* buffer (may be NULL)	*/
) {
	char		*buf;
	static char	temp[100];

	if ((buf = ptr) == (char *)0)
		buf = temp;

	(void) sprintf(buf, LMSGSTR(MSG_ERRORL,"%s (error %d)"),
		       strerror(code), code); /* L001 */ /* L002 */

	return buf;
}

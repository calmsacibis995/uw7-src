#ident	"@(#)OSRcmds:lib/libos/errorv.c	1.1"
#pragma comment(exestr, "@(#) errorv.c 25.1 92/07/06 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * errorv	-- Print an error message, vprintf(S)-like, to stderr
 *
 *  MODIFICATION HISTORY
 *	12 May 1992	scol!blf	creation
 *		- Slightly reworked generalization of routines used for
 *		  some time in varying forms -- to encourge the radical
 *		  concept of standardisation and modularity.
 */

#include <stdio.h>
#include <stdarg.h>
/* #include <unistd.h> */
/* #include "errormsg.h" */	/* use local master not unknown installed version */

extern char *command_name;

void
errorv(
	const char	*fmt,
	va_list		args
) {
	FILE	*errmsg_fp = stderr;

	(void) fflush(stdout);	/* synchronize error mesg with output */

	if (errmsg_fp != (FILE *)0) {
		if (command_name != (char *)0 && *command_name != '\0')
			(void) fprintf(errmsg_fp, "%s: ", command_name);

		if (fmt != (const char *)0)
			(void) vfprintf(errmsg_fp, fmt, args);
		putc((int)'\n', errmsg_fp);

		(void) fflush(errmsg_fp);
	}
}

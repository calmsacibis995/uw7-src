#ident	"@(#)OSRcmds:lib/libos/psyserrorv.c	1.1"
#pragma comment(exestr, "@(#) psyserrorv.c 25.1 92/07/06 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * psyserrorv	-- Print an errno-based message, vprintf(S)-like, to stderr
 *
 *  MODIFICATION HISTORY
 *	6 March 1992	scol!blf	creation
 *		- Slightly reworked generalization of routines used for
 *		  some time in varying forms -- to encourge the radical
 *		  concept of standardisation and modularity.
 */

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
/* #include "errormsg.h" */	/* use local master not unknown installed version */

extern char *command_name;

void
psyserrorv(
	int		code,
	const char	*fmt,
	va_list		args
) {
	FILE *errmsg_fp = stderr;

	(void) fflush(stdout);	/* synchronize error mesg with output */

	if (errmsg_fp != (FILE *)0) {
		if (command_name != (char *)0 && *command_name != '\0')
			(void) fprintf(errmsg_fp, "%s: ", command_name);

		if (fmt != (char *)0) {
			(void) vfprintf(errmsg_fp, fmt, args);
			(void) fputs(": ", errmsg_fp);
		}

		(void) fputs((char *)sysmsgstr(code, (char *)0), errmsg_fp);
		putc('\n', errmsg_fp);

		(void) fflush(errmsg_fp);
	}
}

#ident	"@(#)OSRcmds:lib/libos/errorl.c	1.1"
#pragma comment(exestr, "@(#) errorl.c 25.1 92/07/06 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * errorl	-- Print an error, printf(S)-like, to stderr
 *
 *  MODIFICATION HISTORY
 *	12 May 1992	scol!blf	creation
 *		- Slightly reworked generalization of routines used for
 *		  some time in varying forms -- to encourge the radical
 *		  concept of standardisation and modularity.
 */

#include <unistd.h>
#include <stdarg.h>

extern void errorv(const char *fmt, va_list args);

/* #include "errormsg.h" */	/* use local master not unknown installed version */

/* PRINTFLIKE2 */
void
errorl(
	const char	*fmt,
	...
) {
	va_list		args;

	va_start(args, fmt);
	errorv(fmt, args);
	va_end(args);
}

#ifdef TEST

char	*command_name	= "TEST-only errorl(S) test";

void
main(
	int		argc,
	const char	*argv[]
) {
	register int	n;

	if (argc > 0)
		command_name = *argv;

	for (n = 1; n < argc; n++)
		errorl("argv[%d] is \"%s\"", n, argv[n]);
	exit(0);
	/* NOTREACHED */
}

#endif /* TEST */

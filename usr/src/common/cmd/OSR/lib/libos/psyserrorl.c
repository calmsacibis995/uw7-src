#ident	"@(#)OSRcmds:lib/libos/psyserrorl.c	1.1"
#pragma comment(exestr, "@(#) psyserrorl.c 25.1 92/07/06 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1992.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 * psyserrorl	-- Print an errno-based message, printf(S)-like, to stderr
 *
 *  MODIFICATION HISTORY
 *	6 March 1992	scol!blf	creation
 *		- Slightly reworked generalization of routines used for
 *		  some time in varying forms -- to encourge the radical
 *		  concept of standardisation and modularity.
 */

#include <stdarg.h>
#include <unistd.h>
/* #include "errormsg.h" */	/* use local master not unknown installed version */

extern void psyserrorv(int code, const char *fmt, va_list args);

/* PRINTFLIKE2 */
void
psyserrorl(
	int		code,
	const char	*fmt,
	...
) {
	va_list		args;

	va_start(args, fmt);
	psyserrorv(code, fmt, args);
	va_end(args);
}

#ifdef TEST

char	*command_name	= "TEST-only psyserrorl(S) test";

void
main(
	int		argc,
	const char	*argv[]
) {
	register int	n;

	if (argc > 0)
		command_name = *argv;

	for (n = 1; n < argc; n++)
		psyserrorl(
			atoi(argv[n]),
			"argv[%d] is \"%s\" which means", n, argv[n]
		);
	exit(0);
	/* NOTREACHED */
}

#endif /* TEST */

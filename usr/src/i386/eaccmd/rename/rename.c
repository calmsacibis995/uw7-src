/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/rename/rename.c	1.1.1.2"
#ident  "$Header$"
/*
 *	@(#) rename.c 1.1 90/01/12 
 *
 * Copyright (c) 1989, The Santa Cruz Operation, Inc.
 * All rights reserved.
 *
 * This Module contains Proprietary Information of the Santa Cruz
 * Operation, Inc., and should be treated as Confidential. 
 *
 * The code marked with symbols from the list below, is owned
 * by The Santa Cruz Operation Inc., and represents SCO value
 * added portions of source code requiring special arrangements
 * with SCO for inclusion in any product.
 *  Symbol:		 Market Module:
 * SCO_BASE 		Platform Binding Code
 * SCO_ENH 		Enhanced Device Driver
 * SCO_ADM 		System Administration & Miscellaneous Tools
 * SCO_C2TCB 		SCO Trusted Computing Base-TCB C2 Level
 * SCO_DEVSYS 		SCO Development System Extension
 * SCO_INTL 		SCO Internationalization Extension
 * SCO_BTCB 		SCO Trusted Computing Base TCB B Level Extension
 * SCO_REALTIME 	SCO Realtime Extension
 * SCO_HIGHPERF 	SCO High Performance Tape and Disk Device Drivers
 * SCO_VID 		SCO Video and Graphics Device Drivers (2.3.x)
 * SCO_TOOLS 		SCO System Administration Tools
 * SCO_FS 		Alternate File Systems
 * SCO_GAMES 		SCO Games
 */

#pragma comment(exestr, "@(#) rename.c 1.1 90/01/12 ")

/* BEGIN SCO_BASE */

/*
 * MODIFICATION HISTORY
 *	12 Dec 1989	scol!blf	creation
 *		- Entirely obvious; created after neither /usr/lib/mv_dir
 *		  nor mv(C) would do what I want with a lot of hassle.
 */

#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE	/* use POSIX (Draft 13) values (if any) */
#endif

#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#ifndef _POSIX_VERSION
	ERROR -- This program will only run on a POSIX-conformant system
#endif

char	*pgm	= "rename";

main(
	int argc,
	char *argv[]
) {
	int saverr;

	if (argc > 0)
		pgm = *argv;
	if (argc != 3) {
		(void) fprintf(stderr, "Usage: %s file1 file2\n", pgm);
		exit(2);
		/* NOTREACHED */
	}
	if (rename(argv[1], argv[2])) {
		saverr = errno;
		perror(pgm);
		exit(saverr);
		/* NOTREACHED */
	}
	exit(0);
	/* NOTREACHED */
}

/* END SCO_BASE */

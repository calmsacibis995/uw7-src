/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:i386/cmd/fs.d/cdfs/cdvd/cdvd.c	1.6"
#ident	"$Header$"

static char cdvd_copyright[] = "Copyright 1991, 1992 Intel Corp. 469260";

/* Tabstops: 4 */

#include <errno.h>
#include <libgen.h>
#include <locale.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdrom.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/fs/cdfs_fs.h>
#include <sys/fs/cdfs_ioctl.h>

#include "cdvd.h"

/*
 * cdvd - Read Volume Descriptor from CD-ROM.  This is one of the XCDR API
 * utilities.
 *
 * usage: cdvd [ -b ] <file>
 */


main (argc, argv)
	int				argc;
	char			*argv[];
{
	/*
	 * Find the executable name.
	 */
	ProgName = (uchar_t *) basename (argv[0]);

	DoLocale();

	ParseOpts (argc, argv);

	GetPVD ();

	PrintPVD ();

	return (0);
}





/*
 * Set the necessary elements of the locale.
 */
static void
DoLocale ()
{
	uchar_t		string[MAXNAMELEN];

	(void) setlocale (LC_ALL, "");
	(void) sprintf ((char *) string, "UX:");
	(void) strcat ((char *) string, (char *) ProgName);
	(void) setlabel ((char *) string);
	(void) setcat ("uxcdfs");
	
	return;
}





/*
 * Parse command-line options.
 */
static void
ParseOpts (count, vector)
	int				count;
	char			*vector[];
{
	int				option;

	opterr = 0;
	while ((option = getopt ((int) count, vector, "b")) != EOF) {
		switch (option) {
			case 'b': {
				Binary = B_TRUE;
				break;
			}
			default: {
				ErrMsg (MM_ERROR, EXIT_CODE_1, SHOW_USAGE,
							":82:Invalid option: -%1s\n", (char *) &optopt,
							NULL, NULL);
				break;
			}
		}
	}
	if (count - optind != 1) {
		ErrMsg (MM_ERROR, EXIT_CODE_1, SHOW_USAGE,
					":109:Need to specify a file or directory or a block device.\n",
					NULL, NULL, NULL);
	}
	File = vector[optind];

	return;
}





/*
 * Display usage message.
 */
static void
DispUsage ()
{
	(void) pfmt (stderr, MM_ACTION, ":140:Usage: %s [ -b ] <file>\n", ProgName);

	return;
}





/*
 * Display error message, exit if desired.  All a[1-3] arguments are
 * character pointers.  Print out integers by casting them as character
 * pointers when they are passed to this function.
 */
static void
ErrMsg (severity, code, usage, format, a1, a2, a3)
	long		severity;					/* Severity of message			*/
	uint_t		code;						/* Exit code (0 to not exit)	*/
	uint_t		usage;						/* Flag to indicate usage msg	*/
	const char	*format;					/* printf-style format string	*/
	void		*a1, *a2, *a3;				/* Arguments					*/
{
	(void) pfmt (stderr, severity, format, a1, a2, a3);

	if (usage == SHOW_USAGE) {
		DispUsage ();
	}
	if (code != ONLY_NOTIFY) {
		exit (code);
	}
	return;
}





/*
 * Gracefully handle a library function failure.  Allow the caller to
 * specify the exit value if they want, but provide some reasonable defaults.
 *
 * We are required to exit with certain values if we get certain failures
 * from the library functions.  This routine provides that mapping.
 */
static void
InterpretFailure (err)
	uint_t			err;						/* Pre-specified exit value	*/
{
	uint_t			ErrCode;					/* Exit value for utility	*/

	Msg = strerror (errno);
	if (err != UNKNOWN_EXIT_STATUS) {
		ErrCode = err;
	} else {
		switch (errno) {
			case ENXIO:
				/* FALLTHROUGH */
			case EINVAL: {
				ErrCode = EXIT_CODE_2;
				break;
			}
			default: {
				ErrCode = EXIT_CODE_1;
				break;
			}
		}
	}
	ErrMsg (MM_ERROR, ErrCode, NO_USAGE, ":13:%s\n", Msg, NULL, NULL);
}





/*
 * Get the PVD.
 */
static void
GetPVD ()
{
	int				RetVal;					/* Return value holder			*/

	RetVal = cd_cpvd (File, Buf);

	if (RetVal == -1) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}
	return;
}





/*
 * Print the PVD.
 */
static void
PrintPVD ()
{
	uint_t			count;					/* Loop counter					*/
	enum cdfs_type	Type;					/* Type of file system			*/

	if (Binary) {
		for (count = 0; count < CD_PVDLEN; count++) {
			(void) putchar (Buf[count]);
		}
	} else {
		Type = cd_type (File);
		cdfs_DispPvd (Buf, Type);
	}
	return;
}

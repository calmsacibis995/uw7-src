/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:i386/cmd/fs.d/cdfs/cdsuf/cdsuf.c	1.8"
#ident	"$Header$"

static char cdsuf_copyright[] = "Copyright 1991, 1992 Intel Corp. 469259";

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
#include <sys/fs/iso9660.h>

#include "cdsuf.h"

/*
 * cdsuf - Get System Use Field from CD-ROM Directory Record.  This is one
 * of the RRIP API utilities.
 *
 * usage: cdsuf [ -s number ] [ -b ] <file>
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

	DoSUF ();

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
	while ((option = getopt ((int) count, vector, "s:b")) != EOF) {
		switch (option) {
			case 's': {
				Section = atoi (optarg);
				if (Section < 1) {
					ErrMsg (MM_ERROR, EXIT_CODE_2, NO_USAGE,
								":51:File section %d invalid.\n",
								(void *) Section, NULL, NULL);
				}
				break;
			}
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
					":71:Need to specify a file or directory.\n", NULL, NULL,
					NULL);
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
	(void) pfmt (stderr, MM_ACTION, ":97:Usage: %s [ -s number ] [ -b ] <file>\n",
				ProgName);

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
			case ENOMATCH: {
				ErrCode = EXIT_CODE_3;
				break;
			}
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
 * Get the SUFs, and print them out as we go.
 */
static void
DoSUF ()
{
	int				RetVal;					/* Return value holder			*/
	uint_t			index;					/* Which SUF # are we getting?	*/
	boolean_t		NeedHeader = B_TRUE;	/* Do we still need the header?	*/

	/*
	 * Cycle through all available indices until there are no more SUFs.
	 */
	for (index = 1; ; index++) {
		RetVal = cd_suf (File, Section, NULL, index, (char *) SufBuf,
					sizeof (SufBuf));
		if (RetVal == 0) {
			/*
			 * cd_suf can find no more SUFs.
			 */
			break;
		}
		if (RetVal == -1) {
			if (errno == ENXIO) {
				ErrMsg (MM_ERROR, EXIT_CODE_2, NO_USAGE,
							":51:File section %d invalid.\n", (void *) Section,
							NULL, NULL);
			} else {
				if (errno == ENOMATCH) {
					ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
								":50:File section %d has no system use area or SUSP is disabled.\n",
								(void *) Section, NULL, NULL);
				} else {
					InterpretFailure (UNKNOWN_EXIT_STATUS);
				}
			}
		}
		if (NeedHeader && !Binary) {
			if (Section == -1) {
				Msg = gettxt (":132", "System Use Fields for file %s:\n");
				(void) printf (Msg, File);
			} else {
				Msg = gettxt (":133", "System Use Fields for section %d of file %s:\n");
				(void) printf (Msg, Section, File);
			}
			Msg = gettxt (":126", "Sig Len Ver     Data\n");
			(void) printf (Msg);
			NeedHeader = B_FALSE;
		}
		PrintSUF ((uint_t) RetVal);
	}

	if (index == 1) {
		ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
					":153:File has no system use area or SUSP is disabled.\n",
					NULL, NULL, NULL);
	}

	return;
}





/*
 * Print a single SUF.
 */
static void
PrintSUF (length)
	uint_t		length;						/* # of bytes to print			*/
{
	uint_t		count;						/* Loop counter					*/
	struct susp_suf *Ptr;					/* Pointer at the current SUF	*/

	if (Binary) {
		for (count = 0; count < length; count++) {
			(void) putchar (SufBuf[count]);
		}
	} else {
		Ptr = (struct susp_suf *) SufBuf;
		(void) printf ("%c%c %3hu %3hu  ", Ptr->suf_Sig1, Ptr->suf_Sig2,
					(ushort_t) Ptr->suf_Len, (ushort_t) Ptr->suf_Ver);
		for (count = 0;
					count < ((uint_t) Ptr->suf_Len - sizeof (struct susp_suf));
					count++) {
			if ((count != 0) && ((count % 22) == 0)) {
				(void) printf ("\n            ");
			}
			(void) printf (" %.2X", SufBuf[count + sizeof (struct susp_suf)]);
		}
		(void) printf ("\n");
	}
	return;
}

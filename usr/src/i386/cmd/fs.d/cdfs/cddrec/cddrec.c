/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:i386/cmd/fs.d/cdfs/cddrec/cddrec.c	1.9"
#ident	"$Header$"

static char cddrec_copyright[] = "Copyright 1991, 1992 Intel Corp. 469257";

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

#include "cddrec.h"

/*
 * cddrec - Read Directory Record from CD-ROM.  This is one of the XCDR API
 * utilities.
 *
 * usage: cddrec [ -s number ] [ -b ] <file>
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

	GetDREC ();

	PrintDREC ();

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
					":71:Need to specify a file or directory.\n", NULL,
					NULL, NULL);
	}
	if (strlen (vector[optind]) > MAXPATHLEN) {
		errno = ENAMETOOLONG;
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}
	File = (char *) vector[optind];

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
			case EACCES:
				/* FALLTHROUGH */
			case ENAMETOOLONG:
				/* FALLTHROUGH */
			case ENOENT:
				/* FALLTHROUGH */
			case ENOTDIR: {
				ErrCode = EXIT_CODE_1;
				break;
			}
			case ENXIO: {
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
 * Get the DREC.
 */
static void
GetDREC ()
{
	int				RetVal;					/* Return value holder			*/

	if (Binary) {
		RetVal = cd_cdrec (File, Section, Buf);
	} else {
		RetVal = cd_drec (File, Section, &DRec);
	}
	if (RetVal == -1) {
		if (errno == ENXIO) {
			ErrMsg (MM_ERROR, EXIT_CODE_2, NO_USAGE,
						":51:File section %d invalid.\n", (void *) Section,
						NULL, NULL);
		} else {
			InterpretFailure (UNKNOWN_EXIT_STATUS);
		}
	}
	return;
}





/*
 * Print the DREC.
 */
static void
PrintDREC ()
{
	uint_t		count;						/* Loop counter					*/
	char		DateString[80];				/* Temporary date holder		*/
	struct tm	*TimeStruct;				/* Working Time Structure		*/

	if (Binary) {
		for (count = 0; count < CD_MAXDRECL; count++) {
			(void) putchar (Buf[count]);
		}
	} else {
		Msg = gettxt (":98", "Directory Record for %s:\n");
		(void) printf (Msg, File);
		Msg = gettxt (":49", "Directory Record Length:  %u bytes\n");
		(void) printf (Msg, (uint_t) DRec.drec_len);
		Msg = gettxt (":123", "XAR Length:  %u logical block(s)\n");
		(void) printf (Msg, (uint_t) DRec.xar_len);
		Msg = gettxt (":60", "Extent Location:  Logical block %lu\n");
		(void) printf (Msg, DRec.locext);
		Msg = gettxt (":41", "Data Length:  %u byte(s)\n");
		(void) printf (Msg, (uint_t) DRec.data_len);
		Msg = gettxt (":115", "Recording Date and Time:  %s\n");
		if (DRec.rec_time != (time_t) 0) {
			TimeStruct = localtime (&DRec.rec_time);
			(void) strftime (DateString, sizeof (DateString), "%N", TimeStruct);
		} else {
			(void) strcpy (DateString, "Not defined");
		}
		(void) printf (Msg, DateString);
		Msg = gettxt (":65", "File Flags:  0x%.2X\n");
		(void) printf (Msg, (uint_t) DRec.file_flags);
		Msg = gettxt (":70", "File Unit Size:  %u bytes\n");
		(void) printf (Msg, (uint_t) DRec.file_usize);
		Msg = gettxt (":79", "Interleave Gap Size:  %u logical block(s)\n");
		(void) printf (Msg, (uint_t) DRec.ileav_gsize);
		Msg = gettxt (":118", "Volume Sequence Number:  %hu\n");
		(void) printf (Msg, (uint_t) DRec.volseqno);
		Msg = gettxt (":66", "File ID Length:  %u byte(s)\n");
		(void) printf (Msg, (uint_t) DRec.fileid_len);
		Msg = gettxt (":67", "File ID:  %.37s\n");
		if (DRec.file_id[0] == '\0') {
			(void) printf (Msg, "byte 00");
		} else {
			(void) printf (Msg, DRec.file_id);
		}
		Msg = gettxt (":131", "System Use Field Length:  %u byte(s)\n");
		(void) printf (Msg, (uint_t) DRec.sysuse_len);
	}
	return;
}

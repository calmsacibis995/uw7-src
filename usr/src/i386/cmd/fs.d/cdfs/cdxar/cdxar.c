/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:i386/cmd/fs.d/cdfs/cdxar/cdxar.c	1.9"
#ident	"$Header$"

static char cdxar_copyright[] = "Copyright 1991, 1992 Intel Corp. 469261";

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

#include "cdxar.h"

/*
 * cdxar - Read Extended Attribute Record from CD-ROM.  This is one of the
 * XCDR API utilities.
 *
 * usage: cdxar [ -s number ] [ -b ] <file>
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

	GetXARSize ();

	if (XARBlks != 0) {
		GetXAR ();

		PrintXAR ();
	} else {
		ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
					":52:File section has no XAR.\n", NULL, NULL, NULL);
	}

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
	File = vector[optind];

	return;
}





/*
 * Display usage message.
 */
static void
DispUsage ()
{
	(void) pfmt (stderr, MM_ACTION, ":97:Usage: %s [ -s number ] [ -b ] <file>\n", ProgName);

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
			/*
			 * Note that we're supposed to use an exit value of 4 if the
			 * given file section is valid, but on another CD.  Since
			 * we don't support multi-volume CDs, we don't have any way
			 * of knowing this.  Claim the file section does not exist.
			 */
			case ENODEV:
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
	return;
}





/*
 * Get the size of the XAR.
 */
static void
GetXARSize ()
{
	int				RetVal;					/* Return value holder			*/
	
	RetVal = cd_drec (File, Section, &DREC);
	if (RetVal == -1) {
		if (errno == ENXIO) {
			ErrMsg (MM_ERROR, EXIT_CODE_2, NO_USAGE,
						":51:File section %d invalid.\n", (void *) Section,
						NULL, NULL);
		} else {
			InterpretFailure (UNKNOWN_EXIT_STATUS);
		}
	}
	XARBlks = (ushort_t) DREC.xar_len;

	RetVal = cd_pvd (File, &PVD);
	if (RetVal == -1) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}

	XARBytes = PVD.lblksize * XARBlks;

	return;
}





/*
 * Get the XAR in one of two ways.  Either way, capture the size of the
 * value returned.
 */
static void
GetXAR ()
{
	int				RetVal;					/* Return value holder			*/

	if (Binary) {
		Buf = malloc ((size_t) XARBytes);
		if (Buf == NULL) {
			ErrMsg (MM_ERROR, EXIT_CODE_1, NO_USAGE,
						":135:Unable to allocate %d bytes of memory.\n",
						(void *) XARBytes, NULL, NULL);
		}
		RetVal = cd_cxar (File, Section, Buf, XARBytes);
		XARBytes = RetVal;
	} else {
		RetVal = cd_xar (File, Section, &XAR, 0, 0);
		XARBytes = RetVal + CD_XARFIXL;
	}
	if (RetVal == -1) {
		if (Binary) {
			free (Buf);
		}
		switch (errno) {
			case ENXIO: {
				ErrMsg (MM_ERROR, EXIT_CODE_2, NO_USAGE,
							":51:File section %d invalid.\n", (void *) Section,
							NULL, NULL);
				break;
			}
			case ENOMATCH: {
				ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
							":52:File section has no XAR.\n", NULL, NULL, NULL);
				break;
			}
			default: {
				InterpretFailure (UNKNOWN_EXIT_STATUS);
			}
		}
	}

	return;
}





/*
 * Print the XAR.
 */
static void
PrintXAR ()
{
	uint_t		count;						/* Loop counter					*/
	char		DateString[80];				/* Temporary date holder		*/
	struct tm	*TimeStruct;				/* Working time structure		*/
	int			DateSize;					/* sizeof DateString			*/
	uint_t		OctalPerms;					/* Octal perm representation	*/

	if (Binary) {
		for (count = 0; count < XARBytes; count++) {
			(void) putchar (Buf[count]);
		}
		free (Buf);
	} else {
		DateSize = sizeof (DateString);
		OctalPerms = 0;
		OctalPerms |= ((XAR.permissions & CD_RUSR) == CD_RUSR) ? IREAD_USER : 0;
		OctalPerms |= ((XAR.permissions & CD_XUSR) == CD_XUSR) ? IEXEC_USER : 0;
		OctalPerms |= ((XAR.permissions & CD_RGRP) == CD_RGRP) ?
					IREAD_GROUP : 0;
		OctalPerms |= ((XAR.permissions & CD_XGRP) == CD_XGRP) ?
					IEXEC_GROUP : 0;
		OctalPerms |= ((XAR.permissions & CD_ROTH) == CD_ROTH) ?
					IREAD_OTHER : 0;
		OctalPerms |= ((XAR.permissions & CD_XOTH) == CD_XOTH) ?
					IEXEC_OTHER : 0;

		Msg = gettxt (":59", "Extended Attribute Record for %s:\n");
		(void) printf (Msg, File);
		Msg = gettxt (":104", "Owner ID:  %hu\n");
		(void) printf (Msg, (ushort_t) XAR.own_id);
		Msg = gettxt (":75", "Group ID:  %hu\n");
		(void) printf (Msg, (ushort_t) XAR.grp_id);
		Msg = gettxt (":108", "Permission bits:  0x%.4X (or %.3o)\n");
		(void) printf (Msg, XAR.permissions, OctalPerms);

		Msg = gettxt (":62", "File Creation Date and Time:  %s\n");
		TimeStruct = localtime (&XAR.cre_time);
		(void) strftime (DateString, DateSize, "%N", TimeStruct);
		(void) printf (Msg, DateString);
		Msg = gettxt (":68", "File Modification Date and Time:  %s\n");
		TimeStruct = localtime (&XAR.mod_time);
		(void) strftime (DateString, DateSize, "%N", TimeStruct);
		(void) printf (Msg, DateString);
		Msg = gettxt (":64", "File Expiration Date and Time:  %s\n");
		TimeStruct = localtime (&XAR.exp_time);
		(void) strftime (DateString, DateSize, "%N", TimeStruct);
		(void) printf (Msg, DateString);
		Msg = gettxt (":63", "File Effective Date and Time:  %s\n");
		TimeStruct = localtime (&XAR.eff_time);
		(void) strftime (DateString, DateSize, "%N", TimeStruct);
		(void) printf (Msg, DateString);
		Msg = gettxt (":113", "Record Format:  %hu\n");
		(void) printf (Msg, (ushort_t) XAR.rec_form);
		Msg = gettxt (":112", "Record Attributes:  %hu\n");
		(void) printf (Msg, (ushort_t) XAR.rec_attr);
		Msg = gettxt (":114", "Record Length:  %hu bytes\n");
		(void) printf (Msg, (ushort_t) XAR.rec_len);
		Msg = gettxt (":129", "System ID:  %.32s\n");
		(void) printf (Msg, XAR.sys_id);
		Msg = gettxt (":134", "System Use:  %.64s\n");
		(void) printf (Msg, XAR.sys_use);
		Msg = gettxt (":124", "XAR Version:  %hu\n");
		(void) printf (Msg, (ushort_t) XAR.xar_vers);
		Msg = gettxt (":87", "Length of Escape Sequences:  %hu\n");
		(void) printf (Msg, (ushort_t) XAR.esc_len);
		Msg = gettxt (":86", "Length of Application Use:  %hu\n");
		(void) printf (Msg, (ushort_t) XAR.appuse_len);
	}

	return;
}

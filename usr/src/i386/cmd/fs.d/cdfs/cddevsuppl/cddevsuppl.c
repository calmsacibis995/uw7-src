/*		copyright	"%c%" 	*/

/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:i386/cmd/fs.d/cdfs/cddevsuppl/cddevsuppl.c	1.9"
#ident	"$Header$"

static char cddevsuppl_copyright[] = "Copyright 1991, 1992 Intel Corp. 469256";

/* Tabstops: 4 */

#include <errno.h>
#include <libgen.h>
#include <locale.h>
#include <pfmt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/cdrom.h>
#include <sys/mnttab.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/fs/cdfs_fs.h>
#include <sys/fs/cdfs_ioctl.h>

#include "cddevsuppl.h"


/*
 * cddevsuppl - Set and get major and minor numbers of a device file on
 * CD-ROM.  This is one of the RRIP API utilities.
 *
 * usage: cddevsuppl [ -m mapfile | -u unmapfile ] [ -c ]
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

	DoMappings ();

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
	while ((option = getopt ((int) count, vector, "m:u:c")) != EOF) {
		switch (option) {
			case 'm': {
				if (DoUnmap == B_TRUE) {
					ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
								":103:Options -%1s and -%1s are mutually exclusive.\n",
								(char *) &option, "u", NULL);
				}
				DoMap = B_TRUE;
				(void) strncpy ((char *) MapFile, optarg, sizeof (MapFile));
				break;
			}
			case 'u': {
				if (DoMap == B_TRUE) {
					ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
								":103:Options -%1s and -%1s are mutually exclusive.\n",
								(char *) &option, "m", NULL);
				}
				DoUnmap = B_TRUE;
				(void) strncpy ((char *) MapFile, optarg, sizeof (MapFile));
				break;
			}
			case 'c': {
				TmpCont = B_TRUE;
				break;
			}
			default: {
				ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
							":82:Invalid option: -%1s\n", (char *) &optopt,
							NULL, NULL);
				break;
			}
		}
	}

	if (optind != count) {
		ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
					":91:Extra arguments at end of command.\n",
					NULL, NULL, NULL);
	}

	return;
}





/*
 * Display usage message.
 */
static void
DispUsage ()
{
	(void) pfmt (stderr, MM_ACTION,
				":139:Usage: %s [ -m mapfile | -u unmapfile ] [ -c ]\n",
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
	if ((code != ONLY_NOTIFY) && (!ContOnError)) {
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
	if (ContOnError) {
		ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE, ":13:%s\n", Msg, NULL, NULL);
	} else {
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
				case EPERM: {
					ErrCode = EXIT_CODE_2;
					break;
				}
				case EBADFD: {
					ErrCode = EXIT_CODE_6;
					break;
				}
				default: {
					ErrCode = EXIT_CODE_4;
					break;
				}
			}
		}
		ErrMsg (MM_ERROR, ErrCode, NO_USAGE, ":13:%s\n", Msg, NULL, NULL);
	}
}





/*
 * Do the device mappings.
 */
static void
DoMappings ()
{
	int				RetVal;					/* Return value holder			*/
	uint_t			count = 0;				/* Loop counter					*/
	int				num_read;				/* Number of fields sscanf read	*/
	FILE			*fp;					/* Map/mnttab file pointer		*/
	int				cmd;					/* Command to execute			*/
	int				MapSize;				/* Size of Maps arg				*/
	struct mnttab	MntEnt;					/* Mount table entry			*/
	boolean_t		AnyMappings = B_FALSE;	/* Are there any mappings?		*/
	uchar_t			*String[MAXPATHLEN + MAXNAMELEN];	/* Working string	*/
	
	if (DoMap || DoUnmap) {
		/*
		 * Fill array from map file.
		 */
		fp = fopen ((char *) MapFile, "r");
		if (fp == NULL) {
			ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
						":32:Cannot open device map file: %s\n", MapFile,
						NULL, NULL);
		}

		ContOnError = TmpCont;

		MapSize = sizeof (Maps);
		if (DoMap) {
			cmd = CD_SETDMAP;
			while ((count < MapSize) &&
						(fgets ((char *) String, sizeof (String), fp) !=
						NULL) &&
						((num_read = sscanf ((char *) String, "%s%d%d",
						Maps[count].DevPath, &Maps[count].Major,
						&Maps[count].Minor)) != EOF)) {
				if (num_read < 3) {
					if (!ContOnError) {
						(void) fclose (fp);
					}
					ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
								":76:Incorrect map entry at line %u in file %s\n",
								(void *) (count + 1), MapFile, NULL);
					count++;
					continue;
				}

				/*
				 * Do the library function and print out the values we got back.
				 */
				RetVal = cd_setdevmap (Maps[count].DevPath, cmd,
							&Maps[count].Major, &Maps[count].Minor);
				if (RetVal == 1) {
					(void) printf ("%s	%d	%d\n", Maps[count].DevPath,
								Maps[count].Major, Maps[count].Minor);
				} else {
					if (RetVal == 0) {
						ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
									":154:Too many device mappings, %d allowed.\n",
									(void *) CD_MAXDMAP, NULL, NULL);
					} else {
						switch (errno) {
							case ENOENT: {
								ErrMsg (MM_ERROR, EXIT_CODE_1, NO_USAGE,
											":72:No such file: %s\n",
											Maps[count].DevPath, NULL, NULL);
								break;
							}
							case ENOSYS: {
								ErrMsg (MM_ERROR, EXIT_CODE_5, NO_USAGE,
											":73:Not a device file: %s\n",
											Maps[count].DevPath, NULL, NULL);
								break;
							}
							default: {
								InterpretFailure (UNKNOWN_EXIT_STATUS);
								break;
							}
						}
					}
				}

				count++;
			}
			if (count >= MapSize) {
				ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
							":80:Too many entries in file %s, %d allowed.\n",
							MapFile, (void *) MapSize, NULL);
			}
		}
		if (DoUnmap) {
			cmd = CD_UNSETDMAP;
			while ((fgets ((char *) String, sizeof (String), fp) != NULL) &&
						((num_read = sscanf ((char *) String, "%s",
						Maps[count].DevPath)) != EOF) && (count < MapSize)) {
				if (num_read < 1) {
					if (!ContOnError) {
						(void) fclose (fp);
					}
					ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
								":77:Incorrect unmap entry at line %u in file %s\n",
								(void *) (count + 1), MapFile, NULL);
					count++;
					continue;
				}

				/*
				 * Do the library function and print out the values we got back.
				 */
				RetVal = cd_setdevmap (Maps[count].DevPath, cmd,
							&Maps[count].Major, &Maps[count].Minor);
				if (RetVal == 1) {
					(void) printf ("%s	%d	%d\n", Maps[count].DevPath,
								Maps[count].Major, Maps[count].Minor);
				} else {
					if (!ContOnError) {
						(void) fclose (fp);
					}
					if (RetVal == 0) {
						ErrMsg (MM_ERROR, EXIT_CODE_6, NO_USAGE,
									":42:File not previously mapped, cannot unmap: %s\n",
									Maps[count].DevPath, NULL, NULL);
					} else {
						switch (errno) {
							case ENOENT: {
								ErrMsg (MM_ERROR, EXIT_CODE_1, NO_USAGE,
											":72:No such file: %s\n",
											Maps[count].DevPath, NULL, NULL);
								break;
							}
							case ENOSYS: {
								ErrMsg (MM_ERROR, EXIT_CODE_5, NO_USAGE,
											":73:Not a device file: %s\n",
											Maps[count].DevPath, NULL, NULL);
								break;
							}
							default: {
								InterpretFailure (UNKNOWN_EXIT_STATUS);
								break;
							}
						}
					}
				}

				count++;
			}
			if (count >= MapSize) {
				ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
							":80:Too many entries in file %s, %d allowed.\n",
							MapFile, (void *) MapSize, NULL);
			}
		}

		if (fclose (fp) == EOF) {
			ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE,
						":56:Error closing file: %s\n", MapFile, NULL, NULL);
		}
	} else {
		/*
		 * Fill the array of mappings.  Note that the index argument begins
		 * at 1, but we number the array starting at 0.
		 */
		fp = fopen (MNTTAB, "r");
		if (fp == NULL) {
			InterpretFailure (UNKNOWN_EXIT_STATUS);
		}

		ContOnError = TmpCont;

		/*
		 * Cycle through all cdfs file systems currently mounted, getting
		 * all device mappings for them.
		 *
		 * For each, initialize DevPath to be the mount point - note that
		 * this is an undocumented feature of cd_getdevmap to support
		 * multiple cdfs file systems on a system.
		 */
		while ((RetVal = getmntent (fp, &MntEnt)) == 0) {
			if (strcmp (MntEnt.mnt_fstype, CDFS_ID) == 0) {
				for (count = 0; count < CD_MAXDMAP; count++) {
					(void) strcpy (Maps[count].DevPath, MntEnt.mnt_mountp);
					RetVal = cd_getdevmap (Maps[count].DevPath, MAXPATHLEN,
								count + 1, &Maps[count].Major,
								&Maps[count].Minor);
					if (RetVal == 0) {
						break;
					}
					if (RetVal == -1) {
						if (!ContOnError) {
							(void) fclose (fp);
						}
						InterpretFailure (UNKNOWN_EXIT_STATUS);
						break;
					}

					if (!AnyMappings) {
						Msg = gettxt (":39",
									"Current mappings of devices under %s:\n");
						(void) printf (Msg, MntEnt.mnt_mountp);
					}
					AnyMappings = B_TRUE;

					(void) printf ("%s	%d	%d\n", Maps[count].DevPath,
								Maps[count].Major, Maps[count].Minor);
				}
			}
		}
		(void) fclose (fp);

		/*
		 * Handle unusual (non-EOF) failures of getmntent.
		 */
		switch (RetVal) {
			case MNT_TOOLONG: {
				ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
							":37:Entry too long in %s file.\n",
							MNTTAB, NULL, NULL);
				/* NOTREACHED */
				break;
			}
			case MNT_TOOMANY: {
				ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
							":78:Too many entries in %s file.\n",
							MNTTAB, NULL, NULL);
				/* NOTREACHED */
				break;
			}
			case MNT_TOOFEW: {
				ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
							":74:Too few entries in %s file.\n",
							MNTTAB, NULL, NULL);
				/* NOTREACHED */
				break;
			}
		}

		if (!AnyMappings) {
			ErrMsg (MM_INFO, ONLY_NOTIFY, NO_USAGE,
						":69:No device mappings on system.\n",
						NULL, NULL, NULL);
		}
	}

	return;
}

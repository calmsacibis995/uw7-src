/*		copyright	"%c%" 	*/

/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	
#ident	"@(#)cdfs.cmds:i386/cmd/fs.d/cdfs/cdmntsuppl/cdmntsuppl.c	1.9"
#ident	"$Header$"

static char cdmntsuppl_copyright[] = "Copyright 1991, 1992 Intel Corp. 469255";

/* Tabstops: 4 */

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <libgen.h>
#include <locale.h>
#include <pfmt.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdrom.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fs/cdfs_fs.h>
#include <sys/fs/cdfs_ioctl.h>

#include "cdmntsuppl.h"


/*
 * cdmntsuppl - Set and get administrative CD-ROM features.  This is one
 * of the XCDR API utilities.
 *
 * The particular features are:
 *		Set the default owner and group for files that do not have one.
 *		Set the default permissions for files that do not have permissions.
 *		Set the default permissions for directories that do not have
 *			permissions.
 *		Set user ID mapping to convert user IDs to different ones.
 *		Set group ID mapping to convert group IDs to different ones.
 *		Set file name conversion features:
 *			Turn name conversion off.
 *			Set conversion to convert identifiers to lower case, and to
 *				remove separator 1 (.) if no file name extension is present.
 *			Set conversion to remove the version number and separator 2 (;)
 *				from identifiers.
 *		Set directory search permissions:
 *			Use the execute permission bits as they are in the XAR.
 *			Use the inclusive OR of the read and execute permission bits in
 *				the XAR to set the execute permission.
 *		Get all current settings.
 *
 * usage: cdmntsuppl [ -u owner ] [ -g group ] [ -F mode ] [ -D mode ]
 *		[ -U umfile ] [ -G gmfile ] [ -c ] [ -l ] [ -m ] [ -x ] [ -s ]
 *		<mount_point>
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

	ValParams ();

	if (DoDefaults) {
		SetDefaults ();
	}

	if (DoMapping) {
		SetMappings ();
	}

	if (DoConversion) {
		SetNameConv ();
	}

	if (!DoDefaults && !DoMapping && !DoConversion) {
		GetSettings ();
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
	int			option;

	opterr = 0;
	while ((option = getopt ((int) count, vector, "u:g:F:D:U:G:clmxs")) !=
				EOF) {
		switch (option) {
			case 'u': {
				OwnerString = (uchar_t *) optarg;
				break;
			}
			case 'g': {
				GroupString = (uchar_t *) optarg;
				break;
			}
			case 'F': {
				FModeString = (uchar_t *) optarg;
				break;
			}
			case 'D': {
				DModeString = (uchar_t *) optarg;
				break;
			}
			case 'U': {
				UMFile = (uchar_t *) optarg;
				break;
			}
			case 'G': {
				GMFile = (uchar_t *) optarg;
				break;
			}
			/*
			 * No name conversion option - this may not be specified along
			 * with another name conversion option.  Make sure that
			 * another option has not already been specified.
			 */
			case 'c': {
				if ((NameConv & (CD_LOWER | CD_NOVERSION)) == 0) {
					DoConversion = B_TRUE;
					NameConv = CD_NOCONV;
				} else {
					if (NameConv & CD_LOWER != 0) {
						ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
									":103:Options -%1s and -%1s are mutually exclusive.\n",
									(char *) &option, "l", NULL);
					} else {
						ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
									":103:Options -%1s and -%1s are mutually exclusive.\n",
									(char *) &option, "m", NULL);
					}
				}
				break;
			}
			/*
			 * Lower case name conversion option - this may not be specified
			 * along with the no name conversion option.  Make sure that
			 * that option has not already been specified.
			 */
			case 'l': {
				if ((NameConv & CD_NOCONV) == 0) {
					DoConversion = B_TRUE;
					NameConv |= CD_LOWER;
				} else {
					ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
								":103:Options -%1s and -%1s are mutually exclusive.\n",
								(char *) &option, "c", NULL);
				}
				break;
			}
			/*
			 * No version name conversion option - this may not be specified
			 * along with the no name conversion option.  Make sure that
			 * that option has not already been specified.
			 */
			case 'm': {
				if ((NameConv & CD_NOCONV) == 0) {
					DoConversion = B_TRUE;
					NameConv |= CD_NOVERSION;
				} else {
					ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
								":103:Options -%1s and -%1s are mutually exclusive.\n",
								(char *) &option, "c", NULL);
				}
				break;
			}
			case 'x': {
				if (!DirInterpFlag) {
					DirInterpFlag = B_TRUE;
					DirSearch = CD_DIRXAR;
				} else {
					ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
								":103:Options -%1s and -%1s are mutually exclusive.\n",
								(char *) &option, "s", NULL);
				}
				break;
			}
			case 's': {
				if (!DirInterpFlag) {
					DirInterpFlag = B_TRUE;
					DirSearch = CD_DIRRX;
				} else {
					ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
								":103:Options -%1s and -%1s are mutually exclusive.\n",
								(char *) &option, "x", NULL);
				}
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

	if (count - optind != 1) {
		ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
					":95:Mount point argument is missing.\n", NULL, NULL, NULL);
	}
	MntPt = (uchar_t *) vector[optind];

	return;
}





/*
 * Validate command-line parameters.
 */
static void
ValParams ()
{
	int				RetVal;				/* Return value holder				*/
	struct passwd	*PwEnt;				/* Password file entry				*/
	struct group	*GrEnt;				/* Group file entry					*/
	struct stat		StatBuf;			/* Results of stat() call			*/

	/*
	 * Verify default file/dir owner.  Note that a decimal uid is not validated,
	 * since we may specify any uid to be the default file owner.
	 */
	if (OwnerString != NULL) {
		if (IsDecimal (OwnerString)) {
			Owner = (uid_t) atoi ((char *) OwnerString);
		} else {
			if ((PwEnt = getpwnam ((char *) OwnerString)) == NULL) {
				ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
							":101:No such user: %s\n", OwnerString, NULL, NULL);
			} else {
				Owner = PwEnt->pw_uid;
			}
		}
		DoDefaults = B_TRUE;
	}

	/*
	 * Verify default file/dir group.  Note that a decimal gid is not validated,
	 * since we may specify any gid to be the default file group.
	 */
	if (GroupString != NULL) {
		if (IsDecimal (GroupString)) {
			Group = (uid_t) atoi ((char *) GroupString);
		} else {
			if ((GrEnt = getgrnam ((char *) GroupString)) == NULL) {
				ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
							":100:No such group: %s\n", GroupString, NULL,
							NULL);
			} else {
				Group = GrEnt->gr_gid;
			}
		}
		DoDefaults = B_TRUE;
	}

	/*
	 * Verify default file permissions.  Valid permissions are either
	 * of the form 0xxx or [who]op[permission].
	 */
	if (FModeString != NULL) {
		if (IsOctal (FModeString)) {
			FMode = strtol ((char *) FModeString, NULL, 8);
		} else {
			FMode = GetPerms (FModeString, &FModeOp);
		}
		DoDefaults = B_TRUE;
	}

	/*
	 * Verify default directory permissions.  Valid permissions are either
	 * of the form 0xxx or [who]op[permission].
	 */
	if (DModeString != NULL) {
		if (IsOctal (DModeString)) {
			DMode = strtol ((char *) DModeString, NULL, 8);
		} else {
			DMode = GetPerms (DModeString, &DModeOp);
		}
		DoDefaults = B_TRUE;
	}

	if (DirInterpFlag && !DoDefaults) {
		DoDefaults = B_TRUE;
	}

	if (UMFile != NULL) {
		DoMapping = B_TRUE;
	}

	if (GMFile != NULL) {
		DoMapping = B_TRUE;
	}

	/*
	 * Check the mount point argument.
	 */
	RetVal = stat ((char *) MntPt, &StatBuf);
	if (RetVal != 0) {
		ErrMsg (MM_ERROR, EXIT_CODE_1, SHOW_USAGE,
					":25:Cannot access mount point: %s\n", MntPt, NULL, NULL);
	}
	if ((StatBuf.st_mode & S_IFDIR) != S_IFDIR) {
		ErrMsg (MM_ERROR, EXIT_CODE_1, SHOW_USAGE,
					":102:Not a mount point: %s\n", MntPt, NULL, NULL);
	}

	return;
}





/*
 * Display usage message.
 */
static void
DispUsage ()
{
	(void) pfmt (stderr, MM_ACTION, ":141:Usage: %s [ -u owner ] [ -g group ] \
[ -F mode ]\n	[ -D mode ] [ -U umfile ] [ -G gmfile ] [ -c | -l | -m ] \
[ -x | -s ]\n	<mount_point>\n", ProgName);

	return;
}





/*
 * Display error message, exit if desired.  All a[1-3] arguments are
 * character pointers.  Print out integers by casting them as character
 * pointers when they are passed to this function.
 */
static void
ErrMsg (severity, code, usage, format, a1, a2, a3)
	long		severity;	/* Severity of message		*/
	uint_t		code;		/* Exit code (0 to not exit)	*/
	uint_t		usage;		/* Flag to indicate usage msg	*/
	const char	*format;	/* printf-style format string	*/
	void		*a1, *a2, *a3;	/* Arguments			*/
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
 * Set default file/directory attributes.
 */
static void
SetDefaults ()
{
	struct cd_defs	Defs;				/* Default structure				*/
	int				RetVal;				/* Return value holder				*/

	/*
	 * Collect the current defaults, so they may be selectively overwritten.
	 */
	RetVal = cd_defs ((char *) MntPt, CD_GETDEFS, &Defs);
	if (RetVal != 0) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}

	/*
	 * Overwrite only those fields specified here.
	 */
	if (OwnerString != NULL) {
		Defs.def_uid = Owner;
	}
	if (GroupString != NULL) {
		Defs.def_gid = Group;
	}
	if (DirInterpFlag) {
		Defs.dirsperm = DirSearch;
	}

	/*
	 * Apply the permission operators to the existing defaults.
	 */
	if (FModeString != NULL) {
		switch (FModeOp) {
			case '+':
				Defs.def_fperm = Defs.def_fperm | FMode;
				break;
			case '-':
				Defs.def_fperm = Defs.def_fperm & (FMode ^ ALL);
				break;
			case '=':
				/* FALLTHROUGH */
			default: {
				Defs.def_fperm = FMode;
				break;
			}
		}
	}
	if (DModeString != NULL) {
		switch (DModeOp) {
			case '+':
				Defs.def_dperm = Defs.def_dperm | DMode;
				break;
			case '-':
				Defs.def_dperm = Defs.def_dperm & (DMode ^ ALL);
				break;
			case '=':
				/* FALLTHROUGH */
			default: {
				Defs.def_dperm = DMode;
				break;
			}
		}
	}

	/*
	 * Set the new defaults.
	 */
	RetVal = cd_defs ((char *) MntPt, CD_SETDEFS, &Defs);
	if (RetVal != 0) {
		InterpretFailure (errno == EINVAL ? EXIT_CODE_4 : UNKNOWN_EXIT_STATUS);
	}

	return;
}





/*
 * Set file/directory user/group mappings.
 */
static void
SetMappings ()
{
	FILE				*ufp;				/* UID map file pointer			*/
	FILE				*gfp;				/* GID map file pointer			*/
	uint_t				ucount;				/* User mapping loop counter	*/
	uint_t				gcount;				/* Group mapping loop counter	*/
	int					num_read;			/* fscanf() counter				*/
	uchar_t				ToName[MAXNAMELEN];	/* Name to map to				*/
	struct passwd		*PwEnt;				/* Password file entry			*/
	struct group		*GrEnt;				/* Group file entry				*/
	int					RetVal;				/* Return value holder			*/
	int					MapSize;			/* Size of Maps arg				*/

	/*
	 * Cycle through UID mapping file, filling UID mapping array.  Entries
	 * are of the form "from_uid:to_whom".  The field (to_whom) may be either
	 * a user ID or a user name.
	 */
	if (UMFile != NULL) {
		ufp = fopen ((char *) UMFile, "r");
		if (ufp == NULL) {
			ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
						":35:Cannot open user ID map file: %s\n", UMFile,
						NULL, NULL);
		}
		ucount = 0;
		MapSize = sizeof (UIDMap) / sizeof (struct cd_idmap);
		while ((ucount < MapSize) && ((num_read =
					fscanf (ufp, "%hu:%s", &UIDMap[ucount].from_id,
					ToName)) != EOF)) {
			if (num_read != 2) {
					(void) fclose (ufp);
					ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
								":122:Wrong number of fields on line %u of file %s\n",
								(void *) (ucount + 1), UMFile, NULL);
			}
			if (IsDecimal (ToName)) {
				UIDMap[ucount].to_uid = (uid_t) atoi ((char *) ToName);
			} else {
				if ((PwEnt = getpwnam ((char *) ToName)) == NULL) {
					(void) fclose (ufp);
					ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
								":92:Invalid user %s at line %u in file %s\n",
								ToName, (void *) (ucount + 1), UMFile);
				} else {
					UIDMap[ucount].to_uid = PwEnt->pw_uid;
				}
			}
			UIDMap[ucount].to_gid = CDFS_UNUSED_MAP_ENTRY;
			ucount++;
		}
		if (num_read != EOF) {
			ErrMsg (MM_ERROR, EXIT_CODE_3, NO_USAGE,
						":80:Too many entries in file %s, %d allowed.\n",
						UMFile, (void *) MapSize, NULL);
			ucount = MapSize;
		}
		if (fclose (ufp) == EOF) {
			ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE,
						":56:Error closing file: %s\n", UMFile, NULL, NULL);
		}
	}

	/*
	 * Cycle through GID mapping file, filling GID mapping array.  Entries
	 * are of the form "from_gid:to_whom".  The field (to_whom) may be either
	 * a group ID or a group name.
	 */
	if (GMFile != NULL) {
		gfp = fopen ((char *) GMFile, "r");
		if (gfp == NULL) {
			ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
						":33:Cannot open group ID map file: %s\n", GMFile,
						NULL, NULL);
		}
		gcount = 0;
		MapSize = sizeof (GIDMap) / sizeof (struct cd_idmap);
		while ((gcount < MapSize) && ((num_read =
					fscanf (gfp, "%hu:%s", &GIDMap[gcount].from_id,
					ToName)) != EOF)) {
			if (num_read != 2) {
					(void) fclose (gfp);
					ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
								":122:Wrong number of fields on line %u of file %s\n",
								(void *) (gcount + 1), GMFile, NULL);
			}
			if (IsDecimal (ToName)) {
				GIDMap[gcount].to_gid = (gid_t) atoi ((char *) ToName);
			} else {
				if ((GrEnt = getgrnam ((char *) ToName)) == NULL) {
					(void) fclose (gfp);
					ErrMsg (MM_ERROR, EXIT_CODE_4, NO_USAGE,
								":85:Invalid group %s at line %u in file %s\n",
								ToName, (void *) (gcount + 1), GMFile);
				} else {
					GIDMap[gcount].to_gid = GrEnt->gr_gid;
				}
			}
			GIDMap[gcount].to_uid = CDFS_UNUSED_MAP_ENTRY;
			gcount++;
		}
		if (num_read != EOF) {
			ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE,
						":80:Too many entries in file %s, %d allowed.\n",
						GMFile, (void *) MapSize, NULL);
			gcount = MapSize;
		}
		if (fclose (gfp) == EOF) {
			ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE,
						":56:Error closing file: %s\n", GMFile, NULL, NULL);
		}
	}

	/*
	 * Now go do the real work.
	 */
	if (UMFile != NULL) {
		RetVal = cd_idmap ((char *) MntPt, CD_SETUMAP, UIDMap, (int *) &ucount);
		if (RetVal != 0) {
			switch (errno) {
				case ENXIO: {
					ErrMsg (MM_WARNING, EXIT_CODE_3, NO_USAGE,
								":150:Too many ID mappings, %d allowed.\n",
								(void *) CD_MAXUMAP, NULL, NULL);
					break;
				}
				default: {
					InterpretFailure (UNKNOWN_EXIT_STATUS);
					break;
				}
			}
		}
	}
	if (GMFile != NULL) {
		RetVal = cd_idmap ((char *) MntPt, CD_SETGMAP, GIDMap, (int *) &gcount);
		if (RetVal != 0) {
			switch (errno) {
				case ENXIO: {
					ErrMsg (MM_WARNING, EXIT_CODE_3, NO_USAGE,
								":150:Too many ID mappings, %d allowed.\n",
								(void *) CD_MAXUMAP, NULL, NULL);
					break;
				}
				default: {
					InterpretFailure (UNKNOWN_EXIT_STATUS);
					break;
				}
			}
		}
	}

	return;
}





/*
 * Set file/directory name conversion options.
 */
static void
SetNameConv ()
{
	int				RetVal;					/* Return value holder			*/
	
	RetVal = cd_nmconv ((char *) MntPt, CD_SETNMCONV, (int *) &NameConv);
	if (RetVal != 0) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}

	return;
}





/*
 * Get current settings of administrative features.
 */
static void
GetSettings ()
{
	struct cd_defs		Defaults;	/* Default attribute structure*/
	int			NmConv;		/* Name conversion options*/
	int			numap;		/* Number of UID mappings to get*/
	int			ngmap;		/* Number of UID mappings to get*/
	int			RetVal;		/* Return value holder	*/
	uint_t			count;		/* Loop counter		*/

	/*
	 * Retrieve the current settings.
	 */
	RetVal = cd_defs ((char *) MntPt, CD_GETDEFS, &Defaults);
	if (RetVal != 0) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}
	RetVal = cd_nmconv ((char *) MntPt, CD_GETNMCONV, &NmConv);
	if (RetVal != 0) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}

	/*
	 * Note that numap and ngmap are modified by the cd_idmap calls
	 * to be the numbers of mappings actually present.
	 */
	numap = CD_MAXUMAP;
	RetVal = cd_idmap ((char *) MntPt, CD_GETUMAP, UIDMap, &numap);
	if (RetVal != 0) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}
	ngmap = CD_MAXGMAP;
	RetVal = cd_idmap ((char *) MntPt, CD_GETGMAP, GIDMap, &ngmap);
	if (RetVal != 0) {
		InterpretFailure (UNKNOWN_EXIT_STATUS);
	}

	/*
	 * Print out the current settings.
	 */
	Msg = gettxt (":15", "Administrative feature settings for the CD-ROM on %s:\n");
	(void) printf (Msg, MntPt);
	Msg = gettxt (":46", "Default user ID:  %d\n");
	(void) printf (Msg, (int) Defaults.def_uid);
	Msg = gettxt (":45", "Default group ID:  %d\n");
	(void) printf (Msg, (int) Defaults.def_gid);
	Msg = gettxt (":44", "Default file permissions:  %4o\n");
	(void) printf (Msg, (int) Defaults.def_fperm);
	Msg = gettxt (":43", "Default directory permissions:  %4o\n");
	(void) printf (Msg, (int) Defaults.def_dperm);
	Msg = gettxt (":17", "Directory search permission comes from:  %s.\n");
	switch (Defaults.dirsperm) {
		case CD_DIRXAR: {
			(void) printf (Msg, "XAR Execute bits");
			break;
		}
		case CD_DIRRX: {
			(void) printf (Msg, "XAR Execute and Read bits");
			break;
		}
		default: {
			ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE, Msg,
						":116:Mode not recognized", NULL, NULL);
			break;
		}
	}

	Msg = gettxt (":53", "Name conversion:  %s.\n");
	if (NmConv == CD_NOCONV) {
		(void) printf (Msg, "None");
	}
	if ((NmConv & CD_LOWER) != 0) {
		if ((NmConv & CD_NOVERSION) != 0) {
			(void) printf (Msg, "No Version Numbers and Lower Case");
			Msg = gettxt (":14", "	(File name version numbers and separators \
(\";\") will be removed.)\n");
			(void) printf (Msg);
		} else {
			(void) printf (Msg, "Lower Case");
		}
		Msg = gettxt (":16", "	(File names will be converted to lower case, \
and the separator \".\" will\n	be removed if there is no file name \
extension.)\n");
		(void) printf (Msg);
	} else {
		if ((NmConv & CD_NOVERSION) != 0) {
			(void) printf (Msg, "No Version Numbers");
			Msg = gettxt (":14", "	(File name version numbers and separators \
(\";\") will be removed.)\n");
			(void) printf (Msg);
		}
	}
	if ((NmConv & (CD_NOCONV | CD_LOWER | CD_NOVERSION)) == 0) {
		ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE, Msg, "Mode not recognized",
					NULL, NULL);
	}

	if (numap > 0) {
		Msg = gettxt (":40", "Current user ID mappings (from : to) are:\n");
		(void) printf (Msg);
		for (count = 0; count < numap; count++) {
			if (UIDMap[count].to_uid == CDFS_UNUSED_MAP_ENTRY) {
				ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE,
							":144:User ID mapping %u unused.\n",
							(void *) count, NULL, NULL);
			}
			(void) printf ("%d:%ld\n", UIDMap[count].from_id,
						UIDMap[count].to_uid);
		}
	} else {
		Msg = gettxt (":90", "There are no user ID mappings defined.\n");
		(void) printf (Msg);
	}

	if (ngmap > 0) {
		Msg = gettxt (":38", "Current group ID mappings (from : to) are:\n");
		(void) printf (Msg);
		for (count = 0; count < ngmap; count++) {
			if (GIDMap[count].to_gid == CDFS_UNUSED_MAP_ENTRY) {
				ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE,
							":1:Group ID mapping %u unused.\n",
							(void *) count, NULL, NULL);
			}
			(void) printf ("%d:%ld\n", GIDMap[count].from_id,
						GIDMap[count].to_gid);
		}
	} else {
		Msg = gettxt (":89", "There are no group ID mappings defined.\n");
		(void) printf (Msg);
	}

	return;
}





/*
 * Check whether the given string holds a valid decimal number.
 */
static boolean_t
IsDecimal (s)
	uchar_t			*s;						/* String to check				*/
{
	register char	c;						/* Working character			*/

	while ((c = *s++) != '\0') {
		if (isdigit (c) == 0) {
			return (B_FALSE);
		}
	}
	return (B_TRUE);
}





/*
 * Check whether the given string holds a valid octal number.
 */
static boolean_t
IsOctal (s)
	uchar_t			*s;						/* String to check				*/
{
	register char	c;						/* Working character			*/

	while ((c = *s++) != '\0') {
		if ((isdigit (c) == 0) || (c == '8') || (c == '9')) {
			return (B_FALSE);
		}
	}
	return (B_TRUE);
}





/*
 * Turn permission string into a mode_t and a permission operator (+|-|=).
 * Note that this handles the case where "<who>=" is given, which results
 * in a permission of 000.
 *
 * Modifies string pointer.
 */
static mode_t
GetPerms (string, op)
	uchar_t		*string;					/* Extract perms from this string*/
	uchar_t		*op;						/* +|-|= operator				*/
{
	mode_t		perm;						/* Working copy of permission	*/
	mode_t		who_mask;					/* Which users are we setting?	*/
	uchar_t		*tmp;						/* Working pointer				*/

	tmp = string;
	who_mask = who (tmp);
	*op = what (tmp);
	if (*op == 0) {
		return (who_mask);
	}

	for (; ((*tmp != '\0') && (*tmp != *op)); tmp++) {
		continue;
	}

	if (*tmp == *op) {
		tmp++;
	}

	for (perm = 0; *tmp != '\0'; tmp++) {
		switch (*tmp) {
			case 'r': {
				perm |= READ;
				break;
			}
			case 'x': {
				perm |= EXECUTE;
				break;
			}
			case 'w': {
				ErrMsg (MM_WARNING, ONLY_NOTIFY, NO_USAGE,
							":81:Write permission is invalid and is being ignored.\n",
							NULL, NULL, NULL);
				break;
			}
			default: {
				ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
							":83:Invalid permission mode: %1s\n",
							tmp, NULL, NULL);
			}
		}
	}
	
	return (who_mask & perm);
}





/*
 * Set the user permission mask.
 */
static mode_t
who (string)
	uchar_t		*string;					/* String to manipulate			*/
{
	uchar_t		*tmp;						/* Working copy of string		*/
	register mode_t	m;						/* Working copy of perm mode	*/

	for (m = (mode_t) 0, tmp = string; *tmp != '\0'; tmp++) {
		switch (*tmp) {
			case 'u': {
				m |= USER;
				break;
			}
			case 'g': {
				m |= GROUP;
				break;
			}
			case 'o': {
				m |= OTHER;
				break;
			}
			case 'a': {
				m |= ALL;
				break;
			}
			/*
			 * The user might have specified the operator and no "who" info.
			 */
			case '+':
				/* FALLTHROUGH */
			case '-':
				/* FALLTHROUGH */
			case '=': {
				if (m == 0) {
					return ((mode_t) ALL);
				} else {
					return (m);
				}

				/* NOTREACHED */
				break;
			}
			default: {
				ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
							":83:Invalid permission mode: %1s\n",
							tmp, NULL, NULL);
			}
		}
	}

	return ((mode_t) 0);
}





/*
 * Get the permission operation to perform.
 */
static int
what (string)
	uchar_t		*string;					/* String ptr to manipulate		*/
{
	uchar_t		*tmp;						/* Working copy of string		*/

	for (tmp = string; *tmp != '\0'; tmp++) {
		switch (*tmp) {
			case 'u':
				/* FALLTHROUGH */
			case 'g':
				/* FALLTHROUGH */
			case 'o':
				/* FALLTHROUGH */
			case 'a': {
				break;
			}

			case '+':
				/* FALLTHROUGH */
			case '-':
				/* FALLTHROUGH */
			case '=': {
				return (*tmp);

				/* NOTREACHED */
				break;
			}

			case 'r':
				/* FALLTHROUGH */
			case 'w':
				/* FALLTHROUGH */
			case 'x': {
				ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
							":93:Missing permission operator: +, -, or =.\n",
							NULL, NULL, NULL);
			}

			default: {
				ErrMsg (MM_ERROR, EXIT_CODE_4, SHOW_USAGE,
							":84:Invalid permission operator: %1s\n", tmp,
							NULL, NULL);
			}
		}
	}

	return (0);
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
		uint_t			err;					/* Pre-specified exit value	*/
{
		uint_t			ErrCode;				/* Exit value for utility	*/

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
				case EPERM: {
					ErrCode = EXIT_CODE_2;
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

/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:common/cmd/fs.d/cdfs/mount/mount.c	1.10"
#ident	"$Header$"

static char mount_copyright[] = "Copyright 1991,1992 Intel Corp. 469253";

/* Tabstops: 4 */

#include	<errno.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/errno.h>
#include	<sys/mount.h>
#include	<sys/mnttab.h>
#include	<sys/param.h>
#include	<sys/signal.h>
#include	<sys/statvfs.h>
#include	<sys/fs/cdfs_fs.h>
#include	<sys/fs/cdfs_inode.h>
#include	<sys/fs/cdfs_ioctl.h>

#include	"mount_local.h"

/*
 * Token list for the various CDFS-specific options.
 * These "base" options do not include extension-specific
 * options such as those defined by XCDR and RRIP.
 */
STATIC char	*CdfsOptList[] = {
	"ro",									/* Mount CDFS as Read-Only		*/
	"Not_Supported", /* "rw" */ 			/* Mount CDFS as Read-Write		*/
	"suid",									/* Allow Set-UID execution		*/
	"nosuid",								/* Don't allow Set-UID execution*/
	"Not_Supported", /*	"remount" */		/* Remount a R/O CDFS as R/W	*/
	"Not_Supported", /* "lock" */ 			/* Lock media into drive		*/
	"Not_Supported", /* "nolock" */			/* Don't lock media into drive	*/
	"noextend",								/* Ignore all ISO-9660 extensions*/
	"susp",									/* Process SUSP extension data	*/
	"nosusp",								/* Ignore SUSP extension data	*/
	"rrip",									/* Process RRIP extension data	*/
	"norrip",								/* Ignore RRIP extension data	*/
	"lsectsz",								/* Logical sector size of FS	*/
	NULL
};

#define	READONLY_INDX		0				/* Array index of 'base' tokens	*/
#define	READWRITE_INDX		1			
#define SETUID_INDX			2
#define NOSETUID_INDX		3
#define REMOUNT_INDX		4
#define	LOCK_INDX			5
#define	NOLOCK_INDX			6
#define	NOEXTEND_INDX		7
#define	SUSP_INDX			8
#define	NOSUSP_INDX			9
#define	RRIP_INDX			10
#define	NORRIP_INDX			11
#define	SECTSZ_INDX			12

STATIC boolean_t 	ReadOnly =	 B_FALSE;	/* Token was specified by user	*/
STATIC boolean_t 	ReadWrite =	 B_FALSE;
STATIC boolean_t 	SetUid =	 B_FALSE;
STATIC boolean_t 	NoSetUid =	 B_FALSE;
STATIC boolean_t 	Remount =	 B_FALSE;
STATIC boolean_t 	NoExtend =	 B_FALSE;
STATIC boolean_t 	Susp =		 B_FALSE;
STATIC boolean_t 	NoSusp =	 B_FALSE;
STATIC boolean_t 	Rrip =		 B_FALSE;
STATIC boolean_t 	NoRrip =	 B_FALSE;
STATIC boolean_t 	LSectSz =	 B_FALSE;

STATIC uint_t		LSectSzVal;				/* Value specified with token	*/

/*
 * Token list for the various XCDR-specific options.
 */
STATIC char	*XcdrOptList[] = {
	"uid",	 								/* Defines the default User ID	*/
	"gid",									/* Defines the default Group ID	*/
	"fperm",								/* Defines default file perms	*/
	"dperm",								/* Defines default directory perm*/
	"uidmap",								/* Defines a User ID mapping file*/
	"gidmap",								/* Defines a Group mapping file	*/
	"nmconv",								/* Defines name conversion mode	*/
	"dsearch",								/* Defines directory search mode*/
	NULL
};

#define	UID_INDX		0					/* Array index of XCDR tokens	*/
#define	GID_INDX		1
#define	FILEPERM_INDX	2
#define	DIRPERM_INDX	3
#define	UIDMAP_INDX		4
#define	GIDMAP_INDX		5
#define	NAMECONV_INDX	6
#define DIRSEARCH_INDX	7

STATIC boolean_t	Uid =		 B_FALSE;	/* Token was specified by user	*/
STATIC boolean_t	Gid =		 B_FALSE;
STATIC boolean_t	FilePerm =	 B_FALSE;
STATIC boolean_t	DirPerm =	 B_FALSE;
STATIC boolean_t	UidMap =	 B_FALSE;
STATIC boolean_t	GidMap =	 B_FALSE;
STATIC boolean_t	NameConv =	 B_FALSE;
STATIC boolean_t	DirSearch =	 B_FALSE;

STATIC char		*UidVal;					/* Value specified with token	*/
STATIC char		*GidVal;
STATIC char		*FilePermVal;
STATIC char		*DirPermVal;
STATIC char		*UidMapVal;
STATIC char		*GidMapVal;
STATIC char		*NameConvVal;
STATIC char		*DirSearchVal;

STATIC char	XcdrCmdName[] =					/* XCDR cmd to implement options*/
	"/usr/lib/fs/cdfs/cdmntsuppl";



/*
 * Token list of RRIP-specific options.
 */
STATIC char	*RripOptList[] = {
	"devmap",								/* Defines device node map file	*/
	NULL
};

#define	DEVMAP_INDX		0					/* Array index of RRIP tokens	*/

STATIC boolean_t 	DevMap = 0;				/* Token was specified by user	*/

STATIC char 		*DevMapVal = 0;			/* Value specified with token	*/

STATIC char	RripCmdName[] =					/* RRIP cmd to implement options*/
	"/usr/lib/fs/cdfs/cddevsuppl";



/*
 * General globals for this program.
 */
#define MAX_PROG_NAME	128					/* Max size of a program name	*/
#define TMP_BUF_SZ		512					/* Size of temp storage buffer	*/
#define TMP_MNTTBL		"/var/tmp/mnttab.tmp"/* Tmp version of Mount table	*/

STATIC char	*ProgName;						/* Program name pointer			*/
STATIC char	FsProgName[MAX_PROG_NAME];		/* CDFS-version of prog name	*/

STATIC char	TmpBuf[TMP_BUF_SZ];				/* Tempoary storage buffer		*/
STATIC char	*Msg;							/* Temporary string				*/



/*
 * Mount a CDFS file system.
 */
main(argc, argv)
int	argc;
char	*argv[];
{
	char	*DevNode;					/* Device node with file system		*/
	char	*MntPnt;					/* Mount-point 						*/

	char	MntTblName[] = MNTTAB;		/* File name of current mount table	*/
	char	TmpMntTblName[]= TMP_MNTTBL;/* File name of temp. mount table	*/
	FILE	*MntTbl;					/* Current mount table file			*/
	FILE	*TmpMntTbl;					/* Temporary mount table file		*/

	struct mnttab		NewMTE;			/* New mount table entry			*/
	struct mnttab		TmpMTE;			/* Temporary mount table entry		*/

	int 				MntFlags = 0;	/* Mount(2) system call	flags 		*/
	struct cdfs_mntargs CdfsMntArgs;	/* Mount(2) CDFS specific mnt args	*/

	struct statvfs		StatVfs;		/* Status/info of mounted file system*/
	int 				RetVal;			/* Return value of procedure calles	*/
	uchar_t				string[MAXNAMELEN];	/* Temporary string				*/

	/*
	 * Modify program-name to include the CDFS ID string:
	 * <CDFS ID string> <Basename of command>
	 */
	ProgName = strrchr(argv[0], '/');
	if (ProgName) {
		ProgName++;
	} else {
		ProgName = argv[0];
	}
	(void) sprintf(FsProgName, "%s %s", CDFS_ID, ProgName);
	argv[0] = &FsProgName[0];

	(void) setlocale (LC_ALL, "");
	(void) sprintf ((char *) string, "UX:");
	(void) strcat ((char *) string, FsProgName);
	(void) setlabel ((char *) string);
	(void) setcat ("uxcdfs");

	/*
	 *	Parse command-line options.
	 */
	RetVal = ParseOpts(argc, argv);
	if (RetVal != RET_OK) {
		DispUsage();
		exit(1);
	}
	
	/*
	 * Get Device node and Mount point arguments.
	 * Note: Exactly two more arguments should exist.  Even if the user
	 * had origianlly specified on one argument, the generic 'mount()'
	 * command should have determined the other argument via '/etc/vfstab'.
	 */
	if (argc - optind == 2) {
		DevNode = argv[optind++];
		MntPnt = argv[optind++];

	} else if (argc - optind == 1) {
			(void) pfmt(stderr, MM_ERROR,
				":95:Mount point argument is missing.\n");
			DispUsage();
			exit(1);

	} else if (argc - optind <= 0) {
			(void) pfmt(stderr, MM_ERROR,
				":127:Special file and mount point arguments are missing.\n");
			DispUsage();
			exit(1);

	} else {
			(void) pfmt(stderr, MM_WARNING,
				":61:Extraneous argument(s) starting with: %s\n",
				argv[optind+2]);
	}
	
	/*
	 * Setup the default operating mode:
	 * - Set UID execution is allowed,
	 * - SUSP and RRIP extensions are enabled,
	 * - Media is locked into drive while mounted
	 */
	if (NoSetUid == B_FALSE) {
		SetUid = B_TRUE;
	}
	if ((NoExtend == B_FALSE) && (NoSusp == B_FALSE)) {
		Susp = B_TRUE;
	}
	if ((Susp == B_TRUE) && (NoRrip == B_FALSE)) {
		Rrip = B_TRUE;
	}
		
	/* 
	 * Validate pre-conditions.
	 */

	if (ReadOnly != B_TRUE) {
		(void) pfmt(stderr, MM_ERROR,
			":23:CDFS is read-only: Either '-r' or '-o ro' is required.\n");
		DispUsage();
		exit(1);
	}

	if ((Rrip == B_TRUE) && (Susp == B_FALSE)) {
		(void) pfmt(stderr, MM_ERROR,
			":111:RRIP extension requires the SUSP extension.\n");
		exit(1);
	}
		
	/*
	 * Build a new mount table entry.
	 * Note: Storage area for the Mount-Time data follows the storage
	 * area for the mount option list within the temporary buffer.
	 * Also, it is assumed that : %ld == long == time_t.
	 */
	NewMTE.mnt_mountp = MntPnt;
	NewMTE.mnt_special = DevNode;
	NewMTE.mnt_fstype = CDFS_ID;

	RetVal = BldOpts(&TmpBuf[0], sizeof(TmpBuf));
	if (RetVal != RET_OK) {
		exit(2);
	}
	NewMTE.mnt_mntopts = &TmpBuf[0];

	NewMTE.mnt_time = &TmpBuf[strlen(TmpBuf)+1];
	(void) sprintf(NewMTE.mnt_time, "%ld", time(0L));

	/*
	 * Open and lock the mount table file.
	 * The lock may sleep, but it prevents simultaneous updates.
	 */
	 MntTbl = fopen(MntTblName, "r");
	 if (MntTbl == NULL) {
		(void) pfmt(stderr, MM_WARNING,
			":34:Cannot open mount table (%s) for reading.\n", MntTblName);
		Msg = strerror(errno);
		(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
	}

	MntTbl = fopen(MntTblName, "r+");
	if (MntTbl == NULL) {
		(void) pfmt(stderr, MM_WARNING,
			":88:Cannot open mount table (%s) to update.\n", MntTblName);
		Msg = strerror (errno);
		(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
		exit(3);
	}

	RetVal = lockf(fileno(MntTbl), F_LOCK, 0L);
	if (RetVal < 0) {
		(void) pfmt(stderr, MM_WARNING,
			":29:Cannot lock mount table (%s).\n", MntTblName);
		Msg = strerror (errno);
		(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
		exit(3);
	}

	/*
	 * Setup the mount(2) system call arguments and make the call.
	 * Note: MntFlags is used only by mount(2).  CdfsMntArgs and
	 * its length are passed to the CDFS code in the kernel.
	 */
	(void) signal(SIGHUP,  SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGINT,  SIG_IGN);

	MntFlags = MS_DATA |
		(ReadOnly == B_TRUE ? MS_RDONLY : 0) |
		(NoSetUid == B_TRUE ? MS_NOSUID : 0) |
		(Remount == B_TRUE ? MS_REMOUNT : 0);

	CdfsMntArgs.mnt_Flags =
		(Susp == B_TRUE ? CDFS_SUSP : 0) |
		(Rrip == B_TRUE ? CDFS_RRIP : 0);

	if (LSectSz == B_TRUE) {
		CdfsMntArgs.mnt_Flags |= CDFS_USER_BLKSZ;
		CdfsMntArgs.mnt_LogSecSz = LSectSzVal;
	} else {
		CdfsMntArgs.mnt_LogSecSz = 0;
	}

	RetVal = mount(DevNode, MntPnt, MntFlags, CDFS_ID,
		(caddr_t)&CdfsMntArgs, sizeof(CdfsMntArgs));

	if (RetVal != 0) {
		DispMntErr(DevNode, MntPnt);
 		exit(4);
	}

	RetVal = statvfs(MntPnt, &StatVfs);
	if (RetVal != 0) {
		(void) pfmt(stderr, MM_WARNING,
			":24:Cannot 'stat' mounted file system.\n");
	}

	/*
	 * The mount was successful, so update the mount table
	 * with the new entry as appropriate.
	 */
	if (Remount != B_TRUE) {
		/*
		 * Since the file system is not already mounted, the new
		 * mount table entry can just be added to the end of the
		 * current mount table.
		 */
		RetVal = fseek(MntTbl, 0L, SEEK_END);
		if (RetVal == 0) {
			putmntent(MntTbl, &NewMTE);
		} else {
			(void) pfmt(stderr, MM_WARNING,
				":136:Unable to locate end of mount table file.\n");
		}
		RetVal = fclose(MntTbl);
		if (RetVal !=0) {
			(void) pfmt(stderr, MM_WARNING,
				":26:Cannot close mount table file.\n");
		}
	} else {
		/*
		 * File system is being remounted, so replace the current
		 * mount table entry with the new one.  Create a temp
		 * mount table file with all of the entries except the
		 * one being replaced.  Then, add the new entry to the
		 * temp file and rename the temp file to the name of the
		 * original mount table file.
		 * Note: The original file is closed AFTER the rename
		 *	 in order to eliminate the race condition.
		 */
		TmpMntTbl = fopen(TmpMntTblName, "w");
		if (TmpMntTbl == NULL) {
			(void) pfmt(stderr, MM_ERROR,
				":28:Cannot create temporary mount table (%s).\n",
				TmpMntTblName);
			Msg = strerror (errno);
			(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
			exit(1);
		}
	
		rewind(MntTbl);
		for (;;) {
			RetVal = getmntent(MntTbl, &TmpMTE);
			if (RetVal != 0) {
				break;
			}
			if (strcmp(TmpMTE.mnt_special, NewMTE.mnt_special) != 0) {
				putmntent(TmpMntTbl, &TmpMTE);
			}
		}

		putmntent(TmpMntTbl, &NewMTE);
		RetVal = fclose(TmpMntTbl);
		if (RetVal != 0) {
			(void) pfmt(stderr, MM_ERROR,
				":27:Cannot close temporary mount table file.\n");
		}
		RetVal = rename(TmpMntTblName, MntTblName);
		if (RetVal != 0) {
			(void) pfmt(stderr, MM_ERROR,
				":36:Cannot rename temporary mount table file.\n");
		}
		RetVal = fclose(MntTbl);
		if (RetVal != 0) {
			(void) pfmt(stderr, MM_ERROR,
				":26:Cannot close mount table file.\n");
		}
	}

	/*
	 * Build and invoke the XCDR command string for processing
	 * the XCDR-specific options. 
	 */
	RetVal = BldXcdrCmd(MntPnt, TmpBuf, sizeof(TmpBuf));
	if (RetVal != RET_OK) {
		(void)pfmt(stderr, MM_ERROR,
			":55:Error building XCDR command-line.\n");
		/* No exit - Continue mounting */
	}

	if (strlen(TmpBuf) != 0) {
		RetVal = system(TmpBuf);
		if (RetVal != RET_OK) {
			(void)pfmt(stderr, MM_ERROR,
				":58:Error processing XCDR options.\n");
			/* No exit - Continue mounting */
		}
	}

	/*
	 * Build and invoke the RRIP command string for processing
	 * the RRIP-specific options.
	 */ 
	RetVal = BldRripCmd(MntPnt, TmpBuf, sizeof(TmpBuf));
	if (RetVal != RET_OK) {
		(void)pfmt(stderr, MM_ERROR,
			":54:Error building RRIP command-line.\n");
		/* No exit - Continue mounting */
	}

	if (strlen(TmpBuf) != 0) {
		RetVal = system(TmpBuf);
		if (RetVal != RET_OK) {
			(void)pfmt(stderr, MM_ERROR,
				":57:Error processing RRIP options.\n");
			/* No exit - Continue mounting */
		}
	}

	exit(0);
}




STATIC void
DispUsage()
{
	(void) pfmt(stderr, MM_ACTION, ":128:Comply with CDFS usage:\n");
	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":130:\n%s [-F %s] [generic_opts] [-o <CDFS_opts>] {special | mount_point}\n",
		"mount", CDFS_ID);
	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":138:%s [-F %s] [generic_opts] [-o <CDFS_opts>] special mount_point\n",
		"mount", CDFS_ID);

	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":143:\nwhere <CDFS_opts> include:\n");

	(void) pfmt(stderr, MM_INFO|MM_NOSTD, ":145:\nBasic Options:\n");
	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":146:\t[ro][suid|nosuid][lsectsz=<value>]\n");
	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":147:\t[[susp[rrip|norrip]|nosusp]|noextend]\n");

	(void) pfmt(stderr, MM_INFO|MM_NOSTD, ":148:\nXCDR Options:\n");
	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":149:\t[uid=<UID>][gid=<GID>][fperm=<perm>][dperm=<perm>]\n");
	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":117:\t[uidmap=<file>][gidmap=<file>][nmconv=<token>][dsearch=<token>]\n");

	(void) pfmt(stderr, MM_INFO|MM_NOSTD, ":110:\nRRIP Options:\n");
	(void) pfmt(stderr, MM_INFO|MM_NOSTD, ":119:\t[devmap=<file>]\n");

	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":120:\nMandatory options: '-r' or '-o ro'\n");
	(void) pfmt(stderr, MM_INFO|MM_NOSTD,
		":121:Default options: '-o suid,susp,rrip'\n");
}




/*
 * Parse the command-line arguments.  A flag is set for
 * each option specified as well as the value, if any,
 * associated with that option.
 */
STATIC int
ParseOpts(ArgCnt, ArgList)
u_int	ArgCnt;
char	*ArgList[];
{
	int 	Opt;					/* Current command-line option		*/

	char	*SubOpts;	 			/* Address of '-o' suboption string	*/
	int 	SubOptIndx;				/* Index of matching CDFS suboption	*/
	char	*SubOptVal;				/* Value of matching CDFS suboption	*/

	boolean_t	Conflict;				/* Conflict between options 		*/
	char	*ConflictMsg;			/* Conflict error message			*/

	boolean_t Duplicate; 				/* Same option specified twice		*/
	char	*DuplicateMsg;			/* Duplicate error message			*/

	boolean_t Error; 					/* Unknown option detected			*/
	int		RetVal; 				/* Return value of called functions */

	/*
	 * Use 'getopt(3)' to help parse the argument list.
	 * Note: 'opterr = 0' instructs 'getopt(3)' not display
	 * any error messages.  Error messages are handled here.
	 */
	opterr = 0;				
	Conflict = B_FALSE;
	ConflictMsg = NULL;
	Duplicate = B_FALSE;
	DuplicateMsg = NULL;
	Error = B_FALSE;
	while ((Opt = getopt((int)ArgCnt, ArgList, "?o:r")) != -1) {
		switch (Opt) {
		case 'r': {
			/*
			 * Read-Only flag.
			 */
			if (ReadOnly == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Read-Only'";
			} else if (ReadWrite == B_TRUE) { 
				Conflict = B_TRUE;
				ConflictMsg = "'Read-Only' and 'Read-Write'";
			} else {
				ReadOnly = B_TRUE;
			}
			break;
		}
		case 'o': {
			/*
			 * Parse CDFS-specific options.
			 */
			SubOpts = optarg;
			while (*SubOpts != '\0') {
				SubOptIndx =
					getsubopt(&SubOpts, CdfsOptList, &SubOptVal);
				switch (SubOptIndx) {
				case READONLY_INDX: {
					/*
					 * Mount file system as Read-Only.
					 */
					if (ReadOnly == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'Read-Only'";
					} else if (ReadWrite == B_TRUE) { 
						Conflict = B_TRUE;
						ConflictMsg = "'Read-Only' and 'Read-Write'";
					} else {
						ReadOnly = B_TRUE;
					}
					break;
				}
				case READWRITE_INDX: {
					/*
					 * Mount file system as Read-Write.
					 */
					/* WARNING:
					 * Suboption needs to be added to CDFS option list
					 * when ReadWrite is supported.
					 */
					if (ReadWrite == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'Read-Write'";
					} else if (ReadOnly == B_TRUE) { 
						Conflict = B_TRUE;
						ConflictMsg = "'Read-Write' and 'Read-Only'";
					} else {
						ReadWrite = B_TRUE;
					}
					break;
				}
				case SETUID_INDX: {
					/*
					 * Allow Set UID program execution.
					 */
					if (SetUid == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'Set UID'";
					} else if (NoSetUid == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'Set UID and No Set UID'";
					} else {
						SetUid = B_TRUE;
					}
					break;
				}
				case NOSETUID_INDX: {
					/*
					 * Disable Set UID program execution.
					 */
					if (NoSetUid == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'No Set UID'";
					} else if (SetUid == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'No Set UID and Set UID'";
					} else {
						NoSetUid = B_TRUE;
					}
					break;
				}
				case REMOUNT_INDX: {
					/*
					 * Remount file system.  It is presumed the
					 * file system is alread mounted Read-Only and
					 * that this remount is to make it Read-Write.
					 */
					/* WARNING:
					 * Suboption needs to be added to CDFS option list
					 * when ReadWrite is supported.
					 */
					if (Remount == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'Remount'";
					} else if (ReadOnly == B_TRUE) { 
						Conflict = B_TRUE;
						ConflictMsg = "'Remount and Read-Only'";
					} else {
						Remount = B_TRUE;
					}
					break;
				}
				case NOEXTEND_INDX: {
					/*
					 * Ignore any (ISO-9660) file system extensions
					 * such as SUSP/RRIP.
					 */
					if (NoExtend == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'No Extend'";
					} else if (Susp == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'No Extend and SUSP'";
					} else if (Rrip == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'No Extend and RRIP'";
					} else {
						NoExtend = B_TRUE;
					}
					break;
				}
				case SUSP_INDX: {
					/*
					 * Recognize and process any SUSP extensions.
					 */
					if (Susp == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'SUSP'";
					} else if (NoExtend == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'SUSP and No Extend'";
					} else if (NoSusp == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'SUSP and No SUSP'";
					} else {
						Susp = B_TRUE;
					}
					break;
				}
				case NOSUSP_INDX: {
					/*
					 * Ignore SUSP extensions.
					 */
					if (NoSusp == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'No SUSP'";
					} else if (Susp == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'No SUSP and SUSP'";
					} else if (Rrip == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'No SUSP and RRIP'";
					} else {
						NoSusp = B_TRUE;
					}
					break;
				}
				case RRIP_INDX: {
					/*
					 * Recognize any process any RRIP extensions.
					 */
					if (Rrip == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'RRIP'";
					} else if (NoRrip == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'RRIP and No RRIP'";
					} else if (NoExtend == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'RRIP and No Extend'";
					} else if (NoSusp == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'RRIP and No SUSP'";
					} else {
						Rrip = B_TRUE;
					}
					break;
				}
				case NORRIP_INDX: {
					/*
					 * Ignore RRIP extensions.
					 */
					if (NoRrip == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'No RRIP'";
					} else if (Rrip == B_TRUE) {
						Conflict = B_TRUE;
						ConflictMsg = "'No RRIP and RRIP'";
					} else {
						NoRrip = B_TRUE;
					}
					break;
				}
				case SECTSZ_INDX: {
					/*
					 * Sector size of media.
					 */
					if (LSectSz == B_TRUE) {
						Duplicate = B_TRUE;
						DuplicateMsg = "'Sector Size'";
					} else {
						LSectSz = B_TRUE;
						LSectSzVal = (uint_t)atoi(SubOptVal);
					}
					break;
				}
				default: {
					/*
					 * Unrecognized option.
					 * See if its an XCDR or RRIP option before
					 * displaying an error message.
					 */
					RetVal = ParseXcdrOpt(SubOptVal);
					if (RetVal == RET_OK) {
						break;
					}
					if (RetVal == RET_ERR) {
						Error = B_TRUE;
						break;
					}

					RetVal = ParseRripOpt(SubOptVal);
					if (RetVal == RET_OK) {
						break;
					}
					if (RetVal == RET_ERR) {
						Error = B_TRUE;
						break;
					}

					/*
					 * Unrecognized CDFS-specific (-o) option.
					 */
					(void) pfmt(stderr, MM_ERROR,
						":137:Unrecognized %s-specific (-o) suboption: '%s'\n",
						CDFS_ID, SubOptVal);
					Error = B_TRUE;
				}
				} /* '-o: getsubopt(3)' Switch */

				/*
				 * Display appropriate error message, if any.
				 */
				if (Duplicate == B_TRUE) {
					(void) pfmt(stderr, MM_WARNING,
						":10:%s option specified more than once.\n",
						DuplicateMsg);
					Duplicate = B_FALSE;
				}
				
				if (Conflict == B_TRUE) {
					(void) pfmt(stderr, MM_ERROR,
						":11:%s options are mutually exclusive.\n",
						ConflictMsg);
					Conflict = B_FALSE;
					Error = B_TRUE;
				}
			}
			break;
		}
		case '?': {
			/*
			 * Unfortunately, there is no way to distinguish between
			 * an invalid option and a valid option that is
			 * missing its argument.
			 */
			(void) pfmt(stderr, MM_ERROR,
				":82:Invalid option: -%1s\n", (char *) &optopt);
			Error = B_TRUE;
			break;
		}
		} /* 'getopt(3)' Switch */

		/*
		 * Display appropriate error message, if any.
		 */
		if (Duplicate == B_TRUE) {
			(void) pfmt(stderr, MM_WARNING,
				":10:%s option specified more than once.\n", DuplicateMsg);
			Duplicate = B_FALSE;
		}
		
		if (Conflict == B_TRUE) {
			(void) pfmt(stderr, MM_WARNING,
				":11:%s options are mutually exclusive.\n", ConflictMsg);
			Conflict = B_FALSE;
			Error = B_TRUE;
		}
	}

	return(Error == B_FALSE ? RET_OK : RET_ERR); 
}



/*
 * Parse the option string to see if the first token is
 * one of the XCDR-specific options.
 */
STATIC int
ParseXcdrOpt(OptString)
char	*OptString;
{
	char	*OptVal;					/* Value associated with option		*/

	boolean_t	Conflict;				/* Conflict between options 		*/
	char	*ConflictMsg;				/* Conflict error message			*/

	boolean_t Duplicate; 				/* Same option specified twice		*/
	char	*DuplicateMsg;				/* Duplicate error message			*/

	/*
	 * Get the first token from the option string and compare it
	 * with all of the XCDR-specific options.  If the token is
	 * recognized then the token is parsed.  Unrecognized tokens
	 * cause a return status other than an Error or OK.
	 */
	Conflict = B_FALSE;
	ConflictMsg = NULL;
	Duplicate = B_FALSE;
	DuplicateMsg = NULL;
	switch (getsubopt(&OptString, XcdrOptList, &OptVal)) {
		case UID_INDX: {
			/*
			 * Define a default User ID.
			 */
			if (Uid == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Default User ID'";
			} else {
				Uid = B_TRUE;
				UidVal = OptVal;
			}
			break;
		}
		case GID_INDX: {
			/*
			 * Define a default Group ID.
			 */
			if (Gid == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Default Group ID'";
			} else {
				Gid = B_TRUE;
				GidVal = OptVal;
			}
			break;
		}
		case FILEPERM_INDX: {
			/*
			 * Set the default file permissions.
			 */
			if (FilePerm == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Default File Permission'";
			} else {
				FilePerm = B_TRUE;
				FilePermVal = OptVal;
			}
			break;
		}
		case DIRPERM_INDX: {
			/*
			 * Set the default directory permissions.
			 */
			if (DirPerm == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Default Directory Permission'";
			} else {
				DirPerm = B_TRUE;
				DirPermVal = OptVal;
			}
			break;
		}
		case UIDMAP_INDX: {
			/*
			 * Process the User ID mapping defined in the specified file.
			 */
			if (UidMap == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'User ID Mapping'";
			} else {
				UidMap = B_TRUE;
				UidMapVal = OptVal;
			}
			break;
		}
		case GIDMAP_INDX: {
			/*
			 * Process the Group ID mapping defined in the specified file.
			 */
			if (GidMap == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Group ID Mapping'";
			} else {
				GidMap = B_TRUE;
				GidMapVal = OptVal;
			}
			break;
		}
		case NAMECONV_INDX: {
			/*
			 * Set the file name comversion method.
			 */
			if (NameConv == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'File Name Conversion Mode'";
			} else {
				NameConv = B_TRUE;
				NameConvVal = OptVal;
			}
			break;
		}
		case DIRSEARCH_INDX: {
			/*
			 * Set the directory search method.
			 */
			if (DirSearch == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Directory Search Mode'";
			} else {
				DirSearch = B_TRUE;
				DirSearchVal = OptVal;
			}
			break;
		}
		default: {
			/*
			 * Unrecognized XCDR-specific option.
			 * Note: Don't displayed an error message because the
			 * option may belong to another extension.
			 */
			return(RET_OTHER);
		}
	}

	/*
	 * Display appropriate error message, if any.
	 */
	if (Duplicate == B_TRUE) {
		(void) pfmt(stderr, MM_WARNING,
			":10:%s option specified more than once.\n", DuplicateMsg);
	}
	
	if (Conflict == B_TRUE) {
		(void) pfmt(stderr, MM_WARNING,
			":11:%s options are mutually exclusive.\n", ConflictMsg);
		return(RET_ERR);
	}

	return(RET_OK); 
}




/*
 * Parse the option string to see if the first token is
 * one of the RRIP-specific options.
 */
STATIC int
ParseRripOpt(OptString)
char	*OptString;
{
	char	*OptVal;				/* Value associated with token		*/

	boolean_t	Conflict;				/* Conflict between options 		*/
	char	*ConflictMsg;			/* Conflict error message			*/

	boolean_t Duplicate; 				/* Same option specified twice		*/
	char	*DuplicateMsg;			/* Duplicate error message			*/

	/*
	 * Get the first token from the option string and compare it
	 * with all of the RRIP-specific options.  If the token is
	 * recognized then the token is parsed.  Unrecognized tokens
	 * cause a return status other than an Error or OK.
	 */
	Conflict = B_FALSE;
	ConflictMsg = NULL;
	Duplicate = B_FALSE;
	DuplicateMsg = NULL;
	switch (getsubopt(&OptString, RripOptList, &OptVal)) {
		case DEVMAP_INDX: {
			/*
			 * Use the specified Device Node mapping.
			 */
			if (DevMap == B_TRUE) {
				Duplicate = B_TRUE;
				DuplicateMsg = "'Default User ID'";
			} else {
				DevMap = B_TRUE;
				DevMapVal = OptVal;
			}
			break;
		}
		default: {
			/*
			 * Unrecognized RRIP-specific option.
			 * Note: Don't display a message since this option
			 * may belong to another set of options.
			 */
			return(RET_OTHER);
		}
	}

	/*
	 * Display appropriate error message, if any.
	 */
	if (Duplicate == B_TRUE) {
		(void) pfmt(stderr, MM_WARNING,
			":10:%s option specified more than once.\n", DuplicateMsg);
	}
	
	if (Conflict == B_TRUE) {
		(void) pfmt(stderr, MM_WARNING,
			":11:%s options are mutually exclusive.\n", ConflictMsg);
		return(RET_ERR);
	}

	return(RET_OK); 
}




/*
 * Build the Mount table Entry option-list string.
 * The option-list string will be NULL terminated and
 * will not overflow the buffer.
 */
STATIC int
BldOpts(Buf, BufLen)
char	Buf[];
u_int	BufLen;
{
	char	*BufPtr;
	char	*BufEnd;
	int		RetVal;

	BufPtr = &Buf[0];
	BufEnd = &Buf[BufLen-1];
	*BufEnd = '\0';

	if (ReadOnly == B_TRUE) {
		(void) strncpy(BufPtr, "ro", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	} else {
		(void) strncpy(BufPtr, "rw", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (NoSetUid == B_TRUE) {
		(void) strncpy(BufPtr, ",nosuid", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	} else {
		(void) strncpy(BufPtr, ",suid", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (Remount == B_TRUE) {
		(void) strncpy(BufPtr, ",remount", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	/*
	 * "No Extend" precludes ALL extensions including SUSP and RRIP.
	 * "No SUSP" precludes RRIP.
	 * When SUSP is enabled, "No Extend" will NOT be enabled.
	 * When RRIP is enabled, SUSP will also be enabled.
	 */
	if (NoExtend == B_TRUE) {
		(void) strncpy(BufPtr, ",noextend", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	} else {
		if (NoSusp) {
			(void) strncpy(BufPtr, ",nosusp,norrip", (u_int)(BufEnd-BufPtr));
			BufPtr += strlen(BufPtr);
		} else {
			(void) strncpy(BufPtr, ",susp", (u_int)(BufEnd-BufPtr));
			BufPtr += strlen(BufPtr);
			if (NoRrip) {
				(void) strncpy(BufPtr, ",norrip", (u_int)(BufEnd-BufPtr));
				BufPtr += strlen(BufPtr);
			} else {
				(void) strncpy(BufPtr, ",rrip", (u_int)(BufEnd-BufPtr));
				BufPtr += strlen(BufPtr);
			}
		}
	}

	RetVal = BldXcdrOpts(BufPtr, (u_int)(BufEnd-BufPtr)+1);
	if (RetVal != RET_OK) {
		return(RetVal);
	}
	BufPtr += strlen(BufPtr);

	RetVal = BldRripOpts(BufPtr, (u_int)(BufEnd-BufPtr)+1);
	if (RetVal != RET_OK) {
		return(RetVal);
	}
	BufPtr += strlen(BufPtr);

	if (BufPtr >= BufEnd) {
		(void) pfmt(stderr, MM_ERROR,
			":18:Buffer overflow while building option list!\n");
		return(RET_ERR);
	}

	return(RET_OK);
}




/*
 * Build the Mount table Entry option-list string for
 * the XCDR-specific options.  The option-list string will
 * be NULL terminated and will not overflow the buffer.
 */
STATIC int
BldXcdrOpts(Buf, BufLen)
char	*Buf;
u_int	BufLen;
{
	char	*BufPtr;
	char	*BufEnd;

	BufPtr = &Buf[0];
	BufEnd = &Buf[BufLen-1];
	*BufEnd = '\0';

	if (Uid == B_TRUE) {
		(void) strncpy(BufPtr, ",uid=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, UidVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (Gid == B_TRUE) {
		(void) strncpy(BufPtr, ",gid=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, GidVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (FilePerm == B_TRUE) {
		(void) strncpy(BufPtr, ",fperm=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, FilePermVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (DirPerm == B_TRUE) {
		(void) strncpy(BufPtr, ",dperm=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, DirPermVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (UidMap == B_TRUE) {
		(void) strncpy(BufPtr, ",uidmap=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, UidMapVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (GidMap == B_TRUE) {
		(void) strncpy(BufPtr, ",gidmap=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, GidMapVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (NameConv == B_TRUE) {
		(void) strncpy(BufPtr, ",nmconv=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, NameConvVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (DirSearch == B_TRUE) {
		(void) strncpy(BufPtr, ",dsearch=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, DirSearchVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (BufPtr >= BufEnd) {
		(void) pfmt(stderr, MM_ERROR,
			":21:Buffer overflow while building XCDR option list!\n");
		return(RET_ERR);
	}

	return(RET_OK);
}




/*
 * Build the Mount table Entry option-list string for
 * the RRIP-specific options.  The option-list string will
 * be NULL terminated and will not overflow the buffer.
 */
STATIC int
BldRripOpts(Buf, BufLen)
char	*Buf;
u_int	BufLen;
{
	char	*BufPtr;
	char	*BufEnd;

	BufPtr = &Buf[0];
	BufEnd = &Buf[BufLen-1];
	*BufEnd = '\0';

	if (DevMap == B_TRUE) {
		(void) strncpy(BufPtr, ",devmap=", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, DevMapVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (BufPtr >= BufEnd) {
		(void) pfmt(stderr, MM_ERROR,
			":19:Buffer overflow while building RRIP option list!\n");
		return(RET_ERR);
	}

	return(RET_OK);
}




/*
 * Build the command string to invoke the program that implements
 * the XCDR options.
 */
STATIC int
BldXcdrCmd(MntPnt, Buf, BufLen)
char	*MntPnt;
char	*Buf;
u_int	BufLen;
{
	char	*BufPtr;
	char	*BufEnd;
	char	*CmdNameEnd;

	BufPtr = &Buf[0];
	BufEnd = &Buf[BufLen-1];
	*BufEnd = '\0';

	(void) strncpy(BufPtr, XcdrCmdName, (u_int)(BufEnd-BufPtr));
	BufPtr += strlen(BufPtr);
	CmdNameEnd = BufPtr;

	if (Uid == B_TRUE) {
		(void) strncpy(BufPtr, " -u ", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, UidVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (Gid == B_TRUE) {
		(void) strncpy(BufPtr, " -g ", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, GidVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (FilePerm == B_TRUE) {
		(void) strncpy(BufPtr, " -F ", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, FilePermVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (DirPerm == B_TRUE) {
		(void) strncpy(BufPtr, " -D ", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, DirPermVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (UidMap == B_TRUE) {
		(void) strncpy(BufPtr, " -U ", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, UidMapVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (GidMap == B_TRUE) {
		(void) strncpy(BufPtr, " -G ", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, GidMapVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (NameConv == B_TRUE) {
		(void) strncpy(BufPtr, " -", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, NameConvVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	if (DirSearch == B_TRUE) {
		(void) strncpy(BufPtr, " -", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, DirSearchVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	/*
	 * Verify that there is at least on XCDR argument was specified.
	 * If so, then append the mount-point to the command string.
	 * Otherwise, there is no need to invoke the command, so just
	 * return a NULL command string.
	 */
	if (BufPtr > CmdNameEnd) {
		*(BufPtr++) = ' ';
		(void) strncpy(BufPtr, MntPnt, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	} else {
		BufPtr = &Buf[0];
		*BufPtr = '\0';
	}

	if (BufPtr >= BufEnd) {
		(void)pfmt(stderr, MM_ERROR,
			":22:Buffer overflow while building XCDR command string!\n");
		return(RET_ERR);
	}

	*BufPtr = '\0';
	return(RET_OK);
}




/*
 * Build the command string to invoke the RRIP command that
 * implements the RRIP-specific options.
 */
STATIC int
BldRripCmd(MntPnt, Buf, BufLen)
char	*MntPnt;
char	*Buf;
u_int	BufLen;
{
	char	*BufPtr;
	char	*BufEnd;
	char	*CmdNameEnd;

	BufPtr = &Buf[0];
	BufEnd = &Buf[BufLen-1];
	*BufEnd = '\0';

	(void) strncpy(BufPtr, RripCmdName, (u_int)(BufEnd-BufPtr));
	BufPtr += strlen(BufPtr);
	CmdNameEnd = BufPtr;

	if (DevMap == B_TRUE) {
		(void) strncpy(BufPtr, " -c -m ", (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
		(void) strncpy(BufPtr, DevMapVal, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	}

	/*
	 * Verify that there is at least on RRIP argument was specified.
	 * If so, then append the mount-point to the command string.
	 * Otherwise, there is no need to invoke the command, so just
	 * return a NULL command string.
	 */
	if (BufPtr > CmdNameEnd) {
		*(BufPtr++) = ' ';
		(void) strncpy(BufPtr, MntPnt, (u_int)(BufEnd-BufPtr));
		BufPtr += strlen(BufPtr);
	} else {
		BufPtr = &Buf[0];
		*BufPtr = '\0';
	}

	if (BufPtr >= BufEnd) {
		(void)pfmt(stderr, MM_ERROR,
			":20:Buffer overflow while building RRIP command string!\n");
		return(RET_ERR);
	}

	*BufPtr = '\0';
	return(RET_OK);
}




/*
 * Display the error message corresponding to the error
 * returned by the mount(2) system call.
 */
STATIC void
DispMntErr(DevNode, MntPnt)
char	*DevNode;
char	*MntPnt;
{
	switch (errno) {
	case EPERM: {
		(void) pfmt(stderr, MM_ERROR, ":96:Must be a privileged user.\n");
		break;
	}
	case ENXIO: {
		(void) pfmt(stderr, MM_ERROR, ":99:No such device: %s\n", DevNode);
		break;
	}
	case ENOTDIR: {
		(void) pfmt(stderr, MM_ERROR,
			":8:%s is not a directory\n\tor a component of %s is not a directory.\n", MntPnt, DevNode);
		break;
	}
	case ENOENT: {
		(void) pfmt(stderr, MM_ERROR,
			":12:%s or %s:  No such file or directory,\n\tor no previous mount was performed.\n", DevNode, MntPnt);
		break;
	}
	case EINVAL: {
		(void) pfmt(stderr, MM_ERROR,
			":7:%s is not a cdfs file system.\n", DevNode);
		break;
	}
	case EBUSY: {
		(void) pfmt(stderr, MM_ERROR,
			":4:%s is already mounted, %s is busy,\n\tor allowable number of mount points exceeded.\n", DevNode, MntPnt);
		break;
	}
	case ENOTBLK: {
		(void) pfmt(stderr, MM_ERROR, ":6:%s is not a block device.\n",
			DevNode);
		break;
	}
	case EROFS: {
		(void) pfmt(stderr, MM_ERROR, ":9:%s is write-protected.\n", DevNode);
		break;
	}
	case ENOSPC: {
		(void) pfmt(stderr, MM_ERROR,
			":5:%s is corrupted and needs checking.\n", DevNode);
		break;
	}
	default: {
		(void) pfmt(stderr, MM_ERROR, ":30:Cannot mount: %s\n", DevNode);
		Msg = strerror (errno);
		(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
	}
	} /* Switch */
}

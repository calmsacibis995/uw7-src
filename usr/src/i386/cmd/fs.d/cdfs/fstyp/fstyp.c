/*	Copyright (c) 1991, 1992  Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION CONFIDENTIAL INFORMATION	*/

/*	This software is supplied to USL under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)cdfs.cmds:i386/cmd/fs.d/cdfs/fstyp/fstyp.c	1.8"
#ident	"$Header$"

static char fstyp_copyright[] = "Copyright 1991,1992 Intel Corp. 469254";

/* Tabstops: 4 */

#include	<errno.h>
#include	<fcntl.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/fs/cdfs_fs.h>
#include	<sys/fs/cdfs_inode.h>
#include	<sys/fs/cdfs_ioctl.h>

/*
 * Keep 'lint' from complaining about not using 'static'.
 */
#ifdef lint
#define STATIC static
#else
#define STATIC
#endif



STATIC char		*ProgName;					/* Progam Name (Global access)	*/
STATIC char		*Msg;						/* Error message string ptr		*/

extern int	optind;							/* 'argv' index of cmd-line opt	*/

STATIC void	DispUsage();



/*
 * Examine the specified device to see if it
 * contains a recognizable CDFS file-system.
 */
main(argc,argv)
int	argc;
char	*argv[];
{
	char			*DevNode;				/* Name of Dev Node to check	*/
	int				Dev;					/* File Descr. of Dev Node		*/

	uint_t			SectSize;				/* Log Sect size of CDFS file-sys*/
	char			*Buf;					/* Temp buffer to hold PVD		*/
	enum cdfs_type	FsType;					/* CDFS file-system type		*/

	int				Opt;					/* Command-line options			*/
	boolean_t 		Verbose;				/* Flag for '-v' option			*/
	boolean_t		Error;					/* Option parsing error flag	*/

	uchar_t			string[MAXNAMELEN];		/* Temporary string				*/
	int				RetVal;					/* Ret value of called procs	*/

	ProgName = argv[0];

	/*
	 * Setup the error message stuff.
	 */
	(void) setlocale(LC_ALL, "");
	(void) sprintf((char *) string, "UX:");
	(void) strcat((char *) string, ProgName);
	(void) setlabel((char *) string);
	(void) setcat("uxcdfs");

	/*
	 * Process the command-line arguments.
	 */
	Verbose = B_FALSE;
	Error = B_FALSE;
	while ((Opt = getopt(argc, argv, "v")) != EOF) {
		switch (Opt) {
			case 'v': {
				Verbose = B_TRUE;
				break;
			}
			default: {
				Error = B_TRUE;
				(void) pfmt(stderr, MM_ERROR, ":82:Invalid option: -%1s\n",
					argv[optind-1]);
			}
		}
	}

	/*
	 * There should be exactly one more argument which
	 * specifies the device to be examined.
	 */
	if (optind == argc-1) {
		DevNode = argv[optind];

	} else if (optind < argc-1) {
		(void) pfmt(stderr, MM_ERROR,
			":61:Extraneous argument(s) starting with: %s\n",
			argv[optind+1]);
		Error = B_TRUE;

	} else if (optind > argc-1) {
		(void) pfmt(stderr, MM_ERROR,
			":94:Missing special file argument.\n");
		Error = B_TRUE;
	}

	if (Error == B_TRUE) {
		DispUsage();
		exit(1);
	}

	/*
	 * Open the device.
	 */
	Dev = open(DevNode, O_RDONLY);
	if (Dev < 0) {
		(void) pfmt(stderr, MM_ERROR, ":31:Unable to open device: %s\n",
					DevNode);
		Msg = strerror(errno);
		(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
		exit(1);
	}

	/*
	 * Determine the logical sector size of the media.
	 * Note: No error message is displayed since, we don't want
	 * an error message from each fstyp() command that failed.
	 */
	RetVal = cdfs_GetSectSize(Dev, &SectSize); 
	if (RetVal != 0) {
		if (RetVal > 0) {
			(void) pfmt(stderr, MM_ERROR, ":125:Unable to access media.\n");
			Msg = strerror(errno);
			(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
		}
		(void)close(Dev);
		exit(1);
	}

	/*
	 * Allocate a buffer to hold the contents of a logical sector.
	 * This is large enough to store the PVD.
	 */
	Buf = malloc(SectSize);
	if (Buf == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":135:Unable to allocate %d bytes of memory.\n", SectSize);
		Msg = strerror(errno);
		(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
		(void)close(Dev);
		exit(1);
	}
		
	/*
	 * Read the PVD (super block) from the device.
	 */
	RetVal = cdfs_ReadPvd(Dev, Buf, SectSize, NULL, &FsType);
	if (RetVal != 0) {
		if (RetVal > 0) {
			(void) pfmt(stderr, MM_ERROR, ":125:Unable to access media.\n");
			Msg = strerror(errno);
			(void) pfmt(stderr, MM_ERROR, ":13:%s\n", Msg);
		}
		free(Buf);
		(void)close(Dev);
		exit(1);
	}

	/*
	 * The PVD was found, so display the CDFS ID String.
	 */
	switch (FsType) {
		case CDFS_ISO_9660: {
			Msg = gettxt (":2", "%s (ISO-9660 format)\n");
			(void) printf (Msg, CDFS_ID);
			break;
		}
		case CDFS_HIGH_SIERRA: {
			Msg = gettxt (":1", "%s (High-Sierra format)\n");
			(void) printf (Msg, CDFS_ID);
			break;
		}
		default: {
			Msg = gettxt (":3", "%s (Unrecognized format)\n");
			(void) printf (Msg, CDFS_ID);
			break;
		}
	}
		
	/*
	 * Display the contents of the PVD if the user requested it.
 	 */
	if (Verbose == B_TRUE) {
		cdfs_DispPvd(Buf, FsType);
	}

	free(Buf);
	(void)close(Dev);
	exit(0);
}



/*
 * Display command usage.
 */
STATIC void
DispUsage()
{
	(void) pfmt (stderr, MM_ACTION,
		":142:Usage: %s [-v] special\n", ProgName);
}

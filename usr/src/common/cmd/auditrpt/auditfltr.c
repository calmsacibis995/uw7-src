#ident	"@(#)auditfltr.c	1.2"
#ident "$Header$"
/*
 * Command: auditfltr
 * Inheritable Privileges: None
 *       Fixed Privileges: None
 * Level:	USER_PUBLIC
 *
 * Usage:	cat filename | auditfltr [[[-iN] [-oX]] | [-iX -oN]]
 * 			-i type = specifies the type of input
 * 			-o type = specifies the type of output
 *
 * Notes:	Auditfltr reads from stdin and writes to stdout.
 *              Auditfltr translates an audit log from machine dependent
 *              format to external data representation (XDR) and vice-versa.
 *		Output of command is in data format therefore stdout must 
 *              be redirected to a file or into to a pipe.
 *              Auditfltr is to be run in maintenance mode when ES is installed.
 */

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <string.h>
#include <mac.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include "auditrpt.h"

#define PROGLEN	128	/* length of version specific program name */

#define MSG_USAGE	 ":80:usage: auditfltr [[-iN] [-oX]] | [-iX -oN]\n"
#define FDR1		 ":81:conversion type %s is not supported\n"
#define FDR2 		 ":82:invalid combination of conversion types\n"
#define FDR3 		 ":83:input file is in invalid format\n"

int	char_spec;	/* is file character special? */	

/* 
 *  static variables
 */
static level_t	mylvl, audlvl;
static ushort	cur_spec = 0;	/* current active log special character file? */
static char adt_ver[ADT_VERLEN];/* logfile version number */

/*
 * external functions
 */
extern int getopt();
extern int optind;   /*getopt()*/
extern char *optarg; /*getopt()*/

/*
 * local functions
 */
static void	umsg(),
		check_opts();

static int 	getlog1(),
		openlog(),
		get_magic();


/*
 * Procedure:	main
 *
 * Privileges: lvlproc:   P_SETPLEVEL
 *             auditbuf:  P_AUDIT
 *             auditevt:  P_AUDIT
 *
 *
 * Restrictions:	none
 */

main(argc, argv)
int argc;
char **argv;
{
        struct stat status;
	int outopt, inopt;
	char log_byord[ADT_BYORDLEN];
	char program[PROGLEN];	/*version specific program name*/
	int c;

        /* initialize locale, message label, and catalog information for pfmt */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:auditfltr");
	(void)setcat("uxaudit");

	inopt = outopt = NULL;

	/* Parse the command line */
	while ((c = getopt(argc,argv,"i:o:")) != EOF) {
		switch (c) {
			case 'i':
				if (((*optarg=='X') ||
				     (*optarg == 'N')) &&
				     (strlen(optarg) == 1))
					inopt=*optarg;
				else {
	                       		(void)pfmt(stderr,MM_ERROR,FDR1,optarg);
	        			(void)pfmt(stderr,MM_ACTION,MSG_USAGE);
					exit(ADT_BADSYN);
				}
				break;
			case 'o':
				if (((*optarg=='X') ||
				     (*optarg == 'N')) &&
				     (strlen(optarg) == 1))
					outopt=*optarg;
				else {
	                       		(void)pfmt(stderr,MM_ERROR,FDR1,optarg);
	        			(void)pfmt(stderr,MM_ACTION,MSG_USAGE);
					exit(ADT_BADSYN);
				}
				break;
			case '?':
	                       	(void)pfmt(stderr,MM_ACTION,MSG_USAGE);
				exit(ADT_BADSYN);
		} /* switch */
	} /* while */

	/* Invalid argument combinations: -iX -oX, -iN -oN, -oN, -iX */
	if ((outopt == 'N') && ((inopt == 'N') || (inopt == NULL))) {
        	(void)pfmt(stderr,MM_ERROR,FDR2);
                (void)pfmt(stderr,MM_ACTION,MSG_USAGE);
		exit(ADT_BADSYN);
	}

	if ((inopt == 'X') && ((outopt == 'X') || (outopt == NULL))) {
                (void)pfmt(stderr,MM_ERROR,FDR2);
                (void)pfmt(stderr,MM_ACTION,MSG_USAGE);
		exit(ADT_BADSYN);
	}

	/* The command line is invalid if it contains an argument */
	if (optind < argc) {
        	(void)pfmt(stderr,MM_ACTION,MSG_USAGE);
		exit(ADT_BADSYN);
	}

	/*
	 * Input for auditfltr is stdin, therefore the input could be a
	 * special character device. Value used in adt_getrec().
	 */
	char_spec = 1;

	/*
	 * When an fread is done on stdin, it will fill it's buffer. 
	 * This buffer will be lost when the version specific program
	 * is exec'ed. 
	 * So, turn off buffering.
	 */
	setbuf(stdin, NULL);

	if ((inopt == 'N') || (inopt == NULL)) {
		/* Translate audit log file to XDR */

		/*
		 * Read the first field containing the audit magic number.
		 * If the format of the log file is not in sync with the
		 * requested conversion the process terminates.
		 */
		get_magic(stdin, ADT_BYORD, log_byord); 

	} else {
		/* Translate audit log file to machine dependent format */

		/*
		 * Read the first field containing the audit magic number.
		 * If the format of the log file is not in sync with the
		 * requested conversion the process terminates.
		 */
		get_magic(stdin, ADT_XDR_BYORD, log_byord);
	}
	/*
	 * Read the second field, version number, from the log file
	 * This field identifies the version number of the log file:
	 * 1.0, 2.0, 3.0 or 4.0.
	 * Note that for versions 1.0, 2.0, 3.0 this field will be NULL.
	 */
	(void)memset(adt_ver, NULL, ADT_VERLEN);
        if (fread(adt_ver, 1, ADT_VERLEN, stdin) != ADT_VERLEN) {
		(void)pfmt(stderr, MM_ERROR, E_NO_VER);
		exit(ADT_FMERR);
	}

	/*
	 * set correct path for dependent command,
	 * versions 1.0, 2.0, and 3.0 are equal, so set to version 1.0
	 * Versions 1.0, 2.0, and 3.0 use the first 16 bytes for the
	 * magic number, but only the first 8 bytes should be used
	 * so the version number should be NULL.
	 * 
	 */
	if ((strcmp(adt_ver, V1) == 0) || (strcmp(adt_ver, V2) == 0) ||
            (strcmp(adt_ver, V3) == 0) || (strcmp(adt_ver, NULL) == 0))
		strcpy(program, V1FLTR);
	else if (strcmp(adt_ver, V4) == 0)
		strcpy(program, V4FLTR);
	else {
		(void)pfmt(stderr, MM_ERROR, E_BAD_VER);
		exit(ADT_FMERR);
	     }

	if ((stat(program, &status)) == -1){
		(void)pfmt(stderr, MM_ERROR, E_NOVERSPECF, program);
		exit(ADT_BADEXEC);
	}
	if ((status.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0){
		(void)pfmt(stderr, MM_ERROR, E_RPTNOTXF, program);
		exit(ADT_BADEXEC);
	}

	/*
	 * Exec version specific auditfltr with original command line.
	 * Note: argv[0] is not changed to the version specific program,
	 * because the program would display a command line different than
	 * what was entered by the user.
	 */
	execvp(program, argv);
} /*end of main*/

/*
 * This routine is called to read the first field of the audit log
 * file. This field identifies the format or byte order of the log file:
 * ADT_3B2_BYORD, ADT_386_BYORD, or ADT_XDR_BYORD.
 * If this field is not an expected value, an error message is displayed and
 * processing ceases.
 */
int
get_magic(fp, byte_order, log_byord)
FILE *fp;
char *byte_order;
char *log_byord;
{
        if (fread(log_byord, 1, ADT_BYORDLEN, fp) != ADT_BYORDLEN) {
		(void)pfmt(stderr, MM_ERROR, E_FMERR);
		exit(ADT_FMERR);
		
	}

	/*
	 * Auditfltr: The -i"type of input" must match the byte
	 * ordering of the log file.
	 */

	if (strcmp(log_byord, byte_order) != 0) {
		(void)pfmt(stderr, MM_ERROR, FDR3);
		exit(ADT_BADARCH);
	}
	return(0);
}

#ident	"@(#)auditrpt.c	1.3"
#ident "$Header$"
/*
 * Command: auditrpt
 * Inheritable Privileges: P_AUDIT, P_SETPLEVEL
 *	 Fixed Privileges: None
 * Notes: determine the version of the audit log file and exec the version
 *        specific auditrpt
 *
 * usage:
 * auditrpt [-o] [-i] [-b | -w] [-x] [-e [!]event[,...]] [-u user[,...]]
 *	[-f object_id[,...]] [-t object_type[,...]] [-p all | priv[,...]]
 *	[-l level| -r levelmin-levelmax] [-s time] [-h time]
 *	[-a outcome] [-m map] [-v subtype] [log [...]]
 *
 *	This is the auditrpt control program, which determines the
 *	appropriate auditrpt version specific program to be exec'ed
 *	With the log_file_name, the audit trail data is taken from the
 *	specified file rather than from the default (current) log file.
 *
 * options:
 *	-o print the union of all options requested
 *	-i log file is taken from standard input
 *	-b backward display
 *	-w follow on
 *	-x display light weight process id (only valid for version 4.0)
 *	-e event type(s)
 *	-u user name/uid list
 *	-f object name list
 *	-t object type list
 *	-p events using specified privilege(s)
 *	-l specifies security level to report
 *	-r specifies security level range to report
 *	-s events existing at or after time interval
 *	-h events existing at or before time interval
 *	-a display successful (s) or failed (f) events only
 *	-m audit map  directory
 *	-v miscellaneous subtype
 *
 * level:	resides at SYS_PRIVATE, runs at SYS_AUDIT
 *
 * files:	none
 *
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
	int optmask = 0;
	int a_on;
	short	no_audit = 0;
	char log[MAXPATHLEN+1];
        int     ret;            /* return values from routines */
	char program[PROGLEN];	/*version specific program name*/
	int c, i;

        /* initialize locale, message label, and catalog information for pfmt */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:auditrpt");
	(void)setcat("uxaudit");

        /* make the process exempt */
	if (auditevt(ANAUDIT, NULL, sizeof(aevt_t)) == -1){
		if (errno == ENOPKG)
                        no_audit = 1;
		else
                    	if (errno == EPERM) {
                                (void)pfmt(stderr, MM_ERROR, E_PERM);
                                exit(ADT_NOPERM);
                        }
	}

        /* get the current level of this process */
	if (lvlproc(MAC_GET, &mylvl) == 0){
		/* set level if not SYS_AUDIT */
		if (lvlin(SYS_AUDIT, &audlvl) == -1) {
			(void)pfmt(stderr, MM_ERROR, E_LVLOPER, "lvlin", errno);
			exit(ADT_LVLOPER);
		}
		if (lvlequal(&audlvl, &mylvl) == 0){
			/* SET level if not SYS_AUDIT */
			if (lvlproc(MAC_SET, &audlvl) == -1) {
                                if (errno == EPERM) {
                                        (void)pfmt(stderr, MM_ERROR, E_PERM);
                                        exit(ADT_NOPERM);
                                }
                                (void)pfmt(stderr, MM_ERROR, E_LVLOPER, "lvlproc", errno);
                                exit(ADT_LVLOPER);
			}
		}
	}else {
		if (errno != ENOPKG){
			(void)pfmt(stderr, MM_ERROR, E_LVLOPER, "lvlproc", errno);
			exit(ADT_LVLOPER);
		}
	}

	/* parse the command line */
	while ((c = getopt(argc, argv, "?owbixz:a:e:r:m:u:f:s:h:t:l:p:v:")) != EOF) {
		switch (c) {
			case 'x':	/* only valid for version 4.0 */
				optmask |= X_MASK;
				break;
			case 'i':	/* read from stdin */
				optmask |= I_MASK;
				break;
			case 'w':	/* read from active log file */
				optmask |= W_MASK;
				break;
			case '?':
				umsg();
				exit(ADT_BADSYN);
				break;
		}/* switch */
	}

	/* 
	 * if the -w option was specified or no audit log files were specified,
	 * auditing must be installed in the current machine, otherwise
	 * exit with ENOPKG error code.
	 */
	if ( no_audit && ((W_MASK & optmask) || (strlen(argv[optind]) == 0))){
		(void)pfmt(stderr, MM_ERROR, RE_NOPKG);
		exit(ADT_NOPKG);
	}

	(void)memset(log, NULL, MAXPATHLEN+1);
	/* get current log file if auditing is on */
	if (!no_audit) {
		if (getlog1(log) != -1){
			a_on = 1;		/*auditing is on*/
		}
		else
			a_on = 0;
	}
	else
		a_on = 0;

        /*
         * process log files
	 */
	/* log file obtained from stdin */
	if (I_MASK & optmask){			/* -i specified */
		/*
		 * When an fread is done on stdin, it will fill it's buffer. 
		 * This buffer will be lost when the version specific program
		 * is exec'ed. 
		 * So, turn off buffering.
		 */
		setbuf(stdin, NULL);
		if ((ret=(openlog(STD_IN))) != 0)
                        exit(ret);
	}
        /* if no logs specified	 or -w specified, use current active log */
	else if ((argv[optind] && (strlen(argv[optind]) == 0)) || (W_MASK & optmask)){
		if (a_on){
                        if (cur_spec){	/* current active log character special? */
                                (void)pfmt(stdout, MM_WARNING, RW_SPEC);
                                exit(ADT_NOLOG);
                        }
                        if ((ret=(openlog(log)))!=0){
                                exit(ret);
                        }
		} else {
			(void)pfmt(stderr, MM_ERROR, RE_NOT_ON);
			exit(ADT_BADSYN);
		}
	} else {				/* log file(s) specified */
		for (i = optind; i < argc; i++){
			/* current active log character special? */
			if ((strcmp(argv[i], log) == 0) && (cur_spec))
				;
			else {
				if ((ret = openlog(argv[i])) == 0) {
					break;
				}
				else if (ret != ADT_NOLOG)
					exit(ret);
			}
		}
		/*
		 * If no log file could be opened, call the version 4.0
		 * program, which will display appropriate warning and error
		 * messages.
		 */
		if (i == argc){
			strcpy(adt_ver, V4);
		}
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
             (strcmp(adt_ver, V3) == 0) || (strcmp(adt_ver, VNULL) == 0))
		strcpy(program, V1RPT);
	else if (strcmp(adt_ver, V4) == 0)
		strcpy(program, V4RPT);
	     else {
		(void)pfmt(stderr, MM_ERROR, E_BAD_VER);
		exit(ADT_FMERR);
	     }

	/* check for version specific options */
	check_opts(optmask);

	if ((stat(program, &status)) == -1){
		(void)pfmt(stderr, MM_ERROR, E_NOVERSPEC, program);
		exit(ADT_BADEXEC);
	}
	if ((status.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0){
		(void)pfmt(stderr, MM_ERROR, E_RPTNOTX, program);
		exit(ADT_BADEXEC);
	}
	/*
	 * Exec version specific auditrpt with original command line.
	 * Note: argv[0] is not changed to the version specific program,
	 * because the program would display a command line different than
	 * what was entered by the user.
	 */
	execvp(program, argv);
} /*end of main*/





/*
 * Procedure:	getlog1
 *
 * Privileges:	auditctl:  P_AUDIT
 *		auditlog:  P_AUDIT
 *
 * Restrictions:	none
 *
 * Get the current log name from the auditctl structure.
 */
int
getlog1(logname)
char	*logname;
{
	actl_t	actl;		/* auditctl(2) structure */
	alog_t 	alog;		/* auditlog(2) structure */
	char 	seq[ADT_SEQSZ+1];

	if (auditctl(ASTATUS, &actl, sizeof(actl_t))) {
		(void)pfmt(stderr, MM_ERROR, E_BADSTAT);
		exit(ADT_BADSTAT);
	}
	if (!actl.auditon)
		return(-1);
	/* allocate space for the ppathp, apathp, progp in alog struct */
	(void)memset(&alog, NULL, sizeof(alog_t));
	if (((alog.ppathp=(char *)malloc(MAXPATHLEN+1))==NULL)
	  ||((alog.apathp=(char *)malloc(MAXPATHLEN+1))==NULL)
	  ||((alog.progp=(char *)malloc(MAXPATHLEN+1))==NULL)){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		exit(ADT_MALLOC);
	}
	(void)memset(alog.ppathp, NULL, MAXPATHLEN+1);
	(void)memset(alog.apathp, NULL, MAXPATHLEN+1);
	(void)memset(alog.progp, NULL, MAXPATHLEN+1);

	/* get current log file attributes */
	if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, E_BADLGET);
		exit(ADT_BADLGET);
	}
	if (alog.flags & PPATH) 
		(void)strcat(logname, alog.ppathp);
	else
		(void)strcat(logname, ADT_DEFPATH);
	if (alog.flags & PSPECIAL){
		cur_spec=1;
		return(0);
	}
	(void)strcat(logname, "/");
	(void)strncat(logname, alog.mmp, ADT_DATESZ);
	(void)strncat(logname, alog.ddp, ADT_DATESZ);
	(void)sprintf(seq, "%03d", alog.seqnum);
	(void)strncat(logname, seq, ADT_SEQSZ);
	if (alog.pnodep[0] != '\0')
		(void)strcat(logname, alog.pnodep);
	return(0);
}



/*
 * Open audit log file for reading, and read the
 * magic number and version number.
 */

int
openlog(filename)
char    *filename;      /* log file's name */
{
        struct stat status;
        int ret;
        char log_byord[ADT_BYORDLEN];

        if (strcmp(filename, STD_IN) != 0){
                if (access(filename, F_OK) != 0) {
                        return(ADT_NOLOG);
                }
                if ((stat(filename, &status)) == -1){
                        (void)pfmt(stderr, MM_ERROR, E_FMERR);
                        return(ADT_FMERR);
                }
                if ((status.st_mode & S_IFMT) == S_IFCHR){
                        char_spec=1;    /* this is a character special file */
                }
                else{
                        if ((status.st_mode & S_IFMT) == S_IFREG)
                                char_spec=0;
                        else {
                                return(ADT_NOLOG);
                        }
                }
                if ((freopen(filename, "r", stdin))==NULL) {
                        (void)pfmt(stderr, MM_ERROR, E_FMERR);
                        return(ADT_FMERR);
                }

        }
        else
                char_spec = 1;  /* if stdin, might be character special */

        /* read first field (magic number identifying byte order) */
        (void)memset(log_byord, NULL, ADT_BYORDLEN);
        if ((ret=get_magic(stdin, ADT_BYORD, log_byord)) != 0){
                if (ret == ADT_BADARCH){
                        /* log file's format or byte order can't be processed */
                       (void)pfmt(stderr, MM_ERROR, E_BAD_ARCH, log_byord);
                }
                return(ret);
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
        return(0);
}


/*
 * This routine is called to read the first field of the audit log
 * file. This field identifies the format or byte order of the log file:
 * ADT_3B2_BYORD, ADT_386_BYORD, or ADT_XDR_BYORD.
 * An error code is returned if this field is not an expected value.
 */
int
get_magic(fp, byte_order, log_byord)
FILE *fp;
char *byte_order;
char *log_byord;
{
        if (fread(log_byord, 1, ADT_BYORDLEN, fp) != ADT_BYORDLEN) {
		(void)pfmt(stderr, MM_ERROR, E_FMERR);
		return(ADT_FMERR);
		
	}

	/*
	 * Auditrpt: The log file byte ordering must match the
	 * the residing machine's byte ordering.
	 */

	if (strcmp(log_byord, byte_order) != 0)
		return(ADT_BADARCH);

	return(0);
}

/*
 * Print usage message.
 */
void
umsg()
{
        (void)pfmt(stderr, MM_ACTION, USAGE1);
        (void)pfmt(stderr, MM_NOSTD, V4USAGE2);
        (void)pfmt(stderr, MM_NOSTD, USAGE3);
        (void)pfmt(stderr, MM_NOSTD, USAGE4);
        (void)pfmt(stderr, MM_NOSTD, USAGE5);
}

/*
 * Check for options that are not common to all versions
 * of the auditrpt command.
 */
void
check_opts(optmask)
int optmask;
{
	/*-x option is not valid for versions 1.0, 2.0 or 3.0*/
         if ((X_MASK & optmask) && ((strcmp(adt_ver, "1.0") == 0) ||
	     (strcmp(adt_ver, "2.0") == 0) || (strcmp(adt_ver, "3.0") == 0))){
		(void)pfmt(stderr, MM_ERROR, E_BAD_XOP);
		exit(ADT_BADSYN);
	}
}

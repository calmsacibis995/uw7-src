/*		copyright	"%c%" 	*/

#ident	"@(#)auditrptv4.c	1.2"
#ident        "@(#)auditrptv4.c	1.5"
#ident  "$Header$"

/*
 * Command: auditrptv4
 * Inheritable Privileges: P_AUDIT, P_SETPLEVEL
 *       Fixed Privileges: None
 * Notes: displays recorded audit information
 *
 * usage:
 * auditrpt [-o] [-i] [-b | -w] [-x] [-e [!]event[,...]] [-u user[,...]] 
 *	[-f object_id[,...]] [-t object_type[,...]] [-p all | priv[,...]]
 *	[-l level| -r levelmin-levelmax] [-s time] [-h time] 
 *	[-a outcome] [-m map] [-v subtype] [log [...]]
 *	
 *	With no arguments, all audited events from the log file are displayed.
 *	With the log_file_name, the audit trail data is taken from the specified
 *	file rather than from the default (current) log file.
 *	Without the -o option, the intersection of the criteria selection 
 *	options will be displayed.
 *
 * options:
 *	-o print the union of all options requested
 *	-i log file is taken from standard input
 *	-b backward display
 * 	-w follow on 
 *	-x light weight process ID
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
 * level:  	resides at SYS_PRIVATE, runs at SYS_AUDIT
 *
 * files:	/var/audit/auditmap/auditmap
 *		/var/audit/auditmap/lid.internal
 *		/var/audit/auditmap/ltf.alias
 *		/var/audit/auditmap/ltf.cat
 *		/var/audit/auditmap/ltf.class
 *		/var/audit/MMDD###
 * 
 */

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <sys/vnode.h>
#include <sys/param.h>
#include <string.h>
#include <sys/privilege.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <mac.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include "auditrptv4.h"

/* 
 * external variables
 */
extern int	char_spec;	/* is file character special? */	
extern int 	free_size;	/* size of free-format data */
save_t 	*fn_beginp,		/* beginning of filename records list */
	*fn_lastp;		/* end of filename records list */

char 	*lid_fp, 		/* LTDB filenames */
	*class_fp,	
	*ctg_fp, 
	*alias_fp;

uint 	uidtab[MXUID], 		/* -u<uid> option table */
	ipctab[MAXOBJ]; 	/* -f<ipc object> option table */

char 	*usertab[MXUID], 	/* -u<logname> option table */
	*objtab[MAXOBJ]; 	/* -f<filename> option table */

int 	objnum, 		/* option tables' counters */
	ipcnum, 		
	uidnum, 
	usernum;

ids_t 	*uidbegin, 		/* begin of uid map */
	*gidbegin,   		/* begin of gid map */
	*typebegin, 		/* begin of event type map */
	*privbegin, 		/* begin of privelege map */
	*scallbegin;		/* begin of system call map */
cls_t	*clsbegin;  		/* begin of class map */

int     s_user, 		/* no. of users in map */
	s_grp, 			/* no. of groups in map */
	s_class,		/* no. of classes in map */
	s_type, 		/* no. of event types in map */
	s_priv,			/* no. of privileges in map */
	s_scall;		/* no. of system calls in map */

int	mac_installed;		/* MAC installed in the audited system? */

char	*mach_info;		/* env and mach_info hold map id info */
env_info_t	env;		
int 	optmask;

/* 
 * static variables
 */
static int	type_mask = 0;		/* set to object types specified with -t */
static long 	s_time, h_time;		/* time specified with -s/-h */
static uint 	uidtab2[MXUID];
static int 	uidnum2 = 0;
static int 	match = 0;
static pvec_t 	pmask = 0;
static int 	option = 0;   
static FILE 	*bkfp = NULL;
static int	bad_ltdb = 0;
static char 	*tname;			/* temporary file name for -b option */
static ushort	cur_spec = 0;		/* current active log special character file? */
static char 	subtypep[SUBTYPEMAX + 2];
static level_t	lvlno;
static level_t	lvlmin, lvlmax;
static level_t	mylvl, audlvl;
static adtemask_t	rpt_emask;
static adtemask_t	zrpt_emask;


/* 
 * external functions
 */
extern int	adt_loadmap();		/* from adt_loadmap.c */

extern void	setcred(); 		/* from adt_cred.c */

extern int	getcred(),		/* from adt_cred.c */
		freecred();

extern int	parse_hour(), 		/* from adt_optparse.c */
		parse_range(), 			
		parse_lvl(), 		
		parse_priv(),	
		parse_obj(), 
		parse_objt(),
		parse_uid();

extern int	evtparse();		/* from adt_evtparse.c */
extern int	zevtparse();		/* from adt_evtparse.c */

extern int	adt_getrec(), 		/* from adt_getrec.c */
		getspec();

extern void	printrec();		/* from adt_print.c */

/* 
 * local functions
 */
static void 	chck_order(), 
		filenmfnc(), 
		chk_pr_zevt(), 
		user_check(),
		check_recs(),
		ck_fn_dmp(),
		ck_cr_dmp(),
		ad_fn_lst(),
		part_recs();

static struct rec *ad_cr_lst();

static int 	check_ltdb(), 
		dologs(), 
		proc_rec(), 
		check_lvlfile(), 
		check_events(), 
		backward(), 
		nexteol(),
		get_fn_cnt(),
		getlog();

static char	*parsepath();

needcred_t	*cr_dmp_lp;	/* beginning of list of needcred entries */
unsigned long	cred_new;	/* c_crseqnum of new cred */
needfname_t	*fn_dmp_lp;	/* beginning of list of needfname entries */
unsigned long	fname_new;	/* sequence number  of new fname record */

/*
 * Procedure:	main
 *
 * Privileges:	lvlproc:   P_SETPLEVEL
 *		auditbuf:  P_AUDIT
 *		auditevt:  P_AUDIT
 *
 * Restrictions:	none
 */
main(argc, argv)
int	argc;
char	**argv;
{
	extern int optind;	
	extern char *optarg;
	int 	c;	
	int	lognum = 0;
	char  	*opt1, *opt2;
	char 	*eventsp, *zeventsp, *uidp, *objp, *objtp;
	char 	*lvlp, *rangep, *h_hourp, *s_hourp, *privp;
	char	*map;		/* pathname of auditmap file */
	int	ret;		/* return values from routines */
	int 	i, a_on;
	char	log[MAXPATHLEN+1];
	abuf_t	abuf;
	ushort	badlog = 0;
	struct  stat statpath;
	short	no_audit = 0;

	fn_beginp = fn_lastp = NULL;
	s_time = h_time = 0;
	lvlmin = lvlmax = 0;
	optmask = 0;
	cr_dmp_lp  = NULL;
	cred_new = 0;
	fn_dmp_lp = NULL;
	fname_new = 0;

	/* initialize locale, message label, and catalog information for pfmt */
	(void)setlocale(LC_ALL, "");
	(void)setlabel("UX:auditrpt");
	(void)setcat("uxaudit");

	/* make the process exempt */
	if (auditevt(ANAUDIT, NULL, sizeof(aevt_t)) == -1){
		if (errno == ENOPKG)
			no_audit = 1;
		else
			/* if error is EPERM, exit (otherwise it is EINVAL, continue) */
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
	} else
		if (errno != ENOPKG){ 
			(void)pfmt(stderr, MM_ERROR, E_LVLOPER, "lvlproc", errno);
			exit(ADT_LVLOPER);
		}

	/* print command line */
	(void)pfmt(stdout, MM_NOSTD, M_1);
	for (i = 0; i < argc; i++)
		(void)fprintf(stdout,"%s ", argv[i]);
	(void)fprintf(stdout,"\n");

	/* parse the command line  */
	while ((c = getopt(argc, argv, "?owbixz:a:e:r:m:u:f:s:h:t:l:p:v:")) != EOF) {
		switch (c) {
                        case 'x':
                                optmask |= X_MASK;
                                break;
			case 'b':
				optmask |= B_MASK;
				break;
			case 'o':
				optmask |= O_MASK;
				break;
			case 'w':
				optmask |= W_MASK;
				break;
			case 'm':
				if((map = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(map, optarg);
				optmask |= M_MASK;
				break;
			case 'i':
				optmask |= I_MASK;
				break;
			case 'e':
				if((eventsp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(eventsp, optarg);
				optmask |= E_MASK;
				option = 1;	/* criteria selection option */
				break;
			case 'u': 
				if((uidp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(uidp, optarg);
				optmask |= U_MASK;
				option = 1;
				break;
			case 'f':
				if((objp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(objp, optarg);
				optmask |= F_MASK;
				option = 1;
				break;
			case 't':
				if((objtp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(objtp, optarg);
				optmask |= T_MASK;
				option = 1;
				break;
			case 'l': 
				if((lvlp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(lvlp, optarg);
				optmask |= L_MASK;
				option = 1;
				break;
			case 'r': 
				if((rangep = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(rangep, optarg);
				optmask |= R_MASK;
				option = 1;
				break;
			case 'h': 
				if((h_hourp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(h_hourp, optarg);
				optmask |= H_MASK;
				option = 1;
				break;
			case 's': 
				if((s_hourp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(s_hourp, optarg);
				optmask |= S_MASK;
				option = 1;
				break;
			case 'a':
				if ((*optarg == 's') && (strlen(optarg)== 1)){
					/* if -a f was specified, undo it */
					if (AF_MASK & optmask) {
						optmask &= ~AF_MASK; 
					}
					optmask |= AS_MASK;
				
				}
				else if ((*optarg == 'f')&&(strlen(optarg)==1)) {
					if (AS_MASK & optmask) {
						optmask &= ~AS_MASK; 
					}
					optmask |= AF_MASK;
				} else {
					(void)pfmt(stderr, MM_ERROR, RE_BAD_OUT);
					exit(ADT_BADSYN);
				}
				option = 1;
				break;
			case 'p':
				if((privp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(privp, optarg);
				optmask |= P_MASK;
				option = 1;
				break;
			case 'v':
				(void)strncpy(subtypep, optarg,(SUBTYPEMAX + 1));
				optmask |= V_MASK;
				option = 1;
				break;
			case 'z':
				if((zeventsp = (char *)malloc(strlen(optarg)+1)) == NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(zeventsp, optarg);
				optmask |= Z_MASK;
				option =1;
				break;
			case '?':
				usage();
				exit(ADT_BADSYN);
				break;
		}/* switch */
	}/* while */

	/*
	 * validate options
 	 */

	/* -z option only valid with -i option*/
	if ((Z_MASK & optmask) && !((optmask == Z_MASK) || (optmask == (Z_MASK | I_MASK)))){
		usage();
		exit(ADT_BADSYN);
	}

	/* -o with no selection criteria options? */
	if ((O_MASK & optmask) && !(option)) {	
		(void)pfmt(stderr, MM_ERROR, RE_INCOMPLETE);
		usage();
		exit(ADT_BADSYN);
	}
	if ((L_MASK & optmask) && (R_MASK & optmask)) {
		opt1="-l";
		opt2="-r";
		(void)pfmt(stderr, MM_ERROR, RE_BAD_COMB, opt1, opt2);
		usage();
		exit(ADT_BADSYN);
	}
	if (I_MASK & optmask){			/* -i specified */
		/* if log file and -i specified */
		if (argv[optind] && (strlen(argv[optind]) != 0)){
			for( ; optind<argc; optind++) 
				(void)pfmt(stdout, MM_WARNING, RW_IGNORE, argv[optind]);
		}
	}
	if (M_MASK & optmask){
		/* determine if argument to -m is a readable directory */
		if (  (access(map, R_OK) != 0) 
		   || (stat(map,&statpath) != 0)
		   || ((statpath.st_mode & S_IFMT) != S_IFDIR))	{
			(void)pfmt(stderr, MM_ERROR, RE_BADDIR, map);
			exit(ADT_BADSYN);
		}
	}

	/* 
	 * if the -w option was specified or no audit log files were specified,
	 * auditing must be installed in the current machine, otherwise
	 * exit with ENOPKG error code.
	 */
	if ( no_audit && ((W_MASK & optmask) || !argv[optind] || (strlen(argv[optind]) == 0))){
		(void)pfmt(stderr, MM_ERROR, RE_NOPKG);
		exit(ADT_NOPKG);
	}

	/* get current log file if auditing is on */
	(void)memset(log, NULL, MAXPATHLEN+1);
	if (!no_audit) {
		if (getlog(log) != -1){
			a_on = 1;		/* auditing is on */
		} else
			a_on = 0;
	} else
		a_on = 0;

	if (W_MASK & optmask) {
		if (B_MASK &optmask){
			opt1="-b";
			opt2="-w";
			(void)pfmt(stderr, MM_ERROR, RE_BAD_COMB, opt1, opt2);
			usage();
			exit(ADT_BADSYN);
		}
		if (a_on){
			if (argv[optind] && (strlen(argv[optind]) != 0)) /* log file(s) specified */
				for( ; optind<argc; optind++)
					(void)pfmt(stdout, MM_WARNING, RW_IGNORE, argv[optind]);
		} else {
			(void)pfmt(stderr, MM_ERROR, RE_W_DISABLE);
			exit(ADT_BADSYN);
		}
		if (auditbuf(ABUFGET, &abuf, sizeof(abuf_t))) {
			(void)pfmt(stderr, MM_ERROR, E_BADBGET);
			exit(ADT_BADBGET);
		}
		if (abuf.vhigh !=0)
			(void)pfmt(stdout, MM_WARNING, RW_NO_VH0);
	}
	if (!(M_MASK & optmask)) {
		map = ADT_MAPDIR;
	}

	if ((ret = check_ltdb(map)) != 0) /* check accessability of ltdb files */
		exit(ret);
	if ((ret = adt_loadmap(map)) != 0) /* load the auditmap file */
		exit(ret);	

	if (E_MASK & optmask) {
		if (evtparse(eventsp, rpt_emask) != 0) 
			exit(ADT_BADSYN);	
	}
	if (U_MASK & optmask) {
		if (parse_uid(uidp) != 0) 
			exit(ADT_BADSYN);
		user_check();
	} 
	if (F_MASK & optmask) {
		if ((ret = parse_obj(objp)) != 0) 
			exit(ret);
	}
	if (T_MASK & optmask) {
		if (parse_objt(objtp,&type_mask) != 0) 
			exit(ADT_BADSYN);
	}
	if (L_MASK & optmask) {
		if ((lvlno = parse_lvl(lvlp)) == -1) 
			exit(ADT_BADSYN);
	}
	if (R_MASK & optmask) {
		if (parse_range(rangep,&lvlmin,&lvlmax) != 0) 
			exit(ADT_BADSYN);
	}
	if (H_MASK & optmask) {
		if (parse_hour(h_hourp,&h_time) != 0) 
			exit(ADT_BADSYN);
	}
	if (S_MASK & optmask) {
		if (parse_hour(s_hourp,&s_time) != 0) 
			exit(ADT_BADSYN);
	}
	if ((S_MASK & optmask) && (H_MASK & optmask) && !(O_MASK & optmask))
		/* - o must be specified if start time is after end time */
			if (s_time > h_time){
				(void)pfmt(stderr, MM_ERROR, RE_BAD_TIME);
				exit(ADT_BADSYN);
			}
	if (P_MASK & optmask) {
		if ((pmask = parse_priv(privp)) == -1)
		 /*-1 is returned when invalid priv specified,
		 otherwise, priv vector will be returned */
			exit(ADT_BADSYN);
	}

	if (Z_MASK & optmask) {
		if (zevtparse(zeventsp, zrpt_emask) != 0) 
			exit(ADT_BADSYN);	
		
	}

	/*
	 * process log files
  	 */
	if (I_MASK & optmask){			/* -i specified */
		if ((ret = (dologs(STD_IN))) != 0)
			exit(ret);
	}
	/* if no logs specified  or -w specified, use current active log*/
	else if ( !argv[optind] || (strlen(argv[optind])==0) || (W_MASK & optmask)){
		if (a_on){
			if (cur_spec){	/* current active log character special? */
				(void)pfmt(stdout, MM_WARNING, RW_SPEC);
				adt_exit(ADT_NOLOG);
			}
			if ((ret = (dologs(log)))!=0){ 
				adt_exit(ret);
			}
		} else {
			(void)pfmt(stderr, MM_ERROR, RE_NOT_ON);
			adt_exit(ADT_BADSYN);
		}
	} else {				/* log file(s) specified */
		for ( ; optind < argc; optind++){
			/* current active log character special? */
			if((strcmp(argv[optind], log) == 0) && (cur_spec))
				(void)pfmt(stdout, MM_WARNING, RW_SPEC);
			else{
				if ((ret = dologs(argv[optind]))==0) {
					lognum++;
				}
				else if (ret == ADT_NOLOG)
					badlog++;
			 	else
					adt_exit(ret);
			}
		}
		if (lognum == 0) {
			(void)pfmt(stderr, MM_ERROR, E_NO_LOGS);
			adt_exit(ADT_NOLOG);
		}
	}
	/* if no match was found in  log files */
	if ((option > 0) && (match == 0))
		(void)pfmt(stdout, MM_WARNING, RW_NO_MATCH);
	if (badlog)
		adt_exit(ADT_INVARG);
	else {
		/* process partial records, missing cred or fname info */
		part_recs();
		adt_exit(ADT_SUCCESS);
	}
/* NOTREACHED */
}/* end of main*/

/*
 * Procedure:	getlog
 *
 * Privileges:	auditctl:  P_AUDIT
 *		auditlog:  P_AUDIT
 *
 * Restrictions:	none
 *
 * Get the current log name from the auditlog structure.
 */
int
getlog(logname)
char	*logname;
{
	actl_t	actl;		/* auditctl(2) structure */
	alog_t 	alog;		/* auditlog(2) structure */
	char 	seq[ADT_SEQSZ+1];

	if (auditctl(ASTATUS, &actl, sizeof(actl_t))) {
		(void)pfmt(stderr, MM_ERROR, E_BADSTAT);
		adt_exit(ADT_BADSTAT);
	}
	if (!actl.auditon)
		return(-1);
	/* allocate space for the ppathp, apathp, progp in alog struct*/
	(void)memset(&alog, NULL, sizeof(alog_t));
	if (((alog.ppathp = (char *)malloc(MAXPATHLEN+1)) == NULL)
	  ||((alog.apathp = (char *)malloc(MAXPATHLEN+1)) == NULL)
	  ||((alog.progp = (char *)malloc(MAXPATHLEN+1)) == NULL)){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		adt_exit(ADT_MALLOC);
	}
	(void)memset(alog.ppathp, NULL, MAXPATHLEN+1);
	(void)memset(alog.apathp, NULL, MAXPATHLEN+1);
	(void)memset(alog.progp, NULL, MAXPATHLEN+1);

	/* get current log file attributes */
	if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, E_BADLGET);
		adt_exit(ADT_BADLGET);
	}
	if (alog.flags & PPATH) 
		(void)strcat(logname, alog.ppathp);
	else
		(void)strcat(logname, ADT_DEFPATH);
	if (alog.flags & PSPECIAL){
		cur_spec = 1;
		goto out;
	}
	(void)strcat(logname,"/");
	(void)strncat(logname, alog.mmp, ADT_DATESZ);
	(void)strncat(logname, alog.ddp, ADT_DATESZ);
	(void)sprintf(seq,"%03d", alog.seqnum);
	(void)strncat(logname, seq, ADT_SEQSZ);
	if (alog.pnodep[0] != '\0')
		(void)strcat(logname, alog.pnodep);
out:
	free(alog.ppathp);
	free(alog.apathp);
	free(alog.progp);
	return(0);
}

/*
 * Open audit log file for reading.  Call the proc_rec routine to
 * process one record at a time.
 */
int
dologs(filename)
char 	*filename;	/* log file's name */
{
	struct stat status;
	int ret;

	if (strcmp(filename, STD_IN) != 0){
		if (access(filename, F_OK) != 0) {
			(void)pfmt(stdout, MM_WARNING, RW_NO_LOG, filename);
			return(ADT_NOLOG);
		}
		if ((stat(filename, &status)) == -1){
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		if ((status.st_mode & S_IFMT) == S_IFCHR){
			char_spec = 1;	/* this is a character special file */
		} else {
			if ((status.st_mode & S_IFMT) == S_IFREG)
				char_spec = 0;
			else {
				(void)pfmt(stdout, MM_WARNING, RW_IGNORE, filename);
				return(ADT_NOLOG);
			}
		}
		if ((freopen(filename,"r", stdin)) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
	} else
		char_spec = 1;	/* if stdin, might be character special */

	/* if -b specified, open temp file to write output */
	if (optmask & B_MASK) {
		if ((tname = tempnam(P_tmpdir,"AD")) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		if ((bkfp = fopen(tname, "w+r+")) == NULL){
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		/* change mode of temporary file before writing to it */
		if (chmod(tname, S_IRUSR|S_IWUSR) != 0){
			(void)pfmt(stderr, MM_ERROR, E_CHMOD, errno);
			return(ADT_FMERR);
		}
	}
	/*
	 * Initialize the version number.
	 */
	(void)memset(adt_ver, NULL, ADT_VERLEN);
	(void)strcpy(adt_ver, V4);

	while ((ret = proc_rec(stdin, filename)) <= 0){
		/* a magic number and version number have been read, or a record has been read */
		if (ret == GOTNEWMV || ret == 0)
			continue;
		/* if -w specified, ignore EOF and continue */
		else if (optmask & W_MASK) {
                		(void)sleep(1);
				continue;
		} else 
			break;
	}
	if (ret > 0)
		return(ret);

	/* if -b specified, print the records saved in file (bkfp) in reverse */
	if (optmask & B_MASK) {
		(void)fprintf(stdout,"\n");
		(void)fflush(stdout);
		(void)fflush(bkfp);
		ret = backward();
		(void)fclose(bkfp);
		(void)unlink(tname);
		tname = NULL;
	}
	return(0);
}

/*
 * Process the next record from audit log file using the following logic:
 *	-call the adt_getrec routine to fill the cmn, spec and freeformat
 * 	 parts of the dumpp record (see dumprec_t in auditrptv4.h). 
 *	-if this is a filename record, save the filename information in
 *	 list of filenames
 *	-else if this is a credential record, save information of the new 
 *	 credential in the list of credential blocks
 *	-else if any other kind of record (event record):
 *		-get the credential block for this record (fill the credential
 *		 part of the dumpp record)
 *		-determine if the record is post-selected
 *		-display the record if post-selected
 */
int
proc_rec(log_fp, filename)
FILE		*log_fp;
char		*filename;
{
	static int	first_time = 1;	/* denote 1st time in this function */
	int 		rtype, ret, cred_found, fname_found;
	int		recnum;		/* num. of filename records dumped for this event */
	static dumprec_t	*dumpp;
	struct id_r	*ip;
	ulong_t *crseqnump;
	int i;
	cred_found = rtype = 0;
	/*
	 * if this is the first time in this function,
	 * allocate the dumprec structure 
	 */
	if (first_time){
		first_time = 0;
		if ((dumpp = (dumprec_t *)malloc(sizeof(struct rec))) == NULL){
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
	}
	/* get the next record */
	if ((ret = adt_getrec(log_fp, dumpp)) != 0){
		return (ret);
	}
	rtype = dumpp->r_rtype;	
	switch (rtype) {
		case  FILEID_R :
			ip = (struct id_r *)&dumpp->spec_data;
			if (ip->i_flags & ADT_MAC_INSTALLED){
				mac_installed = 1;
				if (bad_ltdb)
					(void)pfmt(stdout, MM_WARNING, RW_BADLTDB);
			} else
				mac_installed = 0;
			ip->i_mmp[ADT_DATESZ] = '\0';
			ip->i_ddp[ADT_DATESZ] = '\0';
			(void)pfmt(stdout, MM_NOSTD, M_2, ip->i_mmp, ip->i_ddp);
			(void)pfmt(stdout, MM_NOSTD, M_3, dumpp->cmn.c_seqnum);
			(void)pfmt(stdout, MM_NOSTD, M_4, adt_ver);
			(void)pfmt(stdout, MM_NOSTD, M_5, dumpp->freeformat);
			(void)fflush(stdout);
			if ((strcmp(mach_info, dumpp->freeformat)) != 0) {
				(void)pfmt(stdout, MM_WARNING, RW_MISMATCH, filename, dumpp->freeformat, mach_info);
				(void)fflush(stderr);
			}
			chck_order(ip, dumpp->cmn.c_seqnum);
			break;
		case FNAME_R : 	
			/* add this record to linked list of filename records */
			filenmfnc(dumpp);
			/* if a dumpp record needs an fname */
			if (fn_dmp_lp)
				fname_new = EXTRACTSEQ(dumpp->cmn.c_seqnum);
			break;
		case CRFREE_R :
			crseqnump = (ulong_t *)dumpp->freeformat;
			/* free the credential structure for each credential specified in the freeformat */

			for (i = 0; i < dumpp->spec_data.r_credf.cr_ncrseqnum; i++, crseqnump++){
#ifdef DEBUG
				/* Print on stdout so record order is shown */
                		(void)printf("\nDEBUG: FREE CREDENTIAL NUMBER == %ld\n",*crseqnump);
#endif
				freecred(*crseqnump);
			}
			break;
		case ZMISC_R :
			/* records for trusted applications*/
			/* if new cred add to cred list */
			if(dumpp->cmn.c_crseqnum == 0){
				setcred(dumpp);
				/* if a dumpp record needs a cred */
				if (cr_dmp_lp)
					cred_new = dumpp->cred.cr_crseqnum;
			}
			(void)chk_pr_zevt(dumpp);
			break;
		default :
			/*
			 * if the crseqnum is 0, this record contains
			 * credential information, and it should be
			 * added to the credential list.
			 */
			if(dumpp->cmn.c_crseqnum == 0){
#ifdef DEBUG
	printf("DEBUG: NEW CRED ENCOUNTERED == %d\n", dumpp->cred.cr_crseqnum);
#endif
				setcred(dumpp);
				cred_found = 1;
				/* if a dumpp record needs a cred */
				if (cr_dmp_lp)
					cred_new = dumpp->cred.cr_crseqnum;
			} else {
				/* get cred info */
				if ((getcred(dumpp)) == -1){
#ifdef DEBUG
					(void)fprintf(stderr, "DEBUG: Process information for P%d is incomplete\n", dumpp->cmn.c_pid);
					(void)fprintf(stderr, "DEBUG: Credential information for C%d is missing\n", dumpp->cmn.c_crseqnum);
#endif
					dumpp = ad_cr_lst(dumpp);	/* put dumprec in list and get new dumprec */
					cred_found = 0; /* no cred info found */
				} else {
					cred_found = 1;
				}
			}
			/* if cred info was found, check for fname info */
			if (cred_found == 1){
				/*
				 * Set fname_found to true at this point. If the event has no
				 * fname records it will remain true. If all fname records
				 * are not available, fname_found will be set to false.
				 */
				fname_found = 1;
				/*
			 	 * Make sure all fname records associated with this event are available.
			 	 *
			 	 * EXTRACTREC gives the number of records dumped for the event ==
			 	 * number of filename records + 1 event record.
			 	 */
				recnum = EXTRACTREC(dumpp->cmn.c_seqnum) - 1;
				/* Check if dumpp record has fname records associated with it.*/
				if (recnum > 1){
					/* if all fname records not available, put in dump_fname list */
					if (!get_fn_cnt(dumpp)) {
						fname_found = 0;	/* all fname recs not found */
						ad_fn_lst(dumpp);	/* put dumprec in fname dumprec list */
						/* allocate a new dumprec structure */
						if ((dumpp = (dumprec_t *)malloc(sizeof(struct rec))) == NULL){
							(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
							adt_exit(ADT_MALLOC);
						}
					}
				}
			}
			if (cred_found && fname_found){
				/* If this record meets the selection criteria, print it. */
				check_recs(dumpp, cred_found);
				break;
			}
	}
	/* did the last record read have a new cred? */
	if (cred_new){
		ck_cr_dmp();	/* see if any need_cred records need this cred */
		cred_new = 0;
	} else {
		/* was the last record read an fname record? */
		if (fname_new){
			ck_fn_dmp();	/* see if any need_fname records need this fname */
			fname_new = 0;
		}
	}
	return(0);
}

/*
 * Check if the specified uid or logname is valid.  These are validated
 * against the auditmap.
 */
void
user_check()
{
	int i, ii;
	int found;
	ids_t *uidtbl;

	if (uidnum > 0) {
		for (i = 0; i<uidnum; i++) {
			found = 0;
			for (ii = 0, uidtbl = uidbegin; ii < s_user; ii++) {
				if(uidtab[i] == uidtbl->id) {
					found = 1;
					break;
				} else
					uidtbl++;
			}
			if (!found)
				(void)pfmt(stdout, MM_WARNING, RW_NO_USR1, uidtab[i]);
	        }
	}
	if (usernum > 0) {
		for (i = 0; i < usernum; i++) {
			found = 0;
			for (ii = 0, uidtbl = uidbegin; ii < s_user; ii++) {
				if(strcmp(usertab[i], uidtbl->name)== 0) {
					uidtab2[uidnum2++] = uidtbl->id;
					found = 1;
					break;
				} else
					uidtbl++;
			}
			if (!found)
				(void)pfmt(stdout, MM_WARNING, RW_NO_USR2, usertab[i]);
	        }
	}
}

/*
 * Determine if the record pointed to by d is selectable (i.e. was 
 * post-selected with criteria selection options).
 */
int
check_events(d)
char *d;
{
	int		infomask = 0;
	int		i, obj, ftypemask;
	dumprec_t	*dumpp;
	cmnrec_t	*cmnp;
	struct cred_rec	*credp;
	int		*ipcp;
	struct save	*scan;
	char		*freep;
	char		typetemp[SUBTYPEMAX + 2];
	char		*token;
	int		seqnum;
	pvec_t		priv_vec;


	dumpp = (dumprec_t *)d;
	cmnp = &dumpp->cmn;
	credp = &dumpp->cred;
	ipcp = (int *)&dumpp->d_spec;
	freep = dumpp->freeformat;

	/*
	 * The options -b, -w, -m, -i, and -x must be accompanied by
  	 * criteria selection options for this routine to be called.
  	 */
	if (optmask & X_MASK) {
		infomask |= X_MASK; 
	}
	if (optmask & B_MASK) {
		infomask |= B_MASK; 
	}
	else	/* -b and -w are mutually exclusive */
		if (optmask & W_MASK) {
			infomask |= W_MASK; 
	}
	if (optmask & M_MASK) {
		infomask |= M_MASK; 
	}
	if (optmask & I_MASK) {
		infomask |= I_MASK; 
	}
	if (optmask & E_MASK) {
		if (EVENTCHK(cmnp->c_event, rpt_emask)) {
			if (O_MASK & optmask)
				return(1);
			else
				infomask |= E_MASK; 
		}
	}
	if (optmask & F_MASK ) { 
		/*
		 * A user entered object id may refer to an IPC object or a
		 * loadable module. The numbers assigned to IPC objects and
		 * loadable modules are not mutually exclusive, therefore
		 * additional records may be displayed.
		 */
		if ((cmnp->c_rtype == IPCS_R) || (cmnp->c_rtype == IPCACL_R) ||
		    (cmnp->c_rtype == MODLOAD_R)) {
			if (ipcnum > 0) {
				for (i = 0; i< ipcnum; i++) {
					/*
					 * The first member of the ipc_r, ipcacl_r,
					 * and modload_r structures is the id
					 * of the object.
					 */
					if (ipctab[i] == *ipcp) {
						if (O_MASK & optmask)
							return(1);
						else
							infomask |= F_MASK; 
					}

				}
			}
		}

		if (has_path(cmnp->c_rtype, cmnp->c_event)){
			seqnum = EXTRACTSEQ(cmnp->c_seqnum);
			for (scan = fn_beginp; scan != NULL; scan = scan->next){
				if (scan->seqnum == seqnum) {
					for (obj = 0; obj < objnum; obj++) {
						if (strcmp(objtab[obj], scan->name) == 0){
							if (O_MASK & optmask)
								return(1);
							else
								infomask |= F_MASK; 
						}
					}
				}
			}
		}
	}

	if (optmask & T_MASK) 
	{
		ftypemask = 0;
	 	if (has_path(cmnp->c_rtype, cmnp->c_event)){
			seqnum = EXTRACTSEQ(cmnp->c_seqnum);
			for (scan = fn_beginp; scan != NULL; scan = scan->next){
				if (scan->seqnum == seqnum) {
					switch(scan->rec.f_vnode.v_type) {
						case 1:
							if (type_mask & REG)
								ftypemask |= REG;
							break;
						case 2:
							if (type_mask & DIR)
								ftypemask |= DIR;
							break;
						case 3:
							if (type_mask & BLOCK)
								ftypemask |= BLOCK;
							break;
						case 4:
							if (type_mask & CHAR)
								ftypemask |= CHAR;
							break;
						case 5:
							if (type_mask & LINK)
								ftypemask |= LINK;
							break;
						case 6:
							if (type_mask & PIPE)
								ftypemask |= PIPE;
							break;
						default:
							break;
					}/* switch */
				}
			}/* for */
		} else {
			/* ipc objects do not have filename records */
			if (type_mask & (SEMA | SHMEM | MSG)) {
				switch(cmnp->c_event) {
					case ADT_SEM_CTL:
					case ADT_SEM_GET:
					case ADT_SEM_OP:
							if (type_mask & SEMA)
								ftypemask |= SEMA;
							break;
					case ADT_SHM_CTL:
					case ADT_SHM_GET:
					case ADT_SHM_OP:
							if (type_mask & SHMEM)
								ftypemask |= SHMEM;
							break;
					case ADT_MSG_CTL:
					case ADT_MSG_GET:
					case ADT_MSG_OP:
							if (type_mask & MSG)
								ftypemask |= MSG;
							break;
					default:
							break;
				}/* switch */
			}					

		}
		
		if (ftypemask & type_mask) {
			if (O_MASK & optmask)
				return(1);
			else {
				infomask |= T_MASK; 
				}
		}
	}

	if ((optmask & L_MASK ) && has_path(cmnp->c_rtype, cmnp->c_event)){
		seqnum = EXTRACTSEQ(cmnp->c_seqnum);
		for (scan = fn_beginp; scan != NULL; scan = scan->next){
			if (scan->seqnum == seqnum) {
				if (scan->rec.f_vnode.v_lid == lvlno) {
		        		if (optmask & O_MASK)
						return(1);
					else
						infomask |= L_MASK;
				}
			}
		}
	} else {
	if ((optmask & R_MASK ) && has_path(cmnp->c_rtype, cmnp->c_event)){
		seqnum = EXTRACTSEQ(cmnp->c_seqnum);
		for (scan = fn_beginp; scan != NULL; scan = scan->next){
			if (scan->seqnum == seqnum) {
		   		/* is level within the range ? */
				if ((adt_lvldom(scan->rec.f_vnode.v_lid, lvlmin) > 0) && 
				(adt_lvldom(lvlmax, scan->rec.f_vnode.v_lid) > 0)) 
				{
		        		if (optmask & O_MASK)
						return(1);
					else
						infomask |= R_MASK;
				}
			}
		}
	}
	}
	if (optmask & U_MASK) {
		if (uidnum > 0){
			for ( i = 0; i<uidnum; i++){
				if (uidtab[i] == credp->cr_uid 
				   || uidtab[i] == credp->cr_ruid){
					if (optmask & O_MASK)
						return(1);
					else
						infomask |= U_MASK; 
				}
			}
		}
		if (usernum > 0){
			for ( i = 0; i<uidnum2; i++){
				if (uidtab2[i] == credp->cr_uid
				   || uidtab2[i] == credp->cr_ruid){
					if (optmask & O_MASK)
						return(1);
					else
						infomask |= U_MASK;
				} 
			}
		}
	} 
	/* if only s_time is specified */
	if ((optmask & S_MASK) && !(optmask & H_MASK)) {
		if (cmnp->c_time.tv_sec >= s_time) {
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= S_MASK;
		}
	}
	/* if only end time is specified */
	if ((optmask & H_MASK ) && !(optmask & S_MASK)) {
		if (cmnp->c_time.tv_sec <= h_time) {
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= H_MASK;
		}
	}
	/* if both end time and start time are specified */
	if ((optmask & H_MASK) && (optmask & S_MASK)) {
		if (optmask & O_MASK) {
			if ((cmnp->c_time.tv_sec >= s_time) || (cmnp->c_time.tv_sec <= h_time)) 
				return(1);
		} else {
			if ((cmnp->c_time.tv_sec >= s_time)&&(cmnp->c_time.tv_sec <= h_time)){ 
				infomask |= S_MASK;
				infomask |= H_MASK;
			}
		}
	} 
        if (optmask & AS_MASK) {
		if (cmnp->c_status == 0) {
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= AS_MASK;
		}
	}
        if (optmask & AF_MASK) {
		if (cmnp->c_status != 0) {
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= AF_MASK;
		}
	}
	if ((optmask & V_MASK) && (cmnp->c_event == ADT_MISC)){
		(void)strncpy(typetemp, freep, (SUBTYPEMAX+1));
		typetemp[SUBTYPEMAX]='\0';
		if (strchr(typetemp,':') == NULL){
			(void)pfmt(stdout, MM_WARNING, RW_MISC);
			token = typetemp;
		} else {
			token = strtok(typetemp,":");
		}
		if (!strncmp(token, subtypep, SUBTYPEMAX)){
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= V_MASK;
		}
	}
	/*
	 * All records may have privileges (c_rprivs), only check
	 * for a privilege match if event type is pm_denied.
	 */
	if ((optmask & P_MASK) && (cmnp->c_event == ADT_PM_DENIED)) {
		if (cmnp->c_rprivs > 0){
			/* convert from bit position to priv vector */
			priv_vec = adt_privbit(cmnp->c_rprivs);
			if (priv_vec & pmask) {
		 		if (O_MASK & optmask)
					return(1);
				else
					infomask |= P_MASK; 
			}
			
		}
	}

	if (optmask == infomask) 
		return(1);
	else
		return(0);
}

/* 
 * This function returns 1 if the record type rtype may have pathname 
 * records (FNAME_R) related to the same event.
 */
int
has_path(rtype, event)
ushort rtype;
ushort event;
{
	switch(rtype){
	case CHOWN_R : case CHMOD_R : case MAC_R :  case DEV_R :
	case MOUNT_R : case FPRIV_R : case ALOG_R : case FILE_R :
		return (1);
	case ACL_R :
		if (event == ADT_FILE_ACL)
			return (1);
		else
			return (0);		
	case TIME_R :
		if (event == ADT_CHG_TIMES)
			return (1);
		else
			return (0);
	case MODLOAD_R:
		if (event == ADT_MODLOAD)
			return (1);
		else
			return (0);
	default:
		return (0);
	}		
}

/*  
 * Parse a pathname and reduce its size  by eliminating '.'s and '..'s.
 * Only reduce if the path is absolute (begins with '/').
 *
 * Note:
 * We always resolve "dir1/dir2/.." to "dir1".  This would not be correct if
 * dir2 was a symbolic link, but we needn't concern ourselves with that,
 * since symbolic links never appear in the audit trail -- they are resolved
 * before the audit record is written.
 */
char *
parsepath(sp)
char * sp;
{
	register char *p, *src, *dest;

	/* if path is not absolute, return (don't reduce) */
	if (sp[0] != '/')
		return (sp);
	dest = sp;
	src = sp + 1;
	while(src) {
		if (p = strchr(src,'/'))
			*p++ = '\0';
		if ((src[0] == '\0') || ((src[0] == '.') && (src[1] == '\0')))
			/* EMPTY */
			;
		else if ((src[0] == '.') && (src[1] == '.') && src[2] == '\0') {
			if (dest != sp)
				while (*--dest != '/')
					;
		} else {
			*dest++ = '/';
			while (*src)
				*dest++ = *src++;
		}
		src = p;
	}
	if (dest == sp)
		dest++;
	*dest = '\0';
	return (sp);
}

/*
 * Display in reverse order the output of post-selected records saved in 
 * the file pointed to by bkfp.  This routine is called after processing
 * all the records if the -b option was specified.
 */
int
backward()
{
 	char 	buf[BUFSIZE];
	int 	total, letters, i, n;
	int	sav_letters = 0;
	long 	size,  start;
	char	save[BUFSIZE];
	int	sp, ep;
	
	save[0] = '\0';
	if ((size = ftell(bkfp)) == -1){
		(void)pfmt(stderr, MM_ERROR, E_FMERR);
		return(ADT_FMERR);
	}
	/* determine how many parts to divide the file into */
	n = size / BUFSIZE;
	/* if size divides exactly into BUFSIZE, consider one less part */
	if ((n * BUFSIZE) == size)
		n--;
	for(i = n; i >= 0; i--){
		start = BUFSIZE * i;
		if ((fseek(bkfp, start, SEEK_SET)) != 0){
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		total = fread(&buf[0], 1, BUFSIZE, bkfp);

		/*
		 * Decrement the number of bytes read by one because it
		 * is used to index into an array.
		 */
		if(buf[total - 1] != '\n'){
			sp = nexteol(buf, total - 1, &letters) + 1;
			(void)write(1, &buf[sp], letters);
			sp--;
		}
		else
			sp = total - 1;

		if (sav_letters != 0){
			(void)write(1, save, sav_letters);
			sav_letters = 0;
		}
		ep = sp;
		while(sp > 0){
			sp = nexteol(buf, (ep-1), &letters) + 1;
			if (sp < 1){
			/*
			 * Save the end of this line until the beginning
			 * is printed in the next loop. Note memcpy is
			 * used since there may be NULL characters in
			 * in the string.
			 */
				(void)memset(save, NULL, BUFSIZE);
				(void)memcpy(save, buf, letters + 1);
				sav_letters = letters + 1;
			} else
				(void)write(1, &buf[sp], letters + 1);
			ep = sp - 1;
		}
	}
	return(0);
}

/*
 * Return the position of the next end-of-line character from right
 * to left starting to search at position last.  Set letters to the
 * number of characters counted before the '\n'.
 */
int
nexteol(buf, last, letters)
char	*buf;
int	last;
int	*letters;
{
	int count;
	count = last; 
	*letters = 0;
	while ((buf[count] != '\n')&& (count>=0)) {
		count--;	
		(*letters)++;
	}
	return(count);
}
	
/*
 * Print usage message. 
 */
void
usage()
{
	(void)pfmt(stderr, MM_ACTION, USAGE1);
	(void)pfmt(stderr, MM_NOSTD, V4USAGE2);
	(void)pfmt(stderr, MM_NOSTD, USAGE3);
	(void)pfmt(stderr, MM_NOSTD, USAGE4);
	(void)pfmt(stderr, MM_NOSTD, USAGE5);
}

/*
 * Determine if the ltdb map files ~lid.internal and ~ltf.* are accessible.
 */
int
check_ltdb(mapdir)
char *mapdir;
{
	int	ret;

	if ((ret = check_lvlfile(mapdir, LDF, LID)) != 0)
		return(ret);
	if ((ret = check_lvlfile(mapdir, ALASF, ALAS)) !=0)
		return(ret);
	if ((ret = check_lvlfile(mapdir, CATF, CTG)) !=0)
		return(ret);
	if ((ret = check_lvlfile(mapdir, CLASSF, CLS)) !=0)
		return(ret);
	return(0);
}
/*
 * Construct the pathname of an audit map file based on the directory name
 * (possibly supplied by -m option) and the file name.  Return 1 if the
 * file is accessible, 0 otherwise.
 */
int
check_lvlfile(mapdir, fname, file)
char *mapdir;
char *fname;
int  file;
{
	int     msize;
	char 	*fullname;
	/* allocate space to hold pathname */
	msize = strlen(mapdir)+strlen(fname)+1;
	if ((fullname = ((char *)malloc(msize))) ==  (char *)NULL) {
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		return(ADT_MALLOC);
	}
	/* determine the full pathname of the file */
	(void)strcpy(fullname, mapdir);
	(void)strcat(fullname, fname);
	if (access(fullname, R_OK) != 0) {
		bad_ltdb++;
		return(0);
	}
	switch(file) {
		case LID:
			lid_fp = fullname;
			break;
		case ALAS:
			alias_fp = fullname;
			break;
		case CTG:
			ctg_fp = fullname;
			break;
		case CLS:
			class_fp = fullname;
			break;
	}
	return(0);
}
/*
 * Determine if the specified log files are not in sequence or if 
 * a file is missing.
 */
void
chck_order(ip, seq)
struct id_r *ip;
unsigned long seq;
{
	int date;
	static ulong lastseq = 0;	/* sequence number of last log file */
	static ulong lastdate = 0;	/* date of last log file */
	static int msg_printed = 0;	/* display error message only once */

	date = atoi(strcat(ip->i_mmp, ip->i_ddp));
	if ((lastseq) && !(msg_printed)){
		if ( ((date == lastdate) && (seq != (lastseq + 1)))
		     || (date < lastdate)
	             || ((date>lastdate) && (seq != 1)) ) {
				(void)pfmt(stdout, MM_WARNING, RW_OUTSEQ);
				msg_printed++;
		}
	}
	lastdate = date;
	lastseq = seq;
}
	
/*
 * This routine handles "pathname" records.  It builds a linked list of
 * pathnames and associates each pathname to an event based on the 
 * sequence number.
 */
void
filenmfnc(dumpp)
dumprec_t	*dumpp;
{
	char		*filenmp = NULL;

	/* add this record to linked list of filename records */
	if (fn_beginp == NULL){		/* list is empty */
		if ((fn_beginp = (save_t *)malloc(sizeof(save_t))) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		fn_lastp = fn_beginp;
	} else {
		if ((fn_lastp->next = (save_t *)malloc(sizeof(save_t))) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		fn_lastp = fn_lastp->next;
	}
	/*
	 * EXTRACTSEQ gives the sequence number of the event; all records
	 * for the same event have the same sequence number.
	 */
	fn_lastp->seqnum = EXTRACTSEQ(dumpp->spec_data.r_fname.f_seqnum);
	(void)memcpy(&(fn_lastp->rec),&dumpp->spec_data.r_fname, sizeof(struct fname_r));
	if (strlen(dumpp->freeformat) != 0){
		/* 
		 * don't reduce the path if it is relative, there
		 * can be paths above the working directory, e.g. ~/../..
		 */
		if (dumpp->freeformat[0] != '/')
			filenmp = dumpp->freeformat;
		else
			filenmp = parsepath(dumpp->freeformat);

		if ((fn_lastp->name = (char *)malloc(strlen(filenmp)+1))== NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		(void)strcpy(fn_lastp->name, filenmp);
	} else
		fn_lastp->name = NULL;

	fn_lastp->next = NULL;
}

/*
 * routine called upon exit
 */
void
adt_exit(status)
int status;
{
	if (bkfp != NULL)
		(void)unlink(tname);
	exit(status);
}

/*
 * print ZMISC_R records
 */
void
chk_pr_zevt(dumpp)
dumprec_t *dumpp;
{
	cmnrec_t *cmnp;
	int zsize;

	cmnp = &dumpp->cmn;
	if (EVENTCHK(cmnp->c_event, zrpt_emask))
	{
		match = 1;
		/* Size of a ZMISC_R record is: event + size + freeformat */
		zsize = sizeof(ushort) + sizeof(int) + free_size;
		if (write(1,&(cmnp->c_event), sizeof(ushort)) == -1)
		{
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			exit(ADT_FMERR);
		}
		if (write(1,&zsize, sizeof(int)) == -1)
		{
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			exit(ADT_FMERR);
		}
		if (dumpp->freeformat != NULL)
		{
			if (write(1, dumpp->freeformat, free_size) == -1)
			{
				(void)pfmt(stderr, MM_ERROR, E_FMERR);
				exit(ADT_FMERR);
			}
		}
	}
}

/*
 * Count the number of fname records in the fname list associated
 * with the event record. If all fname records are available return
 * one, otherwise zero is returned.
 */
static
int
get_fn_cnt(dumpp)
dumprec_t *dumpp;
{

	int recnum, seqnum;
	int file_found = 0;
	struct save	*scan;

	/*
	 * EXTRACTREC gives the number of records dumped for the event ==
	 * number of filename records + 1 event record.
	 */
	recnum = EXTRACTREC(dumpp->cmn.c_seqnum) - 1;
	/*
	 * EXTRACTSEQ gives the sequence number of the event; all records
	 * for the same event have the same sequence number.
	 */
	seqnum = EXTRACTSEQ(dumpp->cmn.c_seqnum);
	
	/* scan the fname list to see if all fname records available */
	for (scan = fn_beginp; (scan != NULL) && (file_found < recnum); scan = scan->next){
		if (scan->seqnum == seqnum){
			file_found++;	/* number fname records found */
		}
	}
	if (file_found == recnum){
		return (1);		/* all fname records available */
	} else {
		return (0);
	}
}

/*
 * If a record contains privileges expand it into one record
 * per privilege (pm_denied) event. Call the function to determine
 * if the record meets the selection criteria and if so, call the
 * function to print the record.
 */
static
void
check_recs(dumpp, cred_found)
dumprec_t	*dumpp;
int		cred_found;
{
	int		j, auditable; 
	long		status;
	ushort_t	event;
	ulong_t		seqnum;
	pvec_t		privs;
	pvec_t		mask;

	auditable = 0;
	/*
	 * Each bit set in dumpp->cmn.c_rprivs is actually a pm_denied
	 * event.  For each bit set, the dump record will be manipulated
	 * to make it look like a pm_denied record, and printed if appropriate.
	 */
	if (dumpp->cmn.c_rprivs  > 0 ) {
		privs = dumpp->cmn.c_rprivs;
		/*
		 * used to mask off all but 1 bit
		 * at a time, start with low order bit.
		 */
		mask = 1;
		/* save values */
		status = dumpp->cmn.c_status;
		event =  dumpp->cmn.c_event;
		seqnum =  dumpp->cmn.c_seqnum;

		/* make it look like there is no object info with the pm_denied record */
		dumpp->cmn.c_seqnum = 0;

		/*
	 	 * For each bit set in privs,
		 * check if it should be printed.
	 	 */
		for (j = 0; j < NPRIVS; j++){
			if (privs & mask){	/* check this record */
				/* make this look like a pm_denied event */
				dumpp->cmn.c_event = ADT_PM_DENIED;
				/*
			 	 * Set status to success/failure of
				 * pm_denied event. A zero in c_rprvstat
				 * means success.
			 	 */
				if ((mask & dumpp->cmn.c_rprvstat) > 0)
					dumpp->cmn.c_status = 1;	/* failure */
				else
					dumpp->cmn.c_status = 0;	/* success */
				/* 
				 * Set c_rprivs to the bit position being examined.
				 * The print function requires the priv bit position to
				 * determine the privilege name.
				 */
				dumpp->cmn.c_rprivs = j;
				/*
				 * if selection criteria are specified, check
				 * the record to see if they are met.
				 */
				if (option > 0){
					/* determine if this record meets the  selection criteria */
					auditable = (check_events((char *)dumpp));	
					if (auditable == 1) {
						match = 1;
						if (B_MASK & optmask){
 							printrec(bkfp,(char *)dumpp, cred_found);
						} else {
 							printrec(stdout,(char *)dumpp, cred_found);
						}
					}
				} else {
					if (B_MASK & optmask){
 						printrec(bkfp,(char *)dumpp, cred_found);
					} else {
 						printrec(stdout,(char *)dumpp, cred_found);
					}
				}
			}
			mask <<= 1;
		}
		/* restore values */
		dumpp->cmn.c_status = status;
		dumpp->cmn.c_event = event;
		dumpp->cmn.c_rprivs = privs;
		dumpp->cmn.c_seqnum = seqnum;
	}  /* done with privileges/pm_denied records */
	/*
	 * If the event type of this record is not pm_denied
	 * determine if it should be printed, it has already
	 * been printed if it is pm_denied.
	 */
	if (option > 0){		/* Check selection criteria */
		/* determine if the event record meets the selection criteria */
		auditable = (check_events((char *)dumpp));	
		if (auditable == 1) {
			match = 1;
			if (dumpp->cmn.c_event != ADT_PM_DENIED){
			 	if (B_MASK & optmask){
 				 	printrec(bkfp,(char *)dumpp, cred_found);
			 	} else {
 				 	printrec(stdout,(char *)dumpp, cred_found);
			 	}
			}
		}
	} else {
		if (dumpp->cmn.c_event != ADT_PM_DENIED){
			/* print the event record */
			if (B_MASK & optmask){
 				printrec(bkfp,(char *)dumpp, cred_found);
			} else {
 				printrec(stdout,(char *)dumpp, cred_found);
			}
		}
	}
}

/*
 * When processing of logfile is complete, display any remaining
 * partial records (records that are missing cred info or fname info).
 */
static
void
part_recs()
{
	int 		cred_found;
	needcred_t	*crlistp;
	needfname_t	*fnlistp;
	
	/*
 	 * For each record in the cr_dmp_lp linked list, see if it meets
 	 * the criteria specified, and if so print it.
 	 */
	cred_found = 0; 	/* no credential info*/
	for (crlistp = cr_dmp_lp; crlistp != NULL; crlistp = crlistp->next){
		(void)pfmt(stdout, MM_WARNING, W_NOCRED, crlistp->dumpp->cmn.c_pid);
		check_recs(crlistp->dumpp, cred_found);
	}
	/*
 	 * For each record in the fn_dmp_lp linked list, see if it meets
 	 * the criteria specified and if so print it.
 	 */
	cred_found = 1;		/* cred info available */
	for (fnlistp = fn_dmp_lp; fnlistp != NULL; fnlistp = fnlistp->next){
		check_recs(fnlistp->dumpp, cred_found);
	}

}


/*
 * Add this dumprec to the cr_dmp_lp linked
 * list and allocate a new dumprec.
 */
static
struct rec *
ad_cr_lst(dumpp)
dumprec_t *dumpp;
{
	needcred_t	*newp;
	needcred_t	*crlistp;	/* current list entry */

	/* allocate a new need_cred entry */
	if ((newp = (needcred_t *)malloc(sizeof(needcred_t))) == NULL){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		adt_exit(ADT_MALLOC);
	}
	
	/* put this new entry at the end of the list */
	if (cr_dmp_lp == NULL){		/* list is empty */
		cr_dmp_lp = newp;	/* insert at start of list */
	} else {
		/* find the end of the list */
		for (crlistp = cr_dmp_lp; crlistp->next != NULL; crlistp = crlistp->next)
			;
		/* add this entry to the end */
		crlistp->next = newp;
	}
	newp->next = NULL;
	newp->dumpp = dumpp;	/* assign the dumprec to this list entry */

	/* allocate a new dumprec structure */
	if ((dumpp = (dumprec_t *)malloc(sizeof(dumprec_t))) == NULL){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		adt_exit(ADT_MALLOC);
	}
	return(dumpp);
}

/*
 * Add this dumprec to the fn_dmp_lp linked list.
 */
static
void
ad_fn_lst(dumpp)
dumprec_t *dumpp;
{
	needfname_t	*newp;
	needfname_t	*fnlistp;	/* current list entry */

	/* allocate a new need_fname entry */
	if ((newp = (needfname_t *)malloc(sizeof(needfname_t))) == NULL){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		adt_exit(ADT_MALLOC);
	}

	/* put this new entry at the end of the list */
	if (fn_dmp_lp == NULL){		/* list is empty */
		fn_dmp_lp = newp;	/* insert at start of list */
	} else {
		/* find the end of the list */
		for (fnlistp = fn_dmp_lp; fnlistp->next != NULL; fnlistp = fnlistp->next)
			;
		/* add this entry to the end */
		fnlistp->next = newp;
	}
	newp->next = NULL;
	newp->dumpp = dumpp;	/* assign the dumprec to this list entry */
	newp->seqnum = EXTRACTSEQ(dumpp->cmn.c_seqnum);	/* keep the sequence number, to make checks faster */
}

/*
 * Traverse the cred dump list to see if the credential information for
 * each record is now available. If available, copy cred info to the record.
 * Check availability of fname records, then check selection criteria and print
 * record if criteria are met.
 */
static
void
ck_cr_dmp()
{
	needcred_t	*crlistp;	/* current list entry */
	needcred_t	*pcrlistp;	/* previous list entry */
	dumprec_t	*dumpp;
	int		recnum;
	int		fname_missing = 0;		/* initialize to false */

	/* check each dumprec in the the list to see if it is waiting for the new cred */
	for (crlistp = cr_dmp_lp; crlistp != NULL; pcrlistp = crlistp, crlistp = crlistp->next){
		if (cred_new == crlistp->dumpp->cmn.c_crseqnum){	/* match */
			dumpp = crlistp->dumpp;
			getcred(dumpp);		/* copy cred info into dumprec */

			/* delete current entry from the list and free it */
			if (crlistp == cr_dmp_lp)	/* this is the first entry in the list */
				cr_dmp_lp = crlistp->next;
			else
				pcrlistp->next = crlistp->next;
			free(crlistp);

			/*
			 * Make sure all fname records associated with this event are available.
			 *
			 * EXTRACTREC gives the number of records dumped for the event ==
			 * number of filename records + 1 event record.
			 */
			recnum = EXTRACTREC(dumpp->cmn.c_seqnum) - 1;
			/* Check if dumpp record has fname records associated with it.*/
			if (recnum > 1){
				/* if all fname records not available, put in dump_fname list */
				if (!get_fn_cnt(dumpp)) {
					fname_missing = 1;	/* set to true */
					ad_fn_lst(dumpp);	/* put dumprec in list */
				}
			}
			/* if all fname records are available */
			if (!fname_missing){
				/* If this record meets the selection criteria, print it. */
				check_recs(dumpp, CRED_FOUND);
				free(dumpp);	/* free the dumprec structure */
			}
		}
	}
}

/*
 * Traverse the fname dump list to see if the fname information for
 * each record is now available. If available, check selection criteria
 * and print record if criteria are met.
 */
static
void
ck_fn_dmp()
{
	needfname_t	*fnlistp;	/* current list entry */
	needfname_t	*pfnlistp;	/* previous list entry */

	/* check each dumprec in the the list to see if it is waiting for the new fname record  */
	for (fnlistp = fn_dmp_lp; fnlistp != NULL; pfnlistp = fnlistp, fnlistp = fnlistp->next){
		if (fname_new == fnlistp->seqnum){	/* match */
			if (get_fn_cnt(fnlistp->dumpp)) {
				/* If this record meets the selection criteria, print it. */
				check_recs(fnlistp->dumpp, CRED_FOUND);

				free(fnlistp->dumpp);	/* free the dumprec structure */
				/* delete current entry from list and free it */
				if (fnlistp == fn_dmp_lp)	/* this is the first entry in the list */
					fn_dmp_lp = fnlistp->next;
				else
					pfnlistp->next = fnlistp->next;
				free(fnlistp);
			}
		}
	}

}

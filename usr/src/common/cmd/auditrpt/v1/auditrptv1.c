/*		copyright	"%c%" 	*/

#ident	"@(#)auditrptv1.c	1.5"
#ident  "$Header$"
/***************************************************************************
 * Command: auditrptv1
 * Inheritable Privileges: P_AUDIT,P_SETPLEVEL
 *       Fixed Privileges: None
 * Notes: displays recorded audit information
 *
 * usage:
 * auditrpt [-o] [-i] [-b | -w] [-e [!]event[,...]] [-u user[,...]] 
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
 ***************************************************************************/

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <sys/vnode.h>
#include <sys/param.h>
#include <string.h>
#include <sys/privilege.h>
#include "audit.h"
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
#include "auditrec.h"
#include "auditrptv1.h"

/** 
 ** external variables
 **/
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

	int	char_spec;	/* is file character special? */	
	int 	free_size;	/* size of free-format data */
/** 
 ** static variables
 **/
static int	type_mask = 0;		/* set to object types specified with -t */
static long 	s_time, h_time;		/* time specified with -s/-h */
static uint 	uidtab2[MXUID];
static int 	uidnum2 = 0;
static int 	match = 0;
static int 	optmask = 0;
static ulong 	pmask = 0;
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


/** 
 ** external functions
 **/
extern int	adt_loadmap();		/* from adt_loadmap.c */

extern void	setgrplist(), 		/* from adt_proc.c */
		setproc();			

extern int	getproc();		/* from adt_proc.c */

extern int	parse_hour(), 		/* from adt_optparse.c */
		parse_range(), 			
		parse_lvl(), 		
		parse_priv(),	
		parse_obj(), 
		parse_objt(),
		parse_uid();

extern int	evtparse();		/* from adt_evtparse.c */
extern int	zevtparse();		/* from adt_evtparse.c */

extern int	adt_getrec(); 		/* from adt_getrec.c */

extern void	printrec();		/* from adt_print.c */

/** 
 ** local functions
 **/
static void 	chck_order(), 
		filenmfnc(), 
		chk_pr_zevt(), 
		user_check();

static int 	check_ltdb(), 
		dologs(), 
		proc_rec(), 
		check_lvlfile(), 
		check_events(), 
		backward(), 
		nexteol(),
		getlog();

static char	*parsepath();

#define STD_IN	"stdin"

/**
 ** Procedure:	main
 **
 ** Privileges:	lvlproc:   P_SETPLEVEL
 **		auditbuf:  P_AUDIT
 **		auditevt:  P_AUDIT
 **
 ** Restrictions:	none
 **/
main(argc, argv)
int	argc;
char	**argv;
{
	extern int optind;	
	extern char *optarg;
	int 	c;	
	int	lognum=0;
	char  	*opt1, *opt2;
	char 	*eventsp, *zeventsp, *uidp, *objp,*objtp;
	char 	*lvlp, *rangep, *h_hourp, *s_hourp, *privp;
	char	*map;		/* pathname of auditmap file */
	int	ret;		/* return values from routines */
	int 	i, a_on;
	char	log[MAXPATHLEN+1];
	abuf_t	abuf;
	ushort	badlog=0;
	struct  stat statpath;
	short	no_audit = 0;

	fn_beginp=fn_lastp=NULL;
	s_time=h_time=0;
	lvlmin=lvlmax=0;

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
	}else
		if (errno != ENOPKG){ 
			(void)pfmt(stderr, MM_ERROR, E_LVLOPER, "lvlproc", errno);
			exit(ADT_LVLOPER);
		}

	/* print command line */
	(void)pfmt(stdout,MM_NOSTD,M_1);
	for (i=0; i < argc; i++)
		(void)fprintf(stdout,"%s ",argv[i]);
	(void)fprintf(stdout,"\n");

	/* parse the command line  */
	while ((c=getopt(argc,argv,"?owbiz:a:e:r:m:u:f:s:h:t:l:p:v:"))!=EOF) {
		switch (c) {
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
				if((map=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(map,optarg);
				optmask |= M_MASK;
				break;
			case 'i':
				optmask |= I_MASK;
				break;
			case 'e':
				if((eventsp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(eventsp,optarg);
				optmask |= E_MASK;
				option=1;	/* criteria selection option */
				break;
			case 'u': 
				if((uidp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(uidp,optarg);
				optmask |= U_MASK;
				option=1;
				break;
			case 'f':
				if((objp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(objp,optarg);
				optmask |= F_MASK;
				option=1;
				break;
			case 't':
				if((objtp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(objtp,optarg);
				optmask |= T_MASK;
				option=1;
				break;
			case 'l': 
				if((lvlp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(lvlp,optarg);
				optmask |= L_MASK;
				option=1;
				break;
			case 'r': 
				if((rangep=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(rangep,optarg);
				optmask |= R_MASK;
				option=1;
				break;
			case 'h': 
				if((h_hourp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(h_hourp,optarg);
				optmask |= H_MASK;
				option=1;
				break;
			case 's': 
				if((s_hourp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(s_hourp,optarg);
				optmask |= S_MASK;
				option=1;
				break;
			case 'a':
				if ((*optarg=='s') && (strlen(optarg)== 1)){
					/* if -a f was specified, undo it */
					if (AF_MASK & optmask) {
						optmask &= ~AF_MASK; 
					}
					optmask |= AS_MASK;
				
				}
				else if ((*optarg=='f')&&(strlen(optarg)==1)) {
					if (AS_MASK & optmask) {
						optmask &= ~AS_MASK; 
					}
					optmask |= AF_MASK;
				}
				else {
					(void)pfmt(stderr, MM_ERROR,RE_BAD_OUT);
					exit(ADT_BADSYN);
				}
				option=1;
				break;
			case 'p':
				if((privp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(privp,optarg);
				optmask |= P_MASK;
				option=1;
				break;
			case 'v':
				(void)strncpy(subtypep,optarg,(SUBTYPEMAX+1));
				optmask |= V_MASK;
				option = 1;
				break;
			case 'z':
				if((zeventsp=(char *)malloc(strlen(optarg)+1))==NULL){
					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
					adt_exit(ADT_MALLOC);
				}
				(void)strcpy(zeventsp,optarg);
				optmask |= Z_MASK;
				option =1;
				break;
			case '?':
				usage();
				exit(ADT_BADSYN);
				break;
		}/* switch */
	}/* while */

	/**
	 ** validate options
 	 **/

	/* -z option only valid with -i option*/
	if ((Z_MASK & optmask) &&  (optmask & (O_MASK | B_MASK | W_MASK|E_MASK | U_MASK | F_MASK | T_MASK| P_MASK| L_MASK| R_MASK| S_MASK | H_MASK |AS_MASK | AF_MASK |M_MASK | V_MASK))) {
		usage();
		exit(ADT_BADSYN);
	}

	/* -o with no selection criteria options? */
	if ((O_MASK & optmask) && !(option)) {	
		(void)pfmt(stderr,MM_ERROR,RE_INCOMPLETE);
		usage();
		exit(ADT_BADSYN);
	}
	if ((L_MASK & optmask) && (R_MASK & optmask)) {
		opt1="-l";
		opt2="-r";
		(void)pfmt(stderr, MM_ERROR,RE_BAD_COMB, opt1, opt2);
		usage();
		exit(ADT_BADSYN);
	}
	if (I_MASK & optmask){			/* -i specified */
		if (argv[optind] && (strlen(argv[optind]) != 0)){	/*log file and -i specified*/
			for( ; optind<argc; optind++) 
				(void)pfmt(stdout,MM_WARNING,RW_IGNORE,argv[optind]);
		}
	}
	if (M_MASK & optmask){
		/* determine if argument to -m is a readable directory */
		if (  (access(map,R_OK) != 0) 
		   || (stat(map,&statpath) != 0)
		   || ((statpath.st_mode & S_IFMT) != S_IFDIR))	{
			(void)pfmt(stderr,MM_ERROR,RE_BADDIR,map);
			exit(ADT_BADSYN);
		}
	}

	/** 
	 ** if the -w option was specified or no audit log files were specified,
	 ** auditing must be installed in the current machine, otherwise
	 ** exit with ENOPKG error code.
	 **/
	if ( no_audit && ((W_MASK & optmask) || !argv[optind] || (strlen(argv[optind]) == 0))){
		(void)pfmt(stderr,MM_ERROR,RE_NOPKG);
		exit(ADT_NOPKG);
	}

	/* get current log file if auditing is on */
	(void)memset(log,NULL,MAXPATHLEN+1);
	if (!no_audit) {
		if (getlog(log) != -1){
			a_on = 1;		/*auditing is on*/
		}
		else
			a_on = 0;
	}
	else
		a_on = 0;

	if (W_MASK & optmask) {
		if (B_MASK &optmask){
			opt1="-b";
			opt2="-w";
			(void)pfmt(stderr, MM_ERROR,RE_BAD_COMB, opt1, opt2);
			usage();
			exit(ADT_BADSYN);
		}
		if (a_on){
			if (argv[optind] && (strlen(argv[optind]) != 0) ) /*log file(s) specified*/
				for( ; optind<argc; optind++)
					(void)pfmt(stdout,MM_WARNING,RW_IGNORE,argv[optind]);
		}
		else {
			(void)pfmt(stderr,MM_ERROR,RE_W_DISABLE);
			exit(ADT_BADSYN);
		}
		if (auditbuf(ABUFGET, &abuf, sizeof(abuf_t))) {
			(void)pfmt(stderr,MM_ERROR,E_BADBGET);
			exit(ADT_BADBGET);
		}
		if (abuf.vhigh !=0)
			(void)pfmt(stdout,MM_WARNING,RW_NO_VH0);
	}
	if (!(M_MASK & optmask)) {
		map=ADT_MAPDIR;
	}

	if ((ret=check_ltdb(map)) != 0) /*check accessability of ltdb files */
		exit(ret);
	if ((ret=adt_loadmap(map)) != 0) /*load the auditmap file*/
		exit(ret);	

	if (E_MASK & optmask) {
		if (evtparse(eventsp,rpt_emask) != 0) 
			exit(ADT_BADSYN);	
	}
	if (U_MASK & optmask) {
		if (parse_uid(uidp) != 0) 
			exit(ADT_BADSYN);
		user_check();
	} 
	if (F_MASK & optmask) {
		if ((ret=parse_obj(objp)) != 0) 
			exit(ret);
	}
	if (T_MASK & optmask) {
		if (parse_objt(objtp,&type_mask) != 0) 
			exit(ADT_BADSYN);
	}
	if (L_MASK & optmask) {
		if ((lvlno=parse_lvl(lvlp)) == -1) 
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
				(void)pfmt(stderr,MM_ERROR,RE_BAD_TIME);
				exit(ADT_BADSYN);
			}
	if (P_MASK & optmask) {
		if ((pmask=parse_priv(privp)) == -1)
		 /*-1 is returned when invalid priv specified,
		 otherwise, priv vector will be returned */
			exit(ADT_BADSYN);
	}

	if (Z_MASK & optmask) {
		if (zevtparse(zeventsp,zrpt_emask) != 0) 
			exit(ADT_BADSYN);	
		
	}

	/**
	 ** process log files
  	 **/
	if (I_MASK & optmask){			/* -i specified */
		if ((ret=(dologs(STD_IN))) != 0)
			exit(ret);
	}
	/* if no logs specified  or -w specified, use current active log*/
	else if ( !argv[optind] || (strlen(argv[optind])==0) || (W_MASK & optmask)){ 
		if (a_on){
			if (cur_spec){	/* current active log character special? */
				(void)pfmt(stdout,MM_WARNING,RW_SPEC);
				adt_exit(ADT_NOLOG);
			}
			if ((ret=(dologs(log)))!=0){ 
				adt_exit(ret);
			}
		} else {
			(void)pfmt(stderr,MM_ERROR,RE_NOT_ON);
			adt_exit(ADT_BADSYN);
		}
	} else {				/* log file(s) specified */
		for ( ; optind < argc; optind++){
			/* current active log character special? */
			if((strcmp(argv[optind],log) == 0) && (cur_spec))
				(void)pfmt(stdout,MM_WARNING,RW_SPEC);
			else{
				if ((ret=dologs(argv[optind]))==0) {
					lognum++;
				}
				else if (ret == ADT_NOLOG)
					badlog++;
			 	else
					adt_exit(ret);
			}
		}
		if (lognum == 0) {
			(void)pfmt(stderr,MM_ERROR,E_NO_LOGS);
			adt_exit(ADT_NOLOG);
		}
	}
	/** if no match was found in  log files **/
	if ((option > 0) && (match == 0))
		(void)pfmt(stdout,MM_WARNING,RW_NO_MATCH);
	if (badlog)
		adt_exit(ADT_INVARG);
	else
		adt_exit(ADT_SUCCESS);
/* NOTREACHED */
}/* end of main*/

/**
 ** Procedure:	getlog
 **
 ** Privileges:	auditctl:  P_AUDIT
 **		auditlog:  P_AUDIT
 **
 ** Restrictions:	none
 **
 ** Get the current log name from the auditctl structure.
 **/
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
	else {
	/* allocate space for the ppathp, apathp, progp in alog struct*/
		(void)memset(&alog,NULL,sizeof(alog_t));
		if (((alog.ppathp=(char *)malloc(MAXPATHLEN+1))==NULL)
	  	||((alog.apathp=(char *)malloc(MAXPATHLEN+1))==NULL)
	  	||((alog.progp=(char *)malloc(MAXPATHLEN+1))==NULL)){
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		(void)memset(alog.ppathp,NULL,MAXPATHLEN+1);
		(void)memset(alog.apathp,NULL,MAXPATHLEN+1);
		(void)memset(alog.progp,NULL,MAXPATHLEN+1);

		/* get current log file attributes */
		if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
			(void)pfmt(stderr, MM_ERROR, E_BADLGET);
			adt_exit(ADT_BADLGET);
		}
		if (alog.flags & PPATH) 
			(void)strcat(logname,alog.ppathp);
		else
			(void)strcat(logname,ADT_DEFPATH);
		if (alog.flags & PSPECIAL){
			cur_spec=1;
			goto out;
		}
		(void)strcat(logname,"/");
		(void)strncat(logname,alog.mmp,ADT_DATESZ);
		(void)strncat(logname,alog.ddp,ADT_DATESZ);
		(void)sprintf(seq,"%03d",alog.seqnum);
		(void)strncat(logname,seq,ADT_SEQSZ);
		if (alog.pnodep[0] != '\0')
			(void)strcat(logname,alog.pnodep);
out:
		free(alog.ppathp);
		free(alog.apathp);
		free(alog.progp);
		return(0);
	}
}

/**
 ** Open audit log file for reading.  Call the proc_rec routine to
 ** process one record at a time.
 **/
int
dologs(filename)
char 	*filename;	/* log file's name */
{
	struct stat status;
	int ret;

	if (strcmp(filename,STD_IN) != 0){
		if (access(filename,F_OK) != 0) {
			(void)pfmt(stdout,MM_WARNING,RW_NO_LOG,filename);
			return(ADT_NOLOG);
		}
		if ((stat(filename, &status)) == -1){
			(void)pfmt(stderr,MM_ERROR,E_FMERR);
			return(ADT_FMERR);
		}
		if ((status.st_mode & S_IFMT) == S_IFCHR){
			char_spec=1;	/* this is a character special file */
		}
		else{
			if ((status.st_mode & S_IFMT) == S_IFREG)
				char_spec=0;
			else {
				(void)pfmt(stdout,MM_WARNING,RW_IGNORE,filename);
				return(ADT_NOLOG);
			}
		}
		if ((freopen(filename,"r",stdin))==NULL) {
			(void)pfmt(stderr,MM_ERROR,E_FMERR);
			return(ADT_FMERR);
		}

	}
	else
		char_spec = 1;	/* if stdin, might be character special */

	/* if -b specified, open temp file to write output */
	if (optmask & B_MASK) {
		if ((tname=tempnam(P_tmpdir,"AD"))==NULL) {
			(void)pfmt(stderr,MM_ERROR,E_FMERR);
			return(ADT_FMERR);
		}
		if ((bkfp=fopen(tname, "w+r+"))==NULL){
			(void)pfmt(stderr,MM_ERROR,E_FMERR);
			return(ADT_FMERR);
		}
		/* change mode of temporary file before writing to it */
		if (chmod(tname,S_IRUSR|S_IWUSR) != 0){
			(void)pfmt(stderr,MM_ERROR,E_CHMOD,errno);
			return(ADT_FMERR);
		}
	}

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
		ret=backward();
		(void)fclose(bkfp);
		(void)unlink(tname);
		tname=NULL;
	}
	if (ret > 0)
		return(ret);
	return(0);
}

/**
 ** Process the next record from audit log file using the following logic:
 **	-call the adt_getrec routine to fill the cmn, spec and freeformat
 ** 	 parts of the dump_rec record (see dumprec_t in auditrptv1.h). 
 **	-if this is a filename record, save the filename information in
 **	 list of filenames
 **	-else if this is a process record, save information of the new (or
 **	 updated) process in the list of process blocks
 **	-else if this is a groups record, save the new supplementary groups
 **	 information for this process
 **	-else if any other kind of record (event record):
 **		-get the process block for this record (fill the proc and
 **		 and groups part of the dump_rec record)
 **		-determine if the record is post-selected
 **		-display the record if post-selected
 **/
int
proc_rec(log_fp,filename)
FILE	*log_fp;
char	*filename;
{
	int rtype,auditable,ret;
	dumprec_t	dump_rec;
	struct id_r	*ip;
	int proc_found;

	auditable=rtype=0;
	/* get the next record */
	if ((ret=adt_getrec(log_fp, &dump_rec)) != 0){
		return (ret);
	}
	rtype = dump_rec.r_rtype;	
	switch (rtype) {
		case A_FILEID :
			ip=(struct id_r *)&dump_rec.spec_data;
			if (ip->i_mac){
				mac_installed++;
				if (bad_ltdb)
					(void)pfmt(stdout,MM_WARNING,RW_BADLTDB);
			}	
			else
				mac_installed=0;
			ip->i_aver[ADT_VERLEN-1] = '\0';
			ip->i_mmp[ADT_DATESZ] = '\0';
			ip->i_ddp[ADT_DATESZ] = '\0';
			(void)pfmt(stdout,MM_NOSTD, M_2, ip->i_mmp, ip->i_ddp);
			(void)pfmt(stdout,MM_NOSTD, M_3, dump_rec.r_seqnm);
			(void)pfmt(stdout,MM_NOSTD, M_4, ip->i_aver);
			(void)pfmt(stdout,MM_NOSTD, M_5, dump_rec.freeformat);
			(void)fflush(stdout);
			if ((strcmp(mach_info, dump_rec.freeformat)) != 0) {
				(void)pfmt(stdout,MM_WARNING,RW_MISMATCH,filename,dump_rec.freeformat,mach_info);
				(void)fflush(stderr);
			}
			chck_order(ip,dump_rec.r_seqnm);
			break;
		case FILENAME_R : 	
			/* add this record to linked list of filename records */
			filenmfnc(&dump_rec);
			break;
		case PROC_R :
			/* new process or process information changed */
			setproc(&dump_rec.cmn, (struct proc_r *)&dump_rec.d_spec);
			break;
		case GROUPS_R :
			/* new multple groups info -- update process info */
			setgrplist(&dump_rec.cmn,dump_rec.ngroups,dump_rec.groups);
			break;
		case ZMISC_R :
			/* records for trusted applications*/
			(void)chk_pr_zevt(&dump_rec);
			break;
		default :
		/* get process info and display output for everything else */
			if ((getproc(&dump_rec)) == -1){
				(void)pfmt(stdout,MM_WARNING,W_NOPROC,dump_rec.cmn.c_pid);
				proc_found = 0; /*no process info found*/
			}
			else
				proc_found = 1;
	   		if (option > 0) {
				auditable = (check_events((char *)&dump_rec));	
				if (auditable) {
					match=1;
					if (B_MASK & optmask)
 						printrec(bkfp,(char *)&dump_rec,proc_found);
					else
 						printrec(stdout,(char *)&dump_rec,proc_found);
				}
			}
	        	else {
				if (B_MASK & optmask)
 					printrec(bkfp,(char *)&dump_rec,proc_found);
				else
 					printrec(stdout,(char *)&dump_rec,proc_found);
			}
			break;
	}
	if (dump_rec.freeformat)
		free(dump_rec.freeformat);
	return(0);
}

/**
 ** Check if the specified uid or logname is valid.  These are validated
 ** against the auditmap.
 **/
void
user_check()
{
	int i, ii;
	int found;
	ids_t *uidtbl;

	if (uidnum > 0) {
		for (i=0; i<uidnum; i++) {
			found = 0;
			for (ii=0, uidtbl=uidbegin; ii<s_user; ii++) {
				if(uidtab[i] == uidtbl->id) {
					found = 1;
					break;
				} else
					uidtbl++;
			}
			if (!found)
				(void)pfmt(stdout,MM_WARNING,RW_NO_USR1,uidtab[i]);
	        }
	}
	if (usernum > 0) {
		for (i=0; i<usernum; i++) {
			found = 0;
			for (ii=0, uidtbl=uidbegin; ii<s_user; ii++) {
				if(strcmp(usertab[i],uidtbl->name)== 0) {
					uidtab2[uidnum2++] = uidtbl->id;
					found = 1;
					break;
				} else
					uidtbl++;
			}
			if (!found)
				(void)pfmt(stdout,MM_WARNING,RW_NO_USR2,usertab[i]);
	        }
	}
}

/**
 ** Determine if the record pointed to by d is selectable (i.e. was 
 ** post-selected with criteria selection options).
 **/
int
check_events(d)
char *d;
{
	int		infomask = 0;
	int		i,obj,ftypemask;
	dumprec_t	*dumpp;
	cmnrec_t	*cmnp;
	struct proc_r	*procp;
	struct	priv_r	*pp;
	struct	ipc_r	*ipcp;
	struct save	*scan;
	pvec_t 		prec;
	char		*freep;
	char		typetemp[SUBTYPEMAX + 2];
	char		*token;
	int		seqnum;


	dumpp=(dumprec_t *)d;
	cmnp=&dumpp->cmn;
	procp=&dumpp->proc;
	pp=(struct priv_r *)&dumpp->d_spec;
	ipcp=(struct ipc_r *)&dumpp->d_spec;
	freep=dumpp->freeformat;

	/** The options -b, -w, -m and -i must be accompanied by
  	 ** criteria selection options for this routine to be called.
  	 **/
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
		if (EVENTCHK(cmnp->c_event,rpt_emask)) {
			if (O_MASK & optmask)
				return(1);
			else
				infomask |= E_MASK; 
		}
	}
	if ((optmask & P_MASK) && (cmnp->c_rtype == PRIV_R)) {
		prec = 0;
		if (pp->p_req >=0) {
			pm_setbits(pp->p_req,prec);
			if (prec & pmask) {
			 	if (O_MASK & optmask)
					return(1);
				else
					infomask |= P_MASK; 
			}
			
		}
	}

	if (optmask & F_MASK ) { 
		/*A user entered object id may refer to an IPC object or a*/
		/*loadable module. The numbers assigned to IPC objects and*/
		/*loadable modules are not mutually exclusive, therefore  */
		/*additional records may be displayed.                    */
		if ((cmnp->c_rtype == IPCR_R)|| (cmnp->c_rtype == MODLD_R)) {
			if (ipcnum > 0) {
				for (i=0; i< ipcnum; i++) {
					/*The first two members of the ipc_r and modld_r*/
					/*structures are the same size.                 */
					if (ipctab[i] == ipcp->i_id) {
						if (O_MASK & optmask)
							return(1);
						else
							infomask |= F_MASK; 
					}

				}
			}
		}

		if (has_path(cmnp->c_rtype,cmnp->c_event)){
			for (scan=fn_beginp; scan != NULL; scan=scan->next){
				seqnum=EXTRACTSEQ(cmnp->c_seqnm);
				if (scan->seqnum == seqnum) {
					for (obj=0;obj < objnum; obj++) {
						if (strcmp(objtab[obj],scan->name) == 0){
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
		ftypemask=0;
	 	if (has_path(cmnp->c_rtype,cmnp->c_event)){
			seqnum=EXTRACTSEQ(cmnp->c_seqnm);
			for (scan=fn_beginp; scan != NULL; scan=scan->next){
				if (scan->seqnum == seqnum) {
					switch(scan->rec.f_type) {
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
					}/*switch*/
				}
			}/*for*/
		} 
		else {
			/*ipc objects do not have filename records*/
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
				}/*switch*/
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

	if ((optmask & L_MASK ) && has_path(cmnp->c_rtype,cmnp->c_event)){
		seqnum=EXTRACTSEQ(cmnp->c_seqnm);
		for (scan=fn_beginp; scan != NULL; scan=scan->next){
			if (scan->seqnum == seqnum) {
				if (scan->rec.f_lid == lvlno) {
		        		if (optmask & O_MASK)
						return(1);
					else
						infomask |= L_MASK;
				}
			}
		}
	} else {
	if ((optmask & R_MASK ) && has_path(cmnp->c_rtype,cmnp->c_event)){
		seqnum=EXTRACTSEQ(cmnp->c_seqnm);
		for (scan=fn_beginp; scan != NULL; scan=scan->next){
			if (scan->seqnum == seqnum) {
		   		/* is level within the range ? */
				if ((adt_lvldom(scan->rec.f_lid,lvlmin) > 0) && 
				(adt_lvldom(lvlmax,scan->rec.f_lid) > 0)) 
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
			for ( i=0; i<uidnum; i++){
				if (uidtab[i]==procp->pr_uid 
				   || uidtab[i]==procp->pr_ruid){
					if (optmask & O_MASK)
						return(1);
					else
						infomask |= U_MASK; 
				}
			}
		}
		if (usernum > 0){
			for ( i=0; i<uidnum2; i++){
				if (uidtab2[i]==procp->pr_uid
				   || uidtab2[i]==procp->pr_ruid){
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
		if (cmnp->c_time >= s_time) {
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= S_MASK;
		}
	}
	/* if only end time is specified */
	if ((optmask & H_MASK ) && !(optmask & S_MASK)) {
		if (cmnp->c_time <= h_time) {
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= H_MASK;
		}
	}
	/* if both end time and start time are specified */
	if ((optmask & H_MASK) && (optmask & S_MASK)) {
		if (optmask & O_MASK) {
			if ((cmnp->c_time >= s_time) || (cmnp->c_time <= h_time)) 
				return(1);
		}
		else {
			if ((cmnp->c_time >= s_time)&&(cmnp->c_time <= h_time)){ 
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
		(void)strncpy(typetemp,freep,(SUBTYPEMAX+1));
		typetemp[SUBTYPEMAX]='\0';
		if (strchr(typetemp,':') == NULL){
			(void)pfmt(stdout, MM_WARNING, RW_MISC);
			token=typetemp;
		}
		else{
			token=strtok(typetemp,":");
		}
		if (!strncmp(token,subtypep,SUBTYPEMAX)){
			if (optmask & O_MASK)
				return(1);
			else
				infomask |= V_MASK;
		}
	}
	if (optmask == infomask) 
		return(1);
	else
		return(0);
}

/** 
 ** This function returns 1 if the record type rtype may have pathname 
 ** records (FILENAME_R) related to the same event.
 **/
int
has_path(rtype, event)
ushort rtype;
ushort event;
{
	switch(rtype){
	case CHOWN_R : case CHMOD_R : case EXEC_R : case MAC_R : 
	case DEV_R : case MOUNT_R : case FPRIV_R : case ALOG_R :
		return (1);
	case FILE_R:
		if (event != ADT_EXIT)
			return (1);
		else
			return (0);
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
	case MODLD_R:
		if (event == ADT_MODLOAD)
			return (1);
		else
			return (0);
	default:
		return (0);
	}		
}

/**  
 ** Parse a pathaname and reduce its size  by eliminating '.'s and '..'s.
 ** Only reduce if the path is absolute (begins with '/').
 **
 ** Note:
 ** We always resolve "dir1/dir2/.." to "dir1".  This would not be correct if
 ** dir2 was a symbolic link, but we needn't concern ourselves with that,
 ** since symbolic links never appear in the audit trail -- they are resolved
 ** before the audit record is written.
 **/
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

/**
 ** Display in reverse order the output of post-selected records saved in 
 ** the file pointed to by bkfp.  This routine is called after processing
 ** all the records if the -b option was specified.
 **/
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
		(void)pfmt(stderr,MM_ERROR,E_FMERR);
		return(ADT_FMERR);
	}
	/* determine how many parts to divide the file into */
	n = size / BUFSIZE;
	/* if size divides exactly into BUFSIZE, consider one less part */
	if ((n * BUFSIZE) == size)
		n--;
	for(i=n; i>=0; i--){
		start=BUFSIZE * i;
		if ((fseek(bkfp, start, SEEK_SET)) != 0){
			(void)pfmt(stderr,MM_ERROR,E_FMERR);
			return(ADT_FMERR);
		}
		total=fread(&buf[0], 1, BUFSIZE, bkfp);

		/*Decrement the number of bytes read by one because it*/
		/*is used to index into an array.                     */
		if(buf[total - 1] != '\n'){
			sp=nexteol(buf,total - 1,&letters) + 1;
			(void)write(1,&buf[sp],letters);
			sp--;
		}
		else
			sp=total - 1;

		if (sav_letters != 0){
			(void)write(1, save, sav_letters);
			sav_letters = 0;
		}
		ep=sp;
		while(sp > 0){
			sp = nexteol(buf,(ep-1), &letters) + 1;
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
				(void)write(1,&buf[sp],letters+1);
			ep = sp-1;
		}
	}
	return(0);
}

/**
 ** Return the position of the next end-of-line character from right
 ** to left starting to search at position last.  Set letters to the
 ** number of characters counted before the '\n'.
 **/
int
nexteol(buf,last,letters)
char	*buf;
int	last;
int	*letters;
{
	int count;
	count=last; 
	*letters = 0;
	while ((buf[count] != '\n')&& (count>=0))
	{
			count--;	
			(*letters)++;
	}
	return(count);
}
	
/**
 ** Print usage message. 
 **/
void
usage()
{
	(void)pfmt(stderr,MM_ACTION,USAGE1);
	(void)pfmt(stderr,MM_NOSTD,USAGE2);
	(void)pfmt(stderr,MM_NOSTD,USAGE3);
	(void)pfmt(stderr,MM_NOSTD,USAGE4);
	(void)pfmt(stderr,MM_NOSTD,USAGE5);
}

/**
 ** Determine if the ltdb map files ~lid.internal and ~ltf.* are accessible.
 **/
int
check_ltdb(mapdir)
char *mapdir;
{
	int	ret;

	if ((ret=check_lvlfile(mapdir,LDF,LID)) != 0)
		return(ret);
	if ((ret=check_lvlfile(mapdir,ALASF,ALAS)) !=0)
		return(ret);
	if ((ret=check_lvlfile(mapdir,CATF,CTG)) !=0)
		return(ret);
	if ((ret=check_lvlfile(mapdir,CLASSF,CLS)) !=0)
		return(ret);
	return(0);
}
/**
 ** Construct the pathname of an audit map file based on the directory name
 ** (possibly supplied by -m option) and the file name.  Return 1 if the
 ** file is accessible, 0 otherwise.
 **/
int
check_lvlfile(mapdir,fname,file)
char *mapdir;
char *fname;
int  file;
{
	int     msize;
	char 	*fullname;
	/* allocate space to hold pathname */
	msize=strlen(mapdir)+strlen(fname)+1;
	if ((fullname = ((char *)malloc(msize))) ==  (char *)NULL) {
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		return(ADT_MALLOC);
	}
	/* determine the full pathname of the file */
	(void)strcpy(fullname,mapdir);
	(void)strcat(fullname,fname);
	if (access(fullname,R_OK) != 0) {
		bad_ltdb++;
		return(0);
	}
	switch(file) {
		case LID:
			lid_fp=fullname;
			break;
		case ALAS:
			alias_fp=fullname;
			break;
		case CTG:
			ctg_fp=fullname;
			break;
		case CLS:
			class_fp=fullname;
			break;
	}
	return(0);
}
/**
 ** Determine if the specified log files are not in sequence or if 
 ** a file is missing.
 **/
void
chck_order(ip,seq)
struct id_r *ip;
unsigned long seq;
{
	int date;
	static ulong lastseq = 0;	/* sequence number of last log file */
	static ulong lastdate = 0;	/* date of last log file */
	static int msg_printed = 0;	/* display error message only once */

	date = atoi(strcat(ip->i_mmp,ip->i_ddp));
	if ((lastseq) && !(msg_printed)){
		if ( ((date == lastdate) && (seq != (lastseq + 1)))
		     || (date < lastdate)
	             || ((date>lastdate) && (seq != 1)) ) {
				(void)pfmt(stdout,MM_WARNING,RW_OUTSEQ);
				msg_printed++;
		}
	}
	lastdate=date;
	lastseq=seq;
}
	
/**
 ** This routine handles "pathname" records.  It builds a linked list of
 ** pathnames and associates each pthaname to an event based on the 
 ** sequence number.
 **/
void
filenmfnc(dump_rec)
dumprec_t	*dump_rec;
{
	char		*filenmp = NULL;
	filnmrec_t 	*fname_rec;
	save_t		*this_p;

	fname_rec = (filnmrec_t *)dump_rec;			
	/* add this record to linked list of filename records */
	if (fn_beginp == NULL){
		if ((fn_beginp=(save_t *)malloc(sizeof(save_t))) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		this_p = fn_beginp;
		fn_lastp=fn_beginp;
	}
	else {
		if ((fn_lastp->next=(save_t *)malloc(sizeof(save_t))) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		this_p = fn_lastp->next;
		fn_lastp = this_p;
	}
	this_p->pid=fname_rec->cmn.c_pid;
	this_p->seqnum=EXTRACTSEQ(fname_rec->cmn.c_seqnm);
	(void)memcpy(&(this_p->rec), &(fname_rec->spec),sizeof(struct fname_r));
	if (strlen(dump_rec->freeformat) != 0){
		/* 
		 * don't reduce the path if it is relative, there
		 * can be paths above the working directory, e.g. ~/../..
		 */
		if (dump_rec->freeformat[0] != '/')
			filenmp = dump_rec->freeformat;
		else
			filenmp = parsepath(dump_rec->freeformat);

		if ((this_p->name=(char *)malloc(strlen(filenmp)+1))== NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			adt_exit(ADT_MALLOC);
		}
		(void)strcpy(this_p->name,filenmp);
	}
	else
		this_p->name=NULL;

	this_p->next=NULL;
}

/**
 ** routine called upon exit
 **/
void
adt_exit(status)
int status;
{
	if (bkfp != NULL)
		(void)unlink(tname);
	exit(status);
}

/**
 **print ZMISC_R records
 **/
void
chk_pr_zevt(dumpp)
dumprec_t *dumpp;
{
	cmnrec_t *cmnp;
	int zsize;

	cmnp = &dumpp->cmn;
	if (EVENTCHK(cmnp->c_event,zrpt_emask))
	{
		match = 1;
		/*Size of a ZMISC_R record is: event + size + freefomat*/
		zsize = sizeof(ushort) + sizeof(int) + free_size;
		if (write(1,&(cmnp->c_event),sizeof(ushort)) == -1)
		{
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			exit(ADT_FMERR);
		}
		if (write(1,&zsize,sizeof(int)) == -1)
		{
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			exit(ADT_FMERR);
		}
		if (dumpp->freeformat != NULL)
		{
			if (write(1,dumpp->freeformat,free_size) == -1)
			{
				(void)pfmt(stderr, MM_ERROR, E_FMERR);
				exit(ADT_FMERR);
			}
		}
	}
}

		

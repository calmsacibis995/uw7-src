#ident  "@(#)usermod.c	1.5"
#ident  "$Header$"
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<sys/mman.h>
#include	<sys/wait.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<limits.h>
#include	<grp.h>
#include	<string.h>
#include	<userdefs.h>
#include	"users.h"
#include	"messages.h"
#include 	<shadow.h>
#include	"uidage.h"
#include	<nwusers.h>
#include 	<pwd.h>
#include 	<signal.h>
#include 	<errno.h>
#include 	<time.h>
#include 	<unistd.h>
#include 	<stdlib.h>
#include	<deflt.h>
#include	<mac.h>
#include	<ia.h>
#include	<sys/vnode.h>
#include	<audit.h>
#include	<priv.h>
#include	<fcntl.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<rpcsvc/ypclnt.h>
#include	<rpcsvc/yp_prot.h>
#include	"ypdefs.h"

/*
 * Usage:
 *
 * no MAC - no auditing installed (base system)
 *
 *	usermod [-u uid [-o]] [-g group] [-G group[[,group]...]] [-d dir [-m]]
 *		[-s shell] [-c comment] [-l new_logname] [-f inactive]
 *		[-e expire] [-p passgen] login
 *
 * MAC - no auditing installed
 *
 *	usermod [-u uid [-o]] [-g group] [-G group[[,group]...]] [-d dir [-m]]
 *		[-s shell] [-c comment] [-l new_logname] [-f inactive]
 *		[-e expire] [-p passgen] [-h [operator2]level[-h level [...]]]
		[-v def_level] login
 *
 * no MAC - auditing installed
 *
 *	usermod [-u uid [-o]] [-g group] [-G group[[,group]...]] [-d dir [-m]]
 *		[-s shell] [-c comment] [-l new_logname] [-f inactive]
 *		[-e expire] [-p passgen]] [-a [operator1]event[,...]] login
 *
 * MAC and auditing installed
 *
 *	usermod [-u uid [-o]] [-g group] [-G group[[,group]...]] [-d dir [-m]]
 *		[-s shell] [-c comment] [-l new_logname] [-f inactive]
 *		[-e expire] [-p passgen] [-h [operator2]level[-h level [...]]]
		[-v def_level] [-a [operator1]event[,...]] login
 *
 * Level:	SYS_PRIVATE
 *
 * Inheritable Privileges:	P_OWNER,P_AUDIT,P_COMPAT,P_DACREAD,P_DACWRITE
 *				P_FILESYS,P_MACREAD,P_MACWRITE,P_MULTIDIR,
 *				P_SETPLEVEL,P_SETUID,P_FSYSRANGE,P_SETFLEVEL
 * Fixed Privileges:		none
 *
 * Files:	/etc/passwd
 *		/etc/shadow
 *		/etc/netware/nwusers
 *		/etc/.pwd.lock
 *		/etc/security/ia/master
 *
 * Notes: 	This command modifies user logins on the system.
 */

extern	void	errmsg(),
		adumprec();

static void	get_defaults();

extern	long	strtol();

extern	struct	tm	*getdate();

extern	struct	passwd	*nis_getpwnam();

extern	int	lvlia(),
		lvlin(),
		isbusy(),
		getopt(),
		cremask(),
		uid_age(),
		rm_files(),
		chk_sysdir(),
		chk_subdir(),
		auditctl(),
		valid_uid(),
		check_perm(),
		edit_group(),
		all_numeric(),
		create_home(),
		mkdirp(),
		restore_ia(),
		valid_group(),
		valid_login(),
		valid_ndsname(),
		call_udelete(),
		**valid_lgroup(),
		ck_and_set_flevel();

extern	char	*optarg,			/* used by getopt */
		*errmsgs[],
		argvtostr();

extern	int	errno,
		optind,
		opterr,				/* used by getopt */
		getdate_err;

static	void	fatal(),
		rm_all(),
		clean_up(),
		bad_news(), 
		validate(),
		file_error(),
		no_service(),
		replace_all();

static	char	*pgenp,
		*logname,			/* login name to add */
		*getgrps_str(),
		*Dir = NULL;


static	struct	master	*masp,
			*mastp,
			*new_mastp;

static	struct	index	index,
			*indxp = &index;

static	struct	adtuser	adtuser,
			*adtp = &adtuser;

static	level_t	*lidlstp,
		lidlist[64];

static	int	mac = 0,
		plus = 0,
		audit = 0,
		minus = 0,
		cflag = 0,
		hflag = 0,
		pflag = 0,
		vflag = 0,
		**Gidlist,
		Gidcnt = 0,
		replace = 0,
		new_home = 0,
		call_mast = 0,
		call_pass = 0,
		call_shad = 0,
		call_hybrid = 0,
		new_groups = 0,
		new_master = 0,
		Nistononis = 0,
		Nisuser_op = 0,
		Nisplusfromnonis = 0,
		Nisminusfromnonis = 0;

static	long	lidcnt = 0;

static	gid_t	*Sup_gidlist;

static	int	rec_pwd(),
		ck_p_sz(),
		ck_s_sz(),
		getgrps(),
		mod_audit(),
		add_audit(),
		ck_def_lid(),
		mod_passwd(),
		mod_master(),
		mod_shadow(),
		mod_nwusers(),
		save_and_remove_crontab(),
		replace_crontab();

/*
 * The following variables can be supplied on the command line
 */
static	char	*Fpass = NULL,
		*Group = NULL,
		*Inact = NULL,
		*Ndsname = NULL,
		*Shell = NULL,
		*Expire = NULL,
		*Uidstr = NULL,
		*Comment = NULL,
		*Def_lvl = NULL,
		*usr_lvl = NULL,
		*Auditmask = NULL,
		*HomeMode = NULL,
		*new_logname = NULL;

int HomePermissions;
/*
 * The following variables are set in the get_defaults()
 * routine if the value is contained in the /etc/defaults
 * file.
*/
static	char	*def_Fpass = NULL,
		*def_Shell = NULL,
		*def_Inact = NULL,
		*def_Expire = NULL,
		*def_Def_lvl = NULL,
		*def_Groupid = NULL,
		*def_Homedir = NULL,
		*def_Skel_dir = NULL,
		*def_Auditmask = NULL;

uid_t		def_Uid = NULL;
char		tmpcronfile[32];  /* filename used to store crontab */

static int rm_index();
static int chownfiles();
static int rm_auditent();
static uid_t	get_uid();
static	struct	uid_blk	*uid_sp,
			*uid_sp2;
static	adtemask_t	emask;
/*
 * The following variable MUST be global so that it is
 * accessible to the external error printing routine "errmsg()".
 */
char	*msg_label = "UX:usermod";

/*
 * These strings are used as replacements in the ``usage'' line depending
 * on the features installed so they should never have their own catalog
 * index numbers.
 */
static	char
	*aud_str = " [ -a [operator1]event[,...]]",
	*mac_str = " [ -h [operator2]level [-h level [...]] ]\n               [-v def_level]";

/* NIS defines and declarations */
#define       HOMEDIR         "/home"
#define       DEF_LVL         "USER_LOGIN"
#define       HOME_MODE         "0755"
#define NIS_UID     (UID_MIN-1) /* this is probably better than 0 */
struct spwd	shad;
struct passwd	pass;
struct	passwd	nispw;
char	homedir[MAXPATHLEN];

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	i, ch, cll, uid = 0,
		mflag = 0, nflag = 0, oflag = 0;
	int	chownflag = 0;
	int	rval, cronrval;
	gid_t	gid = 1;
	uid_t	olduid, newuid;

	level_t	level;

	struct	spwd	shadow_st;
	struct	hybrid	hybrid_st;
	struct	passwd	passwd_st,
			pstruct;
	struct	passwd	*pstructp;

	char	*cmdlp,
		lvlfl[64],
		*grps = NULL,		/* multi groups from command line */
		*Mac_check = "SYS_PRIVATE";
	char	*chownlogname, *chownhomedir;
	int end_of_file=0;
	FILE *fp=NULL;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	opterr = 0;			/* no print errors from getopt */


	/*
	 * save command line arguments
	*/
	if ((cmdlp = (char *)argvtostr(argv)) == NULL) {
		adumprec(ADT_MOD_USR, 1, strlen(argv[0]), argv[0]);
                clean_up(1);
        }

	cll = (int) strlen(cmdlp);

	tzset();


	/*
	 * Determine if the MAC feature is installed.  If
	 * so, then  the ``lvlin()'' call  will return 0.
	*/
	if (lvlin(Mac_check, &level) == 0)
		mac = 1;
	/*
	 * Determine if the AUDIT feature is installed.
	 * If so, update the appropriate data files with
	 * audit information.
	*/
	if (access(AUDITMASK, EFF_ONLY_OK) == 0) {
		audit = 1;
	}

	errno = 0;

	while((ch = getopt(argc, argv, "c:d:e:f:G:g:l:n:mop:s:u:a:h:v:U")) != EOF)
		switch(ch) {
		case 'c':
			Comment = optarg;
			cflag = 1;
			break;
		case 'd':
			Dir = optarg;
			break;

		case 'e':
			Expire = optarg;
			break;

		case 'f':
			Inact = optarg;
			break;

		case 'G':
			grps = optarg;
			break;

		case 'g':
			Group = optarg;
			break;

		case 'l':
			new_logname = optarg;
			break;

		case 'm':
			mflag = 1;
			break;

		case 'n':
			nflag = 1;
			Ndsname = optarg;
			break;

		case 'o':
			oflag = 1;
			break;

		case 'p':
			if (strlen(optarg))
				Fpass = optarg;
			pflag = 1;
			break;

		case 's':
			Shell = optarg;
			break;

		case 'u':
			Uidstr = optarg;
			break;

		case 'U':
			chownflag = 1;
			break;

		case 'a':
			if (audit) {
				Auditmask = optarg;
			}
			else {
				no_service(M_NO_AUDIT, EX_SYNTAX, cll, cmdlp);
			}
			break;

		case 'h':
			if (mac) {
				usr_lvl = optarg;
				if (!*usr_lvl) {
					fatal(M_MUSAGE, cll, cmdlp, mac_str,
						(audit ? aud_str : ""));
				}
				call_pass = call_mast = 1;
				if (!lidcnt) {
					if (*usr_lvl == '+') {
						++plus;
						++usr_lvl;
					}
					else if (*usr_lvl == '-') {
						++minus;
						++usr_lvl;
					}
					else
						++replace;
				}
				if (lvlin(usr_lvl, &level) == 0)
					lidlist[lidcnt++] = level;
				else {
					if (lidcnt && ((*usr_lvl == '+') ||
						(*usr_lvl == '-'))) {

						errmsg(M_INVALID_LVL, "", "");
						fatal(M_MUSAGE, cll, cmdlp, mac_str,
							(audit ? aud_str : ""));
					}
					else {
						fatal(M_INVALID_LVL, cll, cmdlp, NULL, NULL);
					}
				}
				hflag = 1;
			}
			else {
				no_service(M_NO_MAC_H, EX_SYNTAX, cll, cmdlp);
			}
			break;

		case 'v':
			if (mac) {
				Def_lvl = optarg;
				if (!*Def_lvl) {
					fatal(M_MUSAGE, cll, cmdlp, mac_str,
						(audit ? aud_str : ""));
				}
				vflag = 1;
			}
			else {
				no_service(M_NO_MAC_V, EX_FAILURE, cll, cmdlp);
			}
			break;

		case '?':
			errmsg(M_MUSAGE, (mac ? mac_str : ""),
				(audit ? aud_str : ""));
			adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
			clean_up(EX_SYNTAX);
			break;
		}	/* end of switch */

	/*
	 * check syntax
	*/
	if (optind != argc - 1) {
		errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
		adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
		clean_up(EX_SYNTAX);
	}
	if ((uid && !oflag) || (mflag && !Dir)) {
		errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
		adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
		clean_up(EX_SYNTAX);
	}
	if (chownflag && Uidstr == NULL) {
		errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
		adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
		clean_up(EX_SYNTAX);
	}

	logname = argv[optind];

	 /*
	  * For Nis only allow the following: 
	  * user ---> +user (Nisplusfromnonis)
	  * user ---> -user (Nisminusfromnonis)
	  * +user ---> user (Nistononis)
	  * -user ---> user (Nistononis)
	  * +user (Nisuser_op) operation on +user
	  *
	  */
	if ((*new_logname == '+' && *logname == '+') || (*new_logname == '-' && *logname == '-') || (*new_logname == '+' && *logname == '-') || (*new_logname == '-' && *logname == '+')) {
		errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
		adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
		clean_up(EX_SYNTAX);
	}

	/*
	 * NIS processing: if changing "user" to a "+user" 
         * set Nisplusfromnonis=1
	 */
	if (strlen(new_logname) && *new_logname == '+'){

		if (*logname != '+' || *logname != '-'){
			Nisplusfromnonis = 1;
			/*
			 * Not allowed to specify Group and Uidstr the
			 * Nisplusfromnonis case
			 */
			if (strcmp((new_logname+1),logname)) {
				errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
				adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
				clean_up(EX_SYNTAX);
			}
			if (Group || Uidstr) {
				errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
				adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
				clean_up(EX_SYNTAX);
			}
		} 
	}

	/*
	 * NIS processing: if changing "user" to a "-user" 
         * set Nisminusfromnonis=1
	 */
	if (strlen(new_logname) && *new_logname == '-'){

		if (*logname != '+' || *logname != '-'){
			Nisminusfromnonis = 1;
			/*
			 * Not allowed to specify an options except '-l'
			 * for Nisminusfromnonis case
			 */
			if (strcmp((new_logname+1),logname)) {
				errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
				adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
				clean_up(EX_SYNTAX);
			}
			if (cflag || Dir || Expire || Inact || grps || mflag || pflag || Shell || Auditmask || hflag || vflag || Group || Uidstr) {
				errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
				adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
				clean_up(EX_SYNTAX);
			}
		} 
	}
	
	/*
	 * NIS processing: if changing "+/-user" to  "user" 
	 * set Nistononis=1, also detect if we of just
         * trying to modify a +user entry 
	 * (i.e., usermod -m -d /home/foo +user) then set Nisuser_op=1
	 */
	if (*logname == '+' || *logname == '-'){

		/*
		 * if this is true then we are changing an nis user
		 * to a non nis user.
	 	 */
		if (strlen(new_logname) && (*new_logname != '+' || *new_logname != '-')){
			Nistononis = 1;
			if (strcmp(new_logname,(logname+1))) {
				errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
				adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
				clean_up(EX_SYNTAX);
			}
		}
		else if (*logname == '-') {
			errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
			adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
			clean_up(EX_SYNTAX);
		} else if (!strlen(new_logname)) {
			Nisuser_op = 1;
	}
	}
	/*
	 * Only non NIS users  are in
	 * the index file
	 */
	if (!Nistononis && !Nisuser_op)
		(void) strcpy(indxp->name, logname);

	/*
	 * Set a lock on the password file so that no other
	 * process can update the "/etc/passwd" and "/etc/shadow"
	 * at the same time.  Also, ignore all signals so the
	 * work isn't interrupted by anyone.  In addition,
	 * check appropriate privileges.
	*/
	if (lckpwdf() != 0) {
		if (access(SHADOW, EFF_ONLY_OK|W_OK) != 0) {
			(void) pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			adumprec(ADT_MOD_USR, 1, cll, cmdlp);
			clean_up(1);
		} 
		errmsg (M_PASSWD_BUSY);
		adumprec(ADT_MOD_USR, 8, cll, cmdlp);
		clean_up(8);
	}
	for (i = 1; i < NSIG; ++i)
		if ( i != SIGCLD )
			(void) sigset(i, SIG_IGN);

	/*
	 * clear errno since it was set by some of the calls to sigset
	 */
	errno = 0;

 	if ((fp = fopen(PASSWD , "r")) == NULL){
               file_error(cll, cmdlp);
	}
	while (!end_of_file) {
		if ((pstructp=fgetpwent(fp)) != NULL){

			if(!strcmp(logname, pstructp->pw_name)){
				end_of_file=1;
				continue;
			}
			else
				continue;
		}
		else { /* EOF */
			if(errno == 0){
				end_of_file=1;
				errmsg(M_EXIST, logname);
				adumprec(ADT_MOD_USR, EX_NAME_NOT_EXIST, cll, cmdlp);
				clean_up(EX_NAME_NOT_EXIST);
			}
			else if(errno==EINVAL) {
				errno=0;
               			file_error(cll, cmdlp);
			}
		}
	}

	(void) fclose(fp);

	/* copy to real memory since this is in static area */
	pstruct.pw_name = strdup(pstructp->pw_name);
	pstruct.pw_passwd = strdup(pstructp->pw_passwd);
	pstruct.pw_uid = pstructp->pw_uid;
	pstruct.pw_gid = pstructp->pw_gid;
	pstruct.pw_age = strdup(pstructp->pw_age);
	pstruct.pw_comment = strdup(pstructp->pw_comment);
	pstruct.pw_gecos = strdup(pstructp->pw_gecos);
	pstruct.pw_dir = strdup(pstructp->pw_dir);
	pstruct.pw_shell = strdup(pstructp->pw_shell);

	(void) strcpy(homedir, pstruct.pw_dir);

	if (!Nistononis && !Nisuser_op) {
		if (getiasz(indxp) != 0) {
			errmsg(M_EXIST, logname);
			adumprec(ADT_MOD_USR, EX_NAME_NOT_EXIST, cll, cmdlp);
			clean_up(EX_NAME_NOT_EXIST);
		}
	} 

	mastp = (struct master *) malloc(indxp->length);
	memset(mastp, 0, sizeof(struct master));

	if (mastp == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":1341:failed malloc\n");
		adumprec(ADT_MOD_USR, 1, cll, cmdlp);
		clean_up(1);
	}
	
	if (!Nistononis && !Nisuser_op) {
		if (getianam(indxp, mastp) != 0) {
			(void) pfmt(stderr, MM_ERROR, ":1342:failed getianam\n");
			adumprec(ADT_MOD_USR, 1, cll, cmdlp);
			clean_up(1);
		}
	}

	if ((Dir && isbusy(logname))){
		errmsg(M_BUSY, logname, "change");
		adumprec(ADT_MOD_USR, EX_BUSY, cll, cmdlp);
		clean_up(EX_BUSY);
	}

	if (new_logname && strcmp(new_logname, logname)) {

		switch (valid_login(new_logname, (struct pwd **)NULL)) {
		case INVALID:
			fatal(M_INVALID, cll, cmdlp, new_logname, 
					"login name");
			/*NOTREACHED*/

		case NOTUNIQUE:
			if (!Nisplusfromnonis && !Nisminusfromnonis) {
				errmsg(M_USED, new_logname);
				adumprec(ADT_MOD_USR, EX_NAME_EXISTS, cll, cmdlp);
				clean_up(EX_NAME_EXISTS);
			}
			/* else fall through to default */
		default:
			if (Nistononis || Nisplusfromnonis || Nisminusfromnonis || Nisuser_op) {
				if (Nisplusfromnonis || Nisminusfromnonis) {
					hybrid_st.unix_namp = new_logname+1;
					call_hybrid = 1;
				}
				else if (Nistononis) {
					  hybrid_st.unix_namp = new_logname;
					  call_hybrid = 1;
				}
				passwd_st.pw_name = new_logname;
				shadow_st.sp_namp = new_logname;
				call_pass = 1;
				call_shad = 1;
			} else {
				call_mast = 1;
				call_shad = call_pass = call_hybrid = 1;
				passwd_st.pw_name = new_logname;
				shadow_st.sp_namp = new_logname;
				hybrid_st.unix_namp = new_logname;
				(void) strcpy(mastp->ia_name, new_logname);
			}
			break;
		}
		/*
		* even though the ``logname'' might be valid, check to
		* see if it's ALL numeric in which case print a 
		* diagnostic indicating that this might not be such 
		* a good idea.
		*/
		if (all_numeric(new_logname)) {
			(void) pfmt(stdout, MM_WARNING, errmsgs[M_NUMERIC],
				new_logname);
		}
		if (audit && !Nistononis && !Nisplusfromnonis && !Nisminusfromnonis && !Nisuser_op) {
			if (getadtnam(new_logname, adtp) == 0)
				fatal(M_USED, cll, cmdlp, new_logname, "");
		}
	}
	else {
		passwd_st.pw_name = logname;
		shadow_st.sp_namp = logname;
		hybrid_st.unix_namp = logname;
	}
	/*
	 * check and set the default values for new users
	 * and initialize both password structures with valid
	 * information for the new user
	*/
	if (Nisplusfromnonis || Nisminusfromnonis) {
	if( !grps )
		if ( Group )
			grps=getgrps_str(logname,(gid_t) atol(Group));
		else if ( new_logname )
			grps=getgrps_str(logname,pstruct.pw_gid);
	} /* ?? else {
		Group = NULL;
		Uidstr = NULL;
	}*/
	
	validate(&passwd_st, &shadow_st, &hybrid_st, &pstruct, cll, cmdlp,
				grps, gid, uid, mflag, nflag, oflag);

	/*
	 * If Nisplusfromnonis or Nisminusfromnonis 
         * make sure entry is not added to master file. 
	 * If Nistononis make sure entry is added to master file. 
	 */
	if (Nisplusfromnonis || Nisminusfromnonis) {
		call_mast = 0;
	}

	if (Nistononis || Nisuser_op) {
		new_master = 1;
		call_mast = 1;
	}
	
	/*
	 * For Nistononis and Nisplusfromnonis call special NIS versions
	 * of mod_passwd and mod_shadow. 
	 */
	if (Nistononis || Nisplusfromnonis || Nisminusfromnonis || Nisuser_op) { 
		if (call_pass && nismod_passwd(&passwd_st)) {
			file_error(cll, cmdlp);
		}
		if (call_shad && nismod_shadow(&shadow_st)) {
			file_error(cll, cmdlp);
		}
		/*
		 * If this is a Nisuser_op operation we will do
		 * the hybrid mapping if neccesary. Then clean up
		 * and done.
		 */
		if (Nisuser_op) {
			/* modify the user's mapping entry */
			if (call_hybrid && mod_nwusers(&hybrid_st)) {
				file_error(cll, cmdlp);
			}
			replace_all(cll, cmdlp);
			(void) ulckpwdf();
			adumprec(ADT_MOD_USR, 0, cll, cmdlp);
			clean_up(0);
		}
	} 
	else { /* normal non NIS case */
		if (call_pass && mod_passwd(&passwd_st)) {
			file_error(cll, cmdlp);
		}
		if (call_shad && mod_shadow(&shadow_st)) {
			file_error(cll, cmdlp);
		}
	}

	/* modify the user's mapping entry */
	if (call_hybrid && mod_nwusers(&hybrid_st)) {
		file_error(cll, cmdlp);
	}

	/*
	 * For Nis(plus/minus)fromnonis make sure user entry is 
	 * deleted from ia database
	 */

	if (Nisplusfromnonis || Nisminusfromnonis) {
		/*
		 * If the user had an audit entry, remove it from 
		 * the AUDITMASK file.
		*/
		if (audit) {
			if (rm_auditent(logname)) {
				file_error(cll, cmdlp);
			}
		}
		rm_index(logname);
		call_mast = 0;
	}
	/*
	 * update the audit file
	*/
	if (!Nisplusfromnonis && !Nisminusfromnonis) {
		if (!Nistononis) {
			if (audit && ((Auditmask && *Auditmask) || new_logname)) {
				if (mod_audit()) {
					file_error(cll, cmdlp);
				}
			}
		}
		else { /* Nistononis case */
			if (audit) {
				if (add_audit(new_logname)) {
					file_error(cll, cmdlp);
				}
			}
		}
	}
	/*
	 * set the master structure and call putiaent
	*/
	if (Nistononis) {
 		if (call_mast && nisadd_master(&pass, &shad)) {
 			file_error(cll, cmdlp);
		}
	} 
	else {
 		if (call_mast && mod_master()) {
 			file_error(cll, cmdlp);
		}
	}
	if (mac) {
		if (replace) {
			lidlstp = &lidlist[0];
		}
		else if (minus) {
			lidlstp = mastp->ia_lvlp;
			lidcnt = mastp->ia_lvlcnt;
		}
		if (vflag && !(plus || replace || minus)) {
			lidlstp = mastp->ia_lvlp;
			lidcnt = mastp->ia_lvlcnt;
		}
		if (new_logname || vflag || hflag) {
			if (lvlia(IA_WRITE, (level_t **)lidlstp,
				passwd_st.pw_name, &lidcnt)) {
				(void) pfmt(stderr, MM_ERROR, ":1343:can not update level file\n");
				file_error(cll, cmdlp);
			}
		}
		if (new_logname) {
			(void) strcpy(lvlfl, LVLDIR);
			(void) strcat(lvlfl, logname);
			(void) unlink(lvlfl);
		}
	}


	if (chownflag && Uidstr && *Uidstr) { 
		if (new_home) {
			chownhomedir = Dir;
		} else {
			chownhomedir = homedir;
		}
		rval = chownfiles(logname, pstruct.pw_uid, 
				  passwd_st.pw_uid, chownhomedir);
		/*
		 * Even if chownfiles() failed, we don't want to quit now 
		 * since we may have partially succeeded in changing files.
		 * If chownfiles() did fail, a message will be printed
		 * indicating so.
		 */

		/* use the old logname since it hasn't actually changed yet */
		cronrval = save_and_remove_crontab(logname);

		/*
		 * make sure new owner can read tmpcronfile since
		 * crontab -e does a setuid!
		 */
		if (cronrval == 0) {
			rval = chown(tmpcronfile, passwd_st.pw_uid, -1);
			if (rval < 0) {
				(void) chmod(tmpcronfile, 
					S_IRUSR|S_IRGRP|S_IROTH);
			}
			if (new_logname && *new_logname) {
				chownlogname = new_logname;
			} else {
				chownlogname = logname;
			}
		}
	}

	if (new_home){
		if ( chk_sysdir(pstruct.pw_dir) && chk_subdir(pstruct.pw_dir, Dir))
		  (void) rm_files(pstruct.pw_dir);
	}

	replace_all(cll, cmdlp);

	(void) ulckpwdf();

	if (cronrval == 0) {
		/* use the new logname since it has changed at this point */
		(void) replace_crontab(chownlogname);
	}

	adumprec(ADT_MOD_USR, 0, cll, cmdlp);

	clean_up(0);

	/*NOTREACHED*/
}


/*
 * Procedure:	ck_p_sz
 *
 * Notes:	Check for the size of the whole passwd entry
 */
static	int
ck_p_sz(pwp)
	struct	passwd	*pwp;
{
	char ctp[128];

	/*
	 * Ensure that the combined length of the individual
	 * fields will fit in a passwd entry. The 1 accounts for the
	 * newline and the 6 accounts for the colons (:'s)
	*/
	if ((int) (strlen(pwp->pw_name) + 1 +
		(int) sprintf(ctp, "%ld", pwp->pw_uid) +
		(int) sprintf(ctp, "%ld", pwp->pw_gid) +
		(int) strlen(pwp->pw_comment) +
		(int) strlen(pwp->pw_dir)	+
		(int) strlen(pwp->pw_shell) + 6) > (ENTRY_LENGTH - 1)) {

		(void) pfmt(stderr, MM_ERROR, "New password entry too long");
		return 1;
	}
	return 0;
}


/*
 * Procedure:	ck_s_sz
 *
 * Notes:	Check for the size of the whole shadow entry
 */
static	int
ck_s_sz(ssp)
	struct	spwd	*ssp;
{
	char ctp[128];

	/*
	 * Ensure that the combined length of the individual
	 * fields will fit in a shadow entry. The 1 accounts for the
	 * newline and the 7 accounts for the colons (:'s)
	*/
	if ((int) (strlen(ssp->sp_namp) + 1 +
		(int) strlen(ssp->sp_pwdp) +
		(int) sprintf(ctp, "%ld", ssp->sp_lstchg) +
		(int) sprintf(ctp, "%ld", ssp->sp_min) +
		(int) sprintf(ctp, "%ld", ssp->sp_max) + 
		(int) sprintf(ctp, "%ld", ssp->sp_warn) + 
		(int) sprintf(ctp, "%ld", ssp->sp_inact) + 
		(int) sprintf(ctp, "%ld", ssp->sp_expire) + 7) > (ENTRY_LENGTH - 1)) {

		(void) pfmt(stderr, MM_ERROR, "New password entry too long");
		return 1;
	}
	return 0;
}


/*
 * Procedure:	file_error
 *
 * Notes:	issues a diagnostic message, removes all temporary files
 *		(unconditionally), allows updtate access to the password
 *		files again, writes an ausit record, and calls clean_up().
 */
static	void
file_error(cmdll, cline)
	int	cmdll;
	char	*cline;
{
	errmsg (M_PASSWD_UNCHANGED);
	rm_all();
	(void) ulckpwdf();
	adumprec(ADT_MOD_USR, 6, cmdll, cline);
	clean_up(6);
}


/*
 * Procedure:	bad_news
 *
 * Notes:	Exactly the same functionality as ``file_error'' above
 *		but it prints a different diagnostic message and status
 *		number.
 */
static	void
bad_news(cmdll, cline)
	int	cmdll;
	char	*cline;
{
	errmsg (M_PASSWD_MISSING);
	rm_all();
	(void) ulckpwdf();
	adumprec(ADT_MOD_USR, 7, cmdll, cline);
	clean_up(7);
}


/*
 * Procedure:	fatal
 *
 * Notes:	prints out diagnostic message, writes an audit record,
 *		and calls clean_up() with the given exit value.
 */
static	void
fatal(msg_no, cmdll, cline, arg1, arg2)
	int	msg_no,
		cmdll;
	char	*cline,
		*arg1,
		*arg2;
{
	errmsg(msg_no, arg1, arg2);
	adumprec(ADT_MOD_USR, EX_BADARG, cmdll, cline);
	(void) ulckpwdf();
	clean_up(EX_BADARG);
}


/*
 * Procedure:	mod_passwd
 *
 * Notes:	modify user attributes in "/etc/passwd" file
 */
static	int
mod_passwd(passwd)
	struct	passwd	*passwd;
{
	struct	stat	statbuf;
	struct	passwd	*pw_ptr1p;
	FILE		*fp_temp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;

	errno = 0;

	if (stat(PASSWD, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & (S_IRUSR|S_IRGRP|S_IROTH)));

 	if ((fp_temp = fopen(PASSTEMP , "w")) == NULL)
		return errno;

	/*
	 * open the password file.
	*/
	nis_setpwent();
	/*
	 * The while loop for reading PASSWD entries
	*/
	while (!end_of_file) {
		if ((pw_ptr1p = (struct passwd *) nis_getpwent()) != NULL) {
			if (!strcmp(pw_ptr1p->pw_name, logname)) {
				found = 1;
				if (new_logname && *new_logname)
					pw_ptr1p->pw_name = passwd->pw_name;
				if (Uidstr && *Uidstr)
					pw_ptr1p->pw_uid = passwd->pw_uid;
				if (Group && *Group)
					pw_ptr1p->pw_gid = passwd->pw_gid;
				if (cflag) {
					pw_ptr1p->pw_gecos = passwd->pw_gecos;
					pw_ptr1p->pw_comment = passwd->pw_comment;
				}
				if (Dir && *Dir)
					pw_ptr1p->pw_dir = passwd->pw_dir;
				if (Shell && *Shell) {
					pw_ptr1p->pw_shell = passwd->pw_shell;
				}
				/*
				 * check the size of the password entry.
				*/
				if (ck_p_sz(pw_ptr1p)) {
					nis_endpwent();
					(void) fclose(fp_temp);
					return 1;
				}
			}
			if (putpwent(pw_ptr1p, fp_temp)) {
				nis_endpwent();
				(void) fclose(fp_temp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)			 /* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip*/
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	}

	/*
	 * close the password file
	*/
	nis_endpwent();

	(void) fclose(fp_temp);

	if (error >= 1) {
		errmsg (M_BADENTRY,PASSWD);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, PASSTEMP))
		return errno;

	if (chown(PASSTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}


/*
 * Procedure:	mod_shadow
 *
 * Notes:	modify user attributes in "/etc/shadow" file
 */
static	int
mod_shadow(shadow)
	struct	spwd	*shadow;
{
	struct	stat	statbuf;
	struct	spwd	*sp_ptr1p;
	FILE		*fp_temp;
	FILE		*fp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;

	errno = 0;

	if (stat(SHADOW, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(SHADTEMP , "w")) == NULL)
		return errno;

 	if ((fp = fopen(SHADOW , "r")) == NULL)
		return errno;
	/*
	 * The while loop for reading SHADOW entries
	*/
	while (!end_of_file) {
		if ((sp_ptr1p = (struct spwd *)fgetspent(fp)) != NULL) {
			if (!strcmp(logname, sp_ptr1p->sp_namp)) {
				found = 1;
				if (new_logname && *new_logname)
					sp_ptr1p->sp_namp = shadow->sp_namp;
				if (Inact && *Inact)
			      		sp_ptr1p->sp_inact = shadow->sp_inact;
				if (Expire && *Expire)
			      		sp_ptr1p->sp_expire = shadow->sp_expire;
				if (pflag) {
					if (Fpass && strlen(Fpass)) {
						sp_ptr1p->sp_flag &=
							(unsigned) 0xffffff00;
						sp_ptr1p->sp_flag |=
							(unsigned) pgenp[0];
					}
					else
						sp_ptr1p->sp_flag &=
							(unsigned) 0xffffff00;
				}
				/*
	 			* check the size of the shadow entry.
				*/
				if (ck_s_sz(sp_ptr1p)) {
					(void) fclose(fp_temp);
					(void) fclose(fp);
					return 1;
				}
			}
			if (putspent(sp_ptr1p, fp_temp)) {
				(void) fclose(fp_temp);
				(void) fclose(fp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)			/* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip */
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	}

	/*
	 * close the shadow file
	*/

	(void) fclose(fp);
	(void) fclose(fp_temp);

	if (error >= 1) {
		errmsg (M_BADENTRY,SHADOW);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, SHADTEMP))
		return errno;

	if (chown(SHADTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}

/*
 * Procedure:	mod_nwusers
 *
 * Notes:	modify hybrid user mapping entry in "/etc/netware/nwusers" file
 */
static	int
mod_nwusers(hybrid)
	struct	hybrid	*hybrid;
{
	struct	stat	statbuf;
	struct	hybrid	*np_ptr1p;
	FILE		*fp_temp;
	FILE		*fp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;
	char 		*real_logname = logname;

	errno = 0;

	if (stat(NWUSERS, &statbuf) < 0) 
		return error;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(NWTEMP , "w")) == NULL)
		return errno;

 	if ((fp = fopen(NWUSERS , "r")) == NULL)
		return errno;

	if (Nistononis == 1 || Nisuser_op == 1)
		real_logname = logname+1;
	/*
	 * The while loop for reading SHADOW entries
	*/
	while (!end_of_file) {
		if ((np_ptr1p = (struct hybrid *)fgetnwent(fp)) != NULL) {
			if (!strcmp(real_logname, np_ptr1p->unix_namp)) {
				found = 1;
				if (new_logname && *new_logname) 
					np_ptr1p->unix_namp = hybrid->unix_namp;
				if (Ndsname && *Ndsname) 
					np_ptr1p->nds_namp = hybrid->nds_namp;
				else
				   if (Ndsname)
				       continue;
			}
			if (fputnwent(np_ptr1p, fp_temp)) {
				(void) fclose(fp_temp);
				(void) fclose(fp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)			/* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip */
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	} 

	/* If we hit the end of the file and couldn't find the 
	* entry in nwusers, we append the new entry to the end of
	* the file.
	*/

	if ( !found && end_of_file) {
		np_ptr1p = (struct hybrid *) malloc(sizeof(struct hybrid) + 1);
		if (new_logname && *new_logname) 
			np_ptr1p->unix_namp = hybrid->unix_namp;
	  	else	
			np_ptr1p->unix_namp = real_logname;
		if (Ndsname && *Ndsname) 
  			 np_ptr1p->nds_namp = hybrid->nds_namp;
		else
   			if (Ndsname)
      			goto end; 

		if (fputnwent(np_ptr1p, fp_temp)) {
			(void) fclose(fp_temp);
			(void) fclose(fp);
			return 1;
		}
 	}

	/*
	 * close the nwusers file
	*/

end:
	(void) fclose(fp);
	(void) fclose(fp_temp);

	if (error >= 1) {
		errmsg(M_NW_BADENTRY,NWUSERS);
	}

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, NWTEMP))
		return errno;

	if (chown(NWTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}
	if (chmod(NWTEMP, statbuf.st_mode) < 0) {
		return errno;
	}

	return error;
}

/*
 * Procedure:	mod_audit
 *
 * Notes:	modifies user's audit record if auditing is installed.
 */
static	int
mod_audit()
{
	struct	stat	statbuf;
	struct	adtuser	adtuser;
	struct	adtuser	*adtup = &adtuser;
	FILE		*fp_temp;
	register int	ret = 0,
			found = 0,
			i, error = 0,
			end_of_file = 0;

	errno = 0;

	if (stat(AUDITMASK, &statbuf) < 0)
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(MASKTEMP, "w")) == NULL) {
		return errno;
	}

	/*
	 * open the audit file
	*/
	setadtent();

	while (!end_of_file) {
		if ((ret = getadtent(adtup)) == 0) {
			if (!strcmp(logname, adtup->ia_name)) {
				found = 1;
				if (new_logname && *new_logname)
					(void) strcpy(adtup->ia_name, adtp->ia_name);
				for (i = 0; i < ADT_EMASKSIZE; ++i)
					adtup->ia_amask[i] = adtp->ia_amask[i];
			}
			if (putadtent(adtup, fp_temp)) {
				(void) fclose(fp_temp);
				endadtent();
				return 1;
			}
		}
		else {
			if (ret == -1) 
				end_of_file = 1;
			else if (errno == EINVAL){	/*bad entry found, skip */
				error++;
				errno = 0;
			} else		/* unexpected error found */
				end_of_file = 1;
		}	
	}
	/*
	 * close the audit file
	*/
	endadtent();

	(void) fclose(fp_temp);

	if (error >= 1) {
		errmsg (M_AUDIT_BADENTRY);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, MASKTEMP))
		return errno;

	if (chown(MASKTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}


/*
 * Procedure:	rm_all
 *
 * Notes:	does an unconditional unlink (without error checking)
 *		for all temporary files created by usermod.
 */
static	void
rm_all()
{
	(void) unlink(SHADTEMP);
	(void) unlink(PASSTEMP);
	(void) unlink(MASKTEMP);
}


/*
 * Procedure:	replace_all
 *
 * Notes:	renames all the REAL files created to the OLD file,
 * 		renames all the TEMP files created to the REAL file.
 */
static	void
replace_all(cll, cmdlp)
	int	cll;
	char	*cmdlp;
{
	if (call_pass) {
		if (rename(PASSWD, OPASSWD) == -1)
			file_error(cll, cmdlp);

		if (rename(PASSTEMP, PASSWD) == -1) {
			if (link (OPASSWD, PASSWD))
				bad_news(cll, cmdlp);
			file_error(cll, cmdlp);
		}
	}
	if (call_shad) {
		if (access(OSHADOW, 0) == 0) {
			if (rename(SHADOW, OSHADOW) == -1) {
				if (rec_pwd()) 
					bad_news(cll, cmdlp);
				else file_error(cll, cmdlp);
			}
		}
		
		if (rename(SHADTEMP, SHADOW) == -1) {
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news(cll, cmdlp);
			if (rec_pwd()) 
				bad_news(cll, cmdlp);
			else file_error(cll, cmdlp);
		}
	}
	if (call_hybrid) {
		if (rename(NWUSERS, ONWUSERS) == -1) {
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news(cll, cmdlp);
			if (rec_pwd()) 
				bad_news(cll, cmdlp);
			else file_error(cll, cmdlp);
		}
		
		if (rename(NWTEMP, NWUSERS) == -1) {
			if (rename(ONWUSERS, NWUSERS) == -1)
				bad_news(cll, cmdlp);
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news(cll, cmdlp);
			if (rec_pwd()) 
				bad_news(cll, cmdlp);
			else file_error(cll, cmdlp);
		}
	}
	if (audit && ((Auditmask && *Auditmask) || new_logname)) {
		if (access(OAUDITMASK, 0) == 0) {
			if (rename(AUDITMASK, OAUDITMASK) == -1) {
				(void) unlink(MASKTEMP);
				if (rename(OSHADOW, SHADOW) == -1)
					bad_news(cll, cmdlp);
				if (rec_pwd()) 
					bad_news(cll, cmdlp);
				else file_error(cll, cmdlp);
			}
		}

		if (rename(MASKTEMP, AUDITMASK) == -1) {
			(void) unlink(MASKTEMP);
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news(cll, cmdlp);
			if (rec_pwd()) 
				bad_news(cll, cmdlp);
			else file_error(cll, cmdlp);
		}
	}
	if (Nisplusfromnonis || Nisminusfromnonis) {
		if (access(OINDEX, 0) == 0) {
			if (rename(INDEX, OINDEX) == -1) {
				if (rename(OSHADOW, SHADOW) == -1)
					bad_news();
				if (rec_pwd()) 
					bad_news();
				else file_error(errno);
			}
		}
		if (access(TMPINDEX, 0) == 0) {
			if (rename(TMPINDEX, INDEX) == -1) {
				if (rename(OINDEX, INDEX) == -1)
					bad_news();
				if (rename(OSHADOW, SHADOW) == -1)
					bad_news();
				if (rec_pwd()) 
					bad_news();
				else {
					file_error(errno);
				}
			}
		}
	} /* end if Nisplusfromnonis/Nisminusfromnonis */
	restore_ia(); /* Restore attributes on master and index files*/
	return;
}


/*
 * Procedure:	no_service
 *
 * Notes:	prints out three error messages indicating that this
 *		particular service was not available at this time,
 *		writes an audit record, and calls clean_up() with the
 *		value passed as exit_no.
 */
static	void
no_service(msg_no, exit_no, cll, cline)
	int	msg_no,
		exit_no,
		cll;
	char	*cline;
{
	errmsg(msg_no);
	errmsg(M_NO_SERVICE);
	errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
	adumprec(ADT_MOD_USR, exit_no, cll, cline);
	clean_up(exit_no);
}

/*
 * Procedure:	validate
 *
 * Notes:	this routine sets the values in the password structures
 *		for any value that can be specified via the command line.
 *		This is where ALL validation for those values takes place.
 */
static	void
validate(pass, shad, hybrid, o_pass, cll, cmdlp, grps, gid, uid, mflg, nflg, oflg)
	struct	passwd	*pass;
	struct	spwd	*shad;
	struct	hybrid	*hybrid;
	struct	passwd	*o_pass;
	int		cll;
	char		*cmdlp;
	char		*grps;
	gid_t		gid;
	int		uid,
			mflg,
			oflg;
{
	register int	i, j, k,
			ret = 0,
			dupcnt = 0,
			existed = 0,
			use_skel = 0;
	struct	stat	statbuf, statbuf2;
	struct	group	*g_ptr;
	char		*ptr,
			*s_dir = NULL,
			lvlname[LVL_MAXNAMELEN + 1];
	struct	tm	*tm_ptr;
	long		lvlcnt,
			date = 0,
			inact = 0;
	level_t		level,
			*lvlp,
			*nlvlp,
			*ia_lvlp,
			def_lid = *mastp->ia_lvlp;
	uinfo_t		uinfo;
	struct	passwd	nispwd;


	Gidlist = (int **)0;
	Sup_gidlist = (gid_t *) malloc((sysconf(_SC_NGROUPS_MAX) * sizeof(gid_t)));

	if (mac) {
		if (hflag) {
			j = lidcnt;
			/*
			 * check to see if there were any duplicate
			 * levels passed as arguments via the -h option.
			*/
			for (i = 0; i < lidcnt; ++i) {
				for (k = (i + 1); k < lidcnt; ++k) {
					if (lidlist[i] && lidlist[i] == lidlist[k]) {
						--j;
						lidlist[k] = 0;
					}
				}
			}
			lidcnt = j;
			/*
			 * sort the lidlist
			*/
			for (i = 0; i < lidcnt; ++i) {
				if (!lidlist[i]) {
					if (((i + 1) < lidcnt) && lidlist[i + 1]) {
						lidlist[i] = lidlist[i + 1];
						--j;
					}
				}
			}
			lidcnt = j;
			/*
			 * add levels
			*/
			if (plus) {
				/*
			 	 * remove duplicates
				*/
				for (i = 0; i < lidcnt; i++) {
					lvlp = mastp->ia_lvlp;
					for (j = 0; j < mastp->ia_lvlcnt; j++) {
						if (lidlist[i] == *lvlp++) {
							++dupcnt;
							lidlist[i] = 0;
						}
					}
				}
				j = dupcnt;
				for (i = 0; j; i++) {
					if (lidlist[i] == 0) {
						--j;
						for (k = i; k < lidcnt; k++) { 
							lidlist[k] = lidlist[k + 1];
						}
					}
				}
				lidcnt -= dupcnt;

				lidlstp = (level_t *)calloc((lidcnt+mastp->ia_lvlcnt),
						sizeof(level_t));
				if (lidlstp == NULL) {
					(void) pfmt(stderr, MM_ERROR, ":1344: failed calloc\n");
					adumprec(ADT_MOD_USR, 1, cll, cmdlp);
					exit(1);
				}
				lvlp = mastp->ia_lvlp;
				nlvlp = lidlstp;
				for (i = 0; i < mastp->ia_lvlcnt; i++) 
					*nlvlp++ = *lvlp++;
				for (i = 0; i < lidcnt; i++) 
					*nlvlp++ = lidlist[i];
				lidcnt += mastp->ia_lvlcnt;
			}
			/*
			 * delete levels
			*/
			if (minus) {
				for (i = 0; i < lidcnt; i++) {
					lvlp = mastp->ia_lvlp;
					nlvlp = mastp->ia_lvlp;
					for (j = 0; j < mastp->ia_lvlcnt; j++, lvlp++) {
						if (lidlist[i] == *lvlp) {
							nlvlp = lvlp;
							for (k = j; k < mastp->ia_lvlcnt; k++) 
								*lvlp++ = *++nlvlp;
							mastp->ia_lvlcnt--;
							break;
						}
					}
				}
			}
		}
		/*
		 * if the -v option was present on the command line,
		 * check the validity of the value now.
		*/
		if (vflag) {
			if (lvlin(Def_lvl, &level) == 0) 
				def_lid = level;
			else {
				fatal(M_INVALID_DLVL, cll, cmdlp, "", "");
			}
		}
		/*
		 * general processing if either MAC-related flag was
		 * specified on the command line.
		*/
		if (hflag || vflag) {
			if (replace) {
				lidlstp = &lidlist[0];
			}  
			else if (minus) {
				lidlstp = mastp->ia_lvlp;
				lidcnt = mastp->ia_lvlcnt;
			}
			if (vflag && !(plus || replace || minus)) {
				lidlstp = mastp->ia_lvlp;
				lidcnt = mastp->ia_lvlcnt;
			}
			if (ck_def_lid(lidlstp, lidcnt, def_lid)) {
				if (minus) {
					lvlout(&def_lid, &lvlname[0],
						MAXNAMELEN + 1, LVL_FULL);
					fatal(M_NO_DEFLVL, cll, cmdlp, lvlname, "");
				}
				fatal(M_INVALID_DLVL, cll, cmdlp, "", "");
			}
			if (vflag || ((lidcnt > mastp->ia_lvlcnt) && replace)
				|| (lidcnt && plus)) {

				call_mast = 1;
				new_master = 1;
			}
		}
	}

	if (Uidstr) {
		/* convert Uidstr to integer */
		uid = (int) strtol(Uidstr, &ptr, (int) 10);
		if (*ptr) {
			fatal(M_INVALID, cll, cmdlp, Uidstr, "uid");
		}
		if (uid_age(CHECK, uid, 0) == 0) {
			errmsg(M_UID_AGED, uid);
			adumprec(ADT_ADD_USR, EX_ID_EXISTS, cll, cmdlp);
			clean_up(EX_ID_EXISTS);
		}

		if (uid != o_pass->pw_uid) {
			switch (valid_uid(uid, NULL)) {
			case NOTUNIQUE:
				if (!oflg) {
					/* override not specified */
					errmsg(M_UID_USED, uid);
					adumprec(ADT_MOD_USR, EX_ID_EXISTS, cll, cmdlp);
					clean_up(EX_ID_EXISTS);
				}
				if (uid <= DEFRID)
				/* FALLTHROUGH */
			case RESERVED:
				(void) pfmt(stdout, MM_WARNING,
					errmsgs[M_RESERVED], uid);
				break;
			case TOOBIG:
				fatal(M_TOOBIG, cll, cmdlp, "uid", Uidstr);
				break;
			case INVALID:
				fatal(M_INVALID, cll, cmdlp, Uidstr, "uid");
				break;
			}
			call_mast = 1;
			call_pass = 1;
			pass->pw_uid = mastp->ia_uid = uid;
		}
		else
			Uidstr = NULL;
	}
	else {
		uid = o_pass->pw_uid;
	}

	if (Group) {
		switch (valid_group(Group, &g_ptr)) {
		case INVALID:
			fatal(M_INVALID, cll, cmdlp, Group, "group id");
			/*NOTREACHED*/
		case TOOBIG:
			fatal(M_TOOBIG, cll, cmdlp, "gid", Group);
			/*NOTREACHED*/
		case UNIQUE:
			errmsg(M_GRP_NOTUSED, Group);
			adumprec(ADT_MOD_USR, EX_NAME_NOT_EXIST, cll, cmdlp);
			clean_up(EX_NAME_NOT_EXIST);
			/*NOTREACHED*/
		}

		gid = g_ptr->gr_gid;

		if (gid == o_pass->pw_gid)
			Group = NULL;
		else {
			call_mast = 1;
			call_pass = 1;
			new_groups = 1;
			pass->pw_gid = mastp->ia_gid = gid;
		}
	}
	else {
		gid = o_pass->pw_gid;
	}
	/*
	 * put user's primary group into list so its added to master
	 * file information later (if necessary).
	*/
	Gidcnt = getgrps(pass->pw_name, gid);

	if (grps ) {
		if (Gidlist = valid_lgroup(grps, gid)) {
			Gidcnt = i = 0;
			Sup_gidlist[Gidcnt++] = gid;
			while ((unsigned)Gidlist[i] != -1)
				Sup_gidlist[Gidcnt++] = (gid_t) Gidlist[i++];
		}
		else {
			adumprec(ADT_MOD_USR, EX_BADARG, cll, cmdlp);
			clean_up(EX_BADARG);
		}
		/*
		 * redefine login's supplentary group memberships
		*/
		if (!Nistononis && !Nisplusfromnonis && !Nisminusfromnonis && !Nisuser_op) {
			ret = edit_group(logname, new_logname, Gidlist, 1);
		}

		if (ret != EX_SUCCESS) {
			errmsg(M_UPDATE, "modified");
			adumprec(ADT_ADD_USR_GRP, ret, cll, cmdlp);
			clean_up(ret);
		}
		else {
			adumprec(ADT_ADD_USR_GRP, ret, cll, cmdlp);
		}

		if (Gidcnt > mastp->ia_sgidcnt) {
			new_master = 1;
		}
		call_mast = 1;
		new_groups = 1;
	} 

	if (Dir) {
		if (REL_PATH(Dir)) {
			fatal(M_RELPATH, cll, cmdlp, Dir, "");
		}
		if (INVAL_CHAR(Dir)) {
			fatal(M_INVALID, cll, cmdlp, Dir, "directory name");
		}
		if (strcmp(o_pass->pw_dir, Dir) != 0) {
			/*
			 * ignore -m option if ``Dir'' and
			 * o_pass->pw_dir are the same directory
			 * i.e. one is a symbolic link to the other.
			 */
			if(stat(Dir, &statbuf2) == 0
			&& stat(o_pass->pw_dir, &statbuf) == 0
			&& statbuf.st_dev == statbuf2.st_dev
			&& statbuf.st_ino == statbuf2.st_ino)
				mflg = 0;
			pass->pw_dir = Dir;
			if ((int)(strlen(Dir)) > (int)mastp->ia_dirsz )
				new_master = 1;
			else
				(void) strcpy(mastp->ia_dirp, Dir);

			mastp->ia_dirsz = strlen(Dir);
			call_mast = 1;
			call_pass = 1;
		}
		else {
			if (stat(o_pass->pw_dir, &statbuf) == 0) {
				/*
				 * ignore -m option if ``Dir'' is not
				 * different and the directory existed.
				 */
				mflg = 0;
				Dir = NULL;
			}
			use_skel = 1;
		}
	}
	if (mflg) {
		if (Nisplusfromnonis || Nisminusfromnonis || Nisuser_op) {
			int niserr = 0;
			if (!Nisuser_op)
				niserr=nis_getuser((Nisplusfromnonis ? logname : new_logname), &nispwd);
			else 
				niserr=nis_getuser(logname+1, &nispwd);
			/* 
			 * If creating an NIS user we must be 
			 * able to verify that
			 * user is in NIS map. So fail if we can not.
			 */
			if(niserr)
				if (Nisplusfromnonis || Nisuser_op) {
					switch (niserr) {
						case YPERR_YPBIND:
							errmsg(M_NO_NIS, logname);
							adumprec(ADT_MOD_USR, EX_NO_NIS, cll, cmdlp);
							clean_up(EX_NO_NIS);
							break;
						case YPERR_KEY:
							errmsg(M_NO_NISMATCH, logname);
							adumprec(ADT_MOD_USR, EX_NO_NISMATCH, cll, cmdlp);
							clean_up(EX_NO_NISMATCH);
							break;
						default:
							errmsg(M_UNK_NIS, logname);
							adumprec(ADT_MOD_USR, EX_UNK_NIS, cll, cmdlp);
							clean_up(EX_UNK_NIS);
								break;
					}
				}
			if (stat(Dir, &statbuf) == 0) {
				/* Home directory exists */
				if (check_perm(statbuf, nispwd.pw_uid, 
				   nispwd.pw_gid, S_IWOTH|S_IXOTH) != 0) {
					errmsg(M_NO_PERM, logname, Dir);
					adumprec(ADT_MOD_USR, EX_NO_PERM, cll, cmdlp);
					clean_up(EX_NO_PERM);
				}
				existed = 1;
			}
			if (stat(o_pass->pw_dir, &statbuf) < 0) {
				if (ia_openinfo(o_pass->pw_name, &uinfo) || (uinfo == NULL)) {
					statbuf.st_level = 0;
				}
				else {
					if (ia_get_lvl(uinfo, &ia_lvlp, &lvlcnt)) {
						statbuf.st_level = 0;
					}
					else {
						statbuf.st_level = *ia_lvlp;
					}
					ia_closeinfo(uinfo);	/* close master file */
				}
			}
			else {
				s_dir = o_pass->pw_dir;
			}
			if (use_skel) {
				existed = 1;
				s_dir = SKEL_DIR;
			}
			if(!Nisuser_op)
				ret = create_home(Dir, s_dir, logname, existed, nispwd.pw_uid, nispwd.pw_gid, statbuf.st_level,HomePermissions);
			else
				ret = create_home(Dir, s_dir, logname+1, existed, nispwd.pw_uid, nispwd.pw_gid, statbuf.st_level,HomePermissions);
	
			if (ret == EX_SUCCESS) {
				new_home = 1;
			}
			else {
				if (!existed) {
					(void) rm_files(Dir);
				}
				adumprec(ADT_MOD_USR, M_NOSPACE, cll, cmdlp);
				clean_up(EX_NOSPACE);
			}
		} /*  end Nisplusfromnonis */
		if (!Nisplusfromnonis && !Nisminusfromnonis && !Nisuser_op) {
			if (stat(Dir, &statbuf) == 0) {
				/* Home directory exists */
				if (check_perm(statbuf, o_pass->pw_uid, 
				   o_pass->pw_gid, S_IWOTH|S_IXOTH) != 0) {
					errmsg(M_NO_PERM, logname, Dir);
					adumprec(ADT_MOD_USR, EX_NO_PERM, cll, cmdlp);
					clean_up(EX_NO_PERM);
				}
				existed = 1;
			}
			if (stat(homedir, &statbuf) < 0) {
				if (ia_openinfo(o_pass->pw_name, &uinfo) || (uinfo == NULL)) {
					statbuf.st_level = 0;
				}
				else {
					if (ia_get_lvl(uinfo, &ia_lvlp, &lvlcnt)) {
						statbuf.st_level = 0;
					}
					else {
						statbuf.st_level = *ia_lvlp;
					}
					ia_closeinfo(uinfo);	/* close master file */
				}
			}
			else {
				s_dir = homedir;
			}
			if (use_skel) {
				existed = 1;
				s_dir = SKEL_DIR;
			}
			ret = create_home(Dir, s_dir, logname, existed, uid,
				gid, statbuf.st_level,HomePermissions);
	
			if (ret == EX_SUCCESS) {
				new_home = 1;
			}
			else {
				if (!existed) {
					(void) rm_files(Dir);
				}
				adumprec(ADT_MOD_USR, M_NOSPACE, cll, cmdlp);
				clean_up(EX_NOSPACE);
			}
		} /*  end !Nisplusfromnonis */
	}
	if (Shell) {
		int	abits = EFF_ONLY_OK | EX_OK | X_OK;

		if (REL_PATH(Shell))
			fatal(M_RELPATH, cll, cmdlp, Shell, "");
		if (INVAL_CHAR(Shell)) {
      			fatal(M_INVALID, cll, cmdlp, Shell, "shell name"); 
		}
		if (strcmp(o_pass->pw_shell, Shell) != 0) {
			/*
			 * check that shell is an executable file
			*/
			if (access(Shell, abits) != 0)
				fatal(M_INVALID, cll, cmdlp, Shell, "shell");
			else {
				pass->pw_shell = Shell;
				if ((int) strlen(Shell) > (int) mastp->ia_shsz)
					new_master = 1;
				else
					(void) strcpy((char *)mastp->ia_shellp, Shell);
				mastp->ia_shsz = strlen(Shell);
				call_mast = 1;
				call_pass = 1;
			}
		}
		else {
			/*
			 * ignore -s option if shell is not different
			*/
			Shell = NULL;
		}
	}

	if (cflag) {
		if (Comment) {
			if ((int)(strlen(Comment)) > CMT_SIZE)
				fatal(M_INVALID, cll, cmdlp, Comment, "Comment");
			if (INVAL_CHAR(Comment)) {
				fatal(M_INVALID, cll, cmdlp, Comment, "comment");
			}
			/*
			 * ignore comment if its not different from passwd entry
			*/
			if (!strcmp(o_pass->pw_comment, Comment))
				cflag = 0;

		}
		if (cflag) {		/* not ignored above */
			call_pass = 1;
			pass->pw_comment = pass->pw_gecos = Comment;
		}
	}
	/*
	 * inactive string is a positive integer
	*/
	if (Inact) {
		/* convert Inact to integer */
		inact = (int) strtol(Inact, &ptr, (int) 10);
		if (*ptr || inact < -1) {
			fatal(M_INVALID, cll, cmdlp, Inact, "inactivity string");
		}
		call_mast = 1;
		call_shad = 1;
		shad->sp_inact = mastp->ia_inact = inact;
	}

	/*
	 * Set the NDS name in hybrid structure.
	*/
	if (nflg) {
	   if (Ndsname && *Ndsname) {
		/*
 		 * the valid_ndsname routine checks to see if the ndsname is
		 * in "/etc/netware/nwusers" file
		*/
		switch (valid_ndsname(Ndsname)) {
		case INVALID:
			errmsg(M_INVALID, Ndsname, "Netware login name");
			adumprec(ADT_MOD_USR, EX_NDSNAME_INVALID, cll, cmdlp);
			(void) ulckpwdf();
			clean_up(EX_NDSNAME_INVALID);
			/*NOTREACHED*/

		case NOTUNIQUE:
			errmsg(M_USED, Ndsname);
			adumprec(ADT_ADD_USR, EX_NDSNAME_EXISTS, cll, cmdlp);
			exit(EX_NDSNAME_EXISTS);
			/*NOTREACHED*/
		}
		call_hybrid = 1;
		hybrid->nds_namp = Ndsname;
	   }
	   else {
		if (Ndsname) {
		call_hybrid = 1;
		hybrid->nds_namp = Ndsname;
		}
	   }
	}

	/* expiration string is a date, newer than today */
	if (Expire) {
		if (!*Expire) {
			Expire = "0";
			shad->sp_expire = 0;
		}
		else {
			(void) putenv(DATMSK);
                	if ((tm_ptr = getdate(Expire)) == NULL) {
				if ((getdate_err > 0) && 
				     (getdate_err < GETDATE_ERR)) 
					fatal(M_GETDATE+getdate_err - 1, cll,
						cmdlp, "", "");
				else
					fatal(M_INVALID, cll, cmdlp, Expire, "expire date");
			}
 
			if ((date =  mktime(tm_ptr)) < 0)
				fatal(M_INVALID, cll, cmdlp, Expire, "expire date");

       			if ((shad->sp_expire = (date / DAY)) <= DAY_NOW)
				fatal(M_INVALID, cll, cmdlp, Expire, "expire date");
		}
		call_mast = 1;
		call_shad = 1; 
		mastp->ia_expire = shad->sp_expire;
	}

	if (pflag) {
		if (Fpass && strlen(Fpass)) {
			pgenp = (char *) malloc((int) strlen(Fpass) + 1);

			(void) sprintf(pgenp, "%s", Fpass);
			if ((strlen(pgenp) > (size_t)1) || !isprint(pgenp[0])) {
				errmsg(M_BAD_PFLG);
				clean_up(EX_SYNTAX);
			}
			mastp->ia_flag &= (unsigned) 0xffffff00;
			mastp->ia_flag |= (unsigned) pgenp[0];
		}
		else
			mastp->ia_flag &= (unsigned) 0xffffff00;

		call_mast = 1;
		call_shad = 1;
	}
	/*
	 * auditing event types or classes
	*/
	if (audit && !Nistononis) {
		if (getadtnam(logname, adtp) != 0) {
			(void) pfmt(stderr, MM_ERROR, ":1345:%s not found in audit file\n",
				logname);
			adumprec(ADT_MOD_USR, 1, cll, cmdlp);
			clean_up(1);
		}
		if (Auditmask && *Auditmask) {
			if (!cremask(ADT_UMASK, Auditmask, adtp->ia_amask)) {
				for (i = 0; i < ADT_EMASKSIZE; i++)
					mastp->ia_amask[i] = adtp->ia_amask[i];

				call_mast = 1;
			}
			else {
				audit = 0;
			}
		}
		if (audit && new_logname && *new_logname)
			(void) strcpy(adtp->ia_name, new_logname);
	}
	return;
}

/*
 * Procedure:	rec_pwd
 *
 * Notes:	unlinks the current password file ("/etc/passwd") and
 *		links the old password file to the current.  Only called
 *		if there is a problem after new password file was created.
 */
static	int
rec_pwd()
{
	if (unlink (PASSWD) || link (OPASSWD, PASSWD))
		return -1;
	return 0;
}


static	int
ck_def_lid(lidp, cnt, d_lid)
	level_t	*lidp;
	long	cnt;
	level_t	d_lid;
{
	int	i;
	level_t *p;

	p = lidp;

	for (i = 0; i < cnt; i++) {
		if (d_lid == *p++) {
			*--p = *lidp;
			*lidp = d_lid;
			return 0;
		}
	}
	return 1;
}


/*
 * Procedure:	mod_master
 *
 * Notes:	modifies user's master file entry
 */
static	int

mod_master()
{
	long	dirsz = 0,
		mastsz = 0;

	int	i, cnt = 0,
		shsz = 0,
		g_cnt = 0;
	char	*tmpsh = NULL,
		*tmpdir = NULL;

	if (new_master) {
		if (plus || replace)
			mastsz = ( sizeof(struct master) 
				+ mastp->ia_dirsz + mastp->ia_shsz + 2
				+ (lidcnt * sizeof(level_t) ) );
		else
			mastsz = (sizeof(struct master) 
				+ mastp->ia_dirsz + mastp->ia_shsz + 2
				+ (mastp->ia_lvlcnt * sizeof(level_t)));

		if (new_groups)
			mastsz += (Gidcnt * sizeof(gid_t));
		else
			mastsz += (mastp->ia_sgidcnt * sizeof(gid_t));

		new_mastp = (struct master *) malloc(mastsz);

		if (new_mastp == NULL) {
			(void) pfmt(stderr, MM_ERROR, ":1341:failed malloc\n");
			return 1;
		}

		masp = new_mastp;

		(void) strcpy(new_mastp->ia_name, mastp->ia_name);
		(void) strcpy(new_mastp->ia_pwdp, mastp->ia_pwdp);

		new_mastp->ia_uid = mastp->ia_uid;
		new_mastp->ia_gid = mastp->ia_gid;
		new_mastp->ia_lstchg = mastp->ia_lstchg;
		new_mastp->ia_min = mastp->ia_min;
		new_mastp->ia_max = mastp->ia_max;
		new_mastp->ia_warn = mastp->ia_warn;
		new_mastp->ia_inact = mastp->ia_inact;
		new_mastp->ia_expire = mastp->ia_expire;
		new_mastp->ia_flag = mastp->ia_flag;

		if (audit) {
			for (i = 0; i < ADT_EMASKSIZE; i++)
				new_mastp->ia_amask[i] = mastp->ia_amask[i];
		}

                new_mastp->ia_dirsz = mastp->ia_dirsz;
                new_mastp->ia_shsz = mastp->ia_shsz;

		if (plus || replace)
                	new_mastp->ia_lvlcnt = lidcnt;
		else
                	new_mastp->ia_lvlcnt = mastp->ia_lvlcnt;

		if (new_groups)
			new_mastp->ia_sgidcnt = Gidcnt;
		else
			new_mastp->ia_sgidcnt = mastp->ia_sgidcnt;

                new_mastp++;


		if (plus) {
			(void) memcpy(new_mastp, lidlstp, (lidcnt * sizeof(level_t)));
			new_mastp = (struct master *) ((char *)new_mastp 
					+ (lidcnt * sizeof(level_t)));
		}
		else if (replace) {
			(void) memcpy(new_mastp, &lidlist[0], (lidcnt * sizeof(level_t)));
			new_mastp = (struct master *) ((char *)new_mastp
					+ (lidcnt * sizeof(level_t)));
		}
		else {
			(void) memcpy(new_mastp, mastp->ia_lvlp, (mastp->ia_lvlcnt * sizeof(level_t)));
			new_mastp = (struct master *) ((char *)new_mastp 
				  + (mastp->ia_lvlcnt * sizeof(level_t)));
		}
		if (new_groups) {
			(void) memcpy(new_mastp, &Sup_gidlist[0], (Gidcnt * sizeof(gid_t)));
			new_mastp = (struct master *) ((char *)new_mastp
					+ (Gidcnt * sizeof(gid_t)));
		}
		else {
			(void) memcpy(new_mastp, mastp->ia_sgidp, (mastp->ia_sgidcnt * sizeof(gid_t)));
			new_mastp = (struct master *) ((char *) new_mastp
			 	  + (mastp->ia_sgidcnt * sizeof(gid_t)));
		}
		

		if (Dir && *Dir) 
                	(void) strcpy((char *)new_mastp, Dir);
		else
			(void) strcpy((char *)new_mastp, mastp->ia_dirp);

                new_mastp = (struct master *) ((char *)new_mastp
				+ (mastp->ia_dirsz + 1));

		if (Shell && *Shell)  
                	(void) strcpy((char *) new_mastp, Shell);
		else
			(void) strcpy((char *)new_mastp, mastp->ia_shellp);

		new_mastp = masp;

		if (putiaent(logname, new_mastp))
			return 1;
	}
	else {
		cnt = mastp->ia_lvlcnt;
		g_cnt = mastp->ia_sgidcnt;
		dirsz = mastp->ia_dirsz;
		shsz = mastp->ia_shsz;
		tmpdir = (char *)malloc(dirsz);
		tmpsh = (char *)malloc(shsz);
		(void) strcpy(tmpdir, mastp->ia_dirp);
		(void) strcpy(tmpsh, mastp->ia_shellp);

		masp = mastp;
		mastp++;

		if (replace) {
			(void) memcpy(mastp, &lidlist[0], (lidcnt * sizeof(level_t)));
			mastp = (struct master *) ((char *)mastp
				+ (lidcnt * sizeof(level_t)));
		}
		else  
			mastp = (struct master *) ((char *)mastp
				+ (cnt * sizeof(level_t)));

		if (new_groups) {
			(void) memcpy(mastp, &Sup_gidlist[0], (Gidcnt * sizeof(gid_t)));
			mastp = (struct master *) ((char *)mastp
				+ (Gidcnt * sizeof(gid_t)));
		}
		else {
			mastp = (struct master *) ((char *)mastp
				+ (g_cnt * sizeof(gid_t)));
		}
		if (Dir) 
               		(void) strcpy((char *)mastp, Dir);
		else 
			(void) strcpy((char *)mastp, tmpdir);

                mastp = (struct master *) ((char *)mastp + (dirsz + 1));

		if (Shell) 
                	(void) strcpy((char *)mastp, Shell);
		else
			(void) strcpy((char *)mastp, tmpsh);

		mastp = masp;

		if (replace)
			mastp->ia_lvlcnt = lidcnt;
		if (new_groups)
			mastp->ia_sgidcnt = Gidcnt;

		if (putiaent(logname, mastp))
			return 1;
	}
	return 0;
}
/*
 * Procedure:	getgrps
 *
 * Notes:	reads the group entries from the "/etc/group" file
 *		for the specified user name.
 */
static	int
getgrps(name, gid)
	char	*name;
	gid_t	gid;
{
	register int	i;
	int		ngroups = 0,
			grp_max = 0;
	register struct	group *grp;
 
	grp_max = sysconf(_SC_NGROUPS_MAX);

	if (gid >= 0)
		Sup_gidlist[ngroups++] = gid;

	setgrent();

	while (grp = getgrent()) {
		if (grp->gr_gid == gid)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], name))
				continue;
			if (ngroups == grp_max) {
				endgrent();
				return ngroups;
			}
			Sup_gidlist[ngroups++] = grp->gr_gid;
		}
	}
	endgrent();
	return ngroups;
}
static	char *
getgrps_str(name, gid)
	char	*name;
	gid_t	gid;
{
	register int	i,cnt;
	int		ngroups = 0,
			grp_max = 0;
	char *ret,*tmp;

	register struct	group *grp;
 
	grp_max = sysconf(_SC_NGROUPS_MAX);
	ret=(char *)calloc(1024, sizeof(char) );
	tmp=ret;

	setgrent();

	while (grp = getgrent()) {
		if (grp->gr_gid == gid)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], name))
				continue;
			if (ngroups == grp_max) {
				endgrent();
				if (*ret)
					*(tmp-1)=NULL;
				return *ret ? ret : NULL;
			}
			ngroups++;
			cnt=sprintf(tmp,"%d,",grp->gr_gid);
			tmp +=cnt;
		}
	}
	endgrent();
	if (*ret)
		*(tmp-1)=NULL;
	return *ret ? ret : NULL;
}


/*
 * Procedure:	clean_up
 *
 * Notes:	checks to see if a new home directory was created.
 *		If so, it removes it and then calls exit() with the
 *		argument passed.
 */
static	void
clean_up(ex_val)
	int	ex_val;
{
	if (new_home && ex_val)
		(void) rm_files(Dir);

	exit(ex_val);
}

#define	f_c	"/usr/bin/find . -print | /usr/bin/cpio -pdum %s > /dev/null 2>&1"
#define	o_c	"/usr/bin/chown -h -R %s . > /dev/null 2>&1"
#define	g_c	"/usr/bin/chgrp -h -R other . > /dev/null 2>&1"
static 
mv_files(name, src, dest)
	char *name;
	char *src;
	char *dest;
{
	char *sh_cmd = "/sbin/sh",
		*cmd = NULL;
	pid_t	pid;
	int status = 0;

	if (access(src, F_OK) < 0) {
		errmsg(M_NO_SOURCE_DIR, strerror(errno));
		exit(1);
	}
	if ((pid = fork()) < 0) {
		errmsg(M_NO_FORK,strerror(errno));
		return EX_HOMEDIR;
	}
	if (pid == (pid_t) 0) {
		/*
		 * in the child
		*/
		/*
		 * set up the command string exec'ed by the shell
		 * so it contains the correct directory name where
		 * the files are copied.
		*/
		cmd = (char *)malloc(strlen(f_c) + strlen(dest) + (size_t)1);
		(void) sprintf(cmd, f_c, dest);

		/*
		 * change directory to the source directory.  If
		 * the chdir fails, print out a message and exit.
		*/
		if (chdir(src) < 0) {
			errmsg(M_UNABLE_TO_CD, strerror(errno));
			exit(1);
		}
		(void) execl(sh_cmd, sh_cmd, "-c", cmd, (char *)NULL);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	(void) wait(&status);

	if (((status >> 8) & 0377) != 0) {
		return EX_HOMEDIR;
	}

	return EX_SUCCESS;
}

/*
 * The following is an explanation of how NIS user modifications 
 * will be handle for usermod.
 */
/*
*Nistononis 
*----------
*    - When an NIS user entry is changed to a local entry ...
*     allow entries to be changed from NIS to local regardless of
*     whether NIS is available
*
*   - the '+' character is removed from the login field
*
*   - if -d option is used, -d argument is stored in the home_dir field
*     else if value for home_dir is obtained from NIS, store NIS value 
*	in home_dir field
*     else set home_dir field to default (e.g., HOMEDIR/login)
*
*   - if -c option is used, -c argument is stored in comment field
*     else if value for comment is obtained from NIS, store NIS value in 
*	comment field
*     else set comment field to default ("")
*
*   - if -s option is used, -s argument is stored in login_shell field
*     else if value for login_shell is obtained from NIS, store NIS value 
*	in login_shell field
*     else set login_shell field to default (SHELL)
*
*   - if -g option is used, -g argument is stored in gid field
*     else if value for GID is obtained from NIS, store value from NIS in 
*	gid field
*     else set gid field to default (GROUPID)
*
*   - if -u option is used, -u argument is stored in uid field
*     else if value for UID is obtained from NIS, store NIS value in the
*	uid field
*     else set uid field to default (LASTUSED+1)
*
*   - if the password field in /etc/shadow is non-null, leave it alone  
*     else if a value is available from NIS, store the NIS value in the 
*	password field
*     else set the password field to "*LK*" 
*
*   - update IA databases so local entry can be used
*       
*
*Nisplusfromnonis
*------------
*   - When a local entry is changed to an NIS entry ...
*     allow local entries to be changed to NIS entries only if NIS
*     is available
*
*   - fail if no corresponding entry is found in the NIS map 
*
*   - a '+' character is prepended to the login field
*
*   - if -d option is used, -d argument is stored in the home_dir field 
*     else if value for home_dir is obtained from NIS, home_dir field 
*	is set to null
**    else leave home_dir field set to whatever was in /etc/passwd.
*
*   - if -c option is used, -c argument is stored in the comment field
*     else if value for comment is obtained from NIS, comment field is 
*	set to null
**    else leave comment field set to whatever was in /etc/passwd.
*
*   - if -s option is used, -s argument is stored in the login_shell field
*     else if value for login_shell is obtained from NIS, login_shell field 
*	is set to null
**    else leave login_shell field set to whatever was in /etc/passwd.
*
*   - if -g option is used, fail
*     else gid field set to 0 (could this just be null?)
*
*   - if -u option is used, fail
*     else uid field set to 0 (could this just be null?)
*
*   - if -m option is used, create home directory determined above with
*     ownership assigned based on UID/GID from NIS
*
*   - clear the password field for the entry in /etc/shadow
*
*   - update IA databases so local entry is no longer used
*
*/
/*
 * Procedure:	nismod_passwd
 *
 * Notes:	modify user attributes in "/etc/passwd" file
 * 		for the case when we are changing from an NIS user
 * 		to a non NIS user.
 */
static	int
nismod_passwd(passwd)
	struct	passwd	*passwd;
{
	struct	stat	statbuf;
	struct	passwd	*pw_ptr1p;
	FILE		*fp_temp;
	register int	error = 0,
			found = 0,
			niserr = 0,
			end_of_file = 0;
	struct	passwd	*nobody;

	errno = 0;
	memset(&nispw, 0, sizeof(struct passwd));

	if (stat(PASSWD, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & (S_IRUSR|S_IRGRP|S_IROTH)));

 	if ((fp_temp = fopen(PASSTEMP , "w")) == NULL)
		return errno;
	/*
	 * Get passwd info from NIS if it is up.
	 */
	if (!Nisuser_op)
		niserr=nis_getuser((Nisplusfromnonis ? logname : new_logname), &nispw);
	else 
		niserr=nis_getuser(logname+1, &nispw);
	/* 
	 * If creating an NIS user we must be able to verify that
	 * user is in NIS map. So fail if we can not.
	 */
	if (niserr)
		switch (niserr) {
			case YPERR_YPBIND:
				if (Nisplusfromnonis || Nisuser_op) {
				 	(void) pfmt(stderr, MM_ERROR, ":1338:NIS not available\n");
					clean_up(EX_NO_NIS);
				} else {
					if(Nistononis) {
				 		(void) pfmt(stderr, MM_ERROR, ":1338:NIS not available\n");
					}
				}
				break;
			case YPERR_KEY:
				if (Nisplusfromnonis || Nisuser_op) {
				 	(void) pfmt(stderr, MM_ERROR, ":1339:%s not found in NIS passwd map\n", logname);
					clean_up(EX_NO_NISMATCH);
				} else {
					if(Nistononis) {
				 		(void) pfmt(stderr, MM_ERROR, ":1339:%s not found in NIS passwd map\n", logname);
					}
				}
				break;
			default:
				if (Nisplusfromnonis || Nisuser_op) {
				 	(void) pfmt(stderr, MM_ERROR, ":1340:Unknown NIS error\n");
					clean_up(EX_UNK_NIS);
				} else {
					if(Nistononis) {
					 	(void) pfmt(stderr, MM_ERROR, ":1340:Unknown NIS error\n");
					}
				}
				break;
		}



	/*
	 * get default values in case they're needed.
	 */
	get_defaults();
	/*
	 * open the password file.
	*/
	nis_setpwent();
	/*
	 * The while loop for reading PASSWD entries
	*/
	while (!end_of_file && (Nistononis || Nisuser_op)) {
		if ((pw_ptr1p = (struct passwd *) nis_getpwent()) != NULL) {
			if (!strcmp(pw_ptr1p->pw_name, logname)) {
				found = 1;
				if (new_logname && *new_logname)
					pw_ptr1p->pw_name = passwd->pw_name;
				if (Uidstr && *Uidstr) {
					pw_ptr1p->pw_uid = passwd->pw_uid;
				} else if (nispw.pw_uid) {
					pw_ptr1p->pw_uid = nispw.pw_uid;
				} else {
					pw_ptr1p->pw_uid = (uid_t)def_Uid;
				}
				if (Group && *Group) {
					pw_ptr1p->pw_gid = passwd->pw_gid;
				} else if (nispw.pw_gid) {
					pw_ptr1p->pw_gid = nispw.pw_gid;
				} else {
					pw_ptr1p->pw_gid = atoi(def_Groupid);
				}
				if (cflag) {
					pw_ptr1p->pw_gecos = passwd->pw_gecos;
					pw_ptr1p->pw_comment = passwd->pw_comment;
				} else if (strlen(nispw.pw_gecos)) {
					pw_ptr1p->pw_gecos = nispw.pw_gecos;
					pw_ptr1p->pw_comment = nispw.pw_comment;
				} else if (!strlen(pw_ptr1p->pw_gecos)) {
					pw_ptr1p->pw_gecos = NULL;
					pw_ptr1p->pw_comment = NULL;
				}
				if (Dir && *Dir) {
					pw_ptr1p->pw_dir = passwd->pw_dir;
				} else if (strlen(nispw.pw_dir)) {
					pw_ptr1p->pw_dir = nispw.pw_dir;
				} else if (!strlen(pw_ptr1p->pw_dir)) {
					pw_ptr1p->pw_dir = def_Homedir;
				}
				if (Shell && *Shell) {
					pw_ptr1p->pw_shell = passwd->pw_shell;
				} else if (strlen(nispw.pw_shell)) {
					pw_ptr1p->pw_shell = nispw.pw_shell;
				} else if (!strlen(pw_ptr1p->pw_shell)) {
					pw_ptr1p->pw_shell = def_Shell;
				}
				/*
				 * check the size of the password entry.
				*/
				if (ck_p_sz(pw_ptr1p)) {
					nis_endpwent();
					(void) fclose(fp_temp);
					return 1;
				}
				/* 
				 * set up pass struct to be used by 
				 * nisadd_master().
				 */
				pass.pw_name= pw_ptr1p->pw_name;
				pass.pw_passwd= pw_ptr1p->pw_passwd = "x";
				pass.pw_uid= pw_ptr1p->pw_uid;
				pass.pw_gid= pw_ptr1p->pw_gid;
				pass.pw_age= pw_ptr1p->pw_age = "";
				pass.pw_comment= pw_ptr1p->pw_comment;
				pass.pw_gecos= pw_ptr1p->pw_gecos;
				pass.pw_dir= pw_ptr1p->pw_dir;
				pass.pw_shell= pw_ptr1p->pw_shell;

			}
			if (putpwent(pw_ptr1p, fp_temp)) {
				nis_endpwent();
				(void) fclose(fp_temp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)	 /* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {/* Bad entry, skip*/
				error++;
				errno = 0;
			}  
			else	/* unexpected error found */
				end_of_file = 1;		
		}
	}
	while (!end_of_file && Nisplusfromnonis) {
		if ((pw_ptr1p = (struct passwd *) nis_getpwent()) != NULL) {
			if (!strcmp(pw_ptr1p->pw_name, logname)) {
				found = 1;
				if (new_logname && *new_logname)
					pw_ptr1p->pw_name = passwd->pw_name;
					if (nobody = (struct passwd *)nis_getpwnam("nobody")) {
						pw_ptr1p->pw_uid = nobody->pw_uid;
						pw_ptr1p->pw_gid = nobody->pw_gid;
					}
					else {
						pw_ptr1p->pw_uid = NIS_UID;
						pw_ptr1p->pw_gid = NIS_UID;
					}
				if (cflag) {
					pw_ptr1p->pw_gecos = passwd->pw_gecos;
					pw_ptr1p->pw_comment = passwd->pw_comment;
				} else if (strlen(nispw.pw_gecos)) {
					pw_ptr1p->pw_gecos = NULL;
					pw_ptr1p->pw_comment = NULL;
				}
				if (Dir && *Dir) {
					pw_ptr1p->pw_dir = passwd->pw_dir;
				} else if (strlen(nispw.pw_dir)) {
					pw_ptr1p->pw_dir = NULL;
				} else {
					pw_ptr1p->pw_dir = def_Homedir;
				}
				if (Shell && *Shell) {
					pw_ptr1p->pw_shell = passwd->pw_shell;
				} else if (strlen(nispw.pw_shell)) {
					pw_ptr1p->pw_shell = NULL;
				} else {
					pw_ptr1p->pw_shell = def_Shell;
				}
				/*
				 * check the size of the password entry.
				*/
				if (ck_p_sz(pw_ptr1p)) {
					nis_endpwent();
					(void) fclose(fp_temp);
					return 1;
				}
			}
			if (putpwent(pw_ptr1p, fp_temp)) {
				nis_endpwent();
				(void) fclose(fp_temp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)	 /* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {
				error++;
				errno = 0;
			}  
			else	/* unexpected error found */
				end_of_file = 1;		
		}
	}
	while (!end_of_file && Nisminusfromnonis) {
		if ((pw_ptr1p = (struct passwd *) nis_getpwent()) != NULL) {
			if (!strcmp(pw_ptr1p->pw_name, logname)) {
				found = 1;

				if (new_logname && *new_logname)
					pw_ptr1p->pw_name = passwd->pw_name;
			}
			if (putpwent(pw_ptr1p, fp_temp)) {
				nis_endpwent();
				(void) fclose(fp_temp);
				return 1;
			}
		}
		else { 
			if (errno == 0)	 /* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {
				/* Bad entry found, skip*/
				error++;
				errno = 0;
			}  
			else	/* unexpected error found */
				end_of_file = 1;		
		}
	}

	/*
	 * close the password file
	*/
	nis_endpwent();

	(void) fclose(fp_temp);

	if (error >= 1) {
		errmsg (M_BADENTRY,PASSWD);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, PASSTEMP))
		return errno;

	if (chown(PASSTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}

/*
 * Procedure:	nismod_shadow
 *
 * Notes:	modify user attributes in "/etc/shadow" file
 */
static	int
nismod_shadow(shadow)
	struct	spwd	*shadow;
{
	struct	stat	statbuf;
	struct	spwd	*sp_ptr1p;
	FILE		*fp_temp;
	FILE		*fp;
	register int	error = 0,
			found = 0,
			end_of_file = 0,
			niserr = 0;

	errno = 0;

	if (stat(SHADOW, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(SHADTEMP , "w")) == NULL)
		return errno;

 	if ((fp = fopen(SHADOW , "r")) == NULL)
		return errno;

	/*
	 * The while loop for reading SHADOW entries
	*/
	while (!end_of_file) {
		if ((sp_ptr1p = (struct spwd *)fgetspent(fp)) != NULL) {
			if (!strcmp(logname, sp_ptr1p->sp_namp)) {
				found = 1;
				if (new_logname && *new_logname)
					sp_ptr1p->sp_namp = shadow->sp_namp;
				if (Inact && *Inact)
			      		sp_ptr1p->sp_inact = shadow->sp_inact;
				if (Expire && *Expire)
			      		sp_ptr1p->sp_expire = shadow->sp_expire;
				if (pflag) {
					if (Fpass && strlen(Fpass)) {
						sp_ptr1p->sp_flag &=
							(unsigned) 0xffffff00;
						sp_ptr1p->sp_flag |=
							(unsigned) pgenp[0];
					}
					else
						sp_ptr1p->sp_flag &=
							(unsigned) 0xffffff00;
				}
				/*
	 			* check the size of the shadow entry.
				*/
				if (ck_s_sz(sp_ptr1p)) {
					(void) fclose(fp_temp);
					(void) fclose(fp);
					return 1;
				}
				/*
				 * set up the passwd field. 
				 * This is a new semantic for 
				 * mod_shadow added specifically for NIS.
				 */
				 if (strlen(sp_ptr1p->sp_pwdp) == NULL) {
					/*
					 * if passwd is in NIS map use it 
					 */
					if(Nistononis) {
						if (strlen(nispw.pw_passwd)) {
							strcpy(sp_ptr1p->sp_pwdp, nispw.pw_passwd);
						} else { 
						/* 
						 * passwd is not in map and 
						 * no value in shadow - lock it.
						 */
							strcpy(sp_ptr1p->sp_pwdp, "*LK*");
						}
					} else if (Nisplusfromnonis) {
						sp_ptr1p->sp_pwdp = (char *)NULL;
					}
				 }
				/* 
				 * If Nistononis,
				 * Set up defaults for shadow passwd 
				 * entry
				 */
				 if (Nistononis) {
					sp_ptr1p->sp_lstchg = -1;
					sp_ptr1p->sp_min = atoi("0");
					sp_ptr1p->sp_max = atoi("168");
					sp_ptr1p->sp_warn = atoi("7");
					sp_ptr1p->sp_expire = atoi(def_Expire);
					sp_ptr1p->sp_inact = atoi(def_Inact);
					shad.sp_namp = sp_ptr1p->sp_namp; 
					shad.sp_pwdp = sp_ptr1p->sp_pwdp; 
					shad.sp_lstchg = sp_ptr1p->sp_lstchg; 
					shad.sp_min = sp_ptr1p->sp_min; 
					shad.sp_max = sp_ptr1p->sp_max; 
					shad.sp_warn = sp_ptr1p->sp_warn; 
					shad.sp_inact = sp_ptr1p->sp_inact; 
					shad.sp_expire = sp_ptr1p->sp_expire; 

				 }
			}
			if (putspent(sp_ptr1p, fp_temp)) {
				(void) fclose(fp_temp);
				(void) fclose(fp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)	/* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	
				error++;
				errno = 0;
			}  
			else	/* unexpected error found */
				end_of_file = 1;		
		}
	}


	/*
	 * close the shadow file
	*/

	(void) fclose(fp);
	(void) fclose(fp_temp);

	if (error >= 1) {
		errmsg (M_BADENTRY,SHADOW);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, SHADTEMP))
		return errno;

	if (chown(SHADTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}

/*
 * Procedure:	get_defaults
 *
 * Notes:	opens the file ``/etc/defaults/useradd'' and reads in as
 *		many default values that are set.  For some values, if
 *		they don't exist in the file, the defaults are taken from
 *		this program.  In all cases, verification of the values
 *		that are set is done in the ``set_defaults()'' routine.
 */
static	void
get_defaults()
{
	char	*Usrdefault = "useradd";
	FILE	*def_fp;

	if ((def_fp = defopen(Usrdefault)) != (FILE *)NULL) {
		if (!def_Homedir) {
			if ((def_Homedir = defread(def_fp, "HOMEDIR")) != NULL)
				def_Homedir = strdup(def_Homedir);
		}
		if (!def_Shell) {
			if ((def_Shell = defread(def_fp, "SHELL")) != NULL){
				def_Shell = strdup(def_Shell);
			}
		}
		if (!def_Skel_dir) {
			if ((def_Skel_dir = defread(def_fp, "SKELDIR")) != NULL)
				def_Skel_dir = strdup(def_Skel_dir);
		}
		if (!def_Fpass) {
			if ((def_Fpass = defread(def_fp, "FORCED_PASS")) != NULL)
				def_Fpass = strdup(def_Fpass);
		}
		if (audit && !def_Auditmask) {
			if ((def_Auditmask = defread(def_fp, "AUDIT_MASK")) != NULL) {
				def_Auditmask = strdup(def_Auditmask);
				if (*def_Auditmask) {
					if (strncmp(def_Auditmask, "none",
						(size_t)strlen("none")) == 0) {
							def_Auditmask = NULL;
					}
				}
				else {
					def_Auditmask = NULL;
				}
			}
		}
		if (mac && !def_Def_lvl) {
			if ((def_Def_lvl = defread(def_fp, "DEFLVL")) != NULL)
				def_Def_lvl = strdup(def_Def_lvl);
		}
		if (!def_Groupid) {
			if ((def_Groupid = defread(def_fp, "GROUPID")) != NULL)
				def_Groupid = strdup(def_Groupid);
		}
		if (!def_Expire) {
			if ((def_Expire = defread(def_fp, "EXPIRE")) != NULL)
				def_Expire = strdup(def_Expire);
		}
		if (!def_Inact) {
			if ((def_Inact = defread(def_fp, "INACT")) != NULL)
				def_Inact = strdup(def_Inact);
		}

		if (!HomeMode) {
			if ((HomeMode = defread(def_fp, "HOME_MODE")) != NULL)
				HomeMode = strdup(HomeMode);
		}
		(void) defclose(def_fp);
	}
	/*
	 * Set these values to their defaults in case the file
	 * ``/etc/default/useradd'' is inaccessible and they weren't
	 * set via the command line.
	*/

	if (!def_Shell || !*def_Shell)
		def_Shell = SHELL;
	if (!def_Groupid || !*def_Groupid)
		def_Groupid = "other";
	if (!def_Homedir || !*def_Homedir)
		def_Homedir = HOMEDIR;
	if (!def_Skel_dir || !*def_Skel_dir)
		def_Skel_dir = SKEL_DIR;
	if (!def_Def_lvl || !*def_Def_lvl)
		def_Def_lvl = DEF_LVL;
	if (!HomeMode || !*HomeMode)
		HomeMode = HOME_MODE;
	def_Uid = get_uid();

	{
		char *a1 =&HomeMode[0];
		int c;
		
		while(c = *a1++) {
			if (c>='0' && c<='7')
				HomePermissions = (HomePermissions<<3) + (c-'0');
			else {
				errmsg(M_CHMOD_NEW, def_Homedir);
				exit(EX_HOMEDIR);
			}
		}
		if( HomePermissions < 0 || HomePermissions > 0777 ) {
			errmsg(M_CHMOD_NEW, def_Homedir);
		}
	}

	return;
}

/*
 * Procedure:	rm_index
 *
 * Restrictions:
 *		stat(2):	None
 *		fopen:		None
 *		open(2):	None
 *		fclose:		None
 *		mmap(2):	None
 *		fwrite:		None
 *		chown(2):	None
 *
 * Notes:	deletes the user from the "/etc/security/ia/index" file
*/
static	int
rm_index(logname)
	char	*logname;
{
	struct	stat	statbuf;
	struct	index	*midxp;
	struct	index	index;
	struct	index	*indxp = &index;
	FILE		*fp_temp;
	register int	cnt = 0,
			i, fd_indx;

	if (stat(INDEX, &statbuf) < 0) {
		return errno;
	}

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(TMPINDEX, "w")) == NULL) {
		return errno;
	}

	cnt = (statbuf.st_size/sizeof(struct index));

        if ((fd_indx = open(INDEX, O_RDONLY)) < 0) {
		(void) fclose(fp_temp);
		return errno;
        }

        if ((midxp = (struct index *)mmap(0, (unsigned int)statbuf.st_size,
		PROT_READ, MAP_SHARED, fd_indx, 0)) < (struct index *) 0) {

		(void) close(fd_indx);
		(void) fclose(fp_temp);
		return errno;
	}

	indxp = (struct index *) malloc(statbuf.st_size - sizeof(struct index));

	if (indxp == NULL) {
		(void) munmap((char *)midxp, (unsigned int)statbuf.st_size);
		(void) close(fd_indx);
		(void) fclose(fp_temp);
		return 1;
	}

	for (i = 0; i < cnt; i++) {
		if (strcmp(midxp->name, logname) == 0)
{
			midxp++;
}
		else {
			if (fwrite(midxp, sizeof(struct index), 1, fp_temp) != 1) {
				(void) munmap((char *)midxp, (unsigned int)statbuf.st_size);
				(void) close(fd_indx);
				(void) fclose(fp_temp);
				return 1;
			}
			midxp++;
		}
	}

	(void) munmap((char *)midxp, (unsigned int)statbuf.st_size);
	(void) close(fd_indx);
	(void) fclose(fp_temp);
	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, TMPINDEX))
{
		return errno;
}

	if (chown(TMPINDEX, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}
	return 0;
}

static uid_t
get_uid()
{
	int luid=NULL;
	register int end_of_file=0;
	struct	passwd	*pw_ptr1p;
		/*
		 * create the head of the uid number list
		*/
		if ((uid_sp = (struct uid_blk *) malloc((size_t) sizeof(struct uid_blk))) == NULL) {
			errmsg(M_UNABLE_TO_ALLOC_UID_LIST);
/*
			adumprec(ADT_ADD_USR, EX_FAILURE, cll, cmdlp);
*/
			exit(EX_FAILURE);
		}
		uid_sp->link = NULL;
		uid_sp->low = (UID_MIN - 1);
		uid_sp->high = (UID_MIN - 1);

	nis_setpwent();
		while (!end_of_file) {
			if ((pw_ptr1p = (struct passwd *) nis_getpwent()) != NULL) {
				if (add_uid(pw_ptr1p->pw_uid, &uid_sp) == -1) {
					errmsg(M_UNABLE_TO_ALLOC_UID_LIST);
					exit(EX_FAILURE);
				}
			} else {
				end_of_file = 1;
			}
			
		}
		luid = uid_sp->high + 1;	
		uid_sp2 = uid_sp;
		while (uid_age(CHECK, luid, 0) == 0) {
			if (++luid == uid_sp2->link->low) {
				uid_sp2=uid_sp2->link;
				luid = uid_sp2->high + 1;	 
			}
		}
	nis_endpwent();
		return(luid);
}

/*
 * Procedure:	add_master
 *
 * Notes:	adds the information concerning the new user to the
 *		"/etc/security/ia/master" file
 */
static	int
nisadd_master(passwd, shadow)
	struct	passwd	*passwd;
	struct	spwd	*shadow;
{
	struct	master	*mastp,
			*mastsp;
	long	dirsz = 0;
	long	shsz  = 0;
	long	length = 0;

	int	i;

 	dirsz = (long) strlen(passwd->pw_dir);
	shsz = (long) strlen(passwd->pw_shell);
	length = (dirsz + shsz ) 
		+ (Gidcnt * sizeof(gid_t))
		+ (sizeof(struct master) + 2);

	mastp = (struct master *)malloc(length);

	if (mastp == NULL) 
		return 1;

	mastsp = mastp;
	(void) strcpy(mastp->ia_name, passwd->pw_name);
	(void) strcpy(mastp->ia_pwdp, shadow->sp_pwdp);

	mastp->ia_uid = passwd->pw_uid;
	mastp->ia_gid = passwd->pw_gid;
	mastp->ia_lstchg = shadow->sp_lstchg;
	mastp->ia_min = shadow->sp_min;
	mastp->ia_max = shadow->sp_max;
	mastp->ia_warn = shadow->sp_warn;
	mastp->ia_inact = shadow->sp_inact;
	mastp->ia_expire = shadow->sp_expire;
	mastp->ia_flag = shadow->sp_flag;
	if (audit) {
		for (i = 0; i < ADT_EMASKSIZE; i++)
			mastp->ia_amask[i] = emask[i];
	}
	mastp->ia_dirsz = dirsz;
	mastp->ia_shsz = shsz;
	mastp->ia_lvlcnt = 0;
	mastp->ia_sgidcnt = Gidcnt;
	mastp++;
	if (mac) {
		(void) memcpy(mastp, lidlist, (lidcnt * sizeof(level_t)));
		mastp = (struct master *) ((char *)mastp + (lidcnt * sizeof(level_t)));
	}
	if (Gidcnt) {
		(void) memcpy(mastp, &Sup_gidlist[0], (Gidcnt * sizeof(gid_t)));
		mastp = (struct master *) ((char *)mastp + (Gidcnt * sizeof(gid_t)));
	}
	(void) strcpy((char *)mastp, passwd->pw_dir);
	mastp = (struct master *) ((char *)mastp + (dirsz + 1));
	(void) strcpy((char *)mastp, passwd->pw_shell);

	if (putiaent(new_logname, mastsp) != 0)
		return 1;

	return 0;
}


/*
 * Procedure:	rm_auditent
 *
 * Restrictions:
 *		stat(2):	None
 *		fopen:		None
 *		getadtent:	None
 *		fclose:		None
 *		chown(2):	None
 *
 * Notes:	deletes the user's entry from the "/etc/security/ia/audit"
 *		file
*/
static	int
rm_auditent(logname)
	char	*logname;
{
	struct	stat	statbuf;
	struct	adtuser	adtuser;
	struct	adtuser	*adtp = &adtuser;
	FILE		*fp_temp;
	register int	ret = 0,
			error = 0,
			end_of_file = 0;

	if (stat(AUDITMASK, &statbuf) < 0)
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(MASKTEMP , "w")) == NULL) {
		return errno;
	}

	errno = error = end_of_file = 0;

	/*
	 * since the audit file was already opened, just rewind it
	*/
	setadtent();

	while (!end_of_file) {
		if ((ret = getadtent(adtp)) == 0) {
			if (!strcmp(logname, adtp->ia_name)) {
			 	continue;
	      		}
			if (putadtent(adtp, fp_temp)) {
				(void) fclose(fp_temp);
				endadtent();
				return 1;
			}
		}
		else {
			if (ret == -1) 
				end_of_file = 1;
			else if (errno == EINVAL){	/*bad entry found, skip */
				error++;
				errno = 0;
			} else		/* unexpected error found */
				end_of_file = 1;
		}	
	}

	(void) fclose(fp_temp);
	/*
	 * close the audit file as well.
	*/
	endadtent();

	if (error >= 1) {
		errmsg (M_AUDIT_BADENTRY);
	}

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, MASKTEMP)) {
		return errno;
	}
	return error;
}
/*
 * Procedure:	add_audit
 *
 * Notes:	adds the new user's audit entry to the "/etc/security/ia/audit"
 *		file
 */
static	int
add_audit(logname)
	char	*logname;
{
	struct	stat	statbuf;
	struct	adtuser	adtuser;
	struct	adtuser	*adtp = &adtuser;
	FILE		*fp_temp;
	register int	ret = 0,
			i, error = 0,
			end_of_file = 0;

	errno = 0;

	if (stat(AUDITMASK, &statbuf) < 0)
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(MASKTEMP, "w")) == NULL) {
		return errno;
	}

	/*
	 * open the audit file
	*/
	setadtent();

	while (!end_of_file) {
		if ((ret = getadtent(adtp)) == 0) {
			if (putadtent(adtp, fp_temp)) {
				(void) fclose(fp_temp);
				endadtent();
				return 1;
			}
		}
		else {
			if (ret == -1) 
				end_of_file = 1;
			else if (errno == EINVAL){	/*bad entry found, skip */
				error++;
				errno = 0;
			} else		/* unexpected error found */
				end_of_file = 1;
		}	
	}
	/*
	 * close the audit file
	*/
	endadtent();

	if (error >= 1) {
		errmsg (M_AUDIT_BADENTRY);
	}

	(void) strcpy(adtp->ia_name, logname);

	for (i = 0; i < ADT_EMASKSIZE; i++)
		adtp->ia_amask[i] = emask[i];

	if (putadtent(adtp, fp_temp)) {
		(void) fclose(fp_temp);
		return 1;
	}

	(void) fclose(fp_temp);

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, MASKTEMP))
		return errno;

	if (chown(MASKTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}

/*
 * Procedure:   chownfiles()
 *
 * Notes:       changes ownership of user's files to new uid; uses path in
 *		/etc/default/usermod to determine where to search for files.
 *
 *		Returns zero on success, non-zero on failure.
 */
static  int
chownfiles(logname, old_uid, new_uid, homedir)
	char	*logname;   /* user's logname  (may have just changed) */
	uid_t	 old_uid;   /* user's previous uid */
	uid_t	 new_uid;   /* user's new uid */
	char	*homedir;   /* user's home directory  (may have just changed) */
{
	char	 cmd_buf[MAXPATHLEN];
	char	*sh_cmd = "/sbin/sh";
	pid_t	 pid;
	int	 rval, status;


	(void) sprintf(cmd_buf, 
	  "/usr/sadm/bin/usermodU %s %d %d %s",
	  logname, old_uid, new_uid, homedir);


	if ((pid = fork()) < 0) {
		errmsg(M_NO_FORK, strerror(errno));
		return(1);
        }
	if (pid == (pid_t) 0) {   /* child process */
		(void) execl (sh_cmd, sh_cmd, "-c", cmd_buf, (char *)NULL);
		exit(1);
	}

	/* parent process follows */
	(void) wait(&status);
	if (WIFEXITED(status)) {
		if ((rval = WEXITSTATUS(status)) != 0) {
			errmsg(M_CHOWN_FAIL, "chown files");
			return(rval);
		}
        }
	return(0);
}

/*
 * Procedure:   save_and_remove_crontab()
 *
 * Notes:       Saves user's crontab in tmpcronfile and inactivates it.
 *		This is used when changing uid before the uid is actually
 *		changed.  The crontab will be reactivated under the new uid 
 *		after the uid has been changed.
 *
 *		Returns zero if a crontab is found and saved successfully.
 *		Return non-zero on error or if no crontab is found.
 */
static  int
save_and_remove_crontab(logname)
	char	*logname;   /* user's logname */
{
	char	 cmd_buf[MAXPATHLEN];
	char	*sh_cmd = "/sbin/sh";
	pid_t	 pid;
	int	 rval, status;


	/* save the file name to be used to save the crontab */
	(void) sprintf(tmpcronfile, "/tmp/umod.cron.%d", getpid());

	(void) sprintf(cmd_buf, "/usr/bin/crontab -l %s > %s 2>/dev/null",
		logname, tmpcronfile);

	if ((pid = fork()) < 0) {
		errmsg(M_NO_FORK, strerror(errno));
		unlink(tmpcronfile);
		return(1);
        }
	if (pid == (pid_t) 0) { /* first child process -- save the crontab */
		(void) execl (sh_cmd, sh_cmd, "-c", cmd_buf, (char *)NULL);
		unlink(tmpcronfile);
		exit(1);
	}

	/* parent process */
	(void) wait(&status);
	if (WIFEXITED(status)) {
		if ((rval = WEXITSTATUS(status)) != 0) {
			/* either no crontab or an unexpected error */
			unlink(tmpcronfile);
			return(rval);
		}
        }

	(void) sprintf(cmd_buf, "/usr/bin/crontab -r %s > /dev/null 2>&1",
		logname);

	if ((pid = fork()) < 0) {
		errmsg(M_NO_FORK, strerror(errno));
		/*
		 * even if fork failed, we return 0 anyway since first 
		 * child succeeded; we have to restore the crontab for the
		 * new uid
		 */
		return(0);
        }
	if (pid == (pid_t) 0) { /* second child process - inactivate crontab */
		(void) execl (sh_cmd, sh_cmd, "-c", cmd_buf, (char *)NULL);
		exit(1);
	}

	/* parent process */
	(void) wait(&status);

	/* return 0 since first child succeeded */
	return(0);
}

/*
 * Procedure:   replace_crontab()
 *
 * Notes:       Reactivates user's crontab from tmpcronfile.
 *		This is used when changing uid after the uid is actually
 *		changed.
 *
 *		Returns zero if crontab is reactivated successfully.
 *		Return non-zero otherwise.
 */
static  int
replace_crontab(logname)
	char	*logname;
{
	char	 cmd_buf[MAXPATHLEN];
	FILE	*fp;

	/* set umask to be sure we can create and edit the crontab */
	(void) umask(S_IRWXG|S_IRWXO);

	/*
	 * execute the equivalent of the following crontab editing session:
	 *   VISUAL=ed /usr/bin/crontab -e <logname> <<!
	 *   r <tmpcronfile>
	 *   w
	 *   q
	 *   !
	 */
	(void) sprintf(cmd_buf, 
	  "VISUAL=ed /usr/bin/crontab -e %s >/dev/null 2>&1", logname);
	fp = popen(cmd_buf, "w");
	if (fp != NULL) {
		fprintf(fp, "r %s\n", tmpcronfile);
		fprintf(fp, "w\n");
		fprintf(fp, "q\n");
		(void) pclose(fp);
		unlink(tmpcronfile);
	}
	return(0);
}

#ident  "@(#)useradd.c	1.7"
#ident  "$Header$"


#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<limits.h>
#include	<grp.h>
#include	<string.h>
#include	<userdefs.h>
#include	"users.h"
#include	"messages.h"
#include	<nwusers.h>
#include 	<shadow.h>
#include	"uidage.h"
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
 *	useradd [-u uid [-o] [-i] ] [-g group ] [-G group[[,group]...]] [-d dir ]
 *		[-s shell] [ -c comment] [-m [-k skel_dir]] [-f inactive]
 *		[-e expire] [-p passgen] login
 *
 * MAC - no auditing installed
 *
 *	useradd [-u uid [-o] [-i] ] [-g group ] [-G group[[,group]...]] [-d dir ]
 *		[-s shell] [ -c comment] [-m [-k skel_dir]] [-f inactive]
 *		[-e expire] [-p passgen] [-h level [-h level [...]]]
 *		[-v def_level] [-w hd_level] login
 *
 * no MAC - auditing installed
 *
 *	useradd [-u uid [-o] [-i] ] [-g group ] [-G group[[,group]...]] [-d dir ]
 *		[-s shell] [ -c comment] [-m [-k skel_dir]] [-f inactive]
 *		[-e expire] [-p passgen] [-a event[,...]] login
 *
 * MAC and auditing installed
 *
 *	useradd [-u uid [-o] [-i] ] [-g group ] [-G group[[,group]...]] [-d dir ]
 *		[-s shell] [ -c comment] [-m [-k skel_dir]] [-f inactive]
 *		[-e expire] [-p passgen] [-h level [-h level [...]]]
 *		[-v def_level] [-w hd_level] [-a event[,...]] login
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
 *		/etc/defaults/useradd
 *		/etc/security/ia/master
 *
 * Notes: 	This command adds new user logins to the system.  Arguments are:
 *
 *			uid - an integer less than 2**16 (USHORT)
 *			group - an existing group's integer ID or char str name
 *			dir - home directory
 *			shell - a program to be used as a shell
 *			comment - any text string
 *			skel_dir - a skeleton directory
 *			login - a string of alphanumeric, '+', '-', '.' and '_' characters
 *			eventstr - a set of auditing event types or classes
*/

#define	HOMEDIR		"/home"
#define	HOME_MODE	"0755"
#define	DEF_LVL		"USER_LOGIN"
#define NIS_UID		(UID_MIN-1) /* this is probably better than 0 */

extern	void	errmsg(),
		adumprec();

extern	long	strtol();

extern	struct	tm	*getdate();

extern	int	lvlin(),
		getopt(),
		add_uid(), 
		cremask(),
		uid_age(),
		auditctl(),
		valid_uid(),
		check_perm(),
		edit_group(),
		all_numeric(),
		create_home(),
		restore_ia(),
		valid_group(),
		valid_login(),
		valid_ndsname(),
		call_udelete(),
		**valid_lgroup();

extern	char	*optarg,			/* used by getopt */
		*errmsgs[],
		*strrchr(),
		argvtostr();

extern	int	errno,
		optind,
		opterr,				/* used by getopt */
		getdate_err;

static	void	fatal(),
		file_error(),
		no_service(),
		get_defaults();

static	char	*logname,			/* login name to add */
		*usr_lvl,
		*Dir = NULL,
		*Ustr = NULL,
		*Comment = NULL,
		Home[PATH_MAX + 1];		/* home directory */

extern  char *strchr (const char *s, int c);	/* used to validate login name */
#define okChars "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-._"

static	adtemask_t	emask;

static	level_t	def_lid,
		hom_lid,
		lidlist[64];

static	int	mac = 0,
		uid = 0,
		audit = 0,
		sflg = 0,
		dflg = 0,
		**Gidlist,
		Gidcnt = 0,
		Nisuid = -1, 
		Nisgid = -1,
		Nisname = 0,
		Nisuser = 0,
		Nisplus = 0,
		Nisdfluid = NIS_UID,
		Nisdflgid = 1;

static	long	lidcnt = 0;

static	gid_t	*Sup_gidlist;

static	int	rec_pwd(),
		ck_p_sz(),
		ck_s_sz(),
		add_audit(),
		add_passwd(),
		add_master(),
		add_shadow(),
		add_nwusers(),
		ck_4_nisuser(),
		set_defaults();

/*
 * The following variables are set in the get_defaults()
 * routine if the value is contained in the /etc/defaults
 * file.
*/
static	char	*Fpass = NULL,
		*Shell = NULL,
		*Inact = NULL,
		*Expire = NULL,
		*Def_lvl = NULL,
		*Groupid = NULL,
		*Homedir = NULL,
		*Skel_dir = NULL,
		*Ndsname = NULL,
		*HomeMode = NULL,
		*Auditmask = NULL;

int HomePermissions;

/*
 * The following variable MUST be global so that it is
 * accessible to the external error printing routine "errmsg()".
*/
char	*msg_label = "UX:useradd";

/*
 * These strings are used as replacements in the ``usage'' line depending
 * on the features installed so they should never have their own catalog
 * index numbers.
*/

static	char
	*aud_str = " [ -a event[,...]]",
	*mac_str = " [ -h level [-h level [...]]]\n               [-v def_level] [-w hd_level]";

static	struct	uid_blk	*uid_sp,
			*uid_sp2;

static	struct	passwd	passwd_nis;


main(argc, argv)
	int	argc;
	char	*argv[];
{
	char *ptr;
	int	i, ch, cll, okName,
		mflag = 0, oflag = 0,
		iflag = 0, wflag = 0,
		need_uid = 0, gid = 1;

	level_t	level;

	struct	spwd	shadow_st;
	struct	hybrid	hybrid_st;
	struct	passwd	passwd_st;

	char	*cmdlp,
		*grps = NULL,		/* multi groups from command line */
		*Mac_check = "SYS_PRIVATE";

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	opterr = 0;			/* no print errors from getopt */

	/*
	 * save command line arguments
	*/
	if ((cmdlp = (char *)argvtostr(argv)) == NULL) {
		adumprec(ADT_ADD_USR, 1, strlen(argv[0]), argv[0]);
                exit(1);
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

	while((ch = getopt(argc, argv, "c:d:e:f:G:g:k:n:imop:s:u:a:v:h:w:")) != EOF)
		switch(ch) {
		case 'c':
			if (optarg) {
				if ((int)strlen(optarg) > CMT_SIZE || strpbrk (optarg, ":\n"))
					fatal(M_INVALID, cll, cmdlp, optarg, "comment");
				Comment = optarg;
			}
			break;
		case 'd':
			Dir = optarg;
			dflg = 1;
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
			Groupid = optarg;
			break;

		case 'k':
			Skel_dir = optarg;
			break;

		case 'i':
			iflag = 1;
			break;

		case 'm':
			mflag = 1;
			break;

		case 'n':
			Ndsname = optarg;
			break;

		case 'o':
			oflag = 1;
			break;

		case 'p':
			Fpass = optarg;
			break;

		case 's':
			Shell = optarg;
			sflg = 1;
			break;

		case 'u':
			Ustr = optarg;
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
				if (!usr_lvl || !*usr_lvl) {
					fatal(M_AUSAGE, cll, cmdlp,
						(mac ? mac_str : ""),
						(audit ? aud_str : ""));
				}
				
				if (lvlin(usr_lvl, &level) == 0)
					lidlist[lidcnt++] = level;
				else {
					fatal(M_INVALID_LVL, cll, cmdlp, NULL, NULL);
				}
			}
			else {
				no_service(M_NO_MAC_H, EX_SYNTAX, cll, cmdlp);
			}
			break;

		case 'v':
			if (mac) {
				Def_lvl = optarg;
				if (!Def_lvl || !*Def_lvl) {
					fatal(M_AUSAGE, cll, cmdlp,
						(mac ? mac_str : ""),
						(audit ? aud_str : ""));
				}
			}
			else {
				no_service(M_NO_MAC_V, EX_FAILURE, cll, cmdlp);
			}
			break;

		case 'w':
			if (mac) {
				if (!optarg || !*optarg) {
					fatal(M_AUSAGE, cll, cmdlp,
						(mac ? mac_str : ""),
						(audit ? aud_str : ""));
				}
				if (lvlin(optarg, &hom_lid) != 0)
					fatal(M_INVALID_WLVL, cll, cmdlp, NULL, NULL);
				wflag = 1;
			}
			else {
				no_service(M_NO_MAC_W, EX_FAILURE, cll, cmdlp);
			}
			break;

		case '?':
			errmsg(M_AUSAGE, (mac ? mac_str : ""),
				(audit ? aud_str : ""));
			adumprec(ADT_ADD_USR, EX_SYNTAX, cll, cmdlp);
			exit(EX_SYNTAX);
			break;
		}	/* end of switch */

	/*
	 * check syntax
	 */
	if (optind != argc - 1 || (Skel_dir && !mflag)) {
		errmsg(M_AUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
		adumprec(ADT_ADD_USR, EX_SYNTAX, cll, cmdlp);
		exit(EX_SYNTAX);
	}
	if (mac) {
		if (wflag && !(mflag && (usr_lvl && *usr_lvl))) {
			errmsg(M_AUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
			adumprec(ADT_ADD_USR, EX_SYNTAX, cll, cmdlp);
			exit(EX_SYNTAX);
		}
		if (Def_lvl && *Def_lvl && !(usr_lvl && *usr_lvl)) {
			errmsg(M_AUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
			adumprec(ADT_ADD_USR, EX_SYNTAX, cll, cmdlp);
			exit(EX_SYNTAX);
		}
	}

	/* validate the login name */

	logname = argv[optind];
	for (i = 0, okName = 1; i < strlen(logname); okName &= strchr(okChars,logname[i++]) != 0);
	if (!okName) {
		errmsg (M_LOGIN_SYNTAX);
		adumprec(ADT_ADD_USR, EX_SYNTAX, cll, cmdlp);
		exit(EX_SYNTAX);
	}

	/*
	 * Set a lock on the password file so that no other
	 * process can update the "/etc/passwd" and "/etc/shadow"
	 * at the same time.  Also, ignore all signals so the
	 * work isn't interrupted by anyone.  In addition,
	 * check for appropriate privileges.
	*/
	if (lckpwdf() != 0) {
		if (access(SHADOW, EFF_ONLY_OK|W_OK) != 0) {
			(void) pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			adumprec(ADT_ADD_USR, 1, cll, cmdlp);
			exit(1);
		}
		errmsg (M_PASSWD_BUSY);
		adumprec(ADT_ADD_USR, 8, cll, cmdlp);
		exit(8);
	}
	for (i = 1; i < NSIG; ++i)
		if ( i != SIGCLD )
			(void) sigset(i, SIG_IGN);
	/*
	 * clear errno since it was set by some of the calls to sigset
	 */
	errno = 0;

	/*
	 * the valid_login routine checks to see if the login is
	 * in either the "/etc/passwd" file or the "/etc/shadow" file
	*/
	switch (valid_login(logname, (struct pwd **) NULL)) {
	case INVALID:
		fatal(M_INVALID, cll, cmdlp, logname, "login name");
		/*NOTREACHED*/

	case NOTUNIQUE:
		errmsg(M_USED, logname);
		adumprec(ADT_ADD_USR, EX_NAME_EXISTS, cll, cmdlp);
		exit(EX_NAME_EXISTS);
		/*NOTREACHED*/
	}
	/*
	 * See if we're adding a NIS entry
	 */
	Nisname = ck_4_nisuser(logname, cll, cmdlp);

	/*
	 * Even though the ``logname'' might be valid, check to
	 * see if it's ALL numeric in which case print a diagnostic
	 * indicating that this might not be such a good idea.
	*/
	if (all_numeric(logname)) {
		(void) pfmt(stdout, MM_WARNING, errmsgs[M_NUMERIC], logname);
	}
	/*
	 * check to see if the login name already
	 * exists in the audit file.  If so, exit.
	*/
	if (audit && !Nisname) {
		struct	adtuser	adt;

		if (getadtnam(logname, &adt) == 0) {
			errmsg(M_USED, logname);
			adumprec(ADT_ADD_USR, EX_NAME_EXISTS, cll, cmdlp);
			exit(EX_NAME_EXISTS);
		}
	}
	/*
	 * get defaults for adding new users
	*/
	get_defaults();

	/*
	 * If this is a +user, reset defaults if needed.
	 * This is so they can be verified by set_defaults().
	 * Also set up Nisuid and Nisgid in case they are
	 * needed.
	 */ 
	if (Nisuser && !Nisplus){
		if (!Dir)
			Dir = passwd_nis.pw_dir;
		/*
		 * If Shell was set on command line then use it.
		 */
		if (!sflg) {
			/*
			 * If there is a shell entry in the NIS database, 
			 * use it. Otherwise, use the default shell in 
			 * default file.
			 */
			if (passwd_nis.pw_shell && *passwd_nis.pw_shell)
				Shell = passwd_nis.pw_shell;
		}
		Nisuid = passwd_nis.pw_uid;
		Nisgid = passwd_nis.pw_gid;
	}

	/*
	 * check and set the default values for new users
	 * and initialize both password structures with valid
	 * information for the new user
	*/
	need_uid = set_defaults(&passwd_st, &shadow_st, &hybrid_st, Ustr, cmdlp, 
			oflag, grps, cll, &gid, &mflag, iflag, &wflag);
	/*
	 * don't forget to set login name
	*/
	passwd_st.pw_name = logname;
	/*
	 * update the password file
	*/
	if (add_passwd(&passwd_st, need_uid)) {
		file_error(cll, cmdlp);
	}
	/*
	 * update the shadow file
	*/
	shadow_st.sp_namp = logname;
	if (add_shadow(&shadow_st)) {
		file_error(cll, cmdlp);
	}
	/*
	 * update the hybrid mapping file. Note: we don't create a hybrid
	 * mapping for a reserved uid.
	*/
	if (passwd_st.pw_uid > DEFRID) {
		if (Nisname)
			hybrid_st.unix_namp = logname+1;
		else 
			hybrid_st.unix_namp = logname;
		if (Ndsname && *Ndsname) {
			if(add_nwusers(&hybrid_st)) {
				file_error(cll, cmdlp);
			}
		}
	}
	if (!Nisname) {

		/*
		 * update the audit file
		*/
		if (audit) {
			if (add_audit(logname)) {
				file_error(cll, cmdlp);
			}
		}

		if (mac) {
			if (lvlia(IA_WRITE, (level_t **)&lidlist[0], 
						passwd_st.pw_name, &lidcnt) != 0)
				file_error(cll, cmdlp);
		}
		/*
		 * set the master structure and call putiaent
		*/
		if (add_master(&passwd_st, &shadow_st, &lidlist[0], lidcnt)) {
			file_error(cll, cmdlp);
		}

		/*
		 * add group entry
		*/
		if (grps) {
			if (edit_group(logname, (char *)0, Gidlist, 0)) {
				errmsg(M_UPDATE, "created");
				if (call_udelete(logname) != 0)
					errmsg(M_AUSAGE, (mac ? mac_str : ""),
						(audit ? aud_str : ""));
				adumprec(ADT_ADD_USR_GRP, EX_UPDATE, cll, cmdlp);
				exit(EX_UPDATE);
			}
			else {
				adumprec(ADT_ADD_USR_GRP, 0, cll, cmdlp);
			}
		}
	}

	restore_ia();

	(void) ulckpwdf();
	/*
	 * create home directory
	*/
	(void) umask(0); 

	if (Nisname && mflag){
		/*
		 * If logname is +user and there is a directory 
		 * to create, create it using Nisuid and Nisgid.
		 */
		if (*Home && Nisuser && !Nisplus) {
			if (create_home(Home, NULL, NULL, 0,Nisuid, Nisgid, 0,HomePermissions)){
				(void)call_udelete(logname);
				adumprec(ADT_ADD_USR, EX_HOMEDIR, cll, cmdlp);
				exit(EX_HOMEDIR);
			}
		}
	} else if (mflag &&
		(create_home(Home, Skel_dir, logname, 0, uid, gid, hom_lid,HomePermissions)
			!= EX_SUCCESS)) {
		(void) edit_group(logname, (char *)0, (int **)0, 1);
		if (call_udelete(logname) != 0)
			errmsg(M_AUSAGE, (mac ? mac_str : ""),
				(audit ? aud_str : ""));
		adumprec(ADT_ADD_USR, EX_HOMEDIR, cll, cmdlp);
		exit(EX_HOMEDIR);
	}

	adumprec(ADT_ADD_USR, 0, cll, cmdlp);
	exit(0);
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

		errmsg (M_NEW_PASSWD_ENT_TO_LONG);
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

		errmsg (M_NEW_PASSWD_ENT_TO_LONG);
		return 1;
	}
	return 0;
}


/*
 * Procedure:	file_error
 *
 * Notes:	issues a diagnostic message, removes all temporary files
 *		(unconditionally), allows updtate access to the password
 *		files again, writes an audit record, and exits.
 */
static	void
file_error(cmdll, cline)
	int	cmdll;
	char	*cline;
{
	char	lvlfl[64];

	errmsg (M_PASSWD_UNCHANGED);
	if (mac) {
		(void) strcpy(lvlfl, LVLDIR);
		(void) strcpy(lvlfl, logname);
		(void) unlink(lvlfl);
	}
	(void) ulckpwdf();
	adumprec(ADT_ADD_USR, 6, cmdll, cline);
	exit(6);
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
		if (!Homedir) {
			if ((Homedir = defread(def_fp, "HOMEDIR")) != NULL)
				Homedir = strdup(Homedir);
		}
		if (!Shell) {
			if ((Shell = defread(def_fp, "SHELL")) != NULL){
				Shell = strdup(Shell);
			}
		}
		if (!Skel_dir) {
			if ((Skel_dir = defread(def_fp, "SKELDIR")) != NULL)
				Skel_dir = strdup(Skel_dir);
		}
		if (!Fpass) {
			if ((Fpass = defread(def_fp, "FORCED_PASS")) != NULL)
				Fpass = strdup(Fpass);
		}
		if (audit && !Auditmask) {
			if ((Auditmask = defread(def_fp, "AUDIT_MASK")) != NULL) {
				Auditmask = strdup(Auditmask);
				if (*Auditmask) {
					if (strncmp(Auditmask, "none",
						(size_t)strlen("none")) == 0) {
							Auditmask = NULL;
					}
				}
				else {
					Auditmask = NULL;
				}
			}
		}
		if (mac && !Def_lvl) {
			if ((Def_lvl = defread(def_fp, "DEFLVL")) != NULL)
				Def_lvl = strdup(Def_lvl);
		}
		if (!Groupid) {
			if ((Groupid = defread(def_fp, "GROUPID")) != NULL)
				Groupid = strdup(Groupid);
		}
		if (!Expire) {
			if ((Expire = defread(def_fp, "EXPIRE")) != NULL)
				Expire = strdup(Expire);
		}
		if (!Inact) {
			if ((Inact = defread(def_fp, "INACT")) != NULL)
				Inact = strdup(Inact);
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

	if (!Shell || !*Shell)
		Shell = SHELL;
	if (!Def_lvl || !*Def_lvl)
		Def_lvl = DEF_LVL;
	if (!Groupid || !*Groupid)
		Groupid = "other";
	if (!Homedir || !*Homedir)
		Homedir = HOMEDIR;
	if (!Skel_dir || !*Skel_dir)
		Skel_dir = SKEL_DIR;
	if (!HomeMode || !*HomeMode)
		HomeMode = HOME_MODE;

	{
		char *a1 =&HomeMode[0];
		int c;

		while(c = *a1++) { 
			if (c>='0' && c<='7')
				HomePermissions = (HomePermissions<<3) + (c-'0');
			else {
				errmsg(M_CHMOD_NEW, Homedir);
				exit(EX_HOMEDIR);
			}
		}
		if( HomePermissions < 0 || HomePermissions > 0777 ) {
				errmsg(M_CHMOD_NEW, Homedir);
				exit(EX_HOMEDIR);
		}
	}

	return;
}


/*
 * Procedure:	fatal
 *
 * Notes:	prints out diagnostic message, writes an audit record,
 *		and exits with the given exit value.
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
	adumprec(ADT_ADD_USR, EX_BADARG, cmdll, cline);
	(void) ulckpwdf();
	exit(EX_BADARG);
}


/*
 * Procedure:	add_passwd
 *
 * Notes:	adds the new user to the "/etc/passwd" file
 */
static	int
add_passwd(passwd, n_uid)
	struct	passwd	*passwd;
	int	n_uid;
{
	struct	passwd	*pw_ptr1p;
	FILE		*fp;
	int		uid;
	register int	error = 0,
			end_of_file = 0;

	errno = 0;

	/*
	 * this routine checks the size of the password entry.
	 * It will exit the program if there is a problem.
	*/
	if (ck_p_sz(passwd))
		return 1;

	if (n_uid) {
		/*
		 * open the password file.
		 */
 		if ((fp = fopen(PASSWD , "r")) == NULL)
			return errno;

		while (!end_of_file) {
			if ((pw_ptr1p = (struct passwd *)fgetpwent(fp)) != NULL) { 
				if (add_uid(pw_ptr1p->pw_uid, &uid_sp) == -1) {
					(void) fclose(fp);
					return 1;
				}

			} 
			else { 
				if (errno == 0)		/* end of file */
					end_of_file = 1;
				else if (errno == EINVAL) {
					error++;
					errno = 0;
				}  
				else	/* unexpected error found */
					end_of_file = 1;		
			}
		}
		(void) fclose(fp);	/* close the password file */
	
		if (error >= 1)
			errmsg (M_BADENTRY, PASSWD);
	
		uid = uid_sp->high + 1;	
		uid_sp2 = uid_sp;
		while (uid_age(CHECK, uid, 0) == 0) {
			if (++uid == uid_sp2->link->low) {
				uid_sp2=uid_sp2->link;
				uid = uid_sp2->high + 1;
			}
		}
		passwd->pw_uid = uid;
	}

 	if ((fp = fopen(PASSWD , "a+")) == NULL)
		return errno;

	if (putpwent(passwd, fp))
		error = 1;

	(void) fclose(fp);

	return error;
}


/*
 * Procedure:	add_shadow
 *
 * Notes:	adds the new user to the "/etc/shadow" file
 */
static	int
add_shadow(shadow)
	struct	spwd	*shadow;
{
	struct	spwd	*sp_ptr1p;
	FILE		*fp_temp;
	register int	error = 0;

	errno = 0;

	/*
	 * this routine checks the size of the shadow entry.
	 * It will exit the program if there is a problem.
	*/
	if (ck_s_sz(shadow))
		return 1;

 	if ((fp_temp = fopen(SHADOW, "a+")) == NULL)
		return errno;

	if (putspent(shadow, fp_temp))
		error = 1;

	(void) fclose(fp_temp);

	return error;
}


/*
 * Procedure:	add_nwusers
 *
 * Notes:	adds the new hybrid user mapping entry to 
 *		the "/etc/netware/nwusers" file
 */
static	int
add_nwusers(hybrid)
	struct	hybrid	*hybrid;
{
	FILE		*fp_temp;
	register int	error = 0;

	errno = 0;

 	if ((fp_temp = fopen(NWUSERS, "a+")) == NULL)
		return errno;

	if (fputnwent(hybrid, fp_temp))
		error = 1;

	(void) fclose(fp_temp);

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
	struct	adtuser	adtuser;
	struct	adtuser	*adtp = &adtuser;
	FILE		*fp_temp;
	register int	i, error = 0;

	errno = 0;

	(void) strcpy(adtp->ia_name, logname);

	for (i = 0; i < ADT_EMASKSIZE; i++)
		adtp->ia_amask[i] = emask[i];

 	if ((fp_temp = fopen(AUDITMASK, "a+")) == NULL)
		return errno;

	if (putadtent(adtp, fp_temp))
		error = 1;

	(void) fclose(fp_temp);

	return error;
}


/*
 * Procedure:	add_master
 *
 * Notes:	adds the information concerning the new user to the
 *		"/etc/security/ia/master" file
 */
static	int
add_master(passwd, shadow, lidlist, lidcnt)
	struct	passwd	*passwd;
	struct	spwd	*shadow;
	level_t	lidlist[];
	long	lidcnt;
{
	struct	master	*mastp,
			*mastsp;
	long	dirsz = 0;
	long	shsz  = 0;
	long	length = 0;

	int	i;

 	dirsz = (long) strlen(passwd->pw_dir);
	shsz = (long) strlen(passwd->pw_shell);
	length = (dirsz + shsz + (lidcnt * sizeof(level_t)) 
		+ (Gidcnt * sizeof(gid_t))
		+ sizeof(struct master) + 2);

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
	mastp->ia_lvlcnt = lidcnt;
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

	if (putiaent(logname, mastsp) != 0)
		return 1;

	return 0;
}


/*
 * Procedure:	no_service
 *
 * Notes:	prints out three error messages indicating that this
 *		particular service was not available at this time,
 *		writes an audit record, and exits with the value passed
 *		as exit_no.
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
	errmsg(M_AUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
	adumprec(ADT_ADD_USR, exit_no, cll, cline);
	exit(exit_no);
}


/*
 * Procedure:	set_defaults
 *
 * Notes:	this routine sets the values in the password structures
 *		for any value that can be specified via the "/etc/defaults"
 *		file, or the command line.  This is where ALL validation
 *		for those values takes place.
 */
static	int
set_defaults(pass, shad, hybrid, uidstr, cmdlp, oflg, grps, cll, gid, mflg, iflg, wflg)
	struct	passwd	*pass;
	struct	spwd	*shad;
	struct	hybrid	*hybrid;
	char		*uidstr,
			*cmdlp;
	int		oflg;
	char		*grps;
	int		cll,
			*gid,
			*mflg,
			iflg,
			*wflg;
{
	register int	i;
	struct	stat	statbuf;
	struct	group	*g_ptr;
	char		*ptr,
			*pgenp;
	struct	tm	*tm_ptr;
	long		date = 0,
			inact = -1;
	level_t		level;
	int		need_uid = 0;

	/*
	 * If we are disallowing a user or netgroup, fill in
	 * rest of password entry and return;
	 */
	if (*logname == '-') {
		pass->pw_uid = 0;
		pass->pw_gid = 0;
		pass->pw_dir = "";
		pass->pw_shell = "";
		pass->pw_passwd = "";
		pass->pw_age = "";
		pass->pw_comment = pass->pw_gecos = "";
		*mflg = 0; /* make sure */
		return(0);
	}


	/*
	 * validate the the skelton directory field regardless of
	 * where it came from (i.e., default file, command line,
	 * default value).
	*/
	if ( Skel_dir && *Skel_dir) {
		if (REL_PATH(Skel_dir)) {
			fatal(M_RELPATH, cll, cmdlp, Skel_dir, "");
		}

		if (stat(Skel_dir, &statbuf) < 0 && (statbuf.st_mode & S_IFMT)
			!= S_IFDIR) {
			fatal(M_INVALID, cll, cmdlp, Skel_dir, "directory");
		}
	}
	if (INVAL_CHAR(Comment)) {
      			fatal(M_INVALID, cll, cmdlp, Comment, "comment");
	}
	if (Comment) {
		pass->pw_comment = pass->pw_gecos = Comment;
	}
	else {
		/*
		 * no comment information.
		*/
		pass->pw_comment = pass->pw_gecos = "";
	}
	if (Nisname) {
		need_uid = 0; /* don't need uid */
		pass->pw_uid = Nisdfluid;
	} else if (uidstr) {
		/*
		 * convert argument to integer
		*/
		uid = (int) strtol(uidstr, &ptr, (int) 10);

		if (*ptr) {
			fatal(M_INVALID, cll, cmdlp, uidstr, "user id");
		}

		switch (valid_uid(uid, NULL)) {
		case NOTUNIQUE:
			if (!oflg) {
				/*
				 * override not specified
				*/
				errmsg(M_UID_USED, uid);
					adumprec(ADT_ADD_USR, EX_ID_EXISTS, cll, cmdlp);
					exit(EX_ID_EXISTS);
			}
			if (uid <= DEFRID)
			/* FALLTHROUGH */
		case RESERVED:
			(void) pfmt(stdout, MM_WARNING,
				errmsgs[M_RESERVED], uid);
			break;
		case TOOBIG:
			fatal(M_TOOBIG, cll, cmdlp, "uid", uidstr);
			break;
		case INVALID:
			fatal(M_INVALID, cll, cmdlp, uidstr, "uid");
			break;
		}
		/*
		 * had a good uid.  If the "iflag" is set check to see
		 * if it is being aged.  If so, ERROR.  Otherwise, set
		 * the value in the password structure.
		*/
		if (!iflg) {
			if (uid_age(CHECK, uid, 0) == 0) {
				errmsg(M_UID_AGED, uid);
				adumprec(ADT_ADD_USR, EX_ID_EXISTS, cll, cmdlp);
				exit(EX_ID_EXISTS);
			}
		}
		pass->pw_uid = uid;
	} else {
		need_uid = 1;
		/*
		 * create the head of the uid number list
		*/
		if ((uid_sp = (struct uid_blk *) malloc((size_t) sizeof(struct uid_blk))) == NULL) {
			errmsg(M_UNABLE_TO_ALLOC_UID_LIST);
			adumprec(ADT_ADD_USR, EX_FAILURE, cll, cmdlp);
			exit(EX_FAILURE);
		}
		uid_sp->link = NULL;
		uid_sp->low = (UID_MIN - 1);
		uid_sp->high = (UID_MIN - 1);
	}
	/*
	 * set up the new user's groupid if supplied via command line
	 * or ``/etc/default/useradd''.
	*/
	if (Groupid && *Groupid && !Nisname) {
		switch(valid_group(Groupid, &g_ptr)) {
		case INVALID:
			fatal(M_INVALID, cll, cmdlp, Groupid, "group id");
			/*NOTREACHED*/
		case TOOBIG:
			fatal(M_TOOBIG, cll, cmdlp, "gid", Groupid);
			/*NOTREACHED*/
		case UNIQUE:
			errmsg(M_GRP_NOTUSED, Groupid);
			adumprec(ADT_ADD_USR, EX_NAME_NOT_EXIST, cll, cmdlp);
			exit(EX_NAME_NOT_EXIST);
			/*NOTREACHED*/
		}
		pass->pw_gid = *gid = g_ptr->gr_gid;
	}
	else {
		pass->pw_gid = *gid = (Nisname ? Nisdflgid : 1);
	}

	/*
	 * Homedir must be an existing directory
	*/
	if (Homedir && *Homedir) {
		if (REL_PATH(Homedir)) {
			fatal(M_RELPATH, cll, cmdlp, Homedir, "");
		}
		if (INVAL_CHAR(Homedir)) {
			fatal(M_INVALID, cll, cmdlp, Homedir, "directory name");
		}
		if (stat(Homedir, &statbuf) < 0
			&& (statbuf.st_mode & S_IFMT) != S_IFDIR) {
			fatal(M_INVALID, cll, cmdlp, Homedir, "base directory");
		}
	}
	/*
	 * set homedir to home directory made from Homedir
	*/
	if (!Dir || !*Dir) {
		if (Nisname) {
			/*
			 * No home is the default for NIS users
			 */
			Home[0] = '\0';
		} else
			(void) sprintf(Home, "%s/%s", Homedir, logname);
	}
	else if (REL_PATH(Dir)) {
		fatal(M_RELPATH, cll, cmdlp, Dir, "");
	}
	else if (INVAL_CHAR(Dir)) {
		fatal(M_INVALID, cll, cmdlp, Dir, "directory name");
	}
	else (void) strcpy(Home, Dir);

	if (*mflg && *Home) {
		/*
		 * Does home directory already exist?
		*/
		if (stat(Home, &statbuf) == 0) {
			/*
			 * directory exists - don't try to create
			*/
			*mflg = 0;
			/*
			 * if wflag is set, print a WARNING message and
			 * ignore the wflag.
			*/
			if (*wflg) {
				*wflg = 0;
				(void) pfmt(stdout, MM_WARNING, errmsgs[M_INVALID_WOPT]);
			}
			if (Nisuser && !Nisplus){
				if (check_perm(statbuf,Nisuid, Nisgid, S_IXOTH) != 0)
					errmsg(M_NO_PERM, logname, Home);
			}else if (!Nisname)
				if (check_perm(statbuf,uid, pass->pw_gid, S_IXOTH) != 0)
					errmsg(M_NO_PERM, logname, Home);
		}
	}
	/*
	 * set up home directory in password structure
	 * For NIS case, only set the home directory
	 * they asked for it (i.e. dflg == 1)
	*/
	pass->pw_dir = (Nisname ? (dflg ? Home : "") : Home);

	if (Shell && *Shell) {
		int	abits = EFF_ONLY_OK | EX_OK | X_OK;

		if (REL_PATH(Shell)) {
			fatal(M_RELPATH, cll, cmdlp, Shell, "");
		}
		if (INVAL_CHAR(Shell)) {
      			fatal(M_INVALID, cll, cmdlp, Shell, "shell name");
		}
		/*
		 * check that shell is an executable file
		*/
		if (access(Shell, abits) != 0) {
			fatal(M_INVALID, cll, cmdlp, Shell, "shell");
		}
	}
	/*
	 * Set the shell for NIS users only, if they asked.
	 */
	pass->pw_shell = (Nisname ? (sflg ? Shell : "") : Shell);

	/*
	 * expiration string is a date, newer than today
	*/
	if (Expire && *Expire) {
		(void) putenv(DATMSK);
 		if ((tm_ptr = getdate(Expire)) == NULL) {
			if ((getdate_err > 0) && (getdate_err < GETDATE_ERR))
				fatal(M_GETDATE+getdate_err - 1, cll, cmdlp,
					"", "");
			else
				fatal(M_INVALID, cll, cmdlp, Expire, "expire date");
		}
		if (((date = mktime(tm_ptr)) < 0) ||
                	((shad->sp_expire = (date / DAY)) <= DAY_NOW)) {
			fatal(M_INVALID, cll, cmdlp, Expire, "expire date");
		}
	}
	else {
		shad->sp_expire = -1;
	}

	/*
	 * convert inactstr to integer
	*/
	if (Inact && *Inact) {
		inact = strtol(Inact, &ptr, 10);
		if (*ptr || inact < -1) {
			fatal(M_INVALID, cll, cmdlp, Inact, "inactivity string");
		}
	}
	shad->sp_inact = inact;

	if (!Nisname) {
		Sup_gidlist = (gid_t *) 
				malloc((sysconf(_SC_NGROUPS_MAX) * sizeof(gid_t)));

		/*
		 * put user's primary group into list so its added to master
		 * file information later.
		*/
		Sup_gidlist[Gidcnt++] = pass->pw_gid;


		if (grps && *grps) {
			if ((Gidlist = valid_lgroup(grps, pass->pw_gid))) {
				i = 0;
				while ((unsigned)Gidlist[i] != -1) {
					Sup_gidlist[Gidcnt++] = (gid_t) Gidlist[i++];
				}
			}
			else {
				adumprec(ADT_ADD_USR, EX_BADARG, cll, cmdlp);
				exit(EX_BADARG);
			}
		}
	}
	/*
	 * initialize the flag field and then set it based on
	 * the value contained in the "Fpass" variable.
	*/
	shad->sp_flag = 0;

	if (Fpass && strlen(Fpass)) {
		pgenp = (char *) malloc((int) strlen(Fpass) + 1);

		(void) sprintf(pgenp, "%s", Fpass);
		if ((strlen(pgenp) > (size_t)1) || !isprint(pgenp[0])) {
			errmsg(M_BAD_PFLG);
			exit(EX_SYNTAX);
		}
		shad->sp_flag &= (unsigned) 0xffffff00;
		shad->sp_flag |= (unsigned) pgenp[0];
	}
	else {
		shad->sp_flag &= (unsigned) 0xffffff00;
	}


	/*
	 * auditing event types or classes
	*/
	if (audit && Auditmask && *Auditmask) {
		if (cremask(ADT_UMASK, Auditmask, emask)) {
			Auditmask = NULL;
			for (i = 0; i < ADT_EMASKSIZE; i++)
				emask[i] = 0;
		}
	}

	/*
	 * if MAC is installed, the different combinations of MAC options
	 * specified via the command line or extracted from the "/etc/defaults"
	 * file must be checked for validity and correctness.
	*/
	if (mac) {
		if (lvlin(Def_lvl, &def_lid) != 0)
			fatal(M_INVALID_DLVL, cll, cmdlp, "", "");
		/*
		 * verify default level
		*/
		if (usr_lvl && *usr_lvl && Def_lvl && *Def_lvl) {
			for (i = 0; i < lidcnt; i++) {
				if (def_lid == lidlist[i]) {
					level = lidlist[0];
					lidlist[0] = def_lid;
					lidlist[i] = level;
					break;
				}
			}
			if (i == lidcnt) {
				fatal(M_INVALID_DLVL, cll, cmdlp, "", "");
			}
		}
		/*
		 * verify home dir level
		*/
		else if (usr_lvl && *usr_lvl && wflg) {
			for (i = 0; i < lidcnt; i++) {
				if (hom_lid == lidlist[i])
					break;
				else {
					fatal(M_INVALID_WLVL, cll, cmdlp, "", "");
				}
			}
		}
		else if (!usr_lvl || !*usr_lvl) {
			/*
			 * set the default login level value in lidlist to
			 * the value contained in "def_lid".  This either came
			 * from the command line, the "/etc/defaults" file, or
			 * the default value specified in this command (DEF_LVL).
			*/
			lidlist[lidcnt++] = def_lid;
			/*
			 * use default level for home dir
			*/
			if (*mflg) {
				hom_lid = def_lid;
			}
		}
		/*
		 * set the home directory level if not done already.
		 * It might be used later if create_home() is called.
		*/
		if (!hom_lid)
			hom_lid = def_lid;
	}
	/*
	 * the following fields are not set in the password or shadow
	 * structures anywhere else in the code except here.  This must
	 * be done for completeness.
	*/
	pass->pw_passwd = "x";			/* bogus password */
	pass->pw_age = "";			/* no aging info. */

	if (Nisname) {
		shad->sp_pwdp = ""; 		/* no password  */
	} else {
		shad->sp_pwdp = "*LK*"; 	/* locked password */
	}
	shad->sp_lstchg = -1; 			/* no lastchanged date */
	shad->sp_min = -1; 			/* no min */
	shad->sp_max = -1; 			/* no max */
	shad->sp_warn = -1; 			/* no warn */


	/* Set the nds_namp field for hybrid structure. */
	if (Ndsname && *Ndsname) {
		/*
 		 * the valid_ndsname routine checks to see if the ndsname is
		 * in "/etc/netware/nwusers" file
		*/
		switch (valid_ndsname(Ndsname)) {
		case INVALID:
			errmsg(M_INVALID, Ndsname, "Netware login name");
			adumprec(ADT_ADD_USR, EX_NDSNAME_INVALID, cll, cmdlp);
			(void) ulckpwdf();
			exit(EX_NDSNAME_INVALID);
			/*NOTREACHED*/

		case NOTUNIQUE:
			errmsg(M_USED, Ndsname);
			adumprec(ADT_ADD_USR, EX_NDSNAME_EXISTS, cll, cmdlp);
			exit(EX_NDSNAME_EXISTS);
			/*NOTREACHED*/
		}
		hybrid->nds_namp = Ndsname;
	}

	return need_uid;
}

/*
 * Procedure:	rec_pwd
 *
 * Notes:	unlinks the current password file ("/etc/passwd") and
 *		links the old password file to the current.  Only called
 *		if there is a problem after new password file was created.
 */
static	int
rec_pwd ()
{
	if (unlink (PASSWD) || link (OPASSWD, PASSWD))
		return -1;
	return 0;
}


/*
 * Procedure:	ck_4_nisuser
 *
 * Notes:	Checks the new login name and returns 1 if the user
 *		being added is an NIS user; otherwise it returns 0.
 */
static	int
ck_4_nisuser(logname, cll, cmdlp)
	char	*logname;
	int	cll;
	char	*cmdlp;
{
	register int	nisname = 0;
	struct	passwd	*nobody;

	if (*logname == '+' || *logname == '-') {
		nisname = 1;
		/*
		 * Set default NIS uid and gids
		 */
		if (nobody = (struct passwd *)nis_getpwnam("nobody")) {
			Nisdfluid = nobody->pw_uid;
			Nisdflgid = nobody->pw_gid;
		}

		/*
		 * Now see if it is a +user entry.
		 */
		if (*logname != '-' && *(logname+1) != '@')
			Nisuser = 1;

		if (!strcmp(logname, "+"))
			Nisplus = 1;
	}
	/*
	 * If we are adding a +user NIS entry, make sure NIS is up.
	 */
	if (Nisuser && !Nisplus) {
		int niserr = 0;
		if ((niserr = nis_getuser(logname+1, &passwd_nis)) != 0) {
			switch (niserr) {
			case YPERR_YPBIND:
				errmsg (M_NIS_UNAVAILABLE);
				adumprec(ADT_ADD_USR, EX_FAILURE, cll, cmdlp);
				exit (EX_NO_NIS);
				break;
			case YPERR_KEY:
				errmsg (M_NIS_PASSWDMAP, logname);
				adumprec(ADT_ADD_USR, EX_FAILURE, cll, cmdlp);
				exit (EX_NO_NISMATCH);
				break;
			default:
				errmsg (M_NIS_UNKNOWN);
				adumprec(ADT_ADD_USR, EX_FAILURE, cll, cmdlp);
				exit (EX_UNK_NIS);
				break;
			}
		}
	}
	return nisname;
}


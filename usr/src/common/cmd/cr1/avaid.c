/*		copyright	"%c%" 	*/

#ident	"@(#)avaid.c	1.2"
#ident  "$Header$"

/***************************************************************************
 * Function:	ava_id()
 *
 * Usage:	ava_id(char *logname, char *fn_default)
 *
 * Inheritable Privileges:	P_MACREAD(?)
 *
 * Files:
 *		/etc/default/<fn_default>
 *		/etc/security/ia/index
 *		/etc/security/ia/master
 *
 * Notes:	None
 *
 ***************************************************************************/
/* LINTLIBRARY */
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>			/* For logfile locking */
#include <string.h>
#include <sys/stat.h>
#include <termio.h>
#include <sys/param.h> 
#include <deflt.h>
#include <mac.h>
#include <ia.h>
#include <audit.h>
#include <errno.h>
#include <iaf.h>
#include <sys/stream.h>
#include <sys/tp.h>

/*
 * The following defines are macros used throughout ava_id()
 */

#define	STRNCAT(to, from)	{ (void) strncat((to), (from), \
				 	sizeof((to)) - strlen(to) - 1); }
#define STRPRINT(to, from)	{ (void) sprintf((to) + strlen(to), "%lu", \
					(unsigned long)(from)); }
#define PUTAVA(v, a)		{ if ((a = putava(v, a)) == NULL) return(NULL); }
#define delava(v, a)		(a)

/*
 * The following defines are for different files.
*/

#define	SHELL1		"/usr/bin/sh"
#define	SHELL2		"/sbin/sh"

/*
 * The following defines are for DEFAULT values.
 */

#define	DEF_TZ		"EST5EDT"
#define	DEF_HZ		"100"
#define	DEF_PATH	"/usr/bin"

/*
 * The following defines don't fit into the MAXIMUM or DEFAULT
 * categories listed above.
 */

#define	LNAME_SIZE	  32	/* size of logname */
#define	TTYN_SIZE	  15	/* size of logged tty name */

extern	int 	errno,
		putenv(),
		lvlproc(),
		fdevstat(),
		auditctl(),
		tp_fgetinf();

extern	long	atol(),
		wait();

extern  FILE	*defopen();

extern  char	*getenv(),
		*strdup(),
		*strchr(),
		*strcat(),
		*ttyname(),
		*defread(),
		*sttyname();

extern	time_t	time();

static	char	**getargs(),
		*findttyname(),
		*findrttyname();

static	char	u_name[LNAME_SIZE],
		def_lvl[LVL_MAXNAMELEN + 1],
		usr_lvl[LVL_MAXNAMELEN + 1],
		Auditmask[128]		= { "AUDITMASK=" },
		Env[1024]		= { "ENV=" },
		Gid[16]			= { "GID=" },
		Gidcnt[16]		= { "GIDCNT=" },
		Home[256]		= { "HOME=" },
		Hz[10]			= { "HZ=" },
		Lid[32]			= { "LID=" },
		Logname[LNAME_SIZE + 8]	= { "LOGNAME=" },
#ifndef	NO_MAIL
		Mail[LNAME_SIZE + 15]	= { "MAIL=/var/mail/" },
#endif
		Path[64]		= { "PATH=" },
		SGid[256]		= { "SGID=" },
		Shell[256]		= { "SHELL=" },
		Term[30]		= { "TERM=" },
		Tty[32]			= { "TTY=" },
		Tz[100]			= { "TZ=" },
		Uid[16]			= { "UID=" },
		Ulimit[16]		= { "ULIMIT=" };

static	char	*auditmask	= NULL,
		*env		= NULL,
		*home		= NULL,
		*hz		= NULL,
		*logname	= NULL,
		*path		= NULL,
		*shell		= NULL,
		*term		= NULL,
		*tty		= NULL,
		*ttyn		= NULL,
		*tz		= NULL;
		
static	char	*Altshell	= NULL,
		*Def_tz		= NULL,
		*Def_path	= NULL,
		*Def_term	= NULL,
	 	*Def_hz		= NULL;

static	long	Def_ulimit	= 0;

static	int	verify_macinfo();

static	void	usage(),
		init_defaults();

static	char	**setup_avas();

static	uinfo_t	uinfo;

static	uid_t	ia_uid;
static	gid_t	ia_gid,
		gidcnt,
		*ia_sgidp;

static	long	lvlcnt;

static	char	*ia_dirp,
		*ia_shellp;

static	adtemask_t ia_amask;

static	level_t *ia_lvlp,
		lid,
		level		= 0,
		proc_lid	= 0,
		def_lvllow	= 0,
		def_lvlhigh	= 0;

static	actl_t	actl;

/*
 * Procedure:	ava_id()
 *
 * Restrictions:
 *		ia_openinfo:	P_MACREAD
 */
char **
ava_id(user, fn_default)
	char	*user;		/* user's logname, or NULL */
	char	*fn_default;	/* namea of "defaults" file to use */
{
	register int	mac = 0,
			audit = 0;

	char 	**ava_p;

	errno = 0;

	/*
	 * Get any existing AVA's from the module
	 */

	ava_p = retava(0);

	/*
	 * make sure we have a user name. If we were given a
	 * name, use it, otherwise see if LOGNAME is already
	 * set in the AVA list.
	 */

	if (user)
		logname = user;
	else if ( (logname = getava(Logname, ava_p)) == NULL )
		return(NULL);

	/*
	 * Determine if AUDIT is installed
	 */

	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == 0)
		audit = actl.auditon;
	else if (errno != ENOPKG)
		return(NULL);

	/*
	 * Determine if MAC is installed
	 */

	if (lvlproc(MAC_GET, &proc_lid) == 0) 
		mac = 1;
	else if (errno != ENOPKG)
		return(NULL);

	/*
	 * Initialize info from defaults file
	 */

	init_defaults(fn_default, mac);

	/*
	 * get the info for this logname
	 */

	if ( ia_openinfo(logname, &uinfo) || (uinfo == NULL) )
		return(NULL);

	ia_get_uid(uinfo, &ia_uid);
	ia_get_gid(uinfo, &ia_gid);
	(void) ia_get_sgid(uinfo, &ia_sgidp, &gidcnt);
	ia_get_dir(uinfo, &ia_dirp);
	ia_get_sh(uinfo, &ia_shellp);
	if (audit)
		ia_get_mask(uinfo, ia_amask);
	if (mac && ia_get_lvl(uinfo, &ia_lvlp, &lvlcnt)) {
		ia_closeinfo(uinfo);
		return(NULL);
	}

	/*
	 * if devicename is not already set, call findttyname(0)
	 * and findrttyname(0). Otherwise assume it is right.
	 */

	ttyn = getava(Tty, ava_p);

	if (ttyn == NULL) {
		ttyn = findttyname(0);
		tty = findrttyname(0);
	} else
		tty = ttyn;

	/*
	 * if running MAC, verify possible user choice of levels
	 */

	if (mac && !verify_macinfo(ava_p)) {
		ia_closeinfo(uinfo);
		return(NULL);
	}

	/*
	 * set up the AVA list, including ENV
	 */

	if ((ava_p = setup_avas(ava_p, audit, mac)) == NULL) {
		ia_closeinfo(uinfo);
		return(NULL);
	}

	/*
	 * commit the appropriate information to the stream
	 * module for the process that called this function.
	 */

	if (setava(0, ava_p) != 0) {
		ia_closeinfo(uinfo);
		return(NULL);
	}

	ia_closeinfo(uinfo);
	return(ava_p);
}

/*
 * Procedure:	findttyname
 *
 * Restrictions:
 *		access(2):	none
 *		ttyname:	none
 *
 * Notes:	call ttyname(), but do not return syscon, systty,
 *		or sysconreal do not use syscon or systty if console
 *		is present, assuming they are links.
*/
static	char *
findttyname(fd)
	int	fd;
{
	char	*lttyn;

	lttyn = ttyname(fd);

	if (lttyn == NULL)
		lttyn = "/dev/???";
	else if ( ((strcmp(lttyn, "/dev/syscon") == 0) ||
		   (strcmp(lttyn, "/dev/sysconreal") == 0) ||
		   (strcmp(lttyn, "/dev/systty") == 0)) &&
		  (access("/dev/console", F_OK) == 0) )
			lttyn = "/dev/console";

	return lttyn;
}

/*
 * Procedure:	findrttyname
 *
 * Restrictions:
 *		access(2):	none
 *
 * Notes:	Get the real/physical tty device special file name
 *		if the given file descriptor is a Trusted Path
 *		Data channel, otherwise call findttyname().
 *		Return /dev/console if name returned is /dev/systty,
 *		/dev/syscon, or /dev/sysconreal.
*/
static	char *
findrttyname(fd)
	int	fd;
{
	char		*lttyn;
	struct tp_info	tpinf;
	struct stat	statbuf;

    
	if (tp_fgetinf(fd, &tpinf) == -1){
		return (findttyname(fd));
	}

	statbuf.st_rdev = tpinf.tpinf_rdev;
	statbuf.st_dev = tpinf.tpinf_rdevfsdev;
	statbuf.st_ino = tpinf.tpinf_rdevino;
	statbuf.st_mode = tpinf.tpinf_rdevmode;
	lttyn = sttyname(&statbuf);

	if (lttyn == NULL)
		lttyn = "/dev/???";
	else if (((strcmp(lttyn, "/dev/syscon") == 0) ||
		 (strcmp(lttyn, "/dev/sysconreal") == 0) ||
		 (strcmp(lttyn, "/dev/systty") == 0)) &&
		 (access("/dev/console", F_OK) == 0))
			lttyn = "/dev/console";

	return lttyn;
}



/*
 * Procedure:	init_defaults
 *
 * Restrictions:
 *		defopen:	None
 *		defread:	None
 *		lvlin:		None
 *		lvlvalid:	None
 *
 * Notes:	reads the default file from the "/etc/defaults"
 *		directory.  Also initializes other variables used
 *		throughout the code.
*/
static	void
init_defaults(fn_default, mac)
	char	*fn_default;
	register int	mac;
{
	FILE *defltfp;
	register char	*ptr;

	if ((defltfp = defopen(fn_default)) != NULL) {

		if ((Altshell = defread(defltfp, "ALTSHELL")) != NULL)
			if (*Altshell)
				Altshell = strdup(Altshell);
			else
				Altshell = NULL;

		if ((Def_hz = defread(defltfp, "HZ")) != NULL)
			if (*Def_hz)
				Def_hz = strdup(Def_hz);
			else
				Def_hz = NULL;

		if ((Def_path = defread(defltfp, "PATH")) != NULL)
			if (*Def_path)
				Def_path = strdup(Def_path);
			else
				Def_path = NULL;

		if ((Def_tz = defread(defltfp, "TIMEZONE")) != NULL)
			if (*Def_tz)
				Def_tz = strdup(Def_tz);
			else
				Def_tz = NULL;

		if ((ptr = defread(defltfp, "ULIMIT")) != NULL)
			Def_ulimit = atol(ptr);

		if (mac) {
			if ((ptr = defread(defltfp, "SYS_LOGIN_LOW")) != NULL) {
				if (*ptr) {
					if (lvlin(ptr, &def_lvllow))
						def_lvllow = 0;
					else if(lvlvalid(&def_lvllow))
						def_lvllow = 0;
				}
			}
			if ((ptr = defread(defltfp, "SYS_LOGIN_HIGH")) != NULL) {
				if (*ptr) {
					if (lvlin(ptr, &def_lvlhigh))
						def_lvlhigh = 0;
					else if(lvlvalid(&def_lvlhigh))
						def_lvlhigh = 0;
				}
			}
		}

		(void) defclose(defltfp);
	}

	/*
	 * Now set anything which isn't already set from defaults
	 */

	if ((Def_hz == NULL) && ((Def_hz = getenv("HZ")) == NULL))
		Def_hz = DEF_HZ;

	if ((Def_tz == NULL) && ((Def_tz = getenv("TZ")) == NULL))
		Def_tz = DEF_TZ;

	if (Def_path == NULL)
		Def_path = DEF_PATH;

	/*
	 * Only set the environment variable "TERM" if it
	 * already exists in the environment.  This allows features
	 * such as doconfig with the port monitor to work correctly
	 * if an administrator specifies a particular terminal for a
	 * particular port.
	 *
	 */

	Def_term = getenv("TERM");

	return;
}

/*
 * Procedure:	setup_avas
 *
 * Restrictions: 
 *		access(2):	None
 *
 * Notes:	Set up the required AVAs as well as the basic
 *		environment for the exec.  This includes HOME,
 *		PATH, LOGNAME, SHELL, TERM, HZ, TZ, and MAIL.
 */
static	char **
setup_avas(ava_p, audit, mac)
	register char	**ava_p;
	int	audit,
		mac;
{
	int i;
	char 	*temp_p;
	char	**env_p,
		**new_env,
		**tp;

	/*
	 * actually, SHELL, HOME, LOGNAME, MAIL, and PATH are illegal,
	 * too, but since they are overwritten, they need not be checked.
	 */

	char *illegal[3] = {
		"CDPATH=",
		"IFS=",
		0
		};

	/*
	 * extract ENV= from current AVA list
	 */

	temp_p = getava(Env, ava_p);
	env_p = strtoargv(temp_p);
	new_env = NULL;

	if (env_p) {
		/*
		 * if we have an ENV variable, we will create
		 * a new list using putava(). This will eliminate
		 * duplicates. We'll also exclude illegal ones
		 * during the copying process.
		 */
		for (tp = env_p; *tp; tp++) {
			register char **p;
	
			for (p = illegal; *p; p++)
				if (!strncmp(*tp, *p, strlen(*p)))
					break;

			if (!(*p))
				new_env = putava(*tp, new_env);
		}

		/*
		 * free the original list and work with the new one
		 */

		free(env_p);
		env_p = new_env;
	}

	/*
	 * We'll do these in alphabetical order but save ENV for last
	 */

	if (audit) {
		for (i = 0; i < ADT_EMASKSIZE; i++) { 
			STRPRINT(Auditmask, ia_amask[i]);
			STRNCAT(Auditmask, ",");
		}
		PUTAVA(Auditmask, ava_p);
	}

	STRPRINT(Gid, ia_gid);
	PUTAVA(Gid, ava_p);

	if (gidcnt) {
		STRPRINT(Gidcnt, gidcnt);
		PUTAVA(Gidcnt, ava_p);

		for (i = 0; i < gidcnt; i++) { 
			STRPRINT(SGid, *ia_sgidp++);
			STRNCAT(SGid, ",");
		}

		PUTAVA(SGid, ava_p);
	}

	STRNCAT(Home, ia_dirp);
	PUTAVA(Home, ava_p);
	PUTAVA(Home, env_p);

	STRNCAT(Hz, Def_hz);
	PUTAVA(Hz, env_p);

	if (mac) {
		STRPRINT(Lid, lid);
		PUTAVA(Lid, ava_p);
	}

	STRNCAT(Logname, logname);
	PUTAVA(Logname, ava_p);
	PUTAVA(Logname, env_p);

#ifndef	NO_MAIL
	STRNCAT(Mail, logname);
	PUTAVA(Mail, env_p);
#endif

	STRNCAT(Path, Def_path);
	PUTAVA(Path, env_p);

	if (ia_shellp[0] == '\0') {
		/*
		 * if none provided, use the primary default shell
		 * if possible, otherwise, use the secondary one.
		 */
		if (access(SHELL1, X_OK) == 0)
			shell = SHELL1;
		else
			shell = SHELL2;
	} else { 
			shell = ia_shellp;
	}

	STRNCAT(Shell, shell);
	PUTAVA(Shell, ava_p);
	if (Altshell && strcmp(Altshell, "YES") == 0)
		PUTAVA(Shell, env_p);

	if (Def_term) {
		STRNCAT(Term, Def_term);
		PUTAVA(Term, env_p);
	}

	STRNCAT(Tty, tty);
	PUTAVA(Tty, ava_p);

	if (Def_tz) {
		STRNCAT(Tz, Def_tz);
		PUTAVA(Tz, env_p);
	}

	STRPRINT(Uid, ia_uid);
	PUTAVA(Uid, ava_p);

	STRPRINT(Ulimit, Def_ulimit);
	PUTAVA(Ulimit, ava_p);

	temp_p = argvtostr(env_p);

	STRNCAT(Env, temp_p);
	PUTAVA(Env, ava_p);

	return(ava_p);

}

/*
 * Procedure:	verify_macinfo
 *
 * Restrictions:
 *		lvlin:		None
 *		lckpwdf:	None
 *		putiaent:	None
 *		ulckpwdf:	None
 *		lvlvalid:	None
 *		fdevstat(2):	None
 *
 * Notes:	Used to check any user supplied MAC information if
 *		MAC is installed.  If any check fails, it returns
 *		0.  On success, 1.
*/
static	int
verify_macinfo(ava_p)
	char	**ava_p;
{
	struct	devstat	devstat;
	register int	i,
			good_lid = 0;
	level_t	*lvlp, tmplvl;
	char *lid_p;

	/*
	 * If LID is already specified, make sure it is valid.
	 * Otherwise, pick the default for this user.
	 */

	if ( (lid_p = getava("LID=", ava_p)) != NULL ) {
		lid = atol(lid_p);
		lvlp = ia_lvlp;
		for (i = 0; i < lvlcnt; i++, lvlp++) {
			if (lvlequal(&lid, lvlp) > 0) {
				good_lid++;
				break;
			}
		}
		if (!good_lid)
			return 0;
	} else
		lid = *ia_lvlp;		/* use the user's default level */

	if (lvlvalid(&lid))
		return 0;

	/*
	 * check level against device range
	 */

	if (fdevstat(0, DEV_GET, &devstat) == 0)
		if ((lvldom(&devstat.dev_hilevel, &lid) <= 0) ||
			(lvldom(&lid, &devstat.dev_lolevel) <= 0))
			return 0;

	/*
	 * check level against login range - if set
	 */

	if (def_lvlhigh)
		if (lvldom(&def_lvlhigh, &lid) <= 0)
			return 0;

	if (def_lvllow)
		if (lvldom(&lid, &def_lvllow) <= 0)
			return 0;

	return 1;
}

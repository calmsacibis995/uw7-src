/*		copyright	"%c%" 	*/


#ident	"@(#)su.c	1.6"
#ident  "$Header$"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/***************************************************************************
 * Command:	su
 *
 * Fixed Privileges:		None
 * Inheritable Privileges:	P_MACREAD,P_MACWRITE,P_SETUID,P_AUDIT
 *				P_SETFLEVEL
 *
 * Files: 	/var/adm/sulog
 *		/etc/default/su
 *		/etc/security/ia/index
 *		/etc/security/ia/master
 *
 * Notes:	change userid, `-' changes environment.
 *
 *		If SULOG is defined, all attempts to su to another user are
 *		logged there.
 *
 *		If CONSOLE is defined, all successful attempts to su to a
 *		privileged ID (in an ID-based privilege environment) are also
 *		logged there.
 *
 *		If PROMPT is defined (and ``No''), the user will not be prompted
 *		for a password. 
 *
 *		If su cannot create, open, or write entries into SULOG,
 *		(or on the CONSOLE, if defined), the entry will not be logged
 *		thus losing a record of the su commands attempted during this
 *		period.
 *
 **************************************************************************/
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <crypt.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <ia.h>
#include <mac.h>
#include <deflt.h>
#include <libgen.h>
#include <errno.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/secsys.h>
#ifdef NIS
#include <dlfcn.h>
#endif

#define	ELIM					   128
#define	DEFFILE					   "su"	/* default file M000 */

#define	PATH				     "/usr/bin:/usr/ccs/bin"
#define	SUPATH		"/sbin:/usr/sbin:/usr/bin:/etc:/usr/ccs/bin"

extern	void	ia_closeinfo();

extern	char	**environ,
		*dirname();

extern	int	lvlin(),
		lvlfile();

static	uid_t	uid,
		r_uid;

static	uinfo_t	uinfo;

static	char	*ttyn;

static	void	to(),
		log(),
		envalt(),
		err_out();

static	int	prompt = 1,		/* default is to prompt as always */
		ask_prompt();

static	char	*nptr,
		*whoami,
		*cptr,
		*Sulog = NULL,
		*Prompt = NULL,
		*Console = NULL;

static	FILE	*su_ptr,
		*con_ptr,
		*open_con();

/*
 * Procedure:	main
 *
 * Restrictions:
 *		ttyname:	none 
 *		stat:		none
 *		lvlfile:	none
 *		chown:		none
 *		fopen:		none
 *		getpass:	none
 *		setgroups:	none
 *		fclose:		none
 *		ia_openinfo():	none
 *		defopen():	none
 *		open():		none
 *		setgid():	none
 *		setuid():	none
 *		chdir():	none
 *		execv:		P_ALLPRIVS
 *		execl:		P_ALLPRIVS
 *
 * Notes:	main procedure of "su" command.
*/
main(argc, argv)
	int	argc;
	char	**argv;
{
	char		*term,
			*path,
			*supath,
			*hz, *tz,
			*ia_pwdp, 
			*ia_dirp,
			*password,
			*ia_shellp,
			*Sulog_dir,
			*Path = NULL,
			*Supath = NULL,		/* M004 */
			*envinit[ELIM],
			*str = "PATH=",
			*pshell = "/sbin/sh",	/*default shell*/
			*c = NULL,  			/*temp shell pointer */
			*tznam;

 	static char	 su[16] = "su",		/*arg0 for exec of pshell*/
			ttynam[64] = "_TTYNAME=",
			hzname[10] = "HZ=",
			termtyp[64] = "TERM=",		/* M002 */
			logname[30] = "LOGNAME=",
			homedir[BUFSIZ] = "HOME=";

	int	eflag = 0,
		envidx = 0;
		
#ifdef NIS
	int nisuser;
#endif

	FILE	*def_fp;
	uid_t	privid;
	gid_t	gid;
	gid_t   *ia_sgidp;
	long	gidcnt;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:su");

	privid = (uid_t)secsys(ES_PRVID, 0);

	errno = 0;

	/*
	 * Don't clear the P_MACREAD privilege for any open() call.
	 * In a system running a ``B2'' level of security, the only way
	 * to get to ``su'' is to be at a level that dominates all
	 * security-relevent data files.
	 *
	 * If this is a system that is running at level ``B1'', but with
	 * ``su'' at a level dominated by ALL users, then ``su'' should
	 * work provided the user is allowed to acquire a new USER-ID.
	*/
	if (argc > 1 && *argv[1] == '-') {
		eflag++;		/*set eflag if `-' is specified*/
		argv++;
		argc--;
	}
	/* 
	 * determine specified userid, get their master file entry,
	 * and set variables to values in master file entry fields
	*/
	nptr = (argc > 1) ? argv[1] : "root";

	/*
	**	Later, when I log this in the logfile, I want
	**	to log from who - to who.  So, I will save my
	**	current logname, now.
	*/
	whoami = cuserid((char *)0);

	if((ttyn = ttyname(0)) == NULL)
		if((ttyn = ttyname(1)) == NULL)
			if((ttyn = ttyname(2)) == NULL)
				ttyn = "/dev/???";
	
	if ((def_fp = defopen(DEFFILE)) != NULL) {
		if ((cptr = defread(def_fp, "SULOG")) != NULL)
			if (*cptr)
				Sulog = strdup(cptr);

		if ((Prompt = defread(def_fp, "PROMPT")) != NULL) {
			if (*Prompt) {
				/*
	 			 * if the keyword ``PROMPT'' existed and it
				 * had a value, determine if that value was
				 * the string ``No''.  If not, then ``su''
				 * should prompt as always.  Otherwise, set
	 			 * the indicator so ``su'' doesn't prompt.
	 			 *
	 			 * NOTE:	setting the keyword PROMPT to
				 *		``No'' on a system that doesn't
				 *		have the ES Package installed
				 *		is a severe security violation.
	 			 *		The administrator must be aware
				 *		of this possible violation.
				 */
				if (!strcmp(Prompt, "No")) {
					prompt = 0;
				}
			}
		}
		if ((cptr = defread(def_fp, "CONSOLE")) != NULL)
			if (*cptr)
				Console = strdup(cptr);

		if ((cptr = defread(def_fp, "PATH")) != NULL)
			if (*cptr)
				Path = strdup(cptr);

		if ((cptr = defread(def_fp, "SUPATH")) != NULL)
			if (*cptr)
				Supath = strdup(cptr);

		(void) defclose(def_fp);
	}
	/*
	 * if Sulog defined, create SULOG if it does not exist with
	 * mode read/write user. Change owner and group to that of the
	 * directory SULOG resides in.
	 *
	 * Note:	DON'T turn OFF the P_MACWRITE privilege so that
	 *		the file can be written by ANY level process
	 *		assuming, of course, that the user is allowed
	 *		to "exec" the "su" command in the first place.
	*/
	if (Sulog != NULL) {
		struct	stat	f_buf;
		struct	stat	dirbuf;

		if (stat(Sulog, &f_buf) < 0) {
			if((Sulog_dir = dirname(strdup(Sulog))) == NULL)
				err_out(privid, EAGAIN, 2,"Out of memory\n","");
			(void) stat(Sulog_dir, &dirbuf);
			(void) close(open(Sulog, O_WRONLY | O_APPEND | O_CREAT,
				(S_IRUSR|S_IWUSR)));
			(void) lvlfile(Sulog, MAC_SET, &dirbuf.st_level);
			(void) chown(Sulog, dirbuf.st_uid, dirbuf.st_gid);
		}
		su_ptr = fopen(Sulog, "a");
	}
	if (Console != NULL) {
		con_ptr = open_con(Console);
	}

	if (ia_openinfo(nptr, &uinfo) || (uinfo == NULL)) {
		uid = -1;
		err_out(privid, errno, 1, ":26:Unknown user id: %s\n", nptr);
	}

	path = (char *) malloc((Path ? strlen(Path) : strlen(PATH)) + strlen(str) + 1);
	supath = (char *) malloc((Supath ? strlen(Supath) : strlen(SUPATH)) + strlen(str) + 1);

	/* check for failed malloc() */
	if(path == NULL || supath == NULL)
			err_out(privid, EAGAIN, 2,"Out of memory\n","");

	(void) strcpy(path, str);
	(void) strcat(path, (Path) ? Path : PATH);

	(void) strcpy(supath, str);
	(void) strcat(supath, (Supath) ? Supath : SUPATH);

	ia_get_uid(uinfo, &uid);
	ia_get_gid(uinfo, &gid);

	ia_get_logpwd(uinfo, &ia_pwdp);
	if (ask_prompt(ia_pwdp, privid)) {
		if((password = getpass(gettxt(":352", "Password:"))) == NULL){
			err_out(privid, EPERM, 2, ":353:Sorry\n", "");
		}

#ifdef NIS
		nisuser = nischeck(nptr);
		if (nisuser == 0) password[8] = '\0';
#endif
		
		if((strcmp(ia_pwdp, (char *)bigcrypt(password, ia_pwdp)) != 0)) {
			err_out(privid, EPERM, 2, ":353:Sorry\n", "");
		}
	}
	/*
	 * set the new users group id.
	*/
	if (setgid(gid) != 0) {
		err_out(privid, errno, 2, ":841:Invalid GID\n", "");
	}
	/*
	 * Initialize the supplementary group access list
	*/
	ia_get_sgid(uinfo, &ia_sgidp, &gidcnt);
	if (gidcnt) {
		if (setgroups(gidcnt, ia_sgidp)) {
			err_out(privid, errno, 2, ":842:Invalid supplementary GIDs\n", "");
		}
	}

	/*
	 * set the new users user-ID.
	*/
	if (setuid(uid) != 0) {
		err_out(privid, errno, 2, ":843:Invalid ID\n", "");
	}
	/*
	 * set environment variables for new user;
	 * arg0 for exec of pshell must now contain
	 * `-' so that environment of new user is given
	*/
	if (eflag) {
		ia_get_dir(uinfo, &ia_dirp);
		(void) strcat(homedir, ia_dirp);
		(void) strcat(logname, nptr);		/* M003 */
		if ((hz = getenv("HZ")) != NULL)
			(void) strcat(hzname, hz);
		if ((tz = getenv("TZ")) != NULL) {
			if((tznam = (char *) malloc (strlen(tz) + 4 )) == NULL)
				err_out(privid, EAGAIN, 2,"Out of memory\n","");
			if (tznam) {
				(void) strcpy(tznam, "TZ=");
				(void) strcat(tznam, tz);
			}
		}
		else {
			if((tznam=(char *) malloc (4)) == NULL)
				err_out(privid, EAGAIN, 2,"Out of memory\n","");
			(void) strcpy(tznam, "TZ=");
		}
		/*
		 * this chdir might fail if the directory of the new user
		 * we are su'ing to is not at the level of this process.
		*/
		(void) chdir(ia_dirp);
		envinit[envidx = 0] = homedir;
		envinit[++envidx] = ((uid == privid) ? supath : path);
		envinit[++envidx] = logname;
		envinit[++envidx] = hzname;
		envinit[++envidx] = tznam;
		if ((term = getenv("TERM")) != NULL) {
			(void) strcat(termtyp, term);
			envinit[++envidx] = termtyp;
		}
		strcat(ttynam, ttyn);
		envinit[++envidx] = ttynam;
		envinit[++envidx] = NULL;
		environ = envinit;
		(void) strcpy(su, "-su");
	}
	/*
	 * if new user's shell field is not NULL or equal to /sbin/sh,
	 * set:
	 *	pshell = their shell
	 *	su = [-]last component of shell's pathname
	*/
	ia_get_sh(uinfo, &ia_shellp);
	if (*ia_shellp != '\0' && (strcmp(pshell, ia_shellp) != 0) ) {
		if((cptr=malloc(strlen(ia_shellp)+1)) == NULL)
			err_out(privid, EAGAIN, 2,"Out of memory\n","");
		pshell = strcpy(cptr, ia_shellp);
		(void) strcpy(su, eflag ? "-" : "" );
		if(c = (char *) strrchr(pshell, '/'))
			strcat(su, c + 1);
		else
			err_out(privid, EPERM, 2, ":353:Sorry\n", "");
			
	}
	/*
	 * close the master file so the file descriptor is not
	 * inherited by the exec'ed process.  Besides, it's no
	 * longer needed.
	*/
	ia_closeinfo(uinfo);

	log(su_ptr, nptr, whoami, 1);	/*log entry*/

	/*
	 * if new user is privileged (in an ID-based privilege mechanism):
	*/
	if (uid == privid) {
		/*
	 	 * if eflag not set, change environment to that of the
	  	 * privileged ID.
		*/
		if (!eflag)
			envalt(supath);
		/*
		 * if CONSOLE is defined, log there only if the privilege
		 * mechanism is ID-based AND the new user identity is that
		 * of the ``privileged-ID''.
	        */
		log(con_ptr, nptr, whoami, 1);	/*log entry*/
	}
	/*
	 * unconditionally close the file pointer to the console.
	*/
	(void) fclose(con_ptr);
	/*
	 * Clear the maximum privileges if we are running with
	 * a privilege mechanism that is file based.  Otherwise,
	 * pass on whatever privilege this user may have had.
	*/
	if (privid < 0) {
		(void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), 0);
	}
	/*
	 * if additional arguments, exec shell program with array
	 *    of pointers to arguments:
	 *	-> if shell = /sbin/sh, then su = [-]su
	 *	-> if shell != /sbin/sh, then su = [-]last component of
	 *					     shell's pathname
	*/
	if (argc > 2) {
		argv[1] = su;
		(void) execv(pshell, &argv[1]);
	}
	/*
	 *
	 * if no additional arguments, exec shell with arg0 of su su_ptr:
	 *	-> if shell = /sbin/sh, then su = [-]su
	 *	-> if shell != /sbin/sh, then su = [-]last component of
	 *					     shell's pathname
	*/
	else {
		(void) execl(pshell, su, (char *)0);
	}

	pfmt(stderr, MM_ERROR, ":355:No shell\n");
	exit(3);

	/* NOTREACHED */
}


/*
 * Procedure:	envalt
 *
 * Notes:	This routine is called when a user is su'ing to the
 *		privileged ID (in an ID-based privilege mechanism)
 *		without specifying the - flag.  The user's PATH and
 *		PS1 variables are reset to the correct value for that
 *		ID.  All of the user's other environment variables
 *		retain 	their current values after the su (if they are
 *		exported).
*/
static	void
envalt(newpath)
	char	*newpath;
{
	char	*suprmt = "PS1=# ";
	static const char nomem[] = ":312:Out of memory: %s\n";

	/*
	 * If user has PATH variable in their environment, change its value
	 *		to /bin:/etc:/usr/bin ;
	 *  if user does not have PATH variable, add it to the user's
	 *		environment;
	 *  if either of the above fail, an error message is printed.
	*/
	if ((putenv(newpath)) != 0) {
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
		exit(4);
	}
	/*
	 * If user has PROMPT variable in environment, change its value to #;
	 * if user doesn't have PROMPT variable, add it to the environment;
	 * if either of the above fail, an error message is printed.
	*/
	if ((putenv(suprmt)) != 0) {
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
		exit(4);
	}
	return;
}


/*
 * Procedure:	log
 *
 * Notes:	Logging routine
 *
 * Restrictions:
 *		cuserid:	none
 *		fclose:		none
 *
 *		towho = specified user (user being su'ed to)
 *		how = 0 if su attempt failed; 1 if su attempt succeeded
*/
static	void
log(fptr, towho, fromwho, how)
	FILE	*fptr;
	char	*towho;
	char	*fromwho;
	int	how;
{
	long now;
	struct tm *tmp;

	now = time(0);
	tmp = localtime(&now);

	/*
	 * check if the "fptr" variable is valid.
	 * If so, write out to where it points.
	*/
        if (fptr != NULL) {
		(void) fprintf(fptr,"SU %.2d/%.2d %.2d:%.2d %c %s %s-%s\n",
			tmp->tm_mon+1,tmp->tm_mday,tmp->tm_hour,tmp->tm_min,
			how?'+':'-',(ttyn + sizeof("/dev/") -1),
			fromwho, towho); 
		(void) fclose(fptr);	/* close the file */
	}
	return;
}


/*
 * Procedure:	ask_prompt
 *
 * Notes:	"su" will NOT prompt for a password if:
 *
 *			1)  the new user does not require a password,
 *
 *			2)  the privilege mechanism supports ID-based
 *			    privileges and the real user is privileged.
 *
 *			3)  the real UID is 0.  "su" never prompted in
 *			    this case, so don't start now.  If the user's
 *			    real UID is 0, but doesn't have adequate
 *			    privilege, the "su" will fail anyway.
 *
 *			4)  the PROMPT keyword is defined and equal to
 *			    the string ``No''.  If this is the case, the
 *			    indicator "prompt" will be 0.  Otherwise it
 *			    will be 1.			    
*/
static	int
ask_prompt(usr_pwdp, privid)
	char	*usr_pwdp;
	uid_t	privid;
{
	r_uid = getuid();

	if (!*usr_pwdp) {			/* #1, above */
		return 0;
	}
	if (privid == r_uid) {			/* #2, above */
		return 0;
	}
	if (!r_uid) {				/* #3, above */
		return 0;
	}

	return prompt;				/* #4, above */
}


/*
 * Procedure:	to
 *
 * Notes:	called in "main" routine when logging the "su"
 *		attempt to the console (if defined).  It is
 *		called if the alarm is caught.
*/
static	void
to() {}


/*
 * Procedure:	err_out
 *
 * Notes:	called in many places by "main" routine to print
 *		the appropriate error message and exit.  The string
 *		"Sorry" is printed if the errno is equal to EPERM.
 *		Otherwise, the "alt_msg" message is printed.
*/
static	void
err_out(privid, err, astatus, alt_msg, arg)
	uid_t	privid;
	register int	err;
	register int	astatus;
	register char	*alt_msg;
	char	*arg;
{
	switch (err) {
	case EPERM:
		(void) pfmt(stderr, MM_ERROR, ":353:Sorry\n");
		break;
	case EACCES:
	case EAGAIN:
		(void) pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(err));
		break;
	default:
		if (*arg) {
			(void) pfmt(stderr, MM_ERROR, alt_msg, arg);
		}
		else {
			(void) pfmt(stderr, MM_ERROR, alt_msg);
		}
		break;
	}

	/*
	 * If the /var/adm/sulog file is defined,
	 * log this as a bad attempt.
	*/
	log(su_ptr, nptr, whoami, 0);	/*log entry*/

	/*
	 * Only log this failure to CONSOLE iff the
	 * new user-ID has been set and it's equal to
	 * the privileged ID (in an ID-based privilege
	 * mechanism).
	*/
	if (uid != -1) {
		/*
	 	 * close the master file.
		*/
		ia_closeinfo(uinfo);

		if (uid == privid) {
			log(con_ptr, nptr, whoami, 0);
		}
	}

	(void) fclose(con_ptr);

	exit(astatus);
}


/*
 * Procedure:	open_con
 *
 * Restrictions:
 *		fopen:	none
 *
 * Notes:	returns a file pointer to the named file ``namep''.
 *		Only waits 30 seconds to open the file.  If the
 *		alarm goes off, NULL is returned.
*/
static	FILE	*
open_con(namep)
	char	*namep;
{
	FILE	*fptr = NULL;

	(void) sigset(SIGALRM, (void (*)())to);
	(void) alarm(30);
	fptr = fopen(namep, "a");
	(void) alarm(0);
	(void) sigset(SIGALRM, SIG_DFL);

	return fptr;
}

/* Checks to see if user is NIS user. */

#ifdef NIS
int
nischeck(namep)
char    *namep;
{
	struct passwd *p;
	
	static void *dl;
	static struct passwd *(*nsptr)();
	if (dl == NULL){
		if ((dl = dlopen("/usr/lib/ns.so", RTLD_LAZY)) == NULL)
			dl = &dl;
	}
	if (dl == &dl)
		return(-1);
	if (nsptr == NULL)
		nsptr = (struct passwd *(*)())dlsym(dl, "nis_getpwentry");
	if (nsptr == NULL || (p = (*nsptr)(namep)) == NULL)
		return(-1);
	
	return 0;
}
#endif

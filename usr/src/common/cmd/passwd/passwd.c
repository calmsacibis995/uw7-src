/*		copyright	"%c%" 	*/

#ident	"@(#)passwd.c	1.8"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */
/*
 * Usage:
 *
 * Administrative:
 *
 *		passwd [name]
 *		passwd [-l|-d] [-n min] [-f] [-x max] [-w warn] name
 *		passwd -s [-a]
 *		passwd -s [name]
 *
 * Non-administrative:
 *
 *	 	passwd [-s] [name]
 *
 * Level:	SYS_PUBLIC
 *
 * Inheritable Privileges:	P_DEV,P_MACWRITE,P_DACREAD,P_DACWRITE,
 *				P_SYSOPS,P_SETFLEVEL,P_OWNER
 *
 * Fixed Privileges:		P_AUDIT,P_MACREAD
 *
 * Files:	/etc/passwd
 *		/etc/shadow
 *		/etc/.pwd.lock
 *		/etc/default/passwd
 *		/etc/security/ia/index
 *		/etc/security/ia/master
 *
 * Notes:	Passwd is a program whose sole purpose is to manage 
 *		the password file. It allows the password administrator
 *		the ability to add, change and display password attributes.
 *		Non-password administrators can change password or display 
 *		password attributes which corresponds to their login name.
 */

/* LINTLIBRARY */
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <time.h>
#include <string.h>
#include <ctype.h>         /* isalpha(c), isdigit(c), islower(c), toupper(c) */
#include <errno.h>
#include <crypt.h>
#include <deflt.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ia.h>
#include <iaf.h>
#include <audit.h>
#include <priv.h>
#include <sys/mac.h>
#include <sys/systeminfo.h>
#include <locale.h>
#include <pfmt.h>
#include <limits.h>

#include <sys/wait.h>

#define	PWADMIN		"passwd"
#define	MAXSTRS		     20	/* max num of strings from password generator */
#define	MINWEEKS	     -1	/* minimum weeks before next password change */
#define	MAXWEEKS	     -1	/* maximum weeks before password change */
#define	MINLENGTH	      6	/* minimum length for passwords */
#define	WARNWEEKS	     -1	/* number of weeks before password expires
				   to warn the user */

#define ENCRYPTED_PASSWORD_LENGTH	112

/* flags indicate password attributes to be modified */

#define	LFLAG	0x001		/* lock user's password  */
#define	DFLAG	0x002		/* delete user's  password */
#define	MFLAG	0x004		/* max field -- # of days passwd is valid */
#define NFLAG	0x008		/* min field -- # of days between passwd mods */
#define SFLAG	0x010		/* display password attributes */
#define FFLAG	0x020		/* expire  user's password */
#define AFLAG	0x040		/* display password attributes for all users*/
#define	PFLAG	0x080		/* change user's password */  
#define WFLAG	0x100		/* warn user to change passwd */
#ifdef PASSWD_STDIN
# define EFLAG	0x200		/* take encrypted string from stdin */
#endif
#define	PWGEN	0x0ff		/* passgen byte in flag field */
#define SAFLAG	(SFLAG|AFLAG)	/* display password attributes for all users*/

/* exit codes */

#define SUCCESS		0	/* succeeded */
#define NOPERM		1	/* No permission */
#define BADSYN		2	/* Incorrect syntax */
#define FMERR		3	/* File manipulation error */
#define FATAL		4	/* Old file can not be recover */
#define FBUSY		5	/* Lock file busy */
#define BADOPT		6	/* Invalid argument to option */
#define UFAIL		7	/* Unexpected failure  */
#define NOID		8	/* Unknown ID  */
#define	BADAGE		9	/* Aging is disabled	*/
 
/* local macros */

#define	SCPYN(a, b)	(void) strncpy((a), (b), (sizeof((a))-1))

/* define error messages */

const	char 
	*sorry = ":353:Sorry\n",
	*MSG_NP = ":354:Permission denied\n";
static	const	char 
	*again = ":364:Try again later.\n",
/* usage message of non-password administrator */
 	*usage  = ":362:Usage: passwd [-s] [name]\n",
	*MSG_NV = ":356:Invalid argument to option -%c\n",
	*MSG_BS = ":357:Invalid combination of options\n",
	*MSG_FE = ":815:Unexpected failure. Password file(s) unchanged.\n",
	*MSG_FF = ":816:Unexpected failure. Password file(s) missing.\n",
	*MSG_FB = ":360:Password file(s) busy.\n",
	*MSG_UF = ":817:Unexpected failure.\n",
	*MSG_UI = ":818:Unknown logname: %s\n",
	*MSG_AD = ":361:Password aging is disabled\n",
	*MSG_NO = ":819:Password may only be changed at login time\n",
	*badusage = ":8:Incorrect usage\n",
	*syntax = ":233:Syntax error\n",
	*badentry = ":365:Bad entry found in the password file.\n",
	*dupuid = ":1248:More than one login name with UID: %d\n",
	*pwgenerr = ":820:Must select an offered value\n",
	*pwgenmsg = ":821:Select a password from the following:\n\n",
	*pwchgmsg = ":1254:passwd name\n",
/* usage message of password administrator */
 	*sausage  = ":363:Usage:\n\tpasswd [name]\n\tpasswd  [-l|-d]  [-n min] [-f] [-x max] [-w warn] name\n\tpasswd -s [-a]\n\tpasswd -s [name]\n";

#define FAIL 		-1

/* print password status */

#define PRT_PWD(pwdp)	{\
	if (*pwdp == NULL) \
		 (void) fprintf(stdout, "NP  ");\
	else if (strlen(pwdp) < (size_t) PASS_MAX) \
		(void) fprintf(stdout, "LK  ");\
	else		(void) fprintf(stdout, "PS  ");\
}

#define PRT_AGE()	{\
	if (mastp->ia_max != -1) { \
		if (mastp->ia_lstchg) {\
			lstchg = mastp->ia_lstchg * DAY;\
			tmp = gmtime(&lstchg);\
			(void) fprintf(stdout,"%.2d/%.2d/%.2d  ",(tmp->tm_mon + 1),tmp->tm_mday,tmp->tm_year%100);\
		} else			(void) fprintf(stdout,"00/00/00  ");\
		if ((mastp->ia_min >= 0) && (mastp->ia_warn > 0))\
                        (void) fprintf(stdout, "%ld  %ld	%ld ", mastp->ia_min, mastp->ia_max, mastp->ia_warn);\
            	else if (mastp->ia_min >= 0) \
                        (void) fprintf(stdout, "%ld  %ld	", mastp->ia_min, mastp->ia_max);\
		else if (mastp->ia_warn > 0) \
                        (void) fprintf(stdout, "    %ld	%ld ",  mastp->ia_max, mastp->ia_warn);\
 		else \
                     	(void) fprintf(stdout, "    %ld	", mastp->ia_max);\
	}\
}

extern	int	errno,
		optind,
		munmap(),
		lvlfile(),
		getiasz(),
		auditctl(),
		auditdmp(),
		putiaent();

extern	char	*strdup(),
		*dirname(),
		*getpass();

extern	long	wait();

extern	FILE	*fdopen();

extern	struct	passwd	*getpwuid();

extern	struct	master	*nis_getmaster();
extern	int	nis_getflags();
extern	int	nis_display();
extern	int	nis_passwd();

static	struct	index	index,
			*indxp = &index;
static	struct	master	*mastp;

char	Xopass[10];
char 	*pw;

static	char Xnpass[10],
		Xlogname[128];

static	int	circ(),
		ckarg(),
		update(),
		display(),
		pwd_adm(),
		ck_ifadm(),
		ck_xpass(),
		ck_passwd(),
		update_sfile();
int	std_pwgen();

int	graphics = 0;
static	int 	retval = 0,
		nisusr = 0,			/* flag indicating NIS user */ 
		login_only = 0,
	 	mindate, maxdate,		/* password aging information */
	 	warndate, minlength;

static	void	adumprec(),
		call_pwgen(),
		init_defaults();

static	FILE	*get_pwgen(),
		*open_pwgen();
FILE	*err_iop = stderr;
struct	passwd	*p_ptr;

#ifdef PASSWD_STDIN
	char	pwstr[32];
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	int	flag = 0,
		nisnam = 0,	/* flag indicating name beginning with '+' */
		nissdw = 0,	/* flag indicating NIS user with local passwd */
		nispwd = 0,
		pw_admin;
	uid_t	uid;
	char	*uname;
	char	*lname;		/* local name used to search SHADOW */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:passwd");

	/*
	 * Call ck_xpass() to determine if passwd was called by the
	 * graphical port monitor.  If so, then all communication
	 * via stdin, stdout, and stderr is avoided.
	 */
	graphics = ck_xpass();

	/*
	 * Get the real user ID.  The effective user ID will always
	 * be 0 since "passwd" is a setuid-on-exec to "root" program.
	 */
	uid = getuid();

	/*
	 * Call the init_defaults() routine to set the tunables
	 * used by "passwd".
	 */
	init_defaults();

	/*
	 * A return value > 0 from ck_ifadm indicates that this
	 * is being run by a password administrator. Obviously,
	 * 0 means non-administrator.
	 */
	pw_admin = ck_ifadm(uid);

	/* 
	 * The routine ckarg() parses the arguments.  In case of
	 * an error it sets the retval and returns FAIL (-1).  It
	 * returns the value indicating which password attributes
	 * to modify.
	 */
	if (!graphics) {
		switch (flag = ckarg(argc, argv, pw_admin, uid)) {
		case FAIL:		/* failed */
			adumprec(ADT_PASSWD, retval, uid);
			exit(retval); 
			/* NOTREACHED */
		case SAFLAG:		/* display password attributes */
			adumprec(ADT_PASSWD, retval, uid);
			exit(display((struct master *) NULL));
			/* NOTREACHED */
		default:	
			break;
		}
	
		argc -= optind;
	
		if (argc < 1) {
	
			char	*namp;
			int	name = 0;
			int	file_end = 0;
	
		namp = getenv("LOGNAME");

		if ((p_ptr = getpwnam(namp)) == NULL) { 
			(void) pfmt(err_iop, MM_ERROR, MSG_UI, namp);
			adumprec(ADT_PASSWD, NOID, -1);
			exit(NOID);
		}
		if (uid == p_ptr->pw_uid)
			uname = p_ptr->pw_name;
		else {
 			setpwent();
			while (!file_end) {
				if ((p_ptr = (struct passwd *) getpwent()) != NULL) {
					if (uid == p_ptr->pw_uid) {
						uname = (char *) malloc (strlen(p_ptr->pw_name)+1);
						strcpy(uname, p_ptr->pw_name);
						if (++name > 1) {
							errno = 0;
							file_end = 1;
						}
					}
				} 
				else { 
					if (errno == 0)	/* end of file */
						file_end = 1;
					else if (errno == EINVAL) {
						errno = 0;
					}  
					else	/* unexpected error found */
						file_end = 1;		
				}
			}
	
			if (name == 0 && errno) {
				errno = 0;
				(void) pfmt(err_iop, MM_ERROR, badusage);
				(void) pfmt(err_iop, MM_ACTION, !pw_admin ? usage : sausage);
				adumprec(ADT_PASSWD, NOPERM, uid);
				exit(NOPERM);
			}
			if (name > 1 && errno == 0) {
				(void) pfmt(err_iop, MM_ERROR, dupuid, uid);
				(void) pfmt(err_iop, MM_ACTION, pwchgmsg);
				adumprec(ADT_PASSWD, NOID, -1);
				exit(NOID);
			}
		}
		endpwent();	/* close the password file */
	
			/*
		  	 * If flag set, must be displaying or modifying
		 	 * password aging attributes.
			 */
			if (!flag)
				(void) pfmt(err_iop, MM_INFO,
				":366:Changing password for %s\n", uname);
		}
		else
			uname = argv[optind];
	}
	else {
		/*
		 * Must have been called in a graphical manner.
		 * If so, check to see if this can only be done
		 * at login time by a process with appropriate
		 * privilege.
		 */
		if (login_only && (access(SHADOW, W_OK) != 0)) {
			adumprec(ADT_PASSWD, BADOPT, uid);
			exit(BADOPT);
		}
		uname = &Xlogname[0];
	}

	lname = uname;

	(void) strcpy(indxp->name, uname);
	if (getiasz(indxp) != 0) {
		/*
		 * This person is not in the ia database, but they
		 * might be a NIS user.  Check to see.
		 */
		if (nisusr = nis_getflags(uname, &nisnam, &nissdw, &nispwd)) {
			if (!nisnam && !nissdw) {
				if (flag == SFLAG)
					retval = nis_display();
				else 
					retval = nis_passwd(pw_admin, flag);
				exit(retval);
				/* NOTREACHED */
			}
			if (nisnam) {
				uname = &uname[1];
			} 
			else {	/* !nisname && nissdw */
				lname = (char *) malloc (strlen(uname)+2);
				strcpy(lname, "+");
				strcat(lname, uname);
			}
		}
		else {
			(void) pfmt(err_iop, MM_ERROR, MSG_UI, uname);
			adumprec(ADT_PASSWD, NOID, -1);
			exit(NOID);
		}
	}
	/*
	 * At this point, uname is the unadorned user name.  If uname
	 * is a NIS name or a NIS user with a corresponding entry in 
	 * SHADOW, lname is the username with a prepended '+'.  
	 * Otherwise, lname is the same as uname.
	 */  

	/*
	 * If this is a NIS name or a NIS user whose password is
	 * administered locally, get a "fake" master record.
	 *
	 * NOTE: If NIS is not available, the "fake" master
	 * structure will have a -1 UID.  This will cause the
	 * permissions check below to fail unless this user is
	 * a password administrator. 
	 */
	if (nisnam || nissdw) {
		if ((mastp = nis_getmaster(lname)) == NULL) {
			(void) pfmt(err_iop, MM_ERROR, MSG_UI, lname);
			adumprec(ADT_PASSWD, NOID, -1);
			exit(NOID);
		}
	}
	else {
		mastp = (struct master *) malloc(indxp->length);
	
		if (mastp == NULL) {
			(void) pfmt(err_iop, MM_ERROR, MSG_UF);
			adumprec(ADT_PASSWD, UFAIL, uid);
			exit(UFAIL);
		}
		if (getianam(indxp, mastp) != 0) {
			(void) pfmt(err_iop, MM_ERROR, MSG_UF);
			adumprec(ADT_PASSWD, UFAIL, uid);
			exit(UFAIL);
		}
	}
	/*
	 * The following statement determines who is allowed to
	 * modify a password.  If this user isn't a password
	 * administrator and the real uid isn't equal to the
	 * user's password that was requested or MAXUID, then
	 * the user doesn't have permission to change the password.
	 *
	 * NOTE:	a real uid of MAXUID indicates that this
	 *		was called from the login scheme.
	 */
	if (!graphics && !pw_admin && uid != mastp->ia_uid && uid != MAXUID) {
		(void) pfmt(err_iop, MM_ERROR, MSG_NP);
		adumprec(ADT_PASSWD, NOPERM, mastp->ia_uid);
		exit(NOPERM);
	}

	/*
	 * For NIS entries, we impose the additional restriction that
    	 * only a password administrator may create a local password
	 * for a user whose password previously came from NIS.  
	 */
	if (nisusr && !nissdw && !pw_admin) { 
		(void) pfmt(err_iop, MM_ERROR, MSG_NP);
		adumprec(ADT_PASSWD, NOPERM, mastp->ia_uid);
		exit(NOPERM);
	}

	/*
	 * If the flag is set display/update password attributes.
	 */
	switch (flag) {
	case SFLAG:		/* display password attributes */
		adumprec(ADT_PASSWD, retval, mastp->ia_uid);
		exit(display(mastp));
		/* NOTREACHED */
	case 0:			/* changing user password */
		break;
	default:		/* changing user password attributes */
		/* lock the password file */
		if (lckpwdf() != 0) {
			(void) pfmt(err_iop, MM_ERROR, MSG_FB);
			(void) pfmt(err_iop, MM_ACTION, again);
			adumprec(ADT_PASSWD, FBUSY, mastp->ia_uid);
			exit(FBUSY);
		}
#ifdef PASSWD_STDIN
		/*
		 * if we provided an encrypted string, make it look like
		 * it came from ck_passwd()
		 */
		if(flag & EFLAG) {
			flag |= PFLAG;
			pw = pwstr;
			}
#endif
		retval = update(flag, lname);
		(void) ulckpwdf();
		adumprec(ADT_PASSWD, retval, mastp->ia_uid);
		exit(retval);
	}

	/*
	 * Prompt for the users new password, check the triviality
	 * and verify the password entered is correct.
	 */
	if ((retval = ck_passwd(&flag, pw_admin, uname)) != SUCCESS) {
		adumprec(ADT_PASSWD, retval, mastp->ia_uid);
		exit(retval);
	}

	/* lock the password file */
	if (lckpwdf() != 0) {
		(void) pfmt(err_iop, MM_ERROR, MSG_FB);
		(void) pfmt(err_iop, MM_ACTION, again);
		exit(FBUSY);
	}
	flag |= PFLAG;
	retval = update(flag, lname);
	(void) ulckpwdf();
	adumprec(ADT_PASSWD, retval, mastp->ia_uid);
	exit(retval);
	/* NOTREACHED */
}


/* 
 * Procedure:	ck_passwd
 *
 * Notes:	Verify users old password. It also checks password 
 *		aging information to verify that user is authorized
 *		to change password.  Also, prompt for the users new 
 *		password, check the triviality and verify that the
 *		password entered is correct.
 */ 
static	int
ck_passwd(flagp, adm, uname)
	int	*flagp,
		adm;
	char	*uname;
{
	/*
	 * In the original UnixWare code the following was true:
	 *
	 * Passwd calls getpass() to get the password. The getpass() routine
	 * returns at most 8 charaters. Therefore, array of 10 chars is
	 * enough for buf, pwbuf, and opwbuf.
	 *
	 * Long password support has been added, the new getpass() routine
	 * accepts passwords up to 80 characters in length. So buf, pwbuf and
	 * opwbuf have been increased accordingly.
	 *
	 * NOTE: PASS_MAX is set to 80 in include/limits.h
	 */
	register int	now,
			err = 0;
	char	*pswd,
		buf[PASS_MAX],
	 	saltc[2],		/* bigcrypt() takes 2 char string as salt */
		pwbuf[PASS_MAX],			
		opwbuf[PASS_MAX];
	time_t 	salt;
	int	ret, i, c,		/* for triviality checks */
		pw_genchar = 0;
	FILE	*iop;

	ret = SUCCESS;
	pwbuf[0] = buf[0] = '\0';		/* used for "while" loop */

	/*
	 * If the user isn't a password administrator and they had an
	 * old password, get it and verify.
	 */
	if (!graphics) {
		if (!adm && *mastp->ia_pwdp) {
			if ((pswd = getpass(gettxt(":378", "Old password:"))) == NULL) {
				(void) pfmt(stderr, MM_ERROR, sorry);
				return FMERR;
			}
			else {
				(void) strcpy(opwbuf, pswd);
				pw = (char *) bigcrypt(opwbuf, mastp->ia_pwdp);
			}
			if (strcmp(pw, mastp->ia_pwdp) != 0) {
				(void) pfmt(stderr, MM_ERROR, sorry);
				return NOPERM;
			}
		}
		else {
			opwbuf[0] = '\0';
		}
	}
	else {
		(void) strcpy(opwbuf, Xopass);

		pw = (char *) bigcrypt(opwbuf, mastp->ia_pwdp);
		if (strcmp(pw, mastp->ia_pwdp) != 0)
			return NOPERM;
	}
	/* password age checking applies */
	if (mastp->ia_max != -1 && mastp->ia_lstchg != 0) {
		now  =  DAY_NOW;
		if (mastp->ia_lstchg <= now) {
			if (!graphics && !adm
				&& (now < (mastp->ia_lstchg + mastp->ia_min))) { 

				(void) pfmt(err_iop, MM_ERROR,
					":379:Sorry: < %ld days since the last change\n",
					mastp->ia_min);
				return NOPERM;
			}
			if (!graphics && !adm && (mastp->ia_min > mastp->ia_max)) {
				(void) pfmt(err_iop, MM_ERROR,
					":380:You may not change this password\n");
				return NOPERM;
			}
		}
	}
	/* aging is turned on */
	else if (mastp->ia_lstchg == 0 && mastp->ia_max > 0 || mastp->ia_min > 0) {
		ret = SUCCESS;
	}
	else {
		/*
		 * Aging not turned on so turn on passwd for
		 * user with default values.
		 */
		*flagp |= MFLAG;
		*flagp |= NFLAG;
		*flagp |= WFLAG;
	}
	/*
	 * If this system supports password generating programs,
	 * "exec" the program now.  If the generator fails, or the
	 * the expected password generator does not exist, follow
	 * the standard password algorithm.
	 */
	if (!graphics && !adm && (pw_genchar = (mastp->ia_flag & PWGEN)) &&
		((iop = get_pwgen(pw_genchar)) != NULL)) {

		call_pwgen(iop, (char *)&buf, (char *)&pwbuf);
	}
	/*
	 * Didn't call password generator or it failed for some
	 * reason so now get the password the old-fashioned way.
	 */
	if (!strlen(buf)) {
		if ((err = std_pwgen(adm, uname, opwbuf,
				(char *)&buf, (char *)&pwbuf))) {
			return err;
		}
	}
	/*
	 * Construct salt, then encrypt the new password.
	 */
	(void) time((time_t *)&salt);
	salt += (long)getpid();

	saltc[0] = salt & 077;
	saltc[1] = (salt >> 6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c>'9') c += 7;
		if (c>'Z') c += 6;
		saltc[i] = (char) c;
	}
	pw = (char *) bigcrypt(pwbuf, saltc);

	return ret;
}


/*
 * Procedure:	update
 *
 * Notes:	Updates the password file. It takes "flag" as an argument
 *		to determine which password attributes to modify. It returns
 *		0 for success and > 0 for failure.
 *
 *		Side effect: Variable sp points to NULL.
 */
static	int 
update(flag, uname)
	int	flag;
	char	*uname;
{
	register int i, found = 0;
	struct	stat	buf;
	struct	spwd	*sp;
	int	fd,
	 	sp_error = 0,
	 	end_of_file = 0;
	level_t	lid;
	FILE	*tsfp;
	FILE	*sfp;
	char	newshadpwd[14];

	long uidstore ;

	pid_t child ;
	int status ;

	/* ignore all the signals */

	for (i = 1; i < NSIG; i++) {
		if(i!=SIGCHLD) /* we must know the child's exit status below */
			(void) sigset(i, SIG_IGN);
	}

 	/* Clear the errno. It is set because SIGKILL can not be ignored. */

	errno = 0;

	/*
	 * Get the level of the shadow file so that the temp file
	 * can be set to that level (if MAC is installed).
	 */
	if (lvlfile(SHADOW, MAC_GET, &lid) < 0)
		lid = 0;	/* couldn't get the lid */
	/*
	 * Mode of the shadow file should be 400.
	 * We should be able to read it for two reasons:
	 *
	 *	1) the process has the P_MACREAD privilege.
	 *
	 *	2) the discretionary access has been set to
	 *	   "root", owner of the shadow file.
	 */
	if (stat(SHADOW, &buf) < 0) {
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	/*
	 * Do unconditional unlink of temporary shadow file.
	 * Shouldn't exist anyway!!
	 */
	(void) unlink(SHADTEMP);
	/*
	 * Create temporary file.
	 */
	if ((fd = creat(SHADTEMP, 0600)) == FAIL) {
		/*
		 * Couldn't create temporary shadow file.
		 * Big problems, better exit with message.
		 */
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	(void) close(fd);

	if ((tsfp = fopen(SHADTEMP, "w")) == NULL) {
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}

	/*
	 * Copy passwd file to temp, replacing matching lines
	 * with new password attributes.
	 */

	end_of_file = sp_error = 0;

	/*
	 * We're going to open SHADOW directly in order to use
	 * the fgetspent() routines to parse it.  We're using
	 * fgetspent() instead of getspent() because the latter
	 * skips NIS entries while the former does not.
	 */

	if ((sfp = fopen(SHADOW, "r")) == NULL) {
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}

	uidstore = mastp->ia_uid;
	

	/*
	 * While it would seem to be good security practice to turn
	 * OFF the P_MACREAD privilege at this point, it isn't because
	 * "passwd" has that privilege in it's FIXED set to be able to
	 * READ this file.  Therefore, turning it OFF would be an ERROR.
	 */
	while (!end_of_file) {
	if ((sp = fgetspent(sfp)) != NULL) {
		if (strcmp(sp->sp_namp, uname) == 0) { 
			found = 1;
			/*
			 * LFLAG and DFLAG should be checked before FFLAG.
			 * FFLAG clears the sp_lstchg field. We do not
			 * want sp_lstchg field to be set if one execute
			 * passwd -d -f name or passwd -l -f name.
			 */

			if (flag & LFLAG) {	 /* lock password */
				sp->sp_pwdp = pw;
				sp->sp_lstchg = DAY_NOW;
				(void) strcpy(mastp->ia_pwdp, pw);
				mastp->ia_lstchg = DAY_NOW;
			} 
			if (flag & DFLAG) {	 /* delete password */
				/*
				 * Set the aging information to -1.
				 */
				sp->sp_min = -1;
				sp->sp_max = -1;
				mastp->ia_min = -1;
				mastp->ia_max = -1;
				/*
				 * NULL out the current password
				 */
				sp->sp_pwdp = "";
				sp->sp_lstchg = DAY_NOW;
				mastp->ia_pwdp[0] = '\0';
				mastp->ia_lstchg = DAY_NOW;
			} 
			if (flag & FFLAG) {	 /* expire password */
				sp->sp_lstchg = (long) 0;
				mastp->ia_lstchg = (long) 0;
			}
			if (flag & MFLAG) { 	/* set max field */
				if (!(flag & NFLAG) && mastp->ia_min == -1) {
					sp->sp_min = 0;
					mastp->ia_min = 0;
				}
				if (maxdate == -1) { 	/* turn off aging */
					sp->sp_min = -1;
					mastp->ia_min = -1;
					sp->sp_warn = -1;
					mastp->ia_warn = -1;
				}
				else if (maxdate == 0) { /* turn off aging and force new passwd */
					sp->sp_lstchg = 0;
					mastp->ia_lstchg = 0;
					sp->sp_min = -1;
					mastp->ia_min = -1;
					sp->sp_warn = -1;
					mastp->ia_warn = -1;
				}
				sp->sp_max = maxdate;
				mastp->ia_max = maxdate;
			}
			if (flag & NFLAG) {   /* set min field */
				if (mastp->ia_max == -1 && mindate != -1) {
					(void) pfmt(err_iop, MM_ERROR, MSG_AD);
					(void) pfmt(err_iop, MM_ACTION, sausage);
					endspent();
					(void) unlink(SHADTEMP);
					return BADAGE;
				}
				sp->sp_min = mindate;
				mastp->ia_min = mindate;
			}
			if (flag & WFLAG) {   /* set warn field */
				if (mastp->ia_max == -1 && warndate != -1) {
					(void) pfmt(err_iop, MM_ERROR, MSG_AD);
					(void) pfmt(err_iop, MM_ACTION, sausage);
					endspent();
					(void) unlink(SHADTEMP);
					return BADAGE;
				}
				sp->sp_warn = warndate;
				mastp->ia_warn = warndate;
			}
			if (flag & PFLAG) {	/* change password */

			/* For backwards compatibility reasons, we need to be able to use
			 * /etc/shadow passwd entries, and these have to be traditional
			 * Unix like passwds. Therefore we want to write the first 13 chars
			 * of the passwd into shadow. To get full benefit of the longer
			 * passwd apps should use bigcrypt() + check against the
			 * /etc/security/ia/master file via getianam() etc.
			 */

				(void) strncpy(newshadpwd, pw, 13);
				newshadpwd[13] = NULL;
				sp->sp_pwdp = &newshadpwd[0];
				(void) strncpy(mastp->ia_pwdp, pw, ENCRYPTED_PASSWORD_LENGTH);
				mastp->ia_pwdp[ENCRYPTED_PASSWORD_LENGTH] = NULL;
				mastp->ia_uid = uidstore;
				/* update the last change field */
				sp->sp_lstchg = DAY_NOW;
				mastp->ia_lstchg = DAY_NOW;
				if (mastp->ia_max == 0) {   /* turn off aging */
					sp->sp_max = -1;
					sp->sp_min = -1;
					mastp->ia_max = -1;
					mastp->ia_min = -1;
				}
			}
		}
		if (putspent (sp, tsfp) != 0) { 
			endspent();
			(void) unlink(SHADTEMP);
			(void) pfmt(err_iop, MM_ERROR, MSG_FE);
			return FMERR;
		}
	}
	else {
 		if (errno == 0) 
			/* end of file */
			end_of_file = 1;	
		else if (errno == EINVAL) {
			/*bad entry found in /etc/shadow, skip it */
			errno = 0;
			sp_error++;
		}  else
			/* unexpected error found */
			end_of_file = 1;
	}
	} /*end of while*/

	fclose(sfp);

	if (sp_error >= 1)
		(void) pfmt(err_iop, MM_ERROR, badentry);

	if (fclose (tsfp)) {
		(void) unlink(SHADTEMP);
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	/*
	 * Check if user name exists
	 */
	if (!found) {
		(void) unlink(SHADTEMP);
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	/*
	 * If MAC is installed set the level of the temp file.
	 */
	if (lid) {
		(void) lvlfile(SHADTEMP, MAC_SET, &lid);
	}
	/*
	 * Change the mode of the temporary file to the mode
	 * of the original shadow file.
	 */
	if (chmod(SHADTEMP, buf.st_mode) != SUCCESS) {
		(void) unlink(SHADTEMP);
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	/*
	 * Do a chown() on the temporary shadow file to the
	 * owner and group of the original shadow file.
	 */
	if (chown(SHADTEMP, buf.st_uid, buf.st_gid) != SUCCESS) {
		(void) unlink(SHADTEMP);
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	/*
	 * Rename temp file back to appropriate passwd file and return.
	 */
	
	child=fork() ;

	switch(child) {
		case -1:
			return(FATAL) ; /* fork() failed */
		case 0: /* child */
			_exit(update_sfile(SHADOW, OSHADOW, SHADTEMP, uname)) ;
		default: /* parent */
			wait(&status) ;
			if(WIFEXITED(status))
				return(WEXITSTATUS(status)) ;
			else
				return(UFAIL) ; /* unknown status */
	}
}


/*
 * Procedure:	circ
 *
 * Notes:	Compares the password entered by user with the login name
 *		of the user to determine if the password is just a circular
 *		representation of the users login name.  Since this is a
 *		common practice for ease of remembering one's password, it
 *		is disallowed for just that reason;  someone attempting to
 *		"break-in" would use this method and therefore be successful.
 */
static	int
circ(s, t)
	char *s, *t;
{
	char c, *p, *o, *r, buff[25], ubuff[25], pubuff[25];
	int i, j, k, l, m;
	 
	m = 2;
	i = strlen(s);
	o = &ubuff[0];

	for (p = s; c = *p++; *o++ = c) 
		if (islower(c))
			 c = toupper(c);

	*o = '\0';
	o = &pubuff[0];

	for (p = t; c = *p++; *o++ = c) 
		if (islower(c)) 
			c = toupper(c);

	*o = '\0';
	p = &ubuff[0];

	while (m--) {
		for (k = 0; k  <=  i; k++) {
			c = *p++;
			o = p;
			l = i;
			r = &buff[0];
			while (--l) 
				*r++ = *o++;
			*r++ = c;
			*r = '\0';
			p = &buff[0];
			if (strcmp(p, pubuff) == 0) 
				return 1;
		}
		p = p + i;
		r = &ubuff[0];;
		j = i;
		while (j--) 
			*--p = *r++;
	}
	return SUCCESS;
}


/*
 * Procedure:	ckarg
 *
 * Notes:	This function parses and verifies the 	
 *		arguments.  It takes three parameters:			
 *
 *			argc => # of arguments				
 *			argv => pointer to an argument			
 *			adm  => password administrator flag
 *
 *		In case of an error it prints the appropriate error
 *		message, sets the retval and returns FAIL(-1).
 */
static	int
ckarg(argc, argv, adm, real_uid)
	int	argc;
	char	**argv;
	int	adm;
	uid_t	real_uid;
{
	extern	char	*optarg;
	register int	c, flag = 0;
	char	*char_p,
		*lkstring = "*LK*";	/*lock string  to lock user's password*/

#ifdef PASSWD_STDIN
	while ((c = getopt(argc, argv, "aldfsx:n:w:e:")) != EOF) {
#else
	while ((c = getopt(argc, argv, "aldfsx:n:w:")) != EOF) {
#endif
		switch (c) {
		case 'd':		/* delete the password */
			/* Only password administrator can execute this */
			if (!pwd_adm(adm))
				return FAIL;
			if (flag & (LFLAG|SAFLAG|DFLAG)) {
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, sausage); 
				retval = BADSYN;
				return FAIL;
			}
			flag |= DFLAG;
			pw = "";
			break;
		case 'l':		/* lock the password */
			/* Only password administrator can execute this */
			if (!pwd_adm(adm))
				return FAIL;
			if (flag & (DFLAG|SAFLAG|LFLAG)) {
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, sausage); 
				retval = BADSYN;
				return FAIL;
			}
			flag |= LFLAG;
			pw = lkstring;
			break;
		case 'x':		/* set the max date */
			/* Only password administrator can execute this */
			if (!pwd_adm(adm))
				return FAIL;
			if (flag & (SAFLAG|MFLAG)) {
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, sausage); 
				retval = BADSYN;
				return FAIL;
			}
			flag |= MFLAG;
			if ((maxdate = (int) strtol(optarg, &char_p, 10)) < -1
			    || *char_p != '\0' || strlen(optarg) <= (size_t) 0) {
				(void) pfmt(err_iop, MM_ERROR, MSG_NV, 'x');
				retval = BADOPT;
				return FAIL;
			}
			break;
		case 'n':		/* set the min date */
			/* Only password administrator can execute this */
			if (!pwd_adm(adm))
				return FAIL;
			if (flag & (SAFLAG|NFLAG)) { 
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, sausage); 
				retval = BADSYN;
				return FAIL;
			}
			flag |= NFLAG;
			if (((mindate = (int) strtol(optarg, &char_p,10)) < 0 
			    || *char_p != '\0') || strlen(optarg) <= (size_t)0) {
				(void) pfmt(err_iop, MM_ERROR, MSG_NV, 'n');
				retval = BADOPT;
				return FAIL;
			} 
			break;
		case 'w':		/* set the warning field */
			/* Only password administrator can execute this */
			if (!pwd_adm(adm))
				return FAIL;
			if (flag & (SAFLAG|WFLAG)) { 
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, sausage); 
				retval = BADSYN;
				return FAIL;
			}
			flag |= WFLAG;
			if (((warndate = (int) strtol(optarg, &char_p,10)) < 0 
			    || *char_p != '\0') || strlen(optarg) <= (size_t)0) {
				(void) pfmt(err_iop, MM_ERROR, MSG_NV, 'w');
				retval = BADOPT;
				return FAIL;
			} 
			break;
		case 's':		/* display password attributes */
			if (flag && (flag != AFLAG)) { 
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, !adm ? usage : sausage); 
				retval = BADSYN;
				return FAIL;
			} 
			flag |= SFLAG;
			break;
		case 'a':		/* display password attributes */
			/* Only password administrator can execute this */
			if (!pwd_adm(adm))
				return FAIL;
			if (flag && (flag != SFLAG)) { 
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, sausage); 
				retval = BADSYN;
				return FAIL;
			} 
			flag |= AFLAG;
			break;
		case 'f':		/* expire password attributes */
			/* Only password administrator can execute this */
			if (!pwd_adm(adm))
				return FAIL;
			if (flag & (SAFLAG|FFLAG)) { 
				(void) pfmt(err_iop,MM_ERROR, MSG_BS);
				(void) pfmt(err_iop, MM_ACTION, sausage); 
				retval = BADSYN;
				return FAIL;
			} 
			flag |= FFLAG;
			break;
#ifdef PASSWD_STDIN
		case 'e':	/* pre-encrypted string for installation */
			strcpy(pwstr, optarg);
			flag |= EFLAG;
			break;
#endif
		case '?':
			(void) pfmt(err_iop, MM_ACTION, !adm ? usage : sausage);
			retval = BADSYN;
			return FAIL;
		}
	}

	argc -=  optind;
	if (argc > 1) {
		(void) pfmt(err_iop, MM_ERROR, badusage);
		(void) pfmt(err_iop, MM_ACTION, !adm ? usage : sausage);
		retval = BADSYN;
		return FAIL;
	}
	/*
	 * If no options are specified or there were options but
	 * NOT the show option, check if this user can modify
	 * the SHADOW file.  If not, the user is trying to do
	 * something that a non-administrator on this system is
	 * not allowed to do, so exit with an error.
	 *
	 * NOTE:	If "passwd" was called by the login scheme
	 *		the user WILL have access but won't be
	 *		considered an administrator based on the
	 *		conditions in the "ck_ifadm" routine.
	 *
	 * IMPORTANT:	Also, and this is important, if the keyword
	 * 		``LOGIN_ONLY'' is set in the tunables file
	 *		for "passwd", then the password can only be
	 *		modified via the login scheme.  This is
	 *		accomplished by the login scheme setting the
	 *		user's UID to MAXUID and calling "passwd".
	 *		The only way to do this is with privilege.
	 *		Any other method of MAXUID being set when
	 *		"passwd" is called is in violation of the
	 *		security policies governing password admin-
	 *		istration.
	 */
	if (!flag || (flag != SFLAG)) { 
		if ((access(SHADOW, (EFF_ONLY_OK|W_OK)) != 0) ||
			(!adm && login_only && real_uid != MAXUID)) {
			(void) pfmt(err_iop, MM_ERROR, MSG_NO);
			retval = BADOPT;
			return FAIL;
		}
	}
	/*
	 * If no options specified, or only the show option, return.
	 */
	if (!flag || (flag == SFLAG)) {
		return flag;
	}
	if (flag == AFLAG) {
		(void) pfmt(err_iop, MM_ERROR, badusage);
		(void) pfmt(err_iop, MM_ACTION, sausage);
		retval = BADSYN;
		return FAIL;
	}
	if (flag != SAFLAG && argc < 1) {
		(void) pfmt(err_iop, MM_ERROR, badusage);
		(void) pfmt(err_iop, MM_ACTION, sausage);
		retval = BADSYN;
		return FAIL;
	}
	if (flag == SAFLAG && argc >= 1) {
		(void) pfmt(err_iop, MM_ERROR, badusage);
		(void) pfmt(err_iop, MM_ACTION, sausage);
		retval = BADSYN;
		return FAIL;
	}
	if ((maxdate == -1) &&  (flag & NFLAG)) {
		(void) pfmt(err_iop, MM_ERROR, MSG_NV, 'x');
		retval = BADOPT;
		return FAIL;
	}
	return flag;
}

/*
 * Procedure:	display
 *
 * Notes:	Displays password attributes. It takes master struct
 *		pointer as a parameter. If the pointer is NULL then it
 *		displays password attributes for all entries on the file.
 *		Returns 0 for success and positive integer for failure.	      
 */
static	int
display(mastp)
	struct master *mastp;
{
	struct tm *tmp;
	int	fd_indx;
	int	cnt, i;
	int	max_len = 0;
	struct	index	*indxp, *bas_indxp;
	struct	stat	statbuf;
	struct	stat	*statp = &statbuf;
	long lstchg;

	if (mastp != NULL) {
		(void) fprintf(stdout,"%s  ", mastp->ia_name);
		PRT_PWD(mastp->ia_pwdp);
		PRT_AGE();
		(void) fprintf(stdout,"\n");
		return SUCCESS;
	} 
	errno = 0;

	if (stat(INDEX, &statbuf) < 0) {
 		(void) pfmt(err_iop, MM_ERROR, MSG_FF);
                return FATAL;
	}
	if ((fd_indx = open(INDEX, O_RDONLY)) < 0) {
 		(void) pfmt(err_iop, MM_ERROR, MSG_FF);
                return FATAL;
	}

	cnt = (statp->st_size/sizeof(struct index));

	if ((indxp = (struct index *)mmap(0, (unsigned)statp->st_size, PROT_READ,
                 	MAP_SHARED, fd_indx, 0)) < (struct index *) 0) {

		(void) close(fd_indx);
 		(void) pfmt(err_iop, MM_ERROR, MSG_FF);
                return FATAL;
	}
	bas_indxp = indxp;

	for (i = 0; i < cnt; i++) {
		if (indxp->length > max_len)
			max_len = indxp->length;
		indxp++;
	}

	mastp = (struct master *) malloc(max_len);

	if (mastp == NULL) {
		(void) munmap((char *) indxp, (unsigned int) statp->st_size);
		(void) close(fd_indx);
 		(void) pfmt(err_iop, MM_ERROR, MSG_FF);
                return UFAIL;
	}

	for (i = 0; i < cnt; i++) {
		if (getianam(bas_indxp, mastp) != 0) {
			(void) munmap((char *) indxp, (unsigned int) statp->st_size);
			(void) close(fd_indx);
	 		(void) pfmt(err_iop, MM_ERROR, MSG_FF);
                	return UFAIL;
		}
		(void) fprintf(stdout,"%s  ", mastp->ia_name);
                PRT_PWD(mastp->ia_pwdp);
                PRT_AGE();
                (void) fprintf(stdout, "\n");
		bas_indxp++;
	}

	(void) munmap((char *) indxp, (unsigned int) statp->st_size);
	(void) close(fd_indx);
	free(mastp);
	return SUCCESS;
}


/*
 * Procedure:	pwd_adm
 *
 * Notes:	Determines if the user is a password administrator
 *		or not.  If not, print a diagnostic message, set the
 *		return value for the process and return to the caller
 *		indicating the user is NOT an administrator.  Other-
 *		wise return to the caller indicating the user is a
 *		password administrator.
 */
static	int
pwd_adm(adm)
	register int	adm;
{
	if(!adm) {
		(void) pfmt(err_iop, MM_ERROR, MSG_NP);
		retval = NOPERM;
		return 0; 
	} 
	return 1;
}


/*
 * Procedure:	adumprec
 *
 * Notes:	Only writes the record if auditing is turned on.
 *		It determines this by checking the "auditon" flag
 *		in the audit structure.
 */
static void
adumprec(rtype, status, puid)
	int rtype;			/* event type */
	int status;			/* event status */
	uid_t puid;			/* uid of user's passwd to be changed */
{
	arec_t		rec;		/* auditdmp(2) structure */
	actl_t		actl;		/* auditctl(2) structure */
	apasrec_t	apasrec;	/* passwd record structure */

        actl.auditon = 0;

	(void) auditctl(ASTATUS, &actl, sizeof(actl_t));

	if (actl.auditon) {
		rec.rtype = rtype;
		rec.rstatus = status;
		rec.rsize = sizeof(struct apasrec);

		if (getpwuid(puid) == NULL)
		        apasrec.nuid = -1;
                else {
			/*
			 * If the passwd is being changed via login -p
			 * set the real and effective user id to the
			 * uid of the user whose passwd is being
			 * modified. Therefore the real and effective
			 * user fields of the audit record will
			 * accurately reflect the user whose passwd was
			 * modified.
			 */
			if (!graphics && getuid() == MAXUID)
				(void) setuid(puid);
		        apasrec.nuid = puid;
		}
		rec.argp = (char *)&apasrec;
		auditdmp(&rec, sizeof(arec_t));
	}
	return;
}


/*
 * Procedure:	ck_ifadm
 *
 * Notes:	A user who is considered a password administrator is
 *		granted the ability to circumvent certain restrictions
 *		regarding the compostion and control of user passwords.
 *		The "passwd" command relies on the success or failure
 *		of the sysinfo() system call to determine if the user
 *		executing "passwd" is a password administrator.
 *
 *		Further restrictions of what non-administrators can and
 *		can't do can be found in the "ckarg" routine.
 */
static	int
ck_ifadm(real_uid)
	uid_t	real_uid;
{
	register uid_t	eff_uid;
		 char	buf[257];
		 long	bsiz = 0;
		 int	r_val = 0;	/* default = non-administrator */

	eff_uid = geteuid();
	
	/*
	 * The effect of the seteuid() system call will differ depending
	 * on which privilege mechanism is installed in the kernel.  The
	 * results, however, are accurate with respect to determining if
	 * the user invoking passwd is actually a password administrator.
	 */
	(void) seteuid(real_uid);

	/*
	 * Get the current system name.
	 */
	bsiz = sysinfo(SI_HOSTNAME, (char *) &buf, (long) sizeof(buf));
	
	/*
	 * The "passwd" command uses the return value from the
	 * sysinfo() system call that sets the HOSTNAME in order
	 * to determine if the user is a password administrator or
	 * not.  If the call to sysinfo() is successful, the user is
	 * considered a password administrator.  Otherwise, the user
	 * is bounded by whatever restrictions apply to regular users.
	 *
	 * NOTE:	Any interface that calls "passwd" and requires it
	 *		to function as if a non-privileged user invoked
	 *		"passwd" must explicitily clear the P_SYSOPS
	 *		privilege in its maximum privilege set before
	 *		invoking "passwd".
	 *
	 * CAUTION:	Changing the password has dependencies on "passwd"
	 *		inheriting P_SYSOPS so don't remove the following
	 *		system call!!
	 */

	if (sysinfo(SI_SET_HOSTNAME, (char *) &buf, bsiz) != -1)
		r_val = 1;		/* administrator */

	(void) seteuid(eff_uid);

	return r_val;
}


/*
 * Procedure:	update_sfile
 *
 * Notes:	This routine performs all the necessary checks when moving
 *		the current shadow file to the old shadow file and renaming
 *		the temporary shadow file to the current shadow file.
 */
static	int 
update_sfile(sfilep, osfilep, tsfilep, uname)
	char	*sfilep;		/* shadow file */
	char	*osfilep;		/* old shadow file */
	char	*tsfilep;		/* temporary shadow file */
	char	*uname;
{
	/*
	 * First check to see if there was an existing shadow file.
	 */
	if (access(sfilep, 0) == 0) {
		/*
		 * If so, remove old shadow file.
		 */
		if (access(osfilep, 0) == 0) {
			if (unlink(osfilep) < 0) {
				(void) unlink(tsfilep);
				(void) pfmt(err_iop, MM_ERROR, MSG_FE);
				return FMERR;
			}
		}
		/*
		 * Rename shadow file to old shadow file.
		 */
		if (rename(sfilep, osfilep) < 0) {
			(void) unlink(tsfilep);
			(void) pfmt(err_iop, MM_ERROR, MSG_FE);
			return FMERR;
		}
	}
	/*
	 * Rename temporary shadow file to shadow file.
	 */
	if (rename(tsfilep, sfilep) < 0) {
		(void) unlink(sfilep);
		if (link(osfilep, sfilep) < 0) { 
			(void) pfmt(err_iop, MM_ERROR, MSG_FF);
			return FATAL;
		}
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	/*
	 * Enter the new password into the master file
	 * only if this is not an NIS user.
	 */
	if (!nisusr && putiaent(uname, mastp) != 0) {
		(void) unlink(SHADOW);
		if (link (OSHADOW, SHADOW)) { 
			(void) pfmt(err_iop, MM_ERROR, MSG_FF);
			return FATAL;
		}
		(void) pfmt(err_iop, MM_ERROR, MSG_FE);
		return FMERR;
	}
	return SUCCESS;
}


/*
 * Procedure:	get_pwgen
 *
 * Notes:	Does validation for determining what the password
 *		generator is, reads the default passwd file, and
 *		"exec" the password generator if it exists.
 *		This routine calls "open_pwgen" that creates a pipe and
 *		does a "fork()" and "exec()".
 */
static	FILE	*
get_pwgen(pw_ident)
	int	pw_ident;
{
	char	prog[BUFSIZ],
		passgen[BUFSIZ],
		*progp = &prog[0],
		*pstrp = &passgen[0];
	FILE	*iop;

	/*
	 * There is an inclination to clear the P_MACREAD privilege in
	 * this routine so that a user who doesn't have non-privileged
	 * access to the necessary files would fail, but that would be
	 * an error since ordinary users on a system with MAC installed
	 * but no file-based privilege mechanism also need access to
	 * these particular files.
	 */
	progp = '\0';
	(void) sprintf(pstrp, "PASSGEN%c", pw_ident);

	if ((iop = defopen(PWADMIN)) == NULL) {
		/*
		 * Couldn't open "passwd" default file, no generator!
		 */
		return (FILE *)NULL;
	}
	if ((progp = defread(iop, pstrp)) == NULL) {
		/*
		 * Didn't find the password generator keyword in the
		 * "passwd" default file, no generator!
		 */
		defclose(iop);
		return (FILE *)NULL;
	}
	else if (*progp) {
		progp = strdup(progp);
		defclose(iop);
		/*
		 * Found a value for a possible password generator, now
		 * check to see if it's a full pathname.  If not, error!
		 */
		if (strncmp(progp, (char *)"/", 1)) {
			return (FILE *)NULL;
		}
		/*
		 * Otherwise, try to "exec" whatever was found in the
		 * appropriate password generator field.
		 */
		if ((iop = open_pwgen(progp)) == NULL) {
			return (FILE *)NULL;
		}
		return iop;
	}
	defclose(iop);
	return (FILE *)NULL;
}


/*
 * Procedure:	open_pwgen
 *
 * Notes:	Sets up a pipe, forks and executes the password
 *		generator program.  If the exec fails it returns
 *		a value to indicate that it did fail.  On success,
 *		it returns a file pointer to the pipe.
 */
static	FILE *
open_pwgen(progp)
	char *progp;
{
	register int	i;
	int		pfd[2];
	FILE		*iop;

	(void) pipe(pfd);
	if ((i = fork()) < 0) {
		(void) close(pfd[0]);
		(void) close(pfd[1]);
		return (FILE *)NULL;
	}
	else if (i == 0) {
		(void) close(pfd[0]);
		(void) close(1);
		(void) dup(pfd[1]);
		(void) close(pfd[1]);
		for (i = 5; i < 15; i++)
			(void) close(i);

		(void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);
		(void) execl(progp, progp, 0);
		(void) close(1);
		exit(1);		/* tell parent that 'execl' failed */
	}
	else {
		(void) close(pfd[1]);
		iop = fdopen(pfd[0], "r");
		return iop;
	}
	/* NOTREACHED */
}



/* diagnostic strings for following routine */
static	const	char
	*tshort = ":898:Password is too short - must be at least %d characters\n",
	*nocirc = ":371:Password cannot be circular shift of logonid\n",
	*s_char = ":1114:Password must contain at least two alphabetic characters\n\tand at least one numeric or special character within the first\n\teight characters.\n",
	*df3pos = ":373:Passwords must differ by at least 3 positions\n",
	*tomany = ":375:Too many tries\n",
	*nmatch = ":376:They don't match\n",
	*tagain = ":377:Try again.\n";

/*
 * Procedure:	std_pwgen
 *
 * Notes:	Does the standard password algorithm checking of the
 *		user's entry for the requested password.  If it succeeds
 *		the return is 0, otherwise, an error.
 */
int
std_pwgen(adm, uname, opass, buf, pwbuf)
	int	adm;
	char	*uname,
		*opass,
		*buf,
		*pwbuf;
{
	register int	i, j, k,
			pwlen = 0,
			count = 0,
			opwlen = 0,
			insist = 0,
			tmpflag, flags, c;
	char		*pswd,
			*p, *o;

	opwlen = (int) strlen(opass);

	while (!strlen(buf)) {
		if (insist >= 3) {       /* 3 chances to meet triviality */
			(void) pfmt(err_iop, MM_ERROR, ":368:Too many failures\n");
			(void) pfmt(err_iop, MM_ACTION, again);
			return NOPERM;
		}
		if (graphics) {
			pswd = &Xnpass[0];
		}
		else if ((pswd = getpass(gettxt(":369", "New password:"))) == NULL) {
			(void) pfmt(err_iop, MM_ERROR, sorry);
			return FMERR;
		}
		(void) strcpy(pwbuf, pswd);
		pwlen = strlen(pwbuf);
		/*
		 * Make sure new password is long enough.
		 */
		if (!adm && (pwlen < minlength)) { 
			if (graphics)
				return 10;
			(void) pfmt(err_iop, MM_ERROR, tshort, minlength);
			++insist;
			continue;
		}
		/*
		 * Check the circular shift of the logonid.
		 */
		if(!adm && circ(uname, pwbuf)) {
			if (graphics)
				return 11;
			(void) pfmt(err_iop, MM_ERROR, nocirc);
			++insist;
			continue;
		}
		/*
		 * Insure passwords contain at least two alpha characters
		 * and one numeric or special character.
		 */
		tmpflag = flags = 0;
		p = pwbuf;
		if (!adm) {
			while (c = *p++) {
				if (isalpha(c) && tmpflag)
					 flags |= 1;
				else if (isalpha(c) && !tmpflag) {
					flags |= 2;
					tmpflag = 1;
				} else if (isdigit(c)) 
					flags |= 4;
				else 
					flags |= 8;
			}
			/*
			 *		7 = lca,lca,num
			 *		7 = lca,uca,num
			 *		7 = uca,uca,num
			 *		11 = lca,lca,spec
			 *		11 = lca,uca,spec
			 *		11 = uca,uca,spec
			 *		15 = spec,num,alpha,alpha
			 */
			if (flags != 7 && flags != 11 && flags != 15) {
				if (graphics)
					return 12;
				(void) pfmt(err_iop, MM_ERROR, s_char);
				++insist;
				continue;
			}
		}
		/*
		 * Old password and New password must differ by at least
		 * three characters.
		 */
		if (!adm) {
			p = pwbuf;
			o = opass;
			if (pwlen >= opwlen) {
				i = pwlen;
				k = pwlen - opwlen;
			} else {
				i = opwlen;
				k = opwlen - pwlen;
			}
			for (j = 1; j <= i; j++) 
				if (*p++ != *o++) 
					k++;
			if (k < 3) {
				if (graphics)
					return 13;
				(void) pfmt(err_iop, MM_ERROR, df3pos);
				++insist;
				continue;
			}
		}
		/*
		 * Ensure password was typed correctly, user gets 3 chances.
		 */
		if (!graphics) {
			if ((pswd = getpass(gettxt(":822", "Re-enter new password:"))) == NULL) {
				(void) pfmt(err_iop, MM_ERROR, sorry);
				return FMERR;
			}
			else 
				(void) strcpy(buf, pswd);
	
			if (strcmp(buf, pwbuf)) {
				buf[0] ='\0';
				if (++count >= 3) { 
					(void) pfmt(err_iop, MM_ERROR, tomany);
					(void) pfmt(err_iop, MM_ACTION, again);
					return NOPERM;
				} else {
					(void) pfmt(err_iop, MM_ERROR, nmatch);
					(void) pfmt(err_iop, MM_ACTION, tagain);
				}
				continue;
			}
		}
		break;			/* terminate "while" loop */
	}	/* end of "insist" while */
	return 0;	/* success */
}


/*
 * Procedure:	call_pwgen
 *
 * Notes:	Does the actual work of reading from the pipe, setting
 *		up the choices array, and prompting the user for input.
 *		Also sets the contents of buf and pwbuf for use later.
 */
static	void
call_pwgen(iop, buf, pwbuf)
	FILE	*iop;
	char	*buf,
		*pwbuf;
{
	char	*pswd,
		line[BUFSIZ],
		*linep = &line[0],
		*choices[MAXSTRS];

	int	status = 0;

	register int	i, len,
			lcnt = 0,
			mlen = 0,
			firstime = 1,
			todo = MM_INFO;

	while ((linep = fgets(linep,sizeof(line),iop)) != NULL) {
		if (lcnt > MAXSTRS) {
			(void) pfmt(err_iop, MM_ERROR, MSG_UF);
			(void) fclose(iop);
			buf = '\0';
			return;
		}
		++lcnt;
		len = strlen(linep) - 1;
		choices[(lcnt - 1)] = (char *)malloc(len + 1);
		(void) strncpy(choices[(lcnt - 1)], linep, len);
		choices[(lcnt - 1)][len] = '\0';
	}
	(void) fflush(iop);
	(void) fclose(iop);
	(void) wait(&status);
	/*
	 * If the password generator exited with 0 (success), show
	 * the user the list of available passwords to select.
	 */
	if (((status >> 8) & 0377) == 0) {
		while (lcnt) {
			if (!firstime)
				todo = MM_ACTION;
			firstime = 0;
			(void) pfmt(err_iop, todo, pwgenmsg);
			for (i = 0; i < lcnt; i++) {
				(void) printf("\t%s\n", choices[i]);
			}
			(void) printf("\n");
			pswd = getpass(gettxt(":369", "New password:"));
			if (!strlen(pswd)) {
				(void) pfmt(err_iop, MM_ERROR, pwgenerr);
				continue;
			}
			for (i = 0; i < lcnt; i++) {
				if ((size_t)strlen(pswd) < (size_t)strlen(choices[i])) {
					mlen = 8;
				}
				else {
					mlen = strlen(choices[i]);
				}
				if (!strncmp(pswd, choices[i], mlen)) {
					lcnt = 0;
					(void) strcpy(pwbuf, pswd);
					(void) strcpy(buf, pswd);
					break;
				}
			}
			if (lcnt) {
				(void) pfmt(err_iop, MM_ERROR, pwgenerr);
			}
		}
	}
	else
		buf = '\0';
	return;
}


/*
 * Procedure:	init_defaults
 *
 * Notes:	Opens the default file with the tunables for passwd
 *		if it can and sets the variables accordingly.  If it
 *		can't open the file (for any reason) it sets the
 *		variables to "hard" default values defined in the
 *		source code.
 */
static	void
init_defaults()
{
	int	temp;
	char	*char_p;
	FILE	*defltfp;

        /*
	 * Set up default values if password administration file
	 * can't be opened, or not all keywords are defined.
         */
	login_only = 0;
	mindate = MINWEEKS;
	maxdate = MAXWEEKS;
	warndate = WARNWEEKS;
	minlength = MINLENGTH;

	if ((defltfp = defopen(PWADMIN)) != NULL) { /* M005  start */
		if ((char_p = defread(defltfp, "LOGIN_ONLY")) != NULL) {
			if (*char_p) {
				/*
	 			 * If the keyword ``LOGIN_ONLY'' was defined
				 * and had a value, determine if that value
				 * was the string ``Yes''.  If so, then the
				 * user can only change the password at login
				 * time. Otherwise, passwd behaves as always.
				 */
				if (strcmp(char_p, "Yes") == 0) {
					login_only = 1;
				}
			}
		}
		if ((char_p = defread(defltfp, "PASSLENGTH")) != NULL) {
			if (*char_p) {
				temp = atoi(char_p);
				if (temp > 1 && temp < 9) {
					minlength = temp;
				}
			}
		}
		if ((char_p = defread(defltfp, "MINWEEKS")) != NULL) {
			if (*char_p) {
				temp = atoi(char_p);
				if (temp >= 0) {
					mindate = temp * 7;
				}
			}
		}
		if ((char_p = defread(defltfp, "WARNWEEKS")) != NULL) {
			if (*char_p) {
				temp = atoi(char_p);
				if (temp >= 0) {
					warndate = temp * 7;
				}
			}
		}
		if ((char_p = defread(defltfp, "MAXWEEKS")) != NULL) {
			if (*char_p) {
				if ((temp = atoi(char_p)) == FAIL) {
					mindate = -1;
					warndate = -1;
				}
				else if (temp < -1) {
					maxdate = MAXWEEKS;
				}
				else {
					maxdate = temp * 7;
				}
			}
		}
		defclose(defltfp);                  /* close defaults file */
	}

#ifdef DEBUG
	(void) printf("init_defaults: maxdate == %d, mindate == %d\n", maxdate, mindate);
#endif
	return;
}


/*
 * Procedure:	ck_xpass
 *
 * Notes:	Checks to see if the IAF module was pushed and if there is
 *		any data on the stream.  Specifically checks for the terms
 *		XLOGNAME, XOPASS and XNPASS.  All MUST be present to consider
 *		this a graphical passwd command.
 */
static	int
ck_xpass()
{
	char	*p;
	char	**avap;

	if ((avap = retava(0)) == NULL)
		return 0;

	if ((p = getava("XLOGNAME", avap)) == NULL)
		return 0;

	SCPYN(Xlogname, p);

	if ((p = getava("XOPASS", avap)) == NULL)
		return 0;

	SCPYN(Xopass, p);

	if ((p = getava("XNPASS", avap)) == NULL)
		return 0;

	SCPYN(Xnpass, p);

	err_iop = fopen("/dev/null", "a+");

	return 1;
}

/*		copyright	"%c%" 	*/

#ident	"@(#)sulogin.c	1.4"
/***************************************************************************
 * Command: sulogin
 *
 * Inheritable Privileges: P_ALLPRIVS
 *       Fixed Privileges: None
 *
 * Notes:	special login program exec'd from init to let user
 *		come up single user, or go multi straight away.
 *
 *		Explain the scoop to the user, and prompt for
 *		root password or ^D. Good root password gets you
 *		single user, ^D exits sulogin, and init will
 *		go multi-user.
 *
 *		If /etc/passwd is missing, or there's no entry for root,
 *		go single user, no questions asked.
 *
 *      	11/29/82
 *
 ***************************************************************************/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 *	MODIFICATION HISTORY
 *
 *	M000	01 May 83	andyp	3.0 upgrade
 *	- index ==> strchr.
 */

#include <sys/types.h>
#include <termio.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ia.h>
#include <utmpx.h>
#include <unistd.h>
#include <priv.h>
#include <audit.h>
#include <mac.h>
#include <errno.h>
#include <sys/secsys.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>	
#include <limits.h>
#include <sys/param.h>	

#define SCPYN(a, b)	(void) strncpy(a, b, sizeof(a))
#define UNIXWARE_CLEARTEXT_PASSWD_SIZE 8
#define UNIXWARE_ENCRYPTED_PASSWD_SIZE 13
#define UFAIL       7   /* Unexpected failure  */

static	char	*get_passwd(),
		*ttyn = NULL,
		minus[]	= "-",
		*findttyname(),
		shell[]	= "/sbin/su",
		SHELL[]	= "/sbin/sh";

extern	char	*bigcrypt(),
		*strcpy(),
		*strncpy(),
		*ttyname();

static void adumprec(int rtype, int status, uid_t puid);

extern	int	errno,
		strcmp(),
		devstat();

extern	void	free(),
		*malloc(),
		ia_closeinfo();

extern	struct	utmpx	*getutxent(),
			*pututxline();

static struct utmpx *u;

static	void	single(),
		consalloc();	/* security code to put console in public state */

static const 
	char *MSG_FE = ":10:Unexpected failure. Password file(s) unchanged.\n",
	*MSG_UF = ":11:Unexpected failure.\n",
	*truncatedwarning = "Your password was truncated to 8 characters during an operating system upgrade. Please re-enter it for conversion to the new long password standard.\n",
	*truncatedwarningid = ":12",
	*notmatched = "The Passwords did not match please re-enter.\n",
	 *notmatchedid = ":13",
	*repeatpassword = "Repeat ",
	*repeatpasswordid = ":14",
	*prmt = "password :",
	*prmtid = ":15";
char root[] = "root";


/*
 * Procedure:     main
 *
 * Restrictions:
 *		printf:		None
 *		ia_openinfo:	P_MACREAD
*/
main()
{
	uinfo_t	uinfo = NULL;
	char	*enteredpassword;	/* password read from user */
	char	*pw;	/* password to compare */
	char	*ia_pwdp;	/* password from master file */
	char	*ia_shellp;	/* shell from master file */
	char    passwordstore1[PASS_MAX];
    char    passwordstore2[PASS_MAX];
    char    passwordtobeencrypted[PASS_MAX];

	static  struct  index   index,
		*indxp = &index;
	static  struct  master  *mastp;


	(void)setlocale(LC_ALL,"");
	(void)setcat("uxsulogin");
	(void)setlabel("UX:sulogin");

	(void) procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0); 

	if ((ia_openinfo("root", &uinfo)) || (uinfo == NULL)) {
		(void) pfmt(stderr,MM_ERROR,
			":1:\n**** NO ENTRY FOR root IN MASTER FILE! ****\n");
		(void) pfmt(stderr,MM_ACTION,
			":2:\n**** RUN /sbin/creatiadb ****\n\n");		
		single(0);
	}
	ia_get_logpwd(uinfo, &ia_pwdp);
	if (ia_pwdp)
		ia_pwdp = strdup(ia_pwdp);
	ia_get_sh(uinfo, &ia_shellp);
	if (ia_shellp)
		ia_shellp = strdup(ia_shellp);
	ia_closeinfo(uinfo);

	(void) procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0); 

	if (ia_pwdp == NULL)		/* shouldn't happen */
		ia_pwdp = "";		/* but do something reasonable */

	for (;;) {
		(void) pfmt(stdout,MM_NOSTD,
			":3:\nType Ctrl-d to proceed with normal startup,\n(or give root password for Single User Mode): ");

		if((enteredpassword = get_passwd()) == NULL)
			exit(0);	/* ^D, so straight to multi-user */


		if ((strlen(enteredpassword) > UNIXWARE_CLEARTEXT_PASSWD_SIZE) &&
				(strlen(ia_pwdp) == UNIXWARE_ENCRYPTED_PASSWD_SIZE)) {

		/* It appears that the O.S upgrade may have taken place. The user has
		 * passed 2 tests, (a) He has entered a long password (>8), and  (b) the
		 * entry in /etc/ia/master is a password 13 characters in length.
		 * The next test is to encrypt the password, and compare the first 13
		 * characters of it against the stored password.
		 */

			pw = (char *) bigcrypt (enteredpassword, ia_pwdp);
			*(pw+12) = '\0';

			if ((strncmp(pw, ia_pwdp,12)) == 0) {

			/* It looks like the upgrade took place, we have to do a few things.
		 	 * First tell the user whats going on.
		 	 */

				bzero (passwordtobeencrypted,sizeof(passwordtobeencrypted));

				pfmt(stdout, MM_NOSTD|MM_NOGET, gettxt(truncatedwarningid,
					truncatedwarning));

				pfmt(stdout, MM_NOSTD|MM_NOGET, gettxt(repeatpasswordid,
					repeatpassword));

				bzero (passwordstore1,sizeof(passwordstore1));
				strcpy (passwordstore1,enteredpassword);

			/* Get the user to re-type his password, so we can check for typo's
		 	 * in the password beyond the 8 character short password size */

				pfmt(stdout, MM_NOSTD|MM_NOGET, gettxt(prmtid,
					prmt));

		 		enteredpassword = get_passwd();
		 		bzero (passwordstore2,sizeof(passwordstore2));
		 		strcpy (passwordstore2,enteredpassword);

				if (strcmp(passwordstore1,passwordstore2) != 0) {
			
				/* The user put a typo in on either the first or second request
			   	 * for a password, go back to him again for another confirmation
			  	 */

					pfmt(stderr, MM_ERROR|MM_NOGET, gettxt(notmatchedid,
						notmatched));
					pfmt(stdout, MM_NOSTD|MM_NOGET, gettxt(prmtid,
						prmt));
					enteredpassword = get_passwd();

					if ((strcmp (passwordstore1,enteredpassword)) == 0 )
					{
						/* Got passwordstore1 typed the same twice,
					 	 * assume it is right.
					 	 */

						strcpy (passwordtobeencrypted,passwordstore1);
                	}
					else if ((strcmp (passwordstore2,enteredpassword)) == 0 )
					{
						strcpy (passwordtobeencrypted,passwordstore2);
					}
					else
						{
							/* Too much messing around !!! Kick the user,
							 * he can have another go.
							 */
							(void) pfmt(stderr,MM_ERROR,
								":4:Login incorrect\n");
							continue;

						}
            	} strcpy (passwordtobeencrypted,passwordstore1);

				/* O.K a verified password, store it in /etc/ia/master */

				/* First build a new encrypted password */

				pw = bigcrypt(passwordtobeencrypted, ia_pwdp);

				/* Store the new password in /etc/security/ia/master. */

				(void) strcpy(indxp->name, root);

				if (getiasz(indxp) !=0) {
					(void) pfmt(stderr, MM_ERROR, MSG_UF);
                    adumprec(ADT_PASSWD, UFAIL, (uid_t) 0);
					continue;
				}

				mastp = (struct master *) malloc(indxp->length);

				if (mastp == NULL) {
					(void) pfmt(stderr, MM_ERROR, MSG_UF);
					adumprec(ADT_PASSWD, UFAIL, (uid_t) 0);
					continue;
				}

				if (getianam(indxp, mastp) !=0) {
					(void) pfmt(stderr, MM_ERROR, MSG_UF);
					adumprec(ADT_PASSWD, UFAIL, (uid_t) 0);
					continue;
            	}

				strcpy (mastp->ia_pwdp, pw);

				if (putiaent(root, mastp) !=0) {
					(void) pfmt(stderr, MM_ERROR, MSG_FE);
					continue;
				}

				return 0;

			}
			else
				{
					(void) pfmt(stderr,MM_ERROR,
						":4:Login incorrect\n");
					continue;
				}
		}else 
			{

				if (ia_pwdp[0])
					pw = bigcrypt(enteredpassword, ia_pwdp);
				else
					pw = enteredpassword;
	

				if (strcmp(pw, ia_pwdp)) {

					(void) pfmt(stderr,MM_ERROR,
						":4:Login incorrect\n");
					continue;
				}
			}
	

		/*
		 * Password from master file and password read from
		 * stdin were the same, so we have a correct login.
		 *
		 * If login shell from master file doesn't look good,
		 * don't bother invoking su since it will probably fail.
		 * (An empty string is OK here since that will default to
		 * /sbin/sh in su anyway, but a NULL pointer isn't good.)
		 */
		if (ia_shellp == NULL ||
		    ia_shellp[0] && access(ia_shellp, EX_OK) != 0) {
			(void) pfmt(stdout,MM_WARNING,
				":9:No shell; will try %s\n", SHELL);
			single(0);
		}

		/*
		 * All is well, so invoke su.
		 */
		single(1);
	}
	/* NOTREACHED */
}


/*
 * Procedure:     single
 *
 * Restrictions:
 *		getutxent:	P_MACREAD accesses the /var/adm/*tmp* files
 *		pututxline:	P_MACREAD accesses the /var/adm/*tmp* files
 *		printf:		None
 *		execl(2):	P_MACREAD access to the shell
 *
 * Notes:	exec shell for single user mode. 
*/
static	void
single(ok)
	int	ok;
{
	/*
	 * update the utmpx file.
	 */
	ttyn = findttyname(0);

	if (*ttyn) {
		consalloc();
	}
	else {
		ttyn = "/dev/???";
	}

	(void) procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0);

	while ((u = getutxent()) != NULL) {
		if (!strcmp(u->ut_line, (ttyn + sizeof("/dev/") - 1))) {
			(void) time(&u->ut_tv.tv_sec);
			u->ut_type = INIT_PROCESS;
			/*
			 * force user to "look" like
			 * "root" in utmp entry.
			*/
			if (strcmp(u->ut_user, "root")) {
				u->ut_pid = getpid();
				SCPYN(u->ut_user, (char *) "root");
			}
			break;
		}
	}
	if (u != NULL) {
		(void) pututxline(u);
	}

	endutxent();		/* Close utmp file */

	(void) pfmt(stdout,MM_NOSTD,
		":5:Entering Single User Mode\n\n");

	if (ok) 
		(void) execl(shell, shell, minus, (char *)0);
	else
		(void) execl(SHELL, SHELL, minus, (char *)0);

	exit(0);
	/* NOTREACHED */
}



/*
 * Procedure:     get_passwd
 *
 * Restrictions:
 *		fopen:		None
 *		setbuf:		None
 *		ioctl(2):	None
 *		fprintf:	None
 *		fclose:		None
 *
 * Notes: 	hacked from the stdio library version so we can
 *		distinguish newline and EOF.  Also don't need this
 *		routine to give a prompt.
 *
 * RETURN:	(char *)address of password string (could be null string)
 *
 *			or
 *
 *		(char *)0 if user typed EOF
*/
static char *
get_passwd()
{
	struct	termio	ttyb;
	static	char	pbuf[PASS_MAX];
	int		flags;
	register char	*p;
	register c;
	FILE	*fi;
	void	(*sig)();
	char	*rval;		/* function return value */

	fi = stdin;
	setbuf(fi, (char *)NULL);

	sig = signal(SIGINT, SIG_IGN);
	(void) ioctl(fileno(fi), TCGETA, &ttyb);
	flags = ttyb.c_lflag;
	ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

	(void) ioctl(fileno(fi), TCSETAF, &ttyb);
	p = pbuf;
	rval = pbuf;
	while ((c = getc(fi)) != '\n') {
		if (c == EOF) {
			if (p == pbuf)		/* ^D, No password */
				rval = (char *)0;
			break;
		}
		if (p < &pbuf[PASS_MAX])
			*p++ = (char) c;
	}
	*p = '\0';			/* terminate password string */
	(void) fprintf(stderr, "\n");		/* echo a newline */

	ttyb.c_lflag = (long) flags;

	(void) ioctl(fileno(fi), TCSETAW, &ttyb);
	(void) signal(SIGINT, sig);

	if (fi != stdin)
		(void) fclose(fi);

	return rval;
}


/*
 * Procedure:     findttyname
 *
 * Restrictions:
 *		ttyname:	P_MACREAD
 *		access(2):	P_MACREAD
*/
static	char *
findttyname(fd)
	int	fd;
{
	static	char	*l_ttyn;

	(void) procprivl(CLRPRV, pm_work(P_MACREAD), (priv_t)0);

	l_ttyn = ttyname(fd);

/* do not use syscon or systty if console is present, assuming they are links */
	if (((strcmp(l_ttyn, "/dev/syscon") == 0) ||
		(strcmp(l_ttyn, "/dev/systty") == 0)) &&
		(access("/dev/console", F_OK)))
			l_ttyn = "/dev/console";

	(void) procprivl(SETPRV, pm_work(P_MACREAD), (priv_t)0);

	return l_ttyn;
}


/*
 * Procedure:     consalloc
 *
 * Restrictions:
 *		lvlproc(2):	None
 *		lvlin:		None
 *		devstat:	None
 *		printf:		None
*/
static	void
consalloc()
{
	struct	devstat	mybuf;
	level_t		lid_low,
			lid_user,
			lid_high;
	char		*ttynamep;

	errno = 0;
	ttynamep = ttyname(0);

	if ((lvlproc(MAC_GET, &lid_user) == -1) && errno == ENOPKG)
		return; 

	if (lvlin(SYS_RANGE_MAX, &lid_high) == -1) {
		(void) pfmt(stderr,MM_ERROR,
			":6:LTDB is inacessible or corrupt\n");
		(void) pfmt(stderr,MM_ERROR,
			":7:failed to allocate %s\n", ttynamep);
		return;
	}
	if (lvlin(SYS_PUBLIC, &lid_low) == -1) {
		(void) pfmt(stderr,MM_ERROR,
			":6:LTDB is inacessible or corrupt\n");
		(void) pfmt(stderr,MM_ERROR,
			":7:failed to allocate %s\n", ttynamep);
		return;
	}
	mybuf.dev_relflag = DEV_PERSISTENT;
	mybuf.dev_mode = DEV_STATIC;
	mybuf.dev_hilevel = lid_high;
	mybuf.dev_lolevel = lid_low;
	mybuf.dev_state = DEV_PUBLIC;

	if (fdevstat(0, DEV_SET, &mybuf)){
		(void) pfmt(stderr,MM_ERROR,
			":8:devstat failed on %s\n", ttynamep); 
		(void) pfmt(stderr,MM_ERROR,
			":7:failed to allocate %s\n", ttynamep);
	}
	return;
}


/*
 * Procedure:   adumprec
 *
 * Notes:   Only writes the record if auditing is turned on.
 *      It determines this by checking the "auditon" flag
 *      in the audit structure.
 */
static void
adumprec(rtype, status, puid)
    int rtype;          /* event type */
    int status;         /* event status */
    uid_t puid;         /* uid of user's passwd to be changed */
{
    arec_t      rec;        /* auditdmp(2) structure */
    actl_t      actl;       /* auditctl(2) structure */
    apasrec_t   apasrec;    /* passwd record structure */

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
            if (getuid() == MAXUID)
                (void) setuid(puid);
                apasrec.nuid = puid;
        }
        rec.argp = (char *)&apasrec;
        auditdmp(&rec, sizeof(arec_t));
    }
    return;
}

/*		copyright	"%c%" 	*/

#ident	"@(#)newgrp:newgrp.c	1.10.5.1"
#ident "$Header$"

/************************************************************
 * newgrp [-l] [group]
 * newgrp [-] [group]
 *
 * Inheritable Privileges: none
 *       Fixed Privileges: P_SETUID
 *
 * Notes:
 *	if no arg, group id in password file is used
 *	else if group id == id in password file
 *	else if login name is in member list
 *	else if password is present and user knows it
 *	else exec new shell with original uid & gid
 *
 *      Eventhough the passwd prompt is here, the group passwd
 *      prompting is currently NOT supported.  The field for 
 *      the group passwd in /etc/group is empty.      
 ************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <crypt.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/secsys.h>
#include <priv.h>

#define ELIM	128
#define	SHELL	"/sbin/sh"

#define PATH		"PATH=:/usr/bin:/bin"
#define ALT_PATH	"PATH=:/usr/bin:/sbin:/usr/sbin:/etc:/bin"

const char	PW[] = "Password:";
const char	PWID[] = ":352";
const char	NG[] = ":353:Sorry\n";
const char	PD[] = ":354:Permission denied\n";
const char	UG[] = ":10:Unknown group: %s\n";
const char	NS[] = ":355:No shell\n";

char homedir[64]="HOME=";
char logname[20]="LOGNAME=";

char *envinit[ELIM];
char *path=PATH;
char *alt_path=ALT_PATH;
extern char **environ;

uid_t privid;		/* privileged ID if there is one */

/************************************************************
 * Procedure: main()
 ************************************************************/
main(argc,argv)
int argc;
char *argv[];
{
	register struct passwd *p;
	gid_t chkgrp();
	int eflag = 0;
	char *shell, *dir, *name;

	(void) procprivc(CLRPRV, ALLPRIVS_W, (priv_t)0);

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:newgrp");

#ifdef	DEBUG
	chroot(".");
#endif
	if ((p = getpwuid(getuid())) == NULL) 
		error(NG);
	endpwent();

	privid = (uid_t)secsys(ES_PRVID, 0);
	
	if(argc > 1 && ((strcmp(argv[1], "-") == 0) ||
			(strcmp(argv[1], "-l") == 0))) {
		eflag++;
		argv++;
		--argc;
	}
	if (argc > 1 && strcmp(argv[1], "--") == 0)
		argc--, argv++;

	if (argc > 1)
		p->pw_gid = chkgrp(argv[1], p);

	dir = strcpy((char *)malloc(strlen(p->pw_dir)+1),p->pw_dir);
	name = strcpy((char *)malloc(strlen(p->pw_name)+1),p->pw_name);

	(void) procprivc(SETPRV, SETUID_W, (priv_t)0);

	if (setgid(p->pw_gid) < 0 || setuid(p->pw_uid) < 0) {
		(void) procprivc(CLRPRV, ALLPRIVS_W, (priv_t)0);
		error(NG);
	}
	/*
	 * The following check clears the privileges in the maximum
	 * set for the case when the user is not the privileged ID
	 * or the privilege mechanism does not support a privileged ID.
	 */
	if (p->pw_uid != privid)
		(void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);

	if (!*p->pw_shell) {
		if ((shell = getenv("SHELL")) != NULL) {
			p->pw_shell = shell;
		} else {
			p->pw_shell = SHELL;
		}
	}
	if(eflag){
		char *simple;
		
		strcat(homedir, dir);
		strcat(logname, name);
		envinit[2] = logname;
		chdir(dir);
		envinit[0] = homedir;

		/* if privileged ID system, set PATH to alt_path */
		envinit[1] = ((p->pw_uid == privid) ? alt_path : path);

		envinit[3] = NULL;
		environ = envinit;
		shell = strcpy((char *)malloc(strlen(p->pw_shell) + 2), "-");
		shell = strcat(shell,p->pw_shell);
		if(simple = strrchr(shell,'/')){
			*(shell+1) = '\0';
			shell = strcat(shell,++simple);
		}
	}
	else
		shell = p->pw_shell;

	(void) execl(p->pw_shell, shell, NULL);
	error(NS);
	/* NOTREACHED */
}

warn(s, a1)
char *s, *a1;
{
	pfmt(stderr, MM_ERROR, s, a1);
}

error(s)
char *s;
{
	warn(s);
	exit(1);
}

/************************************************************
 * Procedure: chkgrp()
 *
 * Restrictions: none
 *
 * Notes: checks to see if group requested is valid & 
 *		        if the user is in requested group
 *	  if yes, return group id
 *	  if no,  print warning message & 
 *                return current group id 
 ************************************************************/
gid_t
chkgrp(gname, p)
char	*gname;
struct	passwd *p;
{
	register char **t;
	register struct group *g;

	g = getgrnam(gname);
	endgrent();
	if (g == NULL) {
		warn(UG, gname);
		return getgid();
	}
	if (p->pw_gid == g->gr_gid)
		return g->gr_gid;

	/* if this is a privileged ID base system and the user id is
	   equal to the privileged ID, return the specified group ID */

	if (privid >= 0 && p->pw_uid == privid)
		return g->gr_gid;

	for (t = g->gr_mem; *t; ++t) {
		if (strcmp(p->pw_name, *t) == 0)
			return g->gr_gid;
	}

	/* This group passwd prompting is here for flexibility but
	   currently the system does not support group passwds    */

	if (*g->gr_passwd) {
		if (!isatty(fileno(stdin)))
			error(PD);
		if (strcmp(g->gr_passwd, crypt(getpass(gettxt(PWID, PW)),
						g->gr_passwd)) == 0)
			return g->gr_gid;
	}
	warn(NG);
	return getgid();
}

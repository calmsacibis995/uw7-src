/*		copyright	"%c%" 	*/

#ident	"@(#)shio.c	1.2"
#ident  "$Header$"

#include "uucp.h"
#include <sys/secsys.h>
#include <priv.h>
#include <pwd.h>
#include <wait.h>

/*
 * use shell to execute command with
 * fi, fo, and fe as standard input/output/error
 *	cmd 	-> command to execute
 *	fi 	-> standard input
 *	fo 	-> standard output
 *	fe 	-> standard error
 * return:
 *	0		-> success 
 *	non zero	-> failure  -  status from child
			(Note - -1 means the fork failed)
 */
int
shio(cmd, fi, fo, fe, logname)
char *logname;
char *cmd, *fi, *fo, *fe;
{
	register pid_t pid, ret;
	int status;

	if (fi == NULL)
		fi = "/dev/null";
	if (fo == NULL)
		fo = "/dev/null";
	if (fe == NULL)
		fe = "/dev/null";

	DEBUG(3, "shio - %s\n", cmd);
	if ((pid = fork()) == 0) {
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
		closelog();
		(void) close(Ifn);	/* close connection fd's */
		(void) close(Ofn);
		(void) close(0);	/* get stdin from file fi */
		if (open(fi, O_RDONLY) != 0)
			exit(errno);
		(void) close(1);	/* divert stdout to fo */
		if (creat(fo, PUB_FILEMODE) != 1)
			exit(errno);
		(void) close(2);	/* divert stderr to fe */
		if (creat(fe, PUB_FILEMODE) != 2)
			exit(errno);
		if (logname) {
			/* we need privilege to set_id()	*/
			/* P_MACREAD to read the ia databse	*/
			/* P_SETUID to change to the right id	*/
			seteuid(Euid);
			(void)procprivl(SETPRV, MACREAD_W, (priv_t)0);
			if (uu_set_id(logname) != 0)
				exit(101);
		} else {
			seteuid(Euid);	/* set back to original */
			setgid(UUCPGID);
			setuid(UUCPUID);/* give up "root-ness" */
		}

		/* give up the only two privileges we should have had */
		(void) procprivl(CLRPRV, P_MACREAD|P_SETUID, (priv_t)0);

		(void) execle(SHELL, "sh", "-c", cmd, (char *) 0, Env);
		exit(100);
	}

	/*
	 * the status returned from wait can never be -1
	 * see man page wait(2)
	 * So we use the -1 value to indicate fork failed
	 * or the wait failed.
	 */
	if (pid == -1)
		return(-1);
	
	while ((ret = wait(&status)) != pid)
	    if (ret == -1 && errno != EINTR)
		return(-1);
	DEBUG(3, "status %d\n", status);
	return(status);
}

#undef MASTER	/* this is also in <ia.h> */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<grp.h>
#include	<sys/vnode.h>
#include	<audit.h>
#include	<ia.h>
#include	<iaf.h>
#include	<mac.h>


int
uu_set_id(namep)
char *namep;
{

	int	i = 0;
	long	lvlcnt = 0;
	uid_t	uid;
	gid_t	gid;
	gid_t	gidcnt;
	gid_t	*groups;
	level_t level;
	level_t *ia_lvlp;
	aevt_t	aevt;
	actl_t	actl;
	uinfo_t	uinfo;
	
	if (!namep)
		return(0);

	/*
	 * if secsys() returns -1, we're not running ES, so we'll
	 * use the system set_id() call
	 */

	if (secsys(ES_PRVSETCNT, (char *) NULL) < 0)
		return(set_id(namep));

	if ( (ia_openinfo(namep, &uinfo)) || (uinfo == NULL) )
		return(1);
	ia_get_uid(uinfo, &uid);
	ia_get_gid(uinfo, &gid);
	ia_get_sgid(uinfo, &groups, &gidcnt);
	if (lvlproc(MAC_GET, &level) == 0) {
		if (ia_get_lvl(uinfo, &ia_lvlp, &lvlcnt)) {
			ia_closeinfo(uinfo);
			return(1);
		}
		for (i=0; i<lvlcnt; ia_lvlp++) {
			if (level == *ia_lvlp) 
				break;
		}
		if (i == lvlcnt) {	/* level not in user's list */
			ia_closeinfo(uinfo);
			return(1);
		}
	}

	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) < 0) {
		if (errno != ENOPKG)
			return(1);
	} else {
		ia_get_mask(uinfo,aevt.emask);
		aevt.uid = uid;
		if (auditevt(AGETUSR, &aevt, sizeof(aevt_t))) {
			if (errno != ESRCH) 
				return(1);
		}
		if (auditevt(ASETME, &aevt, sizeof(aevt_t))) 
			return(1);
	}

	/*
	 * close the master file because it's no longer needed
	*/
	ia_closeinfo(uinfo);

	if( setgid(gid) == -1 )
		return(1);

	/* Initialize the supplementary group access list. */
	if (setgroups(gidcnt, groups)) 
		return(1);

	if( setuid(uid) == -1 )
		return(1);

	return(0);
}

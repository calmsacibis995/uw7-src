#ident	"%W"
/******************************************************************************
 *      execScript.c
 *-----------------------------------------------------------------------------
 * Comments:
 * POSIX conforming execScript() routine for Systems Management utilities
 *
 *-----------------------------------------------------------------------------
 *      @(#) getxopt.c 0
 *
 *-----------------------------------------------------------------------------
 * Revision History:
 *
 *      27 Mar 1997     Carol Small
 *			Original version handed off.
 *
 *============================================================================*/

#include	<grp.h>
#include	<pwd.h>
#include	<limits.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<unistd.h>
#include        <sys/errno.h>
#include	"messages.h"

#define	MaxEnvSize		(_POSIX_ARG_MAX*_POSIX_NAME_MAX)

static	char	newEnv[MaxEnvSize];
static	char	*envp[_POSIX_ARG_MAX];			/* the environment being built		*/

static	int	putEnv(int index, char *name, char *value) {
	static	char *env = newEnv;
	int	sz = strlen(name)+strlen(value)+2;

	if (value) {
		if ((env+sz) < (newEnv+MaxEnvSize)) {
			envp[index] = env;
			sprintf(env,"%s=%s",name,value);
			env += sz;
			return(1);
		}
		else {
			errmsg(M_INTERNAL_ERROR);
			exit(1);
		}
	}
	else	return(0);
}

static	void	initialiseEnv() {
	char	string[_POSIX_NAME_MAX];
	struct	passwd	*pw = getpwuid(geteuid());
	struct	group	*gp = getgrgid(getegid());
	int	envInd;

	envInd = 0;
	sprintf(string,"%d",pw->pw_uid);
	envInd += putEnv(envInd,"UG_PW_UID",string);
	sprintf(string,"%d",pw->pw_gid);
	envInd += putEnv(envInd,"UG_PW_GID",string);
	envInd += putEnv(envInd,"UG_NAME",pw->pw_name);
	envInd += putEnv(envInd,"UG_PW_GRNAME",gp->gr_name);
	envInd += putEnv(envInd,"UG_PW_DIR",pw->pw_dir);
	envInd += putEnv(envInd,"UG_HOME_DIR",getenv("HOME"));
	envInd += putEnv(envInd,"UG_RETIRE","0");
	envInd += putEnv(envInd,"UG_OPTIONS",getenv("UG_OPTIONS"));
	envInd += putEnv(envInd,"UG_UTIL_OPTS",getenv("UG_UTIL_OPTS"));
	envp[envInd] = (char*) 0;
}

int	execScript(char *path, char *file) {
	int	pid, status;
	extern	int errno;
	char	script[_POSIX_PATH_MAX];		/* name of script to be executed	*/

	if (_POSIX_PATH_MAX < (strlen(path)+strlen(file)+1)) {
		errmsg(M_PATH_TOO_LONG);
		exit(1);
	}
	else	sprintf(script,"%s/%s",path,file);

	switch (pid = fork()) {
	case 0:
	/* the child branch	*/
		initialiseEnv();			/* set up a new environment	*/
		close(0);				/* close standard input ...	*/
		execle(script,file,(char*) 0,envp); 	/* exec designated script ...	*/
		errmsg(M_NO_EXEC,strerror(errno));
		exit(1);
	case -1:
	/* the fork failed	*/
		errmsg(M_NO_FORK,strerror(errno));
		exit(1);
	default:
	/* the parent branch	*/
		while (wait(&status) != pid);
		return(WEXITSTATUS(status));
	}
}

#ident  "@(#)homedir.c	1.5"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <userdefs.h>
#include <errno.h>
#include <mac.h>
#include <sys/stat.h>
#include <priv.h>
#include "messages.h"

extern	int	execl(),
		chown(),
		mkdir(),
		unlink(),
		setgid(),
		lvlfile(),
		rm_files();

extern	size_t	strlen();

extern	pid_t	fork(),
		wait();

extern	char	*strchr(),
		*strrchr(),
		*dirname(),
		*strerror();

extern	void	exit(),
		errmsg();

static	int	_mk_skel();

/*
 * Procedure:	create_home
 *
 * Restrictions:
 *		mkdir(2):	none
 *		chown(2):	none
 *		unlink(2):	none
 *		lvlfile(2):	none
 *
 * Notes:	Create a home directory and populate with files from skeleton
 *		directory.
*/
int
create_home(hdir, skeldir, logname, existed, uid, gid, def_lvl,permissions)
	char	*hdir,			/* home directory to create */
		*skeldir,		/* skel directory to copy if indicated */
		*logname;		/* login name of user */
	int	existed;		/* determine if directory existed */
	uid_t	uid;			/* uid of user */
	gid_t	gid;			/* group id of user */
	level_t	def_lvl;		/* level for user's files */
	int permissions;		/* permissions for home dir */
{
	if (*skeldir) {
		return _mk_skel(hdir, skeldir, logname, existed, gid, def_lvl,permissions);
	}
	else {
		if (!existed) {
			if (mkdirp(hdir, permissions, permissions) != 0) {
				errmsg (M_HOME_DIR, strerror(errno));
				return EX_HOMEDIR;
			}
		}

		(void) lvlfile(hdir, MAC_SET, &def_lvl);

		if (chown(hdir, uid, gid) != 0) {
			errmsg (M_OWNERSHIP, strerror(errno));
			(void) unlink(hdir);
			return EX_HOMEDIR;
		}
	}
	return EX_SUCCESS;
}


#define	f_c	"/usr/bin/find . ! -name '.' -print | /usr/bin/cpio -pdum '%s' > /dev/null 2>&1"
#define	chl	"/usr/bin/find '%s' -print | /usr/bin/xargs $TFADMIN /sbin/chlvl '%s' > /dev/null 2>&1"

/*
 * Procedure:	_mk_skel
 *
 * Restrictions:
 *		lvlout:		none
 *		setgid(2):	none
 *		stat(2):	none
 *		chdir(2):	none
 *		mkdir(2):	none
 *		lvlfile(2):	none
 *		setgid():	none
 *		execl():	P_ALLPRIVS
 *
 * Notes:	This routine forks and execs a command with the specified
 *		option to copy ALL files in the named directory to the new
 *		home directory.
 *
 *		Also forks and execs the "/usr/bin/chown" command with
 *		the "-R" option to change the owner of ALL the files
 *		in the new directory to the new user.
 *
 *		Also forks and execs the "/usr/bin/chgrp" command with
 *		the "-R" option to change the group of ALL the files
 *		in the new directory to the new user.
 *
*/
static	int
_mk_skel(hdir, skeldir, logname, existed, gid, u_lvl,permissions)
	char	*hdir,			/* real home directory */
		*skeldir,		/* skeleton directory to copy from */
		*logname;		/* login name of user */
	int	existed;		/* determine if directory existed */
	gid_t	gid;			/* the new group ID for all files */
	level_t	u_lvl;			/* level for user's files */
	int permissions;		/* permissions for home dir */
{
	char	*bufp,
		*opt_h = "-h",
		*opt_R = "-R",
		*sh_cmd = "/sbin/sh",
		*cmd = NULL,
		*own_cmd = "/usr/bin/chown",
		*grp_cmd = "/usr/bin/chgrp";

	int	i,
		mac = 0,
		status = 0;
	pid_t	pid;

	char ctp[128];


	if ((i = lvlout(&u_lvl, bufp, 0, LVL_ALIAS)) != -1) {
		mac = 1;
		bufp = (char *)malloc((unsigned int) i);
		if (bufp != NULL) {
			(void) lvlout(&u_lvl, bufp, i, LVL_ALIAS);
		}
	}

	if ((pid = fork()) < 0) {
		errmsg (M_NO_FORK, strerror(errno));
		return EX_HOMEDIR;
	}
	if (pid == (pid_t) 0) {
		/*
		 * in the child
		*/
		if (!existed) {
			/*
	 		 * Fork and exec "/sbin/sh" with the "-c" option and the
	 		 * command string specified in the define "cmd".  This
	 		 * copies all files from "skeldir" to the new "homedir".
			*/
			struct	stat	statbuf;

			if (stat(skeldir, &statbuf) < 0) {
				errmsg (M_NO_SOURCE_DIR, strerror(errno));
				exit(1);
			}
			if (mkdirp(hdir, permissions, permissions) != 0) {
				errmsg (M_HOME_DIR, strerror(errno));
				exit(EX_HOMEDIR);
			}
			(void) lvlfile(hdir, MAC_SET, &u_lvl);
		}
		/*
		 * set up the command string exec'ed by the shell
		 * so it contains the correct directory name where
		 * the files are copied.
		*/
		cmd = (char *)malloc(strlen(f_c) + strlen(hdir) + (size_t)1);
		(void) sprintf(cmd, f_c, hdir);

		/*
		 * change directory to the source directory.  If
		 * the chdir fails, print out a message and exit.
		*/
		if (chdir(skeldir) < 0) {
			errmsg (M_UNABLE_TO_CD, strerror(errno));
			exit(1);
		}
		/*
		 * clear all privileges in the working set.
		*/
		(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
		(void) execl(sh_cmd, sh_cmd, "-c", cmd, (char *)NULL);
		(void) procprivl(SETPRV, ALLPRIVS_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	else {
		(void) wait(&status);
	}
	if (existed) {
		(void) lvlfile(hdir, MAC_SET, &u_lvl);
	}
	if (((status >> 8) & 0377) != 0) {
		errmsg(M_COPY_SKELDIR);
		return EX_HOMEDIR;
	}

	cmd = '\0';
	status = 0;

	/*
	 * if MAC is installed, fork and exec "/usr/bin/find" and pipe
	 * the output to /sbin/chlvl to change the level of the files
	 * just copied to the new user's default level.
	*/
	if (mac) {
		if ((pid = fork()) < 0) {
			errmsg (M_NO_FORK, strerror(errno));
			return EX_HOMEDIR;
		}
		if (pid == (pid_t) 0) {
			/*
			 * in the child
			*/
			cmd = (char *) malloc(strlen(chl) + strlen(bufp) +
				strlen(hdir) + (size_t) 1);
			(void) sprintf(cmd, chl, hdir, bufp);
			(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
			(void) execl(sh_cmd, sh_cmd, "-c", cmd, (char *)NULL);
			(void) procprivl(SETPRV, ALLPRIVS_W, 0);
			exit(1);
		}

		/*
		 * the parent sits quietly and waits for the child to terminate.
		*/
		else {
			(void) wait(&status);
		}
	
		if (((status >> 8) & 0377) != 0) {
			errmsg(M_UNABLE_TO_CHLVL);
			return EX_HOMEDIR;
		}
	 
		cmd = '\0';
		status = 0;
	}

	/*
	 * Now fork and exec "/usr/bin/chown" with the "-R" flag to
	 * change the owner of the files just copied to the new user.
	 * Doing it this way means that "/usr/bin/cp" does not need
	 * P_MACWRITE as a fixed privilege in a system that is running
	 * an ID based privilege mechanism.
	*/
	if ((pid = fork()) < 0) {
		errmsg (M_NO_FORK, strerror(errno));
		return EX_HOMEDIR;
	}
	if (pid == (pid_t) 0) {
		/*
		 * in the child
		*/
		(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
		(void) execl(own_cmd, own_cmd, opt_h, opt_R, logname, hdir,
			(char *)NULL);
		(void) procprivl(SETPRV, ALLPRIVS_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	else {
		(void) wait(&status);
	}

	if (((status >> 8) & 0377) != 0) {
		errmsg(M_CHOWN_NEW,strerror(13));
		(void) unlink(hdir);
		return EX_HOMEDIR;
	}

	/*
	 * Now fork and exec "/usr/bin/chgrp" with the "-R" flag to
	 * change the group of the files just copied to the new user.
	*/
	if ((pid = fork()) < 0) {
		errmsg (M_NO_FORK, strerror(errno));
		return EX_HOMEDIR;
	}
	if (pid == (pid_t) 0) {
		/*
		 * in the child
		*/
		(void) procprivl(CLRPRV, ALLPRIVS_W, 0);
		(int) sprintf(ctp, "%ld", gid);
		(void) execl(grp_cmd, grp_cmd, opt_h, opt_R, ctp, hdir,
			(char *)NULL);
		(void) procprivl(SETPRV, ALLPRIVS_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	else {
		(void) wait(&status);
	}

	if (((status >> 8) & 0377) != 0) {
		errmsg(M_CHGRP_NEW,strerror(13));
		(void) unlink(hdir);
		return EX_HOMEDIR;
	}

	if (chmod(hdir, permissions) < 0 ) {
		errmsg(M_CHMOD_NEW,strerror(13));
		(void) unlink(hdir);
		return EX_HOMEDIR;
	}
	return EX_SUCCESS;
}


static char *compress();
void free();

/* 
 * Procedure: compress
 */

static char *
compress(str)
char *str;
{

	char *tmp;
	char *front;

	tmp=(char *)malloc(strlen(str)+1);
	if ( tmp == NULL )
		return(NULL);
	front = tmp;
	while ( *str != '\0' ) {
		if ( *str == '/' ) {
			*tmp++ = *str++;
			while ( *str == '/' )
				str++;
		}
		*tmp++ = *str++;
	}
	*tmp = '\0';
	return(front);
} /* compress() */


/*
 * Procedure: mkdirp - creates an directory and it's parents if the parents
 *		       do not exist yet.
 *
 * Restrictions: mkdir(2): <none> access(2): <none>
 *
 * Notes: Returns -1 if fails for reasons other than non-existing parents.
 * 	  Does NOT compress pathnames with . or .. in them.
 */

int
mkdirp(d, parents_mode, mode)
const char *d;		/* directory pathname */
mode_t parents_mode ;	/* parents directory mode */
mode_t mode;		/* target directory mode */
{
	char  *endptr, *ptr, *slash, *str, *lastcomp;

	/*
	 * Remove extra slashes from pathname.
	 */

	str=compress(d);

	/* If space couldn't be allocated for the compressed names, return. */

	if ( str == NULL )
		return(-1);

	ptr = str;

        /* Try to make the directory */
	if (mkdir(str, mode) == 0){
		free(str);
		return(0);
	}

	if (errno != ENOENT) {
		free(str);
		return(-1);
	}
	endptr = strrchr(str, '\0');
	ptr = endptr;
	slash = strrchr(str, '/');
	lastcomp = slash;

		/* Search upward for the non-existing parent */
	while (slash != NULL) {

		ptr = slash;
		*ptr = '\0';

			/* If reached an existing parent, break */

		if (access(str, 00) ==0)
			break;

			/* If non-existing parent*/

		else {
			slash = strrchr(str,'/');

			/* If under / or current directory, make it. */

			if (slash  == NULL || slash== str) {
				if (mkdir(str, parents_mode)) {
					free(str);
					return(-1);
				}
				break;
			}
		}
	}
	/* Create directories starting from upmost non-existing parent*/

	while ((ptr = strchr(str, '\0')) != lastcomp){
		*ptr = '/';
		if (mkdir(str, parents_mode)) {
			free(str);
			return(-1);
		}
	}
	*lastcomp = '/';

	if (mkdir(str, mode)) {
		free(str);
		return -1;
	}

	free(str);
	errno = 0;
	return 0;
} /* mkdirp() */

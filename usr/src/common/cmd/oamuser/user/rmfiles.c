#ident  "@(#)rmfiles.c	1.3"
#ident  "$Header$"

/*
 * Procedure:	rm_files
 *
 * Restrictions:
 *		execl():	P_MACREAD
 *
 * Notes:	This routine forks and execs the "/usr/bin/rm" command
 *		with the "-rf" option to remove ALL files in the named
 *		directory.
*/

#include <sys/types.h>
#include <stdio.h>
#include <userdefs.h>
#include <errno.h>
#include <priv.h>
#include <pfmt.h>
#include "messages.h"

#define 	SBUFSZ	256

extern void errmsg();
extern int rmdir();
extern	long	wait();

int
rm_files(homedir)
	char *homedir;			/* home directory to remove */
{
	char	*cmd = "/usr/bin/rm",
		*options = "-rf";

	int	status = 0;
	pid_t	pid;

	/* delete all files belonging to owner */

	if ((pid = fork()) == 0) {
		/*
		 * in the child
		*/
		(void) procprivl(CLRPRV, MACREAD_W, 0);
		(void) execl(cmd, cmd, options, homedir, (char *)NULL);
		(void) procprivl(SETPRV, MACREAD_W, 0);
		exit(1);
	}

	/*
	 * the parent sits quietly and waits for the child to terminate.
	*/
	(void) wait(&status);

	if (((status >> 8) & 0377) != 0) {
		errmsg(M_RMFILES);
		return EX_HOMEDIR;
	}
	return EX_SUCCESS;
}


/*
 *	Check, in case the original home directroy is one of the 
 *	system directories.
 */
int
chk_sysdir(homedir)
	char *homedir;			/* home directory to remove */
{
	if (strcmp(homedir, "/") != 0 
	&& strcmp(homedir, "/etc") != 0 
	&& strcmp(homedir, "/home") != 0 
	&& strcmp(homedir, "/sbin") != 0 
	&& strcmp(homedir, "/usr") != 0 
	&& strcmp(homedir, "/usr/bin") != 0
	&& strcmp(homedir, "/usr/sbin") != 0
	&& strcmp(homedir, "/var") != 0)
	/*
	 * make sure the system critical directories (the above
	 * ones) don't get removed.
	 */
		return 1;
	else
		return 0;
}


/*
 *	Check to see if the new home diretory is a sub-dirctory of the
 *	old one.
 */
int
chk_subdir(old, new)
	char *old;			/* the old home directory  */
	char *new;			/* the new home directory  */
{
	if (strncmp( old, new, strlen(old) ))
	/*
	 * make sure the new directory is not a sub-directory
	 * of old one.
	 */
		return 1;
	else {
		errmsg (M_NEW_HOME);
		return 0;
	}
}

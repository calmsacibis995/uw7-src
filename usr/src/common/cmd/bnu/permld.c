/*		copyright	"%c%" 	*/

#ident	"@(#)permld.c	1.2"
#ident  "$Header$"

/***************************************************************************
 * Command: permld <arg>
 *
 * Inheritable Privileges:
 *	Required: P_MACREAD,P_SETPLEVEL,P_SYSOPS
 *	Optional: allprivs are passed to the specified command
 *
 * Fixed Privileges:
 *	None
 *
 * Notes:
 *	This command will execute the /sbin/sh once for each of
 *	the levels found in the directory specified by <arg>.
 *	Stdin will be the current stdin, i.e. the current shell.
 *	The process level will be set before the execution.
 *
 *	The program will execute at least once, at the current level.
 *
 *	If the -x option is specified, the command to be executed
 *	will be executed as an executable.
 *
 *	For things which care, the order of execution will be the
 *	order in the entries appear in the specified directory.
 ***************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <priv.h>
#include <mac.h>
#include <string.h>

#include "uucp.h"

struct lid_list {
	level_t	lid;
	struct lid_list *next;
};

static struct lid_list
	*list_head = NULL,
	*list_tail = NULL;

static int list_count = 0;

/*
 *	This function gets all the levels under an associated
 *	(multi-level) directory. We already know that MAC is
 *	installed, otherwise we wouldn't be here.
 */

static void
getlevels(dirname)
char *dirname;
{
	struct	stat	buf;
	DIR *mlddir;
	char f[BUFSIZ];
	char g[BUFSIZ];

	/* turn on real mode to test for MLDs */

	if (mldmode(MLD_REAL) == -1)
		return;

	/* from here on, remember to return in MLD_VIRT mode */

	if ( (stat(dirname, &buf) == 0)
		&& (S_ISMLD & buf.st_flags)
		&& ((mlddir = opendir(dirname)) != NULL) ) {

		while (gdirf(mlddir, f, dirname) == TRUE) {

			level_t dir_lid;
			struct lid_list *ptr;
	
			(void) sprintf(g, "%s/%s", dirname, f);
			if (lvlfile(g, MAC_GET, &dir_lid) == -1)
				continue;

			for (ptr = list_head; ptr != NULL; ptr = ptr->next)
				if (ptr->lid == dir_lid)
					break;

			if (ptr == NULL) {	/* lid not found */
				ptr = malloc((size_t) sizeof(struct lid_list));
				if (ptr == NULL)
					break;

				list_count++;
				ptr->lid = dir_lid;
				ptr->next = NULL;
				if (list_tail)
					list_tail->next = ptr;
				else
					list_head = ptr;
				list_tail = ptr;
			}

		}	/* end of while(gdirf) */

	}	/* end of if() */

	/* return in MLD_VIRT mode only */

	if (mldmode(MLD_VIRT) == -1)
		exit(9);

	return;
}

/*
 *	This function executes the appropriate command once
 *	for each of the levels in the list, or once at the
 *	current level if the list is empty.
 */

static void
execlevels(cmd, args)
char *cmd;
char ** args;
{
	id_t pid;
	siginfo_t status;
	struct lid_list *tmp;

	if (list_count == 0) {
		execvp(cmd, args);
		exit(10);
	}

	for (tmp = list_head; tmp != NULL; tmp = tmp->next) {

		switch( pid = (id_t) fork() ) {

		case -1:	/* fork() failed */
			exit(11);
			break;

		case 0:		/* child process */

			if (lvlproc(MAC_SET, &(tmp->lid)) == -1)
				exit(12);
	
			(void) execvp(cmd, args);

			exit(13);	/* exec must have failed */

			break;

		default:	/* parent process */
			waitid((idtype_t) P_PID, pid, &status, WEXITED);
			break;

		}

	}	/* end of for() */

	return;
}

/*
 *	main program for permld
 */

main(argc,argv)
int argc;
char **argv;
{
	level_t mylid;

	char *dirname;
	char **args;
	char *cmd;
	char *shell = "/sbin/sh";	/* only /sbin/sh can pass privileges */

	int argind = 0;
	int ret;
	int executable = 0;
	int usage = 0;

	while ((ret = getopt(argc, argv, "x")) != EOF) {
		switch(ret) {
		case 'x':
			executable++;
			break;
		default:
			usage = 1;
			break;
		}
	}
 
	if (optind == argc) {
		usage = 1;
	}
	
	if (usage) {	
		fprintf(stderr,
			"Usage: permld [-x] directory [cmd] [options]\n");
		exit(1);
	}

	if ((args = (char **) calloc((size_t) argc, sizeof(char *))) == NULL)
		exit(2);

	dirname = argv[optind++];	/* skip over directory name */

	if (optind == argc)
		args[0] = shell;	/* if none provided, use sh */
	else {
		/* set up argv[0], et. al. for next program */
		args[argind++] = argv[optind];
		while (argv[optind])
			args[argind++] = argv[optind++];
	}

	if (!executable)		/* if not executable, use sh */
		cmd = shell;
	else
		cmd = args[0];

	if (lvlproc(MAC_GET, &mylid) == -1) {
		if (errno != ENOPKG)
			exit(3);
	} else {
		getlevels(dirname);
	}

	execlevels(cmd, args);

	exit(0);
}

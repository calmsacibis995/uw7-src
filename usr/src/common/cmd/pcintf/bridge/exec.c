#ident	"@(#)pcintf:bridge/exec.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)exec.c	6.3	LCC);	/* Modified: 21:01:15 6/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/
/*
 *	MODIFICATION HISTORY
 *	12/16/87 Jeremy Daw SPR # 2212
 *		Removed close_all() call from resetEnv() because what it
 *	did was close all the files that the child inherited from the parent.
 *	Since shared memory is not process sensitive it would delete these
 *	files' entries from the shared memory table as well. This left the
 *	parent, *dossvr*, without access to the files that were open before
 *	the mkdir or rmdir calls that call fork and exec children that call
 *	resetEnv() on completion. TBD, perhaps we should do system mkdir
 *	and rmdir calls rather than forking "/bin/mkdir" and "/bin/rmdir".
 */


#include "sysconfig.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "const.h"
#include "dossvr.h"

LOCAL void	resetEnv	PROTO((void));

/*
 * exec_cmd:	Executes a command and returns 1 if successful and 0
 *		otherwise.
 */

int
exec_cmd(cmd, args)
char *cmd;			/* Pointer to command to be executed */
char *args[];			/* pointer to arguements */
{
    register int
	pid,			/* Process id from wait() */
	child_pid;		/* Process id of child from fork() */

    int
	cstat;			/* Status of process from wait() */

    void (*saved)() = (void(*)())signal(SIG_CHILD, SIG_DFL);
    child_pid = fork();

    if (child_pid == 0) {
	resetEnv();
	if (strcmp(cmd, "rmdir") == 0)
	{
	    if (*args[1] == '/')
		chdir("/");
#ifdef ATT6300PLUS
	    cmd = "/osm/osm_rmdir";
#endif
	}
#ifdef ATT6300PLUS
	else if (strcmp(cmd, "mkdir") == 0)
	    cmd = "/osm/osm_mkdir";
#endif
	execvp(cmd, args);
	exit(255);
	/*NOTREACHED*/
    }

/*
 * Wait for signal from child and return the completion code of program.
 */
    else {
	for (;;) {
	    pid = u_wait(&cstat);

	    if (child_pid == pid) {
		signal(SIG_CHILD, saved);
		return cstat == 0;
	    } else
	        childExit(child_pid, cstat);
	}
    }
}

/*  
    reset environment after fork; close all currently opened Unix 
    file descriptors and open /dev/null for descriptors 0,1,and 2.
*/

void resetEnv()
{
int	i,
	maxfiles,
	dummyDesc;
				/* 12/16/87 JD used to do a close_all() here
				 * but it unecessaryly closed all record locking
				 * entries too. Which included for the parent
				 * which in this case is the dossvr!
				 */
	maxfiles = uMaxDescriptors();
	for (i=0; i < maxfiles; i++)
		close(i);
	do
		dummyDesc = open("/dev/null", O_RDONLY);
	while (dummyDesc == -1 && errno == EINTR);
	do
		dummyDesc = fcntl(dummyDesc, F_DUPFD, 0);
	while (dummyDesc == -1 && errno == EINTR);
	do
		dummyDesc = fcntl(dummyDesc, F_DUPFD, 0);
	while (dummyDesc == -1 && errno == EINTR);
}

/*
 * fork_wait:	Forks a child.  Parent wiaits for child to exit and returns
 *		pid of child or -1 if fork failed.  Returns 0 to child.
 *
 */
int 
fork_wait(status)
int *status;

{
    register int
	pid,			/* Process id from wait() */
	child_pid;		/* Process id of child from fork() */

    int
	cstat;			/* Status of process from wait() */

    void (*saved)() = (void(*)())signal(SIG_CHILD, SIG_DFL);
    child_pid = fork();

    if (child_pid == 0) return 0;

/*
 * Wait for signal from child and return the completion code of program.
 */
    if (child_pid == -1) return -1;
    else {
	for (;;) {
	    pid = u_wait(&cstat);

	    if (child_pid == pid) {
		signal(SIG_CHILD, saved);
		if (status != (int *)NULL) *status = cstat;
		return child_pid;
	    } else
	        childExit(child_pid, cstat);
	}
    }
}

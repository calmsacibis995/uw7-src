/*		copyright	"%c%" 	*/

#ident	"@(#)killall:killall.c	1.21.1.6"
#ident "$Header$"

/***************************************************************************
 * Command: killall
 * Inheritable Privileges: P_COMPAT,P_OWNER
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include <sys/types.h>
#include <sys/procset.h>
#include <signal.h>
#include <errno.h>
#include <priv.h>

char usage[] = "usage: killall [signal]\n";
char perm[] = "permission denied\n";


/*
 * Procedure:     main
 *
 * Restrictions:
 *                sigsendset(2): none
 */

main(argc,argv)
int argc;
char **argv;
{
	int sig;
	procset_t set;


	switch(argc) {
		case 1:
			sig = SIGKILL;
			break;
		case 2:
			if (str2sig(argv[1], &sig) == 0)
				break;
		default:
			write(2,usage,sizeof(usage)-1);
			exit(1);
	}

	setprocset(&set, POP_DIFF, P_ALL, P_MYID, P_SID, P_MYID);

	if ((sigsendset(&set, sig) < 0)  &&  (errno != ESRCH)) {
		write(2,perm,sizeof(perm)-1);
		exit(1);
	}
	exit(0);
}

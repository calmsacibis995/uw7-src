/*		copyright	"%c%" 	*/

#ident	"@(#)nice:nice.c	1.6.1.10"
#ident "$Header$"

/***************************************************************************
 * Command: nice
 * Inheritable Privileges: P_TSHAR
 *       Fixed Privileges: None
 *
 ***************************************************************************/


#include	<stdio.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<priv.h>
#include	<mac.h>
#include	<sys/errno.h>
#include	<locale.h>
#include	<pfmt.h>


/*
 * Procedure:     main
 *
 * Restrictions:
 *                nice(2): none
 *                execvp(2): all privs, if LPM module is installed.
 */

main(argc, argv)
int argc;
char *argv[];
{
	int	nicarg = 10;
	int	nflag = 0;
	int	rc;
	extern	errno;
	extern	char *sys_errlist[];
	level_t level;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:nice");

	argc--;
	argv++;
	if(argc && (strcmp(*argv, "-n") == 0)) {
		register char *p;

		argc--;
		argv++;
		p = argv[0];
		if (*p == '-') {
			p++;
		}
		while(*p) {
			if(!isdigit(*p)) {
				pfmt(stderr, MM_ERROR,
					 ":955:argument must be numeric.\n");
				exit(2);
			}
			p++;
		}
		nicarg = atoi(argv[0]);
		argc--;
		argv++;
		nflag = 1;
	}
	if (!nflag && argc && (argv[0][0] == '-')) {
		register char	*p = argv[0];

		if(*++p != '-') {
			--p;
		}
		while(*++p)
			if(!isdigit(*p)) {
				pfmt(stderr, MM_ERROR,
					 ":955:argument must be numeric.\n");
				exit(2);
			}
		nicarg = atoi(&argv[0][1]);
		argc--;
		argv++;
	}
	if(argc < 1) {
		pfmt(stderr, MM_ERROR,
			":956:usage: nice [-n num | -num] command\n");
		exit(2);
	}

	errno = 0;

	if (nice(nicarg) == -1) {
		/*
		 * Could be an error or a legitimate return value.
		 * The only error we care about is EINVAL, which will
		 * be returned if we are not in the time sharing
		 * scheduling class.  For any error other than EINVAL
		 * we will go ahead and exec the command even though
		 * the priority change failed.
		 */
		if (errno == EINVAL) {
			pfmt(stderr, MM_ERROR, 
			    ":957:invalid operation; not a time sharing process\n");
			exit(2);
		}
	}
	
	/*
	 *  If the LPM module is installed clear the maximum privilege
	 *  set. This will force the user to invoke the command in argv[0]
	 *  through tfadmin command.
	 */

	if (lvlproc(MAC_GET,&level) == 0)
		procprivl(CLRPRV,pm_max(P_ALLPRIVS),0);

	execvp(argv[0], &argv[0]);
	if (errno == ENOENT) {
		rc = 127;
	} else {
		rc = 126;
	}
	pfmt(stderr, MM_ERROR, ":37:%s: %s\n", sys_errlist[errno], argv[0]);
	exit(rc);
}

/*		copyright	"%c%" 	*/

#ident	"@(#)chroot:chroot.c	1.8.2.3"
#ident  "$Header$"
/***************************************************************************
 * Command: chroot
 * Inheritable Privileges: P_FILESYS 
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/
# include <stdio.h>
# include <errno.h>
# include <unistd.h>
# include <priv.h>
#include  <sys/secsys.h>

/*
 * Procedure:     main
 *
 * Restrictions:
                 chroot(2): none
*/
main(argc, argv)
int argc;
char **argv;
{
	extern char *sys_errlist[];
	extern int sys_nerr;
	int retval;		/* scratch variable */
	uid_t priv_uid;

	if(argc < 3) {
		fprintf(stderr, "usage: chroot rootdir command [arg ... ]\n");
		exit(1);
	}
	/*restrict P_MACREAD even though not in the inheritable set*/
	procprivl(CLRPRV,MACREAD_W,(priv_t)0);
	/*Check to see if LPM is installed and use priv_uid 
	 *to determine if procprivl should be used to clear privileges
	 *or not
	 */
	priv_uid = (uid_t)secsys(ES_PRVID,0);
	retval = chroot(argv[1]);
	if (retval < 0) {
		perror(argv[1]);
		exit(1);
	}
	if (chdir("/") < 0) {
		fprintf(stderr, "can't chdir to new root\n");
		exit(1);
	}

				/*If priv_uid is less than zero this system is running
				 *with a file based privilege mechanism
				 */
				 if(priv_uid <0 || priv_uid != getuid())
					 /* Clear all privs. in the max set to prevent
						* newly execed command from inheriting P_FILESYS
						* priv.
						*/
					 procprivl(CLRPRV,pm_max(P_ALLPRIVS),(priv_t)0);
				 
	execv(argv[2], &argv[2]);
	if((errno > 0) && (errno <= sys_nerr)) 
		fprintf(stderr, "chroot: %s\n",sys_errlist[errno]);
	else 
		fprintf(stderr, "chroot: exec failed, errno = %d\n",errno);
	exit(1);
}

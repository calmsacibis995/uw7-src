#ident  "@(#)finduid.c	1.3"
#ident  "$Header$"
/*
 * Command:	finduid
 *
 * Usage:	finduid
 *
 * Level:	SYS_PRIVATE
 *
 * Inheritable Privileges:	P_DACREAD
 *	 Fixed Privileges:	none
 *
 * Notes:	prints the next available uid
*/

/* LINTLIBRARY */
#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>
#include	<mac.h>
#include	<pwd.h>
#include	<errno.h>
#include	<unistd.h>
#include	<locale.h>
#include	<pfmt.h>
#include	"uidage.h"

extern	int	errno;
extern	void	exit();
extern	uid_t	findnextuid();
extern	char	*strerror();

char *msg_label = "UX:finduid";

/* return the next available uid */
main()
{
	uid_t	uid;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxsysadm");
	(void) setlabel(msg_label);

	errno = 0;

	if (access(UIDAGEF, R_OK) != 0) {
		if (errno != ENOENT) {
			(void) pfmt(stderr, MM_ERROR | MM_NOGET, "%s\n",
				strerror(errno));
			exit(EX_FAILURE);
		}
	}

	uid = findnextuid();

	if (uid == -1)
		exit(EX_FAILURE);

	(void) fprintf(stdout, "%ld\n", uid);

	exit(EX_SUCCESS);

	/*NOTREACHED*/
}

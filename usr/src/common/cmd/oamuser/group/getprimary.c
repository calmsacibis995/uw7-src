#ident  "@(#)getprimary.c	1.3"
#ident  "$Header$"

#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <userdefs.h>
#include <locale.h>
#include "messages.h"

struct group *getgrnam();
struct passwd *getpwent();

extern void exit();
char *msg_label = "UX:getprimary";

main(argc, argv)
int argc;
char *argv[];
{
	struct group *gstruct;		/* group structure */
	struct passwd *pstruct;		/* passwd structure */
	register nprinted = 0;

	/* Set up the l10n stuff */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	if(argc != 2) {
		errmsg (M_GETPRIMARY_USAGE);
		exit( EX_SYNTAX );
	}

	/* validate group name */
	if((gstruct = getgrnam( argv[1] )) == NULL) {
		errmsg (M_GRP_INVALID, argv[1]);
		exit( EX_BADARG );
	}

	/* search passwd file looking for matches on group */
	if( (pstruct = getpwent()) != NULL ) {

		if(pstruct->pw_gid == gstruct->gr_gid) {
			nprinted++;
			(void) fprintf( stdout, "%s", pstruct->pw_name );
		}

		while( (pstruct = getpwent()) != NULL )
			if(pstruct->pw_gid == gstruct->gr_gid) {
				(void) fprintf( stdout, "%s%s", (nprinted++? ",": ""),
					pstruct->pw_name );
			}

		(void) fprintf( stdout, "\n" );

	}
		
	exit( EX_SUCCESS );
	/*NOTREACHED*/
}

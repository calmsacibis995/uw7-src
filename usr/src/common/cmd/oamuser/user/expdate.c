#ident	"@(#)oamuser:user/expdate.c	1.2.11.3"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<userdefs.h>
#include	<users.h>
#include	<locale.h>
#include	<pfmt.h>
#include	"messages.h"

char *msg_label = "UX:userls";

extern void exit();
extern int valid_expire();

/* Validate an expiration date */
main(argc, argv)
	char *argv[];
{

	/* Set up the l10n stuff */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	if (argc != 2) {
		errmsg (M_EXP_DATE);
		exit(EX_SYNTAX);
	}
	exit(valid_expire(argv[1], 0 ) == INVALID ? EX_FAILURE : EX_SUCCESS);
	/*NOTREACHED*/
}

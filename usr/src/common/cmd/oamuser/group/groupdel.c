#ident  "@(#)groupdel.c	1.5"
#ident  "$Header$"

/*
 * Command:	groupdel
 *
 * Usage:	groupdel [-K path] group
 *
 * Inheritable Privileges:	P_MACWRITE,P_SETFLEVEL,P_DACWRITE,
 *				P_MACREAD,P_DACREAD
 *       Fixed Privileges:	None
 *
 * Notes:	Delete a group definition from the system.
 *
 *		Arguments are:
 *
 *			path -  directory containing files `groupdel.pre' and `groupdel.post'
 *			group - a character string group name
 *
 *		P_MACWRITE is required for renaming tmp file to /etc/group.
 *		P_SETFLEVEL is required to reset level of that file.
 */

/* LINTLIBRARY */
#include <sys/types.h>
#include <stdio.h>
#include <userdefs.h>
#include <priv.h>
#include "messages.h"
#include <pfmt.h>
#include <locale.h>

char *msg_label = "UX:groupdel";

extern void errmsg(), exit();
extern int del_group();
extern int optind;

#define       EX_SCRIPT_ERR	50

 /*
  * Procedure:     main
  *
  * Restrictions:  none
  */

main(argc, argv)
int argc;
char **argv;
{
	register char *group;			/* group name from command line */
	register retval = 0;

	char *customdir = NULL;			/* directory name with -K option */
	int ch;					/* return from getopt */

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	while((ch = getopt(argc, argv, "K:")) != EOF)  {
		switch(ch) {
			case 'K':
				customdir = optarg;
				break;
			case '?':
				errmsg( M_GROUPDEL_USAGE );
				exit( EX_SYNTAX );
		}
	}

	if(optind != argc - 1) {
		errmsg( M_GROUPDEL_USAGE );
		exit( EX_SYNTAX );
	}
	group = argv[optind];

	if (customdir && (execScript(customdir,"groupdel.pre") != 0))
		exit(EX_SCRIPT_ERR);
	switch(retval = del_group(group)) {
        case EX_UPDATE:
                errmsg(M_GROUP_DELETE);
                break;
	case EX_NAME_NOT_EXIST:
                errmsg(M_NO_GROUP,group);
                break;
	default:
		if (customdir)
			execScript(customdir,"groupdel.post");
		break;
	}
	exit(retval);
	/*NOTREACHED*/
}

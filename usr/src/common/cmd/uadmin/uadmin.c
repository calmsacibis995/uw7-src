/*	copyright	"%c%"	*/

#ident	"@(#)uadmin:uadmin.c	1.4.5.3"

/***************************************************************************
 * Command: uadmin
 *
 * Inheritable Privileges: P_SYSOPS
 *       Fixed Privileges: None
 *
 ***************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <priv.h>
#include <sys/uadmin.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>


char *Usage = ":1:Usage: %s cmd fcn\n";
extern int errno;
int  isnumber();


/*
 * Procedure:     main
 *
 * Restrictions:
 *                uadmin(2): none
*/

main(argc, argv)
char *argv[];
{
	register cmd, fcn;
	sigset_t set, oset;
	int tmperr=0;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxuadmin");
	(void)setlabel("UX:uadmin");

	if (argc != 3) {
		pfmt(stderr, MM_ACTION, Usage, argv[0]);
		exit(1);
	}

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oset);

        if (isnumber(argv[1]) && isnumber(argv[2])) {
                cmd = atoi(argv[1]);
                fcn = atoi(argv[2]);
        }
        else {
                pfmt(stderr, MM_ERROR, ":2:cmd and fcn must be integers\n");
                exit(1);
        }

	if (uadmin(cmd, fcn, 0) < 0) {
		tmperr=errno;
		pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",strerror(errno));
	}

	sigprocmask(SIG_BLOCK, &oset, (sigset_t *)0);
	exit(tmperr);

}


/*
 * Procedure:     isnumber
 *
 * Restrictions:  none
*/

isnumber(s)
char *s;
{
        register c;

        while(c = *s++)
                if(!isdigit(c))
                        return(0);
        return(1);
}


/*		copyright	"%c%" 	*/


#ident	"@(#)rlpcmd.c	1.3"
#ident  "$Header$"

/*
 * rlpcmd	- XENIX-style remote printing from UNIX
 *
 * Brian Foster, SCO Ltd. (London), February 14th 1990.
 *
 * Quick hack to do remote printing from SCO UNIX (3.2) in
 * the style of SCO XENIX (via /usr/spool/lp/remote and the
 * `network' printer interface script).  Must be setUID-"root",
 * and should only be runnable by group "lp":
 *
 *	/usr/spool/lp/bin/rlpcmd	4110	root	lp
 *
 * This program should be listed in SecureWare's File Control
 * database (as should be all set-UID and set-GID programs).
 *
 * TCP/IPs rcmd(C?), which is set-UID-"root", uses the RUID
 * to determine the username which is to run the job on the
 * remote machine.  Printer interface scripts run with both
 * an EUID and RUID of the user who submitted the request
 * (and an EGID and RGID of group "lp").  So to do remote
 * printing over TCP/IP, this program sets its RUID to that
 * of user "lp", and then exec(S)s rcmd.  The only way to
 * set the RUID is to be have an EUID of 0 ("root") and do
 * a setuid(S), which also sets the EUID.  (Fortunately, the
 * way the XENIX `network' interface script works, the EUID
 * does not really matter!)
 *
 * The remote machine's user "lp" must be configured as
 * equivalent to the local machine's user "lp".  This is
 * probably best done by a `.rhosts' file in the home
 * directory of the remote machine's user "lp".
 *
 * Whilst this program has only been tested printing from
 * SCO UNIX (3.2) to SCO XENIX (2.3), it is believed to
 * work from SCO UNIX to any system which understands the
 * TCP/IP rcmd protocal.
 */
/*
 * Modification History
 *
 *  28-01-97 Paul Cunningham   Ported to UnixWare
 *
 */
#include <stdio.h>
#include <errno.h>
#include <pwd.h>

#include <locale.h>
#include "rlp_msg.h"
nl_catd catd;

/* For UnixWare we need to use 'rsh' instead of 'rcmd' (used in OpenServer)
 */
char	rcmd[]	= "/usr/bin/rsh";


/*******************************************************************************
 *
 * Discription: Main function for the 'rlpcmd' command
 *
 *******************************************************************************
 */

main( int argc, char *argv[])
{
	struct passwd *pwd;
	int e, u;


	setlocale(LC_ALL,"");
	catd = catopen( MF_RLP, MC_FLAGS);

	if ((pwd = getpwnam( "lp")) == (struct passwd *)0)
	{
	  (void)fputs
                 (
	         MSGSTR( RLPM_CMD_NUID,"Cannot determine UID of user \"lp\"\n"),
	         stderr
	         ); 
	  exit(1);
	  /* NOTREACHED */
	}

	u = pwd->pw_uid;
	endpwent();
	if ( setuid( u))
	{
	  e = errno;
	  perror( "setuid");
	}
	else 
	{
	  /* execute the command remotely
	   */
	  argv[0] = rcmd;
	  execv( rcmd, argv);
	  e = errno;
	  perror( rcmd);
	}

	exit(e);
	/* NOTREACHED */
}


/*******************************************************************************
 *               End of Module
 *******************************************************************************
 */

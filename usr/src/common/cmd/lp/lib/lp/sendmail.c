/*		copyright	"%c%" 	*/

#ident	"@(#)sendmail.c	1.2"
#ident	"$Header$"
/* sendmail(user, msg) -- send msg to user's mailbox */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "lp.h"

void
#if	defined(__STDC__)
sendmail (
	char *			user,
	char *			msg
)
#else
sendmail (user, msg)
	char			*user,
				*msg;
#endif
{
	FILE			*pfile;

	char			*mailcmd;

	if (isnumber(user))
		return;

	if ((mailcmd = Malloc(strlen(MAIL) + 1 + strlen(user) + 1))) {
		(void)sprintf (mailcmd, "%s %s", MAIL, user);
		if ((pfile = popen(mailcmd, "w"))) {
			(void)fprintf (pfile, "%s\n", msg);
			(void)pclose (pfile);
		}
		Free (mailcmd);
	}
	return;
}

/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/lib/namtouid.c	1.7.3.4"
#ident "$Header$"
/*
 *	namtouid converts login names to uids
 *	maintains ulist for speed only
 */
#include <stdio.h>
#include <sys/types.h>
#include "acctdef.h"
#include <pwd.h>
static	usize;
static	struct ulist {
	char	uname[NSZ];
	uid_t	uuid;
} ul[A_USIZE];
char	ntmp[NSZ+1];

char	*strncpy();

uid_t
namtouid(name)
char	name[NSZ];
{
	register struct ulist *up;
	register uid_t tuid;
	struct passwd *getpwnam(), *pp;

	for (up = ul; up < &ul[usize]; up++)
		if (strncmp(name, up->uname, NSZ) == 0)
			return(up->uuid);
	strncpy(ntmp, name, NSZ);
	(void) setpwent();
	while ((pp = getpwent()) != NULL) {
		if (strncmp(name, pp->pw_name, NSZ) == 0) {
	    		tuid = pp->pw_uid;
	    		if (usize < A_USIZE) {
	      			CPYN(up->uname, name);
	     			up->uuid = tuid;
	      			usize++;
			}
			break;
		}
	}

	if (pp == NULL) 
	  	tuid = -1;

	return(tuid);
}

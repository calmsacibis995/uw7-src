/*		copyright	"%c%" 	*/

#ident	"@(#)acct:common/cmd/acct/lib/uidtonam.c	1.6.1.3"
#ident "$Header$"
/*
 * convert uid to login name; interface to getpwuid that keeps up to USIZE1
 * names to avoid unnecessary accesses to passwd file
 * returns ptr to NSZ-byte name (not necessarily null-terminated)
 * returns ptr to "?" if cannot convert
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include "acctdef.h"
#include <pwd.h>

static	usize1;
static struct ulist {
	char	uname[NSZ];
	uid_t	uuid;
} ul[USIZE1];

char *strncpy();

char *
uidtonam(uid)
uid_t	uid;
{
	register struct ulist *up;
	struct passwd *getpwuid();
	register struct passwd *pp;

	for (up = ul; up < &ul[usize1]; up++)
		if (uid == up->uuid)
			return(up->uname);
	setpwent();
	if ((pp = getpwuid(uid)) == NULL)
		return("?");
	else {
		if (usize1 < USIZE1) {
			up->uuid = uid;
			CPYN(up->uname, pp->pw_name);
			usize1++;
		}
		return(pp->pw_name);
	}
}

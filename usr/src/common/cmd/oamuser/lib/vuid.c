#ident	"@(#)vuid.c	1.2"
#ident  "$Header$"

#include	<sys/types.h>
#include	<stdio.h>
#include	<pwd.h>
#include	<userdefs.h>
#include	<users.h>

#include	<sys/param.h>

#ifndef MAXUID
#include	<limits.h>
#define	MAXUID	UID_MAX
#endif

struct passwd *getpwuid();

int
valid_uid(uid, pptr)
	uid_t uid;
	struct passwd **pptr;
{
	register struct passwd *t_pptr;

	if (uid < 0)
		return INVALID;

	if (uid > MAXUID)
		return TOOBIG;

	if (t_pptr = getpwuid(uid)) {
		if (pptr)
			*pptr = t_pptr;
		return NOTUNIQUE;
	}

	if (uid <= DEFRID) {
		if (pptr)
			*pptr = getpwuid(uid);
		return RESERVED;
	}

	return UNIQUE;
}

/*		copyright	"%c%" 	*/

#ident	"@(#)getpwent.c	1.2"
#ident  "$Header$"

#include "lpsched.h"

/*
 * These routines duplicate some of those of "getpwent(3C)". We have
 * them so that we can use our special "open_lpfile()" routine,
 * which opens files and REUSES preallocated buffers. Without
 * this, every new print job will hit malloc with a request for
 * a large buffer; this typically (with most versions of malloc)
 * leads to increased fragmentation of the free memory arena.
 */

#include "sys/types.h"
#include "stdlib.h"
#include "pwd.h"
#include "string.h"

#include "lp.h"

static char		PASSWD[] = "/etc/passwd";

static FILE		*pwf = NULL;

/*
 * Procedure:     lp_setpwent
 *
 * Restrictions:
 *               open_lpfile: None
 *               rewind: None
*/

void
#ifdef	__STDC__
lp_setpwent (
	void
)
#else
lp_setpwent ()
#endif
{
	DEFINE_FNNAME (lp_setpwent)

	if (!pwf)
		pwf = open_lpfile(PASSWD, "r", 0);
	else
		rewind (pwf);
}

/*
 * Procedure:     lp_endpwent
 *
 * Restrictions:
 *               close_lpfile: None
*/
void
#ifdef	__STDC__
lp_endpwent (
	void
)
#else
lp_endpwent ()
#endif
{
	DEFINE_FNNAME (lp_endpwent)

	if (pwf) {
		(void) close_lpfile(pwf);
		pwf = (FILE *)0;
	}
}

/*
 * Procedure:     lp_getpwuid
 *
 * Restrictions:
 *               fgetpwent: None
*/

struct passwd *
#ifdef	__STDC__
lp_getpwuid (
	register uid_t		uid
)
#else
lp_getpwuid (uid)
	register uid_t		uid;
#endif
{
	DEFINE_FNNAME (lp_getpwuid)

	register struct passwd *p;

	p = getpwuid(uid);

	return (p);
}

/*
 * Procedure:     lp_getpwnam
 *
 * Restrictions:
 *               fgetpwent: None
*/

struct passwd *
#ifdef	__STDC__
lp_getpwnam (
	char *			name
)
#else
lp_getpwnam (name)
	char *			name;
#endif
{
	DEFINE_FNNAME (lp_getpwnam)

	register struct passwd *p;

	lp_setpwent ();

	/*
	 * Call the REAL routine to access the data (not "getpwent()",
	 * though, which will call the REAL "setpwent()" which we don't
	 * want to happen!)
	 */
	while ((p = fgetpwent(pwf)) && strcmp(name, p->pw_name))
		;

	lp_endpwent ();
	return (p);
}
#ifdef	__STDC__
char *
lp_uidtoname (uid_t uid)
#else
char *
lp_uidtoname (uid)

uid_t	uid;
#endif
{
	char *	namep;
	struct
	passwd *passwdp;

	passwdp = lp_getpwuid (uid);
	lp_endpwent();

	if (passwdp && passwdp->pw_name && *passwdp->pw_name)
		namep = Strdup (passwdp->pw_name);
	else
	{
		namep = Strdup(BIGGEST_NUMBER_S);
		(void) sprintf (namep, "%ld", uid);
	}
	return	namep;
}

/*		copyright	"%c%" 	*/

#ident	"@(#)isadmin.c	1.2"
#ident	"$Header$"
/*
 *
 * Administrator check - only for Unix 4.0 lp support. 
 *
 */

#include "downloader.h"

#if (LEVEL & 01) && (LEVEL & 02)
#include <stdio.h>
#include <pwd.h>

#include "gen.h"

#define ADMIN	"lp"

/*****************************************************************************/

isadmin()

{

    int			uid;
    char		*l;
    struct passwd	*p;

    char		*getenv();
    struct passwd	*getpwnam(), *getpwuid();

/*
 *
 * Returns TRUE if the user is root or lp - trivial variation of lp's getname().
 *
 */

    if ( (uid = getuid()) == 0 )
	return(TRUE);

    setpwent();

    if ( (l = getenv("LOGNAME")) == NULL || (p = getpwnam(l)) == NULL || p->pw_uid != uid )
	if ( (p = getpwuid(uid)) != NULL )
	    l = p->pw_name;
	else l = NULL;

    if ( l != NULL && strcmp(l, ADMIN) == 0 )
	return(TRUE);

    return(FALSE);

}   /* End of isadmin */

/*****************************************************************************/

#endif


/*		copyright	"%c%" 	*/

#ident	"@(#)uucpname.c	1.2"
#ident "$Header$"

#include "uucp.h"
#include <sys/utsname.h>

/*
 * get the uucp name
 * return:
 *	none
 */
void
uucpname(name)
register char *name;
{
	char *s;

	struct utsname utsn;

	uname(&utsn);
	s = utsn.nodename;

	(void) strncpy(name, s, MAXBASENAME);
	name[MAXBASENAME] = '\0';
	return;
}

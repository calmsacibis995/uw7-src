/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*		copyright	"%c%" 	*/

#ident	"@(#)gethostnm.c	1.2"
#ident	"$Header$"

#include <unistd.h>
#include <string.h>
#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <sys/utsname.h>
#include "lpd.h"

char *
#if defined (__STDC__)
lp_gethostname(void)
#else
lp_gethostname()
#endif
{
	struct utsname	utsname;
	static char 	lhost[HOSTNM_LEN];

	if (uname(&utsname) < 0)
		return(NULL);
	strncpy(lhost, utsname.nodename, sizeof(lhost));
	return(lhost);
}

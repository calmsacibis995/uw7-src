#ident	"@(#)pcintf:bridge/common.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)common.c	6.4	LCC);	/* Modified: 14:12:58 1/7/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <string.h>

#if !defined(UDP42)		/* for UDP42 use gethostname(), not uname() */
#	include <sys/utsname.h>
#endif

#if defined(POSIX)
#	include	<unistd.h>
#endif

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"


#if defined(UDP42)
extern int	gethostname	PROTO((char *, int));
#else
extern int	uname		PROTO((struct utsname *));
#endif

#if !defined(POSIX)
#	if defined(BERKELEY42)
extern int	getdtablesize	PROTO((VOID));
#	else
extern long	ulimit		PROTO((int, long));
#	endif
#endif	/* !POSIX */


char *myhostname()
{
	char *p, *dp;
#if defined(UDP42)
	static char hostName[256];

	/* Some uname() implementations can only return 9 characters,  */
	/* so use gethostname(), which usually can return at least 64. */
	p = (gethostname(hostName, sizeof hostName) >= 0) ? hostName : "unix";
#else
	static struct utsname uName;

	p = (uname(&uName) >= 0) ? uName.nodename : "unix";
#endif

	/*
	 *  Truncate name so that it will fit into a map entry.
	 *  Also, truncate it to contain only the first domain component.
	 */
	dp = p;
	while (*dp && *dp != '.' && dp < &p[SZHNAME - 1])
		dp++;
	*dp = '\0';
	return p;
}



static char
	qName[MAX_FN_TOTAL];		/* Fully qualified name (result) */

/*
   fnQualify: Fully qualify and canonicalize a path name
	   w.r.t a current directory.
*/

char *
fnQualify(fName, dName)
register char
	*fName;				/* File name to qualify */
char
	*dName;				/* Current working directory */
{
register char
	*qApp;				/* Appends fName to qName */

	/* If file name is absolute, initialize qName to /, otherwise dName */
	if (*fName != '/') {
		(void) strcpy(qName, dName);
		qApp = &qName[strlen(qName) - 1];
	} else {
		qApp = qName;
		*qApp = '/';
		fName++;
	}

	for (;;) {
		/* Insert path separator */
		if (*qApp != '/')
			*++qApp = '/';

		/*
		   If leading component of fName is .. or ../,
		   remove trailing component of qName
		*/
		if ((*fName == '.') && (
		    (fName[1] == '.'  && (fName[2] == '/' || fName[2] == '\0'))
		    || (fName[1] == '/'))) {
		        if (fName[1] == '.') {
			/* Remove trailing component of qName, if any */
				if (qApp > qName)
					do {
						*qApp-- = '\0';
					} while (*qApp != '/');
			}
			/* Skip over .. or ./ in fName */
			fName += 2;
		} else
			/* ..else append leading component of fName to qName */
			do {
				*++qApp = *fName++;
			} while (*fName != '\0' && *fName != '/');

		/* Skip leading /'s of next fName component */
		while (*fName == '/')
			fName++;

		/* If at end of fName, quit */
		if (*fName == '\0')
			break;
	}

	/* remove trailing slash, but not if full name is only '/' */
	if (*qApp == '/' && qApp != qName)
		*qApp = '\0';
	else
		qApp[1] = '\0';

	return qName;
}



/*
**	uMaxDescriptors:
**		returns system-configured per-process descriptor table size
*/

int uMaxDescriptors()
{
	static int maxDesc = 0;

	if (maxDesc == 0) {

#if defined(POSIX)
		maxDesc = (int)sysconf(_SC_OPEN_MAX);
#elif defined(BERKELEY42)
		maxDesc = getdtablesize();
#else
		maxDesc = (int)ulimit(4, 0L);
#endif

	}
	return maxDesc;
}

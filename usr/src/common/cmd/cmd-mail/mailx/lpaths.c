#ident	"@(#)lpaths.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)lpaths.c	1.8 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 */

/*
 *	libpath(file) - return the full path to the library file
 *	If POST is defined in the environment, use that.
 */

#ifndef preSVr4
#include <unistd.h>
#endif
#include "rcv.h"

static char	buf[500];

#ifdef preSVr4
char *
libpath(filename)
char	*filename;
{
	sprintf(buf, "%s/%s", LIBPATH, filename);
	return buf;
}
#endif

/*
 * Return the path to a help file.
 * In non-internationalised version, the help file is directly available
 * In internationalised version, there are localised versions of the
 * help files located in a directory. The name of the directory is the
 * name of the help file in non-internationalised version. The name of
 * the files correspond to the name of the locale for the messages.
 */
char *
helppath(filename)
char	*filename;
{
#ifndef preSVr4
	char		*locale;
#endif
	
#ifdef preSVr4		
	/* non-internationalised version */
	sprintf(buf, "%s/%s", LIBPATH, filename);
#else
	/* internationalised version */
	if ((locale = setlocale(LC_MESSAGES, (char*)NULL)) == NULL)
		locale = "C";
	sprintf(buf, "%s/%s/%s", LIBPATH, locale, filename);
	if (access(buf, R_OK) < 0)
		sprintf(buf, "%s/C/%s", LIBPATH, filename);
#endif
	return buf;
}

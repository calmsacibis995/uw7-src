/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xgetenv.c	11.1"
#ident	"@(#)xgetenv.c	11.1"
#include "libmail.h"
/*
    NAME
	xsetenv, xgetenv, Xgetenv - manage an alternate environment space

    SYNOPSIS
	int xsetenv(file)
	char *xgetenv("FOO");
	char *Xgetenv("FOO");

    DESCRIPTION
	xsetenv() reads the given file into an internal buffer
	and sets up an alternate environment.

	Return values:	 1 - OKAY
			 0 - troubles reading the file
			-1 - troubles opening the file

	xgetenv() returns the environment value from the
	alternate environment.

	Return values:	(char*)0 - no value for that variable
			pointer  - the value

	Xgetenv() returns the environment value from the
	alternate environment.

	Return values:	"" - no value for that variable
			pointer  - the value

    LIMITATIONS
	Assumes the environment is < 5120 bytes (as in the UNIX
	System environment). Assumes < 512 lines in the file.
	These values may be adjusted below.

*/

#define MAXVARS  512
#define MAXENV  5120

static char **xenv = 0;
static char *(xenvptrs[MAXVARS+1]);
static char xbuf[MAXENV+1];

static void reduce ARGS((char *from));
static char *skipcomments ARGS((char*p));

/*
 *	set up an environment buffer
 *	and the pointers into it
 */
#ifdef __STDC__
int xsetenv(char *xfile)
#else
int xsetenv(xfile)
char *xfile;
#endif
{
    register char *endl = 0;
    register int envctr, nread, infd;

    /* Open the file */
    infd = open(xfile, O_RDONLY);
    if (infd == -1) {
	return (-1);
    }

    /* Read in the entire file. */
    nread = read(infd, xbuf, sizeof(xbuf)-1);
    if (nread < 0) {
	(void) close(infd);
	return (0);
    }
    xbuf[nread] = '\0';

    /*
	Set up pointers into the buffer.
	Replace \n with \0.
	Collapse white space around the = sign and at the
	beginning and end of the line.
	Remove comments.
    */
    xenv = xenvptrs;
    envctr = 0;
    for (xenv[envctr] = skipcomments(xbuf); xenv[envctr] != 0; xenv[++envctr] = skipcomments(endl + 1))
	{
	endl = strchr(xenv[envctr], '\n');
	if (!endl || (envctr == MAXVARS)) break;
	*endl = '\0';
	reduce(xenv[envctr]);
	}

    xenv[envctr] = 0;
    (void) close(infd);
    return (1);
}

/*
 *	Let getenv() do the dirty work
 *	of looking up the variable. We
 *	do this by temporarily resetting
 *	environ to point to the local area.
 */
#ifdef __STDC__
char *xgetenv(const char *env)
#else
char *xgetenv(env)
char *env;
#endif
{
    extern char **environ;
    register char *ret, **svenviron = environ;

    environ = xenv;
    ret = getenv(env);
    environ = svenviron;
    return ret;
}

/*
 *	Let xgetenv() do the dirty work
 *	of looking up the variable.
 */
#ifdef __STDC__
char *Xgetenv(const char *env)
#else
char *Xgetenv(env)
char *env;
#endif
{
    char *ret = xgetenv(env);
    return ret ? ret : "";
}

/*
 * Skip leading white space and comment lines.
 */
#ifdef __STDC__
static char *skipcomments(char *p)
#else
static char *skipcomments(p)
char *p;
#endif
{
    /* Skip leading space on the line. */
    while (Isspace(*p))
	p++;

    /* Check for a comment. */
    while (*p == '#')
	{
	/* Skip the comment line. */
	while (*p && (*p != '\n'))
	    p++;
	if (*p) p++;

	/* Skip leading space on the next line. */
	while (Isspace(*p))
	    p++;
	}

    if (*p) return p;
    else return 0;
}

/*
 * Remove the spaces within the environment variable.
 * The variable can look like this:
 *
 * <sp1> variable <sp2> = <sp3> value <sp4> \0
 *
 * All spaces can be removed, except within
 * the value.
 */

#ifdef __STDC__
static void reduce(register char *from)
#else
static void reduce(from)
register char *from;
#endif
{
    register char *to = from;
    register char *svfrom = from;

    /* <sp1> */
    while (*from && Isspace(*from))
	from++;

    /* variable */
    while (*from && (*from != '=') && !Isspace(*from))
	*to++ = *from++;

    /* <sp2> */
    while (*from && Isspace(*from))
	from++;

    /* = */
    if (*from == '=')
	*to++ = *from++;

    /* <sp3> */
    while (*from && Isspace(*from))
	from++;

    /* value */
    while (*from)
	*to++ = *from++;

    /* <sp4> */
    while ((to > svfrom) && Isspace(to[-1]))
	to--;
    *to = '\0';
}

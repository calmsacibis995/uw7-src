/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mgetenv.c	11.1"
#ident	"@(#)mgetenv.c	11.1"
# include "libmailx.h"
/*
    NAME
	mgetenv, Mgetenv - manage the /etc/mail/mailcnfg environment space

    SYNOPSIS
	char *mgetenv(char *name);
	char *Mgetenv(char *name);

    DESCRIPTION
	mgetenv() returns the environment value from the
	/etc/mail/mailcnfg environment.

	Return values:	(char*)0 - no value for that variable
			pointer  - the value

	Mgetenv() returns the environment value from the
	/etc/mail/mailcnfg environment.

	Return values:	"" - no value for that variable
			pointer  - the value

	All work is passed on to xgetenv() and Xgetenv(),
	with a check for xsetenv(MAILCNFG).
*/

static int xset = 0;

static void msetenv()
{
    struct stat statb;
    static char mailcnfg[] = MAILCNFG;
    if (xsetenv(mailcnfg) != 1)
	if (stat(mailcnfg, &statb) == 0)
	    {
	    /* file DOES exist! */
	    lfmt(stderr, MM_ERROR, ":119:Cannot access %s: %s\n",
		mailcnfg, Strerror(errno));
	    exit(1);
	    /* NOTREACHED */
	    }

    xset = 1;
}

#ifdef __STDC__
char *mgetenv(const char *env)
#else
char *mgetenv(env)
char *env;
#endif
{
    if (xset == 0)
	msetenv();
    return xgetenv(env);
}

#ifdef __STDC__
char *Mgetenv(const char *env)
#else
char *Mgetenv(env)
char *env;
#endif
{
    if (xset == 0)
	msetenv();
    return Xgetenv(env);
}

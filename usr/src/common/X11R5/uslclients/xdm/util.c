#ident	"@(#)xdm:util.c	1.4"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: util.c,v 1.13 91/04/17 10:06:32 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * util.c
 *
 * various utility routines
 */

# include   "dm.h"
#if defined(X_NOT_POSIX) || defined(_POSIX_SOURCE)
# include   <signal.h>
#else
#define _POSIX_SOURCE
# include   <signal.h>
#undef _POSIX_SOURCE
#endif

printEnv (e)
char	**e;
{
	while (*e)
		Debug ("%s\n", *e++);
}

static char *
makeEnv (name, value)
char	*name;
char	*value;
{
	char	*result;

#ifdef DEBUG
	Debug("makeEnv name=%s value=%s\n",name?name : "<NULL>",value?value: "<NULL>");
#endif
	result = malloc ((unsigned) (strlen (name) + strlen (value) + 2));
	if (!result) {
		LogOutOfMem ("makeEnv");
		return 0;
	}
	sprintf (result, "%s=%s", name, value);
	return result;
}

char *
getEnv (e, name)
	char	**e;
	char	*name;
{
	int	l = strlen (name);

#ifdef DEBUG
	if (name) Debug("getEnv name=%s\n",name);
#endif
	while (*e) {
		if ((int)strlen (*e) > l && !strncmp (*e, name, l) &&
			(*e)[l] == '=')
			return (*e) + l + 1;
		++e;
	}
	return 0;
}

char **
setEnv (e, name, value)
	char	**e;
	char	*name;
	char	*value;
{
	char	**new, **old;
	char	*newe;
	int	envsize;
	int	l;

#ifdef DEBUG
	if (name) Debug("setEnv name=%s \n",name);
	if (value) Debug("setEnv  value=%s\n",value);
#endif
	l = strlen (name);
	newe = makeEnv (name, value);
	if (!newe) {
		LogOutOfMem ("setEnv");
		return e;
	}
	if (e) {
		for (old = e; *old; old++)
			if ((int)strlen (*old) > l && !strncmp (*old, name, l) && (*old)[l] == '=')
				break;
		if (*old) {
			free (*old);
			*old = newe;
			return e;
		}
		envsize = old - e;
		new = (char **) realloc ((char *) e,
				(unsigned) ((envsize + 2) * sizeof (char *)));
	} else {
		envsize = 0;
		new = (char **) malloc (2 * sizeof (char *));
	}
	if (!new) {
		LogOutOfMem ("setEnv");
		free (newe);
		return e;
	}
	new[envsize] = newe;
	new[envsize+1] = 0;
	return new;
}

freeEnv (env)
    char    **env;
{
    char    **e;

#ifdef DEBUG
	Debug("freeEnv\n");
#endif
    if (env)
    {
    	for (e = env; *e; e++)
	    free (*e);
    	free (env);
    }
}

# define isblank(c)	((c) == ' ' || c == '\t')

char **
parseArgs (argv, string)
char	**argv;
char	*string;
{
	char	*word;
	char	*save;
	int	i;

	i = 0;
	while (argv && argv[i])
		++i;
	if (!argv) {
		argv = (char **) malloc (sizeof (char *));
		if (!argv) {
			LogOutOfMem ("parseArgs");
			return 0;
		}
	}
	word = string;
	for (;;) {
		if (!*string || isblank (*string)) {
			if (word != string) {
				argv = (char **) realloc ((char *) argv,
					(unsigned) ((i + 2) * sizeof (char *)));
				save = malloc ((unsigned) (string - word + 1));
				if (!argv || !save) {
					LogOutOfMem ("parseArgs");
					if (argv)
						free ((char *) argv);
					if (save)
						free (save);
					return 0;
				}
				argv[i] = strncpy (save, word, string-word);
				argv[i][string-word] = '\0';
				i++;
			}
			if (!*string)
				break;
			word = string + 1;
		}
		++string;
	}
	argv[i] = 0;
	return argv;
}

freeArgs (argv)
    char    **argv;
{
    char    **a;

    if (!argv)
	return;

    for (a = argv; *a; a++)
	free (*a);
    free ((char *) argv);
}

CleanUpChild ()
{
#if defined(SYSV) || defined(SVR4)
/*
 *	WIPRO : Kumar K.V.
 *	CHANGE # UNKNOWN
 *	FILE # util.c
 * 
 *  The setpgrp function has been disabled as an effective workaround
 *  for the "mouse not working" problem. Since in the daemon mode the 
 *  parent daemon has already executed a setpgrp it is a process and 
 *  session leader. Since it has also gotten rid of the controlling 
 *  terminal there is no great harm in not making the sub-daemons as 
 *  leaders. 
 *  This problem was detected on the i386 platform only.
 *  
 *	ENDCHANGE # UNKNOWN
 */
	/*
	setpgrp ();
	*/
#else
	setpgrp (0, getpid ());
	sigsetmask (0);
#endif
#ifdef SIGCHLD
	(void) Signal (SIGCHLD, SIG_DFL);
#endif
	(void) Signal (SIGTERM, SIG_DFL);
	(void) Signal (SIGPIPE, SIG_DFL);
	(void) Signal (SIGALRM, SIG_DFL);
	(void) Signal (SIGHUP, SIG_DFL);
	CloseOnFork ();
}

static char localHostbuf[256];
static int  gotLocalHostname;

char *
localHostname ()
{
#ifdef DEBUG
	Debug("localHostname\n");
#endif
    if (!gotLocalHostname)
    {
	XmuGetHostname (localHostbuf, sizeof (localHostbuf) - 1);
	gotLocalHostname = 1;
    }
    return localHostbuf;
}

SIGVAL (*Signal (sig, handler))()
    int sig;
    SIGVAL (*handler)();
{
#ifndef X_NOT_POSIX
    struct sigaction sigact, osigact;
    sigact.sa_handler = handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(sig, &sigact, &osigact);
    return osigact.sa_handler;
#else
    return signal(sig, handler);
#endif
}

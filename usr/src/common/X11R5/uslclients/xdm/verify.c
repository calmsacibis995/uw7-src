/*              copyright "%c%" */

#ident	"@(#)xdm:verify.c	1.33"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: verify.c,v 1.24 91/07/18 22:22:45 rws Exp $
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
 * verify.c
 *
 * typical unix verification routine.
 */

#include	"dm.h"
#include	<pwd.h>
#ifdef USE_IAF
#include <sys/types.h>
#include <utmpx.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>			/* For logfile locking */
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <utime.h> 
#include <termio.h>
#include <sys/stropts.h>
#include <shadow.h>			/* shadow password header file */
#include <time.h>
#include <sys/param.h> 
#include <sys/fcntl.h>
#include <deflt.h>
#include <grp.h>
#include <ia.h>
#include <sys/vnode.h>
#include <errno.h>
#include <lastlog.h>
#include <iaf.h>
#include <sys/audit.h>
#include <mac.h>
#include <priv.h>
#include <sys/secsys.h>
#include <locale.h>
#include <sys/stream.h>
#include <sys/resource.h>

static char	*encryptPass;
#endif

#include	<crypt.h>
#ifdef X_NOT_STDC_ENV
char *getenv();
#endif

int	xset_id();
static	int	xset_env();

extern	char	**environ;

int
CreateEnv (user, verify, d, fd)
char *user;
struct verify_info	*verify;
struct display *d;
int	fd;
{
	int		ret = 0;
	char		**userEnv (), **systemEnv (), **parseArgs ();
	char 		*home;
	char		**argv;

 	struct passwd *p;

#ifdef DEBUG
	Debug ("CreateEnv for user=%s\n",user); 
#endif
	argv = 0;
	if (*d->session != 0) {
		argv = parseArgs (argv, d->session);
                Debug ("d->session set to %s\n", d->session);
        }

	verify->argv = argv;
	p = (struct passwd *) getpwnam(user);
	if (!p) {
		fprintf(stderr,"BadLogName: %s\n", user);
		return 0;
	}
	home = p->pw_dir;
	Debug("home for user=%s is %s\n", user, home);
	verify->userEnviron = userEnv (d, 0, home);
	if ((ret = xset_id(verify, d->name, fd)) != 0) {
		Debug ("Failure in xset_id() = %d\n", ret);
		return 1;
	}
	Debug ("user environment:\n");
	printEnv (verify->userEnviron);
	home = (char *) getenv("HOME");
	verify->systemEnviron = systemEnv (d, user, home);
	Debug ("system environment:\n");
	printEnv (verify->systemEnviron);
	Debug ("end of environments\n");

	return 0;
}

extern char **setEnv ();

char **
defaultEnv ()
{
    char    **env, **exp, *value;
    char  *var;
    int i;
    env = 0;

    for (exp = userExportList; exp && *exp; ++exp)
    {
#ifdef DEBUG
	Debug("defaultEnv: exp=%s\n", *exp);
#endif
	i = strfind(*exp, "=");
	if (i) {
		var = strdup(*exp);
		if (!var) {
			Debug("Unable to allocate environment variables\n");
			break;
		}
		var[i] = NULL;	
		value = *exp + 1 + i;
	}
	if (value) {
#ifdef DEBUG
	    if (var) Debug("defaultEnv: setting var = %s value=%s\n",var,value);

#endif
	    env = setEnv (env, var, value);
	}
	 if (var) {
		free(var);
    		var = 0;
		}
	}

    for (exp = exportList; exp && *exp; ++exp)
    {
        value = getenv (*exp);
        if (value)
            env = setEnv (env, *exp, value);
    }
 
    return env;
}

char **
userEnv (d, useSystemPath, home)
struct display	*d;
int	useSystemPath;
char *home;
{
    char	**env;
    char	**envvar;
    char	*str;
    
#ifdef DEBUG
	Debug("userEnv\n");
#endif
    env = defaultEnv ();
    env = setEnv (env, "HOME", home);
    env = setEnv (env, "TERM", "xterm");
    env = setEnv (env, "PATH", useSystemPath ? d->systemPath : d->userPath);
    env = setEnv (env, "DISPLAY", d->name);
    env = setEnv (env, "LANG", getenv("LANG")); 
#ifdef DEBUG
 	Debug("set DISPLAY to d->name=%s\n", d->name);
#endif
    env = setEnv (env, "CONSEM", "no");
    return env;
}

char **
systemEnv (d, user, home)
struct display	*d;
char	*user, *home;
{
    char	**env;
#ifdef DEBUG
    Debug("systemEnv\n");
#endif
    
    env = defaultEnv ();
    env = setEnv (env, "DISPLAY", d->name);
    if (home)
	env = setEnv (env, "HOME", home);
    if (user)
	env = setEnv (env, "LOGNAME", user);
    env = setEnv (env, "PATH", d->systemPath);
    env = setEnv (env, "SHELL", d->systemShell);
    if (d->authFile)
	    env = setEnv (env, "XAUTHORITY", d->authFile);
#ifdef DEBUG
    if (env) Debug("env=%s\n",*env);
#endif
    return env;
}

/*
 * Procedure:	xset_id
 *
 * Notes:	called to set the necessary attributes for this user
 *		based on information retrieved via the "ava" stream
 *		or the I&A database file.
 */
int
xset_id(verify, display, fd)
struct verify_info	*verify;
char			*display;
int			fd;
{
	int	i;
	char	**avap;
	char	*p, *tp;

#ifdef DEBUG
	Debug("xset_id\n");
#endif
	avap = retava (fd);

	if ((p = getava("UID", avap)) != NULL)
		verify->uid = atol(p);
	else
		return 1;

	if ((p = getava("GID", avap)) != NULL) 
		verify->gid = atol(p);
	else
		return 1;

	if ((p = getava("GIDCNT", avap)) != NULL) {
		verify->ngroups = atol(p);

		if (verify->ngroups) {
			if ((p = getava("SGID", avap)) == NULL) 
				return 1;
			for (i = 0; i < verify->ngroups; i++) {
				verify->groups[i] = strtol(p, &tp, 10);
				p = ++tp;
			}
		}
	} else
		return 1;

	if (set_id(NULL) != 0) {
		Debug ("Problem with set_id\n");
		return 1;
	}

	if (xset_env (verify, display, fd) != 0) {
		Debug ("Problem with set_env\n");
		return 1;
	}
	return 0;
}

static	int
xset_env(verify,display, fd)
struct verify_info	*verify;
char	*display;
int	fd;
{
	char	*ptr;
	char	**envp;
	char	**avap;
	char	env[BUFSIZ] = { "ENV=" };
	char	disp[1024];

	extern	char	**environ;
	extern	char	*argvtostr();
	extern	char	**strtoargv();
	char **f;

#ifdef DEBUG
	Debug("xset_env\n");
#endif
	if ((avap = retava(fd)) == NULL)
		return 1;

	if ((ptr = getava("ENV", avap)) == NULL)
		return 1;

	if ((envp = strtoargv(ptr)) == NULL)
		return 1;

	environ = envp;
#ifdef DEBUG
	 
        if (envp) {
        	Debug ("xset_env %s: ", *envp);
        	for (f = envp; *f; f++) 
                	Debug ("%s ", *f);
		Debug (";\n ");
        	
    	}
		/* add user environment variables to the exported list */
#endif

        if (verify->userEnviron) {
#ifdef DEBUG
        	Debug ("xset_env verify->userEnviron= %s: ", *verify->userEnviron);
#endif
        	for (f = verify->userEnviron; *f; f++) {
#ifdef DEBUG
                Debug ("%s ", *f);
#endif
        	(void) putenv(*f);	
	}
		Debug (";\n ");
    	}



	(void) putenv("XDM_LOGIN=yes");
	(void) putenv("CONSEM=no");
	(void) sprintf (disp, "DISPLAY=%s", display);
	(void) putenv(disp);

	if ((ptr = argvtostr(environ)) == NULL)
		return 1;

	(void) strcat(env, ptr);

	if ((avap = putava(env, avap)) == NULL)
		return 1;

	(void) setava (fd, avap);

	return 0;
}

/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:wksh.c	1.5"

#include <stdio.h>
#include <sys/stat.h>

char *strdup();

#define CONSTCHAR (const char *)
#define TRUE 1
#define FALSE 0

#ifndef WKSHLIBDIR
#define WKSHLIBDIR "/usr/X/lib/wksh"
#endif

#ifndef WKSHBINDIR
#define WKSHBINDIR "/usr/X/bin"
#endif

static int
FileExists(dir, file)
char *dir, *file;
{
	struct stat sbuf;
	char path[1024];

	sprintf(path, "%s/%s", dir, file);
	return(stat(path, &sbuf) != -1);
}

static int
File2Exists(dir, subdir, file)
char *dir, *subdir, *file;
{
	struct stat sbuf;
	char path[1024];

	sprintf(path, "%s/%s/%s", dir, subdir, file);
	return(stat(path, &sbuf) != -1);
}



/*
 * Bootstrap wksh by calling either olwksh or xmwksh and forcing it to execute
 * an rc file that calls the wksh init function and does some
 * other minor housekeeping.
 *
 * The rc file then sees if there was a previous user rc file (in $_HOLDENV_)
 * and if so executes it.
 *
 * The user can choose motif vs. open look by making the first
 * command line option -motif or -openlook.  If neither of these options
 * is chosen, then next it looks to see if the variable WKSH_TOOLKIT is
 * set to either the string MOTIF or OPENLOOK.  Failing this, it defaults
 * to OPENLOOK (for compatibility reasons, sorry Motif users).
 */

int
main(argc, argv)
int argc;
char *argv[];
{
	char *getenv();
	char *env, *var;
	char *libdir = NULL, *bindir = NULL;
	char *executable, *envfile;
	char *buf;
	char envbuf[1024];
	char *getenv(), *malloc();
#define GUI_UNKNOWN  0
#define GUI_OPENLOOK 1
#define GUI_MOTIF    2
	int gui = GUI_UNKNOWN;
	int need_compress = 0;
#if defined (SVR4_USER)
	char *newldpath;
	char *ldpath = getenv("LD_LIBRARY_PATH");
	static char svr4xlib[] = "/usr/X/lib";

	/*
	 * Corrects a problem that occurs on SVR4.2 systems.  If a user
	 * makes WKSH their login shell on such systems, they cannot login
	 * because $LD_LIBRARY_PATH is not set by /etc/passwd, thus the
	 * X libraries are not found.  If we are on a 4.0 or later system,
	 * we make sure /usr/X/lib is in the $LD_LIBRARY_PATH before
	 * proceding.
	 */
	 
	newldpath = malloc(strlen(ldpath ? ldpath : "") + sizeof(svr4xlib) + 32);
	sprintf(newldpath, "LD_LIBRARY_PATH=%s:%s", ldpath ? ldpath : "", svr4xlib);
	putenv(newldpath);
#endif
	/*
	 * Set the ENV variable to the standard wksh rc file, which
	 * will do a libdirload of the wksh shared object file and add all
	 * the wksh commands and widgets to the exksh.  If the user already
	 * had an ENV file, then set the variable _HOLDENV_ to it so the
	 * standard wksh rc file can execute it later.
	 */
	env = getenv((const char *)"ENV");
	buf = (char *)malloc((env ? strlen(env) : 0) + 12);
	strcpy(buf, "_HOLDENV_=");
	strcat(buf, env ? env : "");
	putenv(buf); 

	/*
	 * if the first argument is "-motif" or "-openlook" then
	 * use that widget set.
	 */

	need_compress = 0;
	if (argc > 1) {
		if (strcmp(argv[1], CONSTCHAR "-motif") == 0) {
			gui = GUI_MOTIF;
			need_compress = TRUE;
		} else if (strcmp(argv[1], CONSTCHAR "-openlook") == 0) {
			gui = GUI_OPENLOOK;
			need_compress = TRUE;
		}
	}
	/*
	 * if one of the special args was found, move down all other
	 * args to get rid of it.
	 */
	if (need_compress) {
		register int i;

		for (i = 1; i < argc; i++)
			argv[i] = argv[i+1];
		argc--;
	}
	/*
	 * If there is still no gui named, look for the variable
	 * WKSH_TOOLKIT to determine which to use.
	 */
	if (gui == GUI_UNKNOWN) {
		char *guivar = getenv(CONSTCHAR "WKSH_TOOLKIT");
		if (guivar && 
			(strcmp(guivar, CONSTCHAR "OPENLOOK") == 0 ||
			 strcmp(guivar, CONSTCHAR "OPEN_LOOK") == 0)) {
			gui = GUI_OPENLOOK;
		} else if (guivar && strcmp(guivar, CONSTCHAR "MOTIF") == 0) {
			gui = GUI_MOTIF;
		}
	}

	/* still unknown, default to OPEN LOOK */

	if (gui == GUI_UNKNOWN)
		gui = GUI_OPENLOOK;

	switch (gui) {
	case GUI_MOTIF:
		executable = "xmwksh";
		envfile = "xmwksh.rc";
		break;
	case GUI_OPENLOOK:
		executable = "olwksh";
		envfile = "olwksh.rc";
		break;
	}
	/*
	 * Search order for WKSH libraries (binaries):
	 *
	 * 1. if $WKSHLIBDIR ($WKSHBINDIR) is set, use that path if it
	 *    exists.
	 * 2. if the hard-wired #define WKSHLIBDIR (WKSHBINDIR) is
	 *    set and is a readable directory, use it.
	 * 3. if we're on a SUN system, and $OPENWINHOME is set,
	 *    use $OPENWINHOME/lib/wksh ($OPENWINHOME/bin)
	 * 4. if $XWINHOME is set, use $XWINHOME/lib/wksh ($XWINHOME/bin)
	 * 5. punt.
	 */

	if ((libdir = getenv((const char *)"WKSHLIBDIR")) != NULL) {
		if (!FileExists(libdir, envfile))
			libdir = NULL;
	}
	if (libdir == NULL && FileExists(WKSHLIBDIR, envfile)) {
		libdir = WKSHLIBDIR;
	}
#ifdef SUNOS
	if (libdir == NULL && (var = getenv((const char *)"OPENWINHOME")) != NULL) {
		if (File2Exists(var, "lib", envfile)) {
			sprintf(envbuf, "%s/lib", var);
			libdir = strdup(envbuf);
		}
	}
#endif
	if (libdir == NULL && (var = getenv((const char *)"XWINHOME")) != NULL) {
		if (File2Exists(var, "lib", envfile)) {
			sprintf(envbuf, "%s/lib", var);
			libdir = strdup(envbuf);
		}
	}

	if (libdir == NULL) {
		fprintf(stderr, "wksh: bootstrap failed.  Try setting $WKSHLIBDIR.\n");
		exit(1);
	}
	sprintf(envbuf, "WKSHLIBDIR=%s", libdir);
	putenv(strdup(envbuf));

	if ((bindir = getenv((const char *)"WKSHBINDIR")) != NULL) {
		if (!FileExists(bindir, executable))
			bindir = NULL;
	}
	if (bindir == NULL && FileExists(WKSHBINDIR, executable)) {
		bindir = WKSHBINDIR;
	}
#ifdef SUNOS
	if (bindir == NULL && (var = getenv((const char *)"OPENWINHOME")) != NULL) {
		if (File2Exists(bindir, "bin", executable)) {
			sprintf(envbuf, "%s/bin", var);
			bindir = strdup(envbuf);
		}
	}
#endif
	if (bindir == NULL && (var = getenv((const char *)"XWINHOME")) != NULL) {
		if (File2Exists(bindir, "bin", executable)) {
			sprintf(envbuf, "%s/bin", var);
			bindir = strdup(envbuf);
		}
	}

	if (bindir == NULL) {
		fprintf(stderr, "wksh: bootstrap failed.  Try setting $WKSHBINDIR.\n");
		exit(1);
	}
	sprintf(envbuf, "WKSHBINDIR=%s", bindir);
	putenv(strdup(envbuf));

	sprintf(envbuf, (const char *) "ENV=%s/%s", libdir, envfile);
	putenv(strdup(envbuf));
	sprintf(envbuf, (const char *) "%s/%s", bindir, executable);
	execv(envbuf, argv);

	fprintf(stderr, (const char *)"%s: Bootstrap of wksh failed: '%s'\n", argv[0], envbuf);
	perror("Reason");
	return(1);	/* failure */
}

#pragma ident	"@(#)dtbuiltin:dtbuiltin.c	1.10"

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <limits.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "builtin.h"

static XrmDatabase GetResources(int * argc, char * argv[]);
static int	do_builtin(Display **, int *);
static void	builtin(Display *, int, int, char * argv[], char * envp[]);
static void	normal(int argc, char * argv[]);

static char *		prog_bname;		/* basename(argv[0]) */
static XrmDatabase	resDB;
static void		(*ClientsMainLoop)(Display *, int);
static Display *	ClientsDisplay;

main(int argc, char * argv[], char * envp[])
{
    Display *	dpy;
    int		event_base;
    char	name[PATH_MAX];
    char	bname[PATH_MAX];

    prog_bname = bname;
    strcpy(name, argv[0]);
    strcpy(bname, basename(name));

    /* Create resource database (needed below) */
    resDB = GetResources(&argc, argv);

    if (do_builtin(&dpy, &event_base))
	builtin(dpy, event_base, argc, argv, envp);
    else
	normal(argc, argv);
}

/****************************************************************************
 * GetXDefaultsDB-
 */
static XrmDatabase
GetXDefaultsDB(void)
{
    char *	environment;
    char *	home;
    char	name[PATH_MAX];

    if (environment = getenv("XENVIRONMENT"))
    {
	strcpy(name, environment);

    } else if (home = getenv("HOME"))
    {
	strcpy(name, home);
	strcat(name, "/.Xdefaults");

    } else
	return(NULL);

    return(XrmGetFileDatabase(name));
}

/****************************************************************************
 * GetResources-
 */
static XrmDatabase
GetResources(int * argc, char * argv[])
{
    static XrmOptionDescRec option_tbl[] = {
	{ "-builtin",	".builtin",	XrmoptionSepArg, NULL },
	{ "-daemonize",	".daemonize",	XrmoptionSepArg, NULL },
	};
    XrmDatabase	resDB = NULL;
    XrmDatabase	db;

    XrmInitialize();
    if (db = XrmGetFileDatabase("/usr/lib/X11/app-defaults/Dtbuiltin"))
	XrmMergeDatabases(db, &resDB);
    if (db = GetXDefaultsDB())
	XrmMergeDatabases(db, &resDB);
    db = NULL;
    XrmParseCommand(&db, option_tbl, sizeof(option_tbl)/sizeof(option_tbl[0]),
		    prog_bname, argc, argv);
    if (db)
	XrmMergeDatabases(db, &resDB);
    return(resDB);
}

/****************************************************************************
 * Daemonize- get "daemonize" resource and fork() if True.  Assumes this is
 *	called once by builtin() or normal() and is called just before
 *	executing client.  (This means resDB won;'t be used again so it can
 *	be destroyed).
 *
 *	Daemonizing is done to reduce contention during startup (between
 *	clients started up in .olinitrc and the desktop manager).  Once the
 *	client has finished its initialization, it can return control to the
 *	shell executing .olinitrc.  (Initialization completion is indicated
 *	by client about to drop into MainLoop).
 */
static void
Daemonize(void)
{
    char	res_name[PATH_MAX];
    char *	res_class = res_name;
    char *	str_type;
    XrmValue	value;
    int		daemonize;

    strcpy(res_name, prog_bname);
    strcat(res_name, ".daemonize");
    daemonize =
	!XrmGetResource(resDB, res_name, res_class, &str_type, &value) ||
	    (strncmp(value.addr, "True", value.size) == 0) ||
		(strncmp(value.addr, "true", value.size) == 0);
    XrmDestroyDatabase(resDB);

    if (daemonize)
	if (fork() != 0)
	    exit(0);	/* exit parent or if fork fails */
}

/****************************************************************************
 * do_builtin- return whether client can be run builtin to server.
 *
 *	Open a connection to the server and see if it supports the "builtin"
 *	feature (ie, we're running with our server).
 */
static int
do_builtin(Display ** ret_dpy, int * ret_event_base)
{
    Display *	dpy;
    int		event_base, error_base;
    int		major_version, minor_version;
    u_char	valid;
    int		res_builtin;
    char	res_name[PATH_MAX];
    char *	res_class = res_name;
    char *	str_type;
    XrmValue	value;

    strcpy(res_name, prog_bname);
    strcat(res_name, ".builtin");
    res_builtin =
	!XrmGetResource(resDB, res_name, res_class, &str_type, &value) ||
	    (strncmp(value.addr, "True", value.size) == 0) ||
		(strncmp(value.addr, "true", value.size) == 0);

    if (!res_builtin || ((dpy = XOpenDisplay(NULL)) == NULL))
	return(0);

    if (XBuiltinQueryExtension(dpy, &event_base, &error_base) &&
	XBuiltinQueryVersion(dpy, prog_bname,
			     &valid, &major_version, &minor_version) &&
	valid && (major_version == 1) && (minor_version == 0))
    {
	*ret_dpy	= dpy;
	*ret_event_base	= event_base;
	return(1);
    } else
    {
	XCloseDisplay(dpy);
	return(0);
    }
}

/****************************************************************************
 * normal- run client normally (not builtin to the server).
 *
 */
static void
normal(int argc, char * argv[])
{
    void *	dl_handle;
    int		(*client_main)(int, char **);
    char	bname[PATH_MAX];

    /* The convention is to dlopen basename of argv[0] with ".so" appended */
    strcpy(bname, prog_bname);
    if (!(dl_handle = dlopen(strcat(bname, ".so"), RTLD_LAZY | RTLD_GLOBAL)))
    {
	fprintf(stderr, "dlopen failed: %s\n", dlerror());
	exit(1);
    }
	
    if (!(client_main = (int (*)(int, char **)) dlsym(dl_handle, "main")))
    {
	fprintf(stderr, "dlsym failed to find main: %s\n", dlerror());
	exit(1);
    }
    (void)client_main(argc, argv);	/* Call client's main */
    Daemonize();
    ClientsMainLoop(ClientsDisplay, 0);	/* process events */
    /*NOTREACHED*/
}

/****************************************************************************
 * builtin- run client "builtin" to the server.
 *	Run the client and drop into event loop waiting for "event"
 */
static void
builtin(Display * dpy, int event_base, int argc, char * argv[], char * envp[])
{
    if (!XBuiltinRunClient(dpy, argc, argv, envp))
	exit(1);

    Daemonize();

    while (1)
    {
	XEvent	event;
	int	builtin_event_type;

	XNextEvent(dpy, &event);

	if ((builtin_event_type = event.type - event_base) >= 0)
	{
	    int		exit_code;
	    char *	args[1000];
	    int		i;

	    switch(builtin_event_type)
	    {
	    case BuiltinExitNotify:
		XCloseDisplay(dpy);
		exit_code = ((XBuiltinExitNotifyEvent *)&event)->exit_code;
		exit(exit_code);
		/* NOTREACHED */

	    case BuiltinExecNotify:
		XCloseDisplay(dpy);
		for(i = 0; i < argc; i++)
		    args[i] = argv[i];
		args[argc] = NULL;
		execvp(args[0], args);
		/* NOTREACHED */
	    }
	}
    }
}

/****************************************************************************
 * BIRegisterInternalClient-
 */
/*ARGSUSED*/
void
BIRegisterClient(int c_s_fd, Display * dpy,
		 void (*main_loop_func)(Display *, int),
		 void (*clean_up_func)(Display *))
{
    ClientsDisplay	= dpy;
    ClientsMainLoop	= main_loop_func;
}

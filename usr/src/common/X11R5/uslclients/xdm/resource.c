#ident	"@(#)xdm:resource.c	1.24.1.2"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: resource.c,v 1.44 91/07/24 00:06:52 keith Exp $
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
 * resource.c
 */

# include "dm.h"
# include <X11/Intrinsic.h>
# include <X11/Xmu/CharSet.h>

char	*config;

char	*servers;
int	request_port;
int	debugLevel;
char	*errorLogFile;
int	daemonMode;
char	*pidFile;
int	lockPidFile;
char	*authDir;
int	autoRescan;
int	removeDomainname;
char	*keyFile;
char	*accessFile;
char	*backgroundPixmap;
char	*companyLogoPixmap;
char	**userExportList;
char	**exportList;
char	*randomFile;
int	show_mne;

# define DM_STRING	0
# define DM_INT		1
# define DM_BOOL	2
# define DM_ARGV	3

/*
 * the following constants are supposed to be set in the makefile from
 * parameters set util/imake.includes/site.def (or *.macros in that directory
 * if it is server-specific).  DO NOT CHANGE THESE DEFINITIONS!
 */
#ifndef DEF_SERVER_LINE 
#define DEF_SERVER_LINE ":0 local /usr/X/bin/X :0"
#endif
#ifndef XRDB_PROGRAM
#define XRDB_PROGRAM "/usr/X/bin/xrdb"
#endif
#ifndef DEF_SESSION
#define DEF_SESSION (char *)0
#endif
#ifndef DEF_USER_PATH
#define DEF_USER_PATH ":/usr/bin:/usr/X/bin"
#endif
#ifndef DEF_SYSTEM_PATH
#define DEF_SYSTEM_PATH "/sbin:/usr/sbin:/usr/bin:/etc:/usr/X/bin"
#endif
#ifndef DEF_SYSTEM_SHELL
#define DEF_SYSTEM_SHELL "/bin/sh"
#endif
#ifndef DEF_FAILSAFE_CLIENT
#define DEF_FAILSAFE_CLIENT "/usr/X/bin/nondesktop"
#endif
#ifndef DEF_XDM_CONFIG
#define DEF_XDM_CONFIG "/usr/X/lib/xdm/xdm-config"
#endif
#ifndef DEF_CHOOSER
#define DEF_CHOOSER "/usr/X/lib/xdm/chooser"
#endif
#ifndef DEF_XPM_FILE
#define DEF_XPM_FILE "/usr/X/lib/pixmaps/Nlogo.xpm"
#endif

#ifndef DEF_AUTH_NAME
#ifdef HASXDMAUTH
#define DEF_AUTH_NAME	"XDM-AUTHORIZATION-1 MIT-MAGIC-COOKIE-1"
#else
#define DEF_AUTH_NAME	"MIT-MAGIC-COOKIE-1"
#endif
#endif
#ifndef DEF_AUTH_DIR
#define DEF_AUTH_DIR "/dev/X/"
#endif
#ifndef DEF_USER_AUTH_DIR
#define DEF_USER_AUTH_DIR	""
#endif
#ifndef DEF_KEY_FILE
#define DEF_KEY_FILE	""
#endif
#ifndef DEF_ACCESS_FILE
#define DEF_ACCESS_FILE	""
#endif
#ifndef DEF_RANDOM_FILE
#define DEF_RANDOM_FILE "/dev/mem"
#endif
#ifndef	DEF_LINK_PATH
#define	DEF_LINK_PATH	"/usr/X/lib"
#endif

#define DEF_UDP_PORT	"177"	    /* registered XDMCP port, dont change */

struct dmResources {
	char	*name, *class;
	int	type;
	char	**dm_value;
	char	*default_value;
} DmResources[] = {
"servers",	"Servers", 	DM_STRING,	&servers,
				DEF_SERVER_LINE,
"requestPort",	"RequestPort",	DM_INT,		(char **) &request_port,
				DEF_UDP_PORT,
"showMnemonic","ShowMnemonic",	DM_INT,		(char **) &show_mne,
				"1",	/* > 0 == YES */
"debugLevel",	"DebugLevel",	DM_INT,		(char **) &debugLevel,
				"0",
"errorLogFile",	"ErrorLogFile",	DM_STRING,	&errorLogFile,
				"/var/xdm/xdm-errors",
"daemonMode",	"DaemonMode",	DM_BOOL,	(char **) &daemonMode,
				"true",
"pidFile",	"PidFile",	DM_STRING,	&pidFile,
				"",
"lockPidFile",	"LockPidFile",	DM_BOOL,	(char **) &lockPidFile,
				"true",
"authDir",	"authDir",	DM_STRING,	&authDir,
				DEF_AUTH_DIR,
"autoRescan",	"AutoRescan",	DM_BOOL,	(char **) &autoRescan,
				"true",
"removeDomainname","RemoveDomainname",DM_BOOL,	(char **) &removeDomainname,
				"true",
"keyFile",	"KeyFile",	DM_STRING,	&keyFile,
				DEF_KEY_FILE,
"accessFile",	"AccessFile",	DM_STRING,	&accessFile,
				DEF_ACCESS_FILE,

"backgroundPixmap","BackgroundPixmap",DM_STRING,  &backgroundPixmap,
				NULL,

"companyLogoPixmap","CompanyLogoPixmap",DM_STRING,  &companyLogoPixmap,
				NULL,

"userExportList","UserExportList",DM_ARGV, (char **) &userExportList,
				"",
"exportList",	"ExportList",	DM_ARGV,	(char **) &exportList,
				"",
"randomFile",	"RandomFile",	DM_STRING,	&randomFile,
				DEF_RANDOM_FILE,
};

# define NUM_DM_RESOURCES	(sizeof DmResources / sizeof DmResources[0])

# define boffset(f)	XtOffsetOf(struct display, f)

struct displayResource {
	char	*name, *class;
	int	type;
	int	offset;
	char	*default_value;
};

/* resources for managing the server */

struct displayResource serverResources[] = {
"serverAttempts","ServerAttempts",DM_INT,	boffset(serverAttempts),
				"1",
"openDelay",	"OpenDelay",	DM_INT,		boffset(openDelay),
				"2",
"openRepeat",	"OpenRepeat",	DM_INT,		boffset(openRepeat),
				"7",
"openTimeout",	"OpenTimeout",	DM_INT,		boffset(openTimeout),
				"30",
"startAttempts","StartAttempts",DM_INT,		boffset(startAttempts),
				"4",
"thrashThreshold", "ThrashThreshold", DM_INT,	boffset(thrashThreshold),
				"60",
"trashThreshold", "TrashThreshold", DM_INT,	boffset(trashThreshold),
				"5",
"pingInterval",	"PingInterval",	DM_INT,		boffset(pingInterval),
				"5",
"pingTimeout",	"PingTimeout",	DM_INT,		boffset(pingTimeout),
				"5",
"terminateServer","TerminateServer",DM_BOOL,	boffset(terminateServer),
				"false",
"grabServer",	"GrabServer",	DM_BOOL,	boffset(grabServer),
				"false",
"grabTimeout",	"GrabTimeout",	DM_INT,		boffset(grabTimeout),
				"3",
"resetSignal",	"Signal",	DM_INT,		boffset(resetSignal),
				"1",	/* SIGHUP */
"termSignal",	"Signal",	DM_INT,		boffset(termSignal),
				"15",	/* SIGTERM */
"resetForAuth",	"ResetForAuth",	DM_BOOL,	boffset(resetForAuth),
				"false",
"authorize",	"Authorize",	DM_BOOL,	boffset(authorize),
				"true",
"authComplain",	"AuthComplain",	DM_BOOL,	boffset(authComplain),
				"true",
"authName",	"AuthName",	DM_ARGV,	boffset(authNames),
				DEF_AUTH_NAME,
"authFile",	"AuthFile",	DM_STRING,	boffset(clientAuthFile),
				"",
"showIcons",	"ShowIcons",	DM_BOOL,	boffset(showIcons),
				"false",
"showRootIcon",	"ShowRootIcon",	DM_BOOL,	boffset(showRootIcon),
				"false",
};

# define NUM_SERVER_RESOURCES	(sizeof serverResources/\
				 sizeof serverResources[0])

/* resources which control the session behaviour */

struct displayResource sessionResources[] = {
"resources",	"Resources",	DM_STRING,	boffset(resources),
				"",
"xrdb",		"Xrdb",		DM_STRING,	boffset(xrdb),
				XRDB_PROGRAM,
"setup",	"Setup",	DM_STRING,	boffset(setup),
				"",
"startup",	"Startup",	DM_STRING,	boffset(startup),
				"",
"reset",	"Reset",	DM_STRING,	boffset(reset),
				"",
"session",	"Session",	DM_STRING,	boffset(session),
				DEF_SESSION,
"userPath",	"Path",		DM_STRING,	boffset(userPath),
				DEF_USER_PATH,
"systemPath",	"Path",		DM_STRING,	boffset(systemPath),
				DEF_SYSTEM_PATH,
"systemShell",	"Shell",	DM_STRING,	boffset(systemShell),
				DEF_SYSTEM_SHELL,
"failsafeClient","FailsafeClient",	DM_STRING,	boffset(failsafeClient),
				DEF_FAILSAFE_CLIENT,
"userAuthDir",	"UserAuthDir",	DM_STRING,	boffset(userAuthDir),
				DEF_AUTH_DIR,
"linkPath",	"LinkPath",	DM_STRING,	boffset(linkPath),
				DEF_LINK_PATH,
"chooser",	"Chooser",	DM_STRING,	boffset(chooser),
				DEF_CHOOSER,
};

# define NUM_SESSION_RESOURCES	(sizeof sessionResources/\
				 sizeof sessionResources[0])

XrmDatabase	DmResourceDB;

GetResource (name, class, valueType, valuep, default_value)
    char    *name, *class;
    int	    valueType;
    char    **valuep;
    char    *default_value;
{
    char	*type;
    XrmValue	value;
    char	*string, *new_string;
    char	str_buf[50];
    int	len;
    extern char **parseArgs();

    if (DmResourceDB && XrmGetResource (DmResourceDB,
	name, class,
	&type, &value))
    {
	string = value.addr;
	len = value.size;
    }
    else
    {
	string = default_value;
	len = strlen (string);
    }

    Debug ("%s/%s value %*.*s\n", name, class, len, len, string);

    if (valueType == DM_STRING && *valuep)
    {
	Debug("*valuep=%s\n",*valuep);
	if (strlen (*valuep) == len && !strncmp (*valuep, string, len))
	    return;
	else
	    free (*valuep);
    }

    switch (valueType) {
    case DM_STRING:
#ifdef DEBUG
	Debug("DM_STRING\n");
#endif

	new_string = malloc ((unsigned) (len+1));
	if (!new_string) {
		LogOutOfMem ("GetResource");
		return;
	}
	strncpy (new_string, string, len);
	new_string[len] = '\0';
#ifdef DEBUG
	Debug("new_string=%s\n",new_string);
#endif
	*(valuep) = new_string;
	break;
    case DM_INT:
#ifdef DEBUG
	Debug("DM_INT\n");
#endif
	strncpy (str_buf, string, sizeof (str_buf));
	str_buf[sizeof (str_buf)-1] = '\0';
	*((int *) valuep) = atoi (str_buf);
	break;
    case DM_BOOL:
#ifdef DEBUG
	Debug("DM_BOOL\n");
#endif
	strncpy (str_buf, string, sizeof (str_buf));
	str_buf[sizeof (str_buf)-1] = '\0';
	XmuCopyISOLatin1Lowered (str_buf, str_buf);
	if (!strcmp (str_buf, "true") ||
	    !strcmp (str_buf, "on") ||
	    !strcmp (str_buf, "yes"))
		*((int *) valuep) = 1;
	else if (!strcmp (str_buf, "false") ||
		 !strcmp (str_buf, "off") ||
		 !strcmp (str_buf, "no"))
		*((int *) valuep) = 0;
	break;
    case DM_ARGV:
#ifdef DEBUG
	Debug("DM_ARGV\n");
#endif
	freeArgs (*(char ***) valuep);
	*((char ***) valuep) = parseArgs ((char **) 0, string);
	break;
    }
}

XrmOptionDescRec configTable [] = {
{"-server",	NULL,			XrmoptionSkipArg,	(caddr_t) NULL },
{"-udpPort",	NULL,			XrmoptionSkipArg,	(caddr_t) NULL },
{"-error",	NULL,			XrmoptionSkipArg,	(caddr_t) NULL },
{"-resources",	NULL,			XrmoptionSkipArg,	(caddr_t) NULL },
{"-session",	NULL,			XrmoptionSkipArg,	(caddr_t) NULL },
{"-debug",	NULL,			XrmoptionSkipArg,	(caddr_t) NULL },
{"-xrm",	NULL,			XrmoptionSkipArg,	(caddr_t) NULL },
{"-config",	".configFile",		XrmoptionSepArg,	(caddr_t) NULL }
};

XrmOptionDescRec optionTable [] = {
{"-server",	".servers",		XrmoptionSepArg,	(caddr_t) NULL },
{"-udpPort",	".requestPort",		XrmoptionSepArg,	(caddr_t) NULL },
{"-error",	".errorLogFile",	XrmoptionSepArg,	(caddr_t) NULL },
{"-resources",	"*resources",		XrmoptionSepArg,	(caddr_t) NULL },
{"-session",	"*session",		XrmoptionSepArg,	(caddr_t) NULL },
{"-debug",	"*debugLevel",		XrmoptionSepArg,	(caddr_t) NULL },
{"-xrm",	NULL,			XrmoptionResArg,	(caddr_t) NULL },
{"-daemon",	".daemonMode",		XrmoptionNoArg,		"true"         },
{"-nodaemon",	".daemonMode",		XrmoptionNoArg,		"false"        }
};

static int	originalArgc;
static char	**originalArgv;

InitResources (argc, argv)
int	argc;
char	**argv;
{
#ifdef DEBUG
	Debug("InitResources\n");
#endif
	XrmInitialize ();
	originalArgc = argc;
	originalArgv = argv;
	ReinitResources ();
}

ReinitResources ()
{
    int	argc;
    char	**a;
    char	**argv;
    XrmDatabase newDB;

#ifdef DEBUG
    Debug("ReinitResource\n");
#endif
    argv = (char **) malloc ((originalArgc + 1) * sizeof (char *));
    if (!argv)
	LogPanic ("no space for argument realloc\n");
    for (argc = 0; argc < originalArgc; argc++)
	argv[argc] = originalArgv[argc];
    argv[argc] = 0;
    if (DmResourceDB)
	XrmDestroyDatabase (DmResourceDB);
    DmResourceDB = XrmGetStringDatabase ("");
    /* pre-parse the command line to get the -config option, if any */
    XrmParseCommand (&DmResourceDB, configTable,
		     sizeof (configTable) / sizeof (configTable[0]),
		     "DisplayManager", &argc, argv);
    Debug("Call GetResource for configfile\n");
    GetResource ("DisplayManager.configFile", "DisplayManager.ConfigFile",
		 DM_STRING, &config, DEF_XDM_CONFIG);
#ifdef DEBUG
if (config) Debug("config=%s\n",config);
#endif
    newDB = XrmGetFileDatabase ( config );
    if (newDB)
    {
	Debug("newDB\n");
	if (DmResourceDB) {
	    Debug("destroy DmResourceDB\n");
	    XrmDestroyDatabase (DmResourceDB);
	}
	DmResourceDB = newDB;
    }
    else if (argc != originalArgc)
	LogError ("Can't open configuration file %s\n", config );
    XrmParseCommand (&DmResourceDB, optionTable,
		     sizeof (optionTable) / sizeof (optionTable[0]),
		     "DisplayManager", &argc, argv);
    if (argc > 1)
    {
	LogError ("extra arguments on command line:");
	for (a = argv + 1; *a; a++)
		LogError (" \"%s\"", *a);
	LogError ("\n");
    }
    free (argv);
}

LoadDMResources ()
{
	int	i;
	char	name[1024], class[1024];

#ifdef DEBUG
	Debug("LoadDMResources\n");
#endif
	for (i = 0; i < NUM_DM_RESOURCES; i++) {
		sprintf (name, "DisplayManager.%s", DmResources[i].name);
		sprintf (class, "DisplayManager.%s", DmResources[i].class);
		GetResource (name, class, DmResources[i].type,
			      (char **) DmResources[i].dm_value,
			      DmResources[i].default_value);
	}
}

static
CleanUpName (src, dst, len)
char	*src, *dst;
int	len;
{
#ifdef DEBUG
    Debug("CleanUpName\n");
#endif
    while (*src) {
	if (--len <= 0)
		break;
	switch (*src)
	{
	case ':':
	case '.':
	    *dst++ = '_';
	    break;
	default:
	    *dst++ = *src;
	}
	++src;
    }
    *dst = '\0';
}

LoadDisplayResources (d, resources, numResources)
    struct display	    *d;
    struct displayResource  *resources;
    int			    numResources;
{
    int	i;
    char	name[1024], class[1024];
    char	dpyName[512], dpyClass[512];

#ifdef DEBUG
    Debug("LoadDisplayResources\n");
#endif
    CleanUpName (d->name, dpyName, sizeof (dpyName));
    CleanUpName (d->class ? d->class : d->name, dpyClass, sizeof (dpyClass));
    for (i = 0; i < numResources; i++) {
	    sprintf (name, "DisplayManager.%s.%s", 
		    dpyName, resources[i].name);
	    sprintf (class, "DisplayManager.%s.%s",
		    dpyClass, resources[i].class);
	    GetResource (name, class, resources[i].type,
			  (char **) (((char *) d) + resources[i].offset),
			  resources[i].default_value);
    }
}

LoadServerResources (d)
    struct display  *d;
{
    Debug("LoadServerResources\n");
    LoadDisplayResources (d, serverResources, NUM_SERVER_RESOURCES);
}

LoadSessionResources (d)
    struct display  *d;
{
    Debug("LoadSessionResources\n");
    LoadDisplayResources (d, sessionResources, NUM_SESSION_RESOURCES);
}

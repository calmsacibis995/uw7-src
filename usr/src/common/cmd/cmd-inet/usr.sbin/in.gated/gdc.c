#ident	"@(#)gdc.c	1.3"
#ident	"$Header$"

/*
 * Public Release 3
 * 
 * $Id$
 */

/*
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1996, 1997 The Regents of the University of Michigan
 * All Rights Reserved
 *  
 * Royalty-free licenses to redistribute GateD Release
 * 3 in whole or in part may be obtained by writing to:
 * 
 * 	Merit GateDaemon Project
 * 	4251 Plymouth Road, Suite C
 * 	Ann Arbor, MI 48105
 *  
 * THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE REGENTS OF THE
 * UNIVERSITY OF MICHIGAN AND MERIT DO NOT WARRANT THAT THE
 * FUNCTIONS CONTAINED IN THE SOFTWARE WILL MEET LICENSEE'S REQUIREMENTS OR
 * THAT OPERATION WILL BE UNINTERRUPTED OR ERROR FREE. The Regents of the
 * University of Michigan and Merit shall not be liable for
 * any special, indirect, incidental or consequential damages with respect
 * to any claim by Licensee or any third party arising from use of the
 * software. GateDaemon was originated and developed through release 3.0
 * by Cornell University and its collaborators.
 * 
 * Please forward bug fixes, enhancements and questions to the
 * gated mailing list: gated-people@gated.merit.edu.
 * 
 * ------------------------------------------------------------------------
 * 
 * Copyright (c) 1990,1991,1992,1993,1994,1995 by Cornell University.
 *     All rights reserved.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT
 * LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * GateD is based on Kirton's EGP, UC Berkeley's routing
 * daemon	 (routed), and DCN's HELLO routing Protocol.
 * Development of GateD has been supported in part by the
 * National Science Foundation.
 * 
 * ------------------------------------------------------------------------
 * 
 * Portions of this software may fall under the following
 * copyrights:
 * 
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and
 * this paragraph are duplicated in all such forms and that
 * any documentation, advertising materials, and other
 * materials related to such distribution and use
 * acknowledge that the software was developed by the
 * University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote
 * products derived from this software without specific
 * prior written permission.  THIS SOFTWARE IS PROVIDED
 * ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * gdc - a way to diddle gated (even if you are otherwise unprivileged)
 */

#define INCLUDE_CTYPE
#define INCLUDE_TIME
#define INCLUDE_SIGNAL
#define	INCLUDE_FILE
#define	INCLUDE_STAT
#define	INCLUDE_WAIT
#define	MALLOC_OK

#include "include.h"
#include <pwd.h>
#include <grp.h>
#if	defined(GDC_RESOURCE) || defined(GDC_CORESIZE) || defined(GDC_FILESIZE) || defined(GDC_DATASIZE) || defined(GDC_STACKSIZE)
#include <sys/resource.h>
#define	GDC_DOING_RESOURCES	1
#endif	/* defined(GDC_RESOURCE) || defined(GDC_CORESIZE) || defined(GDC_FILESIZE) || defined(GDC_DATASIZE) || defined(GDC_STACKSIZE) */

#define	STREQ(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

/*
 * This provides more-user-friendly support for delivering signals
 * to gated, and doing other useful things.
 *
 * If root runs the program it does exactly what it is told.
 *
 * If a non-root user runs the program it may also do what it is told iff:
 *
 *  (1) gdc is installed setuid root
 *
 *  (2) a magic group exists (by default, "gdmaint") and the user is
 *	a member
 *
 *  (3) the user running the program is known to the password file
 *
 *  (4) if the program is run with a particular name (by default, "gdc")
 *	so we have some (minimal) comfort the guy won't run the program
 *	undetected by accounting.
 *
 * Runs are logged to syslog() so you can keep track of who did what
 * when.  Attempts to run the program by people the program thinks shouldn't
 * have are logged loudly.
 *
 * The program maintains a discipline for maintaining configuration files
 * which you may or may not like.  The configuration file names it knows
 * by default (see GDC*SUFFIX and GDCCONFIG* below) are
 *
 *	/etc/gated.conf+
 *	/etc/gated.conf
 *	/etc/gated.conf-
 *	/etc/gated.conf--
 *
 * When you do a "gdc rotate" the files are rotated so the "+" version
 * becomes the active version, the active version becomes the "-" version
 * and the "-" version becomes the "--" version.  The previous "--" version
 * disappears.
 *
 * When you do a "gdc backout" the files are rotated back so that the
 * active version becomes the "+" version, the "-" version becomes the
 * active version and the "--" version becomes the "-".  It will decline
 * to delete a non-zero sized "+" version when doing this, however,
 * unless "gdc BACKOUT" is done to force this.
 *
 * Gdc provides commands to change the mode of all the above files
 * to 0664, owner root, group gdmaint, and to create a zero-length "+"
 * file if none exists already.  This allows an otherwise unprivileged
 * user full control over gated configuration and operation.  If you
 * don't want this part add a -DGDCFILEMODE=0 to the Makefile.  If
 * you don't want any users other than root running gdc don't
 * install it setuid, or don't put any users in the gdc group.
 */


/*
 * Number of groups in the group list to check.  There are too many
 * different ways to obtain this number from a system to worry about
 * this much.
 */
#ifndef	GDC_GROUPS_MAX
#define	GDC_GROUPS_MAX	512
#endif	/* GDC_GROUPS_MAX */

/*
 * Owner for our configuration files
 */
#ifndef	GDCFILEUID
#define	GDCFILEUID	0
#endif	/* GDCFILEUID */

/*
 * Default time to wait for gated to die.
 */
#ifndef	GDC_WAIT
#define	GDC_WAIT	10
#endif	/* GDC_WAIT */

/*
 * Name of a core file on this system.  This could be "core.%s", where
 * the %s is replaced by gated's name.
 */
#ifndef	GDCCORENAME
#define	GDCCORENAME	"core"
#endif	/* GDCCORENAME */

/*
 * Suffixes.  We derive the names of the auxilliary .conf files by appending
 * these to them.
 */
#ifndef	GDCNEWSUFFIX
#define	GDCNEWSUFFIX	"+"
#endif	/* GDCNEWSUFFIX */

#ifndef	GDCOLDSUFFIX
#define	GDCOLDSUFFIX	"-"
#endif	/* GDCOLDSUFFIX */

#ifndef	GDCREALOLDSUFFIX
#define	GDCREALOLDSUFFIX	"--"
#endif	/* GDCREALOLDSUFFIX */

/*
 * Directory and file name to which parse errors get sent
 */
#ifndef	GDCPARSEDIR
#define	GDCPARSEDIR	_PATH_DUMPDIR
#endif	/* GDCPARSEDIR */

#ifndef	GDCPARSENAME
#define	GDCPARSENAME	"%s_parse"
#endif	/* GDCPARSENAME */

/*
 * Length of time in milliseconds to wait before doing a run check
 */
#ifndef	GDCMAXWAIT
#define	GDCMAXWAIT	250
#endif	/* GDCMAXWAIT */

/*
 * Maximum number of arguments we ever give gated.
 */
#define	GDCMAXARGS	10

/*
 * Options to make gated do a configuration file syntax check, and to
 * specify the name of the file to check.
 */
#ifndef	GDCGATEDCFLAG
#define	GDCGATEDCFLAG	"-C"
#endif	/* GDCGATEDCFLAG */

#ifndef	GDCGATEDFFLAG
#define	GDCGATEDFFLAG	"-f"
#endif	/* GDCGATEDFFLAG */

/*
 * Sanity.  We don't believe gated will ever run with a pid less than this.
 */
#define	GDCMINPID	3

/*
 * Resource limits.  We may have some defaults from the configuration file.
 */
#ifdef	GDC_DOING_RESOURCES

#define	GDC_SET_CORESIZE	0x01
#define	GDC_SET_DATASIZE	0x02
#define	GDC_SET_STACKSIZE	0x04
#define	GDC_SET_FILESIZE	0x08

static int gdc_rflags =
#ifdef	GDC_CORESIZE
		GDC_SET_CORESIZE |
#endif	/* GDC_CORESIZE */
#ifdef	GDC_DATASIZE
		GDC_SET_DATASIZE |
#endif	/* GDC_DATASIZE */
#ifdef	GDC_STACKSIZE
		GDC_SET_STACKSIZE |
#endif	/* GDC_STACKSIZE */
#ifdef	GDC_FILESIZE
		GDC_SET_FILESIZE |
#endif	/* GDC_FILESIZE */
		0 ;

#ifdef	GDC_CORESIZE
static int gdc_coresize = GDC_CORESIZE;
#else	/* GDC_CORESIZE */
static int gdc_coresize = 0;
#endif	/* GDC_CORESIZE */
#ifdef	GDC_DATASIZE
static int gdc_datasize = GDC_DATASIZE;
#else	/* GDC_DATASIZE */
static int gdc_datasize = 0;
#endif	/* GDC_DATASIZE */
#ifdef	GDC_STACKSIZE
static int gdc_stacksize = GDC_STACKSIZE;
#else	/* GDC_STACKSIZE */
static int gdc_stacksize = 0;
#endif	/* GDC_STACKSIZE */
#ifdef	GDC_FILESIZE
static int gdc_filesize = GDC_FILESIZE;
#else	/* GDC_FILESIZE */
static int gdc_filesize = 0;
#endif	/* GDC_FILESIZE */

#endif	/* GDC_DOING_RESOURCES */

/* Rules to build a clean environment for gated */
static const char *gatedenv[] = {
    "HOME",	"/",
    "SHELL",	"/bin/sh",
    "TERM",	"dumb",
    "USER",	"root",
#ifdef	GDC_TZ_HACK
    "TZ",	NULL,
#endif	/* GDC_TZ_HACK */
    NULL
};
static char * const *gated_envp;

/*
 * Routines which handle commands
 */
PROTOTYPE(gdstop, int, (const int));
PROTOTYPE(gdstart, int, (const int));
PROTOTYPE(gdrestart, int, (const int));
PROTOTYPE(gdsig, int, (const int));
PROTOTYPE(gdcheck, int, (const int));
PROTOTYPE(gdrunning, int, (const int));
PROTOTYPE(gdconfig, int, (const int));
PROTOTYPE(gdrm, int, (const int));
#if	CONFIG_MODE != 0
PROTOTYPE(gdmode, int, (const int));
PROTOTYPE(gdcreate, int, (const int));
#endif	/* CONFIG_MODE != 0 */

/*
 * How to test if a pid is running
 */
#define	isrunning(pid)	sendsignal((pid), 0)

/*
 * Command flags.  Only one, to indicate commands which need the group
 */
#define	CF_GRP		0x1

/*
 * Command table.
 */
struct gdcommand {
    const char *cmdname;
    const char *cmddesc;
    _PROTOTYPE(cmdrtn, int, (const int));
    const int cmdarg;
    const int cmdflags;
} gdcmds[] = {
    { "backout", "back out to the previous conf file",	gdconfig, 1, 0 },
    { "BACKOUT", "back out without questions",		gdconfig, 2, 0 },
    { "checkconf", "check the current conf file for syntax errors", gdcheck, 0, 0},
    { "checknew", "check the new conf file for syntax errors", gdcheck, 1, 0 },
    { "COREDUMP",	"make gated dump core",		gdsig, SIGABRT, 0 },
#if	CONFIG_MODE != 0
    { "createconf", "create a new conf file",		gdcreate, 0, CF_GRP },
#endif	/* CONFIG_MODE != 0 */
    { "dump",	"signal gated to dump its state",	gdsig, SIGINT, 0 },
    { "interface", "signal gated to check for interface changes", gdsig, SIGUSR2, 0 },
    { "KILL",	"kill gated ungracefully",		gdsig, SIGKILL, 0 },
#if	CONFIG_MODE != 0
    { "modeconf", "clean up conf file modes",		gdmode, 0, CF_GRP },
#endif	/* CONFIG_MODE != 0 */
    { "newconf", "rotate the new conf file into place",	gdconfig, 0, 0 },
    { "reconfig", "signal gated to reread the conf file", gdsig, SIGHUP, 0 },
    { "restart",	"stop and then start gated",	gdrestart, 0, 0 },
    { "rmcore",	"remove existing gated core file",	gdrm, 1, 0 },
    { "rmdump",	"remove existing gated dump file",	gdrm, 0, 0 },
    { "rmparse", "remove existing gated parse error file", gdrm, 2, 0 },
    { "running", "determine whether gated is running",	gdrunning, 0, 0 },
    { "start",	"start gated",				gdstart, 0, 0 },
    { "stop",	"signal gated until it stops",		gdstop,	0, 0 },
    { "term",	"send a terminate signal to gated",	gdsig, SIGTERM, 0 },
    { "toggletrace", "toggle tracing on/off",		gdsig, SIGUSR1, 0 },
    { NULL,	NULL,					NULL, 0 }
};

/*
 * Strings we use.  Some of these are created on the fly, some are
 * just pointers to #define'd stuff.
 */
const char *gdcprogname = NAME_GDC;
const char *gated = NAME_GATED;

char pidfile[MAXPATHLEN+1];		/* path to pid file */
char dumpfile[MAXPATHLEN+1];		/* path to gated dump file */
char gatedbinary[MAXPATHLEN+1];		/* path to gated binary */
char conffile[MAXPATHLEN+1];		/* path to gated config file */
char newconffile[MAXPATHLEN+1];		/* path to "+" config file */
char oldconffile[MAXPATHLEN+1];		/* path to "-" config file */
char realoldconffile[MAXPATHLEN+1];	/* path to "--" config file */
char corefile[MAXPATHLEN+1];		/* path to core file */
char parsefile[MAXPATHLEN+1];		/* place to write parse check errs */

/*
 * Buffer for writing error messages
 */
char errbuf[MAXPATHLEN*3];

/*
 * Stuff we figure out on the fly
 */
int userokay = 0;	/* is user okay to do this? */
const char *username;	/* name of user running program */
GID_T gdmgid;		/* group ID for good guys */

/*
 * Options
 */
int debug = 0;		/* debugging on */
int quiet = 0;		/* run quietly, only exit status matters */
int noinstall = 0;	/* run without modifying kernel routing table */
int timewait = GDC_WAIT;	/* time to wait for gated to terminate/start */

/*
 * Forward declarations
 */
PROTOTYPE(getcmd, struct gdcommand *, (char *));
PROTOTYPE(checkoutuser, int, (struct gdcommand *));
PROTOTYPE(checkprogname, void, (char *));
PROTOTYPE(makenames, void, (void));

char *progname;

/*
 * errprint - print an error message, paying attention to the quiet flag
 */
static void
errprint __PF1(errmsg, const char *)
{
    if (!quiet) {
	(void) fprintf(stderr,
		       "%s: %s\n",
		       gdcprogname,
		       errmsg);
    } else {
	syslog(LOG_ERR, errmsg);
    }
}

/*
 * Perror - print an error about a system call failure
 */
static void
Perror __PF2(errmsg, const char *,
	     errarg, char *)
{
    char buf[sizeof(errbuf) + 200];
    int error = errno;

    (void) sprintf(buf,
		   errmsg,
		   errarg);
    if (!quiet) {
	(void) fprintf(stderr,
		       "%s: %s: %s",
		       gdcprogname,
		       buf,
		       strerror(error));
    } else {
	syslog(LOG_ERR,
	       	"%s: %s: %m",
		gdcprogname,
	       	buf);
    }
}


static void
makeenv __PF0(void)
{
    register char **ep;
    const char **cp = gatedenv;

    /* Calculate size needed */
    for (cp = gatedenv; *cp; cp += 2) ;

    gated_envp = ep = (char **) (void_t) calloc((size_t) ((cp - gatedenv) >> 1) + 1, sizeof (*gatedenv));

    for (cp = gatedenv; *cp; cp += 2, ep++) {
	const char *vp = *(cp + 1);

	if (!vp) {
	    vp = getenv(*cp);
	}

	if (vp) {
	    *ep = (char *) malloc((size_t) (strlen(*cp) + strlen(vp) + 2));

	    strcpy(*ep, *cp);
	    strcat(*ep, "=");
	    strcat(*ep, vp);
	}
    }

}


/*
 * main - parse arguments and handle options
 */
int
main __PF2(argc, int,
	   argv, char **)
{
    int c;
    int errflg = 0;
    struct gdcommand *docmd = (struct gdcommand *) 0;

    progname = argv[0];
#ifdef	GDC_DOING_RESOURCES
    while ((c = getopt(argc, argv, "c:df:m:ns:qt:")) != EOF) {
#else	/* GDC_DOING_RESOURCES */
    while ((c = getopt(argc, argv, "dnqt:")) != EOF) {
#endif	/* GDC_DOING_RESOURCES */
	switch (c) {
	case 'd':
	    ++debug;
	    break;
	case 'q':
	    ++quiet;
	    break;
	case 'n':
	    ++noinstall;
	    break;
	case 't':
	    timewait = atoi(optarg);
	    if (timewait == 0) {
		(void) fprintf(stderr,
			       "%s: invalid wait time: %s\n",
			       progname,
			       optarg);
		errflg++;
	    }
	    break;
#ifdef	GDC_DOING_RESOURCES
	case 'c':
	    BIT_SET(gdc_rflags, GDC_SET_CORESIZE);
	    gdc_coresize = atoi(optarg);
	    break;
	case 'f':
	    BIT_SET(gdc_rflags, GDC_SET_FILESIZE);
	    gdc_filesize = atoi(optarg);
	    break;
	case 'm':
	    BIT_SET(gdc_rflags, GDC_SET_DATASIZE);
	    gdc_datasize = atoi(optarg);
	    break;
	case 's':
	    BIT_SET(gdc_rflags, GDC_SET_STACKSIZE);
	    gdc_stacksize = atoi(optarg);
	    break;
#endif	/* GDC_DOING_RESOURCES */
	default:
	    errflg++;
	    break;
	}
    }
    if (!errflg
	&& (optind != argc-1 || (docmd = getcmd(argv[optind])) == NULL)) {
	errflg++;
    }
    if (errflg) {
	setvbuf(stderr, NULL, _IOLBF, 0);
	(void) fprintf(stderr,
#ifdef	GDC_DOING_RESOURCES
		       "Usage: `%s [-dnq] [-c|f|m|s <size>] [-t time] cmd', where `cmd' is one of:\n",
#else	/* GDC_DOING_RESOURCES */
		       "Usage: `%s [-dnq] [-t time] cmd', where `cmd' is one of:\n",
#endif	/* GDC_DOING_RESOURCES */
		       gdcprogname);
	for (c = 0; gdcmds[c].cmdname != NULL; ++c) {
	    (void) fprintf(stderr,
			   "    %s\t%s\n",
			   gdcmds[c].cmdname,
			   gdcmds[c].cmddesc);
	}
	exit(2);
    }

#ifdef	LOG_DAEMON
    openlog(gdcprogname, LOG_NOWAIT, LOG_FACILITY);
#else	/* LOG_FACILITY */
#ifdef	LOG_NOWAIT
    openlog(gdcprogname, LOG_NOWAIT);
#else	/* LOG_NOWAIT */
    openlog(gdcprogname, 0);
#endif	/* LOG_NOWAIT */
#endif	/* LOG_DAEMON */

    userokay = checkoutuser(docmd);
    if (!userokay) {
	errno = EACCES;
	perror("");
	exit(1);		/* ignore plebes */
    }
    checkprogname(progname);
    makenames();
    makeenv();
    syslog(LOG_WARNING,
	   "performing a \"gdc %s\" command for user %s",
	   docmd->cmdname,
	   username);


    return docmd->cmdrtn(docmd->cmdarg);
}

/*
 * makenames - make names of things we may need to know
 */
void
makenames __PF0(void)
{
    register char *cp;

    /*
     * Make up configuration file names
     */
    (void) sprintf(conffile,
		   _PATH_CONFIG,
		   gated);
    (void) strcpy(newconffile, conffile);
    (void) strcpy(oldconffile, conffile);
    (void) strcpy(realoldconffile, conffile);
    (void) strcat(newconffile, GDCNEWSUFFIX);
    (void) strcat(oldconffile, GDCOLDSUFFIX);
    (void) strcat(realoldconffile, GDCREALOLDSUFFIX);

    /*
     * Make the dump and pid files.  Just need to know gated's name
     */
    (void) sprintf(dumpfile,
		   _PATH_DUMP,
		   gated);
    (void) sprintf(pidfile,
		   _PATH_PID,
		   gated);

    /*
     * The core file ends up in the dump directory.  Make up its name
     */
    (void) strcpy(corefile, _PATH_DUMPDIR);
    cp = &corefile[strlen(corefile)];
    *cp++ = '/';
    (void) sprintf(cp,
		   GDCCORENAME,
		   gated);

    /*
     * The gated binary lives in another directory.  Make it up the same way.
     */
    (void) strcpy(gatedbinary, _PATH_SBINDIR);
    cp = &gatedbinary[strlen(gatedbinary)];
    *cp++ = '/';
    (void) strcpy(cp, gated);

    /*
     * We know the parse file's directory and can construct a name for it.
     * Do so now.
     */
    (void) strcpy(parsefile, GDCPARSEDIR);
    cp = &parsefile[strlen(parsefile)];
    *cp++ = '/';
    (void) sprintf(cp,
		   GDCPARSENAME,
		   gated);
}


/*
 * getcmd - return the command corresponding to the argument string
 */
struct gdcommand *
getcmd __PF1(cmd, char *)
{
    register struct gdcommand *gdc;

    for (gdc = gdcmds; gdc->cmdname != NULL; gdc++) {
	if (STREQ(cmd, gdc->cmdname)) {
	    return gdc;
	}
    }
    return NULL;
}


/*
 * checkprogname - check the program name we were run with
 */
void
checkprogname __PF1(name, char *)
{
    register char *cp = name;
    int nlen, gdlen;
    int err = 1;

    /*
     * The last element in the path in argv[0] is compared against
     * the required program name.  This check is grossly inadequate,
     * but prevents the easiest method of running the program invisibly
     * to the accounting (with the shell).
     */
    nlen = strlen(name);
    gdlen = strlen(gdcprogname);

    if (nlen == gdlen) {
	cp = name;
	err = 0;
    } else if (nlen > gdlen) {
	cp += nlen-gdlen;
	if (*(cp-1) == '/') {
	    err = 0;
	}
    }

    if (!err && STREQ(cp, gdcprogname)) {
	return;
    }
	
    (void) sprintf(errbuf,
		   "must run progam with name of %s",
		   gdcprogname);
    errprint(errbuf);
    /* Complain loudly, at least until holes are fixed */
    syslog(LOG_ALERT,
	   "user %s attempted to run %s with name %s",
	   username,
	   gdcprogname,
	   name);
    errno = EACCES;
    perror("");
    exit(1);
}


/*
 * checkoutuser - see if this user is one of the privileged few
 */
int
checkoutuser __PF1(cmd, struct gdcommand *)
{
    register int i;
    struct passwd *pw;
    struct group *gr;
    uid_t uid;
    int ngroups;
    GID_T gidset[GDC_GROUPS_MAX];

    uid = getuid();
    if ((gr = getgrnam(GDC_GROUP)) == NULL) {
	/*
	 * Super gross hack.  A side effect of this routine
	 * is to set gdmgid to the proper gid.  If root
	 * is running this, however, we don't necessarily
	 * want to prevent him from doing things even if the
	 * group doesn't exist, except that he can't do commands
	 * command if we don't know the group.  Sigh.
	 */
	if (uid == 0) {
	    if (cmd->cmdflags & CF_GRP) {
		(void) sprintf(errbuf,
			       "the \"%s\" command requires group %s to exist",
			       cmd->cmdname,
			       GDC_GROUP);
		errprint(errbuf);
		return 0;
	    }
	} else {
	    return 0;
	}
    } else {
	gdmgid = gr->gr_gid;
    }

    if (uid == 0) {
	username = "root";
	/*
	 * Let root do what he wants.
	 */
	return 1;
    }

    if ((pw = getpwuid(uid)) == NULL) {
	(void) sprintf(errbuf,
		       "username for your uid (%d) unknown",
		       uid);
	errprint(errbuf);
	syslog(LOG_ALERT,
	       "unknown user with uid %d tried to run %s",
	       uid,
	       gdcprogname);
	return (0);
    }
    username = pw->pw_name;

    if (gdmgid == getgid()) {
	goto okay;	/* okay */
    }

    if ((ngroups = getgroups((int) (sizeof(gidset)/sizeof(gidset[0])), gidset)) == -1) {
	Perror("getgroups() failed", (char *)NULL);
	exit(1);
    }

    for (i = 0; i < ngroups; i++) {
	if (gdmgid == gidset[i]) {
	    goto okay;
	}
    }

    (void) setuid(uid);
    syslog(LOG_ALERT,
	   "user %s attempted to run %s",
	   username,
	   gdcprogname);
    errno = EACCES;
    perror("");
    return 0;

okay:
    if (setuid(0) == (-1)) {
	(void) sprintf(errbuf,
		       "permission denied: make %s setuid",
		       gdcprogname);
	errprint(errbuf);
	exit (1);
    }
    return 1;
}


/*
 * getmodesize - determine if a file exists, if it is a regular file, and
 *		 its size.
 */
static int
getmodesize __PF4(name, char *,
		  nofollow, int,
		  size, off_t *,
		  isreg, int *)
{
    struct stat sbuf;
    int res;

#if	defined(S_ISLNK) || defined(S_IFLNK)
    if (nofollow) {
	res = lstat(name, &sbuf);
    } else
#endif	/* defined(S_ISLNK) || defined(S_IFLNK) */
	res = stat(name, &sbuf);

    if (res == (-1)) {
	if (errno == ENOENT) {
	    /*
	     * File doesn't exist, gated probably not running
	     */
	    return (0);
	}
	Perror("stat(%s) failed",
	       name);
	exit(1);
    }

    if (isreg != NULL) {
#ifdef	S_ISREG
	if (S_ISREG(sbuf.st_mode)) {
#else
	if ((sbuf.st_mode & S_IFMT) == S_IFREG) {
#endif	/* S_ISREG */
	    *isreg = 1;
#if	defined(S_ISLNK) || defined(S_IFLNK)
	} else if (nofollow &&
#ifdef	S_ISLNK
	  S_ISLNK(sbuf.st_mode)) {
#else
	  (sbuf.st_mode & S_IFMT) == S_IFREG) {
#endif	/* S_ISLNK */
	    *isreg = 2;
	    if (size != NULL) {
		*size = 0;
		size = NULL;
	    }
#endif	/* defined(S_ISLNK) || defined(S_IFLNK) */
	} else {
	    *isreg = 0;
	}
    }

    if (size != NULL) {
	*size = sbuf.st_size;
    }

    return (1);
}

/*
 * White space, from our point of view
 */
#define	ISSPACE(c)	((c) == ' ' || (c) == '\t')

/*
 * getrunningpid - see if gated is running and, if so, fetch the pid.
 */
static int
getrunningpid __PF2(pid, PID_T *,
		    startup, int)
{
    register char *cp;
    int p, n;
    char buf[50];
    int isreg;
    off_t size;
    int fd;

    /*
     * See if file exists.  If not gated probably isn't running.  If
     * it isn't a regular file just forget about it.
     */
    if (!getmodesize(pidfile, 1, &size, &isreg)) {
	return (0);
    }
    if (isreg != 1) {
	(void) sprintf(errbuf,
		       "%s is not a regular file!",
		       pidfile);
	errprint(errbuf);
	exit(1);
    }

    /* We don't intend to write the file, but if the following call */
    /* to flock() is actually our emulation using lockf() we need to */
    /* have the file opened for writing. */
    if ((fd = open(pidfile, O_RDWR, 0664)) == -1) {
	Perror("open(%s) failed",
	       pidfile);
	exit(1);
    }

    /*
     * Try to get a lock on the file.  If we get it, gated isn't running.
     */
    if (flock(fd, LOCK_EX|LOCK_NB) != (-1)) {
	(void) close(fd);
	return 0;
    }
    if (errno != EWOULDBLOCK) {
	Perror("flock(%s) failed",
	       pidfile);
	exit(1);
    }
    if (pid == NULL) {
	(void) close(fd);
	return 1;
    }

    /*
     * Try to read the pid out of the file.  We assume the pid looks like
     *
     *	[ \t]*[0-9][0-9]*[ \t\n\0]
     *
     * Anything else loses
     */
    n = read(fd, buf, (sizeof(buf) - 1));
    if (n == -1) {
	Perror("read(%s) failed",
	       pidfile);
	exit(1);
    }
    (void) close(fd);
    buf[n] = '\0';

    for (cp = buf; ISSPACE(*cp); cp++) {
	/* nothing */
    }

    p = 0;
    while (*cp != '\n' && *cp != '\0' && !ISSPACE(*cp)) {
	if (!isdigit(*cp)) {
	    if (startup) {
		return 0;
	    }
	    (void) sprintf(errbuf,
			   "pid file %s mangled!",
			   pidfile);
	    errprint(errbuf);
	    exit(1);
	}
	p *= 10;
	p += (PID_T)(*cp++ - '0');
    }

    if (p < GDCMINPID) {
	if (startup) {
	    return 0;
	}
	(void) sprintf(errbuf,
		       "pid in file %s unreasonably small (%d)",
		       pidfile,
		       p);
	errprint(errbuf);
	exit(1);
    }

    *pid = p;
    return 1;
}


#ifdef	GDC_DOING_RESOURCES
/*
 * dorlimits - set any limits he has told us need changing
 */
static int
dorlimits __PF0(void)
{
    int i, res;
    int changed = 0;
    char buf[20];
    struct rlimit rlim;
    static const struct {
	int gdflag;
	int limit;
	int *gdclimit;
	const char *desc;
    } gdlimits[] = {
	{ GDC_SET_CORESIZE,	RLIMIT_CORE,	&gdc_coresize,	"core size" },
	{ GDC_SET_DATASIZE,	RLIMIT_DATA,	&gdc_datasize,	"data size" },
	{ GDC_SET_STACKSIZE,	RLIMIT_STACK,	&gdc_stacksize,	"stack size" },
	{ GDC_SET_FILESIZE,	RLIMIT_FSIZE,	&gdc_filesize,	"file size" },
	{ 0,			0,		(int *) 0, (const char *) 0 }
    };

    if (gdc_rflags == 0) {
	return 0;
    }
    i = 0;
    do {
	if (gdc_rflags & gdlimits[i].gdflag) {
	    res = getrlimit(gdlimits[i].limit, &rlim);
	    if (res < 0) {
		strcpy(buf, gdlimits[i].desc);
		Perror("getrlimit(%s) failed", buf);
		return 1;
	    }

	    if (rlim.rlim_max < *(gdlimits[i].gdclimit)) {
		rlim.rlim_max = rlim.rlim_cur = *(gdlimits[i].gdclimit);
		changed = 1;
	    } else if (rlim.rlim_cur < *(gdlimits[i].gdclimit)) {
		rlim.rlim_cur = *(gdlimits[i].gdclimit);
		changed = 1;
	    }
	    if (changed) {
		res = setrlimit(gdlimits[i].limit, &rlim);
		if (res < 0) {
		    strcpy(buf, gdlimits[i].desc);
		    Perror("setrlimit(%s) failed", buf);
		    return 1;
		}
	    }
	}
    } while (gdlimits[++i].gdflag != 0);

    return 0;
}
#endif	/* GDC_DOING_RESOURCES */


/*
 * sendsignal - send a signal to a pid, die if we can't
 */
static int
sendsignal __PF2(pid, PID_T,
		 sig, int)
{
    if (kill(pid, sig) == (-1)) {
	if (errno != ESRCH) {
	    (void) sprintf(errbuf,
			   "%u",
			   pid);
	    Perror("kill(%s) failed",
		   errbuf);
	    exit(1);
	}
	return 0;
    }
    return 1;
}


/*
 * gdsig - deliver a signal to gated
 */
int
gdsig __PF1(val, const int)
{
    PID_T pid;

    if (!getrunningpid(&pid, 0)) {
	errprint("gated is not running");
	return 1;
    }

    if (!sendsignal(pid, val)) {
	(void) sprintf(errbuf,
		       "gated appears to be running as pid %u, but pid %u doesn't exist!",
		       pid,
		       pid);
	errprint(errbuf);
	exit (1);
    }

    return 0;
}


/*
 * gdrunning - determine whether gated is running or not
 */
int
gdrunning __PF1(val, const int)
{
    PID_T pid;

    if (!getrunningpid(&pid, 0)) {
	if (!quiet) {
	    (void) printf("gated is not running\n");
	}
	return 1;
    }

    if (!isrunning(pid)) {
	(void) sprintf(errbuf,
		       "gated appears to be running as pid %u, but pid %u doesn't exist!",
		       pid,
		       pid);
	errprint(errbuf);
	return 1;
    }

    if (!quiet) {
	(void) printf("gated is running (pid %lu)\n",
		      (u_long) pid);
    }
    return 0;
}


/*
 * nap - sleep for a few milliseconds
 */
static void
nap __PF1(ms, int)
{
    struct timeval tv;

    tv.tv_sec = (long)(ms / 1000);
    tv.tv_usec = (long)(ms % 1000) * 1000;
    (void) select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv);
}


/*
 * gdstart - start gated
 */
int
gdstart __PF1(val, const int)
{
    PID_T pid;
    int isreg;
    int status;
    PID_T wpid;
    int i;

    /*
     * See if it is running already
     */
    if (getrunningpid(&pid, 0)) {
	(void) sprintf(errbuf,
		       "gated already running (pid %u)",
		       pid);
	errprint(errbuf);
	return 1;
    }

    /*
     * Not running, see if the binary exists
     */
    if (!getmodesize(gatedbinary, 0, (off_t *)0, &isreg)) {
	(void) sprintf(errbuf,
		       "gated binary %s not found",
		       gatedbinary);
	errprint(errbuf);
	return 1;
    }
    if (!isreg) {
	(void) sprintf(errbuf,
		       "gated binary %s not regular file!",
		       gatedbinary);
	errprint(errbuf);
	return 1;
    }

#ifdef	GDC_DOING_RESOURCES
    /*
     * If we have to set some resource limits, do it here before
     * the fork since errors will be handled more gracefully.
     */
    if (gdc_rflags) {
	if (dorlimits()) {
	    return 1;
	}
    }
#endif	/* GDC_DOING_RESOURCES */

    /*
     * Attempt to run binary
     */
    if ((pid = fork()) == (-1)) {
	Perror("can't fork", (char *)0);
	exit(1);
    } else if (pid == 0) {
	union {
	    char * const wrong[3];
	    const char *right[3];
	} args;	/* Thanks POSIX */
	const char **ap = args.right;

	*ap++ = gatedbinary;
	if (noinstall) {
	    *ap++ = "-n";
	}
	*ap++ = (char *) 0;
	execve(gatedbinary, args.wrong, gated_envp);
	Perror("can't exec %s",
	       gatedbinary);
	exit(254);
    }

    for (;;) {
	wpid = wait(&status);
	if (wpid == (-1)) {
	    Perror("wait for gated startup failed", (char *)0);
	    exit(1);
	}
	if (wpid == pid) {
	    break;
	}
	(void) sprintf(errbuf,
		       "spurious exit pid %u status 0x%04x",
		       wpid,
		       status);
	errprint(errbuf);
    }

    if ((status & 0x7f) == 0x7f) {
	(void) sprintf(errbuf,
		       "child process %u stopped",
		       pid);
	errprint(errbuf);
	return 1;
    } else if ((status & 0x7f) != 0) {
	(void) sprintf(errbuf,
		       "gated process %u terminated on signal %u%s",
		       pid,
		       (status & 0x7f),
		       ((status & 0x80) ? " with core" : ""));
	errprint(errbuf);
	return 1;
    }

    status >>= 8;
    status &= 0xff;

    if (status != 0) {
	if (status != 254) {
	    (void) sprintf(errbuf,
			   "gated process %u exitted with status %d",
			   pid,
			   status);
	}
	return 1;
    }

    /*
     * Nap for a fraction of a second, then check to see if gated is running.
     */
    nap(350);
    for (i = (timewait * (1000/GDCMAXWAIT)); i > 0; i--) {
	nap(GDCMAXWAIT);
	if (getrunningpid(&pid, 1)) {
	    if (!quiet) {
		(void) printf("gated started, pid %lu\n",
			      (u_long) pid);
	    }
	    return 0;
	}
    }

    (void) sprintf(errbuf,
		   "%s was started, but not found running",
		   gatedbinary);
    errprint(errbuf);
    return 1;
}


/*
 * waitforpid - wait for "n" milliseconds for a pid to exit
 */
static int
waitforpid __PF2(pid, PID_T,
		 msec, int)
{
    int msec_left;
    int msec_towait;

    if (!isrunning(pid)) {
	return 1;
    }

    msec_left = msec;
    while (msec_left > 0) {
	msec_towait = MIN(msec_left, GDCMAXWAIT);
	nap(msec_towait);
	msec_left -= msec_towait;
	if (!isrunning(pid)) {
	    return 1;
	}
    }
    return 0;
}


/*
 * gdstop - stop gated by any means necessary
 */
int
gdstop __PF1(val, const int)
{
    PID_T pid, npid;
    int sigs_sent;
    int seconds;

    /*
     * See if gated is running, determine its pid.
     */
    if (!getrunningpid(&pid, 0)) {
	errprint("gated doesn't seem to be running");
	return 1;
    }

    /*
     * We send 3 signals, each followed by a wait of timewait seconds.
     * The first two are SIGTERMs, the third is a SIGKILL.
     */
    for (sigs_sent = 0; sigs_sent < 3; sigs_sent++) {
	if (!sendsignal(pid, ((sigs_sent == 2) ? SIGKILL : SIGTERM))) {
	    goto stopped;
	}
	for (seconds = 1; seconds <= timewait; seconds++) {
	    if (waitforpid(pid, 1000)) {
		goto stopped;
	    }
	    if (!getrunningpid(&npid, 0)) {
		goto stopped;
	    }
	    if (npid != pid) {
		(void) sprintf(errbuf,
			       "gated restarted after termination: old pid %u, new pid %u",
			       pid,
			       npid);
		return 1;
	    }
	    if (!quiet && sigs_sent == 0 && seconds == 2 && timewait > 3) {
		(void) printf("gated signalled but still running, waiting %d seconds more\n",
			      (timewait - seconds));
	    }
	}
	if (!quiet && sigs_sent < 2) {
	    (void) printf("gated still running, sending %s signal\n",
			  ((sigs_sent == 0) ? "another terminate" : "a kill"));
	}
    }

    (void) sprintf(errbuf,
		   "gated pid %u failed to terminate",
		   pid);
    errprint(errbuf);
    return 1;

stopped:
    if (!quiet) {
	(void) printf("gated terminated\n");
    }
    return 0;
}


/*
 * gdrestart - stop gated if it is running, then start it
 */
int
gdrestart __PF1(val, const int)
{
    PID_T pid;

    if (getrunningpid(&pid, 0)) {
	if (!quiet) {
	    (void) printf("gated running, sending terminate signal\n");
	}
	if (gdstop(0)) {
	    return 1;
	}
    } else if (!quiet) {
	(void) printf("gated not currently running\n");
    }

    return gdstart(0);
}

/*
 * gdrm - remove dump or core file
 */
int
gdrm __PF1(val, const int)
{
    int isreg;
    char *file;

    if (val == 1) {
	file = corefile;
    } else if (val == 2) {
	file = parsefile;
    } else {
	file = dumpfile;
    }

    if (!getmodesize(file, 0, (off_t *)0, &isreg)) {
	(void) sprintf(errbuf,
		       "rm%s: %s not found",
		       (val ? (val == 2 ? "parse" : "core") : "dump"),
		       file);
	errprint(errbuf);
	return 1;
    }

    if (!isreg) {
	(void) sprintf(errbuf,
		       "rm%s: %s is not regular file!",
		       (val ? "core" : "dump"),
		       file);
	errprint(errbuf);
	return 1;
    }

    if (unlink(file) == (-1)) {
	Perror("unlink(%s) failed",
	       file);
	exit(1);
    }
    return 0;
}

/*
 * Configuration files
 */
static struct conffiles {
    char *file;
    int exists;
    off_t size;
} cf[] = {
    { newconffile, 0, 0 },
    { conffile, 0, 0 },
    { oldconffile, 0, 0 },
    { realoldconffile, 0, 0 }
};

#define	NCF	(sizeof(cf) / sizeof(cf[0]))

/*
 * Get the state of all configuration files
 */
static int
getconfstate __PF2(linkok, int,
		   string, const char *)
{
    int i;
    off_t size;
    int isreg;
    int errs = 0;

    for (i = 0; i < NCF; i++) {
	if (getmodesize(cf[i].file, 1, &size, &isreg)) {
	    if (!isreg) {
		(void) sprintf(errbuf,
			       "%s: %s is not a regular file!",
			       string,
			       cf[i].file);
		errprint(errbuf);
		errs++;
	    } else if (!linkok && isreg == 2) {
		/* nothing */
	    } else {
		cf[i].exists = 1;
		cf[i].size = size;
	    }
	}
    }

    if (errs) {
	return 1;
    }
    return 0;
}

/*
 * dorename - do a rename(), exit if it fails
 */
static void
dorename __PF2(namefrom, char *,
	       nameto, char *)
{
    if (rename(namefrom, nameto) == (-1)) {
	Perror("rename(%s) failed",
	       namefrom);
	exit(1);
    }
}

/*
 * rollforward - roll the specified conf file forward
 */
static void
rollforward __PF1(fn, int)
{
    /*
     * Check to see if the next guy exists and is non-zero sized.  If so
     * he'll need to be rolled too.
     */
    if (fn == 3) {
	if (unlink(realoldconffile) == (-1)) {
	    Perror("unlink(%s) failed",
		   realoldconffile);
	    exit(1);
	}
	return;
    }
    if (cf[fn+1].exists && cf[fn+1].size > 0) {
	rollforward(fn+1);
    }
    dorename(cf[fn].file, cf[fn+1].file);
}

/*
 * gdconfig - rotate the config files in one direction or another
 */
int
gdconfig __PF1(val, const int)
{
    /*
     * Scan the config files to find out their status
     */
    if (getconfstate(1,
      (val ? ((val == 2) ? "BACKOUT" : "backout") : "newconf"))) {
	return 1;
    }

    if (val == 0) {
	if (!cf[0].exists) {
	    (void) sprintf(errbuf,
			   "newconf: %s not found",
			   newconffile);
	    errprint(errbuf);
	    return 1;
	}
	if (cf[0].size == 0) {
	    (void) sprintf(errbuf,
			   "newconf: %s is zero length",
			   newconffile);
	    errprint(errbuf);
	    return 1;
	}
	rollforward(0);
    } else {
	if (!cf[2].exists || cf[2].size == 0) {
	    (void) sprintf(errbuf,
			   "backout: %s nonexistant or zero length",
			   oldconffile);
	    errprint(errbuf);
	    return 1;
	}
	if (val == 1 && cf[0].exists && cf[0].size > 0
	  && cf[1].exists && cf[1].size > 0) {
	    (void) sprintf(errbuf,
			   "backout: %s exists and is non-zero length, use BACKOUT instead",
			   newconffile);
	    errprint(errbuf);
	    return 1;
	}

	if (cf[1].exists && cf[1].size > 0) {
	    dorename(cf[1].file, cf[0].file);
	}
	dorename(cf[2].file, cf[1].file);
	if (cf[3].exists) {
	    dorename(cf[3].file, cf[2].file);
	}
    }

    return 0;
}


#if	CONFIG_MODE != 0
/*
 * gdmode - change the mode of the configuration files to 664, owner
 *	    root, group gdmaint.
 */
int
gdmode __PF1(val, const int)
{
    int i;
    int errs = 0;

    if (getconfstate(0, "modeconf")) {
	return 1;
    }

    for (i = 0; i < NCF; i++) {
	if (cf[i].exists) {
	    if (chmod(cf[i].file, CONFIG_MODE) == (-1)) {
		Perror("chmod(%s) failed",
		       cf[i].file);
		errs++;
	    }
	    if (chown(cf[i].file, GDCFILEUID, gdmgid) == (-1)) {
		Perror("chown(%s) failed",
		       cf[i].file);
		errs++;
	    }
	}
    }

    if (errs) {
	return 1;
    }
    return 0;
}


/*
 * gdcreate - create a zero-length "new" configuration file if none exists
 */
int
gdcreate __PF1(val, const int)
{
    off_t size;
    int isreg;
    int fd;

    if (getmodesize(newconffile, 1, &size, &isreg)) {
	(void) sprintf(errbuf,
		       "file %s exists already",
		       newconffile);
	errprint(errbuf);
	return 1;
    }
    (void) umask(002);
    if ((fd = open(newconffile, O_CREAT, 0664)) == (-1)) {
	Perror("open(%s) failed",
	       newconffile);
	return 1;
    }
    (void) close(fd);
    if (chown(newconffile, GDCFILEUID, gdmgid) == (-1)) {
	Perror("chown(%s) failed",
	       newconffile);
	return 1;
    }
    return 0;
}
#endif	/* CONFIG_MODE != 0 */


/*
 * gdcheck - syntax check (one of) the configuration file(s)
 */
int
gdcheck __PF1(val, const int)
{
    off_t size;
    off_t nsize;
    int isreg;
    char *cfile;
    int pexists;
    union {
	char * const wrong[GDCMAXARGS];
	const char *right[GDCMAXARGS];
    } gatedargs;	/* Thanks POSIX! */
    const char **ap = gatedargs.right;
    PID_T pid, wpid;
    WAIT_T status;

    /*
     * We're either syntax checking the current configuration file or
     * the new one.  Pick the one we like.
     */
    if (val == 0) {
	cfile = conffile;
    } else {
	cfile = newconffile;
    }

    /*
     * See if the file exists and is non-zero in size
     */
    if (!getmodesize(cfile, 0, &size, &isreg)) {
	(void) sprintf(errbuf,
		       "file %s not found",
		       cfile);
	errprint(errbuf);
	return 1;
    }
    if (!isreg) {
	(void) sprintf(errbuf,
		       "file %s not regular file",
		       cfile);
	errprint(errbuf);
	return 1;
    }
    if (size == 0) {
	(void) sprintf(errbuf,
		       "file %s is zero length",
		       cfile);
	errprint(errbuf);
	return 1;
    }

    /*
     * See if the parse file exists.  Don't let it be a symbolic link.
     */
    pexists = getmodesize(parsefile, 1, &size, &isreg);
    if (pexists) {
	if (isreg != 1) {
	    (void) sprintf(errbuf,
			   "parse file %s exists and is not regular file!",
			   parsefile);
	    syslog(LOG_ALERT, errbuf);
	    if (!quiet) {
		errprint(errbuf);
	    }
	    return 1;
	}
    } else {
	size = 0;
    }

    /*
     * Make sure we've also got a gated binary
     */
    if (!getmodesize(gatedbinary, 0, (off_t *)0, &isreg)) {
	(void) sprintf(errbuf,
		       "gated binary %s not found",
		       gatedbinary);
	errprint(errbuf);
	return 1;
    }
    if (!isreg) {
	(void) sprintf(errbuf,
		       "gated binary %s not regular file!",
		       gatedbinary);
	errprint(errbuf);
	return 1;
    }

    /*
     * So far so good.  Make up an argument list to run gated with.
     * Include the file name if we're testing the new configuration.
     */
    *ap++ = gatedbinary;	/* argv[0] /blah/gated */
    *ap++ = GDCGATEDCFLAG;	/* argv[1] -C */
    if (val) {
	*ap++ = GDCGATEDFFLAG;	/* argv[2] -f */
	*ap++ = cfile;		/* argv[3] /blah/gated.conf+ */
    }
    *ap++ = (char *)0;

    /*
     * Now fork.  In the child close the current stdout and stderr
     * and open the parse error file on the same descriptors.  Then
     * exec gated.  In the parent pick up the exit status and tell
     * the luser the results.
     */
    if ((pid = fork()) == (-1)) {
	Perror("can't fork",
	       (char *)0);
	exit(1);
    } else if (pid == 0) {
	int fd;

	if (!pexists) {
	    (void) umask(002);
	    if ((fd = open(parsefile, O_WRONLY|O_CREAT|O_APPEND|O_EXCL, 0664)) == -1) {
		Perror("open(%s) failed",
		       parsefile);
		_exit(254);
	    }
	    if (chown(parsefile, GDCFILEUID, gdmgid) == (-1)) {
		Perror("chown(%s) failed",
		       parsefile);
		_exit(254);
	    }
	} else {
	    if  ((fd = open(parsefile, O_WRONLY|O_APPEND, 0664)) == (-1)) {
		Perror("open(%s) failed",
		       parsefile);
		_exit(254);
	    }
	}
	if (fd != 1) {
	    if (dup2(fd, 1) == (-1)) {
		Perror("dup2(fd,1) failed",
		       (char *)0);
		_exit(254);
	    }
	}
	if (fd != 2) {
	    if (dup2(fd, 2) == (-1)) {
		Perror("dup2(fd,2) failed",
		       (char *)0);
		_exit(254);
	    }
	}
	if (fd != 1 && fd != 2) {
	    (void) close(fd);
	}

	(void) execve(gatedbinary, gatedargs.wrong, gated_envp);

	/*
	 * Can't complain here, just exit.
	 */
	_exit(253);
    }

    for (;;) {
	wpid = waitpid(-1, &status, 0);
	if (wpid == (-1)) {
	    Perror("wait for gated configuration check failed",
		   (char *)0);
	    exit(1);
	}
	if (wpid == pid) {
	    break;
	}
	(void) sprintf(errbuf,
		       "spurious exit pid %u status 0x%04x",
		       wpid,
		       status);
	errprint(errbuf);
    }

    if (WIFSTOPPED(status)) {
	(void) sprintf(errbuf,
		       "child process %u stopped",
		       pid);
	errprint(errbuf);
	return 1;
    } else if (WIFSIGNALED(status)) {
	(void) sprintf(errbuf,
		       "gated process %u terminated on signal %u%s",
		       pid,
		       WIFSIGNALED(status),
		       WIFCOREDUMP(status) ? " with core" : "");
	errprint(errbuf);
	goto dumpit;
    }

    switch (WEXITSTATUS(status)) {
    case 253:
	(void) sprintf(errbuf,
		       "exec(%s) failed",
		       gatedbinary);
	errprint(errbuf);
	break;

    case 254:
	break;

    case 0:
	(void) getmodesize(parsefile, 1, &nsize, &isreg);
	if (nsize != size) {
	    if (nsize > size) {
		if (quiet) {
		    syslog(LOG_ERR,
		    	   "configuration check on %s may have failed (broken yacc?), check %s%s to see",
			   cfile,
			   (pexists ? "end of " : ""),
			   parsefile);
		} else {
		    (void) printf("configuration check on %s may have failed (broken yacc?), check %s%s to see\n",
				  cfile,
				  (pexists ? "end of " : ""),
				  parsefile);
		}
	    } else {
		if (quiet) {
		    syslog(LOG_ERR,
			   "while checking %s the parse output file %s shrank!  Better check it",
			   cfile,
			   parsefile);
		} else {
		    (void) printf("while checking %s the parse output file %s shrank!  Better check it\n",
				  cfile,
				  parsefile);
		}
	    }
	    return 1;
	}
	if (!quiet) {
	    (void) printf("configuration file %s checks out okay\n",
			  cfile);
	}
	if (!pexists) {
	    (void) unlink(parsefile);
	}
	return 0;

    default:
	if (quiet) {
	    syslog(LOG_ERR,
		   "configuration check on %s fails, errors %s %s",
		   cfile,
		   (pexists ? "appended to" : "in"),
		   parsefile);
	} else {
	    (void) printf("check on %s fails, errors %s %s\n",
			  cfile,
			  (pexists ? "appended to" : "in"),
			  parsefile);
	}
	return 1;
    }

dumpit:
    if (!pexists) {
	(void) unlink(parsefile);
    }
    return 1;
}

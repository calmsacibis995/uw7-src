#ident	"@(#)general.c	1.2"

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident "$Header$"

/*
	generic interface to dfs commands.

	usage:	cmd [-F fstype] [-o fs_options] [ args ]

	exec's /usr/lib/fs/<fstype>/<cmd>
	<cmd> is the basename of the command.

	if -F is missing, fstype is the first entry in /etc/dfs/fstypes
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>

#define DFSTYPES	"/etc/dfs/fstypes"		/* dfs list */
#define	FSCMD		"/usr/lib/fs/%s/%s"
#define	NAME_MAX	64

#define	ARGVPAD		4	/* non-[arg...] elements in new argv list:
				   cmd name, -o, opts, (char *)0 terminator */

static char *getfs();
static char *nwgetfs();

main(argc, argv)
int argc;
char **argv;
{
	extern int getopt();
	char *malloc();
	static int invalid();
	extern char *optarg;
	extern int optind;
	FILE *dfp;		/* fp for dfs list */
	int c, err = 0; 
	char subcmd[BUFSIZ];	/* fs specific command */
	char *cmd;		/* basename of this command */
	char *fsname = NULL;	/* file system name */
	char *opts = NULL;	/* -o options */
	char **nargv;		/* new argv list */
	int nargc = 0;		/* new argc */
	char label[NAME_MAX];
	static char usage[] =
		":9:Usage: %s [-F fstype] [-o fs_options ] [arg ...]\n";

	cmd = strrchr(argv[0], '/');	/* find the basename */
	if (cmd)
		++cmd;
	else
		cmd = argv[0];

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxdfscmds");
	(void) sprintf(label, "UX:%s", cmd);
	(void) setlabel(label);

	while ((c = getopt(argc, argv, "F:o:")) != -1)
		switch (c) {
		case 'F':
			err |= (fsname != NULL);	/* at most one -F */
			fsname = optarg;
			break;
		case 'o':			/* fs specific options */
			err |= (opts != NULL);		/* at most one -o */
			opts = optarg;
			break;
		case '?':
			err = 1;
			break;
		}
	if (err) {
		pfmt(stderr, MM_ERROR, usage, cmd);
		exit(1);
	}

	if ((dfp = fopen(DFSTYPES, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":2:cannot open %s: %s\n",
		     DFSTYPES, strerror(errno));
		pfmt(stderr, MM_INFO, 
		     ":3:possible cause: NFS not installed\n");
		exit(1);
	}

	if (fsname) {		/* generate fs specific command name */
		if (invalid(fsname, dfp)) {	/* valid ? */
			pfmt(stderr, MM_ERROR, ":8:invalid file system name: %s\n", fsname);
			pfmt(stderr, MM_ACTION, usage, cmd);
			exit(1);
		}
		else
			(void)sprintf(subcmd, FSCMD, fsname, cmd);
	}
	else if (fsname = nwgetfs(dfp))		/* use 1st line in dfstypes */
		(void)sprintf(subcmd, FSCMD, fsname, cmd);
	else {
		pfmt(stderr, MM_ERROR, ":7:no file systems listed in %s\n", DFSTYPES);
		pfmt(stderr, MM_ACTION, usage, cmd);
		exit(1);
	}

	/* allocate a block for the new argv list */
	if (!(nargv = (char **)malloc(sizeof(char *)*(argc-optind+ARGVPAD)))) {
		pfmt(stderr, MM_ERROR, ":4:%s failed\n", "malloc");
		exit(1);
		/*NOTREACHED*/
	}
	nargv[nargc++] = cmd;
	if (opts) {
		nargv[nargc++] = "-o";
		nargv[nargc++] = opts;
	}
	for (; optind <= argc; ++optind)	/* this copies the last NULL */
		nargv[nargc++] = argv[optind];

	(void)execvp(subcmd, nargv);
	pfmt(stderr, MM_ERROR, ":5:%s failed: %s\n",
	     subcmd, strerror(errno));
	exit(1);
	/*NOTREACHED*/
}


/*
	invalid(name, f)  -  return non-zero if name is not in
			     the list of fs names in file f
*/

static int
invalid(name, f)
char *name;		/* file system name */
FILE *f;		/* file of list of systems */
{
	char *s;

	while (s = getfs(f))	/* while there's still hope ... */
		if (strcmp(s, name) == 0)
			return 0;	/* we got it! */
	return 1;
}


/*
   getfs(fp) - get the next file system name from fp
	       ignoring lines starting with a #.
	       All leading whitespace is discarded.
*/

static char buf[BUFSIZ];

static char *
getfs(fp)
FILE *fp;
{
	register char *s;

	while (s = fgets(buf, BUFSIZ, fp)) {
		while (isspace(*s))	/* leading whitespace doesn't count */
			++s;
		if (*s != '#') {	/* not a comment */
			char *t = s;

			while (!isspace(*t))	/* get the token */
				++t;
			*t = '\0';		/* ignore rest of line */
			return s;
		}
	}
	return NULL;	/* that's all, folks! */
}

/*
 * nwgetfs(fp) - skip nucfs when getting file system name from fp
 */
static char *
nwgetfs(fp)
FILE *fp;
{
	char *s;

	while (s = getfs(fp)) {
		if (strcmp(s, "nucfs") != 0)
			return(s);
	}
	return(NULL);
}

#ident	"@(#)unshare.c	1.2"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * nfs unshare
 */
#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include "sharetab.h"


/*
 * exit values
 */
#define RET_OK		0
#define RET_ERR		33	/* usage error */
#define RET_DEL		35	/* could not delete from sharetab */
#define RET_EXP		36	/* could not unshare */

void usage();

main(argc, argv)
	int argc;
	char **argv;
{

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:nfs unshare");

	if (argc != 2) {
		usage();
		exit(RET_ERR);
	}

	exit(do_unshare(argv[1]));
}

int
do_unshare(path)
	char *path;
{
	if (shared(path) < 0)
		return (RET_ERR);

	/*
	 * Only tell the kernel if there are no other shared resources
	 * that resolve to the same as this one by following symlinks
	 */
	if (c_link(path) >= 0)
		if (exportfs(path, NULL) < 0 && errno != ENOENT) {
			pfmt(stderr, MM_ERROR, ":33:%s failed for %s: %s\n",
			     "exportfs", path, strerror(errno));
			return (RET_EXP);
		}

	if (sharetab_del(path) < 0)
		return (RET_DEL);

	return (RET_OK);
}

int
shared(path)
	char *path;
{
	FILE *f;
	struct share *sh;
	int res;

	f = fopen(SHARETAB, "r");
	if (f == NULL) {
		pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n", 
		     "shared", SHARETAB, strerror(errno));
		return (-1);
	}

	while ((res = getshare(f, &sh)) > 0) {
		if (strcmp(path, sh->sh_path) == 0
		 && strcmp("nfs", sh->sh_fstype) == 0) {
			(void) fclose(f);
			return (1);
		}
	}
	if (res < 0) {
		pfmt(stderr, MM_ERROR, ":37:error reading %s\n", SHARETAB);
		(void) fclose(f);
		return (-1);
	}
	pfmt(stderr, MM_ERROR, ":45:%s is not shared\n", path);
	(void) fclose(f);
	return (-1);
}

/*
 * Remove an entry from the sharetab file.
 */
int
sharetab_del(path)
	char *path;
{
	FILE *f;

	f = fopen(SHARETAB, "r+");
	if (f == NULL) {
		pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n", 
		     "sharetab_del", SHARETAB, strerror(errno));
		return (-1);
	}
	if (lockf(fileno(f), F_LOCK, 0L) < 0) {
		pfmt(stderr, MM_ERROR, ":21:%s: cannot lock %s: %s\n", 
		     "sharetab_del", SHARETAB, strerror(errno));
		(void) fclose(f);
		return (-1);
	}
	if (remshare(f, path) < 0) {
		pfmt(stderr, MM_ERROR, ":33:%s failed on %s: %s\n", 
		     "remshare", path, strerror(errno));
		return (-1);
	}
	(void) fclose(f);
	return (0);
}

void
usage()
{
	pfmt(stderr, MM_ACTION, ":44:Usage: unshare pathname\n");
}

/*
 * Check if 'path' is either a symbolic link that
 * resolves to something else that is already shared or
 * if something else shared is a symbolic link to 'path'.
 * Return -1 if we find a symlink.
 * Return 0 on errors to be sure we unshare the resource.
 */
static int
c_link(path)
	char *path;
{
	char buf[BUFSIZ];
	int i, j;
	FILE *fp;
	struct share *sh;
	int res;

	if ((i = readlink(path, buf, BUFSIZ)) > 0) {
		/* check if 'path' is a symlink to any other entries in sharetab. */
		buf[i] = '\0';
		if ((fp = fopen(SHARETAB, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n", 
			     "c_link", SHARETAB, strerror(errno));
			return (0);
		}
		while ((res = getshare(fp, &sh)) > 0) {
			if (strcmp(buf, sh->sh_path) == 0 &&
			    strcmp("nfs", sh->sh_fstype) == 0 &&
			    strcmp(path, sh->sh_path) != 0) {
				fclose(fp);
				return (-1);
			}
		}
		if (res < 0) {
			pfmt(stderr, MM_ERROR, ":37:error reading %s\n", 
			     SHARETAB);
			fclose(fp);
			return (0);
		}
	} else {
		/* check if any entry in sharetab is a symlink to 'path' */
		if ((fp = fopen(SHARETAB, "r")) == NULL) {
			pfmt(stderr, MM_ERROR, ":20:%s: cannot open %s: %s\n", 
			     "c_link", SHARETAB, strerror(errno));
			return (0);
		}
		while ((res = getshare(fp, &sh)) > 0) {
			if ((j = readlink(sh->sh_path, buf, BUFSIZ)) > 0) {
				buf[j] = '\0';
				if (strcmp(path, buf) == 0 &&
			 	    strcmp("nfs", sh->sh_fstype) == 0 &&
				    strcmp(path, sh->sh_path) != 0) {
					fclose(fp);
					return (-1);
				}
			}
		}
		if (res < 0) {
			pfmt(stderr, MM_ERROR, ":37:error reading %s\n",
			     SHARETAB);
			fclose(fp);
			return (0);
		}
	}
	return(0);
}

/*		copyright	"%c%" 	*/

#ident	"@(#)gwd.c	1.2"
#ident "$Header$"

#include "uucp.h"
#include <sys/stat.h>

/*
 *	gwd - get working directory
 *	Uid, Gid, Euid, and Egid are global
 *	return
 *		0 - ok
 *	 	FAIL - failed
 */

int
gwd(wkdir)
char *wkdir;
{
	FILE *fp;
	char cmd[BUFSIZ];

	*wkdir = '\0';
	(void) sprintf(cmd, "%s pwd 2>&-", PATH);

	(void) seteuid(Uid);
	(void) setegid(Gid);
	fp = popen(cmd, "r");
	(void) seteuid(Euid);
	(void) setegid(Egid);

	if (fp == NULL)
		return(FAIL);

	if (fgets(wkdir, MAXFULLNAME, fp) == NULL) {
		(void) pclose(fp);
		return(FAIL);
	}
	if (wkdir[strlen(wkdir)-1] == '\n')
		wkdir[strlen(wkdir)-1] = '\0';
	(void) pclose(fp);
	return(0);
}


/*
 * uidstat(file, &statbuf)
 * This is a stat call with the [ug]id set from effective to real.
 * Used from uucp.c and uux.c to permit file copies
 * from directories that may not be searchable by other.
 * return:
 *	same as stat()
 */

int
uidstat(file, buf)
char *file;
struct stat *buf;
{
	register ret;

	(void) seteuid(Uid);
	(void) setegid(Gid);
	ret = stat(file, buf);
	(void) seteuid(Euid);
	(void) setegid(Egid);
	return(ret);
}

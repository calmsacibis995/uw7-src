/*		copyright	"%c%" 	*/

#ident	"@(#)gnamef.c	1.2"
#ident "$Header$"

#include "uucp.h"
#include <sys/stat.h>

/*
 * get next file name from directory
 *	p	 -> file description of directory file to read
 *	filename -> address of buffer to return filename in
 *		    must be of size MAXBASENAME+1
 * returns:
 *	FALSE	-> end of directory read
 *	TRUE	-> returned name
 */
int
gnamef(p, filename)
register char *filename;
DIR *p;
{
	struct dirent dentry;
	register struct dirent *dp = &dentry;

	for (;;) {
		if ((dp = readdir(p)) == NULL)
			return(FALSE);
		if (dp->d_ino != 0 && dp->d_name[0] != '.')
			break;
	}

	(void) strncpy(filename, dp->d_name, MAXBASENAME);
	filename[MAXBASENAME] = '\0';
	return(TRUE);
}

/*
 * get next directory name from directory
 *	p	 -> file description of directory file to read
 *	filename -> address of buffer to return filename in
 *		    must be of size MAXBASENAME+1
 * returns:
 *	FALSE	-> end of directory read
 *	TRUE	-> returned dir
 */
int
gdirf(p, filename, dir)
register char *filename;
DIR *p;
char *dir;
{
	char statname[MAXNAMESIZE];

	for (;;) {
		if(gnamef(p, filename) == FALSE)
			return(FALSE);
		(void) sprintf(statname, "%s/%s", dir, filename);
		DEBUG(4, "stat %s\n", statname);
		if (DIRECTORY(statname))
		    break;
	}

	return(TRUE);
}

/*		copyright	"%c%" 	*/

#ident	"@(#)expfile.c	1.2"
#ident "$Header$"

#include "uucp.h"
#include <pwd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

/*
 * expand file name expansion is based on first characters
 *	/	-> fully qualified pathname. no
 *		   processing necessary
 *	~	-> prepended with login directory
 *	~/	-> prepended with Pubdir
 *	default	-> prepended with current directory
 *	file	-> filename to expand
 * returns: 
 *	0	-> ok
 *      FAIL	-> no Wrkdir name available
 */
int
expfile(file)
register char *file;
{
	register char *fpart, *up;
	uid_t uid;
	char user[NAMESIZE], save[MAXFULLNAME];
	extern int gninfo(), canPath();

	strcpy(save, file);
	if (*file != '/')
	    if (*file ==  '~') {
		/* find / and copy user part */
		for (fpart = save + 1, up = user; *fpart != '\0'
			&& *fpart != '/'; fpart++)
				*up++ = *fpart;
		*up = '\0';
		if ((user[0]=='\0') || (gninfo(user, &uid, file) != 0)){
			(void) strcpy(file, Pubdir);
		}
		(void) strcat(file, fpart);
	    } else {
		(void) sprintf(file, "%s/%s", Wrkdir, save);
		if (Wrkdir[0] == '\0')
			return(FAIL);
	    }

	if (canPath(file) != 0) { /* I don't think this will ever fail */
	    (void) strcpy(file, CORRUPTDIR);
	    return(FAIL);
	} else
	    return(0);
}

/*
 * Collision:
 * Invent a new filename for an existing file, if necessary
 * input:
 *	file	-> filename to expand
 * output:
 *	file	-> possibly rewritten filename target
 * returns: 
 *	>0	-> ok, invented new name
 *	0	-> ok, name remains the same
 *      FAIL	-> no usable name available
 */

int
collision (file)
register char *file;		/* name of file to check for collisions */
{
	register char *p;
	int i;

	if (eaccess(file, F_OK))	/* file doesn't exist? */
		return(0);
	/*
	 * Try inventing a new filename by appending a .NN to it
	 * where NN is a two digit number.  
	 */

	if (! (p = strrchr(file, '/')))
		p = file;
	else
		p++;
	if (strlen(p) <= (size_t)(MAXBASENAME - 3))
		p = p + strlen(p);
	else
		p += MAXBASENAME - 3; /* get to the last N - 3 positions */
	*p++ = '.';

	for (i = 0; i < 100; i++) {
		sprintf(p, "%02.2d", i);
		if (eaccess(file, F_OK))	/* file doesn't exist? */
			return(1);
	}
	return(FAIL);	/* couldn't invent a name! */
}

/*
 * make all necessary directories
 *	name	-> directory to make
 *	mask	-> mask to use during directory creation
 * return: 
 *	0	-> success
 * 	FAIL	-> failure
 */
int
mkdirs(name, mask)
mode_t mask;
register char *name;
{
	register char *p;
	mode_t omask;
	mode_t pmask = 0002;
	uid_t parent = 0;
	char dir[MAXFULLNAME];

	strcpy(dir, name);
	if (*LASTCHAR(dir) != '/')
	    	(void) strcat(dir, "/");
	p = dir + 1;
	for (;;) {
	    if ((p = strchr(p, '/')) == NULL)
		return(0);
	    *p = '\0';
	    if (DIRECTORY(dir)) {
		/* if directory exists the child's permissions
		   should be no more open than parent */
		parent = __s_.st_uid;
		pmask = ~__s_.st_mode;
	    } else {
		DEBUG(4, "mkdir - %s\n", dir);
		omask = umask(pmask);
		if (mkdir(dir, PUB_DIRMODE) == FAIL) {
		    umask(omask);
		    return (FAIL);
		}
		umask(omask);
		/* uucp can give away children directories to users */
		/* but only if the parent is publicly writable. */
		if ((parent == UUCPUID) && !(pmask & 0002)) {
			struct passwd *entry;
			char *dname;

			dname = BASENAME(dir, '/');
			if ( (entry = getpwnam(dname)) != NULL ) {
				pmask |= 01007;
				parent = entry->pw_uid;
			}
		}
		(void) chmod(dir, ~pmask);
		if (chown(dir, parent, UUCPGID) != 0) {
			parent = UUCPUID;
			pmask = mask;
		}
	    }
	    *p++ = '/';
	}
	/* NOTREACHED */
}

/*
 * expand file name and check return
 * print error if it failed.
 *	file	-> file name to check
 * returns: 
 *      0	-> ok
 *      FAIL	-> if expfile failed
 */
int
ckexpf(file)
char *file;
{
	if (expfile(file) == 0)
		return(0);

	pfmt(stderr, MM_ERROR, ":81:%s: <%s>: illegal filename\n", Progname, file);
	return(FAIL);
}


/*
 * make canonical path out of path passed as argument.
 *
 * Eliminate redundant self-references like // or /./
 * (A single terminal / will be preserved, however.)
 * Dispose of references to .. in the path names.
 * In relative path names, this means that .. or a/../..
 * will be treated as an illegal reference.
 * In full paths, .. is always allowed, with /.. treated as /
 *
 * returns:
 *	0	-> path is now in canonical form
 *	FAIL	-> relative path contained illegal .. reference
 */

int
canPath(path)
register char *path;	/* path is modified in place */
{
    register char *to, *fr;

    to = fr = path;
    if (*fr == '/') *to++ = *fr++;
    for (;;) {
	/* skip past references to self and validate references to .. */
	for (;;) {
	    if (*fr == '/') {
		fr++;
		continue;
	    }
	    if ((strncmp(fr, "./", 2) == SAME) || EQUALS(fr, ".")) {
		fr++;
		continue;
	    }
	    if ((strncmp(fr, "../", 3) == SAME) || EQUALS(fr, "..")) {
		fr += 2;
		/*	/.. is /	*/
		if (((to - 1) == path) && (*path == '/')) continue;
		/* error if no previous component */
		if (to <= path) return (FAIL);
		/* back past previous component */
		while ((--to > path) && (to[-1] != '/'));
		continue;
	    }
	    break;
	}
	/*
	 * What follows is a legitimate component,
	 * terminated by a null or a /
	 */
	if (*fr == '\0') break;
	while (((*to++ = *fr) != '\0') && (*fr++ != '/'));
    }
    /* null path is . */
    if (to == path) *to++ = '.';
    *to = '\0';
    return (0);
}

/*		copyright	"%c%" 	*/

/* 	Portions Copyright(c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident  "@(#)cron:common/cmd/cron/funcs.c	1.8.2.3"

/*#ident	"@(#)cron:common/cmd/cron/funcs.c	1.8.2.2" */

/*
 * Shared routines NOT used by the cron command proper.
 * These are separated out to avoid compiling code into
 * cron that is not part of the Trusted Computing Base.
 */

#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <mac.h>
#include <pfmt.h>
#include <string.h>
#include <stdlib.h>
#include "cron.h"

/*
 * Sends a message to the cron daemon.
 */

sendmsg(action, login, fname, etype, egid)
        char action;
	char *login;
        char *fname;
	char etype;
	gid_t egid;
{
        int msgfd;
	static struct message msgbuf;
        register int i;
	gid_t ogid = getegid();

	/* /etc/cron.d is accessible by group "cron" */
	(void) setegid(egid);

        /*The following do-while takes care of MR ul96-16201 */
        do{
                msgfd = open(NPIPE,O_WRONLY|O_NDELAY);
        } while (msgfd < 0 && errno == EAGAIN);

	(void) setegid(ogid);
        if (msgfd < 0) {
        	if (errno == ENXIO || errno == ENOENT){
                        pfmt(stderr, MM_ERROR, ":132:cron may not be running\n");
                        pfmt(stderr, MM_ACTION, ":133:Consult your system administrator\n");
		} else
                        pfmt(stderr, MM_ERROR, ":134:Cannot open message queue: %s\n",
                               	strerror(errno));
		return;
	}

	msgbuf.lid = 0;		/* presently unused */
        msgbuf.etype = etype;
        msgbuf.action = action;
        (void) strncpy(msgbuf.fname, fname, FILENAME_MAX);
        (void) strncpy(msgbuf.logname, login, UNAMESIZE);
	msgbuf.logname[UNAMESIZE] = '\0';

        if ((i = write(msgfd, (char *)&msgbuf, sizeof msgbuf)) < 0)
                pfmt(stderr, MM_ERROR, ":135:Cannot send message: %s\n",
                	strerror(errno));
        else if (i != sizeof msgbuf)
                pfmt(stderr, MM_ERROR, ":136:Cannot send message: Premature EOF\n");
	(void) close(msgfd);
}

int
filewanted(direntry)
        struct dirent *direntry;
{
        char *p;
        register char c;

        p = direntry->d_name;
        (void) num(&p);
        if (p == direntry->d_name)
                return (0);     /* didn't start with a number */
        if (*p++ != '.')
                return (0);     /* followed by a period */
        c = *p++;
        if (c < 'a' || c > 'z') 
                return (0);     /* followed by a queue name */
        return (1);
}

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct direct (through namelist). Returns -1 if there were any errors.
 */

#ifdef DIRSIZ
#undef DIRSIZ
#endif

#define DIRSIZ(dp) \
	(dp)->d_reclen

ascandir(dirname, namelist, select, dcomp)
	char *dirname;
	struct dirent *(*namelist[]);
	int (*select)(), (*dcomp)();
{
	register struct dirent *d, *p, **names;
	register int nitems;
	register char *cp1, *cp2;
	struct stat stb;
	long arraysz;
	DIR *dirp;

	if ((dirp = opendir(dirname)) == NULL)
		return(-1);
	if (fstat(dirp->dd_fd, &stb) < 0)
		return(-1);

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (stb.st_size / 24);
	names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
	if (names == NULL)
		return(-1);

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (select != NULL && !(*select)(d)) 
			continue;	/* just selected names */

		/*
		 * Make a minimum size copy of the data
		 */
		p = (struct dirent *)malloc(DIRSIZ(d));
		if (p == NULL) 
			return(-1); 
		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		/*p->d_namlen = d->d_namlen;*/
		for (cp1 = p->d_name, cp2 = d->d_name; *cp1++ = *cp2++; );
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			if (fstat(dirp->dd_fd, &stb) < 0) 
				return(-1);	/* just might have grown */
			arraysz = stb.st_size / 12;
			names = (struct dirent **)realloc((char *)names,
				arraysz * sizeof(struct dirent *));
			if (names == NULL)
				return(-1); 
		}
		names[nitems-1] = p;
	}
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct dirent *), dcomp);
	*namelist = names;
	return(nitems);
}

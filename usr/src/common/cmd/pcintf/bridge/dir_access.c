#ident	"@(#)pcintf:bridge/dir_access.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)dir_access.c	6.2	LCC);	/* Modified: 21:00:29 6/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"
#if defined(NO_DIR_LIB)

#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "log.h"
#include "xdir.h"


/*			Directory Access Routines 			*/

/*
 * Opendir() - opens a directory.
 */

DIR *
opendir(name)
    char *name;
{
    register DIR *dirp;
    register int fd;

    do
	fd = open(name, O_RDONLY);
    while (fd == -1 && errno == EINTR);
    if (fd < 0) {
	debug(0, ("opendir(%s) open failed\n", name));
	return NULL;
    }
    if ((dirp = (DIR *)memory(sizeof(DIR))) == NULL) {
	debug(0, ("opendir(%s) malloc failed\n", name));
        close(fd);
	return NULL;
    }
    dirp->dd_fd = fd;
    dirp->dd_loc = 0;
    dirp->dd_size = 0;
    return(dirp);
}


/*
 * Readdir() - get next entry in a directory.
 */

struct direct *
readdir(dirp)
    register DIR *dirp;
{
    register struct direct *dp;

    for (;;) {
    	if (dirp->dd_loc == 0) {
 	    do
		dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, BLKSIZ);
	    while (dirp->dd_size == -1 && errno == EINTR);
 	    if (dirp->dd_size <= 0)
    		return NULL;
	}
    	if (dirp->dd_loc >= dirp->dd_size) {
    	    dirp->dd_loc = 0;
    	    continue;
    	}
    	dp = (struct direct *)(dirp->dd_buf + dirp->dd_loc);
    	dirp->dd_loc += sizeof(struct direct);
    	if (dp->d_ino == 0)
    	   continue;
    	dirp->dd_dir.d_ino = dp->d_ino;
    	(void) strncpy(dirp->dd_dir.d_name, dp->d_name, DIRSIZ);
	dirp->dd_dirchars[sizeof (struct direct)] = '\0';
    	return(&dirp->dd_dir);
    }
}




/*
 * Telldir - returns current offset into a directory.
 */

long 
telldir(dirp)
    DIR *dirp;
{
    return (lseek(dirp->dd_fd, 0L, 1) - dirp->dd_size + dirp->dd_loc);
}




/*
 * Seekdir - seek to an entry in a directory.
 */

void 
seekdir(dirp, loc)
    register DIR *dirp;
    long loc;
{
    long curloc, base, offset;

    curloc = telldir(dirp);
    if (loc == curloc)
    	return;
    base = loc & ~(BLKSIZ - 1);
    offset = loc & (BLKSIZ - 1);
    lseek(dirp->dd_fd, base, 0);
    dirp->dd_loc = 0;
    if (offset != 0) {
	readdir(dirp);
	dirp->dd_loc = offset;
    }
}



/*
 * Closedir - close a directory.
 */

void 
closedir(dirp)
    register DIR *dirp;
{
    close(dirp->dd_fd);
    dirp->dd_fd = -1;
    dirp->dd_loc = 0;
    free(dirp);
}

#endif	/* NO_DIR_LIB */

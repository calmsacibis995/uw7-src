/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)pax:fileio.c	1.1.2.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/* 
 * fileio.c - file I/O functions for all archive interfaces
 *
 * DESCRIPTION
 *
 *	These function all do I/O of some form or another.  They are
 *	grouped here mainly for convienence.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms and that any documentation, advertising materials, and other 
 * materials related to such distribution and use acknowledge that the 
 * software was developed * by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Revision 1.2  89/02/12  10:04:31  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:09  mark
 * Initial revision
 * 
 */

/* Headers */
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"




/* open_archive -  open an archive file.  
 *
 * DESCRIPTION
 *
 *	Open_archive will open an archive file for reading or writing,
 *	setting the proper file mode, depending on the "mode" passed to
 *	it.  All buffer pointers are reset according to the mode
 *	specified.
 *
 * PARAMETERS
 *
 * 	int	mode 	- specifies whether we are reading or writing.   
 *
 * RETURNS
 *
 *	Returns a zero if successfull, or -1 if an error occured during 
 *	the open.
 */

    
int open_archive(int mode)

{
    if (ar_file[0] == '-' && ar_file[1] == '\0') {
	if (mode == AR_READ) {
	    archivefd = STDIN;
	    bufend = bufidx = bufstart;
	} else {
	    archivefd = STDOUT;
	}
    } else if (mode == AR_READ) {
	archivefd = open(ar_file, O_RDONLY | O_BINARY);
	bufend = bufidx = bufstart;	/* set up for initial read */
    } else if (mode == AR_WRITE) {
	archivefd = open(ar_file, O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0666);
    } else if (mode == AR_APPEND) {
	archivefd = open(ar_file, O_RDWR | O_BINARY, 0666);
	bufend = bufidx = bufstart;	/* set up for initial read */
    }

    if (archivefd < 0) {
	warnarch(strerror(errno), (OFFSET) 0);
	return (-1);
    }
    ++arvolume;
    return (0);
}


/* close_archive - close the archive file
 *
 * DESCRIPTION
 *
 *	Closes the current archive and resets the archive end of file
 *	marker.
 */


void close_archive(void)

{
    if (archivefd != STDIN && archivefd != STDOUT) {
	close(archivefd);
    }
    areof = 0;
}


/* openout - open an output file
 *
 * DESCRIPTION
 *
 *	Openo opens the named file for output.  The file mode and type are
 *	set based on the values stored in the stat structure for the file.
 *	If the file is a special file, then no data will be written, the
 *	file/directory/Fifo, etc., will just be created.  Appropriate
 *	permission may be required to create special files.
 *
 * PARAMETERS
 *
 *	char 	*name		- The name of the file to create
 *	Stat	*asb		- Stat structure for the file
 *	Link	*linkp;		- pointer to link chain for this file
 *	int	 ispass		- true if we are operating in "pass" mode
 *
 * RETURNS
 *
 * 	Returns the output file descriptor, 0 if no data is required or -1 
 *	if unsuccessful. Note that UNIX open() will never return 0 because 
 *	the standard input is in use. 
 */


int openout(char *name, Stat *asb, Link *linkp, int ispass)

{
    int             exists;
    int             fd;
    mode_t          perm;
    mode_t          operm = 0;
    Stat            osb;
    Dirlist	    *dp;
    int             len;
#ifdef	S_IFLNK
    int             ssize;
    char            sname[PATH_MAX + 1];
#endif	/* S_IFLNK */

    if (exists = (LSTAT(name, &osb) == 0)) {
	if (f_no_overwrite) {
	    warn(name, gettxt(FIO_EXISTS, "exists - will not overwrite"));
	    return (-1);
        }
	if (ispass && osb.sb_ino == asb->sb_ino && osb.sb_dev == asb->sb_dev) {
	    warn(name, gettxt(FIO_SAME, "Same file"));
	    return (-1);
	} else if ((osb.sb_mode & S_IFMT) == (asb->sb_mode & S_IFMT)) {
	    operm = osb.sb_mode & S_IPERM;
	} else if (REMOVE(name, &osb) < 0) {
	    warn(name, strerror(errno));
	    return (-1);
	} else {
	    exists = 0;
	}
    }

    if (linkp) {
	if (exists) {
	    if (asb->sb_ino == osb.sb_ino && asb->sb_dev == osb.sb_dev) {
		return (0);
	    } else if (unlink(name) < 0) {
		warn(name, strerror(errno));
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if (link(linkp->l_name, name) != 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) != 0 ||
			    link(linkp->l_name, name) != 0) {
			char err[PATH_MAX+10];
			int se = errno;	/* sprintf can change errno */
			sprintf(err,"link: %s", linkp->l_name);
			warn(err, strerror(se));
			return (-1);
		    }
		} else {
		    warn(name, 
			 gettxt(FIO_DIR, "Directories are not being created"));
		}
		return(0);
	    } else if ((errno != EXDEV) || (!ispass)){
		warn(name, strerror(errno));
		return (-1);
	    }
	} else {
	    return(0);
	} 
    }
    perm = asb->sb_mode & S_IPERM;
    switch (asb->sb_mode & S_IFMT) {
    case S_IFBLK:
    case S_IFCHR:
	fd = 0;
	if (exists) {
	    if (asb->sb_rdev == osb.sb_rdev) {
		if (perm != operm && chmod(name, perm) < 0) {
		    warn(name, strerror(errno));
		    return (-1);
		} else {
		    break;
		}
	    } else if (REMOVE(name, &osb) < 0) {
		warn(name, strerror(errno));
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if (mknod(name, (int) asb->sb_mode, (int) asb->sb_rdev) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 || mknod(name, (int) asb->sb_mode, 
			   (int) asb->sb_rdev) < 0) {
			warn(name, strerror(errno));
			return (-1);
		    }
		} else {
		    warn(name, 
			 gettxt(FIO_DIR, "Directories are not being created"));
		}
	    } else {
		warn(name, strerror(errno));
		return (-1);
	    }
	}
	return(0);
	break;
    case S_IFDIR:
	if (exists) {
	    if (perm != operm && chmod(name, perm) < 0) {
		warn(name, strerror(errno));
		return (-1);
	    }
	} else if (f_dir_create) {
	    if (dirmake(name, asb->sb_mode & S_IPERM, 1)<0 && dirneed(name)<0) {
		warn(name, strerror(errno));
		return (-1);
	    }
	} else {
	    warn(name, gettxt(FIO_DIR, "Directories are not being created"));
	    return(0);
	}
	/* Add this directory to the list so we can go back
	 * and restore modification times and permissions later.
	 */
	if (((dp = (Dirlist *) mem_get(sizeof(Dirlist))) != (Dirlist *)NULL) &&
	    ((dp->name = mem_str(name)) != (char *)NULL)) {
		dp->perm = asb->sb_mode;
		dp->atime = asb->sb_atime;
		dp->mtime = asb->sb_mtime;
		dp->uid = asb->sb_uid;
		dp->gid = asb->sb_gid;
		dp->next = NULL;
		if (dirhead == NULL)
			dirhead = dp;
		else
			dirtail->next = dp;
		dirtail = dp;
	}
	return (0);
#ifdef	S_IFIFO
    case S_IFIFO:
	fd = 0;
	if (exists) {
	    if (perm != operm && chmod(name, perm) < 0) {
		warn(name, strerror(errno));
		return (-1);
	    }
	} else if (mknod(name, (int) asb->sb_mode, 0) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0
		       || mknod(name, (int) asb->sb_mode, 0) < 0) {
			warn(name, strerror(errno));
			return (-1);
		    }
		} else {
		    warn(name, 
			 gettxt(FIO_DIR, "Directories are not being created"));
		}
	    } else {
		warn(name, strerror(errno));
		return (-1);
	    }
	}
	return(0);
	break;
#endif				/* S_IFIFO */
#ifdef	S_IFLNK
    case S_IFLNK:
	if (exists) {
	    if ((ssize = readlink(name, sname, sizeof(sname))) < 0) {
		warn(name, strerror(errno));
		return (-1);
	    } else if (strncmp(sname, asb->sb_link, ssize) == 0) {
		return (0);
	    } else if (REMOVE(name, &osb) < 0) {
		warn(name, strerror(errno));
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if (symlink(asb->sb_link, name) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 || symlink(asb->sb_link, name) < 0) {
			warn(name, strerror(errno));
			return (-1);
		    }
		} else {
		    warn(name, 
			 gettxt(FIO_DIR, "Directories are not being created"));
		}
	    } else {
		warn(name, strerror(errno));
		return (-1);
	    }
	}
	return (0);		/* Can't chown()/chmod() a symbolic link */
#endif				/* S_IFLNK */
    case S_IFREG:
	if (exists) {
	    if (unlink(name) < 0) {
		warn(name, strerror(errno));
		return (-1);
	    } else {
		exists = 0;
	    }
	}
	if ((fd = creat(name, perm)) < 0) {
	    if (errno == ENOENT) {
		if (f_dir_create) {
		    if (dirneed(name) < 0 || 
			    (fd = creat(name, perm)) < 0) {
			warn(name, strerror(errno));
			return (-1);
		    }
		} else {
		    /* 
		     * the file requires a directory which does not exist
		     * and which the user does not want created, so skip
		     * the file...
		     */
		    warn(name, 
			 gettxt(FIO_DIR, "Directories are not being created"));
		    return(0);
		}
	    } else {
		warn(name, strerror(errno));
		return (-1);
	    }
	}
	break;
    default:
	warn(name, gettxt(FIO_UKNOWN, "Unknown filetype"));
	return (-1);
    }
    return (fd);
}


/* openin - open the next input file
 *
 * DESCRIPTION
 *
 *	Openi will attempt to open the next file for input.  If the file is
 *	a special file, such as a directory, FIFO, link, character- or
 *	block-special file, then the file size field of the stat structure
 *	is zeroed to make sure that no data is written out for the file.
 *	If the file is a special file, then a file descriptor of 0 is
 *	returned to the caller, which is handled specially.  If the file
 *	is a regular file, then the file is opened and a file descriptor
 *	to the open file is returned to the caller.
 *
 * PARAMETERS
 *
 *	char   *name	- pointer to the name of the file to open
 *	Stat   *asb	- pointer to the stat block for the file to open
 *
 * RETURNS
 *
 * 	Returns a file descriptor, 0 if no data exists, or -1 at EOF. This 
 *	kludge works because standard input is in use, preventing open() from 
 *	returning zero. 
 */


int openin(char *name, Stat *asb)

{
    int             fd;

    switch (asb->sb_mode & S_IFMT) {
    case S_IFDIR:
	asb->sb_nlink = 1;
	asb->sb_size = 0;
	return (0);
#ifdef	S_IFLNK
    case S_IFLNK:
	if ((asb->sb_size = readlink(name,
			     asb->sb_link, sizeof(asb->sb_link) - 1)) < 0) {
	    warn(name, strerror(errno));
	    return(0);
	}
	asb->sb_link[asb->sb_size] = '\0';
	return (0);
#endif				/* S_IFLNK */
    case S_IFREG:
	if (asb->sb_size == 0) {
	    return (0);
	}
	if ((fd = open(name, O_RDONLY | O_BINARY)) < 0) {
	    warn(name, strerror(errno));
	}
	return (fd);
    default:
	asb->sb_size = 0;
	return (0);
    }
}

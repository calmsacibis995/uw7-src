#ident	"@(#)pcintf:bridge/p_open.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_open.c	6.6	LCC);	/* Modified: 16:56:11 1/6/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#if defined(RLOCK)
#	include <rlock.h>
#endif	/* RLOCK */

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "dossvr.h"
#include "log.h"
#include "xdir.h"


/*				External Routines			*/

extern unsigned short	btime		PROTO((struct tm *));
extern unsigned short	bdate		PROTO((struct tm *));


void
pci_open(dos_fname, mode, attr, pid, addr, request)
    char	*dos_fname;		/* File name(with possible metachars) */
    int		mode;			/* Mode to open file with */
    int		attr;			/* MS-DOS file attribute */
    int		pid;			/* Process id of PC process */
    struct	output	*addr;		/* Pointer to response buffer */
    int		request;		/* DOS request number simulated */
{
    register int
	adescriptor,			/* Actual UNIX file descriptor */
	vdescriptor;			/* PCI virtual file descriptor */

    int
	hflg,				/* Hidden MS-DOS file attribute */
	allowDir;			/* Let open succeed on directory */

    long
	dos_time_stamp;			/* virtual DOS time stamp on file */

    char
	filename[MAX_FN_TOTAL],		/* dos_fname converted to unix format */
	*dotptr,			/* Pointer to "." in filename */
	pathcomponent[MAX_FN_TOTAL],	/* Pathname component of filename */
	namecomponent[MAX_FN_TOTAL],	/* Filename component of filename */
	pipefilename[MAX_FN_TOTAL];         /* Filename of pipe file in /tmp */

    struct	tm 
	*timeptr;			/* Pointer for locattime() */

    struct	stat
	filstat;			/* File status structure for stat() */
    struct	temp_slot
	*tslot;				/* Temp slot for directory redirect */

#ifdef RLOCK  /* record locking */
    int		share;			/* file share access mode */
#endif  /* RLOCK */


#ifdef RLOCK  /* record locking */
	/* At this point "mode" holds both the share (upper 4 bits) and
	   the open mode (lower four bits).
	*/
	/* Pull out the share bits */
	share = SHR_BITS(mode);

	/* Pull out the r/w open bits */
	mode = RW_BITS(mode);
#endif  /* RLOCK */

    /* "Clean-up" MS-DOS pathname */
	/* If unmapping filename returns collision return failure */
	if (cvt_fname_to_unix(MAPPED, (unsigned char *)dos_fname,
	    (unsigned char *)filename)) {
	    addr->hdr.res = FILE_NOT_FOUND;
	    return;
	}

	/*** Kludge to allow access to protected directories for tmps ***/
	if ((tslot = redirdir(dos_fname, NULL)) != NULL) {
	    strcpy(filename, "/tmp/");
	    strcat(filename, tslot->ts_fname);
	}

#ifdef	JANUS
	fromfunny(filename); /* this translates funny names, as well as */
#endif  /* JANUS                  autoexec.bat, con, ibmbio.com, ibmdos.com  */

	allowDir = (attr & SUB_DIRECTORY) ? TRUE : FALSE;
	hflg = (attr & HIDDEN) ? TRUE : FALSE;

    /* Split input string into pathname and filename */
	getpath(filename, pathcomponent, namecomponent); 

    /* If this is for a pipe open the pipe in /tmp */
	if ((strncmp(filename, "/%pipe", PIPEPREFIXLEN)) == 0) {
	    strcpy(pipefilename, "/tmp");
	    dotptr = strchr(filename, '.');
	    sprintf(dotptr+1, "%d", getpid());
	    strcat(pipefilename, filename);
	    strcpy(filename, pipefilename);
	}

    /* If filename has wildcards search the directory for a matching file */
	else if (!wildcard(filename, strlen(filename))) {
	    if (stat(filename, &filstat) < 0) {
	        err_handler(&addr->hdr.res, request, filename);
		return;
	    }
	    if ((filstat.st_mode & S_IFMT) == S_IFDIR && !allowDir) {
		addr->hdr.res = ACCESS_DENIED;
		return;
	    }
	} else {
		debug(0, ("pci_open:ERR wildcard file %s\n", filename));
		addr->hdr.res = FILE_NOT_FOUND;
		return;
	}

	if ((vdescriptor = open_file(filename, mode,
#ifdef RLOCK  /* record locking */
						share,
#endif  /* RLOCK */
						pid, request)) < 0) {
	    addr->hdr.res = (unsigned char)-vdescriptor;	/* error code */
	    return;
	}

    /* Get the actual file descriptor from the virtual descriptor */
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
			   		 ? FILDES_INVALID : ACCESS_DENIED);
	    return;
	}

	if ((fstat(adescriptor, &filstat)) < 0) {
	    err_handler(&addr->hdr.res, request, NULL);
	    return;
	}


    /* Fill-in response buffer */
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
	addr->hdr.fdsc = (unsigned short)vdescriptor;
	addr->hdr.inode = (u_short)filstat.st_ino;
	addr->hdr.f_size = filstat.st_size;
	addr->hdr.attr = attribute(&filstat, filename);

	dos_time_stamp = get_dos_time (vdescriptor);
	timeptr = localtime(&(dos_time_stamp));
	addr->hdr.date = bdate(timeptr);
	addr->hdr.time = btime(timeptr);

	addr->hdr.mode = (unsigned short)((filstat.st_mode & 070000) == 0);
	return;
}

#ident	"@(#)pcintf:bridge/p_delete.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_delete.c	6.5	LCC);	/* Modified: 11:44:32 1/7/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#ifdef	RLOCK
#include <fcntl.h>
#include <rlock.h>
#endif	/* RLOCK */

#include "pci_proto.h"
#include "pci_types.h"
#include "dossvr.h"
#include "log.h"
#include "table.h"
#include "xdir.h"


LOCAL int	delete_common	PROTO((char *, int, int, struct output *));


void
pci_delete(dos_fname, attr, pid, addr, request)
    char	*dos_fname;		/* Name of file to unlink */
    int		attr;			/* MS-DOS file attribute */
    int		pid;			/* process ID of MSDOS process */
    struct	output	*addr;		/* Address of response buffer */
    int		request;		/* DOS request number simulated */
{
    register	int
	    hflg;			/* Hidden attribute flag */
    int found;				/* Have we found anything? */

    char
	    filename[MAX_FN_TOTAL],	/* File name converted to unix */
	    *dotptr,			/* Pointer to "." in file name */
	    *slashptr,			/* Pointer to last '/' in filename */
	    tmpfilename[MAX_FN_TOTAL],	/* File name of pipe */
	    pathcomponent[MAX_FN_TOTAL],/* Path component of filename */
	    mappedname[MAX_FN_COMP + 1],	/* mapped version of candidate
					*  directory entries
					*/
	    *namecomponent;		/* File component of filename */

    DIR
	    *dirdp;			/* Pointer to opendir() structure */

    struct dirent
	    *direntryptr;		/* Pointer to directory entry */

    struct	temp_slot
	*tslot;				/* Temp slot for directory redirect */

	found = 0;
	addr->hdr.res = FILE_NOT_FOUND;

    /* Translate MS-DOS filename to UNIX filename */

    /* If unmapping filename returns collision return failure */
	if (cvt_fname_to_unix(UNMAPPED, (unsigned char *)dos_fname,
	    (unsigned char *)filename)) {
	    addr->hdr.res = ACCESS_DENIED;
	    return;
	}

	hflg = (attr & HIDDEN) ? TRUE : FALSE;

    /* Delete all files that match with "name" */
	if (wildcard(filename, strlen(filename))) {

	    /*** Kludge to allow access to protected directories for tmps ***/
	    tslot = NULL;
	    while ((tslot = redirdir(dos_fname, tslot)) != NULL) {
		strcpy(tmpfilename, "/tmp/");
		strcat(tmpfilename, tslot->ts_fname);
		found |= delete_common(tmpfilename,pid,request,addr);
		tslot = remove_redirdir(tslot);
	    }

	    set_tables(U2U);
	    lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE, 0,country);

	    /* Split search pattern into directory and filename component */
	    if ((slashptr = strrchr(filename, '/')) == NULL) {
		namecomponent = filename;
		strcpy(pathcomponent, CurDir);
	    } else {
		namecomponent = slashptr + 1;
		if (slashptr == filename)
		    strcpy(pathcomponent, "/");
		else {
		    *slashptr = '\0';
		    lcs_translate_string(pathcomponent, MAX_FN_TOTAL, filename);
		    unmapfilename(CurDir, pathcomponent);
		    *slashptr = '/';
		}
	    }

	    if ((dirdp = opendir(pathcomponent)) == NULL) {
log("pci_delete:  can't open directory %s\n", pathcomponent);
		if (found)
		    goto done;
		addr->hdr.res = PATH_NOT_FOUND;
		return;
	    }

	    while ((direntryptr = readdir(dirdp)) != NULL) {
		if ((is_dot(direntryptr->d_name)) ||
		    (is_dot_dot(direntryptr->d_name)) ||
		    ((!hflg) && (direntryptr->d_name[0] == '.')))
			continue;
		strcpy(mappedname,direntryptr->d_name);
		mapfilename(pathcomponent,mappedname);
log("pci_delete:  consider %s\n", mappedname);
		if (match(mappedname, namecomponent, MAPPED)) {
		    strcpy(tmpfilename, pathcomponent);
		    strcat(tmpfilename, "/");
		    strcat(tmpfilename, direntryptr->d_name);
		    found |= delete_common(tmpfilename,pid,request,addr);
		}
	    }
	    closedir(dirdp);
	} else {   /* no wild cards */
	    if (cvt_fname_to_unix(MAPPED, (unsigned char *)dos_fname,
		(unsigned char *)filename)) {
		    addr->hdr.res = ACCESS_DENIED;
		    return;
	    }

	    /*** Kludge to allow access to protected directories for tmps ***/
	    if ((tslot = redirdir(dos_fname, NULL)) != NULL) {
		strcpy(filename, "/tmp/");
		strcat(filename, tslot->ts_fname);
		remove_redirdir(tslot);
	    }

	    /* If this is for a pipe open the pipe in /tmp */
	    if ((strncmp(filename, "/%pipe", PIPEPREFIXLEN)) == 0) {
		strcpy(tmpfilename, "/tmp");
		dotptr = strchr(filename, '.');
		sprintf(dotptr+1, "%d", getpid());
		strcat(tmpfilename, filename);
		strcpy(filename, tmpfilename);
	    }

	    found |= delete_common(filename,pid,request,addr);
	}

done:
	/* fill in response header */
	if (found) {
	    addr->hdr.res = SUCCESS;
	    addr->hdr.stat = NEW;
	} else {
	    /* use the hdr.res value from the last error encountered */
	}
	return;
}


int
delete_common(filename,pid,request,addr)
char *filename;
int pid;
int request;		/* DOS request number simulated */
struct	output	*addr;		/* Address of response buffer */
{
	struct stat statb;
	int failed;
	int vdescriptor;

	log("delete_common(%s, %d, %d, %lx)\n", filename, pid, request, addr);
	if (stat(filename, &statb) < 0) goto fail;

	/* only unlink "regular" files		*/
	if((statb.st_mode & S_IFMT) != S_IFREG) return 0;

	/* If we don't have write perms and aren't root, fail. */
	if (access(filename, 2) && getuid()) goto fail;

#ifdef RLOCK  /* record locking */
	if ((vdescriptor = open_file(filename, O_RDONLY, SHR_RDWRT,
					pid, request)) < 0) {
	    addr->hdr.res = (unsigned char)-vdescriptor;	/* error code */

	    return 0;
	}
#endif  /* RLOCK */
	failed = unlink(filename);
#ifdef RLOCK
	close_file(vdescriptor,0);
#endif	/* RLOCK */
	if(!failed) {
		del_fname(filename);
		return 1;
	}

fail:
	err_handler(&addr->hdr.res, request, filename);
	return 0;
}

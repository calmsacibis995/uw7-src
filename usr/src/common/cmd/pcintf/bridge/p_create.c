#ident	"@(#)pcintf:bridge/p_create.c	1.1.1.4"
#include	"sccs.h"
SCCSID(@(#)p_create.c	6.9	LCC);	/* Modified: 15:05:03 2/20/92 */

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

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "dossvr.h"
#include "log.h"
#include "table.h"

/*				External Routines			*/

extern unsigned short	btime		PROTO((struct tm *));
extern unsigned short	bdate		PROTO((struct tm *));
extern char		*mktemp		PROTO((char *));


/*			Imported Structures				*/

extern  int print_desc[NPRINT];    /* File descriptor of print/spool file */

extern  char *print_name[NPRINT];  /* Filename of file to be printed */

struct temp_slot *temp_slot_head = NULL;

void
pci_create(dos_fname, mode, dosAttr, pid, addr, request)
char
	*dos_fname;			/* File name(with possible metachars) */
int
	mode,				/* Mode to open file with */
	dosAttr,			/* Dos file attribute */
	pid;				/* Process id of PC process */

struct output
	*addr;				/* Pointer to response buffer */
int
	request;			/* DOS request number simulated */
{
    register int
	vdescriptor, 			/* PCI virtual file descriptor */
	adescriptor;			/* Actual UNIX file descriptor */

    int
	ddev,	 			/* dev of directory */
	printx = -1,                    /* printer table index */
	unlink_on_sig = FALSE;

    long
	dos_time_stamp;			/* Virtual DOS time stamp on file */

    ino_t 
	dino;				/*Inode of directory */ 

    char
	*dotptr, 			/* Pointer to "." in filename */
	filename[MAX_FN_TOTAL],		/* Unix style filename */
	pipefilename[MAX_FN_TOTAL],	/* Filename of pipe file in /tmp */
	pathcomponent[MAX_FN_TOTAL];	/* Path part of full filename */

    struct tm 
	*timeptr;			/* Pointer for locattime() */

    struct	stat
	filstat;			/* File status structure for stat() */

    struct temp_slot
	*tslot;				/* temp file slot number */

	dino = 0;
      /* Transform filename if need be */
	if (cvt_fname_to_unix(MAPPED, (unsigned char *)dos_fname,
	    (unsigned char *)filename)) {
		addr->hdr.res = ACCESS_DENIED;
		return;
	}
	if (strpbrk(filename, "?*")) {		/* disallow wildcards */
	    addr->hdr.res = FILE_NOT_FOUND;	/* sic */
	    return;
	}
	if ((dosAttr & SUB_DIRECTORY) ||
	    (mode == TEMPFILE && (dosAttr & VOLUME_LABEL))) {
	    addr->hdr.res = ACCESS_DENIED;
	    return;
	}
	if (mode == PRINT) {
	  /* Create a tmp file for print spooling */
	    if ((printx = find_printx(-1)) == -1) {
		log("Print file table full!\n\n");
		addr->hdr.res = TOO_MANY_FILES;
		return;
	    }
	    print_name[printx] = mktemp(savestr("/tmp/pciXXXXXX"));
	    strcpy(filename, print_name[printx]);
	    unlink_on_sig = TRUE;
	    log("Opened print file.  printx = %d, name = %s\n\n",printx,
		print_name[printx]);
	} else {

	    if (mode != TEMPFILE &&
		strncmp(filename, "/%pipe", PIPEPREFIXLEN) != 0) {
		if ((tslot = redirdir(dos_fname, NULL)) != NULL) {
		    strcpy(filename, "/tmp/");
		    strcat(filename, tslot->ts_fname);
		}
		strcpy(pathcomponent, "/");
	    }

#ifdef  JANUS
	    fromfunny(filename); /* this translates funny names, as well as */
			      /* autoexec.bat, con, ibmbio.com, ibmdos.com  */
#endif  /* JANUS */

	    addr->hdr.t_cnt = 0;

	    if (mode == TEMPFILE) {
		if (strlen(filename) == 0)   /* null string becomes root */
		    strcpy(filename, "/");

		dotptr = strlen(filename) + filename; /* point at ending null */

		if ((*(dotptr-1) != '/') && (*(dotptr-1) != '\\'))
		{
		    *(dotptr++) = '/';
		    *dotptr = '\0';         /* add in terminator */
		}

		/*** Check that directory is writeable ***/
		if (access(filename, 2) &&
		    ((errno == EACCES) || (errno == EROFS)))
		{
		    /* Not writeable */
		    if (stat(filename, &filstat)) {
			err_handler(&addr->hdr.res, request, filename);
			return;     /* Directory has serious problems */
		    }
		    dino = filstat.st_ino;  /* Remember directory inode */
		    ddev = filstat.st_dev;
		    strcpy(filename, "/tmp/00XXXXXX");
		    dotptr = &filename[5];
		} else {
		    /* add in the temp string */
		    strcat(filename, "00XXXXXX");
		}
		dotptr[1] --;
		do {
		    dotptr[1]++;
		    mktemp(filename);
		} while (stat(filename, &filstat) >= 0);

	    /* just return name portion to DOS */

		unlink_on_sig = TRUE;
		strncpy(pathcomponent, filename, (dotptr - filename));
		pathcomponent[dotptr - filename] = '\0';
	    } /*endif TEMP */
	    /* If this is for a pipe open the pipe in /tmp */
	    else if ((strncmp(filename, "/%pipe", PIPEPREFIXLEN)) == 0)
	    {
		    strcpy(pipefilename, "/tmp");
		    dotptr = strchr(filename, '.');
		    sprintf(dotptr+1, "%d", getpid());
		    strcat(pipefilename, filename);
		    strcpy(filename, pipefilename);
		    unlink_on_sig = TRUE;
	    } /*endif PIPE */
	}

      /* At this point am done converting filename. */
      /* Now, create the file, and get a virtual descriptor. */

	if (mode == CREATENEW && stat(filename, &filstat) >= 0) {
		addr->hdr.res = FILE_EXISTS;
		return;
	}

	vdescriptor = create_file(filename, 2,
				pid, unlink_on_sig, dosAttr, mode);

	if (vdescriptor < 0) {
	    addr->hdr.res = (unsigned char)-vdescriptor;
	    return;
	}

	/* Get the actual descriptor */
	if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
	    addr->hdr.res = (unsigned char)((adescriptor == NO_FDESC)
	   				 ? FILDES_INVALID : ACCESS_DENIED);
	    return;
	}

	if ((fstat(adescriptor, &filstat)) < 0)
	{
	    err_handler(&addr->hdr.res, request, NULL);
	    return;
	}

    /* If for print/spooling, store file descriptor */
	if (printx != -1)
	    print_desc[printx] = vdescriptor;

	if (mode == TEMPFILE) {
	    cvt_fname_to_dos(MAPPED, (unsigned char *)pathcomponent,
		(unsigned char *)dotptr, (unsigned char *)addr->text, (ino_t)0);
	    addr->hdr.t_cnt = (unsigned short)strlen(addr->text)+1;
	    if (dino) {  /* if put the temp file in /tmp then put */
			/*filename in temp slot array, so can find later. */
		if ((tslot = (struct temp_slot *)
				memory(sizeof(struct temp_slot))) != NULL) {
			logchan(0x80, ("redirdir: Allocated %s (%d, %d) %s\n",
				pathcomponent, ddev, dino, dotptr));
		        strcpy(tslot->ts_fname, dotptr);
		        tslot->ts_ino = dino;
		        tslot->ts_dev = ddev;
			tslot->ts_next = temp_slot_head;
			temp_slot_head = tslot;
	        }
	    }
	}

    /* Fill-in response header */
	addr->hdr.res    = SUCCESS;
	addr->hdr.stat   = NEW;
	addr->hdr.fdsc   = (unsigned short)vdescriptor;
	addr->hdr.offset = (long)lseek(adescriptor, 0L, 1);
	addr->hdr.inode = (u_short)filstat.st_ino;
	addr->hdr.f_size = filstat.st_size;
	addr->hdr.attr   = attribute(&filstat, filename);

	dos_time_stamp = get_dos_time (vdescriptor);
	timeptr = localtime(&(dos_time_stamp));
	addr->hdr.date = bdate(timeptr);
	addr->hdr.time = btime(timeptr);

	addr->hdr.mode = (unsigned short)((filstat.st_mode & 070000) == 0);
}


/*--------------------------------------------------------*/
/* Test for match of filename in relocated filename table
/* Return zero if no match.
/* Return index into temp filename table plus 1.
/*--------------------------------------------------------*/
struct temp_slot *
redirdir(dos_filename, first_tslot)
char *dos_filename;
struct temp_slot *first_tslot;
{
	struct stat filstat;
	register struct temp_slot *tslot;
	int ddev;
	ino_t dino;
	char *slashptr;			/* Pointer to last '/' in filename */
	char filename[MAX_FN_TOTAL];
	char pathcomponent[MAX_FN_TOTAL];/* Path component of filename */
	char mappedname[MAX_FN_COMP + 1];/* mapped version of candidate */
	char *namecomponent;		/* File component of filename */

	cvt_fname_to_unix(UNMAPPED, (unsigned char *)dos_filename,
	    (unsigned char *)filename);
	
	set_tables(U2U);
	lcs_set_options(LCS_MODE_NO_MULTIPLE|LCS_MODE_LOWERCASE, 0, country);

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
	
	/* In the next statement we are checking for filenames
	 * starting with a '0' (temp files in UNIX) rather than
	 * a null string.
	 */
	if (*namecomponent != '0' ||
	    stat(pathcomponent, &filstat))
		return NULL;

	dino = filstat.st_ino;
	ddev = filstat.st_dev;

	tslot = (first_tslot != NULL) ? first_tslot : temp_slot_head;
	while (tslot != NULL) {
	    if (tslot->ts_ino == dino && tslot->ts_dev == ddev) {
		strcpy(mappedname, tslot->ts_fname);
		mapfilename("/tmp", mappedname);
		if (match(mappedname, namecomponent, MAPPED)) {
		    logchan(0x80, ("redirdir: Found %s (%d, %d) %s\n",
				pathcomponent, ddev, dino, tslot->ts_fname));
		    return tslot;
		}
	    }
	    tslot = tslot->ts_next;
	}
	return NULL;
}


struct temp_slot *
remove_redirdir(tslot)
struct temp_slot *tslot;
{
	register struct temp_slot *ts;

	if (temp_slot_head == tslot) {
		logchan(0x80, ("redirdir: Freed (%d, %d) %s\n",
			tslot->ts_dev, tslot->ts_ino, tslot->ts_fname));
		temp_slot_head = tslot->ts_next;
		free(tslot);
		return temp_slot_head;
	}
	for (ts = temp_slot_head; ts; ts = ts->ts_next) {
		if (ts->ts_next == tslot) {
			logchan(0x80, ("redirdir: Freed (%d, %d) %s\n",
				tslot->ts_dev, tslot->ts_ino, tslot->ts_fname));
			ts->ts_next = tslot->ts_next;
			free(tslot);
			return ts->ts_next;
		}
	}
	return NULL;
}

#ident	"@(#)pcintf:bridge/p_close.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)p_close.c	6.5	LCC);	/* Modified: 16:59:57 1/6/92 */

/****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>
#include <string.h>

#include <lmf.h>

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"
#include "dossvr.h"
#include "log.h"
#include "table.h"

#ifndef FALSE
#define FALSE           0
#endif

#ifndef TRUE
#define TRUE            1
#endif

#ifdef	JANUS
#define	PRINT_PGM	"/usr/lib/merge/lp -i %1 -o %2"
#define DEFAULT_PPATH	"PATH=:/usr/bin:/bin"
#endif	/* JANUS */

#ifndef	PRINT_PGM
#define	PRINT_PGM	"/usr/pci/bin/pciprint -i %1 -o %2"
#endif	/* PRINT_PGM */

#ifndef	DEFAULT_PPATH
#define DEFAULT_PPATH	"PATH=:/usr/bin:/bin:/usr/pci/printprogs"
#endif	/* DEFAULT_PPATH */

extern	int	print_desc[NPRINT];  /* Print file descriptors */
extern	char	*print_name[NPRINT]; /* Print file names */
extern  char	**environ;		     /* current server environment */

#if defined(__STDC__)
void pci_close(int vdescriptor, long filesize, u_short inode, int mode,
		int txtcnt, char *txt, struct output *addr, int request)
#else
void
pci_close(vdescriptor, filesize, inode, mode, txtcnt, txt, addr, request)
    int		vdescriptor;		/* PCI virtual file descriptor */
    long	filesize;		/* Length file should be after close */
    u_short	inode;			/* Inode of file to be closed */
    int		mode;			/* Mode (print file close only) */
    int		txtcnt;			/* length of printer command */
    char	*txt;			/* Printer command */
    struct	output	*addr;
    int		request;		/* Transaction request type */
#endif
{
    register    int     status;         /* Return value from system call */
    register    int     adescriptor;    /* Acutual UNIX file descriptor */

    struct      stat    filstat;        /* Buffer contains data from stat() */

    int		printx;			/* index into print file table */

    /* Submit print/spool request */
	if ((printx = find_printx(vdescriptor)) != -1)
	{
	    log("\nSpooling print file.	 printx = %d\n",printx);
	    if (s_print(printx, mode, txtcnt, txt)) {
		log("\n	 Error spooling print file.\n");
		addr->hdr.res = FILDES_INVALID;
	    }

	/* Clean-up after print request */
	    unlink(print_name[printx]);
	    free(print_name[printx]);
	    print_name[printx] = (char *)NULL;
	    print_desc[printx] = -1;
	}

    /*
     * MS-DOS extend/truncates files on OLD STYLE closes if file has been
     * written.  If the file size is changed in the fcb then DOS
     * extends/truncates it.
     */
	if (request == OLD_CLOSE && write_done(vdescriptor)) {
	    if ((adescriptor = swap_in(vdescriptor, DONT_CARE)) < 0) {
		addr->hdr.res = FILDES_INVALID;
		return;
	    }

	    if (fstat(adescriptor, &filstat) < 0) {
		err_handler(&addr->hdr.res, request, NULL);
		return;
	    }

	    if (filstat.st_size > filesize) {
		if ((pci_truncate(filesize, vdescriptor)) == FALSE) {
		    addr->hdr.res = FILDES_INVALID;
		    return;
		}
	    }
	    else if (filstat.st_size < filesize) {
		if ((status = lseek(adescriptor, filesize - 1, 0)) < 0) {
		    err_handler(&addr->hdr.res, request, NULL);
		    return;
		}
		do
		    status = write(adescriptor, "", 1);
		while (status == -1 && errno == EINTR);
		if (status < 0) {
		    err_handler(&addr->hdr.res, request, NULL);
		    return;
		}
	    }
	}

	if ((status = close_file(vdescriptor, 
 				(request == OLD_CLOSE) ? inode : 0)) < 0) {
	    if (status == NO_FDESC) {
		addr->hdr.res = FILDES_INVALID;
		return;
	    }
	}
	addr->hdr.res = SUCCESS;
	addr->hdr.stat = NEW;
}

/******************************************
 *      print spool file
 *      return TRUE if problems
 */
int s_print(printx, mode, txtcnt, txt)
int printx;
int mode;
int txtcnt;
char *txt;
{
    int status;
    int pid;
    int ii;
    char *cp;
    char *printprog;
    char *args[6];
    char path[256];

    /* txtcnt > 0 means we have a "new" bridge on the dos side who knows
       about deleting the output.  If mode is non-zero then delete the printer
       output.
    */
    if ((txtcnt > 0) && (mode != 0)) return FALSE;

    /* start up a child who will start sh as his child to do the spooling.
       This disconnects us from the printing process.
    */
    if ((pid = fork_wait((int *)NULL)) != 0) {
	return ((int)(pid == -1));
    }
    else {
						/* close all files */
	for (ii=0; ii < uMaxDescriptors(); ii++)
		close(ii);
						/* open print file as stdin */
	do
		status = open(print_name[printx],O_RDONLY);
	while (status == -1 && errno == EINTR);
	if (status == -1) {
		exit(1);
	}

	/* fail soft here.  If we can, start a child to do the printing.  If
	   the fork fails we will do the printing at this level.
	*/
	if ((pid = fork()) > 0) {
		exit(0);
	}

	while (open("/dev/null",O_RDWR) == -1 && errno == EINTR)
	    ;		/* open stdout and stderr */
	while (fcntl(1,F_DUPFD,0) == -1 && errno == EINTR)
	    ;

	if ((txtcnt > 0) && (strlen(txt) > (unsigned int)0))
	    printprog = txt;
	else
	    if ((printprog = getenv("PRINTPROG")) == (char *)NULL)
		printprog = PRINT_PGM;

	printprog = lmf_format_string(NULL, 0, printprog,
				      "%s%s", dos_table_name, unix_table_name);

	/* exec with modified environment */
	if (cp = getenv("PRINTPATH")) {
		strcpy(path, "PATH=");
		strncat(path, cp, 249);
		(void)putenv(path);
	}
	else if (!getenv("PATH"))
		(void)putenv(DEFAULT_PPATH);

	ii = 0;
	args[ii++] = "sh";
	args[ii++] = "-c";
	args[ii++] = printprog;
	args[ii++] = (char *)NULL;

	execve("/bin/sh",args,environ);
	exit(1);
    }
}

#if	!defined(lint) && !defined(DOS)
static char rcsid[] = "$Id$";
#endif
/*
 * Program:	ASCII file reading routines
 *
 *
 * Michael Seibel
 * Networks and Distributed Computing
 * Computing and Communications
 * University of Washington
 * Administration Builiding, AG-44
 * Seattle, Washington, 98195, USA
 * Internet: mikes@cac.washington.edu
 *
 * Please address all bugs and comments to "pine-bugs@cac.washington.edu"
 *
 *
 * Pine and Pico are registered trademarks of the University of Washington.
 * No commercial use of these trademarks may be made without prior written
 * permission of the University of Washington.
 * 
 * Pine, Pico, and Pilot software and its included text are Copyright
 * 1989-1996 by the University of Washington.
 * 
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this distribution.
 *
 */
/*
 * The routines in this file read and write ASCII files from the disk. All of
 * the knowledge about files are here. A better message writing scheme should
 * be used.
 */
#include        <stdio.h>
#include	<errno.h>
#include	"osdep.h"
#include	"pico.h"
#include	"estruct.h"
#include        "edef.h"


#if	defined(bsd) || defined(lnx)
extern int errno;
#endif


FILE    *ffp;                           /* File pointer, all functions. */

/*
 * Open a file for reading.
 */
ffropen(fn)
  char    *fn;
{
    int status;

    if ((status=fexist(fn, "r", NULL)) == FIOSUC)
      if ((ffp=fopen(fn, "r")) == NULL)
	status = FIOFNF;

    return (status);
}


/*
 * Write a line to the already opened file. The "buf" points to the buffer,
 * and the "nbuf" is its length, less the free newline. Return the status.
 * Check only at the newline.
 */
ffputline(buf, nbuf)
    CELL  buf[];
{
    register int    i;

    for (i = 0; i < nbuf; ++i)
       if(fputc(buf[i].c&0xFF, ffp) == EOF)
	 break;

   if(i == nbuf)
      fputc('\n', ffp);

    if (ferror(ffp)) {
        emlwrite("\007Write error: %s", errstr(errno));
	sleep(5);
        return (FIOERR);
    }

    return (FIOSUC);
}



/*
 * Read a line from a file, and store the bytes in the supplied buffer. The
 * "nbuf" is the length of the buffer. Complain about long lines and lines
 * at the end of the file that don't have a newline present. Check for I/O
 * errors too. Return status.
 */
ffgetline(buf, nbuf)
  register char   buf[];
{
    register int    c;
    register int    i;

    i = 0;

    while ((c = fgetc(ffp)) != EOF && c != '\n') {
	/*
	 * Don't blat the CR should the newline be CRLF and we're
	 * running on a unix system.  NOTE: this takes care of itself
	 * under DOS since the non-binary open turns newlines into '\n'.
	 */
	if(c == '\r'){
	    if((c = fgetc(ffp)) == EOF || c == '\n')
	      break;

	    if (i < nbuf-2)		/* Bare CR. Insert it and go on... */
	      buf[i++] = '\r';		/* else, we're up a creek */
	}

        if (i >= nbuf-2) {
	    buf[nbuf - 2] = c;	/* store last char read */
	    buf[nbuf - 1] = 0;	/* and terminate it */
            emlwrite("File has long line");
            return (FIOLNG);
        }
        buf[i++] = c;
    }

    if (c == EOF) {
        if (ferror(ffp)) {
            emlwrite("File read error");
            return (FIOERR);
        }

        if (i != 0)
	  emlwrite("File doesn't end with newline.  Adding one.", NULL);
	else
	  return (FIOEOF);
    }

    buf[i] = 0;
    return (FIOSUC);
}

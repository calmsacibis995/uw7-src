#ident	"@(#)xdm:daemon.c	1.3"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: daemon.c,v 1.8 91/05/11 15:37:38 gildea Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xos.h>

#ifdef SVR4
#include <termios.h>
#else
#include <sys/ioctl.h>
#endif

#ifdef hpux
#include <sys/ptyio.h>
#endif

extern void exit ();
extern int chdir(const char*);

BecomeOrphan ()
{
    /*
     * fork so that the process goes into the background automatically. Also
     * has a nice side effect of having the child process get inherited by
     * init (pid 1).
     */

	Debug("BecomeOrphan\n");
    if (fork ())
	exit (0);
}

BecomeDaemon ()
{
    register int i;

    /*
     * Close standard file descriptors and get rid of controlling tty
     */
    Debug("BecomeDaemon\n");

#if defined(SYSV) || defined(SVR4)
    Debug("SYSV or SVR4 BecomeDaemon\n");
    setpgrp ();
#else
    Debug("NOT SYSV or SVR4 BecomeDaemon\n");
    setpgrp (0, getpid());
#endif

    close (0); 
    close (1);
/*
 *	WIPRO : Kumar K.V.
 *	CHANGE # UNKNOWN
 *	FILE # daemon.c
 * 
 *  It is not necessary to close stderr as stderr has been dup'ed 
 *  to point to the error log file. The stdout and stdin can also 
 *  be safely dup'ed to stderr. It is therefore not necessary to 
 *  open the root inode.
 * 
 *	ENDCHANGE # UNKNOWN
 */
	/*
    close (2);
	*/
	dup2(2,0);
	dup2(2,1);

#ifndef SYSV386
    if ((i = open ("/dev/tty", O_RDWR)) >= 0) {	/* did open succeed? */
#if (defined(SYSV) || defined(SVR4)) && defined(TIOCTTY)
	int zero = 0;
	(void) ioctl (i, TIOCTTY, &zero);
#else
	(void) ioctl (i, TIOCNOTTY, (char *) 0);    /* detach, BSD style */
#endif
	(void) close (i);
    }
#endif /* !SYSV386 */

/*
 *	WIPRO : Kumar K.V.
 *	CHANGE # UNKNOWN
 *	FILE # daemon.c
 * 
 *  The following piece of code has been commented out as the standard 
 *  file descriptors have been redirected safely.
 * 
 *	ENDCHANGE # UNKNOWN
 */
    /*
     * Set up the standard file descriptors.
     */
	/*
    (void) open ("/", O_RDONLY);
    (void) dup2 (0, 1);
    (void) dup2 (0, 2);
	*/
/*
 *	WIPRO : Kumar K.V.
 *	CHANGE # UNKNOWN
 *	FILE # daemon.c
 * 
 *  Change the current working directory to / just in case the filesystem
 *  from where xdm was fired gets umounted.
 *	ENDCHANGE # UNKNOWN
 */
	(void) chdir("/");
}

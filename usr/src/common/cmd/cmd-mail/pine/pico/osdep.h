/*
 * $Id$
 *
 * Program:	Operating system dependent routines - Ultrix 4.1
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
 */

#ifndef	OSDEP_H
#define	OSDEP_H

#if	defined(dyn) || defined(AUX)
#include	<strings.h>
#else
#include	<string.h>
#endif

#undef	CTRL
#include	<signal.h>
#if	defined (ptx) || defined(sv4)
/* DYNIX/ptx signal semantics are AT&T/POSIX; the sigset() call sets
   the handler permanently, more like BSD signal(). */
#define signal(s,f) sigset (s, f)
#endif

/* signal handling is broken in the delivered SCO shells, so punt on 
   suspending the current session */
#if defined(sco) && defined(SIGTSTP)
#undef SIGTSTP
#endif

#include	<ctype.h>
#include	<sys/types.h>

#if	defined(POSIX) || defined(aix) || defined(COHERENT) || defined(isc) || defined(sv3) || defined(asv)
#include	<dirent.h>
#else
#include	<sys/dir.h>
#endif

#if	defined(asv)
#include	<sys/socket.h>  /* for fd_set */
extern struct passwd *getpwnam();
extern struct passwd *getpwuid();
#endif

#if	defined(sco)
#include	<sys/stream.h>
#include	<sys/ptem.h>
#endif

#include	<sys/stat.h>

#if	defined(asv) || defined(bsd)
extern int stat();
#endif

/*
 * 3b1 definition requirements
 */
#if	defined(ct)
#define opendir(dn)	fopen(dn, "r")
#define closedir(dirp)	fclose(dirp)
typedef struct
	{
	int	dd_fd;			/* file descriptor */
	int	dd_loc;			/* offset in block */
	int	dd_size;		/* amount of valid data */
	char	*dd_buf;		/* directory block */
	}	DIR;			/* stream data from opendir() */
#endif

/* Machine/OS definition			*/
#if	defined(ptx) || defined(sgi) || defined(sv4) || defined(sco)
#define TERMINFO	1               /* Use TERMINFO                  */
#else
#define TERMCAP		1               /* Use TERMCAP                  */
#endif

/*
 * type qsort() expects
 */
#if	defined(nxt) || defined(neb) || defined(BSDI2) || defined(gsu)
#define	QSType	  void
#define QcompType const void
#else
#if	defined(hpp)
#define	QSType	  void
#define QcompType void
#else
#define	QSType	  int
#define QcompType void
#endif
#endif

/*
 * File name separator, as a char and string
 */
#define	C_FILESEP	'/'
#define	S_FILESEP	"/"

/*
 * Place where mail gets delivered (for pico's new mail checking)
 */
#if	defined(sv3) || defined(ct) || defined(isc) || defined(AUX) || defined(sgi)
#define	MAILDIR		"/usr/mail"
#else
#define	MAILDIR		"/usr/spool/mail"
#endif


/*
 * What and where the tool that checks spelling is located.  If this is
 * undefined, then the spelling checker is not compiled into pico.
 */
#if	defined(COHERENT) || defined(AUX)
#define SPELLER         "/bin/spell"
#else
#define	SPELLER		"/usr/bin/spell"
#endif

/* memcpy() is no good for overlapping blocks.  If that's a problem, use
 * the memmove() in ../c-client
 */
#if	defined (ptx) || defined(sv4) || defined(sco) || defined(isc) || defined(AUX)
#define bcopy(a,b,s) memcpy (b, a, s)
#endif

/* memmove() is a built-in for AIX 3.2 xlc. */
#if	defined (a32) || defined(a41) || defined(COHERENT)
#define bcopy(a,b,s) memmove (b, a, s)
#endif

#if	defined(dyn)
#define	strchr	index			/* Dynix doesn't know about strchr */
#define	strrchr	rindex
#endif	/* dyn */

#ifdef	MOUSE
#define	XTERM_MOUSE_ON	"\033[?1000h"	/* DECSET with parm 1000 */
#define	XTERM_MOUSE_OFF	"\033[?1000l"	/* DECRST with parm 1000  */
#endif

#if	defined(bsd) || defined(nxt) || defined(dyn)
#ifdef	ANSI
extern char *getcwd(char *, int);
#else
extern char *getcwd();
#endif
#endif

#if	defined(COHERENT)
#define void char
#endif

/*
 * Mode passed chmod() to make tmp files exclusively user read/write-able
 */
#define	MODE_READONLY	(0600)


/*
 * Default terminal dimensions...
 */
#define	NROW	24
#define	NCOL	80

#endif	/* OSDEP_H */

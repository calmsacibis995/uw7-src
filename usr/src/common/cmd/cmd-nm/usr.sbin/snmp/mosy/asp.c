#ident	"@(#)asp.c	1.2"
#ident	"$Header$"
#ifndef lint
static char TCPID[] = "@(#)asp.c	1.1 STREAMWare TCP/IP SVR4.2 source";
#endif /* lint */
#ifndef lint
static char SysVr3TCPID[] = "@(#)asp.c	6.1 Lachman System V STREAMS TCP source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
/* asprintf.c - sprintf with errno */

/*
 *      System V STREAMS TCP - Release 5.0
 *
 *  Copyright 1992 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 * 
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*
 *
 * Contributed by NYSERNet Inc. This work was partially supported by
 * the U.S. Defense Advanced Research Projects Agency and the Rome
 * Air Development Center of the U.S. Air Force Systems Command under
 * contract number F30602-88-C-0016.
 *
 */

/*
 * All contributors disclaim all warranties with regard to this
 * software, including all implied warranties of mechantibility
 * and fitness. In no event shall any contributor be liable for
 * any special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in action of contract, negligence or other tortuous action,
 * arising out of or in connection with, the use or performance
 * of this software.
 */

/*
 * As used above, "contributor" includes, but not limited to:
 * NYSERNet, Inc.
 * Marshall T. Rose
 */

#include <stdio.h>
#include <varargs.h>

extern int errno;
#define	VSPRINTF

void
_asprintf (bp, what, ap)	/* fmt, args, ... */
register char *bp;
char   *what;
va_list	ap;
{
    register int    eindex;
    char   *fmt;

    eindex = errno;

    *bp = '\0';
    fmt = va_arg (ap, char *);

    if (fmt) {
#ifndef	VSPRINTF
	struct _iobuf iob;
#endif

#ifndef	VSPRINTF
#ifdef	pyr
	bzero ((char *) &iob, sizeof iob);
	iob._file = _NFILE;
#endif
	iob._flag = _IOWRT | _IOSTRG;
#if	!defined(vax) && !defined(pyr)
	iob._ptr = (unsigned char *) bp;
#else
	iob._ptr = bp;
#endif
	iob._cnt = BUFSIZ;
	_doprnt (fmt, ap, &iob);
	putc (NULL, &iob);
#else
	(void) vsprintf (bp, fmt, ap);
#endif
	bp += strlen (bp);

    }

    if (what) {
	if (*what) {
	    (void) sprintf (bp, " %s: ", what);
	    bp += strlen (bp);
	}
	(void) strcpy (bp, strerror (eindex));
	bp += strlen (bp);
    }

    errno = eindex;
}

void
asprintf (bp, ap)		/* what, fmt, args, ... */
char *bp;
va_list	ap;
{
    char   *what;

    what = va_arg (ap, char *);

    _asprintf (bp, what, ap);
}

#ident	"@(#)uucpspt.c	1.2"
#ident	"$Header$"
extern char *gettxt();
#ident "@(#) $Id: uucpspt.c,v 1.2 1994/11/15 01:17:13 neil Exp"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1995 Legent Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <pfmt.h>
#include <locale.h>
#include <unistd.h>
#include "../../../bnu/uucp.h"


jmp_buf Sjbuf;			/*needed by connection routines*/

/*VARARGS*/
/*ARGSUSED*/
void
assert(s1,s2,i1,s3,i2)
char *s1, *s2, *s3;
int i1, i2;
{}	/* for ASSERT in conn() */

/*ARGSUSED*/
void
logent(s1,s2)
char *s1, *s2;
{}	/* so we can load unlockf() */

void
myundial(fd)
int     fd;
{
        struct  stat    _st_buf;
        char    lockname[BUFSIZ];

        if (fd) {
                /* NOTE: undial() doesn't remove the lock file.
                 * we have to remove the lock file
                 */
                if (fstat(fd, &_st_buf) == 0 ) {
                (void) sprintf(lockname, "%s.%3.3lu.%3.3lu.%3.3lu", L_LOCK,
                        (unsigned long) major(_st_buf.st_dev),
                        (unsigned long) major(_st_buf.st_rdev),
                        (unsigned long) minor(_st_buf.st_rdev));
                }
                (void) unlink(lockname);

                undial(fd);
        }
}



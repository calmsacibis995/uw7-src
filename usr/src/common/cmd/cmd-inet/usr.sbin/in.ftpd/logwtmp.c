#ident	"@(#)logwtmp.c	1.4"

/* Copyright (c) 1988 The Regents of the University of California. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution. 3. All advertising
 * materials mentioning features or use of this software must display the
 * following acknowledgement: This product includes software developed by the
 * University of California, Berkeley and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef lint
static char sccsid[] = "@(#)logwtmp.c   5.7 (Berkeley) 2/25/91";
static char rcsid[] = "@(#)$Id$";
#endif /* not lint */

#include "config.h"

#include <sys/types.h>
#ifdef BSD
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <utmp.h>
#ifdef SVR4
#if !(defined(LINUX) || defined(__hpux)|| defined(_AIX)) 
#include <utmpx.h>
#ifndef _SCO_DS
#include <sac.h>
#endif
#endif
#endif
#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#ifdef __FreeBSD__
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "pathnames.h"

static int fd = -1;
#ifdef SVR4
static int fdx = -1;
#endif

/* Modified version of logwtmp that holds wtmp file open after first call,
 * for use with ftp (which may chroot after login, but before logout). */

#ifdef __STDC__
logwtmp(char *line, char *name, char *host)
#else
logwtmp(line,name,host)
char *line;
char *name;
char *host;
#endif
{
    struct stat buf;
    struct utmp ut;

#ifdef SVR4
#if !(defined(LINUX) || defined(__hpux) || defined (_AIX))
    struct utmpx utx;

    if (fdx < 0 && (fdx = open(WTMPX_FILE, O_WRONLY | O_APPEND, 0)) < 0) {
        syslog(LOG_ERR, "wtmpx %s %m", WTMPX_FILE);
        return;
    }

    if (fstat(fdx, &buf) == 0) {
        memset((void *)&utx, '\0', sizeof(utx));
        (void) strncpy(utx.ut_user, name, sizeof(utx.ut_user));
        (void) strncpy(utx.ut_id, "ftp", sizeof(utx.ut_id));
        (void) strncpy(utx.ut_line, line, sizeof(utx.ut_line));
        (void) strncpy(utx.ut_host, host, sizeof(utx.ut_host));
        utx.ut_syslen = strlen(utx.ut_host)+1;
        utx.ut_pid = getpid();
        (void) time (&utx.ut_tv.tv_sec);
        if (name && *name)
            utx.ut_type = USER_PROCESS;
        else
            utx.ut_type = DEAD_PROCESS;
        utx.ut_exit.e_termination = 0;
        utx.ut_exit.e_exit = 0;
        if (write(fdx, (char *) &utx, sizeof(struct utmpx)) !=
            sizeof(struct utmpx))
	  (void) ftruncate(fdx, buf.st_size);
      }
#endif
#endif

#ifdef __FreeBSD__
      if (strlen(host) > UT_HOSTSIZE) {
	    struct hostent *hp = gethostbyname(host);

	    if (hp != NULL) {
		    struct in_addr in;

		    memmove(&in, hp->h_addr, sizeof(in));
		    host = inet_ntoa(in);
	    } else
		    host = "invalid hostname";
      }
#endif

    if (fd < 0 && (fd = open(_PATH_WTMP, O_WRONLY | O_APPEND, 0)) < 0) {
        syslog(LOG_ERR, "wtmp %s %m", _PATH_WTMP);
        return;
    }
    if (fstat(fd, &buf) == 0) {
#ifdef UTMAXTYPE
        memset((void *)&ut, 0, sizeof(ut));
        (void) strncpy(ut.ut_user, name, sizeof(ut.ut_user));
#ifdef LINUX
        (void) strncpy(ut.ut_id, "", sizeof(ut.ut_id));
#else
        (void) strncpy(ut.ut_id, "ftp", sizeof(ut.ut_id));
#endif
        (void) strncpy(ut.ut_line, line, sizeof(ut.ut_line));
        ut.ut_pid = getpid();
        if (name && *name)
            ut.ut_type = USER_PROCESS;
        else
            ut.ut_type = DEAD_PROCESS;
#ifndef LINUX
        ut.ut_exit.e_termination = 0;
        ut.ut_exit.e_exit = 0;
#endif
#else
        (void) strncpy(ut.ut_line, line, sizeof(ut.ut_line));
        (void) strncpy(ut.ut_name, name, sizeof(ut.ut_name));
#endif /* UTMAXTYPE */
#ifdef HAVE_UT_UT_HOST  /* does have host in utmp */
        (void) strncpy(ut.ut_host, host, sizeof(ut.ut_host));
#endif

        (void) time(&ut.ut_time);
        if (write(fd, (char *) &ut, sizeof(struct utmp)) !=
            sizeof(struct utmp))
              (void) ftruncate(fd, buf.st_size);
    }
}

#ident	"@(#)realpath.c	1.3"

/* Copyright (c) 1993, 1994  Washington University in Saint Louis
 * All rights reserved.
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
 * Washington University in Saint Louis and its contributors. 4. Neither the
 * name of the University nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASHINGTON UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASHINGTON
 * UNIVERSITY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef lint
static char rcsid[] = "@(#)$Id$";
#endif

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <string.h>

#ifndef HAVE_SYMLINK
#define lstat stat
#endif

char *
#ifdef __STDC__
realpath(const char *pathname, char *result)
#else
realpath(pathname,result)
char *pathname;
char *result;
#endif
{
    struct stat sbuf;
    char curpath[MAXPATHLEN],
      workpath[MAXPATHLEN],
      linkpath[MAXPATHLEN],
      namebuf[MAXPATHLEN],
     *where,
     *ptr,
     *last;
    int len;

    /* check arguments! */

  
    if (result == NULL) /* result must not be null! */
      return(NULL);

    if(pathname == NULL){ /* if pathname is null, there is nothing to do */
      *result = '\0'; 
      return(NULL);
    }

    strcpy(curpath, pathname);

    if (*pathname != '/') {
		uid_t userid;
		
#ifdef HAVE_GETCWD
	if (!getcwd(workpath,MAXPATHLEN)) {
#else
        if (!getwd(workpath)) {
#endif	
		userid = geteuid();
		delay_signaling(); /* we can't allow any signals while euid==0: kinch */
		seteuid(0);
#ifdef HAVE_GETCWD
	        if (!getcwd(workpath,MAXPATHLEN)) {
#else
	        if (!getwd(workpath)) {
#endif
	            strcpy(result, ".");
		    seteuid(userid);
		    enable_signaling(); /* we can allow signals once again: kinch */
	            return (NULL);
        	}
		seteuid(userid);
		enable_signaling(); /* we can allow signals once again: kinch */
	}
    } else
        *workpath = '\0';

    /* curpath is the path we're still resolving      */
    /* linkpath is the path a symbolic link points to */
    /* workpath is the path we've resolved            */

  loop:
    where = curpath;
    while (*where != '\0') {
        if (!strcmp(where, ".")) {
            where++;
            continue;
        }
        /* deal with "./" */
        if (!strncmp(where, "./", 2)) {
            where += 2;
            continue;
        }
        /* deal with "../" */
        if (!strncmp(where, "../", 3)) {
            where += 3;
            ptr = last = workpath;
            while (*ptr != '\0') {
                if (*ptr == '/')
                    last = ptr;
                ptr++;
            }
            *last = '\0';
            continue;
        }
        ptr = strchr(where, '/');
        if (ptr == (char *)NULL)
            ptr = where + strlen(where) - 1;
        else
            *ptr = '\0';

        strcpy(namebuf, workpath);
        for (last = namebuf; *last; last++)
            continue;
        if ((last == namebuf) || (*--last != '/'))
            strcat(namebuf, "/");
        strcat(namebuf, where);

        where = ++ptr;
        if (lstat(namebuf, &sbuf) == -1) {
            strcpy(result, namebuf);
            return (NULL);
        }
		/* was IFLNK */
#ifdef HAVE_SYMLINK
        if ((sbuf.st_mode & S_IFMT) == S_IFLNK) {
            len = readlink(namebuf, linkpath, MAXPATHLEN);
            if (len == 0) {
                strcpy(result, namebuf);
                return (NULL);
            }
            *(linkpath + len) = '\0';   /* readlink doesn't null-terminate
                                         * result */
            if (*linkpath == '/')
                *workpath = '\0';
            if (*where) {
                strcat(linkpath, "/");
                strcat(linkpath, where);
            }
            strcpy(curpath, linkpath);
            goto loop;
        }
#endif
        if ((sbuf.st_mode & S_IFDIR) == S_IFDIR) {
            strcpy(workpath, namebuf);
            continue;
        }
        if (*where) {
            strcpy(result, namebuf);
            return (NULL);      /* path/notadir/morepath */
        } else
            strcpy(workpath, namebuf);
    }
    strcpy(result, workpath);
    return (result);
}

















#ident	"@(#)acl.c	1.2"

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
char * rcsid = "$Id$";
#endif
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "pathnames.h"
#include "extensions.h"

char *aclbuf = NULL;
static struct aclmember *aclmembers;

/*************************************************************************/
/* FUNCTION  : getaclentry                                               */
/* PURPOSE   : Retrieve a named entry from the ACL                       */
/* ARGUMENTS : pointer to the keyword and a handle to the acl members    */
/* RETURNS   : pointer to the acl member containing the keyword or NULL  */
/*************************************************************************/

struct aclmember *
#ifdef __STDC__
getaclentry(char *keyword, struct aclmember **next)
#else
getaclentry(keyword,next)
char *keyword;
struct aclmember **next;
#endif
{
    do {
        if (!*next)
            *next = aclmembers;
        else
            *next = (*next)->next;
    } while (*next && strcmp((*next)->keyword, keyword));

    return (*next);
}

/*************************************************************************/
/* FUNCTION  : parseacl                                                  */
/* PURPOSE   : Parse the acl buffer into its components                  */
/* ARGUMENTS : A pointer to the acl file                                 */
/* RETURNS   : nothing                                                   */
/*************************************************************************/

void
#ifdef __STDC__
parseacl(void)
#else
parseacl()
#endif
{
    char *ptr,
     *aclptr = aclbuf,
     *line;
    int cnt;
    struct aclmember *member,
     *acltail;

    if (!aclbuf || !(*aclbuf))
        return;

    aclmembers = (struct aclmember *) NULL;
    acltail = (struct aclmember *) NULL;

    while (*aclptr != '\0') {
        line = aclptr;
        while (*aclptr && *aclptr != '\n')
            aclptr++;
        *aclptr++ = (char) NULL;

        /* deal with comments */
        if ((ptr = strchr(line, '#')) != NULL)
            /* allowed escaped '#' chars for path-filter (DiB) */
            if (*(ptr-1) != '\\')
                *ptr = '\0';

        ptr = strtok(line, " \t");
        if (ptr) {
            member = (struct aclmember *) calloc(1, sizeof(struct aclmember));

            (void) strcpy(member->keyword, ptr);
            cnt = 0;
            while ((ptr = strtok(NULL, " \t")) != NULL) {
		if (cnt >= MAXARGS) {
		    syslog(LOG_ERR,
			"Too many args (>%d) in ftpaccess: %s %s %s %s %s ...",
			MAXARGS - 1, member->keyword, member->arg[0],
			member->arg[1], member->arg[2], member->arg[3]);
		    break;
		}
                member->arg[cnt++] = ptr;
	    }
            if (acltail)
                acltail->next = member;
            acltail = member;
            if (!aclmembers)
                aclmembers = member;
        }
    }
}

/*************************************************************************/
/* FUNCTION  : readacl                                                   */
/* PURPOSE   : Read the acl into memory                                  */
/* ARGUMENTS : The pathname of the acl                                   */
/* RETURNS   : 0 if error, 1 if no error                                 */
/*************************************************************************/

int
#ifdef __STDC__
readacl(char *aclpath)
#else
readacl(aclpath)
char *aclpath;
#endif
{
    FILE *aclfile;
    struct stat finfo;
    extern int use_accessfile;

    if (!use_accessfile)
        return (0);

    if ((aclfile = fopen(aclpath, "r")) == NULL) {
        syslog(LOG_ERR, "cannot open access file %s: %s", aclpath,
               strerror(errno));
        return (0);
    }
    if (fstat(fileno(aclfile), &finfo) != 0) {
        syslog(LOG_ERR, "cannot fstat access file %s: %s", aclpath,
               strerror(errno));
        (void) fclose(aclfile);
        return (0);
    }
    if (finfo.st_size == 0) {
        aclbuf = (char *) calloc(1, 1);
    } else {
        if (!(aclbuf = (char *)malloc((unsigned) finfo.st_size + 1))) {
            syslog(LOG_ERR, "could not malloc aclbuf (%d bytes)", finfo.st_size + 1);
            (void) fclose(aclfile);
            return (0);
        }
        if (!fread(aclbuf, (size_t) finfo.st_size, 1, aclfile)) {
            syslog(LOG_ERR, "error reading acl file %s: %s", aclpath,
                   strerror(errno));
            aclbuf = NULL;
            (void) fclose(aclfile);
            return (0);
        }
        *(aclbuf + finfo.st_size) = '\0';
    }
    (void) fclose(aclfile);
    return (1);
}

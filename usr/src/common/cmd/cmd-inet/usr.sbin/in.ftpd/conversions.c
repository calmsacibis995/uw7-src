#ident	"@(#)conversions.c	1.4"

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

#include "config.h"
#ifndef lint
static char rcsid[] = "$Id$";
#endif /* not lint */

#include <stdio.h>
#include <errno.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "conversions.h"
#include "extensions.h"
#include "pathnames.h"

/*************************************************************************/
/* FUNCTION  : readconv                                                  */
/* PURPOSE   : Read the conversions into memory                          */
/* ARGUMENTS : The pathname of the conversion file                       */
/* RETURNS   : 0 if error, 1 if no error                                 */
/*************************************************************************/

char *convbuf = NULL;
struct convert *cvtptr;

struct str2int {
    char *string;
    int value;
};

struct str2int c_list[] =
{"T_REG", T_REG,
 "T_ASCII", T_ASCII,
 "T_DIR", T_DIR,
 "O_COMPRESS", O_COMPRESS,
 "O_UNCOMPRESS", O_UNCOMPRESS,
 "O_TAR", O_TAR,
 NULL, 0,};

int
#ifdef __STDC__
conv(char *str)
#else
conv(str)
char *str;
#endif
{
    int rc = 0;
    int counter;

    /* check for presence of ALL items in string... */

    for (counter = 0; c_list[counter].string; ++counter)
        if (strstr(str, c_list[counter].string))
            rc = rc | c_list[counter].value;
    return (rc);
}

int
#ifdef __STDC__
readconv(char *convpath)
#else
readconv(convpath)
char *convpath;
#endif
{
    FILE *convfile;
    struct stat finfo;

    if ((convfile = fopen(convpath, "r")) == NULL) {
        if (errno != ENOENT)
            syslog(LOG_ERR, "cannot open conversion file %s: %s",
                   convpath, strerror(errno));
        return (0);
    }
    if (fstat(fileno(convfile), &finfo) != 0) {
        syslog(LOG_ERR, "cannot fstat conversion file %s: %s", convpath,
               strerror(errno));
        (void) fclose(convfile);
        return (0);
    }
    if (finfo.st_size == 0) {
        convbuf = (char *) calloc(1, 1);
    } else {
        if (!(convbuf = (char *) malloc((unsigned) finfo.st_size + 1))) {
            syslog(LOG_ERR, "could not malloc convbuf (%d bytes)", finfo.st_size + 1);
            (void) fclose(convfile);
            return (0);
        }
        if (!fread(convbuf, (size_t) finfo.st_size, 1, convfile)) {
            syslog(LOG_ERR, "error reading conv file %s: %s", convpath,
                   strerror(errno));
            convbuf = NULL;
            (void) fclose(convfile);
            return (0);
        }
        *(convbuf + finfo.st_size) = '\0';
    }
    (void) fclose(convfile);
    return (1);
}

void
#ifdef __STDC__
parseconv(void)
#else
parseconv()
#endif
{
    char *ptr;
    char *convptr = convbuf,
     *line;
    char *argv[8],
     *p,
     *val;
    struct convert *cptr,
     *cvttail = (struct convert *) NULL;
    int n;

    if (!convbuf || !(*convbuf))
        return;

    /* read through convbuf, stripping comments. */
    while (*convptr != '\0') {
        line = convptr;
        while (*convptr && *convptr != '\n')
            convptr++;
        *convptr++ = '\0';

        /* deal with comments */
        if ((ptr = strchr(line, '#')) != NULL)
            *ptr = '\0';

        if (*line == '\0')
            continue;

        /* parse the lines... */
        for (n = 0, p = line; n < 8 && p != NULL; n++) {
            while ((val = (char *) strsep(&p, ":\n")) != NULL &&
                   *val == '\0' && p != NULL)
                ;
            argv[n] = val;
            if (argv[n][0] == ' ')
                argv[n] = NULL;
        }
        /* check their were 8 fields, if not skip the line... */
        if (n != 8 || p != NULL)
            continue;

        /* add element to end of list */
        cptr = (struct convert *) calloc(1, sizeof(struct convert));

        if (cvttail)
            cvttail->next = cptr;
        cvttail = cptr;
        if (!cvtptr)
            cvtptr = cptr;

        cptr->stripprefix = (char *) argv[0];
        cptr->stripfix = (char *) argv[1];
        cptr->prefix = (char *) argv[2];
        cptr->postfix = (char *) argv[3];
        cptr->external_cmd = (char *) argv[4];
        cptr->types = conv((char *) argv[5]);
        cptr->options = conv((char *) argv[6]);
        cptr->name = (char *) argv[7];
    }
}

void
#ifdef __STDC__
conv_init(void)
#else
conv_init()
#endif
{
#ifdef VERBOSE
    struct convert *cptr;

#endif

    if ((readconv(_PATH_CVT)) < 0)
        return;
    parseconv();
}

#ident	"@(#)ftpshut.c	1.3"

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

/* ftpshut 
 * ======= 
 * creates the ftpd shutdown file.
 */

#include "config.h"

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#include "pathnames.h"

#ifdef	INTL
#  include <locale.h>
#  include "ftpshut_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num, str)	(str)
#endif	/* INTL */

#define  WIDTH  70

int denyoffset = 10;            /* default deny time   */
int discoffset = 5;             /* default disc time   */

extern version[];

void
#ifdef __STDC__
massage(char *buf)
#else
massage(buf)
char *buf;
#endif
{
    char *sp = NULL;
    char *ptr;
    int i = 0;
    int j = 0;

    ptr = buf;

    while (*ptr++ != '\0') {
        ++i;

        /* if we have a space, keep track of where and at what "count" */

        if (*ptr == ' ') {
            sp = ptr;
            j = i;
        }
        /* magic cookies... */

        if (*ptr == '%') {
            ++ptr;
            switch (*ptr) {
            case 'r':
            case 's':
            case 'd':
            case 'T':
                i = i + 24;
                break;
            case '\n':
                i = 0;
                break;
            case 'C':
            case 'R':
            case 'L':
            case 'U':
                i = i + 10;
                break;
            case 'M':
            case 'N':
                i = i + 3;
                break;
            case '\0':
                return;
                break;
            default:
                i = i + 1;
                break;
            }
        }
        /* break up the long lines... */

        if ((i >= WIDTH) && (sp != NULL)) {
            *sp = '\n';
            sp = NULL;
            i = i - j;
        }
    }
}

int
#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc,argv)
int argc;
char **argv;
#endif
{
    time_t c_time;
    struct tm *tp;

    char buf[BUFSIZ];

    int c;
    extern int optind;
    extern char *optarg;

    FILE *fp;
    FILE *accessfile;
    char *aclbuf,
     *myaclbuf,
     *crptr;
    char *sp = NULL;
    char linebuf[1024];
    struct stat finfo;

    struct passwd *pwent;

#ifdef	INTL
    setlocale(LC_ALL, "");
    catd = catopen(MF_FTPSHUT, MC_FLAGS);
#endif	/* INTL */

    while ((c = getopt(argc, argv, "vl:d:")) != EOF) {
        switch (c) {
        case 'l':
            denyoffset = atoi(optarg);
            break;
        case 'd':
            discoffset = atoi(optarg);
            break;
		case 'v':
			fprintf(stderr, "%s\n", version);
			exit(0);
        default:
            fprintf(stderr, MSGSTR(MSG_USAGE_NOW,
                "Usage: %s [-d min] [-l min] now [\"message\"]\n"), argv[0]);
            fprintf(stderr, MSGSTR(MSG_USAGE_DD,
                "       %s [-d min] [-l min] +dd [\"message\"]\n"), argv[0]);
            fprintf(stderr, MSGSTR(MSG_USAGE_HHMM,
               "       %s [-d min] [-l min] HHMM [\"message\"]\n"), argv[0]);
            exit(-1);
        }
    }

    if ((accessfile = fopen(_PATH_FTPACCESS, "r")) == NULL) {
        if (errno != ENOENT)
            perror(MSGSTR(MSG_OPEN, "ftpshut: could not open() access file"));
        exit(1);
    }
    if (stat(_PATH_FTPACCESS, &finfo)) {
        perror(MSGSTR(MSG_STAT, "ftpshut: could not stat() access file"));
        exit(1);
    }
    if (finfo.st_size == 0) {
        printf(MSGSTR(MSG_NO_SHUTDOWN_PATH,
                        "ftpshut: no service shutdown path defined\n"));
        exit(0);
    } else {
        if (!(aclbuf = (char *) malloc(finfo.st_size + 1))) {
            perror(MSGSTR(MSG_MALLOC_ACLBUF,
                        "ftpshut: could not malloc aclbuf"));
            exit(1);
        }
        fread(aclbuf, finfo.st_size, 1, accessfile);
        *(aclbuf + finfo.st_size) = '\0';
    }

    myaclbuf = aclbuf;
    while (*myaclbuf != '\0') {
        if (strncasecmp(myaclbuf, "shutdown", 8) == 0) {
            for (crptr = myaclbuf; *crptr++ != '\n';) ;
            *--crptr = '\0';
            strcpy(linebuf, myaclbuf);
            *crptr = '\n';
            (void) strtok(linebuf, " \t");  /* returns "shutdown" */
            sp = strtok(NULL, " \t");   /* returns shutdown path */
        }
        while (*myaclbuf && *myaclbuf++ != '\n') ;
    }

    /* three cases 
     * -- now 
     * -- +ddd 
     * -- HHMM 
     */

    c = -1;

    if (optind < argc) {
        if (!strcasecmp(argv[optind], "now")) {
            c_time = time(0);
            tp = localtime(&c_time);
        } else if ((*(argv[optind])) == '+') {
            c_time = time(0);
            c_time += 60 * atoi(++(argv[optind]));
            tp = localtime(&c_time);
        } else if ((c = atoi(argv[optind])) >= 0) {
            c_time = time(0);
            tp = localtime(&c_time);
            tp->tm_hour = c / 100;
            tp->tm_min = c % 100;

            if ((tp->tm_hour > 23) || (tp->tm_min > 59)) {
                fprintf(stderr, MSGSTR(MSG_ILLEGAL_TIME,
                        "Illegal time format.\n"));
                return(1);
            }
        }
    }
    if (c_time <= 0) {
        fprintf(stderr, MSGSTR(MSG_USAGE_NOW,
                "Usage: %s [-d min] [-l min] now [\"message\"]\n"), argv[0]);
        fprintf(stderr, MSGSTR(MSG_USAGE_DD,
                "       %s [-d min] [-l min] +dd [\"message\"]\n"), argv[0]);
        fprintf(stderr, MSGSTR(MSG_USAGE_HHMM,
                "       %s [-d min] [-l min] HHMM [\"message\"]\n"), argv[0]);
        return(1);
    }
    /* do we have a shutdown message? */

    if (++optind < argc)
        strcpy(buf, argv[optind++]);
    else
        strcpy(buf, MSGSTR(MSG_SHUTDOWN_AT, "System shutdown at %s"));

    massage(buf);

    if ( sp == NULL ) {
        fprintf(stderr, MSGSTR(MSG_NO_SHUTDOWN_FILE,
                "No shutdown file defined in ftpaccess file.\n"));
        return(1);
    }
    
    if ( (fp = fopen(sp, "w")) == NULL )  {
        perror(MSGSTR(MSG_OPEN_SHUTDOWN, "Couldn't open shutdown file"));
        return(1);
    }

    fprintf(fp, "%.4d %.2d %.2d %.2d %.2d %.4d %.4d\n",
            (tp->tm_year) + 1900,
            tp->tm_mon,
            tp->tm_mday,
            tp->tm_hour,
            tp->tm_min,
            denyoffset,
            discoffset);
    fprintf(fp, "%s\n", buf);
    fclose(fp);
}

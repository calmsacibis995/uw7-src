#ident	"@(#)ftpcount.c	1.4"

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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#include "pathnames.h"
#include "extensions.h"

#if defined(SVR4) || defined(ISC)
#include <fcntl.h>
#endif

#ifdef	INTL
#  include <locale.h>
#  include "ftpcount_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num, str)	(str)
#endif	/* INTL */

struct c_list {
    char *class;
    struct c_list *next;
};

extern char version[];

char *progname;

/*************************************************************************/
/* FUNCTION  : parse_time                                                */
/* PURPOSE   : Check a single valid-time-string against the current time */
/*             and return whether or not a match occurs.                 */
/* ARGUMENTS : a pointer to the time-string                              */
/*************************************************************************/

int
#ifdef __STDC__
parsetime(char *whattime)
#else
parsetime(whattime)
char *whattime;
#endif
{
    static char *days[] =
    {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", "Wk"};
    time_t clock;
    struct tm *curtime;
    int wday,
      start,
      stop,
      ltime,
      validday,
      loop,
      match;

    (void) time(&clock);
    curtime = localtime(&clock);
    wday = curtime->tm_wday;
    validday = 0;
    match = 1;

    while (match && isalpha(*whattime) && isupper(*whattime)) {
        match = 0;
        for (loop = 0; loop < 8; loop++) {
            if (strncmp(days[loop], whattime, 2) == 0) {
                whattime += 2;
                match = 1;
                if ((wday == loop) || ((loop == 7) && wday && (wday < 6)))
                    validday = 1;
            }
        }
    }

    if (!validday) {
        if (strncmp(whattime, "Any", 3) == 0) {
            validday = 1;
            whattime += 3;
        }
        else
            return (0);
    }

    if (sscanf(whattime, "%d-%d", &start, &stop) == 2) {
        ltime = curtime->tm_min + 100 * curtime->tm_hour;
        if ((start < stop) && ((ltime > start) && ltime < stop))
            return (1);
        if ((start > stop) && ((ltime > start) || ltime < stop))
            return (1);
    } else
        return (1);

    return (0);
}

/*************************************************************************/
/* FUNCTION  : validtime                                                 */
/* PURPOSE   : Break apart a set of valid time-strings and pass them to  */
/*             parse_time, returning whether or not ANY matches occurred */
/* ARGUMENTS : a pointer to the time-string                              */
/*************************************************************************/

int
#ifdef __STDC__
validtime(char *ptr)
#else
validtime(ptr)
char *ptr;
#endif
{
    char *nextptr;
    int good;

    while (1) {
        nextptr = strchr(ptr, '|');
        if (strchr(ptr, '|') == NULL)
            return (parsetime(ptr));
        *nextptr = '\0';
        good = parsetime(ptr);
        *nextptr++ = '|';       /* gotta restore the | or things get skipped! */
        if (good)
            return (1);
        ptr = nextptr;
    }
}

int
#ifdef __STDC__
acl_getlimit(char *aclbuf, char *class)
#else
acl_getlimit(aclbuf,class)
char *aclbuf;
char *class;
#endif
{
    char *crptr,
     *ptr,
      linebuf[1024];
    int limit;

    while (*aclbuf != '\0') {
        if (strncasecmp(aclbuf, "limit", 5) == 0) {
            for (crptr = aclbuf; *crptr++ != '\n';) ;
            *--crptr = '\0';
            strcpy(linebuf, aclbuf);
            *crptr = '\n';
            (void) strtok(linebuf, " \t");  /* returns "limit" */
            if (strcmp(class, strtok(NULL, " \t")) == 0) {
                limit = atoi(strtok(NULL, " \t"));  /* returns limit <n> */
                if ((ptr = strtok(NULL, " \t")) && validtime(ptr))
                    return (limit);
            }
        }
        while (*aclbuf && *aclbuf++ != '\n') ;
    }

    return (-1);
}

int
#ifdef __STDC__
acl_countusers(char *class)
#else
acl_countusers(class)
char *class;
#endif
{
    int pidfd,
      count,
      stat,
      which;
    char pidfile[1024];
    char line[1024];
    pid_t buf[MAXUSERS];
    FILE *ZeFile;
#ifndef HAVE_FLOCK
struct flock arg;
#endif

    sprintf(pidfile, _PATH_PIDNAMES, class);
    pidfd = open(pidfile, O_RDONLY, 0644);
    if (pidfd == -1) {
        return (0);
    }
    lseek(pidfd, 0, L_SET);

    count = 0;

    if (read(pidfd, buf, sizeof(buf)) == sizeof(buf)) {
        for (which = 0; which < MAXUSERS; which++)
        if (buf[which]) {
            stat = kill(buf[which], SIGCONT);
            if (((stat == -1) && (errno == EPERM)) || !stat) {
                if (strcmp(progname,"ftpcount")) {
#if defined(SVR4)
#ifdef AIX
                    sprintf(line,"/bin/ps %d",buf[which]);
#elif defined(sun)
                    sprintf(line,"/usr/ucb/ps auxww %d",buf[which]);
#else
                    sprintf(line,"/bin/ps -f -p %d",buf[which]);
#endif
#elif defined(M_UNIX)
                    sprintf(line,"/bin/ps -f -p %d",buf[which]);
#else
                    sprintf(line,"/bin/ps %d",buf[which]);
#endif
                    ZeFile = popen(line, "r");
                    fgets(line, 1024, ZeFile);
                    fgets(line, 1024, ZeFile);
                    printf("%s",line);
                    pclose(ZeFile);
                }
                count++;
            }
        }
    }
#ifdef HAVE_FLOCK
    flock(pidfd, LOCK_UN);
#else
    arg.l_type = F_UNLCK; arg.l_whence = arg.l_start = arg.l_len = 0;
    fcntl(pidfd, F_SETLK, &arg);
#endif
    close(pidfd);

    return (count);
}

void
#ifdef __STDC__
new_list(struct c_list **list)
#else
new_list(list)
struct c_list **list;
#endif
{
    (*list) = (struct c_list *) malloc(sizeof(struct c_list));

    (*list)->next = NULL;
}

int
#ifdef __STDC__
add_list(char *class, struct c_list **list)
#else
add_list(class,list)
char *class;
struct c_list **list;
#endif
{
    struct c_list *cp;

    for (cp = (*list)->next; cp; cp = cp->next) {
        if (!strcmp(cp->class, class))
            return (-1);
    }

    cp = (struct c_list *) malloc(sizeof(struct c_list));

    cp->class = (char *) malloc(strlen(class) + 1);
    strcpy(cp->class, class);
    cp->next = (*list)->next;
    (*list)->next = cp;
    return (1);
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
    FILE *accessfile;
    char class[80],
      linebuf[1024],
     *aclbuf,
     *myaclbuf,
     *crptr;
    int limit;
    struct stat finfo;
    struct c_list *list;

#ifdef	INTL
	setlocale(LC_ALL, "");
	catd = catopen(MF_FTPCOUNT, MC_FLAGS);
#endif	/* INTL */

	if (progname = strrchr(argv[0], '/'))  ++progname;
	else  progname = argv[0];

	if (argc > 1) {
		fprintf(stderr, "%s\n", version);
		exit(0);
	}

    if ((accessfile = fopen(_PATH_FTPACCESS, "r")) == NULL) {
        if (errno != ENOENT)
            perror(MSGSTR(MSG_OPEN, "ftpcount: could not open() access file"));
        exit(1);
    }
    if (stat(_PATH_FTPACCESS, &finfo)) {
        perror(MSGSTR(MSG_STAT, "ftpcount: could not stat() access file"));
        exit(1);
    }
    if (finfo.st_size == 0) {
        printf(MSGSTR(MSG_NO_CLASSES,
            "%s: no service classes defined, no usage count kept\n"), progname);
        exit(0);
    } else {
        if (!(aclbuf = (char *) malloc(finfo.st_size + 1))) {
            perror(MSGSTR(MSG_MALLOC_ACLBUF,
                        "ftpcount: could not malloc aclbuf"));
            exit(1);
        }
        fread(aclbuf, finfo.st_size, 1, accessfile);
        *(aclbuf + finfo.st_size) = '\0';
    }

    (void) new_list(&list);
    myaclbuf = aclbuf;
    while (*myaclbuf != '\0') {
        if (strncasecmp(myaclbuf, "class", 5) == 0) {
            for (crptr = myaclbuf; *crptr++ != '\n';) ;
            *--crptr = '\0';
            strcpy(linebuf, myaclbuf);
            *crptr = '\n';
            (void) strtok(linebuf, " \t");  /* returns "class" */
            strcpy(class, strtok(NULL, " \t")); /* returns class name */
            if ((add_list(class, &list)) < 0) {
                /* we have a class with multiple "class..." lines so, only
                 * display one count... */
                ;
            } else {
                limit = acl_getlimit(myaclbuf, class);
            if (strcmp(progname,"ftpcount")) {
                printf(MSGSTR(MSG_CLASS, "Service class %s: \n"), class);
                printf(MSGSTR(MSG_USERS, "   - %3d users (%3d maximum)\n\n"),
            acl_countusers(class), limit);
        }
        else
                printf(MSGSTR(MSG_CLASS_USERS,
                        "Service class %-20.20s - %3d users (%3d maximum)\n"),
                       class, acl_countusers(class), limit);
            }
        }
        while (*myaclbuf && *myaclbuf++ != '\n') ;
    }
    exit(0);
}

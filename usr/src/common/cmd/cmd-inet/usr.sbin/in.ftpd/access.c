#ident	"@(#)access.c	1.5"

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
#endif /* not lint */

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef SYSSYSLOG
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif
#include <time.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#include "pathnames.h"
#include "extensions.h"

#if defined(SVR4) || defined(ISC)
#include <fcntl.h>
#endif

extern char remotehost[],
  remoteaddr[],
 *aclbuf;
extern int nameserved,
  anonymous,
  guest,
  use_accessfile;
char Shutdown[MAXPATHLEN];
#define MAXLINE	80
static  char  incline[MAXLINE];
int pidfd = -1;

extern int fnmatch();

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
    static char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", "Wk"};
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
                if ((wday == loop) || ((loop == 7) && wday && (wday < 6))) {
                    validday = 1;
                }
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
        /* gotta restore the | or things get skipped! */
        *nextptr++ = '|';
        if (good)
            return (1);
        ptr = nextptr;
    }
}

/*************************************************************************/
/* FUNCTION  : hostmatch                                                 */
/* PURPOSE   : Match remote hostname or address against a glob string    */
/* ARGUMENTS : The string to match                                       */
/* RETURNS   : 0 if no match, 1 if a match occurs                        */
/*************************************************************************/

#ifdef __STDC__
hostmatch(char *addr)
#else
hostmatch(addr)
char *addr;
#endif
{
    FILE  *incfile;
    char  *ptr;
    int   found = 0;

    if (addr == NULL) return(0);

    if (isdigit(*addr))
        return(!fnmatch(addr, remoteaddr, NULL));
    else if (*addr == '/') {
        /*
         * read addrglobs from named path using similar format as addrglobs
         * in access file
         */
        if ((incfile = fopen(addr, "r")) == NULL) {
            if (errno != ENOENT) syslog(LOG_ERR,
		        "cannot open addrglob file %s: %s", addr, strerror(errno));
            return(0);
        }
        
        while (!found && (fgets(incline, MAXLINE, incfile) != NULL)) {
            ptr = strtok(incline, " \t\n");
            if (ptr && hostmatch(ptr))
            	found = 1;
            while (!found && ((ptr = strtok(NULL, " \t\n")) != NULL)) {
                if (ptr && hostmatch(ptr))
                    found = 1;
            }
        }
        fclose(incfile);
        return(found);
    }
    else
      {   /* match a hostname or hostname glob */
        char *addrncase,*hostncase;
        int i,j;
        /* must convert both to lower case for match */
        if ((addrncase = (char *)malloc(strlen(addr)+1)) == NULL)
          return(0);
        if ((hostncase = (char *)malloc(strlen(remotehost)+1)) == NULL){
	  free(addrncase);
          return(0);
	}
        j = strlen(addr) + 1;
        for (i = 0;i < j; i++)
          addrncase[i] = tolower(addr[i]);
        j = strlen(remotehost) + 1;
        for (i = 0;i < j; i++)
          hostncase[i] = tolower(remotehost[i]);
        found = !fnmatch(addrncase, hostncase, NULL); 
        free(addrncase);
        free(hostncase);
        return(found);
      }

}

/*************************************************************************/
/* FUNCTION  : acl_guestgroup                                            */
/* PURPOSE   : If the real user is a member of any of the listed groups, */
/*             return 1.  Otherwise return 0.                            */
/* ARGUMENTS : pw, a pointer to the passwd struct for the user           */
/*************************************************************************/

int
#ifdef __STDC__
acl_guestgroup(struct passwd *pw)
#else
acl_guestgroup(pw)
struct passwd *pw;
#endif
{
    struct aclmember *entry = NULL;
    struct group *grp;
    int which;
    char **member;

    /* guestgroup <group> [<group> ...] */
    while (getaclentry("guestgroup", &entry)) {
        for (which = 0; (which < MAXARGS) && ARG[which]; which++) {
            if (!(grp = getgrnam(ARG[which])))
                continue;
            if (pw->pw_gid == grp->gr_gid)
                return (1);
            for (member = grp->gr_mem; *member; member++) {
                if (!strcmp(*member, pw->pw_name))
                    return (1);
            }
        }
    }
    return (0);
}

/*************************************************************************/
/* FUNCTION  : acl_autogroup                                             */
/* PURPOSE   : If the guest user is a member of any of the classes in    */
/*             the autogroup comment, cause a setegid() to the specified */
/*             group.                                                    */
/* ARGUMENTS : pw, a pointer to the passwd struct for the user           */
/*************************************************************************/

void
#ifdef __STDC__
acl_autogroup(struct passwd *pw)
#else
acl_autogroup(pw)
struct passwd *pw;
#endif
{
    char class[1024];

    struct aclmember *entry = NULL;
    struct group *grp;
    int which;

    (void) acl_getclass(class);

    /* autogroup <group> <class> [<class> ...] */
    while (getaclentry("autogroup", &entry)) {
        if (!ARG0 || !ARG1)
            continue;
        if ((grp = getgrnam(ARG0))) {
            for (which = 1; (which < MAXARGS) && ARG[which]; which++) {
                if (!strcmp(ARG[which], class)) {
                    pw->pw_gid = grp->gr_gid;
                    endgrent();
                    return;
                }
            }
        } else
            syslog(LOG_ERR, "autogroup: set group %s not found", ARG0);
        endgrent();
    }
}

/*************************************************************************/
/* FUNCTION  : acl_setfunctions                                          */
/* PURPOSE   : Scan the ACL buffer and determine what logging to perform */
/*             for this user, and whether or not user is allowed to use  */
/*             the automatic TAR and COMPRESS functions.                 */
/* ARGUMENTS : pointer to buffer to class name, pointer to ACL buffer    */
/*************************************************************************/

void
#ifdef __STDC__
acl_setfunctions(void)
#else
acl_setfunctions()
#endif
{
    char class[1024];

    extern int log_incoming_xfers,
      log_outbound_xfers,
      mangleopts,
      log_commands,
      lgi_failure_threshold;

    struct aclmember *entry = NULL;

    int l_compress,
      l_tar,
      inbound = 0,
      outbound = 0,
      which,
      set;

    log_incoming_xfers = 0;
    log_outbound_xfers = 0;
    log_commands = 0;

    memset((void *)&class[0], 0, sizeof(class));
 
    (void) acl_getclass(class);

    entry = (struct aclmember *) NULL;
    if (getaclentry("loginfails", &entry) && ARG0 != NULL) {
        lgi_failure_threshold = atoi(ARG0);
    }
#ifndef NO_PRIVATE
    entry = (struct aclmember *) NULL;
    if (getaclentry("private", &entry) && !strcmp(ARG0, "yes"))
        priv_setup(_PATH_PRIVATE);
#endif /* !NO_PRIVATE */

    entry = (struct aclmember *) NULL;
    set = 0;
    while (!set && getaclentry("compress", &entry)) {
        l_compress = 0;
        if (!strcasecmp(ARG0, "yes"))
            l_compress = 1;
        for (which = 1; (which < MAXARGS) && ARG[which]; which++) {
            if (!fnmatch(ARG[which], class, NULL)) {
                mangleopts |= l_compress * (O_COMPRESS | O_UNCOMPRESS);
                set = 1;
            }
        }
    }

    entry = (struct aclmember *) NULL;
    set = 0;
    while (!set && getaclentry("tar", &entry)) {
        l_tar = 0;
        if (!strcasecmp(ARG0, "yes"))
            l_tar = 1;
        for (which = 1; (which < MAXARGS) && ARG[which]; which++) {
            if (!fnmatch(ARG[which], class, NULL)) {
                mangleopts |= l_tar * O_TAR;
                set = 1;
            }
        }
    }

    /* plan on expanding command syntax to include classes for each of these */

    entry = (struct aclmember *) NULL;
    while (getaclentry("log", &entry)) {
        if (!strcasecmp(ARG0, "commands")) {
            if (anonymous && strcasestr(ARG1, "anonymous"))
                log_commands = 1;
            if (guest && strcasestr(ARG1, "guest"))
                log_commands = 1;
            if (!guest && !anonymous && strcasestr(ARG1, "real"))
                log_commands = 1;
        }
        if (!strcasecmp(ARG0, "transfers")) {
            set = 0;
            if (strcasestr(ARG1, "anonymous") && anonymous)
                set = 1;
            if (strcasestr(ARG1, "guest") && guest)
                set = 1;
            if (strcasestr(ARG1, "real") && !guest && !anonymous)
                set = 1;
            if (strcasestr(ARG2, "inbound"))
                inbound = 1;
            if (strcasestr(ARG2, "outbound"))
                outbound = 1;
            if (set)
                log_incoming_xfers = inbound;
            if (set)
                log_outbound_xfers = outbound;
        }
    }
}

/*************************************************************************/
/* FUNCTION  : acl_getclass                                              */
/* PURPOSE   : Scan the ACL buffer and determine what class user is in   */
/* ARGUMENTS : pointer to buffer to class name, pointer to ACL buffer    */
/*************************************************************************/

int
#ifdef __STDC__
acl_getclass(char *classbuf)
#else
acl_getclass(classbuf)
char *classbuf;
#endif
{
    int which;
    struct aclmember *entry = NULL;

    while (getaclentry("class", &entry)) {
        if (ARG0)
            strcpy(classbuf, ARG0);

        for (which = 2; (which < MAXARGS) && ARG[which]; which++) {
            if (anonymous && strcasestr(ARG1, "anonymous") &&
                hostmatch(ARG[which]))
                return (1);

            if (guest && strcasestr(ARG1, "guest") && hostmatch(ARG[which]))
                return (1);

            if (!guest && !anonymous && strcasestr(ARG1, "real") &&
                hostmatch(ARG[which]))
                return (1);
        }
    }

    *classbuf = (char) NULL;
    return (0);

}

/*************************************************************************/
/* FUNCTION  : acl_getlimit                                              */
/* PURPOSE   : Scan the ACL buffer and determine what limit applies to   */
/*             the user                                                  */
/* ARGUMENTS : pointer class name, pointer to ACL buffer                 */
/*************************************************************************/

int
#ifdef __STDC__
acl_getlimit(char *class, char *msgpathbuf)
#else
acl_getlimit(class,msgpathbuf)
char *class;
char *msgpathbuf;
#endif
{
    int limit;
    struct aclmember *entry = NULL;

    if (msgpathbuf)
        *msgpathbuf = '\0';

    /* limit <class> <n> <times> [<message_file>] */
    while (getaclentry("limit", &entry)) {
        if (!ARG0 || !ARG1 || !ARG2)
            continue;
        if (!strcmp(class, ARG0)) {
            limit = atoi(ARG1);
            if (validtime(ARG2)) {
                if (ARG3 && msgpathbuf)
                    strcpy(msgpathbuf, ARG3);
                return (limit);
            }
        }
    }
    return (-1);
}

/*************************************************************************/
/* FUNCTION  : acl_deny                                                  */
/* PURPOSE   : Scan the ACL buffer and determine a deny command applies  */
/* ARGUMENTS : pointer class name, pointer to ACL buffer                 */
/*************************************************************************/

int
#ifdef __STDC__
acl_deny(char *msgpathbuf)
#else
acl_deny(msgpathbuf)
char *msgpathbuf;
#endif
{
    struct aclmember *entry = NULL;

    if (msgpathbuf)
        *msgpathbuf = (char) NULL;

    /* deny <addrglob> [<message_file>] */
    while (getaclentry("deny", &entry)) {
        if (!ARG0)
            continue;
        if (!nameserved && !strcmp(ARG0, "!nameserved")) {
            if (ARG1)
                strcpy(msgpathbuf, entry->arg[1]);
            return (1);
        }
        if (hostmatch(ARG0)) {
            if (ARG1)
                strcpy(msgpathbuf, entry->arg[1]);
            return (1);
        }
    }
    return (0);
}

/*************************************************************************/
/* FUNCTION  : acl_countusers                                            */
/* PURPOSE   : Check the anonymous FTP access lists to see if this       */
/*             access is permitted.                                      */
/* ARGUMENTS : none                                                      */
/*************************************************************************/

int
#ifdef __STDC__
acl_countusers(char *class)
#else
acl_countusers(class)
char *class;
#endif
{
    int count,
      which;
    char pidfile[MAXPATHLEN];
    pid_t buf[MAXUSERS];
#ifndef HAVE_FLOCK
struct flock arg;
#endif

    /* 
     * if pidfd was not opened previously... 
     * pidfd must stay open after the chroot(~ftp)  
     */

    sprintf(pidfile, _PATH_PIDNAMES, class);

    if (pidfd < 0) {
        pidfd = open(pidfile, O_RDWR | O_CREAT, 0644);
    }

    if (pidfd < 0) {
        syslog(LOG_ERR, "cannot open pid file %s: %s", pidfile,
               strerror(errno));
        return -1;
    }

#ifdef HAVE_FLOCK
    while (flock(pidfd, LOCK_EX)) {
        syslog(LOG_ERR, "sleeping: flock of pid file failed: %s",
#else 
	arg.l_type = F_WRLCK;
	arg.l_whence = arg.l_start = arg.l_len = 0;
	while ( -1 == fcntl( pidfd, F_SETLK, &arg) ) {
		syslog(LOG_ERR, "sleeping: fcntl lock of pid file failed: %s",
#endif
               strerror(errno));
        sleep(1);
    }
    lseek(pidfd, 0, L_SET);

    count = 0;

    if (read(pidfd, buf, sizeof(buf)) == sizeof(buf)) {
        for (which = 0; which < MAXUSERS; which++)
            if (buf[which] && !kill(buf[which], 0))
                count++;
    }
#ifdef HAVE_FLOCK
    flock(pidfd, LOCK_UN);
#else
	arg.l_type = F_UNLCK; arg.l_whence = arg.l_start = arg.l_len = 0;
	fcntl(pidfd, F_SETLK, &arg);
#endif
    return (count);
}

/*************************************************************************/
/* FUNCTION  : acl_join                                                  */
/* PURPOSE   : Add the current process to the list of processes in the   */
/*             specified class.                                          */
/* ARGUMENTS : The name of the class to join                             */
/*************************************************************************/

void
#ifdef __STDC__
acl_join(char *class)
#else
acl_join(class)
char *class;
#endif
{
    int which,
      avail;
    pid_t buf[MAXUSERS];
    char pidfile[MAXPATHLEN];
    pid_t procid;
#ifndef HAVE_FLOCK
    struct flock arg;
#endif

    /* 
     * if pidfd was not opened previously... 
     * pidfd must stay open after the chroot(~ftp)  
     */

    sprintf(pidfile, _PATH_PIDNAMES, class);

    if (pidfd < 0) {
        pidfd = open(pidfile, O_RDWR | O_CREAT, 0644);
    }

    if (pidfd < 0) {
        syslog(LOG_ERR, "cannot open pid file %s: %s", pidfile,
               strerror(errno));
        return;
    }

#ifdef HAVE_FLOCK
    while (flock(pidfd, LOCK_EX)) {
        syslog(LOG_ERR, "sleeping: flock of pid file failed: %s",
#else 
    arg.l_type = F_WRLCK;
    arg.l_whence = arg.l_start = arg.l_len = 0;
    while ( -1 == fcntl( pidfd, F_SETLK, &arg) ) {
        syslog(LOG_ERR, "sleeping: fcntl lock of pid file failed: %s",
#endif
               strerror(errno));
        sleep(1);
    }

    procid = getpid();

    lseek(pidfd, 0, L_SET);
    if (read(pidfd, buf, sizeof(buf)) < sizeof(buf))
        for (which = 0; which < MAXUSERS; buf[which++] = 0)
            continue;

    avail = 0;
    for (which = 0; which < MAXUSERS; which++) {
        if ((buf[which] == 0) || (kill(buf[which], 0) == -1)) {
            avail = which;
            buf[which] = 0;
        } else if (buf[which] == procid) {
            /* already exists in pid file... */
#ifdef HAVE_FLOCK
            flock(pidfd, LOCK_UN);
#else
            arg.l_type = F_UNLCK; arg.l_whence = arg.l_start = arg.l_len = 0; 
            fcntl(pidfd, F_SETLK, &arg);
#endif
            return;
        }
    }

    buf[avail] = procid;

    lseek(pidfd, 0, L_SET);
    write(pidfd, buf, sizeof(buf));
#ifdef HAVE_FLOCK
    flock(pidfd, LOCK_UN);
#else
    arg.l_type = F_UNLCK; arg.l_whence = arg.l_start = arg.l_len = 0;
    fcntl(pidfd, F_SETLK, &arg);
#endif

}

/*************************************************************************/
/* FUNCTION  : acl_remove                                                */
/* PURPOSE   : remove the current process to the list of processes in    */
/*             the specified class.                                      */
/* ARGUMENTS : The name of the class to remove                           */
/*************************************************************************/

void
#ifdef __STDC__
acl_remove(void)
#else
acl_remove()
#endif
{
    char class[1024];
    int which,
      avail;
    pid_t buf[MAXUSERS];
    char pidfile[MAXPATHLEN];
    pid_t procid;
#ifndef HAVE_FLOCK
    struct flock arg;
#endif

    if (!acl_getclass(class)) {
        return;
    }

    /* 
     * if pidfd was not opened previously... 
     * pidfd must stay open after the chroot(~ftp)  
     */

    sprintf(pidfile, _PATH_PIDNAMES, class);

    if (pidfd < 0) {
        pidfd = open(pidfile, O_RDWR | O_CREAT, 0644);
    }

    if (pidfd < 0) {
        syslog(LOG_ERR, "cannot open pid file %s: %s", pidfile,
               strerror(errno));
        return;
    }

#ifdef HAVE_FLOCK
    while (flock(pidfd, LOCK_EX)) {
        syslog(LOG_ERR, "sleeping: flock of pid file failed: %s",
#else 
    arg.l_type = F_WRLCK;
    arg.l_whence = arg.l_start = arg.l_len = 0;
    while ( -1 == fcntl( pidfd, F_SETLK, &arg) ) {
        syslog(LOG_ERR, "sleeping: fcntl lock of pid file failed: %s",
#endif
               strerror(errno));
        sleep(1);
    }

    procid = getpid();

    lseek(pidfd, 0, L_SET);
    if (read(pidfd, buf, sizeof(buf)) < sizeof(buf))
        for (which = 0; which < MAXUSERS; buf[which++] = 0)
            continue;

    avail = 0;
    for (which = 0; which < MAXUSERS; which++) {
        if ((buf[which] == 0) || (kill(buf[which], 0) == -1)) {
            avail = which;
            buf[which] = 0;
        } else if (buf[which] == procid) {
            buf[which] = 0;
        }
    }

    lseek(pidfd, 0, L_SET);
    write(pidfd, buf, sizeof(buf));
#ifdef HAVE_FLOCK
    flock(pidfd, LOCK_UN);
#else
    arg.l_type = F_UNLCK; arg.l_whence = arg.l_start = arg.l_len = 0;
    fcntl(pidfd, F_SETLK, &arg);
#endif

    close(pidfd);
    pidfd = -1;
}

/*************************************************************************/
/* FUNCTION  : pr_mesg                                                   */
/* PURPOSE   : Display a message to the user                             */
/* ARGUMENTS : message code, name of file to display                     */
/*************************************************************************/

int
#ifdef __STDC__
pr_mesg(int msgcode, char *msgfile)
#else
pr_mesg(msgcode,msgfile)
int msgcode;
char *msgfile;
#endif
{
    FILE *infile;
    char inbuf[1024],
      outbuf[1024],
     *cr;

    if (msgfile && (int)strlen(msgfile) > 0) {
        infile = fopen(msgfile, "r");
        if (infile) {
            while (fgets(inbuf, 255, infile) != NULL) {
                if ((cr = strchr(inbuf, '\n')) != NULL)
                    *cr = '\0';
                msg_massage(inbuf, outbuf);
                lreply(msgcode, "%s", outbuf);
            }
            fclose(infile);
        }
    }
}

/*************************************************************************/
/* FUNCTION  : access_init                                               */
/* PURPOSE   : Read and parse the access lists to set things up          */
/* ARGUMENTS : none                                                      */
/*************************************************************************/

void
#ifdef __STDC__
access_init(void)
#else
access_init()
#endif
{
    struct aclmember *entry;

    if (!readacl(_PATH_FTPACCESS))
        return;
    (void) parseacl();

    Shutdown[0] = '\0';
    entry = (struct aclmember *) NULL;
    if (getaclentry("shutdown", &entry) && ARG0 != NULL)
        (void) strncpy(Shutdown, ARG0, sizeof(Shutdown));

}

/*************************************************************************/
/* FUNCTION  : access_ok                                                 */
/* PURPOSE   : Check the anonymous FTP access lists to see if this       */
/*             access is permitted.                                      */
/* ARGUMENTS : none                                                      */
/*************************************************************************/

int
#ifdef __STDC__
access_ok(int msgcode)
#else
access_ok(msgcode)
int msgcode;
#endif
{
    char class[1024],
      msgfile[MAXPATHLEN];
    int limit;

    if (!use_accessfile)
        return (1);

    if (aclbuf == NULL) {
        syslog(LOG_NOTICE, 
		"ACCESS DENIED (error reading access file) TO %s [%s]",
                 remotehost, remoteaddr);
        return (0);
    }
    if (acl_deny(msgfile)) {
        pr_mesg(msgcode, msgfile);
        syslog(LOG_NOTICE, "ACCESS DENIED (deny command) TO %s [%s]",
               remotehost, remoteaddr);
        return (0);
    }
    /* if user is not in any class, deny access */
    if (!acl_getclass(class)) {
        syslog(LOG_NOTICE, "ACCESS DENIED (not in any class) TO %s [%s]",
               remotehost, remoteaddr);
        return (0);
    }
    /* if no limits defined, no limits apply -- access OK */
    limit = acl_getlimit(class, msgfile);

    if ((limit == -1) || (acl_countusers(class) < limit)) {
        acl_join(class);
        return (1);
    } else {
#ifdef LOG_TOOMANY
        syslog(LOG_NOTICE, "ACCESS DENIED (user limit %d; class %s) TO %s [%s]",
               limit, class, remotehost, remoteaddr);
#endif
        pr_mesg(msgcode, msgfile);
        return (-1);
    }

    /* NOTREACHED */
}

#ident	"@(#)extensions.c	1.9"

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
#include <pwd.h>
#include <setjmp.h>
#include <grp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#if defined(HAVE_STATVFS)
#include <sys/statvfs.h>
#elif defined(HAVE_SYS_VFS)
#include <sys/vfs.h>
#elif defined(HAVE_SYS_MOUNT)
#include <sys/mount.h>
#endif

#include <arpa/ftp.h>

#include "pathnames.h"
#include "extensions.h"

#if defined(HAVE_FTW)
#include <ftw.h>
#else
#include "support/ftw.h"
#endif

#ifdef HAVE_REGEX_H
#include <regex.h>
#endif

#if defined(REGEX) && defined(SVR4) && !(defined(__hpux))
#include <libgen.h>
#endif

#ifdef	INTL
#  include <locale.h>
#  include "ftpd_msg.h"
   extern nl_catd catd;
#else
#  define MSGSTR(num, str)	(str)
#endif	/* INTL */

extern int fnmatch(),
  type,
  transflag,
  ftwflag,
  authenticated,
  autospout_free,
  data,
  pdata,
  anonymous,
  guest;
#ifdef __STDC__
extern char **ftpglob(register char *v),
#else
extern char **ftpglob(),
#endif
 *globerr,
  remotehost[],
  hostname[],
  authuser[],
 *autospout,
  Shutdown[];

char shuttime[30],
  denytime[30],
  disctime[30];

#ifdef __STDC__
extern char *realpath(const char *pathname, char *result);
#else
extern char *realpath();
#endif

#ifndef REGEX
char *re_comp();
#elif defined(M_UNIX)
extern char *regcmp(), *regex();
#endif

#ifdef __STDC__
extern FILE *dataconn(char *name, off_t size, char *mode);
#else
extern FILE *dataconn();
#endif
FILE *dout;

time_t newer_time;

int show_fullinfo;

int
#ifdef __STDC__
check_newer(char *path, struct stat *st, int flag)
#else
check_newer(path,st,flag)
char *path;
struct stat *st;
int flag;
#endif
{

    if (st->st_mtime > newer_time) {
        if (show_fullinfo != 0) {
            if (flag == FTW_F || flag == FTW_D) {
                fprintf(dout, "%s %d %d %s\n", flag == FTW_F ? "F" : "D",
                        st->st_size, st->st_mtime, path);
            }
        } else if (flag == FTW_F)
            fprintf(dout, "%s\n", path);
    }
    /* When an ABOR has been received (which sets ftwflag > 1) return a
     * non-zero value which causes ftw to stop tree traversal and return. */
    return (ftwflag > 1 ? 1 : 0);
}

#if defined(HAVE_STATVFS)
int getSize(s)
char *s;
{
    int c;
    struct statvfs buf;

    if (( c = statvfs(s, &buf)) != 0)
        return(0);

    return(buf.f_bavail * buf.f_frsize / 1024);
}
#elif defined(HAVE_SYS_VFS) || defined (HAVE_SYS_MOUNT)
int getSize(s)
char *s;
{
    int c;
    struct statfs buf;

    if (( c = statfs(s, &buf)) != 0)
        return(0);

    return(buf.f_bavail * buf.f_bsize / 1024);
}
#endif

/*************************************************************************/
/* FUNCTION  : msg_massage                                               */
/* PURPOSE   : Scan a message line for magic cookies, replacing them as  */
/*             needed.                                                   */
/* ARGUMENTS : pointer input and output buffers                          */
/*************************************************************************/

int
#ifdef __STDC__
msg_massage(char *inbuf, char *outbuf)
#else
msg_massage(inbuf,outbuf)
char *inbuf;
char *outbuf;
#endif
{
    char *inptr = inbuf;
    char *outptr = outbuf;
    char buffer[MAXPATHLEN];
    time_t curtime;
    int limit;
    extern struct passwd *pw;
    struct aclmember *entry;

    (void) time(&curtime);
    (void) acl_getclass(buffer);

    limit = acl_getlimit(buffer, NULL);

    while (*inptr) {
        if (*inptr != '%')
            *outptr++ = *inptr;
        else {
            entry = NULL;
            switch (*++inptr) {
            case 'E':
                if ( (getaclentry("email", &entry)) && ARG0 )
                    sprintf(outptr, "%s", ARG0); 
                else
                    *outptr = '\0';
                break;
            case 'N': 
                sprintf(outptr, "%d", acl_countusers(buffer)); 
                break; 
            case 'M':
                sprintf(outptr, "%d", limit);
                break;

            case 'T':
                strncpy(outptr, ctime(&curtime), 24);
                *(outptr + 24) = '\0';
                break;

            case 'F':
#if defined(HAVE_STATVFS) || defined(HAVE_SYS_VFS) || defined(HAVE_SYS_MOUNT)
                sprintf(outptr, "%lu", getSize("."));
#endif
                break;

            case 'C':
#ifdef HAVE_GETCWD
                (void) getcwd(outptr, MAXPATHLEN);
#else
                (void) getwd(outptr);
#endif
                break;

            case 'R':
                strcpy(outptr, remotehost);
                break;

            case 'L':
                strcpy(outptr, hostname);
                break;

            case 'U':
                if (pw)
		  strcpy(outptr, pw->pw_name);
		else
		  strcpy(outptr, "[unknown]");
                break;

            case 's':
                strncpy(outptr, shuttime, 24);
                *(outptr + 24) = '\0';
                break;

            case 'd':
                strncpy(outptr, disctime, 24);
                *(outptr + 24) = '\0';
                break;

            case 'r':
                strncpy(outptr, denytime, 24);
                *(outptr + 24) = '\0';
                break;

/* KH : cookie %u for RFC931 name */
            case 'u':
                if (authenticated) strncpy(outptr, authuser, 24);
                else strcpy(outptr,"[unknown]");
                *(outptr + 24) = '\0'; 
                break;

            case '%':
                *outptr++ = '%';
                *outptr = '\0';
                break;

            default:
                *outptr++ = '%';
                *outptr++ = '?';
                *outptr = '\0';
                break;
            }
            while (*outptr)
                outptr++;
        }
        inptr++;
    }
    *outptr = '\0';
}

/*************************************************************************/
/* FUNCTION  : cwd_beenhere                                              */
/* PURPOSE   : Return 1 if the user has already visited this directory   */
/*             via C_WD.                                                 */
/* ARGUMENTS : a power-of-two directory function code (README, MESSAGE)  */
/*************************************************************************/

int
#ifdef __STDC__
cwd_beenhere(int dircode)
#else
cwd_beenhere(dircode)
int dircode;
#endif
{
    struct dirlist {
        struct dirlist *next;
        int dircode;
        char dirname[1];
    };

    static struct dirlist *head = NULL;
    struct dirlist *curptr;
    char cwd[MAXPATHLEN];

    (void) realpath(".", cwd);

    for (curptr = head; curptr != NULL; curptr = curptr->next)
        if (strcmp(curptr->dirname, cwd) == 0) {
            if (!(curptr->dircode & dircode)) {
                curptr->dircode |= dircode;
                return (0);
            }
            return (1);
        }
    curptr = (struct dirlist *) malloc(strlen(cwd) + 1 + sizeof(struct dirlist));

    if (curptr != NULL) {
        curptr->next = head;
        head = curptr;
        curptr->dircode = dircode;
        strcpy(curptr->dirname, cwd);
    }
    return (0);
}

/*************************************************************************/
/* FUNCTION  : show_banner                                               */
/* PURPOSE   : Display a banner on the user's terminal before login      */
/* ARGUMENTS : reply code to use                                         */
/*************************************************************************/
 
void
#ifdef __STDC__
show_banner(int msgcode)
#else
show_banner(msgcode)
int msgcode;
#endif
{
    char *crptr,
      linebuf[1024],
      outbuf[1024];
    struct aclmember *entry = NULL;
    FILE *infile;

#ifdef VIRTUAL
    extern int virtual_mode;
    extern char virtual_banner[];

    if (virtual_mode) {
        infile = fopen(virtual_banner, "r");
	if (infile) {
 	    while (fgets(linebuf, 255, infile) != NULL) {
	           if ((crptr = strchr(linebuf, '\n')) != NULL)
		        *crptr = '\0';
		   msg_massage(linebuf, outbuf);
		   lreply(msgcode, "%s", outbuf);
		 }
	    fclose(infile);
	    lreply(msgcode, "");
	  }
      }
    else {
#endif
      /* banner <path> */
      while (getaclentry("banner", &entry)) {
	    infile = fopen(ARG0, "r");
	    if (infile) {
	        while (fgets(linebuf, 255, infile) != NULL) {
		  if ((crptr = strchr(linebuf, '\n')) != NULL)
		    *crptr = '\0';
		  msg_massage(linebuf, outbuf);
		  lreply(msgcode, "%s", outbuf);
		}
		fclose(infile);
		lreply(msgcode, "");
	    }
	 }
#ifdef VIRTUAL
    }
#endif
  }
/*************************************************************************/
/* FUNCTION  : show_message                                              */
/* PURPOSE   : Display a message on the user's terminal if the current   */
/*             conditions are right                                      */
/* ARGUMENTS : reply code to use, LOG_IN|CMD                             */
/*************************************************************************/

void
#ifdef __STDC__
show_message(int msgcode, int mode)
#else
show_message(msgcode,mode)
int msgcode;
int mode;
#endif
{
    char *crptr,
      linebuf[1024],
      outbuf[1024],
      class[MAXPATHLEN],
      cwd[MAXPATHLEN];
    int show,
      which;
    struct aclmember *entry = NULL;
    FILE *infile;

    if (cwd_beenhere(1) != 0)
        return;

#ifdef HAVE_GETCWD
    (void) getcwd(cwd,MAXPATHLEN-1);
#else
    (void) getwd(cwd);
#endif
    (void) acl_getclass(class);

    /* message <path> [<when> [<class>]] */
    while (getaclentry("message", &entry)) {
        if (!ARG0)
            continue;
        show = 0;

        if (mode == LOG_IN && (!ARG1 || !strcasecmp(ARG1, "login")))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == 0)
                        show++;
            }
        if (mode == C_WD && ARG1 && !strncasecmp(ARG1, "cwd=", 4) &&
            (!strcmp((ARG1) + 4, cwd) || *(ARG1 + 4) == '*' ||
            !fnmatch((ARG1) + 4, cwd, FNM_PATHNAME)))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == 0)
                        show++;
            }
        if (show && (int)strlen(ARG0) > 0) {
            infile = fopen(ARG0, "r");
            if (infile) {
                while (fgets(linebuf, 255, infile) != NULL) {
                    if ((crptr = strchr(linebuf, '\n')) != NULL)
                        *crptr = '\0';
                    msg_massage(linebuf, outbuf);
                    lreply(msgcode, "%s", outbuf);
                }
                fclose(infile);
                lreply(msgcode, "");
            }
        }
    }
}

/*************************************************************************/
/* FUNCTION  : show_readme                                               */
/* PURPOSE   : Display a message about a README file to the user if the  */
/*             current conditions are right                              */
/* ARGUMENTS : pointer to ACL buffer, reply code, LOG_IN|C_WD            */
/*************************************************************************/

void
#ifdef __STDC__
show_readme(int code, int mode)
#else
show_readme(code,mode)
int code;
int mode;
#endif
{
    char **filelist,
      **sfilelist,
      class[MAXPATHLEN],
      cwd[MAXPATHLEN];
    int show,
      which,
      days;
    time_t clock;

    struct stat buf;
    struct tm *tp;
    struct aclmember *entry = NULL;

    if (cwd_beenhere(2) != 0)
        return;

#ifdef HAVE_GETCWD
    (void) getcwd(cwd,MAXPATHLEN-1);
#else
    (void) getwd(cwd);
#endif
    (void) acl_getclass(class);

    /* readme  <path> {<when>} */
    while (getaclentry("readme", &entry)) {
        if (!ARG0)
            continue;
        show = 0;

        if (mode == LOG_IN && (!ARG1 || !strcasecmp(ARG1, "login")))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == 0)
                        show++;
            }
        if (mode == C_WD && ARG1 && !strncasecmp(ARG1, "cwd=", 4)
            && (!strcmp((ARG1) + 4, cwd) || *(ARG1 + 4) == '*' ||
                !fnmatch((ARG1) + 4, cwd, FNM_PATHNAME)))
            if (!ARG2)
                show++;
            else {
                for (which = 2; (which < MAXARGS) && ARG[which]; which++)
                    if (strcasecmp(class, ARG[which]) == 0)
                        show++;
            }
        if (show) {
            globerr = NULL;
            filelist = ftpglob(ARG0);
            sfilelist = filelist;  /* save to free later */
            if (!globerr) {
                while (filelist && *filelist) {
                   errno = 0;
                   if (!stat(*filelist, &buf) &&
                       (buf.st_mode & S_IFMT) == S_IFREG) {
                       lreply(code, MSGSTR(MSG_PLEASE_READ,
                                "Please read the file %s"), *filelist);
                       (void) time(&clock);
                       tp = localtime(&clock);
                       days = 365 * tp->tm_year + tp->tm_yday;
                       tp = localtime((time_t *)&buf.st_mtime);
                       days -= 365 * tp->tm_year + tp->tm_yday;
/*
                       if (days == 0) {
                         lreply(code, "  it was last modified on %.24s - Today",
                           ctime((time_t *)&buf.st_mtime));
                       } else {
*/
                         if (days == 1)
                             lreply(code, MSGSTR(MSG_DAY_AGO,
                               "  it was last modified on %.24s - 1 day ago"),
                               ctime((time_t *)&buf.st_mtime));
                         else
                             lreply(code, MSGSTR(MSG_DAYS_AGO,
                              "  it was last modified on %.24s - %d days ago"),
                               ctime((time_t *)&buf.st_mtime), days);
/*
                       }
*/
                   }
                   filelist++;
                }
            }
            if (sfilelist) {
                blkfree(sfilelist);
                free((char *) sfilelist);
            }
        }
    }
}

/*************************************************************************/
/* FUNCTION  : deny_badxfertype                                          */
/* PURPOSE   : If user is in ASCII transfer mode and tries to retrieve a */
/*             binary file, abort transfer and display appropriate error */
/* ARGUMENTS : message code to use for denial, path of file to check for */
/*             binary contents or NULL to assume binary file             */
/*************************************************************************/

int
#ifdef __STDC__
deny_badasciixfer(int msgcode, char *filepath)
#else
deny_badasciixfer(msgcode,filepath)
int msgcode;
char *filepath;
#endif
{

    if (type == TYPE_A && !*filepath) {
        reply(msgcode, MSGSTR(MSG_BINARY_FILE,
"This is a BINARY file, using ASCII mode to transfer will corrupt it."));
        return (1);
    }
    /* The hooks are here to prevent transfers of actual binary files, not
     * just TAR or COMPRESS mode files... */
    return (0);
}

/*************************************************************************/
/* FUNCTION  : is_shutdown                                               */
/* PURPOSE   :                                                           */
/* ARGUMENTS :                                                           */
/*************************************************************************/

int
#ifdef __STDC__
is_shutdown(int quiet, int new)
#else
is_shutdown(quiet, new)
int quiet;
int new;
#endif
{
    static struct tm tmbuf;
    static struct stat s_last;
    static time_t last = 0,
      shut,
      deny,
      disc;
    static int valid;
    static char text[2048];

    struct stat s_cur;

    FILE *fp;

    int deny_off,
      disc_off;

    time_t curtime = time(NULL);

    char buf[1024],
      linebuf[1024];

    if (Shutdown[0] == '\0' || stat(Shutdown, &s_cur))
        return (0);

    if (s_last.st_mtime != s_cur.st_mtime) {
        s_last = s_cur;
        valid = 0;

        fp = fopen(Shutdown, "r");
        if (fp == NULL)
            return (0);
        fgets(buf, sizeof(buf), fp);
        if (sscanf(buf, "%d %d %d %d %d %d %d", &tmbuf.tm_year, &tmbuf.tm_mon,
        &tmbuf.tm_mday, &tmbuf.tm_hour, &tmbuf.tm_min, &deny, &disc) != 7) {
            (void) fclose(fp);
            return (0);
        }
        valid = 1;
        deny_off = 3600 * (deny / 100) + 60 * (deny % 100);
        disc_off = 3600 * (disc / 100) + 60 * (disc % 100);

        tmbuf.tm_year -= 1900;
        tmbuf.tm_isdst = -1;
        shut = mktime(&tmbuf);
        strcpy(shuttime, ctime(&shut));

        disc = shut - disc_off;
        strcpy(disctime, ctime(&disc));

        deny = shut - deny_off;
        strcpy(denytime, ctime(&deny));

        text[0] = '\0';

        while (fgets(buf, sizeof(buf), fp) != NULL) {
            msg_massage(buf, linebuf);
            if ((strlen(text) + strlen(linebuf)) < sizeof(text))
                strcat(text, linebuf);
        }

        (void) fclose(fp);
    }
    if (!valid)
        return (0);

    /* if last == 0, then is_shutdown() only called with quiet == 1 so far */
    if (last == 0 && !quiet) {
        autospout = text;       /* warn them for the first time */
        autospout_free = 0;
        last = curtime;
    }
    /* if a new connection and past deny time, tell caller to drop 'em */
    if (new && curtime > deny)
        return (1);

    /* if past disconnect time, tell caller to drop 'em */
    if (curtime > disc)
        return (1);

    /* if less than 60 seconds to disconnection, warn 'em continuously */
    if (curtime > (disc - 60) && !quiet) {
        autospout = text;
        autospout_free = 0;
        last = curtime;
    }
    /* if less than 15 minutes to disconnection, warn 'em every 5 mins */
    if (curtime > (disc - 60 * 15)) {
        if ((curtime - last) > (60 * 5) && !quiet) {
            autospout = text;
            autospout_free = 0;
            last = curtime;
        }
    }
    /* if less than 24 hours to disconnection, warn 'em every 30 mins */
    if (curtime < (disc - 24 * 60 * 60) && !quiet) {
        if ((curtime - last) > (60 * 30)) {
            autospout = text;
            autospout_free = 0;
            last = curtime;
        }
    }
    /* if more than 24 hours to disconnection, warn 'em every 60 mins */
    if (curtime > (disc - 24 * 60 * 60) && !quiet) {
        if ((curtime - last) >= (24 * 60 * 60)) {
            autospout = text;
            autospout_free = 0;
            last = curtime;
        }
    }
    return (0);
}

void
#ifdef __STDC__
newer(char *date, char *path, int showlots)
#else
newer(date,path,showlots)
char *date;
char *path;
int showlots;
#endif
{
    struct tm tm;

    if (sscanf(date, "%04d%02d%02d%02d%02d%02d",
               &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
               &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {

        tm.tm_year -= 1900;
        tm.tm_mon--;
        tm.tm_isdst = -1;
        newer_time = mktime(&tm);
        dout = dataconn(MSGSTR(MSG_FILE_LIST, "file list"), (off_t) - 1, "w");

        if (dout != NULL) {
            /* As ftw allocates storage it needs a chance to cleanup, setting
             * ftwflag prevents myoob from calling longjmp, incrementing
             * ftwflag instead which causes check_newer to return non-zero
             * which makes ftw return. */
            ftwflag = 1;
            transflag++;
            show_fullinfo = showlots;
#if defined(HAVE_FTW)
            ftw(path, check_newer, -1);
#else
            treewalk(path, check_newer, -1, NULL);
#endif

            /* don't send a reply if myoob has already replied */
            if (ftwflag == 1) {
                if (ferror(dout) != 0)
                    perror_reply(550, MSGSTR(MSG_DATA_CONN, "Data connection"));
                else
                    reply(226, MSGSTR(MSG_TRANSFER_COMP, "Transfer complete."));
            }

            (void) fclose(dout);
            data = -1;
            pdata = -1;
            transflag = 0;
            ftwflag = 0;
        }
    } else
        reply(501, MSGSTR(MSG_BAD_DATE, "Bad DATE format"));
}

int
#ifdef __STDC__
type_match(char *typelist)
#else
type_match(typelist)
char *typelist;
#endif
{
    if (anonymous && strcasestr(typelist, "anonymous"))
        return (1);
    if (guest && strcasestr(typelist, "guest"))
        return (1);
    if (!guest && !anonymous && strcasestr(typelist, "real"))
        return (1);

    return (0);
}

int
#ifdef __STDC__
path_compare(char *p1, char *p2)
#else
path_compare(p1,p2)
char *p1;
char *p2;
#endif
{
    if ( fnmatch(p1, p2, NULL) == 0 ) /* 0 means they matched */
        return(strlen(p1));
    else
        return(-2);
}

void
#ifdef __STDC__
expand_id(void)
#else
expand_id()
#endif
{
    struct aclmember *entry = NULL;
    struct passwd *pwent;
    struct group *grent;
    char buf[BUFSIZ];

    while (getaclentry("upload", &entry) && ARG0 && ARG1 && ARG2 != NULL) {
        if (ARG3 && ARG4) {
            pwent = getpwnam(ARG3);
            grent = getgrnam(ARG4);

            if (pwent)  sprintf(buf, "%d", pwent->pw_uid);
            else        sprintf(buf, "%d", 0);
            ARG3 = (char *) malloc(strlen(buf) + 1);
            strcpy(ARG3, buf);

            if (grent)  sprintf(buf, "%d", grent->gr_gid);
            else        sprintf(buf, "%d", 0);
            ARG4 = (char *) malloc(strlen(buf) + 1);
            strcpy(ARG4, buf);
	    endgrent();
        }
    }
}

int
#ifdef __STDC__
fn_check(char *name)
#else
fn_check(name)
char *name;
#endif
{
  /* check to see if this is a valid file name... path-filter <type>
   * <message_file> <allowed_charset> <disallowed> */

  struct aclmember *entry = NULL;
  int   j;
  char *sp;
  char *path;
#ifdef M_UNIX
# ifdef REGEX
  char *regp;
# endif
#endif

#ifdef REGEXEC
  regex_t regexbuf;
  regmatch_t regmatchbuf;
#endif

  while (getaclentry("path-filter", &entry) && ARG0 != NULL) {
      if (type_match(ARG0) && ARG1 && ARG2) {

		  /*
		   * check *only* the basename
		   */

		  if (path = strrchr(name, '/'))  ++path;
		  else	path = name;

          /* is it in the allowed character set? */
#if defined(REGEXEC)
          if (regcomp(&regexbuf, ARG2, REG_EXTENDED) != 0) {
              reply(553, MSGSTR(MSG_REGEX_ERROR, "REGEX error"));
#elif defined(REGEX)
          if ((sp = regcmp(ARG2, (char *) 0)) == NULL) {
              reply(553, MSGSTR(MSG_REGEX_ERROR, "REGEX error"));
#else
          if ((sp = re_comp(ARG2)) != 0) {
              perror_reply(553, sp);
#endif
              return(0);
          }
#if defined(REGEXEC)
          if (regexec(&regexbuf, path, 1, &regmatchbuf, 0) != 0) {
#elif defined(REGEX)
# ifdef M_UNIX
          regp = regex(sp, path);
          free(sp);
          if (regp == NULL) {
# else
          if ((regex(sp, path)) == NULL) {
# endif
#else
          if ((re_exec(path)) != 1) {
#endif
              pr_mesg(553, ARG1);
              reply(553, MSGSTR(MSG_PERM_DENIED_ACC,
                        "%s: Permission denied. (Filename (accept))"), name);
              return(0);
          }
          /* is it in any of the disallowed regexps */

          for (j = 3; j < MAXARGS; ++j) {
              /* ARGj == entry->arg[j] */
              if (entry->arg[j]) {
#if defined(REGEXEC)
                  if (regcomp(&regexbuf, entry->arg[j], REG_EXTENDED) != 0) {
                      reply(553, MSGSTR(MSG_REGEX_ERROR, "REGEX error"));
#elif defined(REGEX)
                  if ((sp = regcmp(entry->arg[j], (char *) 0)) == NULL) {
                      reply(553, MSGSTR(MSG_REGEX_ERROR, "REGEX error"));
#else
                  if ((sp = re_comp(entry->arg[j])) != 0) {
                      perror_reply(553, sp);
#endif
                      return(0);
                  }
#if defined(REGEXEC)
                  if (regexec(&regexbuf, path, 1, &regmatchbuf, 0) == 0) {
#elif defined(REGEX)
# ifdef M_UNIX
                  regp = regex(sp, path);
                  free(sp);
                  if (regp != NULL) {
# else
                  if ((regex(sp, path)) != NULL) {
# endif
#else
                  if ((re_exec(path)) == 1) {
#endif
                      pr_mesg(553, ARG1);
                      reply(553, MSGSTR(MSG_PERM_DENIED_DEN,
                        "%s: Permission denied. (Filename (deny))"), name);
                      return(0);
                  }
              }
          }
      }
  }
  return(1);
}

int
#ifdef __STDC__
dir_check(char *name, uid_t *uid, gid_t *gid, int *valid)
#else
dir_check(name,uid,gid,valid)
char *name;
uid_t *uid;
gid_t *gid;
int *valid;
#endif
{
  struct aclmember *entry = NULL;

  int i,
    match_value = -1;
  char *ap2 = NULL,
       *ap3 = NULL,
       *ap4 = NULL,
       *ap6 = NULL;
  char cwdir[BUFSIZ];
  char path[BUFSIZ];
  char *sp;
  extern struct passwd *pw;

  *valid = 0;
  /* what's our current directory? */

  strcpy(path, name);
  if (sp = strrchr(path, '/'))  *sp = '\0';
  else strcpy(path, ".");

  if ((realpath(path, cwdir)) == NULL) {
    perror_reply(553, MSGSTR(MSG_CWDIR, "Could not determine cwdir"));
    return(-1);
  }
  while (getaclentry("upload", &entry) && ARG0 && ARG1 && ARG2 != NULL) {
      if ( (!strcmp(ARG0, pw->pw_dir)) &&
           ((i = path_compare(ARG1, cwdir)) >= match_value) ) {
          match_value = i;
          ap2 = ARG2;
          if (ARG3)  ap3 = ARG3;
          else       ap3 = NULL;
          if (ARG4)  ap4 = ARG4;
          else       ap4 = NULL;
          if (ARG6)  ap6 = ARG6;
          else       ap6 = NULL;
      }
  }
  if ( (ap3 && !strcasecmp(ap3, "nodirs")) ||
       (ap6 && !strcasecmp(ap6, "nodirs")) ) {
      reply(530, MSGSTR(MSG_PERM_DENIED_UPLOAD_DIRS,
            "%s: Permission denied. (Upload dirs)"), name);
      return(0);
  }
  if (ap3)
     *uid = atoi(ap3);    /* the uid  */
  if (ap4) {
     *gid = atoi(ap4);    /* the gid */
     *valid = 1;
   }
  return(1);
}

int
#ifdef __STDC__
upl_check(char *name, uid_t *uid, gid_t *gid, int *f_mode, int *valid)
#else
upl_check(name,uid,gid,f_mode,valid)
char *name;
uid_t *uid;
gid_t *gid;
int *f_mode;
int *valid;
#endif
{
  int  match_value = -1;
  char cwdir[BUFSIZ];
  char path[BUFSIZ];
  char *sp;
  int  i;

  char *ap1 = NULL,
   *ap2 = NULL,
   *ap3 = NULL,
   *ap4 = NULL,
   *ap5 = NULL;

  struct aclmember *entry = NULL;
  extern struct passwd *pw;

  *valid = 0;

      /* what's our current directory? */

      strcpy(path, name);
      if (sp = strrchr(path, '/'))  *sp = '\0';
      else strcpy(path, ".");

      if ((realpath(path, cwdir)) == NULL) {
          perror_reply(553, MSGSTR(MSG_CWDIR, "Could not determine cwdir"));
          return(-1);
      }

      /* we are doing a "best match"... ..so we keep track of what "match
       * value" we have received so far... */

      while (getaclentry("upload", &entry) && ARG0 && ARG1 && ARG2 != NULL) {
          if ( (!strcmp(ARG0, pw->pw_dir)) &&
		       ((i = path_compare(ARG1, cwdir)) >= match_value) ) {
              match_value = i;
              ap1 = ARG1;
              ap2 = ARG2;
              if (ARG3) ap3 = ARG3;
              else      ap3 = NULL;
              if (ARG4) ap4 = ARG4;
              else      ap4 = NULL;
              if (ARG5) ap5 = ARG5;
              else      ap5 = NULL;
          }
	}

      if (ap3 && ( (!strcasecmp("dirs",ap3)) || (!strcasecmp("nodirs", ap3)) ))
        ap3 = NULL;

      /* if we did get matches... ..else don't do any of this stuff */
      if (match_value >= 0) {
          if (!strcasecmp(ap2, "yes")) {
              if (ap3)
                  *uid = atoi(ap3);    /* the uid  */
              if (ap4) {
                  *gid = atoi(ap4);    /* the gid  */
		  *valid = 1;
		}
              if (ap5)
                  sscanf(ap5, "%o", f_mode); /* the mode */
          } else {
              reply(553, MSGSTR(MSG_PERM_DENIED_UPLOAD,
                    "%s: Permission denied. (Upload)"), name);
              return(-1);
          }
      } else {
          /*
           * upload defaults to "permitted"
           */
          return(1);
      }

  return(match_value);
}

int
#ifdef __STDC_
del_check(char *name)
#else
del_check(name)
char *name;
#endif
{
  int pdelete = 1;
  struct aclmember *entry = NULL;

  while (getaclentry("delete", &entry) && ARG0 && ARG1 != NULL) {
      if (type_match(ARG1))
          if (*ARG0 == 'n')
              pdelete = 0;
  }
  
/* H* fix: no deletion, period. You put a file here, I get to look at it. */
#ifdef PARANOID
  pdelete = 0;
#endif

  if (!pdelete) {
      reply(553, MSGSTR(MSG_PERM_DENIED_DELETE,
            "%s: Permission denied. (Delete)"), name);
      return(0);
  } else {
      return(1);
  }
}

/* The following is from the Debian add-ons. */

#define lbasename(x) (strrchr(x,'/')?1+strrchr(x,'/'):x)

int
#ifdef __STDC__
checknoretrieve (char *name)
#else
checknoretrieve (name)
char *name;
#endif
{
  char cwd[MAXPATHLEN+1], realwd[MAXPATHLEN+1], realname[MAXPATHLEN+1];
  int i;
  struct aclmember *entry = NULL;

  if (name == (char *)NULL || *name == '\0')
    return 0; 

#ifdef HAVE_GETCWD
  (void)getcwd (cwd, MAXPATHLEN);
#else
  (void)getwd (cwd);
#endif

  realpath (cwd, realwd);
  realpath (name, realname);

   while (getaclentry("noretrieve", &entry)) {
        if (ARG0 == (char *)NULL)
            continue;
	for (i = 0; i< MAXARGS && 
	     (entry->arg[i] != (char *)NULL) && (*(entry->arg[i]) !='\0'); i++)
	  if (strcmp (((*(entry->arg[i]) == '/') ? realname : 
			lbasename (realname)), entry->arg[i]) == 0)
	  {
	    reply (550, MSGSTR(MSG_UNRETRIEVABLE,
             "%s is marked unretrievable"), entry->arg[i]);
	    return 1;
	  }
      }
   return 0;
}

#ifdef	INTL
#define MAXLANGLEN      NL_LANGMAX	/* defined in limits.h */
site_lang(char *lang)
{
	static char	envstr[MAXLANGLEN + 5];

	strcpy(envstr, "LANG=");
	if (strlen(lang) < MAXLANGLEN) {
		strcat(envstr, lang);
		if (setlocale(LC_ALL, lang) != (char *)0) {
			(void) putenv(envstr);
			(void) catclose(catd);
			catd = catopen(MF_FTPD, MC_FLAGS);
			reply(200, MSGSTR(MSG_LOCALE_SET_TO,
					  "Locale set to %s."), lang);
		}
		else
			reply(550, MSGSTR(MSG_SETLOCALE_FAILED,
				"setlocale '%s' failed."), lang);
	} else
		reply(500, MSGSTR(MSG_ILLEGAL_LANG, "Illegal lang name"));
}
#endif	/* INTL */
